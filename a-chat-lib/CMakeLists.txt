cmake_minimum_required(VERSION 3.10)
project(a-chat-lib VERSION 1.0.0 LANGUAGES C)

add_library(a-chat-lib
    src/chat.c
    src/message.c
)

target_include_directories(a-chat-lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
