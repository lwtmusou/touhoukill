#include "engine.h"
#include "CardFace.h"
#include "RoomObject.h"
#include "card.h"
#include "exppattern.h"
#include "general.h"
#include "global.h"
#include "lua-wrapper.h"
#include "package.h"
#include "player.h"
#include "protocol.h"
#include "serverinfostruct.h"
#include "skill.h"
#include "structs.h"
#include "trigger.h"
#include "util.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QStringList>
#include <QTextStream>
#include <QVersionNumber>

#include <random>

using namespace QSanguosha;

class EnginePrivate
{
public:
    QHash<QString, QString> translations;
    QHash<QString, const General *> generals;
    QHash<QString, const Skill *> skills;
    QMap<QString, QString> modes;
    QMap<QString, const CardPattern *> responsePatterns;
    QMap<QString, const CardPattern *> expPatterns;
    QHash<QString, const CardFace *> faces;
    QHash<QString, const Trigger *> triggers;

    // Package
    QList<const Package *> packages;

    QList<CardDescriptor> cards;
    QSet<QString> lord_list;
    QSet<QString> ban_package;

    JsonObject configFile;

    LuaStatePointer l;

    QSet<QString> LatestGeneralList;

    EnginePrivate()
        : l(nullptr)
    {
    }
};

Engine::Engine()
    : d(new EnginePrivate)
{
    // This file should be in qrc
    JsonDocument doc = JsonDocument::fromFilePath(QStringLiteral("config/gameconfig.json"));
    if (doc.isValid())
        d->configFile = doc.object();

    d->l = LuaMultiThreadEnvironment::luaStateForCurrentThread();

    foreach (const Trigger *trigger, LuaMultiThreadEnvironment::triggers())
        registerTrigger(trigger);

    foreach (const CardFace *cardFace, LuaMultiThreadEnvironment::cardFaces())
        registerCardFace(cardFace);

    addSkills(LuaMultiThreadEnvironment::skills());

    foreach (const Package *package, LuaMultiThreadEnvironment::packages())
        addPackage(package);

    d->LatestGeneralList = List2Set(config(QStringLiteral("latest_generals")).toStringList());

    // I'd like it to refactor to use Qt-builtin way for it
    QString locale = config(QStringLiteral("locale")).toString();
    if (locale.length() == 0)
        locale = QStringLiteral("zh_CN");
    loadTranslations(locale);

    // available game modes
    d->modes[QStringLiteral("02p")] = QObject::tr("2 players");
    d->modes[QStringLiteral("03p")] = QObject::tr("3 players");
    d->modes[QStringLiteral("04p")] = QObject::tr("4 players");
    d->modes[QStringLiteral("05p")] = QObject::tr("5 players");
    d->modes[QStringLiteral("06p")] = QObject::tr("6 players");
    d->modes[QStringLiteral("06pd")] = QObject::tr("6 players (2 renegades)");
    d->modes[QStringLiteral("07p")] = QObject::tr("7 players");
    d->modes[QStringLiteral("08p")] = QObject::tr("8 players");
    d->modes[QStringLiteral("08pd")] = QObject::tr("8 players (2 renegades)");
    d->modes[QStringLiteral("08pz")] = QObject::tr("8 players (0 renegade)");
    d->modes[QStringLiteral("09p")] = QObject::tr("9 players");
    d->modes[QStringLiteral("10pd")] = QObject::tr("10 players");
    d->modes[QStringLiteral("10p")] = QObject::tr("10 players (1 renegade)");
    d->modes[QStringLiteral("10pz")] = QObject::tr("10 players (0 renegade)");
    d->modes[QStringLiteral("hegemony_02")] = QObject::tr("hegemony 2 players");
    d->modes[QStringLiteral("hegemony_03")] = QObject::tr("hegemony 3 players");
    d->modes[QStringLiteral("hegemony_04")] = QObject::tr("hegemony 4 players");
    d->modes[QStringLiteral("hegemony_05")] = QObject::tr("hegemony 5 players");
    d->modes[QStringLiteral("hegemony_06")] = QObject::tr("hegemony 6 players");
    d->modes[QStringLiteral("hegemony_07")] = QObject::tr("hegemony 7 players");
    d->modes[QStringLiteral("hegemony_08")] = QObject::tr("hegemony 8 players");
    d->modes[QStringLiteral("hegemony_09")] = QObject::tr("hegemony 9 players");
    d->modes[QStringLiteral("hegemony_10")] = QObject::tr("hegemony 10 players");
}

