#ifndef QSANGUOSHA_GLOBAL_H
#define QSANGUOSHA_GLOBAL_H

// BE WARE! THIS FILE IS USED IN BOTH SWIG AND C++.
// MAKE SURE THE GRAMMAR IS COMPATIBLE BETWEEN 2 LANGUAGES.

#ifndef SWIG
#include <QMetaObject>
#include <QSet>
#endif

// Fs: I prefer following:
// typedef QSet<int> IDSet;
// Are there any advantages using 'using' instead of 'typedef'?
using IDSet = QSet<int>;

#ifndef SWIG
namespace QSanguosha {
Q_NAMESPACE
#else
#ifndef Q_DOC
class QSanguosha
{
private:
    QSanguosha() = delete;
    ~QSanguosha() = delete;
    Q_DISABLE_COPY_MOVE(QSanguosha)

public:

#define Q_ENUM_NS(...)
#endif
#endif

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

} // namespace QSanguosha
#ifdef SWIG
;
#endif

typedef int LuaFunction;

#ifndef SWIG
#define QSGS_LUA_API
#endif

#endif // GLOBAL_H
