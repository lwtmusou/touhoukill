#include "th06.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"
#include "util.h"
#include "maneuvering.h" // for beishui
#include "th10.h" //for beishui
#include "settings.h"

SkltKexueCard::SkltKexueCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
    m_skillName = "skltkexue_attach";
}
void SkltKexueCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (who != NULL && who->hasSkill("skltkexue")) {
        room->notifySkillInvoked(who, "skltkexue");
        room->loseHp(source);
        if (source->isAlive())
            source->drawCards(2);

        RecoverStruct recover;
        recover.recover = 1;
        recover.who = source;
        room->recover(who, recover);
    }
}

class SkltKexueVS : public ZeroCardViewAsSkill
{
public:
    SkltKexueVS() : ZeroCardViewAsSkill("skltkexue_attach")
    {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getHp() > player->dyingThreshold() && pattern.contains("peach")) {
            foreach (const Player *p, player->getAliveSiblings()) {
                if (p->hasFlag("Global_Dying") && p->hasSkill("skltkexue"))
                    return true;
            }
        }
        return false;
    }

    virtual const Card *viewAs() const
    {
        return new SkltKexueCard;
    }
};

class SkltKexue : public TriggerSkill
{
public:
    SkltKexue() : TriggerSkill("skltkexue")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Death << Debut << Revive;
    }

    void record(TriggerEvent, Room *room, QVariant &) const
    {
        static QString attachName = "skltkexue_attach";
        QList<ServerPlayer *> sklts;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->hasSkill(this, true))
                sklts << p;
        }

        if (sklts.length() > 1) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (!p->hasSkill(attachName, true))
                    room->attachSkillToPlayer(p, attachName);
            }
        } else if (sklts.length() == 1) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(this, true) && p->hasSkill(attachName, true))
                    room->detachSkillFromPlayer(p, attachName, true);
                else if (!p->hasSkill(this, true) && !p->hasSkill(attachName, true))
                    room->attachSkillToPlayer(p, attachName);
            }
        } else { // the case that sklts is empty
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(attachName, true))
                    room->detachSkillFromPlayer(p, attachName, true);
            }
        }
    }
};

class Mingyun : public TriggerSkill
{
public:
    Mingyun() : TriggerSkill("mingyun")
    {
        events << StartJudge;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (!judge->who || !judge->who->isAlive())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> r;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->hasSkill(this))
                r << SkillInvokeDetail(this, p, p);
        }

        return r;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        invoke->invoker->tag["mingyun_judge"] = data;
        JudgeStruct *judge = data.value<JudgeStruct *>();
        QString prompt = "judge:" + judge->who->objectName() + ":" + judge->reason;
        if (invoke->invoker->askForSkillInvoke(this, prompt)) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), judge->who->objectName());
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<int> list = room->getNCards(2);
        room->returnToTopDrawPile(list);

        room->fillAG(list, invoke->invoker);
        int obtain_id = room->askForAG(invoke->invoker, list, false, objectName());
        room->clearAG(invoke->invoker);
        room->obtainCard(invoke->invoker, obtain_id, false);
        return false;
    }
};



class Xueyi : public TriggerSkill
{
public:
    Xueyi() : TriggerSkill("xueyi$")
    {
        events << HpRecover;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        RecoverStruct r = data.value<RecoverStruct>();
        if (r.to->getKingdom() != "hmx")
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> details;
        foreach (ServerPlayer *rem, room->getOtherPlayers(r.to)) {
            if (rem->hasLordSkill(objectName()))
                details << SkillInvokeDetail(this, rem, r.to);
        }
        return details;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->tag["xueyi-target"] = QVariant::fromValue(invoke->owner);
        if (invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->owner))) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(invoke->owner, objectName());
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = invoke->invoker;
            log.to << invoke->owner;
            log.arg = objectName();
            room->sendLog(log);

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->owner->drawCards(1, objectName());
        return false;
    }
};

