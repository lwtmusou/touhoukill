#include "th08.h"
#include "th10.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"


//#include <QCommandLinkButton>

class yongheng : public TriggerSkill {
public:
    yongheng() : TriggerSkill("yongheng") {
        events << EventPhaseChanging << CardsMoveOneTime << EventAcquireSkill << MarkChanged;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    static void  adjustHandcardNum(ServerPlayer *player, int card_num, QString reason) {
        Room *room = player->getRoom();
        int hc_num = player->getHandcardNum();
        if (card_num != hc_num){
            room->touhouLogmessage("#TriggerSkill", player, "yongheng");
            room->notifySkillInvoked(player, "yongheng");
        }
        if (card_num > hc_num)
            player->drawCards(card_num - hc_num, reason);
        else if (card_num < hc_num)
            room->askForDiscard(player, reason, hc_num - card_num, hc_num - card_num);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging){
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard && player->hasSkill(objectName())){
                room->touhouLogmessage("#TriggerSkill", player, "yongheng");
                room->notifySkillInvoked(player, objectName());
                player->skip(change.to);
            } else if (change.to == Player::NotActive){
                if (player->hasSkill("yongheng"))
                    adjustHandcardNum(player, 4, objectName());
                /*else {
                    foreach (ServerPlayer *source, room->findPlayersBySkillName("yongheng")){
                        if (!source->isCurrent())
                            adjustHandcardNum(source, 4, "yongheng");
                    }
                }*/
            }else {
                foreach (ServerPlayer *source, room->findPlayersBySkillName("yongheng")){
                    if (!source->isCurrent())
                        adjustHandcardNum(source, 4, "yongheng");
                }
            }
        } else if (triggerEvent == CardsMoveOneTime && player->hasSkill(objectName())){
            if (player->getPhase() == Player::NotActive){
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if ((move.from && move.from == player && move.from_places.contains(Player::PlaceHand))
                    || (move.to && move.to == player && move.to_place == Player::PlaceHand))
                    adjustHandcardNum(player, 4, objectName());
            }
        } else if (triggerEvent == EventAcquireSkill &&  data.toString() == "yongheng"){
            if (player->getPhase() == Player::NotActive && player->hasSkill(objectName()))
                adjustHandcardNum(player, 4, objectName());
        }
        else if (triggerEvent ==MarkChanged){
            MarkChangeStruct change = data.value<MarkChangeStruct>();
            if (change.name != "@pingyi" && change.name != "@changshi")
                return false;
            if (player->getPhase() == Player::NotActive && player->hasSkill(objectName()))
                adjustHandcardNum(player, 4, objectName());
        }
        return false;
    }
};

class zhuqu : public TriggerSkill {
public:
    zhuqu() : TriggerSkill("zhuqu$") {
        events << FinishJudge;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL && target->isAlive() && target->getKingdom() == "yyc");
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        if (judge->card->getSuit() != Card::Diamond)
            return false;
        QList<ServerPlayer *> lords;
        foreach(ServerPlayer *p, room->getOtherPlayers(player)){
            if (p->hasLordSkill(objectName()) && p->isWounded())
                lords << p;
        }
        while (!lords.isEmpty()) {
            ServerPlayer *target = room->askForPlayerChosen(player, lords, objectName(), "@zhuqu", true, true);
            if (target){
                room->notifySkillInvoked(target, objectName());
                lords.removeOne(target);
                RecoverStruct recov;
                recov.recover = 1;
                recov.who = player;
                room->recover(target, recov);
            }
            else
                break;
        }
        return false;
    }
};


class ruizhi : public TriggerSkill {
public:
    ruizhi() : TriggerSkill("ruizhi") {
        events << PostCardEffected << CardEffected;
        skill_property = "cause_judge";
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == CardEffected){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->isNDTrick()){
                player->tag["ruizhi_effect"] = data;//for ai need damage
                //when triggerEvent  postcardeffected,the effect card information which is transformed willbe cleared.
                //we can not find the real name in cardused,if this card is transformed
                player->setFlags("ruizhi_effect");
            }
        } else if (triggerEvent == PostCardEffected){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            QVariantList ids1 =  player->tag["ruizhi_effect"].toList();
            if (effect.card->isNDTrick() && effect.to == player
                && player->isWounded() && player->hasFlag("ruizhi_effect")){
                player->setFlags("-ruizhi_effect");
                QString prompt = "invoke:" + effect.card->objectName();
                if (room->askForSkillInvoke(player, objectName(), prompt)) {
                    JudgeStruct  judge;
                    judge.who = player;
                    judge.pattern = ".|red";
                    judge.good = true;
                    judge.reason = objectName();
                    room->judge(judge);
                    if (judge.isGood()) {
                        RecoverStruct recover;
                        recover.recover = 1;
                        room->recover(player, recover);
                    }
                }

            }
        }
        return false;
    }
};

