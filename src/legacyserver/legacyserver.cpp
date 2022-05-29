#include "legacyserver.h"

#include "engine.h"
#include "general.h"
#include "json.h"
#include "legacyroom.h"
#include "mode.h"
#include "nativesocket.h"
#include "package.h"
#include "protocol.h"
#include "settings.h"

#include <QHostInfo>
#include <QJsonArray>

using namespace QSanProtocol;

LegacyServer::LegacyServer(QObject *parent)
    : QObject(parent)
{
    server = new NativeServerSocket;
    server->setParent(this);

    // Synchonize ServerInfo and Config
    ServerInfo.Name = Config.ServerName;
    ServerInfo.GameModeStr = Config.GameMode;
    ServerInfo.GameMode = Mode::findMode(Config.GameMode);
    if (Config.GameMode == QStringLiteral("02_1v1"))
        ServerInfo.GameRuleMode = Config.value(QStringLiteral("1v1/Rule"), QStringLiteral("2013")).toString();
    else if (Config.GameMode == QStringLiteral("06_3v3"))
        ServerInfo.GameRuleMode = Config.value(QStringLiteral("3v3/OfficialRule"), QStringLiteral("2013")).toString();
    ServerInfo.OperationTimeout = Config.OperationNoLimit ? 0 : Config.OperationTimeout;
    ServerInfo.NullificationCountDown = Config.NullificationCountDown;

    ServerInfo.EnabledPackages = getNeededPackages();
    ServerInfo.RandomSeat = Config.RandomSeat;
    ServerInfo.EnableCheat = Config.EnableCheat;
    ServerInfo.FreeChoose = Config.FreeChoose;
    ServerInfo.GeneralsPerPlayer = (Config.Enable2ndGeneral ? 2 : 1);
    ServerInfo.EnableSame = Config.EnableSame;
    ServerInfo.DisableChat = Config.DisableChat;
    ServerInfo.MaxHpScheme = Config.MaxHpScheme;
    ServerInfo.Scheme0Subtraction = Config.Scheme0Subtraction;

    current = nullptr;
    createNewRoom();

    connect(server, &ServerSocket::new_connection, this, &LegacyServer::processNewConnection);
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &QObject::deleteLater);
}

void LegacyServer::broadcast(const QString &msg)
{
    QString to_sent = QString::fromUtf8(msg.toUtf8().toBase64());
    JsonArray arg;
    arg << QStringLiteral(".") << to_sent;

    Packet packet(PacketDescriptionFlag(S_SRC_ROOM) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_SPEAK);
    packet.setMessageBody(arg);
    foreach (LegacyRoom *room, rooms)
        room->broadcastInvoke(&packet);
}

bool LegacyServer::listen()
{
    return server->listen();
}

void LegacyServer::daemonize()
{
    server->daemonize();
}

LegacyRoom *LegacyServer::createNewRoom()
{
    LegacyRoom *new_room = new LegacyRoom(this, Config.GameMode);
    if (new_room->getLuaState() == nullptr) {
        delete new_room;
        return nullptr;
    }
    current = new_room;
    rooms.insert(current);

    connect(current, &LegacyRoom::room_message, this, &LegacyServer::server_message);
    connect(current, &LegacyRoom::game_over, this, &LegacyServer::gameOver);

    return current;
}

void LegacyServer::processNewConnection(ClientSocket *socket)
{
    Packet packet(PacketDescriptionFlag(S_SRC_ROOM) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_CHECK_VERSION);
    packet.setMessageBody(QJsonValue(Sanguosha->version()));
    socket->send((packet.toString()));
    emit server_message(tr("%1 connected").arg(socket->peerName()));

    connect(socket, &ClientSocket::message_got, this, &LegacyServer::processRequest);
}

void LegacyServer::processRequest(const char *request)
{
    ClientSocket *socket = qobject_cast<ClientSocket *>(sender());
    socket->disconnect(this, SLOT(processRequest(const char *)));

    Packet signup;
    if (!signup.parse(request) || signup.getCommandType() != S_COMMAND_SIGNUP) {
        emit server_message(tr("Invalid signup string: %1").arg(QString::fromUtf8(request)));
        QSanProtocol::Packet packet(PacketDescriptionFlag(S_SRC_ROOM) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_WARN);
        packet.setMessageBody(QJsonValue(QStringLiteral("INVALID_FORMAT")));
        socket->send(packet.toString());
        socket->disconnectFromHost();
        return;
    }

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
            connect(socket, &ClientSocket::disconnected, this, &LegacyServer::cleanupSimc);
        }
    }

    Packet packet2(PacketDescriptionFlag(S_SRC_ROOM) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_SETUP);
    QVariant s = ServerInfo.serialize();
    packet2.setMessageBody(s);
    socket->send((packet2.toString()));

    if (reconnection_enabled) {
        LegacyServerPlayer *player = players.value(ps.last());
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

    LegacyServerPlayer *player = current->addSocket(socket);
    current->signup(player, screen_name, avatar, false);
}

void LegacyServer::cleanupSimc()
{
    if (Config.ForbidSIMC) {
        const ClientSocket *socket = qobject_cast<const ClientSocket *>(sender());
        addresses.remove(socket->peerAddress());
    }
}

void LegacyServer::signupPlayer(LegacyServerPlayer *player)
{
    name2objname.insert(player->screenName(), player->objectName());
    players.insert(player->objectName(), player);
}

void LegacyServer::gameOver()
{
    LegacyRoom *room = qobject_cast<LegacyRoom *>(sender());
    rooms.remove(room);

    foreach (LegacyServerPlayer *player, room->findChildren<LegacyServerPlayer *>()) {
        name2objname.remove(player->screenName(), player->objectName());
        players.remove(player->objectName());
    }
}

QStringList LegacyServer::getNeededPackages() const
{
    QStringList ret;
    if (ServerInfo.GameMode->category() == QSanguosha::ModeHegemony) {
        ret << QStringLiteral("hegemonyGeneral") << QStringLiteral("hegemony_card");
    } else {
        QStringList ban = Config.BanPackages;
        QStringList packages = Sanguosha->packageNames();
        foreach (const QString &package, packages) {
            if (package == QStringLiteral("hegemonyGeneral"))
                continue;
            if (package == QStringLiteral("hegemony_card"))
                continue;
            if (ban.contains(package))
                continue;
            ret << package;
        }
    }
    return ret;
}
