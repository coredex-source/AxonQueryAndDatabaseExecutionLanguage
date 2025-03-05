#pragma once
#include <string>
#include <vector>

struct Column {
    std::string name;
    std::string dataType;
    int stringLength;
};

class CLI {
public:
    CLI() = default;
    void start();
private:
    bool processCommand(const std::string& command);
    bool createDatabase(const std::string& name);
    bool useDatabase(const std::string& name);
    void listDatabases();
    std::string currentDatabase;
    bool createTable(const std::string& command);
    bool parseColumns(const std::string& columnStr, std::vector<Column>& columns);
    void writeTableToDatabase(const std::string& tableName, const std::vector<Column>& columns);
    bool tableExists(const std::string& tableName);
};
