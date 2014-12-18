#ifndef _th09_H
#define _th09_H

#include "package.h"
#include "card.h"


class yanhuiCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE yanhuiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &card_use) const;
};

class tianrenCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE tianrenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};


class th09Package : public Package {
    Q_OBJECT

public:
    th09Package();
};

#endif

