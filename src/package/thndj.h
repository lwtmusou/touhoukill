#ifndef _thndj_H
#define _thndj_H

#include "package.h"
#include "card.h"



class youmingCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE youmingCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class thndjPackage : public Package {
    Q_OBJECT

public:
    thndjPackage();
};

#endif

