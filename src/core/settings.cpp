#include "settings.h"
#include "card.h"
#include "engine.h"
#include "photo.h"

#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QFontDatabase>
#include <QGlobalStatic>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QStringList>

#include <random>

Q_GLOBAL_STATIC(Settings, ConfigInstance)

static const qreal ViewWidth = 1280 * 0.8;
static const qreal ViewHeight = 800 * 0.8;

//consts
const int Settings::S_SURRENDER_REQUEST_MIN_INTERVAL = 5000;
const int Settings::S_PROGRESS_BAR_UPDATE_INTERVAL = 200;
const int Settings::S_SERVER_TIMEOUT_GRACIOUS_PERIOD = 1000;
const int Settings::S_MOVE_CARD_ANIMATION_DURATION = 600;
const int Settings::S_JUDGE_ANIMATION_DURATION = 1200;
const int Settings::S_JUDGE_LONG_DELAY = 800;

namespace {
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
    QSettings::registerFormat(QStringLiteral("json"), JsonReadFunc, JsonWriteFunc);
}
}

Q_COREAPP_STARTUP_FUNCTION(registerCustomFormat1)

void Settings::loadSettingsFromConfigIni()
{
    {
        QSettings oldConfig
#ifdef Q_OS_WIN
            (QStringLiteral("config.ini"), QSettings::IniFormat)
#else
            ("QSanguosha.org", "QSanguosha")
#endif
                ;
        {
            QVariant value(oldConfig.value(QStringLiteral("1v1/Rule"), QStringLiteral("2013")).toString());
            setValue(QStringLiteral("1v1/Rule"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("1v1/UsingCardExtension"), false).toBool());
            setValue(QStringLiteral("1v1/UsingCardExtension"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("1v1/UsingExtension"), false).toBool());
            setValue(QStringLiteral("1v1/UsingExtension"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("3v3/ExcludeDisasters"), true).toBool());
            setValue(QStringLiteral("3v3/ExcludeDisasters"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("3v3/ExtensionGenerals")).toStringList());
            setValue(QStringLiteral("3v3/ExtensionGenerals"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("3v3/OfficialRule"), QStringLiteral("2013")).toString());
            setValue(QStringLiteral("3v3/OfficialRule"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("3v3/RoleChoose"), QStringLiteral("Normal")).toString());
            setValue(QStringLiteral("3v3/RoleChoose"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("3v3/UsingExtension"), false).toBool());
            setValue(QStringLiteral("3v3/UsingExtension"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("Address")).toString());
            setValue(QStringLiteral("Address"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("AIDelayAD"), 0).toInt());
            setValue(QStringLiteral("AIDelayAD"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("AIProhibitBlindAttack"), false).toBool());
            setValue(QStringLiteral("AIProhibitBlindAttack"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("AlterAIDelayAD"), false).toBool());
            setValue(QStringLiteral("AlterAIDelayAD"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("AppFont"), QApplication::font("QMainWindow")).value<QFont>());
            setValue(QStringLiteral("AppFont"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("AssignLatestGeneral"), true).toBool());
            setValue(QStringLiteral("AssignLatestGeneral"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("BackgroundImage")).toString());
            setValue(QStringLiteral("BackgroundImage"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("BackgroundMusic"), QStringLiteral("audio/title/main.ogg")).toString());
            setValue(QStringLiteral("BackgroundMusic"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("Banlist/1v1")).toStringList());
            setValue(QStringLiteral("Banlist/1v1"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("Banlist/Basara")).toStringList());
            setValue(QStringLiteral("Banlist/Basara"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("Banlist/Hegemony")).toStringList());
            setValue(QStringLiteral("Banlist/Hegemony"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("Banlist/HulaoPass")).toStringList());
            setValue(QStringLiteral("Banlist/HulaoPass"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("Banlist/Pairs")).toStringList());
            setValue(QStringLiteral("Banlist/Pairs"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("Banlist/Roles")).toStringList());
            setValue(QStringLiteral("Banlist/Roles"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("Banlist/XMode")).toStringList());
            setValue(QStringLiteral("Banlist/XMode"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("Banlist/Zombie")).toStringList());
            setValue(QStringLiteral("Banlist/Zombie"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("BanPackages")).toInt());
            setValue(QStringLiteral("BanPackages"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("BGMVolume"), 1.0f).toFloat());
            setValue(QStringLiteral("BGMVolume"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("BubbleChatBoxDelaySeconds"), 2).toInt());
            setValue(QStringLiteral("BubbleChatBoxDelaySeconds"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("CountDownSeconds"), 3).toInt());
            setValue(QStringLiteral("CountDownSeconds"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("DefaultFontPath"), QStringLiteral("font/simli.ttf")).toString());
            setValue(QStringLiteral("DefaultFontPath"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("DefaultHeroSkin"), true).toBool());
            setValue(QStringLiteral("DefaultHeroSkin"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("DetectorPort"), 9526u).toUInt());
            setValue(QStringLiteral("DetectorPort"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("DisableChat"), false).toBool());
            setValue(QStringLiteral("DisableChat"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("DisableLua"), false).toBool());
            setValue(QStringLiteral("DisableLua"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EffectVolume"), 1.0f).toFloat());
            setValue(QStringLiteral("EffectVolume"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("Enable2ndGeneral"), false).toBool());
            setValue(QStringLiteral("Enable2ndGeneral"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableAI"), true).toBool());
            setValue(QStringLiteral("EnableAI"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableAutoSaveRecord"), false).toBool());
            setValue(QStringLiteral("EnableAutoSaveRecord"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableAutoTarget"), true).toBool());
            setValue(QStringLiteral("EnableAutoTarget"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableAutoUpdate"), true).toBool());
            setValue(QStringLiteral("EnableAutoUpdate"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableBgMusic"), true).toBool());
            setValue(QStringLiteral("EnableBgMusic"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableCheat"), false).toBool());
            setValue(QStringLiteral("EnableCheat"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableDoubleClick"), false).toBool());
            setValue(QStringLiteral("EnableDoubleClick"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableEffects"), true).toBool());
            setValue(QStringLiteral("EnableEffects"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableHotKey"), true).toBool());
            setValue(QStringLiteral("EnableHotKey"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableIntellectualSelection"), true).toBool());
            setValue(QStringLiteral("EnableIntellectualSelection"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableLastWord"), true).toBool());
            setValue(QStringLiteral("EnableLastWord"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableMinimizeDialog"), false).toBool());
            setValue(QStringLiteral("EnableMinimizeDialog"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnablePindianBox"), true).toBool());
            setValue(QStringLiteral("EnablePindianBox"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableSame"), false).toBool());
            setValue(QStringLiteral("EnableSame"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableSPConvert"), true).toBool());
            setValue(QStringLiteral("EnableSPConvert"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("EnableSurprisingGenerals"), false).toBool());
            setValue(QStringLiteral("EnableSurprisingGenerals"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("ForbidPackages")).toStringList());
            setValue(QStringLiteral("ForbidPackages"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("ForbidSIMC"), false).toBool());
            setValue(QStringLiteral("ForbidSIMC"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("FreeAssign"), false).toBool());
            setValue(QStringLiteral("FreeAssign"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("FreeAssignSelf"), false).toBool());
            setValue(QStringLiteral("FreeAssignSelf"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("FreeChoose"), false).toBool());
            setValue(QStringLiteral("FreeChoose"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("GameMode"), QStringLiteral("08p")).toString());
            setValue(QStringLiteral("GameMode"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("GodLimit"), 1).toInt());
            setValue(QStringLiteral("GodLimit"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("HistoryIPs")).toStringList());
            setValue(QStringLiteral("HistoryIPs"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("HostAddress"), QStringLiteral("127.0.0.1")).toString());
            setValue(QStringLiteral("HostAddress"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("KnownSurprisingGenerals")).toStringList());
            setValue(QStringLiteral("KnownSurprisingGenerals"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("LastReplayDir")).toString());
            setValue(QStringLiteral("LastReplayDir"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("LimitRobot"), false).toBool());
            setValue(QStringLiteral("LimitRobot"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("LordMaxChoice"), 6).toInt());
            setValue(QStringLiteral("LordMaxChoice"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("LuaPackages")).toString());
            setValue(QStringLiteral("LuaPackages"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("LuckCardLimitation"), 0).toInt());
            setValue(QStringLiteral("LuckCardLimitation"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("MaxCards"), 12).toInt());
            setValue(QStringLiteral("MaxCards"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("MaxChoice"), 6).toInt());
            setValue(QStringLiteral("MaxChoice"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("MaxHpScheme"), 0).toInt());
            setValue(QStringLiteral("MaxHpScheme"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("MiniSceneStage"), 1).toInt());
            setValue(QStringLiteral("MiniSceneStage"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("NetworkOnly"), false).toBool());
            setValue(QStringLiteral("NetworkOnly"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("NeverNullifyMyTrick"), true).toBool());
            setValue(QStringLiteral("NeverNullifyMyTrick"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("NoEquipAnim"), false).toBool());
            setValue(QStringLiteral("NoEquipAnim"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("NoIndicator"), false).toBool());
            setValue(QStringLiteral("NoIndicator"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("NonLordMaxChoice"), 6).toInt());
            setValue(QStringLiteral("NonLordMaxChoice"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("NullificationCountDown"), 8).toInt());
            setValue(QStringLiteral("NullificationCountDown"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("OperationNoLimit"), false).toBool());
            setValue(QStringLiteral("OperationNoLimit"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("OperationTimeout"), 15).toInt());
            setValue(QStringLiteral("OperationTimeout"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("OriginAIDelay"), 1000).toInt());
            setValue(QStringLiteral("OriginAIDelay"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("PileSwappingLimitation"), 5).toInt());
            setValue(QStringLiteral("PileSwappingLimitation"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("PreventAwakenBelow3"), false).toBool());
            setValue(QStringLiteral("PreventAwakenBelow3"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("RandomSeat"), true).toBool());
            setValue(QStringLiteral("RandomSeat"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("RecordSavePath"), QStringLiteral("records/")).toString());
            setValue(QStringLiteral("RecordSavePath"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("Scheme0Subtraction"), 3).toInt());
            setValue(QStringLiteral("Scheme0Subtraction"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("ServerPort"), 9527u).toUInt());
            setValue(QStringLiteral("ServerPort"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("SurrenderAtDeath"), false).toBool());
            setValue(QStringLiteral("SurrenderAtDeath"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("TableBgImage"), QStringLiteral("backdrop/default.jpg")).toString());
            setValue(QStringLiteral("TableBgImage"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("TextEditColor"), QStringLiteral("white")).toString());
            setValue(QStringLiteral("TextEditColor"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("ToolTipBackgroundColor"), QStringLiteral("#000000")).toString());
            setValue(QStringLiteral("ToolTipBackgroundColor"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("UseLordBackdrop"), true).toBool());
            setValue(QStringLiteral("UseLordBackdrop"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("UseLordBGM"), true).toBool());
            setValue(QStringLiteral("UseLordBGM"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("UserAvatar"), QStringLiteral("keine_sp")).toString());
            setValue(QStringLiteral("UserAvatar"), value);
        }

        {
#ifdef Q_OS_WIN32
            UserName = oldConfig.value(QStringLiteral("UserName"), QString::fromUtf8(qgetenv("USERNAME"))).toString();
#else
            UserName = oldConfig.value("USERNAME", qgetenv("USER")).toString();
#endif

            if (UserName == QStringLiteral("Admin") || UserName == QStringLiteral("Administrator"))
                UserName = tr("Sanguosha-fans");

            QVariant value(UserName);
            setValue(QStringLiteral("UserName"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("ServerName"), tr("%1's server").arg(UserName)).toString());
            setValue(QStringLiteral("ServerName"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("UIFont"), QApplication::font("QTextEdit")).value<QFont>());
            setValue(QStringLiteral("UIFont"), value);
        }

        {
            QPoint p(oldConfig.value(QStringLiteral("WindowPosition"), QPoint(-8, -8)).toPoint());
            setValue(QStringLiteral("WindowX"), p.x());
            setValue(QStringLiteral("WindowY"), p.y());
        }

        {
            QSize s(oldConfig.value(QStringLiteral("WindowSize"), QSize(1366, 706)).toSize());
            setValue(QStringLiteral("WindowWidth"), s.width());
            setValue(QStringLiteral("WindowHeight"), s.height());
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("WithoutLordskill"), false).toBool());
            setValue(QStringLiteral("WithoutLordskill"), value);
        }

        {
            QVariant value(oldConfig.value(QStringLiteral("XMode/RoleChooseX"), QStringLiteral("Normal")).toString());
            setValue(QStringLiteral("XMode/RoleChooseX"), value);
        }
    }

    QFile::remove(QStringLiteral("config.ini"));
}

Settings::Settings()
    : QSettings(QStringLiteral("config.json"), QSettings::CustomFormat1)
    , Rect(-ViewWidth / 2, -ViewHeight / 2, ViewWidth, ViewHeight)
{
    if (!QFile::exists(QStringLiteral("config.json")))
        loadSettingsFromConfigIni();
}

void Settings::init()
{
    if (!QCoreApplication::instance()->arguments().contains(QStringLiteral("-server"))) {
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
    ServerPort = value(QStringLiteral("ServerPort"), 9527u).toUInt();
    DisableLua = value(QStringLiteral("DisableLua"), false).toBool();
    LimitRobot = value(QStringLiteral("LimitRobot"), false).toBool();

#ifdef Q_OS_WIN32
    UserName = value(QStringLiteral("UserName"), qgetenv("USERNAME")).toString();
#else
    UserName = value("USERNAME", qgetenv("USER")).toString();
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
    DetectorPort = value(QStringLiteral("DetectorPort"), 9526u).toUInt();
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
    BGMVolume = value(QStringLiteral("BGMVolume"), 1.0f).toFloat();
    EffectVolume = value(QStringLiteral("EffectVolume"), 1.0f).toFloat();

    int length = 8;
    int index = QRandomGenerator::global()->generate() % length + 1;
    QString bgFilename = QStringLiteral("%1%2%3").arg(QStringLiteral("backdrop/hall/gensoukyou_")).arg(index).arg(QStringLiteral(".jpg"));

    BackgroundImage = bgFilename;
    TableBgImage = value(QStringLiteral("TableBgImage"), QStringLiteral("backdrop/default.jpg")).toString();
    UseLordBackdrop = value(QStringLiteral("UseLordBackdrop"), true).toBool();

    EnableAutoSaveRecord = value(QStringLiteral("EnableAutoSaveRecord"), false).toBool();
    NetworkOnly = value(QStringLiteral("NetworkOnly"), false).toBool();
    RecordSavePath = value(QStringLiteral("RecordSavePath"), QStringLiteral("records/")).toString();

    QStringList roles_ban, kof_ban, hulao_ban, xmode_ban, basara_ban, hegemony_ban, pairs_ban;

    roles_ban = Sanguosha->getConfigFromConfigFile(QStringLiteral("roles_ban")).toStringList();
    kof_ban = Sanguosha->getConfigFromConfigFile(QStringLiteral("kof_ban")).toStringList();
    hulao_ban = Sanguosha->getConfigFromConfigFile(QStringLiteral("hulao_ban")).toStringList();
    xmode_ban = Sanguosha->getConfigFromConfigFile(QStringLiteral("xmode_ban")).toStringList();
    basara_ban = Sanguosha->getConfigFromConfigFile(QStringLiteral("basara_ban")).toStringList();
    hegemony_ban = Sanguosha->getConfigFromConfigFile(QStringLiteral("hegemony_ban")).toStringList();
    hegemony_ban.append(basara_ban);
    foreach (QString general, Sanguosha->getLimitedGeneralNames()) {
        if (Sanguosha->getGeneral(general)->getKingdom() == QStringLiteral("god") && !hegemony_ban.contains(general))
            hegemony_ban << general;
    }
    pairs_ban = Sanguosha->getConfigFromConfigFile(QStringLiteral("pairs_ban")).toStringList();

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

    ExtraHiddenGenerals = Sanguosha->getConfigFromConfigFile(QStringLiteral("extra_hidden_generals")).toStringList();
    RemovedHiddenGenerals = Sanguosha->getConfigFromConfigFile(QStringLiteral("removed_hidden_generals")).toStringList();

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
