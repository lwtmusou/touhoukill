#ifndef _thndj_H
#define _thndj_H

#include "card.h"
#include "package.h"

class HunpoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HunpoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class THNDJPackage : public Package
{
    Q_OBJECT

public:
    THNDJPackage();
};

#endif
