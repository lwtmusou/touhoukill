#include "protagonist.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h" 
#include "maneuvering.h"



/*lingqiCard::lingqiCard() {
    will_throw = true;
    handling_method = Card::MethodDiscard;
    mute = true;
    }
    void lingqiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    const Card *card = source->tag["lingcard"].value<const Card *>();
    if (card==NULL)
    return;
    //for UI
    if  (targets.length()>1 && source->getGeneralName()=="zhu001"){
    room->getThread()->delay(1000);
    room->doLightbox("$lingqiAnimate", 2000);
    }
    foreach(ServerPlayer *p, targets){
    JudgeStruct judge;
    judge.reason="lingqi";
    judge.who=p;
    judge.good=true;
    judge.pattern = ".|red";
    room->judge(judge);
    if (judge.isGood())
    room->setCardFlag(card, "lingqi"+p->objectName());
    }
    }

    bool lingqiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    QString str = Self->property("lingqi_targets").toString();
    QStringList lingqi_targets = str.split("+");
    return  lingqi_targets.contains(to_select->objectName());
    }

    class lingqivs: public OneCardViewAsSkill {
    public:
    lingqivs():OneCardViewAsSkill("lingqi") {
    filter_pattern = ".|.|.|.!";
    response_pattern ="@@lingqi";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
    lingqiCard *card = new lingqiCard;
    card->addSubcard(originalCard);
    return card;
    }
    };
    */

class lingqi : public TriggerSkill {
public:
    lingqi() : TriggerSkill("lingqi") {
        events << TargetConfirming << SlashEffected << CardEffected;
        //view_as_skill=new lingqivs;
    }

    //virtual bool triggerable(const ServerPlayer *target) const{
    //     return (target != NULL && target->isAlive());
    // }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            //if (player !=use.from )
            //    return false;
            //ServerPlayer *lingmeng = room->findPlayerBySkillName(objectName());
            //if  (lingmeng==NULL || lingmeng->isNude())
            //    return false;
            if (!use.card->isKindOf("Slash") && !use.card->isNDTrick())
                return false;
            //lingmeng->tag["lingqicarduse"] = data;
            //lingmeng->tag["lingcard"]= QVariant::fromValue(use.card);
            player->tag["lingqicarduse"] = data;
            //player->tag["lingcard"]= QVariant::fromValue(use.card);
            //QStringList lingqitargets;
            //foreach (ServerPlayer *p, use.to)
            //    lingqitargets<<p->objectName();

            //if (lingqitargets.isEmpty())
            //     return false;
            //room->setPlayerProperty(lingmeng, "lingqi_targets", lingqitargets.join("+"));
            //QString    prompt="@lingqi:"+use.from->objectName()+":"+use.card->objectName();
            QString    prompt = "target:" + use.from->objectName() + ":" + use.card->objectName();

            if (!player->askForSkillInvoke(objectName(), prompt))
                return false;
            JudgeStruct judge;
            judge.reason = "lingqi";
            judge.who = player;
            judge.good = true;
            judge.pattern = ".|heart";
            room->judge(judge);
            if (judge.isGood())
                room->setCardFlag(use.card, "lingqi" + player->objectName());
            //room->askForUseCard(lingmeng, "@@lingqi",prompt );
            //room->setPlayerProperty(lingmeng, "lingqi_targets", QVariant());        
        } else if (triggerEvent == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash != NULL && effect.slash->hasFlag("lingqi" + player->objectName())) {
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.slash->objectName(), QList<ServerPlayer *>(), objectName());
                room->setEmotion(effect.to, "skill_nullify");
                return true;
            }
        } else if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card != NULL && effect.card->isNDTrick() && effect.card->hasFlag("lingqi" + player->objectName())){
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.card->objectName(), QList<ServerPlayer *>(), objectName());
                room->setEmotion(effect.to, "skill_nullify");
                return true;
            }
        }

        return false;
    }
};

/*class hongbai: public FilterSkill{
public:
hongbai(): FilterSkill("hongbai"){

}

virtual bool viewFilter(const Card *to_select) const{
Room *room = Sanguosha->currentRoom();
if (room->getCardPlace(to_select->getEffectiveId()) == Player::PlaceJudge){
ServerPlayer *reimu = room->getCardOwner(to_select->getEffectiveId());
if (reimu != NULL && reimu->hasSkill(objectName())){
return (to_select->isBlack());
}
}
return false;
}

virtual const Card *viewAs(const Card *originalCard) const{
WrappedCard *wrap = Sanguosha->getWrappedCard(originalCard->getEffectiveId());
wrap->setSuit(Card::Heart);
wrap->setSkillName(objectName());
wrap->setModified(true);
return wrap;
}
};
*/

class bllmqixiang : public TriggerSkill {
public:
    bllmqixiang() : TriggerSkill("bllmqixiang") {
        events << FinishJudge;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (source){
            if (judge->who->getHandcardNum() >= source->getMaxHp())
                return false;
            source->tag["qixiang_judge"] = data;

            QString prompt = "target:" + judge->who->objectName() + ":" + judge->reason;
            if (room->askForSkillInvoke(source, objectName(), prompt))
                judge->who->drawCards(1);
        }
        return false;
    }
};



