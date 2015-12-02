#include "th07.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"




class Sidie : public TriggerSkill
{
public:
    Sidie() : TriggerSkill("sidie")
    {
        events << DamageCaused << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (triggerEvent == DamageCaused){
            if (!TriggerSkill::triggerable(player)) return QStringList();
            if (player->getPhase() != Player::Play)
                return QStringList();
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.chain || damage.transfer || !damage.by_user)
                return QStringList();
            if (!damage.from  || !damage.to || damage.from == damage.to)
                return QStringList();

            if (player->hasFlag("sidie_used"))
                return QStringList();
            if (damage.card && damage.card->isKindOf("Slash")) {
                Slash *slash = new Slash(Card::NoSuit, 0);
                if (damage.to->isCardLimited(slash, Card::MethodUse))
                    return QStringList();
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (damage.to->canSlash(p, slash, false))
                    return QStringList(objectName());
                }
            }
        }else if (triggerEvent == EventPhaseChanging){
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("sidie_used"))
                    room->setPlayerFlag(player, "-sidie_used");
            }
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<ServerPlayer *> listt;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (damage.to->canSlash(p, NULL, false))
                listt << p;
        }
        player->tag["sidie_target"] = QVariant::fromValue(damage.to);
        ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@sidie:" + damage.to->objectName(), true, true);

        if (target) {
            player->tag["sidie-victim"] = QVariant::fromValue(target);
            room->setPlayerFlag(player, "sidie_used");
            QList<ServerPlayer *> logto;
            logto << damage.to;
            room->touhouLogmessage("#Dongjie", player, "sidie", logto);
            return true;
        } else {
            player->tag.remove("sidie-victim");
            player->tag.remove("sidie_target");
            return false;
        }
        return false;
    }
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        Slash *slash = new Slash(Card::NoSuit, 0);

        ServerPlayer *target = player->tag["sidie-victim"].value<ServerPlayer *>();
        if (target) {    
            player->drawCards(2);
            slash->setSkillName("_" + objectName());
            room->useCard(CardUseStruct(slash, damage.to, target), false);
            return true;
        }
        player->tag.remove("sidie-victim");
        player->tag.remove("sidie_target");
        return false;
    }
};



class Wangxiang : public TriggerSkill
{
public:
    Wangxiang() : TriggerSkill("wangxiang$")
    {
        events << Damaged << FinishJudge;
    }

    virtual QStringList triggerable(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        QStringList skill_list;
        if (event == Damaged){
            if (player->isAlive() && player->getKingdom() == "yym"){
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->hasLordSkill("wangxiang"))
                        skill_list << p->objectName() + "'" + objectName();
                }
            }
        }  else if (event == FinishJudge){
            JudgeStar judge = data.value<JudgeStar>();
            if (judge->reason == objectName() && judge->isGood()) {
                ServerPlayer *uuz = judge->who->tag["uuz_wangxiang"].value<ServerPlayer *>();
                if (uuz)
                    uuz->obtainCard(judge->card);
            }
        }
        
        return skill_list;
    }
    
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *skill_invoker) const{
        //ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@@xueyi", true, true);
        skill_invoker->tag["wangxiang-target"] = QVariant::fromValue(skill_target);
        if (skill_invoker->askForSkillInvoke(objectName(), QVariant::fromValue(skill_target))) {
            skill_invoker->tag["uuz_wangxiang"] = QVariant::fromValue(skill_target);
            
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
        skill_invoker->tag.remove("wangxiang-target");
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *skill_invoker) const{
    
        ServerPlayer *target = skill_invoker->tag["wangxiang-target"].value<ServerPlayer *>();
         if (target) {
            JudgeStruct judge;
            judge.pattern = ".|black";
            judge.who = skill_invoker;
            judge.reason = objectName();
            judge.good = true;

            room->judge(judge);
        }
        skill_invoker->tag.remove("uuz_wangxiang");
        skill_invoker->tag.remove("wangxiang-target");
        return false;
    }
};

class Jingjie : public TriggerSkill
{
public:
    Jingjie() : TriggerSkill("jingjie")
    {
        frequency = Frequent;
        events << Damaged << AfterDrawNCards;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (TriggerSkill::triggerable(player)) {
            int num = 1;
            if (triggerEvent == AfterDrawNCards)
                num = 1;
            else if (triggerEvent == Damaged)
                num = data.value<DamageStruct>().damage;
            QStringList trigger_list;
            for (int i = 1; i <= num; i++) {
                trigger_list << objectName();
            }
            return trigger_list;
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName(), data);
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        player->drawCards(1);
        if (!player->isKongcheng()) {
            const Card *cards = room->askForExchange(player, objectName(), 1, false, "jingjie_exchange");
            int id = cards->getSubcards().first();
            player->addToPile("jingjie", id);
        }
        return false;
    }
    
};


class Sisheng : public TriggerSkill
{
public:
    Sisheng() : TriggerSkill("sisheng")
    {
        events << Dying;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        ServerPlayer *who = room->getCurrentDyingPlayer();
        QList<int> pile = player->getPile("jingjie");
        if (who && pile.length() > 0 && player->canDiscard(who, "he") && who->getHp() < 1) {
            return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName(), data);
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *yukari, QVariant &data, ServerPlayer *) const
    {
        ServerPlayer *who = room->getCurrentDyingPlayer();
        QList<int> ids = yukari->getPile("jingjie");
        room->fillAG(ids, yukari);
        int id = room->askForAG(yukari, ids, false, objectName());
        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
        room->clearAG(yukari);
        room->throwCard(Sanguosha->getCard(id), reason, NULL);
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, yukari->objectName(), who->objectName());

