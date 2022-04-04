%include "global.h"
%include "structs.h"

class CardFace
{
public:
    // text property
    QString name() const;

    // type property
    virtual QSanguosha::CardType type() const = 0;
    virtual QString typeName() const = 0;
    QString subTypeName() const;
    bool isKindOf(const QString &cardType) const;

    // Can we have a better way to replace this function? Maybe using `match`
    // Fs: This is just a convenience function....
    bool isNdTrick() const;

    // property identifier.
    // CardFace provides the default value of these property
    // But they could be dynamic and explained by Card itself.
    // For example, some skill may let Slash not be regarded as damage card?
    // Fs: There is a skill which has a skill named "Xianshi" for God Patchouli in TouhouKill. It needs an extremely hacked Card/CardFace which changes all the effect of a certain Card.
    // Return value of "canDamage" and "canRecover" is affected by "Xianshi" in this case.
    // TODO_Fs: ***non-virtual*** property setters for simplifying logic, only reimplement these functions when complex logic is needed
    bool canDamage() const;
    void setCanDamage(bool can);
    bool canRecover() const;
    void setCanRecover(bool can);
    // Fs: canRecast should be property of Card.
    // Seems like it should be dealt in UI and GameRule instead of the logic in Card/CardFace itself.
    // Currently CardFace::onUse and CardFace::targetFixed/targetFeasible are hacked to support recast
    // TODO_Fs: This may be changed to using skillcard/recastcard when UI/client detects a recast operation
    // so that there will be no logic in CardFace for implementing recasting
    // Note: In HulaoPass mode, all weapon can be recast according to the game rule.
    // virtual bool canRecast() const;
    bool hasEffectValue() const;
    void setHasEffectValue(bool can);
    virtual bool hasPreAction() const;
    void setHasPreAction(bool can);

    // This method provides a default handling method suggested by the card face.
    // Almost every actual card has its handlingMethod to be QSanguosha::MethodUse.
    QSanguosha::HandlingMethod defaultHandlingMethod() const;
    void setDefaultHandlingMethod(QSanguosha::HandlingMethod can);

    // Functions
    bool targetFixed(const Player *player, const Card *card) const;
    void setTargetFixed(bool fixed);

    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self, const Card *card) const;

    // This is the merged targetFilter implementation.
    /**
     * Calculate the maximum vote for specific target.
     *
     * @param targets The lists where all selected targets are.
     * @param to_select The player to be judged.
     * @param Self The user of the card.
     * @param card the card itself
     *
     * @return the maximum vote for to_select.
     *
     * @note to_select will be selectable until its appearance in targets >= its maximum vote.
     */
    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const;

    bool isAvailable(const Player *player, const Card *card) const;

    const Card *validate(const CardUseStruct &cardUse) const;
    const Card *validateInResponse(Player *user, const Card *original_card) const;

    void doPreAction(RoomObject *room, const CardUseStruct &card_use) const;

    // TODO_Fs: Aren't the names of these 2 functions easy to be misunderstood?
    void onUse(RoomObject *room, const CardUseStruct &card_use) const; // Shouldn't this be "processOfUsing" / "usingProcess" or something like this?
    void use(RoomObject *room, const CardUseStruct &use) const; // Shouldn't this be "onUse"?

    void onEffect(const CardEffectStruct &effect) const;
    bool isCancelable(const CardEffectStruct &effect) const;
    void onNullified(Player *target, const Card *card) const;

private:
    QSGS_DISABLE_COPY_MOVE_CONSTRUCT(CardFace)
};

class BasicCard : public CardFace
{
};

class EquipCard : public CardFace
{
public:
    QSanguosha::EquipLocation location() const;

    void onInstall(Player *player) const;
    void onUninstall(Player *player) const;
};

class Weapon : public EquipCard
{
public:
    int range() const;
    void setRange(int r);
};

class Armor : public EquipCard
{
};

class DefensiveHorse : public EquipCard
{
};

class OffensiveHorse : public EquipCard
{
};

