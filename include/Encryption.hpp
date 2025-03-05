#pragma once
#include <string>
#include <vector>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/sha.h>

class Encryption {
public:
    static bool initialize();
    static void cleanup();
    static std::string encrypt(const std::string& data);
    static std::string decrypt(const std::string& data);
    static void setMasterPassword(const std::string& password);

private:
    static std::vector<uint8_t> encryptToVector(const std::string& data);
    static std::string decryptFromVector(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> deriveKey(const std::string& password, const std::vector<uint8_t>& salt);
    static std::vector<uint8_t> generateRandomBytes(size_t length);
    static bool verifyHeader(const std::vector<uint8_t>& data);
    
    static std::string masterPassword;
    static bool isInitialized;
};

struct EncryptedData {
    std::vector<uint8_t> iv;
    std::vector<uint8_t> salt;
    std::vector<uint8_t> tag;
    std::vector<uint8_t> data;
};