        int id2 = room->askForCardChosen(yukari, who, "he", objectName(), false, Card::MethodDiscard);
        if (yukari->canDiscard(who, id2))
            room->throwCard(id2, who, yukari);
        RecoverStruct recover;
        recover.recover = 1;
        recover.who = yukari;
        room->recover(who, recover);
        
        return false;
    }
};

class Jingdong : public TriggerSkill
{
public:
    Jingdong() : TriggerSkill("jingdong")
    {
        events << EventPhaseChanging;
    }


    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {   
        TriggerList skill_list;
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Discard) {
            QList<ServerPlayer *> yukaris = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *yukari, yukaris) {
                QList<int> pile = yukari->getPile("jingjie");
                if (!pile.isEmpty())
                    skill_list.insert(yukari, QStringList(objectName()));
            }
        }
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *s) const
    {
        s->tag["jingdong_target"] = QVariant::fromValue(player);
        QString prompt = "target:" + player->objectName();
        return room->askForSkillInvoke(s, objectName(), prompt);
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *s) const
    {
        QList<int> pile = s->getPile("jingjie");
        room->fillAG(pile, s);
        int id = room->askForAG(s, pile, false, objectName());
        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
        room->throwCard(Sanguosha->getCard(id), reason, NULL);
        room->clearAG(s);
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, s->objectName(), player->objectName());

        player->skip(Player::Discard);
        s->tag.remove("jingdong_target");
        return false;
    }
};


ZhaoliaoCard::ZhaoliaoCard()
{
    //mute = true;
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void ZhaoliaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{

}

class ZhaoliaoVS : public ViewAsSkill
{
public:
    ZhaoliaoVS() : ViewAsSkill("zhaoliao")
    {
        response_pattern = "@@zhaoliao";
    }
    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0) {
            ZhaoliaoCard *card = new ZhaoliaoCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;

    }
};

class Zhaoliao : public TriggerSkill
{
public:
    Zhaoliao() : TriggerSkill("zhaoliao")
    {
        events << Damaged;
        view_as_skill = new ZhaoliaoVS;
    }

    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {   
        TriggerList skill_list;
        ServerPlayer *a = data.value<DamageStruct>().to;
        QList<ServerPlayer *> rans = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *ran, rans) {
            if (a->isAlive() && a != ran && !ran->isNude())
                skill_list.insert(ran, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return true;
    }
    
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *s) const
    {
        ServerPlayer *a = data.value<DamageStruct>().to;
        s->tag["zhaoliao_target"] = QVariant::fromValue(a);

        const Card *cards = room->askForCard(s, "@@zhaoliao", "@zhaoliao:" + a->objectName(), data, Card::MethodNone);

        if (cards) {
            room->notifySkillInvoked(s, objectName());
            QList<ServerPlayer *> logto;
            logto << player;
            room->touhouLogmessage("#ChoosePlayerWithSkill", s, "zhaoliao", logto);

            room->obtainCard(player, cards, false);
            QString choice;
            ExNihilo *exnihilo = new ExNihilo(Card::SuitToBeDecided, -1);
            if (player->isKongcheng()) {
                bool expand_pile = false;
                if (!player->getPile("wooden_ox").isEmpty())
                    expand_pile = true;
                if (player->hasSkill("shanji") && !player->getPile("piao").isEmpty())
                    expand_pile = true;
                if (!expand_pile)
                    choice = "zhaoliao1";
            } else if (player->isCardLimited(exnihilo, Card::MethodUse, true))
                choice = "zhaoliao1";
            else
                choice = room->askForChoice(s, objectName(), "zhaoliao1+zhaoliao2");

            if (choice == "zhaoliao1")
                s->drawCards(1);
            else {
                room->setPlayerFlag(player, "Global_expandpileFailed");
                const Card *card = room->askForCard(player, ".|.|.|hand,wooden_ox,piao!", "zhaoliaouse", data, Card::MethodNone);
                room->setPlayerFlag(player, "-Global_expandpileFailed");
                int id = card->getSubcards().first();
                exnihilo->addSubcard(id);
                exnihilo->setSkillName("_zhaoliao");
                room->useCard(CardUseStruct(exnihilo, player, QList<ServerPlayer *>()), true);
            }
        }
        return false;
    }
};


class Jiaoxia : public TriggerSkill
{
public:
    Jiaoxia() : TriggerSkill("jiaoxia")
    {
        events << Dying;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        ServerPlayer *who = room->getCurrentDyingPlayer();
        if (who != player || player->getEquips().length() > 0)
            return QStringList();
        return QStringList(objectName());
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName(), data);
    }
    
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        JudgeStruct judge;
        judge.reason = objectName();
        judge.who = player;
        judge.good = false;
        judge.pattern = ".|heart";

        room->judge(judge);
        if (judge.isGood()) {
            RecoverStruct recover;
            recover.recover = 1;
            recover.who = player;
            room->recover(player, recover);
        }
        return false;
    }
};



