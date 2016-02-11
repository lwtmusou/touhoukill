#ifndef _HEGEMONY_CARDS_PACKAGE_H
#define _HEGEMONY_CARDS_PACKAGE_H

#include "package.h"

#if 0
#include "standard.h"
#include "skill.h"

class AwaitExhausted : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE AwaitExhausted(Card::Suit suit, int number);

    virtual QString getSubtype() const;
    virtual bool isAvailable(const Player *player) const;

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class KnownBoth : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE KnownBoth(Card::Suit suit, int number);
    virtual bool isAvailable(const Player *player) const;

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class BefriendAttacking : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE BefriendAttacking(Card::Suit suit = Heart, int number = 9);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual bool isAvailable(const Player *player) const;
};

class Breastplate : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE Breastplate(Card::Suit suit = Card::Club, int number = 2);
};

class IronArmor : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE IronArmor(Card::Suit suit = Card::Spade, int number = 2);
};

class JadeSeal : public Treasure
{
    Q_OBJECT

public:
    Q_INVOKABLE JadeSeal(Card::Suit suit, int number);
};

class Drowning : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE Drowning(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual bool isAvailable(const Player *player) const;
};

class BurningCamps : public AOE
{
    Q_OBJECT

public:
    Q_INVOKABLE BurningCamps(Card::Suit suit, int number);

    virtual bool isAvailable(const Player *player) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class LureTiger : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LureTiger(Card::Suit suit, int number);

    virtual QString getSubtype() const;

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class FightTogether : public GlobalEffect
{
    Q_OBJECT

public:
    Q_INVOKABLE FightTogether(Card::Suit suit, int number);

    virtual bool isAvailable(const Player *player) const;

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class AllianceFeast : public AOE
{
    Q_OBJECT

public:
    Q_INVOKABLE AllianceFeast(Card::Suit suit = Heart, int number = 1);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual bool isAvailable(const Player *player) const;
};

class ThreatenEmperor : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE ThreatenEmperor(Card::Suit suit, int number);
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual bool isAvailable(const Player *player) const;
};

class ImperialOrder : public GlobalEffect
{
    Q_OBJECT

public:
    Q_INVOKABLE ImperialOrder(Card::Suit suit, int number);

    virtual bool isAvailable(const Player *player) const;

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};
#endif

class HegemonyCardsPackage : public Package
{
    Q_OBJECT

public:
    HegemonyCardsPackage();
};


#endif
