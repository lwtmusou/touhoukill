#ifndef TOUHOUKILL_EXPPATTERN_H
#define TOUHOUKILL_EXPPATTERN_H

#include "package.h"
#include "qsgscore.h"

class Player;
class Card;

class QSGS_CORE_EXPORT ExpPattern : public CardPattern
{
public:
    explicit ExpPattern(const QString &exp);
    ~ExpPattern() override = default;
    bool match(const Player *player, const Card *card) const override;

private:
    QString exp;
    bool matchOne(const Player *player, const Card *card, const QString &exp) const;
};

#endif