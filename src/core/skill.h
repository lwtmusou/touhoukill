#ifndef TOUHOUKILL_SKILL_H
#define TOUHOUKILL_SKILL_H

#include "global.h"
#include "qsgscore.h"

#include <QList>
#include <QObject>
#include <QSet>
#include <QString>

class Player;
class Card;
class RoomObject;
class Trigger;

class SkillPrivate;

class QSGS_CORE_EXPORT Skill : public QObject
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
    };
    Q_DECLARE_FLAGS(Categories, Category)

    // do not use even ANY symbols in skill name anymore!
    // use Flag-based ones
    explicit Skill(const QString &name, Categories skillCategories = SkillNoFlag, ShowType showType = ShowTrigger);
    ~Skill() override;

    // Category getters
    bool isLordSkill() const;
    bool isAttachedSkill() const;
    bool isCompulsory() const;
    bool isEternal() const;
    bool isLimited() const;
    Q_ALWAYS_INLINE bool isWake() const
    {
        return isLimited() && isCompulsory();
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

    // Triggers
    // I decided to make Trigger apart from Skill.
    // This makes Trigger lightweight, and unifies all the process of all skills.
    void addTrigger(const Trigger *trigger);
    const QSet<const Trigger *> &triggers() const;

    // Hidden skills related
    // Attention! since the affiliated skill should be modified for setting its main skill, this parameter is not const.
    void addToAffiliatedSkill(Skill *skill);
    const QSet<const Skill *> &affiliatedSkills() const;
    bool isAffiliatedSkill() const;
    const Skill *mainSkill() const;

    // For audio effect
    // Certain skill requires a logic for its audio effect
    // Currently it is judged by server side
    // return value:
    // -2 and less are for other use, maybe some special condition need this
    // -1 for random audio effect
    // 0 for no audio effect
    // > 0 for specific audio effect number
    virtual int getAudioEffectIndex(const Player *player, const Card *card) const;

    // deprecated
    Q_DECL_DEPRECATED Q_ALWAYS_INLINE bool isHidden() const
    {
        return isAffiliatedSkill();
    }
    Q_DECL_DEPRECATED Q_ALWAYS_INLINE bool isVisible() const
    {
        return !isAffiliatedSkill();
    }

private:
    Skill() = delete;
    SkillPrivate *const d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Skill::Categories)

// Selection
struct QSGS_CORE_EXPORT ViewAsSkillSelection
{
public:
    // Selectable when next is empty
    // Expandable when next is not empty, with a list of "next"
    QString name;
    QList<ViewAsSkillSelection *> next;

    ~ViewAsSkillSelection();

private:
    Q_DISABLE_COPY_MOVE(ViewAsSkillSelection)
};

class ViewAsSkillPrivate;

class QSGS_CORE_EXPORT ViewAsSkill : public Skill
{
    Q_OBJECT

public:
    explicit ViewAsSkill(const QString &name);
    ~ViewAsSkill() override;

    // helper function
    bool isAvailable(const Player *invoker, QSanguosha::CardUseReason reason, const QString &pattern) const;

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

    // This implementation needs overrides to call Self::currentViewAsSkillSelectionChain in viewFilter and viewAs. See example of Guhuo in skill.cpp
#if 0
    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const = 0;
    virtual const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const = 0;
#endif
    // A better choice seems to be just put the 2 parameters in this function
    // since almost all skills will be implemented in Lua, which don't need to write the extra parameter if not needed
    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self, const QStringList &CurrentViewAsSkillChain) const = 0;
    virtual const Card *viewAs(const QList<const Card *> &cards, const Player *Self, const QStringList &CurrentViewAsSkillChain) const = 0;

    virtual bool isEnabledAtPlay(const Player *player) const;
    virtual bool isEnabledAtResponse(const Player *player, QSanguosha::CardUseReason reason, const QString &pattern) const;

    virtual const ViewAsSkillSelection *selections(const Player *Self) const;
    virtual bool isSelectionEnabled(const QStringList &name, const Player *Self) const;

private:
    ViewAsSkillPrivate *const d;
};

class QSGS_CORE_EXPORT ZeroCardViewAsSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    explicit ZeroCardViewAsSkill(const QString &name);

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self, const QStringList &CurrentViewAsSkillChain) const final override;
    const Card *viewAs(const QList<const Card *> &cards, const Player *Self, const QStringList &CurrentViewAsSkillChain) const final override;

    virtual const Card *viewAs(const Player *Self, const QStringList &CurrentViewAsSkillChain) const = 0;
};

class OneCardViewAsSkillPrivate;

class QSGS_CORE_EXPORT OneCardViewAsSkill : public ViewAsSkill
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

class QSGS_CORE_EXPORT FilterSkill : public Skill
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

class QSGS_CORE_EXPORT ProhibitSkill : public Skill
{
    Q_OBJECT

public:
    explicit ProhibitSkill(const QString &name);

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>(),
                              bool include_hidden = false) const = 0;
};

class QSGS_CORE_EXPORT DistanceSkill : public Skill
{
    Q_OBJECT

public:
    explicit DistanceSkill(const QString &name);

    virtual int getCorrect(const Player *from, const Player *to) const = 0;
};

class QSGS_CORE_EXPORT MaxCardsSkill : public Skill
{
    Q_OBJECT

public:
    explicit MaxCardsSkill(const QString &name);

    virtual int getExtra(const Player *target) const;
    virtual int getFixed(const Player *target) const;
};

class QSGS_CORE_EXPORT TargetModSkill : public Skill
{
    Q_OBJECT

public:
    explicit TargetModSkill(const QString &name);
    virtual QString getPattern() const;

    virtual int getResidueNum(const Player *from, const Card *card) const;
    virtual int getDistanceLimit(const Player *from, const Card *card) const;
    virtual int getExtraTargetNum(const Player *from, const Card *card) const;

    // TODO: consider rated (or limited?) targets or extra targets, see GitHub issue #13
    // I prefer solution 1, i.e., do not distinguish rated and extra targets, but with targetFilter / targetsFeasible function added to this skill, but easy to cause trouble which is hard to resolve

protected:
    QString pattern;
};

class QSGS_CORE_EXPORT AttackRangeSkill : public Skill
{
    Q_OBJECT

public:
    explicit AttackRangeSkill(const QString &name);

    virtual int getExtra(const Player *target, bool include_weapon) const;
    virtual int getFixed(const Player *target, bool include_weapon) const;
};

class QSGS_CORE_EXPORT SlashNoDistanceLimitSkill : public TargetModSkill
{
    Q_OBJECT

public:
    explicit SlashNoDistanceLimitSkill(const QString &skill_name);

    int getDistanceLimit(const Player *from, const Card *card) const override;

protected:
    QString name;
};

class QSGS_CORE_EXPORT TreatAsEquippingSkill : public Skill
{
    Q_OBJECT

public:
    explicit TreatAsEquippingSkill(const QString &name);
    virtual bool treatAs(const Player *player, QString equipName, QSanguosha::EquipLocation location) const = 0;
};

#endif
