#include "th06.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"





skltkexueCard::skltkexueCard() {
    mute = true;
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
    m_skillName = "skltkexuepeach";
}
void skltkexueCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (who != NULL && who->hasSkill("skltkexue")){
        if (who->getGeneralName() == "hmx001" && !who->hasFlag("skltkexueAnimate")){
            room->doLightbox("$skltkexueAnimate", 2000);
            room->setPlayerFlag(who, "skltkexueAnimate");
        }
        room->notifySkillInvoked(who, "skltkexue");
        room->loseHp(source);
        source->drawCards(2);

        room->notifySkillInvoked(who, "skltkexue");
        RecoverStruct recover;
        recover.recover = 1;
        recover.who = source;
        room->recover(who, recover);
    }
}

class skltkexuepeach : public ZeroCardViewAsSkill {
public:
    skltkexuepeach() : ZeroCardViewAsSkill("skltkexuepeach") {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (player->getHp() > 1 && pattern.contains("peach")){
            foreach(const Player *p, player->getAliveSiblings()){
                if (p->getHp() < 1 && p->hasSkill("skltkexue"))
                    return true;
            }
        }
        return false;
    }

    virtual const Card *viewAs() const{
        return new skltkexueCard;
    }
};

class skltkexue : public TriggerSkill {
public:
    skltkexue() : TriggerSkill("skltkexue") {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }



    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == GameStart
            || (triggerEvent == EventAcquireSkill && data.toString() == "skltkexue")) {
            QList<ServerPlayer *> lords;
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty()) return false;

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach(ServerPlayer *p, players) {
                if (!p->hasSkill("skltkexuepeach"))
                    room->attachSkillToPlayer(p, "skltkexuepeach");
            }
        } else if (triggerEvent == Death || (triggerEvent == EventLoseSkill && data.toString() == "skltkexue")) {
            if (triggerEvent == Death){
                DeathStruct death = data.value<DeathStruct>();
                if (!death.who->hasSkill(objectName(),false,true))//deal the case that death in round of changshi?
                    return false;
            }
            QList<ServerPlayer *> lords;
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill(objectName()))
                    lords << p;
            }
            if (lords.length() > 2) return false;

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach(ServerPlayer *p, players) {
                if (p->hasSkill("skltkexuepeach"))
                    room->detachSkillFromPlayer(p, "skltkexuepeach", true);
            }
        }
        return false;
    }
};

class mingyun : public TriggerSkill {
public:
    mingyun() : TriggerSkill("mingyun") {
        events << StartJudge;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        ServerPlayer *sklt = room->findPlayerBySkillName(objectName());
        if (sklt != NULL){
            sklt->tag["mingyun_judge"] = data;
            QString prompt = "judge:" + judge->who->objectName() + ":" + judge->reason;
            if (sklt->askForSkillInvoke(objectName(), prompt)){
                if (room->getCurrent() && room->getCurrent()->getPhase() == Player::Judge &&
                    sklt->getGeneralName() == "hmx001" && !sklt->hasFlag("mingyunAnimate")){
                    room->doLightbox("$mingyunAnimate", 2000);
                    room->setPlayerFlag(sklt, "mingyunAnimate");
                }
                QList<int> list = room->getNCards(2);
                room->fillAG(list, sklt);
                int obtain_id = room->askForAG(sklt, list, false, objectName());
                room->clearAG(sklt);

                room->obtainCard(sklt, obtain_id, false);
                list.removeOne(obtain_id);
                room->askForGuanxing(sklt, list, Room::GuanxingUpOnly, objectName());
            }
        }
        return false;
    }
};

class skltxueyi : public TriggerSkill {
public:
    skltxueyi() : TriggerSkill("skltxueyi$") {
        events << HpRecover;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL && target->isAlive() && target->getKingdom() == "hmx");
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->hasLordSkill(objectName()))
                targets << p;
        }

        while (!targets.isEmpty()){
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@@skltxueyi", true, true);
            if (target != NULL){
                if (target->getGeneralName() == "hmx001" && !target->hasFlag("skltxueyiAnimate")){
                    room->doLightbox("$skltxueyiAnimate", 2000);
                    room->setPlayerFlag(target, "skltxueyiAnimate");
                }
                room->notifySkillInvoked(target, objectName());
                targets.removeOne(target);
                target->drawCards(1);
            } else
                break;

        }
        return false;
    }
};

