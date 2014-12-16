#ifndef __3D__H__
#define __3D__H__

#include "package.h"
#include "card.h"
#include "player.h"

class SanD1Package: public Package{
    Q_OBJECT

public:
    SanD1Package();
};

class SanD1XinveCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE SanD1XinveCard();
    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class SanD1BianzhenCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE SanD1BianzhenCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;

private:
    static Player::Phase getPhaseFromString(const QString &s);
};

class SanD1KuangxiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE SanD1KuangxiCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class SanD1ShenzhiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE SanD1ShenzhiCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class SanD1DoudanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE SanD1DoudanCard();
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

#endif