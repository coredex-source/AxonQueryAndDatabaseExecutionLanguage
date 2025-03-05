#include "CLI.hpp"
#include "Constants.hpp"
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "Encryption.hpp"

void CLI::start() {
    std::string command;
    bool running = true;

    while (running) {
        std::cout << "aqadel> ";
        std::getline(std::cin, command);
        running = processCommand(command);
    }
}

bool CLI::processCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    if (cmd == "exit") {
        std::cout << "Goodbye!" << std::endl;
        return false;
    }
    else if (cmd == "help") {
        std::cout << "Available commands:" << std::endl;
        std::cout << "  help - Show this help message" << std::endl;
        std::cout << "  exit - Exit the program" << std::endl;
        std::cout << "  createDatabase <name> - Create a new database" << std::endl;
        std::cout << "  useDatabase <name> - Switch to an existing database" << std::endl;
        std::cout << "  listDatabases - Show all available databases" << std::endl;
        std::cout << "  listTables - Show all tables in current database" << std::endl;
        std::cout << "  descTable <name> - Describe the structure of a table" << std::endl;
        std::cout << "  createTable TableName[col1 type, col2 type, ...] - Create a new table" << std::endl;
        std::cout << "    Supported types: int, float, bool, string{length}" << std::endl;
    }
    else if (cmd == "createDatabase") {
        std::string dbName;
        iss >> dbName;
        if (dbName.empty()) {
            std::cout << "Error: Database name is required" << std::endl;
        } else {
            createDatabase(dbName);
        }
    }
    else if (cmd == "useDatabase") {
        std::string dbName;
        iss >> dbName;
        if (dbName.empty()) {
            std::cout << "Error: Database name is required" << std::endl;
        } else {
            useDatabase(dbName);
        }
    }
    else if (cmd == "listDatabases") {
        listDatabases();
    }
    else if (cmd == "listTables") {
        if (currentDatabase.empty()) {
            std::cout << "Error: No database selected. Use 'useDatabase' first." << std::endl;
        } else {
            listTables();
        }
    }
    else if (cmd == "createTable") {
        if (currentDatabase.empty()) {
            std::cout << "Error: No database selected. Use 'useDatabase' first." << std::endl;
        } else {
            std::string tableCommand;
            std::getline(iss, tableCommand);
            createTable(tableCommand);
        }
    }
    else if (cmd == "descTable") {
        if (currentDatabase.empty()) {
            std::cout << "Error: No database selected. Use 'useDatabase' first." << std::endl;
        } else {
            std::string tableName;
            iss >> tableName;
            if (tableName.empty()) {
                std::cout << "Error: Table name is required" << std::endl;
            } else {
                descTable(tableName);
            }
        }
    }
    else if (!command.empty()) {
        std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
    }
    return true;
}

bool CLI::createDatabase(const std::string& name) {
    std::filesystem::path dbPath = std::filesystem::current_path() / (name + AQADEL_DB_EXT);
    
    if (std::filesystem::exists(dbPath)) {
        std::cout << "Error: Database '" << name << "' already exists" << std::endl;
        return false;
    }

    try {
        std::ofstream dbFile(dbPath, std::ios::binary);
        if (dbFile.is_open()) {
            std::string header = ENCRYPTION_HEADER "\nAQADEL_DATABASE_v1\n";
            std::string encrypted = Encryption::encrypt(header);
            dbFile.write(encrypted.c_str(), encrypted.length());
            dbFile.close();
            currentDatabase = name;
            std::cout << "Database '" << name << "' created successfully" << std::endl;
            return true;
        }
    } catch (const std::exception& e) {
        std::cout << "Error creating database: " << e.what() << std::endl;
    }
    
    return false;
}

bool CLI::useDatabase(const std::string& name) {
    std::filesystem::path dbPath = std::filesystem::current_path() / (name + AQADEL_DB_EXT);
    
    if (!std::filesystem::exists(dbPath)) {
        std::cout << "Error: Database '" << name << "' does not exist" << std::endl;
        return false;
    }

    currentDatabase = name;
    std::cout << "Switched to database '" << name << "'" << std::endl;
    return true;
}

