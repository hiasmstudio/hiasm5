TEMPLATE = lib
TARGET = CodeGen
CONFIG -= qt
CONFIG += shared c++17
DEFINES += BUILDING_DLL HIASM5_ABI
SOURCES += $$files($$PWD/*.cpp)
HEADERS += $$files($$PWD/*.h)
# Place RTCG where the IDE can load it when compiling a scheme.
appOutput = $$clean_path($$OUT_PWD/../qt)
win32:CONFIG(debug, debug|release): appOutput = $$appOutput/debug
win32:CONFIG(release, debug|release): appOutput = $$appOutput/release
DESTDIR = $$appOutput/packs/cpp/compiler
