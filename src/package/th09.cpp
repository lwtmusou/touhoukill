#include "th09.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"
#include "maneuvering.h"




class shenpan : public TriggerSkill {
public:
    shenpan() : TriggerSkill("shenpan") {
        events << EventPhaseStart << EventPhaseEnd;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL) && (target->hasFlag(objectName()) || target->hasSkill(objectName()));
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::Draw)
            return false;

        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@shenpan-select", true, true);
            if (target != NULL){
                room->damage(DamageStruct(objectName(), player, target, 1, DamageStruct::Thunder));
                player->tag["shenpan"] = QVariant::fromValue(target);
                player->setFlags("shenpan");
                return true;
            }
        } else if (triggerEvent == EventPhaseEnd) {
            player->setFlags("-shenpan");
            ServerPlayer *target = player->tag["shenpan"].value<ServerPlayer *>();
            if (target != NULL){
                player->tag.remove("shenpan");
                if (target->getHandcardNum() > target->getHp()){
                    if (room->askForSkillInvoke(player, "shenpan", "drawcard:" + target->objectName()))
                        room->drawCards(player, 1);
                }
            }
        }
        return false;
    }
};

class huiwu : public TriggerSkill {
public:
    huiwu() : TriggerSkill("huiwu") {
        events << TargetConfirming << CardEffected << SlashEffected;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && (use.card->isKindOf("Slash") || (use.card->isNDTrick()))) {
                if (use.from == NULL || use.from->isDead() || use.from == player)
                    return false;

                use.from->tag["huiwu"] = QVariant::fromValue(player);
                room->setTag("huiwu_use", data);

                QString    prompt = "target:" + player->objectName() + ":" + use.card->objectName();

                if (use.from->askForSkillInvoke(objectName(), prompt)){
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, use.from->objectName(), player->objectName());
            
                    //for ai to judge a card if it is already SkillNullified for this player,
                    //the first line is more suitable than the second one.
                    room->setCardFlag(use.card, "huiwu" + player->objectName());
                    //room->setCardFlag(use.card, "huiwuSkillNullify",player);

                    QList<ServerPlayer *> logto;
                    logto << player;
                    room->touhouLogmessage("#InvokeOthersSkill", use.from, objectName(), logto);
                    room->notifySkillInvoked(player, objectName());
                    player->drawCards(1);
                }
                use.from->tag.remove("huiwu");
            }
        } else if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card != NULL && effect.card->isNDTrick()
                && effect.card->hasFlag("huiwu" + effect.to->objectName())){
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.card->objectName(), QList<ServerPlayer *>(), objectName());
                room->setEmotion(effect.to, "skill_nullify");
                return true;
            }
        } else if (triggerEvent == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("huiwu" + effect.to->objectName())) {
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.slash->objectName(), QList<ServerPlayer *>(), objectName());
                room->setEmotion(effect.to, "skill_nullify");
                return true;
            }
        }
        return false;
    }
};

class huazhong : public TriggerSkill {
public:
    huazhong() : TriggerSkill("huazhong$") {
        events << Damage << FinishJudge;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL && target->isAlive() && target->getKingdom() == "zhan");
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Damage) {
            int count = data.value<DamageStruct>().damage;
            for (int i = 0; i < count; i++) {
                QList<ServerPlayer *> targets;
                foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->hasLordSkill(objectName()))
                        targets << p;
                }
                while (!targets.isEmpty()){
                    ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@huazhong-select", true, true);
                    if (target == NULL)
                        break;
                    room->notifySkillInvoked(target, objectName());
                    player->tag["huazhong"] = QVariant::fromValue(target);
                    targets.removeOne(target);

                    JudgeStruct judge;
                    judge.pattern = ".|red";
                    judge.who = player;
                    judge.reason = objectName();
                    judge.good = true;

                    room->judge(judge);

                    player->tag.remove("huazhong");
                }
            }

        } else if (triggerEvent == FinishJudge) {
            JudgeStar judge = data.value<JudgeStar>();
            if (judge->reason == objectName() && judge->isGood()){
                ServerPlayer *lord = judge->who->tag["huazhong"].value<ServerPlayer *>();
                if (lord != NULL)
                    lord->obtainCard(judge->card);
            }
        }

        return false;
    }
};



