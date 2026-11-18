cmake_minimum_required(VERSION 3.10)
project(MyExecutable)

# Set the output directory for the executable to the current source directory
set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_library(make_html main.c)