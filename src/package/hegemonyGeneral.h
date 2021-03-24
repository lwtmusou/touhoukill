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

    int getExtra(const Player *target) const override;
};

class HalfLifeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HalfLifeCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class CompanionCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE CompanionCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class PioneerCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE PioneerCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class QingtingHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QingtingHegemonyCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class ShowShezhengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShowShezhengCard();

    const Card *validate(CardUseStruct &card_use) const override;
};

class XushiHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XushiHegemonyCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class XingyunHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XingyunHegemonyCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class ShowFengsuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShowFengsuCard();

    const Card *validate(CardUseStruct &card_use) const override;
};

class ChunhenHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ChunhenHegemonyCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class DongzhiHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE DongzhiHegemonyCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
};

class BanyueHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BanyueHegemonyCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class KuaizhaoHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KuaizhaoHegemonyCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class HegemonyGeneralPackage : public Package
{
    Q_OBJECT

public:
    HegemonyGeneralPackage();
};

#endif
