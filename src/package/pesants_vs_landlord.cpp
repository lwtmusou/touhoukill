#include "peasants_vs_landlord.h"

#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"


class Zhubing : public TriggerSkill
{
public:
    Zhubing()
        : TriggerSkill("zhubing")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    bool canPreshow() const override
    {
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *a = data.value<ServerPlayer *>();
        if (!a->hasSkill(this) || a->isDead() || a->getPhase() != Player::Start)
            return QList<SkillInvokeDetail>();

        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, a, a, nullptr, true);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        room->broadcastSkillInvoke(objectName());

        player->drawCards(1);

        

        return false;
    }
};

class ZhubingTargetMod : public TargetModSkill
{
public:
    ZhubingTargetMod()
        : TargetModSkill("#zhubing_mod")
    {
        pattern = "Slash";
    }

    int getResidueNum(const Player *from, const Card *) const override
    {
        int num = 0;
        if (from->hasSkill("zhubing"))
            ++num;
        return num;
    }
};



class Jili : public TriggerSkill
{
public:
    Jili()
        : TriggerSkill("jili")
    {
        events << Death;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who->hasSkill(objectName()) && room->getAlivePlayers().length() > 1) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, death.who, death.who, nullptr, true);
        }    
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->broadcastSkillInvoke(objectName());

        ServerPlayer *target;
        foreach(ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (invoke->invoker->getRole() == p->getRole()) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), p->objectName());
                p->drawCards(2);
                break;
            }
        }

        return false;
    }
};



PeasantsVSLandlordPackage::PeasantsVSLandlordPackage()
    : Package("peasants_vs_landlord")
{

    skills << new Zhubing << new ZhubingTargetMod << new Jili;
    related_skills.insertMulti("zhubing", "#zhubing_mod");
}

ADD_PACKAGE(PeasantsVSLandlord)
