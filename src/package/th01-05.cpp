#include "th01-05.h"
#include "general.h"
#include "skill.h"
#include "engine.h"
#include "maneuvering.h"

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
            invoke->invoker->tag["shigui"] = QVariant::fromValue(choice);
            invoke->invoker->tag["shigui1"] = QVariant::fromValue(choice1);
        }

        return choice != "cancel";
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    { 
        
        
        QString choice = invoke->invoker->tag["shigui"].toString();
        Player::Phase phase = ciyuanPhase(choice);
        QString choice1 = invoke->invoker->tag["shigui1"].toString();
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
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Finish) {
                QStringList choices = shiquChoices(change.player);
                if (choices.length() > 0) {
                    foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                        if (change.player->canDiscard(change.player, "he"))
                            d << SkillInvokeDetail(this, p, p, NULL, false, change.player);
                    }
                }
            }  
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QStringList choices = shiquChoices(invoke->preferredTarget);
        QString choice = "cancel";
        
        if (choices.length() > 1) {
            choices << "cancel";
            choice = room->askForChoice(invoke->invoker, objectName(), choices.join("+"));
        } else if (choices.length() == 1)
            choice = choices.first();
            
        if (choice != "cancel") {
            QString prompt = "@shiqu-discard:" + choice + ":" + invoke->preferredTarget->objectName();
            const Card *card = room->askForCard(invoke->invoker, ".|.|.|hand,equipped", prompt, data, objectName());
            if (card)
                invoke->invoker->tag["shiqu"] = QVariant::fromValue(choice);
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
        
        invoke->targets.first()->skip(Player::Finish);

        room->notifySkillInvoked(invoke->owner, objectName());
        LogMessage log;
        log.type = "#shiqu";
        log.from = invoke->invoker;
        //log.to << invoke->owner;
        log.arg = objectName();
        log.arg2 = choice;
        room->sendLog(log);


        invoke->targets.first()->setPhase(phase);
        room->broadcastProperty(invoke->targets.first(), "phase");
        RoomThread *thread = room->getThread();
        if (!thread->trigger(EventPhaseStart, room, QVariant::fromValue(invoke->targets.first())))
            thread->trigger(EventPhaseProceeding, room, QVariant::fromValue(invoke->targets.first()));
        thread->trigger(EventPhaseEnd, room, QVariant::fromValue(invoke->targets.first()));

        //invoke->targets.first()->setPhase(Player::??);
        //room->broadcastProperty(invoke->targets.first(), "phase");
        return false;
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



TH0105Package::TH0105Package()
    : Package("th0105")
{
    General *shinki = new General(this, "shinki$", "pc98", 4, false);
    Q_UNUSED(shinki);
    General *alice = new General(this, "alice_old", "pc98", 4, false);
    Q_UNUSED(alice);
    General *yuka = new General(this, "yuka_old$", "pc98", 4, false);
    Q_UNUSED(yuka)
    General *gengetsumugetsu = new General(this, "gengetsumugetsu", "pc98", 4, false);
    Q_UNUSED(gengetsumugetsu);
    General *elly = new General(this, "elly", "pc98", 4, false);
    Q_UNUSED(elly);
    General *yumemi = new General(this, "yumemi$", "pc98", 4, false);
    yumemi->addSkill(new Ciyuan);
    yumemi->addSkill(new Shigui);
    yumemi->addSkill(new Skill("kongjian$", Skill::Compulsory));

    General *chiyuri = new General(this, "chiyuri", "pc98", 4, false);
    chiyuri->addSkill(new Zhence);
    chiyuri->addSkill(new Shiqu);

    General *rikako = new General(this, "rikako", "pc98", 4, false);
    Q_UNUSED(rikako);
    General *kana = new General(this, "kana", "pc98", 3, false);
    kana->addSkill(new Qianyi);
    kana->addSkill(new Mengxiao);


    General *mima = new General(this, "mima$", "pc98", 4, false);
    Q_UNUSED(mima);
}

ADD_PACKAGE(TH0105)
