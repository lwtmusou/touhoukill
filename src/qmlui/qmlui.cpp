#include "card.h"

#include <QApplication>
#include <QtQml>

namespace {
void registerCore()
{
    int ret = qmlRegisterUncreatableType<Card>("rocks.touhousatsu", 1, 0, "Card", "It is currently not supported to create a card in QML.");

    if (ret == -1)
        qDebug() << "Failed to register Card to Qml";
}
} // namespace

// NOLINTNEXTLINE
Q_COREAPP_STARTUP_FUNCTION(registerCore);
