#include "th01-05.h"
#include "general.h"
#include "skill.h"
#include "engine.h"
#include "maneuvering.h"

#include "clientplayer.h"

class Eling : public TriggerSkill
{
public:
    Eling() : TriggerSkill("eling")
    {
        events << Predamage;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Normal)
            return QList<SkillInvokeDetail>();
        QList<SkillInvokeDetail> d;
        if (damage.from && damage.from->hasSkill(this))
            d << SkillInvokeDetail(this, damage.from, damage.from);
        if (damage.to && damage.to->hasSkill(this))
            d << SkillInvokeDetail(this, damage.to, damage.to);

        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        JudgeStruct judge;
        judge.reason = objectName();
        judge.who = invoke->invoker;
        judge.good = false;
        judge.pattern = ".|heart";

        room->judge(judge);

        if (judge.isGood()) {
            damage.nature = DamageStruct::Thunder;
            data = QVariant::fromValue(damage);
        }
        return false;
    }

};

class Xieqi : public TriggerSkill
{
public:
    Xieqi() : TriggerSkill("xieqi")
    {
        events << Damage;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.card || damage.from == NULL || damage.nature == DamageStruct::Normal)
            return QList<SkillInvokeDetail>();

        QList<int> ids;
        if (damage.card->isVirtualCard())
            ids = damage.card->getSubcards();
        else
            ids << damage.card->getEffectiveId();

        if (ids.isEmpty()) return QList<SkillInvokeDetail>();
        foreach(int id, ids) {
            if (room->getCardPlace(id) != Player::PlaceTable) return QList<SkillInvokeDetail>();
        }
        QList<SkillInvokeDetail> d;
        foreach(ServerPlayer *mima, room->findPlayersBySkillName(objectName()))
                d << SkillInvokeDetail(this, mima, mima);
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (invoke->invoker == damage.from) {
            ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(damage.from), objectName(), "@xieqi:" + damage.card->objectName(), true, true);
            if (target != NULL)
                invoke->targets << target;
            return target != NULL;
        } else
            return invoke->invoker->askForSkillInvoke(this, data);
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (invoke->invoker == damage.from)
            invoke->targets.first()->obtainCard(damage.card);
        else
            invoke->invoker->obtainCard(damage.card);
        return false;
    }

};

class Fuchou : public TriggerSkill
{
public:
    Fuchou() : TriggerSkill("fuchou$")
    {
        frequency = Compulsory;
        events << EventPhaseStart;
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (!current || current->getKingdom() != "pc98" || current->getPhase() != Player::Play)
            return QList<SkillInvokeDetail>();

        QList<ServerPlayer *> rans = room->findPlayersBySkillName(objectName());

        QList<SkillInvokeDetail> d;
        foreach(ServerPlayer *p, room->getOtherPlayers(current)) {
            if (!p->hasLordSkill(objectName()))
                continue;
            foreach(ServerPlayer *t, room->getOtherPlayers(current)) {
                if (current->inMyAttackRange(t) && t->getHp() > p->getHp() && !t->isChained()) {
                    d << SkillInvokeDetail(this, p, current);
                    break;
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *t, room->getOtherPlayers(invoke->invoker)) {
            if (invoke->invoker->inMyAttackRange(t) && t->getHp() > invoke->owner->getHp() && !t->isChained()) {
                targets << t;
            }
        }
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@fuchou:" + invoke->owner->objectName());
        if (target != NULL) {
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = invoke->invoker;
            log.to << invoke->owner;
            log.arg = objectName();
            room->sendLog(log);

            invoke->targets << target;

            LogMessage log1;
            log1.type = "#ChooseFuchou";
            log1.from = invoke->invoker;
            log1.to << target;
            room->sendLog(log1);

            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), target->objectName());
        }

        return target != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (invoke->owner->askForSkillInvoke(this, QVariant::fromValue(invoke->targets.first()))) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->owner->objectName(), invoke->targets.first()->objectName());

            room->setPlayerProperty(invoke->targets.first(), "chained", true);
        }
        return false;
    }
};




class Ciyuan : public TriggerSkill
{
public:
    Ciyuan() : TriggerSkill("ciyuan")
    {
        events << EventPhaseStart;
    }

    static Player::Phase ciyuanPhase(QString choice) {
        Player::Phase phase = Player::Finish;
        if (choice == "start")
            phase = Player::Start;
        else if (choice == "judge")
            phase = Player::Judge;
        else if (choice == "draw")
            phase = Player::Draw;
        else if (choice == "play")
            phase = Player::Play;
        else if (choice == "discard")
            phase = Player::Discard;
        return phase;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->getPhase() == Player::RoundStart)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QStringList phases;
        phases << "start" << "judge" << "draw" << "play" << "discard" << "finish" << "cancel";
        QString choice = room->askForChoice(invoke->invoker, objectName(), phases.join("+"));
        if (choice != "cancel") {
            phases.removeOne("cancel");
            phases.removeOne(choice);
            QString choice1 = room->askForChoice(invoke->invoker, objectName(), phases.join("+"));
            invoke->invoker->tag["ciyuan"] = QVariant::fromValue(choice);
            invoke->invoker->tag["ciyuan1"] = QVariant::fromValue(choice1);
        }

        return choice != "cancel";
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {


        QString choice = invoke->invoker->tag["ciyuan"].toString();
        Player::Phase phase = ciyuanPhase(choice);
        QString choice1 = invoke->invoker->tag["ciyuan1"].toString();
        Player::Phase phase1 = ciyuanPhase(choice1);

        room->notifySkillInvoked(invoke->owner, objectName());
        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = invoke->invoker;
        log.arg = objectName();
        room->sendLog(log);

        log.type = "#ciyuan";
        log.from = invoke->invoker;
        log.arg = "ciyuan:"+choice;
        log.arg2 = "ciyuan:" + choice1;
        room->sendLog(log);

        invoke->invoker->exchangePhases(phase, phase1);
        return false;
    }

};

