#include "thndj.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"




class rexue_count : public TriggerSkill {
public:
    rexue_count() : TriggerSkill("#rexue_count") {
        events << Death << TurnStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Death){
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;
            room->setTag("rexue_count", true);
        }
        if (triggerEvent == TurnStart)
            room->setTag("rexue_count", false);

        return false;
    }
};
class rexue : public TriggerSkill {
public:
    rexue() : TriggerSkill("rexue") {
        events << EventPhaseStart;// EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        //PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        //change.to == Player::NotActive
        if (player->getPhase() == Player::NotActive && player->getHp() == 1
            && !room->getTag("rexue_count").toBool()
            ) {

            room->touhouLogmessage("#TriggerSkill", player, "rexue");
            room->notifySkillInvoked(player, "rexue");
            room->recover(player, RecoverStruct());
            //need set extra turn mark to zero,since canInsertExtraTurn()
            room->setPlayerMark(player, "touhou-extra", 0);

            if (room->canInsertExtraTurn()) {
                room->touhouLogmessage("#touhouExtraTurn", player, objectName());
                player->gainAnExtraTurn();
            }
        }
        return false;
    }
};
class sidou : public TriggerSkill {
public:
    sidou() : TriggerSkill("sidou") {
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        if (player->getPhase() == Player::Start){
            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getAlivePlayers()){
                if (player->canDiscard(p, "ej", "sidou"))
                    targets << p;
            }
            if (targets.length() > 0){
                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@sidou_target", true, true);
                if (target != NULL){
                    QList<int> disable;
                    foreach(const Card *equip, target->getEquips()){
                        if (equip->isKindOf("Weapon") && player->getMark("@tianyi_Weapon") == 0)
                            disable << equip->getId();
                    }
                    int card_id = room->askForCardChosen(player, target, "je", objectName(), false, Card::MethodDiscard, disable);
                    room->throwCard(card_id, target, player);
                    player->drawCards(1);
                    room->loseHp(player);
                }
            }
        }
        return false;
    }
};

class tymhwuyu : public TriggerSkill {
public:
    tymhwuyu() : TriggerSkill("tymhwuyu$") {
        events << Death;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        bool killer_change = false;
        if (death.who == source || source->getRole() != "lord")
            return false;
        if (death.damage != NULL){
            if (death.damage->from == NULL || death.damage->from != source)
                killer_change = true;
            death.damage->from = source;
        }
        else {
            DamageStruct *die = new DamageStruct();
            die->from = source;
            death.damage = die;
            killer_change = true;
        }
        data = QVariant::fromValue(death);
        if (killer_change){
            room->touhouLogmessage("#TriggerSkill", source, "tymhwuyu");
            room->notifySkillInvoked(player, objectName());
        }
        return false;
    }
};



class huanyue : public TriggerSkill {
public:
    huanyue() : TriggerSkill("huanyue") {
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Start) {
            foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getMark("@huanyue") > 0){
                    LogMessage log;
                    log.from = player;
                    log.arg = "huanyue";
                    log.type = "#TriggerSkill";
                    room->sendLog(log);
                    room->notifySkillInvoked(player, "huanyue");
                    p->loseAllMarks("@huanyue");
                }
            }
        }
        if (player->getPhase() == Player::Play){
            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@huanyuechosen", true, true);
            if (target != NULL)
                target->gainMark("@huanyue");
        }
        return false;
    }
};
class huanyue_damage : public TriggerSkill {
public:
    huanyue_damage() : TriggerSkill("#huanyue_damage") {
        events << DamageInflicted;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->getMark("@huanyue") == 0)
            return false;
        if (damage.card == NULL || !damage.card->isNDTrick())
            return false;
        ServerPlayer *s = room->findPlayerBySkillName("huanyue");
        if (s != NULL){
            const Card *ask_card = room->askForCard(s, ".|black|.|hand", "@huanyue-discard:" + damage.to->objectName(), data, Card::MethodDiscard, NULL, true, "huanyue");
            if (ask_card != NULL){
                QList<ServerPlayer *>logto;
                logto << damage.to;
                room->touhouLogmessage("#huanyue_log", damage.from, QString::number(damage.damage), logto, QString::number(damage.damage + 1));
                damage.damage = damage.damage + 1;
                data = QVariant::fromValue(damage);
            }
        }
        return false;
    }
};

class sizhai : public TriggerSkill {
public:
    sizhai() : TriggerSkill("sizhai") {
        events << GameStart << EventPhaseStart;
        frequency = Frequent;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == GameStart){
            if (player != NULL && player->hasSkill("sizhai")){

                foreach(ServerPlayer *p, room->getAlivePlayers()){
                    if (!p->hasSkill("#sizhai_count"))
                        room->acquireSkill(p, "#sizhai_count");
                }
            }
        }
        else if (triggerEvent == EventPhaseStart){
            ServerPlayer *current = room->getCurrent();
            if (current->getPhase() == Player::Finish){
                if (!current->hasFlag("sizhai")){
                    ServerPlayer *s = room->findPlayerBySkillName(objectName());
                    if (s != NULL && room->askForSkillInvoke(s, objectName(), "draw:" + current->objectName()))
                        s->drawCards(1);
                }
            }
        }
        return false;
    }
};
class sizhai_count : public TriggerSkill {
public:
    sizhai_count() : TriggerSkill("#sizhai_count") {
        events << CardUsed << CardResponded;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        ServerPlayer *current = room->getCurrent();
        if (player != current)
            return false;
        if (triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("BasicCard"))
                player->setFlags("sizhai");
        }
        else if (triggerEvent == CardResponded) {
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isKindOf("BasicCard"))
                player->setFlags("sizhai");
        }
        return false;
    }
};