class boli : public TriggerSkill {
public:
    boli() : TriggerSkill("boli$") {
        events << AskForRetrial;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL && target->isAlive() && target->hasLordSkill(objectName()));
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        if (judge->card->getSuit() != Card::Heart){
            player->tag["boli_judge"] = data;
            QString prompt = "judge:" + judge->who->objectName() + ":" + judge->reason;
            if (!room->askForSkillInvoke(player, "boli", prompt))
                return false;
            if (player->getGeneralName() == "zhu001" && !player->hasFlag("boliAnimate")){
                room->doLightbox("$boliAnimate", 2000);
                room->setPlayerFlag(player, "boliAnimate");
            }
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                QStringList prompts;
                prompts << "@boli-retrial" << judge->who->objectName() << player->objectName() << judge->reason;

                const Card *heartcard = room->askForCard(p, ".H", prompts.join(":"), data, Card::MethodResponse, judge->who, true, objectName());
                if (heartcard != NULL){
                    room->retrial(heartcard, p, judge, objectName(), false);
                    return true;
                }
            }
        }
        return false;
    }
};



mofaCard::mofaCard() {
    will_throw = true;
    target_fixed = true;
    handling_method = Card::MethodDiscard;
    mute = true;
}
void mofaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    if (source->getGeneralName() == "zhu002")
        room->doLightbox("$mofaAnimate", 2000);
    Card *card = Sanguosha->getCard(subcards.first());
    if (card->getSuit() == Card::Spade)
        source->drawCards(1);
    room->touhouLogmessage("#mofa_notice", source, "mofa");
    room->setPlayerFlag(source, "mofa_invoked");
}


class mofavs : public OneCardViewAsSkill {
public:
    mofavs() :OneCardViewAsSkill("mofa") {
        filter_pattern = ".|.|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("mofaCard");
        //not player:hasFlag("mofa_invoked")
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        mofaCard *card = new mofaCard;
        card->addSubcard(originalCard);

        return card;
    }
};

/*class mofavs: public ViewAsSkill {
public:
mofavs(): ViewAsSkill("mofa") {

}

virtual bool isEnabledAtPlay(const Player *player) const{
return !player->isKongcheng() && !player->hasUsed("mofaCard") && !player->hasUsed("mofaFullCard");
}

virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
if (selected.length() >= 1)
return false;

if (to_select->isEquipped())
return false;

if (Self->isJilei(to_select))
return false;

return true;
}

virtual const Card *viewAs(const QList<const Card *> &cards) const{
if (cards.length()  == 0)
return new mofaFullCard;
if (cards.length() != 1)
return NULL;

mofaCard *card = new mofaCard;
card->addSubcards(cards);

return card;
}
}; */
class mofa : public TriggerSkill {
public:
    mofa() : TriggerSkill("mofa") {
        events << CardUsed << ConfirmDamage;
        view_as_skill = new mofavs;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == CardUsed) {
            if (player && player->hasFlag("mofa_invoked")){
                CardUseStruct use = data.value<CardUseStruct>();
                room->setCardFlag(use.card, "mofa_card");
            }
        } else if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->hasFlag("mofa_card")) {
                if (damage.from ){
                    room->touhouLogmessage("#TouhouBuff", damage.from, objectName());
                    QList<ServerPlayer *> logto;
                    logto << damage.to;
                    room->touhouLogmessage("#mofa_damage", damage.from, QString::number(damage.damage + 1), logto, QString::number(damage.damage));
                }
                damage.damage = damage.damage + 1;
                data = QVariant::fromValue(damage);
            }
        }
        return false;
    }
};


/*class heibai : public FilterSkill{
public:
    heibai() : FilterSkill("heibai"){

    }

    virtual bool viewFilter(const Card *to_select) const{
        Room *room = Sanguosha->currentRoom();
        if (room->getCardPlace(to_select->getEffectiveId()) == Player::PlaceHand) {
            ServerPlayer *marisa = room->getCardOwner(to_select->getEffectiveId());
            if (marisa && marisa->hasSkill(objectName())){
                return (to_select->isRed() && !to_select->isKindOf("Slash"));
            }
        }
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        WrappedCard *wrap = Sanguosha->getWrappedCard(originalCard->getEffectiveId());
        wrap->setSuit(Card::Spade);
        wrap->setSkillName(objectName());
        wrap->setModified(true);
        return wrap;
    }
};*/


wuyuCard::wuyuCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "wuyuvs";
    mute = true;
}

void wuyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *marisa = targets.first();
    if (marisa->hasLordSkill("wuyu")) {
        if (marisa->getGeneralName() == "zhu002")
            room->doLightbox("$wuyuAnimate", 2000);
        room->setPlayerFlag(marisa, "wuyuInvoked");

        room->notifySkillInvoked(marisa, "wuyu");
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), marisa->objectName(), "wuyu", QString());
        room->obtainCard(marisa, this, reason);
        QList<ServerPlayer *> marisas;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach (ServerPlayer *p, players) {
            if (p->hasLordSkill("wuyu") && !p->hasFlag("wuyuInvoked"))
                marisas << p;
        }
        if (marisas.isEmpty())
            room->setPlayerFlag(source, "Forbidwuyu");
    }
}

bool wuyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasLordSkill("wuyu")
        && to_select != Self && !to_select->hasFlag("wuyuInvoked");
}

class wuyuvs : public OneCardViewAsSkill {
public:
    wuyuvs() :OneCardViewAsSkill("wuyuvs") {
        attached_lord_skill = true;
        filter_pattern = ".|spade|.|hand";

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return  !player->hasFlag("Forbidwuyu");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        wuyuCard *card = new wuyuCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class wuyu : public TriggerSkill {
public:
    wuyu() : TriggerSkill("wuyu$") {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "wuyu")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty()) return false;

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("wuyuvs"))
                    room->attachSkillToPlayer(p, "wuyuvs");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "wuyu") {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.length() > 2) return false;

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("wuyuvs"))
                    room->detachSkillFromPlayer(p, "wuyuvs", true);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                return false;
            if (player->hasFlag("Forbidwuyu"))
                room->setPlayerFlag(player, "-Forbidwuyu");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("wuyuInvoked"))
                    room->setPlayerFlag(p, "-wuyuInvoked");
            }
        }
        return false;
    }
};




