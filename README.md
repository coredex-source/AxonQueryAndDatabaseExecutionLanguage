# AQADEL
- AQADEL or AxonQueryAndDatabaseExecutionLanguage is a DBMS written in C++.
    - Commits and updates will be appriciated!

## Available Commands - 
- createDatabase DatabaseName  -  Creates a database with a specified name and by default switches to the database.

- useDatabase DatabaseName  -  Select a database to use

- listDatabases  -  Shows the list of saved/loadable databases.

- defaultDatabase DatabaseName  -  Select a default database to use at boot.

- deleteDatabase DatabaseName  -  Deletes a database.

- createTable TableName[ColumnName datatype, ...]  -  Creates a table within a database.
  - Note: for string datatypes: string{length}

- listTables  -  Shows a list of tables in the currently selected database.

- descTable TableName  -  Describes the structure of a table.

- insertValues TableName(Value, ...)  -  Appends a value to a specified table.
  - Note: for string put the value in "".

- displayTable TableName  -  Displays a specified table.

- editTable TableName addColumn/removeColumn ColumnName Datatype  -  Adds or removes a column in a specified table.
  - Note: Datatype required only when adding.

- deleteValue TableName if ColumnName == Value  -  Deletes a value from a specified table.
  - Note: Value should be in "" if string.

- editValue TableName set ColumnName = NewValue if ColumnName == Value  -  Edit the value of a specified column in a specified table.
  - Note: Value and NewValue should be in "" if string.

- deleteTable TableName  -  Deletes a table.

- exit  -  Exits the program.

- help  -  Displays this message.

## To be available commands -

- openSafeBlock | openTransaction | openChannel | osb  -  let's user run quaries without permanently saving database until close query is executed (closeSafeBlock | closeTransaction | closeChannel | csb).

- discardSafeBlock | discardTransaction | discardChannel | dsb  -  Discards all changes done in a safe block and closes the safe block. 

- closeSafeBlock | closeTransaction | closeChannel | csb  -  Saves all changes done in a safe block and closes the safe block.

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
