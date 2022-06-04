#ifndef _NATIVESOCKET_H
#define _NATIVESOCKET_H

#include "socket.h"

#include <QTcpSocket>

class QUdpSocket;
class QTcpServer;

class NativeServerSocket : public ServerSocket
{
    Q_OBJECT

public:
    NativeServerSocket();

    bool listen() override;
    void daemonize() override;

private slots:
    void processNewConnection();
    void processNewDatagram();

private:
    QTcpServer *server;
    QUdpSocket *daemon;
};

class NativeClientSocket : public ClientSocket
{
    Q_OBJECT

public:
    NativeClientSocket();
    explicit NativeClientSocket(QTcpSocket *socket);

    void connectToHost() override;
    void disconnectFromHost() override;
    void send(const QString &message) override;
    bool isConnected() const override;
    QString peerName() const override;
    QString peerAddress() const override;
    quint32 ipv4Address() const override;

private slots:
    void getMessage();
    void raiseError(QAbstractSocket::SocketError socket_error);

private:
    QTcpSocket *const socket;

    void init();
};

#endif
