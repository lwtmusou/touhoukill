
#ifndef TOUHOUKILL_SERVERCONFIG_H
#define TOUHOUKILL_SERVERCONFIG_H

#include "global.h"
#include "serverinfostruct.h"

#include <QHostAddress>
#include <QString>

struct ServerConfigStruct
{
    // see https://github.com/lwtmusou/touhoukill/issues/22

    ServerConfigStruct();

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

    QStringList modesServing;

    struct
    {
        QHostAddress bindIp;
        uint16_t bindPort;
        bool simc;
    } tcpServer;

    struct
    {
        int timeout;
        int nullificationTimeout;
        QString serverName;
        bool shuffleSeat;
        bool latestGeneral;
        int pileSwap;
        bool chat;
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

    ServerInfoStruct toServerInfo(const QString &mode) const;
};

extern ServerConfigStruct *serverConfigInstanceFunc();
#define ServerConfig (*serverConfigInstanceFunc())

#endif
