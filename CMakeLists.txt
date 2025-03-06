cmake_minimum_required(VERSION 3.15)
project(AQADEL VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific OpenSSL configuration
if(WIN32)
    set(OPENSSL_USE_STATIC_LIBS ON)
    set(OPENSSL_MSVC_STATIC_RT ON)
    
    if(MINGW)
        # MinGW-specific OpenSSL paths
        set(OPENSSL_ROOT_DIR "C:/msys64/ucrt64")
        set(OPENSSL_INCLUDE_DIR "C:/msys64/ucrt64/include")
        set(OPENSSL_CRYPTO_LIBRARY "C:/msys64/ucrt64/lib/libcrypto.dll.a")
        set(OPENSSL_SSL_LIBRARY "C:/msys64/ucrt64/lib/libssl.dll.a")
        
        # Add MinGW OpenSSL binary directory to PATH for DLLs
        set(ENV{PATH} "C:/msys64/ucrt64/bin;$ENV{PATH}")
    else()
        # MSVC paths
        if(NOT DEFINED OPENSSL_ROOT_DIR)
            set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL")
        endif()
    endif()
endif()

# Find OpenSSL with more detailed error reporting
find_package(OpenSSL REQUIRED)
if(NOT OpenSSL_FOUND)
    message(FATAL_ERROR 
        "OpenSSL not found. Please install OpenSSL or set OPENSSL_ROOT_DIR.\n"
        "Current OPENSSL_ROOT_DIR = ${OPENSSL_ROOT_DIR}\n"
        "Current OPENSSL_INCLUDE_DIR = ${OPENSSL_INCLUDE_DIR}\n"
        "Current OPENSSL_LIBRARIES = ${OPENSSL_LIBRARIES}\n"
        "Using MinGW = ${MINGW}"
    )
endif()

# Add source files
set(SOURCES
    src/main.cpp
    src/CLI.cpp
    src/Encryption.cpp
    src/UserManager.cpp
)

# Add header files
set(HEADERS
    include/CLI.hpp
    include/Constants.hpp
    include/Encryption.hpp
    include/User.hpp
    include/UserManager.hpp
)

# Create executable
add_executable(aqadel ${SOURCES} ${HEADERS})

# Add include directories
target_include_directories(aqadel PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
)

# Link OpenSSL libraries
target_link_libraries(aqadel PRIVATE 
    OpenSSL::SSL 
    OpenSSL::Crypto
)

# Set output directories
set_target_properties(aqadel PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install rules
install(TARGETS aqadel
    RUNTIME DESTINATION bin
)

# Platform specific settings
if(MSVC)
    target_compile_options(aqadel PRIVATE /W4)
    target_compile_definitions(aqadel PRIVATE
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
    )
    
    # Copy OpenSSL DLLs to output directory for MSVC
    if(EXISTS "${OPENSSL_ROOT_DIR}/bin/libcrypto-3.dll")
        add_custom_command(TARGET aqadel POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${OPENSSL_ROOT_DIR}/bin/libcrypto-3.dll"
                "${OPENSSL_ROOT_DIR}/bin/libssl-3.dll"
                $<TARGET_FILE_DIR:aqadel>
        )
    endif()
elseif(MINGW)
    target_compile_options(aqadel PRIVATE -Wall -Wextra)
    
    # Copy OpenSSL DLLs to output directory for MinGW
    add_custom_command(TARGET aqadel POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "C:/msys64/ucrt64/bin/libcrypto-3-x64.dll"
            "C:/msys64/ucrt64/bin/libssl-3-x64.dll"
            $<TARGET_FILE_DIR:aqadel>
    )
else()
    target_compile_options(aqadel PRIVATE -Wall -Wextra)
endif()

# Enable testing
# enable_testing()

# Add tests here when you have them
# add_subdirectory(tests)
