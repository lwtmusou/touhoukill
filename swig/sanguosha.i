%module sgs

%{

#include "structs.h"
#include "engine.h"
#include "client.h"

#include <QDir>

%}

%include "naturalvar.i"
%include "native.i"
%include "qvariant.i"
%include "list.i"

// ----------------------------------------

class QObject {
public:
    QString objectName();
    void setObjectName(const char *name);
    bool inherits(const char *class_name);
    bool setProperty(const char *name, const QVariant &value);
    QVariant property(const char *name) const;
    void setParent(QObject *parent);
    void deleteLater();
};

class General: public QObject {
public:
    explicit General(Package *package, const char *name, const char *kingdom, int max_hp = 4, bool male = true, bool hidden = false, bool never_shown = false);

    // property getters/setters
    int getMaxHp() const;
    QString getKingdom() const;
    bool isMale() const;
    bool isFemale() const;
    bool isNeuter() const;
    bool isLord() const;
    bool isHidden() const;
    bool isTotallyHidden() const;

    bool isVisible() const;

    enum Gender { Sexless, Male, Female, Neuter };
    Gender getGender() const;
    void setGender(Gender gender);

    void addSkill(Skill *skill);
    void addSkill(const char *skill_name);
    bool hasSkill(const char *skill_name) const;
    QList<const Skill *> getSkillList() const;
    QList<const Skill *> getVisibleSkillList() const;
    QSet<const Skill *> getVisibleSkills() const;
    QSet<const TriggerSkill *> getTriggerSkills() const;

    void addRelateSkill(const char *skill_name);
    QStringList getRelatedSkillNames() const;

    QString getPackage() const;
    QString getSkillDescription(bool include_name = false, bool yellow = true) const;

    QSet<QString> getExtraSkillSet() const;

    void lastWord() const;
};

class Player: public QObject {
public:
    enum Phase { RoundStart, Start, Judge, Draw, Play, Discard, Finish, NotActive, PhaseNone };
    enum Place { PlaceHand, PlaceEquip, PlaceDelayedTrick, PlaceJudge, PlaceSpecial, DiscardPile, DrawPile, PlaceTable, PlaceUnknown };
    enum Role { Lord, Loyalist, Rebel, Renegade };

    explicit Player(QObject *parent);

    void setScreenName(const char *screen_name);
    QString screenName() const;

    // property setters/getters
    int getChaoren() const;//for chaoren
    void setChaoren(int chaoren);
    QList<int> getShownHandcards() const;
    bool isShownHandcard(int id);
	QList<int> getBrokenEquips() const;
	bool isBrokenEquip(int id) const;
    int getHp() const;
    int getRenHp() const;
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
    General::Gender getGender() const;
    virtual void setGender(General::Gender gender);
    bool isMale() const;
    bool isFemale() const;
    bool isNeuter() const;

    bool hasShownRole() const;
    void setShownRole(bool shown);

    int getMaxCards(const char *except = NULL) const;

    QString getKingdom() const;
    void setKingdom(const char *kingdom);

    void setRole(const char *role);
    QString getRole() const;
    Role getRoleEnum() const;

    void setGeneral(const General *general);
    void setGeneralName(const char *general_name);
    QString getGeneralName() const;

    void setGeneral2Name(const char *general_name);
    QString getGeneral2Name() const;
    const General *getGeneral2() const;

    void setState(const char *state);
    QString getState() const;

    int getSeat() const;
    void setSeat(int seat);
    bool isAdjacentTo(const Player *another) const;
    QString getPhaseString() const;
    void setPhaseString(const char *phase_str);
    Phase getPhase() const;
    void setPhase(Phase phase);

    int getAttackRange(bool include_weapon = true) const;
    bool inMyAttackRange(const Player *other) const;

    bool isAlive() const;
    bool isDead() const;
    void setAlive(bool alive);

    QString getFlags() const;
    QStringList getFlagList() const;
    virtual void setFlags(const char *flag);
    bool hasFlag(const char *flag) const;
    void clearFlags();

    bool faceUp() const;
    void setFaceUp(bool face_up);

    virtual int aliveCount(bool includeRemoved = true) const = 0;
    void setFixedDistance(const Player *player, int distance);
	int originalRightDistanceTo(const Player *other) const;
    int distanceTo(const Player *other, int distance_fix = 0) const;
    const General *getAvatarGeneral() const;
    const General *getGeneral() const;

    bool isLord() const;
	bool isCurrent() const;

    void acquireSkill(const char *skill_name);
    void detachSkill(const char *skill_name);
    void detachAllSkills();
    virtual void addSkill(const char *skill_name);
    virtual void loseSkill(const char *skill_name);
    bool hasSkill(const char *skill_name, bool include_lose = false, bool include_hidden = true) const;
    bool hasSkills(const char *skill_name, bool include_lose = false) const;
    bool hasInnateSkill(const char *skill_name) const;
    bool hasLordSkill(const char *skill_name, bool include_lose = false) const;
    virtual QString getGameMode() const = 0;

    void setSkillInvalidity(const Skill *skill, bool invalidity);
    void setSkillInvalidity(const char *skill_name, bool invalidity);

    bool isSkillInvalid(const Skill *skill) const;
    bool isSkillInvalid(const char *skill_name) const;

    void setEquip(WrappedCard *equip);
    void removeEquip(WrappedCard *equip);
    bool hasEquip(const Card *card) const;
    bool hasEquip() const;

    QList<const Card *> getJudgingArea() const;
    QList<int> getJudgingAreaID() const; //for marshal
    void addDelayedTrick(const Card *trick);
    void removeDelayedTrick(const Card *trick);
    bool containsTrick(const char *trick_name) const;

    virtual int getHandcardNum() const = 0;
    virtual void removeCard(const Card *card, Place place) = 0;
    virtual void addCard(const Card *card, Place place) = 0;
    virtual QList<const Card *> getHandcards() const = 0;

    WrappedCard *getWeapon() const;
    WrappedCard *getArmor() const;
    WrappedCard *getDefensiveHorse() const;
    WrappedCard *getOffensiveHorse() const;
    WrappedCard *getTreasure() const;
    QList<const Card *> getEquips() const;
    const EquipCard *getEquip(int index) const;

    bool hasWeapon(const char *weapon_name, bool selfOnly = false) const;
    bool hasArmorEffect(const char *armor_name, bool selfOnly = false) const;
    bool hasTreasure(const char *treasure_name) const;

    bool isKongcheng() const;
    bool isNude() const;
    bool isAllNude() const;

    bool canDiscard(const Player *to, const char *flags) const;
    bool canDiscard(const Player *to, int card_id) const;

    void addMark(const char *mark, int add_num = 1);
    void removeMark(const char *mark, int remove_num = 1);
    virtual void setMark(const char *mark, int value);
    int getMark(const char *mark) const;

    void setChained(bool chained);
    bool isChained() const;

    bool canSlash(const Player *other, const Card *slash, bool distance_limit = true,
                  int rangefix = 0, const QList<const Player *> &others = QList<const Player *>()) const;
    bool canSlash(const Player *other, bool distance_limit = true,
                  int rangefix = 0, const QList<const Player *> &others = QList<const Player *>()) const;
    int getCardCount(bool include_equip = true, bool = false) const;

    QList<int> getPile(const char *pile_name) const;
    QStringList getPileNames() const;
    QString getPileName(int card_id) const;

