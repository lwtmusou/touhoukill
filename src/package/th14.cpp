#include "th14.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"
#include "maneuvering.h" //for skill leiting


leitingCard::leitingCard() {
    mute = true;
}
void leitingCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    effect.from->drawCards(2);
    const Card *cards = room->askForCard(effect.from, ".|.|.|hand!", "@leiting:" + effect.to->objectName(), QVariant::fromValue(effect.to), Card::MethodDiscard);
    Card *discard = Sanguosha->getCard(cards->getEffectiveId());
    if (discard->getSuit() == Card::Heart){
        effect.to->drawCards(1);
        room->damage(DamageStruct("leiting", NULL, effect.to, 1, DamageStruct::Thunder));
    }
    else if (discard->getSuit() == Card::Spade){
        ThunderSlash *slash = new ThunderSlash(Card::NoSuit, 0);
        // if return without usecard,we need delete this new thunderslash?
        if (effect.to->isCardLimited(slash, Card::MethodUse))
            return;
        QList<ServerPlayer *> listt;
        foreach(ServerPlayer *p, room->getAlivePlayers()) {
            if (effect.to->inMyAttackRange(p) && effect.to->canSlash(p, slash, true))
                listt << p;
        }
        if (listt.length() <= 0)
            return;
        ServerPlayer *target = room->askForPlayerChosen(effect.to, listt, "leiting", "@leiting_chosen:" + effect.from->objectName(), false);

        if (target != NULL){
            slash->setSkillName("_leiting");
            room->useCard(CardUseStruct(slash, effect.to, target), false);
        }
    }
}

class leiting : public ZeroCardViewAsSkill {
public:
    leiting() : ZeroCardViewAsSkill("leiting") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("leitingCard");
    }

    virtual const Card *viewAs() const{
        return new leitingCard;
    }
};

class nizhuan : public TriggerSkill {
public:
    nizhuan() : TriggerSkill("nizhuan") {
        events << TargetConfirmed << SlashEffected;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return (player != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (!player->hasSkill("nizhuan"))
                return false;
            if (use.card->isKindOf("Slash") && use.to.length() == 1){
                ServerPlayer *target = use.to.first();
                if (use.from == NULL || use.from->isDead() || use.from->getLostHp() >= target->getLostHp())
                    return false;

                Slash *slash = new Slash(Card::NoSuit, 0);
                slash->setSkillName("_" + objectName());
                if (target->isCardLimited(slash, Card::MethodUse) || !target->canSlash(use.from, slash, false))
                    return false;

                //need a prompt
                QString prompt = "target:" + use.from->objectName() + ":" + target->objectName();
                player->tag["nizhuan_carduse"] = data;
                if (player->canDiscard(target, "h") && room->askForSkillInvoke(player, objectName(), prompt)){
                    room->setCardFlag(use.card, "nizhuan" + target->objectName());
                    int id = room->askForCardChosen(player, target, "h", objectName(), false, Card::MethodDiscard);
                    room->throwCard(id, target, player);
                    room->useCard(CardUseStruct(slash, target, use.from), false);
                }
            }
        }
        else if (triggerEvent == SlashEffected){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash != NULL && effect.slash->hasFlag("nizhuan" + effect.to->objectName())){
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.slash->objectName(), QList<ServerPlayer *>(), objectName());
                room->setEmotion(effect.to, "skill_nullify");
                return true;
            }
        }
        return false;
    }
};

