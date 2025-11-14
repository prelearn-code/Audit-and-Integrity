cmake_minimum_required(VERSION 3.10)
project(DecentralizedStorage VERSION 1.0)

# 使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# 查找系统库
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)

# 使用 pkg-config 查找 PBC、GMP、JSONCPP
pkg_check_modules(GMP REQUIRED gmp)
set(PBC_INCLUDE_DIRS "/usr/local/include")
set(PBC_LIBRARIES "/usr/local/lib/libpbc.so")
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# 添加头文件搜索路径
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}  # 项目根目录
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIRS}
    ${PBC_INCLUDE_DIRS}
    ${GMP_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
)

# 添加可执行文件
add_executable(storage_node
    main.cpp
    storage_node.cpp
)
set(CMAKE_BUILD_TYPE Debug)
# 链接库
target_link_libraries(storage_node
    ${OPENSSL_LIBRARIES}
    ${CURL_LIBRARIES}
    ${PBC_LIBRARIES}
    ${GMP_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    pthread
)
