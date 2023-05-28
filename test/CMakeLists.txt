project(http1-test)

include_directories(googletest/include)

set(GOOGLETEST_VERSION 1.13.0)
add_subdirectory(googletest)


function(add_test_file file_name test_name)
    add_executable(${test_name} ${file_name})
    set_property(TARGET ${test_name} PROPERTY CXX_STANDARD 20)
    target_compile_options(${test_name} PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(${test_name} http1)
    target_link_libraries(${test_name} gtest_main gtest)
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_test_file(request_parser.cpp request-parser-test)
add_test_file(response_serializer.cpp response-serializer-test)