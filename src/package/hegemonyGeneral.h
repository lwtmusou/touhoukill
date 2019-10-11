#ifndef _hegemonyGeneral_H
#define _hegemonyGeneral_H

#include "card.h"
#include "skill.h"
#include "package.h"


class NiaoxiangSummon : public ArraySummonCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NiaoxiangSummon();
};


class QiankunHegemony : public MaxCardsSkill
{
public:
    explicit QiankunHegemony(const QString &);

    virtual int getExtra(const Player *target) const;
};

class QingtingHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QingtingHegemonyCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class XingyunHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XingyunHegemonyCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class MocaoHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MocaoHegemonyCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};


class BanyueHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BanyueHegemonyCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TuizhiHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TuizhiHegemonyCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};


class HegemonyGeneralPackage : public Package
{
    Q_OBJECT

public:
    HegemonyGeneralPackage();
};

#endif