void CLI::listDatabases() {
    bool found = false;
    std::cout << "Available databases:" << std::endl;
    
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
        if (entry.path().extension() == AQADEL_DB_EXT) {
            std::string dbName = entry.path().stem().string();
            std::cout << "  " << dbName;
            if (dbName == currentDatabase) {
                std::cout << " (current)";
            }
            std::cout << std::endl;
            found = true;
        }
    }

    if (!found) {
        std::cout << "  No databases found" << std::endl;
    }
}

bool CLI::createTable(const std::string& command) {
    size_t bracketStart = command.find('[');
    size_t bracketEnd = command.find(']');
    
    if (bracketStart == std::string::npos || bracketEnd == std::string::npos) {
        std::cout << "Error: Invalid table creation syntax" << std::endl;
        return false;
    }

    std::string tableName = command.substr(1, bracketStart - 1);
    std::string columnStr = command.substr(bracketStart + 1, bracketEnd - bracketStart - 1);

    // Trim whitespace from table name
    tableName.erase(0, tableName.find_first_not_of(" \t"));
    tableName.erase(tableName.find_last_not_of(" \t") + 1);

    if (tableName.empty()) {
        std::cout << "Error: Table name is required" << std::endl;
        return false;
    }

    if (tableExists(tableName)) {
        std::cout << "Error: Table '" << tableName << "' already exists" << std::endl;
        return false;
    }

    std::vector<Column> columns;
    if (!parseColumns(columnStr, columns)) {
        return false;
    }

    writeTableToDatabase(tableName, columns);
    std::cout << "Table '" << tableName << "' created successfully" << std::endl;
    return true;
}

void CLI::writeTableToDatabase(const std::string& tableName, const std::vector<Column>& columns) {
    std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
    
    // Read and decrypt existing content
    std::string existingContent;
    {
        std::ifstream inFile(dbPath, std::ios::binary);
        std::stringstream buffer;
        buffer << inFile.rdbuf();
        existingContent = Encryption::decrypt(buffer.str());
    }
    
    // Add new table definition
    std::stringstream newContent;
    newContent << existingContent;
    newContent << "TABLE " << tableName << "\n";
    for (const auto& col : columns) {
        newContent << "COLUMN " << col.name << " " << col.dataType;
        if (col.dataType == DT_STRING) {
            newContent << " " << col.stringLength;
        }
        newContent << "\n";
    }
    newContent << "END_TABLE\n";
    
    // Write encrypted content back
    std::ofstream outFile(dbPath, std::ios::binary);
    std::string encrypted = Encryption::encrypt(newContent.str());
    outFile.write(encrypted.c_str(), encrypted.length());
}

bool CLI::tableExists(const std::string& tableName) {
    std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
    std::ifstream dbFile(dbPath, std::ios::binary);
    std::stringstream buffer;
    buffer << dbFile.rdbuf();
    
    std::string decrypted = Encryption::decrypt(buffer.str());
    std::istringstream iss(decrypted);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (line.substr(0, 6) == "TABLE ") {
            std::string existingTable = line.substr(6);
            existingTable.erase(0, existingTable.find_first_not_of(" \t"));
            existingTable.erase(existingTable.find_last_not_of(" \t") + 1);
            
            if (existingTable == tableName) {
                return true;
            }
        }
    }
    
    return false;
}

