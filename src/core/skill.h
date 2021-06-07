#ifndef _SKILL_H
#define _SKILL_H

class Player;
class Card;
class RoomObject;

// TODO: KILL THIS
class Room;

#include "CardFace.h"
#include "structs.h"

#include <QDialog>
#include <QObject>
#include <QSet>

class SkillPrivate;

class Skill : public QObject
{
    Q_OBJECT

public:
    enum ArrayType
    {
        ArrayNone,
        ArrayFormation,
        ArraySiege
    };
    Q_ENUM(ArrayType)

    enum ShowType
    {
        ShowTrigger,
        ShowStatic,
        ShowViewAs,
    };
    Q_ENUM(ShowType)

    enum Category
    {
        SkillNoFlag = 0x0,

        SkillLord = 0x1,

        SkillCompulsory = 0x2,
        SkillEternalBit = 0x4,
        SkillEternal = SkillEternalBit | SkillCompulsory,

        SkillLimited = 0x8,
        SkillWake = SkillCompulsory | SkillLimited,

        SkillHead = 0x10,
        SkillDeputy = 0x20,
        SkillRelateToPlaceMask = SkillHead | SkillDeputy,

        SkillArrayFormation = 0x40,
        SkillArraySiege = 0x80,
        SkillArrayMask = SkillArrayFormation | SkillArraySiege,

        SkillAttached = 0x100,
        SkillHidden = 0x200,
    };
    Q_DECLARE_FLAGS(Categories, Category)

    // do not use even ANY symbols in skill name anymore!
    // use Flag-based ones
    explicit Skill(const QString &name, Skill::Categories skillCategories = SkillNoFlag, ShowType showType = ShowTrigger);
    ~Skill() override;

    // Category getters
    bool isLordSkill() const;
    bool isAttachedSkill() const;
    bool isCompulsory() const;
    bool isEternal() const;
    bool isLimited() const;
    inline bool isWake() const
    {
        return isLimited() && isCompulsory();
    }
    bool isHidden() const;
    inline bool isVisible() const
    {
        return !isHidden();
    }
    bool relateToPlace(bool head = true) const;
    ArrayType arrayType() const;

    // ShowType getter
    ShowType getShowType() const;

    // Other variable getters / setters
    const QString &limitMark() const;
    void setLimitMark(const QString &m);
    const QString &relatedMark() const;
    void setRelatedMark(const QString &m);
    const QString &relatedPile() const;
    void setRelatedPile(const QString &m);
    bool canPreshow() const;
    void setCanPreshow(bool c);
    bool isFrequent() const;
    void setFrequent(bool c);

    // For audio effect
    // Certain skill requires a logic for its audio effect
    // Currently it is judged by server side
    virtual int getAudioEffectIndex(const ServerPlayer *player, const Card *card) const;

    // current UI related, temporary left it alone
    // All these functions should belong to UI.
    // Maybe something like "Description provider" in UI will be ideal?
    Q_DECL_DEPRECATED QString getDescription() const;
    Q_DECL_DEPRECATED QString getNotice(int index) const;

private:
    SkillPrivate *d;
};

// Selection, should be returned by Client::currentViewAsSkillSelectionChain
struct ViewAsSkillSelection
{
    // Selectable when next is empty
    // Expandable when next is not empty, with a list of "next"
    QString name;
    QList<ViewAsSkillSelection *> next;

    ~ViewAsSkillSelection();
};

class ViewAsSkillPrivate;

class ViewAsSkill : public Skill
{
    Q_OBJECT

public:
    explicit ViewAsSkill(const QString &name);
    ~ViewAsSkill() override;

    // helper function
    bool isAvailable(const Player *invoker, CardUseStruct::CardUseReason reason, const QString &pattern) const;

    // property setters / getters
    QSanguosha::HandlingMethod handlingMethod() const;
    Q_ALWAYS_INLINE bool isResponseOrUse() const
    {
        return handlingMethod() == QSanguosha::MethodUse || handlingMethod() == QSanguosha::MethodResponse;
    }
    void setHandlingMethod(QSanguosha::HandlingMethod method);

    virtual QString expandPile(const Player *Self) const; // virtual for Huashen related skill. Intentionally return by value for easy override
    void setExpandPile(const QString &expand);

    const QString &responsePattern() const;
    void setResponsePattern(const QString &pattern);

    // logic related implementations
    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const = 0;
    virtual const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const = 0;

    virtual bool isEnabledAtPlay(const Player *player) const;
    virtual bool isEnabledAtResponse(const Player *player, CardUseStruct::CardUseReason reason, const QString &pattern) const;

