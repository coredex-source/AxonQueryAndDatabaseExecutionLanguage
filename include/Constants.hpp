#pragma once

#define AQADEL_NAME "AQADEL"
#define AQADEL_VERSION "CPP-v1.0"
#define AQADEL_DB_EXT ".aqdb"

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
