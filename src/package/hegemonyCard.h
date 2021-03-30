#ifndef _HEGEMONYCARD_H
#define _HEGEMONYCARD_H

#include "maneuvering.h"
#include "standard.h"

class SixSwords : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit SixSwords(Card::Suit suit = Diamond, int number = 6);
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

    QString getSubtype() const override;
    bool isAvailable(const Player *player) const override;

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class KnownBothHegemony : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KnownBothHegemony(Card::Suit suit, int number);
    QString getSubtype() const override;

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
    bool isAvailable(const Player *player) const override;

private:
    void doKnownBoth(const QString &choice, const CardEffectStruct &effect) const;
};

class BefriendAttacking : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit BefriendAttacking(Card::Suit suit = Heart, int number = 9);

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
    bool isAvailable(const Player *player) const override;
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
