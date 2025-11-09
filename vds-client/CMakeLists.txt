cmake_minimum_required(VERSION 3.10)
project(DecentralizedStorageClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# 查找系统库
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)

# 使用 pkg-config 查找 PBC、GMP、JSONCPP
pkg_check_modules(GMP REQUIRED gmp)
set(PBC_INCLUDE_DIRS "/usr/local/include")
set(PBC_LIBRARIES "/usr/local/lib/libpbc.so")
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# Include directories
include_directories(
    ${PBC_INCLUDE_DIRS}
    ${GMP_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
)

# Add executable
add_executable(storage_client
    main.cpp
    client.cpp
)

# Link libraries
target_link_libraries(storage_client
    ${PBC_LIBRARIES}
    ${GMP_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${CURL_LIBRARIES}
)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")