saiqianCard::saiqianCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "saiqianvs";
}
bool saiqianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasSkill("saiqian")
        && to_select != Self && !to_select->hasFlag("saiqianInvoked");
}
void saiqianCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *reimu = targets.first();
    if (reimu->hasSkill("saiqian")) {
        if (reimu->getGeneralName() == "zhu003")
            room->doLightbox("$saiqianAnimate", 2000);

        reimu->tag["saiqian_source"] = QVariant::fromValue(source);

        room->setPlayerFlag(reimu, "saiqianInvoked");
        room->notifySkillInvoked(reimu, "saiqian");
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), reimu->objectName(), "saiqian", QString());
        room->obtainCard(reimu, this, reason, false);
        QList<ServerPlayer *> reimus;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach (ServerPlayer *p, players) {
            if (p->hasSkill("saiqian") && !p->hasFlag("saiqianInvoked"))
                reimus << p;
        }
        if (reimus.isEmpty())
            room->setPlayerFlag(source, "Forbidsaiqian");


        //start effect
        QStringList saiqian_str;
        saiqian_str << "losehp_saiqian" << "discard_saiqian" << "cancel_saiqian";
        RecoverStruct recov;
        recov.who = reimu;
        recov.reason = "saiqian";
        while (!saiqian_str.isEmpty() && reimu->isAlive()) {
            QString choice = room->askForChoice(reimu, "saiqian", saiqian_str.join("+"));
            if (choice == "cancel_saiqian")
                return;
            else if (choice == "losehp_saiqian") {
                room->touhouLogmessage("#saiqian_lose", reimu, "saiqian");
                room->loseHp(reimu);
                room->recover(source, recov);
            } else if (choice == "discard_saiqian") {
                const Card *heartcard = room->askForCard(reimu, ".H", "@saiqian-discard:" + source->objectName(), QVariant::fromValue(source), Card::MethodDiscard, NULL, true, "saiqian");
                if (heartcard != NULL)
                    room->recover(source, recov);
            }
            saiqian_str.removeOne(choice);
        }
    }
}
class saiqianvs : public ViewAsSkill {
public:
    saiqianvs() : ViewAsSkill("saiqianvs") {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasFlag("Forbidsaiqian");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return !to_select->isEquipped() && selected.length() < 3;//&& !Self->isJilei(to_select)
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() > 0 && cards.length() <= 3) {
            saiqianCard *card = new saiqianCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }
};
class saiqian : public TriggerSkill {
public:
    saiqian() : TriggerSkill("saiqian") {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == GameStart
            || (triggerEvent == EventAcquireSkill && data.toString() == "wuyu")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty()) return false;

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("saiqianvs"))
                    room->attachSkillToPlayer(p, "saiqianvs");
            }
        }
        else if (triggerEvent == Death || (triggerEvent == EventLoseSkill && data.toString() == "wuyu")) {
            if (triggerEvent == Death){
                DeathStruct death = data.value<DeathStruct>();
                if (!death.who->hasSkill(objectName(),false,true))//deal the case that death in round of changshi?
                    return false;
            }
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill(objectName()))
                    lords << p;
            }
            if (lords.length() > 2) return false;

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("saiqianvs"))
                    room->detachSkillFromPlayer(p, "saiqianvs", true);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                return false;
            if (player->hasFlag("Forbidsaiqian"))
                room->setPlayerFlag(player, "-Forbidsaiqian");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("saiqianInvoked"))
                    room->setPlayerFlag(p, "-saiqianInvoked");
            }
        }
        return false;
    }
};



jiezouCard::jiezouCard() {
    will_throw = true;
    handling_method = Card::MethodNone;
    m_skillName = "jiezou";
    mute = true;
}
void jiezouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    if (source->getGeneralName() == "zhu004" && !source->hasFlag("jiezouAnimate")) {
        room->doLightbox("$jiezouAnimate", 2000);
        room->setPlayerFlag(source, "jiezouAnimate");
    }
    ServerPlayer *target = targets.first();
    int id = room->askForCardChosen(source, target, "hej", objectName());
    room->obtainCard(source, id, room->getCardPlace(id) != Player::PlaceHand);
    const Card *spade = room->askForCard(source, ".|spade", "@jiezou_spadecard", QVariant(), Card::MethodDiscard, NULL, false, "jiezou", false);
    if (spade == NULL) {
        room->loseHp(source);
        room->setPlayerFlag(source, "Global_PlayPhaseTerminated");
        room->touhouLogmessage("#jiezou_skip", source, "jiezou_skip", QList<ServerPlayer *>(), "jiezou");
    }
}
bool jiezouCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && !to_select->isAllNude();
}

class jiezou : public ZeroCardViewAsSkill {
public:
    jiezou() : ZeroCardViewAsSkill("jiezou") {
    }

    virtual const Card *viewAs() const{
        return new jiezouCard;
    }
};


