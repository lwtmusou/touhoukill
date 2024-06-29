#ifndef _STANDARD_H
#define _STANDARD_H

#include "card.h"
#include "package.h"
#include "roomthread.h"
// don't include "skill.h" here for skill.h included this file for equipcard

class StandardPackage : public Package
{
    Q_OBJECT

public:
    StandardPackage();
    void addGenerals();
};

class TestPackage : public Package
{
    Q_OBJECT

public:
    TestPackage();
};

class BasicCard : public Card
{
    Q_OBJECT

public:
    BasicCard(Suit suit, int number)
        : Card(suit, number)
    {
        handling_method = Card::MethodUse;
    }
    QString getType() const override;
    CardType getTypeId() const override;
};

class TrickCard : public Card
{
    Q_OBJECT

public:
    TrickCard(Suit suit, int number);
    void setCancelable(bool cancelable);

    QString getType() const override;
    CardType getTypeId() const override;
    bool isCancelable(const CardEffectStruct &effect) const override;

private:
    bool cancelable;
};

class EquipCard : public Card
{
    Q_OBJECT
    Q_ENUMS(Location)

public:
    enum Location
    {
        WeaponLocation,
        ArmorLocation,
        DefensiveHorseLocation,
        OffensiveHorseLocation,
        TreasureLocation
    };

    EquipCard(Suit suit, int number)
        : Card(suit, number, true)
    {
        handling_method = MethodUse;
    }

    QString getType() const override;
    CardType getTypeId() const override;

    bool isAvailable(const Player *player) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;

    virtual void onInstall(ServerPlayer *player) const;
    virtual void onUninstall(ServerPlayer *player) const;

    virtual Location location() const = 0;
};

class GlobalEffect : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GlobalEffect(Card::Suit suit, int number)
        : TrickCard(suit, number)
    {
        target_fixed = true;
    }
    QString getSubtype() const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    bool isAvailable(const Player *player) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
};

class GodSalvation : public GlobalEffect
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit GodSalvation(Card::Suit suit = Heart, int number = 1);
    bool isCancelable(const CardEffectStruct &effect) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class AmazingGrace : public GlobalEffect
{
    Q_OBJECT

public:
    Q_INVOKABLE AmazingGrace(Card::Suit suit, int number);
    void doPreAction(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
    bool isCancelable(const CardEffectStruct &effect) const override;

private:
    void clearRestCards(Room *room) const;
};

class AOE : public TrickCard
{
    Q_OBJECT

public:
    AOE(Suit suit, int number)
        : TrickCard(suit, number)
    {
        target_fixed = true;
    }
    QString getSubtype() const override;
    bool isAvailable(const Player *player) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
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
    Q_INVOKABLE explicit ArcheryAttack(Card::Suit suit = Heart, int number = 1);
    void onEffect(const CardEffectStruct &effect) const override;
};

class SingleTargetTrick : public TrickCard
{
    Q_OBJECT

public:
    SingleTargetTrick(Suit suit, int number)
        : TrickCard(suit, number)
    {
    }
    QString getSubtype() const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
};

class Collateral : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE Collateral(Card::Suit suit, int number);
    bool isAvailable(const Player *player) const override;
    //virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    //virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    void onEffect(const CardEffectStruct &effect) const override;

private:
    bool doCollateral(Room *room, ServerPlayer *killer, ServerPlayer *victim, const QString &prompt) const;
};

class ExNihilo : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE ExNihilo(Card::Suit suit, int number);

    bool targetFixed(const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
    bool isAvailable(const Player *player) const override;
};

class Duel : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE Duel(Card::Suit suit, int number);
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class DelayedTrick : public TrickCard
{
    Q_OBJECT

public:
    DelayedTrick(Suit suit, int number, bool movable = false);
    void onNullified(ServerPlayer *target) const override;

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
    QString getSubtype() const override;
    void onEffect(const CardEffectStruct &effect) const override;
    virtual void takeEffect(ServerPlayer *target) const = 0;
    JudgeStruct getJudge();

protected:
    JudgeStruct judge;

private:
    bool movable;
};

