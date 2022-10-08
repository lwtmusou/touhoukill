#include "serverinfostruct.h"
#include "engine.h"
#include "jsonutils.h"
#include "mode.h"
#include "package.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>

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

    // Server may allow twice or even three times of period, and disconnect if reply hasn't come
    if (instance == QSanProtocol::S_SERVER_INSTANCE)
        timeOut = timeOut * 2.5;
    return timeOut;
}

ServerInfoStruct::ServerInfoStruct()
    : GameMode(nullptr)
    , OperationTimeout(0)
    , NullificationCountDown(0)
    , RandomSeat(false)
    , EnableCheat(false)
    , FreeChoose(false)
    , GeneralsPerPlayer(0)
    , EnableAI(false)
    , DisableChat(false)
{
}

bool ServerInfoStruct::parseLegacy(const QString &str)
{
    static QRegularExpression rx(QRegularExpression::anchoredPattern(QStringLiteral("(.*):(@?\\w+):(\\d+):(\\d+):([+\\w]*):([RCFSAM]*)")));
    QRegularExpressionMatch match;
    if (!(match = rx.match(str)).hasMatch()) {
        qWarning("%s", qPrintable(QStringLiteral("Setup string error!")));
        return false;
    }

    QStringList texts = match.capturedTexts();
    if (texts.isEmpty()) {
        GameMode = nullptr;
    } else {
        const QString &server_name = texts.at(1);
        Name = QString::fromUtf8(QByteArray::fromBase64(server_name.toLatin1()));

        GameModeStr = texts.at(2);

        GameMode = Mode::findMode(GameModeStr);
        if (GameMode == nullptr)
            return false;

        if (GameModeStr.startsWith(QStringLiteral("1v1")) || GameModeStr.startsWith(QStringLiteral("3v3"))) {
            GameModeStr = GameModeStr.mid(0, 4);
            GameRuleMode = GameModeStr.mid(6);
        }

        OperationTimeout = texts.at(3).toInt();
        NullificationCountDown = texts.at(4).toInt();

        QStringList ban_packages = texts.at(5).split(QStringLiteral("+"));
        const QList<const Package *> &packages = Sanguosha->packages();
        foreach (const Package *package, packages) {
            QString package_name = package->name();
            if (!ban_packages.contains(package_name))
                EnabledPackages << package_name;
        }

        const QString &flags = texts.at(6);

        RandomSeat = flags.contains(QStringLiteral("R"));
        EnableCheat = flags.contains(QStringLiteral("C"));
        FreeChoose = EnableCheat && flags.contains(QStringLiteral("F"));
        GeneralsPerPlayer = (flags.contains(QStringLiteral("S")) ? 2 : 1);
        EnableAI = flags.contains(QStringLiteral("A"));
        DisableChat = flags.contains(QStringLiteral("M"));
    }

    return true;
}

bool ServerInfoStruct::parse(const QJsonValue &value)
{
    QJsonObject theOb;
    if (value.isObject()) {
        theOb = value.toObject();
    } else {
        qWarning("%s", qPrintable(QStringLiteral("Setup string error!")));
        return false;
    }

    if (!theOb.isEmpty()) {
        if (theOb.contains(QStringLiteral("ServerName"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("ServerName"));
            if (theValue.isString())
                Name = theValue.toString();
            else
                return false;
        } else {
            return false;
        }

        if (theOb.contains(QStringLiteral("GameMode"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("GameMode"));
            if (theValue.isString())
                GameModeStr = theValue.toString();
            else
                return false;
        } else {
            return false;
        }
        GameMode = Mode::findMode(GameModeStr);
        if (GameMode == nullptr)
            return false;

        if (theOb.contains(QStringLiteral("GameRuleMode"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("GameRuleMode"));
            if (theValue.isString())
                GameRuleMode = theValue.toString();
            else
                return false;
        } else {
            return false;
        }

        if (theOb.contains(QStringLiteral("OperationTimeout"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("OperationTimeout"));
            if (theValue.isDouble())
                OperationTimeout = theValue.toInt();
            else
                return false;
        } else {
            return false;
        }

        if (theOb.contains(QStringLiteral("NullificationCountDown"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("NullificationCountDown"));
            if (theValue.isDouble())
                NullificationCountDown = theValue.toInt();
            else
                return false;
        } else {
            return false;
        }

        bool ok = false;
        EnabledPackages = QSgsJsonUtils::toStringList(theOb.value(QStringLiteral("EnabledPackages")), &ok);
        if (!ok)
            return false;

        RandomSeat = false;
        if (theOb.contains(QStringLiteral("RandomSeat"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("RandomSeat"));
            if (theValue.isBool())
                RandomSeat = theValue.toBool();
        }
        EnableCheat = false;
        if (theOb.contains(QStringLiteral("EnableCheat"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("EnableCheat"));
            if (theValue.isBool())
                EnableCheat = theValue.toBool();
        }
        FreeChoose = false;
        if (theOb.contains(QStringLiteral("FreeChoose"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("FreeChoose"));
            if (theValue.isBool())
                FreeChoose = theValue.toBool();
        }
        GeneralsPerPlayer = 0;
        if (theOb.contains(QStringLiteral("GeneralsPerPlayer"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("GeneralsPerPlayer"));
            if (theValue.isDouble())
                GeneralsPerPlayer = theValue.toInt();
        }
        if (GeneralsPerPlayer < GameMode->generalsPerPlayer())
            GeneralsPerPlayer = GameMode->generalsPerPlayer();
        EnableAI = false;
        if (theOb.contains(QStringLiteral("EnableAI"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("EnableAI"));
            if (theValue.isBool())
                EnableAI = theValue.toBool();
        }
        DisableChat = false;
        if (theOb.contains(QStringLiteral("DisableChat"))) {
            QJsonValue theValue = theOb.value(QStringLiteral("DisableChat"));
            if (theValue.isBool())
                DisableChat = theValue.toBool();
        }
        return true;
    }

    return false;
}

QJsonValue ServerInfoStruct::serialize() const
{
    QJsonObject m;
    if (GameMode == nullptr)
        return m;

    m[QStringLiteral("ServerName")] = Name;
    m[QStringLiteral("GameMode")] = GameModeStr;
    m[QStringLiteral("GameRuleMode")] = GameRuleMode;
    m[QStringLiteral("OperationTimeout")] = OperationTimeout;
    m[QStringLiteral("NullificationCountDown")] = NullificationCountDown;
    m[QStringLiteral("EnabledPackages")] = QSgsJsonUtils::toJsonArray(EnabledPackages);
    m[QStringLiteral("RandomSeat")] = RandomSeat;
    m[QStringLiteral("EnableCheat")] = EnableCheat;
    m[QStringLiteral("FreeChoose")] = FreeChoose;
    m[QStringLiteral("GeneralsPerPlayer")] = GeneralsPerPlayer;
    m[QStringLiteral("DisableChat")] = DisableChat;

    return m;
}

bool ServerInfoStruct::parsed() const
{
    return GameMode != nullptr;
}

bool ServerInfoStruct::isMultiGeneralEnabled() const
{
    if (!parsed())
        return false;

    return GeneralsPerPlayer > GameMode->generalsPerPlayer();
}
