#ifndef THKILL_WASHOUT_H
#define THKILL_WASHOUT_H

#include "maneuvering.h"
#include "standard.h"

class DebuffSlash : public Slash
{
    Q_OBJECT

public:
    DebuffSlash(Suit suit, int number);
    bool matchTypeOrName(const QString &pattern) const override;
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
    bool matchTypeOrName(const QString &pattern) const override;
};

class ChainJink : public NatureJink
{
    Q_OBJECT

public:
    Q_INVOKABLE ChainJink(Card::Suit suit, int number);

    void onEffect(const CardEffectStruct &effect) const override;
};

class LightJink : public NatureJink
{
    Q_OBJECT

public:
    Q_INVOKABLE LightJink(Card::Suit suit, int number);

    void onEffect(const CardEffectStruct &effect) const override;
};

class MagicAnaleptic : public Analeptic
{
    Q_OBJECT

public:
    Q_INVOKABLE MagicAnaleptic(Card::Suit suit, int number);
    bool matchTypeOrName(const QString &pattern) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class SuperPeach : public Peach
{
    Q_OBJECT

public:
    Q_INVOKABLE SuperPeach(Card::Suit suit, int number);
    bool matchTypeOrName(const QString &pattern) const override;

    void onEffect(const CardEffectStruct &effect) const override;
};

class Gun : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE Gun(Card::Suit suit, int number);
};

class Hakkero : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE Hakkero(Card::Suit suit, int number);
};

class JadeSeal : public Treasure
{
    Q_OBJECT

public:
    Q_INVOKABLE JadeSeal(Card::Suit suit, int number);

    void onUninstall(ServerPlayer *player) const override;
};

class Pagoda : public Treasure
{
    Q_OBJECT

public:
    Q_INVOKABLE Pagoda(Card::Suit suit, int number);

    void onUninstall(ServerPlayer *player) const override;
};

class Camouflage : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE Camouflage(Card::Suit suit, int number);
};

class Hagoromo : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE Hagoromo(Card::Suit suit, int number);
};

class AwaitExhausted : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE AwaitExhausted(Card::Suit suit, int number);

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class AllianceFeast : public GlobalEffect
{
    Q_OBJECT

public:
    Q_INVOKABLE AllianceFeast(Card::Suit suit, int number);
    bool isCancelable(const CardEffectStruct &effect) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class BoneHealing : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE BoneHealing(Card::Suit suit, int number);

    bool isAvailable(const Player *player) const override;

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class SpringBreath : public DelayedTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE SpringBreath(Card::Suit suit, int number);

    QString getSubtype() const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void takeEffect(ServerPlayer *target) const override;

    void onNullified(ServerPlayer *target) const override;
};

class WashOutPackage : public Package
{
    Q_OBJECT

public:
    WashOutPackage();
};

#endif
