#include "serverinfostruct.h"
#include "engine.h"
#include "package.h"
#include "settings.h"

ServerInfoStruct ServerInfo;

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
        timeOut += Config.S_SERVER_TIMEOUT_GRACIOUS_PERIOD;
    return timeOut;
}

bool ServerInfoStruct::parse(const QString &str)
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

        QString server_name = texts.at(1);
        Name = QString::fromUtf8(QByteArray::fromBase64(server_name.toLatin1()));

        GameMode = texts.at(2);
        if (GameMode.startsWith(QStringLiteral("02_1v1")) || GameMode.startsWith(QStringLiteral("06_3v3"))) {
            GameMode = GameMode.mid(0, 6);
            GameRuleMode = GameMode.mid(6);
        }
        OperationTimeout = texts.at(3).toInt();
        NullificationCountDown = texts.at(4).toInt();

        QStringList ban_packages = texts.at(5).split(QStringLiteral("+"));
        const QList<const Package *> &packages = Sanguosha->getPackages();
        foreach (const Package *package, packages) {
            QString package_name = package->name();
            if (ban_packages.contains(package_name))
                package_name = QStringLiteral("!") + package_name;

            Extensions << package_name;
        }

        QString flags = texts.at(6);

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
