#ifndef TOUHOUKILL_QMLUI_H_
#define TOUHOUKILL_QMLUI_H_

#include "clientstruct.h"
#include "player.h"

#include <QObject>
#include <QUrl>

class TouhouKillQmlUiGlobal : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString ButtonFontFace READ buttonFontFace STORED false)
    Q_PROPERTY(QString SkillButtonFontFace READ skillButtonFontFace STORED false)
    Q_PROPERTY(QString GameFontFace READ gameFontFace STORED false)

public:
    Q_INVOKABLE QUrl getUrl(const QString &path) const;
    Q_INVOKABLE bool isHegemonyGameMode(const QString &mode) const;
    Q_INVOKABLE bool isNormalGameMode(const QString &mode) const;
    Q_INVOKABLE bool isPlayerMainPhase(Player::Phase phase) const;
    Q_INVOKABLE QString playerPhaseToString(Player::Phase phase) const;

    QString buttonFontFace() const;
    QString skillButtonFontFace() const;
    QString gameFontFace() const;
};

class TouhouKillServerInfoStruct : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString Name READ Name STORED false)
    Q_PROPERTY(QString GameMode READ GameMode STORED false)
    Q_PROPERTY(QString GameRuleMode READ GameRuleMode STORED false)
    Q_PROPERTY(int OperationTimeout READ OperationTimeout STORED false)
    Q_PROPERTY(int NullificationCountDown READ NullificationCountDown STORED false)
    Q_PROPERTY(QStringList Extensions READ Extensions STORED false)
    Q_PROPERTY(bool RandomSeat READ RandomSeat STORED false)
    Q_PROPERTY(bool EnableCheat READ EnableCheat STORED false)
    Q_PROPERTY(bool FreeChoose READ FreeChoose STORED false)
    Q_PROPERTY(bool Enable2ndGeneral READ Enable2ndGeneral STORED false)
    Q_PROPERTY(bool EnableSame READ EnableSame STORED false)
    Q_PROPERTY(bool EnableAI READ EnableAI STORED false)
    Q_PROPERTY(bool DisableChat READ DisableChat STORED false)
    Q_PROPERTY(int MaxHpScheme READ MaxHpScheme STORED false)
    Q_PROPERTY(int Scheme0Subtraction READ Scheme0Subtraction STORED false)
    Q_PROPERTY(bool DuringGame READ DuringGame STORED false)

#ifndef Q_MOC_RUN

public:
#define ServerInfoStructFuncDef(type, name) \
    inline type name() const                \
    {                                       \
        return ServerInfo.name;             \
    }

    ServerInfoStructFuncDef(QString, Name);
    ServerInfoStructFuncDef(QString, GameMode);
    ServerInfoStructFuncDef(QString, GameRuleMode);
    ServerInfoStructFuncDef(int, OperationTimeout);
    ServerInfoStructFuncDef(int, NullificationCountDown);
    ServerInfoStructFuncDef(QStringList, Extensions);
    ServerInfoStructFuncDef(bool, RandomSeat);
    ServerInfoStructFuncDef(bool, EnableCheat);
    ServerInfoStructFuncDef(bool, FreeChoose);
    ServerInfoStructFuncDef(bool, Enable2ndGeneral);
    ServerInfoStructFuncDef(bool, EnableSame);
    ServerInfoStructFuncDef(bool, EnableAI);
    ServerInfoStructFuncDef(bool, DisableChat);
    ServerInfoStructFuncDef(int, MaxHpScheme);
    ServerInfoStructFuncDef(int, Scheme0Subtraction);
    ServerInfoStructFuncDef(bool, DuringGame);

#undef ServerInfoStructFuncDef
#endif
};

#endif
