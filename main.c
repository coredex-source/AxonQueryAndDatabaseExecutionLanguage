#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include "config.h"

#define MAX_NAME_LEN 100

typedef enum { INT, STRING, FLOAT } DataType;

typedef struct {
    char name[MAX_NAME_LEN];
    DataType type;
    int stringSize;
    bool isPrimaryKey;
    bool isRequired;
} Column;


typedef struct {
    int *intValues;
    char **strValues;
    float *floatValues;
} RowData;

typedef struct {
    char tableName[MAX_NAME_LEN];
    char databaseName[MAX_NAME_LEN];
    Column *columns;
    int columnCount;
    RowData *rows;
    int rowCount;
    int maxRows;
} Table;

typedef struct {
    char name[MAX_NAME_LEN];
    Table *tables;
    int tableCount;
    int maxTables;
} Database;

Database *databases = NULL;
int databaseCount = 0;
int maxDatabases = 1;
char currentDatabase[MAX_NAME_LEN] = "";

Table *getTableByName(Database *db, const char *tableName);

void initializeDatabases() {
    databases = malloc(maxDatabases * sizeof(Database));
}

void expandDatabaseList() {
    maxDatabases *= 2;
    databases = realloc(databases, maxDatabases * sizeof(Database));
}

void expandTableList(Database *db) {
    db->maxTables *= 2;
    db->tables = realloc(db->tables, db->maxTables * sizeof(Table));
}

void expandRowList(Table *table) {
    table->maxRows *= 2;
    table->rows = realloc(table->rows, table->maxRows * sizeof(RowData));
    for (int i = table->rowCount; i < table->maxRows; i++) {
        table->rows[i].intValues = malloc(table->columnCount * sizeof(int));
        table->rows[i].strValues = malloc(table->columnCount * sizeof(char *));
        table->rows[i].floatValues = malloc(table->columnCount * sizeof(float));
        for (int j = 0; j < table->columnCount; j++) {
            table->rows[i].strValues[j] = malloc(MAX_NAME_LEN * sizeof(char));
        }
    }
}

Database* getDatabaseByName(const char *name) {
    for (int i = 0; i < databaseCount; i++) {
        if (strcmp(databases[i].name, name) == 0) {
            return &databases[i];
        }
    }
    return NULL;
}

Table *getTableByName(Database *db, const char *tableName) {
    for (int i = 0; i < db->tableCount; i++) {
        if (strcmp(db->tables[i].tableName, tableName) == 0) {
            return &db->tables[i];
        }
    }
    return NULL;
}

void createDatabase(char *dbName) {
    if (databaseCount >= maxDatabases) {
        expandDatabaseList();
    }
    Database *newDb = &databases[databaseCount++];
    strncpy(newDb->name, dbName, MAX_NAME_LEN);
    newDb->tables = malloc(2 * sizeof(Table));
    newDb->tableCount = 0;
    newDb->maxTables = 2;

    strncpy(currentDatabase, dbName, MAX_NAME_LEN);
    printf("Database '%s' created and selected.\n", currentDatabase);
}

void useDatabase(char *dbName) {
    Database *db = getDatabaseByName(dbName);
    if (db) {
        strncpy(currentDatabase, dbName, MAX_NAME_LEN);
        printf("Using database '%s'.\n", currentDatabase);
    } else {
        printf("Error: Database '%s' does not exist.\n", dbName);
    }
}

void createTable(char *command) {
    if (strlen(currentDatabase) == 0) {
        printf("Error: No database selected. Use 'useDatabase' first.\n");
        return;
    }

    Database *db = getDatabaseByName(currentDatabase);
    if (!db) {
        printf("Error: Database '%s' does not exist.\n", currentDatabase);
        return;
    }

    char *tableName = strtok(command, "[");
    if (!tableName) {
        printf("Error: Invalid syntax.\n");
        return;
    }

    if (db->tableCount >= db->maxTables) {
        expandTableList(db);
    }

    Table *newTable = &db->tables[db->tableCount++];
    strncpy(newTable->tableName, tableName, MAX_NAME_LEN);
    strncpy(newTable->databaseName, currentDatabase, MAX_NAME_LEN);
    newTable->columnCount = 0;
    newTable->rowCount = 0;
    newTable->maxRows = 2;
    newTable->rows = malloc(newTable->maxRows * sizeof(RowData));

    char *columnsDef = strtok(NULL, "]");
    if (!columnsDef) {
        printf("Error: Invalid syntax in column definition.\n");
        return;
    }

    char *col = strtok(columnsDef, ",");
    newTable->columns = malloc(MAX_NAME_LEN * sizeof(Column));
    while (col && newTable->columnCount < MAX_NAME_LEN) {
        Column *column = &newTable->columns[newTable->columnCount++];
        char colName[MAX_NAME_LEN], colType[MAX_NAME_LEN];
        int strLen = 0;
        bool isPrimary = false, isRequired = false;

        sscanf(col, "%s %s", colName, colType);
        if (strstr(col, "PRIMARY")) {
            isPrimary = true;
        }
        if (strstr(col, "REQUIRED")) {
            isRequired = true;
        }

        strncpy(column->name, colName, MAX_NAME_LEN);
        column->isPrimaryKey = isPrimary;
        column->isRequired = isRequired;

        if (strcmp(colType, "int") == 0) {
            column->type = INT;
        } else if (strcmp(colType, "float") == 0) {
            column->type = FLOAT;
        } else if (strncmp(colType, "string", 6) == 0) {
            column->type = STRING;
            sscanf(colType, "string{%d}", &strLen);
            column->stringSize = strLen;
        } else {
            printf("Error: Unsupported data type.\n");
            return;
        }

        col = strtok(NULL, ",");
    }

    expandRowList(newTable);
    printf("Table '%s' created in database '%s' with %d columns.\n", newTable->tableName, currentDatabase, newTable->columnCount);
}

