#include "th09.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"
#include "maneuvering.h"




class Shenpan : public TriggerSkill
{
public:
    Shenpan() : TriggerSkill("shenpan")
    {
        events << EventPhaseStart << EventPhaseEnd;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (player->getPhase() != Player::Draw)
            return QStringList();
        if (triggerEvent == EventPhaseStart) {
            if (!TriggerSkill::triggerable(player)) return QStringList();
            return QStringList(objectName());
        }
        else if (triggerEvent == EventPhaseEnd){
            if (!player->hasFlag(objectName())) return QStringList();
            player->setFlags("-shenpan");
            ServerPlayer *target = player->tag["shenpan"].value<ServerPlayer *>();
            if (target) {
                player->tag.remove("shenpan");
                if (target->getHandcardNum() > target->getHp()) {
                    room->touhouLogmessage("#TouhouBuff", player, objectName());
                    room->drawCards(player, 1);
                }
            }
        }
        return QStringList();
    }
   

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@shenpan-select", true, true);
        if (target) {
            room->damage(DamageStruct(objectName(), player, target, 1, DamageStruct::Thunder));
            player->tag["shenpan"] = QVariant::fromValue(target);
            player->setFlags("shenpan");
            return true;
        }
        return false;
    }
   
   virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return true;
    }
};

class Huiwu : public TriggerSkill
{
public:
    Huiwu() : TriggerSkill("huiwu")
    {
        events << TargetConfirming;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {    
        
        QStringList skill_list;
        if (triggerEvent == TargetConfirming) {
            if (!TriggerSkill::triggerable(player)) return QStringList();
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && (use.card->isKindOf("Slash") || (use.card->isNDTrick()))) {
                if (use.from == NULL || use.from->isDead() || use.from == player)
                    return QStringList();
                return QStringList(objectName());
            }
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.from->tag["huiwu"] = QVariant::fromValue(target);
            room->setTag("huiwu_use", data);
            QString    prompt = "target:" + target->objectName() + ":" + use.card->objectName();
            if (use.from->askForSkillInvoke(objectName(), QVariant::fromValue(target))) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, use.from->objectName(), target->objectName());
            
                room->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(target, objectName());
                LogMessage log;
                log.type = "#InvokeOthersSkill";
                log.from = use.from;
                log.to << target;
                log.arg = objectName();
                room->sendLog(log);
                return true;
            }
            use.from->tag.remove("huiwu");
            return false;
        }
        return  false;
    }
    
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();

            use.nullified_list << player->objectName();
            data = QVariant::fromValue(use);
            player->drawCards(1);
            use.from->tag.remove("huiwu");
            
        }
        return false;
    }
};

class Huazhong : public TriggerSkill
{
public:
    Huazhong() : TriggerSkill("huazhong$")
    {
        events << Damage << FinishJudge;
    }

    virtual QStringList triggerable(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        QStringList skill_list;
        if (event == Damage){
            if (player->isAlive() && player->getKingdom() == "zhan"){
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->hasLordSkill(objectName()))
                        skill_list << p->objectName() + "'" + objectName();
                }
            }
        }  else if (event == FinishJudge){
            JudgeStar judge = data.value<JudgeStar>();
            if (judge->reason == objectName() && judge->isGood()) {
                ServerPlayer *lord = judge->who->tag["huazhong"].value<ServerPlayer *>();
                if (lord != NULL)
                    lord->obtainCard(judge->card);
            }
        }
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &data, ServerPlayer *skill_invoker) const{
        skill_invoker->tag["huazhong-target"] = QVariant::fromValue(skill_target);
        int count = data.value<DamageStruct>().damage;
        for (int i = 0; i < count; i++) {
            if (skill_invoker->askForSkillInvoke(objectName(), QVariant::fromValue(skill_target))) {
                skill_invoker->tag["huazhong"] = QVariant::fromValue(skill_target);
            
                room->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(skill_target, objectName());
                LogMessage log;
                log.type = "#InvokeOthersSkill";
                log.from = skill_invoker;
                log.to << skill_target;
                log.arg = objectName();
                room->sendLog(log);
                
                
                    

                JudgeStruct judge;
                judge.pattern = ".|red";
                judge.who = skill_invoker;
                judge.reason = objectName();
                judge.good = true;

                room->judge(judge);

                skill_invoker->tag.remove("huazhong");
                
            }
        }
        
        
        skill_invoker->tag.remove("huazhong-target");
        return false;
    }
};



