#ifndef _th11_H
#define _th11_H

#include "package.h"
#include "card.h"


class maihuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE maihuoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class yaobanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE yaobanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

/*class songzangCard: public SkillCard {
    Q_OBJECT

    public:
    Q_INVOKABLE songzangCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    };*/

class chuanranCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE chuanranCard();


    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};
class th11Package : public Package
{
    Q_OBJECT

public:
    th11Package();
};

#endif

