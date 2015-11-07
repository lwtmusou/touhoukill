#ifndef _th11_H
#define _th11_H

#include "package.h"
#include "card.h"


class MaihuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MaihuoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class YaobanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YaobanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};



class ChuanranCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ChuanranCard();


    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};
class TH11Package : public Package
{
    Q_OBJECT

public:
    TH11Package();
};

#endif