class guizha : public TriggerSkill {
public:
    guizha() : TriggerSkill("guizha") {
        events << AskForPeaches;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return (player != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *victim = room->getCurrentDyingPlayer();
        if (victim == NULL || !victim->hasSkill("guizha") || victim == player)
            return false;
        Peach *dummy_peach = new Peach(Card::SuitToBeDecided, -1);
        dummy_peach->deleteLater();
        if (player->isCardLimited(dummy_peach, Card::MethodUse))
            return false;
        bool hasPeach = false;
        foreach(const Card *card, player->getHandcards()) {
            if (card->isKindOf("Peach")){
                hasPeach = true;
                break;
            }
        }
        room->touhouLogmessage("#TriggerSkill", victim, objectName());
        room->notifySkillInvoked(victim, objectName());
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, victim->objectName(), player->objectName());
            
        room->showAllCards(player);
        room->getThread()->delay(1000);
        room->clearAG();
        while (hasPeach && victim->getHp() < 1){
            const Card *supply_card = room->askForCard(player, "Peach|.|.|hand!", "@guizha:" + victim->objectName(), data, Card::MethodNone, victim, false, objectName(), false);
            Peach *peach = new Peach(Card::SuitToBeDecided, -1);
            peach->addSubcard(supply_card->getEffectiveId());
            peach->setSkillName("_guizha");
            room->useCard(CardUseStruct(peach, player, victim), false);
            if (victim->getHp() > 0){
                room->setPlayerFlag(victim, "-Global_Dying");
                return true; //avoid triggering askforpeach
            }

            hasPeach = false;
            foreach(const Card *card, player->getHandcards()) {
                if (card->isKindOf("Peach")){
                    hasPeach = true;
                    break;
                }
            }
            room->showAllCards(player);
            room->getThread()->delay(1000);
            room->clearAG();
        }

        return false;
    }
};

class canxiang : public TriggerSkill {
public:
    canxiang() : TriggerSkill("canxiang") {
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QList<ServerPlayer *> listt;
        foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getHp() > player->getHp() && !p->isNude())
                listt << p;
        }
        if (listt.length() <= 0)
            return false;
        ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@" + objectName(), true, true);

        if (target) {
            int id = room->askForCardChosen(player, target, "he", objectName());
            room->obtainCard(player, id, room->getCardPlace(id) != Player::PlaceHand);
        }
        return false;
    }
};

class juwang : public TriggerSkill {
public:
    juwang() : TriggerSkill("juwang") {
        events << CardAsked;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QString pattern = data.toStringList().first();
        if (pattern == "jink") {
            Jink *jink = new Jink(Card::NoSuit, 0);
            jink->deleteLater();
            //need check
            if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE){
                if (player->isCardLimited(jink, Card::MethodResponse))
                    return false;
            }
            else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE){
                if (player->isCardLimited(jink, Card::MethodUse))
                    return false;
            }

            ServerPlayer *current = room->getCurrent();
            if (!current || !current->isAlive())
                return false;
            if (player->askForSkillInvoke(objectName(), "throw:" + current->objectName())){
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), current->objectName());
            
                const Card *card = room->askForCard(current, ".|red|.|hand", "@juwang:" + player->objectName(), data, Card::MethodDiscard, NULL, false, objectName());
                if (card == NULL)
                    room->damage(DamageStruct(objectName(), player, current, 1));
            }
        }
        return false;
    }
};


class yuyin : public TriggerSkill {
public:
    yuyin() : TriggerSkill("yuyin") {
        events << Damaged;
    }


    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QList<ServerPlayer *> listt;
        foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getHp() > player->getHp() && !p->isNude())
                listt << p;
        }
        if (listt.length() <= 0)
            return false;
        ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@" + objectName(), true, true);

        if (target) {
            int id = room->askForCardChosen(player, target, "he", objectName());
            room->obtainCard(player, id, room->getCardPlace(id) != Player::PlaceHand);
        }
        return false;
    }
};

class wuchang : public TriggerSkill {
public:
    wuchang() : TriggerSkill("wuchang") {
        events << EventPhaseEnd << CardsMoveOneTime << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *current = room->getCurrent();
        if (!current)
            return false;
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (!source || source->isCurrent())//source->getPhase() != Player::NotActive
            return false;

        if (triggerEvent == CardsMoveOneTime){
            if (current->getPhase() != Player::Discard)
                return false;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == current &&
                (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                foreach(int id, move.card_ids){
                    if ((move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand) &&
                        Sanguosha->getCard(id)->isRed()){
                        room->setTag("wuchang", false);
                        break;
                    }
                }
            }
        }
        else if (triggerEvent == EventPhaseEnd && current->getPhase() == Player::Discard){
            if (!room->getTag("wuchang").toBool())
                return false;
            room->setTag("wuchang", true);
            if (!current->isKongcheng() && current->canDiscard(current, "h")){
                source->tag["wuchang_target"] = QVariant::fromValue(current);
                QString prompt = "target:" + current->objectName();
                if (!room->askForSkillInvoke(source, objectName(), prompt))
                    return false;
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), current->objectName());
            
                room->askForDiscard(current, objectName(), 1, 1, false, false, "wuchang_discard");
            }
        }
        else if (triggerEvent == EventPhaseChanging){
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Discard)
                room->setTag("wuchang", true);
            if (change.to == Player::Discard)
                room->setTag("wuchang", true);
        }
        return false;
    }
};

