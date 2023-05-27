cmake_minimum_required(VERSION 3.0.0)
project(http1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
include_directories(include/http1)

add_executable(http1-server src/main.cpp src/tcp_server.cpp src/http_server.cpp)
set_property(TARGET http1-server PROPERTY CXX_STANDARD 20)
target_compile_options(http1-server PRIVATE -Wall -Wextra -Werror)

configure_file(${CMAKE_SOURCE_DIR}/asset/bg.jpg ${CMAKE_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/asset/index.html ${CMAKE_BINARY_DIR} COPYONLY)