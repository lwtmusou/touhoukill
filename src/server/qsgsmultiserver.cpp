
#include "qsgsmultiserver.h"
#include "serverconfig.h"

#include <QBuffer>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMutex>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>

#include <iostream>

QSgsMultiSocket::QSgsMultiSocket(QObject *parent)
    : QObject(parent)
{
}

class QSgsTcpSocket : public QSgsMultiSocket
{
    Q_OBJECT

public:
    QSgsTcpSocket(const QHostAddress &host, quint16 port, QObject *parent = nullptr)
        : QSgsMultiSocket(parent)
    {
        socket = new QTcpSocket(this);

        connect(socket, &QTcpSocket::connected, this, &QSgsTcpSocket::socketConnected);
        connect(socket, &QTcpSocket::disconnected, this, &QSgsTcpSocket::socketDisconnected);
        connect(socket, &QTcpSocket::errorOccurred, this, &QSgsTcpSocket::errorOccurred);
        connect(socket, &QTcpSocket::readyRead, this, &QSgsTcpSocket::readyRead);
        socket->connectToHost(host, port, QIODevice::ReadWrite);
    }

    QSgsTcpSocket(QTcpSocket *socket, QObject *parent = nullptr)
        : QSgsMultiSocket(parent)
        , socket(socket)
    {
        connect(socket, &QTcpSocket::connected, this, &QSgsTcpSocket::socketConnected);
        connect(socket, &QTcpSocket::disconnected, this, &QSgsTcpSocket::socketDisconnected);
        connect(socket, &QTcpSocket::errorOccurred, this, &QSgsTcpSocket::errorOccurred);
        connect(socket, &QTcpSocket::readyRead, this, &QSgsTcpSocket::readyRead);
    }

    void disconnectFromHost() override
    {
        socket->disconnectFromHost();
    }

    bool isSocketConnected() const override
    {
        return socket->state() == QTcpSocket::ConnectedState;
    }

    QString peerAddress() const override
    {
        return socket->peerAddress().toString();
    }

    bool canReadLine() const override
    {
        return socket->canReadLine();
    }

    QByteArray readLine() override
    {
        return socket->readLine();
    }

    bool writeLine(const QByteArray &lineData) override
    {
        QByteArray removeNewline = lineData;
        removeNewline.replace('\r', ' ');
        removeNewline.replace('\n', ' ');
        removeNewline.append('\n');
        quint64 size = socket->write(removeNewline);
        socket->flush();
        return size == (quint64)removeNewline.length();
    }

    SocketType type() const override
    {
        return TcpSocket;
    }

private:
    QTcpSocket *socket;
};

class QSgsLocalSocket : public QSgsMultiSocket
{
    Q_OBJECT

public:
    QSgsLocalSocket(const QString &localName, QObject *parent = nullptr)
        : QSgsMultiSocket(parent)
    {
        socket = new QLocalSocket(this);

        connect(socket, &QLocalSocket::connected, this, &QSgsLocalSocket::socketConnected);
        connect(socket, &QLocalSocket::disconnected, this, &QSgsLocalSocket::socketDisconnected);
        connect(socket, &QLocalSocket::errorOccurred, this, &QSgsLocalSocket::localErrorOccurred);
        connect(socket, &QLocalSocket::readyRead, this, &QSgsLocalSocket::readyRead);
        socket->connectToServer(localName, QIODevice::ReadWrite);
    }

    QSgsLocalSocket(QLocalSocket *socket, QObject *parent = nullptr)
        : QSgsMultiSocket(parent)
        , socket(socket)
    {
        connect(socket, &QLocalSocket::connected, this, &QSgsLocalSocket::socketConnected);
        connect(socket, &QLocalSocket::disconnected, this, &QSgsLocalSocket::socketDisconnected);
        connect(socket, &QLocalSocket::errorOccurred, this, &QSgsLocalSocket::localErrorOccurred);
        connect(socket, &QLocalSocket::readyRead, this, &QSgsLocalSocket::readyRead);
    }

    void disconnectFromHost() override
    {
        socket->disconnectFromServer();
    }

    bool isSocketConnected() const override
    {
        return socket->state() == QLocalSocket::ConnectedState;
    }

    QString peerAddress() const override
    {
        return socket->serverName();
    }

    bool canReadLine() const override
    {
        return socket->canReadLine();
    }

    QByteArray readLine() override
    {
        return socket->readLine();
    }

    bool writeLine(const QByteArray &lineData) override
    {
        QByteArray removeNewline = lineData;
        removeNewline.replace('\r', ' ');
        removeNewline.replace('\n', ' ');
        removeNewline.append('\n');
        quint64 size = socket->write(removeNewline);
        socket->flush();
        return size == (quint64)removeNewline.length();
    }

