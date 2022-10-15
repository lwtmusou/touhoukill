#include "server.h"
#include "RoomObject.h"
#include "engine.h"
#include "mode.h"
#include "qsgsmultiserver.h"
#include "serverconfig.h"

#include <QJsonObject>
#include <QJsonValue>

const QMap<QSanProtocol::CommandType, Server::SocketVerificationCallback> Server::VerificationCallbacks {
    std::make_pair(QSanProtocol::S_COMMAND_SIGNUP_LEGACY, &Server::socketSignupLegacy),
    std::make_pair(QSanProtocol::S_COMMAND_CHECK_VERSION, &Server::socketSignup),
};
const QMap<QSanProtocol::CommandType, Server::SocketCallback> Server::Callbacks {
    // TODO
};

Server::Server(QObject *parent)
    : QObject(parent)
    , server(new QSgsMultiServer(this))
{
    connect(server, &QSgsMultiServer::newConnection, this, &Server::socketConnected);
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &Server::deleteLater);
}

void Server::broadcast(const QString &msg)
{
#if 0
    QString to_sent = QString::fromUtf8(msg.toUtf8().toBase64());
    QJsonArray arg;
    arg << QStringLiteral(".") << to_sent;

    Packet packet(PacketDescriptionFlag(S_SRC_ROOM) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_SPEAK);
    packet.setMessageBody(arg);
    foreach (LegacyRoom *room, rooms)
        room->broadcastInvoke(&packet);
#endif
}

bool Server::listen()
{
    bool ret = true;

    ret = server->listenLocal(ServerConfig.game.serverName) && ret;
    ret = server->listenTcp(ServerConfig.tcpServer.bindIp, ServerConfig.tcpServer.bindPort) && ret;

    return ret;
}

bool Server::socketSignup(QSgsMultiSocket *socket, const QSanProtocol::Packet &packet)
{
    using namespace QSanProtocol;

    // TODO check the version of every package. Disconnect if version do not match

    // mode, lack

    QJsonObject ob;

    for (auto it = currentRooms.constBegin(); it != currentRooms.constEnd(); ++it) {
        const Mode *mode = Mode::findMode(it.key());
        int lack = mode->playersCount() - it.value()->players().length();
        if (lack == 0)
            lack = mode->playersCount();

        ob[it.key()] = lack;
    }

    Packet roomPacket(PacketDescriptionFlag(S_SRC_LOBBY) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_ROOMS);
    roomPacket.setMessageBody(ob);
    socket->writeLine(roomPacket.toJson());

    return true;
}