class langying : public TriggerSkill {
public:
    langying() : TriggerSkill("langying") {
        events << CardAsked;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QString pattern = data.toStringList().first();
        if (pattern == "jink"){
            if (player->getEquips().length() == 0)
                return false;

            Jink *jink = new Jink(Card::NoSuit, 0);
            jink->deleteLater();
            //need check
            if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE){
                if (player->isCardLimited(jink, Card::MethodResponse))
                    return false;
            }
            else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE){
                if (player->isCardLimited(jink, Card::MethodUse))
                    return false;
            }


            if (player->askForSkillInvoke(objectName(), data)){

                QList<int> equips;
                foreach(const Card *e, (player->getEquips()))
                    equips << e->getId();

                CardsMoveStruct move;
                move.card_ids = equips;
                move.from_place = Player::PlaceEquip;
                move.to_place = Player::PlaceHand;
                move.from = player;
                move.to = player;

                room->moveCardsAtomic(move, true);
                Jink *card = new Jink(Card::NoSuit, 0);
                card->setSkillName("_langying");
                room->provide(card);
            }
        }
        return false;
    }
};


yuanfeiCard::yuanfeiCard() {
    mute = true;
}

bool yuanfeiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !Self->inMyAttackRange(to_select)
        && to_select != Self;
}
void yuanfeiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    ServerPlayer *target = effect.to;
    //room:setPlayerCardLimitation(target, "use,response", pattern, false)
    //".|%1|.|hand$0" color
    //room:setPlayerCardLimitation(player, "use", "Slash", true)
    //room->removePlayerCardLimitation(player, "use,response,discard", jilei_type + "|.|.|hand$1");
    //room:setPlayerCardLimitation(target, "use,response", ".", true)
    room->setPlayerCardLimitation(target, "use,response", "BasicCard|.|.|.", true);
    room->setPlayerCardLimitation(target, "use,response", "EquipCard|.|.|.", true);
    room->setPlayerCardLimitation(target, "use,response", "TrickCard|.|.|.", true);
    //room:setPlayerCardLimitation(target, "use,response", ".|.|.|hand$1", true)
    room->setPlayerFlag(target, "yuanfei");
    room->touhouLogmessage("#yuanfei", target, "yuanfei");
    //room:removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0")   
}

yuanfeiNearCard::yuanfeiNearCard() {
    mute = true;
    m_skillName = "yuanfei";
}
bool yuanfeiNearCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && Self->inMyAttackRange(to_select)
        && to_select != Self;
}
void yuanfeiNearCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    ServerPlayer *target = effect.to;

    room->setPlayerCardLimitation(target, "use,response", "BasicCard|.|.|.", true);
    room->setPlayerCardLimitation(target, "use,response", "EquipCard|.|.|.", true);
    room->setPlayerCardLimitation(target, "use,response", "TrickCard|.|.|.", true);

    room->setPlayerFlag(target, "yuanfei");
    room->touhouLogmessage("#yuanfei", target, "yuanfei");
}


class yuanfei : public ViewAsSkill {
public:
    yuanfei() : ViewAsSkill("yuanfei") {

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("yuanfeiCard") && !player->hasUsed("yuanfeiNearCard");
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
        if (cards.length() == 0)
            return new yuanfeiCard;
        if (cards.length() != 1)
            return NULL;

        yuanfeiNearCard *card = new yuanfeiNearCard;
        card->addSubcards(cards);

        return card;
    }
};

class yuanfei_clear : public TriggerSkill {
public:
    yuanfei_clear() : TriggerSkill("#yuanfei_clear") {
        events << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging){
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive){
                foreach(ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("yuanfei")){
                        p->setFlags("-yuanfei");
                        room->removePlayerCardLimitation(p, "use,response", "BasicCard|.|.|.$1");
                        room->removePlayerCardLimitation(p, "use,response", "EquipCard|.|.|.$1");
                        room->removePlayerCardLimitation(p, "use,response", "TrickCard|.|.|.$1");
                        //room:removePlayerCardLimitation(p, "use,response", ".|.|.|hand$1")
                        //room:removePlayerCardLimitation(p, "use,response", ".$1")
                    }
                }
            }
        }
        return false;
    }
};



