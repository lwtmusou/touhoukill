
#include "qsgsmultiserver.h"

QSgsMultiClient::QSgsMultiClient()
{
}

QIODevice *QSgsMultiClient::underlyingIoDevice() const
{
    // TODO
    return nullptr;
}

void QSgsMultiClient::disconnectFromHost()
{
    // TODO
}

class QSgsMultiServerPrivate
{
};

QSgsMultiServer::QSgsMultiServer(QObject *parent)
    : QObject(parent)
    , d(new QSgsMultiServerPrivate)
{
}

void QSgsMultiServer::listen()
{
}