class mingtu : public TriggerSkill {
public:
    mingtu() : TriggerSkill("mingtu") {
        frequency = Frequent;
        events << EnterDying;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        QList<ServerPlayer *> sources = room->findPlayersBySkillName(objectName());
        if (sources.length() == 0)
            return false;
        room->sortByActionOrder(sources);
        foreach(ServerPlayer *s, sources) {
            if (s->askForSkillInvoke(objectName(), data))
                s->drawCards(1);
        }
        return false;
    }
};

class silian : public TriggerSkill {
public:
    silian() : TriggerSkill("silian") {
        frequency = Compulsory;
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user)
            return false;
        if (!damage.from  || !damage.to || damage.from == damage.to)
            return false;
        if (damage.card && damage.card->isKindOf("Slash") && damage.to->getHp() == 1){
            QList<ServerPlayer *> logto;
            logto << damage.to;
            room->touhouLogmessage("#TriggerSkill", player, "silian", logto);
            room->notifySkillInvoked(player, objectName());
            damage.damage = damage.damage + 2;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};


class weiya : public TriggerSkill {
public:
    weiya() : TriggerSkill("weiya") {
        frequency = Compulsory;
        events << CardUsed << SlashEffected << CardEffected << CardResponded;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer  *current = room->getCurrent();
        if (!current->hasSkill(objectName()) || !current->isAlive())
            return false;
        QString weiya_pattern;
        if (triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from == current)
                return false;
            if (use.card->isKindOf("Nullification")){
                room->notifySkillInvoked(current, objectName());
                room->touhouLogmessage("#weiya_ask", player, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                if (room->askForCard(player, "nullification", "@weiya:nullification", data, Card::MethodDiscard))
                    return false;
                room->touhouLogmessage("#weiya", player, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                room->setPlayerFlag(player, "nullifiationNul");
            } else if (use.card->isKindOf("BasicCard")){
                player->setFlags("weiya_ask");
                weiya_pattern = use.card->objectName();
                if (use.card->isKindOf("Slash"))
                    weiya_pattern = "slash";
                room->notifySkillInvoked(current, objectName());
                room->touhouLogmessage("#weiya_ask", player, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                if (room->askForCard(player, weiya_pattern, "@weiya:" + use.card->objectName(), data, Card::MethodDiscard))
                    return false;
                room->touhouLogmessage("#weiya", player, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                room->setCardFlag(use.card, "weiyaSkillNullify");
            }
        } else if (triggerEvent == SlashEffected){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("weiyaSkillNullify")) //effect.slash!=NULL &&
                return true;
        } else if (triggerEvent == CardEffected){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->hasFlag("weiyaSkillNullify")) //effect.card!=NULL && 
                return true;
        } else if (triggerEvent == CardResponded){
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if (player == current || !card_star->isKindOf("BasicCard")
                || data.value<CardResponseStruct>().m_isRetrial
                || data.value<CardResponseStruct>().m_isProvision)
                return false;
            weiya_pattern = card_star->objectName();
            if (card_star->isKindOf("Slash"))
                weiya_pattern = "slash";
            room->notifySkillInvoked(current, objectName());
            room->touhouLogmessage("#weiya_ask", player, objectName(), QList<ServerPlayer *>(), card_star->objectName());
            if (room->askForCard(player, weiya_pattern, "@weiya:" + card_star->objectName(), data, Card::MethodDiscard))
                return false;
            room->touhouLogmessage("#weiya", player, objectName(), QList<ServerPlayer *>(), card_star->objectName());
            room->setPlayerFlag(player, "respNul");
        }
        return false;
    }
};


class judu : public TriggerSkill {
public:
    judu() : TriggerSkill("judu") {
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->isAlive()){
            QVariant _data = QVariant::fromValue(damage.to);
            if (!player->askForSkillInvoke(objectName(), _data))
                return false;
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());
            JudgeStruct judge;
            judge.reason = objectName();
            judge.who = player;
            judge.good = true;
            judge.pattern = ".|black|2~9";
            //judge.negative = true;
            room->judge(judge);

