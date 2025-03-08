#pragma once
#include <string>
#include <vector>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

struct Column {
    std::string name;
    std::string dataType;
    int stringLength;
};

class CLI {
public:
    CLI();
    void start();
    static std::string getHiddenInput();
private:
    bool processCommand(const std::string& command);
    bool createDatabase(const std::string& name);
    bool useDatabase(const std::string& name);
    void listDatabases();
    void listTables();
    std::string currentDatabase;
    bool createTable(const std::string& command);
    bool parseColumns(const std::string& columnStr, std::vector<Column>& columns);
    void writeTableToDatabase(const std::string& tableName, const std::vector<Column>& columns);
    bool tableExists(const std::string& tableName);
    void descTable(const std::string& tableName);
    bool insertValues(const std::string& command);
    void displayTable(const std::string& tableName);
    std::vector<Column> getTableColumns(const std::string& tableName);
    bool validateValue(const std::string& value, const Column& column);
    void parseValue(const std::string& value, std::string& parsedValue);
    bool setDefaultDatabase(const std::string& name);
    bool loadDefaultDatabase();
    std::string resolveCommandAlias(const std::string& cmd);
    std::string getRandomFunFact();
    std::string readAndVerifyDatabaseContent(const std::filesystem::path& dbPath);
    std::string extractEncryptedContent(const std::string& fileContent);
    std::string getDecryptedContent(const std::filesystem::path& dbPath);
    bool deleteValues(const std::string& command);
    bool deleteTable(const std::string& tableName);
    bool deleteDatabase(const std::string& dbName);
};
