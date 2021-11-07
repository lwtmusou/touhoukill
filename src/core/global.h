#ifndef TOUHOUKILL_GLOBAL_H
#define TOUHOUKILL_GLOBAL_H

// BE WARE! THIS FILE IS USED IN BOTH SWIG AND C++.
// MAKE SURE THE GRAMMAR IS COMPATIBLE BETWEEN 2 LANGUAGES.

#ifndef SWIG
#include "qsgscore.h"

#include <QMetaObject>
#include <QSet>
#endif

// Fs: I prefer following:
// typedef QSet<int> IDSet;
// Are there any advantages using 'using' instead of 'typedef'?
using IdSet = QSet<int>;

#ifndef SWIG
namespace QSanguosha {
Q_NAMESPACE_EXPORT(QSGS_CORE_EXPORT)
#else
#ifndef Q_QDOC
class QSanguosha
{
private:
    QSGS_DISABLE_COPY_MOVE_CONSTRUCT(QSanguosha)

public:

#define Q_ENUM_NS(...)
#endif
#endif

enum GuanxingType
{
    GuanxingUpOnly = 1,
    GuanxingBothSides = 0,
    GuanxingDownOnly = -1
};

enum Suit
{
    Spade,
    Club,
    Heart,
    Diamond,
    NoSuitBlack,
    NoSuitRed,
    NoSuit,
    SuitToBeDecided = -1
};
Q_ENUM_NS(Suit)

enum Color
{
    ColorRed,
    ColorBlack,
    Colorless = -1
};
Q_ENUM_NS(Color)

enum HandlingMethod
{
    MethodNone,
    MethodUse,
    MethodResponse,
    MethodDiscard,
    MethodRecast,
    MethodPindian
};
Q_ENUM_NS(HandlingMethod)

// implementation in engine.cpp
#ifdef SWIG
static
#endif
QSGS_CORE_EXPORT HandlingMethod string2HandlingMethod(const QString &str);

enum Number
{
    // Regular numbers
    NumberA = 1,
    Number2,
    Number3,
    Number4,
    Number5,
    Number6,
    Number7,
    Number8,
    Number9,
    Number10,
    NumberJ,
    NumberQ,
    NumberK,

    // Alternative notation
    Number1 = NumberA,
    NumberX = Number10,
    Number11 = NumberJ,
    Number12 = NumberQ,
    Number13 = NumberK,

#ifdef NUMBER_EXTREMELY_SIMPILIFIED
    // Extremely simplified notation
    A = NumberA,
    X = Number10,
    J = NumberJ,
    Q = NumberQ,
    K = NumberK,
#endif

    // Special numbers
    NumberNA = 0,
    NumberToBeDecided = -1

    // TODO: Add -2
};
Q_ENUM_NS(Number)

enum CardType
{
    TypeSkill,
    TypeBasic,
    TypeTrick,
    TypeEquip,
    TypeUnknown = -1
};
Q_ENUM_NS(CardType)

enum EquipLocation
{
    WeaponLocation,
    ArmorLocation,
    DefensiveHorseLocation,
    OffensiveHorseLocation,
    TreasureLocation,
    UnknownLocation = -1
};
Q_ENUM_NS(EquipLocation)

enum Phase
{
    PhaseRoundStart,
    PhaseStart,
    PhaseJudge,
    PhaseDraw,
    PhasePlay,
    PhaseDiscard,
    PhaseFinish,
    PhaseNotActive,
    PhaseNone = -1
};
Q_ENUM_NS(Phase)

enum Place
{
    PlaceHand,
    PlaceEquip,
    PlaceDelayedTrick,
    PlaceJudge,
    PlaceSpecial,
    PlaceDiscardPile,
    PlaceDrawPile,
    PlaceTable,
    PlaceUnknown = -1
};
Q_ENUM_NS(Place)

enum Role
{
    RoleLord,
    RoleLoyalist,
    RoleRebel,
    RoleRenegade,
};
Q_ENUM_NS(Role)

enum TargetModType
{
    ModResidue,
    ModDistance,
    ModTarget
};
Q_ENUM_NS(TargetModType)

enum PackageType
{
    GeneralPack,
    CardPack,
    MixedPack,
    SpecialPack
};
Q_ENUM_NS(PackageType)

enum Gender
{
    Sexless,
    Male,
    Female,
    Neuter
};
Q_ENUM_NS(Gender)

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
    RoleShownChanged,

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

