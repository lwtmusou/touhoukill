#ifndef TOUHOUKILL_TRIGGER_H
#define TOUHOUKILL_TRIGGER_H

// BE WARE! THIS FILE IS USED IN BOTH SWIG AND C++.
// MAKE SURE THE GRAMMAR IS COMPATIBLE BETWEEN 2 LANGUAGES.

#ifndef SWIG
#include "global.h"

#include <QList>
#include <QString>
#include <QVariant>

class Card;
class Player;
class RoomObject;
class TriggerDetail;
class Skill;
class EquipCard;

class TriggerPrivate;
#endif

class QSGS_CORE_EXPORT Trigger
{
public:
#ifndef SWIG
    Trigger(const QString &name);
#endif
    virtual ~Trigger();

    QString name() const;

    QSanguosha::TriggerEvents triggerEvents() const;
    bool canTrigger(QSanguosha::TriggerEvent e) const;
    void addTriggerEvent(QSanguosha::TriggerEvent e);
    void addTriggerEvents(const QSanguosha::TriggerEvents &e);
    bool isGlobal() const;
    void setGlobal(bool global);

    virtual Q_ALWAYS_INLINE bool isEquipSkill() const
    {
        return false;
    }

    // Let the trigger type to set its priority!
    // Triggers whose priority is not in range {-5,5} are considered to be record with event process

    // Record with event process (Fake move, etc): 10
    // ----- Separator: 5 -----
    // Regular skill: 2
    // Equip skill: 2 (with isEquipSkill set to True)
    // Triggers which is meant to change rule: 1 (return true afterwards!)
    // Game Rule: 0
    // Scenario specific rule: -1
    // ----- Separator: -5 -----
    // Other priority is undefined for now
    // Note that a minus priority is processed after game rule.
    // It may not be what you think since game rule does a lot of things which may not be explained by its name
    // E.g., Minus priority of event TurnStart triggers after turn ends since game rule process the whole turn during its trigger with priority 0.
    virtual int priority() const = 0;

    // Should not trigger other events and affect other things in principle
    virtual void record(QSanguosha::TriggerEvent event, RoomObject *room, QVariant &data) const;

    // TODO: make RoomObject const:
    // Current implementation is: To create a TriggerDetail in this function. All data saved in TriggerDetail is non-const
    // This makes the RoomObject not able to be const even if it should be.
    // EXACTLY STRICTLY NOTHING should be even TOUCHED in this function
    virtual QList<TriggerDetail> triggerable(QSanguosha::TriggerEvent event, RoomObject *room, const QVariant &data) const;
    virtual bool trigger(QSanguosha::TriggerEvent event, RoomObject *room, const TriggerDetail &detail, QVariant &data) const;

private:
    Trigger() = delete;
    Q_DISABLE_COPY_MOVE(Trigger)
    TriggerPrivate *const d;
};

class QSGS_CORE_EXPORT Rule : public Trigger
{
public:
#ifndef SWIG
    Rule(const QString &name);
#endif
    ~Rule() override = default;

    // fixed 0
#ifndef SWIG
    int priority() const final override;
    QList<TriggerDetail> triggerable(QSanguosha::TriggerEvent /*event*/, RoomObject *room, const QVariant & /*data*/) const final override;
#endif
};

class SkillTriggerPrivate;

class QSGS_CORE_EXPORT SkillTrigger : public Trigger
{
public:
#ifndef SWIG
    explicit SkillTrigger(Skill *skill);
    explicit SkillTrigger(const QString &name);
#endif
    ~SkillTrigger() override;

    const QString &skillName() const;

#ifndef SWIG
    // 2 by default, optional override
    int priority() const override;

    // force subclass override this function
    // virtual QList<TriggerDetail> triggerable(TriggerEvent event, RoomObject *room, const QVariant &data) const = 0;

    bool trigger(QSanguosha::TriggerEvent event, RoomObject *room, const TriggerDetail &detail, QVariant &data) const final override;
#endif
    // Limited modification to TriggerDetail, notably tag and target
    virtual bool cost(QSanguosha::TriggerEvent event, RoomObject *room, TriggerDetail &detail, QVariant &data) const;
    // No modification to TriggerDetail since the cost is done
    virtual bool effect(QSanguosha::TriggerEvent event, RoomObject *room, const TriggerDetail &detail, QVariant &data) const;

private:
    SkillTriggerPrivate *const d;
};

class QSGS_CORE_EXPORT EquipSkillTrigger : public SkillTrigger
{
public:
#ifndef SWIG
    explicit EquipSkillTrigger(const EquipCard *card);
    explicit EquipSkillTrigger(const QString &name);
#endif
    ~EquipSkillTrigger() override = default;

#ifndef SWIG
    Q_ALWAYS_INLINE bool isEquipSkill() const final override
    {
        return true;
    }
#endif

    static bool equipAvailable(const Player *p, QSanguosha::EquipLocation location, const QString &equip_name, const Player *to = nullptr);
    static bool equipAvailable(const Player *p, const Card *equip, const Player *to = nullptr);

#ifndef SWIG
    // fixed same as regular Skill trigger
    int priority() const final override;
#endif
};

class QSGS_CORE_EXPORT GlobalRecord : public Trigger
{
public:
#ifndef SWIG
    GlobalRecord(const QString &name);
#endif
    ~GlobalRecord() override = default;

#ifndef SWIG
    // fixed 10
    int priority() const final override;

    // Since it may use only Record, override this function here
    // Optional override in subclass
    QList<TriggerDetail> triggerable(QSanguosha::TriggerEvent event, RoomObject *room, const QVariant &data) const override;
#endif
};

// a nasty way for 'fake moves', usually used in the process of multi-card chosen
class FakeMoveRecordPrivate;
class QSGS_CORE_EXPORT FakeMoveRecord final : public GlobalRecord
{
public:
#ifndef SWIG
    explicit FakeMoveRecord(const QString &name, const QString &skillName = QString());
#endif
    ~FakeMoveRecord() final override;

#ifndef SWIG
    QList<TriggerDetail> triggerable(QSanguosha::TriggerEvent event, RoomObject *room, const QVariant &data) const final override;
    bool trigger(QSanguosha::TriggerEvent event, RoomObject *room, const TriggerDetail &detail, QVariant &data) const final override;
#endif

private:
    FakeMoveRecordPrivate *const d;
};

#endif