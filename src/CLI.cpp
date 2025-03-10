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
    inSafeBlock = false;
    safeBlockBackupPath = "";
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
        // Check if there's an open transaction
        if (inSafeBlock) {
            std::cout << "Error: Cannot exit with an open safe block. Use 'closeSafeBlock' or 'discardSafeBlock' first." << std::endl;
            return true;
        }
        
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
        std::cout << "  deleteDatabase|deleteDB <name> - Delete an existing database" << std::endl;
        std::cout << "  listTables - Show all tables in current database" << std::endl;
        std::cout << "  descTable <name> - Describe the structure of a table" << std::endl;
        std::cout << "  createTable TableName[col1 type, col2 type, ...] - Create a new table" << std::endl;
        std::cout << "    Supported types: int, float, bool, string{length}" << std::endl;
        std::cout << "  deleteTable <name> - Delete a table from current database" << std::endl;
        std::cout << "  editTable TableName addColumn/removeColumn ColumnName [DataType] - Modify table structure" << std::endl;
        std::cout << "    Note: DataType required only when adding columns (e.g., int, float, bool, string{length})" << std::endl;
        std::cout << "  insertValues TableName(value1, value2, ...) - Insert values into table" << std::endl;
        std::cout << "  displayTable <name> - Display all values in table" << std::endl;
        std::cout << "  displayTable <name> select values if ColumnName == <value> - Display selected values from a table" << std::endl;
        std::cout << "  deleteValue TableName if ColumnName == Value - Delete rows where condition is met" << std::endl;
        std::cout << "    Note: Value should be in quotes if string" << std::endl;
        std::cout << "  editValue TableName set ColumnName = NewValue if ColumnName == Value - Edit values" << std::endl;
        std::cout << "    Note: Value and NewValue should be in quotes if string" << std::endl;
        std::cout << "  openSafeBlock|openTransaction|openChannel|osb - Start a transaction" << std::endl;
        std::cout << "    Note: Changes won't be permanently saved until closeSafeBlock is executed" << std::endl;
        std::cout << "  closeSafeBlock|closeTransaction|closeChannel|csb - Save and end transaction" << std::endl;
        std::cout << "  discardSafeBlock|discardTransaction|discardChannel|dsb - Discard changes and end transaction" << std::endl;
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
            std::getline(iss, tableName);
            if (tableName.empty()) {
                std::cout << "Error: Table name is required" << std::endl;
            } else {
                displayTable(tableName);
            }
        }
    }
    else if (cmd == "deleteValue") {
        if (currentDatabase.empty()) {
            std::cout << "Error: No database selected. Use 'useDatabase' first." << std::endl;
        } else {
            std::string deleteCommand;
            std::getline(iss, deleteCommand);
            deleteValues(deleteCommand);
        }
    }
    else if (cmd == "deleteTable") {
        if (currentDatabase.empty()) {
            std::cout << "Error: No database selected. Use 'useDatabase' first." << std::endl;
        } else {
            std::string tableName;
            iss >> tableName;
            if (tableName.empty()) {
                std::cout << "Error: Table name is required" << std::endl;
            } else {
                deleteTable(tableName);
            }
        }
    }
    else if (cmd == "deleteDatabase") {
        std::string dbName;
        iss >> dbName;
        if (dbName.empty()) {
            std::cout << "Error: Database name is required" << std::endl;
        } else {
            deleteDatabase(dbName);
        }
    }
    else if (cmd == "editTable") {
        if (currentDatabase.empty()) {
            std::cout << "Error: No database selected. Use 'useDatabase' first." << std::endl;
        } else {
            std::string editCommand;
            std::getline(iss, editCommand);
            editTable(editCommand);
        }
    }
    else if (cmd == "editValue") {
        if (currentDatabase.empty()) {
            std::cout << "Error: No database selected. Use 'useDatabase' first." << std::endl;
        } else {
            std::string editCommand;
            std::getline(iss, editCommand);
            editValue(editCommand);
        }
    }
    else if (cmd == "openSafeBlock") {
        openSafeBlock();
    }
    else if (cmd == "closeSafeBlock") {
        closeSafeBlock();
    }
    else if (cmd == "discardSafeBlock") {
        discardSafeBlock();
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
        {"defaultDB", "defaultDatabase"},
        {"deleteDB", "deleteDatabase"},
        // Safe block aliases
        {"openTransaction", "openSafeBlock"},
        {"openChannel", "openSafeBlock"},
        {"osb", "openSafeBlock"},
        {"closeTransaction", "closeSafeBlock"},
        {"closeChannel", "closeSafeBlock"},
        {"csb", "closeSafeBlock"},
        {"discardTransaction", "discardSafeBlock"},
        {"discardChannel", "discardSafeBlock"},
        {"dsb", "discardSafeBlock"}
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
    std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
    
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
        std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
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

void CLI::displayTable(const std::string& command) {
    std::istringstream iss(command);
    std::string tableName, selectKeyword, valuesKeyword, ifKeyword, columnName, equalOp, value;
    
    // Get table name first
    iss >> tableName;
    
    // Check if this is a filtered display
    if (iss >> selectKeyword) {
        // Parse: "select values if columnName == value"
        if (selectKeyword == "select" && 
            iss >> valuesKeyword && valuesKeyword == "values" &&
            iss >> ifKeyword && ifKeyword == "if" &&
            iss >> columnName && 
            iss >> equalOp && equalOp == "==" &&
            std::getline(iss, value)) {
            
            // Trim whitespace from value
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Parse quoted strings
            std::string parsedValue;
            parseValue(value, parsedValue);
            
            displayFilteredTable(tableName, columnName, parsedValue);
            return;
        } else {
            std::cout << "Error: Invalid display syntax. Use 'displayTable TableName' or" << std::endl;
            std::cout << "'displayTable TableName select values if columnName == value'" << std::endl;
            return;
        }
    }

    // Original display logic for unfiltered display
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
        
        // Store rows for display
        std::vector<std::vector<std::string>> rows;

        // Calculate column widths - accounting for data content too
        std::vector<size_t> colWidths;
        for (const auto& col : columns) {
            colWidths.push_back(col.name.length());
        }

        // First pass: collect data and calculate column widths
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
                std::vector<std::string> row;
                size_t colIndex = 0;
                
                while (std::getline(rowStream, value, *ROW_SEPARATOR)) {
                    row.push_back(value);
                    if (colIndex < colWidths.size()) {
                        colWidths[colIndex] = std::max(colWidths[colIndex], value.length());
                    }
                    colIndex++;
                }
                
                // Pad row if incomplete
                while (row.size() < columns.size()) {
                    row.push_back("");
                }
                
                rows.push_back(row);
            }
        }
        
        // Add padding to column widths
        for (auto& width : colWidths) {
            width += 2; // Add padding
        }

        // Calculate total width
        size_t totalWidth = 1; // Start with 1 for the first border
        for (const auto& width : colWidths) {
            totalWidth += width + 1; // Add column width and the border character
        }
        
        // Function to print horizontal border
        auto printBorder = [&]() {
            std::cout << '+';
            for (const auto& width : colWidths) {
                std::cout << std::string(width, '-') << '+';
            }
            std::cout << std::endl;
        };
        
        // Print top border
        printBorder();
        
        // Print header row
        std::cout << '|';
        for (size_t i = 0; i < columns.size(); i++) {
            std::cout << ' ' << std::left << std::setw(colWidths[i] - 2) << columns[i].name << " |";
        }
        std::cout << std::endl;
        
        // Print header-data separator
        printBorder();
        
        // Print data rows
        if (hasData) {
            for (const auto& row : rows) {
                std::cout << '|';
                for (size_t i = 0; i < columns.size(); i++) {
                    if (i < row.size()) {
                        std::cout << ' ' << std::left << std::setw(colWidths[i] - 2) << row[i] << " |";
                    } else {
                        std::cout << ' ' << std::left << std::setw(colWidths[i] - 2) << "" << " |";
                    }
                }
                std::cout << std::endl;
            }
        } else {
            std::cout << '|' << std::left << std::setw(totalWidth - 2) << " No data in table" << '|' << std::endl;
        }
        
        // Print bottom border
        printBorder();
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