miyaoCard::miyaoCard() {
    mute = true;
}
bool miyaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || to_select == Self)
        return false;
    return !to_select->isKongcheng();
}
void miyaoCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    if (effect.to->isWounded()){
        RecoverStruct     recover;
        recover.recover = 1;
        recover.who = effect.from;
        room->recover(effect.to, recover);
    }
    const Card *cards = room->askForExchange(effect.to, "miyao", 1, false, "miyao_cardchosen");
    room->throwCard(cards, effect.to);
}
class miyao : public ZeroCardViewAsSkill {
public:
    miyao() : ZeroCardViewAsSkill("miyao") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("miyaoCard");
    }

    virtual const Card *viewAs() const{
        return new miyaoCard;
    }
};

class bumie : public TriggerSkill {
public:
    bumie() : TriggerSkill("bumie") {
        events << DamageInflicted << PreHpLost;
        frequency = Compulsory;
    }

    virtual int getPriority(TriggerEvent) const{
        return -100;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        int will_losehp;
        int original_damage;
        if (triggerEvent == DamageInflicted)
            will_losehp = data.value<DamageStruct>().damage;
        else
            will_losehp = data.toInt();
        original_damage = will_losehp;

        int hp = player->getHp();

        if (hp - will_losehp < 1)
            will_losehp = hp - 1;
        if (will_losehp < original_damage){
            room->touhouLogmessage("#bumie01", player, "bumie", QList<ServerPlayer *>(), QString::number(will_losehp));
            room->notifySkillInvoked(player, objectName());
        }
        if (will_losehp <= 0)
            return true;

        if (triggerEvent == DamageInflicted){
            DamageStruct damage = data.value<DamageStruct>();
            damage.damage = will_losehp;
            data = QVariant::fromValue(damage);
        } else
            data = QVariant::fromValue(will_losehp);
        return false;
    }
};

class bumie_maxhp : public TriggerSkill {
public:
    bumie_maxhp() : TriggerSkill("#bumie") {
        events << HpChanged << CardsMoveOneTime << EventPhaseChanging;
        frequency = Compulsory;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        bool need_judge = false;
        ServerPlayer *src = room->findPlayerBySkillName("bumie");
        if (src == NULL)
            return false;
        if (triggerEvent == HpChanged) {
            if (player == src && player->getHp() == 1 && player->isKongcheng())
                need_judge = true;
        } else if (triggerEvent == CardsMoveOneTime){
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (player == src && player->getHp() == 1 && move.from && move.from == player
                && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard)
                need_judge = true;
        } else if (triggerEvent == EventPhaseChanging){
            if (src->getHp() == 1 && src->isKongcheng())
                need_judge = true;
        }
        if (need_judge){
            JudgeStruct judge;
            judge.who = src;
            judge.reason = "bumie";
            judge.pattern = ".|diamond";
            judge.good = true;
            //judge.negative = true;

            room->touhouLogmessage("#TriggerSkill", src, "bumie");
            room->notifySkillInvoked(src, "bumie");

            room->judge(judge);
            src->obtainCard(judge.card);
            if (!judge.isGood()) {
                room->loseMaxHp(src);
                if (src->isWounded()){
                    RecoverStruct recover;
                    recover.recover = src->getLostHp();
                    room->recover(src, recover);
                }
            }

        }

        return false;
    }
};

class lizhan : public TriggerSkill {
public:
    lizhan() : TriggerSkill("lizhan") {
        events << EventPhaseEnd << CardsMoveOneTime << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *current = room->getCurrent();
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (!source || !current || source->getPhase() != Player::NotActive)
            return false;

        if (triggerEvent == CardsMoveOneTime){
            if (current->getPhase() != Player::Discard)
                return false;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile){
                QVariantList ids1 = source->tag["lizhan"].toList();
                foreach(int id, move.card_ids){
                    if (Sanguosha->getCard(id)->isKindOf("Slash") && !ids1.contains(id))
                        ids1 << id;
                }
                source->tag["lizhan"] = ids1;
            }
        } else if (triggerEvent == EventPhaseEnd && current->getPhase() == Player::Discard){

            QVariantList ids = source->tag["lizhan"].toList();
            if (ids.length() == 0)
                return false;
            source->tag["lizhan_target"] = QVariant::fromValue(current);
            QString prompt = "target:" + current->objectName();
            QString prompt1 = "@lizhan_slash:" + current->objectName();

            QList<int> all;
            foreach(QVariant card_data, ids){
                if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
                    all << card_data.toInt();
            }
            if (all.length() == 0)
                return false;
            if (room->askForSkillInvoke(source, objectName(), prompt)) {
                room->fillAG(all, source);
                int id = room->askForAG(source, all, false, objectName());
                room->clearAG(source);
                Card *lizhan_slash = Sanguosha->getCard(id);
                source->obtainCard(lizhan_slash, true);
                Slash *tmpslash = new Slash(Card::NoSuit, 0);
                if (source->isCardLimited(tmpslash, Card::MethodUse, true))
                    return false;
                if (!source->canSlash(current, tmpslash, false))
                    return false;
                QList<ServerPlayer *> victims;
                victims << current;
                room->askForUseSlashTo(source, victims, prompt1,
                    false, false, false);
            }
        } else if (triggerEvent == EventPhaseChanging){
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Discard)
                source->tag.remove("lizhan");
        }
        return false;
    }
};