class Mingtu : public TriggerSkill
{
public:
    Mingtu() : TriggerSkill("mingtu")
    {
        frequency = Frequent;
        events << EnterDying;
    }
    
    
    
    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {   
        TriggerList skill_list;
        QList<ServerPlayer *> komachis = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *komachi, komachis) 
                skill_list.insert(komachi, QStringList(objectName()));
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &data, ServerPlayer *skill_invoker) const{
        return skill_invoker->askForSkillInvoke(objectName(), data);
    }
    
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &data, ServerPlayer *skill_invoker) const
    {        
        skill_invoker->drawCards(1);
        return false;
    }
};

class Silian : public TriggerSkill
{
public:
    Silian() : TriggerSkill("silian")
    {
        frequency = Compulsory;
        events << DamageCaused;
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {    
        
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user)
            return QStringList();
        if (!damage.from || !damage.to || damage.from == damage.to)
            return QStringList();
        if (damage.card && damage.card->isKindOf("Slash") && damage.to->getHp() == 1) 
            return QStringList(objectName());
        return QStringList();
        
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        
        QList<ServerPlayer *> logto;
        logto << damage.to;
        room->touhouLogmessage("#TriggerSkill", player, "silian", logto);
        room->notifySkillInvoked(player, objectName());
        damage.damage = damage.damage + 2;
        data = QVariant::fromValue(damage);
        return false;
    }
};


class Weiya : public TriggerSkill
{
public:
    Weiya() : TriggerSkill("weiya")
    {
        frequency = Compulsory;
        events << CardUsed  << CardResponded; //<< SlashEffected << CardEffected
    }
    
    

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        ServerPlayer  *current = room->getCurrent();
        if (!current || !current->hasSkill(objectName()) || !current->isAlive())
            return QStringList();
        QString weiya_pattern;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from == current)
                return QStringList();
            if (use.card->isKindOf("Nullification") || use.card->isKindOf("BasicCard")) {
                return QStringList(objectName());
            }
        } else if (triggerEvent == CardResponded) {
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if (player == current || !card_star->isKindOf("BasicCard")
                || data.value<CardResponseStruct>().m_isRetrial
                || data.value<CardResponseStruct>().m_isProvision)
                return QStringList();
            return QStringList(objectName());
        }/* else if (triggerEvent == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("weiyaSkillNullify")) //effect.slash!=NULL &&
                return QStringList(objectName());
        } else if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->hasFlag("weiyaSkillNullify")) //effect.card!=NULL &&
               return QStringList(objectName());
        }  */
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        
        QString weiya_pattern;
        ServerPlayer  *current = room->getCurrent();
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Nullification")) {
                room->notifySkillInvoked(current, objectName());
                room->touhouLogmessage("#weiya_ask", player, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                if (room->askForCard(player, "nullification", "@weiya:nullification", data, Card::MethodDiscard))
                    return false;
                room->touhouLogmessage("#weiya", player, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                room->setPlayerFlag(player, "nullifiationNul");
            } else if (use.card->isKindOf("BasicCard")) {
                //player->setFlags("weiya_ask");
                weiya_pattern = use.card->objectName();
                if (use.card->isKindOf("Slash"))
                    weiya_pattern = "slash";
                room->notifySkillInvoked(current, objectName());
                room->touhouLogmessage("#weiya_ask", player, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                if (room->askForCard(player, weiya_pattern, "@weiya:" + use.card->objectName(), data, Card::MethodDiscard))
                    return false;
                room->touhouLogmessage("#weiya", player, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                //room->setCardFlag(use.card, "weiyaSkillNullify");
                use.nullified_list << "_ALL_TARGETS";
                data = QVariant::fromValue(use);
            }
        }/*  else if (triggerEvent == SlashEffected) {
                return true;
        } else if (triggerEvent == CardEffected) {
                return true; 
        } */else if (triggerEvent == CardResponded) {
            CardStar card_star = data.value<CardResponseStruct>().m_card;
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


class Judu : public TriggerSkill
{
public:
    Judu() : TriggerSkill("judu")
    {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {    
        
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->isAlive() && damage.to != player)
            return QStringList(objectName());
        return QStringList();            
    }
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QVariant _data = QVariant::fromValue(damage.to);
        return player->askForSkillInvoke(objectName(), _data);
    }
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());
        JudgeStruct judge;
        judge.reason = objectName();
        judge.who = player;
        judge.good = true;
        judge.pattern = ".|black|2~9";
        //judge.negative = true;
        room->judge(judge);

        if (judge.isGood()) { 
            room->loseHp(damage.to);
        }

        return false;
    }
};