class Indulgence : public DelayedTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE Indulgence(Card::Suit suit, int number);

    QString getSubtype() const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void takeEffect(ServerPlayer *target) const override;
};

class Disaster : public DelayedTrick
{
    Q_OBJECT

public:
    Disaster(Card::Suit suit, int number);
    QString getSubtype() const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    bool isAvailable(const Player *player) const override;
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

    void use(Room *room, const CardUseStruct &card_use) const override;
    bool isAvailable(const Player *player) const override;
};

class Weapon : public EquipCard
{
    Q_OBJECT

public:
    Weapon(Suit suit, int number, int range);
    int getRange() const;

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    QString getSubtype() const override;

    Location location() const override;
    QString getCommonEffectName() const override;
    bool isAvailable(const Player *player) const override;

protected:
    int range;
};

class Armor : public EquipCard
{
    Q_OBJECT

public:
    Armor(Suit suit, int number)
        : EquipCard(suit, number)
    {
    }
    QString getSubtype() const override;

    Location location() const override;
    QString getCommonEffectName() const override;
};

class Treasure : public EquipCard
{
    Q_OBJECT

public:
    Treasure(Suit suit, int number)
        : EquipCard(suit, number)
    {
    }
    QString getSubtype() const override;

    Location location() const override;

    QString getCommonEffectName() const override;
};

class Horse : public EquipCard
{
    Q_OBJECT

public:
    Horse(Suit suit, int number, int correct);
    int getCorrect() const;

    Location location() const override;
    void onInstall(ServerPlayer *player) const override;
    void onUninstall(ServerPlayer *player) const override;

    QString getCommonEffectName() const override;

private:
    int correct;
};

class OffensiveHorse : public Horse
{
    Q_OBJECT

public:
    Q_INVOKABLE OffensiveHorse(Card::Suit suit, int number, int correct = -1);
    QString getSubtype() const override;
};

class DefensiveHorse : public Horse
{
    Q_OBJECT

public:
    Q_INVOKABLE DefensiveHorse(Card::Suit suit, int number, int correct = +1);
    QString getSubtype() const override;
};

// cards of standard package

class Slash : public BasicCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Slash(Card::Suit suit, int number);
    DamageStruct::Nature getNature() const;
    void setNature(DamageStruct::Nature nature);

    QString getSubtype() const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;

    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool isAvailable(const Player *player) const override;

    static bool IsAvailable(const Player *player, const Card *slash = nullptr, bool considerSpecificAssignee = true);
    static bool IsSpecificAssignee(const Player *player, const Player *from, const Card *slash);

protected:
    DamageStruct::Nature nature;
    mutable int drank;
    mutable int magic_drank;
};

class Jink : public BasicCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Jink(Card::Suit suit, int number);
    QString getSubtype() const override;
    bool isAvailable(const Player *player) const override;
};

class Peach : public BasicCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Peach(Card::Suit suit, int number);
    QString getSubtype() const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool isAvailable(const Player *player) const override;
};

class Snatch : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE Snatch(Card::Suit suit, int number);

    bool isAvailable(const Player *player) const override;

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class Dismantlement : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE Dismantlement(Card::Suit suit, int number);

    bool isAvailable(const Player *player) const override;

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class LureTiger : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LureTiger(Card::Suit suit, int number);

    QString getSubtype() const override;

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    //void use(Room *room, const CardUseStruct &card_use) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class Drowning : public AOE
{
    Q_OBJECT

public:
    Q_INVOKABLE Drowning(Card::Suit suit, int number);

    //virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    bool isCancelable(const CardEffectStruct &effect) const override;
    void onEffect(const CardEffectStruct &effect) const override;
    // virtual bool isAvailable(const Player *player) const;
    //virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class KnownBoth : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KnownBoth(Card::Suit suit, int number);
    QString getSubtype() const override;

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class SavingEnergy : public DelayedTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE SavingEnergy(Card::Suit suit, int number);

    QString getSubtype() const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void takeEffect(ServerPlayer *target) const override;
};

#endif