    virtual const ViewAsSkillSelection *selections(const Player *Self) const;
    virtual bool isSelectionEnabled(const QStringList &name, const Player *Self) const;

private:
    ViewAsSkillPrivate *d;
};

class ZeroCardViewAsSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    explicit ZeroCardViewAsSkill(const QString &name);

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const final override;
    const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const final override;

    virtual const Card *viewAs(const Player *Self) const = 0;
};

class OneCardViewAsSkillPrivate;

class OneCardViewAsSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    explicit OneCardViewAsSkill(const QString &name);
    ~OneCardViewAsSkill() override;

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const final override;
    const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const final override;

    virtual bool viewFilter(const Card *to_select, const Player *Self) const;
    virtual const Card *viewAs(const Card *originalCard, const Player *Self) const = 0;

    void setFilterPattern(const QString &p);

private:
    OneCardViewAsSkillPrivate *d;
};

class FilterSkillPrivate;

class FilterSkill : public Skill
{
    Q_OBJECT

public:
    explicit FilterSkill(const QString &name);
    ~FilterSkill() override;

    virtual bool viewFilter(const Card *to_select, const Player *Self) const;
    virtual const Card *viewAs(const Card *originalCard, const Player *Self) const = 0;

    void setFilterPattern(const QString &p);

private:
    FilterSkillPrivate *d;
};

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
    void addTriggerEvents(const TriggerEvents &e);
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
    virtual void record(TriggerEvent event, ::Room *room, QVariant &data) const;
    virtual QList<TriggerDetail> triggerable(TriggerEvent event, const ::Room *room, const QVariant &data) const = 0;

    // TODO: make TriggerDetail implicitly shared
    virtual bool trigger(TriggerEvent event, ::Room *room, const TriggerDetail &detail, QVariant &data) const;

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
    QList<TriggerDetail> triggerable(TriggerEvent, const ::Room *room, const QVariant &) const final override;
};

class TriggerSkill
    : public Skill
    , public Trigger
{
    Q_OBJECT

public:
    TriggerSkill(const QString &name);

    // Removed functions:
    // const ViewAsSkill *getViewAsSkill(...) const;

    // 2 by default, optional override
    int priority() const override;

    // force subclass override this function
    // virtual QList<TriggerDetail> triggerable(TriggerEvent event, const Room *room, QVariant &data) const = 0;
    bool trigger(TriggerEvent event, ::Room *room, const TriggerDetail &detail, QVariant &data) const final override;

    // Limited modification to TriggerDetail, notably tag and target
    virtual bool cost(TriggerEvent event, ::Room *room, TriggerDetail &detail, QVariant &data) const;
    // No modification to TriggerDetail since the cost is done
    virtual bool effect(TriggerEvent event, ::Room *room, const TriggerDetail &detail, QVariant &data) const = 0;
};

class EquipSkill : public TriggerSkill
{
    Q_OBJECT

public:
    EquipSkill(const QString &name);

    Q_ALWAYS_INLINE bool isEquipSkill() const final override
    {
        return true;
    }

    static bool equipAvailable(const Player *p, QSanguosha::EquipLocation location, const QString &equip_name, const Player *to = nullptr);
    static bool equipAvailable(const Player *p, const Card *equip, const Player *to = nullptr);

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
    QList<TriggerDetail> triggerable(TriggerEvent event, const ::Room *room, const QVariant &data) const override;
};

// a nasty way for 'fake moves', usually used in the process of multi-card chosen
class FakeMoveRecordPrivate;
class FakeMoveRecord final : public GlobalRecord
{
public:
    FakeMoveRecord(const QString &skillName);
    ~FakeMoveRecord() final override;

    QList<TriggerDetail> triggerable(TriggerEvent event, const ::Room *room, const QVariant &data) const final override;
    bool trigger(TriggerEvent event, ::Room *room, const TriggerDetail &detail, QVariant &data) const final override;

private:
    FakeMoveRecordPrivate *d;
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
};

class MaxCardsSkill : public Skill
{
    Q_OBJECT

public:
    explicit MaxCardsSkill(const QString &name);

    virtual int getExtra(const Player *target) const;
    virtual int getFixed(const Player *target) const;
};

class TargetModSkill : public Skill
{
    Q_OBJECT

public:
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

class TreatAsEquippingSkill : public Skill
{
    Q_OBJECT

public:
    explicit TreatAsEquippingSkill(const QString &name);
    virtual bool treatAs(const Player *player, const QString equipName, QSanguosha::EquipLocation location) const = 0;
};

#endif
