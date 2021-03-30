#ifndef _th11_H
#define _th11_H

#include "card.h"
#include "package.h"

class MaihuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MaihuoCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class YaobanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YaobanCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class SongzangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SongzangCard();
    void onEffect(const CardEffectStruct &effect) const override;
};

class JiuhaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JiuhaoCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    const Card *validate(CardUseStruct &card_use) const override;
};

class TH11Package : public Package
{
    Q_OBJECT

public:
    TH11Package();
};

#endif
