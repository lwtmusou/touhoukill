#ifndef _touhougod_H
#define _touhougod_H

#include "package.h"
#include "card.h"

class HongwuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HongwuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class ShenqiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShenqiangCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class HuimieCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuimieCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ShenshouCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShenshouCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class FengyinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE FengyinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class HuaxiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuaxiangCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class ZiwoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZiwoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ChaowoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ChaowoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class WendaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WendaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class RumoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RumoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TouhouGodPackage : public Package
{
    Q_OBJECT

public:
    TouhouGodPackage();
};

#endif

