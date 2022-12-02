#ifndef TH17_H
#define TH17_H

#include "card.h"
#include "package.h"

class LunniCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LunniCard();

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};
/*
class ZhuyingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhuyingCard();

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};
*/
class LiaoguCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LiaoguCard();

    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class TH17Package : public Package
{
    Q_OBJECT

public:
    TH17Package();
};

#endif // TH16_H
