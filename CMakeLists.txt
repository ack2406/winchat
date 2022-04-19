cmake_minimum_required(VERSION 3.21)
project(winchat)

set(CMAKE_CXX_STANDARD 17)

add_executable(client client.cpp)
add_executable(server server.cpp)

target_link_libraries(client Ws2_32)
target_link_libraries(server Ws2_32)