cmake_minimum_required(VERSION 3.22)

project(hiasm
    VERSION 6.0.0.0
    DESCRIPTION "HiAsm IDE"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

option(HIASM_BUILD_RTCG_ONLY "Build RTCG without the IDE or GUI dependencies" OFF)
if(HIASM_BUILD_RTCG_ONLY)
    add_subdirectory(src/RTCG)
    return()
endif()

include(CTest)
include(cmake/CodegenBridge.cmake)
add_subdirectory(src/qt)
add_subdirectory(src/RTCG)
add_dependencies(hiasm rtcg_codegen)
include(cmake/QtPackages.cmake)

option(HIASM_BUILD_FTCG "Build the optional Free Pascal generator" OFF)
if(HIASM_BUILD_FTCG)
    add_subdirectory(src/FTCG)
endif()

if(BUILD_TESTING)
    add_test(NAME qt_native_projects COMMAND Python3::Interpreter
        "${PROJECT_SOURCE_DIR}/tests/qt_native_projects_test.py" "$<TARGET_FILE:hiasm>" "${PROJECT_SOURCE_DIR}")
    add_executable(runner_payload_test tests/runner_payload_test.cpp)
    target_include_directories(runner_payload_test PRIVATE src)
    add_test(NAME runner_payload COMMAND runner_payload_test)
    if(EXISTS "${PROJECT_SOURCE_DIR}/packs/cpp/code/core_runtime.h")
        add_executable(cpp_core_runtime_test tests/cpp_core_runtime_test.cpp)
        add_test(NAME cpp_core_runtime COMMAND cpp_core_runtime_test)
    endif()
endif()
