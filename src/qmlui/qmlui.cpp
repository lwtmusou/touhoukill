#include "qmlui.h"

#include "card.h"
#include "client.h"
#include "clientplayer.h"
#include "general.h"
#include "player.h"
#include "skill.h"
#include "util.h"

#include <QApplication>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QtQml>

QUrl TouhouKillQmlUiGlobal::getAssetUrl(const QString &path) const
{
    return QDir::current().absoluteFilePath(path);
}

bool TouhouKillQmlUiGlobal::assetExists(const QString &path) const
{
    return QFileInfo::exists(path);
}

bool TouhouKillQmlUiGlobal::isHegemonyGameMode(const QString &mode) const
{
    return ::isHegemonyGameMode(mode);
}

bool TouhouKillQmlUiGlobal::isNormalGameMode(const QString &mode) const
{
    return ::isNormalGameMode(mode);
}

bool TouhouKillQmlUiGlobal::isPlayerMainPhase(Player::Phase phase) const
{
    return Player::isMainPhase(phase);
}

QString TouhouKillQmlUiGlobal::playerPhaseToString(Player::Phase phase) const
{
    return Player::getPhaseString(phase);
}

namespace {
struct FontFaceRecorder
{
    QString fontFace;

    explicit FontFaceRecorder(const QString &fontPath)
    {
        int id = QFontDatabase::addApplicationFont(QDir::currentPath() + fontPath);
        fontFace = QApplication::font().family();

        if (id != -1)
            fontFace = QFontDatabase::applicationFontFamilies(id).constFirst();
    }
};
} // namespace

QString TouhouKillQmlUiGlobal::buttonFontFace() const
{
    static const FontFaceRecorder fontFaceRecorder("/font/budingti.ttf");
    return fontFaceRecorder.fontFace;
}

QString TouhouKillQmlUiGlobal::skillButtonFontFace() const
{
    static const FontFaceRecorder fontFaceRecorder("/font/fzktf.ttf");
    return fontFaceRecorder.fontFace;
}

QString TouhouKillQmlUiGlobal::gameFontFace() const
{
    static const FontFaceRecorder fontFaceRecorder("/font/simli.ttf");
    return fontFaceRecorder.fontFace;
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
        int ret = qmlRegisterSingletonType<TouhouKillServerInfoStruct>("rocks.touhousatsu", 1, 0, "ServerInfo", [](QQmlEngine *, QJSEngine *) -> QObject * {
            return new TouhouKillServerInfoStruct;
        });

        if (ret == -1)
            qDebug() << "Failed to register ServerInfo to Qml";
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
        int ret = qmlRegisterUncreatableType<Client>("rocks.touhousatsu", 1, 0, "Client", "It is currently not supported to create a client in QML.");

        if (ret == -1)
            qDebug() << "Failed to register Client to Qml";
    }

    {
        int ret = qmlRegisterUncreatableType<ClientPlayer>("rocks.touhousatsu", 1, 0, "ClientPlayer", "It is currently not supported to create a player in QML.");

        if (ret == -1)
            qDebug() << "Failed to register ClientPlayer to Qml";
    }

    {
        int ret = qmlRegisterUncreatableType<General>("rocks.touhousatsu", 1, 0, "General", "It is currently not supported to create a general in QML.");

        if (ret == -1)
            qDebug() << "Failed to register General to Qml";
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
