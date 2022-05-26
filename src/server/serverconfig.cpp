
#include "serverconfig.h"
#include "mode.h"

#include <QDebug>
#include <QDir>
#include <QString>
#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

#include <iostream>

#if (defined(Q_OS_DARWIN) && !defined(Q_OS_MACOS)) || defined(Q_OS_ANDROID)
// mobile platforms don't use processes, so command line option shouldn't be get
// always use server config file for it
#else
#include <QCommandLineOption>
#include <QCommandLineParser>
#endif

namespace {
const QString &configFilePath()
{
    static QString p;
    if (p.isEmpty()) {
#ifdef Q_OS_UNIX
        if (getuid() == (uid_t)0)
            p = QStringLiteral("/etc/QSanguosha/server.config");
        else
#endif
            p = QDir::homePath() + QDir::separator() + QStringLiteral(".QSanguosha") + QDir::separator() + QStringLiteral("config.json");
    }

    return p;
}

QString helpText = QStringLiteral(R"desc(
QSanguoshaServer version X.X.X, Mogara.org
QSanguoshaServer is a separate server CLI program which starts a server and waits for .... (Description, I feel lazy to edit it.)

All options are read from config file %1 unless specified in command line. (except for Ban options, see description of --ab for details.)

Common options:
-h, --help show this help
-v, --version show version information

Config file options:
--sc, --save-config save the option to config file. Make sure you have enough permission to operate it. Defaults no.

Game mode options:
-m, --mode=<role_x,x,x, hegemony_x> the game mode which the server serves.

Network options:
-I, --bind-ip= bind ip address, default 0.0.0.0
-P, --bind-port= bind port, default 41392
--mc=, --same-ip-with-multiple-connection=<yes, no> same ip with multiple connection
-c, --chat=<yes, no> enables or disables chat

Game options:
-t, --timeout=<0,5~60> operation timeout, set 0 for no limit, default 15
-N, --nullification-timeout=<5~15> nullification timeout, default 8
-S, --server-name= the server name, default "QSanguosha's Server"
-s, --shuffle-seat=<yes, no> arange seats randomly, default yes
-l, --latest-general=<yes, no> assign latest general, default yes
--ps=, --pile-swap-limit=<0~15> pile swapping limit, set 0 for no limit
-A, --ai=<yes, no> AI switch (LuaAI)
--Ad=, --ai-delay=<0~5000> AI delay, default 1000
--Al=, --ai-limit=<yes, no> limit Lua AI in a game, default no
--Am=, --ai-delay-after-death=<-1,0~> AI delay after all players controlled by human died. -1 means do not modify AI delay, default -1
-p, --packages= comma-separated list of enabled packages, or "disabled:" followed by comma-separated list of disabled packages
--mg=, --multi-generals=<0,2(role)/3(hegemony)~> multi generals. 0 for do not use mulit generals, default 0

Ban options:
Currently we only support ban in Role mode and use following parameter multiple times to generate a full ban list.
--br=, --ban-role=<General name>,<BanFor>
  BanFor is an integer where:
    -1: ban this general for whatever condition,
    -2: ban this general if multi general is enabled
    0: ban this general for head if multi general is enabled. This is same as banning this general for General index 0.
    -3: ban this general for deputy if multi general is enabled
    1 and more: ban this general for General index <BanFor>.
    -4 and less: Team unique ban. Generals which are specified as same <BanFor>(less than or equal to -4) are treated as same team. At most one general of a specific team can be selected in anyone's general list.
--ab, --append-banlist-from-config-file append ban list from config file if specified.

Cheat options:
-Z, --cheat=<yes, no> enable cheating
--Zc=, --free-choose=<yes, no> (--cheat) choose generals and cards freely

Role mode specific options:
--cl=, --choice-lord= Numbers of lord generals can be chosen by lord player, default 8
--cn=, --choice-nonlord= Numbers of non-lord generals can be chosen by lord player, default 6
--co=, --choice-others= Number of generals can be chosen by non-lord player, default 6
--cg=, --choice-gods= Number of gods can be chosen by player, default 1
--ls=, --lord-skill=<yes, no> enable or disable lord skill, default Yes

--cm=, --choice-multi= Number of generals can be chosen after first general is chosen

--Zf=, --free-assign=<yes, no, self> (--cheat) assign role and seat freely

Hegemony mode specific options:
--ch=, --choice-hegemony=<kingdomcount*(multigenerals-1)+1~generalscount/n> numbers of generals can be chosen by player, default 6
--rf=, --first-show-reward=<none, instant, postponed> default none
--rc=, --companion-reward=<none, instant, postponed> default instant
--rh=, --half-hp-reward=<none, instant, postponed> default instant
--rk=, --careerist-kill= number of cards a careerist draws when killing a player. default 0 which means following official hegemony rule
)desc");

void showHelp()
{
    std::cout << (helpText.arg(configFilePath())).toLocal8Bit().constData() << std::endl;
}

bool stringToBool(const QString &value, bool *ok = nullptr)
{
    if (ok == nullptr) {
        static bool _ok;
        ok = &_ok;
    }

    *ok = false;

    // empty value means true by default!
    // it means arguments are set but not with any values
    if (value.isEmpty()) {
        *ok = true;
        return true;
    }

    QString lowerValue = value.toLower();

    static const QStringList falseValues {
        // clang-format off
        QStringLiteral("no"),
        QStringLiteral("none"),
        QStringLiteral("false"),
        QStringLiteral("off"),
        QStringLiteral("0"),
        // clang-format on
    };

    static const QStringList trueValues {
        // clang-format off
        QStringLiteral("yes"),
        QStringLiteral("true"),
        QStringLiteral("on"),
        QStringLiteral("1"),
        // clang-format on
    };

    foreach (const QString &trueValue, trueValues) {
        if (lowerValue == trueValue) {
            *ok = true;
            return true;
        }
    }

    foreach (const QString &falseValue, falseValues) {
        if (lowerValue == falseValue) {
            *ok = true;
            return false;
        }
    }
    return false;
}

ServerConfigStruct::FreeAssignOptions stringToFreeAssign(const QString &value, bool *ok = nullptr)
{
    if (ok == nullptr) {
        static bool _ok;
        ok = &_ok;
    }

    // we should support yes and no
    *ok = false;
    bool boolValue = stringToBool(value, ok);
    if (*ok)
        return boolValue ? ServerConfigStruct::FreeAssignAll : ServerConfigStruct::FreeAssignNo;

    // special case: self

    if (value.toLower() == QStringLiteral("self")) {
        *ok = true;
        return ServerConfigStruct::FreeAssignOwner;
    }

    // Return arbitary value, since caller should always judge output variable ok
    return ServerConfigStruct::FreeAssignNo;
}

ServerConfigStruct::HegemonyRewardOptions stringToHegemonyReward(const QString &value, bool *ok = nullptr)
{
    if (ok == nullptr) {
        static bool _ok;
        ok = &_ok;
    }

    // support "yes" as instant
    *ok = false;
    bool boolValue = stringToBool(value, ok);
    if (*ok)
        return boolValue ? ServerConfigStruct::HegemonyRewardInstant : ServerConfigStruct::HegemonyRewardNone;

    // special case: postpone / postponed, instant

    static const QStringList postponeValues {
        // clang-format off
        QStringLiteral("postpone"),
        QStringLiteral("postponed"),
        // clang-format on
    };

    QString lowerValue = value.toLower();

    if (lowerValue == QStringLiteral("instant")) {
        *ok = true;
        return ServerConfigStruct::HegemonyRewardInstant;
    }

    foreach (const QString &postponeValue, postponeValues) {
        if (lowerValue == postponeValue) {
            *ok = true;
            return ServerConfigStruct::HegemonyRewardPostponed;
        }
    }

    // Return arbitary value, since caller should always judge output variable ok
    return ServerConfigStruct::HegemonyRewardNone;
}

QString freeAssignToString(ServerConfigStruct::FreeAssignOptions freeAssign)
{
    static const QStringList assignValues {
        // clang-format off
        QStringLiteral("no"),
        QStringLiteral("yes"),
        QStringLiteral("self"),
        // clang-format on
    };

    int intValue = static_cast<int>(freeAssign);
    return assignValues.value(intValue, QString());
}

QString hegemonyRewardToString(ServerConfigStruct::HegemonyRewardOptions hegemonyReward)
{
    static const QStringList rewardValues {
        // clang-format off
        QStringLiteral("none"),
        QStringLiteral("instant"),
        QStringLiteral("postponed"),
        // clang-format on
    };

    int intValue = static_cast<int>(hegemonyReward);
    return rewardValues.value(intValue, QString());
}

} // namespace

