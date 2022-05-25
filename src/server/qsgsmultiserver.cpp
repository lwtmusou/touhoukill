
#include "qsgsmultiserver.h"

class QSgsMultiServerPrivate
{
};

QSgsMultiServer::QSgsMultiServer(QObject *parent)
    : QObject(parent)
    , d(new QSgsMultiServerPrivate)
{
}
