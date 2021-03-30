#ifndef _th12_H
#define _th12_H

#include "card.h"
#include "package.h"

class PuduCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE PuduCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class WeizhiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WeizhiCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class NihuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NihuoCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class NuhuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NuhuoCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class ShuxinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShuxinCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class HuishengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuishengCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    const Card *validate(CardUseStruct &card_use) const override;
};

class TH12Package : public Package
{
    Q_OBJECT

public:
    TH12Package();
};

#endif
