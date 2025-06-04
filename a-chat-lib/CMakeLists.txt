cmake_minimum_required(VERSION 3.15)
project(a-chat-lib)

# needed for zed
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(a-chat-lib
    include/log.h
    include/server/server.h
    include/client/client.h
    src/log.c
    src/client/client.c
    src/server/server.c
)

target_include_directories(a-chat-lib PUBLIC include)
