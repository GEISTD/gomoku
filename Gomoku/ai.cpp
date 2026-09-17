#include "ai.h"
#include <QDateTime>
#include <climits>
#include <QtGlobal>

// 兼容 Qt 5.8:QRandomGenerator 在 5.10 才引入,这里用 qrand()
namespace {
    inline int randInt(int maxExclusive) {
        static bool seeded = false;
        if (!seeded) { qsrand((uint)QDateTime::currentMSecsSinceEpoch()); seeded = true; }
        return qrand() % maxExclusive;
    }
}

bool GomokuAI::inBoard(int r, int c) const
{
    return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE;
}

// ------------------------------------------------------------
//  获取候选点:遍历全盘,凡是空位且周围 2 格内有棋子,
//  就纳入候选。这样把 225 个点缩减到几十个,速度极快。
// ------------------------------------------------------------
QVector<QPair<int, int>> GomokuAI::getCandidates(const QVector<QVector<int>> &board)
{
    QVector<QPair<int, int>> cands;
    bool hasStone = false;

    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (board[r][c] != 0) { hasStone = true; continue; }
            bool near = false;
            // 检查周围 2 格内是否有棋子
            for (int dr = -2; dr <= 2 && !near; ++dr)
                for (int dc = -2; dc <= 2 && !near; ++dc) {
                    int nr = r + dr, nc = c + dc;
                    if (inBoard(nr, nc) && board[nr][nc] != 0)
                        near = true;
                }
            if (near) cands.append(qMakePair(r, c));
        }
    }

    // 空棋盘:返回空,调用方会下天元
    if (!hasStone) return {};
    return cands;
}

// ------------------------------------------------------------
//  评估单方向得分:
//  假设 player 在 (row,col) 落子,沿 (dx,dy) 及其反方向
//  统计连续同色棋子数 count,并记录两端是否为空(open)。
//  根据"连子数 + 开放端数"映射到棋型分数:
//    连五      => 100000  (必胜)
//    活四(双开)=> 10000   (下一步必胜)
//    冲四(单开)=> 1000
//    活三      => 1000    (威胁很大)
//    眠三      => 100
//    活二      => 100
//    眠二      => 10
// ------------------------------------------------------------
int GomokuAI::scoreDirection(const QVector<QVector<int>> &board, int row, int col,
                             int dx, int dy, int player)
{
    int count = 1;            // (row,col) 自身这一颗
    bool openA = false;       // 正方向端点是否开放(空)
    bool openB = false;       // 反方向端点是否开放(空)

    // 正方向数连续同色
    int r = row + dx, c = col + dy;
    while (inBoard(r, c) && board[r][c] == player) { ++count; r += dx; c += dy; }
    if (inBoard(r, c) && board[r][c] == 0) openA = true;

    // 反方向数连续同色
    r = row - dx; c = col - dy;
    while (inBoard(r, c) && board[r][c] == player) { ++count; r -= dx; c -= dy; }
    if (inBoard(r, c) && board[r][c] == 0) openB = true;

    int openCount = (openA ? 1 : 0) + (openB ? 1 : 0);

    if (count >= 5) return 100000;                 // 连五
    if (count == 4) {
        if (openCount == 2) return 10000;          // 活四
        if (openCount == 1) return 1000;           // 冲四
        return 0;
    }
    if (count == 3) {
        if (openCount == 2) return 1000;           // 活三
        if (openCount == 1) return 100;            // 眠三
        return 0;
    }
    if (count == 2) {
        if (openCount == 2) return 100;            // 活二
        if (openCount == 1) return 10;             // 眠二
        return 0;
    }
    if (count == 1) {
        if (openCount == 2) return 10;
        if (openCount == 1) return 1;
        return 0;
    }
    return 0;
}

// 一个点的总价值 = 4 个方向(横、竖、两条斜线)得分之和
int GomokuAI::evaluatePoint(const QVector<QVector<int>> &board, int row, int col, int player)
{
    static const int dirs[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    int total = 0;
    for (const auto &d : dirs)
        total += scoreDirection(board, row, col, d[0], d[1], player);
    return total;
}

// ------------------------------------------------------------
//  选出最佳落子:
//    score = 进攻分 + 防守分 * 权重
//  难度通过权重 + 随机性 + 前瞻来区分:
//    简单权重低、有随机;普通最优;普通最优 + 1 层前瞻。
// ------------------------------------------------------------
QPair<int, int> GomokuAI::getBestMove(const QVector<QVector<int>> &board, Difficulty diff)
{
    const int AI = 2, HUMAN = 1;
    auto candidates = getCandidates(board);
    if (candidates.isEmpty()) return qMakePair(7, 7);        // 空盘下天元

    // 防守权重:简单偏低(不爱防守),困难偏高(谨慎)
    double defWeight = 0.9;
    if (diff == Difficulty::Easy)   defWeight = 0.5;
    else if (diff == Difficulty::Hard) defWeight = 1.1;

    QVector<QPair<int, QPair<int, int>>> scored;    // (分数, 坐标)
    int bestScore = INT_MIN;

    for (const auto &cand : candidates) {
        int r = cand.first, col = cand.second;
        int attack  = evaluatePoint(board, r, col, AI);    // AI 下这里能形成的威胁
        int defense = evaluatePoint(board, r, col, HUMAN); // 堵在这里能化解的威胁

        int score;
        if (attack >= 100000)       score = attack;    // 能连五,直接赢
        else if (defense >= 100000) score = defense;   // 必须堵对手连五
        else                        score = int(attack + defense * defWeight);

        // 困难:模拟落子后,看对手最强反击,从分值里扣除
        if (diff == Difficulty::Hard && attack < 100000) {
            QVector<QVector<int>> tmp = board;
            tmp[r][col] = AI;
            auto oppCands = getCandidates(tmp);
            int oppBest = 0;
            for (const auto &oc : oppCands)
                oppBest = qMax(oppBest, evaluatePoint(tmp, oc.first, oc.second, HUMAN));
            score -= oppBest;
        }

        scored.append(qMakePair(score, cand));
        bestScore = qMax(bestScore, score);
    }

    // 简单:在"不低于最高分 60%"的候选里随机,故意不那么强
    if (diff == Difficulty::Easy) {
        QVector<QPair<int, int>> pool;
        for (const auto &s : scored)
            if (s.first >= bestScore * 0.6) pool.append(s.second);
        if (!pool.isEmpty())
            return pool.at(randInt(pool.size()));
    }

    // 普通/困难:在最高分候选里随机选一个(避免每局完全一样)
    QVector<QPair<int, int>> best;
    for (const auto &s : scored)
        if (s.first == bestScore) best.append(s.second);
    return best.at(randInt(best.size()));
}
