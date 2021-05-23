#ifndef _SKILL_H
#define _SKILL_H

class Player;
class Card;
class ServerPlayer;
class QDialog;

#include "CardFace.h"
#include "room.h"
#include "structs.h"

#include <QDialog>
#include <QObject>
#include <QSet>

class Skill : public QObject
{
    Q_OBJECT

public:
    enum Frequency
    {
        Frequent,
        NotFrequent,
        Compulsory,
        NotCompulsory,
        Limited,
        Wake,
        Eternal
    };
    Q_ENUM(Frequency)

    enum ArrayType
    {
        None,
        Formation,
        Siege
    };
    Q_ENUM(ArrayType)

    enum ShowType
    {
        ShowTrigger,
        ShowStatic,
        ShowViewAs,
    };
    Q_ENUM(ShowType)

    // do not use even ANY symbols in skill name anymore!
    // use multi-parameterized ones instead
    explicit Skill(const QString &name, Frequency frequency = NotFrequent, ShowType showType = ShowTrigger, bool lordSkill = false, bool attachedLordSkill = false);

    // TODO: refactor propersal:
    // Battle Array Skill should not be a separate type
    // Currently BattleArraySkill runs SummonArray when turn starts.
    // This seems able to be done in a global trigger with priority 3 to trigger a formation summon in start phase.
    // Since a summon may also be done in the spare time during play phase, a common button for the summon is preferred.
    // The summon itself should be a function of GameLogic
    void setupForArraySummon(ArrayType arrayType);

    bool isLordSkill() const;
    bool isAttachedLordSkill() const;
    virtual bool shouldBeVisible(const Player *Self) const; // usually for attached skill
    QString getDescription(bool yellow = true, bool addHegemony = false) const;
    QString getNotice(int index) const;
    bool isVisible() const;

    virtual int getEffectIndex(const ServerPlayer *player, const Card *card) const;
    virtual QDialog *getDialog() const;

    void initMediaSource();
    void playAudioEffect(int index = -1) const;
    Frequency getFrequency() const;
    QString getLimitMark() const;
    QString getRelatedMark() const;
    QString getRelatedPileName() const;
    QStringList getSources() const;
    ShowType getShowType() const; //nue_god
    virtual bool canPreshow() const; //hegemony
    virtual bool relateToPlace(bool head = true) const;

private:
    Frequency frequency;
    ShowType show_type;
    bool lord_skill;
    bool attached_lord_skill;
    QStringList sources;

protected:
    QString limit_mark;
    QString related_mark; //while changing hero, this will be removed
    QString related_pile;
    QString relate_to_place;
};

class ViewAsSkill : public Skill
{
    Q_OBJECT

public:
    explicit ViewAsSkill(const QString &name);

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const = 0;
    virtual const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const = 0;

    bool isAvailable(const Player *invoker, CardUseStruct::CardUseReason reason, const QString &pattern) const;
    virtual bool isEnabledAtPlay(const Player *player) const;
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const;
    virtual bool isEnabledAtNullification(const ServerPlayer *player) const;
    virtual QStringList getDialogCardOptions() const;
    static const ViewAsSkill *parseViewAsSkill(const Skill *skill);
    inline bool isResponseOrUse() const
    {
        return response_or_use;
    }
    virtual QString getExpandPile(const Player *Self) const;

protected:
    QString response_pattern;
    bool response_or_use;
    QString expand_pile;
};

class ZeroCardViewAsSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    explicit ZeroCardViewAsSkill(const QString &name);

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const override;
    const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const override;
    virtual const Card *viewAs(const Player *Self) const = 0;
};

class OneCardViewAsSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    explicit OneCardViewAsSkill(const QString &name);

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const override;
    const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const override;

    virtual bool viewFilter(const Card *to_select, const Player *Self) const;
    virtual const Card *viewAs(const Card *originalCard, const Player *Self) const = 0;

protected:
    QString filter_pattern;
};

class FilterSkill : public Skill
{
    Q_OBJECT

public:
    explicit FilterSkill(const QString &name);

