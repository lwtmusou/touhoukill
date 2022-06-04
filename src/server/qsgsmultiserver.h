#ifndef TOUHOUKILL_QSGSMULTISERVER_H_
#define TOUHOUKILL_QSGSMULTISERVER_H_

#include <QObject>
// needed for QAbstractSocket::SocketError
#include <QAbstractSocket>

// Mostly based on previous socket implementation.

class QHostAddress;

// The public interface of this class should be pure virtual.
// any implementation should be inherited class in the cpp file.

class QSgsMultiSocket : public QObject // TODO: shouldn't it be QIODevice?
{
    Q_OBJECT

public:
    enum SocketType
    {
        TcpSocket,
        LocalSocket,
        SubProcess,
        BluetoothSocket, // TBD
    };

    ~QSgsMultiSocket() override = default;

    static QSgsMultiSocket *ConnectToHost(const QHostAddress &host, quint16 port, QObject *parent = nullptr);
    static QSgsMultiSocket *ConnectToHost(const QString &localName, QObject *parent = nullptr);
    static QSgsMultiSocket *WrapStdio(QObject *parent = nullptr);

    virtual void disconnectFromHost() = 0;

    virtual bool isSocketConnected() const = 0;
    virtual QString peerAddress() const = 0;

    virtual bool canReadLine() const = 0;
    virtual QByteArray readLine() = 0;
    virtual bool writeLine(const QByteArray &lineData) = 0;

    virtual SocketType type() const = 0;

signals:
    void socketConnected();
    void socketDisconnected();
    void errorOccurred(QAbstractSocket::SocketError socketError);
    void readyRead();

protected:
    QSgsMultiSocket(QObject *parent = nullptr);

private:
    Q_DISABLE_COPY_MOVE(QSgsMultiSocket)
};

class QSgsMultiServerPrivate;

class QSgsMultiServer : public QObject
{
    Q_OBJECT

public:
    QSgsMultiServer(QObject *parent = nullptr);
    ~QSgsMultiServer() override = default;

    void listen();
    QSgsMultiSocket *createSubProcess(const QString &program, const QStringList &arguments);

signals:
    void newConnection(QSgsMultiSocket *client);

private:
    QSgsMultiServerPrivate *d;
};

#endif