            if (judge.isGood()){
                room->damage(DamageStruct(objectName(), player, damage.to, 1, DamageStruct::Normal));
            }
        }

        return false;
    }
};

class henyi : public TriggerSkill {
public:
    henyi() : TriggerSkill("henyi") {
        events << Damaged;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ArcheryAttack *card = new ArcheryAttack(Card::NoSuit, 0);
        if (player->isCardLimited(card, Card::MethodUse))
            return false;
        if (player->askForSkillInvoke(objectName(), data)){
            card->setSkillName("_henyi");
            CardUseStruct carduse;
            carduse.card = card;
            carduse.from = player;
            room->useCard(carduse);
        }
        return false;
    }
};


class toupai : public PhaseChangeSkill {
public:
    toupai() :PhaseChangeSkill("toupai") {

    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if (player->getPhase() == Player::Draw) {
            Room *room = player->getRoom();
            QList<ServerPlayer *> players;
            foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
                if (player->canDiscard(p, "h"))
                    players << p;
            }
            if (players.isEmpty())
                return false;
            ServerPlayer *target = room->askForPlayerChosen(player, players, objectName(), "@toupai-select", true, true);
            if (target != NULL){
                for (int i = 0; i < 3; i++) {
                    QList<int>  ids;
                    foreach(const Card *c, target->getCards("h")){
                        if (c->isRed())
                            ids << c->getEffectiveId();

                    }
                    int id = room->doGongxin(player, target, ids, "toupai");

                    if (id == -1)
                        return true;
                    (CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
                    CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, player->objectName(), "", "toupai", "");
                    room->throwCard(Sanguosha->getCard(id), reason, target, player);
                    if (Sanguosha->getCard(id)->isKindOf("BasicCard"))
                        player->drawCards(1);
                }
                return true;
            }
        }

        return false;
    }
};


class zuiyue : public TriggerSkill {
public:
    zuiyue() : TriggerSkill("zuiyue") {
        events << CardUsed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::Play)
            return false;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("TrickCard") && Analeptic::IsAvailable(player)
            && room->askForSkillInvoke(player, objectName(), data)) {
            CardUseStruct ana_use;
            ana_use.from = player;
            Analeptic *card = new Analeptic(Card::NoSuit, 0);
            card->setSkillName(objectName());
            ana_use.card = card;
            room->useCard(ana_use);
        }
        return false;
    }
};

class doujiu : public TriggerSkill {
public:
    doujiu() : TriggerSkill("doujiu") {
        events << CardUsed << CardEffected;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL && target->isAlive());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();

            if (!use.card->isKindOf("Peach") && !use.card->isKindOf("Analeptic"))
                return false;
            if (!use.to.contains(player) || player->getPhase() != Player::Play)
                return false;
            ServerPlayer *target = player; 
            //if (target->hasFlag("Global_Dying") )
            //    return false;
            if (target->isKongcheng() || target->getHp() < 1)
                return false;
            ServerPlayer *source = room->findPlayerBySkillName(objectName());
            if (source != NULL && target != source) {

                QVariant _data = QVariant::fromValue(target);
                source->tag["doujiu_target"] = _data;
                if (!room->askForSkillInvoke(source, objectName(), _data))
                    return false;
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), target->objectName());
            
                source->drawCards(1);
                if (!source->isKongcheng() && source->pindian(target, objectName())){
                    if (source->isWounded()){
                        RecoverStruct recover;
                        recover.recover = 1;
                        room->recover(source, recover);

                    }
                    room->setCardFlag(use.card, "doujiu" + target->objectName());
                    data = QVariant::fromValue(use);
                    room->setPlayerFlag(target, "Global_PlayPhaseTerminated");
                }

            }

        }
        else if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->hasFlag("doujiu" + effect.to->objectName()))
                return true;
        }

        return false;
    }
};



yanhuiCard::yanhuiCard() {
    //will_throw = false;
    handling_method = Card::MethodUse;
    m_skillName = "yanhuivs";
    //mute = true;
}
bool yanhuiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && to_select->isWounded() && to_select->hasLordSkill("yanhui");
}

const Card *yanhuiCard::validate(CardUseStruct &card_use) const{
    Card *card = Sanguosha->getCard(subcards.first());
    card->setSkillName("yanhui");
    return card;
}


