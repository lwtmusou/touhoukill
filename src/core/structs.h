#ifndef _STRUCTS_H
#define _STRUCTS_H

class Room;
class TriggerSkill;
class Card;
class Slash;

#include "player.h"
#include "serverplayer.h"

#include <QVariant>

struct DamageStruct
{
    enum Nature
    {
        Normal, // normal slash, duel and most damage caused by skill
        Fire, // fire slash, fire attack and few damage skill (Yeyan, etc)
        Thunder, // lightning, thunder slash, and few damage skill (Leiji, etc)
        Ice
    };

    DamageStruct();
    DamageStruct(const Card *card, ServerPlayer *from, ServerPlayer *to, int damage = 1, Nature nature = Normal);
    DamageStruct(const QString &reason, ServerPlayer *from, ServerPlayer *to, int damage = 1, Nature nature = Normal);

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

struct CardEffectStruct
{
    CardEffectStruct();

    const Card *card;

    ServerPlayer *from;
    ServerPlayer *to;

    bool multiple; // helper to judge whether the card has multiple targets
    // does not make sense if the card inherits SkillCard
    bool nullified;
    bool canceled; //for cancel process, like "yuyi"
};

struct SlashEffectStruct
{
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

struct CardUseStruct
{
    enum CardUseReason
    {
        CARD_USE_REASON_UNKNOWN = 0x00,
        CARD_USE_REASON_PLAY = 0x01,
        CARD_USE_REASON_RESPONSE = 0x02,
        CARD_USE_REASON_RESPONSE_USE = 0x12
    } m_reason;

    CardUseStruct();
    CardUseStruct(const Card *card, ServerPlayer *from, QList<ServerPlayer *> to = QList<ServerPlayer *>(), bool isOwnerUse = true);
    CardUseStruct(const Card *card, ServerPlayer *from, ServerPlayer *target, bool isOwnerUse = true);
    bool isValid(const QString &pattern) const;
    void parse(const QString &str, Room *room);
    bool tryParse(const QVariant &usage, Room *room);

    QString toString() const;

    const Card *card;
    ServerPlayer *from;
    QList<ServerPlayer *> to;
    bool m_isOwnerUse;
    bool m_addHistory;
    bool m_isHandcard;
    bool m_isLastHandcard;
    QList<int> m_showncards;
    QStringList nullified_list;
};

class CardMoveReason
{
public:
    int m_reason;
    QString m_playerId; // the cause (not the source) of the movement, such as "lusu" when "dimeng", or "zhanghe" when "qiaobian"
    QString m_targetId; // To keep this structure lightweight, currently this is only used for UI purpose.
    // It will be set to empty if multiple targets are involved. NEVER use it for trigger condition
    // judgement!!! It will not accurately reflect the real reason.
    QString m_skillName; // skill that triggers movement of the cards, such as "longdang", "dimeng"
    QString m_eventName; // additional arg such as "lebusishu" on top of "S_REASON_JUDGE"
    QVariant m_extraData; // additional data and will not be parsed to clients
    QVariant m_provider; // additional data recording who provide this card for otherone to use or response, e.g. guanyu provide a slash for "jijiang"

    inline CardMoveReason()
    {
        m_reason = S_REASON_UNKNOWN;
    }
    inline CardMoveReason(int moveReason, QString playerId)
    {
        m_reason = moveReason;
        m_playerId = playerId;
    }

    inline CardMoveReason(int moveReason, QString playerId, QString skillName, QString eventName)
    {
        m_reason = moveReason;
        m_playerId = playerId;
        m_skillName = skillName;
        m_eventName = eventName;
    }

    inline CardMoveReason(int moveReason, QString playerId, QString targetId, QString skillName, QString eventName)
    {
        m_reason = moveReason;
        m_playerId = playerId;
        m_targetId = targetId;
        m_skillName = skillName;
        m_eventName = eventName;
    }

    bool tryParse(const QVariant &);
    QVariant toVariant() const;

    inline bool operator==(const CardMoveReason &other) const
    {
        return m_reason == other.m_reason && m_playerId == other.m_playerId && m_targetId == other.m_targetId && m_skillName == other.m_skillName
            && m_eventName == other.m_eventName;
    }

