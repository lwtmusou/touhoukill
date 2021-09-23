#ifndef _PLAYER_H
#define _PLAYER_H

#include "general.h"
#include "global.h"

#include <QObject>
#include <QTcpSocket>

class EquipCard;
class Weapon;
class Armor;
class Horse;
class DelayedTrick;
class DistanceSkill;
class TriggerSkill;
class RoomObject;
class Card;

class Player : public QObject
{
    Q_OBJECT
#if 0
    Q_PROPERTY(QString screenname READ screenName WRITE setScreenName)
    Q_PROPERTY(int hp READ getHp WRITE setHp)
    Q_PROPERTY(int renhp READ getRenHp WRITE setRenHp)
    Q_PROPERTY(int linghp READ getLingHp WRITE setLingHp)
    Q_PROPERTY(int dyingFactor READ getDyingFactor WRITE setDyingFactor)
    Q_PROPERTY(int maxhp READ getMaxHp WRITE setMaxHp)
    Q_PROPERTY(int chaoren READ getChaoren WRITE setChaoren)

    Q_PROPERTY(QString kingdom READ getKingdom WRITE setKingdom)
    Q_PROPERTY(bool wounded READ isWounded STORED false)
    Q_PROPERTY(QString role READ getRole WRITE setRole)
    Q_PROPERTY(QString general READ getGeneralName WRITE setGeneralName)
    Q_PROPERTY(QString general2 READ getGeneral2Name WRITE setGeneral2Name)
    Q_PROPERTY(QString state READ getState WRITE setState)
    Q_PROPERTY(int handcard_num READ getHandcardNum)
    Q_PROPERTY(int seat READ getSeat WRITE setSeat)
    Q_PROPERTY(int inital_seat READ getInitialSeat WRITE setInitialSeat)
    Q_PROPERTY(QString phase READ getPhaseString WRITE setPhaseString)
    Q_PROPERTY(bool faceup READ faceUp WRITE setFaceUp)
    Q_PROPERTY(bool alive READ isAlive WRITE setAlive)
    Q_PROPERTY(QString flags READ getFlags WRITE setFlags)
    Q_PROPERTY(bool chained READ isChained WRITE setChained)
    Q_PROPERTY(bool removed READ isRemoved WRITE setRemoved)
    Q_PROPERTY(bool owner READ isOwner WRITE setOwner)
    Q_PROPERTY(bool role_shown READ hasShownRole WRITE setShownRole)

    Q_PROPERTY(bool general_showed READ hasShownGeneral WRITE setGeneralShowed)
    Q_PROPERTY(bool general2_showed READ hasShownGeneral2 WRITE setGeneral2Showed)

    Q_PROPERTY(QString next READ getNextName WRITE setNext)

    Q_PROPERTY(bool kongcheng READ isKongcheng)
    Q_PROPERTY(bool nude READ isNude)
    Q_PROPERTY(bool all_nude READ isAllNude)
#endif

public:
    explicit Player(QObject *parent);

    void setScreenName(const QString &screen_name);
    QString screenName() const;

    // property setters/getters
    int getChaoren() const; //for chaoren
    void setChaoren(int chaoren);
    const IDSet &getShownHandcards() const;
    void setShownHandcards(const IDSet &ids);
    bool isShownHandcard(int id) const;
    const IDSet &getBrokenEquips() const;
    void setBrokenEquips(const IDSet &ids);
    bool isBrokenEquip(int id, bool consider_shenbao = false) const;
    QStringList getHiddenGenerals() const;
    void setHiddenGenerals(const QStringList &generals);
    QString getShownHiddenGeneral() const;
    void setShownHiddenGeneral(const QString &general);
    bool canShowHiddenSkill() const;
    bool isHiddenSkill(const QString &skill_name) const;

    int getHp() const;
    int getRenHp() const; //for banling
    int getLingHp() const;
    void setHp(int hp);
    void setRenHp(int renhp);
    void setLingHp(int linghp);
    int getDyingFactor() const;
    void setDyingFactor(int dyingFactor);
    int getMaxHp() const;
    void setMaxHp(int max_hp);
    int getLostHp() const;
    bool isWounded() const;
    int dyingThreshold() const;
    QSanguosha::Gender getGender() const;
    virtual void setGender(QSanguosha::Gender gender);
    bool isMale() const;
    bool isFemale() const;
    bool isNeuter() const;

