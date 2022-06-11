#ifndef TOUHOUKILL_SERVER_H
#define TOUHOUKILL_SERVER_H

#include "protocol.h"
#include "serverinfostruct.h"

class QSgsMultiSocket;
class QSgsMultiServer;
class RoomObject;

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(QObject *parent = nullptr);

    void broadcast(const QString &msg);
    bool listen();

private:
    QSgsMultiServer *server;
    QMap<QSgsMultiSocket *, int> unverifiedSockets;
    QHash<QString /* mode */, QList<RoomObject *>> rooms;
    QHash<QString /* mode */, RoomObject *> currentRooms;

    typedef bool (Server::*SocketVerificationCallback)(QSgsMultiSocket *socket, const QSanProtocol::Packet &packet);
    typedef void (Server::*SocketCallback)(QSgsMultiSocket *socket, const QSanProtocol::Packet &packet);

    bool socketSignup(QSgsMultiSocket *socket, const QSanProtocol::Packet &packet);
    bool socketSignupLegacy(QSgsMultiSocket *socket, const QSanProtocol::Packet &packet);

    static const QMap<QSanProtocol::CommandType, SocketVerificationCallback> VerificationCallbacks;
    static const QMap<QSanProtocol::CommandType, SocketCallback> Callbacks;

private slots:
    void socketConnected(QSgsMultiSocket *socket);
    void socketReadyRead();
    void socketDisconnected();
    void gameOver();

signals:
    void server_message(const QString &);
};

#endif
