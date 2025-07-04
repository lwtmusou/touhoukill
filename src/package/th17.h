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

class JunzhenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JunzhenCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetFixed(const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;

    bool feasible(const Player *Self, const Player *target) const;
};

class LiaoguCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LiaoguCard();

    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    void use(Room *room, const CardUseStruct &card_use) const override;
};

class WeiyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WeiyiCard();

    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class TH17Package : public Package
{
    Q_OBJECT

public:
    TH17Package();
};

#endif // TH16_H