class Pohuai : public TriggerSkill
{
public:
    Pohuai() : TriggerSkill("pohuai")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *fldl = data.value<ServerPlayer *>();
        if (!fldl->hasSkill(this) || fldl->isDead() || fldl->getPhase() != Player::Start)
            return QList<SkillInvokeDetail>();

        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, fldl, fldl, NULL, true);
    }

    // compulsory effect, cost omitted

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
    {
        ServerPlayer *fldl = data.value<ServerPlayer *>();
        room->touhouLogmessage("#TriggerSkill", fldl, objectName());
        room->notifySkillInvoked(fldl, objectName());

        JudgeStruct judge;
        judge.who = fldl;
        judge.pattern = "Slash";
        judge.good = true;
        judge.reason = objectName();
        room->judge(judge);


        if (judge.isBad())
            return false;


        QList<ServerPlayer *> all;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (fldl->distanceTo(p) <= 1)
                all << p;
        }
        if (all.isEmpty())
            return false;

        foreach (ServerPlayer *p, all)
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, fldl->objectName(), p->objectName());

        foreach (ServerPlayer *p, all)
            room->damage(DamageStruct(objectName(), fldl, p));

        return false;
    }
};

class Yuxue : public TriggerSkill
{
public:
    Yuxue() : TriggerSkill("yuxue")
    {
        events << Damaged << ConfirmDamage << PreCardUsed;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->isKindOf("Slash") && use.from && use.from->hasSkill(this))
                room->notifySkillInvoked(use.from, objectName());
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == PreCardUsed)
            return QList<SkillInvokeDetail>();

        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == Damaged) {
            if (damage.to->isAlive() && damage.to->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        } else if (triggerEvent == ConfirmDamage) {
            if (damage.chain || damage.transfer || !damage.by_user)
                return QList<SkillInvokeDetail>();
            if (damage.from == damage.to)
                return QList<SkillInvokeDetail>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->hasFlag("yuxueSlash"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == Damaged) {
            room->setPlayerFlag(invoke->invoker, "SlashRecorder_yuxueSlash");
            if (!room->askForUseCard(invoke->invoker, "slash", "@yuxue", -1, Card::MethodUse, false, objectName()))
                room->setPlayerFlag(invoke->invoker, "-SlashRecorder_yuxueSlash");
        } else if (triggerEvent == ConfirmDamage)
            return true;

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        damage.damage = damage.damage + 1;
        if (damage.from) {
            QList<ServerPlayer *> logto;
            logto << damage.to;
            room->touhouLogmessage("#yuxue_damage", damage.from, "yuxue", logto);
        }
        data = QVariant::fromValue(damage);
        return false;
    }
};

class YuxueSlashNdl : public TargetModSkill
{
public:
    YuxueSlashNdl() : TargetModSkill("#yuxue-slash-ndl")
    {
        pattern = "Slash";
    }

    int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->hasFlag("SlashRecorder_yuxueSlash"))
            return 1000;

        return 0;
    }
};

class Shengyan : public TriggerSkill
{
public:
    Shengyan() : TriggerSkill("shengyan")
    {
        events << Damage;
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive() && damage.from->hasSkill(this)) {
            QList<SkillInvokeDetail> d;
            for (int i = 0; i < damage.damage; ++i)
                d << SkillInvokeDetail(this, damage.from, damage.from);

            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    // the cost is only askForSkillInvoke, omitted

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1, objectName());
        return false;
    }
};


SuodingCard::SuodingCard()
{
}

bool SuodingCard::targetFilter(const QList<const Player *> &, const Player *, const Player *) const
{
    Q_ASSERT(false);
    return false;
}

bool SuodingCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *, int &maxVotes) const
{
    if (to_select->isKongcheng())
        return false;
    int i = 0;

    foreach (const Player *player, targets) {
        if (player == to_select)
            i++;
    }

    maxVotes = qMax(3 - targets.size(), 0) + i;
    return maxVotes > 0;
}

bool SuodingCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    if (targets.toSet().size() > 3 || targets.toSet().size() == 0) return false;
    QMap<const Player *, int> map;

    foreach(const Player *sp, targets)
        map[sp]++;
    foreach (const Player *sp, map.keys()) {
        if (map[sp] > sp->getHandcardNum())
            return false;
    }

    return true;
}

void SuodingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QMap<ServerPlayer *, int> map;
    foreach(ServerPlayer *sp, targets)
        map[sp]++;

    QList<ServerPlayer *> newtargets = map.keys();
    room->sortByActionOrder(newtargets);
    foreach (ServerPlayer *sp, newtargets) {
        if (source == sp) {
            const Card *cards = room->askForExchange(source, "suoding", map.value(sp), map.value(sp), false, "suoding_exchange:" + QString::number(map[sp]));
            DELETE_OVER_SCOPE(const Card, cards)
                    foreach (int id, cards->getSubcards())
                    sp->addToPile("suoding_cards", id, false);
        } else {
            for (int i = 0; i < map[sp]; i++) {
                if (!sp->isKongcheng()) {
                    int card_id = room->askForCardChosen(source, sp, "hs", "suoding"); // fakemove/getrandomhandcard(without 2nd general!!!!)
                    sp->addToPile("suoding_cards", card_id, false);
                }
            }
        }
    }
}