shoucangCard::shoucangCard() {
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodUse;
    m_skillName = "shoucang";
    mute = true;
}
void shoucangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    if (source->getGeneralName() == "zhu004")
        room->doLightbox("$shoucangAnimate", 2000);
    foreach (int id, subcards)
        room->showCard(source, id);
    room->touhouLogmessage("#shoucang_max", source, "shoucang", QList<ServerPlayer *>(), QString::number(subcards.length()));
    room->setPlayerMark(source, "shoucang", subcards.length());

}
class shoucangvs : public ViewAsSkill {
public:
    shoucangvs() : ViewAsSkill("shoucang") {
        response_pattern = "@@shoucang";
    }


    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (to_select->isEquipped())
            return false;
        if (selected.isEmpty())
            return  !to_select->isEquipped();
        else if (selected.length() < 4){
            foreach (const Card *c, selected){
                if (to_select->getSuit() == c->getSuit())
                    return false;
            }
            return true;
        } else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() > 0 && cards.length() < 5) {
            shoucangCard *card = new shoucangCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;

    }
};
class shoucang : public TriggerSkill {
public:
    shoucang() : TriggerSkill("shoucang") {
        events << EventPhaseStart << EventPhaseChanging;
        view_as_skill = new shoucangvs;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Discard && !player->isKongcheng())
                room->askForUseCard(player, "@@shoucang", "@shoucang");
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(player, "shoucang", 0);
        }
        return false;
    }
};
class shoucangKeep : public MaxCardsSkill {
public:
    shoucangKeep() : MaxCardsSkill("#shoucang") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName()))
            return target->getMark("shoucang");
        else
            return 0;
    }
};


baoyiCard::baoyiCard() {
    will_throw = true;
    target_fixed = true;
    handling_method = Card::MethodDiscard;
    mute = true;
}
void baoyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    if (source->getGeneralName() == "zhu005")
        room->doLightbox("$baoyiAnimate", 2000);
    int num = subcards.length();
    foreach (int id, subcards) {
        Card *c = Sanguosha->getCard(id);
        if (c->getSuit() == Card::Spade)
            source->setFlags("baoyi");
    }

    while (num > 0) {
        QList<ServerPlayer *> listt;
        Slash *slash = new Slash(Card::NoSuit, 0);
        foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
            if (source->canSlash(p, slash, false))
                listt << p;

        }
        if (!listt.isEmpty()) {
            ServerPlayer * target = room->askForPlayerChosen(source, listt, "baoyi", "@@baoyi_chosen:" + QString::number(num), true, true);
            if (target == NULL)
                break;
            CardUseStruct carduse;
            slash->setSkillName("baoyi");
            carduse.card = slash;
            carduse.from = source;
            carduse.to << target;
            room->useCard(carduse);
        }
        num = num - 1;
    }
}
class baoyivs : public ViewAsSkill {
public:
    baoyivs() : ViewAsSkill("baoyi") {
        response_pattern = "@@baoyi";
    }


    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return  to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        bool has_delaytrick = false;
        baoyiCard *card = new baoyiCard;
        foreach (int id, Self->getJudgingAreaID()) {
            card->addSubcard(id);
            has_delaytrick = true;
        }
        if (cards.length() == 0 && !has_delaytrick) {
            return NULL;
        }
        if (cards.length() > 0) {
            card->addSubcards(cards);
        }
        return card;
    }
};
class baoyi : public TriggerSkill {
public:
    baoyi() : TriggerSkill("baoyi") {
        events << EventPhaseStart << EventPhaseEnd;
        view_as_skill = new baoyivs;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL) && (target->hasFlag(objectName()) || target->hasSkill(objectName()));
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::Start)
            return false;

        if (triggerEvent == EventPhaseStart) {
            if (player->isAllNude())
                return false;
            room->askForUseCard(player, "@@baoyi", "@baoyi");
        } else if (triggerEvent == EventPhaseEnd) {
            if (player->hasFlag("baoyi")){
                player->setFlags("-baoyi");
                if (room->askForSkillInvoke(player, "baoyi", "drawcard")) {
                    room->touhouLogmessage("#TouhouBuff", player, "baoyi");
                    player->drawCards(2);
                }
            }
        }
        return false;
    }
};


class zhize : public PhaseChangeSkill {
public:
    zhize() :PhaseChangeSkill("zhize") {
        frequency = Compulsory;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if (player->getPhase() == Player::Draw) {

            Room *room = player->getRoom();
            if (player->getGeneralName() == "zhu006")
                room->doLightbox("$zhizeAnimate", 2000);

            QList<ServerPlayer *> players;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!p->isKongcheng())
                    players << p;
            }
            if (players.isEmpty()){
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
                room->loseHp(player);
                player->skip(Player::Play);
                return false;
            }
            ServerPlayer *target = room->askForPlayerChosen(player, players, objectName(), "@@zhize", true, true);
            if (target) {
                //case1 ask for ag  
                /*room->fillAG(target->handCards(),player);
                int id=room->askForAG(player,target->handCards(),false,objectName());
                room->clearAG(player);
                */
                //case2 gongxin  
                /*QList<int>  ids;
                foreach (const Card *c, target->getCards("h"))
                ids<< c->getEffectiveId();

                int id = room->doGongxin(player, target, ids, "zhize");
                */
                //case3 cardchosen
                int id = room->askForCardChosen(player, target, "h", objectName(), true);
                if (id > -1)
                    room->obtainCard(player, id, false);

                return true;
            } else {
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
                room->loseHp(player);
                player->skip(Player::Play);
            }

        }

        return false;
    }
};

