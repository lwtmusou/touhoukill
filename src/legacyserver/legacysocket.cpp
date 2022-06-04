#include "legacysocket.h"
#include "settings.h"

#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QUrl>

LegacyServerSocket::LegacyServerSocket()
{
    server = new QTcpServer(this);
    daemon = nullptr;
    connect(server, &QTcpServer::newConnection, this, &LegacyServerSocket::processNewConnection);
}

bool LegacyServerSocket::listen()
{
    return server->listen(QHostAddress::Any, Config.ServerPort);
}

void LegacyServerSocket::daemonize()
{
    delete daemon;
    daemon = new QUdpSocket(this);
    daemon->bind(Config.ServerPort, QUdpSocket::ShareAddress);
    connect(daemon, &QIODevice::readyRead, this, &LegacyServerSocket::processNewDatagram);
}

void LegacyServerSocket::processNewDatagram()
{
    while (daemon->hasPendingDatagrams()) {
        QHostAddress from;
        char ask_str[256];

        daemon->readDatagram(ask_str, sizeof(ask_str), &from);

        QByteArray data = Config.ServerName.toUtf8();
        daemon->writeDatagram(data, from, Config.DetectorPort);
        daemon->flush();
    }
}

void LegacyServerSocket::processNewConnection()
{
    QTcpSocket *socket = server->nextPendingConnection();
    LegacyClientSocket *connection = new LegacyClientSocket(socket);
    emit new_connection(connection);
}

// ---------------------------------

LegacyClientSocket::LegacyClientSocket()
    : socket(new QTcpSocket(this))
{
    init();
}

LegacyClientSocket::LegacyClientSocket(QTcpSocket *socket)
    : socket(socket)
{
    socket->setParent(this);
    init();
}

void LegacyClientSocket::init()
{
    connect(socket, &QAbstractSocket::disconnected, this, &LegacyClientSocket::disconnected);
    connect(socket, &QIODevice::readyRead, this, &LegacyClientSocket::getMessage);
    connect(socket, &QAbstractSocket::errorOccurred, this, &LegacyClientSocket::raiseError);
    connect(socket, &QAbstractSocket::connected, this, &LegacyClientSocket::connected);
}

void LegacyClientSocket::connectToHost()
{
    QString address = QStringLiteral("127.0.0.1");
    ushort port = 9527U;

    QUrl hostUrl(Config.HostAddress);
    if (hostUrl.scheme() == QStringLiteral("qths")) {
        address = hostUrl.host();
        if (address != QStringLiteral("127.0.0.1"))
            port = hostUrl.port(9527);
        else
            port = Config.value(QStringLiteral("ServerPort"), QStringLiteral("9527")).toString().toUShort();
    }

    socket->connectToHost(address, port);
}

void LegacyClientSocket::getMessage()
{
    QList<QByteArray> bufferList;
    while (socket->canReadLine()) {
        buffer_t msg;
        socket->readLine(msg, sizeof(msg));
        bufferList << QByteArray(msg);
    }
    foreach (const QByteArray &arr, bufferList)
        emit message_got(arr.constData());
}

void LegacyClientSocket::disconnectFromHost()
{
    socket->disconnectFromHost();
}

void LegacyClientSocket::send(const QString &message)
{
    socket->write(message.toLatin1());
    if (!message.endsWith(QStringLiteral("\n")))
        socket->write("\n");
#ifndef QT_NO_DEBUG
    printf("TX: %s\n", message.toLatin1().constData());
#endif
    socket->flush();
}

bool LegacyClientSocket::isConnected() const
{
    return socket->state() == QTcpSocket::ConnectedState;
}

QString LegacyClientSocket::peerName() const
{
    QString peer_name = socket->peerName();
    if (peer_name.isEmpty())
        peer_name = QStringLiteral("%1:%2").arg(socket->peerAddress().toString(), socket->peerPort());

    return peer_name;
}

QString LegacyClientSocket::peerAddress() const
{
    return socket->peerAddress().toString();
}

quint32 LegacyClientSocket::ipv4Address() const
{
    return socket->peerAddress().toIPv4Address();
}

void LegacyClientSocket::raiseError(QAbstractSocket::SocketError socket_error)
{
    // translate error message
    QString reason;
    switch (socket_error) {
    case QAbstractSocket::ConnectionRefusedError:
        reason = tr("Connection was refused or timeout");
        break;
    case QAbstractSocket::RemoteHostClosedError:
        reason = tr("Remote host close this connection");
        break;
    case QAbstractSocket::HostNotFoundError:
        reason = tr("Host not found");
        break;
    case QAbstractSocket::SocketAccessError:
        reason = tr("Socket access error");
        break;
    case QAbstractSocket::NetworkError:
        return; // this error is ignored ...
    default:
        reason = tr("Unknow error");
        break;
    }

    emit error_message(tr("Connection failed, error code = %1\n reason:\n %2").arg(QString::number(static_cast<int>(socket_error)), reason));
}
