#include "th06.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"


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
        if (player->getHp() > 1 && pattern.contains("peach")) {
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
        events << GameStart << EventAcquireSkill << EventLoseSkill << Death << Debut;
    }


    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!player) return QStringList();
        if (triggerEvent == GameStart || triggerEvent == Debut
            || (triggerEvent == EventAcquireSkill && data.toString() == "skltkexue")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill(objectName(), false, true))
                    lords << p;
            }
            if (lords.isEmpty()) return QStringList();

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("skltkexue_attach"))
                    room->attachSkillToPlayer(p, "skltkexue_attach");
            }
        } else if (triggerEvent == Death || (triggerEvent == EventLoseSkill && data.toString() == "skltkexue")) {
            if (triggerEvent == Death) {
                DeathStruct death = data.value<DeathStruct>();
                if (!death.who->hasSkill(objectName(), false, true))//deal the case that death in round of changshi?
                    return QStringList();
            }
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill(objectName(), false, true))
                    lords << p;
            }
            if (lords.length() > 2) return QStringList();

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("skltkexue_attach"))
                    room->detachSkillFromPlayer(p, "skltkexue_attach", true);
            }
        }
        return QStringList();
    }
};

class Mingyun : public TriggerSkill
{
public:
    Mingyun() : TriggerSkill("mingyun")
    {
        events << StartJudge;
    }


    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        JudgeStruct * judge = data.value<JudgeStruct *>();
        if (!judge->who || !judge->who->isAlive())
            return TriggerList();

        TriggerList skill_list;
        QList<ServerPlayer *> remilias = room->findPlayersBySkillName(objectName());
        foreach(ServerPlayer *remilia, remilias)
            skill_list.insert(remilia, QStringList(objectName()));
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const
    {
        JudgeStruct * judge = data.value<JudgeStruct *>();
        ask_who->tag["mingyun_judge"] = data;
        QString prompt = "judge:" + judge->who->objectName() + ":" + judge->reason;
        if (ask_who->askForSkillInvoke(objectName(), prompt)) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, ask_who->objectName(), judge->who->objectName());
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {
        QList<int> list = room->getNCards(2);
        room->fillAG(list, ask_who);
        int obtain_id = room->askForAG(ask_who, list, false, objectName());
        room->clearAG(ask_who);

        room->obtainCard(ask_who, obtain_id, false);
        list.removeOne(obtain_id);
        room->returnToTopDrawPile(list);
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

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        QStringList skill_list;
        if (TriggerSkill::triggerable(player) && player->getKingdom() != "hmx") //target != NULL && target->isAlive()
            return skill_list;

        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->hasLordSkill("xueyi"))
                skill_list << p->objectName() + "'" + objectName();
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *skill_invoker) const
    {
        //ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@@xueyi", true, true);
        skill_invoker->tag["xueyi-target"] = QVariant::fromValue(skill_target);
        if (skill_invoker->askForSkillInvoke(objectName(), QVariant::fromValue(skill_target))) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(skill_target, objectName());
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = skill_invoker;
            log.to << skill_target;
            log.arg = objectName();
            room->sendLog(log);

            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *skill_target, QVariant &, ServerPlayer *) const
    {
        skill_target->drawCards(1, objectName());
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

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Start)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->touhouLogmessage("#TriggerSkill", player, "pohuai");
        room->notifySkillInvoked(player, objectName());

        JudgeStruct judge;
        judge.who = player;
        judge.pattern = "Slash";
        judge.good = true;
        judge.play_animation = true;
        judge.reason = objectName();
        room->judge(judge);

        if (judge.isBad())
            return false;
        QList<ServerPlayer *> all;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (player->distanceTo(p) <= 1)
                all << p;
        }
        if (all.isEmpty())
            return false;

        foreach (ServerPlayer *p, all)
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
        foreach (ServerPlayer *p, all)
            room->damage(DamageStruct(objectName(), player, p));

        return false;
    }
};