class Shuangren : public TargetModSkill
{
public:
	Shuangren() : TargetModSkill("shuangren")
    {
        frequency = NotFrequent;
		pattern = "Slash";
    }

    virtual int getExtraTargetNum(const Player *player, const Card *card) const
    {
        if (player->hasSkill(objectName()))
            return 1;
        else
            return 0;
    }
};

class Youming : public TriggerSkill
{
public:
    Youming() :TriggerSkill("youming")
    {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if ((change.to == Player::Draw && !player->isSkipped(Player::Draw))||
        (change.to == Player::Play && !player->isSkipped(Player::Play)))
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        switch (change.to) {
        case Player::Draw: {
            return player->askForSkillInvoke(objectName(), "draw2play");
            break;
        }
        case Player::Play:{
            return player->askForSkillInvoke(objectName(), "play2draw");
            break;
        }
        default:
            return false;
        }
        return false;
    }
    
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        switch (change.to) {
        case Player::Draw: {
            
            player->skip(Player::Draw);
            Player::Phase phase = player->getPhase();
            player->setPhase(Player::Play);
            room->broadcastProperty(player, "phase");
            RoomThread *thread = room->getThread();
            if (!thread->trigger(EventPhaseStart, room, player))
                thread->trigger(EventPhaseProceeding, room, player);
            thread->trigger(EventPhaseEnd, room, player);

            player->setPhase(phase);
            room->broadcastProperty(player, "phase");
            break;
        }

        case Player::Play:{
            
            player->skip(Player::Play);
            Player::Phase phase = player->getPhase();
            player->setPhase(Player::Draw);
            room->broadcastProperty(player, "phase");
            RoomThread *thread = room->getThread();
            if (!thread->trigger(EventPhaseStart, room, player))
                thread->trigger(EventPhaseProceeding, room, player);
            thread->trigger(EventPhaseEnd, room, player);

            player->setPhase(phase);
            room->broadcastProperty(player, "phase");
            break;
        }

        default:
            return false;
        }

        return false;
    }
};



class XiezouVS : public ZeroCardViewAsSkill
{
public:
    XiezouVS() : ZeroCardViewAsSkill("xiezou")
    {
        response_pattern = "@@xiezou";
    }

    virtual const Card *viewAs() const
    {
        QString cardname = Self->property("xiezou_card").toString();
        Card *card = Sanguosha->cloneCard(cardname);

        card->setSkillName("xiezou");
        return card;
    }
};


class Xiezou : public TriggerSkill
{
public:
    Xiezou() : TriggerSkill("xiezou")
    {
        events << EventPhaseEnd << PreCardUsed << CardResponded << EventPhaseChanging;
        view_as_skill = new XiezouVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == PreCardUsed || triggerEvent == CardResponded) {
            if (player->getPhase() == Player::Play) {
                CardStar card = NULL;
                if (triggerEvent == PreCardUsed)
                    card = data.value<CardUseStruct>().card;
                else {
                    CardResponseStruct response = data.value<CardResponseStruct>();
                    if (response.m_isUse)
                        card = response.m_card;
                }
                if (card && card->getHandlingMethod() == Card::MethodUse)
                    room->setPlayerProperty(player, "xiezou_card", card->objectName());
            }
        }else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                room->setPlayerProperty(player, "xiezou_card", QVariant());
            }
        }else if (triggerEvent == EventPhaseEnd) {
            if (player->getPhase() == Player::Play) {
                QString cardname = player->property("xiezou_card").toString();
                Card *card = Sanguosha->cloneCard(cardname);
                if (card == NULL)
                    return QStringList();
                if (card->isKindOf("Slash") || card->isKindOf("Peach") || card->isNDTrick()) {
                    if (player->isCardLimited(card, Card::MethodUse))
                        return QStringList();
                    //if (card->isKindOf("Peach") && !player->isWounded())
                    //    return false;
                    return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == EventPhaseEnd) {
            QString cardname = player->property("xiezou_card").toString();
            Card *card = Sanguosha->cloneCard(cardname);
            QString prompt = "@xiezou:" + card->objectName();
            room->askForUseCard(player, "@@xiezou", prompt);
        } 
        return false;
    }
};


class Hesheng : public TriggerSkill
{
public:
    Hesheng() : TriggerSkill("hesheng")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        bool can = false;
        foreach (ServerPlayer *p, (room->getAlivePlayers())) {
            if (p->getCards("j").length() > 0) {
                can = true;
                break;
            }
        }
        if (can) 
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return true;
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->touhouLogmessage("#hesheng", player, "hesheng", QList<ServerPlayer *>(), QString::number(damage.damage));
        room->notifySkillInvoked(player, objectName());
        return true;
        
    }
};



class Renou : public TriggerSkill
{
public:
    Renou() : TriggerSkill("renou")
    {
        events << EventPhaseStart;
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Start)
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return room->askForSkillInvoke(player, objectName(), data);
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        
        QList<int> list = room->getNCards(5);
        QList<int> able;
        QList<int> disabled;
        foreach (int id, list) {
            Card *tmp_card = Sanguosha->getCard(id);
            if (tmp_card->isKindOf("EquipCard")) {
                    able << id;
            } else {
                foreach (const Card *c, player->getCards("e")) {
                    if (c->getSuit() == tmp_card->getSuit()) {
                        disabled << id;
                        break;
                    }
                }
                if (!disabled.contains(id))
                    able << id;
            }
        }
        room->fillAG(list, NULL, disabled);
        QStringList cardinfo;
        //for log
        foreach (int id, list) {
            cardinfo << Sanguosha->getCard(id)->toString();
        }
        LogMessage mes;
        mes.type = "$TurnOver";
        mes.from = player;
        mes.card_str = cardinfo.join("+");
        room->sendLog(mes);