    bool isOwner() const;
    void setOwner(bool owner);

    bool hasShownRole() const;
    void setShownRole(bool shown);

    int getMaxCards(const QString &except = QString()) const;

    QString getKingdom() const;
    void setKingdom(const QString &kingdom);

    void setRole(const QString &role);
    QString getRole() const;
    QSanguosha::Role getRoleEnum() const;

    void setGeneral(const General *general);
    void setGeneralName(const QString &general_name);
    QString getGeneralName() const;

    void setGeneral2Name(const QString &general_name);
    QString getGeneral2Name() const;
    const General *getGeneral2() const;

    QString getFootnoteName() const;

    void setState(const QString &state);
    QString getState() const;

    int getSeat() const;
    void setSeat(int seat);
    int getInitialSeat() const;
    void setInitialSeat(int seat);
    bool isAdjacentTo(const Player *another) const;
    QString getPhaseString() const;
    void setPhaseString(const QString &phase_str);
    QSanguosha::Phase getPhase() const;
    void setPhase(QSanguosha::Phase phase);
    bool isInMainPhase() const;

    int getAttackRange(bool include_weapon = true) const;
    bool inMyAttackRange(const Player *other) const;

    bool isAlive() const;
    bool isDead() const;
    void setAlive(bool alive);

    QString getFlags() const;
    QStringList getFlagList() const;
    virtual void setFlags(const QString &flag);
    bool hasFlag(const QString &flag) const;
    void clearFlags();

    bool faceUp() const;
    void setFaceUp(bool face_up);

    virtual int aliveCount(bool includeRemoved = true) const = 0;
    void setFixedDistance(const Player *player, int distance);
    int originalRightDistanceTo(const Player *other) const;
    int distanceTo(const Player *other, int distance_fix = 0) const;

    void setNext(Player *next);
    void setNext(const QString &next);
    Player *getNext(bool ignoreRemoved = true) const;
    QString getNextName() const;
    Player *getLast(bool ignoreRemoved = true) const;
    Player *getNextAlive(int n = 1, bool ignoreRemoved = true) const;
    Player *getLastAlive(int n = 1, bool ignoreRemoved = true) const;

    const General *getAvatarGeneral() const;
    const General *getGeneral() const;

    bool isLord() const;
    bool isCurrent() const;

    void acquireSkill(const QString &skill_name, bool head = true);
    void detachSkill(const QString &skill_name, bool head = true);
    void detachAllSkills();
    virtual void addSkill(const QString &skill_name, bool head_skill = true);
    virtual void loseSkill(const QString &skill_name, bool head = true);
    bool hasSkill(const QString &skill_name, bool include_lose = false, bool include_hidden = true) const;
    bool hasSkill(const Skill *skill, bool include_lose = false, bool include_hidden = true) const;
    bool hasSkills(const QString &skill_name, bool include_lose = false) const;
    bool hasInnateSkill(const QString &skill_name) const;
    bool hasInnateSkill(const Skill *skill) const;
    bool hasLordSkill(const QString &skill_name, bool include_lose = false) const;
    bool hasLordSkill(const Skill *skill, bool include_lose = false) const;

    void setDisableShow(const QString &flags, const QString &reason);
    void removeDisableShow(const QString &reason);
    QStringList disableShow(bool head) const;
    bool canShowGeneral(const QString &flags = QString()) const;

    void setSkillInvalidity(const Skill *skill, bool invalidity);
    void setSkillInvalidity(const QString &skill_name, bool invalidity);

    bool isSkillInvalid(const Skill *skill) const;
    bool isSkillInvalid(const QString &skill_name) const;

    void setEquip(const Card *equip);
    void removeEquip(const Card *equip);
    bool hasEquip(const Card *card) const;
    bool hasEquip() const;

