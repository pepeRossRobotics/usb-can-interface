cmake_minimum_required(VERSION 3.16)
project(can_usb_interface_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source and include setup
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add source files
add_library(can_usb_interface
    src/can_usb_interface.cpp
)

target_include_directories(can_usb_interface PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Build example program
add_executable(can_usb_test
    example/main.cpp
)

target_link_libraries(can_usb_test
    can_usb_interface
    pthread
)

# Find GoogleTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})


# Add test executable
enable_testing()

add_executable(test_can_usb
    test/test_can_usb_interface.cpp
    ${CMAKE_SOURCE_DIR}/src/can_usb_interface.cpp
)
target_include_directories(test_can_usb PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_can_usb GTest::gtest_main pthread)
add_test(NAME CanUsbTests COMMAND test_can_usb)
