#include "engine.h"
#include "CardFace.h"
#include "RoomObject.h"
#include "card.h"
#include "exppattern.h"
#include "general.h"
#include "global.h"
#include "jsonutils.h"
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
#include <QGlobalStatic>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
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
    QHash<QString, const Mode *> modes;

    // Package
    QList<const Package *> packages;

    QList<CardDescriptor> cards;
    QSet<QString> lord_list;

    QJsonObject configFile;

    LuaStatePointer l;

    QSet<QString> LatestGeneralList;

    EnginePrivate()
        : l(nullptr)
    {
    }
};

Q_GLOBAL_STATIC(Engine, EngineInstance)

Engine::Engine()
    : d(new EnginePrivate)
{
    // This file should be in qrc
    QJsonParseError error;
    QJsonDocument doc = JsonDocumentFromFilePath(QStringLiteral("config/gameconfig.json"), &error);
    if (error.error == QJsonParseError::NoError)
        d->configFile = doc.object();
}

void Engine::init()
{
    d->l = LuaMultiThreadEnvironment::luaStateForCurrentThread();

    foreach (const Trigger *trigger, LuaMultiThreadEnvironment::triggers())
        registerTrigger(trigger);

    foreach (const CardFace *cardFace, LuaMultiThreadEnvironment::cardFaces())
        registerCardFace(cardFace);

    foreach (const Skill *skill, LuaMultiThreadEnvironment::skills())
        addSkill(skill);

    foreach (const Package *package, LuaMultiThreadEnvironment::packages())
        addPackage(package);

    d->LatestGeneralList = List2Set(configuration(QStringLiteral("latest_generals")).toStringList());

    // I'd like it to refactor to use Qt-builtin way for it
    QString locale = configuration(QStringLiteral("locale")).toString();
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
    qDeleteAll(d->modes);
    delete d;
}

