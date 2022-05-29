#include "settings.h"
#include "card.h"
#include "engine.h"
#include "general.h"

#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QFontDatabase>
#include <QGlobalStatic>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QRandomGenerator>
#include <QStringList>

#include <random>

Q_GLOBAL_STATIC(Settings, ConfigInstance)

static const qreal ViewWidth = 1280 * 0.8;
static const qreal ViewHeight = 800 * 0.8;

//consts
const int Settings::S_SURRENDER_REQUEST_MIN_INTERVAL = 5000;
const int Settings::S_PROGRESS_BAR_UPDATE_INTERVAL = 200;
const int Settings::S_MOVE_CARD_ANIMATION_DURATION = 600;
const int Settings::S_JUDGE_ANIMATION_DURATION = 1200;
const int Settings::S_JUDGE_LONG_DELAY = 800;

namespace {
QSettings::Format jsonFormat;

bool JsonReadFunc(QIODevice &device, QSettings::SettingsMap &map)
{
    QByteArray arr = device.readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &err);
    if (err.error == QJsonParseError::NoError) {
        map = doc.object().toVariantMap();
        return true;
    } else {
        qDebug("Json error %d: %s", err.error, err.errorString().toLocal8Bit().constData());
        return false;
    }
}

bool JsonWriteFunc(QIODevice &device, const QSettings::SettingsMap &map)
{
    QJsonObject ob = QJsonObject::fromVariantMap(map);
    QJsonDocument doc(ob);
    device.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

void registerCustomFormat1()
{
    jsonFormat = QSettings::registerFormat(QStringLiteral("json"), JsonReadFunc, JsonWriteFunc);
}
} // namespace

Q_COREAPP_STARTUP_FUNCTION(registerCustomFormat1)

Settings::Settings()
    : QSettings(QStringLiteral("config.json"), jsonFormat)
    , Rect(-ViewWidth / 2, -ViewHeight / 2, ViewWidth, ViewHeight)
{
}

