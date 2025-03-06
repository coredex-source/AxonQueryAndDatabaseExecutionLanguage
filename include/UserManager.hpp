#pragma once
#include "User.hpp"
#include <vector>
#include <optional>
#include <openssl/evp.h>

class UserManager {
public:
    static bool initialize();
    static bool createRootUser(const std::string& password);
    static bool authenticate(const std::string& password);
    static bool isFirstBoot();
    static bool addDatabaseKey(const std::string& dbName, const std::string& key);
    static bool getDatabaseKey(const std::string& dbName, std::string& key);
    
private:
    static std::string hashPassword(const std::string& password);
    static bool saveUsers();
    static bool loadUsers();
    static std::string getUserFilePath();
    
    static std::vector<User> users;
    static bool initialized;
};