    SocketType type() const override
    {
        return LocalSocket;
    }

private slots:
    void localErrorOccurred(QLocalSocket::LocalSocketError error)
    {
        emit errorOccurred(static_cast<QAbstractSocket::SocketError>(error));
    }

private:
    QLocalSocket *socket;
};

// TODO: Is there a Qt way for wrapping stdin and stdout of current running process (instead of the process to be run!!!!) to QIODevice?
// I don't like running a separate thread just for monitoring if there is data on stdin, and emitting readyRead on that thread, where a mutex should be used for thread safety.
class QSgsSubProcessSocket : public QSgsMultiSocket
{
    Q_OBJECT

public:
    QSgsSubProcessSocket(const QString &program, const QStringList &arguments, QObject *parent = nullptr)
        : QSgsMultiSocket(parent)
    {
        socket = new QProcess(this);
        socket->setProgram(program);
        socket->setProcessChannelMode(QProcess::ForwardedErrorChannel);
        socket->setReadChannel(QProcess::StandardOutput);
        socket->setArguments(arguments);
        connect(socket, &QProcess::started, this, &QSgsSubProcessSocket::socketConnected);
        connect(socket, &QProcess::finished, this, &QSgsSubProcessSocket::socketDisconnected);
        connect(socket, &QProcess::errorOccurred, this, &QSgsSubProcessSocket::processErrorOccurred);
        connect(socket, &QProcess::readyRead, this, &QSgsSubProcessSocket::readyRead);
        socket->start();
    }

    void disconnectFromHost() override
    {
        socket->kill();
    }

    bool isSocketConnected() const override
    {
        return socket->state() == QProcess::Running;
    }

    QString peerAddress() const override
    {
        return socket->program();
    }

    bool canReadLine() const override
    {
        return socket->canReadLine();
    }

    QByteArray readLine() override
    {
        return socket->readLine();
    }

    bool writeLine(const QByteArray &lineData) override
    {
        QByteArray removeNewline = lineData;
        removeNewline.replace('\r', ' ');
        removeNewline.replace('\n', ' ');
#ifdef Q_OS_WIN
        removeNewline.append('\r');
#endif
        removeNewline.append('\n');
        quint64 size = socket->write(removeNewline);
        // socket->flush(); // QProcess has no flush() function
        return size == (quint64)removeNewline.length();
    }

    SocketType type() const override
    {
        return SubProcess;
    }

private slots:
    void processErrorOccurred(QProcess::ProcessError error)
    {
        static const QMap<QProcess::ProcessError, QAbstractSocket::SocketError> errorMap {
            // clang-format off
            std::make_pair(QProcess::FailedToStart, QAbstractSocket::HostNotFoundError),
            std::make_pair(QProcess::Crashed, QAbstractSocket::RemoteHostClosedError),
            std::make_pair(QProcess::Timedout, QAbstractSocket::SocketTimeoutError),
            std::make_pair(QProcess::ReadError, QAbstractSocket::NetworkError),
            std::make_pair(QProcess::WriteError, QAbstractSocket::NetworkError),
            std::make_pair(QProcess::UnknownError, QAbstractSocket::UnknownSocketError),
            // clang-format on
        };

        emit errorOccurred(errorMap.value(error, QAbstractSocket::UnknownSocketError));
    }

private:
    QProcess *socket;
};

namespace {
class StdInMonitor;
}
// since StdInMonitor and QSgsStdInOutSocket depends on each other, implementation of this class must be under the StdInMonitor class
class QSgsStdInOutSocket : public QSgsMultiSocket
{
    Q_OBJECT

public:
    QSgsStdInOutSocket(QObject *parent = nullptr);
    void disconnectFromHost() override;
    bool isSocketConnected() const override;
    QString peerAddress() const override;
    bool canReadLine() const override;
    QByteArray readLine() override;
    bool writeLine(const QByteArray &lineData) override;
    SocketType type() const override;

private slots:
    void stdinError();
    void stdinFail();

private:
    StdInMonitor *socket;
};

namespace {
class StdInMonitor : public QThread
{
    Q_OBJECT

public:
    explicit StdInMonitor(QSgsStdInOutSocket *parent)
        : QThread(parent)
    {
    }

    void run() override
    {
        emit q->socketConnected();
        setTerminationEnabled(true);
        char localBuffer[16000];
        for (memset(localBuffer, 0, sizeof(localBuffer)); !std::cin.getline(localBuffer, 15999).eof(); memset(localBuffer, 0, sizeof(localBuffer))) {
            if (std::cin.fail()) {
                emit readFail();
                break;
            }
            QMutexLocker l(&mutex);
            bool ok = buf.seek(buf.size());
            if (!ok) {
                emit readError();
                continue;
            }

            buf.write(QByteArray(localBuffer));
            emit readyRead();
        }
        emit eof();
        exit(0);
    }

public:
    QSgsStdInOutSocket *q;
    QMutex mutex;
    QBuffer buf;

signals:
    void readyRead();
    void eof();
    void readError();
    void readFail();
};
} // namespace

