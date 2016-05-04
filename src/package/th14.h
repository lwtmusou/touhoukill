#ifndef _th14_H
#define _th14_H

#include "package.h"
#include "card.h"


class LeitingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LeitingCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class YuanfeiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YuanfeiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};


class LiangeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LiangeCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};



class TH14Package : public Package
{
    Q_OBJECT

public:
    TH14Package();
};

#endif