class Yuxue : public TriggerSkill
{
public:
    Yuxue() : TriggerSkill("yuxue")
    {
        events << PreCardUsed << Damaged << ConfirmDamage;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (triggerEvent == Damaged) {
            if (TriggerSkill::triggerable(player))
                return QStringList(objectName());
        } else if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (player->getMark("yuxue") > 0 && use.card->isKindOf("Slash")) {
                room->setPlayerMark(player, "yuxue", 0);
                room->setCardFlag(use.card, "yuxueinvoked");
                room->touhouLogmessage("#ChoosePlayerWithSkill", player, "yuxue", use.to, NULL);
                room->notifySkillInvoked(player, objectName());
            }
        } else if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.chain || damage.transfer || !damage.by_user)
                return QStringList();
            if (damage.from == damage.to)
                return QStringList();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->hasFlag("yuxueinvoked")) {
                return QStringList(objectName());
            }
        }
        return QStringList();

    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            room->setPlayerMark(player, "yuxue", damage.damage);
            try {
                room->askForUseCard(player, "slash", "@yuxue", -1, Card::MethodUse, false, objectName());
            }
            catch (TriggerEvent triggerEvent) {
                if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                    room->setPlayerMark(player, "yuxue", 0);
            }
            room->setPlayerMark(player, "yuxue", 0);
        } else if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            damage.damage = damage.damage + 1;
            if (damage.from) {
                QList<ServerPlayer *> logto;
                logto << damage.to;
                room->touhouLogmessage("#yuxue_damage", damage.from, "yuxue", logto);
            }
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

class YuxueTargetMod : public TargetModSkill
{
public:
    YuxueTargetMod() : TargetModSkill("#yuxue-dis")
    {
        pattern = "Slash";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->getMark("yuxue") > 0)
            return 1000;
        else
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

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            QStringList trigger_list;
            for (int i = 1; i <= damage.damage; i++) {
                trigger_list << objectName();
            }
            return trigger_list;
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return room->askForSkillInvoke(player, objectName(), data);
    }


    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->drawCards(1);
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
            const Card *cards = room->askForExchange(source, "suoding", map[sp], false, "suoding_exchange:" + QString::number(map[sp]));
            foreach(int id, cards->getSubcards())
                sp->addToPile("suoding_cards", id, false);
        } else {
            for (int i = 0; i < map[sp]; i++) {
                if (!sp->isKongcheng()) {
                    int card_id = room->askForCardChosen(source, sp, "h", "suoding");
                    sp->addToPile("suoding_cards", card_id, false);
                }
            }
        }
    }
    source->setFlags("suoding");
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
        events << EventPhaseChanging << Death;
        view_as_skill = new SuodingVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (!TriggerSkill::triggerable(player) || !player->hasFlag(objectName()))
            return QStringList();
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
            foreach (ServerPlayer *liege, room->getAlivePlayers()) {
                if (!liege->getPile("suoding_cards").isEmpty()) {
                    room->notifySkillInvoked(player, objectName());

                    CardsMoveStruct move;
                    move.card_ids = liege->getPile("suoding_cards");
                    move.to_place = Player::PlaceHand;
                    move.to = liege;
                    room->moveCardsAtomic(move, false);
                    QList<ServerPlayer *> logto;
                    logto << liege;
                    room->touhouLogmessage("#suoding_Trigger", player, objectName(), logto, QString::number(move.card_ids.length()));
                }
            }
            player->setFlags("-suoding");
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (!death.who->hasFlag("suoding"))
                return QStringList();
            foreach (ServerPlayer *liege, room->getOtherPlayers(player)) {
                if (!liege->getPile("suoding_cards").isEmpty()) {

                    room->notifySkillInvoked(death.who, objectName());

                    CardsMoveStruct move;
                    move.card_ids = liege->getPile("suoding_cards");
                    move.to_place = Player::PlaceHand;
                    move.to = liege;
                    room->moveCardsAtomic(move, false);
                    QList<ServerPlayer *> logto;
                    logto << liege;
                    room->touhouLogmessage("#suoding_Trigger", death.who, objectName(), logto, QString::number(move.card_ids.length()));

                }
            }
            death.who->setFlags("-suoding");
        }
        return QStringList();

    }
};

