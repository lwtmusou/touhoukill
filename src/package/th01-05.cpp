#include "th01-05.h"
#include "general.h"
#include "skill.h"
#include "engine.h"
#include "maneuvering.h"

#include "clientplayer.h"

class Meiling : public TriggerSkill
{
public:
    Meiling() : TriggerSkill("meiling")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.card || damage.from == NULL || damage.to->isDead())
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
        foreach(ServerPlayer *mima, room->findPlayersBySkillName(objectName())) {
            if (mima != damage.from)
                d << SkillInvokeDetail(this, mima, mima);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt = "distance:" + damage.to->objectName() + ":" + QString::number(invoke->invoker->distanceTo(damage.to))
                + ":" + damage.card->objectName();
        invoke->invoker->tag["meiling"] = data;
        return invoke->invoker->askForSkillInvoke(this, prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        invoke->invoker->obtainCard(damage.card);
        if (damage.to->isAlive() && damage.from->isAlive()
                && invoke->invoker->distanceTo(damage.to) > invoke->invoker->getLostHp()) {
            room->damage(DamageStruct(objectName(), damage.from, invoke->invoker, 1, DamageStruct::Normal));
        }
        return false;
    }

};

class Fuchou : public TriggerSkill
{
public:
    Fuchou() : TriggerSkill("fuchou$")
    {
        frequency = Compulsory;
        events << TargetConfirmed;
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash") || !use.from->isAlive() || !use.from->hasLordSkill(this))
            return QList<SkillInvokeDetail>();
        
        QList<SkillInvokeDetail> d;
        bool can = false;
        foreach(ServerPlayer *t, use.to) {
            foreach(ServerPlayer *p, room->getLieges("pc98", use.from)) {
                if (p->inMyAttackRange(t) && t->getHp() > p->getHp() && p != use.from) {
                    can = true;
                    break;
                }
            }
        }
        if (can)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->touhouLogmessage("#fuchou", invoke->invoker, objectName());
        //room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->owne1huir->objectName(), invoke->targets.first()->objectName());
        if (use.m_addHistory) {
            room->addPlayerHistory(use.from, use.card->getClassName(), -1);
            use.m_addHistory = false;
            data = QVariant::fromValue(use);
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
            if (current->hasSkill(this) && !current->hasFlag("shigui_used") && (current->getPhase() == Player::Draw || current->getPhase() == Player::Play)
                    && current->getHandcardNum() != current->getMark("shigui"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current);
            //if (current->hasSkill(this) && current->getPhase() == Player::Draw && current->getHandcardNum() < current->getMark("shigui"))
            //    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current);
            //else if (current->hasSkill(this) && current->getPhase() == Player::Play && current->getHandcardNum() > current->getMark("shigui"))
            //    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString skillname = invoke->invoker->getHandcardNum() < invoke->invoker->getMark("shigui") ? "draw" : "play";
        QString prompt = skillname + "_notice:" + QString::number(invoke->invoker->getMark("shigui"));
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *current = invoke->invoker;
        room->setPlayerFlag(current, "shigui_used");
        bool recover = current->getHandcardNum() > current->getMark("shigui");
        int num = current->getHandcardNum() - current->getMark("shigui");
        
        if (recover) {
            room->askForDiscard(invoke->invoker, objectName(), qAbs(num), qAbs(num), false, false, "shigui_discard:" + QString::number(qAbs(num)));
            RecoverStruct recover;
            room->recover(invoke->invoker, recover);
        } else {
            current->drawCards(qAbs(num));
            room->loseHp(current);
        }

        /*if (current->getPhase() == Player::Draw) {
            current->drawCards(current->getMark("shigui") - current->getHandcardNum());
            room->loseHp(current);
        } else if (current->getPhase() == Player::Play) {
            int x = current->getHandcardNum() - current->getMark("shigui");
            room->askForDiscard(invoke->invoker, objectName(), x, x, false, false, "shigui_discard:" + QString::number(x));
            RecoverStruct recover;
            room->recover(invoke->invoker, recover);
        }*/
        return false;
    }

};

class Chongdong : public TriggerSkill
{
public:
    Chongdong() : TriggerSkill("chongdong$")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *current = room->getCurrent();
        if (damage.to->isAlive() && damage.to->hasLordSkill(this) && current && current->isAlive()) {
            foreach(ServerPlayer *liege, room->getLieges("pc98", damage.to)) {
                if (liege->canDiscard(liege, "hs"))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        return invoke->invoker->askForSkillInvoke(this, data);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *current = room->getCurrent();
        foreach(ServerPlayer *liege, room->getLieges("pc98", invoke->invoker)) {
            if (liege->canDiscard(liege, "hs")) {
                const Card *card = room->askForCard(liege, ".|red|.|hand", "@chongdong", data, Card::MethodDiscard);
                if (card != NULL) {
                    room->touhouLogmessage("#chongdong", current, objectName());
                    room->setPlayerFlag(current, "Global_TurnTerminated");
                    break;
                }
            }
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



ShiquCard::ShiquCard()
{
    will_throw = true;
}

bool ShiquCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && (to_select == Self || to_select->isCurrent());
}

void ShiquCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    QString choice = effect.from->tag["shiqu"].toString();
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

    LogMessage log;
    log.type = "#shiqu";
    log.from = effect.from;
    log.to << effect.to;
    log.arg = "shiqu";
    log.arg2 = choice;
    room->sendLog(log);

    ExtraTurnStruct extra;
    extra.player = effect.to;
    extra.set_phases << Player::RoundStart << phase << Player::NotActive;
    room->setTag("ExtraTurnStruct", QVariant::fromValue(extra));
    effect.to->gainAnExtraTurn();
}

class ShiquVS : public OneCardViewAsSkill
{
public:
    ShiquVS() : OneCardViewAsSkill("shiqu")
    {
        response_pattern = "@@shiqu";
        filter_pattern = ".|.|.|hand,equipped!";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            ShiquCard *card = new ShiquCard;
            card->addSubcard(originalCard);
            return card;
        }
        return NULL;
    }
};

class Shiqu : public TriggerSkill
{
public:
    Shiqu() : TriggerSkill("shiqu")
    {
        events << EventPhaseStart << EventPhaseChanging;
        view_as_skill = new ShiquVS;
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
        } else if (e == EventPhaseChanging) {
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
                            d << SkillInvokeDetail(this, p, p);
                    }
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        QStringList choices = shiquChoices(current);
        choices << "cancel";
        QString choice = room->askForChoice(invoke->invoker, objectName(), choices.join("+"));

        if (choice != "cancel") {
            QString prompt = "@shiqu-discard:" + choice + ":" + current->objectName();
            invoke->invoker->tag["shiqu"] = QVariant::fromValue(choice);
            return room->askForUseCard(invoke->invoker, "@@shiqu", prompt);
        }
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



class Lubiao : public TriggerSkill
{
public:
    Lubiao() : TriggerSkill("lubiao")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->getPhase() == Player::RoundStart) {
            QList<SkillInvokeDetail> d;
            foreach(ServerPlayer *p, room->getOtherPlayers(current)) {
                if (p->hasSkill(this) && p->getCards("j").isEmpty())
                    d << SkillInvokeDetail(this, p, p, NULL, false, current);
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        QList<int> list = room->getNCards(1);
        //do not consider cardlimit or targetFilter
        QString choice = room->askForChoice(invoke->invoker, objectName(), "play+draw", data);
        QString cardname = (choice == "draw") ? "supply_shortage" :  "indulgence";

        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), current->objectName());
        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = invoke->invoker;
        log.arg = objectName();
        room->sendLog(log);

        Card *supplyshortage = Sanguosha->cloneCard(cardname);
        WrappedCard *vs_card = Sanguosha->getWrappedCard(list.first());
        vs_card->setSkillName("lubiao");
        vs_card->takeOver(supplyshortage);
        room->broadcastUpdateCard(room->getAlivePlayers(), vs_card->getId(), vs_card);

        CardsMoveStruct  move;
        move.card_ids << vs_card->getId();
        move.to = invoke->invoker;
        move.to_place = Player::PlaceDelayedTrick;
        room->moveCardsAtomic(move, true);

        if (choice == "draw")
            current->skip(Player::Draw);
        else
            current->skip(Player::Play);
        return false;
    }
};

class Mengxiao : public TriggerSkill
{
public:
    Mengxiao() : TriggerSkill("mengxiao")
    {
        events << EventPhaseStart << CardsMoveOneTime << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == CardsMoveOneTime) {
            ServerPlayer *kana = room->getCurrent();
            if (!kana || kana->isDead())
                return;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile)
                room->setPlayerMark(kana, objectName(), kana->getMark(objectName()) + move.card_ids.length()) ;
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(change.player, objectName(), 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e != EventPhaseStart)
            return QList<SkillInvokeDetail>();
        ServerPlayer *kana = data.value<ServerPlayer *>();
        if (kana->hasSkill(this) && kana->isAlive() && kana->getPhase() == Player::Finish
                && kana->getHp() < kana->getMark(objectName()) && kana->isWounded())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, kana, kana);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        RecoverStruct recover;
        room->recover(invoke->invoker, recover);
        return false;
    }

};

