#ifndef _touhougod_H
#define _touhougod_H

#include "package.h"
#include "card.h"
#include "generaloverview.h" //for zun?

class hongwuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE hongwuCard();

	virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class shenqiangCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE shenqiangCard();

	virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class huimieCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE huimieCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
	virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class shenshouCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE shenshouCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class chaorenpreventrecast: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE chaorenpreventrecast();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
	virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
	virtual const Card *validate(CardUseStruct &card_use) const;

};

class ziwoCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ziwoCard();

	virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class chaowoCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE chaowoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
	virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class touhougodPackage: public Package {
    Q_OBJECT

public:
    touhougodPackage();
};

#endif

