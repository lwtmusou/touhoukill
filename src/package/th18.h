#ifndef TH18_H
#define TH18_H

#include "card.h"
#include "package.h"

class BoxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BoxiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &_use) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class BoxiUseOrObtainCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BoxiUseOrObtainCard();

    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    const Card *validate(CardUseStruct &card_use) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class TiaosuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TiaosuoCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class JuezhuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JuezhuCard();
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class ZhuyuUseDiscardPileCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhuyuUseDiscardPileCard();

    bool targetFixed(const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    const Card *validate(CardUseStruct &cardUse) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class ZhuyuSlashCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhuyuSlashCard();

    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    const Card *validate(CardUseStruct &cardUse) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class SimaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SimaoCard();

    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, const CardUseStruct &use) const override;
};

class ShirongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShirongCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class TH18Package : public Package
{
    Q_OBJECT

public:
    TH18Package();
};

#endif // TH16_H
