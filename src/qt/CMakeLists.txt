find_package(Qt6 6.9 REQUIRED COMPONENTS Widgets Sql LinguistTools)

qt_wrap_ui(HIASM_PROJECT_MANAGER_UI "${PROJECT_SOURCE_DIR}/ui/ProjectManager.ui")
add_library(hiasm-project-dialog STATIC package_paths.cpp project_manager_dialog.cpp ${HIASM_PROJECT_MANAGER_UI})
target_include_directories(hiasm-project-dialog PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(hiasm-project-dialog PUBLIC Qt6::Widgets)
target_compile_definitions(hiasm-project-dialog PRIVATE HIASM_SOURCE_PACKS="${PROJECT_SOURCE_DIR}/packs")

qt_wrap_ui(HIASM_MAIN_WINDOW_UI
    "${PROJECT_SOURCE_DIR}/ui/MainWindow.ui"
    "${PROJECT_SOURCE_DIR}/ui/AboutDialog.ui"
    "${PROJECT_SOURCE_DIR}/ui/SchemeEditor.ui")
add_library(hiasm-qt-window OBJECT form_canvas.cpp form_canvas.h element_config.cpp main_window.cpp main_window.h scheme_editor.cpp scheme_editor.h
    legacy_build.cpp legacy_snapshot.cpp
    scheme_diagram.cpp scheme_diagram.h workspace_panels.cpp workspace_panels.h ${HIASM_MAIN_WINDOW_UI}
    "${PROJECT_SOURCE_DIR}/resources/qt_ide.qrc")
set_target_properties(hiasm-qt-window PROPERTIES
    AUTOUIC OFF
    AUTOMOC ON
    AUTORCC ON)
target_link_libraries(hiasm-qt-window PUBLIC hiasm-project-dialog Qt6::Sql)
target_compile_definitions(hiasm-qt-window PRIVATE HIASM_SOURCE_PACKS="${PROJECT_SOURCE_DIR}/packs")
target_compile_definitions(hiasm-qt-window PRIVATE HIASM_QT_VERSION="${PROJECT_VERSION}")

qt_add_executable(hiasm main_window_main.cpp scheme_build.cpp scheme_codegen.cpp)
set_target_properties(hiasm PROPERTIES MACOSX_BUNDLE FALSE)
target_link_libraries(hiasm PRIVATE hiasm-qt-window)
if(TARGET hiasm4_cgbridge)
    add_dependencies(hiasm hiasm4_cgbridge)
    target_compile_definitions(hiasm-qt-window PRIVATE HIASM_BUILD_BRIDGE="${CGBRIDGE_OUTPUT}")
endif()

get_filename_component(qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
find_program(HIASM_PROJECT_NINJA NAMES ninja HINTS "${qt_prefix}/../../Tools/Ninja")
target_compile_definitions(hiasm PRIVATE
    HIASM_SOURCE_PACKS="${PROJECT_SOURCE_DIR}/packs"
    HIASM_BUILD_QT_PREFIX="${qt_prefix}"
    HIASM_BUILD_CMAKE="${CMAKE_COMMAND}"
    HIASM_BUILD_COMPILER="${CMAKE_CXX_COMPILER}"
    HIASM_BUILD_NINJA="${HIASM_PROJECT_NINJA}")

qt_add_library(hiasm-core-qt SHARED core_widgets.cpp core_widgets.h)
target_link_libraries(hiasm-core-qt PRIVATE Qt6::Widgets)
if(WIN32)
    set_target_properties(hiasm-core-qt PROPERTIES PREFIX "")
endif()
qt_add_translations(TARGETS hiasm
    SOURCE_TARGETS hiasm-project-dialog hiasm-qt-window
    TS_FILES "${PROJECT_SOURCE_DIR}/translations/hiasm_ru.ts"
    RESOURCE_PREFIX "/i18n")
if(WIN32)
    set(hiasm_app_dir qt)
else()
    set(hiasm_app_dir bin)
endif()
install(TARGETS hiasm hiasm-core-qt
    RUNTIME DESTINATION ${hiasm_app_dir} LIBRARY DESTINATION ${hiasm_app_dir})
if(TARGET hiasm4_cgbridge)
    install(PROGRAMS "${CGBRIDGE_OUTPUT}" DESTINATION ${hiasm_app_dir})
endif()
if(WIN32)
    find_program(HIASM_WINDEPLOYQT NAMES windeployqt
        PATHS "${Qt6_DIR}/../../../bin" NO_DEFAULT_PATH REQUIRED)
    set(deploy_options --no-translations --include-plugins qoffscreen --skip-plugin-types sqldrivers)
    add_custom_command(TARGET hiasm POST_BUILD
        COMMAND "${HIASM_WINDEPLOYQT}" ${deploy_options} "$<TARGET_FILE:hiasm>"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:hiasm>/sqldrivers"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:Qt6::QSQLiteDriverPlugin>"
            "$<TARGET_FILE_DIR:hiasm>/sqldrivers"
        VERBATIM)
    install(CODE "
        execute_process(COMMAND \"${HIASM_WINDEPLOYQT}\" ${deploy_options}
            \"\${CMAKE_INSTALL_PREFIX}/qt/hiasm.exe\"
            COMMAND_ERROR_IS_FATAL ANY)")
    install(FILES "$<TARGET_FILE:Qt6::QSQLiteDriverPlugin>" DESTINATION qt/sqldrivers)
    if(MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # windeployqt 6.9 leaves out the LLVM-MinGW runtime DLLs.
        get_filename_component(HIASM_QT_COMPILER_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set(llvm_runtime "${HIASM_QT_COMPILER_BIN}/libc++.dll" "${HIASM_QT_COMPILER_BIN}/libunwind.dll")
        add_custom_command(TARGET hiasm POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${llvm_runtime} "$<TARGET_FILE_DIR:hiasm>" VERBATIM)
        install(FILES ${llvm_runtime} DESTINATION qt)
    endif()
endif()

if(BUILD_TESTING)
    qt_add_executable(qt_core_image_test "${PROJECT_SOURCE_DIR}/tests/qt_core_image_test.cpp")
    target_include_directories(qt_core_image_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(qt_core_image_test PRIVATE Qt6::Widgets)
    add_dependencies(qt_core_image_test hiasm-core-qt)
    add_test(NAME qt_core_image COMMAND qt_core_image_test $<TARGET_FILE:hiasm-core-qt>)
    add_test(NAME qt_core_expander COMMAND qt_core_image_test $<TARGET_FILE:hiasm-core-qt> expander)
    add_test(NAME qt_core_sdkedit COMMAND qt_core_image_test $<TARGET_FILE:hiasm-core-qt> sdkedit)
    add_test(NAME qt_main_window COMMAND hiasm --check)
    add_test(NAME qt_main_window_scheme COMMAND hiasm --check
        "${PROJECT_SOURCE_DIR}/packs/_base/example/tutorial/Index.sha")
    qt_add_executable(qt_main_dialogs_test "${PROJECT_SOURCE_DIR}/tests/qt_main_dialogs_test.cpp")
    target_include_directories(qt_main_dialogs_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(qt_main_dialogs_test PRIVATE hiasm-qt-window)
    add_test(NAME qt_main_dialogs COMMAND qt_main_dialogs_test)
    qt_add_executable(qt_scheme_editor_test "${PROJECT_SOURCE_DIR}/tests/qt_scheme_editor_test.cpp")
    target_include_directories(qt_scheme_editor_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(qt_scheme_editor_test PRIVATE hiasm-qt-window)
    add_test(NAME qt_scheme_editor COMMAND qt_scheme_editor_test
        "${PROJECT_SOURCE_DIR}/packs/_base/example/tutorial/Index.sha")
    qt_add_executable(qt_form_editor_test "${PROJECT_SOURCE_DIR}/tests/qt_form_editor_test.cpp")
    target_include_directories(qt_form_editor_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(qt_form_editor_test PRIVATE hiasm-qt-window)
    add_test(NAME qt_form_editor COMMAND qt_form_editor_test "${PROJECT_SOURCE_DIR}/packs/cpp/new/QtForm.sha")
    add_test(NAME qt_form_hierarchy COMMAND qt_form_editor_test --hierarchy
        "${PROJECT_SOURCE_DIR}/packs/cpp/example/Widgets/Notepad.sha")
    set(qt_tests qt_form_hierarchy qt_form_editor qt_main_window qt_main_window_scheme qt_main_dialogs qt_scheme_editor
        qt_core_image qt_core_expander qt_core_sdkedit)
    set_tests_properties(${qt_tests} PROPERTIES TIMEOUT 15
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_executable(qt_project_build_test "${PROJECT_SOURCE_DIR}/tests/qt_project_build_test.cpp")
    target_include_directories(qt_project_build_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(qt_project_build_test PRIVATE hiasm-qt-window)
    add_dependencies(qt_project_build_test hiasm)
    add_test(NAME qt_build_console COMMAND qt_project_build_test "${PROJECT_SOURCE_DIR}/packs/cpp/example/hello.sha" console)
    add_test(NAME qt_build_draft COMMAND qt_project_build_test "${PROJECT_SOURCE_DIR}/packs/cpp/example/hello.sha" draft)
    add_test(NAME qt_build_form COMMAND qt_project_build_test "${PROJECT_SOURCE_DIR}/packs/cpp/new/QtForm.sha" gui)
    add_test(NAME qt_build_core COMMAND qt_project_build_test "${PROJECT_SOURCE_DIR}/packs/_base/new/hcTplForm.sha" core)
    if(TARGET hiasm4_cgbridge)
        set(HIASM4_TEST_ROOT "" CACHE PATH "HiAsm 4 installation used by the Qt legacy build test")
        add_executable(qt_legacy_build_test "${PROJECT_SOURCE_DIR}/tests/qt_legacy_build_test.cpp")
        target_include_directories(qt_legacy_build_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        target_link_libraries(qt_legacy_build_test PRIVATE hiasm-qt-window user32)
        add_dependencies(qt_legacy_build_test hiasm)
        add_test(NAME qt_legacy_build COMMAND qt_legacy_build_test "$<TARGET_FILE:hiasm>" "${HIASM4_TEST_ROOT}")
        set_tests_properties(qt_legacy_build PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
        list(APPEND qt_tests qt_legacy_build)
    endif()
    set_tests_properties(qt_build_console qt_build_draft qt_build_form qt_build_core PROPERTIES TIMEOUT 100 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    list(APPEND qt_tests qt_build_console qt_build_draft qt_build_form qt_build_core)
    if(WIN32)
        set_property(TEST ${qt_tests} PROPERTY ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
endif()
