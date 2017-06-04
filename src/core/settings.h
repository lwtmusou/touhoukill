#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "protocol.h"

#include <QBrush>
#include <QFont>
#include <QPixmap>
#include <QRectF>
#include <QSettings>

class Settings : public QSettings
{
    Q_OBJECT

public:
    explicit Settings();
    void init();

    const QRectF Rect;

    static const QString &getQSSFileContent();
    QFont BigFont;
    QFont SmallFont;
    QFont TinyFont;

    QFont AppFont;
    QFont UIFont;
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
    bool DisableLua;
    bool LimitRobot;

    QStringList ExtraHiddenGenerals;
    QStringList RemovedHiddenGenerals;

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
    bool EnableEffects;
    bool EnableLastWord;
    bool EnableBgMusic;
    bool UseLordBGM;
    float BGMVolume;
    float EffectVolume;

    QString BackgroundImage;
    QString TableBgImage;
    QString RecordSavePath;
    bool UseLordBackdrop;
    bool EnableSurprisingGenerals;
    QStringList KnownSurprisingGenerals;

    // consts
    static const int S_SURRENDER_REQUEST_MIN_INTERVAL;
    static const int S_PROGRESS_BAR_UPDATE_INTERVAL;
    static const int S_SERVER_TIMEOUT_GRACIOUS_PERIOD;
    static const int S_MOVE_CARD_ANIMATION_DURATION;
    static const int S_JUDGE_ANIMATION_DURATION;
    static const int S_JUDGE_LONG_DELAY;
};

extern Settings Config;

#endif