    virtual bool viewFilter(const Card *to_select, const Player *Self) const = 0;
    virtual const Card *viewAs(const Card *originalCard, const Player *Self) const = 0;
};

namespace RefactorProposal {

typedef QSet<TriggerEvent> TriggerEvents;

class TriggerPrivate;

class Trigger // DO NOT INHERIT QObject since it is used in QObject-derived class
{
public:
    Trigger(const QString &name);
    virtual ~Trigger();
    QString name() const; // ... so we have to use a QString name() here

    TriggerEvents triggerEvents() const;
    bool canTrigger(TriggerEvent e) const;
    void addTriggerEvent(TriggerEvent e);
    void addTriggerEvents(TriggerEvents e);
    bool isGlobal() const;
    void setGlobal(bool global);

    // Let the trigger type to set its priority!
    // Record with event process (Fake move, etc): 10
    // ----- Separater: 5 -----
    // Regular skill: 3
    // Equip skill: 2
    // Skill which is meant to change rule: 1 (return true afterwards!)
    // Rule: 0
    // ----- Separater: -5 -----
    // Other priority is undefined for now
    virtual int priority() const = 0;

    // Should not trigger other events and affect other things in principle
    virtual void record(TriggerEvent event, ::Room *room, QVariant &data) const;
    virtual QList<TriggerDetail> triggerable(TriggerEvent event, const ::Room *room, QVariant &data) const = 0;

    // TODO: make TriggerDetail implicitly shared
    // But Even if it's implicitly shared, this should also be a pointer instead of a variable
    // Since it may potenally modifies its tag and/or target
    // What if we make the TriggerDetail const after its construction?
    virtual bool trigger(TriggerEvent event, ::Room *room, TriggerDetail detail, QVariant &data) const;

private:
    TriggerPrivate *d;
    Q_DISABLE_COPY_MOVE(Trigger)
};

class Rule : public Trigger
{
public:
    Rule(const QString &name);
    ~Rule() override;

    // fixed 0
    int priority() const final override;
    QList<TriggerDetail> triggerable(TriggerEvent, const ::Room *room, QVariant &) const final override;
};

class TriggerSkill : public Skill, public Trigger
{
    Q_OBJECT

public:
    TriggerSkill(const QString &name);

    // Removed functions:
    // const ViewAsSkill *getViewAsSkill(...) const;

    // 3 by default, optional override
    int priority() const override;

    // force subclass override this function
    // virtual QList<TriggerDetail> triggerable(TriggerEvent event, const Room *room, QVariant &data) const = 0;
    bool trigger(TriggerEvent event, ::Room *room, TriggerDetail detail, QVariant &data) const final override;
    virtual bool cost(TriggerEvent event, ::Room *room, TriggerDetail detail, QVariant &data) const;
    virtual bool effect(TriggerEvent event, ::Room *room, TriggerDetail detail, QVariant &data) const = 0;
};

class EquipSkill : public TriggerSkill
{
    Q_OBJECT

public:
    EquipSkill(const QString &name);

    // fixed 2
    int priority() const final override;
};

class GlobalRecord : public Trigger
{
public:
    GlobalRecord(const QString &name);

    // fixed 10
    int priority() const final override;

    // Since it may use only Record, override this function here
    // Optional override in subclass
    QList<TriggerDetail> triggerable(TriggerEvent event, const ::Room *room, QVariant &data) const override;
};
}

class TriggerSkill : public Skill
{
    Q_OBJECT

public:
    explicit TriggerSkill(const QString &name, Frequency frequency = NotFrequent);
    const ViewAsSkill *getViewAsSkill() const;
    QList<TriggerEvent> getTriggerEvents() const;

    virtual int getPriority() const;
    virtual void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const;

    virtual QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const;
    virtual bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const;
    virtual bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const;

    inline bool isGlobal() const
    {
        return global;
    }

protected:
    const ViewAsSkill *view_as_skill;
    QList<TriggerEvent> events;
    bool global;
};

class ProhibitSkill : public Skill
{
    Q_OBJECT

public:
    explicit ProhibitSkill(const QString &name);

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>(),
                              bool include_hidden = false) const = 0;
};

