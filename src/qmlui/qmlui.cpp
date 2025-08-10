#include "qmlui.h"

#include "card.h"
#include "clientplayer.h"
#include "player.h"
#include "skill.h"

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

    {
        int ret = qmlRegisterUncreatableType<Skill>("rocks.touhousatsu", 1, 0, "Skill", "It is currently not supported to create a skill in QML.");

        if (ret == -1)
            qDebug() << "Failed to register Skill to Qml";
    }

    {
        int ret = qmlRegisterUncreatableType<ViewAsSkill>("rocks.touhousatsu", 1, 0, "ViewAsSkill", "It is currently not supported to create a view-as skill in QML.");

        if (ret == -1)
            qDebug() << "Failed to register ViewAsSkill to Qml";
    }

    {
        int ret = qmlRegisterUncreatableType<FilterSkill>("rocks.touhousatsu", 1, 0, "FilterSkill", "It is currently not supported to create a filter skill in QML.");

        if (ret == -1)
            qDebug() << "Failed to register FilterSkill to Qml";
    }

    {
        int ret = qmlRegisterUncreatableType<ProhibitSkill>("rocks.touhousatsu", 1, 0, "ProhibitSkill", "It is currently not supported to create a prohibit skill in QML.");

        if (ret == -1)
            qDebug() << "Failed to register ProhibitSkill to Qml";
    }

    {
        int ret = qmlRegisterUncreatableType<DistanceSkill>("rocks.touhousatsu", 1, 0, "DistanceSkill", "It is currently not supported to create a distance skill in QML.");

        if (ret == -1)
            qDebug() << "Failed to register DistanceSkill to Qml";
    }

    {
        int ret = qmlRegisterUncreatableType<MaxCardsSkill>("rocks.touhousatsu", 1, 0, "MaxCardsSkill", "It is currently not supported to create a max-cards skill in QML.");

        if (ret == -1)
            qDebug() << "Failed to register MaxCardsSkill to Qml";
    }

    {
        int ret = qmlRegisterUncreatableType<TargetModSkill>("rocks.touhousatsu", 1, 0, "TargetModSkill", "It is currently not supported to create a target-mod skill in QML.");

        if (ret == -1)
            qDebug() << "Failed to register TargetModSkill to Qml";
    }

    {
        int ret
            = qmlRegisterUncreatableType<AttackRangeSkill>("rocks.touhousatsu", 1, 0, "AttackRangeSkill", "It is currently not supported to create a attack-range skill in QML.");

        if (ret == -1)
            qDebug() << "Failed to register AttackRangeSkill to Qml";
    }
}
} // namespace

// NOLINTNEXTLINE
Q_COREAPP_STARTUP_FUNCTION(registerCore);
