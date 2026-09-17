cmake_minimum_required(VERSION 3.16)
project(Gomoku VERSION 1.0 LANGUAGES CXX)

# ---------- 编译标准 ----------
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)          # 自动处理 Q_OBJECT 的 moc
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ---------- 查找 Qt(兼容 Qt5 / Qt6) ----------
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)

# ---------- 目标与源文件 ----------
add_executable(Gomoku WIN32
    main.cpp
    mainwindow.cpp   mainwindow.h
    boardwidget.cpp  boardwidget.h
    ai.cpp           ai.h
    netmanager.cpp   netmanager.h
)

target_link_libraries(Gomoku PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

# Windows 下生成独立的 .exe,不带控制台黑窗
if(WIN32)
    set_target_properties(Gomoku PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