    bool pileOpen(const char *pile_name, const char *player) const;
    void setPileOpen(const char *pile_name, const char *player);
    QList<int> getHandPile() const;

    void addHistory(const char *name, int times = 1);
    void clearHistory();
    bool hasUsed(const char *card_class) const;
    int usedTimes(const char *card_class) const;
    int getSlashCount() const;

    bool hasEquipSkill(const char *skill_name) const;
    QSet<const TriggerSkill *> getTriggerSkills() const;
    QSet<const Skill *> getSkills(bool include_equip = false, bool visible_only = true) const;
    QList<const Skill *> getSkillList(bool include_equip = false, bool visible_only = true) const;
    QSet<const Skill *> getVisibleSkills(bool include_equip = false) const;
    QList<const Skill *> getVisibleSkillList(bool include_equip = false) const;
    QSet<QString> getAcquiredSkills() const;
    QString getSkillDescription(bool yellow = true) const;

    virtual bool isProhibited(const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;
    bool canSlashWithoutCrossbow(const Card *slash = NULL) const;
    virtual bool isLastHandCard(const Card *card, bool contain = false) const = 0;

    bool isJilei(const Card *card) const;
    bool isLocked(const Card *card) const;

    void setCardLimitation(const char *limit_list, const char *pattern, bool single_turn = false);
    void removeCardLimitation(const char *limit_list, const char *pattern);
    void clearCardLimitation(bool single_turn = false);
    bool isCardLimited(const Card *card, Card::HandlingMethod method, bool isHandcard = false) const;

    // just for convenience
    void addQinggangTag(const Card *card);
    void removeQinggangTag(const Card *card);

    void copyFrom(Player *p);

    QList<const Player *> getSiblings() const;
    QList<const Player *> getAliveSiblings() const;


	void setNext(Player *next);
    void setNext(const char *next);
    Player *getNext(bool ignoreRemoved = true) const;
    QString getNextName() const;
    Player *getLast(bool ignoreRemoved = true) const;
    Player *getNextAlive(int n = 1, bool ignoreRemoved = true) const;
    Player *getLastAlive(int n = 1, bool ignoreRemoved = true) const;
};

%extend Player {
    void setTag(const char *key, QVariant &value) {
        $self->tag[key] = value;
    }

    QVariant getTag(const char *key) {
        return $self->tag[key];
    }

    void removeTag(const char *tag_name) {
        $self->tag.remove(tag_name);
    }
};

class ServerPlayer: public Player {
public:
    explicit ServerPlayer(Room *room);

    void invoke(const char *method, const char *arg = ".");
    QString reportHeader() const;
    void unicast(const char *message);
    void drawCard(const Card *card);
    Room *getRoom() const;
    void broadcastSkillInvoke(const Card *card) const;
    void broadcastSkillInvoke(const char *card_name) const;
    int getRandomHandCardId() const;
    const Card *getRandomHandCard() const;
    void obtainCard(const Card *card, bool unhide = true);
    void throwAllEquips();
    void throwAllHandCards();
    void throwAllHandCardsAndEquips();
    void throwAllCards();
    void bury();
    void throwAllMarks(bool visible_only = true);
    void clearOnePrivatePile(const char *pile_name);
    void clearPrivatePiles();
    void drawCards(int n, const char *reason = NULL);
    bool askForSkillInvoke(const char *skill_name, const QVariant &data = QVariant());
    QList<int> forceToDiscard(int discard_num, bool include_equip, bool is_discard = true);
    QList<int> handCards() const;
    virtual QList<const Card *> getHandcards() const;
    QList<const Card *> getCards(const char *flags) const;
    DummyCard *wholeHandCards() const;
    bool hasNullification() const;
    bool pindian(ServerPlayer *target, const char *reason, const Card *card1 = NULL);
    void turnOver();
    void play(QList<Player::Phase> set_phases = QList<Player::Phase>());
    bool changePhase(Player::Phase from, Player::Phase to);

    QList<Player::Phase> &getPhases();
    void skip(Player::Phase phase, bool isCost = false, bool sendLog = true);
    void insertPhases(QList<Player::Phase> new_phases, int index = -1);
    bool isSkipped(Player::Phase phase);

    void gainMark(const char *mark, int n = 1);
    void loseMark(const char *mark, int n = 1);
    void loseAllMarks(const char *mark_name);

    virtual void addSkill(const char *skill_name);
    virtual void loseSkill(const char *skill_name);
    virtual void setGender(General::Gender gender);

    void setAI(AI *ai);
    AI *getAI() const;
    AI *getSmartAI() const;

    bool isOnline() const;
    bool isOffline() const;

    virtual int aliveCount(bool includeRemoved = true) const;
    virtual int getHandcardNum() const;
    virtual void removeCard(const Card *card, Place place);
    virtual void addCard(const Card *card, Place place);
    virtual bool isLastHandCard(const Card *card, bool contain = false) const;

    void addVictim(ServerPlayer *victim);
    QList<ServerPlayer *> getVictims() const;

    void startRecord();
    void saveRecord(const char *filename);

    //void setNext(ServerPlayer *next);
    //ServerPlayer *getNext() const;
    //ServerPlayer *getNextAlive(int n = 1) const;

    void addToSelected(const char *general);
    QStringList getSelected() const;
    void clearSelected();

    int getGeneralMaxHp() const;
    virtual QString getGameMode() const;

    QString getIp() const;
    void introduceTo(ServerPlayer *player);
    void marshal(ServerPlayer *player) const;

    void addToPile(const char *pile_name, const Card *card, bool open = true);
    void addToPile(const char *pile_name, int card_id, bool open = true);
    void addToPile(const char *pile_name, QList<int> card_ids, bool open = true);
    void addToPile(const char *pile_name, QList<int> card_ids, bool open, CardMoveReason reason, QList<ServerPlayer *> open_players = QList<ServerPlayer *>());
    void gainAnExtraTurn();

    void copyFrom(ServerPlayer *sp);
};

%extend ServerPlayer {
    void speak(const char *msg) {
        QString str = QByteArray(msg).toBase64();
        $self->getRoom()->speakCommand($self, str);
    }
};

class ClientPlayer: public Player {
public:
    explicit ClientPlayer(Client *client);
    virtual void setFlags(const char *flag);
    virtual int aliveCount(bool includeRemoved = true) const;
    virtual int getHandcardNum() const;
    virtual void removeCard(const Card *card, Place place);
    virtual void addCard(const Card *card, Place place);
    virtual void addKnownHandCard(const Card *card);
    virtual bool isLastHandCard(const Card *card, bool contain = false) const;
    virtual void setMark(const char *mark, int value);
};

extern ClientPlayer *Self;

class CardMoveReason {
public:
    int m_reason;
    QString m_playerId; // the cause (not the source) of the movement, such as "lusu" when "dimeng", or "zhanghe" when "qiaobian"
    QString m_targetId; // To keep this structure lightweight, currently this is only used for UI purpose.
                        // It will be set to empty if multiple targets are involved. NEVER use it for trigger condition
                        // judgement!!! It will not accurately reflect the real reason.
    QString m_skillName; // skill that triggers movement of the cards, such as "longdang", "dimeng"
    QString m_eventName; // additional arg such as "lebusishu" on top of "S_REASON_JUDGE"
    QVariant m_extraData; // additional data and will not be parsed to clients
    QVariant m_provider; // additional data record who provide this card for otherone to use or response,
    //etc. guanyu provide a slash for "jijiang"