class Treasure : public EquipCard
{
};

class TrickCard : public CardFace
{
};

class NonDelayedTrick : public TrickCard
{
};

class DelayedTrick : public TrickCard
{
public:
    void takeEffect(Player *target) const;

    void setJudge(const JudgeStruct &j);
    JudgeStruct judge() const;
};

class SkillCard : public CardFace
{
public:
    bool throwWhenUsing() const;
    void setThrowWhenUsing(bool can);
};

class Card
{
public:
    static const int S_UNKNOWN_CARD_ID;

    // Suit method
    QSanguosha::Suit suit() const;
    void setSuit(QSanguosha::Suit suit);
    QString suitString() const;
    bool isRed() const;
    bool isBlack() const;
    QSanguosha::Color color() const;

    // Number method
    QSanguosha::Number number() const;
    void setNumber(QSanguosha::Number number);
    QString numberString() const;

    // id
    int id() const;
    void setId(int id);
    int effectiveId() const;

    // name
    QString faceName() const;
    QString fullName(bool include_suit = false) const;
    QString logName() const;

    // ??
    bool isModified() const;

    // skill name
    QString skillName(bool removePrefix = true) const;
    void setSkillName(const QString &skill_name);
    const QString &showSkillName() const;
    void setShowSkillName(const QString &show_skill_name);

    // Property of card itself
    bool canRecast() const;
    void setCanRecast(bool can);
    bool transferable() const;
    void setTransferable(bool can);

    // handling method
    QSanguosha::HandlingMethod handleMethod() const;
    void setHandleMethod(QSanguosha::HandlingMethod method);

    // property (override the CardFace)
    bool canDamage() const;
    void setCanDamage(bool can);
    bool canRecover() const;
    void setCanRecover(bool can);
    bool hasEffectValue() const;
    void setHasEffectValue(bool has);

    // Face (functional model)
    const CardFace *face() const;
    // For compulsory view as skill. (Doesn't this kind of skill should return a new card and make that card as subcard?)
    void setFace(const CardFace *face);

    // Flags
    const QSet<QString> &flags() const;
    void addFlag(const QString &flag) const /* mutable */;
    void addFlags(const QSet<QString> &flags) const /* mutable */;
    void removeFlag(const QString &flag) const /* mutable */;
    void removeFlag(const QSet<QString> &flag) const /* mutable */;
    void clearFlags() const /* mutable */;
    bool hasFlag(const QString &flag) const;

    // Virtual Card
    bool isVirtualCard() const;

    // Subcard
    const IdSet &subcards() const;
    void addSubcard(int card_id);
    void addSubcard(const Card *card);
    void addSubcards(const IdSet &subcards);
    void clearSubcards();
    QString subcardString() const; // Used for converting card to string

    // UI property
    bool mute() const;
    void setMute(bool mute);

    const QString &userString() const;
    void setUserString(const QString &str);

    // room Object
    RoomObject *room();
    const RoomObject *room() const;
    void setRoomObject(RoomObject *room);

    // toString
    // What's the meaning of this hidden card?
    // Fs: SkillCard with subcard which does not always need to show to others
    QString toString(bool hidden = false) const;

    // helpers
    // static Card *Clone(const Card *other);
    static QString SuitToString(QSanguosha::Suit suit);
    static QString NumberToString(QSanguosha::Number number);
    static Card *Parse(const QString &str, RoomObject *room);

private:
    QSGS_DISABLE_COPY_MOVE_CONSTRUCT(Card)
};

struct CardDescriptor
{
    // const CardFace *face;
    // or following? or both?
    QString faceName;
    QSanguosha::Suit suit;
    QSanguosha::Number number;
    QString package;

    // property of card?

    // share some interfaces of Card?
    QString fullName(bool include_suit = false) const;
    QString logName() const;
    bool isBlack() const;
    bool isRed() const;
    const CardFace *face() const;

private:
    QSGS_DISABLE_COPY_MOVE_CONSTRUCT(CardDescriptor)
};

