#ifndef _th12_H
#define _th12_H

#include "package.h"
#include "card.h"



class puduCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE puduCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};


class weizhiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE weizhiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class nihuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE nihuoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class nuhuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE nuhuoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class th12Package : public Package
{
    Q_OBJECT

public:
    th12Package();
};

#endif