void Engine::loadTranslations(const QString &locale)
{
    QJsonParseError error;
    QJsonDocument doc = JsonDocumentFromFilePath(QStringLiteral("lang/") + locale + QStringLiteral(".json"), &error);
    if (error.error != QJsonParseError::NoError)
        return;
    if (!doc.isArray())
        return;

    QJsonArray jarr = doc.array();

    for (const QJsonValue &fileNameV : jarr) {
        QString fileName = fileNameV.toString();
        QJsonDocument transDoc = JsonDocumentFromFilePath(fileName, &error);
        if ((error.error != QJsonParseError::NoError) || !transDoc.isObject())
            continue;

        QJsonObject ob = transDoc.object();

        for (auto it = ob.constBegin(); it != ob.constEnd(); ++it)
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

QSet<const General *> Engine::generals() const
{
    return List2Set(d->generals.values());
}

QSet<QString> Engine::generalNames() const
{
    return List2Set(d->generals.keys());
}

const CardDescriptor &Engine::cardDescriptor(int cardId) const
{
    static CardDescriptor nullDescriptor = {QString(), NoSuit, NumberNA, nullptr};

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

QString Engine::version() const
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

QSet<QString> Engine::packageNames() const
{
    QSet<QString> extensions;
    const QList<const Package *> &ps = packages();
    foreach (const Package *package, ps)
        extensions.insert(package->name());

    return extensions;
}

QSet<QString> Engine::kingdoms() const
{
    static QSet<QString> kingdoms;

    if (kingdoms.isEmpty())
        kingdoms = List2Set(configuration(QStringLiteral("kingdoms")).toStringList());

    return kingdoms;
}

QSet<QString> Engine::hegemonyKingdoms() const
{
    static QSet<QString> hegemony_kingdoms;
    if (hegemony_kingdoms.isEmpty())
        hegemony_kingdoms = List2Set(configuration(QStringLiteral("hegemony_kingdoms")).toStringList());

    return hegemony_kingdoms;
}

int Engine::cardCount() const
{
    return d->cards.length();
}

QSet<QString> Engine::availableLordNames() const
{
    return d->lord_list;
}

QSet<QString> Engine::latestGeneralNames() const
{
    return d->LatestGeneralList;
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

QVariant Engine::configuration(const QString &key) const
{
    // TODO: special case of "withHeroSkin" and "withBGM"
    return d->configFile.value(key);
}

void Engine::registerCardFace(const CardFace *cardFace)
{
    // ensure that "DummyCard" always match the nullptr return value of cardFace function

    if (cardFace != nullptr) {
        if (cardFace->name() == QStringLiteral("DummyCard"))
            throw std::invalid_argument("DummyCard can't be registered");

        d->faces.insert(cardFace->name(), cardFace);
    }
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

QSet<QString> Engine::availableGameModes() const
{
    return {
        // Role
        QStringLiteral("role_0,1,0"), //  2 players
        QStringLiteral("role_0,1,1"), //  3 players
        QStringLiteral("role_0,2,1"), //  4 players
        QStringLiteral("role_1,2,1"), //  5 players
        QStringLiteral("role_1,3,1"), //  6 players
        QStringLiteral("role_1,2,2"), //  6 players, dual renegades
        QStringLiteral("role_2,3,1"), //  7 players
        QStringLiteral("role_2,4,1"), //  8 players
        QStringLiteral("role_2,3,2"), //  8 players, dual renegades
        QStringLiteral("role_3,4,0"), //  8 players, no renegades
        QStringLiteral("role_3,4,1"), //  9 players
        QStringLiteral("role_3,4,2"), //  10players
        QStringLiteral("role_3,5,1"), //  10players, single renegade
        QStringLiteral("role_4,5,0"), //  10players, no renegades
        // TODO: Shall we support player number > 10 in Role mode?
        // TODO: Shall we support customized Role mode?

        // Hegemony
        QStringLiteral("hegemony_2"), //  2 players
        QStringLiteral("hegemony_3"), //  3 players
        QStringLiteral("hegemony_4"), //  4 players
        QStringLiteral("hegemony_5"), //  5 players
        QStringLiteral("hegemony_6"), //  6 players
        QStringLiteral("hegemony_7"), //  7 players
        QStringLiteral("hegemony_8"), //  8 players
        QStringLiteral("hegemony_9"), //  9 players
        QStringLiteral("hegemony_10"), // 10players
        QStringLiteral("hegemony_11"), // 11players
        QStringLiteral("hegemony_12"), // 12players

        // Others
        // QStringLiteral("1v1"), //      official 1v1 mode
        // QStringLiteral("1v3"), //      official 1v3 mode
        // QStringLiteral("3v3"), //      official 3v3 mode
        // QStringLiteral("3v3x"), //     official 3v3 mode, extreme
        // QStringLiteral("jiange"), //   official 4v4 mode, JianGe Defense

        //         modes["03_1v2"] = tr("Peasants vs Landlord");
        //         modes["04_2v2"] = tr("contest 2v2");

    };
}

const Mode *Engine::gameMode(const QString &name) const
{
    if (d->modes.contains(name))
        return d->modes.value(name);

    Mode *ret = nullptr;

    if (name == QStringLiteral("1v1")) {
        // TODO: create 1v1 mode
    } else if (name == QStringLiteral("1v3")) {
        // TODO: create 1v3 mode
    } else if (name == QStringLiteral("3v3")) {
        // TODO: create 3v3 mode
    } else if (name.startsWith(QStringLiteral("hegemony_"))) {
        if (GenericHegemonyMode::nameMatched(name))
            ret = new GenericHegemonyMode(name);
    } else if (name.startsWith(QStringLiteral("role_"))) {
        if (GenericRoleMode::nameMatched(name))
            ret = new GenericRoleMode(name);
    }

    if (ret != nullptr)
        d->modes[name] = ret;

    return ret;
}

Engine *EngineInstanceFunc()
{
    return EngineInstance;
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
