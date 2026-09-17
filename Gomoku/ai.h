#ifndef AI_H
#define AI_H

#include <QVector>
#include <QPair>

// AI 难度枚举(用 enum class 防止命名污染)
enum class Difficulty { Easy, Medium, Hard };

// ============================================================
//  GomokuAI —— 五子棋电脑对手
//  采用"启发式评分"策略:
//    1. 只在已有棋子周围 2 格内寻找候选点(大幅缩小搜索范围)
//    2. 对每个空位,从 4 个方向评估它对某一方棋型的价值
//    3. 综合进攻分(自己下)与防守分(堵对手),选出最佳落子
//    4. 困难难度额外做一层前瞻,削弱对手的最强反击
// ============================================================
class GomokuAI
{
public:
    // 返回 AI 的最佳落子坐标 (row, col)
    QPair<int, int> getBestMove(const QVector<QVector<int>> &board, Difficulty diff);

private:
    static const int BOARD_SIZE = 15;   // 15x15 棋盘

    // 评估 (row,col) 这个点对 player 的总价值(4 个方向得分之和)
    int evaluatePoint(const QVector<QVector<int>> &board, int row, int col, int player);
    // 评估单条连线方向 (dx,dy) 的得分
    int scoreDirection(const QVector<QVector<int>> &board, int row, int col,
                       int dx, int dy, int player);
    // 获取候选落子点(已有棋子周围 2 格内的空位)
    QVector<QPair<int, int>> getCandidates(const QVector<QVector<int>> &board);
    // 坐标是否在棋盘内
    bool inBoard(int r, int c) const;
};

#endif // AI_H
