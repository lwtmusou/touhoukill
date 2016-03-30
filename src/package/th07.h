#ifndef _th07_H
#define _th07_H

#include "package.h"
#include "card.h"

class MocaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MocaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class YujianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YujianCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class HuayinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuayinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};


class TH07Package : public Package
{
    Q_OBJECT

public:
    TH07Package();
};

#endif