class Shigui : public TriggerSkill
{
public:
    Shigui() : TriggerSkill("shigui")
    {
        events << EventPhaseStart << EventPhaseEnd << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            Player::Phase phase = current->getPhase();
            QList<Player::Phase> phases;
            phases << Player::Start << Player::Judge << Player::Draw << Player::Play
                << Player::Discard << Player::Finish;
            if (phases.contains(phase))
                room->setPlayerMark(current, "shigui", current->getMark("shigui") + 1);
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                room->setPlayerMark(change.player, "shigui", 0);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current->hasSkill(this) && current->getPhase() == Player::Draw && current->getHandcardNum() < current->getMark("shigui"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current);
            else if (current->hasSkill(this) && current->getPhase() == Player::Play && current->getHandcardNum() > current->getMark("shigui"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString skillname = invoke->invoker->getPhase() == Player::Draw ? "draw" : "play";
        QString prompt = skillname + "_notice:" + QString::number(invoke->invoker->getMark("shigui"));
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *current = invoke->invoker;
        if (current->getPhase() == Player::Draw) {
            current->drawCards(current->getMark("shigui") - current->getHandcardNum());
            room->loseHp(current);
        } else if (current->getPhase() == Player::Play) {
            int x = current->getHandcardNum() - current->getMark("shigui");
            room->askForDiscard(invoke->invoker, objectName(), x, x, false, false, "shigui_discard:" + QString::number(x));
            RecoverStruct recover;
            room->recover(invoke->invoker, recover);
        }
        return false;
    }

};




class ZhenceVS : public ZeroCardViewAsSkill
{
public:
    ZhenceVS() : ZeroCardViewAsSkill("zhence")
    {
        response_pattern = "@@zhence";
    }


    virtual const Card *viewAs() const
    {
        FireAttack *fire = new FireAttack(Card::NoSuit, 0);
        fire->setSkillName("zhence");
        return fire;
    }
};

class Zhence : public TriggerSkill
{
public:
    Zhence() : TriggerSkill("zhence")
    {
        events << EventPhaseChanging << DamageCaused;
        view_as_skill = new ZhenceVS;
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->hasSkill(this) && (change.to == Player::Draw || change.to == Player::Play) && !change.player->isSkipped(change.to)) {
                FireAttack *fire = new FireAttack(Card::NoSuit, 0);
                fire->deleteLater();
                if (!change.player->isCardLimited(fire, Card::MethodUse))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player);
            }
        } else if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive() && damage.card && damage.card->getSkillName() == "zhence")
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            invoke->invoker->tag["zhence"] = data;
            QString prompt;
            if (change.to == Player::Draw)
                prompt = "@zhence:draw";
            if (change.to == Player::Play)
                prompt = "@zhence:play";
            room->askForUseCard(invoke->invoker, "@@zhence", prompt);
            return false;
        }
        return true;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1);
        PhaseChangeStruct change = invoke->invoker->tag["zhence"].value<PhaseChangeStruct>();
        invoke->invoker->skip(change.to);
        return false;
    }

};

class Shiqu : public TriggerSkill
{
public:
    Shiqu() : TriggerSkill("shiqu")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    static QStringList shiquChoices(ServerPlayer *player)
    {
        QStringList marks;
        QStringList choices;
        marks << "shiqu_start" << "shiqu_judge" << "shiqu_draw" << "shiqu_play" << "shiqu_discard";
        foreach(QString m, marks) {
            if (player->getMark(m) == 0)
                choices << m;
        }
        return choices;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            switch (current->getPhase()) {
            case Player::Start:
            {
                room->setPlayerMark(current, "shiqu_start", 1);
                break;
            }
            case Player::Judge:
            {
                room->setPlayerMark(current, "shiqu_judge", 1);
                break;
            }
            case Player::Draw:
            {
                room->setPlayerMark(current, "shiqu_draw", 1);
                break;
            }
            case Player::Play:
            {
                room->setPlayerMark(current, "shiqu_play", 1);
                break;
            }
            case Player::Discard:
            {
                room->setPlayerMark(current, "shiqu_discard", 1);
                break;
            }
            default:
                break;
            }


        }
        else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                room->setPlayerMark(change.player, "shiqu_start", 0);
                room->setPlayerMark(change.player, "shiqu_judge", 0);
                room->setPlayerMark(change.player, "shiqu_draw", 0);
                room->setPlayerMark(change.player, "shiqu_play", 0);
                room->setPlayerMark(change.player, "shiqu_discard", 0);
            }
        }
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current->getPhase() == Player::Finish && !current->tag.value("touhou-extra", false).toBool() && !room->getThread()->hasExtraTurn()) {
                QStringList choices = shiquChoices(current);
                if (choices.length() > 0) {
                    foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                        if (p->canDiscard(p, "hes"))
                            d << SkillInvokeDetail(this, p, p, NULL, false, current);
                    }
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QStringList choices = shiquChoices(invoke->preferredTarget);
        choices << "cancel";
        QString choice = room->askForChoice(invoke->invoker, objectName(), choices.join("+"));

        if (choice != "cancel") {
            QString prompt = "@shiqu-discard:" + choice + ":" + invoke->preferredTarget->objectName();
            invoke->invoker->tag["shiqu"] = QVariant::fromValue(choice);
            const Card *card = room->askForCard(invoke->invoker, ".|.|.|hand,equipped", prompt, data, objectName());
            return card != NULL;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString choice = invoke->invoker->tag["shiqu"].toString();
        Player::Phase phase;
        if (choice == "shiqu_start")
            phase = Player::Start;
        else if (choice == "shiqu_judge")
            phase = Player::Judge;
        else if (choice == "shiqu_draw")
            phase = Player::Draw;
        else if (choice == "shiqu_play")
            phase = Player::Play;
        else if (choice == "shiqu_discard")
            phase = Player::Discard;

        //invoke->targets.first()->skip(Player::Finish);
        room->notifySkillInvoked(invoke->owner, objectName());
        LogMessage log;
        log.type = "#shiqu";
        log.from = invoke->invoker;
        log.to << invoke->targets.first();
        log.arg = objectName();
        log.arg2 = choice;
        room->sendLog(log);

        ExtraTurnStruct extra;
        extra.player = invoke->targets.first();
        extra.set_phases << Player::RoundStart << phase << Player::NotActive;
        room->setTag("ExtraTurnStruct", QVariant::fromValue(extra));
        invoke->targets.first()->gainAnExtraTurn();

        /*
        invoke->targets.first()->setPhase(phase);
        room->broadcastProperty(invoke->targets.first(), "phase");
        RoomThread *thread = room->getThread();
        QVariant t = QVariant::fromValue(invoke->targets.first());
        if (!thread->trigger(EventPhaseStart, room, t))
            thread->trigger(EventPhaseProceeding, room, t);
        thread->trigger(EventPhaseEnd, room, t);
        */
        //invoke->targets.first()->setPhase(Player::??);
        //room->broadcastProperty(invoke->targets.first(), "phase");
        return false;
    }
};