void insertValues(char *command) {
    if (strlen(currentDatabase) == 0) {
        printf("Error: No database selected. Use 'useDatabase' first.\n");
        return;
    }

    Database *db = getDatabaseByName(currentDatabase);
    if (!db) {
        printf("Error: Database '%s' not found.\n", currentDatabase);
        return;
    }

    char *tableName = strtok(command, "(");
    if (!tableName) {
        printf("Error: Invalid syntax.\n");
        return;
    }

    Table *table = NULL;
    for (int i = 0; i < db->tableCount; i++) {
        if (strcmp(db->tables[i].tableName, tableName) == 0) {
            table = &db->tables[i];
            break;
        }
    }
    if (!table) {
        printf("Error: Table '%s' not found in database '%s'.\n", tableName, currentDatabase);
        return;
    }

    if (table->rowCount >= table->maxRows) {
        expandRowList(table);
    }

    char *valuesDef = strtok(NULL, ")");
    if (!valuesDef) {
        printf("Error: Values not defined correctly.\n");
        return;
    }

    int valueCount = 0;
    char **values = malloc(sizeof(char*) * table->columnCount);
    char *valueStart = valuesDef;
    int inQuotes = 0;

    for (char *p = valuesDef; *p; p++) {
        if (*p == '"') {
            inQuotes = !inQuotes;
        }
        if (*p == ',' && !inQuotes) {
            *p = '\0';
            values[valueCount++] = valueStart;
            valueStart = p + 1;
        }
    }
    values[valueCount++] = valueStart;

    if (valueCount != table->columnCount) {
        printf("Error: Number of values (%d) does not match the number of columns (%d) in table '%s'.\n", valueCount, table->columnCount, table->tableName);
        free(values);
        return;
    }

    RowData *newRow = &table->rows[table->rowCount++];
    for (int i = 0; i < table->columnCount; i++) {
        Column *col = &table->columns[i];
        char *value = values[i];

        if (col->isRequired && (value == NULL || strlen(value) == 0)) {
            printf("Error: Column '%s' is required.\n", col->name);
            table->rowCount--;
            free(values);
            return;
        }

        if (col->isPrimaryKey) {
            for (int j = 0; j < table->rowCount - 1; j++) {
                if ((col->type == INT && table->rows[j].intValues[i] == atoi(value)) ||
                    (col->type == FLOAT && table->rows[j].floatValues[i] == atof(value)) ||
                    (col->type == STRING && strcmp(table->rows[j].strValues[i], value) == 0)) {
                    printf("Error: Duplicate value for primary key '%s'.\n", col->name);
                    table->rowCount--;
                    free(values);
                    return;
                }
            }
        }

        if (col->type == INT) {
            newRow->intValues[i] = atoi(value);
        } else if (col->type == FLOAT) {
            newRow->floatValues[i] = atof(value);
        } else if (col->type == STRING) {
            strncpy(newRow->strValues[i], value, col->stringSize);
        }
    }

    free(values);
    printf("Values inserted into table '%s'.\n", table->tableName);
}

void displayTable(char *tableName) {
    Database *db = getDatabaseByName(currentDatabase);
    if (!db) {
        printf("Error: Database '%s' does not exist.\n", currentDatabase);
        return;
    }

    Table *table = getTableByName(db, tableName);
    if (!table) {
        printf("Error: Table '%s' not found in database '%s'.\n", tableName, currentDatabase);
        return;
    }

    int columnWidths[table->columnCount];
    for (int i = 0; i < table->columnCount; i++) {
        columnWidths[i] = strlen(table->columns[i].name);

        for (int row = 0; row < table->rowCount; row++) {
            int valueLength = 0;
            Column *column = &table->columns[i];
            
            if (column->type == INT) {
                valueLength = snprintf(NULL, 0, "%d", table->rows[row].intValues[i]);
            } else if (column->type == FLOAT) {
                valueLength = snprintf(NULL, 0, "%.2f", table->rows[row].floatValues[i]);
            } else if (column->type == STRING) {
                valueLength = strlen(table->rows[row].strValues[i]);
            }

            if (valueLength > columnWidths[i]) {
                columnWidths[i] = valueLength;
            }
        }
    }

    printf("\n");
    for (int i = 0; i < table->columnCount; i++) {
        for (int j = 0; j < columnWidths[i] + 3; j++) printf("-");
    }
    printf("-");
    printf("\n");

    for (int i = 0; i < table->columnCount; i++) {
        printf("| %-*s ", columnWidths[i], table->columns[i].name);
    }
    printf("|\n");

    for (int i = 0; i < table->columnCount; i++) {
        for (int j = 0; j < columnWidths[i] + 3; j++) printf("-");
    }
    printf("-");
    printf("\n");

    for (int row = 0; row < table->rowCount; row++) {
        for (int col = 0; col < table->columnCount; col++) {
            Column *column = &table->columns[col];
            if (column->type == INT) {
                printf("| %-*d ", columnWidths[col], table->rows[row].intValues[col]);
            } else if (column->type == FLOAT) {
                printf("| %-*.2f ", columnWidths[col], table->rows[row].floatValues[col]);
            } else if (column->type == STRING) {
                printf("| %-*s ", columnWidths[col], table->rows[row].strValues[col]);
            }
        }
        printf("|\n");
    }

    for (int i = 0; i < table->columnCount; i++) {
        for (int j = 0; j < columnWidths[i] + 3; j++) printf("-");
    }
    printf("-");
    printf("\n");

}

