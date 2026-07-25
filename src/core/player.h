#ifndef THKILL_PLAYER_H
#define THKILL_PLAYER_H

#include "WrappedCard.h"
#include "general.h"

#include <QObject>
#include <QTcpSocket>

class EquipCard;
class Weapon;
class Armor;
class Horse;
class DelayedTrick;
class DistanceSkill;
class TriggerSkill;

class Player : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString screenname READ screenName WRITE setScreenName)
    Q_PROPERTY(QString avatar MEMBER m_avatar NOTIFY avatar_changed)
    Q_PROPERTY(int hp READ getHp WRITE setHp NOTIFY hp_changed)
    Q_PROPERTY(int renhp READ getRenHp WRITE setRenHp NOTIFY hp_changed)
    Q_PROPERTY(int linghp READ getLingHp WRITE setLingHp NOTIFY hp_changed)
    Q_PROPERTY(int dyingFactor READ getDyingFactor WRITE setDyingFactor NOTIFY hp_changed)
    Q_PROPERTY(int maxhp READ getMaxHp WRITE setMaxHp NOTIFY hp_changed)
    Q_PROPERTY(int chaoren READ getChaoren WRITE setChaoren NOTIFY chaoren_changed)

    Q_PROPERTY(QString kingdom READ getKingdom WRITE setKingdom NOTIFY kingdom_changed)
    Q_PROPERTY(bool wounded READ isWounded STORED false NOTIFY hp_changed)
    Q_PROPERTY(QString role READ getRole WRITE setRole NOTIFY role_changed)
    Q_PROPERTY(QString general READ getGeneralName WRITE setGeneralName NOTIFY general_changed)
    Q_PROPERTY(QString general2 READ getGeneral2Name WRITE setGeneral2Name NOTIFY general2_changed)
    Q_PROPERTY(QString state READ getState WRITE setState NOTIFY state_changed)
    Q_PROPERTY(int handcard_num READ getHandcardNum STORED false)
    Q_PROPERTY(int seat READ getSeat WRITE setSeat)
    Q_PROPERTY(int inital_seat READ getInitialSeat WRITE setInitialSeat)
    Q_PROPERTY(QString phase READ getPhaseString WRITE setPhaseString NOTIFY phase_changed STORED false)
    Q_PROPERTY(Player::Phase phaseValue READ getPhase WRITE setPhase NOTIFY phase_changed)
    Q_PROPERTY(bool faceup READ faceUp WRITE setFaceUp NOTIFY faceupchanged)
    Q_PROPERTY(bool alive READ isAlive WRITE setAlive NOTIFY alive_changed)
    Q_PROPERTY(bool chained READ isChained WRITE setChained NOTIFY chainedchanged)
    Q_PROPERTY(bool removed READ isRemoved WRITE setRemoved NOTIFY removedChanged)
    Q_PROPERTY(bool owner READ isOwner WRITE setOwner NOTIFY owner_changed)
    Q_PROPERTY(bool role_shown READ hasShownRole WRITE setShownRole)

    Q_PROPERTY(bool general_showed READ hasShownGeneral WRITE setGeneralShowed NOTIFY head_state_changed)
    Q_PROPERTY(bool general2_showed READ hasShownGeneral2 WRITE setGeneral2Showed NOTIFY deputy_state_changed)

    Q_PROPERTY(QString next READ getNextName WRITE setNext)

    Q_PROPERTY(bool kongcheng READ isKongcheng STORED false)
    Q_PROPERTY(bool nude READ isNude STORED false)
    Q_PROPERTY(bool all_nude READ isAllNude STORED false)

