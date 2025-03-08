#include <iostream>
#include "CLI.hpp"
#include "Constants.hpp"
#include "Encryption.hpp"
#include "UserManager.hpp"

bool handleFirstBoot() {
    std::cout << "Welcome to first-time setup of " << AQADEL_NAME << std::endl;
    std::cout << "Please set up the root password: ";
    std::string password = CLI::getHiddenInput();
    std::cout << std::endl;
    
    if (password.empty()) {
        std::cerr << "Password cannot be empty" << std::endl;
        return false;
    }
    
    // Use the real password directly instead of ENCRYPTION_KEY
    Encryption::setMasterPassword(password);
    if (!UserManager::createRootUser(password)) {
        std::cerr << "Failed to create root user" << std::endl;
        return false;
    }
    
    std::cout << "Root user created successfully!" << std::endl;
    return true;
}

int main(int /*argc*/, char** /*argv*/) {
    if (!Encryption::initialize()) {
        std::cerr << "Failed to initialize encryption" << std::endl;
        return 1;
    }

    // Initialize with default key first for system operations
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
        int attempts = 0;
        bool authenticated = false;

        while (attempts < MAX_PASSWORD_ATTEMPTS && !authenticated) {
            std::cout << "Enter password" << (attempts > 0 ? " (" + std::to_string(MAX_PASSWORD_ATTEMPTS - attempts) + " attempts remaining): " : ": ");
            password = CLI::getHiddenInput();
            std::cout << std::endl;
            
            if (UserManager::authenticate(password)) {
                authenticated = true;
            } else {
                attempts++;
                if (attempts < MAX_PASSWORD_ATTEMPTS) {
                    std::cout << "Invalid password. Try again." << std::endl;
                }
            }
        }

        if (!authenticated) {
            std::cout << "Maximum password attempts exceeded. Exiting..." << std::endl;
            Encryption::cleanup();
            return 1;
        }
    }

    std::cout << "Welcome to " << AQADEL_NAME << "-" << AQADEL_VERSION << std::endl;
    std::cout << "Type 'help' for available commands or 'exit' to quit." << std::endl;

    CLI cli;
    cli.start();

    Encryption::cleanup();
    return 0;
}
