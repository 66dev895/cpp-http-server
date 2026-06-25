cmake_minimum_required(VERSION 3.16)
project(cpp-http-server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- 编译选项 ----
add_compile_options(-Wall -Wextra -O2 -g)

# ---- 可执行文件 ----
add_executable(http_server
    src/main.cpp
    src/server.cpp
    src/http_parser.cpp
    src/http_response.cpp
    src/router.cpp
    src/thread_pool.cpp
)

target_include_directories(http_server PRIVATE include)

# 链接 pthread（epoll 需要）
find_package(Threads REQUIRED)
target_link_libraries(http_server PRIVATE Threads::Threads)

# ---- 测试 ----
enable_testing()
add_executable(test_http_parser
    tests/test_http_parser.cpp
    src/http_parser.cpp
)
target_include_directories(test_http_parser PRIVATE include)

add_test(NAME HttpParserTest COMMAND test_http_parser)