class chunxi : public TriggerSkill {
public:
    chunxi() : TriggerSkill("chunxi") {
        events << CardsMoveOneTime;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to != NULL && move.to == player && move.to_place == Player::PlaceHand){
            if (room->getTag("FirstRound").toBool())
                return false;

            foreach (int id, move.card_ids) {
                if (Sanguosha->getCard(id)->getSuit() == Card::Heart) {
                    QList<ServerPlayer *> targets;
                    foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                        if (!p->isKongcheng())
                            targets << p;
                    }
                    if (targets.isEmpty())
                        return false;
                    ServerPlayer *s = room->askForPlayerChosen(player, targets, objectName(), "@@chunxi", true, true);
                    if (s == NULL)
                        break;
                    if (player->getPhase() != Player::Draw &&
                        player->getGeneralName() == "zhu006" && !player->hasFlag("chunxiAnimate")) {
                        room->doLightbox("$chunxiAnimate", 2000);
                        room->setPlayerFlag(player, "chunxiAnimate");
                    }
                    room->showCard(player, id);
                    room->getThread()->delay();
                    int id = room->askForCardChosen(player, s, "h", objectName());
                    room->obtainCard(player, id, false);
                }
            }
        }
        return false;
    }
};


bllmwuyuCard::bllmwuyuCard() {
    mute = true;
    target_fixed = true;
}
void bllmwuyuCard::use(Room *room, ServerPlayer *bllm, QList<ServerPlayer *> &targets) const{
    QStringList uselist;
    if (Analeptic::IsAvailable(bllm))
        uselist << "bllmshiyu";

    uselist << "bllmseyu";
    uselist << "dismiss";
    QString choice = room->askForChoice(bllm, "bllmwuyu", uselist.join("+"));
    if (choice == "dismiss")
        return;
    else if (choice == "bllmshiyu")
        room->useCard(CardUseStruct(new bllmshiyuCard, bllm, QList<ServerPlayer *>()));
    else if (choice == "bllmseyu")
        room->useCard(CardUseStruct(new bllmseyuCard, bllm, NULL));
}


class bllmwuyuvs : public ViewAsSkill {
public:
    bllmwuyuvs() : ViewAsSkill("bllmwuyu") {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (player->getMark("@yu") == 0 && player->isKongcheng())
            return false;
        if (Analeptic::IsAvailable(player))
            return true;
        return true;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (pattern.contains("@@bllmwuyu"))
            return true;
        Analeptic *card = new Analeptic(Card::NoSuit, 0);
        card->deleteLater();
        if (player->isCardLimited(card, Card::MethodUse))
            return false;
        if (pattern.contains("analeptic"))
            return true;
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@bllmwuyu")
            return selected.isEmpty() && !to_select->isEquipped();
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@bllmwuyu"){
            if (cards.length() != 1)
                return NULL;
            bllmshiyudummy *shiyu = new bllmshiyudummy;
            shiyu->addSubcards(cards);
            return shiyu;
        }
        else if (pattern.contains("analeptic"))
            return new bllmshiyuCard;
        else
            return new bllmwuyuCard;
    }
};

class bllmwuyu : public PhaseChangeSkill {
public:
    bllmwuyu() :PhaseChangeSkill("bllmwuyu") {
        view_as_skill = new bllmwuyuvs;
    }
    static bool bllmwuyucost(Room *room, ServerPlayer *bllm, QString prompt) {
        bllm->tag["wuyu_prompt"] = QVariant::fromValue(prompt);
        if (bllm->getMark("@yu") > 0 && bllm->askForSkillInvoke("bllmwuyu", "useyu:" + prompt)) {
            bllm->loseMark("@yu");
            room->touhouLogmessage("#InvokeSkill", bllm, prompt);
            room->notifySkillInvoked(bllm, prompt);
            return true;
        } else {
            const Card *card = room->askForCard(bllm, "..H", "@bllm-discard:" + prompt, QVariant(), "bllmwuyu");
            if (card != NULL) {
                room->touhouLogmessage("#InvokeSkill", bllm, prompt);
                room->notifySkillInvoked(bllm, prompt);
                return true;
            }
        }
        return false;


    }


    virtual bool onPhaseChange(ServerPlayer *player) const{
        if (player->getPhase() == Player::Start){
            int n = player->getMark("@yu");
            int z = (player->getLostHp() + 1) - n;
            if (z > 0) {
                QString prompt = "bllmwuyu";
                player->tag["wuyu_prompt"] = QVariant::fromValue(prompt);
                if (player->askForSkillInvoke(objectName())) {
                    if (player->getGeneralName() == "zhu007")
                        player->getRoom()->doLightbox("$bllmwuyuAnimate", 2000);
                    player->gainMark("@yu", z);
                }
            }
        }
        return false;
    }
};


class bllmcaiyu : public DrawCardsSkill {
public:
    bllmcaiyu() : DrawCardsSkill("#bllmcaiyu") {
    }

    virtual int getDrawNum(ServerPlayer *bllm, int n) const{
        Room *room = bllm->getRoom();
        if (bllmwuyu::bllmwuyucost(room, bllm, "bllmcaiyu"))
            return n + 1;
        else
            return n;
    }
};

bllmseyuCard::bllmseyuCard() {
    //mute = true;
    will_throw = false;
    target_fixed = true;
}
void bllmseyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    room->setPlayerMark(source, "bllmseyu", source->getMark("bllmseyu") + 1);
}
const Card *bllmseyuCard::validate(CardUseStruct &cardUse) const{
    ServerPlayer *bllm = cardUse.from;
    Room *room = bllm->getRoom();
    if (bllmwuyu::bllmwuyucost(room, bllm, "bllmseyu"))
        return cardUse.card;
    return NULL;
}

