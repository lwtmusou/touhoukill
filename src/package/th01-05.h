#ifndef TH0105_H
#define TH0105_H

#include "package.h"
#include "card.h"

class MenghuanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MenghuanCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};


class TH0105Package : public Package
{
    Q_OBJECT

public:
    TH0105Package();
};

#endif