    CardMoveReason();
    CardMoveReason(int moveReason, char *playerId);
    CardMoveReason(int moveReason, char *playerId, char *skillName, char *eventName);
    CardMoveReason(int moveReason, char *playerId, char *targetId, char *skillName, char *eventName);

    static const int S_REASON_UNKNOWN = 0x00;
    static const int S_REASON_USE = 0x01;
    static const int S_REASON_RESPONSE = 0x02;
    static const int S_REASON_DISCARD = 0x03;
    static const int S_REASON_RECAST = 0x04;          // ironchain etc.
    static const int S_REASON_PINDIAN = 0x05;
    static const int S_REASON_DRAW = 0x06;
    static const int S_REASON_GOTCARD = 0x07;
    static const int S_REASON_SHOW = 0x08;
    static const int S_REASON_TRANSFER = 0x09;
    static const int S_REASON_PUT = 0x0A;

    //subcategory of use
    static const int S_REASON_LETUSE = 0x11;           // use a card when self is not current

    //subcategory of response
    static const int S_REASON_RETRIAL = 0x12;

    //subcategory of discard
    static const int S_REASON_RULEDISCARD = 0x13;       //  discard at one's Player::Discard for gamerule
    static const int S_REASON_THROW = 0x23;             /*  gamerule(dying or punish)
                                                            as the cost of some skills   */
    static const int S_REASON_DISMANTLE = 0x33;         //  one throw card of another

    //subcategory of gotcard
    static const int S_REASON_GIVE = 0x17;              // from one hand to another hand
    static const int S_REASON_EXTRACTION = 0x27;        // from another's place to one's hand
    static const int S_REASON_GOTBACK = 0x37;           // from placetable to hand
    static const int S_REASON_RECYCLE = 0x47;           // from discardpile to hand
    static const int S_REASON_ROB = 0x57;               // got a definite card from other's hand
    static const int S_REASON_PREVIEWGIVE = 0x67;       // give cards after previewing, i.e. Yiji & Miji

    //subcategory of show
    static const int S_REASON_TURNOVER = 0x18;          // show n cards  from drawpile
    static const int S_REASON_JUDGE = 0x28;             // show a card  from drawpile for judge
    static const int S_REASON_PREVIEW = 0x38;           // Not done yet, plan for view some cards for self only(guanxing yiji miji)
    static const int S_REASON_DEMONSTRATE = 0x48;       // show a card which copy one to move to table

    //subcategory of transfer
    static const int S_REASON_SWAP = 0x19;              // exchange card for two players
    static const int S_REASON_OVERRIDE = 0x29;          // exchange cards from cards in game
    static const int S_REASON_EXCHANGE_FROM_PILE = 0x39;// exchange cards from cards moved out of game (for qixing only)

    //subcategory of put
    static const int S_REASON_NATURAL_ENTER = 0x1A;     //  a card with no-owner move into discardpile
    static const int S_REASON_REMOVE_FROM_PILE = 0x2A;  //  cards moved out of game go back into discardpile
    static const int S_REASON_JUDGEDONE = 0x3A;         //  judge card move into discardpile
    static const int S_REASON_CHANGE_EQUIP = 0x4A;      //  replace existed equip


    static const int S_MASK_BASIC_REASON = 0x0F;
};

struct DamageStruct {
    DamageStruct();
    DamageStruct(const Card *card, ServerPlayer *from, ServerPlayer *to, int damage = 1, DamageStruct::Nature nature = Normal);
    DamageStruct(const char *reason, ServerPlayer *from, ServerPlayer *to, int damage = 1, DamageStruct::Nature nature = Normal);

    enum Nature {
        Normal, // normal slash, duel and most damage caused by skill
        Fire,  // fire slash, fire attack and few damage skill (Yeyan, etc)
        Thunder, // lightning, thunder slash, and few damage skill (Leiji, etc)
        Ice
    };

    ServerPlayer *from;
    ServerPlayer *to;
    const Card *card;
    int damage;
    Nature nature;
    bool chain;
    bool transfer;
    bool by_user;
    QString reason;
	bool trigger_chain;

    QString getReason() const;
};

struct CardEffectStruct {
    CardEffectStruct();

    const Card *card;

    ServerPlayer *from;
    ServerPlayer *to;
    bool multiple;
    bool nullified;
	bool canceled;
};

struct SlashEffectStruct {
    SlashEffectStruct();

    int jink_num;

    const Card *slash;
    const Card *jink;

    ServerPlayer *from;
    ServerPlayer *to;

    int drank;

    DamageStruct::Nature nature;
    bool multiple;
    bool nullified;
	bool canceled;
};

struct CardUseStruct {
    enum CardUseReason {
        CARD_USE_REASON_UNKNOWN = 0x00,
        CARD_USE_REASON_PLAY = 0x01,
        CARD_USE_REASON_RESPONSE = 0x02,
        CARD_USE_REASON_RESPONSE_USE = 0x12
    } m_reason;

    CardUseStruct();
    CardUseStruct(const Card *card, ServerPlayer *from, QList<ServerPlayer *> to = QList<ServerPlayer *>(), bool isOwnerUse = true);
    CardUseStruct(const Card *card, ServerPlayer *from, ServerPlayer *target, bool isOwnerUse = true);
    bool isValid(const char *pattern) const;
    void parse(const char *str, Room *room);
    bool tryParse(const QVariant &, Room *room);

    const Card *card;
    ServerPlayer *from;
    QList<ServerPlayer *> to;
    bool m_isOwnerUse;
    bool m_addHistory;
    bool m_isHandcard;
    bool m_isLastHandcard;
    QStringList nullified_list;
};

struct CardsMoveStruct {
    CardsMoveStruct();
    CardsMoveStruct(const QList<int> &ids, Player *from, Player *to, Player::Place from_place, Player::Place to_place, CardMoveReason reason);
    CardsMoveStruct(const QList<int> &ids, Player *to, Player::Place to_place, CardMoveReason reason);
    CardsMoveStruct(int id, Player *from, Player *to, Player::Place from_place, Player::Place to_place, CardMoveReason reason);
    CardsMoveStruct(int id, Player *to, Player::Place to_place, CardMoveReason reason);

    QList<int> card_ids;
    Player::Place from_place, to_place;
    QString from_player_name, to_player_name;
    QString from_pile_name, to_pile_name;
    Player *from, *to;
    CardMoveReason reason;
    bool open;
    bool is_last_handcard;
};

struct CardsMoveOneTimeStruct {
    QList<int> card_ids;
    QList<Player::Place> from_places;
    Player::Place to_place;
    CardMoveReason reason;
    Player *from, *to;
    QStringList from_pile_names;
    QString to_pile_name;

    QList<Player::Place> origin_from_places;
    Player::Place origin_to_place;
    Player *origin_from, *origin_to;
    QStringList origin_from_pile_names;
    QString origin_to_pile_name; //for case of the movement transitted

    QList<bool> open; // helper to prevent sending card_id to unrelevant clients
    bool is_last_handcard;
};

struct DyingStruct
{
    DyingStruct();

    ServerPlayer *who; // who is ask for help
    DamageStruct *damage; // if it is NULL that means the dying is caused by losing hp
    ServerPlayer *nowAskingForPeaches; // who is asking for peaches
};

struct DeathStruct
{
    DeathStruct();

