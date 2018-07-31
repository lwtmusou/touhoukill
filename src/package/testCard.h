#ifndef _TESTCARD_H
#define _TESTCARD_H

#include "standard.h"
#include "maneuvering.h"

class DebuffSlash : public Slash
{
    Q_OBJECT

public:
    DebuffSlash(Suit suit, int number);
    virtual bool match(const QString &pattern) const;
};

class IronSlash : public DebuffSlash
{
    Q_OBJECT

public:
    Q_INVOKABLE IronSlash(Card::Suit suit, int number);

    static void debuffEffect(const SlashEffectStruct &effect);
};

class LightSlash : public DebuffSlash
{
    Q_OBJECT

public:
    Q_INVOKABLE LightSlash(Card::Suit suit, int number);

    static void debuffEffect(const SlashEffectStruct &effect);
};

class PowerSlash : public DebuffSlash
{
    Q_OBJECT

public:
    Q_INVOKABLE PowerSlash(Card::Suit suit, int number);

    static void debuffEffect(const SlashEffectStruct &effect);

};



class NatureJink : public Jink
{
    Q_OBJECT

public:
    NatureJink(Suit suit, int number);
    virtual bool match(const QString &pattern) const;
};

class ChainJink : public NatureJink
{
    Q_OBJECT

public:
    Q_INVOKABLE ChainJink(Card::Suit suit, int number);

    virtual void onEffect(const CardEffectStruct &effect) const;
};


class LightJink : public NatureJink
{
    Q_OBJECT

public:
    Q_INVOKABLE LightJink(Card::Suit suit, int number);

    virtual void onEffect(const CardEffectStruct &effect) const;
};


class MagicAnaleptic : public Analeptic
{
    Q_OBJECT

public:
    Q_INVOKABLE MagicAnaleptic(Card::Suit suit, int number);
    virtual bool match(const QString &pattern) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};


class SuperPeach : public Peach
{
    Q_OBJECT

public:
    Q_INVOKABLE SuperPeach(Card::Suit suit, int number);
    virtual bool match(const QString &pattern) const;

    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool isAvailable(const Player *player) const;
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
