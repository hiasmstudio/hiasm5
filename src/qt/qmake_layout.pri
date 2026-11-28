win32-clang-g++:equals(QT_ARCH, x86_64) {
    CONFIG -= debug_and_release debug_and_release_target
    CONFIG(debug, debug|release): hiasmMode = Debug
    else: hiasmMode = Release
    hiasmRoot = $$clean_path($$PWD/../..)
    hiasmOutput = $$hiasmRoot/build/Windows_LLVM-MinGW-x64-$$hiasmMode/qmake
}
