#ifndef _HEGEMONYCARD_H
#define _HEGEMONYCARD_H

#include "maneuvering.h"
#include "standard.h"

class SixSwords : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE SixSwords(Card::Suit suit = Diamond, int number = 6);
};

class DoubleSwordHegemony : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE DoubleSwordHegemony(Card::Suit suit, int number);
};



class AwaitExhaustedHegemony : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE AwaitExhaustedHegemony(Card::Suit suit, int number);

    virtual QString getSubtype() const;
    virtual bool isAvailable(const Player *player) const;

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};


class KnownBothHegemony : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KnownBothHegemony(Card::Suit suit, int number);
    virtual QString getSubtype() const;

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class BefriendAttacking : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE BefriendAttacking(Card::Suit suit = Heart, int number = 9);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual bool isAvailable(const Player *player) const;

    //virtual QStringList checkTargetModSkillShow(const CardUseStruct &use) const;
};

class HegNullification : public Nullification
{
    Q_OBJECT

public:
    Q_INVOKABLE HegNullification(Card::Suit suit, int number);
};


class HegemonyCardPackage : public Package
{
    Q_OBJECT

public:
    HegemonyCardPackage();
};

#endif
#pragma once
