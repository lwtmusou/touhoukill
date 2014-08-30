#ifndef _LING_H
#define _LING_H

#include "package.h"
#include "card.h"
#include "standard.h"
#include "yjcm-package.h"
#include "hegemony.h"

class LingPackage: public Package {
    Q_OBJECT

public:
    LingPackage();
};

class Ling2013Package: public Package {
    Q_OBJECT

public:
    Ling2013Package();
};

class LingCardsPackage: public Package{
    Q_OBJECT

public:
    LingCardsPackage();
};

class LuoyiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE LuoyiCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class Neo2013XinzhanCard: public XinzhanCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013XinzhanCard();
};

class Neo2013FanjianCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013FanjianCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Neo2013PujiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013PujiCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class Neo2013XiechanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013XiechanCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Neo2013ZhoufuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013ZhoufuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class Neo2013XiongyiCard: public XiongyiCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013XiongyiCard();
    virtual int getDrawNum() const;
};

class Neo2013JiejiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013JiejiCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class Neo2013JinanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013JinanCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Neo2013YongyiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013YongyiCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class AwaitExhausted: public TrickCard{
    Q_OBJECT

public:
    Q_INVOKABLE AwaitExhausted(Card::Suit suit, int number);

    virtual QString getSubtype() const;

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class BefriendAttacking: public SingleTargetTrick{
    Q_OBJECT

public:
    Q_INVOKABLE BefriendAttacking(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};


class KnownBoth: public SingleTargetTrick{
    Q_OBJECT

public:
    Q_INVOKABLE KnownBoth(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NeoDrowning: public AOE{
    Q_OBJECT

public:
    Q_INVOKABLE NeoDrowning(Card::Suit suit, int number);
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class SixSwords: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE SixSwords(Card::Suit suit, int number);
    virtual void onUninstall(ServerPlayer *player) const;
};

class SixSwordsCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE SixSwordsCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Triblade: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE Triblade(Card::Suit suit, int number);
};

class TribladeCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE TribladeCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class DragonPhoenix: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE DragonPhoenix(Card::Suit suit, int number);
};

class PeaceSpell: public Armor{
    Q_OBJECT

public:
    Q_INVOKABLE PeaceSpell(Card::Suit suit, int number);
    virtual void onUninstall(ServerPlayer *player) const;
};

class PeaceSpellCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE PeaceSpellCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif

