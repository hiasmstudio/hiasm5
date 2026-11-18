cmake_minimum_required(VERSION 3.21)

project(hiasm
    VERSION 6.0.0.0
    DESCRIPTION "HiAsm IDE"
    LANGUAGES CXX)

set(HIASM_GTK_VERSION "4" CACHE STRING "GTK major version: 3 or 4")
set_property(CACHE HIASM_GTK_VERSION PROPERTY STRINGS 3 4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

set(HIASM_GTKMM_VERSION "3.0" CACHE STRING "gtkmm pkg-config module version for GTK3 (3.0 or 2.4)")

find_package(PkgConfig REQUIRED)

if(HIASM_GTK_VERSION STREQUAL "4")
    pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)

    pkg_check_modules(GTKSOURCE REQUIRED IMPORTED_TARGET gtksourceview-5)
    set(HIASM_GTK_SOURCE_TARGET PkgConfig::GTKSOURCE)

    set(HIASM_GTK_DEFINITIONS HIASM_GTK4=1)
else()
    pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-${HIASM_GTKMM_VERSION})

    if(HIASM_GTKMM_VERSION STREQUAL "2.4")
        pkg_check_modules(GTKSOURCEMM REQUIRED IMPORTED_TARGET gtksourceviewmm-2.0)
    else()
        pkg_check_modules(GTKSOURCEMM REQUIRED IMPORTED_TARGET gtksourceviewmm-3.0)
    endif()
    set(HIASM_GTK_SOURCE_TARGET PkgConfig::GTKSOURCEMM)
    set(HIASM_GTK_DEFINITIONS HIASM_GTK3=1)
endif()
pkg_check_modules(GLIBCORE REQUIRED IMPORTED_TARGET gthread-2.0 gmodule-2.0)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)

option(HIASM_BUILD_FTCG "Build the optional legacy FTCG code generator" OFF)
if(HIASM_BUILD_FTCG)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/FTCG"
        "${CMAKE_CURRENT_BINARY_DIR}/ftcg")
endif()

option(HIASM_BUILD_CGBRIDGE "Build the 32-bit host for HiAsm 4 Delphi code generators" OFF)
if(HIASM_BUILD_CGBRIDGE)
    if(NOT WIN32)
        message(FATAL_ERROR "HIASM_BUILD_CGBRIDGE loads 32-bit Windows DLLs and requires Windows")
    endif()
    # tools/setup_fpc_windows.ps1 places a portable FPC 3.2.2 under build/deps.
    find_program(FPC_I386_EXECUTABLE NAMES ppc386
        HINTS "${CMAKE_CURRENT_SOURCE_DIR}/build/deps/fpc-3.2.2/extracted/app/bin/i386-win32"
        REQUIRED)
    get_filename_component(FPC_I386_ROOT "${FPC_I386_EXECUTABLE}/../../.." ABSOLUTE)
    set(CGBRIDGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/hiasm4_cgbridge")
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

option(HIASM_BUILD_RTCG "Build the C++ RTCG code generator" ON)
if(HIASM_BUILD_RTCG)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/RTCG"
        "${CMAKE_CURRENT_BINARY_DIR}/rtcg")
endif()

add_library(text_project_maker MODULE tools/text_project_maker.cpp)
target_link_libraries(text_project_maker PRIVATE PkgConfig::GLIBCORE)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)
target_link_libraries(text_project_maker PRIVATE PkgConfig::GIO)
set_target_properties(text_project_maker PROPERTIES PREFIX "" OUTPUT_NAME "make_text"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/packs/cpp/make"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/packs/cpp/make")
add_custom_command(TARGET text_project_maker POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/packs/web/make"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different $<TARGET_FILE:text_project_maker>
        "${CMAKE_BINARY_DIR}/packs/web/make/make_html${CMAKE_SHARED_MODULE_SUFFIX}")

