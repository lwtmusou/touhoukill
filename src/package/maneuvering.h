#ifndef _MANEUVERING_H
#define _MANEUVERING_H

#include "standard.h"

class NatureSlash : public Slash
{
    Q_OBJECT

public:
    NatureSlash(DamageStruct::Nature nature);
    bool matchType(const QString &pattern) const override;
};

class ThunderSlash : public NatureSlash
{
    Q_OBJECT

public:
    Q_INVOKABLE ThunderSlash();
};

class FireSlash : public NatureSlash
{
    Q_OBJECT

public:
    Q_INVOKABLE FireSlash();
};

class IceSlash : public NatureSlash
{
    Q_OBJECT

public:
    Q_INVOKABLE IceSlash();
};

class Analeptic : public BasicCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Analeptic();
    QString subTypeName() const override;
    bool canRecover() const override;
    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    static bool IsAvailable(const Player *player, const Card *analeptic = nullptr);

    bool isAvailable(const Player *player, const Card *card) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class Fan : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE Fan();
};

class GudingBlade : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE GudingBlade();
};

class IronArmor : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE IronArmor();
};

class Vine : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE Vine();
};

class SilverLion : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE SilverLion();

    void onUninstall(ServerPlayer *player) const override;
};

class IronChain : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE IronChain();

    QString subTypeName() const override;

    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self, const Card *card) const override;

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class FireAttack : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE FireAttack();

    bool isAvailable(const Player *player, const Card *card) const override;

    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class SupplyShortage : public DelayedTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE SupplyShortage();

    bool isAvailable(const Player *player, const Card *card) const override;

    QString subTypeName() const override;
    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    void takeEffect(ServerPlayer *target) const override;
};

class ManeuveringPackage : public Package
{
    Q_OBJECT

public:
    ManeuveringPackage();
};

#endif