class pohuai : public TriggerSkill {
public:
    pohuai() : TriggerSkill("pohuai") {
        events << EventPhaseStart;
        frequency = Compulsory;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Start){
            if (player->getGeneralName() == "hmx002")
                room->doLightbox("$pohuaiAnimate", 2000);

            room->touhouLogmessage("#TriggerSkill", player, "pohuai");
            room->notifySkillInvoked(player, objectName());

            JudgeStruct judge;
            judge.who = player;
            judge.pattern = "Slash";
            judge.good = true;
            //judge.negative = false
            judge.play_animation = true;
            judge.reason = objectName();
            room->judge(judge);

            if (judge.isBad())
                return false;
            QList<ServerPlayer *> all;
            foreach(ServerPlayer *p, room->getAlivePlayers()){
                if (player->distanceTo(p) <= 1)
                    all << p;
            }
            if (all.isEmpty())
                return false;

            room->getThread()->delay();
            room->sortByActionOrder(all);
            foreach(ServerPlayer *p, all)
                room->damage(DamageStruct(objectName(), player, p));
        }
        return false;
    }
};

class yuxue : public TriggerSkill {
public:
    yuxue() : TriggerSkill("yuxue") {
        events << PreCardUsed << Damaged;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Damaged){
            DamageStruct damage = data.value<DamageStruct>();
            room->setPlayerMark(player, "yuxue", damage.damage);
            room->askForUseCard(player, "slash", "@yuxue", -1, Card::MethodUse, false, objectName());
            room->setPlayerMark(player, "yuxue", 0);
        } else if (triggerEvent == PreCardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (player->getMark("yuxue") > 0 && use.card->isKindOf("Slash")){
                if (player->getGeneralName() == "hmx002" && !player->hasFlag("yuxueAnimate")){
                    room->doLightbox("$yuxueAnimate", 2000);
                    room->setPlayerFlag(player, "yuxueAnimate");
                }
                room->setPlayerMark(player, "yuxue", 0);
                room->setCardFlag(use.card, "yuxueinvoked");
                room->touhouLogmessage("#ChoosePlayerWithSkill", player, "yuxue", use.to, NULL);
                room->notifySkillInvoked(player, objectName());
            }
        }
        return false;
    }
};
class yuxue_buff : public TriggerSkill {
public:
    yuxue_buff() : TriggerSkill("#yuxue") {
        events << ConfirmDamage;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        DamageStruct damage = data.value<DamageStruct>();

        if (damage.chain || damage.transfer || !damage.by_user)
            return false;
        if (damage.from == damage.to)
            return false;
        if (damage.card != NULL && damage.card->isKindOf("Slash") && damage.card->hasFlag("yuxueinvoked")){
            damage.damage = damage.damage + 1;
            if (damage.from != NULL){
                QList<ServerPlayer *> logto;
                logto << damage.to;
                room->touhouLogmessage("#yuxue_damage", damage.from, "yuxue", logto);
            }
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

class yuxuedis : public TargetModSkill {
public:
    yuxuedis() : TargetModSkill("#yuxue-dis") {
        pattern = "Slash";
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (from->getMark("yuxue") > 0)
            return 1000;
        else
            return 0;
    }
};

class shengyan : public TriggerSkill {
public:
    shengyan() : TriggerSkill("shengyan") {
        events << Damage;
        frequency = Frequent;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        for (int i = 0; i < damage.damage; i++){
            if (room->askForSkillInvoke(player, objectName(), data)){
                if (player->getGeneralName() == "hmx002" && !player->hasFlag("shengyanAnimate")){
                    room->doLightbox("$shengyanAnimate", 2000);
                    room->setPlayerFlag(player, "shengyanAnimate");
                }
                player->drawCards(1);
            } else
                break;
        }
        return false;
    }
};


suodingCard::suodingCard() {
    mute = true;
    m_skillName = "suoding";
}

bool suodingCard::targetFilter(const QList<const Player *> &, const Player *, const Player *) const{
    Q_ASSERT(false);
    return false;
}
bool suodingCard::targetFilter(const QList<const Player *> &targets, const Player *to_select,
    const Player *, int &maxVotes) const{

    if (to_select->isKongcheng())
        return false;
    int i = 0;
    foreach(const Player *player, targets){
        if (player == to_select)
            i++;
    }

    maxVotes = qMax(3 - targets.size(), 0) + i;
    return maxVotes > 0;
}
bool suodingCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    if (targets.toSet().size() > 3 || targets.toSet().size() == 0) return false;
    QMap<const Player *, int> map;

    foreach(const Player *sp, targets)
        map[sp]++;
    foreach(const Player *sp, map.keys()) {
        if (map[sp] > sp->getHandcardNum())
            return false;
    }

    return true;
}
void suodingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    if (source->getGeneralName() == "hmx003")
        room->doLightbox("$suodingAnimate", 2000);

    QMap<ServerPlayer *, int> map;

    foreach(ServerPlayer *sp, targets)
        map[sp]++;


    QList<ServerPlayer *> newtargets = map.keys();
    room->sortByActionOrder(newtargets);
    foreach(ServerPlayer *sp, newtargets){
        if (source == sp){
            const Card *cards = room->askForExchange(source, "suoding", map[sp], false, "suoding_exchange:" + QString::number(map[sp]));
            foreach(int id, cards->getSubcards())
                sp->addToPile("suoding_cards", id, false);
        } else {
            for (int i = 0; i < map[sp]; i++){
                int card_id = room->askForCardChosen(source, sp, "h", "suoding");
                sp->addToPile("suoding_cards", card_id, false);
            }
        }
    }
    source->setFlags("suoding");
}


class suodingvs : public ZeroCardViewAsSkill {
public:
    suodingvs() : ZeroCardViewAsSkill("suoding") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("suodingCard");
    }

