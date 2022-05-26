
#ifndef TOUHOUKILL_SERVERCONFIG_H
#define TOUHOUKILL_SERVERCONFIG_H

#include <QHostAddress>
#include <QString>

struct ServerConfigStruct
{
    // see https://github.com/lwtmusou/touhoukill/issues/22

    ServerConfigStruct()
        : parsed(false)
    {
        defaultValues();
    }

    void defaultValues();
    bool parse();
    bool readConfigFile();
    bool saveConfigFile();

    bool parsed;

    enum FreeAssignOptions
    {
        FreeAssignNo,
        FreeAssignAll,
        FreeAssignOwner,
    };

    enum HegemonyRewardOptions
    {
        HegemonyRewardNone,
        HegemonyRewardInstant,
        HegemonyRewardPostponed,
    };

    QString mode; // or Mode *mode

    struct
    {
        QHostAddress bindIp;
        uint16_t bindPort;
        bool simc;
        bool chat;
    } network;

    struct
    {
        int timeout;
        int nullificationTimeout;
        QString serverName;
        bool shuffleSeat;
        bool latestGeneral;
        int pileSwap;
        bool enableAi;
        int aiDelay;
        bool aiLimit;
        int modifiedAiDelay;
        QStringList enabledPackages;
        int multiGenerals;
    } game;

    struct
    {
        bool enable;
        bool freeChoose;
    } cheat;

    struct
    {
        int choiceLord;
        int choiceNonLord;
        int choiceOthers;
        int choiceGods;
        bool lordSkill;
        int choiceMulti;

        struct
        {
            FreeAssignOptions freeAssign;
        } cheat;
        QList<QPair<QString, int>> banLists;
    } role;

    struct
    {
        int choice;
        HegemonyRewardOptions firstShow, companion, halfHp;
        int careeristKillReward; // 0 for as-usual, > 0 for always draw this number of card
    } hegemony;
};

#endif
