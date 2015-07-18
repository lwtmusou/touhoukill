#ifndef _thxwm_H
#define _thxwm_H

#include "package.h"
#include "card.h"


class shouhuiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE shouhuiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class woyuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE woyuCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class yazhiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE yazhiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class thxwmPackage : public Package
{
    Q_OBJECT

public:
    thxwmPackage();
};

#endif

