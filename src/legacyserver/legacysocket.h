#ifndef qsgslegacy_NATIVESOCKET_H
#define qsgslegacy_NATIVESOCKET_H

#include <QObject>
#include <QTcpSocket>

class QUdpSocket;
class QTcpServer;

class LegacyClientSocket;

class LegacyServerSocket : public QObject
{
    Q_OBJECT

public:
    LegacyServerSocket();

    bool listen();
    void daemonize();

signals:
    void new_connection(LegacyClientSocket *connection);

private slots:
    void processNewConnection();
    void processNewDatagram();

private:
    QTcpServer *server;
    QUdpSocket *daemon;
};

class LegacyClientSocket : public QObject
{
    Q_OBJECT

public:
    LegacyClientSocket();
    explicit LegacyClientSocket(QTcpSocket *socket);

    void connectToHost();
    void disconnectFromHost();
    void send(const QString &message);
    bool isConnected() const;
    QString peerName() const;
    QString peerAddress() const;
    quint32 ipv4Address() const;

private slots:
    void getMessage();
    void raiseError(QAbstractSocket::SocketError socket_error);

signals:
    void message_got(const char *msg);
    void error_message(const QString &msg);
    void disconnected();
    void connected();

private:
    QTcpSocket *const socket;

    void init();
};

typedef char buffer_t[16000];

#endif