class HenyiCount : public TriggerSkill
{
public:
    HenyiCount() : TriggerSkill("#henyi-count")
    {
        events << Damaged << EventPhaseChanging;
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == Damaged) { //DamageDone?
            if (!TriggerSkill::triggerable(player)) return QStringList();
            DamageStruct damage = data.value<DamageStruct>();
            if (player == damage.to)
                player->setFlags("henyi");
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from == Player::NotActive) {//Player::Play
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("henyi"))
                        p->setFlags("-henyi");
                }
            }
        }
        return QStringList();
    }
};

class Henyi : public TriggerSkill
{
public:
    Henyi() : TriggerSkill("henyi")
    {
        events << EventPhaseEnd << DamageCaused;
    }
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == DamageCaused) { //need not check weather damage.from has this skill
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->getSkillName() == "henyi" && damage.to->isCurrent()) {
                return QStringList(objectName());
            }
        }else if (triggerEvent == EventPhaseEnd){
            if (player->getPhase() != Player::Play) return QStringList();
            ServerPlayer *src = room->findPlayerBySkillName(objectName());
            //do not consider more than one players?
            if (!src) return QStringList();
            if (src->hasFlag("henyi")) {
                ArcheryAttack *card = new ArcheryAttack(Card::NoSuit, 0);
                if (src->isCardLimited(card, Card::MethodUse))
                    return QStringList();
                bool hasTarget = false;
                foreach (ServerPlayer *p, room->getOtherPlayers(src)) {
                    if (!src->isProhibited(p, card)) {
                        hasTarget = true;
                        break;
                    }
                }
                if (hasTarget)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *skill_invoker) const
    {
        if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *src = room->findPlayerBySkillName(objectName());
            return src->askForSkillInvoke(objectName(), data); 
        }
        return true;
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *skill_invoker) const
    {
        if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *src = room->findPlayerBySkillName(objectName());
            ArcheryAttack *card = new ArcheryAttack(Card::NoSuit, 0);
            card->setSkillName("_henyi");
            CardUseStruct carduse;
            carduse.card = card;
            carduse.from = src;
            room->useCard(carduse);   
            
        } else if (triggerEvent == DamageCaused) { //need not check weather damage.from has this skill
            DamageStruct damage = data.value<DamageStruct>();
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, damage.from->objectName(), damage.to->objectName());
            RecoverStruct recov;
            room->recover(damage.from, recov);
        }
        return false;
    }
};

class Toupai : public PhaseChangeSkill
{
public:
    Toupai() :PhaseChangeSkill("toupai")
    {

    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Draw) {
            QList<ServerPlayer *> players;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (player->canDiscard(p, "h"))
                    players << p;
            }
            if (!players.isEmpty())
                return QStringList(objectName());
        }
        return QStringList();
        
    }
    
    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        
        Room *room = player->getRoom();
        QList<ServerPlayer *> players;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (player->canDiscard(p, "h"))
                players << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(player, players, objectName(), "@toupai-select", true, true);
        if (target) {
            for (int i = 0; i < 3; i++) {
                QList<int>  ids;
                foreach (const Card *c, target->getCards("h")) {
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
        return false;
    }
};


class Zuiyue : public TriggerSkill
{
public:
    Zuiyue() : TriggerSkill("zuiyue")
    {
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() != Player::Play)
            return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("TrickCard") && Analeptic::IsAvailable(player))    
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return room->askForSkillInvoke(player, objectName(), data);
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        
        CardUseStruct ana_use;
        ana_use.from = player;
        Analeptic *card = new Analeptic(Card::NoSuit, 0);
        card->setSkillName(objectName());
        ana_use.card = card;
        room->useCard(ana_use);
        return false;
    }
};

