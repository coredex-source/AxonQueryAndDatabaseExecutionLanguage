#pragma once
#include <string>

struct User {
    std::string username;
    std::string passwordHash;
    bool isRoot;
};
