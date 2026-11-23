cmake_minimum_required(VERSION 3.10)
project(CodeGenLibrary NONE)

find_program(FPC_EXECUTABLE NAMES fpc REQUIRED)

if(WIN32)
    set(FTCG_NAME "CodeGen.dll")
else()
    set(FTCG_NAME "libCodeGen.so")
endif()
# Legacy tooling only: the bundled Web and Modules packages use RTCG, so the
# FTCG binary is kept out of the staged package compiler directories.
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/ftcg/compiler")
set(UNIT_DIR "${CMAKE_CURRENT_BINARY_DIR}/units")
set(OUTPUT_FILE "${OUTPUT_DIR}/${FTCG_NAME}")

add_custom_command(
    OUTPUT "${OUTPUT_FILE}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${OUTPUT_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${UNIT_DIR}"
    COMMAND "${FPC_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/CodeGen.dpr" -Mdelphi -O1 -XX
            -FE"${OUTPUT_DIR}" -FU"${UNIT_DIR}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/CGTShare.pas"
        "${CMAKE_CURRENT_SOURCE_DIR}/CodeGen.dpr"
        "${CMAKE_CURRENT_SOURCE_DIR}/MD5.pas"
    COMMENT "Building FTCG with Free Pascal into the build tree")

add_custom_target(ftcg ALL DEPENDS "${OUTPUT_FILE}")