class feitouvs : public OneCardViewAsSkill {
public:
    feitouvs() : OneCardViewAsSkill("feitou") {
        filter_pattern = ".|.|.|feitou";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        //need check limitation?
        return  player->getPile("feitou").length() > 0;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern.contains("slash") && player->getPile("feitou").length() > 0;
        //&& Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard != NULL){
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName("feitou");
            return slash;
        }
        else
            return NULL;
    }
};

class feitou : public TriggerSkill {
public:
    feitou() : TriggerSkill("feitou") {
        events << EventPhaseStart;
        view_as_skill = new feitouvs;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart){
            if (player->getPhase() == Player::Finish){
                if (player->isNude())
                    return false;
                const Card *cards = room->askForCard(player, ".|.|.|hand,equipped", "addfeitou", data, Card::MethodNone, false);
                //const Card *cards=room->askForExchange(player, objectName(), 1, true, "addfeitou");
                if (cards){
                    room->notifySkillInvoked(player, objectName());
                    room->touhouLogmessage("#InvokeSkill", player, objectName());
                    player->addToPile("feitou", cards->getSubcards().first());
                }
            }
        }
        return false;
    }
};

class feitoumod : public TargetModSkill {
public:
    feitoumod() : TargetModSkill("#feitoumod") {
        pattern = "Slash";
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (from->hasSkill("feitou") && card->getSkillName() == "feitou")
            return 1000;
        else
            return 0;
    }

    virtual int getResidueNum(const Player *from, const Card *card) const{
        if (from->hasSkill("feitou") && card->getSkillName() == "feitou")
            return 1000;
        else
            return 0;
    }

};




class shizhu : public TriggerSkill {
public:
    shizhu() : TriggerSkill("shizhu") {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return (player != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart){
            if (player->getPhase() == Player::Finish){
                ServerPlayer *source = room->findPlayerBySkillName(objectName());
                if (!source || source->isCurrent())
                    return false;  
                //if (source->getPhase() != Player::NotActive)
                QList<int>    temp_ids;
                QVariantList shizhu_ids = room->getTag("shizhuPeach").toList();
                foreach(QVariant card_data, shizhu_ids){
                     if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
                         temp_ids << card_data.toInt();
                }
                if (temp_ids.length() == 0)
                    return false;
                

                if (room->askForSkillInvoke(source, objectName(), data)) {
                    room->fillAG(temp_ids, source);
                    int id = room->askForAG(source, temp_ids, false, objectName());
                    room->clearAG(source);
                    if (id > -1) {
                        Card *peach = Sanguosha->getCard(id);
                        room->showAllCards(source);
                        room->getThread()->delay(1000);
                        room->clearAG();
                        bool no_peach = true;
                        foreach(const Card *card, source->getHandcards()){
                            if (card->isKindOf("Peach")){
                                no_peach = false;
                                break;
                            }
                        }
                        if (no_peach)
                            source->obtainCard(peach, true);
                    }
                }
            }

        }
        else if (triggerEvent == EventPhaseChanging){
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->removeTag("shizhuPeach");
        }
        return false;
    }
};

class shizhuCount : public TriggerSkill {
public:
    shizhuCount() : TriggerSkill("#shizhu") {
        events << CardsMoveOneTime;

    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to_place == Player::DiscardPile){
            QList<int>    temp_ids;
            QVariantList shizhu_ids = room->getTag("shizhuPeach").toList();
            foreach(QVariant card_data, shizhu_ids)
                temp_ids << card_data.toInt();

            foreach(int id, move.card_ids){
                Card *card = Sanguosha->getCard(id);
                if (card->isKindOf("Peach") && !temp_ids.contains(id)
                    && room->getCardPlace(id) == Player::DiscardPile)                                
                    shizhu_ids << id;
            }
            room->setTag("shizhuPeach", shizhu_ids);
        }
        return false;
    }
};

