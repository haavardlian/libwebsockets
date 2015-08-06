cmake_minimum_required(VERSION 3.0)
project(libwebsockets)

include(CPack)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -W -Wall -pedantic")

include_directories("include")

set(LIB_FILES  src/Client.cpp src/WebSocketServer.cpp src/Base64.cpp src/sha1.cpp)
set(EXE_FILES main.cpp)

add_library(websockets SHARED ${LIB_FILES})
add_library(websockets-static STATIC ${LIB_FILES}) 
set_target_properties(websockets-static PROPERTIES OUTPUT_NAME websockets)

if(DEFINED ENV{VERSION})
  set_target_properties(websockets PROPERTIES VERSION $ENV{VERSION} SOVERSION $ENV{SO_VERSION})
endif()

install(TARGETS websockets DESTINATION lib)
install(TARGETS websockets-static DESTINATION lib)

if(DEFINED ENV{BUILD_TEST})
  add_executable(websockettest ${EXE_FILES})
  target_link_libraries(websockettest websockets)
endif()
