cmake_minimum_required(VERSION 3.10)
project(socket_can_interface_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source and include setup
include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(socket_can_interface
    src/socket_can_interface.cpp
)

add_executable(can_test
    example/main.cpp
)

target_link_libraries(can_test
    socket_can_interface
    pthread
)

# Find GoogleTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

enable_testing()

add_executable(socket_can_tests
    test/test_socket_can_interface.cpp
)
target_link_libraries(socket_can_tests
    socket_can_interface
    GTest::gtest_main
    pthread
)

add_test(NAME socket_can_tests COMMAND socket_can_tests)