void editTable(char *command) {
    char *tableName = strtok(command, " ");
    char *operation = strtok(NULL, " ");
    char *columnName = strtok(NULL, " ");
    char *dataTypeStr = strtok(NULL, " ");

    if (!tableName || !operation || !columnName) {
        printf("Error: Invalid syntax.\n");
        return;
    }

    Database *db = getDatabaseByName(currentDatabase);
    if (!db) {
        printf("Error: No database selected or database not found.\n");
        return;
    }

    Table *table = getTableByName(db, tableName);
    if (!table) {
        printf("Error: Table '%s' not found.\n", tableName);
        return;
    }

    if (strcmp(operation, "addColumn") == 0) {
        if (!dataTypeStr) {
            printf("Error: Datatype required when adding a column.\n");
            return;
        }

        table->columns = realloc(table->columns, (table->columnCount + 1) * sizeof(Column));

        Column *newColumn = &table->columns[table->columnCount++];
        strncpy(newColumn->name, columnName, MAX_NAME_LEN);

        if (strcmp(dataTypeStr, "int") == 0) {
            newColumn->type = INT;
        } else if (strcmp(dataTypeStr, "float") == 0) {
            newColumn->type = FLOAT;
        } else if (strncmp(dataTypeStr, "string", 6) == 0) {
            newColumn->type = STRING;
            sscanf(dataTypeStr, "string{%d}", &newColumn->stringSize);
        } else {
            printf("Error: Unsupported data type.\n");
            return;
        }

        for (int i = 0; i < table->rowCount; i++) {
            table->rows[i].intValues = realloc(table->rows[i].intValues, table->columnCount * sizeof(int));
            table->rows[i].strValues = realloc(table->rows[i].strValues, table->columnCount * sizeof(char *));
            table->rows[i].floatValues = realloc(table->rows[i].floatValues, table->columnCount * sizeof(float));

            if (newColumn->type == INT) {
                table->rows[i].intValues[table->columnCount - 1] = 0;
            } else if (newColumn->type == FLOAT) {
                table->rows[i].floatValues[table->columnCount - 1] = 0.0;
            } else if (newColumn->type == STRING) {
                table->rows[i].strValues[table->columnCount - 1] = malloc(newColumn->stringSize * sizeof(char));
                strcpy(table->rows[i].strValues[table->columnCount - 1], "");
            }
        }

        printf("Column '%s' added to table '%s'.\n", columnName, tableName);

    } else if (strcmp(operation, "removeColumn") == 0) {
        int colIndex = -1;
        for (int i = 0; i < table->columnCount; i++) {
            if (strcmp(table->columns[i].name, columnName) == 0) {
                colIndex = i;
                break;
            }
        }
        if (colIndex == -1) {
            printf("Error: Column '%s' not found.\n", columnName);
            return;
        }

        for (int i = 0; i < table->rowCount; i++) {
            if (table->columns[colIndex].type == STRING) {
                free(table->rows[i].strValues[colIndex]);
            }
            for (int j = colIndex; j < table->columnCount - 1; j++) {
                table->rows[i].intValues[j] = table->rows[i].intValues[j + 1];
                table->rows[i].strValues[j] = table->rows[i].strValues[j + 1];
                table->rows[i].floatValues[j] = table->rows[i].floatValues[j + 1];
            }
        }

        for (int i = colIndex; i < table->columnCount - 1; i++) {
            table->columns[i] = table->columns[i + 1];
        }
        table->columnCount--;

        printf("Column '%s' removed from table '%s'.\n", columnName, tableName);
    } else {
        printf("Error: Invalid operation. Use 'addColumn' or 'removeColumn'.\n");
    }
}

void deleteTable(char *tableName) {
    if (strlen(currentDatabase) == 0) {
        printf("Error: No database selected.\n");
        return;
    }
    Database *db = getDatabaseByName(currentDatabase);
    if (!db) {
        printf("Error: No database selected or database not found.\n");
        return;
    }

    int tableIndex = -1;
    for (int i = 0; i < db->tableCount; i++) {
        if (strcmp(db->tables[i].tableName, tableName) == 0) {
            tableIndex = i;
            break;
        }
    }

    if (tableIndex == -1) {
        printf("Error: Table '%s' not found in database '%s'.\n", tableName, currentDatabase);
        return;
    }

    Table *table = &db->tables[tableIndex];
    for (int i = 0; i < table->rowCount; i++) {
        for (int j = 0; j < table->columnCount; j++) {
            if (table->columns[j].type == STRING) {
                free(table->rows[i].strValues[j]);
            }
        }
        free(table->rows[i].intValues);
        free(table->rows[i].strValues);
        free(table->rows[i].floatValues);
    }
    free(table->rows);
    free(table->columns);

    for (int i = tableIndex; i < db->tableCount - 1; i++) {
        db->tables[i] = db->tables[i + 1];
    }
    db->tableCount--;

    printf("Table '%s' deleted from database '%s'.\n", tableName, currentDatabase);
}