class Jiexi : public TriggerSkill
{
public:
    Jiexi() : TriggerSkill("jiexi")
    {
        events << CardUsed << CardResponded << EventPhaseChanging << EventLoseSkill << EventAcquireSkill << EventSkillInvalidityChange;
        frequency = Compulsory;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        ServerPlayer *current = room->getCurrent();
        if (current == NULL || current->isDead())
            return;
        //we need a visibile mark for noticing player, and unvisibile mark for processing pingyi 
        if (e == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Jink") || use.card->isKindOf("Nullification")) {
                room->setPlayerMark(current, objectName(), current->getMark(objectName()) + 1);
                if (current->hasSkill("jiexi"))
                    room->setPlayerMark(current, "@jiexi", current->getMark(objectName()));
            }
        } else if (e == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse && (resp.m_card->isKindOf("Jink") || resp.m_card->isKindOf("Nullification"))) {
                room->setPlayerMark(current, objectName(), current->getMark(objectName()) + 1);
                if (current->hasSkill("jiexi"))
                    room->setPlayerMark(current, "@jiexi", current->getMark(objectName()));
            }
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && current->getMark(objectName()) > 0) {
                room->setPlayerMark(current, objectName(), 0);
                room->setPlayerMark(current, "@jiexi", 0);
            }
        } else if (e == EventLoseSkill || e == EventAcquireSkill || e == EventSkillInvalidityChange) {
            if (current->hasSkill("jiexi"))
                room->setPlayerMark(current, "@jiexi", current->getMark(objectName()));
            else
                room->setPlayerMark(current, "@jiexi", 0);
        }
    }
};


class JiexiTargetMod : public TargetModSkill
{
public:
    JiexiTargetMod() : TargetModSkill("#jiexi_mod")
    {
        frequency = NotFrequent;
        pattern = "Slash";
    }

    virtual int getExtraTargetNum(const Player *player, const Card *) const
    {
        if (player->hasSkill(objectName()) && player->getPhase() == Player::Play )
            return player->getMark("jiexi");
        else
            return 0;
    }

    virtual int getResidueNum(const Player *from, const Card *) const
    {
        if (from->hasSkill(objectName()) && from->getPhase() == Player::Play)
            return from->getMark("jiexi");
        else
            return 0;
    }
};

class JiexiAttackRange : public AttackRangeSkill
{
public:
    JiexiAttackRange() : AttackRangeSkill("#jiexi_range")
    {

    }

    virtual int getExtra(const Player *target, bool) const
    {
        if (target->hasSkill(objectName()) && target->getPhase() == Player::Play)
            return target->getMark("jiexi");
        return 0;
    }
};




class Qianyi : public TriggerSkill
{
public:
    Qianyi() : TriggerSkill("qianyi")
    {
        events << EventPhaseStart;
    }

    static Player::Phase qianyiPhase(QString choice) {
        if (choice == "start")
            return Player::Start;
        else if (choice == "judge")
            return Player::Judge;
        else if (choice == "draw")
            return Player::Draw;
        else if (choice == "play")
            return Player::Play;
        else if (choice == "discard")
            return Player::Discard;
        else if (choice == "finish")
            return Player::Finish;
        return Player::Finish;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->getPhase() == Player::RoundStart) {
            QList<SkillInvokeDetail> d;
            foreach(ServerPlayer *p, room->getOtherPlayers(current)) {
                if (p->hasSkill(this) && !p->getCards("e").isEmpty())
                    d << SkillInvokeDetail(this, p, p, NULL, false, current);
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *target = invoke->preferredTarget;
        QString prompt = "@qianyi:" + target->objectName();
        const Card *card = room->askForCard(invoke->invoker, ".|.|.|equipped", prompt, data, Card::MethodNone);
        if (card) {
            const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
            int equipped_id = -1;
            if (target->getEquip(equip->location()) != NULL)
                equipped_id = target->getEquip(equip->location())->getEffectiveId();
            QList<CardsMoveStruct> exchangeMove;
            CardsMoveStruct move1(equip->getId(), target, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_TRANSFER, target->objectName()));

            exchangeMove.push_back(move1);
            if (equipped_id != -1) {
                CardsMoveStruct move2(equipped_id, NULL, Player::DiscardPile, CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, target->objectName()));
                exchangeMove.push_back(move2);
            }

            LogMessage zhijian;
            zhijian.type = "$ZhijianEquip";
            zhijian.from = target;
            zhijian.card_str = QString::number(equip->getId());
            room->sendLog(zhijian);

            room->moveCardsAtomic(exchangeMove, true);
        }
        return card != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        RecoverStruct recover;
        room->recover(invoke->invoker, recover);
        QString choice = room->askForChoice(invoke->invoker, objectName(), "start+judge+draw+play+discard+finish", data);
        Player::Phase phase = qianyiPhase(choice);
        if (!invoke->targets.first()->isSkipped(phase))
            invoke->targets.first()->skip(phase);
        return false;
    }
};

