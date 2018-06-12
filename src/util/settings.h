#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "protocol.h"

#include <QBrush>
#include <QFont>
#include <QJSValue>
#include <QPixmap>
#include <QRectF>
#include <QSettings>

class Settings : public QSettings
{
    Q_OBJECT
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged STORED false)

    Q_PROPERTY(QColor TextEditColor MEMBER TextEditColor)
    Q_PROPERTY(QColor ToolTipBackgroundColor MEMBER ToolTipBackgroundColor)

    Q_PROPERTY(QString ServerName MEMBER ServerName)
    Q_PROPERTY(int CountDownSeconds MEMBER CountDownSeconds)
    Q_PROPERTY(int NullificationCountDown MEMBER NullificationCountDown)
    Q_PROPERTY(bool EnableMinimizeDialog MEMBER EnableMinimizeDialog)
    Q_PROPERTY(QString GameMode MEMBER GameMode)
    Q_PROPERTY(QStringList BanPackages MEMBER BanPackages)
    Q_PROPERTY(bool RandomSeat MEMBER RandomSeat)
    Q_PROPERTY(bool AssignLatestGeneral MEMBER AssignLatestGeneral)
    Q_PROPERTY(bool EnableCheat MEMBER EnableCheat)
    Q_PROPERTY(bool FreeChoose MEMBER FreeChoose)
    Q_PROPERTY(bool ForbidSIMC MEMBER ForbidSIMC)
    Q_PROPERTY(bool DisableChat MEMBER DisableChat)
    Q_PROPERTY(bool FreeAssignSelf MEMBER FreeAssignSelf)
    Q_PROPERTY(bool Enable2ndGeneral MEMBER Enable2ndGeneral)
    Q_PROPERTY(bool EnableSame MEMBER EnableSame)
    Q_PROPERTY(int MaxHpScheme MEMBER MaxHpScheme)
    Q_PROPERTY(int Scheme0Subtraction MEMBER Scheme0Subtraction)
    Q_PROPERTY(bool PreventAwakenBelow3 MEMBER PreventAwakenBelow3)
    Q_PROPERTY(QString Address MEMBER Address)
    Q_PROPERTY(bool EnableAI MEMBER EnableAI)
    Q_PROPERTY(int AIDelay MEMBER AIDelay)
    Q_PROPERTY(int OriginAIDelay MEMBER OriginAIDelay)
    Q_PROPERTY(bool AlterAIDelayAD MEMBER AlterAIDelayAD)
    Q_PROPERTY(int AIDelayAD MEMBER AIDelayAD)
    Q_PROPERTY(bool AIProhibitBlindAttack MEMBER AIProhibitBlindAttack)
    Q_PROPERTY(bool SurrenderAtDeath MEMBER SurrenderAtDeath)
    Q_PROPERTY(int LuckCardLimitation MEMBER LuckCardLimitation)
    Q_PROPERTY(ushort ServerPort MEMBER ServerPort)
    Q_PROPERTY(bool DisableLua MEMBER DisableLua)
    Q_PROPERTY(bool LimitRobot MEMBER LimitRobot)

    Q_PROPERTY(QStringList ExtraHiddenGenerals MEMBER ExtraHiddenGenerals)
    Q_PROPERTY(QStringList RemovedHiddenGenerals MEMBER RemovedHiddenGenerals)

    Q_PROPERTY(QString HostAddress MEMBER HostAddress)
    Q_PROPERTY(QString UserName MEMBER UserName)
    Q_PROPERTY(QString UserAvatar MEMBER UserAvatar)
    Q_PROPERTY(QStringList HistoryIPs MEMBER HistoryIPs)
    Q_PROPERTY(ushort DetectorPort MEMBER DetectorPort)
    Q_PROPERTY(int MaxCards MEMBER MaxCards)

    Q_PROPERTY(bool EnableHotKey MEMBER EnableHotKey)
    Q_PROPERTY(bool NeverNullifyMyTrick MEMBER NeverNullifyMyTrick)
    Q_PROPERTY(bool EnableAutoTarget MEMBER EnableAutoTarget)
    Q_PROPERTY(bool EnableIntellectualSelection MEMBER EnableIntellectualSelection)
    Q_PROPERTY(bool EnableDoubleClick MEMBER EnableDoubleClick)
    Q_PROPERTY(bool EnableAutoUpdate MEMBER EnableAutoUpdate)
    Q_PROPERTY(bool EnableAutoSaveRecord MEMBER EnableAutoSaveRecord)
    Q_PROPERTY(bool NetworkOnly MEMBER NetworkOnly)

    Q_PROPERTY(bool DefaultHeroSkin MEMBER DefaultHeroSkin)

    Q_PROPERTY(int BubbleChatBoxDelaySeconds MEMBER BubbleChatBoxDelaySeconds)
    Q_PROPERTY(int OperationTimeout MEMBER OperationTimeout)
    Q_PROPERTY(bool OperationNoLimit MEMBER OperationNoLimit)

    Q_PROPERTY(QString BackgroundImage MEMBER BackgroundImage)
    Q_PROPERTY(QString TableBgImage MEMBER TableBgImage)
    Q_PROPERTY(QString RecordSavePath MEMBER RecordSavePath)
    Q_PROPERTY(bool EnableSurprisingGenerals MEMBER EnableSurprisingGenerals)
    Q_PROPERTY(QStringList KnownSurprisingGenerals MEMBER KnownSurprisingGenerals)

