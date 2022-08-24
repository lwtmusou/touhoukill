#ifndef _LUA_WRAPPER_H
#define _LUA_WRAPPER_H

#include "skill.h"
#include "standard.h"
#include "util.h"

typedef int LuaFunction;

class LuaTriggerSkill : public TriggerSkill
{
    Q_OBJECT

public:
    LuaTriggerSkill(const char *name, Frequency frequency, const char *limit_mark);
    inline void addEvent(TriggerEvent triggerEvent)
    {
        events << triggerEvent;
    }
    inline void setViewAsSkill(ViewAsSkill *view_as_skill)
    {
        this->view_as_skill = view_as_skill;
    }
    inline void setGlobal(bool global)
    {
        this->global = global;
    }

    inline int getPriority() const override
    {
        return priority;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override;

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override;
    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override;
    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override;

    LuaFunction on_record;
    LuaFunction can_trigger;
    LuaFunction on_cost;
    LuaFunction on_effect;

    int priority;
};

class LuaProhibitSkill : public ProhibitSkill
{
    Q_OBJECT

public:
    explicit LuaProhibitSkill(const char *name);

    bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>(),
                      bool include_hidden = false) const override;

    LuaFunction is_prohibited;
};

class LuaViewAsSkill : public ViewAsSkill
{
    Q_OBJECT
    Q_ENUMS(GuhuoDialogType)

public:
    enum GuhuoDialogType
    {
        NoDialog = 0,
        LeftOnlyDialog = 1,
        RightOnlyDialog = 2,
        LeftRightDialog = 3
    };

    explicit LuaViewAsSkill(const char *name, const char *response_pattern = "");

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override;
    const Card *viewAs(const QList<const Card *> &cards) const override;

    bool shouldBeVisible(const Player *player) const override;

    void pushSelf(lua_State *L) const;

    LuaFunction view_filter;
    LuaFunction view_as;

    LuaFunction should_be_visible;

    LuaFunction enabled_at_play;
    LuaFunction enabled_at_response;
    LuaFunction enabled_at_nullification;

    bool isEnabledAtPlay(const Player *player) const override;
    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override;
    bool isEnabledAtNullification(const ServerPlayer *player) const override;

    QDialog *getDialog() const override;

    inline void setGuhuoDialogType(const GuhuoDialogType t)
    {
        guhuo_dialog_type = t;
    }

private:
    GuhuoDialogType guhuo_dialog_type; //left, right, left+right
};

class LuaFilterSkill : public FilterSkill
{
    Q_OBJECT

public:
    explicit LuaFilterSkill(const char *name);

    bool viewFilter(const Card *to_select) const override;
    const Card *viewAs(const Card *originalCard) const override;

    LuaFunction view_filter;
    LuaFunction view_as;
};

class LuaDistanceSkill : public DistanceSkill
{
    Q_OBJECT

public:
    explicit LuaDistanceSkill(const char *name);

    int getCorrect(const Player *from, const Player *to) const override;

    LuaFunction correct_func;
};

class LuaMaxCardsSkill : public MaxCardsSkill
{
    Q_OBJECT

public:
    explicit LuaMaxCardsSkill(const char *name);

    int getExtra(const Player *target) const override;
    int getFixed(const Player *target) const override;

    LuaFunction extra_func;
    LuaFunction fixed_func;
};

class LuaTargetModSkill : public TargetModSkill
{
    Q_OBJECT

public:
    LuaTargetModSkill(const char *name, const char *pattern);

    int getResidueNum(const Player *from, const Card *card) const override;
    int getDistanceLimit(const Player *from, const Card *card) const override;
    int getExtraTargetNum(const Player *from, const Card *card) const override;

    LuaFunction residue_func;
    LuaFunction distance_limit_func;
    LuaFunction extra_target_func;
};

class LuaAttackRangeSkill : public AttackRangeSkill
{
    Q_OBJECT

public:
    explicit LuaAttackRangeSkill(const char *name);

