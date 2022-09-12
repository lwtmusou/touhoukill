#ifndef _thndj_H
#define _thndj_H

#include "card.h"
#include "package.h"

class HunpoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HunpoCard();

    void use(Room *room, const CardUseStruct &card_use) const override;
};

class YaoliCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YaoliCard();

    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;

    void onUse(Room *room, const CardUseStruct &use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class THNDJPackage : public Package
{
    Q_OBJECT

public:
    THNDJPackage();
};

#endif
