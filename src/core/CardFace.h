#ifndef TOUHOUKILL_CARDFACE_H_
#define TOUHOUKILL_CARDFACE_H_

#include "global.h"

#include <QObject>
#include <QString>

class Player;
class RoomObject;
class Card;

class CardFacePrivate;
struct CardUseStruct;
struct CardEffectStruct;
struct JudgeStruct;

/**
 * @interface The functional model of a given card.
 */
class CardFace : public QObject
{
    Q_OBJECT

public:
    CardFace();
    virtual ~CardFace();

    // text property
    virtual QString name() const; // For Lua skill card. Lua skill card would provide dynamic name.
    virtual QString description() const;
    virtual QString commonEffectName() const;
    virtual QString effectName() const;

    // type property
    virtual QSanguosha::CardType type() const = 0;
    virtual QString typeName() const = 0;
    virtual QString subTypeName() const = 0;
    virtual bool isKindOf(const char *cardType) const;
    virtual bool matchType(const QString &pattern) const;

    // Can we have a better way to replace this function? Maybe using `match`
    // Fs: This is just a convenience function....
    virtual bool isNDTrick() const;

    // property identifier.
    // CardFace provides the default value of these property
    // But they could be dynamic and explained by Card itself.
    // For example, some skill may let Slash not be regarded as damage card?
    // Fs: There is a skill which has a skill named "Xianshi" for God Patchouli in TouhouKill. It needs an extremely hacked Card/CardFace which changes all the effect of a certain Card.
    // Return value of "canDamage" and "canRecover" is affected by "Xianshi" in this case.
    // TODO_Fs: ***non-virtual*** property setters for simplifying logic, only reimplement these functions when complex logic is needed
    virtual bool canDamage() const;
    void setCanDamage(bool can);
    virtual bool canRecover() const;
    void setCanRecover(bool can);
    // Fs: canRecast should be property of Card.
    // Seems like it should be dealt in UI and GameRule instead of the logic in Card/CardFace itself.
    // Currently CardFace::onUse and CardFace::targetFixed/targetFeasible are hacked to support recast
    // TODO_Fs: This may be changed to using skillcard/recastcard when UI/client detects a recast operation
    // so that there will be no logic in CardFace for implementing recasting
    // Note: In HulaoPass mode, all weapon can be recast according to the game rule.
    // virtual bool canRecast() const;
    virtual bool hasEffectValue() const;
    void setHasEffectValue(bool can);
    virtual bool throwWhenUsing() const;
    void setThrowWhenUsing(bool can);
    virtual bool hasPreAction() const;
    void setHasPreAction(bool can);

    // This method provides a default handling method suggested by the card face.
    // Almost every actual card has its handlingMethod to be QSanguosha::MethodUse.
    virtual QSanguosha::HandlingMethod defaultHandlingMethod() const;
    void setDefaultHandlingMethod(QSanguosha::HandlingMethod can);

    // Functions
    virtual bool targetFixed(const Player *Self, const Card *card) const;
    void setTargetFixed(bool fixed);

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self, const Card *card) const;

    // This is the merged targetFilter implementation.
    /**
     * Calculate the maximum vote for specific target.
     * 
     * @param targets The lists where all selected targets are.
     * @param to_select The player to be judged.
     * @param Self The user of the card.
     * @param card the card itself
     * 
     * @return the maximum vote for to_select.
     * 
     * @note to_select will be selectable until its appearance in targets >= its maximum vote. 
     */
    virtual int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const;

    virtual bool isAvailable(const Player *player, const Card *card) const;

    // TODO_Fs: Actually I don't know the use case of this function.
    // Is it only for skill "tianqu"?
    virtual bool ignoreCardValidity(const Player *player) const;
    virtual const Card *validate(const CardUseStruct &cardUse) const;
    virtual const Card *validateInResponse(Player *user, const Card *original_card) const;

    virtual void doPreAction(RoomObject *room, const CardUseStruct &card_use) const;

    // TODO_Fs: Aren't the names of these 2 functions easy to be misunderstood?
    virtual void onUse(RoomObject *room, const CardUseStruct &card_use) const; // Shouldn't this be "processOfUsing" / "usingProcess" or something like this?
    virtual void use(RoomObject *room, const CardUseStruct &use) const; // Shouldn't this be "onUse"?

    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual bool isCancelable(const CardEffectStruct &effect) const;
    virtual void onNullified(Player *target, const Card *card) const;