    int getExtra(const Player *target, bool include_weapon) const override;
    int getFixed(const Player *target, bool include_weapon) const override;

    LuaFunction extra_func;
    LuaFunction fixed_func;
};

class LuaSkillCard : public SkillCard
{
    Q_OBJECT

public:
    LuaSkillCard(const char *name, const char *skillName);
    LuaSkillCard *clone() const;
    inline void setTargetFixed(bool target_fixed)
    {
        this->target_fixed = target_fixed;
    }
    inline void setWillThrow(bool will_throw)
    {
        this->will_throw = will_throw;
    }
    inline void setCanRecast(bool can_recast)
    {
        this->can_recast = can_recast;
    }
    inline void setHandlingMethod(Card::HandlingMethod handling_method)
    {
        this->handling_method = handling_method;
    }
    inline void setMute(bool mute)
    {
        this->mute = mute;
    }

    // member functions that do not expose to Lua interpreter
    static LuaSkillCard *Parse(const QString &str);
    void pushSelf(lua_State *L) const;

    QString toString(bool hidden = false) const override;

    // these functions are defined at swig/luaskills.i
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
    const Card *validate(CardUseStruct &cardUse) const override;
    const Card *validateInResponse(ServerPlayer *user) const override;

    // the lua callbacks
    LuaFunction filter;
    LuaFunction feasible;
    LuaFunction about_to_use;
    LuaFunction on_use;
    LuaFunction on_effect;
    LuaFunction on_validate;
    LuaFunction on_validate_in_response;
};

class LuaBasicCard : public BasicCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LuaBasicCard(Card::Suit suit, int number, const char *obj_name, const char *class_name, const char *subtype);
    LuaBasicCard *clone(Card::Suit suit = Card::SuitToBeDecided, int number = -1) const;
    inline void setTargetFixed(bool target_fixed)
    {
        this->target_fixed = target_fixed;
    }
    inline void setCanRecast(bool can_recast)
    {
        this->can_recast = can_recast;
    }

    // member functions that do not expose to Lua interpreter
    void pushSelf(lua_State *L) const;

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;

    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool isAvailable(const Player *player) const override;

    inline QString getClassName() const override
    {
        return QString(class_name);
    }
    inline QString getSubtype() const override
    {
        return QString(subtype);
    }
    inline bool isKindOf(const char *cardType) const override
    {
        if (strcmp(cardType, "LuaCard") == 0 || QString(cardType) == class_name)
            return true;
        else
            return Card::isKindOf(cardType);
    }

    // the lua callbacks
    LuaFunction filter;
    LuaFunction feasible;
    LuaFunction available;
    LuaFunction about_to_use;
    LuaFunction on_use;
    LuaFunction on_effect;

private:
    QString class_name, subtype;
};

class LuaTrickCard : public TrickCard
{
    Q_OBJECT

public:
    enum SubClass
    {
        TypeNormal,
        TypeSingleTargetTrick,
        TypeDelayedTrick,
        TypeAOE,
        TypeGlobalEffect
    };

    Q_INVOKABLE LuaTrickCard(Card::Suit suit, int number, const char *obj_name, const char *class_name, const char *subtype);
    LuaTrickCard *clone(Card::Suit suit = Card::SuitToBeDecided, int number = -1) const;
    inline void setTargetFixed(bool target_fixed)
    {
        this->target_fixed = target_fixed;
    }
    inline void setCanRecast(bool can_recast)
    {
        this->can_recast = can_recast;
    }

    // member functions that do not expose to Lua interpreter
    void pushSelf(lua_State *L) const;

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
    void onNullified(ServerPlayer *target) const override;
    bool isCancelable(const CardEffectStruct &effect) const override;

    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool isAvailable(const Player *player) const override;