class Doujiu : public TriggerSkill
{
public:
    Doujiu() : TriggerSkill("doujiu")
    {
        events << CardUsed;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {   
        TriggerList skill_list;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Peach") && !use.card->isKindOf("Analeptic"))
                return TriggerList();
            if (!use.to.contains(player) || player->getPhase() != Player::Play)
                return TriggerList();
            ServerPlayer *target = player;
            if (target->isKongcheng() || target->getHp() < 1 || !target->isAlive())
                return TriggerList();
            QList<ServerPlayer *> suikas = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *suika, suikas) {
                if (target != suika)
                    skill_list.insert(suika, QStringList(objectName()));
            }
        }
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *source) const
    {
        if (triggerEvent == CardUsed) {
            QVariant _data = QVariant::fromValue(target);
            source->tag["doujiu_target"] = _data;
            return room->askForSkillInvoke(source, objectName(), _data);
        }
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *source) const
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), target->objectName());
            source->drawCards(1);
            if (!source->isKongcheng() && source->pindian(target, objectName())) {
                if (source->isWounded()) {
                    RecoverStruct recover;
                    recover.recover = 1;
                    room->recover(source, recover);
                }
                use.nullified_list << target->objectName();
                data = QVariant::fromValue(use);
                room->setPlayerFlag(target, "Global_PlayPhaseTerminated");
            }
        } 
        return false;
    }
};


//rewrite Peach::targetFilter
class Yanhui : public TriggerSkill
{
public:
    Yanhui() : TriggerSkill("yanhui$")
    {
        events << PreCardUsed; 
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *p, use.to) {
                if (p->hasLordSkill("yanhui") && p != use.from) {
                    if ((use.card->isKindOf("Analeptic") && p->hasFlag("Global_Dying"))
                        || (use.card->isKindOf("Peach") && use.m_reason == CardUseStruct::CARD_USE_REASON_PLAY)) {
                        QList<ServerPlayer *> logto;
                        logto << p;
                        room->touhouLogmessage("#InvokeOthersSkill", use.from, objectName(), logto);
                        room->notifySkillInvoked(p, objectName());
                    }
                }
            }
        }
        return  TriggerList();
    }
};


class Feixiang : public TriggerSkill
{
public:
    Feixiang() : TriggerSkill("feixiang")
    {
        events << AskForRetrial;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        JudgeStar judge = data.value<JudgeStar>();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (!p->isKongcheng())
                targets << p;
        }
        if (!targets.isEmpty())
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        JudgeStar judge = data.value<JudgeStar>();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (!p->isKongcheng())
                targets << p;
        }
        
        QString prompt = "@feixiang-playerchosen:" + judge->who->objectName() + ":" + judge->reason;
        player->tag["feixiang_judge"] = data;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), prompt, true, true);
        player->tag.remove("feixiang_judge");
        if (target) {
            int card_id = room->askForCardChosen(player, target, "h", objectName());
            Card *card = Sanguosha->getCard(card_id);
            room->showCard(target, card_id);
            if (!target->isCardLimited(card, Card::MethodResponse))
                room->retrial(card, target, judge, objectName());
        }
        return false;
    }
};

class Dizhen : public TriggerSkill
{
public:
    Dizhen() : TriggerSkill("dizhen")
    {
        events << FinishJudge;
    }

    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {   
        JudgeStar judge = data.value<JudgeStar>();
        if (!judge->who || !judge->who->isAlive())
            return TriggerList();

        TriggerList skill_list;
        QList<ServerPlayer *> tensis = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *tensi, tensis) {
            if (judge->card->isRed() && judge->who->getHp() > 0)
                skill_list.insert(tensi, QStringList(objectName()));
        }
        return skill_list;
    }
    
    

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *source) const
    {
        JudgeStar judge = data.value<JudgeStar>();
        source->tag["dizhen_judge"] = data;
        QString prompt = "target:" + judge->who->objectName() + ":" + judge->reason;
        return room->askForSkillInvoke(source, objectName(), prompt);
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *source) const
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), player->objectName());

        room->damage(DamageStruct(objectName(), source, player, 1, DamageStruct::Normal));
        return false;
    }
    
};