void Engine::addTranslationEntry(const QString &key, const QString &value)
{
    d->translations.insert(key, value);
}

Engine::~Engine()
{
    qDeleteAll(d->expPatterns);
    delete d;
}

Engine *Engine::instance()
{
    static Engine e;
    return &e;
}

void Engine::loadTranslations(const QString &locale)
{
    JsonDocument doc = JsonDocument::fromFilePath(QStringLiteral("lang/") + locale + QStringLiteral(".json"));
    if (!doc.isValid() || !doc.isArray())
        return;

    JsonArray jarr = doc.array();

    foreach (const QVariant &fileNameV, jarr) {
        QString fileName = fileNameV.toString();
        JsonDocument transDoc = JsonDocument::fromFilePath(fileName);
        if (!transDoc.isValid() || !transDoc.isObject())
            continue;

        JsonObject ob = transDoc.object();

        for (auto it = ob.cbegin(); it != ob.cend(); ++it)
            addTranslationEntry(it.key(), it.value().toString());
    }
}

void Engine::addSkills(const QList<const Skill *> &all_skills)
{
    foreach (const Skill *skill, all_skills) {
        if (d->skills.contains(skill->name()))
            qDebug() << QObject::tr("Duplicated skill : %1").arg(skill->name());
        else
            d->skills.insert(skill->name(), skill);
    }
}

void Engine::addPackage(const Package *package)
{
    if (d->packages.contains(package))
        return;

    d->packages << package;
    d->cards << package->cards();

    foreach (const General *general, package->generals()) {
        d->generals.insert(general->name(), general);
        if (isGeneralHidden(general->name()))
            continue;
        if (general->isLord())
            d->lord_list << general->name();
    }
}

void Engine::addBanPackage(const QString &package_name)
{
    d->ban_package.insert(package_name);
}

QStringList Engine::getBanPackages() const
{
    if (isHegemonyGameMode(ServerInfo.GameMode)) {
        QStringList ban;
        const QList<const Package *> &packs = packanges();
        QStringList needPacks;
        needPacks << QStringLiteral("hegemonyGeneral") << QStringLiteral("hegemony_card");
        foreach (const Package *pa, packs) {
            if (!needPacks.contains(pa->name()))
                ban << pa->name();
        }
        return ban;
    } else {
        QStringList ban = d->ban_package.values();
        if (!ban.contains(QStringLiteral("hegemonyGeneral")))
            ban << QStringLiteral("hegemonyGeneral");
        if (!ban.contains(QStringLiteral("hegemony_card")))
            ban << QStringLiteral("hegemony_card");
        return ban;
    }
}

QList<const Package *> Engine::packanges() const
{
    return d->packages;
}

const Package *Engine::findPackage(const QString &name) const
{
    foreach (const Package *pkg, d->packages) {
        if (pkg->name() == name)
            return pkg;
    }

    return nullptr;
}

QString Engine::translate(const QString &to_translate) const
{
    QStringList list = to_translate.split(QStringLiteral("\\"));
    QString res;
    foreach (const QString &str, list)
        res.append(d->translations.value(str, str));

    return res;
}

int Engine::getRoleIndex() const
{
    if (ServerInfo.GameMode == QStringLiteral("06_3v3") || ServerInfo.GameMode == QStringLiteral("06_XMode"))
        return 4;
    else if (isHegemonyGameMode(ServerInfo.GameMode))
        return 5;
    else
        return 1;
}

const CardPattern *Engine::responsePattern(const QString &name) const
{
    return d->responsePatterns.value(name, nullptr);
}

const CardPattern *Engine::expPattern(const QString &name) const
{
    if (d->expPatterns.contains(name))
        return d->expPatterns.value(name);

    ExpPattern *expptn = new ExpPattern(name);
    d->expPatterns.insert(name, expptn);
    return expptn;
}

bool Engine::matchExpPattern(const QString &pattern, const Player *player, const Card *card) const
{
    const CardPattern *p = expPattern(pattern);
    return p->match(player, card);
}

const General *Engine::general(const QString &name) const
{
    return d->generals.value(name, nullptr);
}

QStringList Engine::generalNames() const
{
    return d->generals.keys();
}

