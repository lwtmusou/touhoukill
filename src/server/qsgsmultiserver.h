#ifndef TOUHOUKILL_QSGSMULTISERVER_H_
#define TOUHOUKILL_QSGSMULTISERVER_H_

#include <QObject>

class QSgsMultiClientPrivate;

class QSgsMultiClient
{
public:
    QSgsMultiClient();

    QIODevice *underlyingIoDevice() const;
    void disconnectFromHost();

private:
    Q_DISABLE_COPY_MOVE(QSgsMultiClient)
    QSgsMultiClientPrivate *d;
};

class QSgsMultiClientPointer final
{
public:
    QSgsMultiClientPointer(const QSgsMultiClientPointer &) = default;
    QSgsMultiClientPointer &operator=(const QSgsMultiClientPointer &) = default;
    inline QSgsMultiClientPointer(const QSgsMultiClient *p)
        : d(p)
    {
    }

    inline const QSgsMultiClient *data() const
    {
        return d;
    }

    inline const QSgsMultiClient *operator->() const
    {
        return d;
    }

    inline const QSgsMultiClient &operator*() const
    {
        Q_ASSERT(d != nullptr);
        return *d;
    }

    inline operator const QSgsMultiClient *() const
    {
        return d;
    }

    inline bool isNull() const
    {
        return d == nullptr;
    }

    inline operator QIODevice *() const
    {
        if (d == nullptr)
            return nullptr;

        return d->underlyingIoDevice();
    }

private:
    QSgsMultiClientPointer() = delete;
    const QSgsMultiClient *d;
};

class QSgsMultiServerPrivate;

class QSgsMultiServer : public QObject
{
    Q_OBJECT

public:
    QSgsMultiServer(QObject *parent = nullptr);
    ~QSgsMultiServer() override = default;

    void listen();

signals:
    void newConnection(QSgsMultiClientPointer client);

private:
    QSgsMultiServerPrivate *d;
};

#endif
