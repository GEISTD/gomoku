#include "netmanager.h"
#include <QTcpServer>
#include <QTcpSocket>

NetManager::NetManager(QObject *parent)
    : QObject(parent), m_server(nullptr), m_socket(nullptr), m_role(None) {}

bool NetManager::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

// ------------------------------------------------------------
//  主机:监听任意网卡的指定端口,等待客户端连入
// ------------------------------------------------------------
void NetManager::host(quint16 port)
{
    closeAll();
    m_role = Host;
    if (!m_server) {
        m_server = new QTcpServer(this);
        connect(m_server, &QTcpServer::newConnection, this, &NetManager::onNewConnection);
    }
    if (!m_server->listen(QHostAddress::Any, port))
        emit connectionFailed(QStringLiteral("监听失败: ") + m_server->errorString());
}

// ------------------------------------------------------------
//  客户端:主动连接主机
// ------------------------------------------------------------
void NetManager::join(const QString &ip, quint16 port)
{
    closeAll();
    m_role = Client;
    if (!m_socket) {
        m_socket = new QTcpSocket(this);
        attachSocket(m_socket);
    }
    m_socket->connectToHost(ip, port);
    // 连接结果由 connected / error 信号通知
}

// 统一挂接 socket 的信号
void NetManager::attachSocket(QTcpSocket *s)
{
    connect(s, &QTcpSocket::connected, this, &NetManager::connected);
    connect(s, &QTcpSocket::readyRead, this, &NetManager::onReadyRead);
    connect(s, &QTcpSocket::disconnected, this, &NetManager::onSocketDisconnected);
    // error 是重载信号,用 SIGNAL/SLOT 宏避免歧义
    connect(s, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onSocketError()));
}

// ------------------------------------------------------------
//  主机侧:收到一个客户端连接,只接受第一个
// ------------------------------------------------------------
void NetManager::onNewConnection()
{
    if (m_socket) {                       // 已有对手,拒绝后续连接
        m_server->nextPendingConnection()->deleteLater();
        return;
    }
    m_socket = m_server->nextPendingConnection();
    attachSocket(m_socket);
    emit connected();                     // 主机侧链路就绪
}

// ------------------------------------------------------------
//  读数据:累积到缓冲,按 '\n' 拆成完整行逐行解析
// ------------------------------------------------------------
void NetManager::onReadyRead()
{
    if (!m_socket) return;
    m_buffer.append(m_socket->readAll());
    int idx;
    while ((idx = m_buffer.indexOf('\n')) >= 0) {
        QByteArray line = m_buffer.left(idx);
        m_buffer = m_buffer.mid(idx + 1);
        processLine(QString::fromUtf8(line).trimmed());
    }
}

void NetManager::processLine(const QString &line)
{
    if (line.isEmpty()) return;
    QStringList parts = line.split(' ', QString::SkipEmptyParts);
    if (parts.isEmpty()) return;
    if (parts[0] == "MOVE" && parts.size() == 3) {
        bool ok1 = false, ok2 = false;
        int r = parts[1].toInt(&ok1);
        int c = parts[2].toInt(&ok2);
        if (ok1 && ok2) emit moveReceived(r, c);
    } else if (parts[0] == "RESET") {
        emit resetReceived();
    }
}

void NetManager::onSocketDisconnected()
{
    emit disconnected();
    if (m_socket) {
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

void NetManager::onSocketError()
{
    // 仅客户端连接阶段才报"连接失败",避免主机侧误报
    if (m_role == Client && !isConnected())
        emit connectionFailed(QStringLiteral("连接失败: ") +
                              (m_socket ? m_socket->errorString() : QStringLiteral("未知错误")));
}

void NetManager::sendMove(int row, int col)
{
    if (!isConnected()) return;
    m_socket->write(QString("MOVE %1 %2\n").arg(row).arg(col).toUtf8());
}

void NetManager::sendReset()
{
    if (!isConnected()) return;
    m_socket->write("RESET\n");
}

// 关闭连接,但保留 server 对象以便复用
void NetManager::closeAll()
{
    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    if (m_server) m_server->close();
    m_buffer.clear();
}
