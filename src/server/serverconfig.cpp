
#include "serverconfig.h"
#include "engine.h"
#include "jsonutils.h"
#include "mode.h"
#include "util.h"

#include <QDebug>
#include <QDir>
#include <QGlobalStatic>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <iostream>

#if (defined(Q_OS_DARWIN) && !defined(Q_OS_MACOS)) || defined(Q_OS_ANDROID) || defined(Q_OS_WASM)
// mobile platforms don't use processes, so command line option shouldn't be get
// always use server config file for it
#else
#ifdef Q_OS_UNIX
#include <unistd.h>
#endif
#include <QCommandLineOption>
#include <QCommandLineParser>
#endif

Q_GLOBAL_STATIC(ServerConfigStruct, ServerConfigInstance)

namespace {
const QString &configFilePath()
{
    static QString p;
    if (p.isEmpty()) {
#if (defined(Q_OS_DARWIN) && !defined(Q_OS_MACOS)) || defined(Q_OS_ANDROID) || defined(Q_OS_WASM)
#if defined(Q_OS_ANDROID)
        p = QStringLiteral("/sdcard/Android/data/rocks.touhousatsu.app/serverconfig.json");
#elif defined(Q_OS_WASM)
        p = QStringLiteral("/serverconfig.json");
#else
        // wait for anyone who's formaliar with iOS / watchOS / tvOS
#endif
#else
#if defined(Q_OS_UNIX)
        if (getuid() == (uid_t)0)
            p = QStringLiteral("/etc/QSanguosha/serverconfig.json");
        else
#endif
            p = QDir::homePath() + QDir::separator() + QStringLiteral(".QSanguosha") + QDir::separator() + QStringLiteral("serverconfig.json");
#endif
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
--sc, --save-config save the option to config file after parsing it SUCCESSFULLY. Do nothing when parser erros out. Make sure you have enough permission to operate it. Defaults no.

Game mode options:
-m, --mode=<role_x,x,x, hegemony_x> the game mode which the server serves. It can be specified multiple times for Lobby serving.

TCP Server options:
-I, --bind-ip= bind ip address, default 0.0.0.0
-P, --bind-port= bind port, default 41392
--mc=, --same-ip-with-multiple-connection=<yes, no> same ip with multiple connection

Game options:
-t, --timeout=<0,5~60> operation timeout, set 0 for no limit, default 15
-N, --nullification-timeout=<5~15> nullification timeout, default 8
-S, --server-name= the server name, default "QSanguosha's Server"
-s, --shuffle-seat=<yes, no> arange seats randomly, default yes
-l, --latest-general=<yes, no> assign latest general, default yes
--ps=, --pile-swap-limit=<0~15> pile swapping limit, set 0 for no limit
-c, --chat=<yes, no> enables or disables chat
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

QHostAddress stringToQHostAddress(const QString &value, bool *ok = nullptr)
{
    if (ok == nullptr) {
        static bool _ok;
        ok = &_ok;
    }
    *ok = false;

    static const QStringList predefinedValues {
        // clang-format off
        QStringLiteral("null"),
        QStringLiteral("broadcast"),
        QStringLiteral("localhost"),
        QStringLiteral("localhostipv6"),
        QStringLiteral("any"),
        QStringLiteral("anyipv6"),
        QStringLiteral("anyipv4"),
        // clang-format on
    };

    QString lowerValue = value.toLower();

    for (int i = 0; i < predefinedValues.length(); ++i) {
        if (lowerValue == predefinedValues.value(i)) {
            *ok = true;
            return static_cast<QHostAddress::SpecialAddress>(i);
        }
    }

    QHostAddress ha;
    *ok = ha.setAddress(value);

    return ha;
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

QString qHostAddressToString(const QHostAddress &address)
{
    if (address.isEqual(QHostAddress::Any, QHostAddress::TolerantConversion))
        return QStringLiteral("any");
    return address.toString();
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

ServerConfigStruct::ServerConfigStruct()
    : parsed(false)
{
    defaultValues();
}

void ServerConfigStruct::defaultValues()
{
    // fill initial default values
    modesServing.clear();
    modesServing << QStringLiteral("role_2,4,1");

    tcpServer.bindIp = QHostAddress::Any;
    tcpServer.bindPort = 41392;
    tcpServer.simc = true;

    game.timeout = 15;
    game.nullificationTimeout = 8;
    game.serverName = QStringLiteral("QSanguosha\'s Server");
    game.shuffleSeat = true;
    game.latestGeneral = true;
    game.pileSwap = 5;
    game.chat = true;
    game.enableAi = true;
    game.aiDelay = 1000;
    game.aiLimit = false;
    game.modifiedAiDelay = -1;
    game.enabledPackages.clear();
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

#if (defined(Q_OS_DARWIN) && !defined(Q_OS_MACOS)) || defined(Q_OS_ANDROID) || defined(Q_OS_WASM)
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

    // TCP Server options
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("I"), QStringLiteral("bind-ip")}, QString(), QStringLiteral("ip")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("P"), QStringLiteral("bind-port")}, QString(), QStringLiteral("port")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("mc"), QStringLiteral("same-ip-with-multiple-connection")}, QString(), QStringLiteral("simc")));

