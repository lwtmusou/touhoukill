#ifndef QSANGUOSHA_SERVERINFOSTRUCT_H
#define QSANGUOSHA_SERVERINFOSTRUCT_H

#include "json.h"
#include "player.h"
#include "protocol.h"
#include "qsgscore.h"

struct QSGS_CORE_EXPORT ServerInfoStruct
{
    bool parseLegacy(const QString &str);

    bool parse(const QVariant &object);
    QVariant serialize() const;

    //Get the timeout allowance for a command. Server countdown is more lenient than the client.
    //@param command: type of command
    //@return countdown for command in milliseconds.
    time_t getCommandTimeout(QSanProtocol::CommandType command, QSanProtocol::ProcessInstanceType instance, int operationRate = 2);

    QString Name;
    QString GameMode;
    QString GameRuleMode;
    int OperationTimeout;
    int NullificationCountDown;
    QStringList Extensions;
    bool RandomSeat;
    bool EnableCheat;
    bool FreeChoose;
    bool Enable2ndGeneral;
    bool EnableSame;
    bool EnableAI;
    bool DisableChat;
    int MaxHpScheme;
    int Scheme0Subtraction;

    bool DuringGame;
};

extern QSGS_CORE_EXPORT ServerInfoStruct ServerInfo;

#endif
