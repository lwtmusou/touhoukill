#include "th14.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"
#include "maneuvering.h" //for skill leiting

class Baochui : public TriggerSkill
{
public:
    Baochui() : TriggerSkill("baochui")
    {
        events << EventPhaseStart;;
    }

    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {    
        TriggerList skill_list;
        if (player->getPhase() == Player::Start && player->getHandcardNum() < 3) {
            QList<ServerPlayer *> srcs = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *src, srcs) {
                if (src->canDiscard(src, "he"))
                    skill_list.insert(src, QStringList(objectName()));
            }   
        }else if (player->getPhase() == Player::Discard && player->getHandcardNum() < 3 && player->hasFlag(objectName())) {
            skill_list.insert(player, QStringList(objectName()));
        }
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *src) const
    {
        if (player->getPhase() == Player::Start) {
            const Card *card = room->askForCard(src, ".|.|.|hand,equipped", "@baochui:" + player->objectName(), QVariant::fromValue(player), Card::MethodDiscard,
                    NULL, false, objectName());
            if (card) 
                return true;
            else
                return false;
        }
        return true;
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *src) const
    {
        if (player->getPhase() == Player::Start) {
            ServerPlayer *src = room->findPlayerBySkillName(objectName());
            player->drawCards(3 - player->getHandcardNum());
            room->setPlayerFlag(player, objectName());
        } else if (player->getPhase() == Player::Discard ) {
            room->setPlayerFlag(player, "-" + objectName());
            room->touhouLogmessage("#BaochuiBuff", player, objectName(), QList<ServerPlayer *>(), objectName());
            room->loseHp(player, 1);
        }
        return false;
    }
};

class Yicun : public TriggerSkill
{
public:
    Yicun() : TriggerSkill("yicun")
    {
        frequency = Compulsory;
        events << TargetConfirming; 
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") 
                && use.to.contains(player) && use.from->getHandcardNum() >= player->getHandcardNum()) 
                return QStringList(objectName());
        }
        return QStringList();
    }
    

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            room->notifySkillInvoked(player, objectName());
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            use.nullified_list << player->objectName();
            data = QVariant::fromValue(use);
        }
        return false;
    }
};


class Moyi : public TriggerSkill
{
public:
    Moyi() : TriggerSkill("moyi$")
    {
        events << EventPhaseEnd << CardsMoveOneTime << EventPhaseChanging;
    }


    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        ServerPlayer *current = room->getCurrent();
        if (!current || current->getKingdom() != "hzc") 
            return;
        if (triggerEvent == CardsMoveOneTime) {
            if (current->getPhase() != Player::Discard)
                return;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile) {
                QVariantList ids1 = current->tag["moyi_basics"].toList();
                foreach (int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->isKindOf("BasicCard") && !ids1.contains(id))
                        ids1 << id;
                }
                current->tag["moyi_basics"] = ids1;
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Discard)
                current->tag.remove("moyi_basics");
        }
        
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        ServerPlayer *current = room->getCurrent();
        if (!current || current->getKingdom() != "hzc") 
            return QStringList();
            
        QStringList skill_list;    
        if (triggerEvent == EventPhaseEnd && current->getPhase() == Player::Discard) {

            QVariantList ids = current->tag["moyi_basics"].toList();
            if (ids.length() == 0 || !current->isAlive())
                return QStringList();
            
            QVariantList all;
            foreach (QVariant card_data, ids) {
                if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
                    all << card_data.toInt();
            }
            if (all.length() == 0)
                return QStringList();

            current->tag["moyi_basics"] = all;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasLordSkill(objectName()))
                    skill_list << p->objectName() + "'" + objectName();
            }
        } 
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *skill_invoker) const
    {
        skill_invoker->tag["moyi-target"] = QVariant::fromValue(target);
        if (skill_invoker->askForSkillInvoke(objectName(), QVariant::fromValue(target))) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(target, objectName());
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = skill_invoker;
            log.to << target;
            log.arg = objectName();
            room->sendLog(log);
            return true;
        }
        skill_invoker->tag.remove("moyi-target");
        return false;
    }
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *skill_invoker) const
    {
        ServerPlayer *current = room->getCurrent();
        QVariantList ids = current->tag["moyi_basics"].toList();
        QList<int> all;
        foreach (QVariant card_data, ids) {
            if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
                all << card_data.toInt();
        }
        
        room->fillAG(all, current);
        int moyiId = room->askForAG(current, all, false, objectName());
        room->clearAG(current);
        all.removeOne(moyiId);
        room->obtainCard(target, moyiId, true);
        return false;
    }
};

