#ifndef _HEGEMONY_H
#define _HEGEMONY_H

#include "standard.h"
#include "skill.h"

class HegemonyPackage: public Package {
    Q_OBJECT

public:
    HegemonyPackage();
};

class FenxunCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE FenxunCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ShuangrenCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ShuangrenCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class XiongyiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE XiongyiCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;

    virtual int getDrawNum() const;
};

class QingchengCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE QingchengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class Sijian: public TriggerSkill{
public:
    Sijian();
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const;
    virtual QString getCardChosenFlag() const;
};

#endif