    static const int S_REASON_UNKNOWN = 0x00;
    static const int S_REASON_USE = 0x01;
    static const int S_REASON_RESPONSE = 0x02;
    static const int S_REASON_DISCARD = 0x03;
    static const int S_REASON_RECAST = 0x04; // ironchain etc.
    static const int S_REASON_PINDIAN = 0x05;
    static const int S_REASON_DRAW = 0x06;
    static const int S_REASON_GOTCARD = 0x07;
    static const int S_REASON_SHOW = 0x08;
    static const int S_REASON_TRANSFER = 0x09;
    static const int S_REASON_PUT = 0x0A;

    //subcategory of use
    static const int S_REASON_LETUSE = 0x11; // use a card when self is not current

    //subcategory of response
    static const int S_REASON_RETRIAL = 0x12;

    //subcategory of discard
    static const int S_REASON_RULEDISCARD = 0x13; //  discard at one's Player::Discard for gamerule
    static const int S_REASON_THROW = 0x23; //  gamerule(dying or punish) as the cost of some skills
    static const int S_REASON_DISMANTLE = 0x33; //  one throw card of another

    //subcategory of gotcard
    static const int S_REASON_GIVE = 0x17; // from one hand to another hand
    static const int S_REASON_EXTRACTION = 0x27; // from another's place to one's hand
    static const int S_REASON_GOTBACK = 0x37; // from placetable to hand
    static const int S_REASON_RECYCLE = 0x47; // from discardpile to hand
    static const int S_REASON_ROB = 0x57; // got a definite card from other's hand
    static const int S_REASON_PREVIEWGIVE = 0x67; // give cards after previewing, i.e. Yiji & Miji

    //subcategory of show
    static const int S_REASON_TURNOVER = 0x18; // show n cards from drawpile
    static const int S_REASON_JUDGE = 0x28; // show a card from drawpile for judge
    static const int S_REASON_PREVIEW = 0x38; // Not done yet, plan for view some cards for self only(guanxing yiji miji)
    static const int S_REASON_DEMONSTRATE = 0x48; // show a card which copy one to move to table

    //subcategory of transfer
    static const int S_REASON_SWAP = 0x19; // exchange card for two players
    static const int S_REASON_OVERRIDE = 0x29; // exchange cards from cards in game
    static const int S_REASON_EXCHANGE_FROM_PILE = 0x39; // exchange cards from cards moved out of game (for qixing only)

    //subcategory of put
    static const int S_REASON_NATURAL_ENTER = 0x1A; //  a card with no-owner move into discardpile e.g. delayed trick enters discardpile
    static const int S_REASON_REMOVE_FROM_PILE = 0x2A; //  cards moved out of game go back into discardpile
    static const int S_REASON_JUDGEDONE = 0x3A; //  judge card move into discardpile
    static const int S_REASON_CHANGE_EQUIP = 0x4A; //  replace existed equip

    static const int S_MASK_BASIC_REASON = 0x0F;
};

struct CardsMoveOneTimeStruct
{
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
    QList<int> broken_ids; //record broken equip IDs from EquipPlace
    QList<int> shown_ids; //record broken shown IDs from HandPlace

    inline void removeCardIds(const QList<int> &to_remove)
    {
        foreach (int id, to_remove) {
            int index = card_ids.indexOf(id);
            if (index != -1) {
                card_ids.removeAt(index);
                from_places.removeAt(index);
                from_pile_names.removeAt(index);
                open.removeAt(index);
            }
        }
    }
};

struct CardsMoveStruct
{
    inline CardsMoveStruct()
    {
        from_place = Player::PlaceUnknown;
        to_place = Player::PlaceUnknown;
        from = NULL;
        to = NULL;
        is_last_handcard = false;
    }

    inline CardsMoveStruct(const QList<int> &ids, Player *from, Player *to, Player::Place from_place, Player::Place to_place, CardMoveReason reason)
    {
        this->card_ids = ids;
        this->from_place = from_place;
        this->to_place = to_place;
        this->from = from;
        this->to = to;
        this->reason = reason;
        is_last_handcard = false;
        if (from)
            from_player_name = from->objectName();
        if (to)
            to_player_name = to->objectName();
    }

    inline CardsMoveStruct(const QList<int> &ids, Player *to, Player::Place to_place, CardMoveReason reason)
    {
        this->card_ids = ids;
        this->from_place = Player::PlaceUnknown;
        this->to_place = to_place;
        this->from = NULL;
        this->to = to;
        this->reason = reason;
        is_last_handcard = false;
        if (to)
            to_player_name = to->objectName();
    }