class bllmseyu : public TargetModSkill {
public:
    bllmseyu() : TargetModSkill("#bllmseyu") {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *card) const{
        return from->getMark("bllmseyu");
    }
};
class bllmseyu_clear : public TriggerSkill {
public:
    bllmseyu_clear() : TriggerSkill("#bllmseyu_clear") {
        events << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL && target->getMark("bllmseyu") > 0);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getMark("bllmseyu") > 0)
            room->setPlayerMark(player, "bllmseyu", 0);

        return false;
    }
};

class bllmmingyu : public TriggerSkill {
public:
    bllmmingyu() : TriggerSkill("#bllmmingyu") {
        events << EventPhaseChanging;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *bllm, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge) {
            if (bllm->getCards("j").length() == 0) return false;
            if (bllmwuyu::bllmwuyucost(room, bllm, "bllmmingyu"))
                bllm->skip(Player::Judge);
        }
        return false;
    }
};


bllmshiyudummy::bllmshiyudummy() {
    //mute = true;
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void bllmshiyudummy::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{

}

bllmshiyuCard::bllmshiyuCard() {
    //mute = true;
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodUse;
}
const Card *bllmshiyuCard::validate(CardUseStruct &cardUse) const{
    ServerPlayer *bllm = cardUse.from;
    Room *room = bllm->getRoom();
    if (bllmwuyu::bllmwuyucost(room, bllm, "bllmshiyu")){
        room->setPlayerFlag(bllm, "Global_expandpileFailed");
        const Card *dummy = room->askForCard(bllm, "@@bllmwuyu", "@bllmshiyu-basics", QVariant(), Card::MethodNone);
        room->setPlayerFlag(bllm, "-Global_expandpileFailed");
        if (dummy) {
            Analeptic *ana = new Analeptic(Card::SuitToBeDecided, -1);
            foreach(int id, dummy->getSubcards())
                ana->addSubcard(id);
            ana->setSkillName("bllmshiyu");
            return ana;
        }
    }
    return NULL;
}
const Card *bllmshiyuCard::validateInResponse(ServerPlayer *bllm) const{
    Room *room = bllm->getRoom();
    if (bllmwuyu::bllmwuyucost(room, bllm, "bllmshiyu")){
        room->setPlayerFlag(bllm, "Global_expandpileFailed");
        const Card *dummy = room->askForCard(bllm, "@@bllmwuyu", "@bllmshiyu-basics", QVariant(), Card::MethodNone);
        room->setPlayerFlag(bllm, "-Global_expandpileFailed");
        if (dummy){
            Analeptic *ana = new Analeptic(Card::SuitToBeDecided, -1);
            foreach (int id, dummy->getSubcards())
                ana->addSubcard(id);
            ana->setSkillName("bllmshiyu");
            return ana;
        }
    }
    return NULL;
}


class bllmshuiyu : public PhaseChangeSkill {
public:
    bllmshuiyu() :PhaseChangeSkill("#bllmshuiyu") {

    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        if (player->getPhase() == Player::Discard) {
            if (bllmwuyu::bllmwuyucost(room, player, "bllmshuiyu"))
                room->setPlayerFlag(player, "bllmshuiyu");
        }
        return false;
    }
};
class bllmshuiyumc : public MaxCardsSkill {
public:
    bllmshuiyumc() : MaxCardsSkill("#bllmshuiyu2") {
    }

    virtual int getFixed(const Player *target) const{
        if (target->hasFlag("bllmshuiyu"))
            return 4;
        return -1;
    }
};


class qiangyu : public TriggerSkill {
public:
    qiangyu() : TriggerSkill("qiangyu") {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to && move.to == player  && move.to_place == Player::PlaceHand && player->hasFlag("qiangyu")){
            room->setPlayerFlag(player, "-qiangyu");
            const Card *card = room->askForCard(player, ".S", "qiangyu_spadecard", data, Card::MethodDiscard, NULL, false, objectName(), false);
            if (!card)
                room->askForDiscard(player, objectName(), 2, 2, false, false);
        }
        return false;
    }
};




class mokai : public TriggerSkill {
public:
    mokai() : TriggerSkill("mokai") {
        events << CardUsed;
    }
    static void tianyi_suit(Room *room, ServerPlayer *player, Card *equip, bool addOrLose) {

        if (addOrLose) {
            if (equip->getSuit() == Card::Spade)
                room->setPlayerMark(player, "tianyi_spade", player->getMark("tianyi_spade") + 1);
            else if (equip->getSuit() == Card::Heart)
                room->setPlayerMark(player, "tianyi_heart", player->getMark("tianyi_heart") + 1);
            else if (equip->getSuit() == Card::Club)
                room->setPlayerMark(player, "tianyi_club", player->getMark("tianyi_club") + 1);
            else if (equip->getSuit() == Card::Diamond)
                room->setPlayerMark(player, "tianyi_diamond", player->getMark("tianyi_diamond") + 1);
        } else {
            if (equip->getSuit() == Card::Spade)
                room->setPlayerMark(player, "tianyi_spade", player->getMark("tianyi_spade") - 1);
            else if (equip->getSuit() == Card::Heart)
                room->setPlayerMark(player, "tianyi_heart", player->getMark("tianyi_heart") - 1);
            else if (equip->getSuit() == Card::Club)
                room->setPlayerMark(player, "tianyi_club", player->getMark("tianyi_club") - 1);
            else if (equip->getSuit() == Card::Diamond)
                room->setPlayerMark(player, "tianyi_diamond", player->getMark("tianyi_diamond") - 1);
        }
    }

