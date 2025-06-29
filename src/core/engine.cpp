#include "engine.h"
#include "RoomState.h"
#include "ai.h"
#include "audio.h"
#include "card.h"
#include "client.h"
#include "lua-wrapper.h"
#include "lua.hpp"
#include "protocol.h"
#include "settings.h"
#include "structs.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QGlobalStatic>
#include <QMessageBox>
#include <QStringList>
#include <QTextStream>
#include <QVersionNumber>

Q_GLOBAL_STATIC(Engine, EngineInstance)

void Engine::addPackage(const QString &name)
{
    Package *pack = PackageAdder::packages()[name];
    if (pack != nullptr)
        addPackage(pack);
    else
        qWarning("Package %s cannot be loaded!", qPrintable(name));
}

Engine::Engine()
{
    lua = CreateLuaState();
}

void Engine::init()
{
    DoLuaScript(lua, "lua/config.lua");

    QStringList package_names = GetConfigFromLuaState(lua, "package_names").toStringList();
    foreach (const QString &name, package_names)
        addPackage(name);

    metaobjects.insert(SurrenderCard::staticMetaObject.className(), &SurrenderCard::staticMetaObject);
    metaobjects.insert(CheatCard::staticMetaObject.className(), &CheatCard::staticMetaObject);

    LordBGMConvertList = GetConfigFromLuaState(lua, "bgm_convert_pairs").toStringList();
    LordBackdropConvertList = GetConfigFromLuaState(lua, "backdrop_convert_pairs").toStringList();
    LatestGeneralList = GetConfigFromLuaState(lua, "latest_generals").toStringList();

    DoLuaScript(lua, "lua/sanguosha.lua");

    // available game modes
    modes["02p"] = tr("2 players");
    modes["03p"] = tr("3 players");
    modes["04p"] = tr("4 players");
    modes["05p"] = tr("5 players");
    modes["06p"] = tr("6 players");
    modes["06pd"] = tr("6 players (2 renegades)");
    modes["07p"] = tr("7 players");
    modes["08p"] = tr("8 players");
    modes["08pd"] = tr("8 players (2 renegades)");
    modes["08pz"] = tr("8 players (0 renegade)");
    modes["09p"] = tr("9 players");
    modes["10pd"] = tr("10 players");
    modes["10p"] = tr("10 players (1 renegade)");
    modes["10pz"] = tr("10 players (0 renegade)");
    modes["hegemony_02"] = tr("hegemony 2 players");
    modes["hegemony_03"] = tr("hegemony 3 players");
    modes["hegemony_04"] = tr("hegemony 4 players");
    modes["hegemony_05"] = tr("hegemony 5 players");
    modes["hegemony_06"] = tr("hegemony 6 players");
    modes["hegemony_07"] = tr("hegemony 7 players");
    modes["hegemony_08"] = tr("hegemony 8 players");
    modes["hegemony_09"] = tr("hegemony 9 players");
    modes["hegemony_10"] = tr("hegemony 10 players");

    modes["03_1v2"] = tr("Peasants vs Landlord");
    modes["04_2v2"] = tr("contest 2v2");

    foreach (const Skill *skill, skills) {
        Skill *mutable_skill = const_cast<Skill *>(skill);
        mutable_skill->initMediaSource();
    }
}

lua_State *Engine::getLuaState() const
{
    return lua;
}

void Engine::addTranslationEntry(const char *key, const char *value)
{
    translations.insert(key, QString::fromUtf8(value));
}

Engine::~Engine()
{
    lua_close(lua);
#ifdef AUDIO_SUPPORT
    Audio::quit();
#endif
}

void Engine::addSkills(const QList<const Skill *> &all_skills)
{
    foreach (const Skill *skill, all_skills) {
        if (skills.contains(skill->objectName()))
            QMessageBox::warning(nullptr, "", tr("Duplicated skill : %1").arg(skill->objectName()));

        skills.insert(skill->objectName(), skill);

        if (skill->inherits("ProhibitSkill"))
            prohibit_skills << qobject_cast<const ProhibitSkill *>(skill);
        else if (skill->inherits("ViewHasSkill"))
            viewhas_skills << qobject_cast<const ViewHasSkill *>(skill);
        else if (skill->inherits("DistanceSkill"))
            distance_skills << qobject_cast<const DistanceSkill *>(skill);
        else if (skill->inherits("MaxCardsSkill"))
            maxcards_skills << qobject_cast<const MaxCardsSkill *>(skill);
        else if (skill->inherits("TargetModSkill"))
            targetmod_skills << qobject_cast<const TargetModSkill *>(skill);
        else if (skill->inherits("AttackRangeSkill"))
            attackrange_skills << qobject_cast<const AttackRangeSkill *>(skill);
        else if (skill->inherits("TriggerSkill")) {
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
            if ((trigger_skill != nullptr) && trigger_skill->isGlobal())
                global_trigger_skills << trigger_skill;
        } else if (skill->inherits("ViewAsSkill"))
            viewas_skills << qobject_cast<const ViewAsSkill *>(skill);
    }
}

QList<const DistanceSkill *> Engine::getDistanceSkills() const
{
    return distance_skills;
}

QList<const MaxCardsSkill *> Engine::getMaxCardsSkills() const
{
    return maxcards_skills;
}

QList<const TargetModSkill *> Engine::getTargetModSkills() const
{
    return targetmod_skills;
}

QList<const AttackRangeSkill *> Engine::getAttackRangeSkills() const
{
    return attackrange_skills;
}

QList<const TriggerSkill *> Engine::getGlobalTriggerSkills() const
{
    return global_trigger_skills;
}

QList<const ViewAsSkill *> Engine::getViewAsSkills() const
{
    return viewas_skills;
}

