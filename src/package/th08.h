#ifndef _th08_H
#define _th08_H

#include "card.h"
#include "package.h"

class MiyaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MiyaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class KuangzaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KuangzaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class BuxianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BuxianCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class XingyunCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XingyunCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class GeshengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GeshengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class YegeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YegeCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class YinghuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YinghuoCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class ChuangshiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ChuangshiCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class HuweiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuweiCard();

    virtual const Card *validate(CardUseStruct &card_use) const;
};

class JinxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JinxiCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class MingmuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MingmuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TH08Package : public Package
{
    Q_OBJECT

public:
    TH08Package();
};

#endif