class Mengxiao : public TriggerSkill
{
public:
    Mengxiao() : TriggerSkill("mengxiao")
    {
        events << CardsMoveOneTime;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *kana = qobject_cast<ServerPlayer *>(move.from);
        if (kana != NULL && kana->hasSkill(this) && kana->isAlive() && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                foreach(const Card *c, p->getCards("ej")) {
                    foreach(int card_id, move.card_ids) {
                        if (Sanguosha->getCard(card_id)->getSuit() == c->getSuit())
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, kana, kana);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QList<ServerPlayer *> targets;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        foreach(ServerPlayer *p, room->getAllPlayers()) {
            foreach(const Card *c, p->getCards("ej")) {
                if (targets.contains(p))
                    continue;
                foreach(int card_id, move.card_ids) {
                    if (Sanguosha->getCard(card_id)->getSuit() == c->getSuit()) {
                        targets << p;
                        break;
                    }
                }
            }
        }
        invoke->invoker->tag["mengxiao"] = data;
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@" + objectName(), true, true);
        if (target)
            invoke->targets << target;
        return target != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *target = invoke->targets.first();
        QList<int> disable;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        foreach(const Card *c, target->getCards("ej")) {
            bool find = false;
            foreach(int card_id, move.card_ids) {
                if (Sanguosha->getCard(card_id)->getSuit() == c->getSuit()) {
                    find = true;
                    break;
                }
            }
            if (!find)
                disable << c->getId();
        }

        int card_id = room->askForCardChosen(invoke->invoker, target, "je", objectName(), false, Card::MethodNone, disable);
        room->obtainCard(invoke->invoker, card_id);

        return false;
    }

};



class Youyue : public TriggerSkill
{
public:
    Youyue() : TriggerSkill("youyue")
    {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from && use.from->hasSkill(this) && use.from->isAlive() && (use.card->isNDTrick() || use.card->isKindOf("Slash"))) {

            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *to, use.to) {
                if (to->isAlive() && to->canDiscard(to, "hs") && use.from != to)
                    targets << to;
            }
            if (!targets.isEmpty())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, targets, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();

        QList<ServerPlayer *> logto;
        logto << invoke->targets;
        room->touhouLogmessage("#youyue", invoke->invoker, objectName(), logto, use.card->objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        foreach(ServerPlayer *to, invoke->targets) {
            if (!to->canDiscard(to, "hs"))
                continue;

            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), to->objectName());
            QStringList prompt_list;
            prompt_list << "youyue-discard" << use.card->objectName()
                << invoke->invoker->objectName();
            QString prompt = prompt_list.join(":");
            const Card *card = room->askForCard(to, ".!", prompt, data, Card::MethodDiscard);
            if (!card) {
                // force discard!!!
                QList<const Card *> hc = to->getHandcards();
                foreach(const Card *c, hc) {
                    if (to->isJilei(c))
                        hc.removeOne(c);
                }

                if (hc.length() == 0)
                    return false;


                int x = qrand() % hc.length();
                const Card *c = hc.value(x);
                card = c;
                room->throwCard(c, to);
            }


            if (invoke->invoker->isKongcheng()) {
                use.nullified_list << to->objectName();
                continue;
            }

            //show card with the same type
            QString type = card->getType();
            QString pattern = QString("%1Card|.|.|hand").arg(type.left(1).toUpper() + type.right(type.length() - 1));
            QStringList prompt_list1;
            prompt_list1 << "youyue-show" << use.card->objectName()
                << to->objectName() << card->getType();
            QString prompt1 = prompt_list1.join(":");
            invoke->invoker->tag["youyue_target"] = QVariant::fromValue(to);
            invoke->invoker->tag["youyue_card"] = QVariant::fromValue(card);

            const Card *card1 = room->askForCard(invoke->invoker, pattern, prompt1, data, Card::MethodNone);
            if (card1)
                room->showCard(invoke->invoker, card1->getEffectiveId());
            else
                use.nullified_list << to->objectName();
        }
        data = QVariant::fromValue(use);

        return false;
    }

};

class Yeyan : public TriggerSkill
{
public:
    Yeyan() : TriggerSkill("yeyan")
    {
        events << EventPhaseEnd;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *yuka = data.value<ServerPlayer *>();
        if (!yuka->hasSkill(this) || yuka->isDead() || yuka->getPhase() != Player::Play)
            return QList<SkillInvokeDetail>();

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->isKongcheng())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, yuka, yuka);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        int n = 0;
        foreach(ServerPlayer *p, room->getAllPlayers()) {
            if (p->isKongcheng())
                n++;
        }
        invoke->invoker->drawCards(n);
        return false;
    }

};

