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
class QSanguosha
{
private:
    QSanguosha() = delete;
    ~QSanguosha() = delete;
    Q_DISABLE_COPY_MOVE(QSanguosha)

public:
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
#ifndef SWIG
Q_ENUM_NS(Suit)
#endif

enum Color
{
    ColorRed,
    ColorBlack,
    Colorless = -1
};
#ifndef SWIG
Q_ENUM_NS(Color)
#endif

enum HandlingMethod
{
    MethodNone,
    MethodUse,
    MethodResponse,
    MethodDiscard,
    MethodRecast,
    MethodPindian
};
#ifndef SWIG
Q_ENUM_NS(HandlingMethod)
#endif

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
#ifndef SWIG
Q_ENUM_NS(Number)
#endif

enum CardType
{
    TypeSkill,
    TypeBasic,
    TypeTrick,
    TypeEquip,
    TypeUnknown = -1
};
#ifndef SWIG
Q_ENUM_NS(CardType)
#endif

enum EquipLocation
{
    WeaponLocation,
    ArmorLocation,
    DefensiveHorseLocation,
    OffensiveHorseLocation,
    TreasureLocation,
    UnknownLocation = -1
};
#ifndef SWIG
Q_ENUM_NS(EquipLocation)
#endif

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
#ifndef SWIG
Q_ENUM_NS(Phase)
#endif

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
#ifndef SWIG
Q_ENUM_NS(Place)
#endif

enum Role
{
    RoleLord,
    RoleLoyalist,
    RoleRebel,
    RoleRenegade,
};
#ifndef SWIG
Q_ENUM_NS(Role)
#endif

enum TargetModType
{
    ModResidue,
    ModDistance,
    ModTarget
};
#ifndef SWIG
Q_ENUM_NS(TargetModType)
#endif

enum PackageType
{
    GeneralPack,
    CardPack,
    MixedPack,
    SpecialPack
};
#ifndef SWIG
Q_ENUM_NS(PackageType)
#endif

enum Gender
{
    Sexless,
    Male,
    Female,
    Neuter
};
#ifndef SWIG
Q_ENUM_NS(Gender)
#endif

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
#ifndef SWIG
Q_ENUM_NS(TriggerEvent)
#endif
typedef QSet<TriggerEvent> TriggerEvents;

enum ModeCategory
{
    ModeRole = 1,
    ModeHegemony = 2,
    ModeScenario = 16,
};

} // namespace QSanguosha
#ifdef SWIG
;
#endif

typedef int LuaFunction;

#ifndef SWIG
#define QSGS_LUA_API
#endif

#endif // GLOBAL_H
