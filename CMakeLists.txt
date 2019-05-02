cmake_minimum_required(VERSION 3.10)
project(server_v2)

set(CMAKE_CXX_STANDARD 11)

add_executable(server src/server.cpp src/Network.cpp src/rwr.cpp include/Network.h include/dhcp_errors.h include/rwr.hpp)
target_link_libraries(server pthread)