kuangzaoCard::kuangzaoCard() {
    mute = true;
}
bool kuangzaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() == 0 && to_select->inMyAttackRange(Self) && to_select != Self;
}
void kuangzaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    QString prompt = "@kuangzao-slash:" + source->objectName();
    const Card *card = room->askForUseSlashTo(targets.first(), source, prompt);
    if (card == NULL)
        room->damage(DamageStruct("kuangzao", NULL, targets.first()));
}


class kuangzao : public ZeroCardViewAsSkill {
public:
    kuangzao() : ZeroCardViewAsSkill("kuangzao") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("kuangzaoCard");
    }

    virtual const Card *viewAs() const{
        return new kuangzaoCard;
    }
};

/*huanshiCard::huanshiCard() {
    mute = true;
}
bool huanshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    QString str = Self->property("huanshi").toString();
    QStringList huanshi_targets = str.split("+");
    return  huanshi_targets.contains(to_select->objectName());
}
void huanshiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    foreach (ServerPlayer *p, targets)
        p->addMark("huanshi_target");
}

class huanshivs: public ZeroCardViewAsSkill {
public:
    huanshivs(): ZeroCardViewAsSkill("huanshi") {
        response_pattern ="@@huanshi";
    }

    virtual const Card *viewAs() const{
        return new huanshiCard;
    }
};
*/
class huanshi: public TriggerSkill {
public:
    huanshi(): TriggerSkill("huanshi") {
        events  <<TargetConfirming;
        //view_as_skill= new huanshivs;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.from !=player && use.to.contains(player)){
            //QStringList huanshiTargets;
            QList<ServerPlayer *> listt;
            foreach ( ServerPlayer *p, room->getOtherPlayers(player)) {
                if ( use.from->canSlash(p,use.card,true)&& !use.to.contains(p) 
                    && use.from->inMyAttackRange(p) ) //need check attackrange
                    //huanshiTargets<< p->objectName(); 
                    listt<<p;
            }
                //if (huanshiTargets.isEmpty() )
            if (listt.isEmpty() )
                return false;
                
            //room->setPlayerProperty(player, "huanshi", huanshiTargets.join("+"));
            player->tag["huanshi_source"]=data;
            
            //room->askForUseCard(player, "@@huanshi", "@huanshi:"+use.from->objectName());
            ServerPlayer *target =   room->askForPlayerChosen(player,listt,objectName(),"@huanshi:"+use.from->objectName(),true,true);         

            // huanshi effect
            //foreach ( ServerPlayer *p, room->getOtherPlayers(player)) {
            //    if (p->getMark("huanshi_target")>0 )
            //        use.to<<p;
            // }
            //room->setPlayerProperty(player, "huanshi", QVariant());
            if (target){
                use.to<<target;
                room->sortByActionOrder(use.to);
                data = QVariant::fromValue(use);
                room->getThread()->trigger(TargetConfirming, room, target, data);
            }
            /*foreach ( ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getMark("huanshi_target")>0){
                    room->setPlayerMark(p, "huanshi_target", 0);
                    room->getThread()->trigger(TargetConfirming, room, p, data);
                }
            }*/
        }
        return false;
    }
};

