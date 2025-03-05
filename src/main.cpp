#include <iostream>
#include "CLI.hpp"
#include "Constants.hpp"
#include "Encryption.hpp"
#include "UserManager.hpp"

bool handleFirstBoot() {
    std::cout << "Welcome to first-time setup of " << AQADEL_NAME << std::endl;
    std::cout << "Please set up the root password: ";
    std::string password;
    std::getline(std::cin, password);
    
    if (password.empty()) {
        std::cerr << "Password cannot be empty" << std::endl;
        return false;
    }
    
    Encryption::setMasterPassword(ENCRYPTION_KEY);
    if (!UserManager::createRootUser(password)) {
        std::cerr << "Failed to create root user" << std::endl;
        return false;
    }
    
    std::cout << "Root user created successfully!" << std::endl;
    return true;
}

int main(int argc, char** argv) {
    if (!Encryption::initialize()) {
        std::cerr << "Failed to initialize encryption" << std::endl;
        return 1;
    }

    // Set default encryption key for initial operations
    Encryption::setMasterPassword(ENCRYPTION_KEY);

    // Now initialize user system
    if (!UserManager::initialize()) {
        std::cerr << "Failed to initialize user system" << std::endl;
        return 1;
    }

    if (UserManager::isFirstBoot()) {
        if (!handleFirstBoot()) {
            Encryption::cleanup();
            return 1;
        }
    } else {
        std::string password;
        do {
            std::cout << "Enter password: ";
            std::getline(std::cin, password);
            
            if (!UserManager::authenticate(password)) {
                std::cout << "Invalid password. Try again." << std::endl;
            }
        } while (!UserManager::authenticate(password));
    }

    std::cout << "Welcome to " << AQADEL_NAME << "-" << AQADEL_VERSION << std::endl;
    std::cout << "Type 'help' for available commands or 'exit' to quit." << std::endl;

    CLI cli;
    cli.start();

    Encryption::cleanup();
    return 0;
}