LeitingCard::LeitingCard()
{
    mute = true;
}
void LeitingCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    effect.from->drawCards(2);
    const Card *cards = room->askForCard(effect.from, ".|.|.|hand!", "@leiting:" + effect.to->objectName(), QVariant::fromValue(effect.to), Card::MethodDiscard);
    Card *discard = Sanguosha->getCard(cards->getEffectiveId());
    if (discard->getSuit() == Card::Heart) {
        effect.to->drawCards(1);
        room->damage(DamageStruct("leiting", NULL, effect.to, 1, DamageStruct::Thunder));
    } else if (discard->getSuit() == Card::Spade) {
        ThunderSlash *slash = new ThunderSlash(Card::NoSuit, 0);
        // if return without usecard,we need delete this new thunderslash?
        if (effect.to->isCardLimited(slash, Card::MethodUse))
            return;
        QList<ServerPlayer *> listt;
        foreach (ServerPlayer *p, room->getOtherPlayers(effect.to)) {
            if (effect.to->inMyAttackRange(p) && effect.to->canSlash(p, slash, true))
                listt << p;
        }
        if (listt.length() <= 0)
            return;
        ServerPlayer *target = room->askForPlayerChosen(effect.to, listt, "leiting", "@leiting_chosen:" + effect.from->objectName(), false);

        if (target != NULL) {
            slash->setSkillName("_leiting");
            room->useCard(CardUseStruct(slash, effect.to, target), false);
        }
    }
}

class Leiting : public ZeroCardViewAsSkill
{
public:
    Leiting() : ZeroCardViewAsSkill("leiting")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("LeitingCard");
    }

    virtual const Card *viewAs() const
    {
        return new LeitingCard;
    }
};

class Nizhuan : public TriggerSkill
{
public:
    Nizhuan() : TriggerSkill("nizhuan")
    {
        events << TargetConfirmed;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {    
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.to.length() == 1) {
                ServerPlayer* target = use.to.first();
                if (use.from == NULL || use.from->isDead() || use.from->getLostHp() >= target->getLostHp())
                    return QStringList();
                
                Slash *slash = new Slash(Card::NoSuit, 0);
                slash->setSkillName("_" + objectName());
                if (target->isCardLimited(slash, Card::MethodUse) || !target->canSlash(use.from, slash, false))
                    return QStringList();
                

                    if (player->canDiscard(target, "h"))
                        return QStringList(objectName());
            }        
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            QString prompt = "target:" + use.from->objectName() + ":" + use.to.first()->objectName();
            player->tag["nizhuan_carduse"] = data;
            return room->askForSkillInvoke(player, objectName(), prompt);
        }
        return false;
    }
    
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            ServerPlayer* target = use.to.first();
            use.nullified_list << target->objectName();
            data = QVariant::fromValue(use);
            
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_" + objectName());
            int id = room->askForCardChosen(player, target, "h", objectName(), false, Card::MethodDiscard);
            room->throwCard(id, target, player);
            room->useCard(CardUseStruct(slash, target, use.from), false);        
        } 
        return false;
    }
};