youmingCard::youmingCard() {
    will_throw = true;
    target_fixed = true;
    mute = true;
}
void youmingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->setPlayerProperty(source, "maxhp", source->getMaxHp() + 1);
    room->touhouLogmessage("#GainMaxHp", source, QString::number(1));
    room->touhouLogmessage("#GetHp", source, QString::number(source->getHp()), QList<ServerPlayer *>(), QString::number(source->getMaxHp()));
}
class youming : public OneCardViewAsSkill {
public:
    youming() : OneCardViewAsSkill("youming") {
        filter_pattern = ".|.|.|.!";
    }

    virtual bool viewFilter(const Card *to_select) const{
        return !Self->isJilei(to_select);
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isNude() && player->getMaxHp() < 4;
    }



    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard != NULL){
            youmingCard *card = new youmingCard;
            card->addSubcard(originalCard);
            return card;
        }
        else
            return NULL;
    }
};


class fanji : public TriggerSkill {
public:
    fanji() : TriggerSkill("fanji") {
        events << Damaged;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *source = room->findPlayerBySkillName("fanji");
        if (source == NULL || damage.from == NULL || damage.from->isDead()
            || damage.to->isDead() || damage.from == damage.to || damage.from == source)
            return false;
        source->tag["fanji_damage"] = data;
        QString prompt = "target:" + damage.from->objectName() + ":" + damage.to->objectName();
        if (damage.to == source){
            if (room->askForSkillInvoke(source, objectName(), prompt))
                room->damage(DamageStruct("fanji", source, damage.from));
        }
        else{
            if (source->inMyAttackRange(damage.to) && source->getMaxHp() > 1
                && room->askForSkillInvoke(source, objectName(), prompt)){
                room->loseMaxHp(source, 1);
                room->damage(DamageStruct("fanji", source, damage.from));
            }
        }
        return false;
    }
};



class liangzi : public TriggerSkill {
public:
    liangzi() : TriggerSkill("liangzi") {
        events << CardUsed << CardResponded;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        bool can = false;
        if (triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("BasicCard"))
                can = true;
        }
        else if (triggerEvent == CardResponded) {
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isKindOf("BasicCard"))
                can = true;
        }
        if (can){
            LogMessage log;
            log.from = player;
            log.arg = "liangzi";
            log.type = "#TriggerSkill";
            room->sendLog(log);
            room->notifySkillInvoked(player, "liangzi");

            player->setChained(!player->isChained());
            Sanguosha->playSystemAudioEffect("chained");
            room->broadcastProperty(player, "chained");
            room->setEmotion(player, "chain");
        }
        return false;
    }
};

class kexue : public TargetModSkill {
public:
    kexue() : TargetModSkill("kexue") {
        pattern = "Slash";
    }
    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (from->getPhase() == Player::Play  && from->hasSkill(objectName()) && from->isChained())
            return 1000;
        else
            return 0;
    }


    virtual int getExtraTargetNum(const Player *player, const Card *card) const{
        if (player->getPhase() == Player::Play  && player->hasSkill(objectName()) && player->isChained())
            return 1000;
        else
            return 0;
    }
};
class kexueEffect : public TriggerSkill {
public:
    kexueEffect() : TriggerSkill("#kexue-effect") {
        events << PreCardUsed;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        if (triggerEvent == PreCardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.to.length() > 1 && player->isChained())
                room->notifySkillInvoked(player, "kexue");
        }
        return false;
    }
};


thndjPackage::thndjPackage()
    : Package("thndj")
{
    General *ndj001 = new General(this, "ndj001$", "zhu", 4, false);
    ndj001->addSkill(new rexue);
    ndj001->addSkill(new rexue_count);
    ndj001->addSkill(new sidou);
    ndj001->addSkill(new tymhwuyu);
    related_skills.insertMulti("rexue", "#rexue_count");

    General *ndj002 = new General(this, "ndj002", "zhu", 3, false);
    ndj002->addSkill(new huanyue);
    ndj002->addSkill(new huanyue_damage);
    ndj002->addSkill(new sizhai);
    related_skills.insertMulti("huanyue", "#huanyue_damage");

    General *ndj004 = new General(this, "ndj004", "yym", 3, false);
    ndj004->addSkill(new youming);
    ndj004->addSkill(new fanji);

    General *ndj010 = new General(this, "ndj010", "wai", 1, false);

    General *ndj011 = new General(this, "ndj011", "wai", 4, false);
    ndj011->addSkill(new liangzi);
    ndj011->addSkill(new kexue);
    ndj011->addSkill(new kexueEffect);
    related_skills.insertMulti("kexue", "#kexue-effect");

    addMetaObject<youmingCard>();
    skills << new sizhai_count;
}

ADD_PACKAGE(thndj)

