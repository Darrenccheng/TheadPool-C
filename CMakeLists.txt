cmake_minimum_required(VERSION 3.17)
project(mythreadPoolC)

file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.c )

set(CMAKE_C_STANDARD 11)
find_package(Threads REQUIRED)

set(EXECUTABLE_OUTPUT_PATH /home/aa/bb/cc)
add_executable(threadPoolC ${SRC})

target_link_libraries(threadPoolC Threads::Threads)