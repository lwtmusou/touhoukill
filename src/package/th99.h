#ifndef _th99_H
#define _th99_H

#include "card.h"
#include "package.h"

class QiuwenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QiuwenCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class DangjiaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE DangjiaCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class XiufuMoveCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XiufuMoveCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
};

class XiufuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XiufuCard();

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;

private:
    static bool putToPile(Room *room, ServerPlayer *mori);
    static void cleanUp(Room *room, ServerPlayer *mori);
};

class LianxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LianxiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    const Card *validate(CardUseStruct &use) const override;
};

class ZhesheCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhesheCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class ZhuonongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhuonongCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class YushouCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YushouCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
};

class PanduCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE PanduCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class ZhuozhiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhuozhiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
};

class XieliCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XieliCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class TH99Package : public Package
{
    Q_OBJECT

public:
    TH99Package();
};

#endif