    ServerPlayer *who; // who is dead
    DamageStruct *damage; // if it is NULL that means the dying is caused by losing hp

    ServerPlayer *viewAsKiller;
    bool useViewAsKiller;
};

struct RecoverStruct
{
    RecoverStruct();

    int recover;
    ServerPlayer *who;
    ServerPlayer *to;
    const Card *card;
    QString reason;
};

struct PindianStruct {
    PindianStruct();

    ServerPlayer *from;
    ServerPlayer *to;
	ServerPlayer *askedPlayer;
    const Card *from_card;
    const Card *to_card;
    int from_number;
    int to_number;
    QString reason;
    bool success;
};

struct JudgeStruct {
    JudgeStruct();
    bool isGood() const;
    bool isBad() const;
    bool isEffected() const;
    void updateResult();

    bool isGood(const Card *card) const; // For AI

    ServerPlayer *who;
    const Card *card;
    QString pattern;
    bool good;
    QString reason;
    bool time_consuming;
    bool negative;
    bool play_animation;
    ServerPlayer *retrial_by_response; // record whether the current judge card is provided by a response retrial
    ServerPlayer *relative_player; // record relative player like skill owner of "huazhong", for processing the case like "huazhong -> dizhen -> huazhong"
};

struct PhaseChangeStruct
{
    PhaseChangeStruct();

    Player::Phase from;
    Player::Phase to;
    ServerPlayer *player;
};

struct PhaseSkippingStruct
{
    PhaseSkippingStruct();

    Player::Phase phase;
    ServerPlayer *player;
    bool isCost;
};

struct CardResponseStruct {
    CardResponseStruct(const Card *card = NULL, ServerPlayer *who = NULL, bool isuse = false, bool isRetrial = false, bool isProvision = false, ServerPlayer *from = NULL);

    const Card *m_card;
    ServerPlayer *m_who;
    bool m_isUse;
    bool m_isRetrial;
    bool m_isProvision;
    bool m_isHandcard;
    ServerPlayer *m_from;
    bool m_isNullified;
};

struct MarkChangeStruct{
    MarkChangeStruct();

    int num;
    QString name;
    ServerPlayer *player;
};

struct SkillAcquireDetachStruct
{
    SkillAcquireDetachStruct();

    const Skill *skill;
    ServerPlayer *player;
    bool isAcquire;
};

struct CardAskedStruct
{
    CardAskedStruct();

    QString pattern;
    QString prompt;
    ServerPlayer *player;
    Card::HandlingMethod method;
};

struct BrokenEquipChangedStruct
{
    BrokenEquipChangedStruct();

    ServerPlayer *player;
    QList<int> ids;
    bool broken;
    bool moveFromEquip;
};

struct ShownCardChangedStruct
{
    ShownCardChangedStruct();

    ServerPlayer *player;
    QList<int> ids;
    bool shown;
    bool moveFromHand;
};




struct SkillInvokeDetail
{
    explicit SkillInvokeDetail(const TriggerSkill *skill = NULL, ServerPlayer *owner = NULL, ServerPlayer *invoker = NULL, QList<ServerPlayer *> targets = QList<ServerPlayer *>(), bool isCompulsory = false, ServerPlayer *preferredTarget = NULL);
    SkillInvokeDetail(const TriggerSkill *skill, ServerPlayer *owner, ServerPlayer *invoker, ServerPlayer *target, bool isCompulsory = false, ServerPlayer *preferredTarget = NULL);

    const TriggerSkill *skill; // the skill
    ServerPlayer *owner; // skill owner. 2 structs with the same skill and skill owner are treated as of a same skill.
    ServerPlayer *invoker; // skill invoker. When invoking skill, we sort firstly according to the priority, then the seat of invoker, at last weather it is a skill of an equip.
    QList<ServerPlayer *> targets; // skill targets.
    bool isCompulsory; // judge the skill is compulsory or not. It is set in the skill's triggerable
    bool triggered; // judge whether the skill is triggered
    ServerPlayer *preferredTarget; // the preferred target of a certain skill


    bool sameSkill(const SkillInvokeDetail &arg2) const; // the operator ==. it only judge the skill name, the skill invoker, and the skill owner. it don't judge the skill target because it is chosen by the skill invoker
    bool sameTimingWith(const SkillInvokeDetail &arg2) const; // used to judge 2 skills has the same timing. only 2 structs with the same priority and the same invoker and the same "whether or not it is a skill of equip"
    bool preferredTargetLess(const SkillInvokeDetail &arg2) const;
};

%extend SkillInvokeDetail {
    QVariant getTag(const char *key)
    {
        return $self->tag.value(QString(key));
    }

    void setTag(const char *key, QVariant *value)
    {
        $self->tag[QString(key)] = *value;
    }

    void removeTag(const char *key)
    {
        $self->tag.remove(QString(key));
    }

    bool lessThan(SkillInvokeDetail *arg2)
    {
        return *$self < *arg2;
    }

    void addTarget(ServerPlayer *target)
    {
        $self->targets << target;
    }
};

struct HpLostStruct
{
    HpLostStruct();

    ServerPlayer *player;
    int num;
};

struct JinkEffectStruct
{
    JinkEffectStruct();

    SlashEffectStruct slashEffect;
    const Card *jink;
};

struct DrawNCardsStruct
{
    DrawNCardsStruct();

    ServerPlayer *player;
    int n;
    bool isInitial;
};

struct SkillInvalidStruct
{
    SkillInvalidStruct();

    ServerPlayer *player;
    const Skill *skill;
    bool invalid;
};

struct ChoiceMadeStruct
{
    ChoiceMadeStruct();

    enum ChoiceType
    {
        NoChoice,

        SkillInvoke,
        SkillChoice,
        Nullification,
        CardChosen,
        CardResponded,
        CardUsed,
        AGChosen,
        CardShow,
        Peach,
        TriggerOrder,
        ReverseFor3v3,
        Activate,
        Suit,
        Kingdom,
        CardDiscard,
        CardExchange,
        ViewCards,
        PlayerChosen,
        Rende,
        Yiji,
        Pindian,

        NumOfChoices
    };

    ServerPlayer *player;
    ChoiceType type;
    QStringList args;
};

enum TriggerEvent
{
    NonTrigger,

    GameStart,
    TurnStart,
    EventPhaseStart,
    EventPhaseProceeding,
    EventPhaseEnd,
    EventPhaseChanging,
    EventPhaseSkipping,

    DrawNCards,
    AfterDrawNCards,
    DrawInitialCards,
    AfterDrawInitialCards,

    PreHpRecover,
    HpRecover,
    PreHpLost,
    PostHpLost,
    HpChanged,
    MaxHpChanged,
    PostHpReduced,

    EventLoseSkill,
    EventAcquireSkill,
    EventSkillInvalidityChange,

    StartJudge,
    AskForRetrial,
    FinishRetrial,
    FinishJudge,

    PindianVerifying,
    Pindian,

    TurnedOver,
    ChainStateChanged,
    RemoveStateChanged,

    ConfirmDamage,    // confirm the damage's count and damage's nature
    Predamage,        // trigger the certain skill -- jueqing
    DamageForseen,    // the first event in a damage -- kuangfeng dawu
    DamageCaused,     // the moment for -- qianxi..
    DamageInflicted,  // the moment for -- tianxiang..
    PreDamageDone,    // before reducing Hp
    DamageDone,       // it's time to do the damage
    Damage,           // the moment for -- lieren..
    Damaged,          // the moment for -- yiji..
    DamageComplete,   // the moment for trigger iron chain

