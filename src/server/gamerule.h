#ifndef _GAME_RULE_H
#define _GAME_RULE_H

#include "skill.h"

class GameRule : public TriggerSkill
{
    Q_OBJECT

public:
    explicit GameRule(QObject *parent);
    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override;
    int getPriority() const override;
    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override;

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
    explicit HulaoPassMode(QObject *parent);
    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override;
};

#endif
