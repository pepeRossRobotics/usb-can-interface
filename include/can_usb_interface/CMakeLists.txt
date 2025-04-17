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

enable_testing()
# Tests can be added here in future
