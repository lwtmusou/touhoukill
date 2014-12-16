#ifndef _TIGERFLY_H
#define _TIGERFLY_H

#include "package.h"
#include "card.h"
#include "wind.h"

class PozhenCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE PozhenCard();
    virtual bool targetFilter(const QList<const Player *> &, const Player *, const Player *) const;
    virtual void onEffect(const CardEffectStruct &) const;

private:
    static QString suittb(Card::Suit s);
};

class TushouGiveCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE TushouGiveCard();
    virtual bool targetFilter(const QList<const Player *> &, const Player *, const Player *) const;
    virtual void onEffect(const CardEffectStruct &) const;

};

class ChouduCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ChouduCard();
    virtual bool targetFilter(const QList<const Player *> &, const Player *, const Player *) const;
    virtual void use(Room *, ServerPlayer *, QList<ServerPlayer *> &) const;
};

class GudanCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE GudanCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class JisiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE JisiCard();
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class ShangjianCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ShangjianCard();
    virtual void onEffect(const CardEffectStruct &effect) const;

};

class JingshangCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE JingshangCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class XingsuanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE XingsuanCard();
    virtual void use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const;
};

class ChanyuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ChanyuCard();
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class SuoshiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE SuoshiCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class ZongjiuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE ZongjiuCard();
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class FengyinCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE FengyinCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const;
};

class DuyiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE DuyiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const;
};

class TigerFlyPackage: public Package {
    Q_OBJECT

public:
    TigerFlyPackage();
};

#endif