void Engine::addPackage(Package *package)
{
    if (findChild<const Package *>(package->objectName()) != nullptr)
        return;

    package->setParent(this);
    patterns.unite(package->getPatterns());
    related_skills.unite(package->getRelatedSkills());

    QList<Card *> all_cards = package->findChildren<Card *>();
    foreach (Card *card, all_cards) {
        card->setId(cards.length());
        cards << card;

        if (card->isKindOf("LuaBasicCard")) {
            const LuaBasicCard *lcard = qobject_cast<const LuaBasicCard *>(card);
            Q_ASSERT(lcard != nullptr);
            luaBasicCard_className2objectName.insert(lcard->getClassName(), lcard->objectName());
            if (!luaBasicCards.contains(lcard->getClassName()))
                luaBasicCards.insert(lcard->getClassName(), lcard->clone());
        } else if (card->isKindOf("LuaTrickCard")) {
            const LuaTrickCard *lcard = qobject_cast<const LuaTrickCard *>(card);
            Q_ASSERT(lcard != nullptr);
            luaTrickCard_className2objectName.insert(lcard->getClassName(), lcard->objectName());
            if (!luaTrickCards.contains(lcard->getClassName()))
                luaTrickCards.insert(lcard->getClassName(), lcard->clone());
        } else if (card->isKindOf("LuaWeapon")) {
            const LuaWeapon *lcard = qobject_cast<const LuaWeapon *>(card);
            Q_ASSERT(lcard != nullptr);
            luaWeapon_className2objectName.insert(lcard->getClassName(), lcard->objectName());
            if (!luaWeapons.contains(lcard->getClassName()))
                luaWeapons.insert(lcard->getClassName(), lcard->clone());
        } else if (card->isKindOf("LuaArmor")) {
            const LuaArmor *lcard = qobject_cast<const LuaArmor *>(card);
            Q_ASSERT(lcard != nullptr);
            luaArmor_className2objectName.insert(lcard->getClassName(), lcard->objectName());
            if (!luaArmors.contains(lcard->getClassName()))
                luaArmors.insert(lcard->getClassName(), lcard->clone());
        } /* else if (card->isKindOf("LuaTreasure")) {
            const LuaTreasure *lcard = qobject_cast<const LuaTreasure *>(card);
            Q_ASSERT(lcard != NULL);
            luaTreasure_className2objectName.insert(lcard->getClassName(), lcard->objectName());
            if (!luaTreasures.keys().contains(lcard->getClassName()))
            luaTreasures.insert(lcard->getClassName(), lcard->clone());
            }*/
        else {
            QString class_name = card->metaObject()->className();
            metaobjects.insert(class_name, card->metaObject());
            className2objectName.insert(class_name, card->objectName());
        }
    }

    addSkills(package->getSkills());

    QList<General *> all_generals = package->findChildren<General *>();
    foreach (General *general, all_generals) {
        addSkills(general->findChildren<const Skill *>());
        foreach (const QString &skill_name, general->getExtraSkillSet()) {
            if (skill_name.startsWith("#"))
                continue;
            foreach (const Skill *related, getRelatedSkills(skill_name))
                general->addSkill(related->objectName());
        }
        generals.insert(general->objectName(), general);
        if (isGeneralHidden(general->objectName()))
            continue;
        if (general->isLord())
            lord_list << general->objectName();
    }

    QList<const QMetaObject *> metas = package->getMetaObjects();
    foreach (const QMetaObject *meta, metas)
        metaobjects.insert(meta->className(), meta);
}

void Engine::addBanPackage(const QString &package_name)
{
    ban_package.insert(package_name);
}

QStringList Engine::getBanPackages() const
{
    if (qApp->arguments().contains("-server"))
        return Config.BanPackages;
    else {
        if (isHegemonyGameMode(ServerInfo.GameMode)) { // && ServerInfo.Enable2ndGeneral
            QStringList ban;
            QList<const Package *> packs = getPackages();
            QStringList needPacks;
            needPacks << "hegemonyGeneral"
                      << "hegemony_card";
            foreach (const Package *pa, packs) {
                if (!needPacks.contains(pa->objectName()))
                    ban << pa->objectName();
            }
            return ban;
        } else {
            QStringList ban = ban_package.toList();
            if (!ban.contains("hegemonyGeneral"))
                ban << "hegemonyGeneral";
            if (!ban.contains("hegemony_card"))
                ban << "hegemony_card";

            if (ServerInfo.GameMode == "03_1v2" || ServerInfo.GameMode == "04_2v2")
                ban << "wash_out"
                    << "touhougod";
            return ban;
        }
    }
}

QList<const Package *> Engine::getPackages() const
{
    return findChildren<const Package *>();
}

QString Engine::translate(const QString &to_translate, bool addHegemony) const
{
    QStringList list = to_translate.split("\\");
    QString res;
    foreach (const QString &str, list) {
        if (addHegemony && !str.endsWith("_hegemony")) {
            QString strh = str + "_hegemony";
            if (translations.contains(strh))
                res.append(translations.value(strh, strh));
            else
                res.append(translations.value(str, str));
        } else
            res.append(translations.value(str, str));
    }

    return res;
}

int Engine::getRoleIndex() const
{
    if (ServerInfo.GameMode == "06_3v3" || ServerInfo.GameMode == "06_XMode") {
        return 4;
    } else if (isHegemonyGameMode(ServerInfo.GameMode))
        return 5;
    else
        return 1;
}

const CardPattern *Engine::getPattern(const QString &name) const
{
    const CardPattern *ptn = patterns.value(name, NULL);
    if (ptn != nullptr)
        return ptn;

    ExpPattern *expptn = new ExpPattern(name);
    patterns.insert(name, expptn);
    return expptn;
}

