#include "serverinfostruct.h"
#include "engine.h"
#include "package.h"

#include <QRegularExpression>

ServerInfoStruct ServerInfo;

const int ServerInfoStruct::S_SERVER_TIMEOUT_GRACIOUS_PERIOD = 1000;

time_t ServerInfoStruct::getCommandTimeout(QSanProtocol::CommandType command, QSanProtocol::ProcessInstanceType instance, int operationRate)
{
    time_t timeOut = OperationTimeout * 500 * operationRate;

    if (OperationTimeout == 0)
        return 0;
    else if (command == QSanProtocol::S_COMMAND_CHOOSE_GENERAL || command == QSanProtocol::S_COMMAND_ASK_GENERAL)
        timeOut = OperationTimeout * 1500;
    else if (command == QSanProtocol::S_COMMAND_SKILL_GUANXING || command == QSanProtocol::S_COMMAND_ARRANGE_GENERAL)
        timeOut = OperationTimeout * 2000;
    else if (command == QSanProtocol::S_COMMAND_NULLIFICATION)
        timeOut = NullificationCountDown * 1000;

    if (instance == QSanProtocol::S_SERVER_INSTANCE)
        timeOut += S_SERVER_TIMEOUT_GRACIOUS_PERIOD;
    return timeOut;
}

bool ServerInfoStruct::parseLegacy(const QString &str)
{
    QRegularExpression rx(QRegularExpression::anchoredPattern(QStringLiteral("(.*):(@?\\w+):(\\d+):(\\d+):([+\\w]*):([RCFSTBHAMN123a-r]*)")));
    QRegularExpressionMatch match;
    if (!(match = rx.match(str)).hasMatch()) {
        qWarning("%s", qPrintable(QStringLiteral("Setup string error!")));
        return false;
    }

    QStringList texts = match.capturedTexts();
    if (texts.isEmpty()) {
        DuringGame = false;
    } else {
        DuringGame = true;

        const QString &server_name = texts.at(1);
        Name = QString::fromUtf8(QByteArray::fromBase64(server_name.toLatin1()));

        GameMode = texts.at(2);
        if (GameMode.startsWith(QStringLiteral("02_1v1")) || GameMode.startsWith(QStringLiteral("06_3v3"))) {
            GameMode = GameMode.mid(0, 6);
            GameRuleMode = GameMode.mid(6);
        }
        OperationTimeout = texts.at(3).toInt();
        NullificationCountDown = texts.at(4).toInt();

        QStringList ban_packages = texts.at(5).split(QStringLiteral("+"));
        const QList<const Package *> &packages = Sanguosha->packanges();
        foreach (const Package *package, packages) {
            QString package_name = package->name();
            if (ban_packages.contains(package_name))
                package_name = QStringLiteral("!") + package_name;

            Extensions << package_name;
        }

        const QString &flags = texts.at(6);

        RandomSeat = flags.contains(QStringLiteral("R"));
        EnableCheat = flags.contains(QStringLiteral("C"));
        FreeChoose = EnableCheat && flags.contains(QStringLiteral("F"));
        Enable2ndGeneral = flags.contains(QStringLiteral("S"));
        EnableSame = flags.contains(QStringLiteral("T"));
        EnableAI = flags.contains(QStringLiteral("A"));
        DisableChat = flags.contains(QStringLiteral("M"));

        if (flags.contains(QStringLiteral("1")))
            MaxHpScheme = 1;
        else if (flags.contains(QStringLiteral("2")))
            MaxHpScheme = 2;
        else if (flags.contains(QStringLiteral("3")))
            MaxHpScheme = 3;
        else {
            MaxHpScheme = 0;
            for (char c = 'a'; c <= 'r'; c++) {
                if (flags.contains(QLatin1Char(c))) {
                    Scheme0Subtraction = int(c) - int('a') - 5;
                    break;
                }
            }
        }
    }

    return true;
}

