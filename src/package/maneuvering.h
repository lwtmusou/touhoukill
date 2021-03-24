#ifndef _MANEUVERING_H
#define _MANEUVERING_H

#include "standard.h"

class NatureSlash : public Slash
{
    Q_OBJECT

public:
    NatureSlash(Suit suit, int number, DamageStruct::Nature nature);
    bool matchTypeOrName(const QString &pattern) const override;
};

class ThunderSlash : public NatureSlash
{
    Q_OBJECT

public:
    Q_INVOKABLE ThunderSlash(Card::Suit suit, int number);
};

class FireSlash : public NatureSlash
{
    Q_OBJECT

public:
    Q_INVOKABLE FireSlash(Card::Suit suit, int number);
};

class IceSlash : public NatureSlash
{
    Q_OBJECT

public:
    Q_INVOKABLE IceSlash(Card::Suit suit, int number);
};

class Analeptic : public BasicCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Analeptic(Card::Suit suit, int number);
    QString getSubtype() const override;
    bool canRecover() const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    static bool IsAvailable(const Player *player, const Card *analeptic = NULL);

    bool isAvailable(const Player *player) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class Fan : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE Fan(Card::Suit suit, int number);
};

class GudingBlade : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE GudingBlade(Card::Suit suit, int number);
};

class IronArmor : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE IronArmor(Card::Suit suit, int number);
};

class Vine : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE Vine(Card::Suit suit, int number);
};

class SilverLion : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE SilverLion(Card::Suit suit, int number);

    void onUninstall(ServerPlayer *player) const override;
};

class IronChain : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IronChain(Card::Suit suit, int number);

    QString getSubtype() const override;

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class FireAttack : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE FireAttack(Card::Suit suit, int number);

    bool isAvailable(const Player *player) const override;

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class SupplyShortage : public DelayedTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE SupplyShortage(Card::Suit suit, int number);

    bool isAvailable(const Player *player) const override;

    QString getSubtype() const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void takeEffect(ServerPlayer *target) const override;
};

class ManeuveringPackage : public Package
{
    Q_OBJECT

public:
    ManeuveringPackage();
};

#endif