class Guizha : public TriggerSkill
{
public:
    Guizha() : TriggerSkill("guizha")
    {
        events << AskForPeaches;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        ServerPlayer *victim = room->getCurrentDyingPlayer();
        if (victim == NULL || !victim->hasSkill("guizha") || victim == player)
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        ServerPlayer *victim = room->getCurrentDyingPlayer();
        Peach *dummy_peach = new Peach(Card::SuitToBeDecided, -1);
        dummy_peach->deleteLater();
        if (player->isCardLimited(dummy_peach, Card::MethodUse))
            return false;
        bool hasPeach = false;
        foreach (const Card *card, player->getHandcards()) {
            if (card->isKindOf("Peach")) {
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
        while (hasPeach && victim->getHp() < 1) {
            const Card *supply_card = room->askForCard(player, "Peach|.|.|hand!", "@guizha:" + victim->objectName(), data, Card::MethodNone, victim, false, objectName(), false);
            Peach *peach = new Peach(Card::SuitToBeDecided, -1);
            peach->addSubcard(supply_card->getEffectiveId());
            peach->setSkillName("_guizha");
            room->useCard(CardUseStruct(peach, player, victim), false);
            if (victim->getHp() > 0) {
                room->setPlayerFlag(victim, "-Global_Dying");
                return true; //avoid triggering askforpeach
            }

            hasPeach = false;
            foreach (const Card *card, player->getHandcards()) {
                if (card->isKindOf("Peach")) {
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


class Yuyin : public TriggerSkill
{
public:
    Yuyin() : TriggerSkill("yuyin")
    {
        events << Damaged;
    }

     virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getHp() > player->getHp() && !p->isNude())
                return QStringList(objectName());
        }
        return QStringList();
    }
    

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QList<ServerPlayer *> listt;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getHp() > player->getHp() && !p->isNude())
                listt << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@" + objectName(), true, true);

        if (target) {
            int id = room->askForCardChosen(player, target, "he", objectName());
            room->obtainCard(player, id, room->getCardPlace(id) != Player::PlaceHand);
        }
        return false;
    }
};

class Wuchang : public TriggerSkill
{
public:
    Wuchang() : TriggerSkill("wuchang")
    {
        events << EventPhaseEnd << CardsMoveOneTime << EventPhaseChanging;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        ServerPlayer *current = room->getCurrent();
        if (!current)
            return;
        if (triggerEvent == CardsMoveOneTime) {
            if (current->getPhase() != Player::Discard)
                return;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == current &&
                (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                foreach (int id, move.card_ids) {
                    if ((move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand) &&
                        Sanguosha->getCard(id)->isRed()) {
                        room->setTag("wuchang", false);
                        break;
                    }
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Discard)
                room->setTag("wuchang", true);
            if (change.to == Player::Discard)
                room->setTag("wuchang", true);
        }
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    { 
        ServerPlayer *current = room->getCurrent();
        if (!current)
            return TriggerList();
        if (triggerEvent == EventPhaseEnd && current->getPhase() == Player::Discard) {
            if (!room->getTag("wuchang").toBool())
                return TriggerList();
            //room->setTag("wuchang", true);
            TriggerList skill_list;
            if (!current->isKongcheng() && current->canDiscard(current, "h")) {
                QList<ServerPlayer *> srcs = room->findPlayersBySkillName(objectName());
                foreach (ServerPlayer *src, srcs) {
                    if (src != current)
                        skill_list.insert(src, QStringList(objectName()));
                }
            }
            return skill_list;
        }
        return TriggerList();
    }
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *current, QVariant &data, ServerPlayer *source) const
    {
        QString prompt = "target:" + current->objectName();
        return room->askForSkillInvoke(source, objectName(), prompt);
    }
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *current, QVariant &data, ServerPlayer *source) const
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), current->objectName());

        room->askForDiscard(current, objectName(), 1, 1, false, false, "wuchang_discard");
        return false;
    }
};


class Canxiang : public TriggerSkill
{
public:
    Canxiang() : TriggerSkill("canxiang")
    {
        events << Damage;
    }

     virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getHp() > player->getHp() && !p->isNude())
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QList<ServerPlayer *> listt;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getHp() > player->getHp() && !p->isNude())
                listt << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@" + objectName(), true, true);

        if (target) {
            int id = room->askForCardChosen(player, target, "he", objectName());
            room->obtainCard(player, id, room->getCardPlace(id) != Player::PlaceHand);
        }
        return false;
    }
};

class Juwang : public TriggerSkill
{
public:
    Juwang() : TriggerSkill("juwang")
    {
        events << CardAsked;
    }

     virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        QString pattern = data.toStringList().first();
        if (pattern == "jink") {
            Jink *jink = new Jink(Card::NoSuit, 0);
            jink->deleteLater();
            //need check
            if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
                if (player->isCardLimited(jink, Card::MethodResponse))
                    return QStringList();
            } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
                if (player->isCardLimited(jink, Card::MethodUse))
                    return QStringList();
            }

            ServerPlayer *current = room->getCurrent();
            if (!current || !current->isAlive())
                return QStringList();
            return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {    
        ServerPlayer *current = room->getCurrent();
        return player->askForSkillInvoke(objectName(), "throw:" + current->objectName());
    }
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        ServerPlayer *current = room->getCurrent(); 
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), current->objectName());
        const Card *card = room->askForCard(current, ".|red|.|hand", "@juwang:" + player->objectName(), data, Card::MethodDiscard, NULL, false, objectName());
        if (card == NULL)
            room->damage(DamageStruct(objectName(), player, current, 1));

        return false;
    }
};



