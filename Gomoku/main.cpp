#include "mainwindow.h"
#include <QApplication>

// 程序入口
// Qt 程序的标准结构:创建 QApplication,构造主窗口,进入事件循环
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Gomoku 五子棋");

    MainWindow w;
    w.show();

    return app.exec();   // 启动 Qt 事件循环,直到窗口关闭
}