    EnterDying,
    Dying,
    QuitDying,
    AskForPeaches,
    AskForPeachesDone,
    Death,
    BuryVictim,
    BeforeGameOverJudge,
    GameOverJudge,
    GameFinished,
    Revive,

    SlashEffected,
    SlashProceed,
    SlashHit,
    SlashMissed,

    JinkEffect,

    CardAsked,
    CardResponded,
    BeforeCardsMove, // sometimes we need to record cards before the move
    CardsMoveOneTime,

    PreCardUsed, // for AI to filter events only.
    CardUsed,
    TargetSpecifying,
    TargetConfirming,
    TargetSpecified,
    TargetConfirmed,
    CardEffect, // for AI to filter events only
    CardEffected,
    PostCardEffected,
    CardFinished,
    TrickCardCanceling,
    TrickEffect,

    PreMarkChange,
    MarkChanged,

    ChoiceMade,

    FetchDrawPileCard, // For miniscenarios only
    ActionedReset, // For 3v3 only
    Debut, // For 1v1 only

    TurnBroken, // For the skill 'DanShou'. Do not use it to trigger events

    //new events for touhoukill,
    DrawPileSwaped,//like qiannian
    AfterGuanXing,
    KingdomChanged,

    NumOfEvents
};

class Card: public QObject {
public:
    enum Suit { Spade, Club, Heart, Diamond, NoSuitBlack, NoSuitRed, NoSuit, SuitToBeDecided = -1 };
    enum Color { Red, Black, Colorless };
    enum HandlingMethod { MethodNone, MethodUse, MethodResponse, MethodDiscard, MethodRecast, MethodPindian };

    static const Suit AllSuits[4];

    // card types
    enum CardType { TypeSkill, TypeBasic, TypeTrick, TypeEquip };

    // constructor
    Card(Suit suit, int number, bool target_fixed = false);

    // property getters/setters
    QString getSuitString() const;
    bool isRed() const;
    bool isBlack() const;
    int getId() const;
    virtual void setId(int id);
    int getEffectiveId() const;

    int getNumber() const;
    virtual void setNumber(int number);
    QString getNumberString() const;

    Suit getSuit() const;
    virtual void setSuit(Suit suit);

    bool sameColorWith(const Card *other) const;
    Color getColor() const;
    QString getFullName(bool include_suit = false) const;
    QString getLogName() const;
    QString getName() const;
    QString getSkillName(bool removePrefix = true) const;
    virtual void setSkillName(const char *skill_name);
    QString getDescription(bool yellow = true) const;

    virtual bool isMute() const;
	virtual bool canDamage() const;
    virtual bool willThrow() const;
    virtual bool canRecast() const;
    void setCanRecast(bool can);
    virtual bool hasPreAction() const;
    virtual Card::HandlingMethod getHandlingMethod() const;

    virtual void setFlags(const char *flag) const;
    bool hasFlag(const char *flag) const;
    virtual void clearFlags() const;

    virtual QString getPackage() const;
    virtual QString getClassName() const;
    virtual bool isVirtualCard() const;
    virtual bool isEquipped() const;
    virtual QString getCommonEffectName() const;
    virtual bool match(const char *pattern) const;

    virtual void addSubcard(int card_id);
    virtual void addSubcard(const Card *card);
    virtual QList<int> getSubcards() const;
    virtual void clearSubcards();
    virtual QString subcardString() const;
    virtual void addSubcards(const QList<const Card *> &cards);
    virtual void addSubcards(const QList<int> &subcards_list);
    virtual int subcardsLength() const;


    virtual QString getType() const = 0;
    virtual QString getSubtype() const = 0;
    virtual CardType getTypeId() const = 0;
    virtual bool isNDTrick() const;

    // card target selection
    virtual bool targetFixed() const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    // @todo: the following two functions should be merged into one.
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self,
                              int &maxVotes) const;
    virtual bool isAvailable(const Player *player) const;

    virtual const Card *getRealCard() const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;

    virtual void doPreAction(Room *room, const CardUseStruct &card_use) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual bool isCancelable(const CardEffectStruct &effect) const;

    virtual bool isKindOf(const char *cardType) const;
    virtual QStringList getFlags() const;

    virtual bool isModified() const;
    virtual void onNullified(ServerPlayer *) const;

    // static functions
    static bool CompareByNumber(const Card *a, const Card *b);
    static bool CompareBySuit(const Card *a, const Card *b);
    static bool CompareByType(const Card *a, const Card *b);
    static Card *Clone(const Card *card);
    static QString Suit2String(Suit suit);
    static const int S_UNKNOWN_CARD_ID;

    static const Card *Parse(const char *str);
    virtual QString toString(bool hidden = false) const;

    virtual QString getEffectName() const;
};

%extend Card {
    EquipCard *toEquipCard() {
        return qobject_cast<EquipCard *>($self);
    }

    Weapon *toWeapon() {
        return qobject_cast<Weapon *>($self);
    }

    WrappedCard *toWrapped() {
        return qobject_cast<WrappedCard *>($self);
    }

    TrickCard *toTrick() {
        return qobject_cast<TrickCard *>($self);
    }

    void cardOnUse(Room *room, const CardUseStruct &card_use) const{
         $self->Card::onUse(room, card_use);
    }

    bool cardIsAvailable(const Player *player) const{
        return $self->Card::isAvailable(player);
    }
};

class WrappedCard: public Card {
public:
    void takeOver(Card *card);
    void copyEverythingFrom(Card *card);
    void setModified(bool modified);
};

class SkillCard: public Card {
public:
    SkillCard();
    void setUserString(const char *user_string);
    QString getUserString() const;

    virtual QString getSubtype() const;
    virtual QString getType() const;
    virtual CardType getTypeId() const;
    virtual QString toString(bool hidden = false) const;
};

class DummyCard: public Card {
public:
    DummyCard();
    DummyCard(const QList<int> &subcards);

    virtual QString getSubtype() const;
    virtual QString getType() const;
    virtual QString toString(bool hidden = false) const;
};

class Package: public QObject {
public:
    enum Type { GeneralPack, CardPack, MixedPack, SpecialPack };

    Package(const char *name, Type pack_type = GeneralPack);
    void insertRelatedSkills(const char *main_skill, const char *related_skill);
};

class Engine: public QObject {
public:
    void addTranslationEntry(const char *key, const char *value);
    QString translate(const char *to_translate) const;
    lua_State *getLuaState() const;

    int getMiniSceneCounts();

    void addPackage(Package *package);
    void addBanPackage(const char *package_name);
    QStringList getBanPackages() const;
    Card *cloneCard(const Card *card) const;
    Card *cloneCard(const char *name, Card::Suit suit = Card::SuitToBeDecided, int number = -1) const;
    SkillCard *cloneSkillCard(const char *name) const;
    QString getVersionNumber() const;
    QString getVersion() const;
    QString getVersionName() const;
    QString getMODName() const;
    QStringList getExtensions() const;
    QStringList getKingdoms() const;
    QString getSetupString() const;

    QString getModeName(const char *mode) const;
    int getPlayerCount(const char *mode) const;
    QString getRoles(const char *mode) const;
    QStringList getRoleList(const char *mode) const;
    int getRoleIndex() const;