TianrenCard::TianrenCard()
{
    mute = true;
    //handling_method = Card::MethodNone;
    //m_skillName = "skltkexuepeach";
}
bool TianrenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}
const Card *TianrenCard::validate(CardUseStruct &cardUse) const
{
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
    foreach (ServerPlayer *liege, lieges) {
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
            room->setCardFlag(slash, "CardProvider_" + liege->objectName());
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

class TianrenVS : public ZeroCardViewAsSkill
{
public:
    TianrenVS() : ZeroCardViewAsSkill("tianren$")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return false;
    }

    static bool hasZhanGenerals(const Player *player)
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->getKingdom() == "zhan")
                return true;
        }
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return hasZhanGenerals(player) && (pattern == "slash")
            && (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            && (!player->hasFlag("Global_tianrenFailed"))
            && !player->isCurrent();
        //(player->getPhase() == Player::NotActive)
    }

    virtual const Card *viewAs() const
    {
        return new TianrenCard;
    }
};

class Tianren : public TriggerSkill
{
public:
    Tianren() : TriggerSkill("tianren$")
    {
        events << CardAsked;
        view_as_skill = new TianrenVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!player->hasLordSkill(objectName()) || player->isCurrent())
             return QStringList();
        QString pattern = data.toStringList().first();
        QString prompt = data.toStringList().at(1);

        bool can = ((pattern == "slash") || (pattern == "jink"));
        if ((!can) || prompt.startsWith("@tianren"))
            return QStringList();
        Card *dummy = Sanguosha->cloneCard(pattern);
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
            if (player->isCardLimited(dummy, Card::MethodResponse))
                return QStringList();
        } else if (player->isCardLimited(dummy, Card::MethodUse))
           return QStringList();

        QList<ServerPlayer *> lieges = room->getLieges("zhan", player);
        if (!lieges.isEmpty())
            return QStringList(objectName());
        return QStringList();
    }
    
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        
        QString pattern = data.toStringList().first();
        //for ai  to add global flag
        //slashsource or jinksource
        if (pattern == "slash")
            room->setTag("tianren_slash", true);
        else
            room->setTag("tianren_slash", false);
        return player->askForSkillInvoke(objectName(), data);
    }
     virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
         QString pattern = data.toStringList().first();
         QList<ServerPlayer *> lieges = room->getLieges("zhan", player);
        QVariant tohelp = QVariant::fromValue((PlayerStar)player);
        foreach (ServerPlayer *liege, lieges) {
            const Card *resp = room->askForCard(liege, pattern, "@tianren-" + pattern + ":" + player->objectName(),
                    tohelp, Card::MethodResponse, player, false, QString(), true);
            if (resp) {
                room->provide(resp, liege);
                return true;
            }
        }
        return false;
    }
};


class Jingdian : public TriggerSkill
{
public:
    Jingdian() : TriggerSkill("jingdian")
    {
        frequency = Compulsory;
        events << DamageInflicted;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature == DamageStruct::Thunder)
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->touhouLogmessage("#jingdian", player, "jingdian", QList<ServerPlayer *>(), QString::number(damage.damage));
        room->notifySkillInvoked(player, objectName());
        for (int i = 0; i < damage.damage; i++)
            room->drawCards(player, 3, objectName());
        return true;
    }
};

class Leiyun : public OneCardViewAsSkill
{
public:
    Leiyun() : OneCardViewAsSkill("leiyun")
    {
        response_or_use = true;
        filter_pattern = ".|spade,heart|.|hand";
    }


    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            Lightning *card = new Lightning(originalCard->getSuit(), originalCard->getNumber());
            card->addSubcard(originalCard);
            card->setSkillName("leiyun");
            return card;
        } else
            return NULL;
    }
};