class Langying : public TriggerSkill
{
public:
    Langying() : TriggerSkill("langying")
    {
        events << CardAsked;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        QString pattern = data.toStringList().first();
        if (pattern == "jink") {
            if (player->getEquips().length() == 0)
                return QStringList();

            Jink *jink = new Jink(Card::NoSuit, 0);
            jink->deleteLater();
            //need check
            if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
                if (player->isCardLimited(jink, Card::MethodResponse))
                    return QStringList();
            } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
                if (player->isCardLimited(jink, Card::MethodUse))
                    return QStringList();
            }
            return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName(), data);
    }
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
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
        return false;
    }
};


YuanfeiCard::YuanfeiCard()
{
    mute = true;
}

bool YuanfeiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !Self->inMyAttackRange(to_select)
        && to_select != Self;
}
void YuanfeiCard::onEffect(const CardEffectStruct &effect) const
{
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

YuanfeiNearCard::YuanfeiNearCard()
{
    mute = true;
    m_skillName = "yuanfei";
}
bool YuanfeiNearCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && Self->inMyAttackRange(to_select)
        && to_select != Self;
}
void YuanfeiNearCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    ServerPlayer *target = effect.to;

    room->setPlayerCardLimitation(target, "use,response", "BasicCard|.|.|.", true);
    room->setPlayerCardLimitation(target, "use,response", "EquipCard|.|.|.", true);
    room->setPlayerCardLimitation(target, "use,response", "TrickCard|.|.|.", true);

    room->setPlayerFlag(target, "yuanfei");
    room->touhouLogmessage("#yuanfei", target, "yuanfei");
}


class Yuanfei : public ViewAsSkill
{
public:
    Yuanfei() : ViewAsSkill("yuanfei")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("YuanfeiCard") && !player->hasUsed("YuanfeiNearCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() >= 1)
            return false;

        if (to_select->isEquipped())
            return false;

        if (Self->isJilei(to_select))
            return false;

        return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 0)
            return new YuanfeiCard;
        if (cards.length() != 1)
            return NULL;

        YuanfeiNearCard *card = new YuanfeiNearCard;
        card->addSubcards(cards);

        return card;
    }
};

class YuanfeiClear : public TriggerSkill
{
public:
    YuanfeiClear() : TriggerSkill("#yuanfei_clear")
    {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("yuanfei")) {
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
        return QStringList();
    }
};


class FeitouVS : public OneCardViewAsSkill
{
public:
    FeitouVS() : OneCardViewAsSkill("feitou")
    {
        filter_pattern = ".|.|.|feitou";
        expand_pile = "feitou";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        //need check limitation?
        return  player->getPile("feitou").length() > 0;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern.contains("slash") && player->getPile("feitou").length() > 0;
        //&& Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName("feitou");
            return slash;
        } else
            return NULL;
    }
};

class Feitou : public TriggerSkill
{
public:
    Feitou() : TriggerSkill("feitou")
    {
        events << EventPhaseStart;
        view_as_skill = new FeitouVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Finish && !player->isNude()) 
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        const Card *cards = room->askForCard(player, ".|.|.|hand,equipped", "addfeitou", data, Card::MethodNone, false);
        if (cards) {
            room->notifySkillInvoked(player, objectName());
            room->touhouLogmessage("#InvokeSkill", player, objectName());
            player->addToPile("feitou", cards->getSubcards().first());
        }
        return false;
    }
};

