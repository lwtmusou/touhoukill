#ifndef _thndj_H
#define _thndj_H

#include "package.h"
#include "card.h"



class hunpoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE hunpoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class thndjPackage : public Package
{
    Q_OBJECT

public:
    thndjPackage();
};

#endif

