#ifndef _TESTCARD_H
#define _TESTCARD_H

#include "standard.h"

class NatureJink : public Jink
{
    Q_OBJECT

public:
    NatureJink(Suit suit, int number);
    virtual bool match(const QString &pattern) const;
};

class AdvancedJink : public NatureJink
{
    Q_OBJECT

public:
    Q_INVOKABLE AdvancedJink(Card::Suit suit, int number);
};

class Camera : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE Camera(Card::Suit suit, int number);
};

class Gun : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE Gun(Card::Suit suit, int number);
};

class JadeSeal : public Treasure
{
    Q_OBJECT

public:
    Q_INVOKABLE JadeSeal(Card::Suit suit, int number);

    virtual void onUninstall(ServerPlayer *player) const;
};

class Pagoda : public Treasure
{
    Q_OBJECT

public:
    Q_INVOKABLE Pagoda(Card::Suit suit, int number);

    virtual void onUninstall(ServerPlayer *player) const;
};

class Camouflage : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE Camouflage(Card::Suit suit, int number);
};

class AwaitExhausted : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE AwaitExhausted(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class SpellDuel : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE SpellDuel(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Kusuri : public BasicCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Kusuri(Card::Suit suit, int number);
    virtual QString getSubtype() const;

    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool isAvailable(const Player *player) const;
    //virtual bool targetFixed() const;
};

class TestCardPackage : public Package
{
    Q_OBJECT

public:
    TestCardPackage();
};

#endif
#pragma once
