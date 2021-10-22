#ifndef _th07_H
#define _th07_H

#include "card.h"
#include "package.h"

class XijianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XijianCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
};

class ShihuiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShihuiCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class QimenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QimenCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
};

class MocaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MocaoCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class YujianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YujianCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class HuayinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuayinCard();

    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class TH07Package : public Package
{
    Q_OBJECT

public:
    TH07Package();
};

#endif
