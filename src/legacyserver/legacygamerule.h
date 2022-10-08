#ifndef qsgslegacy__GAME_RULE_H
#define qsgslegacy__GAME_RULE_H

#include "trigger.h"

class LegacyServerPlayer;
class LegacyRoom;

class LegacyGameRule : public Rule
{
public:
    LegacyGameRule();
    bool trigger(QSanguosha::TriggerEvent triggerEvent, GameLogic *logic, const TriggerDetail &invoke, QVariant &data) const override;

private:
    void onPhaseProceed(LegacyRoom *room, LegacyServerPlayer *player) const;
    void rewardAndPunish(LegacyRoom *room, LegacyServerPlayer *killer, LegacyServerPlayer *victim) const;
    void changeGeneral1v1(LegacyRoom *room, LegacyServerPlayer *player) const;
    void changeGeneralXMode(LegacyRoom *room, LegacyServerPlayer *player) const;
    QString getWinner(LegacyRoom *room, LegacyServerPlayer *victim) const;
};

#endif
