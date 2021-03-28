#ifndef _th14_H
#define _th14_H

#include "card.h"
#include "package.h"

class LeitingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LeitingCard();

    void onEffect(const CardEffectStruct &effect) const override;
};

class YuanfeiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YuanfeiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class LiangeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LiangeCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class TH14Package : public Package
{
    Q_OBJECT

public:
    TH14Package();
};

#endif