void unloadDatabase(char *dbName) {
    int dbIndex = -1;
    for (int i = 0; i < databaseCount; i++) {
        if (strcmp(databases[i].name, dbName) == 0) {
            dbIndex = i;
            break;
        }
    }

    if (dbIndex == -1) {
        printf("Error: Database '%s' not found.\n", dbName);
        return;
    }

    Database *db = &databases[dbIndex];
    for (int i = 0; i < db->tableCount; i++) {
        Table *table = &db->tables[i];
        for (int j = 0; j < table->rowCount; j++) {
            for (int k = 0; k < table->columnCount; k++) {
                if (table->columns[k].type == STRING) {
                    free(table->rows[j].strValues[k]);
                }
            }
            free(table->rows[j].intValues);
            free(table->rows[j].strValues);
            free(table->rows[j].floatValues);
        }
        free(table->rows);
    }
    free(db->tables);

    for (int i = dbIndex; i < databaseCount - 1; i++) {
        databases[i] = databases[i + 1];
    }
    databaseCount--;

    if (strcmp(currentDatabase, dbName) == 0) {
        currentDatabase[0] = '\0';
    }

    printf("Database '%s' unloaded.\n", dbName);
}

void deleteDatabase(char *dbName) {
    int dbIndex = -1;
    for (int i = 0; i < databaseCount; i++) {
        if (strcmp(databases[i].name, dbName) == 0) {
            dbIndex = i;
            break;
        }
    }

    if (dbIndex == -1) {
        printf("Error: Database '%s' not found.\n", dbName);
        return;
    }

    char confirmation;
    printf("Are you sure you want to delete the database '%s'? (y/n): ", dbName);
    scanf(" %c", &confirmation);
    getchar();
    if (confirmation != 'y' && confirmation != 'Y') {
        printf("Database deletion canceled.\n");
        return;
    }

    Database *db = &databases[dbIndex];
    for (int i = 0; i < db->tableCount; i++) {
        Table *table = &db->tables[i];
        for (int j = 0; j < table->rowCount; j++) {
            for (int k = 0; k < table->columnCount; k++) {
                if (table->columns[k].type == STRING) {
                    free(table->rows[j].strValues[k]);
                }
            }
            free(table->rows[j].intValues);
            free(table->rows[j].strValues);
            free(table->rows[j].floatValues);
        }
        free(table->rows);
    }
    free(db->tables);

    for (int i = dbIndex; i < databaseCount - 1; i++) {
        databases[i] = databases[i + 1];
    }
    databaseCount--;

    if (strcmp(currentDatabase, dbName) == 0) {
        currentDatabase[0] = '\0';
    }

    char filePath[MAX_NAME_LEN + 10];
    snprintf(filePath, sizeof(filePath), "data/%s.bin", dbName);
    if (unlink(filePath) == 0) {
        printf("File '%s' deleted successfully.\n", filePath);
    } else {
        perror("Error deleting file");
    }

    printf("Database '%s' deleted.\n", dbName);
}

void editValue(char *command) {
    if (strlen(currentDatabase) == 0) {
        printf("Error: No database selected.\n");
        return;
    }

    Database *db = getDatabaseByName(currentDatabase);
    if (!db) {
        printf("Error: Database '%s' not found.\n", currentDatabase);
        return;
    }

    char *tableName = strtok(command, " ");
    Table *table = getTableByName(db, tableName);
    if (!table) {
        printf("Error: Table '%s' not found in database '%s'.\n", tableName, currentDatabase);
        return;
    }

    char *setClause = strtok(NULL, " ");
    if (!setClause || strcmp(setClause, "set") != 0) {
        printf("Error: Invalid syntax. Expected 'set' after table name.\n");
        return;
    }

    char *columnName = strtok(NULL, " ");
    char *equalsSign = strtok(NULL, " ");
    char *newValueStr = strtok(NULL, "\"");
    if (!columnName || !equalsSign || !newValueStr || strcmp(equalsSign, "=") != 0) {
        printf("Error: Invalid syntax. Expected 'ColumnName = \"NewValue\"'.\n");
        return;
    }

    int targetColumnIndex = -1;
    for (int i = 0; i < table->columnCount; i++) {
        if (strcmp(table->columns[i].name, columnName) == 0) {
            targetColumnIndex = i;
            break;
        }
    }
    if (targetColumnIndex == -1) {
        printf("Error: Column '%s' not found in table '%s'.\n", columnName, table->tableName);
        return;
    }

    Column *targetColumn = &table->columns[targetColumnIndex];
    int intValue;
    float floatValue;
    char strValue[MAX_NAME_LEN];

    if (targetColumn->type == INT) {
        intValue = atoi(newValueStr);
    } else if (targetColumn->type == FLOAT) {
        floatValue = atof(newValueStr);
    } else if (targetColumn->type == STRING) {
        strncpy(strValue, newValueStr, targetColumn->stringSize);
    }

    char *ifClause = strtok(NULL, " ");
    int conditionalUpdate = (ifClause && strcmp(ifClause, "if") == 0);
    int conditionColumnIndex = -1;
    int conditionIntValue;
    float conditionFloatValue;
    char conditionStrValue[MAX_NAME_LEN];

    if (conditionalUpdate) {
        char *conditionColumn = strtok(NULL, " ");
        char *conditionEquals = strtok(NULL, " ");
        char *conditionValue = strtok(NULL, "\"");
        if (!conditionColumn || !conditionEquals || !conditionValue || strcmp(conditionEquals, "==") != 0) {
            printf("Error: Invalid syntax in 'if' clause. Expected 'if ColumnName == \"Value\"'.\n");
            return;
        }

        for (int i = 0; i < table->columnCount; i++) {
            if (strcmp(table->columns[i].name, conditionColumn) == 0) {
                conditionColumnIndex = i;
                break;
            }
        }
        if (conditionColumnIndex == -1) {
            printf("Error: Condition column '%s' not found in table '%s'.\n", conditionColumn, table->tableName);
            return;
        }

        Column *conditionColumnType = &table->columns[conditionColumnIndex];
        if (conditionColumnType->type == INT) {
            conditionIntValue = atoi(conditionValue);
        } else if (conditionColumnType->type == FLOAT) {
            conditionFloatValue = atof(conditionValue);
        } else if (conditionColumnType->type == STRING) {
            strncpy(conditionStrValue, conditionValue, conditionColumnType->stringSize);
        }
    }

    for (int i = 0; i < table->rowCount; i++) {
        int updateRow = 1;

        if (conditionalUpdate) {
            Column *condCol = &table->columns[conditionColumnIndex];
            if (condCol->type == INT && table->rows[i].intValues[conditionColumnIndex] != conditionIntValue) {
                updateRow = 0;
            } else if (condCol->type == FLOAT && table->rows[i].floatValues[conditionColumnIndex] != conditionFloatValue) {
                updateRow = 0;
            } else if (condCol->type == STRING && strcmp(table->rows[i].strValues[conditionColumnIndex], conditionStrValue) != 0) {
                updateRow = 0;
            }
        }

        if (updateRow) {
            if (targetColumn->type == INT) {
                table->rows[i].intValues[targetColumnIndex] = intValue;
            } else if (targetColumn->type == FLOAT) {
                table->rows[i].floatValues[targetColumnIndex] = floatValue;
            } else if (targetColumn->type == STRING) {
                strncpy(table->rows[i].strValues[targetColumnIndex], strValue, targetColumn->stringSize);
            }
        }
    }

    printf("Column '%s' updated in table '%s'.\n", columnName, table->tableName);
}

