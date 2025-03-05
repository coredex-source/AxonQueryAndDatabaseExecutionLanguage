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
    
private:
    static std::string hashPassword(const std::string& password);
    static bool saveUsers();
    static bool loadUsers();
    static std::string getUserFilePath();
    
    static std::vector<User> users;
    static bool initialized;
};