    QList<const Card *> getJudgingArea() const;
    QList<int> getJudgingAreaID() const; //for marshal
    void addDelayedTrick(const Card *trick);
    void removeDelayedTrick(const Card *trick);
    bool containsTrick(const QString &trick_name) const;

    virtual int getHandcardNum() const = 0;
    virtual void removeCard(const Card *card, QSanguosha::Place place) = 0;
    virtual void addCard(const Card *card, QSanguosha::Place place) = 0;
    virtual QList<const Card *> getHandcards() const = 0;

    const Card *getWeapon() const;
    const Card *getArmor() const;
    const Card *getDefensiveHorse() const;
    const Card *getOffensiveHorse() const;
    const Card *getTreasure() const;
    QList<const Card *> getEquips() const;
    const Card *getEquip(int index) const;

    bool hasWeapon(const QString &weapon_name, bool selfOnly = false, bool ignore_preshow = false) const;
    bool hasArmor(const QString &armor_name, bool selfOnly = false) const;
    bool hasTreasure(const QString &treasure_name, bool selfOnly = false) const;

    bool isKongcheng() const;
    bool isNude() const;
    bool isAllNude() const;

    bool canDiscard(const Player *to, const QString &flags, const QString &reason = QString()) const;
    bool canDiscard(const Player *to, int card_id, const QString &reason = QString()) const;

    void addMark(const QString &mark, int add_num = 1);
    void removeMark(const QString &mark, int remove_num = 1);
    virtual void setMark(const QString &mark, int value);
    int getMark(const QString &mark) const;
    QMap<QString, int> getMarkMap() const;

    void setChained(bool chained);
    bool isChained() const;
    bool isDebuffStatus() const;

    void setRemoved(bool removed);
    bool isRemoved() const;

    bool canSlash(const Player *other, const Card *slash, bool distance_limit = true, int rangefix = 0, const QList<const Player *> &others = QList<const Player *>()) const;
    bool canSlash(const Player *other, bool distance_limit = true, int rangefix = 0, const QList<const Player *> &others = QList<const Player *>()) const;
    int getCardCount(bool include_equip = true, bool = false) const;

    IDSet getPile(const QString &pile_name) const;
    QStringList getPileNames() const;
    QString getPileName(int card_id) const;

    bool pileOpen(const QString &pile_name, const QString &player) const;
    void setPileOpen(const QString &pile_name, const QString &player);
    IDSet getHandPile() const;
    QStringList getHandPileList(bool view_as_skill = true) const;

    void addHistory(const QString &name, int times = 1);
    void clearHistory();
    bool hasUsed(const QString &card_class) const;
    int usedTimes(const QString &card_class) const;
    int getSlashCount() const;
    int getAnalepticCount() const;

    bool hasEquipSkill(const QString &skill_name) const;
    QSet<const TriggerSkill *> getTriggerSkills() const;
    QSet<const Skill *> getSkills(bool include_equip = false, bool visible_only = true) const;
    QList<const Skill *> getSkillList(bool include_equip = false, bool visible_only = true) const;
    QSet<const Skill *> getVisibleSkills(bool include_equip = false) const;
    QList<const Skill *> getVisibleSkillList(bool include_equip = false) const;
    QList<const Skill *> getHeadSkillList(bool visible_only = true, bool include_acquired = false, bool include_equip = false) const;
    QList<const Skill *> getDeputySkillList(bool visible_only = true, bool include_acquired = false, bool include_equip = false) const;

    QSet<QString> getAcquiredSkills() const;
    QString getSkillDescription(bool yellow = true, const QString &flag = QString()) const;

