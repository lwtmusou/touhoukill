#ifndef SCENERULE_H
#define SCENERULE_H

#include "gamerule.h"

class SceneRule : public GameRule
{
public:
    SceneRule(QObject *parent);
    int getPriority() const;
    bool effect(TriggerEvent event, Room *room, QSharedPointer<SkillInvokeDetail> detail, QVariant &data) const;
};

#endif // SCENERULE_H