void saveDatabaseToFile() {
    for (int i = 0; i < databaseCount; i++) {
        char fileName[MAX_NAME_LEN + 10];
        snprintf(fileName, sizeof(fileName), "data/%s.bin", databases[i].name);
        printf("Attempting to save database to file: %s\n", fileName);
        FILE *file = fopen(fileName, "wb");
        if (!file) {
            perror("Error opening file for saving");
            continue;
        }

        int singleDatabaseCount = 1;
        fwrite(&singleDatabaseCount, sizeof(int), 1, file);

        fwrite(databases[i].name, sizeof(char), MAX_NAME_LEN, file);
        
        fwrite(&databases[i].tableCount, sizeof(int), 1, file);

        for (int j = 0; j < databases[i].tableCount; j++) {
            Table *table = &databases[i].tables[j];

            fwrite(table->tableName, sizeof(char), MAX_NAME_LEN, file);

            fwrite(&table->columnCount, sizeof(int), 1, file);

            for (int k = 0; k < table->columnCount; k++) {
                Column *column = &table->columns[k];
                fwrite(column, sizeof(Column), 1, file);

                fwrite(&column->isPrimaryKey, sizeof(int), 1, file);
                fwrite(&column->isRequired, sizeof(int), 1, file);
            }

            fwrite(&table->rowCount, sizeof(int), 1, file);

            for (int r = 0; r < table->rowCount; r++) {
                for (int c = 0; c < table->columnCount; c++) {
                    Column *col = &table->columns[c];
                    if (col->type == INT) {
                        fwrite(&table->rows[r].intValues[c], sizeof(int), 1, file);
                    } else if (col->type == FLOAT) {
                        fwrite(&table->rows[r].floatValues[c], sizeof(float), 1, file);
                    } else if (col->type == STRING) {
                        fwrite(table->rows[r].strValues[c], sizeof(char), col->stringSize, file);
                    }
                }
            }
        }

        fclose(file);
        printf("Database '%s' saved to file '%s'.\n", databases[i].name, fileName);
    }
}

