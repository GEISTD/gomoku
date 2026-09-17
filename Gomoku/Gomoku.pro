# ============================================================
#  五子棋 Gomoku —— qmake 工程文件(适配 Qt5 + MinGW)
#  用法:
#    qmake Gomoku.pro
#    mingw32-make
# ============================================================
QT       += widgets network
CONFIG   += c++14
CONFIG   += release       # 发布版,体积小、跑得快

TARGET    = Gomoku
TEMPLATE  = app

# 源文件与头文件
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    boardwidget.cpp \
    ai.cpp \
    netmanager.cpp

HEADERS += \
    mainwindow.h \
    boardwidget.h \
    ai.h \
    netmanager.h

# Windows 下生成无控制台窗口的 GUI 程序
win32 {
    RC_ICONS =
}