int Engine::availableGeneralCount() const
{
    int total = d->generals.size();
    QHashIterator<QString, const General *> itor(d->generals);
    while (itor.hasNext()) {
        itor.next();
        const General *general = itor.value();
        if (getBanPackages().contains(general->getPackage()))
            total--;
        else if (isGeneralHidden(general->name()))
            total--;
#if 0
        else if (isRoleGameMode(ServerInfo.GameMode) && Config.value(QStringLiteral("Banlist/Roles")).toStringList().contains(general->name()))
            total--;
        else if (ServerInfo.GameMode == QStringLiteral("04_1v3") && Config.value(QStringLiteral("Banlist/HulaoPass")).toStringList().contains(general->name()))
            total--;
#endif
    }

    return total;
}

bool Engine::isGeneralHidden(const QString &general_name) const
{
    const General *theGeneral = general(general_name);
    if (theGeneral == nullptr)
        return false;

    return theGeneral->isHidden();
#if 0
    if (!general->isHidden())
        return Config.ExtraHiddenGenerals.contains(general_name);
    else
        return !Config.RemovedHiddenGenerals.contains(general_name);
#endif
}

const CardDescriptor &Engine::cardDescriptor(int cardId) const
{
    static CardDescriptor nullDescriptor = {QString(), NoSuit, NumberNA, QString()};

    if (cardId == Card::S_UNKNOWN_CARD_ID)
        return nullDescriptor;
    else if (cardId < 0 || cardId >= d->cards.length()) {
        Q_ASSERT(!(cardId < 0 || cardId >= d->cards.length()));
        return nullDescriptor;
    } else {
        return d->cards[cardId];
    }
}

QString Engine::versionDate() const
{
    return QStringLiteral(QT_STRINGIFY(VERSIONDATE));
}

QString Engine::getVersion() const
{
    return QStringLiteral("%1:%2").arg(versionDate(), modName());
}

const QVersionNumber &Engine::versionNumber() const
{
    static QVersionNumber ver = QVersionNumber::fromString(QStringLiteral(QT_STRINGIFY(VERSION)));
    return ver;
}

QString Engine::modName() const
{
    return QStringLiteral("TouhouSatsu");
}

QStringList Engine::packangeNames() const
{
    QStringList extensions;
    const QList<const Package *> &packages = packanges();
    foreach (const Package *package, packages)
        extensions << package->name();

    return extensions;
}

QStringList Engine::kingdoms() const
{
    static QStringList kingdoms;

    if (kingdoms.isEmpty())
        kingdoms = config(QStringLiteral("kingdoms")).toStringList();

    return kingdoms;
}

QStringList Engine::hegemonyKingdoms() const
{
    static QStringList hegemony_kingdoms;
    if (hegemony_kingdoms.isEmpty())
        hegemony_kingdoms = config(QStringLiteral("hegemony_kingdoms")).toStringList();

    return hegemony_kingdoms;
}

QMap<QString, QString> Engine::getAvailableModes() const
{
    return d->modes;
}

QString Engine::getModeName(const QString &mode) const
{
    if (d->modes.contains(mode))
        return d->modes.value(mode);

    return QString();
}

int Engine::getPlayerCount(const QString &mode) const
{
    QRegularExpression rx(QStringLiteral("\\d+"));
    QRegularExpressionMatchIterator it = rx.globalMatch(mode);
    if (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        return match.captured().toInt(nullptr, 10);
    }

    return -1;
}