class Yeyan : public TriggerSkill
{
public:
    Yeyan() : TriggerSkill("yeyan")
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
        room->touhouLogmessage("#yeyan", invoke->invoker, objectName(), logto, use.card->objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        foreach(ServerPlayer *to, invoke->targets) {
            if (!to->canDiscard(to, "hs"))
                continue;

            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), to->objectName());
            QStringList prompt_list;
            prompt_list << "yeyan-discard" << use.card->objectName()
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
            prompt_list1 << "yeyan-show" << use.card->objectName()
                         << to->objectName() << card->getType();
            QString prompt1 = prompt_list1.join(":");
            invoke->invoker->tag["yeyan_target"] = QVariant::fromValue(to);
            invoke->invoker->tag["yeyan_card"] = QVariant::fromValue(card);

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

class Youyue : public TriggerSkill
{
public:
    Youyue() : TriggerSkill("youyue")
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
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->getSuit() == Card::Spade)
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
        return !to_select->isEquipped()  && to_select->isBlack()  && selected.isEmpty();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 1) {
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
    mima->addSkill(new Meiling);
    mima->addSkill(new Fuchou);

    General *yumemi = new General(this, "yumemi$", "pc98", 4, false);
    yumemi->addSkill(new Ciyuan);
    yumemi->addSkill(new Shigui);
    yumemi->addSkill(new Chongdong);

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
    kana->addSkill(new Lubiao);
    kana->addSkill(new Mengxiao);

    General *yuka_old = new General(this, "yuka_old$", "pc98", 4, false);
    yuka_old->addSkill(new Yeyan);
    yuka_old->addSkill(new Youyue);
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

    addMetaObject<ShiquCard>();
    addMetaObject<LianmuCard>();
    addMetaObject<SqChuangshiCard>();
    addMetaObject<ModianCard>();

    skills << new ModianVS;
}

ADD_PACKAGE(TH0105)
