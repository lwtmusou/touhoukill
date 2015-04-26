#ifndef _th06_H
#define _th06_H

#include "package.h"
#include "card.h"


class skltkexueCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE skltkexueCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class suodingCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE suodingCard();

    virtual bool targetFilter(const QList<const Player *> &targets,
        const Player *to_select, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select,
        const Player *Self, int &maxVotes) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


class zhanyiCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE zhanyiCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class banyueCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE banyueCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class th06Package : public Package {
    Q_OBJECT

public:
    th06Package();
};

#endif