protected:
    CardFacePrivate *const d;
};

class BasicCard : public CardFace
{
    Q_OBJECT

public:
    BasicCard() = default;
    ~BasicCard() override = default;

    QSanguosha::CardType type() const override;
    QString typeName() const override;
};

class EquipCard : public CardFace
{
    Q_OBJECT

public:
    EquipCard() = default;
    ~EquipCard() override = default;

    QSanguosha::CardType type() const override;
    QString typeName() const override;

    virtual QSanguosha::EquipLocation location() const = 0;

    virtual void onInstall(Player *player) const;
    virtual void onUninstall(Player *player) const;
};

class WeaponPrivate;

class Weapon : public EquipCard
{
    Q_OBJECT

public:
    Weapon();
    ~Weapon() override;

    QString subTypeName() const override;
    QSanguosha::EquipLocation location() const override;

    int range() const;
    void setRange(int r);

private:
    WeaponPrivate *const d;
};

class Armor : public EquipCard
{
    Q_OBJECT

public:
    Armor() = default;
    ~Armor() override = default;

    QString subTypeName() const override;
    QSanguosha::EquipLocation location() const override;
};

class DefensiveHorse : public EquipCard
{
    Q_OBJECT

public:
    DefensiveHorse() = default;
    ~DefensiveHorse() override = default;

    QString subTypeName() const override;
    QSanguosha::EquipLocation location() const override;

    // TODO_Fs: this should be implemented using DistanceSkill
    // virtual int correction() const;
};

class OffensiveHorse : public EquipCard
{
    Q_OBJECT

public:
    OffensiveHorse() = default;
    ~OffensiveHorse() override = default;

    QString subTypeName() const override;
    QSanguosha::EquipLocation location() const override;

    // virtual int correction() const;
};

class Treasure : public EquipCard
{
    Q_OBJECT

public:
    Treasure() = default;
    ~Treasure() override = default;

    QString subTypeName() const override;
    QSanguosha::EquipLocation location() const override;
};

class TrickCard : public CardFace
{
    Q_OBJECT

public:
    TrickCard() = default;
    ~TrickCard() override = default;

    QSanguosha::CardType type() const override;
    QString typeName() const override;
};

class NonDelayedTrick : public TrickCard
{
    Q_OBJECT

public:
    NonDelayedTrick() = default;
    ~NonDelayedTrick() override = default;

    QString subTypeName() const override;
    bool isNDTrick() const override;
};

class DelayedTrick : public TrickCard
{
    Q_OBJECT

public:
    DelayedTrick();
    ~DelayedTrick() override = default;

    QString subTypeName() const override;
    virtual void takeEffect(Player *target) const = 0;

    // returns a copy of j if j is not nullptr.
    JudgeStruct judge() const;

protected:
    // j is initialized to be nullptr when constructing.
    // DelayedTrick don't own this struct. DO REMEMBER TO CLEANUP d WHEN DelayedTrick IS DESTRUCTING.
    // A suggested way for initializing j is as follows (Let's take Indulgence as an example of inherited DelayedTrick):
#if 0
    Indulgence::Indulgence()
    {
        static JudgeStruct js;
        // do some tweaks about js for Indulgence
        j = &js;
    }
#endif
    const JudgeStruct *j;
};

class SkillCard : public CardFace
{
    Q_OBJECT

public:
    SkillCard() = default;
    ~SkillCard() override = default;

    QSanguosha::CardType type() const override;
    QString typeName() const override;
    QString subTypeName() const override;
};

#endif