    GeneralShown, // For Official Hegemony mode
    GeneralHidden,
    GeneralRemoved,

    // This is the Num Of Events and also All events when used in Trigger::addTriggerEvent[s].
    // Add this event to Trigger makes it trigger for all events
    NumOfEvents
};
Q_ENUM_NS(TriggerEvent)

typedef QSet<TriggerEvent> TriggerEvents;

enum ModeCategory
{
    ModeRole = 1,
    ModeHegemony = 2,
    ModeScenario = 16,
};
Q_ENUM_NS(ModeCategory)

enum CardUseReason
{
    CardUseReasonUnknown = 0x00,
    CardUseReasonPlay = 0x01,
    CardUseReasonResponse = 0x02,
    CardUseReasonResponseUse = 0x12
};
Q_ENUM_NS(CardUseReason)

enum MoveReasonCategory
{
    MoveReasonUnknown = 0x00,
    MoveReasonUse = 0x01,
    MoveReasonResponse = 0x02,
    MoveReasonDiscard = 0x03,
    MoveReasonRecast = 0x04, // ironchain etc.
    MoveReasonPindian = 0x05,
    MoveReasonDraw = 0x06,
    MoveReasonGotCard = 0x07,
    MoveReasonShow = 0x08,
    MoveReasonTransfer = 0x09,
    MoveReasonPut = 0x0A,

    //subcategory of use
    MoveReasonLetUse = 0x11, // use a card when self is not current

    //subcategory of response
    MoveReasonRetrial = 0x12,

    //subcategory of discard
    MoveReasonRuleDiscard = 0x13, //  discard at one's Player::Discard for gamerule
    MoveReasonThrow = 0x23, //  gamerule(dying or punish) as the cost of some skills
    MoveReasonDismantle = 0x33, //  one throw card of another

    //subcategory of gotcard
    MoveReasonGive = 0x17, // from one hand to another hand
    MoveReasonExtraction = 0x27, // from another's place to one's hand
    MoveReasonGotBack = 0x37, // from placetable to hand
    MoveReasonRecycle = 0x47, // from discardpile to hand
    MoveReasonRob = 0x57, // got a definite card from other's hand
    MoveReasonPreviewGive = 0x67, // give cards after previewing, i.e. Yiji & Miji

    //subcategory of show
    MoveReasonTurnover = 0x18, // show n cards from drawpile
    MoveReasonJudge = 0x28, // show a card from drawpile for judge
    MoveReasonPreview = 0x38, // Not done yet, plan for view some cards for self only(guanxing yiji miji)
    MoveReasonDemonstrate = 0x48, // show a card which copy one to move to table

    //subcategory of transfer
    MoveReasonSwap = 0x19, // exchange card for two players
    MoveReasonOverride = 0x29, // exchange cards from cards in game
    MoveReasonExchangeFromPile = 0x39, // exchange cards from cards moved out of game (for qixing only)

    //subcategory of put
    MoveReasonNaturalEnter = 0x1A, //  a card with no-owner move into discardpile e.g. delayed trick enters discardpile
    MoveReasonRemoveFromPile = 0x2A, //  cards moved out of game go back into discardpile
    MoveReasonJudgeDone = 0x3A, //  judge card move into discardpile
    MoveReasonChangeEquip = 0x4A, //  replace existed equip

    MoveReasonBasicMask = 0x0F,
};
Q_ENUM_NS(MoveReasonCategory)

enum DamageNature
{
    DamageNormal, // normal slash, duel and most damage caused by skill
    DamageFire, // fire slash, fire attack and few damage skill (Yeyan, etc)
    DamageThunder // lightning, thunder slash, and few damage skill (Leiji, etc)
};
Q_ENUM_NS(DamageNature)

} // namespace QSanguosha
#ifdef SWIG
;
#endif

using LuaFunction = int;

#ifndef SWIG
#define QSGS_LUA_API
#endif

#endif // GLOBAL_H
