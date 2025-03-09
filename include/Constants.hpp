#pragma once

#define AQADEL_NAME "AQADEL"
#define AQADEL_VERSION "CPP-v1.0-a10"
#define AQADEL_DB_EXT ".aqadb"

// Data type identifiers
#define DT_INT "int"
#define DT_FLOAT "float"
#define DT_STRING "string"
#define DT_BOOL "bool"

// Encryption constants
#define ENCRYPTION_VERSION 1
#define KEY_LENGTH 32        // 256-bit key
#define IV_LENGTH 12         // 96-bit IV for GCM
#define TAG_LENGTH 16        // 128-bit authentication tag
#define SALT_LENGTH 32       // 256-bit salt
#define KEY_ITERATIONS 10000 // PBKDF2 iterations
#define ENCRYPTION_KEY "AQ@DEL2023"  // Basic encryption key
#define ENCRYPTION_HEADER "AQENC"     // Encrypted file header

// Database security
#define DB_KEY_LENGTH 32
#define DB_SALT_LENGTH 32
#define DB_KEY_MARKER "DB_KEY:"
#define DB_SALT_MARKER "DB_SALT:"
#define DB_VERIFY_MARKER "DB_VERIFY:"
#define DB_HEADER_MARKER "HEADER:"
#define DB_INTEGRITY_MARKER "INTEGRITY:"

// Table data markers
#define TABLE_DATA_START "DATA_START"
#define TABLE_DATA_END "DATA_END"
#define ROW_SEPARATOR "|"

// Default database configuration
#define DEFAULT_DB_FILE "default.cfg"

// Authentication constants
#define MAX_PASSWORD_ATTEMPTS 3
