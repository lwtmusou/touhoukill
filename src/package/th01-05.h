#ifndef TH0105_H
#define TH0105_H

#include "card.h"
#include "package.h"

class ShiquCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShiquCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class LianmuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LianmuCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    const Card *validate(CardUseStruct &card_use) const override;
};

class SqChuangshiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SqChuangshiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class ModianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ModianCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class BaosiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BaosiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class MoyanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MoyanCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class QirenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QirenCard();

    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    bool isAvailable(const Player *player) const override;
};

class LuliCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LuliCard();

    void use(Room *room, const CardUseStruct &card_use) const override;
};

class ZhancheCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhancheCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    //void onUse(Room *room, const CardUseStruct &card_use) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class YihuanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YihuanCard();

    void onEffect(const CardEffectStruct &effect) const override;
};

class TH0105Package : public Package
{
    Q_OBJECT

public:
    TH0105Package();
};

#endif
