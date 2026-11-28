TEMPLATE = app
TARGET = hiasm
QT += widgets sql
DEFINES += HIASM_SOURCE_PACKS=\\\"$$clean_path($$PWD/../../packs)\\\"
CONFIG += c++17 console embed_translations qtc_runnable
VERSION = 6.0.0.0
DEFINES += HIASM_QT_VERSION=\\\"$$VERSION\\\"

include(qmake_layout.pri)
!isEmpty(hiasmOutput) {
    DESTDIR = $$hiasmOutput
    include(qmake_catalogs.pri)
}

CONFIG(debug, debug|release): buildVariant = debug
else: buildVariant = release
OBJECTS_DIR = .obj/hiasm/$$buildVariant
MOC_DIR = $$OBJECTS_DIR
RCC_DIR = $$OBJECTS_DIR
UI_DIR = $$OBJECTS_DIR

qtPrefix = $$[QT_INSTALL_PREFIX]
sdkTools = $$clean_path($$qtPrefix/../../Tools)
compilerPath = $$system_path($$QMAKE_CXX)
win32: compilerPath = $$clean_path($$sdkTools/llvm-mingw1706_64/bin/clang++.exe)
DEFINES += HIASM_BUILD_QT_PREFIX=\\\"$$qtPrefix\\\" HIASM_BUILD_COMPILER=\\\"$$compilerPath\\\"
win32: DEFINES += HIASM_BUILD_CMAKE=\\\"$$sdkTools/CMake_64/bin/cmake.exe\\\" HIASM_BUILD_NINJA=\\\"$$sdkTools/Ninja/ninja.exe\\\"
else: DEFINES += HIASM_BUILD_CMAKE=\\\"cmake\\\" HIASM_BUILD_NINJA=\\\"ninja\\\"

SOURCES += package_paths.cpp form_canvas.cpp element_config.cpp scheme_build.cpp scheme_codegen.cpp main_window_main.cpp main_window.cpp project_manager_dialog.cpp scheme_editor.cpp scheme_diagram.cpp
SOURCES += workspace_panels.cpp legacy_build.cpp legacy_snapshot.cpp
HEADERS += package_paths.h form_canvas.h main_window.h project_manager_dialog.h scheme_editor.h scheme_diagram.h ../SchemeSyntax.h
HEADERS += workspace_panels.h legacy_build.h legacy_snapshot.h
FORMS += ../../ui/MainWindow.ui ../../ui/ProjectManager.ui ../../ui/AboutDialog.ui ../../ui/SchemeEditor.ui
RESOURCES += ../../resources/qt_ide.qrc
TRANSLATIONS += ../../translations/hiasm_ru.ts
QM_FILES_RESOURCE_PREFIX = /i18n
LRELEASE_DIR = $$OBJECTS_DIR/translations

# rcc cannot discover the generated translation on a clean shadow build.
load(lrelease)
rcc.depends += $$QM_FILES

win32 {
    RC_ICONS = $$PWD/../../resources/icons/hiasm.ico
    qtBin = $$[QT_INSTALL_BINS]
    QMAKE_POST_LINK += $$shell_quote($$shell_path($$qtBin/windeployqt.exe)) --no-translations --include-plugins qoffscreen,qsqlite "$(DESTDIR_TARGET)"
    for(runtime, $$list(libc++.dll libunwind.dll)) {
        exists($$qtBin/$$runtime): QMAKE_POST_LINK += $$escape_expand(\n\t) $(QINSTALL) $$shell_quote($$shell_path($$qtBin/$$runtime)) $$shell_quote("$(strip $(DESTDIR))$$runtime")
    }
}