    static bool tianyi_compare(const Player *player, const Card *card){
        bool result = false;
        if (card->getSuit() == Card::Spade && player->getMark("tianyi_spade") > 0)
            result = true;
        else if (card->getSuit() == Card::Heart && player->getMark("tianyi_heart") > 0)
            result = true;
        else if (card->getSuit() == Card::Club && player->getMark("tianyi_club") > 0)
            result = true;
        else if (card->getSuit() == Card::Diamond && player->getMark("tianyi_diamond") > 0)
            result = true;

        return result;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("TrickCard")){
            QList<int> equips;
            QList<int> disable;

            foreach(const Card *equip, player->getEquips()){
                if (equip->isKindOf("Weapon") && player->getMark("@tianyi_Weapon") == 0)
                    equips << equip->getId();
                else if (equip->isKindOf("Armor") && player->getMark("@tianyi_Armor") == 0)
                    equips << equip->getId();
                else if (equip->isKindOf("DefensiveHorse") && player->getMark("@tianyi_DefensiveHorse") == 0)
                    equips << equip->getId();
                else if (equip->isKindOf("OffensiveHorse") && player->getMark("@tianyi_OffensiveHorse") == 0)
                    equips << equip->getId();
                else if (equip->isKindOf("Treasure") && player->getMark("@tianyi_Treasure") == 0)
                    equips << equip->getId();
                else
                    disable << equip->getId();
            }

            if (equips.length() == 0)
                return false;
            if (!room->askForSkillInvoke(player, objectName(), data))
                return false;
            int card_id = -1;
            card_id = room->askForCardChosen(player, player, "e", "mokai",
                false, Card::MethodNone, disable);

            if (card_id <= -1)
                return false;
            if (player->getGeneralName() == "zhu008")
                room->doLightbox("$mokaiAnimate", 2000);
            if (!player->hasSkill("#touhou_tianyi"))
                room->handleAcquireDetachSkills(player, "#touhou_tianyi");


            //room->touhouLogmessage("#InvokeSkill",player,"mokai");
            Card *tianyi_card = Sanguosha->getCard(card_id);
            if (tianyi_card->isKindOf("Weapon"))
                player->gainMark("@tianyi_Weapon", 1);
            else if (tianyi_card->isKindOf("Armor")){
                player->gainMark("@tianyi_Armor", 1);
                room->setPlayerMark(player, "Armor_Nullified", 1);
            } else if (tianyi_card->isKindOf("DefensiveHorse"))
                player->gainMark("@tianyi_DefensiveHorse", 1);
            else if (tianyi_card->isKindOf("OffensiveHorse"))
                player->gainMark("@tianyi_OffensiveHorse", 1);
            else if (tianyi_card->isKindOf("Treasure"))
                player->gainMark("@tianyi_Treasure", 1);
            tianyi_suit(room, player, tianyi_card, true);

            Sanguosha->playAudioEffect("audio/card/common/armor.ogg");
            room->touhouLogmessage("#tianyi_set", player, tianyi_card->objectName());
        }
        return false;
    }
};

class touhou_tianyi : public TriggerSkill {
public:
    touhou_tianyi() : TriggerSkill("#touhou_tianyi") {
        events << CardsMoveOneTime << BeforeCardsMove << SlashEffected;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == BeforeCardsMove){
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && move.from == player && move.from_places.contains(Player::PlaceEquip)){
                foreach(int id, move.card_ids){
                    if (room->getCardPlace(id) == Player::PlaceEquip){
                        Card *equip = Sanguosha->getCard(id);
                        if (equip->isKindOf("Weapon") && player->getMark("@tianyi_Weapon") > 0)
                            room->setCardFlag(equip, "tianyi_Weapon");
                        else if (equip->isKindOf("Armor") && player->getMark("@tianyi_Armor") > 0)
                            room->setCardFlag(equip, "tianyi_Armor");
                        else if (equip->isKindOf("DefensiveHorse") && player->getMark("@tianyi_DefensiveHorse") > 0)
                            room->setCardFlag(equip, "tianyi_DefensiveHorse");

                        else if (equip->isKindOf("OffensiveHorse") && player->getMark("@tianyi_OffensiveHorse") > 0)
                            room->setCardFlag(equip, "tianyi_OffensiveHorse");
                        else if (equip->isKindOf("Treasure") && player->getMark("@tianyi_Treasure") > 0)
                            room->setCardFlag(equip, "tianyi_Treasure");
                    }
                }
            }
        } else if (triggerEvent == CardsMoveOneTime){
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && move.from == player && move.from_places.contains(Player::PlaceEquip) && move.to_place != Player::PlaceEquip){
                bool tianyi_draw = false;
                foreach(int id, move.card_ids){
                    Card *card = Sanguosha->getCard(id);

                    if (card->hasFlag("tianyi_Weapon")){
                        room->setCardFlag(card, "-tianyi_Weapon");
                        room->setPlayerMark(player, "@tianyi_Weapon", 0);
                        mokai::tianyi_suit(room, player, card, false);
                        tianyi_draw = true;
                    } else if (card->hasFlag("tianyi_Armor")) {
                        room->setCardFlag(card, "-tianyi_Armor");
                        room->setPlayerMark(player, "@tianyi_Armor", 0);
                        room->setPlayerMark(player, "Armor_Nullified", 0);
                        mokai::tianyi_suit(room, player, card, false);
                        tianyi_draw = true;
                    } else if (card->hasFlag("tianyi_DefensiveHorse")){
                        room->setCardFlag(card, "-tianyi_DefensiveHorse");
                        room->setPlayerMark(player, "@tianyi_DefensiveHorse", 0);
                        mokai::tianyi_suit(room, player, card, false);
                        tianyi_draw = true;
                    } else if (card->hasFlag("tianyi_OffensiveHorse")){
                        room->setCardFlag(card, "-tianyi_OffensiveHorse");
                        room->setPlayerMark(player, "@tianyi_OffensiveHorse", 0);
                        mokai::tianyi_suit(room, player, card, false);
                        tianyi_draw = true;
                    } else if (card->hasFlag("tianyi_Treasure")){
                        room->setCardFlag(card, "-tianyi_Treasure");
                        room->setPlayerMark(player, "@tianyi_Treasure", 0);
                        mokai::tianyi_suit(room, player, card, false);
                        tianyi_draw = true;
                    }
                }
                if (tianyi_draw) {

                    room->touhouLogmessage("#tianyiEquip", player);
                    room->notifySkillInvoked(player, objectName());
                    player->drawCards(2);
                }
            }
        } else if (triggerEvent == SlashEffected){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (mokai::tianyi_compare(player, effect.slash)) {
                room->touhouLogmessage("#tianyiEquip1", effect.to, effect.slash->objectName());
                room->notifySkillInvoked(effect.to, objectName());
                room->setEmotion(effect.to, "skill_nullify");
                return true;
            }
        }
        return false;
    }
};


