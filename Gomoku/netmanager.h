#ifndef NETMANAGER_H
#define NETMANAGER_H

#include <QObject>
#include <QString>

class QTcpServer;
class QTcpSocket;

// ============================================================
//  NetManager —— 联机对战网络层
//  两种角色:
//    Host(主机):用 QTcpServer 监听端口,等待一个客户端连入,执黑先手
//    Client(客户端):用 QTcpSocket 主动连接主机 IP:端口,执白
//  通信协议:行文本,以 '\n' 分隔
//    "MOVE r c\n"   落子
//    "RESET\n"      重新开始
//  网络层只负责收发消息,不关心游戏规则;规则由 BoardWidget 维护。
// ============================================================
class NetManager : public QObject
{
    Q_OBJECT
public:
    enum Role { None, Host, Client };

    explicit NetManager(QObject *parent = nullptr);

    void host(quint16 port);                     // 主机:开始监听
    void join(const QString &ip, quint16 port);  // 客户端:连接主机
    void sendMove(int row, int col);             // 发送落子
    void sendReset();                            // 发送重开
    void closeAll();                             // 关闭所有连接
    bool isConnected() const;
    Role role() const { return m_role; }

signals:
    void connected();                             // 链路建立
    void connectionFailed(const QString &reason); // 连接/监听失败
    void moveReceived(int row, int col);          // 收到对手落子
    void resetReceived();                         // 收到对手重开
    void disconnected();                          // 连接断开

private slots:
    void onNewConnection();
    void onReadyRead();
    void onSocketDisconnected();
    void onSocketError();

private:
    QTcpServer *m_server;
    QTcpSocket *m_socket;
    QByteArray  m_buffer;     // 接收缓冲,按 '\n' 拆行
    Role        m_role;

    void attachSocket(QTcpSocket *s);            // 统一挂接 socket 信号
    void processLine(const QString &line);       // 解析一行协议
};

#endif // NETMANAGER_H
