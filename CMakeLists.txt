cmake_minimum_required(VERSION 3.0)
project(libwebsockets)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories("include")

set(LIB_FILES  src/Client.cpp src/WebSocketServer.cpp src/Base64.cpp src/sha1.cpp)
set(EXE_FILES main.cpp)

add_library(websockets SHARED ${LIB_FILES})
add_executable(websockettest ${EXE_FILES})
target_link_libraries(websockettest websockets)