    virtual const Card *viewAs() const{
        return new suodingCard;
    }
};
class suoding : public TriggerSkill {
public:
    suoding() : TriggerSkill("suoding") {
        events << EventPhaseChanging << Death;
        view_as_skill = new suodingvs;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasFlag(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
            foreach(ServerPlayer *liege, room->getAlivePlayers()){
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
                return false;
            foreach(ServerPlayer *liege, room->getOtherPlayers(player)) {
                if (!liege->getPile("suoding_cards").isEmpty()){

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
        return false;
    }
};

class huisu : public TriggerSkill {
public:
    huisu() : TriggerSkill("huisu") {
        events << PreHpLost << PostHpReduced << Damaged;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        int x = 0;
        bool isLoseHp = false;
        if (triggerEvent == PreHpLost)
            room->setPlayerFlag(player, "huisu_losthp");
        else if (triggerEvent == Damaged){
            DamageStruct damage = data.value<DamageStruct>();
            x = damage.damage;
        } else if (triggerEvent == PostHpReduced){
            x = data.toInt();
            if (player->getHp() < 1 && player->hasFlag("huisu_losthp")){
                room->enterDying(player, NULL);
            }
            isLoseHp = true;
            room->setPlayerFlag(player, "-huisu_losthp");
        }
        if (x <= 0 || !player->isAlive())
            return false;
        if (room->askForSkillInvoke(player, "huisu")){
            if (player->getGeneralName() == "hmx003" && !player->hasFlag("huisuAnimate")){
                room->doLightbox("$huisuAnimate", 2000);
                room->setPlayerFlag(player, "huisuAnimate");
            }
            //for ( int i=0; i<x ;i++) {        
            JudgeStruct judge;
            if (isLoseHp)
                judge.pattern = ".|heart|2~9";
            else
                judge.pattern = ".|red|2~9";
            judge.good = true;
            if (isLoseHp)
                judge.reason = "huisu2";
            else
                judge.reason = "huisu1";
            judge.who = player;
            room->judge(judge);

            if (judge.isGood()){
                RecoverStruct recov;
                recov.recover = 1;
                recov.who = player;
                room->recover(player, recov);
            }
            // }    
        }
        return false;
    }
};

class bolan : public TriggerSkill {
public:
    bolan() : TriggerSkill("bolan") {
        events << EventPhaseChanging << CardsMoveOneTime << CardUsed;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->willThrow() && use.card->isNDTrick() && !use.card->isKindOf("Nullification")){
                if (use.card->isVirtualCard() && use.card->subcardsLength() != 1)
                    return false;
                if (Sanguosha->getEngineCard(use.card->getEffectiveId())->objectName() == use.card->objectName())
                    room->setCardFlag(use.card->getEffectiveId(), "realNDTrick");
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            Card *card = Sanguosha->getCard(move.card_ids.first());
            if (!player->hasSkill(objectName()))
                return false;
            if (card->hasFlag("realNDTrick") && move.card_ids.length() == 1
                && move.to_place == Player::DiscardPile
                && move.reason.m_reason == CardMoveReason::S_REASON_USE
                && player != move.from){
                QString prompt = "obtain:" + card->objectName();
                if (room->askForSkillInvoke(player, objectName(), prompt)){
                    if (player->getGeneralName() == "hmx004" && !player->hasFlag("bolanAnimate")){
                        room->doLightbox("$bolanAnimate", 2000);
                        room->setPlayerFlag(player, "bolanAnimate");
                    }
                    player->addToPile("yao_mark", card);
                    room->setCardFlag(card, "-realNDTrick");
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
            foreach(ServerPlayer *p, room->getAlivePlayers()){
                if (!p->getPile("yao_mark").isEmpty()) {
                    CardsMoveStruct move;
                    move.card_ids = p->getPile("yao_mark");
                    move.to_place = Player::PlaceHand;
                    move.to = p;
                    room->moveCardsAtomic(move, true);

                    room->touhouLogmessage("#bolan_Invoke", player, objectName(), QList<ServerPlayer *>(), QString::number(move.card_ids.length()));
                    int x = p->getHandcardNum() - p->getMaxHp();
                    if (x > 0)
                        room->askForDiscard(p, objectName(), x, x, false, false, "bolan_discard:" + QString::number(x));
                }
            }
        }
        return false;
    }
};


class qiyaovs : public OneCardViewAsSkill {
public:
    qiyaovs() : OneCardViewAsSkill("qiyao") {
        response_or_use = true;
    }

    virtual bool viewFilter(const Card *to_select) const{
        if (Self->getPile("yao_mark").isEmpty())
            return to_select->isNDTrick() && !to_select->isEquipped();
        else
            return !to_select->isEquipped();
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return   pattern.contains("peach")
            && player->getPhase() == Player::NotActive
            && player->getMark("Global_PreventPeach") == 0
            && (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE);
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard != NULL){
            Peach *peach = new Peach(originalCard->getSuit(), originalCard->getNumber());
            peach->addSubcard(originalCard);
            peach->setSkillName("qiyao");
            return peach;
        } else
            return NULL;
    }
};
class qiyao : public TriggerSkill {
public:
    qiyao() : TriggerSkill("qiyao") {
        events << PreCardUsed;
        view_as_skill = new qiyaovs;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getSkillName() == objectName()){
            if (!Sanguosha->getEngineCard(use.card->getEffectiveId())->isNDTrick()){

                QList<int> pile = player->getPile("yao_mark");
                if (pile.length() == 0)
                    return false;
                room->fillAG(pile, player);
                int id = room->askForAG(player, pile, false, objectName());
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
                room->throwCard(Sanguosha->getCard(id), reason, NULL);
                room->clearAG(player);
            }
        }

        return false;
    }
};



class douhun : public TriggerSkill {
public:
    douhun() : TriggerSkill("douhun") {
        events << SlashProceed;
        frequency = Compulsory;
    }

    virtual int getPriority(TriggerEvent) const{
        return 10;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (!effect.from->hasSkill(objectName()) && !effect.to->hasSkill(objectName()))
            return false;

        int douhun_type;
        ServerPlayer *douhun_src;
        ServerPlayer *other;
        if (effect.from->hasSkill(objectName())) {
            douhun_src = effect.from;
            douhun_type = 1;
            other = effect.to;
        } else {
            douhun_src = effect.to;
            douhun_type = 2;
            other = effect.from;
        }
        QList<ServerPlayer *>    logto;
        logto << other;

        room->touhouLogmessage("#douhun_invoke" + QString::number(douhun_type), douhun_src, "douhun", logto);
        room->notifySkillInvoked(douhun_src, objectName());

        ServerPlayer *first = effect.to;
        ServerPlayer *second = effect.from;
        room->setEmotion(first, "duel");
        room->setEmotion(second, "duel");
        forever {
            if (first->isDead())
            break;
            const Card *card = room->askForCard(first, "slash", "douhun-slash:" + douhun_src->objectName(), data, Card::MethodResponse, second, false, objectName());
            if (card == NULL)
                break;
            qSwap(first, second);
        }
        effect.to = first;

        room->slashResult(effect, NULL);
        return true;
    }
};

class zhanyivs : public OneCardViewAsSkill {
public:
    zhanyivs() : OneCardViewAsSkill("zhanyi") {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }


    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return  pattern == "slash" && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard != NULL){

            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName("zhanyi");
            return slash;
        } else
            return NULL;
    }
};
class zhanyi : public TriggerSkill {
public:
    zhanyi() : TriggerSkill("zhanyi") {
        events << CardResponded;
        view_as_skill = new zhanyivs;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardStar card_star = data.value<CardResponseStruct>().m_card;
        if (card_star->getSkillName() == "zhanyi" && card_star->isRed())
            player->drawCards(1);
        return false;
    }
};


class dongjie : public TriggerSkill {
public:
    dongjie() : TriggerSkill("dongjie") {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user)
            return false;
        if (damage.from == NULL || damage.from == damage.to)
            return false;
        if (damage.card != NULL && damage.card->isKindOf("Slash")){
            player->tag["dongjie_damage"] = QVariant::fromValue(damage);
            if (room->askForSkillInvoke(player, "dongjie", QVariant::fromValue(damage.to))){
                if (player->getGeneralName() == "hmx006" && !player->hasFlag("dongjieAnimate")
                    && damage.to->faceUp()){
                    room->doLightbox("$dongjieAnimate", 2000);
                    room->setPlayerFlag(player, "dongjieAnimate");
                }

                QList<ServerPlayer *> logto;
                logto << damage.to;
                room->touhouLogmessage("#Dongjie", player, "dongjie", logto);
                player->drawCards(1);
                damage.to->turnOver();
                damage.to->drawCards(1);
                return true;
            }
        }
        return false;
    }

};