    const CardPattern *getPattern(const char *name) const;
    bool matchExpPattern(const char *pattern, const Player *player, const Card *card) const;
    Card::HandlingMethod getCardHandlingMethod(const char *method_name) const;
    QList<const Skill *> getRelatedSkills(const char *skill_name) const;
    const Skill *getMainSkill(const char *skill_name) const;

    QStringList getModScenarioNames() const;
    void addScenario(Scenario *scenario);
    const Scenario *getScenario(const char *name) const;
    void addPackage(const char *name);

    const General *getGeneral(const char *name) const;
    int getGeneralCount(bool include_banned = false) const;
    const Skill *getSkill(const char *skill_name) const;
    const Skill *getSkill(const EquipCard *card) const;
    QStringList getSkillNames() const;
    const TriggerSkill *getTriggerSkill(const char *skill_name) const;
    const ViewAsSkill *getViewAsSkill(const char *skill_name) const;
    QList<const DistanceSkill *> getDistanceSkills() const;
    QList<const MaxCardsSkill *> getMaxCardsSkills() const;
    QList<const TargetModSkill *> getTargetModSkills() const;
    QList<const AttackRangeSkill *> getAttackRangeSkills() const;
    QList<const TriggerSkill *> getGlobalTriggerSkills() const;
    void addSkills(const QList<const Skill *> &skills);

    int getCardCount() const;
    const Card *getEngineCard(int cardId) const;
    Card *getCard(int cardId);
    WrappedCard *getWrappedCard(int cardId);

    QStringList getLords(bool contain_banned = false) const;
    QStringList getRandomLords() const;
    void banRandomGods() const;
    QStringList getRandomGenerals(int count) const;
    QList<int> getRandomCards() const;
    QString getRandomGeneralName() const;
    QStringList getLimitedGeneralNames() const;

    void playSystemAudioEffect(const char *name) const;
    void playAudioEffect(const char *filename) const;
    void playSkillAudioEffect(const char *skill_name, int index) const;

    const ProhibitSkill *isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;
    int correctDistance(const Player *from, const Player *to) const;
    int correctMaxCards(const Player *target, bool fixed = false, const char *except = NULL) const;
    int correctCardTarget(const TargetModSkill::ModType type, const Player *from, const Card *card) const;
    int correctAttackRange(const Player *target, bool include_weapon = true, bool fixed = false) const;

    void registerRoom(QObject *room);
    void unregisterRoom();
    Room *currentRoom();

    QString getCurrentCardUsePattern();
    CardUseStruct::CardUseReason getCurrentCardUseReason();

    bool isGeneralHidden(const char *general_name) const;
};

extern Engine *Sanguosha;

class Skill: public QObject {
public:
    enum Frequency { Frequent, NotFrequent, Compulsory, NotCompulsory,Limited, Wake, Eternal};

    explicit Skill(const char *name, Frequency frequent = NotFrequent);
    bool isLordSkill() const;
    bool isAttachedLordSkill() const;
    QString getDescription(bool yellow = true) const;
    bool isVisible() const;

    virtual int getEffectIndex(const ServerPlayer *player, const Card *card) const;
    virtual QDialog *getDialog() const;

    void initMediaSource();
    void playAudioEffect(int index = -1) const;
    Frequency getFrequency() const;
    QStringList getSources() const;
};

%extend Skill {
    const TriggerSkill *toTriggerSkill() const{
        return qobject_cast<const TriggerSkill *>($self);
    }
};

class TriggerSkill: public Skill {
public:
    TriggerSkill(const char *name);
    const ViewAsSkill *getViewAsSkill() const;
    QList<TriggerEvent> getTriggerEvents() const;

    virtual int getPriority() const;

    bool isGlobal() const;
};

%{
#pragma message WARN("now we don't support TriggerSkill::triggerable/TriggerSkill::cost/TriggerSkill::effect functions to call in LUA interface")
%}

class QThread: public QObject {
};

struct LogMessage {
    LogMessage();
    QString toString() const;

    QString type;
    ServerPlayer *from;
    QList<ServerPlayer *> to;
    QString card_str;
    QString arg;
    QString arg2;
};

class RoomThread: public QThread {
public:
    explicit RoomThread(Room *room);
    void constructTriggerTable();
    bool trigger(TriggerEvent triggerEvent, Room *room);
    bool trigger(TriggerEvent triggerEvent, Room *room, QVariant &data); // player is deleted. a lot of things is able to put in data. make a struct for every triggerevent isn't absolutely unreasonable.

    void addPlayerSkills(ServerPlayer *player, bool invoke_game_start = false);

    void addTriggerSkill(const TriggerSkill *skill);
    void delay(unsigned long msecs = 1000);
};

class Room: public QThread {
public:
    enum GuanxingType { GuanxingUpOnly = 1, GuanxingBothSides = 0, GuanxingDownOnly = -1 };

