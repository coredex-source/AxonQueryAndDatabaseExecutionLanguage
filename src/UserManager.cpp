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
            users.clear();  // Clear any existing users
            
            if (isFirstBoot()) {
                // Set default encryption key for first boot
                Encryption::setMasterPassword(ENCRYPTION_KEY);
                return true;
            }

            // Try loading with default key first
            Encryption::setMasterPassword(ENCRYPTION_KEY);
            if (loadUsers()) {
                return true;
            }

            // If that fails, we'll let authenticate() try with the user's password
            return true;
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
    
    // Set the master password before encrypting user data
    Encryption::setMasterPassword(ENCRYPTION_KEY);
    
    User root;
    root.username = "root";
    root.passwordHash = hashPassword(password);
    root.isRoot = true;
    
    users.push_back(root);
    bool success = saveUsers();
    
    // Set the actual user password for future operations
    if (success) {
        Encryption::setMasterPassword(password);
    }
    
    return success;
}

bool UserManager::authenticate(const std::string& password) {
    // Try loading users with the provided password
    Encryption::setMasterPassword(password);
    
    try {
        if (!loadUsers()) {
            return false;
        }
    }
    catch (...) {
        return false;
    }
    
    std::string hash = hashPassword(password);
    for (const auto& user : users) {
        if (user.isRoot && user.passwordHash == hash) {
            // Set master password after successful authentication
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
            ss << user.username << "|" << user.passwordHash << "|" << (user.isRoot ? "1" : "0");
            // Add database keys
            ss << "|" << user.dbKeys.size();
            for (const auto& [dbName, key] : user.dbKeys) {
                ss << "|" << dbName << "|" << key;
            }
            ss << "\n";
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
    try {
        if (!std::filesystem::exists(getUserFilePath())) {
            return true;  // First boot case
        }
        
        std::ifstream file(getUserFilePath(), std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open users file");
        }
        
        // Read file content
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        if (content.empty()) {
            return true;  // Empty file is valid for first boot
        }
        
        std::string decrypted;
        try {
            decrypted = Encryption::decrypt(content);
        } catch (const std::runtime_error& e) {
            // Log error or handle specific decryption failures
            return false;
        }
        
        if (decrypted.empty()) {
            return true;
        }
        
        users.clear();
        std::istringstream ss(decrypted);
        std::string line;
        
        while (std::getline(ss, line)) {
            if (line.empty()) continue;
            
            std::vector<std::string> parts;
            std::string part;
            std::istringstream lineStream(line);
            while (std::getline(lineStream, part, '|')) {
                parts.push_back(part);
            }

            if (parts.size() >= 4) {
                User user;
                user.username = parts[0];
                user.passwordHash = parts[1];
                user.isRoot = (parts[2] == "1");
                
                // Load database keys
                int keyCount = std::stoi(parts[3]);
                for (int i = 0; i < keyCount && (4 + i*2 + 1) < parts.size(); i++) {
                    std::string dbName = parts[4 + i*2];
                    std::string dbKey = parts[4 + i*2 + 1];
                    user.dbKeys[dbName] = dbKey;
                }
                
                users.push_back(user);
            }
        }
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::string UserManager::getUserFilePath() {
    return (std::filesystem::current_path() / "users.dat").string();
}

bool UserManager::addDatabaseKey(const std::string& dbName, const std::string& key) {
    if (users.empty()) return false;
    
    // Add key to root user
    for (auto& user : users) {
        if (user.isRoot) {
            user.dbKeys[dbName] = key;
            return saveUsers();
        }
    }
    return false;
}

bool UserManager::getDatabaseKey(const std::string& dbName, std::string& key) {
    if (users.empty()) return false;
    
    // Get key from root user
    for (const auto& user : users) {
        if (user.isRoot) {
            auto it = user.dbKeys.find(dbName);
            if (it != user.dbKeys.end()) {
                key = it->second;
                return true;
            }
            break;
        }
    }
    return false;
}