        int obtainId = -1;
        if (able.length() > 0) {
            obtainId = room->askForAG(player, able, true, objectName());
            if (obtainId != -1)
                room->obtainCard(player, obtainId, true);
        }

        room->getThread()->delay(1000);

        //throw other cards
        DummyCard *dummy = new DummyCard;
        foreach (int id, list) {
            if (id != obtainId)
                dummy->addSubcard(id);
        }
        if (dummy->getSubcards().length() > 0) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), "");
            room->throwCard(dummy, reason, NULL);
        }
        room->clearAG();
        
        return false;
    }
};


class ZhanzhenVS : public OneCardViewAsSkill
{
public:
    ZhanzhenVS() : OneCardViewAsSkill("zhanzhen")
    {
        filter_pattern = "EquipCard";
        response_or_use = true;
    }


    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "jink" || pattern == "slash";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "jink") {
                Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
                jink->addSubcard(originalCard);
                jink->setSkillName(objectName());
                return jink;
            } else {
                Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
                slash->addSubcard(originalCard);
                slash->setSkillName(objectName());
                return slash;
            }
        } else
            return NULL;
    }
};

class Zhanzhen : public TriggerSkill
{
public:
    Zhanzhen() : TriggerSkill("zhanzhen")
    {
        events << CardsMoveOneTime;
        view_as_skill = new ZhanzhenVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from && player == move.from && move.card_ids.length() == 1 && move.to_place == Player::DiscardPile
                && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE
                || (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE
                )){
            const Card *card = move.reason.m_extraData.value<const Card *>();
            if (card && card->getSkillName() == objectName()
                    && room->getCardPlace(move.card_ids.first()) == Player::DiscardPile
                    )  
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        const Card *card = move.reason.m_extraData.value<const Card *>();
        const Card *realcard = Sanguosha->getEngineCard(move.card_ids.first());
        player->tag["zhanzhen"] = QVariant::fromValue(realcard);
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(),
                        "@zhanzhen:" + card->objectName() + ":" + realcard->objectName(), true, true);
        if (target) {
            CardsMoveStruct mo;
            mo.card_ids = move.card_ids;
            mo.to = target;
            mo.to_place = Player::PlaceHand;
            room->moveCardsAtomic(mo, true);
        }
        return false;
    }
};



class Shishen : public TriggerSkill
{
public:
    Shishen() : TriggerSkill("shishen")
    {
        events << EventPhaseStart << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if ((triggerEvent == EventPhaseStart &&  player->getPhase() == Player::Start) || triggerEvent == Damaged) {
            if (player->getMark("@shi") > 0) 
                return QStringList(objectName());
        } else if (triggerEvent == EventPhaseStart &&  player->getPhase() == Player::Play) {
            if (player->getMark("@shi") == 0)
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player->getMark("@shi") > 0) {
            if (triggerEvent == Damaged)
                player->setFlags("shishen_choice");//for ai
            QString choice = room->askForChoice(player, objectName(), "shishen1+cancel");
            player->setFlags("-shishen_choice");
            if (choice == "shishen1") {
                player->loseMark("@shi");
                room->notifySkillInvoked(player, objectName());
            }
        } else if (player->getMark("@shi") == 0){
            QString choice = room->askForChoice(player, objectName(), "shishen2+cancel");
            if (choice == "shishen2") {
                player->gainMark("@shi", 1);
                room->notifySkillInvoked(player, objectName());
            }
        }
        return false;
    }
};

class Yexing : public AttackRangeSkill
{
public:
    Yexing() : AttackRangeSkill("yexing")
    {

    }

    virtual int getExtra(const Player *target, bool) const
    {
        if (target->hasSkill(objectName()) && target->getMark("@shi") == 0)
            return 1;
        return 0;
    }
};

