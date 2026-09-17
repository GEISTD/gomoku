#include "mainwindow.h"
#include "netmanager.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QButtonGroup>
#include <QFrame>
#include <QMessageBox>
#include <QFont>
#include <QDialog>
#include <QRadioButton>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QNetworkInterface>
#include <QHostAddress>

// ============================================================
//  构造主窗口:搭建布局、连线、套用样式、开局
// ============================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentDiff(Difficulty::Medium),
      net(nullptr), onlineLocalPlayer(1), onlineActive(false)
{
    setWindowTitle("五子棋 · Gomoku");
    resize(880, 660);

    QWidget *central = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(14, 14, 14, 14);
    mainLayout->setSpacing(14);

    // ---------- 左侧控制面板 ----------
    QFrame *panel = new QFrame;
    panel->setObjectName("panel");
    panel->setFixedWidth(220);
    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(14, 14, 14, 14);
    panelLayout->setSpacing(10);

    QLabel *title = new QLabel("五 子 棋");
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName("title");
    panelLayout->addWidget(title);

    // 对战模式
    QLabel *modeLbl = new QLabel("对战模式");
    modeLbl->setObjectName("sectionLabel");
    panelLayout->addWidget(modeLbl);

    QButtonGroup *modeGroup = new QButtonGroup(this);
    btnPvP = new QPushButton("双人对战");
    btnPvE = new QPushButton("人机对战");
    btnOnline = new QPushButton("联机对战");
    btnPvP->setCheckable(true);
    btnPvE->setCheckable(true);
    btnOnline->setCheckable(true);
    btnPvP->setChecked(true);
    modeGroup->addButton(btnPvP);
    modeGroup->addButton(btnPvE);
    modeGroup->addButton(btnOnline);
    panelLayout->addWidget(btnPvP);
    panelLayout->addWidget(btnPvE);
    panelLayout->addWidget(btnOnline);

    // AI 难度
    QLabel *diffLbl = new QLabel("AI 难度");
    diffLbl->setObjectName("sectionLabel");
    panelLayout->addWidget(diffLbl);

    QButtonGroup *diffGroup = new QButtonGroup(this);
    btnEasy   = new QPushButton("简单");
    btnMedium = new QPushButton("普通");
    btnHard   = new QPushButton("困难");
    for (auto b : {btnEasy, btnMedium, btnHard}) { b->setCheckable(true); diffGroup->addButton(b); }
    btnMedium->setChecked(true);
    panelLayout->addWidget(btnEasy);
    panelLayout->addWidget(btnMedium);
    panelLayout->addWidget(btnHard);
    setDifficultyEnabled(false);          // 默认双人,难度先禁用

    panelLayout->addStretch();            // 弹性间隔

    // 操作按钮
    btnRestart = new QPushButton("重新开始");
    btnUndo    = new QPushButton("悔  棋");
    panelLayout->addWidget(btnRestart);
    panelLayout->addWidget(btnUndo);

    // 状态显示
    statusLabel = new QLabel("黑棋回合");
    statusLabel->setObjectName("status");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setMinimumHeight(42);
    panelLayout->addWidget(statusLabel);

    // ---------- 右侧棋盘 ----------
    board = new BoardWidget;

    mainLayout->addWidget(panel);
    mainLayout->addWidget(board, 1);     // 棋盘占满剩余空间

    setCentralWidget(central);
    applyStyle();

    // ---------- 信号槽连接 ----------
    connect(btnPvP, &QPushButton::clicked, this, &MainWindow::onModePvP);
    connect(btnPvE, &QPushButton::clicked, this, &MainWindow::onModePvE);
    connect(btnOnline, &QPushButton::clicked, this, &MainWindow::onModeOnline);
    // 难度按钮:点选即更新当前难度并下发给棋盘(无需重开)
    connect(btnEasy,   &QPushButton::clicked, this, [this]{ currentDiff = Difficulty::Easy;   board->setDifficulty(currentDiff); });
    connect(btnMedium, &QPushButton::clicked, this, [this]{ currentDiff = Difficulty::Medium; board->setDifficulty(currentDiff); });
    connect(btnHard,   &QPushButton::clicked, this, [this]{ currentDiff = Difficulty::Hard;   board->setDifficulty(currentDiff); });
    connect(btnRestart, &QPushButton::clicked, this, &MainWindow::onRestart);
    connect(btnUndo,    &QPushButton::clicked, this, &MainWindow::onUndo);
    connect(board, &BoardWidget::statusChanged, this, &MainWindow::onStatusChanged);
    connect(board, &BoardWidget::gameOver, this, &MainWindow::onGameOver);
    connect(board, &BoardWidget::stonePlaced, this, &MainWindow::onLocalStonePlaced);

    // ---------- 网络管理器 ----------
    net = new NetManager(this);
    connect(net, &NetManager::connected, this, &MainWindow::onNetConnected);
    connect(net, &NetManager::connectionFailed, this, &MainWindow::onNetFailed);
    connect(net, &NetManager::moveReceived, this, &MainWindow::onNetMoveReceived);
    connect(net, &NetManager::resetReceived, this, &MainWindow::onNetResetReceived);
    connect(net, &NetManager::disconnected, this, &MainWindow::onNetDisconnected);

    // 开局
    board->setMode(BoardWidget::PvP);
    board->setDifficulty(currentDiff);
    board->restart();
}