QString Engine::getRoles(const QString &mode) const
{
    int n = getPlayerCount(mode);

    if (mode == QStringLiteral("02_1v1")) {
        return QStringLiteral("ZN");
    } else if (mode == QStringLiteral("04_1v3")) {
        return QStringLiteral("ZFFF");
    }
    if (isHegemonyGameMode(mode)) {
        QString role;
        int num = getPlayerCount(mode);
        QStringList roles;
        roles << QStringLiteral("W") << QStringLiteral("S") << QStringLiteral("G") << QStringLiteral("Q"); //wei shu wu qun
        for (int i = 0; i < num; ++i) {
            int role_idx = QRandomGenerator::global()->generate() % roles.length();
            role = role + roles[role_idx];
        }
        return role;
    }

    if (d->modes.contains(mode)) {
        static const char *table1[] = {
            "",          "",

            "ZF", // 2
            "ZFN", // 3
            "ZNFF", // 4
            "ZCFFN", // 5
            "ZCFFFN", // 6
            "ZCCFFFN", // 7
            "ZCCFFFFN", // 8
            "ZCCCFFFFN", // 9
            "ZCCCFFFFFN" // 10
        };

        static const char *table2[] = {
            "",          "",

            "ZF", // 2
            "ZFN", // 3
            "ZNFF", // 4
            "ZCFFN", // 5
            "ZCFFNN", // 6
            "ZCCFFFN", // 7
            "ZCCFFFNN", // 8
            "ZCCCFFFFN", // 9
            "ZCCCFFFFNN" // 10
        };

        const char **table = mode.endsWith(QStringLiteral("d")) ? table2 : table1;
        QString rolechar = QString::fromUtf8(table[n]);
        if (mode.endsWith(QStringLiteral("z")))
            rolechar.replace(QStringLiteral("N"), QStringLiteral("C"));

        return rolechar;
    } else if (mode.startsWith(QStringLiteral("@"))) {
        if (n == 8)
            return QStringLiteral("ZCCCNFFF");
        else if (n == 6)
            return QStringLiteral("ZCCNFF");
    }
    return QString();
}

QStringList Engine::getRoleList(const QString &mode) const
{
    QString roles = getRoles(mode);

    QStringList role_list;
    for (int i = 0; roles[i] != nullptr; i++) {
        QString role;
        switch (roles[i].toLatin1()) {
        case 'Z':
            role = QStringLiteral("lord");
            break;
        case 'C':
            role = QStringLiteral("loyalist");
            break;
        case 'N':
            role = QStringLiteral("renegade");
            break;
        case 'F':
            role = QStringLiteral("rebel");
            break;
        case 'W':
            role = QStringLiteral("wei");
            break;
        case 'S':
            role = QStringLiteral("shu");
            break;
        case 'G':
            role = QStringLiteral("wu");
            break;
        case 'Q':
            role = QStringLiteral("qun");
            break;
        }
        role_list << role;
    }

    return role_list;
}

int Engine::cardCount() const
{
    return d->cards.length();
}

QSet<QString> Engine::availableLords() const
{
    return d->lord_list;
}

QSet<QString> Engine::getRandomLords() const
{
    QStringList l = d->lord_list.values();
    qShuffle(l);

    int lord_num = 6; // Config.value(QStringLiteral("LordMaxChoice"), 6).toInt();
    return List2Set(l.mid(0, lord_num));

#if 0
    QStringList banlist_ban;
    QStringList lords;
    QStringList splords_package; //lords  in sp package will be not count as a lord.
    splords_package << QStringLiteral("thndj");

    foreach (QString alord, availableLords()) {
        if (banlist_ban.contains(alord))
            continue;
        const General *theGeneral = general(alord);
        if (splords_package.contains(theGeneral->getPackage()))
            continue;
        lords << alord;
    }

    // todo: make this variable in serverinfo
    int lord_num = 6; // Config.value(QStringLiteral("LordMaxChoice"), 6).toInt();
    if (lord_num != -1 && lord_num < lords.length()) {
        int to_remove = lords.length() - lord_num;
        for (int i = 0; i < to_remove; i++) {
            lords.removeAt(QRandomGenerator::global()->generate() % lords.length());
        }
    }

    QStringList nonlord_list;
    foreach (QString nonlord, d->generals.keys()) {
        if (isGeneralHidden(nonlord))
            continue;
        const General *general = d->generals.value(nonlord);
        if (d->lord_list.contains(nonlord)) {
            if (!splords_package.contains(general->getPackage()))
                continue;
        }
        if (getBanPackages().contains(general->getPackage()))
            continue;
        if (banlist_ban.contains(general->name()))
            continue;

        nonlord_list << nonlord;
    }

    qShuffle(nonlord_list);

    int addcount = 0;
    int extra = 6; // Config.value(QStringLiteral("NonLordMaxChoice"), 6).toInt();

    int godmax = 1; // Config.value(QStringLiteral("GodLimit"), 1).toInt();
    int godCount = 0;

    if (lord_num == 0 && extra == 0)
        extra = 1;
    bool assign_latest_general = false; // Config.value(QStringLiteral("AssignLatestGeneral"), true).toBool();
    QStringList latest = latestGenerals(QSet<QString>(lords.begin(), lords.end()));
    if (assign_latest_general && !latest.isEmpty()) {
        lords << latest.first();
        if (nonlord_list.contains(latest.first()))
            nonlord_list.removeOne(latest.first());
        extra--;
    }
    for (int i = 0; addcount < extra; i++) {
        if (general(nonlord_list.at(i))->kingdom() != QStringLiteral("touhougod")) {
            lords << nonlord_list.at(i);
            addcount++;
        } else if (godmax > 0 && godCount < godmax) {
            lords << nonlord_list.at(i);
            godCount++;
            addcount++;
        }

        if (i == nonlord_list.length() - 1)
            break;
    }

    return lords;
#endif
}