public:
    explicit Settings();
    void init();

    Q_INVOKABLE QJSValue jsValue(const QString &key, const QJSValue &defaultValue = QJSValue()) const;
    Q_INVOKABLE void setJsValue(const QString &key, const QJSValue &value);

    QFont font() const;
    void setFont(const QFont &font);

signals:
    void fontChanged(const QFont &font);

public:
    const QRectF Rect;

    static const QString &getQSSFileContent();
    QColor TextEditColor;
    QColor ToolTipBackgroundColor;

    // server side
    QString ServerName;
    int CountDownSeconds;
    int NullificationCountDown;
    bool EnableMinimizeDialog;
    QString GameMode;
    QStringList BanPackages;
    bool RandomSeat;
    bool AssignLatestGeneral;
    bool EnableCheat;
    bool FreeChoose;
    bool ForbidSIMC;
    bool DisableChat;
    bool FreeAssignSelf;
    bool Enable2ndGeneral;
    bool EnableSame;
    int MaxHpScheme;
    int Scheme0Subtraction;
    bool PreventAwakenBelow3;
    QString Address;
    bool EnableAI;
    int AIDelay;
    int OriginAIDelay;
    bool AlterAIDelayAD;
    int AIDelayAD;
    bool AIProhibitBlindAttack;
    bool SurrenderAtDeath;
    int LuckCardLimitation;
    ushort ServerPort;
    bool LimitRobot;

    QStringList ExtraHiddenGenerals;
    QStringList RemovedHiddenGenerals;

    QString HegemonyFirstShowReward;
    QString HegemonyCompanionReward;
    QString HegemonyHalfHpReward;
    QString HegemonyCareeristKillReward;

    // client side
    QString HostAddress;
    QString UserName;
    QString UserAvatar;
    QStringList HistoryIPs;
    ushort DetectorPort;
    int MaxCards;

    bool EnableHotKey;
    bool NeverNullifyMyTrick;
    bool EnableAutoTarget;
    bool EnableIntellectualSelection;
    bool EnableDoubleClick;
    bool EnableAutoUpdate;
    bool EnableAutoSaveRecord;
    bool NetworkOnly;

    bool DefaultHeroSkin;

    int BubbleChatBoxDelaySeconds;
    int OperationTimeout;
    bool OperationNoLimit;

    QString BackgroundImage;
    QString TableBgImage;
    QString RecordSavePath;

    // consts
    static const int S_SURRENDER_REQUEST_MIN_INTERVAL;
    static const int S_PROGRESS_BAR_UPDATE_INTERVAL;
    static const int S_MOVE_CARD_ANIMATION_DURATION;
    static const int S_JUDGE_ANIMATION_DURATION;
    static const int S_JUDGE_LONG_DELAY;

    bool AutoUpdateNeedsRestart;
    bool AutoUpdateDataRececived;
};

Settings *configInstance();
#define Config (*(configInstance()))

#endif
