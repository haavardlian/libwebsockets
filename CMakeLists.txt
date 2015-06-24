cmake_minimum_required(VERSION 3.0)
project(libwebsockets)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(LIB_FILES  Client.cpp WebSocketServer.cpp WebSocketServer.h Base64.cpp sha1.cpp)
set(EXE_FILES main.cpp)

add_library(websockets SHARED ${LIB_FILES})
add_executable(websockettest ${EXE_FILES})
target_link_libraries(websockettest websockets)