class YexingEffect : public TriggerSkill
{
public:
    YexingEffect() : TriggerSkill("#yexing")
    {
        frequency = Compulsory;
        events << GameStart << PreMarkChange << CardEffected << SlashEffected << EventAcquireSkill << EventLoseSkill << MarkChanged;
    }


    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {   
        if (triggerEvent == GameStart || (triggerEvent == EventAcquireSkill && data.toString() == "yexing")) {
            if (player && player->hasSkill("yexing") && player->getMark("yexing_limit") == 0) {
                room->setPlayerMark(player, "yexing_limit", 1);
                room->setPlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick", false);
            }
        }
        if (triggerEvent == EventLoseSkill && data.toString() == "yexing") {
            if (!player->hasSkill("yexing") && player->getMark("yexing_limit") > 0) {
                room->setPlayerMark(player, "yexing_limit", 0);
                room->removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0");
            }
        } else if (triggerEvent == MarkChanged) {
            MarkChangeStruct change = data.value<MarkChangeStruct>();
            if (change.name == "@changshi" || change.name == "@pingyi") {
                if (!player->hasSkill("yexing") && player->getMark("yexing_limit") > 0) {
                    room->setPlayerMark(player, "yexing_limit", 0);
                    room->removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0");
                } else if (player->hasSkill("yexing")) {
                    if (player->getMark("@shi") > 0 && player->getMark("yexing_limit") > 0) {
                        room->setPlayerMark(player, "yexing_limit", 0);
                        room->removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0");
                    } else if (player->getMark("@shi") == 0 && player->getMark("yexing_limit") == 0) {
                        room->setPlayerMark(player, "yexing_limit", 1);
                        room->setPlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick", false);
                    }
                }
            }
        }
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == PreMarkChange) {
            MarkChangeStruct change = data.value<MarkChangeStruct>();
            if (change.name != "@shi")
                return QStringList();
            int mark = player->getMark("@shi");

            if (mark > 0 && (mark + change.num == 0) && player->getMark("yexing_limit") == 0) {
                room->setPlayerMark(player, "yexing_limit", 1);
                room->setPlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick", false);
            } else if (mark == 0 && (mark + change.num > 0) && player->getMark("yexing_limit") > 0) {
                room->setPlayerMark(player, "yexing_limit", 0);
                room->removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0");
            }
        }
        if (triggerEvent == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.nature == DamageStruct::Normal && player->getMark("@shi") == 0) 
                return QStringList(objectName());
        }
        if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if ((effect.card->isKindOf("SavageAssault") || effect.card->isKindOf("ArcheryAttack")) && player->getMark("@shi") == 0) 
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return true;
    }
    
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            room->touhouLogmessage("#LingqiAvoid", effect.to, effect.slash->objectName(), QList<ServerPlayer *>(), "yexing");
            room->notifySkillInvoked(player, "yexing");
            room->setEmotion(effect.to, "armor/vine");
            return true;
            
        }
        if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            room->touhouLogmessage("#LingqiAvoid", effect.to, effect.card->objectName(), QList<ServerPlayer *>(), "yexing");
            room->notifySkillInvoked(player, "yexing");
            room->setEmotion(effect.to, "armor/vine");
            return true;
        }

        return false;
    }
};


class YaoshuVS : public ZeroCardViewAsSkill
{
public:
    YaoshuVS() : ZeroCardViewAsSkill("yaoshu")
    {
        response_pattern = "@@yaoshu";
    }

    virtual const Card *viewAs() const
    {
        QString cardname = Self->property("yaoshu_card").toString();
        Card *card = Sanguosha->cloneCard(cardname);

        card->setSkillName("yaoshu");
        return card;
    }
};

class Yaoshu : public TriggerSkill
{
public:
    Yaoshu() : TriggerSkill("yaoshu")
    {
        events << CardFinished;
        view_as_skill = new YaoshuVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card && use.card->isNDTrick() && !use.card->isKindOf("Nullification")) {
            if (use.card->getSkillName() == "yaoshu") {
                room->setPlayerProperty(player, "yaoshu_card", QVariant());
                return QStringList();
            }
            
            if (!use.m_isHandcard)
                return QStringList();
            //for turnbroken
            if (player->hasFlag("Global_ProcessBroken"))
                return QStringList();

            if (player->isCardLimited(Sanguosha->cloneCard(use.card->objectName()), Card::MethodUse))
                return QStringList();
                
            return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        room->setPlayerProperty(player, "yaoshu_card", use.card->objectName());
        room->askForUseCard(player, "@@yaoshu", "@yaoshu:" + use.card->objectName());
        return false;
    }
};




class Jiyi : public TriggerSkill
{
public:
    Jiyi() : TriggerSkill("jiyi")
    {
        events << TurnedOver;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName(), data);
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        player->drawCards(2);
        QList<int> hc = player->handCards();
        room->askForRende(player, hc, objectName(), false, true, qMin(2, player->getHandcardNum()));
		//room->askForYiji(player, hc, objectName(), false, false, true, qMin(2, player->getHandcardNum()));
        
        return false;
    }
};

class Chunmian : public TriggerSkill
{
public:
    Chunmian() : TriggerSkill("chunmian")
    {
        frequency = Compulsory;
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Finish)
            return QStringList(objectName());
        return QStringList();
    }
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {    
        return true;
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        player->turnOver();
        return false;
    }
};



class Baochun : public TriggerSkill
{
public:
    Baochun() : TriggerSkill("baochun")
    {
        events << Damaged;
    }


    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {    

        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@" + objectName() + ":" + QString::number(player->getLostHp()), true, true);

        if (target)
            target->drawCards(player->getLostHp());

        return false;
    }
};


class Chunyi : public TriggerSkill
{
public:
    Chunyi() : TriggerSkill("chunyi")
    {
        frequency = Compulsory;
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Start && player->getMaxHp() < 6)
            return QStringList(objectName());
        return QStringList();
    }
    
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->notifySkillInvoked(player, objectName());

        room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->touhouLogmessage("#GainMaxHp", player, QString::number(1));
        room->touhouLogmessage("#GetHp", player, QString::number(player->getHp()), QList<ServerPlayer *>(), QString::number(player->getMaxHp()));
        return false;
    }
};