// implementations of QSgsStdInOutSocket

QSgsStdInOutSocket::QSgsStdInOutSocket(QObject *parent)
    : QSgsMultiSocket(parent)
{
    socket = new StdInMonitor(this);
    connect(socket, &StdInMonitor::eof, this, &QSgsStdInOutSocket::socketDisconnected);
    connect(socket, &StdInMonitor::readError, this, &QSgsStdInOutSocket::stdinError);
    connect(socket, &StdInMonitor::readFail, this, &QSgsStdInOutSocket::stdinFail);
    connect(socket, &StdInMonitor::readyRead, this, &QSgsStdInOutSocket::readyRead);
    socket->start();
}

void QSgsStdInOutSocket::disconnectFromHost()
{
    socket->terminate();
    emit socketDisconnected();
}

bool QSgsStdInOutSocket::isSocketConnected() const
{
    return socket->isRunning();
}

QString QSgsStdInOutSocket::peerAddress() const
{
    return QStringLiteral("stdio");
}

bool QSgsStdInOutSocket::canReadLine() const
{
    QMutexLocker l(&(socket->mutex));
    socket->buf.reset();
    return socket->buf.canReadLine();
}

QByteArray QSgsStdInOutSocket::readLine()
{
    QMutexLocker l(&(socket->mutex));
    socket->buf.reset();
    return socket->buf.readLine();
}

bool QSgsStdInOutSocket::writeLine(const QByteArray &lineData)
{
    QByteArray removeNewline = lineData;
    removeNewline.replace('\r', ' ');
    removeNewline.replace('\n', ' ');
    std::cout << removeNewline.constData() << std::endl;
    if (std::cout.fail()) {
        emit errorOccurred(QAbstractSocket::RemoteHostClosedError);
        disconnectFromHost();
        return false;
    }

    return true;
}

QSgsMultiSocket::SocketType QSgsStdInOutSocket::type() const
{
    return SubProcess;
}

void QSgsStdInOutSocket::stdinError()
{
    emit errorOccurred(QAbstractSocket::SocketAccessError);
}

void QSgsStdInOutSocket::stdinFail()
{
    emit errorOccurred(QAbstractSocket::RemoteHostClosedError);
}

// implementations of QSgsStdInOutSocket end

QSgsMultiSocket *QSgsMultiSocket::ConnectToHost(const QHostAddress &host, quint16 port, QObject *parent)
{
    return new QSgsTcpSocket(host, port, parent);
}

QSgsMultiSocket *QSgsMultiSocket::ConnectToHost(const QString &localName, QObject *parent)
{
    return new QSgsLocalSocket(localName, parent);
}

QSgsMultiSocket *QSgsMultiSocket::WrapStdio(QObject *parent)
{
    return new QSgsStdInOutSocket(parent);
}

class QSgsMultiServerPrivate : public QObject
{
    Q_OBJECT

public:
    QSgsMultiServerPrivate(QSgsMultiServer *parent)
        : QObject(parent)
        , multiServer(parent)
        , tcpServer(nullptr)
        , localServer(nullptr)
    {
    }

    QSgsMultiServer *multiServer;
    QTcpServer *tcpServer;
    QLocalServer *localServer;

public slots:
    void tcpServerNewConnection()
    {
        QTcpSocket *socket;
        while ((socket = tcpServer->nextPendingConnection()) != nullptr) {
            QSgsMultiSocket *multiSocket = new QSgsTcpSocket(socket, multiServer);
            emit multiServer->newConnection(multiSocket);
        }
    }

    void localServerNewConnection()
    {
        QLocalSocket *socket;
        while ((socket = localServer->nextPendingConnection()) != nullptr) {
            QSgsMultiSocket *multiSocket = new QSgsLocalSocket(socket, multiServer);
            emit multiServer->newConnection(multiSocket);
        }
    }
};

QSgsMultiServer::QSgsMultiServer(QObject *parent)
    : QObject(parent)
    , d(new QSgsMultiServerPrivate(this))
{
    d->tcpServer = new QTcpServer(this);
    d->localServer = new QLocalServer(this);

    connect(d->tcpServer, &QTcpServer::newConnection, d, &QSgsMultiServerPrivate::tcpServerNewConnection);
    connect(d->localServer, &QLocalServer::newConnection, d, &QSgsMultiServerPrivate::localServerNewConnection);
}

void QSgsMultiServer::listen()
{
    d->tcpServer->listen(ServerConfig.tcpServer.bindIp, ServerConfig.tcpServer.bindPort);
    d->localServer->listen(ServerConfig.game.serverName);
}

QSgsMultiSocket *QSgsMultiServer::createSubProcess(const QString &program, const QStringList &arguments)
{
    return new QSgsSubProcessSocket(program, arguments, this);
}

#include "qsgsmultiserver.moc"