void ServerConfigStruct::defaultValues()
{
    // fill initial default values
    mode = QStringLiteral("role_2,4,1");

    network.bindIp = QHostAddress::Any;
    network.bindPort = 41392;
    network.simc = true;
    network.chat = true;

    game.timeout = 15;
    game.nullificationTimeout = 8;
    game.serverName = QStringLiteral("QSanguosha\'s Server");
    game.shuffleSeat = true;
    game.latestGeneral = true;
    game.pileSwap = 5;
    game.enableAi = true;
    game.aiDelay = 1000;
    game.aiLimit = false;
    game.modifiedAiDelay = -1;
    game.enabledPackages = QStringList();
    game.multiGenerals = 0;

    role.banLists.clear();

    cheat.enable = false;
    cheat.freeChoose = false;

    role.choiceLord = 8;
    role.choiceNonLord = 6;
    role.choiceOthers = 6;
    role.choiceGods = 1;
    role.choiceMulti = 6;
    role.lordSkill = true;
    role.cheat.freeAssign = FreeAssignNo;

    hegemony.choice = 6;
    hegemony.firstShow = HegemonyRewardNone;
    hegemony.companion = HegemonyRewardInstant;
    hegemony.halfHp = HegemonyRewardInstant;
    hegemony.careeristKillReward = 0;
}

