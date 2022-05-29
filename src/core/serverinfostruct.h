#ifndef TOUHOUKILL_SERVERINFOSTRUCT_H
#define TOUHOUKILL_SERVERINFOSTRUCT_H

#include "protocol.h"
#include "qsgscore.h"

class Mode;

struct QSGS_CORE_EXPORT ServerInfoStruct
{
    ServerInfoStruct();

    Q_DECL_DEPRECATED bool parseLegacy(const QString &str);

    bool parse(const QVariant &object);
    bool parse(const QJsonValue &value);
    bool parsed() const;
    bool isMultiGeneralEnabled() const;
    QJsonValue serialize() const;

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
    int GeneralsPerPlayer;
    bool EnableSame;
    bool EnableAI;
    bool DisableChat;
    int MaxHpScheme;
    int Scheme0Subtraction;

    // Ban lists:
    // Role mode:
    // Type 1. simple banlist: ban a specific general, for whatever condition
    // Type 2. multi-general banlist: ban a specific general only for multi-general enabled
    // Type 3. multi-general ban-for: ban a specific general only for head or only for deputy or for a specific general index
    // Type 4. multi-general team-unique ban: ban other of specific generals when one of them are selected before

    enum RoleBanFor
    {
        BanTeamStart = -4, // Ban type 4 Team-unique ban. Any BanFor value <= -4 is a team. At most one general of a specific team can be selected in anyone's general list.
        // e.g.: Ban Team -4 contains Reimu, Marisa and Mamizou. Ban Team -5 contains Mouko, Unzan and Mamizou.
        // Player A selected Mamizou as its General index 0 (and Gender is female according to Mamizou). She can't select Reimu, Marisa, Mouko or Unzan as her remained general since Mamizou is in both Ban Team -4 and -5.
        // Player B selected Unzan as its General inex 0 (and Gender is male according to Unzan), he can't select Mouko or Mamizou who are in Ban Team -5 w/ Unzan, but can select at most one of Reimu and Marisa as his remained general since they are not in Ban Team -5.

        BanHead = 0, // since Head general should always in index 0. Banned if multi-general is enabled and this general is head general (index 0)
        // any value >= BanHead means a specific General index for ban type 3. Note that ban type 3 for head is same as ban type 3 for index 0

        BanSimple = -1, // Ban type 1, banned for whatever condition
        BanMulti = -2, // Ban type 2, only banned if multi-general is enabled
        BanDeputy = -3, // Ban type 3 for deputy, only banned if multi-general is enabled and this general is not head general (index > 0)
    };

    // TODO: consider if the banlist should be passed to client
    QMultiHash<QString, RoleBanFor> RoleBanlist;

    // Hegemony mode:
    // No ban list is implemented, all generals should be enabled
    // 1v1:
    // TODO
    // 1v3:
    // TODO
    // 3v3:
    // TODO
    // XMode:
    // TODO

    //Get the timeout allowance for a command. Server countdown is more lenient than the client.
    //@param command: type of command
    //@return countdown for command in milliseconds.
    time_t getCommandTimeout(QSanProtocol::CommandType command, QSanProtocol::ProcessInstanceType instance, int operationRate = 2);
};

extern QSGS_CORE_EXPORT ServerInfoStruct ServerInfo;

#endif
