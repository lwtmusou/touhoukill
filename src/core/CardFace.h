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
class CardFace
{
public:
    explicit CardFace(const QString &name);
    virtual ~CardFace();

    // text property
    QSGS_LUA_API QString name() const;

    // type property
    virtual QSanguosha::CardType type() const = 0;
    virtual QString typeName() const = 0;
    QString subTypeName() const;
    QSGS_LUA_API bool isKindOf(const QString &cardType) const;

    // Can we have a better way to replace this function? Maybe using `match`
    // Fs: This is just a convenience function....
    inline bool isNDTrick() const
    {
        return isKindOf(QStringLiteral("NonDelayedTrick"));
    }

    // property identifier.
    // CardFace provides the default value of these property
    // But they could be dynamic and explained by Card itself.
    // For example, some skill may let Slash not be regarded as damage card?
    // Fs: There is a skill which has a skill named "Xianshi" for God Patchouli in TouhouKill. It needs an extremely hacked Card/CardFace which changes all the effect of a certain Card.
    // Return value of "canDamage" and "canRecover" is affected by "Xianshi" in this case.
    // TODO_Fs: ***non-virtual*** property setters for simplifying logic, only reimplement these functions when complex logic is needed
    QSGS_LUA_API bool canDamage() const;
    void setCanDamage(bool can);
    QSGS_LUA_API bool canRecover() const;
    void setCanRecover(bool can);
    // Fs: canRecast should be property of Card.
    // Seems like it should be dealt in UI and GameRule instead of the logic in Card/CardFace itself.
    // Currently CardFace::onUse and CardFace::targetFixed/targetFeasible are hacked to support recast
    // TODO_Fs: This may be changed to using skillcard/recastcard when UI/client detects a recast operation
    // so that there will be no logic in CardFace for implementing recasting
    // Note: In HulaoPass mode, all weapon can be recast according to the game rule.
    // virtual bool canRecast() const;
    QSGS_LUA_API bool hasEffectValue() const;
    void setHasEffectValue(bool can);
    QSGS_LUA_API bool hasPreAction() const;
    void setHasPreAction(bool can);

    // This method provides a default handling method suggested by the card face.
    // Almost every actual card has its handlingMethod to be QSanguosha::MethodUse.
    QSGS_LUA_API QSanguosha::HandlingMethod defaultHandlingMethod() const;
    void setDefaultHandlingMethod(QSanguosha::HandlingMethod can);

    // Functions
    QSGS_LUA_API bool targetFixed(const Player *player, const Card *card) const;
    void setTargetFixed(bool fixed);

    QSGS_LUA_API bool targetsFeasible(const QList<const Player *> &targets, const Player *Self, const Card *card) const;

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
    // virtual for current aux-skills
    QSGS_LUA_API virtual int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const;

    QSGS_LUA_API bool isAvailable(const Player *player, const Card *card) const;

    QSGS_LUA_API const Card *validate(const CardUseStruct &cardUse) const;
    QSGS_LUA_API const Card *validateInResponse(Player *player, const Card *original_card) const;

    QSGS_LUA_API void doPreAction(RoomObject *room, const CardUseStruct &use) const;

    // TODO_Fs: Aren't the names of these 2 functions easy to be misunderstood?
    QSGS_LUA_API void onUse(RoomObject *room, const CardUseStruct &use) const; // Shouldn't this be "processOfUsing" / "usingProcess" or something like this?
    QSGS_LUA_API void use(RoomObject *room, const CardUseStruct &use) const; // Shouldn't this be "onUse"?

    QSGS_LUA_API void onEffect(const CardEffectStruct &effect) const;
    QSGS_LUA_API bool isCancelable(const CardEffectStruct &effect) const;
    QSGS_LUA_API void onNullified(Player *player, const Card *card) const;

protected:
    virtual void defaultOnUse(RoomObject *room, const CardUseStruct &use) const;
    virtual void defaultUse(RoomObject *room, const CardUseStruct &use) const;

private:
    CardFace() = delete;
    CardFacePrivate *const d;
    Q_DISABLE_COPY_MOVE(CardFace)
};

class BasicCard : public CardFace
{
public:
    BasicCard(const QString &name);
    ~BasicCard() override = default;

    QSanguosha::CardType type() const override;
    QString typeName() const override;
};

class EquipCard : public CardFace
{
public:
    EquipCard(const QString &name);
    ~EquipCard() override = default;

    QSanguosha::CardType type() const override;
    QString typeName() const override;

    virtual QSanguosha::EquipLocation location() const = 0;

    // TODO: should these function have a 2nd parameter which is Card?
    void onInstall(Player *player) const;
    void onUninstall(Player *player) const;

protected:
    virtual void defaultOnInstall(Player *player) const;
    virtual void defaultOnUninstall(Player *player) const;
};

class WeaponPrivate;

class Weapon : public EquipCard
{
public:
    Weapon(const QString &name);
    ~Weapon() override;

    QSanguosha::EquipLocation location() const override;

    int range() const;
    void setRange(int r);

private:
    WeaponPrivate *const d;
};

class Armor : public EquipCard
{
public:
    Armor(const QString &name);
    ~Armor() override = default;

    QSanguosha::EquipLocation location() const override;
};

class DefensiveHorse : public EquipCard
{
public:
    DefensiveHorse(const QString &name);
    ~DefensiveHorse() override = default;

    QSanguosha::EquipLocation location() const override;

    // TODO_Fs: this should be implemented using DistanceSkill
    // virtual int correction() const;
};

class OffensiveHorse : public EquipCard
{
public:
    OffensiveHorse(const QString &name);
    ~OffensiveHorse() override = default;

    QSanguosha::EquipLocation location() const override;

    // virtual int correction() const;
};

class Treasure : public EquipCard
{
public:
    Treasure(const QString &name);
    ~Treasure() override = default;

    QSanguosha::EquipLocation location() const override;
};

class TrickCard : public CardFace
{
public:
    TrickCard(const QString &name);
    ~TrickCard() override = default;

    QSanguosha::CardType type() const override;
    QString typeName() const override;
};

class NonDelayedTrick : public TrickCard
{
public:
    NonDelayedTrick(const QString &name);
    ~NonDelayedTrick() override = default;
};

class DelayedTrick : public TrickCard
{
public:
    DelayedTrick(const QString &name);
    ~DelayedTrick() override = default;

    void takeEffect(Player *target) const;

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

class SkillCardPrivate;

class SkillCard : public CardFace
{
public:
    SkillCard(const QString &name);
    ~SkillCard() override;

    QSanguosha::CardType type() const override;
    QString typeName() const override;

    virtual bool throwWhenUsing() const;
    void setThrowWhenUsing(bool can);

protected:
    void defaultOnUse(RoomObject *room, const CardUseStruct &use) const override;
    void defaultUse(RoomObject *room, const CardUseStruct &use) const override;

private:
    SkillCardPrivate *const d;
};

#endif
