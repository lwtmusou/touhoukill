#ifndef _TESTCARD_H
#define _TESTCARD_H

#include "maneuvering.h"
#include "standard.h"

class DebuffSlash : public Slash
{
    Q_OBJECT

public:
    DebuffSlash();
    bool matchType(const QString &pattern) const override;
};

class IronSlash : public DebuffSlash
{
    Q_OBJECT

public:
    Q_INVOKABLE IronSlash();

    static void debuffEffect(const SlashEffectStruct &effect);
};

class LightSlash : public DebuffSlash
{
    Q_OBJECT

public:
    Q_INVOKABLE LightSlash();

    static void debuffEffect(const SlashEffectStruct &effect);
};

class PowerSlash : public DebuffSlash
{
    Q_OBJECT

public:
    Q_INVOKABLE PowerSlash();

    static void debuffEffect(const SlashEffectStruct &effect);
};

class NatureJink : public Jink
{
    Q_OBJECT

public:
    NatureJink();
    bool matchType(const QString &pattern) const override;
};

class ChainJink : public NatureJink
{
    Q_OBJECT

public:
    Q_INVOKABLE ChainJink();

    void onEffect(const CardEffectStruct &effect) const override;
};

class LightJink : public NatureJink
{
    Q_OBJECT

public:
    Q_INVOKABLE LightJink();

    void onEffect(const CardEffectStruct &effect) const override;
};

class MagicAnaleptic : public Analeptic
{
    Q_OBJECT

public:
    Q_INVOKABLE MagicAnaleptic();
    bool matchType(const QString &pattern) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class SuperPeach : public Peach
{
    Q_OBJECT

public:
    Q_INVOKABLE SuperPeach();
    bool matchType(const QString &pattern) const override;

    void onEffect(const CardEffectStruct &effect) const override;
    bool targetFixed(const Player *Self, const Card *card) const override;
    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    bool isAvailable(const Player *player, const Card *card) const override;
};

class Gun : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE Gun();
};

class Pillar : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE Pillar();
};

class Hakkero : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE Hakkero();
};

class JadeSeal : public Treasure
{
    Q_OBJECT

public:
    Q_INVOKABLE JadeSeal();

    void onUninstall(ServerPlayer *player) const override;
};

class Pagoda : public Treasure
{
    Q_OBJECT

public:
    Q_INVOKABLE Pagoda();

    void onUninstall(ServerPlayer *player) const override;
};

class Camouflage : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE Camouflage();
};

class Hagoromo : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE Hagoromo();
};

class AwaitExhausted : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE AwaitExhausted();

    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class AllianceFeast : public GlobalEffect
{
    Q_OBJECT

public:
    Q_INVOKABLE AllianceFeast();
    bool isCancelable(const CardEffectStruct &effect) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class BoneHealing : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE BoneHealing();

    bool isAvailable(const Player *player, const Card *card) const override;

    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class SpringBreath : public DelayedTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE SpringBreath();

    QString subTypeName() const override;
    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    void takeEffect(ServerPlayer *target) const override;
};

class TestCardPackage : public Package
{
    Q_OBJECT

public:
    TestCardPackage();
};

#endif
#pragma once