class shishi : public TriggerSkill {
public:
    shishi() : TriggerSkill("shishi") {
        events << CardUsed << SlashEffected << CardEffected;
    }


    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == CardUsed){
            ServerPlayer *src = room->findPlayerBySkillName(objectName());
            if (!src || src->getPile("lishi").length() > 0)
                return false;
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.from == src)
                return false;
            if (use.card->isKindOf("Slash") || use.card->isNDTrick()){
                QString    prompt = "use:" + use.from->objectName() + ":" + use.card->objectName();
                src->tag["shishi_use"] = data;
                if (!room->askForSkillInvoke(src, objectName(), prompt))
                    return false;
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, src->objectName(), use.from->objectName());
            
                if (use.card && room->getCardPlace(use.card->getEffectiveId()) == Player::PlaceTable)
                    src->addToPile("lishi", use.card, true);
                //addtopile will clear cardflag(especially  use.from is robot ai )

                if (use.card->isKindOf("Nullification")){
                    room->touhouLogmessage("#weiya", use.from, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                    room->setPlayerFlag(use.from, "nullifiationNul");
                } else
                    room->setCardFlag(use.card, "shishiSkillNullify");
            }
        } else if (triggerEvent == SlashEffected){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("shishiSkillNullify")){
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.slash->objectName(), QList<ServerPlayer *>(), objectName());
                room->setEmotion(effect.to, "skill_nullify");
                return true;
            }
        } else if (triggerEvent == CardEffected){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->hasFlag("shishiSkillNullify")) {
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.card->objectName(), QList<ServerPlayer *>(), objectName());
                room->setEmotion(effect.to, "skill_nullify");
                return true;
            }
        }
        return false;
    }
};

class shouye : public TriggerSkill {
public:
    shouye() : TriggerSkill("shouye") {
        events << EventPhaseStart << Damaged;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        if ((triggerEvent == EventPhaseStart && player->getPhase() == Player::Start)
            || (triggerEvent == Damaged)){
            QList<int> pile = player->getPile("lishi");
            if (pile.length() > 0){
                ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@" + objectName(), true, true);
                DummyCard *dummy = new DummyCard(pile);
                if (target)
                    room->obtainCard(target, dummy);
            }
        }
        return false;
    }
};



buxianCard::buxianCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}
bool buxianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() < 2 && !to_select->isKongcheng() && to_select != Self;
}
bool buxianCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}
void buxianCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->moveCardTo(Sanguosha->getCard(subcards.first()), NULL, Player::DrawPile);
    targets.first()->pindian(targets.last(), "buxian");
}


class buxianvs : public OneCardViewAsSkill {
public:
    buxianvs() : OneCardViewAsSkill("buxian") {
        filter_pattern = ".|.|.|hand";
        response_pattern = "@@buxian";
    }


    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard){
            buxianCard *card = new buxianCard;
            card->addSubcard(originalCard);
            return card;
        }
        return NULL;
    }
};
class buxian : public TriggerSkill {
public:
    buxian() : TriggerSkill("buxian") {
        events << Damaged << EventPhaseStart;
        view_as_skill = new buxianvs;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && player->getPhase() != Player::Play)
            return false;
        if (!player->isKongcheng())
            room->askForUseCard(player, "@@buxian", "@buxian");
        return false;
    }
};
class buxian_effect : public TriggerSkill {
public:
    buxian_effect() : TriggerSkill("#buxian") {
        events << Pindian;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PindianStar pindian = data.value<PindianStar>();
        if (pindian->reason != "buxian")
            return false;
        ServerPlayer *bigger;
        if (pindian->from_number > pindian->to_number)
            bigger = pindian->from;
        else if (pindian->to_number > pindian->from_number)
            bigger = pindian->to;
        if (bigger == pindian->to || bigger == pindian->from) {
            bigger->drawCards(1);
            room->damage(DamageStruct("buxian", NULL, bigger));
        }
        return false;
    }
};

class xingyun : public TriggerSkill {
public:
    xingyun() : TriggerSkill("xingyun") {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (room->getTag("FirstRound").toBool())
            return false;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        if (move.to != NULL && move.to == player && move.to_place == Player::PlaceHand) {
            foreach(int id, move.card_ids) {
                if (Sanguosha->getCard(id)->getSuit() == Card::Heart &&
                    player->askForSkillInvoke(objectName(), data)){
                    QString choice = "letdraw";
                    room->showCard(player, id);
                    if (player->isWounded())
                        choice = room->askForChoice(player, objectName(), "letdraw+recover", data);
                    if (choice == "letdraw"){
                        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@xingyun-select");
                        target->drawCards(1);
                    } else if (choice == "recover"){
                        RecoverStruct recover;
                        recover.who = player;
                        room->recover(player, recover);
                    }
                }

            }
        }
        return false;
    }
};



geshengCard::geshengCard() 
 { 
     handling_method = Card::MethodNone; 
 } 
 
 
