#include "boardwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <QtMath>
#include <cmath>

// ============================================================
//  构造:初始化 15x15 空棋盘,默认双人模式,黑棋先行
// ============================================================
BoardWidget::BoardWidget(QWidget *parent)
    : QWidget(parent),
      board(BOARD_SIZE, QVector<int>(BOARD_SIZE, 0)),
      currentPlayer(1), mode(PvP), difficulty(Difficulty::Medium), localPlayer(1),
      lastRow(-1), lastCol(-1), hoverRow(-1), hoverCol(-1),
      gameEnded(false)
{
    // 开启鼠标追踪,才能收到 mouseMoveEvent(用于悬停预览)
    setMouseTracking(true);
    setMinimumSize(560, 560);
    // 背景由自己绘制,不自动填充
    setAutoFillBackground(false);
}

QSize BoardWidget::sizeHint() const { return QSize(620, 620); }

void BoardWidget::setMode(GameMode m)      { mode = m; }
void BoardWidget::setDifficulty(Difficulty d) { difficulty = d; }
void BoardWidget::setLocalPlayer(int p)    { localPlayer = p; }

// ------------------------------------------------------------
//  计算棋盘几何:取窗口较短边,留出边距,平均分 14 个格子。
//  返回棋盘左上角网格点坐标和每格大小。
// ------------------------------------------------------------
BoardWidget::Geometry BoardWidget::computeGeometry() const
{
    int w = width(), h = height();
    int size = qMin(w, h);
    int margin = size / 18;                       // 留白
    int cellSize = (size - 2 * margin) / (BOARD_SIZE - 1);
    int boardPixels = cellSize * (BOARD_SIZE - 1);
    Geometry g;
    g.cellSize = cellSize;
    g.originX = (w - boardPixels) / 2;            // 水平居中
    g.originY = (h - boardPixels) / 2;            // 垂直居中
    return g;
}

// ------------------------------------------------------------
//  绘制:木板背景 -> 网格线 -> 星位 -> 棋子 -> 上一手标记 -> 悬停预览
// ------------------------------------------------------------
void BoardWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);      // 抗锯齿,棋子更圆润

    Geometry g = computeGeometry();

    // 1) 木纹渐变背景(上亮下暗,有质感)
    QLinearGradient wood(0, 0, 0, height());
    wood.setColorAt(0, QColor(232, 192, 110));
    wood.setColorAt(1, QColor(202, 158, 72));
    p.fillRect(rect(), wood);

    // 2) 网格线
    QPen gridPen(QColor(90, 55, 20), 1.4);
    p.setPen(gridPen);
    int boardPixels = g.cellSize * (BOARD_SIZE - 1);
    for (int i = 0; i < BOARD_SIZE; ++i) {
        p.drawLine(g.originX, g.originY + i * g.cellSize,
                   g.originX + boardPixels, g.originY + i * g.cellSize);
        p.drawLine(g.originX + i * g.cellSize, g.originY,
                   g.originX + i * g.cellSize, g.originY + boardPixels);
    }

    // 3) 星位(天元 + 四角) 0-indexed: (3,3)(3,11)(7,7)(11,3)(11,11)
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(90, 55, 20));
    const int stars[5][2] = {{3,3},{3,11},{7,7},{11,3},{11,11}};
    for (const auto &s : stars)
        p.drawEllipse(QPoint(g.originX + s[1]*g.cellSize, g.originY + s[0]*g.cellSize), 4, 4);

    // 4) 棋子
    int radius = g.cellSize / 2 - 2;
    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c)
            if (board[r][c] != 0)
                drawStone(p, g.originX + c*g.cellSize, g.originY + r*g.cellSize,
                          radius, board[r][c]);

    // 5) 上一手红点标记
    if (lastRow >= 0) {
        p.setPen(QPen(QColor(220, 40, 40), 2.2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPoint(g.originX + lastCol*g.cellSize, g.originY + lastRow*g.cellSize), 5, 5);
    }

    // 6) 悬停半透明预览(仅未结束且该位置空时)
    if (hoverRow >= 0 && hoverCol >= 0 &&
        board[hoverRow][hoverCol] == 0 && !gameEnded) {
        p.setOpacity(0.45);
        drawStone(p, g.originX + hoverCol*g.cellSize, g.originY + hoverRow*g.cellSize,
                  radius, currentPlayer);
        p.setOpacity(1.0);
    }
}