class Zhancao : public TriggerSkill
{
public:
    Zhancao() : TriggerSkill("zhancao")
    {
        events << TargetConfirmed;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        
        if (triggerEvent == TargetConfirmed) {
            if (!TriggerSkill::triggerable(player)) return QStringList();
        
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                foreach (ServerPlayer *to, use.to) {
                if (to->isAlive() && (player->inMyAttackRange(to) || player == to))
                    return QStringList(objectName());
                
                }
            }
        }
        return QStringList();
    }

    
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {

        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *to, use.to) {
                if (to->isAlive() && (player->inMyAttackRange(to) || player == to)) {
                    player->tag["zhancao_carduse"] = data;
                    player->tag["zhancao_target"] = QVariant::fromValue(to);
                    QString prompt = "target:" + use.from->objectName() + ":" + to->objectName();
                    if (room->askForSkillInvoke(player, objectName(), prompt)) {
                        use.nullified_list << to->objectName();
                        data = QVariant::fromValue(use);
                        if (room->askForCard(player, ".Equip", "@zhancao-discard", data, objectName()) == NULL)
                            room->loseHp(player);
                        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), to->objectName());
                    }
                    player->tag.remove("zhancao_carduse");
                }
            }
            
        }
        return false;
    }
};



MocaoCard::MocaoCard()
{
    mute = true;
}

bool MocaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;
    return !to_select->getEquips().isEmpty();
}
void MocaoCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    int card_id = room->askForCardChosen(effect.from, effect.to, "e", "mocao");
    room->obtainCard(effect.from, card_id);
    if (effect.to->isWounded())
        effect.to->drawCards(effect.to->getLostHp());
}


class Mocao : public ZeroCardViewAsSkill
{
public:
    Mocao() : ZeroCardViewAsSkill("mocao")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MocaoCard");
    }

    virtual const Card *viewAs() const
    {
        return new MocaoCard;
    }
};



class Shenyin : public TriggerSkill
{
public:
    Shenyin() : TriggerSkill("shenyin")
    {
        events << DamageCaused;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *to = damage.to;
        if (!to->isNude()) 
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QVariant _data = QVariant::fromValue(damage.to);
        player->tag["shenyin_damage"] = data;
        return room->askForSkillInvoke(player, objectName(), _data);
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *to = damage.to;   
        int to_throw = room->askForCardChosen(player, to, "he", objectName());
        player->addToPile("yin_mark", to_throw);
        if (!to->isNude()) {
            int to_throw1 = room->askForCardChosen(player, to, "he", objectName());
            player->addToPile("yin_mark", to_throw1);
        }
        return true;
    }
};

class Xijian : public TriggerSkill
{
public:
    Xijian() : TriggerSkill("xijian")
    {
        events << EventPhaseStart << Damaged;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {  //check target 
        ServerPlayer  *current = room->getCurrent();
        bool can_invoke = false;

        if (triggerEvent == EventPhaseStart && current && current->getPhase() == Player::Finish) {
            if (player != current)
                return TriggerList();
            can_invoke = true;
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.to || !damage.to->isAlive())
                return TriggerList();
            current = damage.to;
            can_invoke = true;
        }
        if (!can_invoke)
            return TriggerList();
        //check skillowner
        QList<ServerPlayer *> yukaris = room->findPlayersBySkillName(objectName());
        if (yukaris.isEmpty()) return TriggerList();
        
        //check pile
        TriggerList skill_list;        
        QList<ServerPlayer *> plist;
        foreach (ServerPlayer *liege, room->getAlivePlayers()) {
            foreach (QString pile, liege->getPileNames()) {
                if (pile != "suoding_cards" && pile != "jiejie_right" &&  pile != "jiejie_left" && pile != "wooden_ox"){
                    if (liege->getPile(pile).length() >0){
                        foreach (ServerPlayer *yukari, yukaris) 
                            skill_list.insert(yukari, QStringList(objectName()));
                        return skill_list;
                    }
                }
            }
        }
        return TriggerList();
    }
    

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *yukari) const
    {
  
        QList<ServerPlayer *> plist;
        foreach (ServerPlayer *liege, room->getAlivePlayers()) {
            int num = 0;

            foreach (QString pile, liege->getPileNames()) {
                if (pile != "suoding_cards" && pile != "jiejie_right" &&  pile != "jiejie_left" && pile != "wooden_ox")
                    num = num + liege->getPile(pile).length();
            }
            if (num > 0)
                plist << liege;
        }
        yukari->tag["xijian_target"] = QVariant::fromValue(player);

        if (plist.length() > 0) {
            ServerPlayer *player_haspile = room->askForPlayerChosen(yukari, plist, "xijian", "@xijian:" + player->objectName(), true, true);
            if (player_haspile) {
                QList<int> idlist;
                foreach (QString pile, player_haspile->getPileNames()) {
                    if (pile != "suoding_cards" && pile != "jiejie_right" && pile != "jiejie_left" && pile != "wooden_ox") {
                        foreach (int id, player_haspile->getPile(pile)) {
                            idlist << id;
                        }
                    }
                }

                room->fillAG(idlist, yukari);
                int card_id = room->askForAG(yukari, idlist, true, objectName());
                room->clearAG(yukari);
                bool can_open = false;
                QString pile_name;
                foreach (QString pile, player_haspile->getPileNames()) {
                    if (player_haspile->getPile(pile).contains(card_id)) {
                        pile_name = pile;
                        break;
                    }
                }
                if (pile_name != NULL && player_haspile->pileOpen(pile_name, yukari->objectName()))
                    can_open = true;
                //need reason to slove the disambugation of the move of piao is
                //xijian a "piao" or using a "piao"
                CardMoveReason reason(CardMoveReason::S_REASON_GOTBACK, player->objectName(), "xijian", "");
                room->obtainCard(player, Sanguosha->getCard(card_id), reason, can_open);
            }
        }
        yukari->tag.remove("xijian_target");
        return false;
    }
};