class Kuaizhao : public DrawCardsSkill
{
public:
    Kuaizhao() : DrawCardsSkill("kuaizhao")
    {
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        QList<ServerPlayer *>  others = room->getOtherPlayers(player);
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->isKongcheng() || !player->inMyAttackRange(p))
                others.removeOne(p);
        }
        if (others.length() > 0)
            return QStringList(objectName());
        return QStringList();
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        QList<ServerPlayer *>  others = room->getOtherPlayers(player);
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->isKongcheng() || !player->inMyAttackRange(p))
                others.removeOne(p);
        }

        ServerPlayer *target = room->askForPlayerChosen(player, others, objectName(), "@kuaizhao-select_one", true, true);
        if (target) {
            int num = 0;
            room->showAllCards(target);
            room->getThread()->delay(1000);
            room->clearAG();
            foreach (const Card *c, target->getCards("h")) {
                if (c->isKindOf("BasicCard"))
                    num++;
            }
            room->setPlayerMark(player, "kuaizhaoUsed", num);
            return n - 1;
        } else
            return n;
    }
};

class KuaizhaoEffect : public TriggerSkill
{
public:
    KuaizhaoEffect() : TriggerSkill("#kuaizhao")
    {
        events << AfterDrawNCards;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (player->getMark("kuaizhaoUsed") > 0) 
             return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        room->drawCards(player, qMin(2, player->getMark("kuaizhaoUsed")));
        room->setPlayerMark(player, "kuaizhaoUsed", 0);
        return false;
    }
};


class Duanjiao : public AttackRangeSkill
{
public:
    Duanjiao() : AttackRangeSkill("duanjiao")
    {

    }

    virtual int getFixed(const Player *target, bool) const
    {
        if (target->hasSkill("duanjiao"))
            return 3;
        else
            return -1;
    }
};


class Nengwu : public TriggerSkill
{
public:
    Nengwu() : TriggerSkill("nengwu")
    {
        events << HpRecover << Damaged << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        bool draw = true;
        if (triggerEvent == Damaged){
            draw = false;
        }
        else if (triggerEvent == CardsMoveOneTime)  {
            if (room->getTag("FirstRound").toBool())
                return QStringList();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && move.from == player && move.from_places.contains(Player::PlaceHand)) {
                draw = false;
            }
            else if (move.to && move.to == player && move.to_place == Player::PlaceHand) {
                draw = true;
            }
            else {
                return QStringList();
            }
        }
        if (player->getPhase() == Player::Draw && draw)
            return QStringList();
        if (player->getPhase() == Player::Play && !draw)
            return QStringList();
        
        
        QList<ServerPlayer *> targets;
        
        foreach (ServerPlayer *p, room->getOtherPlayers(player)){
            if (player->inMyAttackRange(p) && draw){
                targets << p;
            }
            else if (player->inMyAttackRange(p) && !draw && player->canDiscard(p, "h")) {
                targets << p;
            }
        }
        if (!targets.isEmpty())
            return QStringList(objectName());
        return QStringList();    
        
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        bool draw = true;
        if (triggerEvent == Damaged){
            draw = false;
        }
        else if (triggerEvent == CardsMoveOneTime)  {
            if (room->getTag("FirstRound").toBool())
                return false;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && move.from == player && move.from_places.contains(Player::PlaceHand)) {
                draw = false;
            }
            else if (move.to && move.to == player && move.to_place == Player::PlaceHand) {
                draw = true;
            }
        }

        QList<ServerPlayer *> targets;
        
        foreach (ServerPlayer *p, room->getOtherPlayers(player)){
            if (player->inMyAttackRange(p) && draw){
                targets << p;
            }
            else if (player->inMyAttackRange(p) && !draw && player->canDiscard(p, "h")) {
                targets << p;
            }
        }
        
        ServerPlayer *target;
        QString skillname =  "nengwudiscard";
        QString prompt = "@nengwu-discard";    
        if (draw){
            skillname = "nengwudraw";
            prompt = "@nengwu-draw";        
        } 

        
        target = room->askForPlayerChosen(player, targets, skillname, prompt, true, true);
        if (target){
            room->notifySkillInvoked(player, objectName());
            if (draw)
                target->drawCards(1);
            else
                room->throwCard(room->askForCardChosen(player, target, "h", objectName()), target, player);
        }
        return false;
    }
};