class DistanceSkill : public Skill
{
    Q_OBJECT

public:
    explicit DistanceSkill(const QString &name);

    virtual int getCorrect(const Player *from, const Player *to) const = 0;
    const ViewAsSkill *getViewAsSkill() const;

protected:
    const ViewAsSkill *view_as_skill;
};

class ShowDistanceSkill : public ZeroCardViewAsSkill
{
    Q_OBJECT

public:
    explicit ShowDistanceSkill(const QString &name);

    const Card *viewAs(const Player *Self) const override;
    bool isEnabledAtPlay(const Player *player) const override;
};

class MaxCardsSkill : public Skill
{
    Q_OBJECT

public:
    explicit MaxCardsSkill(const QString &name);

    virtual int getExtra(const Player *target) const;
    virtual int getFixed(const Player *target) const;
    const ViewAsSkill *getViewAsSkill() const;

protected:
    const ViewAsSkill *view_as_skill;
};

class TargetModSkill : public Skill
{
    Q_OBJECT

public:
    enum ModType
    {
        Residue,
        DistanceLimit,
        ExtraTarget
    };
    Q_ENUM(ModType)

    explicit TargetModSkill(const QString &name);
    virtual QString getPattern() const;

    virtual int getResidueNum(const Player *from, const Card *card) const;
    virtual int getDistanceLimit(const Player *from, const Card *card) const;
    virtual int getExtraTargetNum(const Player *from, const Card *card) const;

protected:
    QString pattern;
};

class AttackRangeSkill : public Skill
{
    Q_OBJECT

public:
    explicit AttackRangeSkill(const QString &name);

    virtual int getExtra(const Player *target, bool include_weapon) const;
    virtual int getFixed(const Player *target, bool include_weapon) const;
    const ViewAsSkill *getViewAsSkill() const;

protected:
    const ViewAsSkill *view_as_skill;
};

class SlashNoDistanceLimitSkill : public TargetModSkill
{
    Q_OBJECT

public:
    explicit SlashNoDistanceLimitSkill(const QString &skill_name);

    int getDistanceLimit(const Player *from, const Card *card) const override;

protected:
    QString name;
};

// a nasty way for 'fake moves', usually used in the process of multi-card chosen
class FakeMoveSkill : public TriggerSkill
{
    Q_OBJECT

public:
    explicit FakeMoveSkill(const QString &skillname);

    int getPriority() const override;
    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override;
    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override;

private:
    QString name;
};

class EquipSkill : public TriggerSkill
{
    Q_OBJECT

public:
    explicit EquipSkill(const QString &name);

    static bool equipAvailable(const Player *p, EquipCard::Location location, const QString &equip_name, const Player *to = nullptr);
    static bool equipAvailable(const Player *p, const Card *equip, const Player *to = nullptr);
};

class WeaponSkill : public EquipSkill
{
    Q_OBJECT

public:
    explicit WeaponSkill(const QString &name);
};

class ArmorSkill : public EquipSkill
{
    Q_OBJECT

public:
    explicit ArmorSkill(const QString &name);
};

class TreasureSkill : public EquipSkill
{
    Q_OBJECT

public:
    explicit TreasureSkill(const QString &name);
};

class ViewHasSkill : public Skill
{
    Q_OBJECT

public:
    explicit ViewHasSkill(const QString &name);

    virtual bool ViewHas(const Player *player, const QString &skill_name, const QString &flag, bool ignore_preshow = false) const = 0;
    inline bool isGlobal() const
    {
        return global;
    }

protected:
    bool global;
};

class BattleArraySkill : public TriggerSkill
{
    Q_OBJECT

public:
    BattleArraySkill(const QString &name, const QString arrayType);

    virtual void summonFriends(ServerPlayer *player) const;

    inline QString getArrayType() const
    {
        return array_type;
    }

private:
    QString array_type;
};

class ArraySummonSkill : public ZeroCardViewAsSkill
{
    Q_OBJECT

public:
    explicit ArraySummonSkill(const QString &name);

    const Card *viewAs(const Player *Self) const override;
    bool isEnabledAtPlay(const Player *player) const override;
};

#endif
