#ifndef _thndj_H
#define _thndj_H

#include "card.h"
#include "package.h"

class HunpoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HunpoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class YaoliCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YaoliCard();

    virtual bool targetFixed(const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;

    virtual void onUse(Room *room, const CardUseStruct &use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class THNDJPackage : public Package
{
    Q_OBJECT

public:
    THNDJPackage();
};

#endif
