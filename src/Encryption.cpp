#include "Encryption.hpp"
#include "Constants.hpp"
#include <openssl/rand.h>
#include <openssl/err.h>
#include <stdexcept>
#include <sstream>
#include <iomanip>

std::string Encryption::masterPassword;
bool Encryption::isInitialized = false;

bool Encryption::initialize() {
    if (!isInitialized) {
        OpenSSL_add_all_algorithms();
        isInitialized = true;
    }
    return isInitialized;
}

void Encryption::cleanup() {
    EVP_cleanup();
    isInitialized = false;
}

void Encryption::setMasterPassword(const std::string& password) {
    masterPassword = password;
}

std::string Encryption::encrypt(const std::string& data) {
    auto encryptedVec = encryptToVector(data);
    return std::string(encryptedVec.begin(), encryptedVec.end());
}

std::string Encryption::decrypt(const std::string& data) {
    std::vector<uint8_t> dataVec(data.begin(), data.end());
    return decryptFromVector(dataVec);
}

std::vector<uint8_t> Encryption::encryptToVector(const std::string& data) {
    if (!isInitialized) throw std::runtime_error("Encryption not initialized");
    if (masterPassword.empty()) throw std::runtime_error("Master password not set");

    auto salt = generateRandomBytes(SALT_LENGTH);
    auto iv = generateRandomBytes(IV_LENGTH);
    auto key = deriveKey(masterPassword, salt);
    std::vector<uint8_t> tag(TAG_LENGTH);
    
    // Prepare output buffer
    std::vector<uint8_t> encrypted(data.length());
    int outLen = 0, finalLen = 0;

    // Initialize encryption context
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create cipher context");

    try {
        // Initialize encryption operation
        if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()))
            throw std::runtime_error("Failed to initialize encryption");

        // Encrypt data
        if (1 != EVP_EncryptUpdate(ctx, encrypted.data(), &outLen, 
                                 (const unsigned char*)data.c_str(), data.length()))
            throw std::runtime_error("Failed to encrypt data");

        // Finalize encryption
        if (1 != EVP_EncryptFinal_ex(ctx, encrypted.data() + outLen, &finalLen))
            throw std::runtime_error("Failed to finalize encryption");

        // Get authentication tag
        if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LENGTH, tag.data()))
            throw std::runtime_error("Failed to get authentication tag");

        // Combine all components
        std::vector<uint8_t> result;
        result.reserve(4 + salt.size() + iv.size() + tag.size() + encrypted.size());
        
        // Add version and sizes
        result.push_back(ENCRYPTION_VERSION);
        result.insert(result.end(), salt.begin(), salt.end());
        result.insert(result.end(), iv.begin(), iv.end());
        result.insert(result.end(), tag.begin(), tag.end());
        result.insert(result.end(), encrypted.begin(), encrypted.begin() + outLen + finalLen);

        EVP_CIPHER_CTX_free(ctx);
        return result;
    }
    catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }
}

std::string Encryption::decryptFromVector(const std::vector<uint8_t>& encryptedData) {
    if (!isInitialized) throw std::runtime_error("Encryption not initialized");
    if (masterPassword.empty()) throw std::runtime_error("Master password not set");
    if (!verifyHeader(encryptedData)) throw std::runtime_error("Invalid encrypted data");

    size_t offset = 1; // Skip version byte
    
    // Extract components
    std::vector<uint8_t> salt(encryptedData.begin() + offset, 
                             encryptedData.begin() + offset + SALT_LENGTH);
    offset += SALT_LENGTH;
    
    std::vector<uint8_t> iv(encryptedData.begin() + offset, 
                           encryptedData.begin() + offset + IV_LENGTH);
    offset += IV_LENGTH;
    
    std::vector<uint8_t> tag(encryptedData.begin() + offset,
                            encryptedData.begin() + offset + TAG_LENGTH);
    offset += TAG_LENGTH;
    
    auto key = deriveKey(masterPassword, salt);
    std::vector<uint8_t> decrypted(encryptedData.size() - offset);
    int outLen = 0, finalLen = 0;

    // Initialize decryption context
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("Failed to create cipher context");

    try {
        // Initialize decryption operation
        if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data()))
            throw std::runtime_error("Failed to initialize decryption");

        // Decrypt data
        if (1 != EVP_DecryptUpdate(ctx, decrypted.data(), &outLen,
                                 encryptedData.data() + offset,
                                 encryptedData.size() - offset))
            throw std::runtime_error("Failed to decrypt data");

        // Set expected tag value
        if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LENGTH, tag.data()))
            throw std::runtime_error("Failed to set authentication tag");

        // Finalize decryption
        if (1 != EVP_DecryptFinal_ex(ctx, decrypted.data() + outLen, &finalLen))
            throw std::runtime_error("Decryption failed - data corrupt or tampered");

        EVP_CIPHER_CTX_free(ctx);
        return std::string(decrypted.begin(), decrypted.begin() + outLen + finalLen);
    }
    catch (...) {
        EVP_CIPHER_CTX_free(ctx);
        throw;
    }
}

std::vector<uint8_t> Encryption::deriveKey(const std::string& password, const std::vector<uint8_t>& salt) {
    std::vector<uint8_t> key(KEY_LENGTH);
    
    if (1 != PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                               salt.data(), salt.size(),
                               KEY_ITERATIONS,
                               EVP_sha256(),
                               KEY_LENGTH, key.data())) {
        throw std::runtime_error("Key derivation failed");
    }
    
    return key;
}

std::vector<uint8_t> Encryption::generateRandomBytes(size_t length) {
    std::vector<uint8_t> bytes(length);
    if (1 != RAND_bytes(bytes.data(), length)) {
        throw std::runtime_error("Failed to generate random bytes");
    }
    return bytes;
}

bool Encryption::verifyHeader(const std::vector<uint8_t>& data) {
    if (data.size() < 1 + SALT_LENGTH + IV_LENGTH + TAG_LENGTH) 
        return false;
    return data[0] == ENCRYPTION_VERSION;
}

std::string Encryption::generateRandomKey(size_t length) {
    auto bytes = generateRandomBytes(length);
    std::stringstream ss;
    for (unsigned char byte : bytes) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return ss.str();
}

std::string Encryption::hashString(const std::string& input) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create hash context");
    }
    
    try {
        if (1 != EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) ||
            1 != EVP_DigestUpdate(ctx, input.c_str(), input.length()) ||
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

std::string Encryption::hashWithSalt(const std::string& input, const std::string& salt) {
    return hashString(salt + input + salt);
}
