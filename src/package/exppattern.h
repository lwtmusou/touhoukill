#ifndef _EXPPATTERN_H
#define _EXPPATTERN_H

#include "card.h"
#include "package.h"
#include "player.h"

class ExpPattern : public CardPattern
{
public:
    explicit ExpPattern(const QString &exp);
    bool match(const Player *player, const Card *card) const override;

private:
    QString exp;
    bool matchOne(const Player *player, const Card *card, QString exp) const;
};

#endif
