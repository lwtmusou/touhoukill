#ifndef QSANGUOSHA_GLOBAL_H
#define QSANGUOSHA_GLOBAL_H

#include <QMetaObject>

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
    Red,
    Black,
    Colorless
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

    // Extremely simplified notation
    A = NumberA,
    X = Number10,
    J = NumberJ,
    Q = NumberQ,
    K = NumberK,

    // Special numbers
    NumberNA = 0,
    NumberToBeDecided = -1

    // TODO: Add -2
};
Q_ENUM_NS(Number)

}

#endif // GLOBAL_H