class SuodingVS : public ZeroCardViewAsSkill
{
public:
    SuodingVS() : ZeroCardViewAsSkill("suoding")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("SuodingCard");
    }

    virtual const Card *viewAs() const
    {
        return new SuodingCard;
    }
};

class Suoding : public TriggerSkill
{
public:
    Suoding() : TriggerSkill("suoding")
    {
        events << EventPhaseChanging ;
        view_as_skill = new SuodingVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QList<SkillInvokeDetail>();

            foreach(ServerPlayer *liege, room->getAllPlayers()) {
                if (!liege->getPile("suoding_cards").isEmpty())
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, NULL, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    // compulsory effect, cost omitted

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        QList<CardsMoveStruct> moves;

        foreach (ServerPlayer *liege, room->getAllPlayers()) {
            if (!liege->getPile("suoding_cards").isEmpty()) {
                CardsMoveStruct move;
                move.card_ids = liege->getPile("suoding_cards");
                move.to_place = Player::PlaceHand;
                move.to = liege;
                moves << move;
                QList<ServerPlayer *> logto;
                logto << liege;
                room->touhouLogmessage("#suoding_Trigger", invoke->invoker, objectName(), logto, QString::number(move.card_ids.length()));
            }
        }

        room->moveCardsAtomic(moves, false);
        return false;
    }
};

class Huisu : public TriggerSkill
{
public:
    Huisu() : TriggerSkill("huisu")
    {
        events << PostHpLost << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *sixteen = NULL;
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            sixteen = damage.to;
        } else if (triggerEvent == PostHpLost) {
            HpLostStruct hplost = data.value<HpLostStruct>();
            sixteen = hplost.player;
        }

        if (!sixteen->hasSkill(this) || sixteen->isDead())
            return QList<SkillInvokeDetail>();

        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, sixteen, sixteen);
    }

    // the cost is only askForSkillInvoke, omitted

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        JudgeStruct judge;
        if (triggerEvent == Damaged)
            judge.pattern = ".|red|2~9";
        else
            judge.pattern = ".|heart|2~9";

        judge.good = true;
        if (triggerEvent == Damaged)
            judge.reason = "huisu1";
        else
            judge.reason = "huisu2";

        judge.who = invoke->invoker;
        room->judge(judge);

        if (judge.isGood()) {
            RecoverStruct recov;
            recov.recover = 1;
            room->recover(invoke->invoker, recov);
        }
        return false;
    }
};



class Bolan : public TriggerSkill
{
public:
    Bolan() : TriggerSkill("bolan")
    {
        events << CardUsed << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const
    {
        if (triggerEvent == EventPhaseChanging){
            foreach(ServerPlayer *p, room->getAllPlayers())
                p->setFlags("-" + objectName());
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent != CardUsed)
            return QList<SkillInvokeDetail>();

        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (!use.card->isNDTrick())
            return d;
        foreach(ServerPlayer *p, room->getAllPlayers()) {
            if (p->hasSkill(this) && p !=use.from && !p->hasFlag(objectName()))
                d << SkillInvokeDetail(this, p, p);
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<int> list = room->getNCards(2);
        ServerPlayer *player = invoke->invoker;
        player->setFlags(objectName());

        CardsMoveStruct move(list, NULL, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, invoke->invoker->objectName(), objectName(), QString()));
        room->moveCardsAtomic(move, true);

        QList<int> able;
        QList<int> disabled;
        foreach(int id, list) {
            Card *tmp_card = Sanguosha->getCard(id);
            if (tmp_card->isKindOf("TrickCard") || use.card->getSuit() == tmp_card->getSuit())
                able << id;
            else
                disabled << id;
        }

        if (!able.isEmpty()) {
            DummyCard dummy(able);
            room->obtainCard(player, &dummy);
        }
        if (!disabled.isEmpty()) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, invoke->invoker->objectName(), objectName(), QString());
            DummyCard dummy(disabled);
            room->throwCard(&dummy, reason, NULL);
        }

        return false;
    }
};

