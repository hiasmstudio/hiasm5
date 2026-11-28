cmake_minimum_required(VERSION 3.21)
project(HiAsmRTCG LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB SOURCES "*.cpp")

add_library(rtcg_codegen SHARED ${SOURCES})
target_compile_definitions(rtcg_codegen PRIVATE BUILDING_DLL HIASM5_ABI)
target_include_directories(rtcg_codegen PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

if(WIN32 OR MINGW)
    set(RTCG_PREFIX "")
    set(RTCG_SUFFIX ".dll")
else()
    set(RTCG_PREFIX "lib")
    set(RTCG_SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()
set(RTCG_FILE "${RTCG_PREFIX}CodeGen${RTCG_SUFFIX}")

# Native packages load their own compiler/CodeGen.{dll,so}. The same RTCG
# library serves every package that describes its language sections in
# code/codegen.ini, so stage one copy per package directory.
set(RTCG_PACKAGES cpp web modules)
set(RTCG_OUTPUT_DIR "${CMAKE_BINARY_DIR}/packs/cpp/compiler")
set_target_properties(rtcg_codegen PROPERTIES
    OUTPUT_NAME "CodeGen"
    PREFIX "${RTCG_PREFIX}"
    RUNTIME_OUTPUT_DIRECTORY "${RTCG_OUTPUT_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${RTCG_OUTPUT_DIR}"
)

foreach(package IN LISTS RTCG_PACKAGES)
    add_custom_command(TARGET rtcg_codegen POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_BINARY_DIR}/packs/${package}/compiler"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:rtcg_codegen>"
            "${CMAKE_BINARY_DIR}/packs/${package}/compiler/${RTCG_FILE}"
        VERBATIM)
endforeach()