bool geshengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const 
{ 
    if (!targets.isEmpty()) return false; 
    if (to_select->getPhase() != Player :: Judge) return false;
    Indulgence *indl = new Indulgence(getSuit(), getNumber());
    indl->addSubcard(getEffectiveId());
    indl->setSkillName("gesheng");
    indl->deleteLater();

    bool canUse = !Self->isLocked(indl);
    if (canUse &&  to_select != Self
        && !to_select->containsTrick("indulgence") && !Self->isProhibited(to_select, indl))
        return true;
    return false;
} 
 
 
 const Card *geshengCard::validate(CardUseStruct &cardUse) const 
 { 
     ServerPlayer *to = cardUse.to.first(); 
     if (!to->containsTrick("indulgence")) { 
         Indulgence *indulgence = new Indulgence(getSuit(), getNumber()); 
         indulgence->addSubcard(getEffectiveId()); 
         indulgence->setSkillName("gesheng"); 
         return indulgence; 
     } 
     return this; 
 } 

 


class geshengvs : public OneCardViewAsSkill {
public:
    geshengvs() : OneCardViewAsSkill("gesheng") {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }
    
    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }
    
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        if (pattern == "@@gesheng")
            return true;
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard){
            //Indulgence *indl = new Indulgence(originalCard->getSuit(), originalCard->getNumber());
            geshengCard *indl = new geshengCard();
            indl->addSubcard(originalCard);
            indl->setSkillName(objectName());
            return indl;
        }
        return NULL;
    }
};
/*
class gesheng : public TriggerSkill {
public:
    gesheng() : TriggerSkill("gesheng") {
        events << EventPhaseChanging << PreCardUsed;// << TargetConfirmed;
        view_as_skill = new geshengvs;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == PreCardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == "gesheng")
                room->setPlayerFlag(use.from, "gesheng");
        }
        else if (triggerEvent == EventPhaseChanging){
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                room->setPlayerFlag(player, "-gesheng");
        }
        return false;
    }
};
*/
class gesheng : public TriggerSkill {
public:
    gesheng() : TriggerSkill("gesheng") {
        events << EventPhaseChanging <<  EventPhaseStart << EventPhaseEnd ;
        view_as_skill = new geshengvs;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        
        if (triggerEvent == EventPhaseStart && player->getPhase()== Player::Judge){
            ServerPlayer *src = room->findPlayerBySkillName(objectName());
            if (!src || src == player)
                return false;
            foreach (ServerPlayer *p, room->getAlivePlayers()){
                foreach (const Card *c, p->getCards("j")){
                    if (c->isKindOf("Indulgence") && c->getSuit() != Card::Diamond)
                        return false;
                }
            }
            if (room->askForSkillInvoke(src, objectName(), QVariant::fromValue(player))){
                room->setPlayerFlag(src, "gesheng");
                src->drawCards(2);
            }
        }
        else if (triggerEvent == EventPhaseEnd && player->getPhase()== Player::Judge){
            ServerPlayer *src = NULL;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)){
                if (p->hasFlag("gesheng")){
                    src = p;
                    break;
                }
            }
            if (!src)
                return false;
            bool canuse = true;    
            if ( player->isDead())
                canuse = false;
            Indulgence *indl = new Indulgence(Card::NoSuit, 0);
            indl->deleteLater();
            if (src->isCardLimited(indl, Card::MethodUse, true))
                canuse = false;
            if (player->containsTrick("indulgence") || src->isProhibited(player, indl))
                canuse = false;
            if (canuse){
                QString prompt = "@gesheng:"+ player->objectName();
                const Card *card = room->askForUseCard(src, "@@gesheng", prompt);
                if (!card)
                    room->loseHp(src, 2);
            }
            else
                room->loseHp(src, 2);
        }
        else if (triggerEvent == EventPhaseChanging){
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Judge){
                foreach (ServerPlayer *p, room->getAlivePlayers()){
                    if (p->hasFlag("gesheng"))
                        room->setPlayerFlag(p, "-gesheng");
                }
            }  
        }
        return false;
    }
};

class yemang : public ProhibitSkill {
public:
    yemang() : ProhibitSkill("yemang") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const{
        if (card->isKindOf("Slash")) {
            bool offensive_horse = false;
            foreach(int id, card->getSubcards()){
                if (from->getOffensiveHorse() && from->getOffensiveHorse()->getId() == id){
                    offensive_horse = true;
                    break;
                }
            }

            if (offensive_horse)
                return to->hasSkill(objectName()) && (from->distanceTo(to, 1) >= to->getHp());
            else
                return to->hasSkill(objectName()) && (from->distanceTo(to) >= to->getHp());
        }
        return false;
    }
};


class yinghuo : public TriggerSkill {
public:
    yinghuo() : TriggerSkill("yinghuo") {
        events << CardAsked;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QString pattern = data.toStringList().first();
        if (pattern == "jink") {
            Jink *jink = new Jink(Card::NoSuit, 0);
            //need check
            if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE){
                if (player->isCardLimited(jink, Card::MethodResponse))
                    return false;
            } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE){
                if (player->isCardLimited(jink, Card::MethodUse))
                    return false;
            }


