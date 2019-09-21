#ifndef _hegemonyGeneral_H
#define _hegemonyGeneral_H

#include "card.h"
#include "package.h"


class XingyunHegemonyCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XingyunHegemonyCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class HegemonyGeneralPackage : public Package
{
    Q_OBJECT

public:
    HegemonyGeneralPackage();
};

#endif
