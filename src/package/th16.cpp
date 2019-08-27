#include "th16.h"
#include "engine.h"
#include "general.h"
#include "skill.h"

MenfeiCard::MenfeiCard()
{
}

bool MenfeiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
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
        foreach (const Player *p, player->getAliveSiblings()) {
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

    void record(TriggerEvent, Room *, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeSkill)
            return;

        use.card->setFlags("-menfei");
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeSkill)
            return QList<SkillInvokeDetail>();

        if (use.card->hasFlag("menfei"))
            return QList<SkillInvokeDetail>();

        ServerPlayer *player = use.from;

        if (player && player->isAlive() && player->hasSkill(this)) {
            ServerPlayer *target = NULL;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
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

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.card->setFlags("menfei");
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeBasic || use.card->isNDTrick()) {
            ServerPlayer *player = use.from;
            if (player && player->isAlive() && player->hasSkill(this)) {
                ServerPlayer *target = NULL;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getMark("@door") > 0) {
                        target = p;
                        break;
                    }
                }
                if (target) {
                    if (use.to.contains(target)) {
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, false, target);
                    } else if (!player->isProhibited(target, use.card) && (use.card->targetFilter(QList<const Player *>(), target, player))) {
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, false, target);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.contains(invoke->targets.first())) {
            invoke->invoker->drawCards(1);
        } else {
            CardUseStruct use = data.value<CardUseStruct>();
            use.to << invoke->targets.first();
            room->sortByActionOrder(use.to);
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

class Puti : public TriggerSkill
{
public:
    Puti()
        : TriggerSkill("puti")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                if (change.player->isCardLimited("use", "puti"))
                    room->removePlayerCardLimitation(change.player, "use", "Slash$1", "puti");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (!player->hasSkill(this) || player->isDead() || player->getPhase() != Player::Play)
                return QList<SkillInvokeDetail>();
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        int acquired = 0;
        QList<int> throwIds;
        while (acquired < 1) {
            int id = room->drawCard();
            CardsMoveStruct move(id, NULL, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName()));
            move.reason.m_skillName = "puti";
            room->moveCardsAtomic(move, true);
            room->getThread()->delay();
            Card *card = Sanguosha->getCard(id);
            if (card->getTypeId() == Card::TypeTrick) {
                acquired = acquired + 1;
                CardsMoveStruct move2(id, player, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_GOTBACK, player->objectName()));
                room->moveCardsAtomic(move2, true);

                room->setPlayerCardLimitation(player, "use", "Slash", "puti", true);
                if (!throwIds.isEmpty()) {
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "sishu", QString());
                    DummyCard dummy(throwIds);
                    room->throwCard(&dummy, reason, NULL);
                    throwIds.clear();
                }
            } else
                throwIds << id;
        }

        return false;
    }
};

class Zangfa : public TriggerSkill
{
public:
    Zangfa()
        : TriggerSkill("zangfa")
    {
        events << TargetSpecifying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isNDTrick())
            return QList<SkillInvokeDetail>();

        bool invoke = false;
        use.card->setFlags("IgnoreFailed");
        use.card->setFlags("zangfa");
        foreach (ServerPlayer *q, room->getAlivePlayers()) {
            if (!use.to.contains(q) && !use.from->isProhibited(q, use.card) && use.card->targetFilter(QList<const Player *>(), q, use.from)) {
                invoke = true;
                break;
            }
        }
        use.card->setFlags("-zangfa");
        use.card->setFlags("-IgnoreFailed");
        if (!invoke)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        QList<ServerPlayer *> invokers = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *p, invokers) {
            if (p->isAlive() && (p == use.from || use.to.contains(p)))
                d << SkillInvokeDetail(this, p, p);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QList<ServerPlayer *> listt;
        CardUseStruct use = data.value<CardUseStruct>();
        use.card->setFlags("IgnoreFailed");
        use.card->setFlags("zangfa");
        foreach (ServerPlayer *q, room->getAlivePlayers()) {
            if (!use.to.contains(q) && !use.from->isProhibited(q, use.card) && use.card->targetFilter(QList<const Player *>(), q, use.from))
                listt << q;
        }
        use.card->setFlags("-zangfa");
        use.card->setFlags("-IgnoreFailed");
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, listt, objectName(), "@zangfa", true, true);
        //player->tag.remove("huanshi_source");
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.to << invoke->targets.first();
        room->sortByActionOrder(use.to);
        data = QVariant::fromValue(use);
        return false;
    }
};

class ZangfaDistance : public TargetModSkill
{
public:
    ZangfaDistance()
        : TargetModSkill("zangfa-dist")
    {
        pattern = "TrickCard";
    }

    int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->hasFlag("zangfa"))
            return 1000;

        return 0;
    }
};

TH16Package::TH16Package()
    : Package("th16")
{
    General *okina = new General(this, "okina$", "tkz");
    okina->addSkill(new Menfei);
    okina->addSkill(new Houhu);

    General *eternity = new General(this, "eternity", "tkz", 4, false, true);
    Q_UNUSED(eternity);

    General *nemuno = new General(this, "nemuno", "tkz", 4, false, true);
    Q_UNUSED(nemuno);

    General *aun = new General(this, "aun", "tkz", 4, false, true);
    Q_UNUSED(aun);

    General *narumi = new General(this, "narumi", "tkz");
    narumi->addSkill(new Puti);
    narumi->addSkill(new Zangfa);

    General *satono = new General(this, "satono", "tkz", 4, false, true);
    Q_UNUSED(satono);

    General *mai = new General(this, "mai", "tkz", 4, false, true);
    Q_UNUSED(mai);

    addMetaObject<MenfeiCard>();
    skills << new ZangfaDistance;
}

ADD_PACKAGE(TH16)