class Huisu : public TriggerSkill
{
public:
    Huisu() : TriggerSkill("huisu")
    {
        events << PostHpLost << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        int x = 0;
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            x = damage.damage;
        } else if (triggerEvent == PostHpLost)
            x = data.toInt();

        if (x <= 0 || !player->isAlive())
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForSkillInvoke(player, "huisu");
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
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

        judge.who = player;
        room->judge(judge);

        if (judge.isGood()) {
            RecoverStruct recov;
            recov.recover = 1;
            recov.who = player;
            room->recover(player, recov);
        }
        return false;
    }
};

class Bolan : public TriggerSkill
{
public:
    Bolan() : TriggerSkill("bolan")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &ask) const
    {
        //if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == CardsMoveOneTime) {
            if (!TriggerSkill::triggerable(player)) return QStringList();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.card_ids.length() == 1 && move.from_places.contains(Player::PlaceTable)
                && move.to_place == Player::DiscardPile
                && move.reason.m_reason == CardMoveReason::S_REASON_USE) {
                const Card *card = move.reason.m_extraData.value<const Card *>();

                if (!card || !card->isNDTrick() || card->isKindOf("Nullification"))
                    return QStringList();
                //if (card->isVirtualCard()) {
                //    const Card *realcard = Sanguosha->getEngineCard(move.card_ids.first());
                //    if (realcard->objectName() != card->objectName())
                //        return false;
                //}

                if (room->getCardPlace(move.card_ids.first()) == Player::DiscardPile
                    && player != move.from)
                    return QStringList(objectName());
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!p->getPile("yao_mark").isEmpty()) {
                    ask = p;
                    return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            const Card *card = move.reason.m_extraData.value<const Card *>();

            QString prompt = "obtain:" + card->objectName();
            return room->askForSkillInvoke(player, objectName(), prompt);
        } else if (triggerEvent == EventPhaseChanging) {
            if (ask == NULL)
                return false;
            CardsMoveStruct move;
            move.card_ids = ask->getPile("yao_mark");
            move.to_place = Player::PlaceHand;
            move.to = ask;
            room->moveCardsAtomic(move, true);

            room->touhouLogmessage("#bolan_Invoke", ask, objectName(), QList<ServerPlayer *>(), QString::number(move.card_ids.length()));
            int x = ask->getHandcardNum() - ask->getMaxHp();
            if (x > 0)
                room->askForDiscard(ask, objectName(), x, x, false, false, "bolan_discard:" + QString::number(x));
        }
        return false;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            const Card *card = move.reason.m_extraData.value<const Card *>();
            player->addToPile("yao_mark", card);
        }
        return false;
    }
};



class Qiyao : public OneCardViewAsSkill
{
public:
    Qiyao() : OneCardViewAsSkill("qiyao")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return to_select->isNDTrick() && !to_select->isEquipped();
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return  pattern.contains("peach")
            && !player->isCurrent()
            && player->getMark("Global_PreventPeach") == 0
            && (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            Peach *peach = new Peach(originalCard->getSuit(), originalCard->getNumber());
            peach->addSubcard(originalCard);
            peach->setSkillName("qiyao");
            return peach;
        } else
            return NULL;
    }
};


class Neijin : public TriggerSkill
{
public:
    Neijin() : TriggerSkill("neijin")
    {
        events << EventPhaseEnd;
    }


    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::Play && player->isAlive()) {
            QList<ServerPlayer *> meirins = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *meirin, meirins) {
                if (meirin != player && meirin->getHandcardNum() < meirin->getMaxHp() && !meirin->isChained())
                    skill_list.insert(meirin, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *meirin) const
    {
        return room->askForSkillInvoke(meirin, objectName(), data);
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *meirin) const
    {
        meirin->setChained(true);
        room->broadcastProperty(meirin, "chained");
        room->setEmotion(meirin, "chain");

        int num = meirin->getMaxHp() - meirin->getHandcardNum();
        meirin->drawCards(num);
        num = qMin(num, meirin->getHandcardNum());
        if (num == 0)
            return false;

        const Card *giveCards = room->askForExchange(meirin, objectName(), num, false, "neijin_exchange:" + player->objectName() + ":" + QString::number(num));
        room->obtainCard(player, giveCards, false);
        return false;
    }
};

