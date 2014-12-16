#ifndef _protagonist_H
#define _protagonist_H

#include "package.h"
#include "card.h"


/*class lingqiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE lingqiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
	virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};*/


class mofaCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE mofaCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};



class wuyuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE wuyuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class saiqianCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE saiqianCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
	virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class jiezouCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE jiezouCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
	virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class shoucangCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE shoucangCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class baoyiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE baoyiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class bllmseyuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE bllmseyuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const;
	virtual const Card *validate(CardUseStruct &cardUse) const;
};

class bllmshiyudummy: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE bllmshiyudummy();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const;
};

class bllmshiyuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE bllmshiyuCard();

	virtual const Card *validate(CardUseStruct &cardUse) const;
	virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class bllmwuyuCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE bllmwuyuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class protagonistPackage: public Package {
    Q_OBJECT

public:
    protagonistPackage();
};

#endif

