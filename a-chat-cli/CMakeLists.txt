cmake_minimum_required(VERSION 3.15)
project(a-chat-cli C)

# needed for zed
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(../a-chat-lib a-chat-lib)

add_executable(a-chat-cli src/main.c)

target_link_libraries(a-chat-cli PRIVATE a-chat-lib)
