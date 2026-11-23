TEMPLATE = subdirs

!versionAtLeast(QT_VERSION, 6.9.0): error("HiAsm requires Qt 6.9 or later")

SUBDIRS += main_window rtcg core_qt
rtcg.file = src/RTCG/rtcg.pro
main_window.depends = rtcg
main_window.file = src/qt/main_window.pro
core_qt.file = src/qt/core_qt.pro