class Youqu : public TriggerSkill
{
public:
    Youqu() : TriggerSkill("youqu")
    {
        frequency = Compulsory;
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Start) {
            QList<int> sl = player->getPile("siling");
            if (sl.length() >= 2)
                return QStringList(objectName());
        }else if (player->getPhase() == Player::Finish) 
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player->getPhase() == Player::Start) {
            QList<int> sl = player->getPile("siling");
            room->notifySkillInvoked(player, objectName());

            room->touhouLogmessage("#TriggerSkill", player, objectName());

            CardsMoveStruct move(sl, player, player, Player::PlaceSpecial, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_UNKNOWN, QString()));
            room->moveCardsAtomic(move, true);

            room->touhouLogmessage("#silinggain", player, objectName(), QList<ServerPlayer *>(), QString::number(sl.length()));
            room->damage(DamageStruct("siling", player, player));
            
        } else if (player->getPhase() == Player::Finish) {
            room->notifySkillInvoked(player, objectName());
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            QString choice = room->askForChoice(player, objectName(), "siling1+siling2+siling3");
            QList<int> list;
            if (choice == "siling1")
                list = room->getNCards(1);
            if (choice == "siling2")
                list = room->getNCards(2);
            if (choice == "siling3")
                list = room->getNCards(3);
            CardMoveReason reason(CardMoveReason::S_REASON_UNKNOWN, "", NULL, "siling", "");
            player->addToPile("siling", list, false, reason);
        }
        return false;
    
    }

};

class Wangwu : public TriggerSkill
{
public:
    Wangwu() : TriggerSkill("wangwu")
    {
        events << TargetConfirming;
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card == NULL)
            return QStringList();
        if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
            if (!use.card->isRed() && !use.card->isBlack())
                return QStringList();
            if (use.to.contains(player)) {
                QList<int> list = player->getPile("siling");
                if (!list.isEmpty() && use.from->isAlive()) 
                    return QStringList(objectName());
            }        
        }
        return QStringList();
        
    }
    
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
            
        
        player->tag["wangwu_use"] = data;
        QString prompt = "invoke:" + use.from->objectName() + ":" + use.card->objectName();
        return player->askForSkillInvoke(objectName(), prompt);
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<int> same;
        QList<int> disabled;
        QList<int> list = player->getPile("siling");
        foreach (int id, list) {
            if (Sanguosha->getCard(id)->sameColorWith(use.card))
                same << id;
            else
                disabled << id;
        }
        room->fillAG(list, player, disabled);
        player->tag["wangwu_card"] = QVariant::fromValue(use.card);
        if (same.isEmpty()) {
            //give a delay to avoid lacking "siling" shortage
            room->getThread()->delay(1000);
            room->clearAG(player);
        } 
        else {
            int id = room->askForAG(player, same, true, objectName());
            room->clearAG(player);
            if (id > -1) {
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
                room->throwCard(Sanguosha->getCard(id), reason, NULL);
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.from->objectName());

                player->drawCards(1);
                room->damage(DamageStruct("wangwu", player, use.from));
            }
        }
        player->tag.remove("wangwu_card");

        return false;
    }
};


class HpymSiyu : public TriggerSkill
{
public:
    HpymSiyu() : TriggerSkill("hpymsiyu")
    {
        events << PostHpReduced << EventPhaseEnd;
        frequency = Compulsory;
    }

