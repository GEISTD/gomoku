@echo off
chcp 65001 >nul
REM ============================================================
REM  清理五子棋项目的编译产物与无用文件,仅保留源码与构建脚本
REM  双击运行即可
REM ============================================================
echo 正在清理编译产物...

REM 删除 CMake 构建目录
if exist build rmdir /s /q build
if exist cmake-build-debug rmdir /s /q cmake-build-debug
if exist cmake-build-release rmdir /s /q cmake-build-release
if exist out rmdir /s /q out

REM 删除编译产生的临时文件
del /s /q *.o *.obj *.pdb *.ilk *.exp 2>nul
del /q Gomoku.exe 2>nul

echo 清理完成。
pause
