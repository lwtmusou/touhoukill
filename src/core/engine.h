#ifndef _ENGINE_H
#define _ENGINE_H

#include "RoomObject.h"
#include "card.h"
#include "CardFace.h"
#include "exppattern.h"
#include "general.h"
#include "json.h"
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
class QVersionNumber;
class CardFace;

class Engine : public QObject
{
    Q_OBJECT

public:
    Engine();
    ~Engine() override;

    void loadTranslations(const QString &locale);
    void addTranslationEntry(const QString &key, const QString &value);
    QString translate(const QString &to_translate, bool addHegemony = false) const;

    void addPackage(Package *package);
    void addBanPackage(const QString &package_name);
    QStringList getBanPackages() const;
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

    // currently only used in Card (Pre-Refactor version)
    void registerRoom(RoomObject *room);
    void unregisterRoom();
    RoomObject *currentRoomObject();

    bool isGeneralHidden(const QString &general_name) const;

    QStringList LordBGMConvertList;
    QStringList LordBackdropConvertList;
    QStringList LatestGeneralList;
    int operationTimeRate(QSanProtocol::CommandType command, QVariant msg);

    QString GetMappedKingdom(const QString &role); //hegemony

    QVariant getConfigFromConfigFile(const QString &key) const;

    // Refactoring
    void registerCardFace(const CardFace *cardFace);
    const CardFace *getCardFace(const QString &name) const;

    QString getPackageNameByCard(const Card *c) const; 

private:
    QMutex m_mutex;
    QHash<QString, QString> translations;
    QHash<QString, const General *> generals;
    QHash<QString, const Skill *> skills;
    QHash<QString, const CardFace *> cardFaces;
    QHash<QThread *, RoomObject *> m_rooms;
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

    JsonObject configFile;
};

class SurrenderCard : public SkillCard
{
    Q_GADGET

public:
    Q_INVOKABLE SurrenderCard();

    QString name() const override {
        return staticMetaObject.className();
    }
    void onUse(Room *room, const CardUseStruct &use) const override;
};

class CheatCard : public SkillCard
{
    Q_GADGET

public:
    Q_INVOKABLE CheatCard();

    QString name() const override {
        return staticMetaObject.className();
    }
    void onUse(Room *room, const CardUseStruct &use) const override;
};

extern Engine *Sanguosha;

#endif