bool Engine::matchExpPattern(const QString &pattern, const Player *player, const Card *card) const
{
    ExpPattern p(pattern);
    return p.match(player, card);
}

Card::HandlingMethod Engine::getCardHandlingMethod(const QString &method_name) const
{
    if (method_name == "use")
        return Card::MethodUse;
    else if (method_name == "response")
        return Card::MethodResponse;
    else if (method_name == "discard")
        return Card::MethodDiscard;
    else if (method_name == "recast")
        return Card::MethodRecast;
    else if (method_name == "pindian")
        return Card::MethodPindian;
    else {
        Q_ASSERT(false);
        return Card::MethodNone;
    }
}

QList<const Skill *> Engine::getRelatedSkills(const QString &skill_name) const
{
    QList<const Skill *> skills;
    foreach (const QString &name, related_skills.values(skill_name))
        skills << getSkill(name);

    return skills;
}

const Skill *Engine::getMainSkill(const QString &skill_name) const
{
    const Skill *skill = getSkill(skill_name);
    if ((skill == nullptr) || skill->isVisible() || related_skills.contains(skill_name))
        return skill;
    foreach (const QString &key, related_skills.keys()) {
        foreach (const QString &name, related_skills.values(key))
            if (name == skill_name)
                return getSkill(key);
    }
    return skill;
}

const General *Engine::getGeneral(const QString &name) const
{
    return generals.value(name, NULL);
}

const QStringList Engine::getGenerals() const
{
    return generals.keys();
}

int Engine::getGeneralCount(bool include_banned) const
{
    if (include_banned)
        return generals.size();

    int total = generals.size();
    QHashIterator<QString, const General *> itor(generals);
    while (itor.hasNext()) {
        itor.next();
        const General *general = itor.value();
        if (getBanPackages().contains(general->getPackage()))
            total--;
        else if (isGeneralHidden(general->objectName()))
            total--;
        else if (isNormalGameMode(ServerInfo.GameMode) && Config.value("Banlist/Roles").toStringList().contains(general->objectName()))
            total--;
        else if (ServerInfo.GameMode == "04_1v3" && Config.value("Banlist/HulaoPass").toStringList().contains(general->objectName()))
            total--;
    }

    return total;
}

void Engine::registerRoom(QObject *room)
{
    m_mutex.lock();
    m_rooms[QThread::currentThread()] = room;
    m_mutex.unlock();
}

void Engine::unregisterRoom()
{
    m_mutex.lock();
    m_rooms.remove(QThread::currentThread());
    m_mutex.unlock();
}

QObject *Engine::currentRoomObject()
{
    m_mutex.lock();
    QObject *room = m_rooms[QThread::currentThread()];
    Q_ASSERT(room);
    m_mutex.unlock();
    return room;
}

Room *Engine::currentRoom()
{
    QObject *roomObject = currentRoomObject();
    Room *room = qobject_cast<Room *>(roomObject);
    Q_ASSERT(room != nullptr);
    return room;
}

RoomState *Engine::currentRoomState()
{
    QObject *roomObject = currentRoomObject();
    Room *room = qobject_cast<Room *>(roomObject);
    if (room != nullptr) {
        return room->getRoomState();
    } else {
        Client *client = qobject_cast<Client *>(roomObject);
        Q_ASSERT(client != nullptr);
        return client->getRoomState();
    }
}

QString Engine::getCurrentCardUsePattern()
{
    return currentRoomState()->getCurrentCardUsePattern();
}

CardUseStruct::CardUseReason Engine::getCurrentCardUseReason()
{
    return currentRoomState()->getCurrentCardUseReason();
}

bool Engine::isGeneralHidden(const QString &general_name) const
{
    const General *general = getGeneral(general_name);
    if (general == nullptr)
        return false;
    if (!general->isVisible())
        return false;
    if (!general->isHidden())
        return Config.ExtraHiddenGenerals.contains(general_name);
    else
        return !Config.RemovedHiddenGenerals.contains(general_name);
}

WrappedCard *Engine::getWrappedCard(int cardId)
{
    Card *card = getCard(cardId);
    WrappedCard *wrappedCard = qobject_cast<WrappedCard *>(card);
    Q_ASSERT(wrappedCard != nullptr && wrappedCard->getId() == cardId);
    return wrappedCard;
}

Card *Engine::getCard(int cardId)
{
    Card *card = nullptr;
    if (cardId < 0 || cardId >= cards.length())
        return nullptr;
    QObject *room = currentRoomObject();
    Q_ASSERT(room);
    Room *serverRoom = qobject_cast<Room *>(room);
    if (serverRoom != nullptr) {
        card = serverRoom->getCard(cardId);
    } else {
        Client *clientRoom = qobject_cast<Client *>(room);
        Q_ASSERT(clientRoom != nullptr);
        card = clientRoom->getCard(cardId);
    }
    Q_ASSERT(card);
    return card;
}

const Card *Engine::getEngineCard(int cardId) const
{
    if (cardId == Card::S_UNKNOWN_CARD_ID)
        return nullptr;
    else if (cardId < 0 || cardId >= cards.length()) {
        //Q_ASSERT(FALSE);
        Q_ASSERT(!(cardId < 0 || cardId >= cards.length()));
        return nullptr;
    } else {
        Q_ASSERT(cards[cardId] != NULL);
        return cards[cardId];
    }
}

Card *Engine::cloneCard(const Card *card) const
{
    Q_ASSERT(card->metaObject() != nullptr);
    QString name = card->metaObject()->className();
    Card *result = cloneCard(name, card->getSuit(), card->getNumber(), card->getFlags());
    if (result == nullptr)
        return nullptr;
    result->setId(card->getEffectiveId());
    result->setSkillName(card->getSkillName(false));
    result->setObjectName(card->objectName());
    return result;
}