liangeCard::liangeCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
    mute = true;
}
void liangeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->moveCardTo(Sanguosha->getCard(subcards.first()), NULL, Player::DrawPile);
    QList<int> idlist = room->getNCards(2);//(2, false)
    
    room->fillAG(idlist, targets.first());
    int card_id = room->askForAG(targets.first(), idlist, false, "liange");
    room->clearAG(targets.first());
    room->obtainCard(targets.first(), card_id, false);
    idlist.removeOne(card_id);
    
    DummyCard *dummy = new DummyCard;
    foreach(int id, idlist)
        dummy->addSubcard(id);
    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, targets.first()->objectName(), objectName(), "");
    room->throwCard(dummy, reason, NULL);
}
class liange : public OneCardViewAsSkill {
public:
    liange() :OneCardViewAsSkill("liange") {
        filter_pattern = ".|.|.|.";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("liangeCard");
    }


    virtual const Card *viewAs(const Card *originalCard) const{
        liangeCard *card = new liangeCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class aige : public TriggerSkill {
public:
    aige() : TriggerSkill("aige") {
        events << CardsMoveOneTime;

    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return (player != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (source == NULL || move.from == NULL || move.from == source || player != source)
            return false;
        ServerPlayer *target = NULL;
        if (move.from_places.contains(Player::PlaceDelayedTrick)){
            foreach(ServerPlayer *p, room->getOtherPlayers(source)){
                if (move.from->objectName() == p->objectName()){
                    target = p;
                    break;
                }
            }
        }
        if (target != NULL && source->canDiscard(target, "h")){
            source->tag["aige_target"] = QVariant::fromValue(target);
            if (source->askForSkillInvoke(objectName(), QVariant::fromValue(target))){
                int id = room->askForCardChosen(source, target, "h", objectName());
                room->throwCard(id, target, player);
            }
        }

        return false;
    }
};

class jingtao : public TriggerSkill {
public:
    jingtao() : TriggerSkill("jingtao") {
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from == player && use.card != NULL && use.card->isKindOf("Slash")){
            foreach(ServerPlayer *p, use.to) {
                if (player->canDiscard(p, "e")){
                    player->tag["jingtao_target"] = QVariant::fromValue(p);
                    if (player->askForSkillInvoke(objectName(), "discard:" + p->objectName())){
                        int id = room->askForCardChosen(player, p, "e", objectName(), false, Card::MethodDiscard);
                        room->throwCard(id, p, player);
                    }
                    player->tag.remove("jingtao_target");
                }
            }
        }
        return false;
    }
};

th14Package::th14Package()
    : Package("th14")
{
    General *hzc001 = new General(this, "hzc001$", "hzc",3,false);


    General *hzc002 = new General(this, "hzc002", "hzc", 4, false);
    hzc002->addSkill(new leiting);



    General *hzc003 = new General(this, "hzc003", "hzc", 3, false);
    hzc003->addSkill(new nizhuan);
    hzc003->addSkill(new guizha);

    General *hzc004 = new General(this, "hzc004", "hzc", 3, false);
    hzc004->addSkill(new canxiang);
    hzc004->addSkill(new juwang);

    General *hzc005 = new General(this, "hzc005", "hzc", 3, false);
    hzc005->addSkill(new yuyin);
    hzc005->addSkill(new wuchang);

    General *hzc006 = new General(this, "hzc006", "hzc", 4, false);
    hzc006->addSkill(new langying);
    hzc006->addSkill(new yuanfei);
    hzc006->addSkill(new yuanfei_clear);
    related_skills.insertMulti("yuanfei", "#yuanfei_clear");

    General *hzc007 = new General(this, "hzc007", "hzc", 4, false);
    hzc007->addSkill(new feitou);
    hzc007->addSkill(new feitoumod);
    related_skills.insertMulti("feitou", "#feitoumod");

    General *hzc008 = new General(this, "hzc008", "hzc", 3, false);
    hzc008->addSkill(new shizhu);
    hzc008->addSkill(new shizhuCount);
    related_skills.insertMulti("shizhu", "#shizhu");
    hzc008->addSkill(new liange);
    //hzc008->addSkill(new aige);
    //hzc008->addSkill(new jingtao);

    addMetaObject<leitingCard>();
    addMetaObject<yuanfeiCard>();
    addMetaObject<yuanfeiNearCard>();
    addMetaObject<liangeCard>();
}

ADD_PACKAGE(th14)