/*
MenghuanCard::MenghuanCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "menghuan_attach";
}

void MenghuanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *lord = targets.first();
    if (lord->hasLordSkill("menghuan")) {
        room->setPlayerFlag(lord, "menghuanInvoked");

        room->notifySkillInvoked(lord, "menghuan");
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), lord->objectName(), "menghuan", QString());
        room->obtainCard(lord, this, reason);
        QList<ServerPlayer *> lords;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach(ServerPlayer *p, players) {
            if (p->hasLordSkill("menghuan") && !p->hasFlag("menghuanInvoked"))
                lords << p;
        }
        if (lords.isEmpty())
            room->setPlayerFlag(source, "Forbidmenghuan");
    }
}

bool MenghuanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasLordSkill("menghuan")
        && to_select != Self && !to_select->hasFlag("menghuanInvoked");
}

class MenghuanVS : public OneCardViewAsSkill
{
public:
    MenghuanVS() :OneCardViewAsSkill("menghuan_attach")
    {
        attached_lord_skill = true;
        filter_pattern = "TrickCard";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return  !player->hasFlag("Forbidmenghuan") && shouldBeVisible(player);
    }

    virtual bool shouldBeVisible(const Player *Self) const
    {
        return Self && Self->getKingdom() == "pc98";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        MenghuanCard *card = new MenghuanCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class Menghuan : public TriggerSkill
{
public:
    Menghuan() : TriggerSkill("menghuan$")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging << Revive;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent != EventPhaseChanging) { //the case operating attach skill
            static QString attachName = "menghuan_attach";
            QList<ServerPlayer *> lords;
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasLordSkill(this, true))
                    lords << p;
            }

            if (lords.length() > 1) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->hasSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            }
            else if (lords.length() == 1) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasLordSkill(this, true) && p->hasSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                    else if (!p->hasLordSkill(this, true) && !p->hasSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            }
            else {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                }
            }
        }
        else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from == Player::Play) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("menghuanInvoked"))
                        room->setPlayerFlag(p, "-menghuanInvoked");
                    if (p->hasFlag("Forbidmenghuan"))
                        room->setPlayerFlag(p, "-Forbidmenghuan");
                }
            }
        }
    }

};
*/

class Menghuan : public MaxCardsSkill
{
public:
    Menghuan() : MaxCardsSkill("menghuan$")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        int correct = 0;
        if (target->hasLordSkill("menghuan")) {
            foreach(const Player *p, target->getAliveSiblings()) {
                if (p->getKingdom() == "pc98" && p->getHp() > correct)
                    correct = p->getHp();
            }
        }
        return correct;
    }
};



class HuantongVS : public ViewAsSkill
{
public:
    HuantongVS() : ViewAsSkill("huantong")
    {
        response_pattern = "@@huantong";
        expand_pile = "dream";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return Self->getPile("dream").contains(to_select->getEffectiveId()) && selected.length() < 2;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2){
            DummyCard *dc = new DummyCard;
            dc->addSubcards(cards);
            return dc;
        }
        return NULL;
    }
};

class Huantong : public TriggerSkill
{
public:
    Huantong() : TriggerSkill("huantong")
    {
        events << DamageInflicted;
        view_as_skill = new HuantongVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.damage != 1)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p->inMyAttackRange(damage.to))
                d << SkillInvokeDetail(this, p, p, NULL, false, damage.to);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget))) {
            if (invoke->invoker->getPile("dream").length() >= 2) {
                const Card *c = room->askForCard(invoke->invoker, "@@huantong", "@huantong:"+ invoke->preferredTarget->objectName(), data, Card::MethodNone, NULL, false, objectName());
                if (c) {
                    QVariantList ids;
                    foreach(int card_id, c->getSubcards())
                        ids << card_id;
                    invoke->invoker->tag["huantong"] = ids;
                }
            }
            return true;
        }
        return false;
    }


    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QVariantList ids = invoke->invoker->tag["huantong"].toList();
        invoke->invoker->tag.remove("huantong");
        if (ids.isEmpty()) {
            CardMoveReason reason(CardMoveReason::S_REASON_UNKNOWN, "", NULL, "dream", "");
            invoke->invoker->addToPile("dream", room->getNCards(1), false, reason);
        } else {
            DamageStruct damage = data.value<DamageStruct>();
            
            QList<int> get_ids;
            QList<int> throw_ids;
            foreach(QVariant card_data, ids) {
                int id = card_data.toInt();
                room->showCard(invoke->invoker, id);
                if (Sanguosha->getCard(id)->isKindOf("BasicCard"))
                    get_ids << id;
                else
                    throw_ids << id;
            }
            if (!get_ids.isEmpty()) {
                DummyCard dummy(get_ids);
                damage.to->obtainCard(&dummy);
            }

            if (!throw_ids.isEmpty()) {
                DummyCard dummy(throw_ids);
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
                room->throwCard(&dummy, reason, NULL);
            }

            
            LogMessage log;
            log.type = "#HuantongDamage";
            log.from = damage.to;
            log.arg = objectName();
            log.arg2 = QString::number(get_ids.length());
            room->sendLog(log);
            
            if (get_ids.isEmpty())
                return true;
            damage.damage = get_ids.length();
            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class Mengyan : public TriggerSkill
{
public:
    Mengyan() : TriggerSkill("mengyan")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;
        if (damage.damage > 1) {
            foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->isWounded())
                    d << SkillInvokeDetail(this, p, p, NULL);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        RecoverStruct recover;
        room->recover(invoke->invoker, recover);
        
        CardsMoveStruct move;
        move.card_ids = invoke->invoker->getPile("dream");
        move.to_place = Player::PlaceHand;
        move.to = invoke->invoker;
        room->moveCardsAtomic(move, false);

        return false;
    }
};


LianmuCard::LianmuCard()
{
    will_throw = false;
}

bool LianmuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *card = new Slash(Card::NoSuit, 0);
    card->deleteLater();
    card->setSkillName("lianmu");
    return card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool LianmuCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Slash *card = new Slash(Card::NoSuit, 0);
    card->deleteLater();
    card->setSkillName("lianmu");
    return card->targetsFeasible(targets, Self);
}

const Card *LianmuCard::validate(CardUseStruct &use) const
{
    Slash *card = new Slash(Card::NoSuit, 0);
    card->setSkillName("lianmu");
    use.from->getRoom()->setPlayerFlag(use.from, "lianmu_used");
    return card;
}

class LianmuVS : public ZeroCardViewAsSkill
{
public:
    LianmuVS() : ZeroCardViewAsSkill("lianmu")
    {
        response_pattern = "@@lianmu";
    }

    virtual const Card *viewAs() const
    {
        LianmuCard *slash = new LianmuCard;
        return slash;
    }
};

