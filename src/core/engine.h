#ifndef _ENGINE_H
#define _ENGINE_H

#include "RoomObject.h"
#include "card.h"
#include "exppattern.h"
#include "general.h"
#include "package.h"
#include "protocol.h"
#include "skill.h"
#include "util.h"

#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QMutex>
#include <QStringList>
#include <QThread>

class AI;
class LuaBasicCard;
class LuaTrickCard;
class LuaWeapon;
class LuaArmor;
class LuaTreasure;
class QVersionNumber;

struct lua_State;

class Engine : public QObject
{
    Q_OBJECT

public:
    Engine();
    ~Engine();

    void addTranslationEntry(const char *key, const char *value);
    QString translate(const QString &to_translate, bool addHegemony = false) const;
    lua_State *getLuaState() const;

    void addPackage(Package *package);
    void addBanPackage(const QString &package_name);
    QStringList getBanPackages() const;
    Card *cloneCard(const Card *card) const;
    Card *cloneCard(const QString &name, Card::Suit suit = Card::SuitToBeDecided, int number = -1, const QStringList &flags = QStringList()) const;
    SkillCard *cloneSkillCard(const QString &name) const;
    QString getVersionNumber() const;
    QString getVersion() const;
    QString getVersionName() const;
    QVersionNumber getQVersionNumber() const;
    QString getMODName() const;
    QStringList getExtensions() const;
    QStringList getKingdoms() const;
    QStringList getHegemonyKingdoms() const;
    QColor getKingdomColor(const QString &kingdom) const;
    QStringList getChattingEasyTexts() const;
    QString getSetupString() const;

    QMap<QString, QString> getAvailableModes() const;
    QString getModeName(const QString &mode) const;
    int getPlayerCount(const QString &mode) const;
    QString getRoles(const QString &mode) const;
    QStringList getRoleList(const QString &mode) const;
    int getRoleIndex() const;

    const CardPattern *getPattern(const QString &name) const;
    bool matchExpPattern(const QString &pattern, const Player *player, const Card *card) const;
    Card::HandlingMethod getCardHandlingMethod(const QString &method_name) const;
    QList<const Skill *> getRelatedSkills(const QString &skill_name) const;
    const Skill *getMainSkill(const QString &skill_name) const;

    void addPackage(const QString &name);
    QList<const Package *> getPackages() const;

    const General *getGeneral(const QString &name) const;
    const QList<QString> getGenerals() const;
    int getGeneralCount(bool include_banned = false) const;
    const Skill *getSkill(const QString &skill_name) const;
    const Skill *getSkill(const EquipCard *card) const;
    QStringList getSkillNames() const;
    const TriggerSkill *getTriggerSkill(const QString &skill_name) const;
    const ViewAsSkill *getViewAsSkill(const QString &skill_name) const;
    QList<const DistanceSkill *> getDistanceSkills() const;
    QList<const MaxCardsSkill *> getMaxCardsSkills() const;
    QList<const TargetModSkill *> getTargetModSkills() const;
    QList<const AttackRangeSkill *> getAttackRangeSkills() const;
    QList<const TriggerSkill *> getGlobalTriggerSkills() const;
    QList<const ViewAsSkill *> getViewAsSkills() const;
    void addSkills(const QList<const Skill *> &skills);

    int getCardCount() const;
    const Card *getEngineCard(int cardId) const;
    // @todo: consider making this const Card *
    Card *getCard(int cardId);
    WrappedCard *getWrappedCard(int cardId);

    QStringList getLords(bool contain_banned = false) const;
    QStringList getRandomLords() const;
    void banRandomGods() const;
    QStringList getRandomGenerals(int count, const QSet<QString> &ban_set = QSet<QString>()) const;
    QStringList getLatestGenerals(const QSet<QString> &ban_set = QSet<QString>()) const;
    QList<int> getRandomCards() const;
    QString getRandomGeneralName() const;
    QStringList getLimitedGeneralNames() const;

    void playSystemAudioEffect(const QString &name) const;
    void playAudioEffect(const QString &filename) const;
    void playSkillAudioEffect(const QString &skill_name, int index) const;

    const ProhibitSkill *isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;
    const ViewHasSkill *ViewHas(const Player *player, const QString &skill_name, const QString &flag, bool ignore_preshow = false) const;
    int correctDistance(const Player *from, const Player *to) const;
    int correctMaxCards(const Player *target, bool fixed = false, const QString &except = QString()) const;
    int correctCardTarget(const TargetModSkill::ModType type, const Player *from, const Card *card) const;
    int correctAttackRange(const Player *target, bool include_weapon = true, bool fixed = false) const;

    void registerRoom(QObject *room);
    void unregisterRoom();
    QObject *currentRoomObject();
    Room *currentRoom();
    RoomObject *currentRoomState();

    QString getCurrentCardUsePattern();
    CardUseStruct::CardUseReason getCurrentCardUseReason();

    bool isGeneralHidden(const QString &general_name) const;

    QStringList LordBGMConvertList;
    QStringList LordBackdropConvertList;
    QStringList LatestGeneralList;
    int operationTimeRate(QSanProtocol::CommandType command, QVariant msg);

    QString GetMappedKingdom(const QString &role); //hegemony

private:
    QMutex m_mutex;
    QHash<QString, QString> translations;
    QHash<QString, const General *> generals;
    QHash<QString, const QMetaObject *> metaobjects;
    QHash<QString, QString> className2objectName;
    QHash<QString, const Skill *> skills;
    QHash<QThread *, QObject *> m_rooms;
    QMap<QString, QString> modes;
    QMultiMap<QString, QString> related_skills;
    mutable QMap<QString, const CardPattern *> patterns;

    // special skills
    QList<const ProhibitSkill *> prohibit_skills;
    QList<const DistanceSkill *> distance_skills;
    QList<const ViewHasSkill *> viewhas_skills;
    QList<const MaxCardsSkill *> maxcards_skills;
    QList<const TargetModSkill *> targetmod_skills;
    QList<const AttackRangeSkill *> attackrange_skills;
    QList<const TriggerSkill *> global_trigger_skills;
    QList<const ViewAsSkill *> viewas_skills;

    QList<Card *> cards;
    QStringList lord_list;
    QSet<QString> ban_package;

    lua_State *lua;

    QHash<QString, QString> luaBasicCard_className2objectName;
    QHash<QString, const LuaBasicCard *> luaBasicCards;
    QHash<QString, QString> luaTrickCard_className2objectName;
    QHash<QString, const LuaTrickCard *> luaTrickCards;
    QHash<QString, QString> luaWeapon_className2objectName;
    QHash<QString, const LuaWeapon *> luaWeapons;
    QHash<QString, QString> luaArmor_className2objectName;
    QHash<QString, const LuaArmor *> luaArmors;
    QHash<QString, QString> luaTreasure_className2objectName;
    QHash<QString, const LuaTreasure *> luaTreasures;
};

class SurrenderCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SurrenderCard();
    void onUse(Room *room, const CardUseStruct &use) const;
};

class CheatCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE CheatCard();
    void onUse(Room *room, const CardUseStruct &use) const;
};

static inline QVariant GetConfigFromLuaState(lua_State *L, const char *key)
{
    return GetValueFromLuaState(L, "config", key);
}

extern Engine *Sanguosha;

#endif