class HezhouVS : public ViewAsSkill
{
public:
    HezhouVS() : ViewAsSkill("hezhou")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() == 0)
            return true;
        else if (selected.length() == 1) {
            if (to_select->getTypeId() == selected.first()->getTypeId())
                return false;
            else {
                QList<int> ids = Self->getPile("wooden_ox");
                if (to_select->isKindOf("WoodenOx") && ids.contains(selected.first()->getId()))
                    return false;
                else if (selected.first()->isKindOf("WoodenOx") && ids.contains(to_select->getId()))
                    return false;
                else
                    return true;
            }
        } else
            return false;
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return  matchAvaliablePattern("peach", pattern)
                && !player->isCurrent()
                && player->getMark("Global_PreventPeach") == 0
                && (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE);
    }


    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;
        Peach *peach = new Peach(Card::SuitToBeDecided, -1);
        peach->addSubcards(cards);
        peach->setSkillName(objectName());
        return peach;
    }
};

class Hezhou : public TriggerSkill
{
public:
    Hezhou() : TriggerSkill("hezhou")
    {
        events << CardsMoveOneTime;
        view_as_skill = new HezhouVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
        if (player != NULL && player->isAlive() && player->hasSkill(this) && move.to_place == Player::DiscardPile
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE) {
            const Card *card = move.reason.m_extraData.value<const Card *>();
            if (card && card->getSkillName() == objectName()) {
                foreach(int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->isKindOf("TrickCard") && room->getCardPlace(id) == Player::DiscardPile)
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = invoke->invoker;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QString name = "";
        foreach(int id, move.card_ids) {
            if (Sanguosha->getCard(id)->isKindOf("TrickCard"))
                name = Sanguosha->getCard(id)->objectName();
        }
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(),
                                                        "@hezhou:" + name, true, true);
        if (target != NULL)
            invoke->targets << target;
        return target != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *target = invoke->targets.first();

        QList<int> ids;
        foreach(int id, move.card_ids) {
            if (Sanguosha->getCard(id)->isKindOf("TrickCard") && room->getCardPlace(id) == Player::DiscardPile)
                ids << id;
        }
        
        move.removeCardIds(ids);
        data = QVariant::fromValue(move);

        CardsMoveStruct mo;
        mo.card_ids = ids;
        mo.to = target;
        mo.to_place = Player::PlaceHand;
        room->moveCardsAtomic(mo, true);

        return false;
    }
};



class Taiji : public TriggerSkill
{
public:
    Taiji() : TriggerSkill("taiji")
    {
        events << TargetSpecified << TargetConfirmed << SlashMissed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (e == TargetSpecified || e == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash") || use.to.length() != 1)
                return d;
            QList<int> ids;
            if (use.card->isVirtualCard())
                ids = use.card->getSubcards();
            else
                ids << use.card->getEffectiveId();

            if (ids.isEmpty()) return QList<SkillInvokeDetail>();
            foreach(int id, ids) {
                if (room->getCardPlace(id) != Player::PlaceTable) return QList<SkillInvokeDetail>();
            }
            if (e == TargetSpecified && use.from->isAlive() && use.from->hasSkill(this) && use.to.first()->isAlive())
                d << SkillInvokeDetail(this, use.from, use.from, NULL, false, use.to.first());
            else if (e == TargetConfirmed && use.to.first()->isAlive() && use.to.first()->hasSkill(this))
                d << SkillInvokeDetail(this, use.to.first(), use.to.first(), NULL, false, use.from);
        } else if (e == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (!effect.from || effect.from->isDead())
                return QList<SkillInvokeDetail>();
            if (!effect.slash || !effect.slash->hasFlag("taiji_" + effect.from->objectName())) return QList<SkillInvokeDetail>();
            QList<int> ids;
            if (effect.jink->isVirtualCard())
                ids = effect.jink->getSubcards();
            else
                ids << effect.jink->getEffectiveId();
            if (ids.isEmpty()) return QList<SkillInvokeDetail>();
            foreach(int id, ids) {
                if (room->getCardPlace(id) != Player::DiscardPile) return QList<SkillInvokeDetail>();
            }
            d << SkillInvokeDetail(this, effect.from, effect.from, NULL, true);
        }
        return d;
    }

