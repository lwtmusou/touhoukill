#ifndef _STANDARD_H
#define _STANDARD_H

// #include "card.h"
#include "CardFace.h"
#include "package.h"
#include "roomthread.h"
// don't include "skill.h" here for skill.h included this file for equipcard

class StandardPackage : public Package
{
public:
    StandardPackage();
    void addGenerals();
};

class TestPackage : public Package
{
public:
    TestPackage();
};

class GlobalEffect : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GlobalEffect()
        : TrickCard()
    {
        setTargetFixed(true);
    }
    QString subTypeName() const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    bool isAvailable(const Player *player, const Card *card) const override;
};

class GodSalvation : public GlobalEffect
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit GodSalvation();
    bool isCancelable(const CardEffectStruct &effect) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class AmazingGrace : public GlobalEffect
{
    Q_OBJECT

public:
    Q_INVOKABLE AmazingGrace(Card::Suit suit, int number);
    void doPreAction(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, const CardUseStruct &use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
    bool isCancelable(const CardEffectStruct &effect) const override;

private:
    void clearRestCards(Room *room) const;
};

class AOE : public TrickCard
{
    Q_OBJECT

public:
    AOE()
        : TrickCard()
    {
        setTargetFixed(true);
    }
    QString subTypeName() const override;
    bool isAvailable(const Player *player, const Card *card) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
};

class SavageAssault : public AOE
{
    Q_OBJECT

public:
    Q_INVOKABLE SavageAssault(Card::Suit suit, int number);
    void onEffect(const CardEffectStruct &effect) const override;
};

class ArcheryAttack : public AOE
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit ArcheryAttack();
    void onEffect(const CardEffectStruct &effect) const override;
};

class SingleTargetTrick : public TrickCard
{
    Q_OBJECT

public:
    SingleTargetTrick()
        : TrickCard()
    {
    }
    QString subTypeName() const override;
    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
};

class Collateral : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE Collateral();
    bool isAvailable(const Player *player, const Card *card) const override;
    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    void onEffect(const CardEffectStruct &effect) const override;

private:
    bool doCollateral(Room *room, ServerPlayer *killer, ServerPlayer *victim, const QString &prompt) const;
};

class ExNihilo : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE ExNihilo();

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
    bool isAvailable(const Player *player, const Card *card) const override;
};

class Duel : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE Duel(Card::Suit suit, int number);
    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class Indulgence : public DelayedTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE Indulgence(Card::Suit suit, int number);

    QString subTypeName() const override;
    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    void takeEffect(ServerPlayer *target) const override;
};

class Disaster : public DelayedTrick
{
    Q_OBJECT

public:
    Disaster(Card::Suit suit, int number);
    QString subTypeName() const override;
    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    bool isAvailable(const Player *player, const Card *card) const override;
};

class Lightning : public Disaster
{
    Q_OBJECT

public:
    Q_INVOKABLE Lightning(Card::Suit suit, int number);

    void takeEffect(ServerPlayer *target) const override;
};

class Nullification : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE Nullification(Card::Suit suit, int number);

    void use(Room *room, const CardUseStruct &use) const override;
    bool isAvailable(const Player *player, const Card *card) const override;
};

// cards of standard package

class Slash : public BasicCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Slash(Card::Suit suit, int number);
    DamageStruct::Nature getNature() const;
    void setNature(DamageStruct::Nature nature);

    QString subTypeName() const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;

    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self, const Card *card) const override;
    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    bool isAvailable(const Player *player, const Card *card) const override;

    static bool IsAvailable(const Player *player, const Card *slash = nullptr, bool considerSpecificAssignee = true);
    static bool IsSpecificAssignee(const Player *player, const Player *from, const Card *slash);

protected:
    DamageStruct::Nature nature;
    mutable int drank;
};

class Jink : public BasicCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Jink(Card::Suit suit, int number);
    QString subTypeName() const override;
    bool isAvailable(const Player *player, const Card *card) const override;
};

class Peach : public BasicCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Peach(Card::Suit suit, int number);
    QString subTypeName() const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
    bool targetFixed(const Player *Self, const Card *card) const override;
    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    bool isAvailable(const Player *player, const Card *card) const override;
};

class Snatch : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE Snatch(Card::Suit suit, int number);

    bool isAvailable(const Player *player, const Card *card) const override;

    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class Dismantlement : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE Dismantlement(Card::Suit suit, int number);

    bool isAvailable(const Player *player, const Card *card) const override;

    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class LureTiger : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LureTiger(Card::Suit suit, int number);

    QString subTypeName() const override;

    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    void use(Room *room, const CardUseStruct &use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class Drowning : public AOE
{
    Q_OBJECT

public:
    Q_INVOKABLE Drowning(Card::Suit suit, int number);

    bool isCancelable(const CardEffectStruct &effect) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class KnownBoth : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KnownBoth(Card::Suit suit, int number);
    QString subTypeName() const override;

    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self, const Card *card) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
    void use(Room *room, const CardUseStruct &use) const override;
};

class SavingEnergy : public DelayedTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE SavingEnergy(Card::Suit suit, int number);

    QString subTypeName() const override;
    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const override;
    void takeEffect(ServerPlayer *target) const override;
};

#endif