// ------------------------------------------------------------
//  模式切换:切换后自动重开新局
// ------------------------------------------------------------
void MainWindow::onModePvP()
{
    setOnlineMode(false);
    board->setMode(BoardWidget::PvP);
    setDifficultyEnabled(false);
    board->restart();
}

void MainWindow::onModePvE()
{
    setOnlineMode(false);
    board->setMode(BoardWidget::PvE);
    board->setDifficulty(currentDiff);
    setDifficultyEnabled(true);
    board->restart();
}

void MainWindow::onRestart()
{
    // 联机对局中重开:同步通知对手一起重置
    if (onlineActive && net->isConnected()) net->sendReset();
    board->restart();
}

void MainWindow::onUndo() { board->undo(); }

void MainWindow::onStatusChanged(const QString &text)
{
    statusLabel->setText(text);
    // 胜负时状态条变色提示
    if (text.contains("获胜") || text.contains("平局"))
        statusLabel->setStyleSheet(
            "#status{font-size:16px;font-weight:bold;color:#FFFFFF;"
            "background:#E67E22;border-radius:10px;padding:8px;}");
    else
        statusLabel->setStyleSheet(
            "#status{font-size:16px;font-weight:bold;color:#2C3E50;"
            "background:#ECF0F1;border-radius:10px;padding:8px;}");
}

void MainWindow::onGameOver(int winner)
{
    QString msg;
    if (winner == 0)      msg = "平局!";
    else if (winner == 1) msg = "黑棋获胜!";
    else                  msg = "白棋获胜!";
    QMessageBox::information(this, "对局结束", msg);
}

// ------------------------------------------------------------
//  联机模式:弹出连接设置对话框(创建房间 / 加入房间)
// ------------------------------------------------------------
void MainWindow::onModeOnline()
{
    showConnectDialog();
}

// 获取本机所有 IPv4 地址(供主机告诉对方该连哪个 IP)
QStringList MainWindow::localIps()
{
    QStringList ips;
    for (const QHostAddress &a : QNetworkInterface::allAddresses())
        if (a.protocol() == QAbstractSocket::IPv4Protocol && a != QHostAddress::LocalHost)
            ips << a.toString();
    return ips;
}

void MainWindow::setOnlineMode(bool on)
{
    if (!on) {
        if (onlineActive) net->closeAll();
        onlineActive = false;
    }
    btnUndo->setEnabled(!on);          // 联机时禁用悔棋,避免双方不同步
}

