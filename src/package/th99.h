#ifndef _th99_H
#define _th99_H

#include "package.h"
#include "card.h"


class QiuwenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QiuwenCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class DangjiaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE DangjiaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class XiufuMoveCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XiufuMoveCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onUse(Room *room, const CardUseStruct &card_use) const;
};

class XiufuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XiufuCard();

    void onUse(Room *room, const CardUseStruct &card_use) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;

private:
    static bool putToPile(Room *room, ServerPlayer *mori);
    static void cleanUp(Room *room, ServerPlayer *mori);
};

class LianxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LianxiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &use) const;
};


class ZhesheCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhesheCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ZhuonongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhuonongCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class YushouCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YushouCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;

};

class PanduCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE PanduCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};



class TH99Package : public Package
{
    Q_OBJECT

public:
    TH99Package();
};

#endif

