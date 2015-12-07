#ifndef _thxwm_H
#define _thxwm_H

#include "package.h"
#include "card.h"


class ShouhuiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShouhuiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class WoyuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WoyuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class YazhiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YazhiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class THXWMPackage : public Package
{
    Q_OBJECT

public:
    THXWMPackage();
};

#endif