void loadDatabaseFromFile(const char *database_file) {
    char filePath[MAX_NAME_LEN + 10];
    snprintf(filePath, sizeof(filePath), "data/%s", database_file);

    FILE *file = fopen(filePath, "rb");
    if (!file) {
        printf("Error: Could not open file '%s' for loading. A new database will be created.\n", database_file);
        return;
    }

    int fileDatabaseCount;
    if (fread(&fileDatabaseCount, sizeof(int), 1, file) != 1) {
        printf("Error: Could not read database count in file '%s'.\n", database_file);
        fclose(file);
        return;
    }

    databases = realloc(databases, (databaseCount + fileDatabaseCount) * sizeof(Database));
    if (!databases) {
        printf("Error: Memory allocation failed for databases.\n");
        fclose(file);
        return;
    }

    for (int i = 0; i < fileDatabaseCount; i++) {
        Database *db = &databases[databaseCount + i];

        if (fread(db->name, sizeof(char), MAX_NAME_LEN, file) != MAX_NAME_LEN) {
            printf("Error: Could not read database name for database %d in file '%s'.\n", i, database_file);
            fclose(file);
            return;
        }
        printf("Loaded database name: %s\n", db->name);

        if (fread(&db->tableCount, sizeof(int), 1, file) != 1) {
            printf("Error: Could not read table count for database %s.\n", db->name);
            fclose(file);
            return;
        }

        db->tables = malloc(db->tableCount * sizeof(Table));
        if (!db->tables) {
            printf("Error: Memory allocation failed for tables in database %s.\n", db->name);
            fclose(file);
            return;
        }

        for (int j = 0; j < db->tableCount; j++) {
            Table *table = &db->tables[j];

            if (fread(table->tableName, sizeof(char), MAX_NAME_LEN, file) != MAX_NAME_LEN) {
                printf("Error: Could not read table name for table %d in database %s.\n", j, db->name);
                fclose(file);
                return;
            }
            printf("Loaded table name: %s\n", table->tableName);

            if (fread(&table->columnCount, sizeof(int), 1, file) != 1) {
                printf("Error: Could not read column count for table %s in database %s.\n", table->tableName, db->name);
                fclose(file);
                return;
            }

            table->columns = malloc(table->columnCount * sizeof(Column));
            if (!table->columns) {
                printf("Error: Memory allocation failed for columns in table %s.\n", table->tableName);
                fclose(file);
                return;
            }

            for (int k = 0; k < table->columnCount; k++) {
                Column *column = &table->columns[k];
                if (fread(column, sizeof(Column), 1, file) != 1) {
                    printf("Error: Could not read column data for column %d in table %s of database %s.\n", k, table->tableName, db->name);
                    fclose(file);
                    return;
                }

                if (fread(&column->isPrimaryKey, sizeof(int), 1, file) != 1 ||
                    fread(&column->isRequired, sizeof(int), 1, file) != 1) {
                    printf("Error: Could not read primaryKey or isRequired for column %d in table %s.\n", k, table->tableName);
                    fclose(file);
                    return;
                }
                printf("Loaded column name: %s, type: %d, primaryKey: %d, isRequired: %d\n", column->name, column->type, column->isPrimaryKey, column->isRequired);
            }

            if (fread(&table->rowCount, sizeof(int), 1, file) != 1) {
                printf("Error: Could not read row count for table %s in database %s.\n", table->tableName, db->name);
                fclose(file);
                return;
            }
            table->maxRows = table->rowCount;

            table->rows = malloc(table->rowCount * sizeof(RowData));
            if (!table->rows) {
                printf("Error: Memory allocation failed for rows in table %s of database %s.\n", table->tableName, db->name);
                fclose(file);
                return;
            }

            for (int r = 0; r < table->rowCount; r++) {
                table->rows[r].intValues = malloc(table->columnCount * sizeof(int));
                table->rows[r].strValues = malloc(table->columnCount * sizeof(char *));
                table->rows[r].floatValues = malloc(table->columnCount * sizeof(float));

                for (int c = 0; c < table->columnCount; c++) {
                    Column *col = &table->columns[c];
                    if (col->type == INT) {
                        fread(&table->rows[r].intValues[c], sizeof(int), 1, file);
                    } else if (col->type == FLOAT) {
                        fread(&table->rows[r].floatValues[c], sizeof(float), 1, file);
                    } else if (col->type == STRING) {
                        table->rows[r].strValues[c] = malloc(col->stringSize * sizeof(char));
                        fread(table->rows[r].strValues[c], sizeof(char), col->stringSize, file);
                    }
                }
            }
        }
    }

    databaseCount += fileDatabaseCount;
    fclose(file);
    printf("Database loaded from '%s'.\n", database_file);
}

void loadDatabase(char *databaseName) {
    char fileName[MAX_NAME_LEN + 4];
    snprintf(fileName, sizeof(fileName), "%s.bin", databaseName);
    loadDatabaseFromFile(fileName);
    useDatabase(databaseName);
}

void loadAllDatabases() {
    DIR *dir;
    struct dirent *entry;

    dir = opendir("data");
    if (!dir) {
        perror("Error opening directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".bin") != NULL) {
            printf("Loading database from file: %s\n", entry->d_name);
            loadDatabaseFromFile(entry->d_name);
        }
    }

    closedir(dir);
    printf("\nAll databases loaded.\n");
}

void listDatabases() {
    DIR *dir;
    struct dirent *entry;

    dir = opendir("data");
    if (!dir) {
        perror("Error opening directory");
        return;
    }
    printf("\nDatabase List:\n");
    printf("--------------\n");
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".bin") != NULL) {
            char dbName[256];
            strncpy(dbName, entry->d_name, sizeof(dbName) - 1);
            dbName[sizeof(dbName) - 1] = '\0';
            char *extPos = strstr(dbName, ".bin");
            if (extPos != NULL) {
                *extPos = '\0';
            }

            printf("- %s\n", dbName);
        }
    }

    closedir(dir);
    printf("--------------\n");
}