    bool cost(TriggerEvent e, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == SlashMissed)
            return true;
        invoke->invoker->tag["taiji"] = data;
        return invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget));
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == TargetSpecified || e == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            //room->setCardFlag(use.card, "taiji_" + liege->objectName());
            ServerPlayer *user = (e == TargetSpecified) ? invoke->invoker : invoke->targets.first();
            ServerPlayer*target = (e == TargetSpecified) ? invoke->targets.first() : invoke->invoker;
            room->setCardFlag(use.card, "taiji_" + user->objectName());
            if (use.m_addHistory) {
                room->addPlayerHistory(use.from, use.card->getClassName(), -1);
                use.m_addHistory = false;
                data = QVariant::fromValue(use);
            }
            target->obtainCard(use.card);
        } else if (e == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            effect.from->obtainCard(effect.jink);
        }
        return false;
    }
};


BeishuiCard::BeishuiCard()
{
    will_throw = false;
}

bool BeishuiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first());
            DELETE_OVER_SCOPE(Card, card)
                    card->addSubcards(subcards);
            return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
        }
        return false;
    }
    else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *card = Self->tag.value("beishui").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName());
    DELETE_OVER_SCOPE(Card, new_card)
            new_card->setSkillName("beishui");
    new_card->addSubcards(subcards);
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool BeishuiCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first());
            DELETE_OVER_SCOPE(Card, card)
                    card->addSubcards(subcards);
            return card && card->targetFixed();
        }
        return false;
    }
    else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("beishui").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName());
    DELETE_OVER_SCOPE(Card, new_card)
            new_card->setSkillName("beishui");
    new_card->addSubcards(subcards);
    return new_card && new_card->targetFixed();
}

bool BeishuiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first());
            DELETE_OVER_SCOPE(Card, card)
                    card->addSubcards(subcards);
            return card && card->targetsFeasible(targets, Self);
        }
        return false;
    }
    else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("beishui").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName());
    DELETE_OVER_SCOPE(Card, new_card)
            new_card->setSkillName("beishui");
    new_card->addSubcards(subcards);
    return new_card && new_card->targetsFeasible(targets, Self);
}

const Card *BeishuiCard::validate(CardUseStruct &card_use) const
{
    QString to_use = user_string;
    Card *use_card = Sanguosha->cloneCard(to_use);
    use_card->setSkillName("beishui");
    use_card->addSubcards(subcards);

    card_use.from->getRoom()->setPlayerMark(card_use.from, "beishui", 1);
    return use_card;
}

const Card *BeishuiCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();

    QString to_use;
    if (user_string == "peach+analeptic") {
        QStringList use_list;
        Card *peach = Sanguosha->cloneCard("peach");
        DELETE_OVER_SCOPE(Card, peach)
                if (!user->isCardLimited(peach, Card::MethodResponse, true))
                use_list << "peach";
        Card *ana = Sanguosha->cloneCard("analeptic");
        DELETE_OVER_SCOPE(Card, ana)
                if (!Config.BanPackages.contains("maneuvering") && !user->isCardLimited(ana, Card::MethodResponse, true))
                use_list << "analeptic";
        to_use = room->askForChoice(user, "qiji_skill_saveself", use_list.join("+"));
    }
    else
        to_use = user_string;

    
    Card *use_card = Sanguosha->cloneCard(to_use);
    use_card->setSkillName("beishui");
    use_card->addSubcards(subcards);
    use_card->deleteLater();
    user->getRoom()->setPlayerMark(user, "beishui", 1);
    return use_card;
}

class BeishuiVS : public ViewAsSkill
{
public:
    BeishuiVS() : ViewAsSkill("beishui")
    {
        response_or_use = true;
    }

    static QStringList responsePatterns() {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();

        Card::HandlingMethod method = Card::MethodUse;
        QStringList validPatterns;
        validPatterns << "slash" << "analeptic" << "jink" << "peach";
        QStringList checkedPatterns;
        foreach(QString str, validPatterns) {
            const Skill *skill = Sanguosha->getSkill("beishui");
            if (skill->matchAvaliablePattern(str, pattern)) {
                Card *card = Sanguosha->cloneCard(str);
                DELETE_OVER_SCOPE(Card, card)
                        if (!Self->isCardLimited(card, method))
                        checkedPatterns << str;
            }
        }
        return checkedPatterns;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark("beishui") > 0) return false;
        if (Slash::IsAvailable(player) || Analeptic::IsAvailable(player))
            return true;
        Card *card = Sanguosha->cloneCard("peach", Card::NoSuit, 0);
        DELETE_OVER_SCOPE(Card, card)
                return card->isAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            return false;
        if (player->getMark("beishui") > 0) return false;

        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.contains("peach") && checkedPatterns.length() == 1
                && player->getMark("Global_PreventPeach") > 0) return false;

        return !checkedPatterns.isEmpty();
    }


    virtual bool viewFilter(const QList<const Card *> &selected, const Card *) const
    {
        int num = qMax(1, Self->getHp());
        return selected.length() < num;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        int num = qMax(1, Self->getHp());
        if (cards.length() != num) return NULL;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();

            QStringList checkedPatterns = responsePatterns();
            if (checkedPatterns.length() > 1 || checkedPatterns.contains("slash")) {
                const Card *c = Self->tag.value("beishui").value<const Card *>();
                if (c)
                    pattern = c->objectName();
                else
                    return NULL;
            }
            else
                pattern = checkedPatterns.first();
            BeishuiCard *card = new BeishuiCard;
            card->setUserString(pattern);
            card->addSubcards(cards);
            return card;
        }

        const Card *c = Self->tag.value("beishui").value<const Card *>();
        if (c) {
            BeishuiCard *card = new BeishuiCard;
            card->setUserString(c->objectName());
            card->addSubcards(cards);
            return card;
        }
        else
            return NULL;
    }

    
};