class yanhuivs : public OneCardViewAsSkill {
public:
    yanhuivs() :OneCardViewAsSkill("yanhuivs") {
        attached_lord_skill = true;
        filter_pattern = "Peach";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (player->getKingdom() != "zhan")
            return false;
        foreach(const Player *p, player->getSiblings()){
            if (p->hasLordSkill("yanhui") && p->isAlive() && p->isWounded())
                return true;
        }
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        yanhuiCard *card = new yanhuiCard;
        card->addSubcard(originalCard);

        return card;
    }
};
class yanhui : public TriggerSkill {
public:
    yanhui() : TriggerSkill("yanhui$") {
        events << GameStart << EventAcquireSkill << EventLoseSkill << PreCardUsed;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "yanhui")) {
            QList<ServerPlayer *> lords;
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty()) return false;

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach(ServerPlayer *p, players) {
                if (!p->hasSkill("yanhuivs"))
                    room->attachSkillToPlayer(p, "yanhuivs");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "yanhui") {
            QList<ServerPlayer *> lords;
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.length() > 2) return false;

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach(ServerPlayer *p, players) {
                if (p->hasSkill("yanhuivs"))
                    room->detachSkillFromPlayer(p, "yanhuivs", true);
            }
        } else if (triggerEvent == PreCardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            foreach(ServerPlayer *p, use.to){
                if (p->hasSkill("yanhui") && p != use.from){
                    if (use.card->isKindOf("Analeptic") && p->hasFlag("Global_Dying")){
                        room->notifySkillInvoked(p, objectName());
                    }
                    else if (use.card->isKindOf("Peach") && use.from->getPhase() == Player::Play){
                        room->notifySkillInvoked(p, objectName());
                    }
                }
            }
        }

        return false;
    }
};


class feixiang : public TriggerSkill {
public:
    feixiang() : TriggerSkill("feixiang") {
        events << AskForRetrial;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getAlivePlayers()) {
            if (!p->isKongcheng())
                targets << p;
        }
        if (targets.isEmpty())
            return false;

        QString prompt = "@feixiang-playerchosen:" + judge->who->objectName() + ":" + judge->reason;
        player->tag["feixiang_judge"] = data;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), prompt, true, true);
        player->tag.remove("feixiang_judge");
        if (target != NULL){
            int card_id = room->askForCardChosen(player, target, "h", objectName());
            Card *card = Sanguosha->getCard(card_id);
            room->showCard(target, card_id);
            if (!target->isCardLimited(card, Card::MethodResponse))
                room->retrial(card, target, judge, objectName());
        }
        return false;
    }
};

class dizhen : public TriggerSkill {
public:
    dizhen() : TriggerSkill("dizhen") {
        events << FinishJudge;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (source != NULL && judge->card->isRed() && judge->who->getHp() > 0){
            source->tag["dizhen_judge"] = data;

            QString prompt = "target:" + judge->who->objectName() + ":" + judge->reason;
            if (room->askForSkillInvoke(source, objectName(), prompt)){
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), judge->who->objectName());
            
                room->damage(DamageStruct(objectName(), source, judge->who, 1, DamageStruct::Normal));
            }
        }
        return false;
    }
};

/*class tianren: public TriggerSkill {
public:
tianren(): TriggerSkill("tianren$") {
events  <<TargetConfirming;
}

virtual bool triggerable(const ServerPlayer *target) const{
return (target != NULL && target->isAlive() && target->getKingdom() == "zhan");
}

virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
CardUseStruct use = data.value<CardUseStruct>();
if (use.card !=NULL && (use.card->isKindOf("ExNihilo") || use.card->isKindOf("AmazingGrace"))){
QList<ServerPlayer *> targets;
foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
if (p->hasLordSkill(objectName()))
targets<< p;
}
while (!targets.isEmpty()){
ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@"+objectName(), true,true);
if (target==NULL)
break;
room->notifySkillInvoked(target, objectName());
targets.removeOne(target);
target->drawCards(1);
}
}
return false;
}
};
*/

