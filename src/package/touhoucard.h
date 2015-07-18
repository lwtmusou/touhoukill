#ifndef TOUHOUCARD_H
#define TOUHOUCARD_H


#include "card.h"
#include "package.h"
#include "standard.h"

class touhoucardPackage : public Package
{
    Q_OBJECT

public:
    touhoucardPackage();
};

class touhouskillcardPackage : public Package
{
    Q_OBJECT

public:
    touhouskillcardPackage();
};

class NosRendeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosRendeCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};




#endif // JOYPACKAGE_H
