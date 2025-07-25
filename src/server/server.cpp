#include "server.h"
#include "engine.h"
#include "nativesocket.h"
#include "protocol.h"
#include "room.h"
#include "settings.h"

#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

using namespace QSanProtocol;

Server::Server(QObject *parent)
    : QObject(parent)
{
    server = new NativeServerSocket;
    server->setParent(this);

    //synchronize ServerInfo on the server side to avoid ambiguous usage of Config and ServerInfo
    ServerInfo.parse(Sanguosha->getSetupString());

    current = nullptr;
    createNewRoom();

    connect(server, SIGNAL(new_connection(ClientSocket *)), this, SLOT(processNewConnection(ClientSocket *)));
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(deleteLater()));
}

void Server::broadcast(const QString &msg)
{
    QString to_sent = msg.toUtf8().toBase64();
    JsonArray arg;
    arg << "." << to_sent;

    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_SPEAK);
    packet.setMessageBody(arg);
    foreach (Room *room, rooms)
        room->broadcastInvoke(&packet);
}

bool Server::listen()
{
    return server->listen();
}

void Server::daemonize()
{
    server->daemonize();
}

Room *Server::createNewRoom()
{
    Room *new_room = new Room(this, Config.GameMode);
    if (new_room->getLuaState() == nullptr) {
        delete new_room;
        return nullptr;
    }
    current = new_room;
    rooms.insert(current);

    connect(current, SIGNAL(room_message(QString)), this, SIGNAL(server_message(QString)));
    connect(current, SIGNAL(game_over(QString)), this, SLOT(gameOver()));

    return current;
}

void Server::processNewConnection(ClientSocket *socket)
{
    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_CHECK_VERSION);
    packet.setMessageBody((Sanguosha->getVersion()));
    socket->send((packet.toString()));
    emit server_message(tr("%1 connected").arg(socket->peerName()));

    connect(socket, SIGNAL(message_got(const char *)), this, SLOT(processRequest(const char *)));
}

void Server::processRequest(const char *request)
{
    ClientSocket *socket = qobject_cast<ClientSocket *>(sender());
    socket->disconnect(this, SLOT(processRequest(const char *)));

    Packet signup;
    if (!signup.parse(request) || signup.getCommandType() != S_COMMAND_SIGNUP) {
        emit server_message(tr("Invalid signup string: %1").arg(request));
        QSanProtocol::Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_WARN);
        packet.setMessageBody("INVALID_FORMAT");
        socket->send(packet.toString());
        socket->disconnectFromHost();
        return;
    }

    const JsonArray &body = signup.getMessageBody().value<JsonArray>();
    QString urlPath = body[0].toString();

    enum
    {
        DefaultConnect,
        Observe,
        Reconnect,
    } connectionType
        = DefaultConnect;

    QStringList ps = urlPath.split('/', Qt::SkipEmptyParts);
    QString messageBodyToSend;
    if (ps.length() == 0) {
        // default connected
    } else {
        if (ps.length() == 2) {
            // check valid ps.first
            if (ps.first() == "reconnect") {
                connectionType = Reconnect;

                // check valid ps.last
                if (!ps.last().startsWith("sgs")) {
                    emit server_message(tr("reconnect username incorrect: %1").arg(ps.last()));
                    messageBodyToSend = "USERNAME_INCORRECT";
                } else {
                    QString num = ps.last().mid(3);
                    bool ok = false;
                    num.toInt(&ok);
                    if (ok) {
                        // valid connection name
                    } else {
                        emit server_message(tr("reconnect username incorrect: %1").arg(ps.last()));
                        messageBodyToSend = "USERNAME_INCORRECT";
                    }
                }
            } else if (ps.first() == "observe") {
                // warning, not implemented
                emit server_message(tr("unimplemented operation: %1").arg(ps.first()));
                messageBodyToSend = "OPERATION_NOT_IMPLEMENTED";
            } else if (ps.first() == "getwinners") {
                QString tableName = ps.last();
                getWinnersTableFile(socket, tableName);
                return;
            } else {
                emit server_message(tr("invalid operation: %1").arg(ps.first()));
                messageBodyToSend = "INVALID_OPERATION";
            }
        } else if (ps.length() == 1) {
            if (ps.first() == "getlack") {
                getLack(socket);
                return;
            } else {
                emit server_message(tr("invalid operation: %1").arg(ps.first()));
                messageBodyToSend = "INVALID_OPERATION";
            }
        } else {
            emit server_message(tr("invalid operation: more than 2 parts"));
            messageBodyToSend = "INVALID_OPERATION";
        }

        if (!messageBodyToSend.isEmpty()) {
            QSanProtocol::Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_WARN);
            packet.setMessageBody(messageBodyToSend);
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
            connect(socket, SIGNAL(disconnected()), this, SLOT(cleanupSimc()));
        }
    }

    Packet packet2(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_SETUP);
    QString s = Sanguosha->getSetupString();
    packet2.setMessageBody(s);
    socket->send((packet2.toString()));

    if (connectionType == Reconnect) {
        ServerPlayer *player = players.value(ps.last());
        if ((player != nullptr) && player->getState() == "offline" && !player->getRoom()->isFinished()) {
            player->getRoom()->reconnect(player, socket);
            return;
        }

        // player not found
        emit server_message(tr("reconnect username not found: %1").arg(ps.last()));
        QSanProtocol::Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_WARN);
        packet.setMessageBody("USERNAME_INCORRECT");
        socket->send(packet.toString());
        socket->disconnectFromHost();

        return;
    }

    if (current == nullptr || current->isFull() || current->isFinished())
        createNewRoom();

    QString screen_name = QString::fromUtf8(QByteArray::fromBase64(body[1].toString().toLatin1()));
    QString avatar = body[2].toString();

    ServerPlayer *player = current->addSocket(socket);
    current->signup(player, screen_name, avatar, false);
}

void Server::cleanupSimc()
{
    if (Config.ForbidSIMC) {
        const ClientSocket *socket = qobject_cast<const ClientSocket *>(sender());
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

void Server::gameOver()
{
    Room *room = qobject_cast<Room *>(sender());
    rooms.remove(room);

    foreach (ServerPlayer *player, room->findChildren<ServerPlayer *>()) {
        name2objname.remove(player->screenName(), player->objectName());
        players.remove(player->objectName());
    }
}
