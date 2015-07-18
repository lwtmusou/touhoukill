#ifndef _th99_H
#define _th99_H

#include "package.h"
#include "card.h"


class qiuwenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE qiuwenCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class dangjiaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE dangjiaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class xiufuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE xiufuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class lianxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE lianxiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &use) const;
};

class zhesheCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE zhesheCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class zhuonongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE zhuonongCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class panduCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE panduCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};



class th99Package : public Package
{
    Q_OBJECT

public:
    th99Package();
};

#endif