QStringList Engine::getLimitedGeneralNames() const
{
    // TODO: reimplement this function in separated class Mode
    QStringList general_names;
    QHashIterator<QString, const General *> itor(d->generals);
    if (ServerInfo.GameMode == QStringLiteral("04_1v3")) {
        QList<const General *> hulao_generals = QList<const General *>();
        foreach (QString pack_name, config(QStringLiteral("hulao_packages")).toStringList()) {
            const Package *pack = findPackage(pack_name);
            if (pack != nullptr) {
                foreach (const General *general, pack->generals())
                    hulao_generals << general;
            }
        }

        foreach (const General *general, hulao_generals) {
            if (isGeneralHidden(general->name()) || general->isTotallyHidden() || general->name() == QStringLiteral("yuyuko_1v3"))
                continue;
            general_names << general->name();
        }
    } else {
        while (itor.hasNext()) {
            itor.next();
            if (!isGeneralHidden(itor.value()->name()) && !getBanPackages().contains(itor.value()->getPackage()))
                general_names << itor.key();
        }
    }

    return general_names;
}

QSet<QString> Engine::getRandomGenerals(int count, const QSet<QString> &ban_set) const
{
    // TODO: reimplement this function in separated class Mode
    QStringList all_generals = getLimitedGeneralNames();
    QSet<QString> general_set = QSet<QString>(all_generals.begin(), all_generals.end());

    Q_ASSERT(all_generals.count() >= count);

    QStringList subtractList;
    bool needsubtract = true;
#if 0
    if (isRoleGameMode(ServerInfo.GameMode))
        subtractList = (Config.value(QStringLiteral("Banlist/Roles"), QStringList()).toStringList());
    else if (ServerInfo.GameMode == QStringLiteral("04_1v3"))
        subtractList = (Config.value(QStringLiteral("Banlist/HulaoPass"), QStringList()).toStringList());
    else if (ServerInfo.GameMode == QStringLiteral("06_XMode"))
        subtractList = (Config.value(QStringLiteral("Banlist/XMode"), QStringList()).toStringList());
    else if (isHegemonyGameMode(ServerInfo.GameMode))
        subtractList = (Config.value(QStringLiteral("Banlist/Hegemony"), QStringList()).toStringList());
    else
#endif
    needsubtract = false;

    if (needsubtract)
        general_set.subtract(QSet<QString>(subtractList.begin(), subtractList.end()));

    all_generals = general_set.subtract(ban_set).values();

    // shuffle them
    qShuffle(all_generals);

    int addcount = 0;
    QSet<QString> general_list;
    int godmax = 1; // Config.value(QStringLiteral("GodLimit"), 1).toInt();
    int godCount = 0;
    for (int i = 0; addcount < count; i++) {
        if (general(all_generals.at(i))->kingdom() != QStringLiteral("touhougod")) {
            general_list << all_generals.at(i);
            addcount++;
        } else if (godmax > 0 && godCount < godmax) {
            general_list << all_generals.at(i);
            godCount++;
            addcount++;
        }
        if (i == all_generals.count() - 1)
            break;
    }

    return general_list;
}

QSet<QString> Engine::latestGenerals() const
{
    return d->LatestGeneralList;
}

