#ifndef _hegemonyGeneral_H
#define _hegemonyGeneral_H

#include "card.h"
#include "package.h"
#include "skill.h"

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

class HalfLifeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HalfLifeCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class CompanionCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE CompanionCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class PioneerCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE PioneerCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};



class QingtingHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QingtingHegemonyCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ShowShezhengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShowShezhengCard();

    const Card *validate(CardUseStruct &card_use) const;
};



class SkltKexueHegCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SkltKexueHegCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class XushiHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XushiHegemonyCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class XingyunHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XingyunHegemonyCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ChunhenHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ChunhenHegemonyCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

/*class MocaoHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MocaoHegemonyCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};*/

class DongzhiHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE DongzhiHegemonyCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
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

/*class TuizhiHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TuizhiHegemonyCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};*/

class MengxianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MengxianCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class HegemonyGeneralPackage : public Package
{
    Q_OBJECT

public:
    HegemonyGeneralPackage();
};

#endif
