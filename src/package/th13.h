#ifndef _th13_H
#define _th13_H

#include "package.h"
#include "card.h"



class qingtingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE qingtingCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class xihuaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE xihuaCard();

    bool do_xihua(ServerPlayer *tanuki) const;
    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};


class shijieCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE shijieCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class leishiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE leishiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};


class xiefaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE xiefaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;

};


class huishengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE huishengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &card_use) const;
};



class bumingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE bumingCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class th13Package : public Package
{
    Q_OBJECT

public:
    th13Package();
};

#endif