class bingpo : public TriggerSkill {
public:
    bingpo() : TriggerSkill("bingpo") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Fire) {
            if (damage.damage > 1 || player->getHp() == 1){
                room->touhouLogmessage("#bingpolog", player, "bingpo", QList<ServerPlayer *>(), QString::number(damage.damage));
                room->notifySkillInvoked(player, objectName());
                return true;
            }
        }
        return false;
    }

};


class bendan : public FilterSkill{
public:
    bendan() : FilterSkill("bendan"){

    }

    virtual bool viewFilter(const Card *to_select) const{
        Room *room = Sanguosha->currentRoom();

        ServerPlayer *cirno = room->getCardOwner(to_select->getEffectiveId());
        return (cirno != NULL && cirno->hasSkill(objectName()));
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        WrappedCard *wrap = Sanguosha->getWrappedCard(originalCard->getEffectiveId());
        wrap->setNumber(9);
        wrap->setSkillName(objectName());
        wrap->setModified(true);
        return wrap;
    }
};

class zhenye : public TriggerSkill {
public:
    zhenye() : TriggerSkill("zhenye") {
        events << EventPhaseStart;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Finish){
            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@zhenye-select", true, true);
            if (target != NULL){
                if (player->getGeneralName() == "hmx007")
                    room->doLightbox("$zhenyeAnimate", 2000);

                player->turnOver();
                target->turnOver();
            }
        }
        return false;
    }
};

