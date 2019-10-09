#ifndef _hegemonyGeneral_H
#define _hegemonyGeneral_H

#include "card.h"
#include "skill.h"
#include "package.h"


class QiankunHegemony : public MaxCardsSkill
{
public:
    explicit QiankunHegemony(const QString &);

    virtual int getExtra(const Player *target) const;
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

class HegemonyGeneralPackage : public Package
{
    Q_OBJECT

public:
    HegemonyGeneralPackage();
};

#endif