    explicit Room(QObject *parent, const char *mode);
    ServerPlayer *addSocket(ClientSocket *socket);
    int getId() const;
    bool isFull() const;
    bool isFinished() const;
    bool canPause(ServerPlayer *p) const;
    int getLack() const;
    QString getMode() const;
    const Scenario *getScenario() const;
    RoomThread *getThread() const;
    ServerPlayer *getCurrent() const;
    void setCurrent(ServerPlayer *current);
    int alivePlayerCount() const;
    QList<ServerPlayer *> getOtherPlayers(ServerPlayer *except, bool include_dead = false) const;
    QList<ServerPlayer *> getPlayers() const;
    QList<ServerPlayer *> getAllPlayers(bool include_dead = false) const;
    QList<ServerPlayer *> getAlivePlayers() const;
    void output(const char *message);
    void outputEventStack();
    void enterDying(ServerPlayer *player, DamageStruct *reason);
    ServerPlayer *getCurrentDyingPlayer() const;
    void killPlayer(ServerPlayer *victim, DamageStruct *reason = NULL);
    void revivePlayer(ServerPlayer *player);
    QStringList aliveRoles(ServerPlayer *except = NULL) const;
    void gameOver(const char *winner, bool isSurrender = false);
    void slashEffect(const SlashEffectStruct &effect);
    void slashResult(const SlashEffectStruct &effect, const Card *jink);
    void attachSkillToPlayer(ServerPlayer *player, const char *skill_name,bool is_other_attach =false);
    void detachSkillFromPlayer(ServerPlayer *player, const char *skill_name, bool is_equip = false, bool acquire_only = false);
    void handleAcquireDetachSkills(ServerPlayer *player, const char *skill_names, bool acquire_only = false);
    void setPlayerFlag(ServerPlayer *player, const char *flag);
    void setPlayerProperty(ServerPlayer *player, const char *property_name, const QVariant &value);
    void setPlayerMark(ServerPlayer *player, const char *mark, int value);
    void addPlayerMark(ServerPlayer *player, const char *mark, int add_num = 1);
    void removePlayerMark(ServerPlayer *player, const char *mark, int remove_num = 1);
    void setPlayerCardLimitation(ServerPlayer *player, const char *limit_list, const char *pattern, bool single_turn);
    void removePlayerCardLimitation(ServerPlayer *player, const char *limit_list, const char *pattern);
    void clearPlayerCardLimitation(ServerPlayer *player, bool single_turn);
    void setCardFlag(const Card *card, const char *flag, ServerPlayer *who = NULL);
    void setCardFlag(int card_id, const char *flag, ServerPlayer *who = NULL);
    void clearCardFlag(const Card *card, ServerPlayer *who = NULL);
    void clearCardFlag(int card_id, ServerPlayer *who = NULL);
    bool useCard(const CardUseStruct &card_use, bool add_history = true);
    void damage(const DamageStruct &data);
    void sendDamageLog(const DamageStruct &data);
    void loseHp(ServerPlayer *victim, int lose = 1);
    void loseMaxHp(ServerPlayer *victim, int lose = 1);
    bool changeMaxHpForAwakenSkill(ServerPlayer *player, int magnitude = -1);
    void applyDamage(ServerPlayer *victim, const DamageStruct &damage);
    void recover(ServerPlayer *player, const RecoverStruct &recover, bool set_emotion = false);
    bool cardEffect(const Card *card, ServerPlayer *from, ServerPlayer *to , bool multiple = false);
    bool cardEffect(const CardEffectStruct &effect);
    bool isJinkEffected(SlashEffectStruct effect, const Card *jink);
    void judge(JudgeStruct &judge_struct);
    void sendJudgeResult(const JudgeStruct * judge);
    QList<int> getNCards(int n, bool update_pile_number = true, bool bottom = false);
    ServerPlayer *getLord() const;
    void askForGuanxing(ServerPlayer *zhuge, const QList<int> &cards, GuanxingType guanxing_type = GuanxingBothSides,const char *skillName ="");
    int doGongxin(ServerPlayer *shenlvmeng, ServerPlayer *target, QList<int> enabled_ids = QList<int>(), const char *skill_name = "gongxin");
    int drawCard(bool bottom = false);
    void fillAG(const QList<int> &card_ids, ServerPlayer *who = NULL, const QList<int> &disabled_ids = QList<int>());
    void takeAG(ServerPlayer *player, int card_id, bool move_cards = true, QList<ServerPlayer *> to_notify = QList<ServerPlayer *>());
    void clearAG(ServerPlayer *player = NULL);
    void provide(const Card *card, ServerPlayer *who = NULL);
    QList<ServerPlayer *> getLieges(const char *kingdom, ServerPlayer *lord) const;
    void sendLog(const LogMessage &log);
    void showCard(ServerPlayer *player, int card_id, ServerPlayer *only_viewer = NULL);
    void showAllCards(ServerPlayer *player, ServerPlayer *to = NULL);
    void retrial(const Card *card, ServerPlayer *player, JudgeStruct * judge, const char *skill_name, bool exchange = false);

    bool doRequest(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg, time_t timeOut, bool wait);
    bool doRequest(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg, bool wait);

    bool doBroadcastRequest(QList<ServerPlayer *> &players, QSanProtocol::CommandType command, time_t timeOut);
    bool doBroadcastRequest(QList<ServerPlayer *> &players, QSanProtocol::CommandType command);
    bool doNotify(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg);
    bool doBroadcastNotify(QSanProtocol::CommandType command, const QVariant &arg);
    bool doBroadcastNotify(const QList<ServerPlayer *> &players, QSanProtocol::CommandType command, const QVariant &arg);

    // Verification functions
    bool verifyNullificationResponse(ServerPlayer *, const QVariant &, void *);

    bool notifyMoveFocus(ServerPlayer *player);
    bool notifyMoveFocus(ServerPlayer *player, QSanProtocol::CommandType command);
    bool notifyMoveFocus(const QList<ServerPlayer *> &players, QSanProtocol::CommandType command, QSanProtocol::Countdown countdown);
    bool notifyMoveCards(bool isLostPhase, QList<CardsMoveStruct> move, bool forceVisible, QList<ServerPlayer *> players = QList<ServerPlayer *>());
    bool notifyProperty(ServerPlayer *playerToNotify, const ServerPlayer *propertyOwner, const char *propertyName, const char *value = NULL);
    bool notifyUpdateCard(ServerPlayer *player, int cardId, const Card *newCard);
    bool broadcastUpdateCard(const QList<ServerPlayer *> &players, int cardId, const Card *newCard);
    bool notifyResetCard(ServerPlayer *player, int cardId);
    bool broadcastResetCard(const QList<ServerPlayer *> &players, int cardId);

    bool broadcastProperty(ServerPlayer *player, const char *property_name, const char *value = NULL);
    void notifySkillInvoked(ServerPlayer *player, const char *skill_name);
    void broadcastSkillInvoke(const char *skillName);
    void broadcastSkillInvoke(const char *skillName, const char *category);
    void broadcastSkillInvoke(const char *skillName, int type);
    void broadcastSkillInvoke(const char *skillName, bool isMale, int type);
    void doLightbox(const char *lightboxName, int duration = 2000);
    void doAnimate(QSanProtocol::AnimateType type, const char *arg1 = NULL, const char *arg2 = NULL, QList<ServerPlayer *> players = QList<ServerPlayer *>());
    void doAnimate(int type, const char *arg1 = NULL, const char *arg2 = NULL, QList<ServerPlayer *> players = QList<ServerPlayer *>());

    void changePlayerGeneral(ServerPlayer *player, const char *new_general);
    void changePlayerGeneral2(ServerPlayer *player, const char *new_general);
    void filterCards(ServerPlayer *player, QList<const Card *> cards, bool refilter);

    void acquireSkill(ServerPlayer *player, const Skill *skill, bool open = true);
    void acquireSkill(ServerPlayer *player, const char *skill_name, bool open = true);
    void adjustSeats();
    void swapPile();
    QList<int> getDiscardPile();
    QList<int> getDrawPile() const;
    int getCardFromPile(const char *card_name);
    ServerPlayer *findPlayer(const char *general_name, bool include_dead = false) const;
    QList<ServerPlayer *> findPlayersBySkillName(const char *skill_name) const;
    ServerPlayer *findPlayerBySkillName(const char *skill_name) const;
    ServerPlayer *findPlayerByObjectName(const char *name) const;
    void installEquip(ServerPlayer *player, const char *equip_name);
    void resetAI(ServerPlayer *player);
    void changeHero(ServerPlayer *player, const char *new_general, bool full_state, bool invoke_start = true, bool isSecondaryHero = false, bool sendLog = true);
    void swapSeat(ServerPlayer *a, ServerPlayer *b);
    lua_State *getLuaState() const;
    void setFixedDistance(Player *from, const Player *to, int distance);
    void reverseFor3v3(const Card *card, ServerPlayer *player, QList<ServerPlayer *> &list);
    bool hasWelfare(const ServerPlayer *player) const;
    ServerPlayer *getFront(ServerPlayer *a, ServerPlayer *b) const;
    ServerPlayer *getOwner() const;

    void sortByActionOrder(QList<ServerPlayer *> &players);

    const ProhibitSkill *isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;

    void setTag(const char *key, const QVariant &value);
    QVariant getTag(const char *key) const;
    void removeTag(const char *key);

    void setEmotion(ServerPlayer *target, const char *emotion);

    Player::Place getCardPlace(int card_id) const;
    ServerPlayer *getCardOwner(int card_id) const;
    void setCardMapping(int card_id, ServerPlayer *owner, Player::Place place);

