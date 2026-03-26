cmake_minimum_required(VERSION 2.8)
project(ouch_client)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(SOURCES
    src/socket.cpp
    src/config_parser.cpp
    src/frame_encoder.cpp
    src/main.cpp
)

add_executable(ouch_client ${SOURCES})