bool Server::socketSignupLegacy(QSgsMultiSocket *socket, const QSanProtocol::Packet &signup)
{
    // NOTE: For serving legacy clients, ensure all of our notification or request should be using S_SRC_ROOM instead of S_SRC_LOBBY

    using namespace QSanProtocol;

    const QJsonArray &body = signup.messageBody().toArray();
    QString urlPath = body[0].toString();
    QString screen_name = QString::fromUtf8(QByteArray::fromBase64(body[1].toString().toLatin1()));
    QString avatar = body[2].toString();
    bool reconnection_enabled = false;

    QStringList ps = urlPath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QString messageBodyToSend;
    if (ps.length() == 0) {
        // default connected
    } else {
        if (ps.length() != 2) {
            messageBodyToSend = QStringLiteral("INVALID_OPERATION");
            emit serverMessage(tr("invalid operation: more than 2 parts"));
        } else {
            // check valid ps.first
            if (ps.first() == QStringLiteral("reconnect")) {
                reconnection_enabled = true;
            } else if (ps.first() == QStringLiteral("observe")) {
                // warning, not implemented
                emit serverMessage(tr("unimplemented operation: %1").arg(ps.first()));
                messageBodyToSend = QStringLiteral("OPERATION_NOT_IMPLEMENTED");
            } else {
                emit serverMessage(tr("invalid operation: %1").arg(ps.first()));
                messageBodyToSend = QStringLiteral("INVALID_OPERATION");
            }
        }
        if (messageBodyToSend.isEmpty()) {
            // check valid ps.last
            if (!ps.last().startsWith(QStringLiteral("sgs"))) {
                emit serverMessage(tr("reconnect username incorrect: %1").arg(ps.last()));
                messageBodyToSend = QStringLiteral("USERNAME_INCORRECT");
            } else {
                QString num = ps.last().mid(3);
                bool ok = false;
                num.toInt(&ok);
                if (ok) {
                    // valid connection name
                } else {
                    emit serverMessage(tr("reconnect username incorrect: %1").arg(ps.last()));
                    messageBodyToSend = QStringLiteral("USERNAME_INCORRECT");
                }
            }
        }

        if (!messageBodyToSend.isEmpty()) {
            Packet packet(PacketDescriptionFlag(S_SRC_ROOM) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_WARN);
            packet.setMessageBody(messageBodyToSend);
            socket->writeLine(packet.toJson());
            socket->disconnectFromHost();
            return false;
        }
    }
#if 0
    if (Config.ForbidSIMC) {
        QString addr = socket->peerAddress();
        if (addresses.contains(addr)) {
            socket->disconnectFromHost();
            emit server_message(tr("Forbid the connection of address %1").arg(addr));
            return false;
        } else {
            addresses.insert(addr);
            connect(socket, &LegacyClientSocket::disconnected, this, &Server::cleanupSimc);
        }
#endif

    Packet packet2(PacketDescriptionFlag(S_SRC_ROOM) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_SETUP);
    QJsonValue s = ServerConfig.toServerInfo(ServerConfig.modesServing.first()).serialize();
    packet2.setMessageBody(s);
    socket->writeLine(packet2.toJson());

    if (reconnection_enabled) {
#if 0
        ServerPlayer *player = players.value(ps.last());
        if ((player != nullptr) && player->getState() == QStringLiteral("offline") && player->reconnect(socket))
            return;


        // player not found
        emit server_message(tr("reconnect username not found: %1").arg(ps.last()));
        QSanProtocol::Packet packet(PacketDescriptionFlag(S_SRC_ROOM) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_WARN);
        packet.setMessageBody(QJsonValue(QStringLiteral("USERNAME_INCORRECT")));
        socket->send(packet.toString());
        socket->disconnectFromHost();

        return false;
#endif
    }
#if 0
    if (current == nullptr || current->isFull() || current->isFinished())
        createNewRoom();

    ServerPlayer *player = current->addSocket(socket);
    current->signup(player, screen_name, avatar, false);
#endif
    return true;
}

#if 0
LegacyRoom *Server::createNewRoom()
{
    LegacyRoom *new_room = new LegacyRoom(this, &ServerInfo);
    if (new_room->getLuaState() == nullptr) {
        delete new_room;
        return nullptr;
    }
    current = new_room;
    rooms.insert(current);

    connect(current, &LegacyRoom::room_message, this, &Server::server_message);
    connect(current, &LegacyRoom::game_over, this, &Server::gameOver);

    return current;
}
#endif

void Server::socketConnected(QSgsMultiSocket *socket)
{
    using namespace QSanProtocol;

    Packet packet(PacketDescriptionFlag(S_SRC_LOBBY) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_CHECK_VERSION);

    // TODO: refactor this when passing Extension version number from Lua ready
    packet.setMessageBody(Sanguosha->versionNumber().toString());
    socket->writeLine(packet.toJson());

    unverifiedSockets.insert(socket, 0);

    emit serverMessage(tr("%1 connected").arg(socket->peerName()));
    connect(socket, &QSgsMultiSocket::readyRead, this, &Server::socketReadyRead);
}

// void Server::processRequest(const QByteArray &request)
void Server::socketReadyRead()
{
    QSgsMultiSocket *socket = qobject_cast<QSgsMultiSocket *>(sender());
    if (socket == nullptr)
        return;

    using namespace QSanProtocol;

    while (socket->canReadLine()) {
        QByteArray arr = socket->readLine();

        Packet packet;
        if (!packet.parse(arr))
            continue;

        if (unverifiedSockets.contains(socket)) {
            if (VerificationCallbacks.contains(packet.commandType())) {
                bool verificationResult = (this->*(VerificationCallbacks[packet.commandType()]))(socket, packet);
                if (verificationResult)
                    unverifiedSockets.remove(socket);
            }

            if (unverifiedSockets.contains(socket)) {
                unverifiedSockets[socket]++;
                if (++(unverifiedSockets[socket]) > 3)
                    socket->disconnectFromHost();
            }
            return;
        }
    }

#if 0
    const QJsonArray &body = signup.getMessageBody().toArray();
    QString urlPath = body[0].toString();
    QString screen_name = QString::fromUtf8(QByteArray::fromBase64(body[1].toString().toLatin1()));
    QString avatar = body[2].toString();
    bool reconnection_enabled = false;

    QStringList ps = urlPath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    QString messageBodyToSend;
    if (ps.length() == 0) {
        // default connected
    } else {
        if (ps.length() != 2) {
            messageBodyToSend = QStringLiteral("INVALID_OPERATION");
            emit server_message(tr("invalid operation: more than 2 parts"));
        } else {
            // check valid ps.first
            if (ps.first() == QStringLiteral("reconnect")) {
                reconnection_enabled = true;
            } else if (ps.first() == QStringLiteral("observe")) {
                // warning, not implemented
                emit server_message(tr("unimplemented operation: %1").arg(ps.first()));
                messageBodyToSend = QStringLiteral("OPERATION_NOT_IMPLEMENTED");
            } else {
                emit server_message(tr("invalid operation: %1").arg(ps.first()));
                messageBodyToSend = QStringLiteral("INVALID_OPERATION");
            }
        }
        if (messageBodyToSend.isEmpty()) {
            // check valid ps.last
            if (!ps.last().startsWith(QStringLiteral("sgs"))) {
                emit server_message(tr("reconnect username incorrect: %1").arg(ps.last()));
                messageBodyToSend = QStringLiteral("USERNAME_INCORRECT");
            } else {
                QString num = ps.last().mid(3);
                bool ok = false;
                num.toInt(&ok);
                if (ok) {
                    // valid connection name
                } else {
                    emit server_message(tr("reconnect username incorrect: %1").arg(ps.last()));
                    messageBodyToSend = QStringLiteral("USERNAME_INCORRECT");
                }
            }
            } else if (ps.first() == "getwinners") {
                QString tableName = ps.last();
                getWinnersTableFile(socket, tableName);
                return;
                return;
        }

        if (!messageBodyToSend.isEmpty()) {
            QSanProtocol::Packet packet(PacketDescriptionFlag(S_SRC_ROOM) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_WARN);
            packet.setMessageBody(QJsonValue(messageBodyToSend));
            socket->send(packet.toString());
            socket->disconnectFromHost();
            return;
        }
    }
    if (Config.ForbidSIMC) {
        QString addr = socket->peerAddress();
        if (addresses.contains(addr)) {
            socket->disconnectFromHost();
            emit server_message(tr("Forbid the connection of address %1").arg(addr));
            return;
        } else {
            addresses.insert(addr);
            connect(socket, &LegacyClientSocket::disconnected, this, &Server::cleanupSimc);
        }
    }

    Packet packet2(PacketDescriptionFlag(S_SRC_ROOM) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_SETUP);
    QJsonValue s = ServerInfo.serialize();
    packet2.setMessageBody(s);
    socket->send((packet2.toString()));

    if (reconnection_enabled) {
        ServerPlayer *player = players.value(ps.last());
        if ((player != nullptr) && player->getState() == QStringLiteral("offline") && !player->getRoom()->isFinished()) {
            player->getRoom()->reconnect(player, socket);
            return;
        }

        // player not found
        emit server_message(tr("reconnect username not found: %1").arg(ps.last()));
        QSanProtocol::Packet packet(PacketDescriptionFlag(S_SRC_ROOM) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_WARN);
        packet.setMessageBody(QJsonValue(QStringLiteral("USERNAME_INCORRECT")));
        socket->send(packet.toString());
        socket->disconnectFromHost();

        return;
    }

    if (current == nullptr || current->isFull() || current->isFinished())
        createNewRoom();

    ServerPlayer *player = current->addSocket(socket);
    current->signup(player, screen_name, avatar, false);
#endif
}

void Server::socketDisconnected()
{
    QSgsMultiSocket *socket = qobject_cast<QSgsMultiSocket *>(sender());
    if (socket != nullptr)
        unverifiedSockets.remove(socket);

    sender()->deleteLater();
}

#if 0
void Server::cleanupSimc()
{
    if (Config.ForbidSIMC) {
        const LegacyClientSocket *socket = qobject_cast<const LegacyClientSocket *>(sender());
        addresses.remove(socket->peerAddress());
    }
}

void Server::signupPlayer(ServerPlayer *player)
{
    name2objname.insert(player->screenName(), player->objectName());
    players.insert(player->objectName(), player);
}

void Server::getLack(ClientSocket *socket)
{
    int playingRooms = 0;
    foreach (Room *room, rooms) {
        if (room->isFull())
            ++playingRooms;
    }

    int lack = -1;

    if (current == nullptr || current->isFull() || current->isFinished())
        lack = Sanguosha->getPlayerCount(ServerInfo.GameMode);
    else
        lack = current->getLack();

    QJsonObject ob;
    ob["playingRooms"] = playingRooms;
    ob["currentLack"] = lack;
    QJsonDocument doc(ob);

    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_HEARTBEAT);
    packet.setMessageBody(doc.toJson(QJsonDocument::Compact));
    socket->send(packet.toString());
    socket->disconnectFromHost();
}

void Server::getWinnersTableFile(ClientSocket *socket, const QString &tableName)
{
    QJsonObject ob;
    QFile f("etc/winner/" + tableName + ".txt");

    if (f.exists()) {
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray arr = f.readAll();
            f.close();
            ob["data"] = QString::fromLatin1(arr.toBase64());
        } else {
            ob["error"] = QStringLiteral("file open failed");
        }
    } else {
        ob["data"] = QString();
    }
    QJsonDocument doc(ob);

    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_HEARTBEAT);
    packet.setMessageBody(doc.toJson(QJsonDocument::Compact));
    socket->send(packet.toString());
    socket->disconnectFromHost();
}
#endif

void Server::gameOver()
{
    sender()->deleteLater();
}