void deleteValue(char *command) {
    if (strlen(currentDatabase) == 0) {
        printf("Error: No database selected. Use 'useDatabase' first.\n");
        return;
    }

    char *tableName = strtok(command, " ");
    if (!tableName) {
        printf("Error: Invalid syntax. Usage: deleteValue TableName if ColumnName == Value\n");
        return;
    }

    char *ifKeyword = strtok(NULL, " ");
    if (!ifKeyword || strcmp(ifKeyword, "if") != 0) {
        printf("Error: Invalid syntax. Missing 'if' keyword.\n");
        return;
    }

    char *columnName = strtok(NULL, " ");
    if (!columnName) {
        printf("Error: Invalid syntax. Missing column name.\n");
        return;
    }

    char *equalOperator = strtok(NULL, " ");
    if (!equalOperator || strcmp(equalOperator, "==") != 0) {
        printf("Error: Invalid syntax. Use '==' to specify the condition.\n");
        return;
    }

    char *valueStr = strtok(NULL, " ");
    if (!valueStr) {
        printf("Error: Invalid syntax. Missing value to match.\n");
        return;
    }

    bool isStringValue = valueStr[0] == '"' && valueStr[strlen(valueStr) - 1] == '"';
    if (isStringValue) {
        valueStr[strlen(valueStr) - 1] = '\0';
        valueStr++;
    }

    Database *db = getDatabaseByName(currentDatabase);
    if (!db) {
        printf("Error: Database '%s' does not exist.\n", currentDatabase);
        return;
    }

    Table *table = getTableByName(db, tableName);
    if (!table) {
        printf("Error: Table '%s' not found in database '%s'.\n", tableName, currentDatabase);
        return;
    }

    int columnIndex = -1;
    DataType columnType;
    for (int i = 0; i < table->columnCount; i++) {
        if (strcmp(table->columns[i].name, columnName) == 0) {
            columnIndex = i;
            columnType = table->columns[i].type;
            break;
        }
    }

    if (columnIndex == -1) {
        printf("Error: Column '%s' not found in table '%s'.\n", columnName, tableName);
        return;
    }

    int intValue;
    float floatValue;
    char strValue[MAX_NAME_LEN];

    if (columnType == INT) {
        intValue = atoi(valueStr);
    } else if (columnType == FLOAT) {
        floatValue = atof(valueStr);
    } else if (columnType == STRING) {
        strncpy(strValue, valueStr, MAX_NAME_LEN);
    } else {
        printf("Error: Unsupported data type in condition.\n");
        return;
    }

    int deletedRows = 0;
    for (int row = 0; row < table->rowCount; row++) {
        bool match = false;

        if (columnType == INT && table->rows[row].intValues[columnIndex] == intValue) {
            match = true;
        } else if (columnType == FLOAT && table->rows[row].floatValues[columnIndex] == floatValue) {
            match = true;
        } else if (columnType == STRING && strcmp(table->rows[row].strValues[columnIndex], strValue) == 0) {
            match = true;
        }

        if (match) {
            for (int j = row; j < table->rowCount - 1; j++) {
                table->rows[j] = table->rows[j + 1];
            }
            table->rowCount--;
            deletedRows++;
            row--;
        }
    }

    if (deletedRows > 0) {
        printf("%d row(s) deleted from table '%s' where '%s' == '%s'.\n", deletedRows, tableName, columnName, isStringValue ? strValue : valueStr);
    } else {
        printf("No rows found in table '%s' where '%s' == '%s'.\n", tableName, columnName, isStringValue ? strValue : valueStr);
    }
}

void listTables() {
    if (strlen(currentDatabase) == 0) {
        printf("Error: No database selected. Use 'useDatabase' first.\n");
        return;
    }

    Database *db = getDatabaseByName(currentDatabase);
    if (!db) {
        printf("Error: Database '%s' does not exist.\n", currentDatabase);
        return;
    }

    if (db->tableCount == 0) {
        printf("No tables found in database '%s'.\n", currentDatabase);
        return;
    }

    printf("\nTable list of database: '%s':\n", currentDatabase);
    printf("---------------------------------\n");
    for (int i = 0; i < db->tableCount; i++) {
        printf("- %s\n", db->tables[i].tableName);
    }
    printf("---------------------------------\n");
}

void help(){
    printf("\n------------------------------------------------------------ Help Menu ------------------------------------------------------------\n");
    printf("Available commands:\n");
    printf("\ncreateDatabase DatabaseName  -  Creates a database with a specified name and by default switches to the database.\n");
    printf("\nuseDatabase DatabaseName  -  Select a database to use\n");
    printf("\nlistDatabases  -  Shows the list of saved/loadable databases.\n");
    printf("\ncreateTable TableName[ColumnName datatype, ...]  -  Creates a table within a database.\nNote: for string datatypes: string{length}\n");
    printf("\nlistTables  -  Shows a list of tables in the currently selected database.\n");
    printf("\ninsertValues TableName(Value, ...)  -  Appends a value to a specified table.\nNote: for string put the value in \"\".\n");
    printf("\ndisplayTable TableName  -  Displays a specified table.\n");
    printf("\ndeleteValue TableName if ColumnName == Value  -  Deletes a value from a specified table.\nNote: Value should be in \"\" if string.\n");
    printf("\neditTable TableName addColumn/removeColumn ColumnName Datatype  -  Adds or removes a column in a specified table.\nNote: Datatype required only when adding.\n");
    printf("\ndeleteTable TableName  -  Deletes a table.\n");
    printf("\ndeleteDatabase DatabaseName  -  Deletes a database.\n");
    printf("\neditValue TableName set ColumnName = NewValue if ColumnName == Value  -  Edit the value of a specified column in a specified table.\nNote: Value and NewValue should be in \"\" if string.\n");
    printf("\nloadDatabase DatabaseName  -  Manually load a specific saved database.\n");
    printf("\nunloadDatabase DatabaseName  -  Unloads a database from memory.\n");
    printf("\nloadAllDatabases  -  Manually load all saved databases.\n");
    printf("\ntoggleLoadDatabase  -  Toggles automatic loading at boot of databases. || Alais - TLD\n");
    printf("\ncommitAll  -  Saves all changes done till now permanently.\n");
    printf("\nexit  -  Exits program while saving all changes.\n");
    printf("\nexit --no-save  -  Exits program while discarding all changes.\n");
    printf("\nhelp  -  Displays this message.\n");
    printf("-----------------------------------------------------------------------------------------------------------------------------------\n");
}
void ToggleAutomaticLoading(){
    ConfigNode *configList = NULL;
    loadConfig("config.txt", &configList);
    const char * ConfigValue;
    ConfigValue = checkConfig(configList, "AutomaticallyLoadDatabases");
    if(strcmp(ConfigValue, "True") == 0){
        ChangeConfigOption("False");
        printf("Disabled Automatic database loading.\n");
    } else {
        ChangeConfigOption("True");
        printf("Enabled Automatic database loading.\n");
    }
    freeConfigList(configList);
}

