option(HIASM_BUILD_CGBRIDGE "Build the 32-bit host for HiAsm 4 Delphi code generators" OFF)
if(HIASM_BUILD_CGBRIDGE)
    if(NOT WIN32)
        message(FATAL_ERROR "HIASM_BUILD_CGBRIDGE loads 32-bit Windows DLLs and requires Windows")
    endif()
    # tools/setup_fpc_windows.ps1 places a portable FPC 3.2.2 under build/deps.
    find_program(FPC_I386_EXECUTABLE NAMES ppc386
        HINTS "${CMAKE_CURRENT_LIST_DIR}/../build/deps/fpc-3.2.2/extracted/app/bin/i386-win32"
        REQUIRED)
    get_filename_component(FPC_I386_ROOT "${FPC_I386_EXECUTABLE}/../../.." ABSOLUTE)
    set(CGBRIDGE_DIR "${CMAKE_CURRENT_LIST_DIR}/../src/hiasm4_cgbridge")
    set(CGBRIDGE_UNIT_DIR "${CMAKE_CURRENT_BINARY_DIR}/cgbridge-units")
    set(CGBRIDGE_OUTPUT "${CMAKE_BINARY_DIR}/hiasm4_cgbridge.exe")
    add_custom_command(
        OUTPUT "${CGBRIDGE_OUTPUT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CGBRIDGE_UNIT_DIR}"
        COMMAND "${FPC_I386_EXECUTABLE}" -n -Mdelphi -O1 -XX
            "-Fu${FPC_I386_ROOT}/units/i386-win32/*" "-FE${CMAKE_BINARY_DIR}" "-FU${CGBRIDGE_UNIT_DIR}"
            "${CGBRIDGE_DIR}/hiasm4_cgbridge.lpr"
        DEPENDS "${CGBRIDGE_DIR}/hiasm4_cgbridge.lpr" "${CGBRIDGE_DIR}/Cgt.pas"
            "${CGBRIDGE_DIR}/Model.pas" "${CGBRIDGE_DIR}/Tools.pas"
        COMMENT "Building the 32-bit HiAsm 4 CodeGen bridge"
        VERBATIM)
    add_custom_target(hiasm4_cgbridge ALL DEPENDS "${CGBRIDGE_OUTPUT}")
endif()
