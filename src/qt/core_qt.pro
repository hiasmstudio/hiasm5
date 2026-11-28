TEMPLATE = lib
TARGET = hiasm-core-qt
QT += widgets
CONFIG += c++17 shared

include(qmake_layout.pri)
!isEmpty(hiasmOutput): DESTDIR = $$hiasmOutput

SOURCES += core_widgets.cpp
HEADERS += core_widgets.h