Card *Engine::cloneCard(const QString &name, Card::Suit suit, int number, const QStringList &flags) const
{
    Card *card = nullptr;
    if (luaBasicCard_className2objectName.contains(name)) {
        const LuaBasicCard *lcard = luaBasicCards.value(name, NULL);
        if (lcard == nullptr)
            return nullptr;
        card = lcard->clone(suit, number);
    } else if (luaBasicCard_className2objectName.values().contains(name)) {
        QString class_name = luaBasicCard_className2objectName.key(name, name);
        const LuaBasicCard *lcard = luaBasicCards.value(class_name, NULL);
        if (lcard == nullptr)
            return nullptr;
        card = lcard->clone(suit, number);
    } else if (luaTrickCard_className2objectName.contains(name)) {
        const LuaTrickCard *lcard = luaTrickCards.value(name, NULL);
        if (lcard == nullptr)
            return nullptr;
        card = lcard->clone(suit, number);
    } else if (luaTrickCard_className2objectName.values().contains(name)) {
        QString class_name = luaTrickCard_className2objectName.key(name, name);
        const LuaTrickCard *lcard = luaTrickCards.value(class_name, NULL);
        if (lcard == nullptr)
            return nullptr;
        card = lcard->clone(suit, number);
    } else if (luaWeapon_className2objectName.contains(name)) {
        const LuaWeapon *lcard = luaWeapons.value(name, NULL);
        if (lcard == nullptr)
            return nullptr;
        card = lcard->clone(suit, number);
    } else if (luaWeapon_className2objectName.values().contains(name)) {
        QString class_name = luaWeapon_className2objectName.key(name, name);
        const LuaWeapon *lcard = luaWeapons.value(class_name, NULL);
        if (lcard == nullptr)
            return nullptr;
        card = lcard->clone(suit, number);
    } else if (luaArmor_className2objectName.contains(name)) {
        const LuaArmor *lcard = luaArmors.value(name, NULL);
        if (lcard == nullptr)
            return nullptr;
        card = lcard->clone(suit, number);
    } else if (luaArmor_className2objectName.values().contains(name)) {
        QString class_name = luaArmor_className2objectName.key(name, name);
        const LuaArmor *lcard = luaArmors.value(class_name, NULL);
        if (lcard == nullptr)
            return nullptr;
        card = lcard->clone(suit, number);
    } else if (luaTreasure_className2objectName.contains(name)) {
        const LuaTreasure *lcard = luaTreasures.value(name, NULL);
        if (lcard == nullptr)
            return nullptr;
        card = lcard->clone(suit, number);
    } else if (luaTreasure_className2objectName.values().contains(name)) {
        QString class_name = luaTreasure_className2objectName.key(name, name);
        const LuaTreasure *lcard = luaTreasures.value(class_name, NULL);
        if (lcard == nullptr)
            return nullptr;
        card = lcard->clone(suit, number);
    } else {
        const QMetaObject *meta = metaobjects.value(name, NULL);
        if (meta == nullptr)
            meta = metaobjects.value(className2objectName.key(name, QString()), NULL);
        if (meta != nullptr) {
            QObject *card_obj = meta->newInstance(Q_ARG(Card::Suit, suit), Q_ARG(int, number));
            card_obj->setObjectName(className2objectName.value(name, name));
            card = qobject_cast<Card *>(card_obj);
        }
    }
    if (card == nullptr)
        return nullptr;
    card->clearFlags();
    if (!flags.isEmpty()) {
        foreach (const QString &flag, flags)
            card->setFlags(flag);
    }
    return card;
}

SkillCard *Engine::cloneSkillCard(const QString &name) const
{
    const QMetaObject *meta = metaobjects.value(name, NULL);
    if (meta != nullptr) {
        QObject *card_obj = meta->newInstance();
        SkillCard *card = qobject_cast<SkillCard *>(card_obj);
        if (card == nullptr)
            delete card_obj;
        return card;
    } else
        return nullptr;
}

QString Engine::getVersionNumber() const
{
    return QSGS_VERSIONNUMBER;
}

QString Engine::getVersion() const
{
    return QString("%1:%2").arg(getVersionNumber(), getMODName());
}

QString Engine::getVersionName() const
{
    return "V" QSGS_VERSION;
}

QVersionNumber Engine::getQVersionNumber() const
{
    return QVersionNumber::fromString(QSGS_VERSION);
}

QString Engine::getMODName() const
{
    return "TouhouSatsu";
}

QStringList Engine::getExtensions() const
{
    QStringList extensions;
    QList<const Package *> packages = findChildren<const Package *>();
    foreach (const Package *package, packages)
        extensions << package->objectName();

    return extensions;
}

QStringList Engine::getKingdoms() const
{
    static QStringList kingdoms;

    if (kingdoms.isEmpty()) {
        kingdoms = GetConfigFromLuaState(lua, "kingdoms").toStringList();
    }
    return kingdoms;
}

QStringList Engine::getHegemonyKingdoms() const
{
    static QStringList hegemony_kingdoms;
    if (hegemony_kingdoms.isEmpty()) {
        hegemony_kingdoms = GetConfigFromLuaState(lua, "hegemony_kingdoms").toStringList();
    }
    return hegemony_kingdoms;
}

QColor Engine::getKingdomColor(const QString &kingdom) const
{
    static QMap<QString, QColor> color_map;
    if (color_map.isEmpty()) {
        QVariantMap map = GetValueFromLuaState(lua, "config", "kingdom_colors").toMap();
        QMapIterator<QString, QVariant> itor(map);
        while (itor.hasNext()) {
            itor.next();
            QColor color(itor.value().toString());
            if (!color.isValid()) {
                qWarning("Invalid color for kingdom %s", qPrintable(itor.key()));
                color = QColor(128, 128, 128);
            }
            color_map[itor.key()] = color;
        }

        Q_ASSERT(!color_map.isEmpty());
    }

    return color_map.value(kingdom);
}