tianrenCard::tianrenCard() {
    mute = true;
    //handling_method = Card::MethodNone;
    //m_skillName = "skltkexuepeach";
}
bool tianrenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}
const Card *tianrenCard::validate(CardUseStruct &cardUse) const{
    cardUse.m_isOwnerUse = false;
    ServerPlayer *lord = cardUse.from;
    QList<ServerPlayer *> targets = cardUse.to;
    Room *room = lord->getRoom();

    room->notifySkillInvoked(lord, "tianren");
    LogMessage log;
    log.from = lord;
    log.to = targets;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    const Card *slash = NULL;
    QList<ServerPlayer *> lieges = room->getLieges("zhan", lord);
    foreach(ServerPlayer *target, targets)
        target->setFlags("tianrenTarget");
    foreach(ServerPlayer *liege, lieges) {
        try {
            slash = room->askForCard(liege, "slash", "@tianren-slash:" + lord->objectName(),
                QVariant(), Card::MethodResponse, lord, false, QString(), true);
        }
        catch (TriggerEvent triggerEvent) {
            if (triggerEvent == TurnBroken || triggerEvent == StageChange) {
                foreach(ServerPlayer *target, targets)
                    target->setFlags("-tianrenTarget");
            }
            throw triggerEvent;
        }

        if (slash) {
            foreach(ServerPlayer *target, targets)
                target->setFlags("-tianrenTarget");

            return slash;
        }
    }
    foreach(ServerPlayer *target, targets)
        target->setFlags("-tianrenTarget");
    room->setPlayerFlag(lord, "Global_tianrenFailed");
    return NULL;
}

class tianrenvs : public ZeroCardViewAsSkill {
public:
    tianrenvs() : ZeroCardViewAsSkill("tianren$") {

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    static bool hasZhanGenerals(const Player *player){
        foreach(const Player *p, player->getAliveSiblings()){
            if (p->getKingdom() == "zhan")
                return true;
        }
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return hasZhanGenerals(player) && (pattern == "slash")
            && (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            && (!player->hasFlag("Global_tianrenFailed"))
            && player->isCurrent();
            //(player->getPhase() == Player::NotActive)
    }

    virtual const Card *viewAs() const{
        return new tianrenCard;
    }
};

class tianren : public TriggerSkill {
public:
    tianren() : TriggerSkill("tianren$") {
        events << CardAsked;
        view_as_skill = new tianrenvs;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL && target->hasLordSkill(objectName()));
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->isCurrent())//if (player->getPhase() != Player::NotActive)
            return false;
        QString pattern = data.toStringList().first();
        QString prompt = data.toStringList().at(1);

        bool can = ((pattern == "slash") || (pattern == "jink"));
        if ((!can) || prompt.startsWith("@tianren"))
            return false;
        //need check card limit
        Card *dummy = Sanguosha->cloneCard(pattern);
        //need check
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE){
            if (player->isCardLimited(dummy, Card::MethodResponse))
                return false;
        } else if (player->isCardLimited(dummy, Card::MethodUse))
            return false;


        QList<ServerPlayer *> lieges = room->getLieges("zhan", player);
        //for ai  to add global flag
        //slashsource or jinksource
        if (pattern == "slash")
            room->setTag("tianren_slash", true);
        else
            room->setTag("tianren_slash", false);
        if (!lieges.isEmpty() && player->askForSkillInvoke(objectName(), data)){
            QVariant tohelp = QVariant::fromValue((PlayerStar)player);
            foreach(ServerPlayer *liege, lieges) {
                const Card *resp = room->askForCard(liege, pattern, "@tianren-" + pattern + ":" + player->objectName(),
                    tohelp, Card::MethodResponse, player, false, QString(), true);
                if (resp) {
                    room->provide(resp);
                    return true;
                }
            }
        }
        return false;
    }
};


class jingdian : public TriggerSkill {
public:
    jingdian() : TriggerSkill("jingdian") {
        frequency = Compulsory;
        events << DamageInflicted;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        if (damage.nature == DamageStruct::Thunder){
            room->touhouLogmessage("#jingdian", player, "jingdian", QList<ServerPlayer *>(), QString::number(damage.damage));
            room->notifySkillInvoked(player, objectName());
            for (int i = 0; i < damage.damage; i++)
                room->drawCards(player, 3, objectName());
            return true;
        }
        return false;
    }
};

