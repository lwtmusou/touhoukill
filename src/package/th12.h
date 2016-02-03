#ifndef _th12_H
#define _th12_H

#include "package.h"
#include "card.h"



class PuduCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE PuduCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};


class WeizhiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WeizhiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class NihuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NihuoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NuhuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NuhuoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ShuxinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShuxinCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class TH12Package : public Package
{
    Q_OBJECT

public:
    TH12Package();
};

#endif