    static void touhou_siyu_clear(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        room->clearAG();

        QVariantList ag_list = room->getTag("AmazingGrace").toList();
        room->removeTag("AmazingGrace");
        if (!ag_list.isEmpty()) {
            DummyCard *dummy = new DummyCard(VariantList2IntList(ag_list));
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), "amazing_grace", QString());
            room->throwCard(dummy, reason, NULL);
            delete dummy;
        }


        foreach (int id, Sanguosha->getRandomCards()) {
            if (room->getCardPlace(id) == Player::PlaceTable)
                room->moveCardTo(Sanguosha->getCard(id), NULL, Player::DiscardPile, true);
            if (Sanguosha->getCard(id)->hasFlag("using"))
                room->setCardFlag(Sanguosha->getCard(id), "-using");
        }

        //check "qinlue"  "quanjie"
        //if current->hasFlag("qinlue")
        //need check cardfinish "yaoshu"  "huisheng"    ?
        ServerPlayer *current = room->getCurrent();
        if (current->getMark("quanjie") > 0) {
            room->setPlayerMark(current, "quanjie", 0);
            room->removePlayerCardLimitation(current, "use", "Slash$1");
        }
        //if (current->hasSkill("yaoshu") )
        //    room->setPlayerProperty(current, "yaoshu_card", QVariant());

        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            p->clearFlags();
            if (p->getMark("@duanzui-extra") > 0)
                p->loseMark("@duanzui-extra");


            QStringList marks;
            marks << "chuangshi" << "xinshang_effect" << "shituDamage" << "shituDeath" << "shitu" << "zheshetransfer";
            marks << "touhou-extra" << "@qianxi_red" << "@qianxi_black" << "sizhai" << "@qingting";
            foreach (QString a, marks) {
                room->setPlayerMark(p, a, 0);
            }
        }
    }


    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == PostHpReduced){
            if (player->getHp() < 1 && !player->isCurrent()) //player->getPhase() != Player::NotActive ||
                return QStringList(objectName());
        }else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Play){
            if (player->getMark("siyuinvoke") > 0  && player->getHp() < 1){
				player->removeMark("siyuinvoke");
                return QStringList(objectName());
			}
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return true;
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        
        if (triggerEvent == PostHpReduced){
            if (!player->faceUp())
                player->turnOver();
                
            QList<const Card *> tricks = player->getJudgingArea();
            foreach (const Card *trick, tricks) {
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName());
                room->throwCard(trick, reason, NULL);
            }

            player->addMark("siyuinvoke");
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
        
            room->touhouLogmessage("#touhouExtraTurn", player, objectName());
            //for skill qinlue
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("qinlue"))
                    p->changePhase(p->getPhase(), Player::NotActive);
            }
            ServerPlayer *current = room->getCurrent();
            if (current)
                current->changePhase(current->getPhase(), Player::NotActive);

            touhou_siyu_clear(player);

            //remain bugs: yaoshu, the second trick card??
            player->gainAnExtraTurn();

            throw TurnBroken;
            return true;
        
        }
		else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Play){
            room->notifySkillInvoked(player, "hpymsiyu");
            room->enterDying(player, NULL);
        }
        return false;
    }
};


class Juhe : public DrawCardsSkill
{
public:
    Juhe() : DrawCardsSkill("juhe")
    {
        frequency = Frequent;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return room->askForSkillInvoke(player, objectName());
    }
    
    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        room->setPlayerFlag(player, "juheUsed");
        return n + 3;  
    }
};

class JuheEffect : public TriggerSkill
{
public:
    JuheEffect() : TriggerSkill("#juhe")
    {
        events << AfterDrawNCards;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->hasFlag("juheUsed") && player->getHp() > 0)
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        player->setFlags("-juheUsed");
        room->askForDiscard(player, "juhe", player->getHp(), player->getHp(), false, false, "juhe_discard:" + QString::number(player->getHp()));
        return false;
    }
};




TH07Package::TH07Package()
    : Package("th07")
{
    General *yuyuko = new General(this, "yuyuko$", "yym", 4, false);
    yuyuko->addSkill(new Sidie);
    yuyuko->addSkill(new Wangxiang);
    related_skills.insertMulti("sidie", "#sidie_clear");

    General *yukari = new General(this, "yukari", "yym", 3, false);
    yukari->addSkill(new Jingjie);
    yukari->addSkill(new Sisheng);
    yukari->addSkill(new Jingdong);

    General *ran = new General(this, "ran", "yym", 3, false);
    ran->addSkill(new Zhaoliao);
    ran->addSkill(new Jiaoxia);

    General *youmu = new General(this, "youmu", "yym", 4, false);
    youmu->addSkill(new Shuangren);
    youmu->addSkill(new Youming);


    General *prismriver = new General(this, "prismriver", "yym", 3, false);
    prismriver->addSkill(new Xiezou);
    prismriver->addSkill(new Hesheng);

    General *alice = new General(this, "alice", "yym", 4, false);
    alice->addSkill(new Renou);
    alice->addSkill(new Zhanzhen);


    General *chen = new General(this, "chen", "yym", 3, false);
    chen->addSkill(new Shishen);
    chen->addSkill(new Yexing);
    chen->addSkill(new YexingEffect);
    chen->addSkill(new Yaoshu);
    related_skills.insertMulti("yexing", "#yexing");


    General *letty = new General(this, "letty", "yym", 4, false);
    letty->addSkill(new Jiyi);
    letty->addSkill(new Chunmian);
    
    General *lilywhite = new General(this, "lilywhite", "yym", 3, false);
    lilywhite->addSkill(new Baochun);
    lilywhite->addSkill(new Chunyi);

    General *shanghai = new General(this, "shanghai", "yym", 3, false);
    shanghai->addSkill(new Zhancao);
    shanghai->addSkill(new Mocao);

    General *yukari_sp = new General(this, "yukari_sp", "yym", 4, false);
    yukari_sp->addSkill(new Shenyin);
    yukari_sp->addSkill(new Xijian);

    General *yuyuko_sp = new General(this, "yuyuko_sp", "yym", 3, false);
    yuyuko_sp->addSkill(new Youqu);
    yuyuko_sp->addSkill(new Wangwu);

    General *youmu_slm = new General(this, "youmu_slm", "yym", 2, false);
    youmu_slm->addSkill(new HpymSiyu);
    youmu_slm->addSkill(new Juhe);
    youmu_slm->addSkill(new JuheEffect);
    related_skills.insertMulti("hpymsiyu", "#hpymsiyu");
    related_skills.insertMulti("juhe", "#juhe");

    addMetaObject<ZhaoliaoCard>();
    addMetaObject<MocaoCard>();
}

ADD_PACKAGE(TH07)

