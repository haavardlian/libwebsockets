cmake_minimum_required(VERSION 3.2)
project(libwebsockets)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp PollableSocket.cpp)
add_executable(libwebsockets ${SOURCE_FILES} WebSocketServer.cpp WebSocketServer.h Base64.cpp sha1.cpp)