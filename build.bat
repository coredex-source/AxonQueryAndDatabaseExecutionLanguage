@echo off
setlocal

REM Check for OpenSSL environment variables
if not defined OPENSSL_DIR (
    echo Setting default OpenSSL directory...
    set OPENSSL_DIR=C:\Program Files\OpenSSL-Win64
)

if not exist "%OPENSSL_DIR%" (
    echo Error: OpenSSL directory not found at %OPENSSL_DIR%
    echo Please install OpenSSL or set OPENSSL_DIR to the correct path
    exit /b 1
)

REM Create build directory if it doesn't exist
if not exist "build" mkdir build

REM Compile with OpenSSL includes and libraries
g++ -std=c++17 ^
    -I include ^
    -I"%OPENSSL_DIR%\include" ^
    -L"%OPENSSL_DIR%\lib" ^
    -o build/aqadel.exe ^
    src/main.cpp ^
    src/CLI.cpp ^
    src/Encryption.cpp ^
    src/UserManager.cpp ^
    -lssl ^
    -lcrypto ^
    -lws2_32 ^
    -lgdi32 ^
    -lcrypt32

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    
    REM Copy required DLLs to build directory
    copy "%OPENSSL_DIR%\bin\libssl-3-x64.dll" build\
    copy "%OPENSSL_DIR%\bin\libcrypto-3-x64.dll" build\
    
    echo Executable and dependencies are in the build directory
    echo Execute build\aqadel.exe to run the program
) else (
    echo Build failed!
)

endlocal