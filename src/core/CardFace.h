#ifndef TOUHOUKILL_CARDFACE_H_
#define TOUHOUKILL_CARDFACE_H_

// BE WARE! THIS FILE IS USED IN BOTH SWIG AND C++.
// MAKE SURE THE GRAMMAR IS COMPATIBLE BETWEEN 2 LANGUAGES.

#ifndef SWIG

#include "global.h"
#include "qsgscore.h"

#include <QString>

class Player;
class RoomObject;
class Card;

class CardFacePrivate;
struct CardUseStruct;
struct CardEffectStruct;
struct JudgeStruct;
#endif

/**
 * @brief The functional model of a given card.
 */
class QSGS_CORE_EXPORT CardFace
{
public:
#ifndef SWIG
    explicit CardFace(const QString &name);
    virtual ~CardFace();
#endif

    // text property
    QString name() const;

    // type property
    virtual QSanguosha::CardType type() const = 0;
    virtual QString typeName() const = 0;
    QString subTypeName() const;
    bool isKindOf(const QString &cardType) const;

    // Can we have a better way to replace this function? Maybe using `match`
    // Fs: This is just a convenience function....
    inline bool isNdTrick() const
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
    bool canDamage() const;
    void setCanDamage(bool can);
    bool canRecover() const;
    void setCanRecover(bool can);
    // Fs: canRecast should be property of Card.
    // Seems like it should be dealt in UI and GameRule instead of the logic in Card/CardFace itself.
    // Currently CardFace::onUse and CardFace::targetFixed/targetFeasible are hacked to support recast
    // TODO_Fs: This may be changed to using skillcard/recastcard when UI/client detects a recast operation
    // so that there will be no logic in CardFace for implementing recasting
    // Note: In HulaoPass mode, all weapon can be recast according to the game rule.
    // virtual bool canRecast() const;
    bool hasEffectValue() const;
    void setHasEffectValue(bool can);
    bool hasPreAction() const;
    void setHasPreAction(bool can);

    // This method provides a default handling method suggested by the card face.
    // Almost every actual card has its handlingMethod to be QSanguosha::MethodUse.
    QSanguosha::HandlingMethod defaultHandlingMethod() const;
    void setDefaultHandlingMethod(QSanguosha::HandlingMethod can);

    // Functions
    bool targetFixed(const Player *player, const Card *card) const;
    void setTargetFixed(bool fixed);

    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self, const Card *card) const;

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
    // TODO: remove this virtual
    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const;

    bool isAvailable(const Player *player, const Card *card) const;

    const Card *validate(const CardUseStruct &cardUse) const;
    const Card *validateInResponse(Player *player, const Card *original_card) const;

    void doPreAction(RoomObject *room, const CardUseStruct &use) const;

    // TODO_Fs: Aren't the names of these 2 functions easy to be misunderstood?
    void onUse(RoomObject *room, const CardUseStruct &use) const; // Shouldn't this be "processOfUsing" / "usingProcess" or something like this?
    void use(RoomObject *room, const CardUseStruct &use) const; // Shouldn't this be "onUse"?

    void onEffect(const CardEffectStruct &effect) const;
    bool isCancelable(const CardEffectStruct &effect) const;
    void onNullified(Player *player, const Card *card) const;

protected:
    virtual void defaultOnUse(RoomObject *room, const CardUseStruct &use) const;
    virtual void defaultUse(RoomObject *room, const CardUseStruct &use) const;

private:
    CardFace() = delete;
    CardFacePrivate *const d;
    Q_DISABLE_COPY_MOVE(CardFace)
};

class QSGS_CORE_EXPORT BasicCard : public CardFace
{
public:
#ifndef SWIG
    BasicCard(const QString &name);
    ~BasicCard() override = default;

    QSanguosha::CardType type() const override;
    QString typeName() const override;
#endif
};

class QSGS_CORE_EXPORT EquipCard : public CardFace
{
public:
#ifndef SWIG
    EquipCard(const QString &name);
    ~EquipCard() override = default;

    QSanguosha::CardType type() const override;
    QString typeName() const override;
#endif

    virtual QSanguosha::EquipLocation location() const = 0;

    // TODO: should these function have a 2nd parameter which is Card?
    void onInstall(Player *player) const;
    void onUninstall(Player *player) const;

protected:
    virtual void defaultOnInstall(Player *player) const;
    virtual void defaultOnUninstall(Player *player) const;
};

class WeaponPrivate;

class QSGS_CORE_EXPORT Weapon : public EquipCard
{
public:
#ifndef SWIG
    Weapon(const QString &name);
    ~Weapon() override;

    QSanguosha::EquipLocation location() const override;
#endif

    int range() const;
    void setRange(int r);

private:
    WeaponPrivate *const d;
};

class QSGS_CORE_EXPORT Armor : public EquipCard
{
public:
#ifndef SWIG
    Armor(const QString &name);
    ~Armor() override = default;

    QSanguosha::EquipLocation location() const override;
#endif
};

class QSGS_CORE_EXPORT DefensiveHorse : public EquipCard
{
public:
#ifndef SWIG
    DefensiveHorse(const QString &name);
    ~DefensiveHorse() override = default;

    QSanguosha::EquipLocation location() const override;
#endif

    // TODO_Fs: this should be implemented using DistanceSkill
    // virtual int correction() const;
};

class QSGS_CORE_EXPORT OffensiveHorse : public EquipCard
{
public:
#ifndef SWIG
    OffensiveHorse(const QString &name);
    ~OffensiveHorse() override = default;

    QSanguosha::EquipLocation location() const override;
#endif

    // virtual int correction() const;
};

class QSGS_CORE_EXPORT Treasure : public EquipCard
{
public:
#ifndef SWIG
    Treasure(const QString &name);
    ~Treasure() override = default;

    QSanguosha::EquipLocation location() const override;
#endif
};

class QSGS_CORE_EXPORT TrickCard : public CardFace
{
public:
#ifndef SWIG
    TrickCard(const QString &name);
    ~TrickCard() override = default;

    QSanguosha::CardType type() const override;
    QString typeName() const override;
#endif
};

class QSGS_CORE_EXPORT NonDelayedTrick : public TrickCard
{
public:
#ifndef SWIG
    NonDelayedTrick(const QString &name);
    ~NonDelayedTrick() override = default;
#endif
};

class DelayedTrickPrivate;

class QSGS_CORE_EXPORT DelayedTrick : public TrickCard
{
public:
#ifndef SWIG
    DelayedTrick(const QString &name);
    ~DelayedTrick() override;
#endif

    void takeEffect(Player *target) const;

    void setJudge(const JudgeStruct &j);
    JudgeStruct judge() const;

private:
    DelayedTrickPrivate *d;
};

class SkillCardPrivate;

class QSGS_CORE_EXPORT SkillCard : public CardFace
{
public:
#ifndef SWIG
    SkillCard(const QString &name);
    ~SkillCard() override;

    QSanguosha::CardType type() const override;
    QString typeName() const override;
#endif

    bool throwWhenUsing() const;
    void setThrowWhenUsing(bool can);

protected:
    void defaultOnUse(RoomObject *room, const CardUseStruct &use) const override;
    void defaultUse(RoomObject *room, const CardUseStruct &use) const override;

private:
    SkillCardPrivate *const d;
};

#endif