class Taiji : public TriggerSkill
{
public:
    Taiji() : TriggerSkill("taiji")
    {
        events << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (!player->isChained() || damage.nature != DamageStruct::Normal)
            return QStringList();

        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->tag.remove("taiji-target");
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@taiji", true, true);
        if (target) {
            player->tag["taiji-target"] = QVariant::fromValue(target);
            return true;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag.value("taiji-target").value<ServerPlayer *>();
        if (target) {
            player->setChained(false);
            room->broadcastProperty(player, "chained");
            room->setEmotion(player, "chain");
            room->damage(DamageStruct(objectName(), player, target, 1, DamageStruct::Normal));
        }
        player->tag.remove("taiji-target");
        return false;
    }

};



class Dongjie : public TriggerSkill
{
public:
    Dongjie() : TriggerSkill("dongjie")
    {
        events << DamageCaused;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user)
            return QStringList();
        if (damage.from == NULL || damage.from == damage.to)
            return QStringList();
        if (damage.card  && damage.card->isKindOf("Slash"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        player->tag["dongjie_damage"] = QVariant::fromValue(damage);
        return room->askForSkillInvoke(player, "dongjie", QVariant::fromValue(damage.to));
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());

        QList<ServerPlayer *> logto;
        logto << damage.to;
        room->touhouLogmessage("#Dongjie", player, "dongjie", logto);
        player->drawCards(1);
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

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Fire && (damage.damage > 1 || player->getHp() <= 1))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        return true;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->touhouLogmessage("#bingpolog", player, "bingpo", QList<ServerPlayer *>(), QString::number(damage.damage));
        room->notifySkillInvoked(player, objectName());
        return true;
    }

};


class Bendan : public FilterSkill
{
public:
    Bendan() : FilterSkill("bendan")
    {

    }

    virtual bool viewFilter(const Card *to_select) const
    {
        Room *room = Sanguosha->currentRoom();

        ServerPlayer *cirno = room->getCardOwner(to_select->getEffectiveId());
        return (cirno != NULL && cirno->hasSkill(objectName()));
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        WrappedCard *wrap = Sanguosha->getWrappedCard(originalCard->getEffectiveId());
        wrap->setNumber(9);
        wrap->setSkillName(objectName());
        wrap->setModified(true);
        return wrap;
    }
};

class Zhenye : public TriggerSkill
{
public:
    Zhenye() : TriggerSkill("zhenye")
    {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer * &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Finish)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@zhenye-select", true, true);
        if (target) {
            player->tag["zhenye-target"] = QVariant::fromValue(target);
            return true;
        } else {
            player->tag.remove("zhenye-target");
            return false;
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = player->tag.value("zhenye-target").value<ServerPlayer *>();
        if (target) {
            player->turnOver();
            target->turnOver();
        }
        player->tag.remove("zhenye-target");
        return false;
    }
};