class tianyi_targetmod : public TargetModSkill {
public:
    tianyi_targetmod() : TargetModSkill("#tianyi_targetmod") {
        pattern = "Slash";
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (card->isKindOf("Slash") && mokai::tianyi_compare(from, card))
            return 1000;
        else
            return 0;

    }


};
class tianyi_horse : public DistanceSkill {
public:
    tianyi_horse() : DistanceSkill("#tianyi_horse") {
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        int correct = 0;
        if (from->getMark("@tianyi_OffensiveHorse") > 0)
            correct++;
        if (to->getMark("@tianyi_DefensiveHorse") > 0)
            correct--;
        return correct;
    }
};
class tianyi_attackrange : public AttackRangeSkill{
public:
    tianyi_attackrange() : AttackRangeSkill("#tianyi_attackrange"){

    }

    virtual int getFixed(const Player *target, bool) const{
        if (target->hasSkill("duanjiao"))
            return 3;
        else if (target->getMark("@tianyi_Weapon") > 0)
            return 1;
        return -1;
    }
};
class tianyi_collateral : public ProhibitSkill {
public:
    tianyi_collateral() : ProhibitSkill("#tianyi_collateral") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->getMark("@tianyi_Weapon") > 0 && card->isKindOf("Collateral");
    }
};

protagonistPackage::protagonistPackage()
    : Package("protagonist")
{

    General *zhu001 = new General(this, "zhu001$", "zhu", 4, false);
    zhu001->addSkill(new lingqi);
    zhu001->addSkill(new bllmqixiang);
    //zhu001->addSkill(new hongbai);
    zhu001->addSkill(new boli);

    General *zhu002 = new General(this, "zhu002$", "zhu", 4, false);
    zhu002->addSkill(new mofa);
    //zhu002->addSkill(new heibai);
    zhu002->addSkill(new wuyu);

    General *zhu003 = new General(this, "zhu003", "zhu", 4, false);
    zhu003->addSkill(new saiqian);

    General *zhu004 = new General(this, "zhu004", "zhu", 3, false);
    zhu004->addSkill(new jiezou);
    zhu004->addSkill(new shoucang);
    zhu004->addSkill(new shoucangKeep);
    related_skills.insertMulti("shoucang", "#shoucang");

    General *zhu005 = new General(this, "zhu005", "zhu", 4, false);
    zhu005->addSkill(new baoyi);

    General *zhu006 = new General(this, "zhu006", "zhu", 4, false);
    zhu006->addSkill(new zhize);
    zhu006->addSkill(new chunxi);

    General *zhu007 = new General(this, "zhu007", "zhu", 4, false);
    zhu007->addSkill(new bllmwuyu);
    zhu007->addSkill(new bllmcaiyu);
    zhu007->addSkill(new bllmseyu);
    zhu007->addSkill(new bllmseyu_clear);
    zhu007->addSkill(new bllmmingyu);
    zhu007->addSkill(new bllmshuiyu);
    zhu007->addSkill(new bllmshuiyumc);
    related_skills.insertMulti("bllmwuyu", "#bllmcaiyu");
    related_skills.insertMulti("bllmwuyu", "#bllmseyu");
    related_skills.insertMulti("bllmwuyu", "#bllmseyu_clear");
    related_skills.insertMulti("bllmwuyu", "#bllmmingyu");
    related_skills.insertMulti("bllmwuyu", "#bllmshuiyu");
    related_skills.insertMulti("bllmwuyu", "#bllmshuiyu2");


    General *zhu008 = new General(this, "zhu008", "zhu", 3, false);
    zhu008->addSkill(new qiangyu);
    zhu008->addSkill(new mokai);

    zhu008->addSkill(new tianyi_targetmod);
    zhu008->addSkill(new tianyi_horse);
    zhu008->addSkill(new tianyi_attackrange);
    zhu008->addSkill(new tianyi_collateral);



    //addMetaObject<lingqiCard>();
    addMetaObject<mofaCard>();
    addMetaObject<wuyuCard>();
    addMetaObject<saiqianCard>();
    addMetaObject<jiezouCard>();
    addMetaObject<shoucangCard>();
    addMetaObject<baoyiCard>();
    addMetaObject<bllmseyuCard>();
    addMetaObject<bllmshiyudummy>();
    addMetaObject<bllmshiyuCard>();
    addMetaObject<bllmwuyuCard>();


    skills << new wuyuvs << new saiqianvs << new touhou_tianyi;
}

ADD_PACKAGE(protagonist)

