#ifndef _GAME_RULE_H
#define _GAME_RULE_H

#include "skill.h"

static QVariant _dummy_variant;

class GameRule : public TriggerSkill
{
    Q_OBJECT

public:
    GameRule(QObject *parent);
    virtual bool triggerable(const ServerPlayer *target) const;
    virtual int getPriority(TriggerEvent) const;
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data = _dummy_variant) const;

private:
    void onPhaseProceed(ServerPlayer *player) const;
    void rewardAndPunish(ServerPlayer *killer, ServerPlayer *victim) const;
    void changeGeneral1v1(ServerPlayer *player) const;
    void changeGeneralXMode(ServerPlayer *player) const;
    QString getWinner(ServerPlayer *victim) const;
};

class HulaoPassMode : public GameRule
{
    Q_OBJECT

public:
    HulaoPassMode(QObject *parent);
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data = _dummy_variant) const;
};

#endif