    virtual bool isProhibited(const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;
    bool canSlashWithoutCrossbow(const Card *slash = nullptr) const;
    virtual bool isLastHandCard(const Card *card, bool contain = false) const = 0;

    inline bool isJilei(const Card *card) const
    {
        return isCardLimited(card, QSanguosha::MethodDiscard);
    }
    inline bool isLocked(const Card *card) const
    {
        return isCardLimited(card, QSanguosha::MethodUse);
    }

    void setCardLimitation(const QString &limit_list, const QString &pattern, const QString &reason, bool single_turn = false);
    void removeCardLimitation(const QString &limit_list, const QString &pattern, const QString &reason, bool clearReason = false);
    void clearCardLimitation(bool single_turn = false);
    bool isCardLimited(const Card *card, QSanguosha::HandlingMethod method, bool isHandcard = false) const;
    bool isCardLimited(const QString &limit_list, const QString &reason) const;

    // just for convenience
    void addQinggangTag(const Card *card);
    void removeQinggangTag(const Card *card);

    QList<const Player *> getSiblings() const;
    QList<const Player *> getAliveSiblings() const;

    bool hasShownSkill(const Skill *skill) const; //hegemony
    bool hasShownSkill(const QString &skill_name) const; //hegemony
    bool hasShownSkills(const QString &skill_names) const;
    bool inHeadSkills(const QString &skill_name) const;
    bool inDeputySkills(const QString &skill_name) const;
    void setSkillPreshowed(const QString &skill, bool preshowed = true); //hegemony
    void setSkillsPreshowed(const QString &flag = QStringLiteral("hd"), bool preshowed = true);
    bool hasPreshowedSkill(const QString &name) const;
    bool hasPreshowedSkill(const Skill *skill) const;
    bool isHidden(bool head_general) const;

    bool hasShownGeneral() const;
    void setGeneralShowed(bool showed);
    bool hasShownGeneral2() const;
    void setGeneral2Showed(bool showed);
    bool hasShownOneGeneral() const;
    bool hasShownAllGenerals() const;
    bool ownSkill(const QString &skill_name) const;
    bool ownSkill(const Skill *skill) const;
    bool isFriendWith(const Player *player, bool considerAnjiang = false) const;
    bool willBeFriendWith(const Player *player) const;
    bool canTransform(bool head) const;

    QList<const Player *> getFormation() const;

    const Player *getLord(bool include_death = false) const;

    virtual RoomObject *roomObject() const = 0;

    QVariantMap tag;

protected:
    QMap<QString, int> marks;
    QMap<QString, IDSet> piles;
    QMap<QString, QStringList> pile_open;
    QSet<QString> acquired_skills;
    QSet<QString> acquired_skills2;
    QMap<QString, bool> skills;
    QMap<QString, bool> skills2;
    QStringList skills_originalOrder, skills2_originalOrder; //equals  skills.keys().  unlike QMap, QStringList will keep originalOrder
    QSet<QString> flags;
    QHash<QString, int> history;
    QStringList skill_invalid;
    IDSet shown_handcards;
    IDSet broken_equips;
    QStringList hidden_generals; //for anyun
    QString shown_hidden_general;

private:
    QString screen_name;
    bool owner;
    const General *general;
    const General *general2;
    QSanguosha::Gender m_gender;
    int hp, max_hp;
    int renhp, linghp; //for banling
    int dyingFactor;
    int chaoren;
    QString kingdom;
    QString role;
    bool role_shown;
    QString state;
    int seat;
    int initialSeat; //for record
    bool alive;

    bool general_showed;
    bool general2_showed; //hegemony

    QSanguosha::Phase phase;
    const Card *weapon;
    const Card *armor;
    const Card *defensive_horse;
    const Card *offensive_horse;
    const Card *treasure;
    bool face_up;
    bool chained;
    bool removed;
    QList<int> judging_area;
    QHash<const Player *, int> fixed_distance;

    QString next;

    QMap<QSanguosha::HandlingMethod, QMap<QString, QStringList>> card_limitation; //method, reason , pattern
    QStringList disable_show;

signals:
    void general_changed();
    void general2_changed();
    void role_changed(const QString &new_role);
    void state_changed();
    void hp_changed();
    void kingdom_changed(const QString &new_kingdom);
    void phase_changed();
    void owner_changed(bool owner);
    void chaoren_changed();
    void showncards_changed();
    void removedChanged();
    void brokenEquips_changed();

    void head_state_changed();
    void deputy_state_changed(); //hegemony
    void disable_show_changed();
};

#endif
