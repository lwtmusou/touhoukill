#ifndef TH15_H
#define TH15_H

#include "card.h"
#include "package.h"

class YidanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YidanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
};

class TH15Package : public Package
{
    Q_OBJECT

public:
    TH15Package();
};

#endif
