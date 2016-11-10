#ifndef _th11_H
#define _th11_H

#include "package.h"
#include "card.h"


class MaihuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MaihuoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class YaobanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YaobanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class JiuhaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JiuhaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
};

class TH11Package : public Package
{
    Q_OBJECT

public:
    TH11Package();
};

#endif