class Beishui : public TriggerSkill
{
public:
    Beishui() : TriggerSkill("beishui")
    {
        events << EventPhaseChanging << PreCardUsed << CardResponded;
        view_as_skill = new BeishuiVS;
    }

    virtual QDialog *getDialog() const
    {
        return QijiDialog::getInstance("beishui", true, false);
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseChanging) {
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getMark("beishui") > 0)
                    room->setPlayerMark(p, "beishui", 0);
            }
        }
        //record for ai, since AI prefer use a specific card,  but not the SkillCard QijiCard.
        if (e == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == objectName())
                room->setPlayerMark(use.from, "beishui", 1);
        }
        if (e == CardResponded) {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_from && response.m_isUse && !response.m_isProvision && response.m_card && response.m_card->getSkillName() == objectName())
                room->setPlayerMark(response.m_from, "beishui", 1);
        }
    }
};







class Dongjie : public TriggerSkill
{
public:
    Dongjie() : TriggerSkill("dongjie")
    {
        events << DamageCaused << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach(ServerPlayer *p, room->getAllPlayers())
                p->setFlags("-" + objectName());
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e != DamageCaused)
            return QList<SkillInvokeDetail>();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user)
            return QList<SkillInvokeDetail>();
        if (damage.from && damage.card && damage.from->hasSkill(this) && !damage.from->hasFlag(objectName()))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, false, damage.to);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), damage.to->objectName());

        invoke->invoker->setFlags(objectName());
        QList<ServerPlayer *> logto;
        logto << damage.to;
        room->touhouLogmessage("#Dongjie", invoke->invoker, "dongjie", logto);
        invoke->invoker->drawCards(1);
        damage.to->turnOver();
        damage.to->drawCards(1);
        return true;
    }
};

class Bingpo : public TriggerSkill
{
public:
    Bingpo() : TriggerSkill("bingpo")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->hasSkill(this) && damage.nature != DamageStruct::Fire && damage.damage >= damage.to->getHp())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->touhouLogmessage("#bingpolog", invoke->invoker, "bingpo", QList<ServerPlayer *>(), QString::number(damage.damage));
        room->notifySkillInvoked(invoke->invoker, objectName());
        return true;
    }
};




class Zhenye : public TriggerSkill
{
public:
    Zhenye() : TriggerSkill("zhenye")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *nokia = data.value<ServerPlayer *>();
        if (nokia->hasSkill(this) && nokia->getPhase() == Player::Finish)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, nokia, nokia);

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(), "@zhenye-select", true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->targets.first()->turnOver();
        invoke->invoker->turnOver();

        return false;
    }
};

class Anyu : public TriggerSkill
{
public:
    Anyu() : TriggerSkill("anyu")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.card || !damage.card->isBlack())
            return QList<SkillInvokeDetail>();

        if (damage.to->isAlive() && damage.to->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        //room->touhouLogmessage("#TriggerSkill", invoke->invoker, "anyu");
        QString choice = room->askForChoice(invoke->invoker, objectName(), "turnover+draw", data);
        //room->notifySkillInvoked(invoke->invoker, objectName());
        if (choice == "turnover")
            invoke->invoker->turnOver();
        else
            invoke->invoker->drawCards(1);

        return false;
    }
};