    inline CardsMoveStruct(int id, Player *from, Player *to, Player::Place from_place, Player::Place to_place, CardMoveReason reason)
    {
        this->card_ids << id;
        this->from_place = from_place;
        this->to_place = to_place;
        this->from = from;
        this->to = to;
        this->reason = reason;
        is_last_handcard = false;
        if (from)
            from_player_name = from->objectName();
        if (to)
            to_player_name = to->objectName();
    }

    inline CardsMoveStruct(int id, Player *to, Player::Place to_place, CardMoveReason reason)
    {
        this->card_ids << id;
        this->from_place = Player::PlaceUnknown;
        this->to_place = to_place;
        this->from = NULL;
        this->to = to;
        this->reason = reason;
        is_last_handcard = false;
        if (to)
            to_player_name = to->objectName();
    }

    inline bool operator==(const CardsMoveStruct &other) const
    {
        return from == other.from && from_place == other.from_place && from_pile_name == other.from_pile_name && from_player_name == other.from_player_name;
    }

    inline bool operator<(const CardsMoveStruct &other) const
    {
        return from < other.from || from_place < other.from_place || from_pile_name < other.from_pile_name || from_player_name < other.from_player_name;
    }

    QList<int> card_ids;
    Player::Place from_place, to_place;
    QString from_player_name, to_player_name;
    QString from_pile_name, to_pile_name;
    Player *from, *to;
    CardMoveReason reason;
    bool open; // helper to prevent sending card_id to unrelevant clients
    bool is_last_handcard;

    Player::Place origin_from_place, origin_to_place;
    Player *origin_from, *origin_to;
    QString origin_from_pile_name, origin_to_pile_name; //for case of the movement transitted
    QList<int> broken_ids; //record broken equip IDs from EquipPlace
    QList<int> shown_ids; //record broken shown IDs from HandPlace

    bool tryParse(const QVariant &arg);
    QVariant toVariant() const;
    inline bool isRelevant(const Player *player)
    {
        return player != NULL && (from == player || (to == player && to_place != Player::PlaceSpecial));
    }
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

struct PindianStruct
{
    PindianStruct();
    bool isSuccess() const;

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

struct JudgeStruct
{
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
private:
    enum TrialResult
    {
        TRIAL_RESULT_UNKNOWN,
        TRIAL_RESULT_GOOD,
        TRIAL_RESULT_BAD
    } _m_result;
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

struct PhaseStruct
{
    inline PhaseStruct()
    {
        phase = Player::PhaseNone;
        skipped = 0;
    }

    Player::Phase phase;
    int skipped; // 0 - not skipped; 1 - skipped by effect; -1 - skipped by cost
};

struct CardResponseStruct
{
    inline CardResponseStruct(const Card *card = NULL, ServerPlayer *who = NULL, bool isuse = false, bool isRetrial = false, bool isProvision = false, ServerPlayer *from = NULL)
        : m_card(card)
        , m_who(who)
        , m_isUse(isuse)
        , m_isRetrial(isRetrial)
        , m_isProvision(isProvision)
        , m_isHandcard(false)
        , m_from(from)
        , m_isNullified(false)
        , m_isShowncard(false)
    {
    }

    const Card *m_card;
    ServerPlayer *m_who;
    bool m_isUse;
    bool m_isRetrial;
    bool m_isProvision;
    bool m_isHandcard;
    ServerPlayer *m_from;
    bool m_isNullified;
    bool m_isShowncard;
};

struct MarkChangeStruct
{
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

struct SkillInvokeDetail
{
    explicit SkillInvokeDetail(const TriggerSkill *skill = NULL, ServerPlayer *owner = NULL, ServerPlayer *invoker = NULL, QList<ServerPlayer *> targets = QList<ServerPlayer *>(),
                               bool isCompulsory = false, ServerPlayer *preferredTarget = NULL, bool showHidden = true);
    SkillInvokeDetail(const TriggerSkill *skill, ServerPlayer *owner, ServerPlayer *invoker, ServerPlayer *target, bool isCompulsory = false, ServerPlayer *preferredTarget = NULL,
                      bool showHidden = true);

