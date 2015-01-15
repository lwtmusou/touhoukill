#ifndef _th14_H
#define _th14_H

#include "package.h"
#include "card.h"


class leitingCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE leitingCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class yuanfeiCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE yuanfeiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class yuanfeiNearCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE yuanfeiNearCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class liangeCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE liangeCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};



class th14Package : public Package {
    Q_OBJECT

public:
    th14Package();
};

#endif