class Player : public QObject
{
public:
    void setScreenName(const QString &screen_name);
    QString screenName() const;

    // property setters/getters
    const IdSet &shownHandcards() const;
    void setShownHandcards(const IdSet &ids);
    bool isShownHandcard(int id) const;
    const IdSet &brokenEquips() const;
    void setBrokenEquips(const IdSet &ids);
    bool isBrokenEquip(int id, bool consider_shenbao = false) const;

    int hp() const;
    int renHp() const; //for banling
    int lingHp() const;
    void setHp(int hp);
    void setRenHp(int renhp);
    void setLingHp(int linghp);
    int dyingFactor() const;
    void setDyingFactor(int dyingFactor);
    int maxHp() const;
    void setMaxHp(int max_hp);
    int getLostHp() const;
    bool isWounded() const;
    QSanguosha::Gender gender() const;
    void setGender(QSanguosha::Gender gender);
    bool isMale() const;
    bool isFemale() const;
    bool isNeuter() const;

    bool hasShownRole() const;
    void setShownRole(bool shown);

    int maxCards(const QString &except = QString()) const;

    QString kingdom() const;
    void setKingdom(const QString &kingdom);

    void setRole(const QString &role);
    void setRole(QSanguosha::Role role);
    QString getRoleString() const;
    QSanguosha::Role role() const;

    void setGeneral(const General *general, int pos = 0);
    const General *general(int pos = 0) const;
    QString generalName(int pos = 0) const;
    QStringList generalNames() const;

    void setState(const QString &state);
    QString getState() const;

    int seat() const;
    void setSeat(int seat);
    bool isAdjacentTo(const Player *another) const;
    QString getPhaseString() const;
    void setPhaseString(const QString &phase_str);
    QSanguosha::Phase phase() const;
    void setPhase(QSanguosha::Phase phase);
    bool isInMainPhase() const;

    int getAttackRange(bool include_weapon = true) const;
    bool inMyAttackRange(const Player *other) const;

    bool isAlive() const;
    bool isDead() const;
    void setAlive(bool alive);

    QStringList flagList() const;
    void setFlag(const QString &flag);
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

    const General *avatarGeneral() const;

    bool isLord() const;
    bool isCurrent() const;

    void acquireSkill(const QString &skill_name);
    void detachSkill(const QString &skill_name);
    void detachAllSkills();
    void addSkill(const QString &skill_name, int place = 0);
    void loseSkill(const QString &skill_name, int place = 0);

    bool hasValidSkill(const QString &skill_name, bool include_lose = false, bool include_hidden = true) const;
    bool hasValidSkill(const Skill *skill, bool include_lose = false, bool include_hidden = true) const;
    bool hasValidLordSkill(const QString &skill_name, bool include_lose = false) const;
    bool hasValidLordSkill(const Skill *skill, bool include_lose = false) const;

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

    QList<const Card *> judgingAreaCards() const;
    QList<int> judgingArea() const; // DO NOT USE IdSet SINCE THE ORDER MATTERS
    void addDelayedTrick(const Card *trick);
    void removeDelayedTrick(const Card *trick);
    bool containsTrick(const QString &trick_name) const;

    int handcardNum() const;
    void removeCard(const Card *card, QSanguosha::Place place, const QString &pile_name = QString());
    void addCard(const Card *card, QSanguosha::Place place, const QString &pile_name = QString());
    IdSet handcards() const;
    void setHandCards(const IdSet &hc);
    QList<const Card *> handCards() const;

    const Card *weapon() const;
    const Card *armor() const;
    const Card *defensiveHorse() const;
    const Card *offensiveHorse() const;
    const Card *treasure() const;
    QList<const Card *> equipCards() const;
    IdSet equips() const;
    const Card *equipCard(int index) const;

    bool hasValidWeapon(const QString &weapon_name) const;
    bool hasValidArmor(const QString &armor_name) const;
    bool hasValidTreasure(const QString &treasure_name) const;

    bool isKongcheng() const;
    bool isNude() const;
    bool isAllNude() const;

    bool canDiscard(const Player *to, const QString &flags, const QString &reason = QString()) const;
    bool canDiscard(const Player *to, int card_id, const QString &reason = QString()) const;

    void addMark(const QString &mark, int add_num = 1);
    void removeMark(const QString &mark, int remove_num = 1);
    void setMark(const QString &mark, int value);
    int mark(const QString &mark) const;
    QMap<QString, int> marks() const;

    void setChained(bool chained);
    bool isChained() const;
    bool isDebuffStatus() const;

    void setRemoved(bool removed);
    bool isRemoved() const;

    bool canSlash(const Player *other, const Card *slash, bool distance_limit = true, int rangefix = 0, const QList<const Player *> &others = QList<const Player *>()) const;
    bool canSlash(const Player *other, bool distance_limit = true, int rangefix = 0, const QList<const Player *> &others = QList<const Player *>()) const;
    int getCardCount(bool include_equip = true, bool = false) const;

    IdSet pile(const QString &pile_name) const;
    QStringList pileNames() const;
    QString pileName(int card_id) const;

    IdSet getHandPile() const;
    QStringList getHandPileList(bool view_as_skill = true) const;

    void addHistory(const QString &name, int times = 1);
    void clearHistory();
    bool hasUsed(const QString &card_class) const;
    int usedTimes(const QString &card_class) const;
    int slashCount() const;
    int analapticCount() const;
    QHash<QString, int> histories() const;

    bool hasEquipSkill(const QString &skill_name) const;
    QSet<const Skill *> skills(bool include_equip = false, bool include_acquired = false, const QList<int> &positions = {}) const;

    const QSet<QString> &acquiredSkills() const;

    bool isProhibited(const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;
    bool canSlashWithoutCrossbow(const Card *slash = nullptr) const;
    bool isLastHandCard(const Card *card, bool contain = false) const;

    void setCardLimitation(const QString &limit_list, const QString &pattern, const QString &reason, bool single_turn = false);
    void removeCardLimitation(const QString &limit_list, const QString &pattern, const QString &reason, bool clearReason = false);
    void clearCardLimitation(bool single_turn = false);
    bool isCardLimited(const Card *card, QSanguosha::HandlingMethod method, bool isHandcard = false) const;
    bool isCardLimited(const QString &limit_list, const QString &reason) const;

    bool haveShownSkill(const Skill *skill) const; //hegemony
    bool haveShownSkill(const QString &skill_name) const; //hegemony
    bool haveShownSkills(const QString &skill_names) const;
    int findPositionOfGeneralOwningSkill(const QString &skill_name) const;
    void setSkillPreshowed(const QString &skill, bool preshowed = true); //hegemony
    void setSkillsPreshowed(const QList<int> &positions, bool preshowed = true);

    bool havePreshownSkill(const QString &name) const;
    bool havePreshownSkill(const Skill *skill) const;
    bool isHidden(int pos) const;

    bool haveShownGeneral(int pos = 0) const;
    void setShownGeneral(int pos, bool show);

    bool haveShownOneGeneral() const;
    bool haveShownAllGenerals() const;
    bool hasGeneralCardSkill(const QString &skill_name) const;
    bool hasGeneralCardSkill(const Skill *skill) const;
    bool isFriendWith(const Player *player, bool considerAnjiang = false) const;
    bool willBeFriendWith(const Player *player) const;

    QList<const Player *> getFormation() const;

    void addBrokenEquips(const IdSet &card_ids);
    void removeBrokenEquips(const IdSet &card_ids);
    void addToShownHandCards(const IdSet &card_ids);
    void removeShownHandCards(const IdSet &card_ids);

    RoomObject *roomObject() const;

    // TODO: d->tag?
    QVariantMap tag;

    // bool hasValidSkill()
private:
    QSGS_DISABLE_COPY_MOVE_CONSTRUCT(Player)
};
