#ifndef _SKILL_H
#define _SKILL_H

class Player;
class Card;
class ServerPlayer;
class QDialog;

#include "room.h"
#include "standard.h"
#include "structs.h"

#include <QDialog>
#include <QObject>

class Skill : public QObject
{
    Q_OBJECT
    Q_ENUMS(Frequency)

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

    explicit Skill(const QString &name, Frequency frequent = NotFrequent, const QString &showType = "trigger");
    bool isLordSkill() const;
    bool isAttachedLordSkill() const;
    virtual bool shouldBeVisible(const Player *Self) const; // usually for attached skill
    QString getDescription(bool yellow = true) const;
    QString getNotice(int index) const;
    bool isVisible() const;

    virtual int getEffectIndex(const ServerPlayer *player, const Card *card) const;
    virtual QDialog *getDialog() const;

    void initMediaSource();
    void playAudioEffect(int index = -1) const;
    Frequency getFrequency() const;
    QString getLimitMark() const;
    QStringList getSources() const;
    bool matchAvaliablePattern(QString avaliablePattern, QString askedPattern) const;
    QString getShowType() const;

protected:
    Frequency frequency;
    QString limit_mark;
    bool attached_lord_skill;
    QString show_type;

private:
    bool lord_skill;
    QStringList sources;
};

class ViewAsSkill : public Skill
{
    Q_OBJECT

public:
    ViewAsSkill(const QString &name);

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const = 0;
    virtual const Card *viewAs(const QList<const Card *> &cards) const = 0;

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
    inline QString getExpandPile() const
    {
        return expand_pile;
    }

protected:
    QString response_pattern;
    bool response_or_use;
    QString expand_pile;
};

class ZeroCardViewAsSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    ZeroCardViewAsSkill(const QString &name);

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const;
    virtual const Card *viewAs(const QList<const Card *> &cards) const;
    virtual const Card *viewAs() const = 0;
};

class OneCardViewAsSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    OneCardViewAsSkill(const QString &name);

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const;
    virtual const Card *viewAs(const QList<const Card *> &cards) const;

    virtual bool viewFilter(const Card *to_select) const;
    virtual const Card *viewAs(const Card *originalCard) const = 0;

protected:
    QString filter_pattern;
};

class FilterSkill : public OneCardViewAsSkill
{
    Q_OBJECT

public:
    FilterSkill(const QString &name);
};

class TriggerSkill : public Skill
{
    Q_OBJECT

public:
    TriggerSkill(const QString &name);
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

class Scenario;

class ScenarioRule : public TriggerSkill
{
    Q_OBJECT

public:
    ScenarioRule(Scenario *scenario);

    virtual int getPriority() const;
    virtual QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const;
};

class MasochismSkill : public TriggerSkill
{
    Q_OBJECT

public:
    MasochismSkill(const QString &name);

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const;
    virtual QList<SkillInvokeDetail> triggerable(const Room *room, const DamageStruct &damage) const;
    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const;
    virtual void onDamaged(Room *room, QSharedPointer<SkillInvokeDetail> invoke, const DamageStruct &damage) const = 0;
};

class PhaseChangeSkill : public TriggerSkill
{
    Q_OBJECT

public:
    PhaseChangeSkill(const QString &name);

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const;
    virtual bool onPhaseChange(ServerPlayer *target) const = 0;
};

class DrawCardsSkill : public TriggerSkill
{
    Q_OBJECT

public:
    DrawCardsSkill(const QString &name, bool = false);

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const;
    virtual int getDrawNum(const DrawNCardsStruct &draw) const = 0;

protected:
    bool is_initial;
};

class GameStartSkill : public TriggerSkill
{
    Q_OBJECT

public:
    GameStartSkill(const QString &name);

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const;
    virtual void onGameStart() const = 0;
};

class ProhibitSkill : public Skill
{
    Q_OBJECT

public:
    ProhibitSkill(const QString &name);

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>(),
                              bool include_hidden = false) const = 0;
};

class DistanceSkill : public Skill
{
    Q_OBJECT

public:
    DistanceSkill(const QString &name);

    virtual int getCorrect(const Player *from, const Player *to) const = 0;
};

class MaxCardsSkill : public Skill
{
    Q_OBJECT

public:
    MaxCardsSkill(const QString &name);

    virtual int getExtra(const Player *target) const;
    virtual int getFixed(const Player *target) const;
};

class TargetModSkill : public Skill
{
    Q_OBJECT
    Q_ENUMS(ModType)

public:
    enum ModType
    {
        Residue,
        DistanceLimit,
        ExtraTarget
    };

    TargetModSkill(const QString &name);
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
    AttackRangeSkill(const QString &name);

    virtual int getExtra(const Player *target, bool include_weapon) const;
    virtual int getFixed(const Player *target, bool include_weapon) const;
};

class SlashNoDistanceLimitSkill : public TargetModSkill
{
    Q_OBJECT

public:
    SlashNoDistanceLimitSkill(const QString &skill_name);

    virtual int getDistanceLimit(const Player *from, const Card *card) const;

protected:
    QString name;
};

// a nasty way for 'fake moves', usually used in the process of multi-card chosen
class FakeMoveSkill : public TriggerSkill
{
    Q_OBJECT

public:
    FakeMoveSkill(const QString &skillname);

    int getPriority() const;
    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const;
    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const;

private:
    QString name;
};

class EquipSkill : public TriggerSkill
{
    Q_OBJECT

public:
    EquipSkill(const QString &name);

    static bool equipAvailable(const Player *p, EquipCard::Location location, const QString &equip_name, const Player *to = NULL);
    static bool equipAvailable(const Player *p, const EquipCard *card, const Player *to = NULL);
};

class WeaponSkill : public EquipSkill
{
    Q_OBJECT

public:
    WeaponSkill(const QString &name);
};

class ArmorSkill : public EquipSkill
{
    Q_OBJECT

public:
    ArmorSkill(const QString &name);
};

class TreasureSkill : public EquipSkill
{
    Q_OBJECT

public:
    TreasureSkill(const QString &name);
};
#endif