typedef struct CommandNode {
    char *command;
    struct CommandNode *next;
} CommandNode;

void freeCommandList(CommandNode *head) {
    CommandNode *temp;
    while (head != NULL) {
        temp = head;
        head = head->next;
        free(temp->command);
        free(temp);
    }
}

ssize_t custom_getline(char **lineptr, size_t *n, FILE *stream) {
    size_t pos = 0;
    int c;

    if (*lineptr == NULL || *n == 0) {
        *n = 128;
        *lineptr = malloc(*n);
        if (*lineptr == NULL) return -1;
    }

    while ((c = fgetc(stream)) != EOF) {
        if (pos + 1 >= *n) {
            *n *= 2;
            char *new_ptr = realloc(*lineptr, *n);
            if (new_ptr == NULL) return -1;
            *lineptr = new_ptr;
        }
        (*lineptr)[pos++] = c;
        if (c == '\n') break;
    }

    if (pos == 0 && c == EOF) return -1;

    (*lineptr)[pos] = '\0';
    return pos;
}

int main() {
    initializeDatabases();

    ConfigNode *configList = NULL;
    loadConfig("config.txt", &configList);
    const char *ConfigValue;
    ConfigValue = checkConfig(configList, "AutomaticallyLoadDatabases");
    if (strcmp(ConfigValue, "True") == 0) {
        loadAllDatabases();
    } else {
        printf("Automatic database loading is disabled in config.\n");
    }
    freeConfigList(configList);
    
    CommandNode *commandList = NULL;
    CommandNode *currentCommand = NULL;

    while (1) {
        printf("\nEnter command: ");
        
        char *tempCommand = NULL;
        size_t len = 0;
        ssize_t read = custom_getline(&tempCommand, &len, stdin);
        
        if (read > 0 && tempCommand[read - 1] == '\n') {
            tempCommand[read - 1] = '\0';
        }
        
        CommandNode *newCommandNode = (CommandNode *)malloc(sizeof(CommandNode));
        newCommandNode->command = tempCommand;
        newCommandNode->next = NULL;
        
        if (commandList == NULL) {
            commandList = newCommandNode;
        } else {
            currentCommand->next = newCommandNode;
        }
        currentCommand = newCommandNode;

        if (strncmp(tempCommand, "createDatabase ", 15) == 0) {
            createDatabase(tempCommand + 15);
        } else if (strncmp(tempCommand, "useDatabase ", 12) == 0) {
            useDatabase(tempCommand + 12);
        } else if (strncmp(tempCommand, "createTable ", 12) == 0) {
            createTable(tempCommand + 12);
        } else if (strncmp(tempCommand, "insertValues ", 13) == 0) {
            insertValues(tempCommand + 13);
        } else if (strncmp(tempCommand, "displayTable ", 13) == 0) {
            displayTable(tempCommand + 13);
        } else if (strncmp(tempCommand, "deleteValue ", 12) == 0) {
            deleteValue(tempCommand + 12);
        } else if (strncmp(tempCommand, "editTable ", 10) == 0) {
            editTable(tempCommand + 10);
        } else if (strncmp(tempCommand, "deleteTable ", 12) == 0) {
            deleteTable(tempCommand + 12);
        } else if (strncmp(tempCommand, "deleteDatabase ", 15) == 0) {
            deleteDatabase(tempCommand + 15);
        } else if (strncmp(tempCommand, "unloadDatabase ", 15) == 0) {
            unloadDatabase(tempCommand + 15);
        } else if (strncmp(tempCommand, "editValue ", 10) == 0) {
            editValue(tempCommand + 10);
        } else if (strncmp(tempCommand, "loadDatabase ", 13) == 0) {
            loadDatabase(tempCommand + 13);
        } else if (strcmp(tempCommand, "loadAllDatabases") == 0) {
            loadAllDatabases();
        } else if (strcmp(tempCommand, "toggleLoadDatabase") == 0 || strcmp(tempCommand, "TLD") == 0) {
            ToggleAutomaticLoading();
        } else if (strcmp(tempCommand, "commitAll") == 0) {
            saveDatabaseToFile();
        } else if (strcmp(tempCommand, "listDatabases") == 0) {
            listDatabases();
        } else if (strcmp(tempCommand, "listTables") == 0) {
            listTables();
        } else if (strcmp(tempCommand, "help") == 0) {
            help();
        } else if (strcmp(tempCommand, "exit --no-save") == 0) {
            char input[10];
            int choice = 0;

            printf("Are you sure that you want to exit without saving? (0 - No , 1 - Yes, Enter key - Yes): ");
            fgets(input, sizeof(input), stdin);

            if (input[0] == '\n' || (sscanf(input, "%d", &choice) == 1 && choice == 1)) {
                printf("\nExiting program.\n");
                break;
            }
        } else if (strcmp(tempCommand, "exit") == 0) {
            char input[10];
            int choice = 0;

            printf("Are you sure that you want to exit while saving? (0 - No , 1 - Yes, Enter key - Yes): ");
            fgets(input, sizeof(input), stdin);

            if (input[0] == '\n' || (sscanf(input, "%d", &choice) == 1 && choice == 1)) {
                saveDatabaseToFile();
                printf("\nExiting program.\n");
                break;
            }
        } else {
            printf("Invalid command.\n");
        }
    }

    freeCommandList(commandList);
    
    return 0;
}