class Anyu : public TriggerSkill
{
public:
    Anyu() : TriggerSkill("anyu")
    {
        events << TargetConfirming;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isBlack() && (use.card->isKindOf("Slash") || use.card->isNDTrick()))
            return QStringList(objectName());
        return QStringList();

    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *, QVariant &, ServerPlayer *) const
    {
        return true;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->touhouLogmessage("#TriggerSkill", player, "anyu");
        QString choice = room->askForChoice(player, objectName(), "turnover+draw", data);
        room->notifySkillInvoked(player, objectName());
        if (choice == "turnover")
            player->turnOver();
        else
            player->drawCards(1);

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

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->getPhase() == Player::Start) {
            QList<ServerPlayer *> srcs = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *p, srcs) {
                if (player != p)
                    skill_list.insert(p, QStringList(objectName()));
            }
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ask_who) const
    {
        QString prompt = "target:" + player->objectName();
        return room->askForSkillInvoke(ask_who, objectName(), prompt);
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, ask_who->objectName(), player->objectName());

        ask_who->drawCards(1);
        QString choice = room->askForChoice(ask_who, objectName(), "hp_moxue+maxhp_moxue", data);
        if (choice == "hp_moxue")
            room->loseHp(ask_who, 1);
        else
            room->loseMaxHp(ask_who, 1);

        player->skip(Player::Judge);
        player->skip(Player::Draw);
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

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer * &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getMaxHp() == 1)
            return QStringList(objectName());
        return QStringList();
    }


    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->getGeneralName() == "hmx008") // koakuma??? @lwtmusou
            room->doLightbox("$moxueAnimate", 4000);
        room->touhouLogmessage("#TriggerSkill", player, "moxue");
        room->notifySkillInvoked(player, objectName());
        player->drawCards(player->getHandcardNum());
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


    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer * &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        ServerPlayer *who = room->getCurrentDyingPlayer();
        if (player == who && player->faceUp())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName(), data);
    }


    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->turnOver();
        QList<int> list = room->getNCards(3);
        room->fillAG(list);
        QStringList e;
        QVariantList listc;
        foreach (int c, list) {
            e << Sanguosha->getCard(c)->toString();
            listc << c;
        }
        LogMessage mes;
        mes.type = "$TurnOver";
        mes.from = player;
        mes.card_str = e.join("+");
        room->sendLog(mes);

        player->tag["juxian_cards"] = listc;

        Card::Suit suit = room->askForSuit(player, objectName());
        player->tag.remove("juxian_cards");

        room->touhouLogmessage("#ChooseSuit", player, Card::Suit2String(suit));

        QList<int> get;
        DummyCard dummy;
        foreach (int id, list) {
            if (Sanguosha->getCard(id)->getSuit() != suit)
                get << id;
            else
                dummy.addSubcard(id);
        }
        if (!get.isEmpty()) {
            CardsMoveStruct move;
            move.card_ids = get;
            move.reason.m_reason = CardMoveReason::S_REASON_DRAW;
            move.to = player;
            move.to_place = Player::PlaceHand;
            room->moveCardsAtomic(move, true);
        }
        if (dummy.getSubcards().length() > 0) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), "");
            room->throwCard(&dummy, reason, NULL);
            RecoverStruct recover;
            recover.recover = dummy.getSubcards().length();
            room->recover(player, recover);
        }
        room->clearAG();
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
    flandre->addSkill(new YuxueTargetMod);
    flandre->addSkill(new Shengyan);
    related_skills.insertMulti("yuxue", "#yuxue-dis");

    General *sakuya = new General(this, "sakuya", "hmx", 4, false);
    sakuya->addSkill(new Suoding);
    sakuya->addSkill(new Huisu);

    General *patchouli = new General(this, "patchouli", "hmx", 3, false);
    patchouli->addSkill(new Bolan);
    patchouli->addSkill(new Qiyao);

    General *meirin = new General(this, "meirin", "hmx", 4, false);
    meirin->addSkill(new Neijin);
    meirin->addSkill(new Taiji);

    General *cirno = new General(this, "cirno", "hmx", 3, false);
    cirno->addSkill(new Dongjie);
    cirno->addSkill(new Bingpo);
    cirno->addSkill(new Bendan);

    General *rumia = new General(this, "rumia", "hmx", 3, false);
    rumia->addSkill(new Zhenye);
    rumia->addSkill(new Anyu);

    General *koakuma = new General(this, "koakuma", "hmx", 3, false);
    koakuma->addSkill(new Qiyue);
    koakuma->addSkill(new Moxue);

    General *daiyousei = new General(this, "daiyousei", "hmx", 3, false);
    daiyousei->addSkill(new Juxian);
    daiyousei->addSkill(new Banyue);

    General *sakuya_sp = new General(this, "sakuya_sp", "hmx", 3, false);
    Q_UNUSED(sakuya_sp);

    addMetaObject<SkltKexueCard>();
    addMetaObject<SuodingCard>();
    addMetaObject<BanyueCard>();

    skills << new SkltKexueVS;
}

ADD_PACKAGE(TH06)

