#ifndef _th06_H
#define _th06_H

#include "card.h"
#include "package.h"

class SkltKexueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SkltKexueCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class SuodingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SuodingCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, int &maxVotes) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class BeishuiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BeishuiCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class BanyueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BanyueCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TH06Package : public Package
{
    Q_OBJECT

public:
    TH06Package();
};

#endif