class Qiyue : public TriggerSkill
{
public:
    Qiyue() : TriggerSkill("qiyue")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->getPhase() != Player::Start)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->getOtherPlayers(current)) {
            if (p->hasSkill(this))
                d << SkillInvokeDetail(this, p, p, NULL, false, current);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QString prompt = "target:" + invoke->preferredTarget->objectName();
        if (invoke->invoker->askForSkillInvoke(this, prompt)) {
            invoke->invoker->drawCards(1, objectName());
            QString choice = room->askForChoice(invoke->invoker, objectName(), "hp+maxhp", data);
            (choice == "hp") ? room->loseHp(invoke->invoker) : room->loseMaxHp(invoke->invoker);

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->targets.first()->skip(Player::Judge);
        invoke->targets.first()->skip(Player::Draw);

        return false;
    }
};


class Moxue : public TriggerSkill
{
public:
    Moxue() : TriggerSkill("moxue")
    {
        events << MaxHpChanged;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getMaxHp() == 1)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->doLightbox("$moxueAnimate", 4000);
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, "moxue");
        room->notifySkillInvoked(invoke->invoker, objectName());
        invoke->invoker->drawCards(qMax(invoke->invoker->getHandcardNum(), 1));
        return false;
    }
};



class Juxian : public TriggerSkill
{
public:
    Juxian() : TriggerSkill("juxian")
    {
        events << Dying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DyingStruct dying = data.value<DyingStruct>();

        if (dying.who->hasSkill(this) && dying.who->faceUp())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dying.who, dying.who);

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (invoke->invoker->askForSkillInvoke(this, data)) {
            invoke->invoker->turnOver();
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<int> list = room->getNCards(3);
        CardsMoveStruct move(list, NULL, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, invoke->invoker->objectName(), objectName(), QString()));
        room->moveCardsAtomic(move, true);

        QVariantList listc = IntList2VariantList(list);
        invoke->invoker->tag["juxian_cards"] = listc;
        Card::Suit suit = room->askForSuit(invoke->invoker, objectName());
        invoke->invoker->tag.remove("juxian_cards");

        room->touhouLogmessage("#ChooseSuit", invoke->invoker, Card::Suit2String(suit));

        QList<int> get;
        QList<int> thro;
        foreach (int id, list) {
            if (Sanguosha->getCard(id)->getSuit() != suit)
                get << id;
            else
                thro << id;
        }

        if (!get.isEmpty()) {
            DummyCard dummy(get);
            invoke->invoker->obtainCard(&dummy);
        }
        if (!thro.isEmpty()) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, invoke->invoker->objectName(), objectName(), QString());
            DummyCard dummy(thro);
            room->throwCard(&dummy, reason, NULL);
            RecoverStruct recover;
            recover.recover = thro.length();
            room->recover(invoke->invoker, recover);
        }
        return false;
    }
};

BanyueCard::BanyueCard()
{
}

bool BanyueCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return (targets.length() < 3);
}

void BanyueCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const // onEffect is better?
{
    // the loseHp here is actually cost.
    room->loseHp(source);
    foreach (ServerPlayer *p, targets) {
        if (p->isAlive())
            p->drawCards(1);
    }
}

class Banyue : public ZeroCardViewAsSkill
{
public:
    Banyue() : ZeroCardViewAsSkill("banyue")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("BanyueCard");
    }

    virtual const Card *viewAs() const
    {
        return new BanyueCard;
    }
};




class Mizong : public TriggerSkill
{
public:
    Mizong() : TriggerSkill("mizong")
    {
        events << EventPhaseStart << FinishJudge << EventPhaseChanging;
        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && change.player->hasFlag("mizong")) {
                foreach(ServerPlayer *p, room->getOtherPlayers(change.player)) {
                    if (p->getMark("@mizong") > 0) {
                        room->setPlayerMark(p, "@mizong", 0);
                        room->setFixedDistance(change.player, p, -1);
                        
                        QStringList assignee_list = change.player->property("extra_slash_specific_assignee").toString().split("+");
                        assignee_list.removeOne(p->objectName());
                        room->setPlayerProperty(change.player, "extra_slash_specific_assignee", assignee_list.join("+"));
                    }
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName() && judge->card->isRed())
                judge->pattern = "red";
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *, const QVariant &data) const
    {
        if (event == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (!player->hasSkill(this) || player->isDead() || player->getPhase() != Player::Play)
                return QList<SkillInvokeDetail>();
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        } else if (event == FinishJudge) {
            JudgeStruct * judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName() && judge->card->isBlack()) {
                if (judge->who->isAlive())
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, judge->who, judge->who, NULL, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }


    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart) {
            room->notifySkillInvoked(invoke->owner, objectName());
            ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(), "@mizong-ask", false, true);
            if (target != NULL)
                invoke->targets << target;
            return target != NULL;
        } else
            return true;
        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            JudgeStruct judge;
            judge.who = invoke->invoker;
            judge.reason = objectName();
            judge.good = true;
            judge.play_animation = false;
            room->judge(judge);

            if (judge.pattern == "red") {
                room->setPlayerFlag(invoke->invoker, "mizong");
                room->setFixedDistance(invoke->invoker, invoke->targets.first(), 1);
                invoke->targets.first()->gainMark("@mizong"); //gainMark could trigger skill Ganying

                QStringList assignee_list = invoke->invoker->property("extra_slash_specific_assignee").toString().split("+");
                assignee_list << invoke->targets.first()->objectName();
                room->setPlayerProperty(invoke->invoker, "extra_slash_specific_assignee", assignee_list.join("+"));
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct * judge = data.value<JudgeStruct *>();
            invoke->owner->obtainCard(judge->card);
        }
        return false;
    }
};

