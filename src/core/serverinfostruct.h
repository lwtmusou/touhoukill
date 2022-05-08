#ifndef TOUHOUKILL_SERVERINFOSTRUCT_H
#define TOUHOUKILL_SERVERINFOSTRUCT_H

#include "json.h"
#include "protocol.h"
#include "qsgscore.h"

class Mode;

struct QSGS_CORE_EXPORT ServerInfoStruct
{
    ServerInfoStruct();

    bool parseLegacy(const QString &str);

    bool parse(const QVariant &object);
    QVariant serialize() const;

    QString Name;
    const Mode *GameMode;
    QString GameModeStr;
    QString GameRuleMode;
    int OperationTimeout;
    int NullificationCountDown;
    QStringList EnabledPackages;
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

    //Get the timeout allowance for a command. Server countdown is more lenient than the client.
    //@param command: type of command
    //@return countdown for command in milliseconds.
    time_t getCommandTimeout(QSanProtocol::CommandType command, QSanProtocol::ProcessInstanceType instance, int operationRate = 2);
};

extern QSGS_CORE_EXPORT ServerInfoStruct ServerInfo;

#endif
