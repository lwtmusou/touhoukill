#include "qmlui.h"

#include "card.h"
#include "clientplayer.h"
#include "player.h"

#include <QApplication>
#include <QtQml>

QUrl TouhouKillQmlUiGlobal::getUrl(const QString &path) const
{
    return QDir::current().absoluteFilePath(path);
}

namespace {
void registerCore()
{
    {
        int ret = qmlRegisterSingletonType<TouhouKillQmlUiGlobal>("rocks.touhousatsu", 1, 0, "G", [](QQmlEngine *, QJSEngine *) -> QObject * {
            return new TouhouKillQmlUiGlobal;
        });

        if (ret == -1)
            qDebug() << "Failed to register TouhouKillQmlUiGlobal to Qml";
    }

    {
        int ret = qmlRegisterUncreatableType<Card>("rocks.touhousatsu", 1, 0, "Card", "It is currently not supported to create a card in QML.");

        if (ret == -1)
            qDebug() << "Failed to register Card to Qml";
    }

    {
        int ret = qmlRegisterUncreatableType<Player>("rocks.touhousatsu", 1, 0, "Player", "It is currently not supported to create a player in QML.");

        if (ret == -1)
            qDebug() << "Failed to register Player to Qml";
    }

    {
        int ret = qmlRegisterUncreatableType<ClientPlayer>("rocks.touhousatsu", 1, 0, "ClientPlayer", "It is currently not supported to create a player in QML.");

        if (ret == -1)
            qDebug() << "Failed to register ClientPlayer to Qml";
    }
}
} // namespace

// NOLINTNEXTLINE
Q_COREAPP_STARTUP_FUNCTION(registerCore);
