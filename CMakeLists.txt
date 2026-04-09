cmake_minimum_required(VERSION 3.10)
project(MultiplexerShowdown VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(server src/main.cpp src/SelectServer.cpp)
target_include_directories(server PRIVATE src)

add_executable(bench_client src/bench_client.cpp)