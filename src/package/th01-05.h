#ifndef TH0105_H
#define TH0105_H

#include "package.h"
#include "card.h"

class ShiquCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShiquCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};


class LianmuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LianmuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
};

class SqChuangshiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SqChuangshiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class ModianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ModianCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TH0105Package : public Package
{
    Q_OBJECT

public:
    TH0105Package();
};

#endif
