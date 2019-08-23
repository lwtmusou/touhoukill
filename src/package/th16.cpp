#include "th16.h"
#include "general.h"
#include "skill.h"

MenfeiCard::MenfeiCard()
{
}

bool MenfeiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty();
}

void MenfeiCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->gainMark("@door");

}


class MenfeiVS : public ZeroCardViewAsSkill
{
public:
    MenfeiVS()
        : ZeroCardViewAsSkill("menfei")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark("@door") > 0)
            return false;
        foreach(const Player *p, player->getAliveSiblings()) {
            if (p->getMark("@door") > 0)
                return false;
        }
        return true;
    }

    virtual const Card *viewAs() const
    {
        return new MenfeiCard;
    }
};

class Menfei : public TriggerSkill
{
public:
    Menfei()
        : TriggerSkill("menfei")
    {
        events << CardFinished;
        view_as_skill = new MenfeiVS;
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeSkill)
            return  QList<SkillInvokeDetail>();

        ServerPlayer *player = use.from;

        if (player && player->isAlive() && player->hasSkill(this)) {
            ServerPlayer *target = NULL;
            foreach (ServerPlayer *p, room->getAlivePlayers()) 
            {
                if (p->getMark("@door") > 0) {
                    target = p;
                    break;
                }
            }
            if (target) {
                ServerPlayer *next = qobject_cast<ServerPlayer *>(target->getNextAlive(1));
                if (next && next != target) {
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true, next);
                }
            }
        }
        
        return  QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        foreach(ServerPlayer *p, room->getAlivePlayers())
        {
            if (p->getMark("@door") > 0) {
                room->setPlayerMark(p, "@door", 0);
                break;
            }
        }
        invoke->targets.first()->gainMark("@door");
        return false;
    }
};



class Houhu : public TriggerSkill
{
public:
    Houhu()
        : TriggerSkill("houhu")
    {
        events << TargetSpecifying;
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeBasic || use.card->isNDTrick()) {
            ServerPlayer *player = use.from;
            if (player && player->isAlive() && player->hasSkill(this)) {
                ServerPlayer *target = NULL;
                foreach(ServerPlayer *p, room->getAlivePlayers())
                {
                    if (p->getMark("@door") > 0) {
                        target = p;
                        break;
                    }
                }
                if (target) {
                    if (use.to.contains(target)) {
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, false, target);
                    }
                    else if (!player->isProhibited(target, use.card) && (use.card->targetFilter(QList<const Player *>(), target, player))) {
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, false, target);
                    }
                }
            }
        }
        return  QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.contains(invoke->targets.first())) {
            invoke->invoker->drawCards(1);
        }
        else {
            CardUseStruct use = data.value<CardUseStruct>();
            use.to << invoke->targets.first();
            room->sortByActionOrder(use.to);
            data = QVariant::fromValue(use);
        }
        return false;
    }
};



TH16Package::TH16Package()
    : Package("th16")
{
    General *okina = new General(this, "okina$", "tkz");
    okina->addSkill(new Menfei);
    okina->addSkill(new Houhu);

    General *etanity = new General(this, "etanity", "tkz");
    Q_UNUSED(etanity);

    General *nemuno = new General(this, "nemuno", "tkz");
    Q_UNUSED(nemuno);

    General *aun = new General(this, "aun", "tkz");
    Q_UNUSED(aun);

    General *narumi = new General(this, "narumi", "tkz");
    Q_UNUSED(narumi);

    General *satono = new General(this, "satono", "tkz");
    Q_UNUSED(satono);

    General *mai = new General(this, "mai", "tkz");
    Q_UNUSED(mai);


    addMetaObject<MenfeiCard>();

}

ADD_PACKAGE(TH16)
