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