void CLI::displayFilteredTable(const std::string& tableName, const std::string& columnName, const std::string& value) {
    try {
        std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
        
        // Get table columns
        std::vector<Column> columns = getTableColumns(tableName);
        if (columns.empty()) {
            throw std::runtime_error("Table structure not found");
        }

        // Find column index and validate
        int filterColumnIndex = -1;
        for (size_t i = 0; i < columns.size(); i++) {
            if (columns[i].name == columnName) {
                filterColumnIndex = static_cast<int>(i);
                // Validate the value against column type
                if (!validateValue(value, columns[i])) {
                    throw std::runtime_error("Invalid value type for column '" + columnName + "'");
                }
                break;
            }
        }

        if (filterColumnIndex == -1) {
            throw std::runtime_error("Column '" + columnName + "' not found");
        }

        // Get database content
        std::string decrypted = getDecryptedContent(dbPath);
        
        // Process table data
        std::istringstream iss(decrypted);
        std::string line;
        bool inTargetTable = false;
        bool inData = false;
        bool hasData = false;
        
        // Store matching rows for display
        std::vector<std::vector<std::string>> rows;
        
        // Calculate column widths
        std::vector<size_t> colWidths;
        for (const auto& col : columns) {
            colWidths.push_back(col.name.length());
        }

        // First pass: collect filtered data and calculate column widths
        while (std::getline(iss, line)) {
            if (line.substr(0, 6) == "TABLE " && line.substr(6) == tableName) {
                inTargetTable = true;
            }
            else if (inTargetTable && line == TABLE_DATA_START) {
                inData = true;
            }
            else if (inTargetTable && line == TABLE_DATA_END) {
                break;
            }
            else if (inData) {
                std::vector<std::string> rowValues;
                std::string rowValue;
                std::istringstream rowStream(line);
                
                while (std::getline(rowStream, rowValue, *ROW_SEPARATOR)) {
                    rowValues.push_back(rowValue);
                }
                
                // Check if this row matches the filter
                if (filterColumnIndex < static_cast<int>(rowValues.size()) && 
                    rowValues[filterColumnIndex] == value) {
                    hasData = true;
                    
                    // Update column widths
                    for (size_t i = 0; i < rowValues.size() && i < colWidths.size(); i++) {
                        colWidths[i] = std::max(colWidths[i], rowValues[i].length());
                    }
                    
                    rows.push_back(rowValues);
                }
            }
        }
        
        // Add padding to column widths
        for (auto& width : colWidths) {
            width += 2;
        }

        // Calculate total width including borders
        size_t totalWidth = 1; // Start with 1 for first border
        for (const auto& width : colWidths) {
            totalWidth += width + 1; // Width plus divider
        }

        // If no data found, adjust total width if message is longer
        const std::string noDataMsg = " No matching data found ";
        if (!hasData) {
            totalWidth = std::max(totalWidth, noDataMsg.length() + 4); // +4 for borders and spacing
            // Recalculate column widths for single-cell message
            if (columns.size() == 1) {
                colWidths[0] = totalWidth - 2;
            } else {
                size_t extraSpace = totalWidth - 2 - columns.size() + 1;
                size_t baseWidth = extraSpace / columns.size();
                for (auto& width : colWidths) {
                    width = baseWidth;
                }
                // Add remainder to last column
                colWidths.back() += extraSpace % columns.size();
            }
        }
        
        // Print table
        auto printBorder = [&]() {
            std::cout << '+';
            for (const auto& width : colWidths) {
                std::cout << std::string(width, '-') << '+';
            }
            std::cout << std::endl;
        };
        
        printBorder();
        
        // Print header
        std::cout << '|';
        for (size_t i = 0; i < columns.size(); i++) {
            std::cout << ' ' << std::left << std::setw(colWidths[i] - 2) << columns[i].name << " |";
        }
        std::cout << std::endl;
        
        printBorder();
        
        // Print filtered data
        if (hasData) {
            for (const auto& row : rows) {
                std::cout << '|';
                for (size_t i = 0; i < columns.size(); i++) {
                    if (i < row.size()) {
                        std::cout << ' ' << std::left << std::setw(colWidths[i] - 2) << row[i] << " |";
                    } else {
                        std::cout << ' ' << std::left << std::setw(colWidths[i] - 2) << "" << " |";
                    }
                }
                std::cout << std::endl;
            }
        } else {
            if (columns.size() == 1) {
                std::cout << '|' << std::left << std::setw(totalWidth - 2) << noDataMsg << '|' << std::endl;
            } else {
                std::cout << '|';
                for (size_t i = 0; i < columns.size(); i++) {
                    if (i == 0) {
                        std::cout << ' ' << std::left << std::setw(colWidths[i] - 2) << noDataMsg << " |";
                    } else {
                        std::cout << ' ' << std::left << std::setw(colWidths[i] - 2) << "" << " |";
                    }
                }
                std::cout << std::endl;
            }
        }
        
        printBorder();
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

bool CLI::setDefaultDatabase(const std::string& name) {
    std::filesystem::path dbPath = std::filesystem::current_path() / (name + AQADEL_DB_EXT);
    
    if (!std::filesystem::exists(dbPath)) {
        std::cout << "Error: Database '" << name << "' does not exist" << std::endl;
        return false;
    }

    try {
        std::ofstream configFile(DEFAULT_DB_FILE);
        configFile << "defaultDatabase = " << name << std::endl;
        std::cout << "Set '" << name << "' as default database" << std::endl;
        return true;
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
            std::getline(configFile, line);
            
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

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> distribution(0, funFacts.size() - 1);
    
    return "Fun fact: " + funFacts[distribution(generator)];
}

bool CLI::deleteValues(const std::string& command) {
    // Parse command: TableName if ColumnName == Value
    std::regex pattern(R"(\s*(\S+)\s+if\s+(\S+)\s*==\s*(.+))");
    std::smatch matches;
    
    if (!std::regex_search(command, matches, pattern) || matches.size() < 4) {
        std::cout << "Error: Invalid delete syntax. Use 'deleteValue TableName if ColumnName == Value'" << std::endl;
        return false;
    }
    
    std::string tableName = matches[1].str();
    std::string columnName = matches[2].str();
    std::string valueStr = matches[3].str();
    
    // Trim whitespace
    valueStr.erase(0, valueStr.find_first_not_of(" \t"));
    valueStr.erase(valueStr.find_last_not_of(" \t") + 1);
    
    // Parse quoted strings
    std::string parsedValue;
    parseValue(valueStr, parsedValue);
    
    if (!tableExists(tableName)) {
        std::cout << "Error: Table '" << tableName << "' does not exist" << std::endl;
        return false;
    }
    
    // Get table structure
    std::vector<Column> columns = getTableColumns(tableName);
    
    // Find the column index
    int columnIndex = -1;
    for (size_t i = 0; i < columns.size(); i++) {
        if (columns[i].name == columnName) {
            columnIndex = static_cast<int>(i);
            break;
        }
    }
    
    if (columnIndex == -1) {
        std::cout << "Error: Column '" << columnName << "' does not exist in table '" << tableName << "'" << std::endl;
        return false;
    }
    
    // Validate that the value matches the column type
    if (!validateValue(parsedValue, columns[columnIndex])) {
        std::cout << "Error: Value '" << valueStr << "' is not valid for column '" 
                 << columnName << "' with type '" << columns[columnIndex].dataType << "'" << std::endl;
        return false;
    }
    
    try {
        // Read existing database content
        std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
        std::string fileContent = readAndVerifyDatabaseContent(dbPath);
        std::string encryptedContent = extractEncryptedContent(fileContent);
        std::string decrypted = Encryption::decrypt(encryptedContent);
        
        // Process the content and filter out rows to delete
        std::istringstream iss(decrypted);
        std::ostringstream oss;
        std::string line;
        bool inTargetTable = false;
        bool dataStartFound = false;
        bool dataEndFound = false;
        int rowsDeleted = 0;
        std::vector<std::string> keptRows;
        
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
                // Add all kept rows before DATA_END
                for (const auto& keptRow : keptRows) {
                    oss << keptRow << "\n";
                }
                oss << line << "\n";
                dataEndFound = true;
                inTargetTable = false;
            }
            else if (inTargetTable && dataStartFound && !dataEndFound) {
                // Process data row
                std::string rowValue;
                std::istringstream rowStream(line);
                std::vector<std::string> rowValues;
                
                // Split row values by separator
                while (std::getline(rowStream, rowValue, *ROW_SEPARATOR)) {
                    rowValues.push_back(rowValue);
                }
                
                // Check if this row should be deleted
                if (columnIndex < static_cast<int>(rowValues.size()) && 
                    rowValues[columnIndex] == parsedValue) {
                    // Skip this row (delete it)
                    rowsDeleted++;
                } else {
                    // Keep this row
                    keptRows.push_back(line);
                }
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
        
        std::cout << "Deleted " << rowsDeleted << " row(s) from table '" << tableName << "'" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Error: Failed to delete values - " << e.what() << std::endl;
        return false;
    }
}

bool CLI::deleteTable(const std::string& tableName) {
    if (!tableExists(tableName)) {
        std::cout << "Error: Table '" << tableName << "' does not exist" << std::endl;
        return false;
    }
    
    try {
        // Get database content
        std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
        std::string fileContent = readAndVerifyDatabaseContent(dbPath);
        std::string encryptedContent = extractEncryptedContent(fileContent);
        std::string decrypted = Encryption::decrypt(encryptedContent);
        
        // Process the content to remove the table
        std::istringstream iss(decrypted);
        std::ostringstream oss;
        std::string line;
        bool inTargetTable = false;
        
        while (std::getline(iss, line)) {
            if (line.substr(0, 6) == "TABLE ") {
                std::string currentTable = line.substr(6);
                currentTable.erase(0, currentTable.find_first_not_of(" \t"));
                currentTable.erase(currentTable.find_last_not_of(" \t") + 1);
                
                if (currentTable == tableName) {
                    inTargetTable = true;
                    // Don't write this line - start skipping
                    continue;
                } else {
                    inTargetTable = false;
                }
            }
            
            if (inTargetTable) {
                // Skip all lines until END_TABLE
                if (line == "END_TABLE") {
                    inTargetTable = false;
                    continue; // Skip the END_TABLE line too
                }
            } else {
                // Write all lines that are not part of the target table
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
        
        std::cout << "Table '" << tableName << "' deleted successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Error: Failed to delete table - " << e.what() << std::endl;
        return false;
    }
}

bool CLI::deleteDatabase(const std::string& dbName) {
    std::filesystem::path dbPath = std::filesystem::current_path() / (dbName + AQADEL_DB_EXT);
    
    if (!std::filesystem::exists(dbPath)) {
        std::cout << "Error: Database '" << dbName << "' does not exist" << std::endl;
        return false;
    }
    
    // Check if trying to delete current database
    if (dbName == currentDatabase) {
        std::cout << "Warning: Cannot delete the database you're currently using." << std::endl;
        std::string confirm;
        std::cout << "Are you sure you want to continue? This will disconnect you. (y/n): ";
        std::getline(std::cin, confirm);
        
        if (confirm != "y" && confirm != "Y") {
            std::cout << "Database deletion cancelled" << std::endl;
            return false;
        }
    } else {
        // If not the current database, ask for confirmation anyway
        std::string confirm;
        std::cout << "Are you sure you want to delete database '" << dbName << "'? (y/n): ";
        std::getline(std::cin, confirm);
        
        if (confirm != "y" && confirm != "Y") {
            std::cout << "Database deletion cancelled" << std::endl;
            return false;
        }
    }
    
    try {
        // Try to delete the file
        if (!std::filesystem::remove(dbPath)) {
            std::cout << "Error: Failed to delete database file" << std::endl;
            return false;
        }
        
        // If deleted current database, clear the current database variable
        if (dbName == currentDatabase) {
            currentDatabase = "";
            std::cout << "Disconnected from deleted database" << std::endl;
        }
        
        // Check if this was the default database
        if (std::filesystem::exists(DEFAULT_DB_FILE)) {
            std::ifstream configFile(DEFAULT_DB_FILE);
            std::string line;
            std::getline(configFile, line);
            configFile.close();
            
            size_t pos = line.find("=");
            if (pos != std::string::npos) {
                std::string defaultDB = line.substr(pos + 1);
                defaultDB.erase(0, defaultDB.find_first_not_of(" \t"));
                defaultDB.erase(defaultDB.find_last_not_of(" \t") + 1);
                
                if (defaultDB == dbName) {
                    // Remove or update the default database file
                    std::filesystem::remove(DEFAULT_DB_FILE);
                    std::cout << "Default database configuration updated" << std::endl;
                }
            }
        }
        
        std::cout << "Database '" << dbName << "' deleted successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Error: Failed to delete database - " << e.what() << std::endl;
        return false;
    }
}

bool CLI::editTable(const std::string& command) {
    // Parse command parameters: TableName addColumn/removeColumn ColumnName [DataType]
    std::istringstream iss(command);
    std::string tableName, operation, columnName, dataType;
    
    iss >> tableName >> operation >> columnName;
    
    // Remove leading/trailing whitespace
    tableName.erase(0, tableName.find_first_not_of(" \t"));
    tableName.erase(tableName.find_last_not_of(" \t") + 1);
    operation.erase(0, operation.find_first_not_of(" \t"));
    operation.erase(operation.find_last_not_of(" \t") + 1);
    columnName.erase(0, columnName.find_first_not_of(" \t"));
    columnName.erase(columnName.find_last_not_of(" \t") + 1);
    
    // Validate parameters
    if (tableName.empty() || operation.empty() || columnName.empty()) {
        std::cout << "Error: Invalid command syntax. Use 'editTable TableName addColumn/removeColumn ColumnName [DataType]'" << std::endl;
        return false;
    }
    
    // Check if table exists
    if (!tableExists(tableName)) {
        std::cout << "Error: Table '" << tableName << "' does not exist" << std::endl;
        return false;
    }
    
    // Get existing table structure
    std::vector<Column> columns = getTableColumns(tableName);
    
    if (operation == "addColumn") {
        // For addColumn, we need a data type
        iss >> dataType;
        dataType.erase(0, dataType.find_first_not_of(" \t"));
        dataType.erase(dataType.find_last_not_of(" \t") + 1);
        
        if (dataType.empty()) {
            std::cout << "Error: Data type is required when adding a column" << std::endl;
            return false;
        }
        
        // Check if column already exists
        for (const auto& col : columns) {
            if (col.name == columnName) {
                std::cout << "Error: Column '" << columnName << "' already exists in table '" << tableName << "'" << std::endl;
                return false;
            }
        }
        
        // Parse the data type (handle string with length)
        Column newColumn;
        newColumn.name = columnName;
        
        if (dataType.substr(0, 6) == DT_STRING) {
            newColumn.dataType = DT_STRING;
            size_t openBrace = dataType.find('{');
            size_t closeBrace = dataType.find('}');
            
            if (openBrace != std::string::npos && closeBrace != std::string::npos) {
                try {
                    newColumn.stringLength = std::stoi(dataType.substr(openBrace + 1, closeBrace - openBrace - 1));
                }
                catch (...) {
                    std::cout << "Error: Invalid string length for column '" << columnName << "'" << std::endl;
                    return false;
                }
            }
            else {
                std::cout << "Error: String type requires length specification {n}" << std::endl;
                return false;
            }
        }
        else if (dataType == DT_INT || dataType == DT_FLOAT || dataType == DT_BOOL) {
            newColumn.dataType = dataType;
            newColumn.stringLength = 0;
        }
        else {
            std::cout << "Error: Unknown data type: " << dataType << std::endl;
            return false;
        }
        
        // Add the new column to the table structure
        try {
            // Read database content
            std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
            std::string fileContent = readAndVerifyDatabaseContent(dbPath);
            std::string encryptedContent = extractEncryptedContent(fileContent);
            std::string decrypted = Encryption::decrypt(encryptedContent);
            
            // Process and update the content
            std::istringstream contentStream(decrypted);
            std::ostringstream newContentStream;
            std::string line;
            bool inTargetTable = false;
            bool inDataSection = false;
            std::vector<std::string> dataRows;
            
            while (std::getline(contentStream, line)) {
                if (line.substr(0, 6) == "TABLE ") {
                    std::string currentTable = line.substr(6);
                    currentTable.erase(0, currentTable.find_first_not_of(" \t"));
                    currentTable.erase(currentTable.find_last_not_of(" \t") + 1);
                    
                    if (currentTable == tableName) {
                        inTargetTable = true;
                        newContentStream << line << "\n";
                    } else {
                        inTargetTable = false;
                        newContentStream << line << "\n";
                    }
                }
                else if (inTargetTable && line == TABLE_DATA_START) {
                    // Found data section - all columns should be defined by now
                    inDataSection = true;
                    newContentStream << line << "\n";
                }
                else if (inTargetTable && line == TABLE_DATA_END) {
                    // End of data section
                    inDataSection = false;
                    
                    // Write modified data rows with the new column added (with default values)
                    for (const auto& dataRow : dataRows) {
                        newContentStream << dataRow << ROW_SEPARATOR << getDefaultValueForType(newColumn.dataType) << "\n";
                    }
                    
                    newContentStream << line << "\n";
                }
                else if (inTargetTable && inDataSection && !line.empty() && line != TABLE_DATA_END) {
                    // Store data rows to add default value for the new column
                    dataRows.push_back(line);
                }
                else if (inTargetTable && line == "END_TABLE") {
                    // End of table definition - write the new column before this
                    newContentStream << "COLUMN " << newColumn.name << " " << newColumn.dataType;
                    if (newColumn.dataType == DT_STRING) {
                        newContentStream << " " << newColumn.stringLength;
                    }
                    newContentStream << "\n" << line << "\n";
                    inTargetTable = false;
                }
                else {
                    newContentStream << line << "\n";
                }
            }
            
            // Encrypt and write back to file
            std::string newContent = newContentStream.str();
            std::string encrypted = Encryption::encrypt(newContent);
            
            // Add integrity marker
            std::string integrity = Encryption::hashString(encrypted);
            std::string finalContent = std::string(DB_INTEGRITY_MARKER) + integrity + "\n" + encrypted;
            
            std::ofstream outFile(dbPath, std::ios::binary | std::ios::trunc);
            outFile.write(finalContent.c_str(), finalContent.length());
            
            std::cout << "Column '" << columnName << "' added to table '" << tableName << "'" << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cout << "Error: Failed to add column - " << e.what() << std::endl;
            return false;
        }
    }
    else if (operation == "removeColumn") {
        // Check if column exists
        int columnIndex = -1;
        for (size_t i = 0; i < columns.size(); i++) {
            if (columns[i].name == columnName) {
                columnIndex = static_cast<int>(i);
                break;
            }
        }
        
        if (columnIndex == -1) {
            std::cout << "Error: Column '" << columnName << "' does not exist in table '" << tableName << "'" << std::endl;
            return false;
        }
        
        // Don't allow removal of the last column
        if (columns.size() <= 1) {
            std::cout << "Error: Cannot remove the only column in the table" << std::endl;
            return false;
        }
        
        try {
            // Read database content
            std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
            std::string fileContent = readAndVerifyDatabaseContent(dbPath);
            std::string encryptedContent = extractEncryptedContent(fileContent);
            std::string decrypted = Encryption::decrypt(encryptedContent);
            
            // Process and update the content
            std::istringstream contentStream(decrypted);
            std::ostringstream newContentStream;
            std::string line;
            bool inTargetTable = false;
            bool inDataSection = false;
            
            while (std::getline(contentStream, line)) {
                if (line.substr(0, 6) == "TABLE ") {
                    std::string currentTable = line.substr(6);
                    currentTable.erase(0, currentTable.find_first_not_of(" \t"));
                    currentTable.erase(currentTable.find_last_not_of(" \t") + 1);
                    
                    if (currentTable == tableName) {
                        inTargetTable = true;
                        newContentStream << line << "\n";
                    } else {
                        inTargetTable = false;
                        newContentStream << line << "\n";
                    }
                }
                else if (inTargetTable && line.substr(0, 7) == "COLUMN " && line.find(columnName) != std::string::npos) {
                    // Skip this column definition - don't write it to the new content
                    continue;
                }
                else if (inTargetTable && line == TABLE_DATA_START) {
                    // Found data section
                    inDataSection = true;
                    newContentStream << line << "\n";
                }
                else if (inTargetTable && line == TABLE_DATA_END) {
                    // End of data section
                    inDataSection = false;
                    newContentStream << line << "\n";
                }
                else if (inTargetTable && inDataSection && !line.empty() && line != TABLE_DATA_END) {
                    // Process data row - remove the column value
                    std::vector<std::string> values;
                    std::string value;
                    std::istringstream rowStream(line);
                    
                    int currentCol = 0;
                    while (std::getline(rowStream, value, *ROW_SEPARATOR)) {
                        if (currentCol != columnIndex) {
                            values.push_back(value);
                        }
                        currentCol++;
                    }
                    
                    // Write modified row
                    std::string newRow;
                    for (const auto& val : values) {
                        if (!newRow.empty()) newRow += ROW_SEPARATOR;
                        newRow += val;
                    }
                    
                    newContentStream << newRow << "\n";
                }
                else {
                    newContentStream << line << "\n";
                }
            }
            
            // Encrypt and write back to file
            std::string newContent = newContentStream.str();
            std::string encrypted = Encryption::encrypt(newContent);
            
            // Add integrity marker
            std::string integrity = Encryption::hashString(encrypted);
            std::string finalContent = std::string(DB_INTEGRITY_MARKER) + integrity + "\n" + encrypted;
            
            std::ofstream outFile(dbPath, std::ios::binary | std::ios::trunc);
            outFile.write(finalContent.c_str(), finalContent.length());
            
            std::cout << "Column '" << columnName << "' removed from table '" << tableName << "'" << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            std::cout << "Error: Failed to remove column - " << e.what() << std::endl;
            return false;
        }
    }
    else {
        std::cout << "Error: Unknown operation '" << operation << "'. Use 'addColumn' or 'removeColumn'" << std::endl;
        return false;
    }
}

// Helper method to get default values for different data types
std::string CLI::getDefaultValueForType(const std::string& dataType) {
    if (dataType == DT_INT) {
        return "0";
    } else if (dataType == DT_FLOAT) {
        return "0.0";
    } else if (dataType == DT_BOOL) {
        return "false";
    } else if (dataType == DT_STRING) {
        return "";
    }
    return "";
}

bool CLI::editValue(const std::string& command) {
    // Parse command: TableName set Column1 = NewValue if Column2 == Value
    std::regex pattern(R"(\s*(\S+)\s+set\s+(\S+)\s*=\s*([^\s]+)\s+if\s+(\S+)\s*==\s*(.+))");
    std::smatch matches;
    
    if (!std::regex_search(command, matches, pattern) || matches.size() < 6) {
        std::cout << "Error: Invalid editValue syntax. Use 'editValue TableName set ColumnName = NewValue if ColumnName == Value'" << std::endl;
        return false;
    }
    
    std::string tableName = matches[1].str();
    std::string setColumnName = matches[2].str();
    std::string newValueStr = matches[3].str();
    std::string condColumnName = matches[4].str();
    std::string condValueStr = matches[5].str();
    
    // Trim whitespace
    newValueStr.erase(0, newValueStr.find_first_not_of(" \t"));
    newValueStr.erase(newValueStr.find_last_not_of(" \t") + 1);
    condValueStr.erase(0, condValueStr.find_first_not_of(" \t"));
    condValueStr.erase(condValueStr.find_last_not_of(" \t") + 1);
    
    // Parse quoted strings
    std::string parsedNewValue, parsedCondValue;
    parseValue(newValueStr, parsedNewValue);
    parseValue(condValueStr, parsedCondValue);
    
    if (!tableExists(tableName)) {
        std::cout << "Error: Table '" << tableName << "' does not exist" << std::endl;
        return false;
    }
    
    // Get table structure
    std::vector<Column> columns = getTableColumns(tableName);
    
    // Find column indices
    int setColumnIndex = -1, condColumnIndex = -1;
    for (size_t i = 0; i < columns.size(); i++) {
        if (columns[i].name == setColumnName) {
            setColumnIndex = static_cast<int>(i);
        }
        if (columns[i].name == condColumnName) {
            condColumnIndex = static_cast<int>(i);
        }
    }
    
    if (setColumnIndex == -1) {
        std::cout << "Error: Column '" << setColumnName << "' does not exist in table '" << tableName << "'" << std::endl;
        return false;
    }
    
    if (condColumnIndex == -1) {
        std::cout << "Error: Column '" << condColumnName << "' does not exist in table '" << tableName << "'" << std::endl;
        return false;
    }
    
    // Validate that the values match their respective column types
    if (!validateValue(parsedNewValue, columns[setColumnIndex])) {
        std::cout << "Error: New value '" << newValueStr << "' is not valid for column '" 
                 << setColumnName << "' with type '" << columns[setColumnIndex].dataType << "'" << std::endl;
        return false;
    }
    
    if (!validateValue(parsedCondValue, columns[condColumnIndex])) {
        std::cout << "Error: Condition value '" << condValueStr << "' is not valid for column '" 
                 << condColumnName << "' with type '" << columns[condColumnIndex].dataType << "'" << std::endl;
        return false;
    }
    
    try {
        // Read existing database content
        std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
        std::string fileContent = readAndVerifyDatabaseContent(dbPath);
        std::string encryptedContent = extractEncryptedContent(fileContent);
        std::string decrypted = Encryption::decrypt(encryptedContent);
        
        // Process the content and update matching rows
        std::istringstream iss(decrypted);
        std::ostringstream oss;
        std::string line;
        bool inTargetTable = false;
        bool dataStartFound = false;
        bool dataEndFound = false;
        int rowsUpdated = 0;
        
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
                oss << line << "\n";
                dataEndFound = true;
                inTargetTable = false;
            }
            else if (inTargetTable && dataStartFound && !dataEndFound && !line.empty()) {
                // Process data row
                std::vector<std::string> rowValues;
                std::string value;
                std::istringstream rowStream(line);
                
                // Split row values by separator
                while (std::getline(rowStream, value, *ROW_SEPARATOR)) {
                    rowValues.push_back(value);
                }
                
                // Check if this row matches the condition
                if (condColumnIndex < static_cast<int>(rowValues.size()) && 
                    rowValues[condColumnIndex] == parsedCondValue) {
                    // Update the value
                    rowValues[setColumnIndex] = parsedNewValue;
                    rowsUpdated++;
                    
                    // Write the updated row
                    std::string updatedRow;
                    for (const auto& val : rowValues) {
                        if (!updatedRow.empty()) updatedRow += ROW_SEPARATOR;
                        updatedRow += val;
                    }
                    oss << updatedRow << "\n";
                } else {
                    // Keep the row unchanged
                    oss << line << "\n";
                }
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
        
        std::cout << "Updated " << rowsUpdated << " row(s) in table '" << tableName << "'" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Error: Failed to update values - " << e.what() << std::endl;
        return false;
    }
}

bool CLI::openSafeBlock() {
    // Check if safe block is already open
    if (inSafeBlock) {
        std::cout << "Error: Safe block already open. Close the current safe block first." << std::endl;
        return false;
    }

    // Check if database is selected
    if (currentDatabase.empty()) {
        std::cout << "Error: No database selected. Use 'useDatabase' first." << std::endl;
        return false;
    }

    // Create backup of current database state
    if (!createBackup()) {
        std::cout << "Error: Could not create backup of database." << std::endl;
        return false;
    }

    inSafeBlock = true;
    std::cout << "Safe block opened. All changes will be temporary until closed." << std::endl;
    return true;
}

bool CLI::closeSafeBlock() {
    // Check if safe block is open
    if (!inSafeBlock) {
        std::cout << "Error: No safe block is currently open." << std::endl;
        return false;
    }

    // Delete the backup as we're committing changes
    if (!deleteBackup()) {
        std::cout << "Warning: Could not delete backup file." << std::endl;
    }

    inSafeBlock = false;
    safeBlockBackupPath = "";
    std::cout << "Safe block closed. All changes have been saved permanently." << std::endl;
    return true;
}

bool CLI::discardSafeBlock() {
    // Check if safe block is open
    if (!inSafeBlock) {
        std::cout << "Error: No safe block is currently open." << std::endl;
        return false;
    }

    // Restore from backup
    if (!restoreFromBackup()) {
        std::cout << "Error: Could not restore from backup." << std::endl;
        return false;
    }

    inSafeBlock = false;
    safeBlockBackupPath = "";
    std::cout << "Safe block discarded. All changes have been reverted." << std::endl;
    return true;
}

bool CLI::createBackup() {
    try {
        std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
        safeBlockBackupPath = getBackupPath();
        
        // Copy the database file to the backup location
        std::filesystem::copy_file(
            dbPath, 
            safeBlockBackupPath, 
            std::filesystem::copy_options::overwrite_existing
        );
        
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Error creating backup: " << e.what() << std::endl;
        return false;
    }
}

bool CLI::restoreFromBackup() {
    try {
        if (safeBlockBackupPath.empty()) {
            std::cout << "Error: No backup path specified." << std::endl;
            return false;
        }

        std::filesystem::path dbPath = std::filesystem::current_path() / (currentDatabase + AQADEL_DB_EXT);
        
        // First, make sure the target file is writable - remove it if it exists
        if (std::filesystem::exists(dbPath)) {
            std::filesystem::remove(dbPath);
        }
        
        // Create a copy of the backup file
        std::ifstream src(safeBlockBackupPath, std::ios::binary);
        if (!src) {
            throw std::runtime_error("Cannot open backup file for reading");
        }
        
        std::ofstream dst(dbPath, std::ios::binary);
        if (!dst) {
            throw std::runtime_error("Cannot create destination file for writing");
        }
        
        dst << src.rdbuf();
        
        // Close files
        src.close();
        dst.close();
        
        // Delete the backup file after restoring
        deleteBackup();
        
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Error restoring from backup: " << e.what() << std::endl;
        return false;
    }
}

bool CLI::deleteBackup() {
    try {
        if (safeBlockBackupPath.empty()) {
            return true;  // Nothing to delete
        }

        if (std::filesystem::exists(safeBlockBackupPath)) {
            std::filesystem::remove(safeBlockBackupPath);
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Error deleting backup: " << e.what() << std::endl;
        return false;
    }
}

std::string CLI::getBackupPath() {
    // Create a backup filename based on the current database name
    return (std::filesystem::current_path() / (currentDatabase + ".backup" + AQADEL_DB_EXT)).string();
}