class Xiwang : public TriggerSkill
{
public:
    Xiwang() : TriggerSkill("xiwang$")
    {
        events << CardUsed << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        
        ServerPlayer  *current = room->getCurrent();
        if (!current  || current->getKingdom() != "zhan" || current->isDead())
            return QStringList();
        if (!player || player == current)
            return QStringList();
        if (current->isKongcheng())    
            return QStringList();
        
        
        if (triggerEvent == CardUsed) {
            if (data.value<CardUseStruct>().card->getHandlingMethod() != Card::MethodUse)//caution the case using a skillcard
                return QStringList();
            if (data.value<CardUseStruct>().card->getSuit() != Card::Heart)
                return QStringList();
        } else if (triggerEvent == CardResponded) {
            if (data.value<CardResponseStruct>().m_isProvision || data.value<CardResponseStruct>().m_card->getSuit() != Card::Heart)
                return QStringList();
        }
        
        
        QList<ServerPlayer *> lords;
        foreach (ServerPlayer *p, room->getOtherPlayers(current)){
            if (p->hasLordSkill(objectName()))
                return QStringList(objectName());
        }    
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
        ServerPlayer  *current = room->getCurrent();
        foreach (ServerPlayer *lord, room->getOtherPlayers(current)){
            if (current->isKongcheng()) break;
            if (lord->hasLordSkill(objectName())){
                current->tag["xiwang_target"] = QVariant::fromValue(lord);// for ai
                const Card *card = room->askForCard(current, ".|.|.|hand", "@xiwang:" + lord->objectName(), data, Card::MethodNone);
                if (card){
                    room->notifySkillInvoked(lord, objectName());
                    QList<ServerPlayer *> logto;
                    logto << lord;
                    room->touhouLogmessage("#InvokeOthersSkill", current, objectName(), logto);
                    room->obtainCard(lord, card->getEffectiveId(), room->getCardPlace(card->getEffectiveId()) != Player::PlaceHand);
                }     
            }    
        }    
        return false;
    }
};


TH09Package::TH09Package()
    : Package("th09")
{
    General *zhan001 = new General(this, "zhan001$", "zhan", 3, false);
    zhan001->addSkill(new Shenpan);
    zhan001->addSkill(new Huiwu);
    zhan001->addSkill(new Huazhong);

    General *zhan002 = new General(this, "zhan002", "zhan", 4, false);
    zhan002->addSkill(new Silian);
    zhan002->addSkill(new Mingtu);

    General *zhan003 = new General(this, "zhan003", "zhan", 4, false);
    zhan003->addSkill(new Weiya);

    General *zhan004 = new General(this, "zhan004", "zhan", 3, false);
    zhan004->addSkill(new Judu);
    zhan004->addSkill(new Henyi);
    zhan004->addSkill(new HenyiCount);
    related_skills.insertMulti("henyi", "#henyi-count");

    General *zhan005 = new General(this, "zhan005", "zhan", 4, false);
    zhan005->addSkill(new Toupai);

    General *zhan006 = new General(this, "zhan006$", "zhan", 3, false);
    zhan006->addSkill(new Zuiyue);
    zhan006->addSkill(new Doujiu);
    zhan006->addSkill(new Yanhui);

    General *zhan007 = new General(this, "zhan007$", "zhan", 4, false);
    zhan007->addSkill(new Feixiang);
    zhan007->addSkill(new Dizhen);
    zhan007->addSkill(new Tianren);


    General *zhan008 = new General(this, "zhan008", "zhan", 4, false);
    zhan008->addSkill(new Jingdian);
    zhan008->addSkill(new Leiyun);

    General *zhan009 = new General(this, "zhan009", "zhan", 4, false);
    zhan009->addSkill(new Kuaizhao);
    zhan009->addSkill(new KuaizhaoEffect);
    zhan009->addSkill(new Duanjiao);
    related_skills.insertMulti("kuaizhao", "#kuaizhao");

    General *zhan010 = new General(this, "zhan010$", "zhan", 4, false);
    zhan010->addSkill(new Nengwu);
    zhan010->addSkill(new Xiwang);
    
    addMetaObject<TianrenCard>();
}

ADD_PACKAGE(TH09)

