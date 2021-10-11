#ifndef _SKILL_H
#define _SKILL_H

#include "global.h"

// TODO: kill these
#include "CardFace.h"
#include "structs.h"

#include <QDialog>
#include <QObject>
#include <QSet>

class Player;
class Card;
class RoomObject;

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
    virtual int getAudioEffectIndex(const Player *player, const Card *card) const;

    // current UI related, temporary left it alone
    // All these functions should belong to UI.
    // Maybe something like "Description provider" in UI will be ideal?
    Q_DECL_DEPRECATED QString getDescription() const;
    Q_DECL_DEPRECATED QString getNotice(int index) const;

private:
    SkillPrivate *const d;
};

// Selection
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
    // In fact, the best design is to put "CurrentViewAsSkillSelectionChain" as parameter of this function.
    // By doing so makes this function doesn't depends on other things, such as Player, which seems off-topic.
    // Since most skill doesn't need the parameter, I don't like every skill introduce this extra parameter
    // There is a way for implementing this design, which is to make the function non-pure-virtual on both chain and non-chain variants like following.
    // But I don't like this method which makes every ViewAsSkill able to make instance even if no function is overrided.
#if 0
    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const;
    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self, const QStringList &CurrentViewAsSkillChain) const;
    virtual const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const;
    virtual const Card *viewAs(const QList<const Card *> &cards, const Player *Self, const QStringList &CurrentViewAsSkillChain) const;
#endif

    // Another way is to use vararg, just like what posix open() does. Put the optional const QStringList * in vararg and use it when it is needed like following.
#if 0
    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self, ...) const = 0;
    virtual const Card *viewAs(const QList<const Card *> &cards, const Player *Self, ...) const = 0;
#define VIEWASSKILL_RETRACT_CURRENT_SELECTION_CHAIN(chain)                                                                                                 \
    va_list VIEWASSKILL_RETRACT_CURRENT_SELECTION_CHAIN_list;                                                                                              \
    va_start(VIEWASSKILL_RETRACT_CURRENT_SELECTION_CHAIN_list, Self);                                                                                      \
    const QStringList &chain = *(static_cast<const QStringList *>(va_arg(VIEWASSKILL_RETRACT_CURRENT_SELECTION_CHAIN_list, sizeof(const QStringList *)))); \
    va_end(VIEWASSKILL_RETRACT_CURRENT_SELECTION_CHAIN_list);

    bool SomeSkill::viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self, ...) const override
    {
        VIEWASSKILL_RETRACT_CURRENT_SELECTION_CHAIN(CurrentViewAsSkillChain)

        QString currentSelected = CurrentViewAsSkillChain.last();
        // do anything you like
    }
#endif
    // Current implementation needs overrides to call Self::currentViewAsSkillSelectionChain in viewFilter and viewAs. See example of Guhuo in skill.cpp
#if 0
    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const = 0;
    virtual const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const = 0;
#endif
    // A better choice seems to be just put the 2 parameters in this function
    // since almost all skills will be implemented in Lua, which don't need to write the extra parameter if not needed
    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self, const QStringList &CurrentViewAsSkillChain) const = 0;
    virtual const Card *viewAs(const QList<const Card *> &cards, const Player *Self, const QStringList &CurrentViewAsSkillChain) const = 0;

    virtual bool isEnabledAtPlay(const Player *player) const;
    virtual bool isEnabledAtResponse(const Player *player, CardUseStruct::CardUseReason reason, const QString &pattern) const;

    virtual const ViewAsSkillSelection *selections(const Player *Self) const;
    virtual bool isSelectionEnabled(const QStringList &name, const Player *Self) const;

private:
    ViewAsSkillPrivate *const d;
};

class ZeroCardViewAsSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    explicit ZeroCardViewAsSkill(const QString &name);

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self, const QStringList &CurrentViewAsSkillChain) const final override;
    const Card *viewAs(const QList<const Card *> &cards, const Player *Self, const QStringList &CurrentViewAsSkillChain) const final override;

    virtual const Card *viewAs(const Player *Self, const QStringList &CurrentViewAsSkillChain) const = 0;
};

class OneCardViewAsSkillPrivate;

class OneCardViewAsSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    explicit OneCardViewAsSkill(const QString &name);
    ~OneCardViewAsSkill() override;

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self, const QStringList &CurrentViewAsSkillChain) const final override;
    const Card *viewAs(const QList<const Card *> &cards, const Player *Self, const QStringList &CurrentViewAsSkillChain) const final override;

    virtual bool viewFilter(const Card *to_select, const Player *Self, const QStringList &CurrentViewAsSkillChain) const;
    virtual const Card *viewAs(const Card *originalCard, const Player *Self, const QStringList &CurrentViewAsSkillChain) const = 0;

    void setFilterPattern(const QString &p);

private:
    OneCardViewAsSkillPrivate *const d;
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
    FilterSkillPrivate *const d;
};

class TriggerPrivate;

class Trigger // DO NOT INHERIT QObject since it is used in QObject-derived class
{
public:
    Trigger(const QString &name);
    virtual ~Trigger();
    QString name() const; // ... so we have to use a QString name() here

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
    virtual QList<TriggerDetail> triggerable(QSanguosha::TriggerEvent event, RoomObject *room, const QVariant &data) const = 0;

    // TODO: make TriggerDetail implicitly shared
    virtual bool trigger(QSanguosha::TriggerEvent event, RoomObject *room, const TriggerDetail &detail, QVariant &data) const;

private:
    Q_DISABLE_COPY_MOVE(Trigger)
    TriggerPrivate *const d;
};

class Rule : public Trigger
{
public:
    Rule(const QString &name);
    ~Rule() override = default;

    // fixed 0
    int priority() const final override;
    QList<TriggerDetail> triggerable(QSanguosha::TriggerEvent, RoomObject *room, const QVariant &) const final override;
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
    // virtual QList<TriggerDetail> triggerable(TriggerEvent event, RoomObject *room, const QVariant &data) const = 0;

    bool trigger(QSanguosha::TriggerEvent event, RoomObject *room, const TriggerDetail &detail, QVariant &data) const final override;

    // Limited modification to TriggerDetail, notably tag and target
    virtual bool cost(QSanguosha::TriggerEvent event, RoomObject *room, TriggerDetail &detail, QVariant &data) const;
    // No modification to TriggerDetail since the cost is done
    virtual bool effect(QSanguosha::TriggerEvent event, RoomObject *room, const TriggerDetail &detail, QVariant &data) const = 0;
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
    QList<TriggerDetail> triggerable(QSanguosha::TriggerEvent event, RoomObject *room, const QVariant &data) const override;
};

// a nasty way for 'fake moves', usually used in the process of multi-card chosen
class FakeMoveRecordPrivate;
class FakeMoveRecord final : public GlobalRecord
{
public:
    FakeMoveRecord(const QString &skillName);
    ~FakeMoveRecord() final override;

    QList<TriggerDetail> triggerable(QSanguosha::TriggerEvent event, RoomObject *room, const QVariant &data) const final override;
    bool trigger(QSanguosha::TriggerEvent event, RoomObject *room, const TriggerDetail &detail, QVariant &data) const final override;

private:
    FakeMoveRecordPrivate *const d;
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