class leiyun : public OneCardViewAsSkill {
public:
    leiyun() : OneCardViewAsSkill("leiyun") {
        response_or_use = true;
        filter_pattern = ".|spade,heart|.|hand";
    }


    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard != NULL){
            Lightning *card = new Lightning(originalCard->getSuit(), originalCard->getNumber());
            card->addSubcard(originalCard);
            card->setSkillName("leiyun");
            return card;
        } else
            return NULL;
    }
};


class kuaizhao : public DrawCardsSkill {
public:
    kuaizhao() : DrawCardsSkill("kuaizhao") {
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *>  others = room->getOtherPlayers(player);
        foreach(ServerPlayer *p, room->getOtherPlayers(player)){
            if (p->isKongcheng() || !player->inMyAttackRange(p))
                others.removeOne(p);
        }

        if (others.length() == 0)
            return n;
        ServerPlayer *target = room->askForPlayerChosen(player, others, objectName(), "@kuaizhao-select_one", true, true);
        if (target != NULL){
            int num = 0;
            room->showAllCards(target);
            room->getThread()->delay(1000);
            room->clearAG();
            foreach(const Card *c, target->getCards("h")){
                if (c->isKindOf("BasicCard"))
                    num++;
            }
            room->setPlayerMark(player, "kuaizhaoUsed", num);
            return n - 1;
        } else
            return n;
    }
};

class kuaizhao_effect : public TriggerSkill {
public:
    kuaizhao_effect() : TriggerSkill("#kuaizhao") {
        events << AfterDrawNCards;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getMark("kuaizhaoUsed") > 0)
            room->drawCards(player, qMin(2, player->getMark("kuaizhaoUsed")));
        room->setPlayerMark(player, "kuaizhaoUsed", 0);
        return false;
    }
};


class duanjiao : public AttackRangeSkill{
public:
    duanjiao() : AttackRangeSkill("duanjiao"){

    }

    virtual int getFixed(const Player *target, bool) const{
        if (target->hasSkill("duanjiao"))
            return 3;
        else
            return -1;
    }
};





th09Package::th09Package()
    : Package("th09")
{
    General *zhan001 = new General(this, "zhan001$", "zhan", 3, false);
    zhan001->addSkill(new shenpan);
    zhan001->addSkill(new huiwu);
    zhan001->addSkill(new huazhong);

    General *zhan002 = new General(this, "zhan002", "zhan", 4, false);
    zhan002->addSkill(new silian);
    zhan002->addSkill(new mingtu);

    General *zhan003 = new General(this, "zhan003", "zhan", 4, false);
    zhan003->addSkill(new weiya);

    General *zhan004 = new General(this, "zhan004", "zhan", 3, false);
    zhan004->addSkill(new judu);
    zhan004->addSkill(new henyi);

    General *zhan005 = new General(this, "zhan005", "zhan", 4, false);
    zhan005->addSkill(new toupai);

    General *zhan006 = new General(this, "zhan006$", "zhan", 3, false);
    zhan006->addSkill(new zuiyue);
    zhan006->addSkill(new doujiu);
    zhan006->addSkill(new yanhui);

    General *zhan007 = new General(this, "zhan007$", "zhan", 4, false);
    zhan007->addSkill(new feixiang);
    zhan007->addSkill(new dizhen);
    zhan007->addSkill(new tianren);


    General *zhan008 = new General(this, "zhan008", "zhan", 4, false);
    zhan008->addSkill(new jingdian);
    zhan008->addSkill(new leiyun);

    General *zhan009 = new General(this, "zhan009", "zhan", 4, false);
    zhan009->addSkill(new kuaizhao);
    zhan009->addSkill(new kuaizhao_effect);
    zhan009->addSkill(new duanjiao);
    related_skills.insertMulti("kuaizhao", "#kuaizhao");

    General *zhan010 = new General(this, "zhan010", "zhan", 4, false);

    addMetaObject<yanhuiCard>();
    addMetaObject<tianrenCard>();
    skills << new yanhuivs;
}

ADD_PACKAGE(th09)