class Yinren : public TriggerSkill
{
public:
    Yinren() : TriggerSkill("yinren")
    {
        events << EventPhaseChanging << TargetSpecified;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("yinren")) {
                        p->setFlags("-yinren");
                        room->setPlayerSkillInvalidity(p, NULL, false);
                        room->removePlayerCardLimitation(p, "use,response", ".|red|.|.$1");
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *, const QVariant &data) const
    {
        if (event != TargetSpecified)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from && use.from->hasSkill(this) && use.card->isKindOf("Slash") && use.card->isBlack()) {
            foreach(ServerPlayer *p, use.to)
                d << SkillInvokeDetail(this, use.from, use.from, NULL, false, p);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->tag["yinren-target"] = QVariant::fromValue(invoke->preferredTarget);
        return invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget));
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (!invoke->targets.first()->hasFlag(objectName())) {
            invoke->targets.first()->setFlags(objectName());
            room->setPlayerSkillInvalidity(invoke->targets.first(), NULL, true);
            QString pattern = ".|red|.|.";
            room->setPlayerCardLimitation(invoke->targets.first(), "use,response", pattern, true);
        }
        return false;
    }
};


TH06Package::TH06Package()
    : Package("th06")
{
    General *remilia = new General(this, "remilia$", "hmx", 3, false);
    remilia->addSkill(new SkltKexue);
    remilia->addSkill(new Mingyun);
    remilia->addSkill(new Xueyi);

    General *flandre = new General(this, "flandre", "hmx", 3, false);
    flandre->addSkill(new Pohuai);
    flandre->addSkill(new Yuxue);
    flandre->addSkill(new YuxueSlashNdl);
    flandre->addSkill(new Shengyan);
    related_skills.insertMulti("yuxue", "#yuxue-slash-ndl");

    General *sakuya = new General(this, "sakuya", "hmx", 4, false);
    sakuya->addSkill(new Suoding);
    sakuya->addSkill(new Huisu);

    General *patchouli = new General(this, "patchouli", "hmx", 3, false);
    patchouli->addSkill(new Bolan);
    patchouli->addSkill(new Hezhou);

    General *meirin = new General(this, "meirin", "hmx", 4, false);
    meirin->addSkill(new Taiji);
    meirin->addSkill(new Beishui);

    General *cirno = new General(this, "cirno", "hmx", 3, false);
    cirno->addSkill(new Dongjie);
    cirno->addSkill(new Bingpo);

    General *rumia = new General(this, "rumia", "hmx", 3, false);
    rumia->addSkill(new Zhenye);
    rumia->addSkill(new Anyu);

    General *koakuma = new General(this, "koakuma", "hmx", 3, false);
    koakuma->addSkill(new Qiyue);
    koakuma->addSkill(new Moxue);

    General *daiyousei = new General(this, "daiyousei", "hmx", 3, false);
    daiyousei->addSkill(new Juxian);
    daiyousei->addSkill(new Banyue);

    General *sakuya_sp = new General(this, "sakuya_sp", "hmx", 4, false);
    sakuya_sp->addSkill(new Mizong);
    sakuya_sp->addSkill(new Yinren);

    addMetaObject<SkltKexueCard>();
    addMetaObject<SuodingCard>();
    addMetaObject<BeishuiCard>();
    addMetaObject<BanyueCard>();

    skills << new SkltKexueVS;
}

ADD_PACKAGE(TH06)