void Settings::init()
{
    if (!QCoreApplication::arguments().contains(QStringLiteral("-server"))) {
        QString font_path = value(QStringLiteral("DefaultFontPath"), QStringLiteral("font/simli.ttf")).toString();
        int font_id = QFontDatabase::addApplicationFont(font_path);
        if (font_id != -1) {
            QString font_family = QFontDatabase::applicationFontFamilies(font_id).first();
            BigFont.setFamily(font_family);
            SmallFont.setFamily(font_family);
            TinyFont.setFamily(font_family);
        } else
            QMessageBox::warning(nullptr, tr("Warning"), tr("Font file %1 could not be loaded!").arg(font_path));

        BigFont.setPixelSize(56);
        SmallFont.setPixelSize(27);
        TinyFont.setPixelSize(18);

        SmallFont.setWeight(QFont::Bold);

        AppFont = value(QStringLiteral("AppFont"), QApplication::font("QMainWindow")).value<QFont>();
        UIFont = value(QStringLiteral("UIFont"), QApplication::font("QTextEdit")).value<QFont>();
        TextEditColor = QColor(value(QStringLiteral("TextEditColor"), QStringLiteral("white")).toString());
        ToolTipBackgroundColor = value(QStringLiteral("ToolTipBackgroundColor"), QStringLiteral("#000000")).toString();
    }

    CountDownSeconds = value(QStringLiteral("CountDownSeconds"), 3).toInt();
    GameMode = value(QStringLiteral("GameMode"), QStringLiteral("08p")).toString();

    QStringList banpackagelist = value(QStringLiteral("BanPackages")).toStringList();

    setValue(QStringLiteral("BanPackages"), banpackagelist);

    BanPackages = value(QStringLiteral("BanPackages")).toStringList();

    RandomSeat = value(QStringLiteral("RandomSeat"), true).toBool();
    AssignLatestGeneral = value(QStringLiteral("AssignLatestGeneral"), false).toBool();
    EnableCheat = value(QStringLiteral("EnableCheat"), false).toBool();
    FreeChoose = EnableCheat && value(QStringLiteral("FreeChoose"), false).toBool();
    ForbidSIMC = value(QStringLiteral("ForbidSIMC"), false).toBool();
    DisableChat = value(QStringLiteral("DisableChat"), false).toBool();
    FreeAssignSelf = EnableCheat && value(QStringLiteral("FreeAssignSelf"), false).toBool();
    Enable2ndGeneral = value(QStringLiteral("Enable2ndGeneral"), false).toBool();
    EnableSame = value(QStringLiteral("EnableSame"), false).toBool();
    MaxHpScheme = value(QStringLiteral("MaxHpScheme"), 0).toInt();
    Scheme0Subtraction = value(QStringLiteral("Scheme0Subtraction"), 3).toInt();
    PreventAwakenBelow3 = value(QStringLiteral("PreventAwakenBelow3"), false).toBool();
    Address = value(QStringLiteral("Address"), QString()).toString();
    EnableAI = value(QStringLiteral("EnableAI"), true).toBool();
    OriginAIDelay = value(QStringLiteral("OriginAIDelay"), 1000).toInt();
    AlterAIDelayAD = value(QStringLiteral("AlterAIDelayAD"), false).toBool();
    AIDelayAD = value(QStringLiteral("AIDelayAD"), 0).toInt();
    AIProhibitBlindAttack = value(QStringLiteral("AIProhibitBlindAttack"), false).toBool();
    SurrenderAtDeath = value(QStringLiteral("SurrenderAtDeath"), false).toBool();
    LuckCardLimitation = value(QStringLiteral("LuckCardLimitation"), 0).toInt();
    ServerPort = value(QStringLiteral("ServerPort"), 9527U).toUInt();
    LimitRobot = value(QStringLiteral("LimitRobot"), false).toBool();

#ifdef Q_OS_WIN32
    UserName = value(QStringLiteral("UserName"), QString::fromUtf8(qgetenv("USERNAME"))).toString();
#else
    UserName = value(QStringLiteral("USERNAME"), QString::fromUtf8(qgetenv("USER"))).toString();
#endif

    if (UserName == QStringLiteral("Admin") || UserName == QStringLiteral("Administrator"))
        UserName = tr("Sanguosha-fans");
    ServerName = value(QStringLiteral("ServerName"), tr("%1's server").arg(UserName)).toString();

    if (!contains(QStringLiteral("HostUrl"))) {
        if (contains(QStringLiteral("HostAddress"))) {
            QString h = value(QStringLiteral("HostAddress")).toString();
            setValue(QStringLiteral("HostUrl"), (QStringLiteral("qths://") + h));
            remove(QStringLiteral("HostAddress"));
        }
    }
    HostAddress = value(QStringLiteral("HostUrl"), QStringLiteral("qths://127.0.0.1")).toString();
    UserAvatar = value(QStringLiteral("UserAvatar"), QStringLiteral("keine_sp")).toString();

    if (!(contains(QStringLiteral("HistoryUrls")))) {
        if (contains(QStringLiteral("HistoryIPs"))) {
            QStringList l = value(QStringLiteral("HistoryIPs")).toStringList();
            QStringList l2;
            foreach (const QString &i, l)
                l2 << (QStringLiteral("qths://") + i);

            setValue(QStringLiteral("HistoryUrls"), l2);
            remove(QStringLiteral("HistoryIPs"));
        }
    }
    HistoryIPs = value(QStringLiteral("HistoryUrls")).toStringList();
    DetectorPort = value(QStringLiteral("DetectorPort"), 9526U).toUInt();
    MaxCards = value(QStringLiteral("MaxCards"), 12).toInt();

    HegemonyFirstShowReward = value(QStringLiteral("HegemonyFirstShowReward"), QStringLiteral("None")).toString();
    HegemonyCompanionReward = value(QStringLiteral("HegemonyCompanionReward"), QStringLiteral("Instant")).toString();
    HegemonyHalfHpReward = value(QStringLiteral("HegemonyHalfHpReward"), QStringLiteral("Instant")).toString();
    HegemonyCareeristKillReward = value(QStringLiteral("HegemonyCareeristKillReward"), QStringLiteral("AsUsual")).toString();

    EnableHotKey = value(QStringLiteral("EnableHotKey"), true).toBool();
    NeverNullifyMyTrick = value(QStringLiteral("NeverNullifyMyTrick"), true).toBool();
    EnableMinimizeDialog = value(QStringLiteral("EnableMinimizeDialog"), false).toBool();
    EnableAutoTarget = value(QStringLiteral("EnableAutoTarget"), true).toBool();
    EnableIntellectualSelection = value(QStringLiteral("EnableIntellectualSelection"), true).toBool();
    EnableDoubleClick = value(QStringLiteral("EnableDoubleClick"), false).toBool();
    EnableAutoUpdate = value(QStringLiteral("EnableAutoUpdate"), true).toBool();
    BubbleChatBoxDelaySeconds = value(QStringLiteral("BubbleChatBoxDelaySeconds"), 2).toInt();
    DefaultHeroSkin = value(QStringLiteral("DefaultHeroSkin"), true).toBool();

    NullificationCountDown = value(QStringLiteral("NullificationCountDown"), 8).toInt();
    OperationTimeout = value(QStringLiteral("OperationTimeout"), 15).toInt();
    OperationNoLimit = value(QStringLiteral("OperationNoLimit"), false).toBool();
    EnableEffects = value(QStringLiteral("EnableEffects"), true).toBool();
    EnableLastWord = value(QStringLiteral("EnableLastWord"), true).toBool();
    EnableBgMusic = value(QStringLiteral("EnableBgMusic"), true).toBool();
    UseLordBGM = value(QStringLiteral("UseLordBGM"), true).toBool();
    BGMVolume = value(QStringLiteral("BGMVolume"), 1.0F).toFloat();
    EffectVolume = value(QStringLiteral("EffectVolume"), 1.0F).toFloat();

    int length = 8;
    int index = QRandomGenerator::global()->generate() % length + 1;
    QString bgFilename = QStringLiteral("%1%2%3").arg(QStringLiteral("backdrop/hall/gensoukyou_"), QString::number(index), QStringLiteral(".jpg"));

    BackgroundImage = bgFilename;
    TableBgImage = value(QStringLiteral("TableBgImage"), QStringLiteral("backdrop/default.jpg")).toString();
    UseLordBackdrop = value(QStringLiteral("UseLordBackdrop"), true).toBool();

    EnableAutoSaveRecord = value(QStringLiteral("EnableAutoSaveRecord"), false).toBool();
    NetworkOnly = value(QStringLiteral("NetworkOnly"), false).toBool();
    RecordSavePath = value(QStringLiteral("RecordSavePath"), QStringLiteral("records/")).toString();

    QStringList roles_ban;
    QStringList kof_ban;
    QStringList hulao_ban;
    QStringList xmode_ban;
    QStringList basara_ban;
    QStringList hegemony_ban;
    QStringList pairs_ban;

    roles_ban = Sanguosha->configuration(QStringLiteral("roles_ban")).toStringList();
    kof_ban = Sanguosha->configuration(QStringLiteral("kof_ban")).toStringList();
    hulao_ban = Sanguosha->configuration(QStringLiteral("hulao_ban")).toStringList();
    xmode_ban = Sanguosha->configuration(QStringLiteral("xmode_ban")).toStringList();
    basara_ban = Sanguosha->configuration(QStringLiteral("basara_ban")).toStringList();
    hegemony_ban = Sanguosha->configuration(QStringLiteral("hegemony_ban")).toStringList();
    hegemony_ban.append(basara_ban);
    pairs_ban = Sanguosha->configuration(QStringLiteral("pairs_ban")).toStringList();

    QStringList banlist = value(QStringLiteral("Banlist/Roles")).toStringList();
    if (banlist.isEmpty()) {
        foreach (QString ban_general, roles_ban)
            banlist << ban_general;

        setValue(QStringLiteral("Banlist/Roles"), banlist);
    }

    banlist = value(QStringLiteral("Banlist/1v1")).toStringList();
    if (banlist.isEmpty()) {
        foreach (QString ban_general, kof_ban)
            banlist << ban_general;

        setValue(QStringLiteral("Banlist/1v1"), banlist);
    }

    banlist = value(QStringLiteral("Banlist/HulaoPass")).toStringList();
    if (banlist.isEmpty()) {
        foreach (QString ban_general, hulao_ban)
            banlist << ban_general;

        setValue(QStringLiteral("Banlist/HulaoPass"), banlist);
    }

    banlist = value(QStringLiteral("Banlist/XMode")).toStringList();
    if (banlist.isEmpty()) {
        foreach (QString ban_general, xmode_ban)
            banlist << ban_general;

        setValue(QStringLiteral("Banlist/XMode"), banlist);
    }

    banlist = value(QStringLiteral("Banlist/Basara")).toStringList();
    if (banlist.isEmpty()) {
        foreach (QString ban_general, basara_ban)
            banlist << ban_general;

        setValue(QStringLiteral("Banlist/Basara"), banlist);
    }

    banlist = value(QStringLiteral("Banlist/Hegemony")).toStringList();
    if (banlist.isEmpty()) {
        foreach (QString ban_general, hegemony_ban)
            banlist << ban_general;
        setValue(QStringLiteral("Banlist/Hegemony"), banlist);
    }

    banlist = value(QStringLiteral("Banlist/Pairs")).toStringList();
    if (banlist.isEmpty()) {
        foreach (QString ban_general, pairs_ban)
            banlist << ban_general;

        setValue(QStringLiteral("Banlist/Pairs"), banlist);
    }

    QStringList forbid_packages = value(QStringLiteral("ForbidPackages")).toStringList();
    if (forbid_packages.isEmpty()) {
        forbid_packages << QStringLiteral("New3v3Card") << QStringLiteral("New3v3_2013Card") << QStringLiteral("New1v1Card") << QStringLiteral("test");

        setValue(QStringLiteral("ForbidPackages"), forbid_packages);
    }

    ExtraHiddenGenerals = Sanguosha->configuration(QStringLiteral("extra_hidden_generals")).toStringList();
    RemovedHiddenGenerals = Sanguosha->configuration(QStringLiteral("removed_hidden_generals")).toStringList();

    AutoUpdateNeedsRestart = !EnableAutoUpdate;
    AutoUpdateDataRececived = false;
}

const QString &Settings::getQSSFileContent()
{
    static QString qssFileContent;
    if (qssFileContent.isEmpty()) {
        QFile file(QStringLiteral("sanguosha.qss"));
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream stream(&file);
            qssFileContent = stream.readAll();

            file.close();
        }
    }

    return qssFileContent;
}

Settings *configInstance()
{
    return ConfigInstance;
}
