# AQADEL
- AQADEL or AxonQueryAndDatabaseExecutionLanguage is a DBMS written in C++.
    - Commits and updates will be appriciated!

## Available Commands - 
- createDatabase DatabaseName  -  Creates a database with a specified name and by default switches to the database.

- useDatabase DatabaseName  -  Select a database to use

- listDatabases  -  Shows the list of saved/loadable databases.

- defaultDatabase DatabaseName  -  Select a default database to use at boot.

- deleteDatabase DatabaseName  -  Deletes a database.

- createTable TableName[col1 type attribute, col2 type attribute, ...] - Create a new table, attribute(unique|primary) parameter is optional.
  - Note: for string datatypes: string{length}

- listTables  -  Shows a list of tables in the currently selected database.

- descTable TableName  -  Describes the structure of a table.

- insertValues TableName(Value, ...)  -  Appends a value to a specified table.
  - Note: for string put the value in "".

- displayTable TableName  -  Displays a specified table.

- displayTable Main select values if columnName == value  -  Displays a specific part of a table.
  - Note: String must be in "".

- editTable TableName addColumn/removeColumn ColumnName Datatype  -  Adds or removes a column in a specified table.
  - Note: Datatype required only when adding.

- deleteValue TableName if ColumnName == Value  -  Deletes a value from a specified table.
  - Note: Value should be in "" if string.

- editValue TableName set ColumnName = NewValue if ColumnName == Value  -  Edit the value of a specified column in a specified table.
  - Note: Value and NewValue should be in "" if string.

- deleteTable TableName  -  Deletes a table.

- openSafeBlock | openTransaction | openChannel | osb  -  let's user run quaries without permanently saving database until close query is executed (closeSafeBlock | closeTransaction | closeChannel | csb).

- discardSafeBlock | discardTransaction | discardChannel | dsb  -  Discards all changes done in a safe block and closes the safe block. 

- closeSafeBlock | closeTransaction | closeChannel | csb  -  Saves all changes done in a safe block and closes the safe block.

- exit  -  Exits the program.

- help  -  Displays this message.

## Future Features Roadmap (EXTREMELY VULNERABLE TO CHANGES)

### Data Management and Querying
- **Advanced Data Types** - Support for binary data, JSON objects, and arrays
- **Complex Queries** - Implementation of JOIN operations, complex WHERE clauses with multiple conditions
- **Query Optimization** - Automatic query optimization for faster data retrieval
- **Views** - Create virtual tables based on query results
- **Indexes** - Add primary and secondary indexes for improved query performance
- **Stored Procedures** - Support for defining and executing stored procedures
- **Triggers** - Set up automatic actions when data changes

### Security and User Management
- **Role-Based Access Control** - Define roles with specific permissions
- **Multi-User Support** - Allow multiple users with different privilege levels
- **Row-Level Security** - Control access to specific data rows based on user roles
- **Audit Logging** - Track and log all database operations

### Data Integrity and Recovery
- **Foreign Key Constraints** - Enforce relationships between tables
- **Unique Constraints** - Ensure uniqueness of specific column values
- **Backup and Restore** - Tools for database backup and recovery
- **Point-in-Time Recovery** - Ability to restore database to a specific moment
- TBA commands:
  - enablePointInTimeRecovery | enablePITR [Done]
  - listRecoverableDatabases | listRecoverableDBs | listRDBs
  - createInstantBackup | createIB
  - recoverDatabase | recoverDB | rDB
  - disablePointInTimeRecovery | disablePITR [Done]
  - editPointInTimeRecovery set maxCopies = value | editPITR set maxCopies = value
- **Database Mirroring** - Maintain redundant copies for high availability

### Tools and Interfaces
- **GUI Client** - Graphical interface for database management
- **Import/Export Tools** - Support CSV, JSON, and other formats
- **Language Bindings** - API support for Python, Java, Node.js, etc.
- **Remote Connection** - Connect to the database over a network
- **Connection Pooling** - Efficiently manage multiple simultaneous connections

### Advanced Features
- **Full-Text Search** - Implement advanced text search capabilities
- **Geospatial Data** - Support for location-based queries
- **Time-Series Data** - Optimized storage and querying for time-series data
- **Distributed Database** - Support for distributed database architecture
- **Sharding** - Horizontal partitioning for very large datasets
- **Analytics** - Built-in analytical functions and reporting tools

## Building on windows -
  - Requirements:
      - OpenSSL-Win64
      - MinGW
      - Windows 10 or above
  - run Build.bat file to build the executable.

## Building on Linux - 
  - Requirements:
      - OpenSSL development libraries
      - GCC or Clang compiler
      - Make
  - Install dependencies:
    ```bash
    # Debian/Ubuntu
    sudo apt-get install build-essential libssl-dev
    # Fedora
    sudo dnf install gcc-c++ openssl-devel make
    # Arch Linux
    sudo pacman -S base-devel openssl
    ```
  - Build the project:
    ```bash
    # From project root directory
    mkdir build
    cd build
    cmake ..
    make
    ```
  - Run the executable:
    ```bash
    ./aqadel
    ```

## Building on macOS -
  - Requirements:
      - OpenSSL (via Homebrew)
      - Clang (installed with Xcode Command Line Tools)
      - CMake
  - Install dependencies:
    ```bash
    # Install Homebrew if not already installed
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
    
    # Install OpenSSL and CMake
    brew install openssl cmake
    ```
  - Build the project:
    ```bash
    # From project root directory
    mkdir build
    cd build
    cmake .. -DOPENSSL_ROOT_DIR=$(brew --prefix openssl)
    make
    ```
  - Run the executable:
    ```bash
    ./aqadel
    ```

- Pre-compiled binaries for linux and macOS should be available in the actions tab.
- Any issues should be reported in issues tab.