class anyu : public TriggerSkill {
public:
    anyu() : TriggerSkill("anyu") {
        events << TargetConfirming;
        frequency = Compulsory;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isBlack() && (use.card->isKindOf("Slash") || use.card->isNDTrick())){
            room->touhouLogmessage("#TriggerSkill", player, "anyu");
            QString choice = room->askForChoice(player, objectName(), "turnover+draw", data);
            room->notifySkillInvoked(player, objectName());
            if (choice == "turnover")
                player->turnOver();
            else
                player->drawCards(1);
        }
        return false;
    }
};

class qiyue : public TriggerSkill {
public:
    qiyue() : TriggerSkill("qiyue") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (source == NULL || player == source)
            return false;
        if (player->getPhase() == Player::Start){
            QString prompt = "target:" + player->objectName();
            if (room->askForSkillInvoke(source, objectName(), prompt)){
                source->drawCards(1);
                QString choice = room->askForChoice(source, objectName(), "hp_moxue+maxhp_moxue", data);
                if (choice == "hp_moxue")
                    room->loseHp(source, 1);
                else
                    room->loseMaxHp(source, 1);

                player->skip(Player::Judge);
                player->skip(Player::Draw);
            }
        }
        return false;
    }
};


class moxue : public TriggerSkill {
public:
    moxue() : TriggerSkill("moxue") {
        events << MaxHpChanged;
        frequency = Compulsory;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getMaxHp() == 1) {
            if (player->getGeneralName() == "hmx008")
                room->doLightbox("$moxueAnimate", 4000);
            room->touhouLogmessage("#TriggerSkill", player, "moxue");
            room->notifySkillInvoked(player, objectName());
            player->drawCards(player->getHandcardNum());
        }
        return false;
    }
};



