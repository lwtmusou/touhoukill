#ifndef _th09_H
#define _th09_H

#include "package.h"
#include "card.h"




class TianrenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TianrenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};


class NianliCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NianliCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
};


class TH09Package : public Package
{
    Q_OBJECT

public:
    TH09Package();
};

#endif

