cmake_minimum_required(VERSION 3.5)
project(chatUDP_lincli)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/skel.h src/etcp.h src/iofuncs.cpp src/sofuncs.c src/miscfuncs.c src/automutex.h src/ChatServer.h src/ChatServer.cpp src/main.cpp src/Packet.h src/Data.h src/Data.cpp src/zbridge.cpp)
add_executable(chatUDP_lincli ${SOURCE_FILES})
target_link_libraries(chatUDP_lincli pthread)