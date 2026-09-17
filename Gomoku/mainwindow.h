#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "boardwidget.h"

class QPushButton;
class QLabel;
class NetManager;

// ============================================================
//  MainWindow —— 主窗口
//  左侧控制面板(模式/难度/重开/悔棋/状态) + 右侧棋盘
// ============================================================
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onModePvP();                          // 切到双人
    void onModePvE();                          // 切到人机
    void onModeOnline();                       // 切到联机(弹出连接设置)
    void onRestart();
    void onUndo();
    void onStatusChanged(const QString &text); // 更新状态标签
    void onGameOver(int winner);               // 弹出胜负提示
    // 联机网络回调
    void onNetConnected();
    void onNetFailed(const QString &reason);
    void onNetMoveReceived(int row, int col);
    void onNetResetReceived();
    void onNetDisconnected();
    void onLocalStonePlaced(int row, int col); // 本机落子 -> 转发给对手

private:
    BoardWidget *board;
    QPushButton *btnPvP, *btnPvE, *btnOnline;
    QPushButton *btnEasy, *btnMedium, *btnHard;
    QPushButton *btnRestart, *btnUndo;
    QLabel      *statusLabel;
    Difficulty   currentDiff;                  // 当前 AI 难度
    NetManager  *net;                          // 网络管理器
    int          onlineLocalPlayer;            // 联机:本机执黑(1)/执白(2)
    bool         onlineActive;                 // 是否处于联机对局

    void setDifficultyEnabled(bool on);        // 人机模式才启用难度按钮
    void setOnlineMode(bool on);               // 进入/退出联机时的控件状态
    void showConnectDialog();                  // 联机连接设置对话框
    QStringList localIps();                    // 获取本机 IPv4 地址
    void applyStyle();                         // 统一 QSS 样式
};

#endif // MAINWINDOW_H
