#include "CLI.hpp"
#include "Constants.hpp"
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "Encryption.hpp"
#include <iomanip>
#include <regex>
#include <map>
#include <set>
#include <random>
#include <chrono>
#include "UserManager.hpp"
#include <cstring>

CLI::CLI() {
    loadDefaultDatabase();
}

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

    // Resolve alias
    cmd = resolveCommandAlias(cmd);

    if (cmd == "exit") {
        std::cout << "Thank you for using AQADEL!" << std::endl;
        std::cout << getRandomFunFact() << std::endl;
        return false;
    }
    else if (cmd == "help") {
        std::cout << "Available commands:" << std::endl;
        std::cout << "  help - Show this help message" << std::endl;
        std::cout << "  exit - Exit the program" << std::endl;
        std::cout << "  createDatabase|createDB <name> - Create a new database" << std::endl;
        std::cout << "  useDatabase|useDB <name> - Switch to an existing database" << std::endl;
        std::cout << "  listDatabases|listDBs - Show all available databases" << std::endl;
        std::cout << "  defaultDatabase|defaultDB <name> - Set default database for startup" << std::endl;
        std::cout << "  listTables - Show all tables in current database" << std::endl;
        std::cout << "  descTable <name> - Describe the structure of a table" << std::endl;
        std::cout << "  createTable TableName[col1 type, col2 type, ...] - Create a new table" << std::endl;
        std::cout << "    Supported types: int, float, bool, string{length}" << std::endl;
        std::cout << "  insertValues TableName(value1, value2, ...) - Insert values into table" << std::endl;
        std::cout << "  displayTable <name> - Display all values in table" << std::endl;
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
    else if (cmd == "defaultDatabase") {
        std::string dbName;
        iss >> dbName;
        if (dbName.empty()) {
            std::cout << "Error: Database name is required" << std::endl;
        } else {
            setDefaultDatabase(dbName);
        }
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
    else if (cmd == "insertValues") {
        if (currentDatabase.empty()) {
            std::cout << "Error: No database selected. Use 'useDatabase' first." << std::endl;
        } else {
            std::string insertCommand;
            std::getline(iss, insertCommand);
            insertValues(insertCommand);
        }
    }
    else if (cmd == "displayTable") {
        if (currentDatabase.empty()) {
            std::cout << "Error: No database selected. Use 'useDatabase' first." << std::endl;
        } else {
            std::string tableName;
            iss >> tableName;
            if (tableName.empty()) {
                std::cout << "Error: Table name is required" << std::endl;
            } else {
                displayTable(tableName);
            }
        }
    }
    else if (!command.empty()) {
        std::cout << "Unknown command. Type 'help' for available commands." << std::endl;
    }
    return true;
}

std::string CLI::resolveCommandAlias(const std::string& cmd) {
    static const std::map<std::string, std::string> aliases = {
        {"createDB", "createDatabase"},
        {"useDB", "useDatabase"},
        {"listDBs", "listDatabases"},
        {"defaultDB", "defaultDatabase"}
    };

    auto it = aliases.find(cmd);
    return it != aliases.end() ? it->second : cmd;
}

bool CLI::createDatabase(const std::string& name) {
    std::filesystem::path dbPath = std::filesystem::current_path() / (name + AQADEL_DB_EXT);
    
    if (std::filesystem::exists(dbPath)) {
        std::cout << "Error: Database '" << name << "' already exists" << std::endl;
        return false;
    }

    try {
        // Generate unique key and salt for this database
        std::string dbKey = Encryption::generateRandomKey(DB_KEY_LENGTH);
        std::string dbSalt = Encryption::generateRandomKey(DB_SALT_LENGTH);
        
        // Generate verification token
        std::string verifyToken = Encryption::hashWithSalt(dbKey, dbSalt);
        
        // Store key with user
        if (!UserManager::addDatabaseKey(name, dbKey)) {
            std::cout << "Error: Failed to store database key" << std::endl;
            return false;
        }

        std::ofstream dbFile(dbPath, std::ios::binary);
        if (dbFile.is_open()) {
            // Create database header with security markers
            std::string header = 
                std::string(DB_SALT_MARKER) + dbSalt + "\n" +
                DB_VERIFY_MARKER + verifyToken + "\n" +
                DB_HEADER_MARKER + "AQADEL_DATABASE_v1\n";
            
            std::string encrypted = Encryption::encrypt(header);
            
            // Add integrity check
            std::string integrity = Encryption::hashString(encrypted);
            std::string finalData = 
                std::string(DB_INTEGRITY_MARKER) + integrity + "\n" +
                encrypted;
            
            dbFile.write(finalData.c_str(), finalData.length());
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

    // Get and verify database key
    std::string dbKey;
    if (!UserManager::getDatabaseKey(name, dbKey)) {
        std::cout << "Error: Access denied to database '" << name << "'" << std::endl;
        return false;
    }

    try {
        std::ifstream dbFile(dbPath, std::ios::binary);
        std::stringstream buffer;
        buffer << dbFile.rdbuf();
        std::string fileContent = buffer.str();
        
        // Verify file integrity
        size_t integrityStart = fileContent.find(DB_INTEGRITY_MARKER) + strlen(DB_INTEGRITY_MARKER);
        size_t integrityEnd = fileContent.find("\n", integrityStart);
        std::string storedIntegrity = fileContent.substr(integrityStart, integrityEnd - integrityStart);
        std::string encryptedContent = fileContent.substr(integrityEnd + 1);
        
        if (Encryption::hashString(encryptedContent) != storedIntegrity) {
            std::cout << "Error: Database file has been tampered with" << std::endl;
            return false;
        }
        
        std::string decrypted = Encryption::decrypt(encryptedContent);
        
        // Extract salt and verify key
        size_t saltStart = decrypted.find(DB_SALT_MARKER) + strlen(DB_SALT_MARKER);
        size_t saltEnd = decrypted.find("\n", saltStart);
        std::string dbSalt = decrypted.substr(saltStart, saltEnd - saltStart);
        
        size_t verifyStart = decrypted.find(DB_VERIFY_MARKER) + strlen(DB_VERIFY_MARKER);
        size_t verifyEnd = decrypted.find("\n", verifyStart);
        std::string storedVerify = decrypted.substr(verifyStart, verifyEnd - verifyStart);
        
        // Get stored key and verify
        if (Encryption::hashWithSalt(dbKey, dbSalt) != storedVerify) {
            std::cout << "Error: Invalid database key" << std::endl;
            return false;
        }
        
        // If we get here, authentication successful
        currentDatabase = name;
        std::cout << "Switched to database '" << name << "'" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Error: Failed to verify database access - " << e.what() << std::endl;
        return false;
    }
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
    std::filesystem::path dbPath(std::filesystem::current_path());
    dbPath /= (currentDatabase + AQADEL_DB_EXT);
    
    // Read and decrypt existing content
    std::string fileContent = readAndVerifyDatabaseContent(dbPath);
    std::string encryptedContent = extractEncryptedContent(fileContent);
    std::string existingContent = Encryption::decrypt(encryptedContent);
    
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
    newContent << TABLE_DATA_START << "\n";  // Add empty data section
    newContent << TABLE_DATA_END << "\n";
    newContent << "END_TABLE\n";
    
    // Write encrypted content back with integrity check
    std::string encrypted = Encryption::encrypt(newContent.str());
    std::string integrity = Encryption::hashString(encrypted);
    std::string finalContent = std::string(DB_INTEGRITY_MARKER) + integrity + "\n" + encrypted;
    
    std::ofstream outFile(dbPath, std::ios::binary | std::ios::trunc);
    outFile.write(finalContent.c_str(), finalContent.length());
}

std::string CLI::readAndVerifyDatabaseContent(const std::filesystem::path& dbPath) {
    std::ifstream dbFile(dbPath, std::ios::binary);
    if (!dbFile) {
        throw std::runtime_error("Cannot open database file");
    }

    std::stringstream buffer;
    buffer << dbFile.rdbuf();
    return buffer.str();
}

std::string CLI::extractEncryptedContent(const std::string& fileContent) {
    size_t integrityPos = fileContent.find(DB_INTEGRITY_MARKER);
    if (integrityPos == std::string::npos) {
        throw std::runtime_error("Invalid database format (missing integrity marker)");
    }

    size_t dataStart = fileContent.find('\n', integrityPos);
    if (dataStart == std::string::npos) {
        throw std::runtime_error("Invalid database format (missing newline)");
    }

    std::string storedHash = fileContent.substr(
        integrityPos + strlen(DB_INTEGRITY_MARKER),
        dataStart - (integrityPos + strlen(DB_INTEGRITY_MARKER))
    );

    std::string encryptedContent = fileContent.substr(dataStart + 1);
    std::string computedHash = Encryption::hashString(encryptedContent);

    if (storedHash != computedHash) {
        throw std::runtime_error("Database integrity check failed");
    }

    return encryptedContent;
}

std::string CLI::getDecryptedContent(const std::filesystem::path& dbPath) {
    std::string fileContent = readAndVerifyDatabaseContent(dbPath);
    std::string encryptedContent = extractEncryptedContent(fileContent);
    return Encryption::decrypt(encryptedContent);
}

bool CLI::tableExists(const std::string& tableName) {
    try {
        std::filesystem::path dbPath{std::filesystem::current_path()};
        dbPath /= (currentDatabase + AQADEL_DB_EXT);
        std::string decrypted = getDecryptedContent(dbPath);
        
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
    catch (const std::exception&) {
        return false;
    }
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
    try {
        // Fix the filesystem path construction
        std::filesystem::path dbPath{std::filesystem::current_path()};  // Use {} instead of ()
        dbPath /= (currentDatabase + AQADEL_DB_EXT);

        std::ifstream dbFile(dbPath, std::ios::binary);
        if (!dbFile) {
            throw std::runtime_error("Cannot open database file");
        }
        
        // Read file content and verify integrity
        std::string content;
        {
            std::stringstream buffer;
            buffer << dbFile.rdbuf();
            content = buffer.str();
        }
        
        // Extract and verify integrity marker
        size_t integrityPos = content.find(DB_INTEGRITY_MARKER);
        if (integrityPos == std::string::npos) {
            throw std::runtime_error("Invalid database format");
        }
        
        size_t dataStart = content.find('\n', integrityPos);
        if (dataStart == std::string::npos) {
            throw std::runtime_error("Invalid database format");
        }
        
        std::string storedHash = content.substr(
            integrityPos + strlen(DB_INTEGRITY_MARKER),
            dataStart - (integrityPos + strlen(DB_INTEGRITY_MARKER))
        );
        
        std::string encryptedContent = content.substr(dataStart + 1);
        std::string computedHash = Encryption::hashString(encryptedContent);
        
        if (storedHash != computedHash) {
            throw std::runtime_error("Database integrity check failed");
        }
        
        // Decrypt and process content
        std::string decrypted = Encryption::decrypt(encryptedContent);
        std::istringstream iss(decrypted);
        std::string line;
        bool found = false;
        std::set<std::string> tableNames;
        
        std::cout << "Tables in database '" << currentDatabase << "':" << std::endl;
        
        while (std::getline(iss, line)) {
            if (line.substr(0, 6) == "TABLE ") {
                std::string tableName = line.substr(6);
                tableName.erase(0, tableName.find_first_not_of(" \t"));
                tableName.erase(tableName.find_last_not_of(" \t") + 1);
                tableNames.insert(tableName);
                found = true;
            }
        }
        
        for (const auto& tableName : tableNames) {
            std::cout << "  " << tableName << std::endl;
        }
        
        if (!found) {
            std::cout << "  No tables found" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

void CLI::descTable(const std::string& tableName) {
    try {
        std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
        std::ifstream dbFile(dbPath, std::ios::binary);
        if (!dbFile) {
            throw std::runtime_error("Cannot open database file");
        }

        // Read file content and verify integrity
        std::string content;
        {
            std::stringstream buffer;
            buffer << dbFile.rdbuf();
            content = buffer.str();
        }
        
        // Extract and verify integrity marker
        size_t integrityPos = content.find(DB_INTEGRITY_MARKER);
        if (integrityPos == std::string::npos) {
            throw std::runtime_error("Invalid database format");
        }
        
        size_t dataStart = content.find('\n', integrityPos);
        if (dataStart == std::string::npos) {
            throw std::runtime_error("Invalid database format");
        }
        
        std::string storedHash = content.substr(
            integrityPos + strlen(DB_INTEGRITY_MARKER),
            dataStart - (integrityPos + strlen(DB_INTEGRITY_MARKER))
        );
        
        std::string encryptedContent = content.substr(dataStart + 1);
        std::string computedHash = Encryption::hashString(encryptedContent);
        
        if (storedHash != computedHash) {
            throw std::runtime_error("Database integrity check failed");
        }

        // Decrypt and process content
        std::string decrypted = Encryption::decrypt(encryptedContent);
        std::istringstream iss(decrypted);
        std::string line;
        
        bool found = false;
        bool inTargetTable = false;
        std::cout << "Structure of table '" << tableName << "':" << std::endl;
        const std::string separator(50, '-');
        std::cout << separator << std::endl;
        std::cout << std::left 
                  << std::setw(20) << "Column Name"
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
                         << std::setw(20) << colName
                         << std::setw(15) << colType;
                if (colType == DT_STRING) {
                    std::cout << stringSize;
                } else {
                    std::cout << "-";
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
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

std::vector<Column> CLI::getTableColumns(const std::string& tableName) {
    std::vector<Column> columns;
    try {
        std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
        std::string decrypted = getDecryptedContent(dbPath);
        
        std::istringstream iss(decrypted);
        std::string line;
        bool inTargetTable = false;
        
        while (std::getline(iss, line)) {
            if (line.substr(0, 6) == "TABLE ") {
                std::string currentTable = line.substr(6);
                currentTable.erase(0, currentTable.find_first_not_of(" \t"));
                currentTable.erase(currentTable.find_last_not_of(" \t") + 1);
                
                if (currentTable == tableName) {
                    inTargetTable = true;
                } else {
                    inTargetTable = false;
                }
            }
            else if (inTargetTable && line.substr(0, 7) == "COLUMN ") {
                std::istringstream colStream(line.substr(7));
                Column col;
                colStream >> col.name >> col.dataType;
                if (col.dataType == DT_STRING) {
                    colStream >> col.stringLength;
                }
                columns.push_back(col);
            }
            else if (inTargetTable && line == "END_TABLE") {
                break;
            }
        }
    }
    catch (const std::exception&) {
        columns.clear();
    }
    return columns;
}

bool CLI::validateValue(const std::string& value, const Column& column) {
    if (column.dataType == DT_INT) {
        try {
            std::stoi(value);
            return true;
        } catch (...) {
            return false;
        }
    }
    else if (column.dataType == DT_FLOAT) {
        try {
            std::stof(value);
            return true;
        } catch (...) {
            return false;
        }
    }
    else if (column.dataType == DT_BOOL) {
        return value == "true" || value == "false";
    }
    else if (column.dataType == DT_STRING) {
        return value.length() <= static_cast<size_t>(column.stringLength);
    }
    return false;
}

void CLI::parseValue(const std::string& value, std::string& parsedValue) {
    if (value.front() == '"' && value.back() == '"') {
        parsedValue = value.substr(1, value.length() - 2);
    } else {
        parsedValue = value;
    }
}

bool CLI::insertValues(const std::string& command) {
    size_t bracketStart = command.find('(');
    size_t bracketEnd = command.find(')');

    if (bracketStart == std::string::npos || bracketEnd == std::string::npos) {
        std::cout << "Error: Invalid insert syntax" << std::endl;
        return false;
    }

    std::string tableName = command.substr(1, bracketStart - 1);
    tableName.erase(0, tableName.find_first_not_of(" \t"));
    tableName.erase(tableName.find_last_not_of(" \t") + 1);

    if (!tableExists(tableName)) {
        std::cout << "Error: Table '" << tableName << "' does not exist" << std::endl;
        return false;
    }

    std::vector<Column> columns = getTableColumns(tableName);
    std::string valuesStr = command.substr(bracketStart + 1, bracketEnd - bracketStart - 1);
    std::vector<std::string> values;
    
    // Parse values handling quoted strings
    std::string currentValue;
    bool insideQuotes = false;
    for (char c : valuesStr) {
        if (c == '"') {
            insideQuotes = !insideQuotes;
            currentValue += c;
        }
        else if (c == ',' && !insideQuotes) {
            if (!currentValue.empty()) {
                currentValue.erase(0, currentValue.find_first_not_of(" \t"));
                currentValue.erase(currentValue.find_last_not_of(" \t") + 1);
                values.push_back(currentValue);
                currentValue.clear();
            }
        }
        else {
            currentValue += c;
        }
    }
    if (!currentValue.empty()) {
        currentValue.erase(0, currentValue.find_first_not_of(" \t"));
        currentValue.erase(currentValue.find_last_not_of(" \t") + 1);
        values.push_back(currentValue);
    }

    if (values.size() != columns.size()) {
        std::cout << "Error: Number of values (" << values.size() 
                 << ") does not match number of columns (" << columns.size() << ")" << std::endl;
        return false;
    }

    // Validate and parse values
    std::vector<std::string> parsedValues;
    for (size_t i = 0; i < values.size(); i++) {
        std::string parsedValue;
        parseValue(values[i], parsedValue);
        if (!validateValue(parsedValue, columns[i])) {
            std::cout << "Error: Invalid value for column '" << columns[i].name 
                     << "' (" << columns[i].dataType << "): " << values[i] << std::endl;
            return false;
        }
        parsedValues.push_back(parsedValue);
    }

    try {
        // Read existing database content
        std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
        std::string fileContent = readAndVerifyDatabaseContent(dbPath);
        std::string encryptedContent = extractEncryptedContent(fileContent);
        std::string decrypted = Encryption::decrypt(encryptedContent);
        
        // Process the content and add new data
        std::istringstream iss(decrypted);
        std::ostringstream oss;
        std::string line;
        bool inTargetTable = false;
        bool dataStartFound = false;
        bool dataEndFound = false;
        std::vector<std::string> existingRows;
        
        while (std::getline(iss, line)) {
            if (line.substr(0, 6) == "TABLE " && line.substr(6) == tableName) {
                inTargetTable = true;
                oss << line << "\n";
            }
            else if (inTargetTable && line == TABLE_DATA_START) {
                dataStartFound = true;
                oss << line << "\n";
            }
            else if (inTargetTable && line == TABLE_DATA_END) {
                // Insert new row before DATA_END
                std::string newRow;
                for (const auto& value : parsedValues) {
                    if (!newRow.empty()) newRow += ROW_SEPARATOR;
                    newRow += value;
                }
                for (const auto& existingRow : existingRows) {
                    oss << existingRow << "\n";
                }
                oss << newRow << "\n";
                oss << line << "\n";
                dataEndFound = true;
            }
            else if (inTargetTable && dataStartFound && !dataEndFound && line != TABLE_DATA_END) {
                // Store existing data rows
                existingRows.push_back(line);
            }
            else if (inTargetTable && line == "END_TABLE" && !dataStartFound) {
                // If no data section exists, create one
                oss << TABLE_DATA_START << "\n";
                std::string newRow;
                for (const auto& value : parsedValues) {
                    if (!newRow.empty()) newRow += ROW_SEPARATOR;
                    newRow += value;
                }
                oss << newRow << "\n";
                oss << TABLE_DATA_END << "\n";
                oss << line << "\n";
                inTargetTable = false;
            }
            else {
                oss << line << "\n";
            }
        }

        // Encrypt and write back to file
        std::string newContent = oss.str();
        std::string encrypted = Encryption::encrypt(newContent);
        
        // Add integrity marker
        std::string integrity = Encryption::hashString(encrypted);
        std::string finalContent = std::string(DB_INTEGRITY_MARKER) + integrity + "\n" + encrypted;
        
        std::ofstream outFile(dbPath, std::ios::binary | std::ios::trunc);
        outFile.write(finalContent.c_str(), finalContent.length());
        
        std::cout << "Values inserted successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Error: Failed to insert values - " << e.what() << std::endl;
        return false;
    }
}

void CLI::displayTable(const std::string& tableName) {
    try {
        std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
        
        // Get database content
        std::string decrypted = getDecryptedContent(dbPath);
        
        // Get table columns
        std::vector<Column> columns = getTableColumns(tableName);
        if (columns.empty()) {
            throw std::runtime_error("Table structure not found");
        }

        // Process table data
        std::istringstream iss(decrypted);
        std::string line;
        bool inTargetTable = false;
        bool inData = false;
        bool hasData = false;

        // Calculate column widths
        std::vector<size_t> colWidths;
        for (const auto& col : columns) {
            colWidths.push_back(std::max(col.name.length(), size_t(15)));
        }

        // Print header
        std::cout << std::string(50, '-') << std::endl;
        for (size_t i = 0; i < columns.size(); i++) {
            std::cout << std::left << std::setw(colWidths[i]) << columns[i].name << " ";
        }
        std::cout << std::endl << std::string(50, '-') << std::endl;

        // Display data
        while (std::getline(iss, line)) {
            if (line.substr(0, 6) == "TABLE " && line.substr(6) == tableName) {
                inTargetTable = true;
            }
            else if (inTargetTable && line == TABLE_DATA_START) {
                inData = true;
            }
            else if (inTargetTable && line == TABLE_DATA_END) {
                inData = false;
                break;
            }
            else if (inData) {
                hasData = true;
                std::istringstream rowStream(line);
                std::string value;
                size_t colIndex = 0;
                
                while (std::getline(rowStream, value, *ROW_SEPARATOR)) {
                    if (colIndex < columns.size()) {
                        std::cout << std::left << std::setw(colWidths[colIndex]) << value << " ";
                    }
                    colIndex++;
                }
                std::cout << std::endl;
            }
        }

        std::cout << std::string(50, '-') << std::endl;
        if (!hasData) {
            std::cout << "No data in table" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

bool CLI::setDefaultDatabase(const std::string& name) {
    std::filesystem::path dbPath(std::filesystem::current_path());
    dbPath /= (name + AQADEL_DB_EXT);
    
    if (!std::filesystem::exists(dbPath)) {
        std::cout << "Error: Database '" << name << "' does not exist" << std::endl;
        return false;
    }

    try {
        std::ofstream configFile(DEFAULT_DB_FILE);
        if (!configFile.is_open()) {
            throw std::runtime_error("Cannot open config file");
        }
        configFile << "defaultDatabase = " << name << std::endl;
        bool success = !configFile.fail();
        configFile.close();
        
        if (success) {
            std::cout << "Set '" << name << "' as default database" << std::endl;
        }
        return success;
    }
    catch (const std::exception& e) {
        std::cout << "Error setting default database: " << e.what() << std::endl;
        return false;
    }
}

bool CLI::loadDefaultDatabase() {
    try {
        if (std::filesystem::exists(DEFAULT_DB_FILE)) {
            std::ifstream configFile(DEFAULT_DB_FILE);
            std::string line;
            if (configFile && std::getline(configFile, line)) {  // Fix nodiscard warning
                size_t pos = line.find("=");
                if (pos != std::string::npos) {
                    std::string defaultDB = line.substr(pos + 1);
                    // Trim whitespace
                    defaultDB.erase(0, defaultDB.find_first_not_of(" \t"));
                    defaultDB.erase(defaultDB.find_last_not_of(" \t") + 1);
                    
                    if (!defaultDB.empty() && std::filesystem::exists(defaultDB + AQADEL_DB_EXT)) {
                        useDatabase(defaultDB);
                        return true;
                    }
                }
            }
        }
    }
    catch (...) {
        // Silently fail loading default database
    }
    return false;
}

std::string CLI::getHiddenInput() {
    std::string input;
    
#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));

    int ch;
    while ((ch = getchar()) != '\n' && ch != '\r') {
        if (ch == '\b') {  // Backspace
            if (!input.empty()) {
                std::cout << "\b \b" << std::flush;
                input.pop_back();
            }
        }
        else if (ch >= 32 && ch <= 126) {  // Printable characters
            input += static_cast<char>(ch);
            std::cout << '*' << std::flush;
        }
    }
    std::cout << std::endl;
    
    SetConsoleMode(hStdin, mode);
#else
    termios oldt;
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (ch == 127 || ch == '\b') {  // Backspace
            if (!input.empty()) {
                std::cout << "\b \b" << std::flush;
                input.pop_back();
            }
        }
        else if (ch >= 32 && ch <= 126) {  // Printable characters
            input += static_cast<char>(ch);
            std::cout << '*' << std::flush;
        }
    }
    std::cout << std::endl;
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

    return input;
}

std::string CLI::getRandomFunFact() {
    static const std::vector<std::string> funFacts = {
        "The first commercial database management system was created by Charles Bachman in 1960.",
        "SQL was originally called SEQUEL (Structured English Query Language).",
        "The world's largest database is estimated to be the World Data Center for Climate, containing over 220 terabytes of data.",
        "Oracle was founded under the name Software Development Laboratories (SDL) in 1977.",
        "The term \"database\" was first used in the 1960s during a computer conference.",
        "MongoDB got its name from the word \"humongous\" because it was designed to handle huge amounts of data.",
        "PostgreSQL was originally called Postgres and was created at UC Berkeley.",
        "MySQL is named after the creator's daughter, My.",
        "The first version of Microsoft SQL Server was developed for the IBM OS/2 platform.",
        "Redis, which stands for REmote DIctionary Server, was created by Salvatore Sanfilippo while working on a real-time analytics system."
    };
    
    if (funFacts.empty()) {
        return "Goodbye!";
    }

    unsigned int seed = static_cast<unsigned int>(
        std::chrono::system_clock::now().time_since_epoch().count()
    );
    std::default_random_engine generator(seed);
    int maxIdx = static_cast<int>(funFacts.size() - 1);
    std::uniform_int_distribution<int> distribution(0, maxIdx);
    
    return "Fun fact: " + funFacts[distribution(generator)];
}
