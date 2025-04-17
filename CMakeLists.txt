cmake_minimum_required(VERSION 3.20)
project(can_bridge)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_SOURCE_DIR}/include/can_usb_interface/include
    ${CMAKE_SOURCE_DIR}/include/socket_can_interface/include
)

add_executable(can_bridge
    src/main.cpp
    include/can_usb_interface/src/can_usb_interface.cpp
    include/socket_can_interface/src/socket_can_interface.cpp
)

target_link_libraries(can_bridge pthread)