bool ServerInfoStruct::parse(const QVariant &object)
{
    JsonObject ob;
    bool ok = JsonUtils::tryParse(object, ob);
    if (!ok) {
        qWarning("%s", qPrintable(QStringLiteral("Setup string error!")));
        return false;
    }

    DuringGame = false;
    if (!ob.isEmpty()) {
        QString server_name = ob.value(QStringLiteral("ServerName")).toString();
        if (server_name.isEmpty())
            return false;
        Name = QString::fromUtf8(QByteArray::fromBase64(server_name.toLatin1()));

        GameMode = ob.value(QStringLiteral("GameMode")).toString();
        if (GameMode.isEmpty())
            return false;
        if (GameMode.startsWith(QStringLiteral("02_1v1")) || GameMode.startsWith(QStringLiteral("06_3v3"))) {
            GameMode = GameMode.mid(0, 6);
            GameRuleMode = GameMode.mid(6);
        }

        bool ok = false;
        OperationTimeout = ob.value(QStringLiteral("OperationTimeout")).toInt(&ok);
        if (!ok)
            return false;

        ok = false;
        NullificationCountDown = ob.value(QStringLiteral("NullificationCountDown")).toInt(&ok);
        if (!ok)
            return false;

        DuringGame = true;

        QVariantList ban_packages = ob.value(QStringLiteral("BanPackages")).toList();
        const QList<const Package *> &packages = Sanguosha->packanges();
        foreach (const Package *package, packages) {
            QString package_name = package->name();
            if (ban_packages.contains(package_name))
                package_name = QStringLiteral("!") + package_name;

            Extensions << package_name;
        }

        RandomSeat = ob.value(QStringLiteral("RandomSeat"), false).toBool();
        EnableCheat = ob.value(QStringLiteral("EnableCheat"), false).toBool();
        FreeChoose = ob.value(QStringLiteral("FreeChoose"), false).toBool();
        Enable2ndGeneral = ob.value(QStringLiteral("Enable2ndGeneral"), false).toBool();
        EnableSame = ob.value(QStringLiteral("EnableSame"), false).toBool();
        EnableAI = ob.value(QStringLiteral("EnableAI"), false).toBool();
        DisableChat = ob.value(QStringLiteral("DisableChat"), false).toBool();

        ok = false;
        MaxHpScheme = ob.value(QStringLiteral("MaxHpScheme")).toInt(&ok);
        if (!ok)
            MaxHpScheme = 1;
        else if (MaxHpScheme == 0) {
            ok = false;
            Scheme0Subtraction = ob.value(QStringLiteral("Scheme0Subtraction")).toInt(&ok);
            if (!ok)
                Scheme0Subtraction = 0;
        }
    }

    return true;
}

QVariant ServerInfoStruct::serialize() const
{
    QVariantMap m;
    if (!DuringGame)
        return m;

    m[QStringLiteral("ServerName")] = QString::fromLatin1(Name.toUtf8().toBase64());
    m[QStringLiteral("GameMode")] = GameMode;
    if (GameMode.startsWith(QStringLiteral("02_1v1")) || GameMode.startsWith(QStringLiteral("06_3v3")))
        m[QStringLiteral("GameMode")] = GameMode + GameRuleMode;
    m[QStringLiteral("OperationTimeout")] = OperationTimeout;
    m[QStringLiteral("NullificationCountDown")] = NullificationCountDown;
    QVariantList l;
    foreach (const QString &package, Extensions) {
        if (package.startsWith(QStringLiteral("!")))
            l << package.chopped(1);
    }
    m[QStringLiteral("BanPackages")] = l;
    m[QStringLiteral("RandomSeat")] = RandomSeat;
    m[QStringLiteral("EnableCheat")] = EnableCheat;
    m[QStringLiteral("FreeChoose")] = FreeChoose;
    m[QStringLiteral("Enable2ndGeneral")] = Enable2ndGeneral;
    m[QStringLiteral("EnableSame")] = EnableSame;
    m[QStringLiteral("DisableChat")] = DisableChat;
    m[QStringLiteral("MaxHpScheme")] = MaxHpScheme;
    m[QStringLiteral("Scheme0Subtraction")] = Scheme0Subtraction;

    return m;
}
