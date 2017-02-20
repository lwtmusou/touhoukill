#ifndef _GAME_RULE_H
#define _GAME_RULE_H

#include "skill.h"

class GameRule : public TriggerSkill
{
    Q_OBJECT

public:
    GameRule(QObject *parent);
    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const;
    virtual int getPriority() const;
    virtual bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const;

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
    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const;
};

#endif