QStringList Engine::getChattingEasyTexts() const
{
    static QStringList easy_texts;
    if (easy_texts.isEmpty())
        easy_texts = GetConfigFromLuaState(lua, "easy_text").toStringList();

    return easy_texts;
}

QString Engine::getSetupString() const
{
    int timeout = Config.OperationNoLimit ? 0 : Config.OperationTimeout;
    QString flags;
    if (Config.RandomSeat)
        flags.append("R");
    if (Config.EnableCheat)
        flags.append("C");
    if (Config.EnableCheat && Config.FreeChoose)
        flags.append("F");
    if (Config.Enable2ndGeneral || isHegemonyGameMode(Config.GameMode))
        flags.append("S");
    if (Config.EnableSame)
        flags.append("T");
    if (Config.EnableAI)
        flags.append("A");
    if (Config.DisableChat)
        flags.append("M");

    if (Config.MaxHpScheme == 1)
        flags.append("1");
    else if (Config.MaxHpScheme == 2)
        flags.append("2");
    else if (Config.MaxHpScheme == 3)
        flags.append("3");
    else if (Config.MaxHpScheme == 0) {
        char c = Config.Scheme0Subtraction + 5 + 'a'; // from -5 to 12
        flags.append(c);
    }

    QString server_name = Config.ServerName.toUtf8().toBase64();
    QStringList setup_items;
    QString mode = Config.GameMode;
    if (mode == "02_1v1")
        mode = mode + Config.value("1v1/Rule", "2013").toString();
    else if (mode == "06_3v3")
        mode = mode + Config.value("3v3/OfficialRule", "2013").toString();
    setup_items << server_name << Config.GameMode << QString::number(timeout) << QString::number(Config.NullificationCountDown) << Sanguosha->getBanPackages().join("+") << flags;

    return setup_items.join(":");
}

QMap<QString, QString> Engine::getAvailableModes() const
{
    return modes;
}

QString Engine::getModeName(const QString &mode) const
{
    if (modes.contains(mode))
        return modes.value(mode);

    return QString();
}

int Engine::getPlayerCount(const QString &mode) const
{
    if (isHegemonyGameMode(mode)) {
        QStringList modestrings = mode.split("_");
        return modestrings.last().toInt(nullptr, 10); //return 2;
    }

    if (modes.contains(mode)) {
        QRegExp rx("(\\d+)");
        int index = rx.indexIn(mode);
        if (index != -1)
            return rx.capturedTexts().constFirst().toInt();
    }

    return -1;
}

QString Engine::getRoles(const QString &mode) const
{
    int n = getPlayerCount(mode);

    if (mode == "02_1v1") {
        return "ZN";
    } else if (mode == "04_1v3") {
        return "ZFFF";
    }
    if (mode == "03_1v2") {
        return "ZFF";
    }
    if (mode == "04_2v2") {
        return "CFFC";
    }

    if (isHegemonyGameMode(mode)) {
        QString role;
        int num = getPlayerCount(mode);
        QStringList roles;
        roles << "W"
              << "S"
              << "G"
              << "Q"; //wei shu wu qun
        for (int i = 0; i < num; ++i) {
            int role_idx = qrand() % roles.length();
            role = role + roles[role_idx];
        }
        return role;
    }

    if (modes.contains(mode)) {
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

        const char **table = mode.endsWith("d") ? table2 : table1;
        QString rolechar = table[n];
        if (mode.endsWith("z"))
            rolechar.replace("N", "C");

        return rolechar;
    } else if (mode.startsWith("@")) {
        if (n == 8)
            return "ZCCCNFFF";
        else if (n == 6)
            return "ZCCNFF";
    }
    return QString();
}

QStringList Engine::getRoleList(const QString &mode) const
{
    QString roles = getRoles(mode);

    QStringList role_list;
    for (int i = 0; roles[i] != '\0'; i++) {
        QString role;
        switch (roles[i].toLatin1()) {
        case 'Z':
            role = "lord";
            break;
        case 'C':
            role = "loyalist";
            break;
        case 'N':
            role = "renegade";
            break;
        case 'F':
            role = "rebel";
            break;
        case 'W':
            role = "wei";
            break;
        case 'S':
            role = "shu";
            break;
        case 'G':
            role = "wu";
            break;
        case 'Q':
            role = "qun";
            break;
        }
        role_list << role;
    }

    return role_list;
}

int Engine::getCardCount() const
{
    return cards.length();
}

QStringList Engine::getLords(bool contain_banned) const
{
    QStringList lords;

    // add intrinsic lord
    foreach (QString lord, lord_list) {
        const General *general = generals.value(lord);
        if (getBanPackages().contains(general->getPackage()))
            continue;
        if (!contain_banned) {
            if (ServerInfo.GameMode.endsWith("p") || ServerInfo.GameMode.endsWith("pd") || ServerInfo.GameMode.endsWith("pz"))
                if (Config.value("Banlist/Roles", "").toStringList().contains(lord))
                    continue;
        }
        lords << lord;
    }

    return lords;
}