bool CLI::parseColumns(const std::string& columnStr, std::vector<Column>& columns) {
    std::istringstream iss(columnStr);
    std::string columnDef;
    
    while (std::getline(iss, columnDef, ',')) {
        // Trim whitespace
        columnDef.erase(0, columnDef.find_first_not_of(" \t"));
        columnDef.erase(columnDef.find_last_not_of(" \t") + 1);

        std::istringstream colStream(columnDef);
        Column col;
        std::string dataType;

        colStream >> col.name >> dataType;

        if (col.name.empty() || dataType.empty()) {
            std::cout << "Error: Invalid column definition: " << columnDef << std::endl;
            return false;
        }

        // Handle string type with length
        if (dataType.substr(0, 6) == DT_STRING) {
            col.dataType = DT_STRING;
            size_t openBrace = dataType.find('{');
            size_t closeBrace = dataType.find('}');
            
            if (openBrace != std::string::npos && closeBrace != std::string::npos) {
                try {
                    col.stringLength = std::stoi(dataType.substr(openBrace + 1, closeBrace - openBrace - 1));
                }
                catch (...) {
                    std::cout << "Error: Invalid string length in column " << col.name << std::endl;
                    return false;
                }
            }
            else {
                std::cout << "Error: String type requires length specification {n}" << std::endl;
                return false;
            }
        }
        else if (dataType == DT_INT || dataType == DT_FLOAT || dataType == DT_BOOL) {
            col.dataType = dataType;
            col.stringLength = 0;
        }
        else {
            std::cout << "Error: Unknown data type: " << dataType << std::endl;
            return false;
        }

        columns.push_back(col);
    }

    if (columns.empty()) {
        std::cout << "Error: No columns defined" << std::endl;
        return false;
    }

    return true;
}

void CLI::listTables() {
    std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
    std::ifstream dbFile(dbPath, std::ios::binary);
    std::stringstream buffer;
    buffer << dbFile.rdbuf();
    
    std::string decrypted = Encryption::decrypt(buffer.str());
    std::istringstream iss(decrypted);
    std::string line;
    bool found = false;
    
    std::cout << "Tables in database '" << currentDatabase << "':" << std::endl;
    
    while (std::getline(iss, line)) {
        if (line.substr(0, 6) == "TABLE ") {
            std::string tableName = line.substr(6);
            tableName.erase(0, tableName.find_first_not_of(" \t"));
            tableName.erase(tableName.find_last_not_of(" \t") + 1);
            std::cout << "  " << tableName << std::endl;
            found = true;
        }
    }
    
    if (!found) {
        std::cout << "  No tables found" << std::endl;
    }
}

void CLI::descTable(const std::string& tableName) {
    std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
    std::ifstream dbFile(dbPath, std::ios::binary);
    std::stringstream buffer;
    buffer << dbFile.rdbuf();
    
    std::string decrypted = Encryption::decrypt(buffer.str());
    std::istringstream iss(decrypted);
    std::string line;
    
    bool found = false;
    bool inTargetTable = false;
    std::cout << "Structure of table '" << tableName << "':" << std::endl;
    const std::string separator(50, '-');  // Increased width
    std::cout << separator << std::endl;
    std::cout << std::left 
              << std::setw(20) << "Column Name"    // Increased from 18
              << std::setw(15) << "Type"
              << "Size" << std::endl;
    std::cout << separator << std::endl;
    
    while (std::getline(iss, line)) {
        if (line.substr(0, 6) == "TABLE ") {
            std::string currentTable = line.substr(6);
            currentTable.erase(0, currentTable.find_first_not_of(" \t"));
            currentTable.erase(currentTable.find_last_not_of(" \t") + 1);
            
            if (currentTable == tableName) {
                found = true;
                inTargetTable = true;
            } else {
                inTargetTable = false;
            }
        }
        else if (inTargetTable && line.substr(0, 7) == "COLUMN ") {
            std::istringstream colStream(line.substr(7));
            std::string colName, colType;
            int stringSize = 0;
            
            colStream >> colName >> colType;
            if (colType == DT_STRING) {
                colStream >> stringSize;
            }
            
            std::cout << std::left 
                     << std::setw(20) << colName    // Increased from 18
                     << std::setw(15) << colType;
            if (colType == DT_STRING) {
                std::cout << stringSize;
            } else {
                std::cout << "-";  // Add dash for non-string types
            }
            std::cout << std::endl;
        }
        else if (inTargetTable && line == "END_TABLE") {
            break;
        }
    }
    
    std::cout << separator << std::endl;
    if (!found) {
        std::cout << "Table '" << tableName << "' not found" << std::endl;
    }
}