class Lianmu : public TriggerSkill
{
public:
    Lianmu() : TriggerSkill("lianmu")
    {
        events << DamageDone << CardFinished << EventPhaseChanging;
        view_as_skill = new LianmuVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash"))
                room->setCardFlag(damage.card, "lianmu_damage");
        }

        if (triggerEvent == EventPhaseChanging) {
            foreach(ServerPlayer *p, room->getAllPlayers())
                room->setPlayerFlag(p, "-lianmu_used");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.from->hasSkill(this) && !use.from->hasFlag("lianmu_used") 
                && use.card->isKindOf("Slash") && !use.card->hasFlag("lianmu_damage"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->askForUseCard(invoke->invoker, "@@lianmu", "@lianmu");
        return false;
    }
};

class LianmuTargetMod : public TargetModSkill
{
public:
    LianmuTargetMod() : TargetModSkill("#lianmu_mod")
    {
        frequency = NotFrequent;
        pattern = "Slash";
    }

    virtual int getExtraTargetNum(const Player *, const Card *card) const
    {
        if (card->getSkillName() == "lianmu")
            return 1;
        else
            return 0;
    }

    int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->getSkillName() == "lianmu")
            return 1000;

        return 0;
    }
};



class HuanweiEffect : public TriggerSkill
{
public:
    HuanweiEffect() : TriggerSkill("#huanwei")
    {
        events << EventAcquireSkill << EventLoseSkill << GameStart << EventSkillInvalidityChange << EventPhaseStart << DamageCaused;

        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == GameStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player && player->hasSkill("huanwei"))
                room->filterCards(player, player->getCards("hes"), true);
        }
        if (triggerEvent == EventLoseSkill || triggerEvent == EventAcquireSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            if (a.skill->objectName() == "huanwei")
                room->filterCards(a.player, a.player->getCards("hes"), true);
        }
       
        if (triggerEvent == EventSkillInvalidityChange) {
            QList<SkillInvalidStruct>invalids = data.value<QList<SkillInvalidStruct>>();
            foreach(SkillInvalidStruct v, invalids) {
                if (!v.skill || v.skill->objectName() == "huanwei") {
                    room->filterCards(v.player, v.player->getCards("hes"), true);
                    //if (!v.invalid)
                }
            }
        }
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && (player->getPhase() == Player::RoundStart || player->getPhase() == Player::NotActive))
                room->filterCards(player, player->getCards("hes"), true);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.chain || damage.transfer || !damage.by_user || !damage.from || !damage.from->hasSkill("huanwei") || !damage.from->isCurrent())
                return QList<SkillInvokeDetail>();
            if (damage.card->isKindOf("Slash") && damage.card->getSuit() == Card::Spade)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        damage.damage = damage.damage - 1;
        QList<ServerPlayer *> logto;
        logto << damage.to;
        room->touhouLogmessage("#HuanweiTrigger", invoke->invoker, "huanwei", logto, QString::number(1));
        room->notifySkillInvoked(invoke->invoker, "huanwei");
        data = QVariant::fromValue(damage);
        if (damage.damage == 0)
            return true;

        return false;
    }
};


class Huanwei : public FilterSkill
{
public:
    Huanwei() : FilterSkill("huanwei")
    {
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        Room *room = Sanguosha->currentRoom();
        ServerPlayer *player = room->getCardOwner(to_select->getId());
        return  player != NULL && !player->isCurrent() && to_select->getSuit() == Card::Spade && to_select->isKindOf("Slash");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
        jink->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(jink);
        return card;
    }
};



SqChuangshiCard::SqChuangshiCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool SqChuangshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return !targets.contains(to_select);
}

void SqChuangshiCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
    foreach(ServerPlayer *p, targets)
        room->setPlayerMark(p, "sqchuangshi", 1);
}

class SqChuangshiVS : public ZeroCardViewAsSkill
{
public:
    SqChuangshiVS() : ZeroCardViewAsSkill("sqchuangshi")
    {
        response_pattern = "@@sqchuangshi";
    }


    virtual const Card *viewAs() const
    {
            return new SqChuangshiCard;
    }
};

class SqChuangshi : public TriggerSkill
{
public:
    SqChuangshi() : TriggerSkill("sqchuangshi")
    {
        events << EventPhaseStart << DamageDone << EventPhaseChanging;
        view_as_skill = new SqChuangshiVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->getMark("sqchuangshi") > 0)
                room->setTag("sqchuangshiDamage", true);
        } else if (triggerEvent == EventPhaseChanging) {
            room->setTag("sqchuangshiDamage", false);
            foreach(ServerPlayer *p, room->getAllPlayers())
                room->setPlayerMark(p, "sqchuangshi", 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e == EventPhaseStart) {
             ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->isAlive() && player->getPhase() == Player::Play)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->askForUseCard(invoke->invoker, "@@sqchuangshi", "@sqchuangshi");
        foreach(ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("sqchuangshi") > 0)
                invoke->targets << p;
        }
        return !invoke->targets.isEmpty();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        foreach(ServerPlayer *p, invoke->targets) {
            QVariant damageTag = room->getTag("sqchuangshiDamage");
            bool damage = damageTag.canConvert(QVariant::Bool) && damageTag.toBool();
            if (damage)
                break;
            room->askForUseCard(p, "BasicCard+^Jink,TrickCard+^Nullification,EquipCard|.|.|sqchuangshi", "@sqchuangshi_use", -1, Card::MethodUse, false, objectName());
        }
        return false;
    }
};