// ------------------------------------------------------------
//  绘制单颗棋子:投影 + 径向渐变 + 描边,呈现立体感
// ------------------------------------------------------------
void BoardWidget::drawStone(QPainter &p, int x, int y, int radius, int player)
{
    // 投影
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 60));
    p.drawEllipse(QPoint(x + 2, y + 3), radius, radius);

    // 主体渐变(高光偏向左上)
    QRadialGradient grad(x - radius / 3, y - radius / 3, radius);
    if (player == 1) {                 // 黑子
        grad.setColorAt(0.0, QColor(130, 130, 130));
        grad.setColorAt(0.5, QColor(45, 45, 45));
        grad.setColorAt(1.0, QColor(0, 0, 0));
    } else {                            // 白子
        grad.setColorAt(0.0, QColor(255, 255, 255));
        grad.setColorAt(0.7, QColor(235, 235, 235));
        grad.setColorAt(1.0, QColor(190, 190, 190));
    }
    p.setBrush(grad);
    p.drawEllipse(QPoint(x, y), radius, radius);

    // 细描边
    p.setPen(QPen(QColor(0, 0, 0, 70), 1));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPoint(x, y), radius, radius);
}

// ------------------------------------------------------------
//  鼠标移动:换算成棋盘坐标,刷新悬停预览
// ------------------------------------------------------------
void BoardWidget::mouseMoveEvent(QMouseEvent *e)
{
    Geometry g = computeGeometry();
    int col = std::lround((e->x() - g.originX) / (double)g.cellSize);
    int row = std::lround((e->y() - g.originY) / (double)g.cellSize);

    if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
        if (row != hoverRow || col != hoverCol) {
            hoverRow = row; hoverCol = col;
            update();
        }
    } else if (hoverRow != -1) {
        hoverRow = hoverCol = -1;
        update();
    }
}

void BoardWidget::leaveEvent(QEvent *)
{
    hoverRow = hoverCol = -1;
    update();
}

// ------------------------------------------------------------
//  鼠标点击:人下棋。人机模式下 AI 回合忽略点击
// ------------------------------------------------------------
void BoardWidget::mousePressEvent(QMouseEvent *e)
{
    if (gameEnded) return;
    if (mode == PvE && currentPlayer == 2) return;   // AI 思考中,忽略
    if (mode == Online && currentPlayer != localPlayer) return; // 联机:非自己回合忽略

    Geometry g = computeGeometry();
    int col = std::lround((e->x() - g.originX) / (double)g.cellSize);
    int row = std::lround((e->y() - g.originY) / (double)g.cellSize);
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) return;
    if (board[row][col] != 0) return;                // 已有棋子,忽略

    doPlace(row, col);
    // 联机模式:本机落子后通知主窗口,由它转发给对手
    if (mode == Online) emit stonePlaced(row, col);
}

// ------------------------------------------------------------
//  落子核心流程:
//  写入数据 -> 判胜 -> 判平局 -> 换人 -> (人机则触发AI)
//  本地落子与远程落子都走这里,区别只在调用方是否转发消息。
// ------------------------------------------------------------
void BoardWidget::doPlace(int row, int col)
{
    board[row][col] = currentPlayer;
    lastRow = row; lastCol = col;
    history.append(qMakePair(row, col));

    // 判胜
    if (checkWin(row, col)) {
        gameEnded = true;
        updateStatusText();
        emit gameOver(currentPlayer);
        update();
        return;
    }

    // 判平局(棋盘满)
    bool full = true;
    for (int r = 0; r < BOARD_SIZE && full; ++r)
        for (int c = 0; c < BOARD_SIZE && full; ++c)
            if (board[r][c] == 0) full = false;
    if (full) {
        gameEnded = true;
        emit statusChanged("平局!");
        emit gameOver(0);
        update();
        return;
    }

    // 换人
    currentPlayer = (currentPlayer == 1) ? 2 : 1;
    updateStatusText();
    update();

    // 人机模式且轮到 AI:延时 300ms 再落子,更像"思考"
    if (mode == PvE && currentPlayer == 2 && !gameEnded)
        QTimer::singleShot(300, this, &BoardWidget::aiMove);
}

