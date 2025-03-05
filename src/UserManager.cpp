#include "UserManager.hpp"
#include "Encryption.hpp"
#include "Constants.hpp"
#include <fstream>
#include <filesystem>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>

std::vector<User> UserManager::users;
bool UserManager::initialized = false;

bool UserManager::initialize() {
    if (!initialized) {
        try {
            initialized = true;
            return isFirstBoot() || loadUsers();
        }
        catch (...) {
            initialized = false;
            return false;
        }
    }
    return true;
}

bool UserManager::isFirstBoot() {
    return !std::filesystem::exists(getUserFilePath());
}

bool UserManager::createRootUser(const std::string& password) {
    if (!users.empty()) return false;
    
    User root;
    root.username = "root";
    root.passwordHash = hashPassword(password);
    root.isRoot = true;
    
    users.push_back(root);
    return saveUsers();
}

bool UserManager::authenticate(const std::string& password) {
    if (users.empty()) return false;
    
    std::string hash = hashPassword(password);
    for (const auto& user : users) {
        if (user.isRoot && user.passwordHash == hash) {
            Encryption::setMasterPassword(password);
            return true;
        }
    }
    return false;
}

std::string UserManager::hashPassword(const std::string& password) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create hash context");
    }
    
    try {
        if (1 != EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) ||
            1 != EVP_DigestUpdate(ctx, password.c_str(), password.length()) ||
            1 != EVP_DigestFinal_ex(ctx, hash, &hashLen)) {
            throw std::runtime_error("Failed to compute hash");
        }
        
        EVP_MD_CTX_free(ctx);
        
        std::stringstream ss;
        for (unsigned int i = 0; i < hashLen; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return ss.str();
    }
    catch (...) {
        EVP_MD_CTX_free(ctx);
        throw;
    }
}

bool UserManager::saveUsers() {
    try {
        std::stringstream ss;
        for (const auto& user : users) {
            ss << user.username << "|" << user.passwordHash << "|" << (user.isRoot ? "1" : "0") << "\n";
        }
        
        std::string encrypted = Encryption::encrypt(ss.str());
        std::ofstream file(getUserFilePath(), std::ios::binary);
        file.write(encrypted.c_str(), encrypted.length());
        return true;
    }
    catch (...) {
        return false;
    }
}

bool UserManager::loadUsers() {
    if (!std::filesystem::exists(getUserFilePath())) {
        return true;  // No users file is valid for first boot
    }
    
    try {
        std::ifstream file(getUserFilePath(), std::ios::binary);
        if (!file.is_open()) return false;
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        
        if (buffer.str().empty()) {
            return true;  // Empty file is valid for first boot
        }
        
        std::string decrypted = Encryption::decrypt(buffer.str());
        std::stringstream ss(decrypted);
        std::string line;
        
        users.clear();
        while (std::getline(ss, line)) {
            size_t pos1 = line.find('|');
            size_t pos2 = line.find('|', pos1 + 1);
            if (pos1 != std::string::npos && pos2 != std::string::npos) {
                User user;
                user.username = line.substr(0, pos1);
                user.passwordHash = line.substr(pos1 + 1, pos2 - pos1 - 1);
                user.isRoot = (line.substr(pos2 + 1) == "1");
                users.push_back(user);
            }
        }
        return true;
    }
    catch (...) {
        return false;
    }
}

std::string UserManager::getUserFilePath() {
    return (std::filesystem::current_path() / "users.dat").string();
}
