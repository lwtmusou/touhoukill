#ifndef _PLAYER_H
#define _PLAYER_H

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
class Skill;
class Card;
class General;

class PlayerPrivate;

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
    explicit Player(RoomObject *parent);
    ~Player() override;

    void setScreenName(const QString &screen_name);
    QString screenName() const;

    // property setters/getters
    const IDSet &getShownHandcards() const;
    void setShownHandcards(const IDSet &ids);
    bool isShownHandcard(int id) const;
    const IDSet &getBrokenEquips() const;
    void setBrokenEquips(const IDSet &ids);
    bool isBrokenEquip(int id, bool consider_shenbao = false) const;

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
    void setGender(QSanguosha::Gender gender);
    bool isMale() const;
    bool isFemale() const;
    bool isNeuter() const;

    bool hasShownRole() const;
    void setShownRole(bool shown);

    int getMaxCards(const QString &except = QString()) const;

    QString getKingdom() const;
    void setKingdom(const QString &kingdom);

    void setRole(const QString &role);
    void setRole(QSanguosha::Role role);
    QString getRoleString() const;
    QSanguosha::Role getRole() const;

    void setGeneral(const General *general, int pos = 0);
    const General *getGeneral(int pos = 0) const;
    QString getGeneralName(int pos = 0) const;

    QString getFootnoteName() const;

    void setState(const QString &state);
    QString getState() const;

    int getSeat() const;
    void setSeat(int seat);
    bool isAdjacentTo(const Player *another) const;
    QString getPhaseString() const;
    void setPhaseString(const QString &phase_str);
    QSanguosha::Phase getPhase() const;
    void setPhase(QSanguosha::Phase phase);
    bool isInMainPhase() const;

    int getAttackRange(bool include_weapon = true) const;
    bool inMyAttackRange(const Player *other) const;

    bool isAlive() const;
    inline bool isDead() const
    {
        return !isAlive();
    }
    void setAlive(bool alive);

    QString getFlags() const;
    QStringList getFlagList() const;
    void setFlags(const QString &flag);
    bool hasFlag(const QString &flag) const;
    void clearFlags();

    bool turnSkipping() const;
    void setTurnSkipping(bool turnSkipping);

    void setFixedDistance(const Player *player, int distance);
    int originalRightDistanceTo(const Player *other) const;
    int distanceTo(const Player *other, int distance_fix = 0) const;

    Player *getNext(bool ignoreRemoved = true);
    Player *getLast(bool ignoreRemoved = true);
    Player *getNextAlive(int n = 1, bool ignoreRemoved = true);
    Player *getLastAlive(int n = 1, bool ignoreRemoved = true);

    const Player *getNext(bool ignoreRemoved = true) const;
    const Player *getLast(bool ignoreRemoved = true) const;
    const Player *getNextAlive(int n = 1, bool ignoreRemoved = true) const;
    const Player *getLastAlive(int n = 1, bool ignoreRemoved = true) const;

    const General *getAvatarGeneral() const;

    inline bool isLord() const
    {
        return getRole() == QSanguosha::RoleLord;
    }
    bool isCurrent() const;

    void acquireSkill(const QString &skill_name);
    void detachSkill(const QString &skill_name);
    void detachAllSkills();
    void addSkill(const QString &skill_name, int place = 0);
    void loseSkill(const QString &skill_name, int place = 0);

    bool hasSkill(const QString &skill_name, bool include_lose = false, bool include_hidden = true) const;
    bool hasSkill(const Skill *skill, bool include_lose = false, bool include_hidden = true) const;
    bool hasSkills(const QString &skill_name, bool include_lose = false) const;
    bool hasInnateSkill(const QString &skill_name) const;
    bool hasInnateSkill(const Skill *skill) const;
    bool hasLordSkill(const QString &skill_name, bool include_lose = false) const;
    bool hasLordSkill(const Skill *skill, bool include_lose = false) const;

    void removeDisableShow(const QString &reason);

    void setDisableShow(const QList<int> &positions, const QString &reason);
    QStringList disableShow(int pos) const;
    bool canShowGeneral(const QList<int> &positions = {}) const;

    void setSkillInvalidity(const Skill *skill, bool invalidity);
    void setSkillInvalidity(const QString &skill_name, bool invalidity);

    bool isSkillInvalid(const Skill *skill) const;
    bool isSkillInvalid(const QString &skill_name) const;

    QStringList invalidedSkills() const;

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
    void setMark(const QString &mark, int value);
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

    IDSet getHandPile() const;
    QStringList getHandPileList(bool view_as_skill = true) const;

    void addHistory(const QString &name, int times = 1);
    void clearHistory();
    bool hasUsed(const QString &card_class) const;
    int usedTimes(const QString &card_class) const;
    int getSlashCount() const;
    int getAnalepticCount() const;
    QHash<QString, int> getHistories() const;

    bool hasEquipSkill(const QString &skill_name) const;
    QSet<const TriggerSkill *> getTriggerSkills() const;
    QSet<const Skill *> getSkills(bool include_equip = false, bool visible_only = true, bool include_acquired = false, const QList<int> &positions = {}) const;
    QList<const Skill *> getSkillList(bool include_equip = false, bool visible_only = true, bool include_acquired = false, const QList<int> &positions = {}) const;
    QSet<const Skill *> getVisibleSkills(bool include_equip = false) const;
    QList<const Skill *> getVisibleSkillList(bool include_equip = false) const;

    QSet<QString> getAcquiredSkills() const;
    QString getSkillDescription(bool yellow = true, const QString &flag = QString()) const;

    virtual bool isProhibited(const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;
    bool canSlashWithoutCrossbow(const Card *slash = nullptr) const;
    virtual bool isLastHandCard(const Card *card, bool contain = false) const = 0;

    void setCardLimitation(const QString &limit_list, const QString &pattern, const QString &reason, bool single_turn = false);
    void removeCardLimitation(const QString &limit_list, const QString &pattern, const QString &reason, bool clearReason = false);
    void clearCardLimitation(bool single_turn = false);
    bool isCardLimited(const Card *card, QSanguosha::HandlingMethod method, bool isHandcard = false) const;
    bool isCardLimited(const QString &limit_list, const QString &reason) const;

    bool hasShownSkill(const Skill *skill) const; //hegemony
    bool hasShownSkill(const QString &skill_name) const; //hegemony
    bool hasShownSkills(const QString &skill_names) const;
    int findPositionOfGeneralOwningSkill(const QString &skill_name) const;
    void setSkillPreshowed(const QString &skill, bool preshowed = true); //hegemony
    void setSkillsPreshowed(const QList<int> &positions, bool preshowed = true);

    bool hasPreshowedSkill(const QString &name) const;
    bool hasPreshowedSkill(const Skill *skill) const;
    bool isHidden(int pos) const;

    bool hasShownGeneral(int pos = 0) const;
    void setShownGeneral(int pos, bool show);

    bool hasShownOneGeneral() const;
    bool hasShownAllGenerals() const;
    bool ownSkill(const QString &skill_name) const;
    bool ownSkill(const Skill *skill) const;
    bool isFriendWith(const Player *player, bool considerAnjiang = false) const;
    bool willBeFriendWith(const Player *player) const;

    QList<const Player *> getFormation() const;

    RoomObject *roomObject() const;

    QVariantMap tag;

#ifndef QSGS_PLAYER_NODEPRECATED

    Q_DECL_DEPRECATED inline bool isHidden(bool head_general) const
    {
        return isHidden(head_general ? 0 : 1);
    }
    Q_DECL_DEPRECATED inline void setGeneralShowed(bool showed, int pos = 0)
    {
        setShownGeneral(pos, showed);
    }
    Q_DECL_DEPRECATED inline bool hasShownGeneral2() const
    {
        return hasShownGeneral(1);
    }
    Q_DECL_DEPRECATED inline void setGeneral2Showed(bool showed)
    {
        return setShownGeneral(1, showed);
    }
    Q_DECL_DEPRECATED bool inHeadSkills(const QString &skill_name) const
    {
        return findPositionOfGeneralOwningSkill(skill_name) == 0;
    }
    Q_DECL_DEPRECATED bool inDeputySkills(const QString &skill_name) const
    {
        return findPositionOfGeneralOwningSkill(skill_name) == 1;
    }
    Q_DECL_DEPRECATED inline void setSkillsPreshowed(const QString &flag = QStringLiteral("hd"), bool preshowed = true)
    {
        QList<int> p;
        if (flag.contains(QStringLiteral("h")))
            p << 0;
        if (flag.contains(QStringLiteral("d")))
            p << 1;
        setSkillsPreshowed(p, preshowed);
    }
    Q_DECL_DEPRECATED inline bool isJilei(const Card *card) const
    {
        return isCardLimited(card, QSanguosha::MethodDiscard);
    }
    Q_DECL_DEPRECATED inline bool isLocked(const Card *card) const
    {
        return isCardLimited(card, QSanguosha::MethodUse);
    }
    Q_DECL_DEPRECATED QList<const Skill *> getHeadSkillList(bool visible_only = true, bool include_acquired = false, bool include_equip = false) const
    {
        return getSkillList(include_equip, visible_only, include_acquired, {0});
    }
    Q_DECL_DEPRECATED QList<const Skill *> getDeputySkillList(bool visible_only = true, bool include_acquired = false, bool include_equip = false) const
    {
        return getSkillList(include_equip, visible_only, include_acquired, {1});
    }
    Q_DECL_DEPRECATED void setDisableShow(const QString &flags, const QString &reason)
    {
        QList<int> pos;
        if (flags.contains(QLatin1Char('h')))
            pos << 0;
        if (flags.contains(QLatin1Char('d')))
            pos << 1;

        setDisableShow(pos, reason);
    }
    Q_DECL_DEPRECATED QStringList disableShow(bool head) const
    {
        return disableShow(head ? 0 : 1);
    }
    Q_DECL_DEPRECATED bool canShowGeneral(const QString &flags = QString()) const
    {
        QList<int> pos;
        if (flags.contains(QLatin1Char('h')))
            pos << 0;
        if (flags.contains(QLatin1Char('d')))
            pos << 1;

        return canShowGeneral(pos);
    }
    Q_DECL_DEPRECATED inline void addSkill(const QString &skill_name, bool head_skill)
    {
        addSkill(skill_name, head_skill ? 0 : 1);
    }
    Q_DECL_DEPRECATED void loseSkill(const QString &skill_name, bool head)
    {
        loseSkill(skill_name, head ? 0 : 1);
    }
    Q_DECL_DEPRECATED inline bool faceUp() const
    {
        return !turnSkipping();
    }
    Q_DECL_DEPRECATED inline void setFaceUp(bool face_up)
    {
        setTurnSkipping(!face_up);
    }
    Q_DECL_DEPRECATED inline const General *getGeneral2() const
    {
        return getGeneral(1);
    }
    Q_DECL_DEPRECATED inline QString getGeneral2Name() const
    {
        return getGeneralName(1);
    }
    Q_DECL_DEPRECATED bool pileOpen(const QString &pile_name, const QString &player) const
    {
        return false;
    }
    Q_DECL_DEPRECATED void setPileOpen(const QString &pile_name, const QString &player)
    {
    }
#endif

private:
    PlayerPrivate *d;

#if 0
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
#endif
};

#endif