class Yuanfa : public TriggerSkill
{
public:
    Yuanfa() : TriggerSkill("yuanfa")
    {
        events << EventPhaseStart << EventPhaseChanging << CardUsed << CardResponded;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach(ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerMark(p, "yuanfa", 0);
            }
        } else if (e == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("SkillCard"))
                room->setPlayerMark(use.from, "yuanfa", 1);
        } else if (e == CardResponded) {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_isUse)
                room->setPlayerMark(response.m_from, "yuanfa", 1);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e != EventPhaseStart)
            return QList<SkillInvokeDetail>();
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (!player->hasSkill(this) || player->isDead() || player->getPhase() != Player::Finish)
            return QList<SkillInvokeDetail>();

        foreach(ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("yuanfa") > 0)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        foreach(ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("yuanfa") > 0) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), p->objectName());
                p->drawCards(1);
            }
        }
        return false;
    }
};

class Shenwei : public TriggerSkill
{
public:
    Shenwei() : TriggerSkill("shenwei$")
    {
        frequency = Compulsory;
        events << CardResponded;
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer  *current = room->getCurrent();
        if (!current || !current->hasLordSkill(objectName()) || !current->isAlive())
            return QList<SkillInvokeDetail>();

        CardResponseStruct resp = data.value<CardResponseStruct>();
        int count = 0;
        if (resp.m_isUse && resp.m_from != current && resp.m_card->isKindOf("Jink")
            && resp.m_card->getSuit() == Card::Diamond) {
            foreach(ServerPlayer *p, room->getOtherPlayers(resp.m_from)) {
                if (p->getKingdom() == "pc98" && p->inMyAttackRange(resp.m_from))
                    count++;
            }
        }
        if (count >= 2)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current, NULL, true, resp.m_from);
        
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardResponseStruct resp = data.value<CardResponseStruct>();
        resp.m_isNullified = true;
        data = QVariant::fromValue(resp);

        room->notifySkillInvoked(invoke->invoker, objectName());
        room->touhouLogmessage("#shenwei", resp.m_from, objectName(), QList<ServerPlayer *>(), resp.m_card->objectName());
        return false;
    }
};




ModianCard::ModianCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "modian_attach";
}

bool ModianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->hasSkill("modian")
        && !to_select->hasFlag("modianInvoked");
}

void ModianCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *alice = targets.first();
    if (alice->hasSkill("modian")) {

        room->setPlayerFlag(alice, "modianInvoked");
        room->notifySkillInvoked(alice, "modian");

        CardMoveReason reason(CardMoveReason::S_REASON_UNKNOWN, "", NULL, "modian", "");
        alice->addToPile("modian", subcards, true, reason);

        QList<ServerPlayer *> alices;
        QList<ServerPlayer *> players = room->getAllPlayers();
        foreach(ServerPlayer *p, players) {
            if (p->hasSkill("modian") && !p->hasFlag("modianInvoked"))
                alices << p;
        }
        if (alices.isEmpty())
            room->setPlayerFlag(source, "Forbidmodian");

        if (alice->getPile("modian").length() > alice->getHp()) {
            const Card *c = room->askForCard(alice, "@@modian!", "@modian", QVariant(), Card::MethodNone, NULL, false, "modian");
            // force discard!!!
            if (c == NULL) {
                QList<int> modians = alice->getPile("modian");

                int x = qrand() % modians.length();
                int id = modians.value(x);
                c = Sanguosha->getCard(id);
            }

            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
            room->throwCard(c, reason, NULL);

            QStringList choices;
            choices << "draw";
            if (alice->isWounded())
                choices << "recover";
            QString choice = room->askForChoice(alice, "modian", choices.join("+"));
            if (choice == "draw")
                alice->drawCards(1);
            else
                room->recover(alice, RecoverStruct());
        }
    }
}

class ModianVS : public ViewAsSkill
{
public:
    ModianVS() : ViewAsSkill("modian_attach")
    {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("Forbidmodian");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return !to_select->isEquipped()  && to_select->isBlack()  && selected.length() < 2;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0) {
            ModianCard *card = new ModianCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }
};

class ModianSelfVS : public OneCardViewAsSkill
{
public:
    ModianSelfVS() : OneCardViewAsSkill("modian")
    {
        expand_pile = "modian";
        response_pattern = "@@modian!";
        //filter_pattern = ".|.|.|modian";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("Forbidmodian");
    }

    virtual bool viewFilter(const QList<const Card*> &, const Card*to_select) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY)
            return  to_select->isBlack() && !to_select->isEquipped() && !Self->getPile("modian").contains(to_select->getEffectiveId());
        else
            return  Self->getPile("modian").contains(to_select->getEffectiveId());
        return false;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            ModianCard *card = new ModianCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return originalCard;
    }
};


class Modian : public TriggerSkill
{
public:
    Modian() : TriggerSkill("modian")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging << Death << Debut << Revive;
        view_as_skill = new ModianSelfVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent != EventPhaseChanging) { //the case operating attach skill
            QList<ServerPlayer *> alices;
            static QString attachName = "modian_attach";
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(this, true))
                    alices << p;
            }

            if (alices.length() >= 1) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->hasSkill(attachName, true) && !p->hasSkill("modian"))
                        room->attachSkillToPlayer(p, attachName);
                    else if (p->hasSkill(attachName, true) && p->hasSkill("modian"))
                        room->detachSkillFromPlayer(p, attachName, true);
                }
            } else {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                }
            }

        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from == Player::Play) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("modianInvoked"))
                        room->setPlayerFlag(p, "-modianInvoked");
                    if (p->hasFlag("Forbidmodian"))
                        room->setPlayerFlag(p, "-Forbidmodian");
                }
            }
        }
    }
};