public:
    enum Phase
    {
        RoundStart,
        Start,
        Judge,
        Draw,
        Play,
        Discard,
        Finish,
        NotActive,
        PhaseNone
    };
    Q_ENUM(Phase)

    enum Place
    {
        PlaceHand,
        PlaceEquip,
        PlaceDelayedTrick,
        PlaceJudge,
        PlaceSpecial,
        DiscardPile,
        DrawPile,
        PlaceTable,
        PlaceUnknown
    };
    Q_ENUM(Place)

    enum Role
    {
        Lord,
        Loyalist,
        Rebel,
        Renegade
    };
    Q_ENUM(Role)

    explicit Player(QObject *parent);

    void setScreenName(const QString &screen_name);
    [[nodiscard]] QString screenName() const;

    // property setters/getters
    [[nodiscard]] int getChaoren() const; //for chaoren
    void setChaoren(int chaoren);
    [[nodiscard]] QList<int> getShownHandcards() const;
    void setShownHandcards(QList<int> &ids);
    [[nodiscard]] bool isShownHandcard(int id) const;
    [[nodiscard]] QList<int> getBrokenEquips() const;
    void setBrokenEquips(QList<int> &ids);
    [[nodiscard]] bool isBrokenEquip(int id, bool consider_shenbao = false) const;
    [[nodiscard]] QStringList getHiddenGenerals() const;
    void setHiddenGenerals(const QStringList &generals);
    [[nodiscard]] QString getShownHiddenGeneral() const;
    void setShownHiddenGeneral(const QString &general);
    [[nodiscard]] bool canShowHiddenSkill() const;
    [[nodiscard]] bool isHiddenSkill(const QString &skill_name) const;

    [[nodiscard]] int getHp() const;
    [[nodiscard]] int getRenHp() const; //for banling
    [[nodiscard]] int getLingHp() const;
    void setHp(int hp);
    void setRenHp(int renhp);
    void setLingHp(int linghp);
    [[nodiscard]] int getDyingFactor() const;
    void setDyingFactor(int dyingFactor);
    [[nodiscard]] int getMaxHp() const;
    void setMaxHp(int max_hp);
    [[nodiscard]] int getLostHp() const;
    [[nodiscard]] bool isWounded() const;
    [[nodiscard]] int dyingThreshold() const;
    [[nodiscard]] General::Gender getGender() const;
    virtual void setGender(General::Gender gender);
    [[nodiscard]] bool isMale() const;
    [[nodiscard]] bool isFemale() const;
    [[nodiscard]] bool isNeuter() const;

    [[nodiscard]] bool isOwner() const;
    void setOwner(bool owner);

    [[nodiscard]] bool hasShownRole() const;
    void setShownRole(bool shown);

    [[nodiscard]] int getMaxCards(const QString &except = QString()) const;

    [[nodiscard]] QString getKingdom() const;
    void setKingdom(const QString &kingdom);

    void setRole(const QString &role);
    [[nodiscard]] QString getRole() const;
    [[nodiscard]] Role getRoleEnum() const;

    void setGeneral(const General *general);
    void setGeneralName(const QString &general_name);
    [[nodiscard]] QString getGeneralName() const;

    void setGeneral2Name(const QString &general_name);
    [[nodiscard]] QString getGeneral2Name() const;
    [[nodiscard]] const General *getGeneral2() const;

    [[nodiscard]] QString getFootnoteName() const;

    void setState(const QString &state);
    [[nodiscard]] QString getState() const;

    [[nodiscard]] int getSeat() const;
    void setSeat(int seat);
    [[nodiscard]] int getInitialSeat() const;
    void setInitialSeat(int seat);
    bool isAdjacentTo(const Player *another) const;
    [[nodiscard]] QString getPhaseString() const;
    static QString getPhaseString(Phase phase);
    void setPhaseString(const QString &phase_str);
    [[nodiscard]] Phase getPhase() const;
    void setPhase(Phase phase);
    [[nodiscard]] bool isInMainPhase() const;
    // TODO: static Q_INVOKABLE still can't be called without an instance. currently delegate from TouhouKillQmlUiGlobal
    static bool isMainPhase(Phase phase);

    [[nodiscard]] int getAttackRange(bool include_weapon = true) const;
    bool inMyAttackRange(const Player *other) const;

    [[nodiscard]] bool isAlive() const;
    [[nodiscard]] bool isDead() const;
    void setAlive(bool alive);

    [[nodiscard]] QString getFlags() const;
    [[nodiscard]] QStringList getFlagList() const;
    virtual void setFlags(const QString &flag);
    [[nodiscard]] bool hasFlag(const QString &flag) const;
    void clearFlags();

    [[nodiscard]] bool faceUp() const;
    void setFaceUp(bool face_up);

    [[nodiscard]] virtual int aliveCount(bool includeRemoved = true) const = 0;
    void setFixedDistance(const Player *player, int distance);
    int originalRightDistanceTo(const Player *other) const;
    int distanceTo(const Player *other, int distance_fix = 0) const;

    void setNext(Player *next);
    void setNext(const QString &next);
    [[nodiscard]] Player *getNext(bool ignoreRemoved = true) const;
    [[nodiscard]] QString getNextName() const;
    [[nodiscard]] Player *getLast(bool ignoreRemoved = true) const;
    [[nodiscard]] Player *getNextAlive(int n = 1, bool ignoreRemoved = true) const;
    [[nodiscard]] Player *getLastAlive(int n = 1, bool ignoreRemoved = true) const;

    [[nodiscard]] const General *getAvatarGeneral() const;
    [[nodiscard]] const General *getGeneral() const;

    [[nodiscard]] bool isLord() const;
    [[nodiscard]] bool isCurrent() const;

    void acquireSkill(const QString &skill_name, bool head = true);
    void detachSkill(const QString &skill_name, bool head = true);
    void detachAllSkills();
    virtual void addSkill(const QString &skill_name, bool head_skill = true);
    virtual void loseSkill(const QString &skill_name, bool head = true);
    [[nodiscard]] bool hasSkill(const QString &skill_name, bool include_lose = false, bool include_hidden = true) const;
    bool hasSkill(const Skill *skill, bool include_lose = false, bool include_hidden = true) const;
    [[nodiscard]] bool hasSkills(const QString &skill_name, bool include_lose = false) const;
    [[nodiscard]] bool hasInnateSkill(const QString &skill_name) const;
    bool hasInnateSkill(const Skill *skill) const;
    [[nodiscard]] bool hasLordSkill(const QString &skill_name, bool include_lose = false) const;
    bool hasLordSkill(const Skill *skill, bool include_lose = false) const;

    void setDisableShow(const QString &flags, const QString &reason);
    void removeDisableShow(const QString &reason);
    [[nodiscard]] QStringList disableShow(bool head) const;
    [[nodiscard]] bool canShowGeneral(const QString &flags = QString()) const;

    void setSkillInvalidity(const Skill *skill, bool invalidity);
    void setSkillInvalidity(const QString &skill_name, bool invalidity);

    bool isSkillInvalid(const Skill *skill) const;
    [[nodiscard]] bool isSkillInvalid(const QString &skill_name) const;

    [[nodiscard]] virtual QString getGameMode() const = 0;

    void setEquip(WrappedCard *equip);
    void removeEquip(WrappedCard *equip);
    bool hasEquip(const Card *card) const;
    [[nodiscard]] bool hasEquip() const;

    [[nodiscard]] QList<const Card *> getJudgingArea() const;
    [[nodiscard]] QList<int> getJudgingAreaID() const; //for marshal
    void addDelayedTrick(const Card *trick);
    void removeDelayedTrick(const Card *trick);
    [[nodiscard]] bool containsTrick(const QString &trick_name) const;

    [[nodiscard]] virtual int getHandcardNum() const = 0;
    virtual void removeCard(const Card *card, Place place) = 0;
    virtual void addCard(const Card *card, Place place) = 0;
    [[nodiscard]] virtual QList<const Card *> getHandcards() const = 0;

    [[nodiscard]] WrappedCard *getWeapon() const;
    [[nodiscard]] WrappedCard *getArmor() const;
    [[nodiscard]] WrappedCard *getDefensiveHorse() const;
    [[nodiscard]] WrappedCard *getOffensiveHorse() const;
    [[nodiscard]] WrappedCard *getTreasure() const;
    [[nodiscard]] QList<const Card *> getEquips() const;
    [[nodiscard]] const EquipCard *getEquip(int index) const;

    [[nodiscard]] bool hasWeapon(const QString &weapon_name, bool selfOnly = false, bool ignore_preshow = false) const;
    [[nodiscard]] bool hasArmorEffect(const QString &armor_name, bool selfOnly = false) const;
    [[nodiscard]] bool hasTreasure(const QString &treasure_name, bool selfOnly = false) const;

    [[nodiscard]] bool isKongcheng() const;
    [[nodiscard]] bool isNude() const;
    [[nodiscard]] bool isAllNude() const;

    bool canDiscard(const Player *to, const QString &flags, const QString &reason = "") const;
    bool canDiscard(const Player *to, int card_id, const QString &reason = "") const;

    void addMark(const QString &mark, int add_num = 1);
    void removeMark(const QString &mark, int remove_num = 1);
    virtual void setMark(const QString &mark, int value);
    [[nodiscard]] int getMark(const QString &mark) const;
    [[nodiscard]] QMap<QString, int> getMarkMap() const;

    void setChained(bool chained);
    [[nodiscard]] bool isChained() const;
    [[nodiscard]] bool isDebuffStatus() const;

    void setRemoved(bool removed);
    [[nodiscard]] bool isRemoved() const;

    bool canSlash(const Player *other, const Card *slash, bool distance_limit = true, int rangefix = 0, const QList<const Player *> &others = QList<const Player *>()) const;
    bool canSlash(const Player *other, bool distance_limit = true, int rangefix = 0, const QList<const Player *> &others = QList<const Player *>()) const;
    [[nodiscard]] int getCardCount(bool include_equip = true, bool include_judging = false) const;

    [[nodiscard]] Q_INVOKABLE QList<int> getPile(const QString &pile_name) const;
    [[nodiscard]] Q_INVOKABLE QStringList getPileNames() const;
    [[nodiscard]] QString getPileName(int card_id) const;

    [[nodiscard]] bool pileOpen(const QString &pile_name, const QString &player) const;
    void setPileOpen(const QString &pile_name, const QString &player);
    [[nodiscard]] QList<int> getHandPile() const;
    [[nodiscard]] QStringList getHandPileList(bool view_as_skill = true) const;

    void addHistory(const QString &name, int times = 1);
    void clearHistory();
    [[nodiscard]] bool hasUsed(const QString &card_class) const;
    [[nodiscard]] int usedTimes(const QString &card_class) const;
    [[nodiscard]] int getSlashCount() const;
    [[nodiscard]] int getAnalepticCount() const;

    [[nodiscard]] bool hasEquipSkill(const QString &skill_name) const;
    [[nodiscard]] QSet<const TriggerSkill *> getTriggerSkills() const;
    [[nodiscard]] QSet<const Skill *> getSkills(bool include_equip = false, bool visible_only = true) const;
    [[nodiscard]] QList<const Skill *> getSkillList(bool include_equip = false, bool visible_only = true) const;
    [[nodiscard]] QSet<const Skill *> getVisibleSkills(bool include_equip = false) const;
    [[nodiscard]] QList<const Skill *> getVisibleSkillList(bool include_equip = false) const;
    [[nodiscard]] QList<const Skill *> getHeadSkillList(bool visible_only = true, bool include_acquired = false, bool include_equip = false) const;
    [[nodiscard]] QList<const Skill *> getDeputySkillList(bool visible_only = true, bool include_acquired = false, bool include_equip = false) const;

    [[nodiscard]] QSet<QString> getAcquiredSkills() const;
    [[nodiscard]] QString getSkillDescription(bool yellow = true, const QString &flag = QString()) const;

    virtual bool isProhibited(const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;
    bool canSlashWithoutCrossbow(const Card *slash = nullptr) const;
    virtual bool isLastHandCard(const Card *card, bool contain = false) const = 0;

    inline bool isJilei(const Card *card) const
    {
        return isCardLimited(card, Card::MethodDiscard);
    }
    inline bool isLocked(const Card *card) const
    {
        return isCardLimited(card, Card::MethodUse);
    }

    void setCardLimitation(const QString &limit_list, const QString &pattern, const QString &reason, bool single_turn = false);
    void removeCardLimitation(const QString &limit_list, const QString &pattern, const QString &reason, bool clearReason = false);
    void clearCardLimitation(bool single_turn = false);
    bool isCardLimited(const Card *card, Card::HandlingMethod method, bool isHandcard = false) const;
    [[nodiscard]] bool isCardLimited(const QString &limit_list, const QString &reason) const;

    // just for convenience
    void addQinggangTag(const Card *card);
    void removeQinggangTag(const Card *card);

    void copyFrom(Player *p);

    [[nodiscard]] QList<const Player *> getSiblings() const;
    [[nodiscard]] QList<const Player *> getAliveSiblings() const;

    bool hasShownSkill(const Skill *skill) const; //hegemony
    [[nodiscard]] bool hasShownSkill(const QString &skill_name) const; //hegemony
    [[nodiscard]] bool hasShownSkills(const QString &skill_names) const;
    [[nodiscard]] bool inHeadSkills(const QString &skill_name) const;
    [[nodiscard]] bool inDeputySkills(const QString &skill_name) const;
    void setSkillPreshowed(const QString &skill, bool preshowed = true); //hegemony
    void setSkillsPreshowed(const QString &flag = "hd", bool preshowed = true);
    [[nodiscard]] bool hasPreshowedSkill(const QString &name) const;
    bool hasPreshowedSkill(const Skill *skill) const;
    [[nodiscard]] bool isHidden(bool head_general) const;

    [[nodiscard]] bool hasShownGeneral() const;
    void setGeneralShowed(bool showed);
    [[nodiscard]] bool hasShownGeneral2() const;
    void setGeneral2Showed(bool showed);
    [[nodiscard]] bool hasShownOneGeneral() const;
    [[nodiscard]] bool hasShownAllGenerals() const;
    [[nodiscard]] bool ownSkill(const QString &skill_name) const;
    bool ownSkill(const Skill *skill) const;
    bool isFriendWith(const Player *player, bool considerAnjiang = false) const;
    bool willBeFriendWith(const Player *player) const;
    [[nodiscard]] bool canTransform(bool head) const;

    [[nodiscard]] QList<const Player *> getFormation() const;

    [[nodiscard]] const Player *getLord(bool include_death = false) const;

    QVariantMap tag;

protected:
    QMap<QString, int> marks;
    QMap<QString, QList<int>> piles;
    QMap<QString, QStringList> pile_open;
    QSet<QString> acquired_skills;
    QSet<QString> acquired_skills2;
    QMap<QString, bool> skills;
    QMap<QString, bool> skills2;
    QStringList skills_originalOrder, skills2_originalOrder; //equals  skills.keys().  unlike QMap, QStringList will keep originalOrder
    QSet<QString> flags;
    QHash<QString, int> history;
    QStringList skill_invalid;
    QList<int> shown_handcards;
    QList<int> broken_equips;
    QStringList hidden_generals; //for anyun
    QString shown_hidden_general;

private:
    QString screen_name;
    bool owner;
    const General *general, *general2;
    General::Gender m_gender;
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

    Phase phase;
    WrappedCard *weapon, *armor, *defensive_horse, *offensive_horse, *treasure;
    bool face_up;
    bool chained;
    bool removed;
    QList<int> judging_area;
    QHash<const Player *, int> fixed_distance;

    QString next;

    //QMap<Card::HandlingMethod, QStringList> card_limitation;
    QMap<Card::HandlingMethod, QMap<QString, QStringList>> card_limitation; //method, reason , pattern
    QStringList disable_show;

    void updateYingyingguai();

    QString m_avatar;

signals:
    void avatar_changed(const QString &new_avatar);
    void hp_changed();
    void chaoren_changed();

    void kingdom_changed(const QString &new_kingdom);
    void role_changed(const QString &new_role);
    void general_changed(const QString &general_name);
    void general2_changed(const QString &general2_name);
    void state_changed(const QString &new_state);
    void phase_changed();
    void faceupchanged(bool faceup);
    void alive_changed();
    void chainedchanged(bool chained);
    void removedChanged();
    void owner_changed(bool owner);
    void head_state_changed();
    void deputy_state_changed();

    void showncards_changed();
    void brokenEquips_changed();
    void disable_show_changed();
};

#endif
