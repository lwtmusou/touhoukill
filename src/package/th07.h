#ifndef _th07_H
#define _th07_H

#include "package.h"
#include "card.h"


class zhaoliaoCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE zhaoliaoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

/*class xiezouCard: public SkillCard {
    Q_OBJECT

    public:
    Q_INVOKABLE xiezouCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &card_use) const;
    };*/


class mocaoCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE mocaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};


class th07Package : public Package {
    Q_OBJECT

public:
    th07Package();
};

#endif