    inline QString getClassName() const override
    {
        return class_name;
    }
    inline void setSubtype(const char *subtype)
    {
        this->subtype = subtype;
    }
    inline QString getSubtype() const override
    {
        return subtype;
    }
    inline void setSubClass(SubClass subclass)
    {
        this->subclass = subclass;
    }
    inline SubClass getSubClass() const
    {
        return subclass;
    }
    inline bool isKindOf(const char *cardType) const override
    {
        if (strcmp(cardType, "LuaCard") == 0 || QString(cardType) == class_name)
            return true;
        else {
            if (Card::isKindOf(cardType))
                return true;
            switch (subclass) {
            case TypeSingleTargetTrick:
                return strcmp(cardType, "SingleTargetTrick") == 0;
                break;
            case TypeDelayedTrick:
                return strcmp(cardType, "DelayedTrick") == 0;
                break;
            case TypeAOE:
                return strcmp(cardType, "AOE") == 0;
                break;
            case TypeGlobalEffect:
                return strcmp(cardType, "GlobalEffect") == 0;
                break;
            case TypeNormal:
            default:
                return false;
                break;
            }
        }
    }

    // the lua callbacks
    LuaFunction filter;
    LuaFunction feasible;
    LuaFunction available;
    LuaFunction is_cancelable;
    LuaFunction about_to_use;
    LuaFunction on_use;
    LuaFunction on_effect;
    LuaFunction on_nullified;

private:
    SubClass subclass;
    QString class_name, subtype;
};

class LuaWeapon : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE LuaWeapon(Card::Suit suit, int number, int range, const char *obj_name, const char *class_name);
    LuaWeapon *clone(Card::Suit suit = Card::SuitToBeDecided, int number = -1) const;

    // member functions that do not expose to Lua interpreter
    void pushSelf(lua_State *L) const;

    void onInstall(ServerPlayer *player) const override;
    void onUninstall(ServerPlayer *player) const override;

    inline QString getClassName() const override
    {
        return class_name;
    }
    inline bool isKindOf(const char *cardType) const override
    {
        if (strcmp(cardType, "LuaCard") == 0 || QString(cardType) == class_name)
            return true;
        else
            return Card::isKindOf(cardType);
    }

    // the lua callbacks
    LuaFunction on_install;
    LuaFunction on_uninstall;

private:
    QString class_name;
};

class LuaArmor : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE LuaArmor(Card::Suit suit, int number, const char *obj_name, const char *class_name);
    LuaArmor *clone(Card::Suit suit = Card::SuitToBeDecided, int number = -1) const;

    // member functions that do not expose to Lua interpreter
    void pushSelf(lua_State *L) const;

    void onInstall(ServerPlayer *player) const override;
    void onUninstall(ServerPlayer *player) const override;

    inline QString getClassName() const override
    {
        return class_name;
    }
    inline bool isKindOf(const char *cardType) const override
    {
        if (strcmp(cardType, "LuaCard") == 0 || QString(cardType) == class_name)
            return true;
        else
            return Card::isKindOf(cardType);
    }

    // the lua callbacks
    LuaFunction on_install;
    LuaFunction on_uninstall;

private:
    QString class_name;
};

class LuaTreasure : public Treasure
{
    Q_OBJECT

public:
    Q_INVOKABLE LuaTreasure(Card::Suit suit, int number, const char *obj_name, const char *class_name);
    LuaTreasure *clone(Card::Suit suit = Card::SuitToBeDecided, int number = -1) const;

    // member functions that do not expose to Lua interpreter
    void pushSelf(lua_State *L) const;

    void onInstall(ServerPlayer *player) const override;
    void onUninstall(ServerPlayer *player) const override;

    inline QString getClassName() const override
    {
        return class_name;
    }
    inline bool isKindOf(const char *cardType) const override
    {
        if (strcmp(cardType, "LuaCard") == 0 || QString(cardType) == class_name)
            return true;
        else
            return Card::isKindOf(cardType);
    }

    // the lua callbacks
    LuaFunction on_install;
    LuaFunction on_uninstall;

private:
    QString class_name;
};

#endif
