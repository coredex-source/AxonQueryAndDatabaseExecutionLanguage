# AQADEL
- AQADEL or AxonQueryAndDatabaseExecutionLanguage is a DBMS written in C++.
    - Commits and updates will be appriciated!

## Available Commands - 
- createDatabase DatabaseName  -  Creates a database with a specified name and by default switches to the database.

- useDatabase DatabaseName  -  Select a database to use

- listDatabases  -  Shows the list of saved/loadable databases.

- defaultDatabase DatabaseName  -  Select a default database to use at boot.

- createTable TableName[ColumnName datatype, ...]  -  Creates a table within a database.
  - Note: for string datatypes: string{length}

- listTables  -  Shows a list of tables in the currently selected database.

- descTable TableName  -  Describes the structure of a table.

- insertValues TableName(Value, ...)  -  Appends a value to a specified table.
  - Note: for string put the value in "".

- displayTable TableName  -  Displays a specified table.

- deleteValue TableName if ColumnName == Value  -  Deletes a value from a specified table.
  - Note: Value should be in "" if string.

- exit  -  Exits program while saving all changes.

- help  -  Displays this message.

## To be available commands -

- editTable TableName addColumn/removeColumn ColumnName Datatype  -  Adds or removes a column in a specified table.
  - Note: Datatype required only when adding.

- deleteTable TableName  -  Deletes a table.

- deleteDatabase DatabaseName  -  Deletes a database.

- editValue TableName set ColumnName = NewValue if ColumnName == Value  -  Edit the value of a specified column in a specified table.
  - Note: Value and NewValue should be in "" if string.

- loadDatabase DatabaseName  -  Manually load a specific saved database.

- unloadDatabase DatabaseName  -  Unloads a database from memory.

- loadAllDatabases  -  Manually load all saved databases.

- toggleLoadDatabase  -  Toggles automatic loading at boot of databases. || Alais - TLD

- commitAll  -  Saves all changes done till now permanently.

- exit --no-save  -  Exits program while discarding all changes.

## Building on windows -
  - Requirements:
      - OpenSSL-Win64
      - MinGW
      - Windows 10 or above
  - run Build.bat file to build the executable.

- Pre-compiled binaries should be available in the actions tab.