            const Card *card = room->askForCard(player, "Jink", "@yinghuo", data, Card::MethodNone, NULL, false, objectName());
            if (card) {
                room->notifySkillInvoked(player, objectName());
                room->showCard(player, card->getId());
                room->getThread()->delay();

                jink->setSkillName("_yinghuo");
                room->provide(jink);
            }
        }
        return false;
    }
};

class chongqun : public TriggerSkill {
public:
    chongqun() : TriggerSkill("chongqun") {
        events << CardResponded << CardUsed;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        bool can = false;
        if (triggerEvent == CardResponded){
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isKindOf("BasicCard"))
                can = true;
        } else if (triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("BasicCard"))
                can = true;
        }
        if (can){
            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                if (!p->isNude() && p->canDiscard(p, "he"))
                    targets << p;
            }
            if (targets.isEmpty())
                return false;
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@chongqun_target", true, true);
            if (target)
                room->askForDiscard(target, "chongqun", 1, 1, false, true, "chongqun_discard:" + player->objectName());
        }
        return false;
    }
};



//for using dialog, viewas Skill should be the main skill.
class chuangshivs : public ZeroCardViewAsSkill {
public:
    chuangshivs() : ZeroCardViewAsSkill("chuangshi") {
        response_pattern = "@@chuangshi";
    }

    virtual const Card *viewAs() const{

        CardStar c = Self->tag.value("chuangshi").value<CardStar>();
        //we need get the real subcard.
        if (c) {
            chuangshiCard *card = new chuangshiCard;
            card->setUserString(c->objectName());
            return card;
        }
        else{
            return NULL;
        }
    }

    virtual QDialog *getDialog() const{
        return qijiDialog::getInstance("chuangshi");
    }

};

class chuangshi : public DrawCardsSkill {
public:
    chuangshi() : DrawCardsSkill("#chuangshi") {
        view_as_skill = new chuangshivs;
    }
    static bool use_chuangshi(Room *room, ServerPlayer *player){

        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), "chuangshi", "@chuangshi_target:" + QString::number(player->getMark("chuangshi") + 1), true, true);
        if (target != NULL){
            target->gainMark("chuangshi_user");//need use gainMark to notify the client player.

            room->setPlayerProperty(player, "chuangshi_user", target->objectName());
            const Card *card = room->askForUseCard(player, "@@chuangshi", "@chuangshi_prompt:" + target->objectName());
            return card != NULL;
        }
        return false;
    }
    static const Player *getChuangshiUser(const Player *player){
        foreach(const Player *p, player->getAliveSiblings()){
            if (p->getMark("chuangshi_user") > 0)
                return p;
            //if (p->objectName()==player->property("chuangshi_user").toString())
            //    return p;
        }
        return NULL;
    }

    static ServerPlayer *getChuangshiUser1(ServerPlayer *player){
        Room *room = player->getRoom();
        foreach(ServerPlayer *p, room->getOtherPlayers(player)){
            if (p->getMark("chuangshi_user") > 0)
                return p;
            //if p->objectName()==player->property("chuangshi_user").toString();
            //    return p;
        }
        return NULL;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        if (use_chuangshi(room, player) || player->getMark("chuangshi") > 0){ //  for ai, we need get mark
            n = n - 1;
            if (player->hasSkill("chuangshi") && (use_chuangshi(room, player) || player->getMark("chuangshi") > 1)){
                n = n - 1;
            }
        }
        return  n;
    }
};

chuangshiCard::chuangshiCard() {
    mute = true;
    will_throw = false;
}
bool chuangshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    const Player *user = chuangshi::getChuangshiUser(Self);
    const Card *card = Self->tag.value("chuangshi").value<const Card *>();
    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(card->objectName(), oc->getSuit(), oc->getNumber());
    new_card->addSubcard(oc);
    new_card->setSkillName("chuangshi");
    if (new_card->targetFixed())
        return false;
    if (new_card->isKindOf("FireAttack"))
        return new_card && (new_card->targetFilter(targets, to_select, user) || (to_select == user && !user->isKongcheng()))
        && !user->isProhibited(to_select, new_card, targets);
    else
        return new_card && new_card->targetFilter(targets, to_select, user) && !user->isProhibited(to_select, new_card, targets);
}

bool chuangshiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    const Player *user = chuangshi::getChuangshiUser(Self);
    const Card *card = Self->tag.value("chuangshi").value<const Card *>();
    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(card->objectName(), oc->getSuit(), oc->getNumber());
    new_card->addSubcard(oc);
    new_card->setSkillName("chuangshi");
    if (card->isKindOf("IronChain") && targets.length() == 0)
        return false;
    return new_card && new_card->targetsFeasible(targets, user);
}

void chuangshiCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *from = card_use.from;
    ServerPlayer *to1 = card_use.to.at(0);
    ServerPlayer *to2 = card_use.to.at(1);
    QList<ServerPlayer *>logto;
    logto << to1 << to2;

    Card *card = Sanguosha->cloneCard(user_string);
    if (card->isKindOf("Collateral")){
        ServerPlayer *chuangshi_user = chuangshi::getChuangshiUser1(from);
        room->setPlayerMark(chuangshi_user, "chuangshi_user", 0);
        from->addMark("chuangshi", 1);
        room->touhouLogmessage("#ChoosePlayerWithSkill", from, "chuangshi", logto, "");
        Card *use_card = Sanguosha->cloneCard(card->objectName());
        use_card->setSkillName("_chuangshi");
        CardUseStruct use;
        use.from = chuangshi_user;
        use.to = card_use.to;
        use.card = use_card;
        room->useCard(use);
    } else
        SkillCard::onUse(room, card_use);
}
void chuangshiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    //const Card *card = Self->tag.value("chuangshi").value<const Card *>();
    Card *card = Sanguosha->cloneCard(user_string);
    if (card->isKindOf("Collateral"))
        return;
    ServerPlayer *user = chuangshi::getChuangshiUser1(source);
    Card * use_card = Sanguosha->cloneCard(card->objectName());

    room->setPlayerMark(user, "chuangshi_user", 0);
    source->addMark("chuangshi", 1);
    CardUseStruct carduse;
    use_card->setSkillName("_chuangshi");
    carduse.card = use_card;
    carduse.from = user;
    carduse.to = targets;

    room->sortByActionOrder(carduse.to);
    room->useCard(carduse, true);

}

class chuangshi_effect : public TriggerSkill {
public:
    chuangshi_effect() : TriggerSkill("#chuangshi_effect") {
        events << EventPhaseEnd;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL && target->getMark("chuangshi") > 0);
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Draw){
            if (player->getMark("chuangshi") > 1)
                room->loseHp(player, 2);
            room->setPlayerMark(player, "chuangshi", 0);
        }
        return false;
    }
};

class wangyue : public MasochismSkill {
public:
    wangyue() : MasochismSkill("wangyue") {
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        Room *room = player->getRoom();
        if (damage.from && damage.from->isAlive()
            && damage.from->getHandcardNum() > player->getHp()){
            player->tag["wangyue_target"] = QVariant::fromValue(damage.from);
            QString prompt = "target:" + damage.from->objectName() + "::" + QString::number(player->getHp());
            if (room->askForSkillInvoke(player, objectName(), prompt)){
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());
            
                int x = damage.from->getHandcardNum() - player->getHp();
                room->askForDiscard(damage.from, "wangyue", x, x, false, false);
            }
        }
    }
};


huweiCard::huweiCard() {
    mute = true;
    target_fixed = true;
}
const Card *huweiCard::validate(CardUseStruct &cardUse) const{
    Room *room = cardUse.from->getRoom();
    room->touhouLogmessage("#InvokeSkill", cardUse.from, "huwei");
    room->notifySkillInvoked(cardUse.from, "huwei");
    cardUse.from->drawCards(2);
    room->setPlayerFlag(cardUse.from, "Global_huweiFailed");
    return NULL;
}


class huweivs : public ZeroCardViewAsSkill {
public:
    huweivs() : ZeroCardViewAsSkill("huwei") {
    }