    void drawCards(ServerPlayer *player, int n, const char *reason = NULL);
    void drawCards(QList<ServerPlayer *> players, int n, const char *reason = NULL);
    void drawCards(QList<ServerPlayer *> players, QList<int> n_list, const char *reason = NULL);
    void obtainCard(ServerPlayer *target, const Card *card, bool unhide = true);
    void obtainCard(ServerPlayer *target, int card_id, bool unhide = true);
    void obtainCard(ServerPlayer *target, const Card *card,  const CardMoveReason &reason, bool unhide = true);

    void throwCard(int card_id, ServerPlayer *who, ServerPlayer *thrower = NULL);
    void throwCard(const Card *card, ServerPlayer *who, ServerPlayer *thrower = NULL);
    void throwCard(const Card *card, const CardMoveReason &reason, ServerPlayer *who, ServerPlayer *thrower = NULL);

    void moveCardTo(const Card *card, ServerPlayer *dstPlayer, Player::Place dstPlace, bool forceMoveVisible = false);
    void moveCardTo(const Card *card, ServerPlayer *dstPlayer, Player::Place dstPlace, const CardMoveReason &reason,
                    bool forceMoveVisible = false);
    void moveCardTo(const Card *card, ServerPlayer *srcPlayer, ServerPlayer *dstPlayer, Player::Place dstPlace,
                    const CardMoveReason &reason, bool forceMoveVisible = false);
    void moveCardTo(const Card *card, ServerPlayer *srcPlayer, ServerPlayer *dstPlayer, Player::Place dstPlace,
                    const char *pileName, const CardMoveReason &reason, bool forceMoveVisible = false);
    void moveCardsAtomic(QList<CardsMoveStruct> cards_move, bool forceMoveVisible);
    void moveCardsAtomic(CardsMoveStruct cards_move, bool forceMoveVisible);
    void moveCardsToEndOfDrawpile(QList<int> card_ids, bool forceVisible = false);

    // interactive methods
    void activate(ServerPlayer *player, CardUseStruct &card_use);
    void askForLuckCard();
    Card::Suit askForSuit(ServerPlayer *player, const char *reason);
    QString askForKingdom(ServerPlayer *player);
    bool askForSkillInvoke(ServerPlayer *player, const char *skill_name, const QVariant &data = QVariant());
    QString askForChoice(ServerPlayer *player, const char *skill_name, const char *choices, const QVariant &data = QVariant());
    bool askForDiscard(ServerPlayer *target, const char *reason, int discard_num, int min_num,
                       bool optional = false, bool include_equip = false, const char *prompt = NULL);
    const Card *askForExchange(ServerPlayer *player, const char *reason, int discard_num, int min_num, bool include_equip = false,
                               const char *prompt = NULL, bool optional = false);
    bool askForNullification(const Card *trick, ServerPlayer *from, ServerPlayer *to, bool positive);
    bool isCanceled(const CardEffectStruct &effect);
    int askForCardChosen(ServerPlayer *player, ServerPlayer *who, const char *flags, const char *reason,
                         bool handcard_visible = false, Card::HandlingMethod method = Card::MethodNone, const QList<int> &disabled_ids = QList<int>());
    const Card *askForCard(ServerPlayer *player, const char *pattern, const char *prompt, const QVariant &data, const char *skill_name);
    const Card *askForCard(ServerPlayer *player, const char *pattern, const char *prompt, const QVariant &data = QVariant(),
                           Card::HandlingMethod method = Card::MethodDiscard, ServerPlayer *to = NULL, bool isRetrial = false,
                           const char *skill_name = NULL, bool isProvision = false);
    const Card *askForUseCard(ServerPlayer *player, const char *pattern, const char *prompt, int notice_index = -1,
                       Card::HandlingMethod method = Card::MethodUse, bool addHistory = true);
    const Card *askForUseSlashTo(ServerPlayer *slasher, ServerPlayer *victim, const char *prompt,
                                    bool distance_limit = true, bool disable_extra = false, bool addHistory = false);
    const Card *askForUseSlashTo(ServerPlayer *slasher, QList<ServerPlayer *> victims, const char *prompt,
                                     bool distance_limit = true, bool disable_extra = false, bool addHistory = false);
    int askForAG(ServerPlayer *player, const QList<int> &card_ids, bool refusable, const char *reason);
    const Card *askForCardShow(ServerPlayer *player, ServerPlayer *requestor, const char *reason);
    int askForRende(ServerPlayer *liubei, QList<int> &cards, const char *skill_name = NULL,
                    bool visible = false, bool optional = true, int max_num = -1,
                    QList<ServerPlayer *> players = QList<ServerPlayer *>(), CardMoveReason reason = CardMoveReason(),
                    const char *prompt = NULL, bool notify_skill = false);
    bool askForYiji(ServerPlayer *guojia, QList<int> &cards, const char *skill_name = NULL,
                    bool is_preview = false, bool visible = false, bool optional = true, int max_num = -1,
                    QList<ServerPlayer *> players = QList<ServerPlayer *>(), CardMoveReason reason = CardMoveReason(),
                    const char *prompt = NULL, bool notify_skill = false);
    const Card *askForPindian(ServerPlayer *player, ServerPlayer *from, ServerPlayer *to, const char *reason, PindianStruct *pindian);
    QList<const Card *> askForPindianRace(ServerPlayer *from, ServerPlayer *to, const char *reason);
    ServerPlayer *askForPlayerChosen(ServerPlayer *player, const QList<ServerPlayer *> &targets, const char *reason,
                                     const char *prompt = NULL, bool optional = false, bool notify_skill = false);
    QString askForGeneral(ServerPlayer *player, const char *generals, char *default_choice = NULL);
    const Card *askForSinglePeach(ServerPlayer *player, ServerPlayer *dying);
    void addPlayerHistory(ServerPlayer *player, const char *key, int times = 1);

    void speakCommand(ServerPlayer *player, const char *arg);
    void broadcastInvoke(const char *method, const char *arg = ".", ServerPlayer *except = NULL);
    Card *getCard(int cardId) const;
    void resetCard(int cardId);

    void updateStateItem();

    void setPlayerSkillInvalidity(ServerPlayer *player, const Skill *skill, bool invalidity);
    void setPlayerSkillInvalidity(ServerPlayer *player, const char *skill_name, bool invalidity);
};

%extend Room {
    ServerPlayer *nextPlayer(ServerPlayer *player) const{
		Q_UNUSED($self);
        return qobject_cast<ServerPlayer *>(player->getNextAlive());
        //return $self->getCurrent()->getNextAlive();
    }
    void output(const char *msg) {
        if(Config.value("DebugOutput", false).toBool())
            $self->output(msg);
    }
    void outputEventStack() {
        if(Config.value("DebugOutput", false).toBool())
            $self->outputEventStack();
    }
    void writeToConsole(const char *msg) {
        $self->output(msg);
        qWarning("%s", msg);
    }
    void throwEvent(const TriggerEvent event) {
        Q_UNUSED($self);
        throw event;
    }
};

%{

void Room::doScript(const QString &script)
{
    SWIG_NewPointerObj(L, this, SWIGTYPE_p_Room, 0);
    lua_setglobal(L, "R");

    SWIG_NewPointerObj(L, current, SWIGTYPE_p_ServerPlayer, 0);
    lua_setglobal(L, "P");

    luaL_dostring(L, script.toLatin1());
}

%}

%include "card.i"
%include "luaskills.i"
%include "general_select.i"
%include "ai.i"