void BoardWidget::aiMove()
{
    if (gameEnded) return;
    QPair<int, int> move = ai.getBestMove(board, difficulty);
    doPlace(move.first, move.second);
}

// 联机:应用对手发来的落子(只更新本地棋盘,不再转发)
void BoardWidget::applyRemoteMove(int row, int col)
{
    if (gameEnded) return;
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) return;
    if (board[row][col] != 0) return;                 // 防御:该位置已有子
    doPlace(row, col);
}

// ------------------------------------------------------------
//  胜负判定:以最新落子为中心,检查 4 个方向各连续同色是否 >=5
// ------------------------------------------------------------
bool BoardWidget::checkWin(int row, int col)
{
    int player = board[row][col];
    static const int dirs[4][2] = {{0,1},{1,0},{1,1},{1,-1}};
    for (const auto &d : dirs) {
        int count = 1;
        for (int s = 1; s < 5; ++s) {                       // 正向
            int r = row + d[0]*s, c = col + d[1]*s;
            if (r<0||r>=BOARD_SIZE||c<0||c>=BOARD_SIZE||board[r][c]!=player) break;
            ++count;
        }
        for (int s = 1; s < 5; ++s) {                       // 反向
            int r = row - d[0]*s, c = col - d[1]*s;
            if (r<0||r>=BOARD_SIZE||c<0||c>=BOARD_SIZE||board[r][c]!=player) break;
            ++count;
        }
        if (count >= 5) return true;
    }
    return false;
}

// ------------------------------------------------------------
//  重新开局:清空棋盘,黑棋先行
// ------------------------------------------------------------
void BoardWidget::restart()
{
    for (auto &row : board) row.fill(0);
    currentPlayer = 1;
    lastRow = lastCol = -1;
    hoverRow = hoverCol = -1;
    gameEnded = false;
    history.clear();
    updateStatusText();
    update();
}

// ------------------------------------------------------------
//  悔棋:
//    双人模式退 1 步;
//    人机模式退 2 步(自己 + AI),保证轮次回到玩家
// ------------------------------------------------------------
void BoardWidget::undo()
{
    if (gameEnded || history.isEmpty()) return;

    int steps = (mode == PvE && history.size() >= 2) ? 2 : 1;
    for (int i = 0; i < steps && !history.isEmpty(); ++i) {
        auto last = history.last();
        board[last.first][last.second] = 0;
        history.removeLast();
    }

    // 重算当前轮次与上一手标记
    if (mode == PvE) {
        currentPlayer = 1;                                  // 人机永远回到玩家
    } else {
        currentPlayer = (history.size() % 2 == 0) ? 1 : 2;  // 双人按历史长度推算
    }
    if (history.isEmpty()) { lastRow = lastCol = -1; }
    else { lastRow = history.last().first; lastCol = history.last().second; }

    gameEnded = false;
    updateStatusText();
    update();
}

// 把当前状态文案通过信号发给主窗口的状态栏
void BoardWidget::updateStatusText()
{
    QString text;
    if (gameEnded) {
        text = (currentPlayer == 1) ? "黑棋获胜!" : "白棋获胜!";
    } else if (mode == PvE && currentPlayer == 2) {
        text = "AI 思考中…";
    } else if (mode == Online) {
        text = (currentPlayer == localPlayer) ? "轮到你落子" : "等待对手落子…";
    } else {
        text = (currentPlayer == 1) ? "黑棋回合" : "白棋回合";
    }
    emit statusChanged(text);
}