QList<int> Engine::getRandomCards() const
{
    // TODO: reimplement this function in separated class Mode
#if 0
    bool exclude_disaters = false;
    bool using_2012_3v3 = false;
    bool using_2013_3v3 = false;
    if (Config.GameMode == QStringLiteral("06_3v3")) {
        using_2012_3v3 = (Config.value(QStringLiteral("3v3/OfficialRule"), QStringLiteral("2013")).toString() == QStringLiteral("2012"));
        using_2013_3v3 = (Config.value(QStringLiteral("3v3/OfficialRule"), QStringLiteral("2013")).toString() == QStringLiteral("2013"));
        exclude_disaters = Config.value(QStringLiteral("3v3/ExcludeDisasters"), true).toBool();
    }

    if (Config.GameMode == QStringLiteral("04_1v3"))
        exclude_disaters = true;
    Q_UNUSED(exclude_disaters);
#endif

    QList<int> list;
    for (int i = 0; i < d->cards.length(); ++i)
        list << i;

#if 0
    foreach (const CardDescriptor &card, d->cards) {
        // TODO: deal with this in separated class Mode
        Q_UNUSED(card);

        if (exclude_disaters && card.face()->isKindOf("Disaster"))
            continue;

        if (getPackageNameByCard(card) == "New3v3Card" && (using_2012_3v3 || using_2013_3v3))
            list << card->id();
        else if (getPackageNameByCard(card) == "New3v3_2013Card" && using_2013_3v3)
            list << card->id();

        if (!getBanPackages().contains(getPackageNameByCard(card))) {
            if (card->faceName().startsWith("known_both")) {
                if (isHegemonyGameMode(Config.GameMode) && card->faceName() == "known_both_hegemony")
                    list << card->id();
                else if (!isHegemonyGameMode(Config.GameMode) && card->faceName() == "known_both")
                    list << card->id();

            } else if (card->faceName().startsWith("DoubleSword")) {
                if (isHegemonyGameMode(Config.GameMode) && card->faceName() == "DoubleSwordHegemony")
                    list << card->id();
                else if (!isHegemonyGameMode(Config.GameMode) && card->faceName() == "DoubleSword")
                    list << card->id();
            } else
                list << card->id();
        }
    }
    // remove two crossbows and one nullification?
    if (using_2012_3v3 || using_2013_3v3)
        list.removeOne(98);
    if (using_2013_3v3) {
        list.removeOne(53);
        list.removeOne(54);
    }
#endif

    qShuffle(list);

    return list;
}

QString Engine::getRandomGeneralName() const
{
    return d->generals.keys().at(QRandomGenerator::global()->generate() % d->generals.size());
}

const Skill *Engine::skill(const QString &skill_name) const
{
    return d->skills.value(skill_name, nullptr);
}

const Skill *Engine::skill(const EquipCard *equip) const
{
    const Skill *theSkill = nullptr;

    if (equip != nullptr)
        theSkill = skill(equip->name());

    return theSkill;
}

QStringList Engine::skillNames() const
{
    return d->skills.keys();
}

QVariant Engine::config(const QString &key) const
{
    // TODO: special case of "withHeroSkin" and "withBGM"
    return d->configFile.value(key);
}

void Engine::registerCardFace(const CardFace *cardFace)
{
    if (cardFace != nullptr)
        d->faces.insert(cardFace->name(), cardFace);
}

const CardFace *Engine::cardFace(const QString &name) const
{
    return d->faces.value(name, nullptr);
}

void Engine::unregisterCardFace(const QString &name)
{
    auto face = d->faces.find(name);
    if (face != d->faces.end()) {
        const auto *handle = *face;
        d->faces.erase(face);
        delete handle;
    }
}

void Engine::registerTrigger(const Trigger *trigger)
{
    if (trigger != nullptr)
        d->triggers.insert(trigger->name(), trigger);
}

const Trigger *Engine::trigger(const QString &name) const
{
    return d->triggers.value(name, nullptr);
}

void Engine::unregisterTrigger(const QString &name)
{
    auto face = d->triggers.find(name);
    if (face != d->triggers.end()) {
        const auto *handle = *face;
        d->triggers.erase(face);
        delete handle;
    }
}

// defined in global.h
namespace QSanguosha {
HandlingMethod string2HandlingMethod(const QString &str)
{
    if (str == QStringLiteral("use"))
        return MethodUse;
    else if (str == QStringLiteral("response"))
        return MethodResponse;
    else if (str == QStringLiteral("discard"))
        return MethodDiscard;
    else if (str == QStringLiteral("recast"))
        return MethodRecast;
    else if (str == QStringLiteral("pindian"))
        return MethodPindian;
    else {
        Q_ASSERT(false);
        return MethodNone;
    }
}
} // namespace QSanguosha
