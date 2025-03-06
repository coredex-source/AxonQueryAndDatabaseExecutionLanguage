#pragma once
#include <string>
#include <map>

struct User {
    std::string username;
    std::string passwordHash;
    bool isRoot;
    std::map<std::string, std::string> dbKeys;  // database name -> key mapping
};
