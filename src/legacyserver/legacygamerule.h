#ifndef qsgslegacy__GAME_RULE_H
#define qsgslegacy__GAME_RULE_H

#include "trigger.h"

class LegacyServerPlayer;

class LegacyGameRule : public Rule
{
public:
    LegacyGameRule();
    bool trigger(QSanguosha::TriggerEvent triggerEvent, RoomObject *room, const TriggerDetail &invoke, QVariant &data) const override;

private:
    void onPhaseProceed(LegacyServerPlayer *player) const;
    void rewardAndPunish(LegacyServerPlayer *killer, LegacyServerPlayer *victim) const;
    void changeGeneral1v1(LegacyServerPlayer *player) const;
    void changeGeneralXMode(LegacyServerPlayer *player) const;
    QString getWinner(LegacyServerPlayer *victim) const;
};

#endif
