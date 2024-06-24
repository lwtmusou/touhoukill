#ifndef _th08_H
#define _th08_H

#include "card.h"
#include "package.h"

class MiyaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MiyaoCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class KuangzaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KuangzaoCard();

    void onEffect(const CardEffectStruct &effect) const override;
};

class BuxianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BuxianCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class XingyunCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XingyunCard();

    void use(Room *room, const CardUseStruct &card_use) const override;
};

class YegeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YegeCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    const Card *validate(CardUseStruct &cardUse) const override;
};

class YinghuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YinghuoCard();

    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    const Card *validate(CardUseStruct &card_use) const override;
    const Card *validateInResponse(ServerPlayer *user) const override;
};

class ChuangshiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ChuangshiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class HuweiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuweiCard();

    const Card *validate(CardUseStruct &card_use) const override;
};

class JinxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JinxiCard();

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class MingmuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MingmuCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class TH08Package : public Package
{
    Q_OBJECT

public:
    TH08Package();
};

#endif
