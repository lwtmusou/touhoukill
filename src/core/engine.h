#ifndef _ENGINE_H
#define _ENGINE_H

#include "global.h"
#include "json.h"
#include "protocol.h"

// TODO: kill this
#include "lua-wrapper.h"

#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QMutex>
#include <QStringList>
#include <QThread>

class QVersionNumber;
class LuaStatePointer;
class CardPattern;
class General;
class ViewAsSkill;
class ProhibitSkill;
class TreatAsEquippingSkill;
class EquipCard;
class TargetModSkill;
class Package;
class DistanceSkill;
class Skill;
class MaxCardsSkill;
class AttackRangeSkill;
class Player;
struct CardDescriptor;
class Card;
class RoomObject;

class EnginePrivate;

class Engine final : public QObject
{
    Q_OBJECT

public:
    Engine();
    ~Engine() override;

    void loadTranslations(const QString &locale);
    void addTranslationEntry(const QString &key, const QString &value);
    QString translate(const QString &to_translate) const;

    void addPackage(const Package *package);
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
    QSanguosha::HandlingMethod getCardHandlingMethod(const QString &method_name) const;
    QList<const Skill *> getRelatedSkills(const QString &skill_name) const;
    const Skill *getMainSkill(const QString &skill_name) const;

    QList<const Package *> getPackages() const;
    const Package *findPackage(const QString &name) const;

    const General *getGeneral(const QString &name) const;
    QStringList getGenerals() const;
    int getGeneralCount(bool include_banned = false) const;
    const Skill *getSkill(const QString &skill_name) const;
    const Skill *getSkill(const EquipCard *card) const;
    QStringList getSkillNames() const;
    const ViewAsSkill *getViewAsSkill(const QString &skill_name) const;
    QList<const DistanceSkill *> getDistanceSkills() const;
    QList<const MaxCardsSkill *> getMaxCardsSkills() const;
    QList<const TargetModSkill *> getTargetModSkills() const;
    QList<const AttackRangeSkill *> getAttackRangeSkills() const;
    QList<const ViewAsSkill *> getViewAsSkills() const;
    void addSkills(const QList<const Skill *> &skills);

    int getCardCount() const;
    const CardDescriptor &getEngineCard(int cardId) const;

    QStringList getLords(bool contain_banned = false) const;
    QStringList getRandomLords() const;
    QStringList getRandomGenerals(int count, const QSet<QString> &ban_set = QSet<QString>()) const;
    QStringList getLatestGenerals(const QSet<QString> &ban_set = QSet<QString>()) const;
    QList<int> getRandomCards() const;
    QString getRandomGeneralName() const;
    QStringList getLimitedGeneralNames() const;

    const ProhibitSkill *isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;
    const TreatAsEquippingSkill *treatAsEquipping(const Player *player, const QString &equipName, QSanguosha::EquipLocation location) const;
    int correctDistance(const Player *from, const Player *to) const;
    int correctMaxCards(const Player *target, bool fixed = false, const QString &except = QString()) const;
    int correctCardTarget(const QSanguosha::TargetModType type, const Player *from, const Card *card) const;
    int correctAttackRange(const Player *target, bool include_weapon = true, bool fixed = false) const;

    bool isGeneralHidden(const QString &general_name) const;

    QStringList LordBGMConvertList;
    QStringList LordBackdropConvertList;
    QStringList LatestGeneralList;
    int operationTimeRate(QSanProtocol::CommandType command, const QVariant &msg) const;

    QString GetMappedKingdom(const QString &role); //hegemony

    QVariant getConfigFromConfigFile(const QString &key) const;

    void registerCardFace(const CardFace *face);
    const CardFace *cardFace(const QString &name);
    void unregisterCardFace(const QString &name);

public:
    Q_DECL_DEPRECATED QString translate(const QString &to_translate, bool) const
    {
        return translate(to_translate);
    }

private:
    Q_DISABLE_COPY_MOVE(Engine)
    EnginePrivate *const d;

#if 0
private:
    QHash<QString, QString> translations;
    QHash<QString, const General *> generals;
    QHash<QString, const Skill *> skills;
    QHash<QThread *, RoomObject *> m_rooms;
    QMap<QString, QString> modes;
    QMultiMap<QString, QString> related_skills;
    mutable QMap<QString, const CardPattern *> patterns;

    // Package
    QList<const Package *> packages;

    // special skills
    QList<const ProhibitSkill *> prohibit_skills;
    QList<const DistanceSkill *> distance_skills;
    QList<const TreatAsEquippingSkill *> viewhas_skills;
    QList<const MaxCardsSkill *> maxcards_skills;
    QList<const TargetModSkill *> targetmod_skills;
    QList<const AttackRangeSkill *> attackrange_skills;
    QList<const ViewAsSkill *> viewas_skills;

    QList<CardDescriptor> cards;
    QStringList lord_list;
    QSet<QString> ban_package;

    JsonObject configFile;

    LuaStatePointer l;

#endif
};

extern Engine *Sanguosha;

#endif
