cmake_minimum_required(VERSION 3.10)
project(usb_can_analyser)

add_definitions(-D_GNU_SOURCE)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(can_usb
    src/can_usb.cpp
)

add_library(socket_can_interface
    src/socket_can_interface.cpp
)
target_include_directories(socket_can_interface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)


add_executable(usb_can_debug main.cpp)
target_link_libraries(usb_can_debug can_usb)

add_executable(usb_can_bridge usb_can_bridge.cpp)
target_link_libraries(usb_can_bridge can_usb socket_can_interface)