    virtual const Card *viewAs() const{
        return new huweiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        Slash *tmpslash = new Slash(Card::NoSuit, 0);
        tmpslash->deleteLater();
        if (player->isCardLimited(tmpslash, Card::MethodUse))
            return false;
        return (!player->hasFlag("Global_huweiFailed") && pattern == "slash" && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE);
    }
};
class huwei : public TriggerSkill {
public:
    huwei() : TriggerSkill("huwei") {
        events << CardAsked << CardUsed << CardsMoveOneTime << BeforeCardsMove << CardResponded;
        view_as_skill = new huweivs;
    }
    static bool targetChoiceForHuwei(Room *room, ServerPlayer *player, QString skillname, int num) {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), skillname, "@huwei_targetdraw:" + QString::number(num), true, true);
        if (target){
            target->drawCards(2);
            return true;
        } else
            return false;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Play)
            return false;
        if (triggerEvent == CardAsked){
            QString pattern = data.toStringList().first();
            if (pattern == "slash") {
                Slash *tmpslash = new Slash(Card::NoSuit, 0);
                if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE){
                    if (player->isCardLimited(tmpslash, Card::MethodResponse))
                        return false;
                }

                if (room->askForSkillInvoke(player, objectName(), data))
                    player->drawCards(2);
            }
        } else if (triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash"))
                targetChoiceForHuwei(room, player, objectName(), 1);
        } else if (triggerEvent == CardResponded){
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isKindOf("Slash"))
                targetChoiceForHuwei(room, player, objectName(), 1);
        } else if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && move.to_place == Player::DiscardPile && move.from == player){
                if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {

                    foreach(int id, move.card_ids){
                        Card *card = Sanguosha->getCard(id);
                        if (card->isKindOf("Slash"))
                            player->addMark(objectName());
                    }
                }
            }
        } else if (triggerEvent == CardsMoveOneTime){
            int n = player->getMark(objectName());
            while (player->getMark(objectName()) > 0){
                player->removeMark(objectName());
                if (targetChoiceForHuwei(room, player, objectName(), n))
                    n = n - 1;
                else {
                    room->setPlayerMark(player, objectName(), 0);
                    break;
                }
            }
        }


        return false;
    }
};


jinxiCard::jinxiCard() {
    mute = true;
    target_fixed = true;
}
void jinxiCard::onUse(Room *room, const CardUseStruct &card_use) const{
    room->doLightbox("$jinxiAnimate", 4000);
    SkillCard::onUse(room, card_use);
}
void jinxiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->removePlayerMark(source, "@jinxi");
    RecoverStruct recov;
    recov.recover = source->getMaxHp() - source->getHp();
    recov.who = source;
    room->recover(source, recov);
    if (source->getHandcardNum() < 4)
        source->drawCards(4 - source->getHandcardNum());
}
class jinxi : public ZeroCardViewAsSkill {
public:
    jinxi() : ZeroCardViewAsSkill("jinxi") {
        frequency = Limited;
        limit_mark = "@jinxi";
    }

    virtual const Card *viewAs() const{
        return new jinxiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@jinxi") >= 1 && player->isWounded();
    }
};


th08Package::th08Package()
    : Package("th08")
{
    General *yyc001 = new General(this, "yyc001$", "yyc", 4, false);
    yyc001->addSkill(new yongheng);
    yyc001->addSkill(new zhuqu);


    General *yyc002 = new General(this, "yyc002", "yyc", 4, false);
    yyc002->addSkill(new ruizhi);
    yyc002->addSkill(new miyao);

    General *yyc003 = new General(this, "yyc003", "yyc", 4, false);
    yyc003->addSkill(new bumie);
    yyc003->addSkill(new bumie_maxhp);
    yyc003->addSkill(new lizhan);
    related_skills.insertMulti("bumie", "#bumie");


    General *yyc004 = new General(this, "yyc004", "yyc", 4, false);
    yyc004->addSkill(new kuangzao);
    yyc004->addSkill(new huanshi);

    General *yyc005 = new General(this, "yyc005", "yyc", 3, false);
    yyc005->addSkill(new shishi);
    yyc005->addSkill(new shouye);

    General *yyc006 = new General(this, "yyc006", "yyc", 3, false);
    yyc006->addSkill(new buxian);
    yyc006->addSkill(new buxian_effect);
    yyc006->addSkill(new xingyun);
    related_skills.insertMulti("buxian", "#buxian");

    General *yyc007 = new General(this, "yyc007", "yyc", 3, false);
    yyc007->addSkill(new gesheng);
    yyc007->addSkill(new yemang);

    General *yyc008 = new General(this, "yyc008", "yyc", 3, false);
    yyc008->addSkill(new yinghuo);
    yyc008->addSkill(new chongqun);

    General *yyc009 = new General(this, "yyc009", "yyc", 3, false);
    yyc009->addSkill(new chuangshivs);
    yyc009->addSkill(new chuangshi);
    yyc009->addSkill(new chuangshi_effect);
    yyc009->addSkill(new wangyue);
    related_skills.insertMulti("chuangshi", "#chuangshi_effect");
    related_skills.insertMulti("chuangshi", "#chuangshi");

    General *yyc010 = new General(this, "yyc010", "yyc", 4, false);
    yyc010->addSkill(new huwei);
    yyc010->addSkill(new jinxi);


    addMetaObject<miyaoCard>();
    addMetaObject<kuangzaoCard>();
    //addMetaObject<huanshiCard>();
    addMetaObject<buxianCard>();
    addMetaObject<geshengCard>();
    addMetaObject<chuangshiCard>();
    addMetaObject<huweiCard>();
    addMetaObject<jinxiCard>();
}

ADD_PACKAGE(th08)