class juxian : public TriggerSkill {
public:
    juxian() : TriggerSkill("juxian") {
        events << Dying;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *who = room->getCurrentDyingPlayer();
        if (player == who && player->faceUp()){
            if (player->askForSkillInvoke(objectName(), data)) {
                if (player->getGeneralName() == "hmx009")
                    room->doLightbox("$juxianAnimate", 2000);
                player->turnOver();
                QList<int> list = room->getNCards(3);
                room->fillAG(list);
                QStringList e;
                QVariantList listc;
                foreach(int c, list){
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
                DummyCard *dummy = new DummyCard;
                foreach(int id, list){
                    if (Sanguosha->getCard(id)->getSuit() != suit)
                        get << id;
                    else
                        dummy->addSubcard(id);
                }
                if (!get.isEmpty()){
                    CardsMoveStruct move;
                    move.card_ids = get;
                    move.reason.m_reason = CardMoveReason::S_REASON_DRAW;
                    move.to = player;
                    move.to_place = Player::PlaceHand;
                    room->moveCardsAtomic(move, true);
                }
                if (dummy->getSubcards().length() > 0){
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), "");
                    room->throwCard(dummy, reason, NULL);
                    RecoverStruct recover;
                    recover.recover = dummy->getSubcards().length();
                    room->recover(player, recover);
                }
                room->clearAG();
            }
        }
        return false;
    }
};

banyueCard::banyueCard() {
    mute = true;
    m_skillName = "banyue";
}
bool banyueCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return (targets.length() < 3);
}
void banyueCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->loseHp(source);
    foreach(ServerPlayer *p, targets){
        if (p->isAlive())
            p->drawCards(1);
    }
}
class banyue : public ZeroCardViewAsSkill {
public:
    banyue() : ZeroCardViewAsSkill("banyue") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("banyueCard");
    }

    virtual const Card *viewAs() const{
        return new banyueCard;
    }
};


th06Package::th06Package()
    : Package("th06")
{
    General *hmx001 = new General(this, "hmx001$", "hmx", 3, false);
    hmx001->addSkill(new skltkexue);
    hmx001->addSkill(new mingyun);
    hmx001->addSkill(new skltxueyi);

    General *hmx002 = new General(this, "hmx002", "hmx", 3, false);
    hmx002->addSkill(new pohuai);
    hmx002->addSkill(new yuxue);
    hmx002->addSkill(new yuxue_buff);
    hmx002->addSkill(new yuxuedis);
    hmx002->addSkill(new shengyan);
    related_skills.insertMulti("yuxue", "#yuxue");
    related_skills.insertMulti("yuxue", "#yuxue-dis");

    General *hmx003 = new General(this, "hmx003", "hmx", 4, false);
    hmx003->addSkill(new suoding);
    hmx003->addSkill(new huisu);

    General *hmx004 = new General(this, "hmx004", "hmx", 3, false);
    hmx004->addSkill(new bolan);
    hmx004->addSkill(new qiyao);

    General *hmx005 = new General(this, "hmx005", "hmx", 4, false);
    hmx005->addSkill(new douhun);
    hmx005->addSkill(new zhanyi);

    General *hmx006 = new General(this, "hmx006", "hmx", 3, false);
    hmx006->addSkill(new dongjie);
    hmx006->addSkill(new bingpo);
    hmx006->addSkill(new bendan);


    General *hmx007 = new General(this, "hmx007", "hmx", 3, false);
    hmx007->addSkill(new zhenye);
    hmx007->addSkill(new anyu);

    General *hmx008 = new General(this, "hmx008", "hmx", 3, false);
    hmx008->addSkill(new qiyue);
    hmx008->addSkill(new moxue);

    General *hmx009 = new General(this, "hmx009", "hmx", 3, false);
    hmx009->addSkill(new juxian);
    hmx009->addSkill(new banyue);


    addMetaObject<skltkexueCard>();
    addMetaObject<suodingCard>();
    addMetaObject<banyueCard>();

    skills << new skltkexuepeach;
}

ADD_PACKAGE(th06)

