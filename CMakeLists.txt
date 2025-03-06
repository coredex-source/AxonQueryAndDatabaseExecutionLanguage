cmake_minimum_required(VERSION 3.15)
project(AQADEL VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenSSL
find_package(OpenSSL REQUIRED)

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
else()
    target_compile_options(aqadel PRIVATE -Wall -Wextra)
endif()

# Enable testing
enable_testing()

# Add tests here when you have them
# add_subdirectory(tests)