    const TriggerSkill *skill; // the skill
    ServerPlayer *owner; // skill owner. 2 structs with the same skill and skill owner are treated as of a same skill.
    ServerPlayer *invoker; // skill invoker. When invoking skill, we sort firstly according to the priority, then the seat of invoker, at last weather it is a skill of an equip.
    QList<ServerPlayer *> targets; // skill targets.
    bool isCompulsory; // judge the skill is compulsory or not. It is set in the skill's triggerable
    bool triggered; // judge whether the skill is triggered
    ServerPlayer *preferredTarget; // the preferred target of a certain skill
    bool showhidden;
    QVariantMap tag; // used to add a tag to the struct. useful for skills like Tieqi and Liegong to save a QVariantList for assisting to assign targets

    bool operator<(const SkillInvokeDetail &arg2) const; // the operator < for sorting the invoke order.
    // the operator ==. it only judge the skill name, the skill invoker, and the skill owner. it don't judge the skill target because it is chosen by the skill invoker
    bool sameSkill(const SkillInvokeDetail &arg2) const;
    // used to judge 2 skills has the same timing. only 2 structs with the same priority and the same invoker and the same "whether or not it is a skill of equip"
    bool sameTimingWith(const SkillInvokeDetail &arg2) const;
    bool isValid() const; // validity check
    bool preferredTargetLess(const SkillInvokeDetail &arg2) const;

    QVariant toVariant() const;
    QStringList toList() const;
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

struct ExtraTurnStruct
{
    ExtraTurnStruct();

    ServerPlayer *player;
    QList<Player::Phase> set_phases;
    QString reason;
    ServerPlayer *extraTarget; //record related target  --qinlue
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

    PindianAsked,
    PindianVerifying,
    Pindian,

    TurnedOver,
    ChainStateChanged,
    RemoveStateChanged,
    BrokenEquipChanged,
    ShownCardChanged,

    ConfirmDamage, // confirm the damage's count and damage's nature
    Predamage, // trigger the certain skill -- jueqing
    DamageForseen, // the first event in a damage -- kuangfeng dawu
    DamageCaused, // the moment for -- qianxi..
    DamageInflicted, // the moment for -- tianxiang..
    PreDamageDone, // before reducing Hp
    DamageDone, // it's time to do the damage
    Damage, // the moment for -- lieren..
    Damaged, // the moment for -- yiji..
    DamageComplete, // the moment for trigger iron chain

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
    Cancel,

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

    // StageChange, // For hulao pass only
    FetchDrawPileCard, // For miniscenarios only
    ActionedReset, // For 3v3 only
    Debut, // For 1v1 only

    TurnBroken, // For the skill 'DanShou'. Do not use it to trigger events

    //new events for touhoukill,
    DrawPileSwaped, //for qiannian
    AfterGuanXing,
    KingdomChanged,

    NumOfEvents
};

Q_DECLARE_METATYPE(DamageStruct)
Q_DECLARE_METATYPE(CardEffectStruct)
Q_DECLARE_METATYPE(SlashEffectStruct)
Q_DECLARE_METATYPE(CardUseStruct)
Q_DECLARE_METATYPE(CardsMoveStruct)
Q_DECLARE_METATYPE(CardsMoveOneTimeStruct)
Q_DECLARE_METATYPE(DyingStruct)
Q_DECLARE_METATYPE(DeathStruct)
Q_DECLARE_METATYPE(RecoverStruct)
Q_DECLARE_METATYPE(PhaseChangeStruct)
Q_DECLARE_METATYPE(CardResponseStruct)
Q_DECLARE_METATYPE(MarkChangeStruct)
Q_DECLARE_METATYPE(ChoiceMadeStruct)
Q_DECLARE_METATYPE(SkillAcquireDetachStruct)
Q_DECLARE_METATYPE(CardAskedStruct)
Q_DECLARE_METATYPE(HpLostStruct)
Q_DECLARE_METATYPE(JinkEffectStruct)
Q_DECLARE_METATYPE(PhaseSkippingStruct)
Q_DECLARE_METATYPE(DrawNCardsStruct)
Q_DECLARE_METATYPE(QList<SkillInvalidStruct>)
Q_DECLARE_METATYPE(const Card *)
Q_DECLARE_METATYPE(ServerPlayer *)
Q_DECLARE_METATYPE(JudgeStruct *)
Q_DECLARE_METATYPE(PindianStruct *)
Q_DECLARE_METATYPE(ExtraTurnStruct)
Q_DECLARE_METATYPE(BrokenEquipChangedStruct)
Q_DECLARE_METATYPE(ShownCardChangedStruct)
#endif