file(GLOB HIASM_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

list(REMOVE_ITEM HIASM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/hico.cpp")
add_library(hiasm_core STATIC ${HIASM_SOURCES})
add_executable(hiasm WIN32 src/hico.cpp)
target_link_libraries(hiasm PRIVATE hiasm_core)
add_executable(hiasm_runner src/runner.cpp)
set_target_properties(hiasm_runner PROPERTIES OUTPUT_NAME runner)
target_link_libraries(hiasm_runner PRIVATE hiasm_core)

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/hiasm.ico")
    enable_language(RC)
    target_sources(hiasm PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/resources/hiasm.rc")
endif()

target_include_directories(hiasm_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(hiasm_core PUBLIC ${HIASM_GTK_DEFINITIONS})

target_link_libraries(hiasm_core PUBLIC
    PkgConfig::GTKMM
    ${HIASM_GTK_SOURCE_TARGET}
    PkgConfig::GLIBCORE
    PkgConfig::SQLITE3)

if(MINGW)
    target_link_options(hiasm PRIVATE -mwindows)
    # standard Windows file dialogs (src/win32_file_dialog.cpp)
    target_link_libraries(hiasm_core PUBLIC ole32 uuid shell32)
endif()

find_program(SQLITE3_EXECUTABLE NAMES sqlite3)
if(NOT SQLITE3_EXECUTABLE AND WIN32)
    find_program(SQLITE3_EXECUTABLE NAMES sqlite3
        PATHS "C:/msys64/ucrt64/bin" "C:/msys64/mingw64/bin" "C:/msys64/usr/bin"
        NO_DEFAULT_PATH)
endif()

add_custom_target(stage_runtime ALL
    COMMAND ${CMAKE_COMMAND}
        -DHIASM_BINARY_DIR=${CMAKE_BINARY_DIR}
        -DHIASM_RESOURCES_DIR=${CMAKE_CURRENT_SOURCE_DIR}/resources
        -DHIASM_PACKS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/packs
        -DHIASM_SQLITE3=${SQLITE3_EXECUTABLE}
        -DHIASM_GTK_VERSION=${HIASM_GTK_VERSION}
        -DHIASM_GTK4_RUNTIME_DIR=${CMAKE_CURRENT_SOURCE_DIR}/build/deps/gtk4-win32
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/StageRuntime.cmake"
    COMMENT "Staging HiAsm runtime (resources + packs -> packs)"
    VERBATIM)

add_dependencies(stage_runtime hiasm hiasm_runner text_project_maker)
if(HIASM_BUILD_RTCG)
    add_dependencies(stage_runtime rtcg_codegen)
endif()
if(HIASM_BUILD_FTCG)
    add_dependencies(stage_runtime ftcg)
endif()

if(HIASM_GTK_VERSION STREQUAL "4")
    set(HIASM_GTK_THEME_FILE "${CMAKE_BINARY_DIR}/hiasm.css")
else()
    set(HIASM_GTK_THEME_FILE "${CMAKE_BINARY_DIR}/gtkrc")
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(core_welcome_test tests/core_welcome_test.cpp)
    target_link_libraries(core_welcome_test PRIVATE hiasm_core)
    add_test(NAME core_welcome COMMAND core_welcome_test "${CMAKE_BINARY_DIR}")
    set_tests_properties(core_welcome PROPERTIES TIMEOUT 30)
    add_executable(external_form_helper tests/external_form_helper.cpp)
    add_executable(external_form_editor_test tests/external_form_editor_test.cpp)
    target_link_libraries(external_form_editor_test PRIVATE hiasm_core)
    add_test(NAME external_form_editor COMMAND external_form_editor_test "${CMAKE_BINARY_DIR}" $<TARGET_FILE:external_form_helper>)
    set_tests_properties(external_form_editor PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE)
    add_executable(legacy_catalog_test tests/legacy_catalog_test.cpp src/LegacyPackCatalog.cpp)
    target_include_directories(legacy_catalog_test PRIVATE src)
    target_link_libraries(legacy_catalog_test PRIVATE PkgConfig::GIO)
    add_test(NAME legacy_catalog COMMAND legacy_catalog_test)
    set_tests_properties(legacy_catalog PROPERTIES TIMEOUT 30)
    find_program(GIT_EXECUTABLE NAMES git)
    if(GIT_EXECUTABLE)
        add_executable(git_client_test tests/git_client_test.cpp src/GitClient.cpp)
        target_include_directories(git_client_test PRIVATE src)
        target_link_libraries(git_client_test PRIVATE PkgConfig::GIO)
        add_test(NAME git_client COMMAND git_client_test "${GIT_EXECUTABLE}")
        set_tests_properties(git_client PROPERTIES TIMEOUT 60)
    endif()
    add_executable(legacy_pack_test tests/legacy_pack_test.cpp)
    target_link_libraries(legacy_pack_test PRIVATE hiasm_core)
    add_test(NAME legacy_pack COMMAND legacy_pack_test "${CMAKE_BINARY_DIR}")
    set_tests_properties(legacy_pack PROPERTIES TIMEOUT 30)
    find_program(SVN_EXECUTABLE NAMES svn)
    find_program(SVNADMIN_EXECUTABLE NAMES svnadmin)
    if(SVN_EXECUTABLE AND SVNADMIN_EXECUTABLE)
        add_executable(svn_test tests/svn_test.cpp src/SvnClient.cpp)
        target_include_directories(svn_test PRIVATE src)
        target_link_libraries(svn_test PRIVATE PkgConfig::GIO)
        add_test(NAME svn COMMAND svn_test "${SVN_EXECUTABLE}" "${SVNADMIN_EXECUTABLE}")
        set_tests_properties(svn PROPERTIES TIMEOUT 60)
    endif()
    add_executable(form_editor_test tests/form_editor_test.cpp)
    target_link_libraries(form_editor_test PRIVATE hiasm_core)
    add_test(NAME form_editor COMMAND form_editor_test "${CMAKE_BINARY_DIR}")
    set_tests_properties(form_editor PROPERTIES TIMEOUT 30)
    add_executable(nested_scheme_test tests/nested_scheme_test.cpp)
    target_link_libraries(nested_scheme_test PRIVATE hiasm_core)
    add_test(NAME nested_scheme COMMAND nested_scheme_test "${CMAKE_BINARY_DIR}")
    set_tests_properties(nested_scheme PROPERTIES TIMEOUT 60)
    add_executable(legacy_insert_test tests/legacy_insert_test.cpp)
    target_link_libraries(legacy_insert_test PRIVATE hiasm_core)
    add_test(NAME legacy_insert COMMAND legacy_insert_test "${CMAKE_BINARY_DIR}")
    set_tests_properties(legacy_insert PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
    add_executable(project_manager_test tests/project_manager_test.cpp)
    target_link_libraries(project_manager_test PRIVATE hiasm_core)
    add_test(NAME project_manager COMMAND project_manager_test "${CMAKE_BINARY_DIR}")
    set_tests_properties(project_manager PROPERTIES TIMEOUT 60)
    # Not a test: loads the examples of one HiAsm 4 package folder and reports what the loader says.
    add_executable(legacy_load_probe tests/legacy_load_probe.cpp)
    target_link_libraries(legacy_load_probe PRIVATE hiasm_core)
    add_executable(legacy_build_probe tests/legacy_build_probe.cpp)
    target_link_libraries(legacy_build_probe PRIVATE hiasm_core)
    add_executable(new_project_test tests/new_project_test.cpp)
    target_link_libraries(new_project_test PRIVATE hiasm_core)
    add_test(NAME new_project COMMAND new_project_test "${CMAKE_BINARY_DIR}")
    set_tests_properties(new_project PROPERTIES TIMEOUT 60)
    add_executable(cpp_package_test tests/cpp_package_test.cpp)
    target_link_libraries(cpp_package_test PRIVATE hiasm_core)
    add_test(NAME cpp_package COMMAND cpp_package_test "${CMAKE_BINARY_DIR}")
    set_tests_properties(cpp_package PROPERTIES TIMEOUT 30)
    if(HIASM_BUILD_RTCG)
        add_executable(cpp_hc_test tests/cpp_hc_test.cpp)
        target_link_libraries(cpp_hc_test PRIVATE hiasm_core)
        add_dependencies(cpp_hc_test rtcg_codegen)
        add_test(NAME cpp_hc COMMAND cpp_hc_test "${CMAKE_BINARY_DIR}")
        set_tests_properties(cpp_hc PROPERTIES TIMEOUT 600)
        add_executable(cpp_widgets_test tests/cpp_widgets_test.cpp)
        target_link_libraries(cpp_widgets_test PRIVATE hiasm_core)
        add_dependencies(cpp_widgets_test rtcg_codegen)
        add_test(NAME cpp_widgets COMMAND cpp_widgets_test "${CMAKE_BINARY_DIR}")
        set_tests_properties(cpp_widgets PROPERTIES TIMEOUT 240)
        if(WIN32)
            add_executable(cpp_entries_test tests/cpp_entries_test.cpp)
            target_link_libraries(cpp_entries_test PRIVATE hiasm_core)
            add_dependencies(cpp_entries_test rtcg_codegen)
            add_test(NAME cpp_entries COMMAND cpp_entries_test "${CMAKE_BINARY_DIR}")
            set_tests_properties(cpp_entries PROPERTIES TIMEOUT 600)
        endif()
    endif()
    add_executable(pack_database_test tests/pack_database_test.cpp)
    target_link_libraries(pack_database_test PRIVATE hiasm_core)
    add_test(NAME pack_database COMMAND pack_database_test "${CMAKE_BINARY_DIR}")
    set_tests_properties(pack_database PROPERTIES TIMEOUT 30)
    add_executable(settings_test tests/settings_test.cpp)
    target_link_libraries(settings_test PRIVATE hiasm_core)
    add_test(NAME settings COMMAND settings_test "${CMAKE_BINARY_DIR}")
    set_tests_properties(settings PROPERTIES TIMEOUT 30)
    add_executable(runner_payload_test tests/runner_payload_test.cpp)
    target_include_directories(runner_payload_test PRIVATE src)
    add_test(NAME runner_payload COMMAND runner_payload_test)
    add_executable(property_value_test tests/property_value_test.cpp)
    target_link_libraries(property_value_test PRIVATE hiasm_core)
    add_test(NAME property_value COMMAND property_value_test)
    set_tests_properties(property_value runner_payload PROPERTIES TIMEOUT 30)
    add_executable(editor_commands_test tests/editor_commands_test.cpp)
    target_link_libraries(editor_commands_test PRIVATE hiasm_core)
    add_test(NAME editor_commands COMMAND editor_commands_test "${CMAKE_BINARY_DIR}")
    set_tests_properties(editor_commands PROPERTIES TIMEOUT 30)
    add_executable(scheme_history_test tests/scheme_history_test.cpp)
    target_link_libraries(scheme_history_test PRIVATE hiasm_core)
    add_test(NAME scheme_history COMMAND scheme_history_test "${CMAKE_BINARY_DIR}")
    set_tests_properties(scheme_history PROPERTIES TIMEOUT 30)
    add_executable(layer_hint_test tests/layer_hint_test.cpp)
    target_link_libraries(layer_hint_test PRIVATE hiasm_core)
    add_test(NAME layer_hint COMMAND layer_hint_test "${CMAKE_BINARY_DIR}")
    set_tests_properties(layer_hint PROPERTIES TIMEOUT 30)
    if(WIN32 AND HIASM_GTK_VERSION STREQUAL "4")
        add_executable(native_frame_test tests/native_frame_test.cpp)
        target_link_libraries(native_frame_test PRIVATE hiasm_core)
        add_test(NAME native_frame COMMAND native_frame_test)
        set_tests_properties(native_frame PROPERTIES TIMEOUT 30)
        add_executable(win32_ime_test tests/win32_ime_test.cpp)
        target_link_libraries(win32_ime_test PRIVATE hiasm_core)
        add_test(NAME win32_ime COMMAND win32_ime_test)
        set_tests_properties(win32_ime PROPERTIES TIMEOUT 30 ENVIRONMENT "G_DEBUG=fatal-criticals")
    endif()
    if(HIASM_GTK_VERSION STREQUAL "4")
        add_executable(repository_panel_test tests/repository_panel_test.cpp)
        target_link_libraries(repository_panel_test PRIVATE hiasm_core)
        add_test(NAME repository_panel COMMAND repository_panel_test "${CMAKE_BINARY_DIR}")
        set_tests_properties(repository_panel PROPERTIES TIMEOUT 30 ENVIRONMENT "G_DEBUG=fatal-criticals")
        add_executable(dock_workspace_test tests/dock_workspace_test.cpp)
        target_link_libraries(dock_workspace_test PRIVATE hiasm_core)
        add_test(NAME dock_workspace COMMAND dock_workspace_test)
        set_tests_properties(dock_workspace PROPERTIES TIMEOUT 30
            ENVIRONMENT "G_DEBUG=fatal-criticals")
        add_executable(main_docks_test tests/main_docks_test.cpp)
        target_link_libraries(main_docks_test PRIVATE hiasm_core)
        add_test(NAME main_docks COMMAND main_docks_test "${CMAKE_BINARY_DIR}")
        set_tests_properties(main_docks PROPERTIES TIMEOUT 30
            ENVIRONMENT "G_DEBUG=fatal-criticals")
        add_executable(source_editor_test tests/source_editor_test.cpp)
        target_link_libraries(source_editor_test PRIVATE hiasm_core)
        add_test(NAME source_editor COMMAND source_editor_test "${CMAKE_BINARY_DIR}")
        set_tests_properties(source_editor PROPERTIES TIMEOUT 30)
    endif()
    add_executable(scheme_io_test tests/scheme_io_test.cpp)
    target_link_libraries(scheme_io_test PRIVATE hiasm_core)
    add_test(NAME scheme_io COMMAND scheme_io_test "${CMAKE_BINARY_DIR}")
    add_executable(process_helper tests/process_helper.cpp)
    add_executable(process_lifecycle_test tests/process_lifecycle_test.cpp)
    target_link_libraries(process_lifecycle_test PRIVATE hiasm_core)
    add_test(NAME process_lifecycle COMMAND process_lifecycle_test $<TARGET_FILE:process_helper> "${CMAKE_BINARY_DIR}")
    set_tests_properties(scheme_io process_lifecycle PROPERTIES TIMEOUT 30)
    add_test(NAME runner_validate COMMAND hiasm_runner --validate --scheme
        "${CMAKE_CURRENT_SOURCE_DIR}/packs/_base/new/hcTplForm.sha" --data-dir "${CMAKE_BINARY_DIR}")
    if(HIASM_BUILD_RTCG AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packs/cpp/code/core_runtime.h")
        add_executable(cpp_core_runtime_test tests/cpp_core_runtime_test.cpp)
        set_target_properties(cpp_core_runtime_test PROPERTIES CXX_STANDARD 17)
        add_test(NAME cpp_core_runtime COMMAND cpp_core_runtime_test)
    endif()
    if(HIASM_BUILD_RTCG)
        add_executable(rtcg_test tests/rtcg_test.cpp)
        target_link_libraries(rtcg_test PRIVATE hiasm_core)
        add_test(NAME rtcg COMMAND rtcg_test "${CMAKE_BINARY_DIR}" $<TARGET_FILE:rtcg_codegen>)
        # Compiles multiple generated GUI programs and can also launch WSL/Wine.
        set_tests_properties(rtcg PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
        add_executable(web_codegen_test tests/web_codegen_test.cpp)
        target_link_libraries(web_codegen_test PRIVATE hiasm_core)
        add_dependencies(web_codegen_test rtcg_codegen stage_runtime)
        add_test(NAME web_codegen COMMAND web_codegen_test "${CMAKE_BINARY_DIR}")
        set_tests_properties(web_codegen PROPERTIES TIMEOUT 60)
        add_executable(modules_codegen_test tests/modules_codegen_test.cpp)
        target_link_libraries(modules_codegen_test PRIVATE hiasm_core)
        add_dependencies(modules_codegen_test rtcg_codegen stage_runtime)
        add_test(NAME modules_codegen COMMAND modules_codegen_test "${CMAKE_BINARY_DIR}")
        set_tests_properties(modules_codegen PROPERTIES TIMEOUT 60)
    endif()
    if(HIASM_BUILD_CGBRIDGE)
        # Needs a HiAsm 4 installation; the test is skipped while the path is empty.
        set(HIASM4_TEST_ROOT "" CACHE PATH "HiAsm 4 installation used by the cgbridge test")
        add_executable(cgbridge_test tests/cgbridge_test.cpp)
        target_link_libraries(cgbridge_test PRIVATE hiasm_core)
        add_dependencies(cgbridge_test hiasm4_cgbridge)
        add_test(NAME cgbridge COMMAND cgbridge_test "${CMAKE_BINARY_DIR}" "${CGBRIDGE_OUTPUT}" "${HIASM4_TEST_ROOT}")
        set_tests_properties(cgbridge PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
    endif()
endif()

install(TARGETS hiasm hiasm_runner RUNTIME DESTINATION .)
install(FILES docs/cpp17.md docs/hiasm4-packs.md DESTINATION docs)
install(FILES tools/setup_wsl.ps1 tools/setup_cpp_linux.sh tools/setup_wine.ps1 DESTINATION tools)
install(FILES tools/package_cpp_windows.py DESTINATION tools)
install(PROGRAMS tools/cpp_wine_tool.sh tools/cpp_wine_gxx.sh tools/cpp_wine_pkg_config.sh DESTINATION tools)
if(WIN32 AND HIASM_GTK_VERSION STREQUAL "4" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build/deps/gtk4-win32/libgtk-4-1.dll")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/build/deps/gtk4-win32/libgtk-4-1.dll" DESTINATION .)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/build/deps/gtk4-win32/libaccesskit-c-0.18.dll" DESTINATION .)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/build/deps/gtk4-win32/COPYING" DESTINATION licenses/gtk4)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/build/deps/gtk4-win32/gtk-4.24.1.tar.xz"
        "${CMAKE_CURRENT_SOURCE_DIR}/build/deps/gtk4-win32/SOURCES.txt" DESTINATION licenses/gtk4)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/deps/gtk4-win32/accesskit/" DESTINATION licenses/accesskit)
    install(FILES tools/build_gtk4_runtime.ps1 tools/patches/gtk4-win32-dnd-startup.patch
        tools/patches/gtk4-win32-ime-lifecycle.patch DESTINATION licenses/gtk4)
endif()
install(DIRECTORY "${CMAKE_BINARY_DIR}/int" DESTINATION .)
install(DIRECTORY "${CMAKE_BINARY_DIR}/packs" DESTINATION .
    PATTERN ".goutputstream*" EXCLUDE
    REGEX "/(new|example|tutorial|plugs)/build(/|$)" EXCLUDE
    REGEX "/(new|example|tutorial|plugs)/[^/]*[.](exe|o|obj|dll|so)$" EXCLUDE)
install(FILES
    "${HIASM_GTK_THEME_FILE}"
    "${CMAKE_BINARY_DIR}/splash.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    DESTINATION .)