class FeitouTargetMod : public TargetModSkill
{
public:
    FeitouTargetMod() : TargetModSkill("#feitoumod")
    {
        pattern = "Slash";
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const
    {
        if (from->hasSkill("feitou") && card->getSkillName() == "feitou")
            return 1000;
        else
            return 0;
    }

    virtual int getResidueNum(const Player *from, const Card *card) const
    {
        if (from->hasSkill("feitou") && card->getSkillName() == "feitou")
            return 1000;
        else
            return 0;
    }

};




class Shizhu : public TriggerSkill
{
public:
    Shizhu() : TriggerSkill("shizhu")
    {
        events << EventPhaseStart << EventPhaseChanging << CardsMoveOneTime;
    }

   
    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        if (triggerEvent== CardsMoveOneTime){
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile) {
                QList<int>    temp_ids;
                QVariantList shizhu_ids = room->getTag("shizhuPeach").toList();
                foreach(QVariant card_data, shizhu_ids)
                    temp_ids << card_data.toInt();

                foreach (int id, move.card_ids) {
                    Card *card = Sanguosha->getCard(id);
                    if (card->isKindOf("Peach") && !temp_ids.contains(id)
                        && room->getCardPlace(id) == Player::DiscardPile)
                        shizhu_ids << id;
                }
                room->setTag("shizhuPeach", shizhu_ids);
            }
        }else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->removeTag("shizhuPeach");
        }
        
    }
    
    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    { 
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish) {
            QVariantList  temp_ids;
            QVariantList shizhu_ids = room->getTag("shizhuPeach").toList();
            foreach (QVariant card_data, shizhu_ids) {
                if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
                    temp_ids << card_data.toInt();
            }
            if (temp_ids.length() == 0)
                return TriggerList();
            room->setTag("shizhuPeach", temp_ids);
            TriggerList skill_list;
            QList<ServerPlayer *> srcs = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *src, srcs) {
                if (!src->isCurrent())
                    skill_list.insert(src, QStringList(objectName()));
            }
            return skill_list;
        }
        return TriggerList();
    }
    
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *source) const
    {
        return room->askForSkillInvoke(source, objectName(), data);
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *source) const
    {
       
        QList<int>    temp_ids;
        QVariantList shizhu_ids = room->getTag("shizhuPeach").toList();
        foreach (QVariant card_data, shizhu_ids) {
            if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
                temp_ids << card_data.toInt();
        }

        room->fillAG(temp_ids, source);
       int id = room->askForAG(source, temp_ids, false, objectName());
        room->clearAG(source);
        if (id > -1) {
            Card *peach = Sanguosha->getCard(id);
            room->showAllCards(source);
            room->getThread()->delay(1000);
            room->clearAG();
            bool no_peach = true;
            foreach (const Card *card, source->getHandcards()) {
                if (card->isKindOf("Peach")) {
                    no_peach = false;
                    break;
                }
            }
            if (no_peach)
                source->obtainCard(peach, true);
        }
                
            

        
        return false;
    }
};


LiangeCard::LiangeCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    mute = true;
}
void LiangeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
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
class Liange : public OneCardViewAsSkill
{
public:
    Liange() :OneCardViewAsSkill("liange")
    {
        filter_pattern = ".|.|.|.";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("LiangeCard");
    }


    virtual const Card *viewAs(const Card *originalCard) const
    {
        LiangeCard *card = new LiangeCard;
        card->addSubcard(originalCard);
        return card;
    }
};


TH14Package::TH14Package()
    : Package("th14")
{
    General *shinmyoumaru = new General(this, "shinmyoumaru$", "hzc", 3, false);
    shinmyoumaru->addSkill(new Baochui);
    shinmyoumaru->addSkill(new Yicun);
    shinmyoumaru->addSkill(new Moyi);

    General *raiko = new General(this, "raiko", "hzc", 4, false);
    raiko->addSkill(new Leiting);



    General *seija = new General(this, "seija", "hzc", 3, false);
    seija->addSkill(new Nizhuan);
    seija->addSkill(new Guizha);

    General *benben = new General(this, "benben", "hzc", 3, false);
    benben->addSkill(new Yuyin);
    benben->addSkill(new Wuchang);
    
    General *yatsuhashi = new General(this, "yatsuhashi", "hzc", 3, false);
    yatsuhashi->addSkill(new Canxiang);
    yatsuhashi->addSkill(new Juwang);



    General *kagerou = new General(this, "kagerou", "hzc", 4, false);
    kagerou->addSkill(new Langying);
    kagerou->addSkill(new Yuanfei);
    kagerou->addSkill(new YuanfeiClear);
    related_skills.insertMulti("yuanfei", "#yuanfei_clear");

    General *sekibanki = new General(this, "sekibanki", "hzc", 4, false);
    sekibanki->addSkill(new Feitou);
    sekibanki->addSkill(new FeitouTargetMod);
    related_skills.insertMulti("feitou", "#feitoumod");

    General *wakasagihime = new General(this, "wakasagihime", "hzc", 3, false);
    wakasagihime->addSkill(new Shizhu);
    wakasagihime->addSkill(new Liange);


    addMetaObject<LeitingCard>();
    addMetaObject<YuanfeiCard>();
    addMetaObject<YuanfeiNearCard>();
    addMetaObject<LiangeCard>();
}

ADD_PACKAGE(TH14)

