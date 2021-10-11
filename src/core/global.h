#ifndef QSANGUOSHA_GLOBAL_H
#define QSANGUOSHA_GLOBAL_H

#include <QMetaObject>
#include <QSet>

// Fs: I prefer following:
// typedef QSet<int> IDSet;
// Are there any advantages using 'using' instead of 'typedef'?
using IDSet = QSet<int>;

namespace QSanguosha {
Q_NAMESPACE

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

} // namespace QSanguosha

#endif // GLOBAL_H