// ------------------------------------------------------------
//  连接设置对话框:选择主机/客户端、填写 IP 与端口
// ------------------------------------------------------------
void MainWindow::showConnectDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle("联机对战设置");
    QVBoxLayout *lay = new QVBoxLayout(&dlg);

    QRadioButton *rbHost = new QRadioButton("创建房间(作为主机,执黑先手)");
    QRadioButton *rbJoin = new QRadioButton("加入房间(作为客户端,执白后手)");
    rbHost->setChecked(true);
    lay->addWidget(rbHost);
    lay->addWidget(rbJoin);

    // 显示本机 IP,方便主机告诉对方
    QStringList ips = localIps();
    QLabel *ipHint = new QLabel("本机 IP: " + (ips.isEmpty() ? QStringLiteral("未检测到") : ips.join(", ")));
    ipHint->setWordWrap(true);
    ipHint->setStyleSheet("color:#7F8C8D; font-size:13px;");
    lay->addWidget(ipHint);

    QLabel *ipLbl = new QLabel("对方主机 IP(加入房间时填写):");
    QLineEdit *ipEdit = new QLineEdit;
    ipEdit->setPlaceholderText("例如 192.168.1.10 或公网 IP");
    lay->addWidget(ipLbl);
    lay->addWidget(ipEdit);

    QLabel *portLbl = new QLabel("端口:");
    QLineEdit *portEdit = new QLineEdit("9876");
    lay->addWidget(portLbl);
    lay->addWidget(portEdit);

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    lay->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) {
        btnPvP->setChecked(true);       // 取消则回退到双人
        return;
    }

    quint16 port = portEdit->text().toUShort();
    if (port == 0) port = 9876;

    // 进入联机状态:禁用难度与悔棋
    setDifficultyEnabled(false);
    setOnlineMode(true);
    onlineActive = true;

    if (rbHost->isChecked()) {
        onlineLocalPlayer = 1;          // 主机执黑
        net->host(port);
        statusLabel->setText("等待对手连接…\n把本机 IP 告诉对方: " + ips.join(", "));
        statusLabel->setStyleSheet(
            "#status{font-size:14px;font-weight:bold;color:#FFFFFF;"
            "background:#27AE60;border-radius:10px;padding:8px;}");
    } else {
        onlineLocalPlayer = 2;          // 客户端执白
        QString ip = ipEdit->text().trimmed();
        if (ip.isEmpty()) {
            QMessageBox::warning(this, "提示", "请填写对方主机 IP");
            setOnlineMode(false);
            btnPvP->setChecked(true);
            return;
        }
        net->join(ip, port);
        statusLabel->setText("正在连接 " + ip + " …");
        statusLabel->setStyleSheet(
            "#status{font-size:16px;font-weight:bold;color:#FFFFFF;"
            "background:#27AE60;border-radius:10px;padding:8px;}");
    }
}

// ------------------------------------------------------------
//  网络回调
// ------------------------------------------------------------
void MainWindow::onNetConnected()
{
    // 链路建立:正式进入联机对局,主机黑棋先手
    board->setMode(BoardWidget::Online);
    board->setLocalPlayer(onlineLocalPlayer);
    board->restart();                   // 清空棋盘,黑棋(currentPlayer=1)先行
}

void MainWindow::onNetFailed(const QString &reason)
{
    QMessageBox::warning(this, "联机失败", reason);
    setOnlineMode(false);
    btnPvP->setChecked(true);
    board->setMode(BoardWidget::PvP);
    board->restart();
}

void MainWindow::onNetMoveReceived(int row, int col)
{
    board->applyRemoteMove(row, col);
}

void MainWindow::onNetResetReceived()
{
    // 对手发起重开,本地直接重置(不再回传,避免循环)
    board->restart();
}

void MainWindow::onNetDisconnected()
{
    if (!onlineActive) return;
    onlineActive = false;
    QMessageBox::information(this, "联机", "对手已断开连接,已切换为双人模式");
    btnPvP->setChecked(true);
    board->setMode(BoardWidget::PvP);
    btnUndo->setEnabled(true);
    board->restart();
}

void MainWindow::onLocalStonePlaced(int row, int col)
{
    if (onlineActive) net->sendMove(row, col);
}

void MainWindow::setDifficultyEnabled(bool on)
{
    btnEasy->setEnabled(on);
    btnMedium->setEnabled(on);
    btnHard->setEnabled(on);
}

// ------------------------------------------------------------
//  统一 QSS 样式:圆润按钮、卡片面板、15px 字号
// ------------------------------------------------------------
void MainWindow::applyStyle()
{
    setStyleSheet(R"(
        QMainWindow { background: #F4F6FB; }
        QFrame#panel {
            background: #FFFFFF;
            border-radius: 14px;
        }
        QLabel#title {
            font-size: 24px;
            font-weight: bold;
            color: #2C3E50;
            padding: 6px;
            letter-spacing: 4px;
        }
        QLabel#sectionLabel {
            font-size: 13px;
            color: #95A5A6;
            padding-top: 8px;
        }
        QLabel#status {
            font-size: 16px;
            font-weight: bold;
            color: #2C3E50;
            background: #ECF0F1;
            border-radius: 10px;
            padding: 8px;
        }
        QPushButton {
            background-color: #5B8DEF;
            color: white;
            border: none;
            border-radius: 10px;
            padding: 11px;
            font-size: 15px;
        }
        QPushButton:hover    { background-color: #4A7BD8; }
        QPushButton:pressed  { background-color: #3A66B8; }
        QPushButton:checked  { background-color: #2E5BAA; }
        QPushButton:disabled { background-color: #C7CDD6; color: #ECF0F1; }
    )");
}