    // Game options
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("t"), QStringLiteral("timeout")}, QString(), QStringLiteral("timeout")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("N"), QStringLiteral("nullification-timeout")}, QString(), QStringLiteral("timeout")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("S"), QStringLiteral("server-name")}, QString(), QStringLiteral("name")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("s"), QStringLiteral("shuffle-seat")}, QString(), QStringLiteral("shuffle")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("l"), QStringLiteral("latest-general")}, QString(), QStringLiteral("latest")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("ps"), QStringLiteral("pile-swap-limit")}, QString(), QStringLiteral("limit")));
    parser.addOption(QCommandLineOption(QStringList {QStringLiteral("c"), QStringLiteral("chat")}, QString(), QStringLiteral("chat")));
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

    if (!parser.positionalArguments().isEmpty()) {
        qWarning() << (QStringLiteral("Unknown arguments: ") + parser.positionalArguments().join(QStringLiteral(", ")));
        qWarning() << (QStringLiteral("These options are ignored."));
    }

    QStringList parserFailures;

    if (!parser.unknownOptionNames().isEmpty())
        parserFailures << (QStringLiteral("Unknown options: ") + parser.unknownOptionNames().join(QStringLiteral(", ")));

    // sc is processed last, after judging parserFailures!
    // since if anything fails to process, no configuration file should be generated

    // Game mode options
    if (parser.isSet(QStringLiteral("m"))) {
        modesServing = parser.values(QStringLiteral("m"));
        foreach (const QString &mode, modesServing) {
            const Mode *findMode = Sanguosha->gameMode(mode);
            if (findMode == nullptr)
                parserFailures << QString(QStringLiteral("Value for --mode (%1) is incorrect. Nonexistant mode %1 is specified. Check your input.")).arg(mode);
        }
    }

    // TCP Server options
    if (parser.isSet(QStringLiteral("I"))) {
        QString I = parser.value(QStringLiteral("I"));
        bool ok = false;
        tcpServer.bindIp = stringToQHostAddress(I, &ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --bind-ip (%1) is incorrect. Check your input.")).arg(I);
    }
    if (parser.isSet(QStringLiteral("P"))) {
        // 1024 ~ 49151
        QString P = parser.value(QStringLiteral("P"));
        bool ok = false;
        tcpServer.bindPort = P.toInt(&ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --bind-port (%1) is incorrect. Check your input.")).arg(P);
        else if (tcpServer.bindPort < 1024)
            parserFailures << QString(QStringLiteral("Value for --bind-port (%1) is too small. Please use a number between 1024 and 49151.")).arg(P);
        else if (tcpServer.bindPort > 49151)
            parserFailures << QString(QStringLiteral("Value for --bind-port (%1) is too big. Please use a number between 1024 and 49151.")).arg(P);
    }
    if (parser.isSet(QStringLiteral("mc"))) {
        QString mc = parser.value(QStringLiteral("mc"));
        bool ok = false;
        tcpServer.simc = stringToBool(mc, &ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --same-ip-with-multiple-connection (%1) is incorrect. Check your input.")).arg(mc);
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
    if (parser.isSet(QStringLiteral("c"))) {
        QString c = parser.value(QStringLiteral("c"));
        bool ok = false;
        game.chat = stringToBool(c, &ok);
        if (!ok)
            parserFailures << QString(QStringLiteral("Value for --chat (%1) is incorrect. Check your input.")).arg(c);
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
        game.chat = stringToBool(Al, &ok);
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

        bool disabled = false;
        static QStringList disabledValues {
            QStringLiteral("disable:"),
            QStringLiteral("disabled:"),
            QStringLiteral("ban:"),
            QStringLiteral("banned:"),
        };

        foreach (const QString &disabledValue, disabledValues) {
            if (p.startsWith(disabledValue)) {
                disabled = true;
                p = p.mid(disabledValue.length());
                break;
            }
        }

        QStringList packages = p.split(QStringLiteral(","), Qt::SkipEmptyParts);
        QStringList temp;
        foreach (const QString &ps, packages)
            temp << ps.trimmed();

        packages = temp;

        QSet<QString> totalNames = Sanguosha->packageNames();
        temp.clear();

        foreach (const QString &ps, packages) {
            if (!totalNames.contains(ps))
                parserFailures << QString(QStringLiteral("Value for --package (%1) is inexistant. Check your input.")).arg(ps);
            else
                temp << ps;
        }

        if (disabled) {
            totalNames.subtract(List2Set(temp));
            game.enabledPackages = totalNames.values();
        } else {
            game.enabledPackages = temp;
        }
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

        QStringList br = parser.values(QStringLiteral("br"));
        foreach (const QString &brEach, br) {
            QStringList brEachList = brEach.split(QStringLiteral(","));
            if (brEachList.length() == 2) {
                QString generalName = brEachList.first();
                bool ok = false;
                int banFor = brEachList.last().toInt(&ok);
                if (!ok)
                    parserFailures << QString(QStringLiteral("Value for --ban-role (%1) is incorrect. Check your input.")).arg(brEach);
                else
                    role.banLists << qMakePair(generalName.trimmed(), banFor);
            } else {
                parserFailures << QString(QStringLiteral("Value for --ban-role (%1) is incorrect. Check your input.")).arg(brEach);
            }
        }
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

    if (!document.isObject()) {
        qWarning() << QString(QStringLiteral("Read json from config file failed. Config file is not json object"));
        return false;
    }

    // use default value if not existant, ignore values which are not recognized or can't be converted
    QJsonObject theOb = document.object();
    if (theOb.contains(QStringLiteral("GameModeOptions"))) {
        QJsonValue theValue = theOb.value(QStringLiteral("GameModeOptions"));
        QJsonObject theOb = theValue.toObject();
        if (theOb.contains(QStringLiteral("modes"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("modes"));
            if (QSgsJsonUtils::isStringArray(theValue))
                modesServing = QSgsJsonUtils::toStringList(theValue);
        }
    }

    if (theOb.contains(QStringLiteral("TcpServerOptions"))) {
        QJsonValue theValue = theOb.value(QStringLiteral("TcpServerOptions"));
        QJsonObject theOb = theValue.toObject();
        if (theOb.contains(QStringLiteral("bind-ip"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("bind-ip"));
            if (theValue.isString()) {
                bool ok = false;
                QHostAddress ha = stringToQHostAddress(theValue.toString(), &ok);
                if (ok)
                    tcpServer.bindIp = ha;
            }
        }
        if (theOb.contains(QStringLiteral("bind-port"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("bind-port"));
            if (theValue.isDouble())
                tcpServer.bindPort = theValue.toInt();
        }
        if (theOb.contains(QStringLiteral("same-ip-with-multiple-connection"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("same-ip-with-multiple-connection"));
            if (theValue.isBool())
                tcpServer.simc = theValue.toBool();
        }
    }

    if (theOb.contains(QStringLiteral("GameOptions"))) {
        QJsonValue theValue = theOb.value(QStringLiteral("GameOptions"));
        QJsonObject theOb = theValue.toObject();
        if (theOb.contains(QStringLiteral("timeout"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("timeout"));
            if (theValue.isDouble())
                game.timeout = theValue.toInt();
        }
        if (theOb.contains(QStringLiteral("nullification-timeout"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("nullification-timeout"));
            if (theValue.isDouble())
                game.nullificationTimeout = theValue.toInt();
        }
        if (theOb.contains(QStringLiteral("server-name"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("server-name"));
            if (theValue.isString())
                game.serverName = theValue.toString();
        }
        if (theOb.contains(QStringLiteral("shuffle-seat"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("shuffle-seat"));
            if (theValue.isBool())
                game.shuffleSeat = theValue.toBool();
        }
        if (theOb.contains(QStringLiteral("latest-general"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("latest-general"));
            if (theValue.isBool())
                game.latestGeneral = theValue.toBool();
        }
        if (theOb.contains(QStringLiteral("pile-swap-limit"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("pile-swap-limit"));
            if (theValue.isDouble())
                game.pileSwap = theValue.toInt();
        }
        if (theOb.contains(QStringLiteral("chat"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("chat"));
            if (theValue.isBool())
                game.chat = theValue.toBool();
        }
        if (theOb.contains(QStringLiteral("ai"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("ai"));
            if (theValue.isBool())
                game.enableAi = theValue.toBool();
        }
        if (theOb.contains(QStringLiteral("ai-delay"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("ai-delay"));
            if (theValue.isDouble())
                game.aiDelay = theValue.toInt();
        }
        if (theOb.contains(QStringLiteral("ai-limit"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("ai-limit"));
            if (theValue.isBool())
                game.aiLimit = theValue.toBool();
        }
        if (theOb.contains(QStringLiteral("ai-delay-after-death"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("ai-delay-after-death"));
            if (theValue.isDouble())
                game.modifiedAiDelay = theValue.toInt();
        }
        if (theOb.contains(QStringLiteral("packages"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("packages"));
            if (theValue.isArray()) {
                QJsonArray theArr = theValue.toArray();
                game.enabledPackages.clear();
                // 'foreach' can't be used here! Since Qt 6 supports using 'foreach' only on implicit shared containers
                // It uses a template to check it, and causes compile error if there is no 'detach' function in the container
                for (const QJsonValue &theValue : theArr) {
                    if (theValue.isString())
                        game.enabledPackages << theValue.toString();
                }
            }
        }
        if (theOb.contains(QStringLiteral("multi-generals"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("multi-generals"));
            if (theValue.isDouble())
                game.multiGenerals = theValue.toInt();
        }
    }

    if (theOb.contains(QStringLiteral("BanOptions"))) {
        QJsonValue theValue = theOb.value(QStringLiteral("BanOptions"));
        QJsonObject theOb = theValue.toObject();
        if (theOb.contains(QStringLiteral("ban-role"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("ban-role"));
            if (theValue.isArray()) {
                QJsonArray theArr = theValue.toArray();
                role.banLists.clear();
                // 'foreach' can't be used here! Since Qt 6 supports using 'foreach' only on implicit shared containers
                // It uses a template to check it, and causes compile error if there is no 'detach' function in the container
                for (const QJsonValue &theValue : theArr) {
                    if (theValue.isObject()) {
                        QJsonObject theOb = theValue.toObject();
                        QString name;
                        int banFor = 0;
                        bool flag = false;
                        if (theOb.contains(QStringLiteral("name")) && theOb.contains(QStringLiteral("for"))) {
                            bool flag1 = false;
                            {
                                QJsonValue theValue = theOb.value(QStringLiteral("name"));
                                if (theValue.isString()) {
                                    name = theValue.toString();
                                    flag1 = true;
                                }
                            }
                            {
                                QJsonValue theValue = theOb.value(QStringLiteral("for"));
                                if (theValue.isDouble()) {
                                    banFor = theValue.toInt();
                                    if (flag1)
                                        flag = true;
                                }
                            }
                        }
                        if (flag)
                            role.banLists << qMakePair(name, banFor);
                    }
                }
            }
        }
    }

    if (theOb.contains(QStringLiteral("CheatOptions"))) {
        QJsonValue theValue = theOb.value(QStringLiteral("CheatOptions"));
        QJsonObject theOb = theValue.toObject();
        if (theOb.contains(QStringLiteral("chaat"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("cheat"));
            if (theValue.isBool())
                cheat.enable = theValue.toBool();
        }
        if (theOb.contains(QStringLiteral("free-choose"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("free-choose"));
            if (theValue.isBool())
                cheat.freeChoose = theValue.toBool();
        }
        if (theOb.contains(QStringLiteral("Role"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("Role"));
            QJsonObject theOb = theValue.toObject();
            if (theOb.contains(QStringLiteral("free-assign"))) {
                QJsonValue theValue = theOb.value(QStringLiteral("free-assign"));
                if (theValue.isString()) {
                    bool ok = false;
                    FreeAssignOptions assign = stringToFreeAssign(theValue.toString(), &ok);
                    if (ok)
                        role.cheat.freeAssign = assign;
                }
            }
        }
    }

    if (theOb.contains(QStringLiteral("Role"))) {
        QJsonValue theValue = theOb.value(QStringLiteral("Role"));
        QJsonObject theOb = theValue.toObject();
        if (theOb.contains(QStringLiteral("choice-lord"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("choice-lord"));
            if (theValue.isDouble())
                role.choiceLord = theValue.toInt();
        }
        if (theOb.contains(QStringLiteral("choice-nonlord"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("choice-nonlord"));
            if (theValue.isDouble())
                role.choiceNonLord = theValue.toInt();
        }
        if (theOb.contains(QStringLiteral("choice-others"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("choice-others"));
            if (theValue.isDouble())
                role.choiceOthers = theValue.toInt();
        }
        if (theOb.contains(QStringLiteral("choice-gods"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("choice-gods"));
            if (theValue.isDouble())
                role.choiceGods = theValue.toInt();
        }
        if (theOb.contains(QStringLiteral("lord-skill"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("lord-skill"));
            if (theValue.isBool())
                role.lordSkill = theValue.toBool();
        }
        if (theOb.contains(QStringLiteral("choice-multi"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("choice-muiti"));
            if (theValue.isDouble())
                role.choiceMulti = theValue.toInt();
        }
    }

    if (theOb.contains(QStringLiteral("Hegemony"))) {
        QJsonValue theValue = theOb.value(QStringLiteral("Hegemony"));
        QJsonObject theOb = theValue.toObject();
        if (theOb.contains(QStringLiteral("choice-hegemony"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("choice-hegemony"));
            if (theValue.isDouble())
                hegemony.choice = theValue.toInt();
        }
        if (theOb.contains(QStringLiteral("first-show-reward"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("first-show-reward"));
            if (theValue.isString()) {
                bool ok = false;
                HegemonyRewardOptions firstShow = stringToHegemonyReward(theValue.toString(), &ok);
                if (ok)
                    hegemony.firstShow = firstShow;
            }
        }
        if (theOb.contains(QStringLiteral("companion-reward"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("companion-reward"));
            if (theValue.isString()) {
                bool ok = false;
                HegemonyRewardOptions companion = stringToHegemonyReward(theValue.toString(), &ok);
                if (ok)
                    hegemony.companion = companion;
            }
        }
        if (theOb.contains(QStringLiteral("half-hp-reward"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("half-hp-reward"));
            if (theValue.isString()) {
                bool ok = false;
                HegemonyRewardOptions halfHp = stringToHegemonyReward(theValue.toString(), &ok);
                if (ok)
                    hegemony.halfHp = halfHp;
            }
        }
        if (theOb.contains(QStringLiteral("careerist-kill"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("careerist-kill"));
            if (theValue.isDouble())
                hegemony.careeristKillReward = theValue.toInt();
        }
    }

    return true;
}

bool ServerConfigStruct::saveConfigFile()
{
    QFile f(configFilePath());

    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << QStringLiteral("Open config file failed.");
        return false;
    }

    QJsonObject theOb;
    {
        QJsonObject GameModeOptions;
        {
            GameModeOptions[QStringLiteral("modes")] = QSgsJsonUtils::toJsonArray(modesServing);
        }
        theOb[QStringLiteral("GameModeOptions")] = GameModeOptions;
    }
    {
        QJsonObject TcpServerOptions;
        {
            TcpServerOptions[QStringLiteral("bind-ip")] = qHostAddressToString(tcpServer.bindIp);
            TcpServerOptions[QStringLiteral("bind-port")] = (int)(tcpServer.bindPort);
            TcpServerOptions[QStringLiteral("same-ip-with-multiple-connection")] = tcpServer.simc;
        }
        theOb[QStringLiteral("TcpServerOptions")] = TcpServerOptions;
    }
    {
        QJsonObject GameOptions;
        {
            GameOptions[QStringLiteral("timeout")] = game.timeout;
            GameOptions[QStringLiteral("nullification-timeout")] = game.nullificationTimeout;
            GameOptions[QStringLiteral("server-name")] = game.serverName;
            GameOptions[QStringLiteral("shuffle-seat")] = game.shuffleSeat;
            GameOptions[QStringLiteral("latest-general")] = game.latestGeneral;
            GameOptions[QStringLiteral("pile-swap-limit")] = game.pileSwap;
            GameOptions[QStringLiteral("chat")] = game.chat;
            GameOptions[QStringLiteral("ai")] = game.enableAi;
            GameOptions[QStringLiteral("ai-delay")] = game.aiDelay;
            GameOptions[QStringLiteral("ai-limit")] = game.aiLimit;
            GameOptions[QStringLiteral("ai-delay-after-death")] = game.modifiedAiDelay;
            {
                QJsonArray enabledPackages;
                foreach (const QString &package, game.enabledPackages)
                    enabledPackages.append(package);
                GameOptions[QStringLiteral("packages")] = enabledPackages;
            }
            GameOptions[QStringLiteral("multi-generals")] = game.multiGenerals;
        }
        theOb[QStringLiteral("GameOptions")] = GameOptions;
    }
    {
        QJsonObject BanOptions;
        {
            QJsonArray banRole;
            // I am against using auto everywhere since major C++ coding standard don't suggest including type information in variable name.
            // This makes investigating code much more difficult since sometimes type of a variable matters for checking its member functions, variables, and so on of the specific type.
            // Even if deduction of auto is fully specified in C++ standard, I don't like using it since there is much code rely on implicit conversion of C++ (as you already seen here, where almost everything is implicitly converted to QJsonValue)
            // where auto simply can't do implicit conversion. So I only use auto for where the type name doesn't fit.

            // Typename of following "auto" is "QPair<QString, int>", which contains ',' and can't be used in MACRO foreach.
            foreach (const auto &pair, role.banLists) {
                QJsonObject ob;
                ob[QStringLiteral("name")] = pair.first;
                ob[QStringLiteral("for")] = pair.second;
                banRole.append(ob);
            }
            BanOptions[QStringLiteral("ban-role")] = banRole;
        }
        theOb[QStringLiteral("BanOptions")] = BanOptions;
    }
    {
        QJsonObject CheatOptions;
        {
            CheatOptions[QStringLiteral("cheat")] = cheat.enable;
            CheatOptions[QStringLiteral("free-choose")] = cheat.freeChoose;
            {
                QJsonObject Role;
                {
                    Role[QStringLiteral("free-assign")] = freeAssignToString(role.cheat.freeAssign);
                }
                CheatOptions[QStringLiteral("Role")] = Role;
            }
        }
        theOb[QStringLiteral("CheatOptions")] = CheatOptions;
    }
    {
        QJsonObject Role;
        {
            Role[QStringLiteral("choice-lord")] = role.choiceLord;
            Role[QStringLiteral("choice-nonlord")] = role.choiceNonLord;
            Role[QStringLiteral("choice-others")] = role.choiceOthers;
            Role[QStringLiteral("choice-gods")] = role.choiceGods;
            Role[QStringLiteral("lord-skill")] = role.lordSkill;
            Role[QStringLiteral("choice-multi")] = role.choiceMulti;
        }

        theOb[QStringLiteral("Role")] = Role;
    }
    {
        QJsonObject Hegemony;
        {
            Hegemony[QStringLiteral("choice-hegemony")] = hegemony.choice;
            Hegemony[QStringLiteral("first-show-reward")] = hegemonyRewardToString(hegemony.firstShow);
            Hegemony[QStringLiteral("companion-reward")] = hegemonyRewardToString(hegemony.companion);
            Hegemony[QStringLiteral("half-hp-reward")] = hegemonyRewardToString(hegemony.halfHp);
            Hegemony[QStringLiteral("careerist-kill")] = hegemony.careeristKillReward;
        }
        theOb[QStringLiteral("Hegemony")] = Hegemony;
    }

    QJsonDocument document(theOb);

    QByteArray arr = document.toJson(QJsonDocument::Indented);
    if (f.write(arr) == arr.length()) {
        f.close();
        return true;
    }

    f.close();

    return false;
}

ServerInfoStruct ServerConfigStruct::toServerInfo(const QString &modeName) const
{
    const Mode *mode = Sanguosha->gameMode(modeName);

    ServerInfoStruct info;
    if (mode == nullptr)
        return info;

    info.Name = game.serverName;
    info.GameMode = mode;
    info.GameModeStr = modeName;
    info.GameRuleMode.clear(); // TODO info.GameRuleMode
    info.OperationTimeout = game.timeout;
    info.NullificationCountDown = game.nullificationTimeout;
    info.EnabledPackages = game.enabledPackages;
    info.RandomSeat = game.shuffleSeat;
    info.EnableCheat = cheat.enable;
    info.FreeChoose = cheat.freeChoose;
    info.GeneralsPerPlayer = game.multiGenerals;
    info.EnableAI = game.enableAi;
    info.DisableChat = !game.chat;

    foreach (const auto &pair, role.banLists)
        info.RoleBanlist.insert(pair.first, static_cast<ServerInfoStruct::RoleBanFor>(pair.second));

    return info;
}

ServerConfigStruct *serverConfigInstanceFunc()
{
    return ServerConfigInstance;
}
