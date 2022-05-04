#include "engine.h"
#include "CardFace.h"
#include "RoomObject.h"
#include "card.h"
#include "exppattern.h"
#include "general.h"
#include "global.h"
#include "lua-wrapper.h"
#include "mode.h"
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
    QMap<QString, const CardPattern *> responsePatterns;
    QMap<QString, const CardPattern *> expPatterns;
    QHash<QString, const CardFace *> faces;
    QHash<QString, const Trigger *> triggers;

    // Package
    QList<const Package *> packages;

    QList<CardDescriptor> cards;
    QSet<QString> lord_list;

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

    foreach (const Skill *skill, LuaMultiThreadEnvironment::skills())
        addSkill(skill);

    foreach (const Package *package, LuaMultiThreadEnvironment::packages())
        addPackage(package);

    d->LatestGeneralList = List2Set(config(QStringLiteral("latest_generals")).toStringList());

    // I'd like it to refactor to use Qt-builtin way for it
    QString locale = config(QStringLiteral("locale")).toString();
    if (locale.length() == 0)
        locale = QStringLiteral("zh_CN");
    loadTranslations(locale);
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

void Engine::addSkill(const Skill *skill)
{
    if (d->skills.contains(skill->name()))
        qDebug() << QObject::tr("Duplicated skill : %1").arg(skill->name());
    else
        d->skills.insert(skill->name(), skill);
}

void Engine::addPackage(const Package *package)
{
    if (d->packages.contains(package))
        return;

    d->packages << package;
    d->cards << package->cards();

    foreach (const General *general, package->generals()) {
        d->generals.insert(general->name(), general);
        if (general->isHidden())
            continue;
        if (general->isLord())
            d->lord_list << general->name();
    }
}

QList<const Package *> Engine::packages() const
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

QSet<QString> Engine::generalNames() const
{
    return List2Set(d->generals.keys());
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

QStringList Engine::packageNames() const
{
    QStringList extensions;
    const QList<const Package *> &ps = packages();
    foreach (const Package *package, ps)
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

QString Engine::getModeName(const QString &name) const
{
    return translate(name);
}

int Engine::getPlayerCount(const QString &name) const
{
    const Mode *mode = Mode::findMode(name);
    if (mode == nullptr)
        return -1;

    return mode->playersCount();
}

QString Engine::getRoles(const QString &mode) const
{
    return Mode::findMode(mode)->roles();
}

QStringList Engine::getRoleList(const QString &mode) const
{
    QString roles = Mode::findMode(mode)->roles();

    QStringList role_list;
    for (int i = 0; roles[i] != QChar::Null; i++) {
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

QSet<QString> Engine::availableLordNames() const
{
    return d->lord_list;
}

QStringList Engine::getLimitedGeneralNames() const
{
    // TODO: reimplement this function in separated class Mode
    QStringList general_names;
    QHashIterator<QString, const General *> itor(d->generals);
    if (ServerInfo.GameModeStr == QStringLiteral("04_1v3")) {
        QList<const General *> hulao_generals = QList<const General *>();
        foreach (QString pack_name, config(QStringLiteral("hulao_packages")).toStringList()) {
            const Package *pack = findPackage(pack_name);
            if (pack != nullptr) {
                foreach (const General *general, pack->generals())
                    hulao_generals << general;
            }
        }

        foreach (const General *general, hulao_generals) {
            if (general->isHidden() || general->isTotallyHidden() || general->name() == QStringLiteral("yuyuko_1v3"))
                continue;
            general_names << general->name();
        }
    } else {
        while (itor.hasNext()) {
            itor.next();
            if (!itor.value()->isHidden() && ServerInfo.EnabledPackages.contains(itor.value()->getPackage()))
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

QSet<QString> Engine::latestGeneralNames() const
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
