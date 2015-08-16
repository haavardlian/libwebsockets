cmake_minimum_required(VERSION 2.8.4)
project(libwebsockets)

include(CPack)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "WebSocket library written in C++")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -W -Wall -pedantic")

include_directories("include")

set(LIB_FILES  src/Client.cpp src/WebSocketServer.cpp src/Base64.cpp src/sha1.cpp)
set(EXE_FILES main.cpp)

add_library(websockets SHARED ${LIB_FILES})
add_library(websockets-static STATIC ${LIB_FILES}) 
set_target_properties(websockets-static PROPERTIES OUTPUT_NAME websockets)

if(DEFINED ENV{TEAMCITY})
  set(VERSION "$ENV{MAJOR}.$ENV{MINOR}.$ENV{PATCH}")
  SET(CPACK_PACKAGE_VERSION_MAJOR $ENV{MAJOR})
  SET(CPACK_PACKAGE_VERSION_MINOR $ENV{MINOR})
  SET(CPACK_PACKAGE_VERSION_PATCH $ENV{PATCH})
  set_target_properties(websockets PROPERTIES VERSION ${VERSION} SOVERSION $ENV{MAJOR})
endif()

install(TARGETS websockets DESTINATION lib)
install(TARGETS websockets-static DESTINATION lib)


add_executable(websockettest ${EXE_FILES})
target_link_libraries(websockettest websockets)


