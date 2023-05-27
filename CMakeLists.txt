cmake_minimum_required(VERSION 3.0.0)
project(http1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
include_directories(include/http1)

add_library(http1 src/tcp_server.cpp src/http_server.cpp)
set_property(TARGET http1 PROPERTY CXX_STANDARD 20)
target_compile_options(http1 PRIVATE -Wall -Wextra -Werror)

add_executable(example-server src/main.cpp)
set_property(TARGET example-server PROPERTY CXX_STANDARD 20)
target_compile_options(example-server PRIVATE -Wall -Wextra -Werror)
target_link_libraries(example-server http1)

configure_file(${CMAKE_SOURCE_DIR}/asset/bg.jpg ${CMAKE_BINARY_DIR} COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/asset/index.html ${CMAKE_BINARY_DIR} COPYONLY)

enable_testing()
add_subdirectory(test)