class Guaiqi : public ViewAsSkill
{
public:
    Guaiqi() : ViewAsSkill("guaiqi")
    {
        expand_pile = "modian";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("modian").isEmpty();
    }
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        foreach(int id, player->getPile("modian")) {
            if (Sanguosha->getCard(id)->isKindOf("TrickCard") &&  
                (matchAvaliablePattern("nullification", pattern) || matchAvaliablePattern(Sanguosha->getCard(id)->objectName(), pattern)))
                return true;
        }
        return false;
    }

    virtual bool viewFilter(const QList<const Card*> &selected, const Card *to_select) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            if (selected.isEmpty())
                return to_select->isKindOf("TrickCard") && Self->getPile("modian").contains(to_select->getEffectiveId());
            else if (selected.length() == 1)
                return to_select->isKindOf("Slash") && !Self->getPile("modian").contains(to_select->getEffectiveId());
        } else {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (matchAvaliablePattern("nullification", pattern)) {
                if (selected.isEmpty())
                    return Self->getPile("modian").contains(to_select->getEffectiveId());
                else if (selected.length() == 1) {
                    return selected.first()->isKindOf("Nullification") && to_select->isKindOf("Slash");
                }
                return false;
            } else {
                if (selected.isEmpty())
                    return Self->getPile("modian").contains(to_select->getEffectiveId()) && matchAvaliablePattern(to_select->objectName(), pattern);
                else if (selected.length() == 1)
                    return to_select->isKindOf("Slash") && !Self->getPile("modian").contains(to_select->getEffectiveId());
            }
        }
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            if (cards.length() != 2)
                return NULL;
            foreach(const Card *c, cards) {
                if (c->isKindOf("TrickCard")) {
                    Card *card = Sanguosha->cloneCard(c->objectName());
                    foreach(const Card *c, cards) {
                        if (c->isKindOf("Slash")) {
                            card->addSubcard(c);
                            card->setSkillName(objectName());
                            return card;
                        }
                    }
                }
                return NULL;
            }
        } else {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (matchAvaliablePattern("nullification", pattern)) {
                if (cards.length() == 1) {
                    Nullification *nul = new Nullification(Card::SuitToBeDecided, -1);
                    nul->addSubcards(cards);
                    nul->setSkillName(objectName());
                    return nul;
                } else if (cards.length() == 2) {
                    Nullification *nul = new Nullification(Card::SuitToBeDecided, -1);
                    foreach(const Card *c, cards) {
                        if (c->isKindOf("Slash"))
                            nul->addSubcard(c);
                    }
                    nul->setSkillName(objectName());
                    return nul;
                
                }
                return NULL;
            } else {
                if (cards.length() != 2)
                    return NULL;
                foreach(const Card *c, cards) {
                    if (c->isKindOf("TrickCard")) {
                        Card *card = Sanguosha->cloneCard(c->objectName());
                        foreach(const Card *c, cards) {
                            if (c->isKindOf("Slash")) {
                                card->addSubcard(c);
                                card->setSkillName(objectName());
                                return card;
                            }
                        }
                    }
                    return NULL;
                }
            }
        }
        return NULL;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        foreach(int id, player->getPile("modian")) {
            if (Sanguosha->getCard(id)->isKindOf("TrickCard")) {
                Nullification *nul = new Nullification(Card::SuitToBeDecided, -1);
                nul->addSubcard(id);
                DELETE_OVER_SCOPE(Nullification, nul)
                    
                if (!player->isCardLimited(nul, Card::MethodUse, true))
                    return true;
            }
        }
        return false;
    }
};


TH0105Package::TH0105Package()
    : Package("th0105")
{
    General *mima = new General(this, "mima$", "pc98", 4, false);
    mima->addSkill(new Eling);
    mima->addSkill(new Xieqi);
    mima->addSkill(new Fuchou);

    General *yumemi = new General(this, "yumemi$", "pc98", 4, false);
    yumemi->addSkill(new Ciyuan);
    yumemi->addSkill(new Shigui);
    yumemi->addSkill(new Skill("kongjian$", Skill::Compulsory));

    General *chiyuri = new General(this, "chiyuri", "pc98", 4, false);
    chiyuri->addSkill(new Zhence);
    chiyuri->addSkill(new Shiqu);

    General *rikako = new General(this, "rikako", "pc98", 3, false);
    //Room::_askForNullification
    rikako->addSkill(new Skill("jinfa", Skill::Compulsory));
    rikako->addSkill(new Jiexi);
    rikako->addSkill(new JiexiTargetMod);
    rikako->addSkill(new JiexiAttackRange);
    related_skills.insertMulti("jiexi", "#jiexi_mod");
    related_skills.insertMulti("jiexi", "#jiexi_range");


    General *kana = new General(this, "kana", "pc98", 3, false);
    kana->addSkill(new Qianyi);
    kana->addSkill(new Mengxiao);

    General *yuka_old = new General(this, "yuka_old$", "pc98", 4, false);
    yuka_old->addSkill(new Youyue);
    yuka_old->addSkill(new Yeyan);
    yuka_old->addSkill(new Menghuan);

    General *gengetsumugetsu = new General(this, "gengetsumugetsu", "pc98", 3, false);
    gengetsumugetsu->addSkill(new Huantong);
    gengetsumugetsu->addSkill(new Mengyan);

    General *elly = new General(this, "elly", "pc98", 4, false);
    elly->addSkill(new Lianmu);
    elly->addSkill(new LianmuTargetMod);
    elly->addSkill(new HuanweiEffect);
    elly->addSkill(new Huanwei);
    related_skills.insertMulti("lianmu", "#lianmu_mod");
    related_skills.insertMulti("huanwei", "#huanwei");

    General *shinki = new General(this, "shinki$", "pc98", 4, false);
    shinki->addSkill(new SqChuangshi);
    shinki->addSkill(new Yuanfa);
    shinki->addSkill(new Shenwei);

    General *alice_old = new General(this, "alice_old", "pc98", 3, false);
    alice_old->addSkill(new Modian);
    alice_old->addSkill(new Guaiqi);

    //addMetaObject<MenghuanCard>();
    addMetaObject<LianmuCard>();
    addMetaObject<SqChuangshiCard>();
    addMetaObject<ModianCard>();

    skills << new ModianVS; //<< new MenghuanVS;
}

ADD_PACKAGE(TH0105)