bool ServerConfigStruct::parse()
{
    if (parsed)
        return false;

    if (readConfigFile())
        qWarning() << QStringLiteral("Config file load failed. Default configuration will be used.");

#if (defined(Q_OS_DARWIN) && !defined(Q_OS_MACOS)) || defined(Q_OS_ANDROID)
// mobile platforms don't use processes, so get command line options from it is unable
#else

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsCompactedShortOptions);

    // Note: mentioning 'QStringList' in following constructor is mandatory since QCommandLineOption has an implicit constructor which accepts 2 QStrings
    // I really don't know why C++ decides to accept constructor calls with "{}" without std::initializer_list. It sucks.

    // Common options
    parser.addVersionOption();
    // parser.addHelpOption(); // Since we use custom help text, help text for following commands are not needed, neither is this function call.
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("h"), QStringLiteral("help")})); // ... and use this function call instead, and parse ourselves.

    // Config file options
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("sc"), QStringLiteral("save-config")}));

    // Game mode options
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("m"), QStringLiteral("mode")}, QString(), QStringLiteral("mode")));

    // Network options
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("I"), QStringLiteral("bind-ip")}, QString(), QStringLiteral("ip")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("P"), QStringLiteral("bind-port")}, QString(), QStringLiteral("port")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("mc"), QStringLiteral("same-ip-with-multiple-connection")}, QString(), QStringLiteral("simc")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("c"), QStringLiteral("chat")}, QString(), QStringLiteral("chat")));

    // Game options
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("t"), QStringLiteral("timeout")}, QString(), QStringLiteral("timeout")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("N"), QStringLiteral("nullification-timeout")}, QString(), QStringLiteral("timeout")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("S"), QStringLiteral("server-name")}, QString(), QStringLiteral("name")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("s"), QStringLiteral("shuffle-seat")}, QString(), QStringLiteral("shuffle")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("l"), QStringLiteral("latest-general")}, QString(), QStringLiteral("latest")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("ps"), QStringLiteral("pile-swap-limit")}, QString(), QStringLiteral("limit")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("A"), QStringLiteral("ai")}, QString(), QStringLiteral("ai")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("Ad"), QStringLiteral("ai-delay")}, QString(), QStringLiteral("delay")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("Al"), QStringLiteral("ai-limit")}, QString(), QStringLiteral("limit")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("Am"), QStringLiteral("ai-delay-after-death")}, QString(), QStringLiteral("delay")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("p"), QStringLiteral("packages")}, QString(), QStringLiteral("packages")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("mg"), QStringLiteral("multi-generals")}, QString(), QStringLiteral("generals")));

    // Ban options
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("br"), QStringLiteral("ban-role")}, QString(), QStringLiteral("ban")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("ab"), QStringLiteral("append-banlist-from-config-file")}));

    // Cheat options
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("Z"), QStringLiteral("cheat")}, QString(), QStringLiteral("cheat")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("Zc"), QStringLiteral("free-choose")}, QString(), QStringLiteral("free")));

    // Role options
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("cl"), QStringLiteral("choice-lord")}, QString(), QStringLiteral("choice")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("cn"), QStringLiteral("choice-nonlord")}, QString(), QStringLiteral("choice")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("co"), QStringLiteral("choice-others")}, QString(), QStringLiteral("choice")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("cg"), QStringLiteral("choice-gods")}, QString(), QStringLiteral("choice")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("cm"), QStringLiteral("choice-multi")}, QString(), QStringLiteral("choice")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("ls"), QStringLiteral("lord-skill")}, QString(), QStringLiteral("lord")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("Zf"), QStringLiteral("free-assign")}, QString(), QStringLiteral("free")));

    // Hegemony options
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("ch"), QStringLiteral("choice-hegemony")}, QString(), QStringLiteral("choice")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("rf"), QStringLiteral("first-show-reward")}, QString(), QStringLiteral("reward")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("rc"), QStringLiteral("companion-reward")}, QString(), QStringLiteral("reward")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("rh"), QStringLiteral("half-hp-reward")}, QString(), QStringLiteral("reward")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("rk"), QStringLiteral("careerist-kill")}, QString(), QStringLiteral("reward")));

    // This command eats v
    parser.process(*qApp);
    // .. and we manually parse h, so common options are fully parsed for now.
    if (parser.isSet(QStringLiteral("h"))) {
        showHelp();
        qApp->exit();
        return false;
    }

    if (!parser.unknownOptionNames().isEmpty()) {
        qWarning() << (QStringLiteral("Unknown options: ") + parser.unknownOptionNames().join(QStringLiteral(", ")));
        qWarning() << (QStringLiteral("These options are ignored for now, but they may be of any use in the future. Better remove them before they take place."));
    }

    if (!parser.positionalArguments().isEmpty()) {
        qWarning() << (QStringLiteral("Unknown arguments: ") + parser.positionalArguments().join(QStringLiteral(", ")));
        qWarning() << (QStringLiteral("These options are ignored."));
    }

    QStringList parserFailures;

    // sc is processed last, after judging parserFailures!
    // since if anything fails to process, no configuration file should be generated

    // Game mode options
    if (parser.isSet(QStringLiteral("m"))) {
        mode = parser.value(QStringLiteral("m"));
        const Mode *findMode = Mode::findMode(mode);
        if (findMode == nullptr)
            parserFailures << QString(QStringLiteral("Value for --mode (%1) is incorrect. Nonexistant mode %1 is specified. Check your input.")).arg(mode);
    }

    // Network options
    if (parser.isSet(QStringLiteral("I"))) {
        QString I = parser.value(QStringLiteral("I"));
        QHostAddress ha;
        if (ha.setAddress(I))
            network.bindIp = ha;
        else
            parserFailures << QString(QStringLiteral("Value for --bind-ip (%1) is incorrect. Check your input.")).arg(I);
    }
    if (parser.isSet(QStringLiteral("P"))) {
        // 1024 ~ 49151
        QString P = parser.value(QStringLiteral("P"));
        bool ok = false;
        network.bindPort = P.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --bind-port (%1) is incorrect. Check your input.")).arg(P);
        else if (network.bindPort < 1024)
            parserFailures << QString(QStringLiteral("Value for --bind-port (%1) is too small. Please use a number between 1024 and 49151.")).arg(P);
        else if (network.bindPort > 49151)
            parserFailures << QString(QStringLiteral("Value for --bind-port (%1) is too big. Please use a number between 1024 and 49151.")).arg(P);
    }
    if (parser.isSet(QStringLiteral("mc"))) {
        QString mc = parser.value(QStringLiteral("mc"));
        bool ok = false;
        network.simc = stringToBool(mc, &ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --same-ip-with-multiple-connection (%1) is incorrect. Check your input.")).arg(mc);
    }
    if (parser.isSet(QStringLiteral("c"))) {
        QString c = parser.value(QStringLiteral("c"));
        bool ok = false;
        network.chat = stringToBool(c, &ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --chat (%1) is incorrect. Check your input.")).arg(c);
    }

    // Game options
    if (parser.isSet(QStringLiteral("t"))) {
        QString t = parser.value(QStringLiteral("t"));
        bool ok = false;
        game.timeout = t.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --timeout (%1) is incorrect. Check your input.")).arg(t);
        else if (game.timeout < 5 && game.timeout != 0)
            parserFailures << QString(QStringLiteral("Value for --timeout (%1) is too small. Please use a number between 5 and 60 (or 0 for infinity).")).arg(t);
        else if (game.timeout > 60)
            parserFailures << QString(QStringLiteral("Value for --timeout (%1) is too big. Please use a number between 5 and 60 (or 0 for infinity).")).arg(t);
    }
    if (parser.isSet(QStringLiteral("N"))) {
        QString N = parser.value(QStringLiteral("N"));
        bool ok = false;
        game.nullificationTimeout = N.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --nullification-timeout (%1) is incorrect. Check your input.")).arg(N);
        else if (game.nullificationTimeout < 5)
            parserFailures << QString(QStringLiteral("Value for --nullification-timeout (%1) is too small. Please use a number between 5 and 15.")).arg(N);
        else if (game.nullificationTimeout > 15)
            parserFailures << QString(QStringLiteral("Value for --nullification-timeout (%1) is too big. Please use a number between 5 and 15.")).arg(N);
    }
    if (parser.isSet(QStringLiteral("S")))
        game.serverName = parser.value(QStringLiteral("S"));
    if (parser.isSet(QStringLiteral("ps"))) {
        QString ps = parser.value(QStringLiteral("ps"));
        bool ok = false;
        game.pileSwap = ps.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --pile-swap-limit (%1) is incorrect. Check your input.")).arg(ps);
        else if (game.pileSwap < 0)
            parserFailures << QString(QStringLiteral("Value for --pile-swap-limit (%1) is too small. Please use a number between 1 and 15 (or 0 for infinity).")).arg(ps);
        else if (game.pileSwap > 15)
            parserFailures << QString(QStringLiteral("Value for --pile-swap-limit (%1) is too big. Please use a number between 1 and 15 (or 0 for infinity).")).arg(ps);
    }
    if (parser.isSet(QStringLiteral("A"))) {
        QString A = parser.value(QStringLiteral("A"));
        bool ok = false;
        game.enableAi = stringToBool(A, &ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --ai (%1) is incorrect. Check your input.")).arg(A);
    }
    if (parser.isSet(QStringLiteral("Ad"))) {
        QString Ad = parser.value(QStringLiteral("Ad"));
        bool ok = false;
        game.aiDelay = Ad.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --ai-delay (%1) is incorrect. Check your input.")).arg(Ad);
        else if (game.aiDelay < 0)
            parserFailures << QString(QStringLiteral("Value for --ai-delay (%1) is too small. Please use a number between 0 and 5000.")).arg(Ad);
        else if (game.aiDelay > 5000)
            parserFailures << QString(QStringLiteral("Value for --ai-delay (%1) is too big. Please use a number between 0 and 5000.")).arg(Ad);
    }
    if (parser.isSet(QStringLiteral("Al"))) {
        QString Al = parser.value(QStringLiteral("Al"));
        bool ok = false;
        network.chat = stringToBool(Al, &ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --ai-limit (%1) is incorrect. Check your input.")).arg(Al);
    }
    if (parser.isSet(QStringLiteral("Am"))) {
        QString Am = parser.value(QStringLiteral("Am"));
        bool ok = false;
        game.modifiedAiDelay = Am.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --ai-delay-after-death (%1) is incorrect. Check your input.")).arg(Am);
        else if (game.modifiedAiDelay < 0)
            parserFailures << QString(QStringLiteral("Value for --ai-delay-after-death (%1) is too small. Please use a number between 0 and 5000.")).arg(Am);
        else if (game.modifiedAiDelay > 5000)
            parserFailures << QString(QStringLiteral("Value for --ai-delay-after-death (%1) is too big. Please use a number between 0 and 5000.")).arg(Am);
    }
    if (parser.isSet(QStringLiteral("p"))) {
        QString p = parser.value(QStringLiteral("p"));
        // TODO
        Q_UNUSED(p);
    }
    if (parser.isSet(QStringLiteral("mg"))) {
        QString mg = parser.value(QStringLiteral("mg"));
        bool ok = false;
        game.multiGenerals = mg.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --multi-generals (%1) is incorrect. Check your input.")).arg(mg);
        else if (game.multiGenerals < 0)
            parserFailures << QString(QStringLiteral("Value for --multi-generals (%1) is too small. Please use a number bigger than 0 (with 0 as mode default).")).arg(mg);
    }

    // banlist - role
    if (parser.isSet(QStringLiteral("br"))) {
        if (!parser.isSet(QStringLiteral("ab")))
            role.banLists.clear();

        // TODO
    } else if (parser.isSet(QStringLiteral("ab"))) {
        parserFailures << QStringLiteral("--append-banlist-from-config-file can only be used when --ban-role has been specified.");
    }

    // cheat options
    if (parser.isSet(QStringLiteral("Z"))) {
        QString Z = parser.value(QStringLiteral("Z"));
        bool ok = false;
        cheat.enable = stringToBool(Z, &ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --cheat (%1) is incorrect. Check your input.")).arg(Z);
    }
    if (parser.isSet(QStringLiteral("Zc"))) {
        if (!cheat.enable) {
            parserFailures << QStringLiteral("--free-choose can only be used when --cheat has been enabled.");
        } else {
            QString Zc = parser.value(QStringLiteral("Zc"));
            bool ok = false;
            cheat.freeChoose = stringToBool(Zc, &ok);
            if (!ok)
                parserFailures << QString(QStringLiteral("Value for --free-choose (%1) is incorrect. Check your input.")).arg(Zc);
        }
    }

    // Role options
    if (parser.isSet(QStringLiteral("cl"))) {
        QString cl = parser.value(QStringLiteral("cl"));
        bool ok = false;
        role.choiceLord = cl.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --choice-lord (%1) is incorrect. Check your input.")).arg(cl);
        else if (role.choiceLord < 0)
            parserFailures << QString(QStringLiteral("Value for --choice-lord (%1) is too small. Please use a number bigger than 1 (or 0 for all lords).")).arg(cl);
    }
    if (parser.isSet(QStringLiteral("cn"))) {
        QString cn = parser.value(QStringLiteral("cn"));
        bool ok = false;
        role.choiceNonLord = cn.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --choice-nonlord (%1) is incorrect. Check your input.")).arg(cn);
        else if (role.choiceNonLord < 1)
            parserFailures << QString(QStringLiteral("Value for --choice-nonlord (%1) is too small. Please use a number bigger than 1.")).arg(cn);
    }
    if (parser.isSet(QStringLiteral("co"))) {
        QString co = parser.value(QStringLiteral("co"));
        bool ok = false;
        role.choiceOthers = co.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --choice-others (%1) is incorrect. Check your input.")).arg(co);
        else if (role.choiceOthers < 1)
            parserFailures << QString(QStringLiteral("Value for --choice-others (%1) is too small. Please use a number bigger than 1.")).arg(co);
    }
    if (parser.isSet(QStringLiteral("cg"))) {
        QString cg = parser.value(QStringLiteral("cg"));
        bool ok = false;
        role.choiceGods = cg.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --choice-gods (%1) is incorrect. Check your input.")).arg(cg);
        else if (role.choiceGods < 0)
            parserFailures << QString(QStringLiteral("Value for --choice-gods (%1) is too small. Please use a number bigger than 0.")).arg(cg);
    }
    if (parser.isSet(QStringLiteral("cm"))) {
        QString cm = parser.value(QStringLiteral("cm"));
        bool ok = false;
        role.choiceMulti = cm.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --choice-multi (%1) is incorrect. Check your input.")).arg(cm);
        else if (role.choiceMulti < 1)
            parserFailures << QString(QStringLiteral("Value for --choice-multi (%1) is too small. Please use a number bigger than 1.")).arg(cm);
    }
    if (parser.isSet(QStringLiteral("ls"))) {
        QString ls = parser.value(QStringLiteral("ls"));
        bool ok = false;
        role.lordSkill = stringToBool(ls, &ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --lord-skill (%1) is incorrect. Check your input.")).arg(ls);
    }
    if (parser.isSet(QStringLiteral("Zf"))) {
        if (!cheat.enable) {
            parserFailures << QStringLiteral("--free-assign can only be used when --cheat has been enabled.");
        } else {
            QString Zf = parser.value(QStringLiteral("Zf"));
            bool ok = false;
            role.cheat.freeAssign = stringToFreeAssign(Zf, &ok);
            if (!ok)
                parserFailures << QString(QStringLiteral("Value for --free-assign (%1) is incorrect. Check your input.")).arg(Zf);
        }
    }

    // Hegemony options
    if (parser.isSet(QStringLiteral("ch"))) {
        QString ch = parser.value(QStringLiteral("ch"));
        bool ok = false;
        hegemony.choice = ch.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --choice-hegemony (%1) is incorrect. Check your input.")).arg(ch);
        else if (hegemony.choice < 1)
            parserFailures << QString(QStringLiteral("Value for --choice-hegemony (%1) is too small. Please use a number bigger than 1.")).arg(ch);
    }
    if (parser.isSet(QStringLiteral("rf"))) {
        QString rf = parser.value(QStringLiteral("rf"));
        bool ok = false;
        hegemony.firstShow = stringToHegemonyReward(rf, &ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --first-show-reward (%1) is incorrect. Check your input.")).arg(rf);
    }
    if (parser.isSet(QStringLiteral("rc"))) {
        QString rc = parser.value(QStringLiteral("rc"));
        bool ok = false;
        hegemony.companion = stringToHegemonyReward(rc, &ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --companion-reward (%1) is incorrect. Check your input.")).arg(rc);
    }
    if (parser.isSet(QStringLiteral("rh"))) {
        QString rh = parser.value(QStringLiteral("rh"));
        bool ok = false;
        hegemony.halfHp = stringToHegemonyReward(rh, &ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --half-hp-reward (%1) is incorrect. Check your input.")).arg(rh);
    }
    if (parser.isSet(QStringLiteral("rk"))) {
        QString rk = parser.value(QStringLiteral("rk"));
        bool ok = false;
        hegemony.careeristKillReward = rk.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --careerist-kill (%1) is incorrect. Check your input.")).arg(rk);
        else if (hegemony.choice < 0)
            parserFailures << QString(QStringLiteral("Value for --careerist-kill (%1) is too small. Please use a number bigger than 1 "
                                                     "(or 0 for default, which is same as official rule."))
                                  .arg(rk);
    }

    if (!parserFailures.isEmpty()) {
        foreach (const QString &f, parserFailures)
            qWarning() << f;

        return false;
    }

    if (parser.isSet(QStringLiteral("sc"))) {
        if (!saveConfigFile())
            qWarning() << QStringLiteral("Save config file failed. Won't save it. Program will still run.");
    }

#endif

    return true;
}

bool ServerConfigStruct::readConfigFile()
{
    QFile f(configFilePath());
    if (!f.exists()) {
        // special case: if file don't exist, exit successfully.
        return true;
    }

    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << QStringLiteral("Open config file failed.");
        return false;
    }

    QByteArray arr = f.readAll();
    f.close();

    QJsonParseError err;
    QJsonDocument document = QJsonDocument::fromJson(arr, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << QString(QStringLiteral("Read json from config file failed. Reason: %1")).arg(err.errorString());
        return false;
    }

    // TODO: parse document

    return true;
}

bool ServerConfigStruct::saveConfigFile()
{
    QFile f(configFilePath());

    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << QStringLiteral("Open config file failed.");
        return false;
    }

    // TODO: build up JSON document

    f.close();

    return true;
}
