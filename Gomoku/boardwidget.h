#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPair>
#include "ai.h"

// ============================================================
//  BoardWidget —— 棋盘控件
//  负责:绘制棋盘/棋子、处理鼠标点击、维护对局状态、
//        胜负判定,并在人机模式下调遣 AI 落子。
//  数据模型:board[row][col] = 0 空 / 1 黑 / 2 白
// ============================================================
class BoardWidget : public QWidget
{
    Q_OBJECT
public:
    // 对战模式:PvP 双人 / PvE 人机 / Online 联机
    enum GameMode { PvP, PvE, Online };

    explicit BoardWidget(QWidget *parent = nullptr);

    void setMode(GameMode m);          // 切换模式
    void setDifficulty(Difficulty d);  // 切换 AI 难度
    void setLocalPlayer(int p);        // 联机模式:设置本机执黑(1)或执白(2)
    void restart();                    // 重新开局
    void undo();                       // 悔棋(人机模式悔两步)
    void applyRemoteMove(int row, int col);  // 联机:应用对手发来的落子

    QSize sizeHint() const override;

signals:
    void statusChanged(const QString &text);  // 状态变化(轮到谁/胜负)
    void gameOver(int winner);                // 0 平局 1 黑 2 白
    void stonePlaced(int row, int col);       // 联机:本机落子后通知主窗口转发

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void leaveEvent(QEvent *) override;

private:
    static const int BOARD_SIZE = 15;   // 15x15

    QVector<QVector<int>> board;        // 棋盘数据
    int currentPlayer;                  // 当前轮:1 黑 / 2 白
    GameMode mode;                      // 当前模式
    Difficulty difficulty;              // AI 难度
    int localPlayer;                    // 联机模式:本机所执颜色 1 黑 / 2 白
    int lastRow, lastCol;               // 上一手坐标(画红点标记)
    int hoverRow, hoverCol;             // 鼠标悬停坐标(画半透明预览)
    bool gameEnded;                     // 对局是否结束
    GomokuAI ai;                        // AI 对象
    QVector<QPair<int, int>> history;   // 落子历史,用于悔棋

    // 棋盘几何信息(每次重绘时按窗口尺寸动态计算)
    struct Geometry { int originX, originY, cellSize; };
    Geometry computeGeometry() const;

    void drawStone(QPainter &p, int x, int y, int radius, int player);
    void doPlace(int row, int col);     // 落子核心流程(本地与远程共用)
    bool checkWin(int row, int col);    // 以 (row,col) 为中心判胜
    void aiMove();                      // 触发 AI 落子
    void updateStatusText();            // 刷新状态文本并发射信号
};

#endif // BOARDWIDGET_H