QStringList Engine::getRandomLords() const
{
    QStringList banlist_ban;

    if (Config.GameMode == "zombie_mode")
        banlist_ban.append(Config.value("Banlist/Zombie").toStringList());
    else if (isNormalGameMode(Config.GameMode))
        banlist_ban.append(Config.value("Banlist/Roles").toStringList());

    QStringList lords;
    QStringList splords_package; //lords  in sp package will be not count as a lord.
    splords_package << "thndj";

    foreach (QString alord, getLords()) {
        if (banlist_ban.contains(alord))
            continue;
        const General *general = getGeneral(alord);
        if (splords_package.contains(general->getPackage()))
            continue;
        lords << alord;
    }

    int lord_num = Config.value("LordMaxChoice", 6).toInt();
    if (lord_num != -1 && lord_num < lords.length()) {
        int to_remove = lords.length() - lord_num;
        for (int i = 0; i < to_remove; i++) {
            lords.removeAt(qrand() % lords.length());
        }
    }

    QStringList nonlord_list;
    foreach (const QString &nonlord, generals.keys()) {
        if (isGeneralHidden(nonlord))
            continue;
        const General *general = generals.value(nonlord);
        if (lord_list.contains(nonlord)) {
            if (!splords_package.contains(general->getPackage()))
                continue;
        }
        if (getBanPackages().contains(general->getPackage()))
            continue;
        if (banlist_ban.contains(general->objectName()))
            continue;

        nonlord_list << nonlord;
    }

    qShuffle(nonlord_list);

    int i = 0;
    int addcount = 0;
    int extra = Config.value("NonLordMaxChoice", 6).toInt();

    int godmax = Config.value("GodLimit", 1).toInt();
    int godCount = 0;

    if (lord_num == 0 && extra == 0)
        extra = 1;

    bool assign_latest_general = Config.value("AssignLatestGeneral", true).toBool();
    QStringList latest = getLatestGenerals(lords.toSet());
    if (assign_latest_general && !latest.isEmpty()) {
        lords << latest.first();
        if (nonlord_list.contains(latest.first()))
            nonlord_list.removeOne(latest.first());
        extra--;
    }

    for (i = 0; addcount < extra; i++) {
        if (getGeneral(nonlord_list.at(i))->getKingdom() != "touhougod") {
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
}

QStringList Engine::getLimitedGeneralNames() const
{
    QStringList general_names;
    QHashIterator<QString, const General *> itor(generals);
    if (ServerInfo.GameMode == "04_1v3") {
        QList<const General *> hulao_generals = QList<const General *>();
        foreach (const QString &pack_name, GetConfigFromLuaState(lua, "hulao_packages").toStringList()) {
            const Package *pack = Sanguosha->findChild<const Package *>(pack_name);
            if (pack != nullptr)
                hulao_generals << pack->findChildren<const General *>();
        }

        foreach (const General *general, hulao_generals) {
            if (isGeneralHidden(general->objectName()) || general->isTotallyHidden() || general->objectName() == "yuyuko_1v3")
                continue;
            general_names << general->objectName();
        }
    } else {
        while (itor.hasNext()) {
            itor.next();
            if (!isGeneralHidden(itor.value()->objectName()) && !getBanPackages().contains(itor.value()->getPackage()))
                general_names << itor.key();
        }
    }

    return general_names;
}

void Engine::banRandomGods() const
{
    QStringList all_generals = getLimitedGeneralNames();

    qShuffle(all_generals);

    int count = 0;
    int max = Config.value("GodLimit", 1).toInt();

    if (max == -1)
        return;

    QStringList gods;

    foreach (const QString &general, all_generals) {
        if (getGeneral(general)->getKingdom() == "touhougod") {
            gods << general;
            count++;
        }
    };
    int bancount = count - max;
    if (bancount <= 0)
        return;
    QStringList ban_gods = gods.mid(0, bancount);
    Q_ASSERT(ban_gods.count() == bancount);

    QStringList ban_list = Config.value("Banlist/Roles").toStringList();

    ban_list.append(ban_gods);
    Config.setValue("Banlist/Roles", ban_list);
}

QStringList Engine::getRandomGenerals(int count, const QSet<QString> &ban_set) const
{
    QStringList all_generals = getLimitedGeneralNames();
    QSet<QString> general_set = all_generals.toSet();

    Q_ASSERT(all_generals.count() >= count);

    if (isNormalGameMode(ServerInfo.GameMode))
        general_set.subtract(Config.value("Banlist/Roles", "").toStringList().toSet());
    else if (ServerInfo.GameMode == "03_1v2")
        general_set.subtract(Config.value("Banlist/03_1v2", "").toStringList().toSet());
    else if (ServerInfo.GameMode == "04_2v2")
        general_set.subtract(Config.value("Banlist/04_2v2", "").toStringList().toSet());
    else if (ServerInfo.GameMode == "04_1v3")
        general_set.subtract(Config.value("Banlist/HulaoPass", "").toStringList().toSet());
    else if (ServerInfo.GameMode == "06_XMode")
        general_set.subtract(Config.value("Banlist/XMode", "").toStringList().toSet());
    else if (isHegemonyGameMode(ServerInfo.GameMode))
        general_set.subtract(Config.value("Banlist/Hegemony", "").toStringList().toSet());

    all_generals = general_set.subtract(ban_set).toList();

    // shuffle them
    qShuffle(all_generals);

    int addcount = 0;
    QStringList general_list = QStringList();
    int godmax = Config.value("GodLimit", 1).toInt();
    int godCount = 0;
    for (int i = 0; addcount < count; i++) {
        if (getGeneral(all_generals.at(i))->getKingdom() != "touhougod") {
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

    //QStringList general_list = all_generals.mid(0, count);
    //Q_ASSERT(general_list.count() == count);

    return general_list;
}

QStringList Engine::getLatestGenerals(const QSet<QString> &ban_set) const
{
    QSet<QString> general_set = LatestGeneralList.toSet();
    if (isNormalGameMode(ServerInfo.GameMode))
        general_set.subtract(Config.value("Banlist/Roles", "").toStringList().toSet());
    else if (ServerInfo.GameMode == "04_1v3")
        general_set.subtract(Config.value("Banlist/HulaoPass", "").toStringList().toSet());
    else if (ServerInfo.GameMode == "06_XMode")
        general_set.subtract(Config.value("Banlist/XMode", "").toStringList().toSet());
    else if (isHegemonyGameMode(ServerInfo.GameMode))
        general_set.subtract(Config.value("Banlist/Hegemony", "").toStringList().toSet());

    QStringList latest_generals = general_set.subtract(ban_set).toList();
    if (!latest_generals.isEmpty())
        qShuffle(latest_generals);
    return latest_generals;
}

QList<int> Engine::getRandomCards() const
{
    bool exclude_disaters = false, using_2012_3v3 = false, using_2013_3v3 = false;

    if (Config.GameMode == "06_3v3") {
        using_2012_3v3 = (Config.value("3v3/OfficialRule", "2013").toString() == "2012");
        using_2013_3v3 = (Config.value("3v3/OfficialRule", "2013").toString() == "2013");
        //exclude_disaters = !Config.value("3v3/UsingExtension", false).toBool() || Config.value("3v3/ExcludeDisasters", true).toBool();
        exclude_disaters = Config.value("3v3/ExcludeDisasters", true).toBool();
    }

    if (Config.GameMode == "04_1v3")
        exclude_disaters = true;

    QList<int> list;
    foreach (Card *card, cards) {
        card->clearFlags();

        if (exclude_disaters && card->isKindOf("Disaster"))
            continue;

        if (card->getPackage() == "New3v3Card" && (using_2012_3v3 || using_2013_3v3))
            list << card->getId();
        else if (card->getPackage() == "New3v3_2013Card" && using_2013_3v3)
            list << card->getId();

        /* if (Config.GameMode == "02_1v1" && !Config.value("1v1/UsingCardExtension", false).toBool()) {
            if (card->getPackage() == "New1v1Card")
                list << card->getId();
            continue;
        } */

        /* if (Config.GameMode == "06_3v3" && !Config.value("3v3/UsingExtension", false).toBool()
            && card->getPackage() != "standard_cards" && card->getPackage() != "standard_ex_cards")
            continue; */

        if (!getBanPackages().contains(card->getPackage())) {
            if (card->getPackage() == "standard_ex_cards" && (ServerInfo.GameMode == "03_1v2" || ServerInfo.GameMode == "04_2v2")) {
                QStringList ex;
                ex << "IceSword"
                   << "RenwangShield"
                   << "lightning"
                   << "nullification";
                if (ex.contains(card->objectName()))
                    list << card->getId();

                continue;
            }

            if (card->objectName().startsWith("known_both")) {
                if (isHegemonyGameMode(Config.GameMode) && card->objectName() == "known_both_hegemony")
                    list << card->getId();
                else if (!isHegemonyGameMode(Config.GameMode) && card->objectName() == "known_both")
                    list << card->getId();

            } else if (card->objectName().startsWith("DoubleSword")) {
                if (isHegemonyGameMode(Config.GameMode) && card->objectName() == "DoubleSwordHegemony")
                    list << card->getId();
                else if (!isHegemonyGameMode(Config.GameMode) && card->objectName() == "DoubleSword")
                    list << card->getId();
            } else
                list << card->getId();
        }
    }
    // remove two crossbows and one nullification?
    if (using_2012_3v3 || using_2013_3v3)
        list.removeOne(98);
    if (using_2013_3v3) {
        list.removeOne(53);
        list.removeOne(54);
    }

    qShuffle(list);

    return list;
}

QString Engine::getRandomGeneralName() const
{
    return generals.keys().at(qrand() % generals.size());
}

void Engine::playSystemAudioEffect(const QString &name) const
{
    playAudioEffect(QString("audio/system/%1.ogg").arg(name));
}

void Engine::playAudioEffect(const QString &filename) const
{
#ifdef AUDIO_SUPPORT
    if (!Config.EnableEffects)
        return;
    if (filename.isNull())
        return;

    Audio::play(filename);
#else
    Q_UNUSED(filename);
#endif
}

void Engine::playSkillAudioEffect(const QString &skill_name, int index) const
{
    const Skill *skill = skills.value(skill_name, NULL);
    if (skill != nullptr)
        skill->playAudioEffect(index);
}

const Skill *Engine::getSkill(const QString &skill_name) const
{
    return skills.value(skill_name, NULL);
}

const Skill *Engine::getSkill(const EquipCard *equip) const
{
    const Skill *skill = nullptr;
    if (equip == nullptr)
        skill = nullptr;
    else
        skill = /*Sanguosha->*/ getSkill(equip->objectName());

    return skill;
}

QStringList Engine::getSkillNames() const
{
    return skills.keys();
}

const TriggerSkill *Engine::getTriggerSkill(const QString &skill_name) const
{
    const Skill *skill = getSkill(skill_name);
    if (skill != nullptr)
        return qobject_cast<const TriggerSkill *>(skill);
    else
        return nullptr;
}

const ViewAsSkill *Engine::getViewAsSkill(const QString &skill_name) const
{
    const Skill *skill = getSkill(skill_name);
    if (skill == nullptr)
        return nullptr;

    if (skill->inherits("ViewAsSkill"))
        return qobject_cast<const ViewAsSkill *>(skill);
    else if (skill->inherits("TriggerSkill")) {
        const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
        return trigger_skill->getViewAsSkill();
    } else if (skill->inherits("DistanceSkill")) { //for hegemony showskill
        const DistanceSkill *distance_skill = qobject_cast<const DistanceSkill *>(skill);
        return distance_skill->getViewAsSkill();
    } /*else if (skill->inherits("AttackRangeSkill")) {//for hegemony showskill
        const AttackRangeSkill *distance_skill = qobject_cast<const AttackRangeSkill *>(skill);
        return distance_skill->getViewAsSkill();
    }*/
    else if (skill->inherits("MaxCardsSkill")) { //for hegemony showskill
        const MaxCardsSkill *distance_skill = qobject_cast<const MaxCardsSkill *>(skill);
        return distance_skill->getViewAsSkill();
    } else
        return nullptr;
}

const ProhibitSkill *Engine::isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others) const
{
    bool ignore = (from != nullptr && from->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && to != from
                   && !card->hasFlag("IgnoreFailed"));
    if (ignore && !card->isKindOf("SkillCard"))
        return nullptr;
    foreach (const ProhibitSkill *skill, prohibit_skills) {
        if (skill->isProhibited(from, to, card, others))
            return skill;
    }

    return nullptr;
}

const ViewHasSkill *Engine::ViewHas(const Player *player, const QString &skill_name, const QString &flag, bool ignore_preshow) const
{
    foreach (const ViewHasSkill *skill, viewhas_skills) {
        if (skill->ViewHas(player, skill_name, flag, ignore_preshow))
            return skill;
    }

    return nullptr;
}

int Engine::correctDistance(const Player *from, const Player *to) const
{
    int correct = 0;

    foreach (const DistanceSkill *skill, distance_skills) {
        correct += skill->getCorrect(from, to);
    }

    return correct;
}

int Engine::correctMaxCards(const Player *target, bool fixed, const QString &except) const
{
    int extra = 0;

    QStringList exceptlist = except.split("|");

    foreach (const MaxCardsSkill *skill, maxcards_skills) {
        if (exceptlist.contains(skill->objectName()))
            continue;

        if (fixed) {
            int f = skill->getFixed(target);
            if (f >= 0)
                return f;
        } else {
            extra += skill->getExtra(target);
        }
    }

    return extra;
}

int Engine::correctCardTarget(const TargetModSkill::ModType type, const Player *from, const Card *card) const
{
    int x = 0;
    QString cardskill = card->getSkillName();
    bool checkDoubleHidden = false;
    if (cardskill != nullptr)
        checkDoubleHidden = from->isHiddenSkill(cardskill);

    if (type == TargetModSkill::Residue) {
        foreach (const TargetModSkill *skill, targetmod_skills) {
            ExpPattern p(skill->getPattern());
            if (p.match(from, card)) {
                int residue = skill->getResidueNum(from, card);
                if (checkDoubleHidden && from->isHiddenSkill(skill->objectName()) && cardskill != skill->objectName() && !skill->objectName().startsWith("#" + cardskill))
                    continue;
                if (residue >= 998)
                    return residue;
                x += residue;
            }
        }
    } else if (type == TargetModSkill::DistanceLimit) {
        foreach (const TargetModSkill *skill, targetmod_skills) {
            ExpPattern p(skill->getPattern());
            if (p.match(from, card)) {
                int distance_limit = skill->getDistanceLimit(from, card);
                if (checkDoubleHidden && from->isHiddenSkill(skill->objectName()) && cardskill != skill->objectName() && !skill->objectName().startsWith("#" + cardskill))
                    continue;

                if (distance_limit >= 998)
                    return distance_limit;
                x += distance_limit;
            }
        }
    } else if (type == TargetModSkill::ExtraTarget) {
        foreach (const TargetModSkill *skill, targetmod_skills) {
            ExpPattern p(skill->getPattern());
            if (p.match(from, card) && from->getMark("chuangshi_user") == 0) {
                if (checkDoubleHidden && from->isHiddenSkill(skill->objectName()) && cardskill != skill->objectName() && !skill->objectName().startsWith("#" + cardskill))
                    continue;
                x += skill->getExtraTargetNum(from, card);
            }
        }
    }

    return x;
}

int Engine::correctAttackRange(const Player *target, bool include_weapon /* = true */, bool fixed /* = false */) const
{
    int extra = 0;

    foreach (const AttackRangeSkill *skill, attackrange_skills) {
        if (fixed) {
            int f = skill->getFixed(target, include_weapon);
            if (f >= 0)
                return f;
        } else {
            extra += skill->getExtra(target, include_weapon);
        }
    }

    return extra;
}

int Engine::operationTimeRate(QSanProtocol::CommandType command, QVariant msg)
{
    int rate = 2; //default
    JsonArray arg = msg.value<JsonArray>();
    if (command == QSanProtocol::S_COMMAND_RESPONSE_CARD) {
        QString pattern = arg[0].toString();
        if (pattern == "@@qiusuo")
            rate = 4;
    }
    if (command == QSanProtocol::S_COMMAND_EXCHANGE_CARD) {
        QString reason = arg[5].toString();
        if (reason == "qingting")
            rate = 3;
    }
    return rate;
}

SurrenderCard::SurrenderCard()
{
    target_fixed = true;
    mute = true;
    handling_method = Card::MethodNone;
}

void SurrenderCard::onUse(Room *room, const CardUseStruct &use) const
{
    room->makeSurrender(use.from);
}

CheatCard::CheatCard()
{
    target_fixed = true;
    mute = true;
    handling_method = Card::MethodNone;
}

void CheatCard::onUse(Room *room, const CardUseStruct &use) const
{
    QString cheatString = getUserString();
    JsonDocument doc = JsonDocument::fromJson(cheatString.toUtf8().constData());
    if (doc.isValid())
        room->cheat(use.from, doc.toVariant());
}

QString Engine::GetMappedKingdom(const QString &role)
{
    static QMap<QString, QString> kingdoms;
    if (kingdoms.isEmpty()) {
        kingdoms["lord"] = "wei";
        kingdoms["loyalist"] = "shu";
        kingdoms["rebel"] = "wu";
        kingdoms["renegade"] = "qun";
    }
    if (kingdoms[role].isEmpty())
        return role;
    return kingdoms[role];
}

Engine *EngineInstanceFunc()
{
    return EngineInstance;
}
