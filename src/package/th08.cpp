#include "th08.h"
#include "th10.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"





class Yongheng : public TriggerSkill
{
public:
    Yongheng() : TriggerSkill("yongheng")
    {
        events << EventPhaseChanging << CardsMoveOneTime << EventAcquireSkill << MarkChanged;
        frequency = Compulsory;
    }


    static void  adjustHandcardNum(ServerPlayer *player, int card_num, QString reason)
    {
        Room *room = player->getRoom();
        int hc_num = player->getHandcardNum();
        if (card_num != hc_num) {
            room->touhouLogmessage("#TriggerSkill", player, "yongheng");
            room->notifySkillInvoked(player, "yongheng");
        }
        if (card_num > hc_num)
            player->drawCards(card_num - hc_num, reason);
        else if (card_num < hc_num)
            room->askForDiscard(player, reason, hc_num - card_num, hc_num - card_num);
    }

	virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
		if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard && player->hasSkill(objectName())) {
                return QStringList(objectName());
            } else if (change.to == Player::NotActive) {
                if (player->hasSkill("yongheng") && player->getHandcardNum() != 4)
                    return QStringList(objectName());
            } else {
                foreach (ServerPlayer *source, room->findPlayersBySkillName("yongheng")) {
                    if (!source->isCurrent() && source->getHandcardNum() != 4)
                        return QStringList(objectName());
                }
            }
        }else if (triggerEvent == CardsMoveOneTime && player->hasSkill(objectName())) {
            if (player->getPhase() == Player::NotActive) {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if ((move.from && move.from == player && move.from_places.contains(Player::PlaceHand))
                    || (move.to && move.to == player && move.to_place == Player::PlaceHand))
					if (player->getHandcardNum() != 4)
						return QStringList(objectName());
            }
        } else if (triggerEvent == EventAcquireSkill &&  data.toString() == "yongheng") {
            if (player->getPhase() == Player::NotActive && player->hasSkill(objectName())){
                if (player->getHandcardNum() != 4)
						return QStringList(objectName());
			}
        } else if (triggerEvent == MarkChanged) {
            MarkChangeStruct change = data.value<MarkChangeStruct>();
            if (change.name != "@pingyi" && change.name != "@changshi")
                return QStringList();
            if (player->getPhase() == Player::NotActive && player->hasSkill(objectName())){
                if (player->getHandcardNum() != 4)
					return QStringList(objectName());
			}
        }
		return QStringList();
	}
	
	virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
		return true;
	}
	
	virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const{
     
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard && player->hasSkill(objectName())) {
                room->touhouLogmessage("#TriggerSkill", player, "yongheng");
                room->notifySkillInvoked(player, objectName());
                player->skip(change.to);
            } else if (change.to == Player::NotActive) {
                    adjustHandcardNum(player, 4, objectName());
            } else {
                foreach (ServerPlayer *source, room->findPlayersBySkillName("yongheng")) {
                        adjustHandcardNum(source, 4, "yongheng");
                }
            }
        } else if (triggerEvent == CardsMoveOneTime && player->hasSkill(objectName())) {
            adjustHandcardNum(player, 4, objectName());
        } else if (triggerEvent == EventAcquireSkill &&  data.toString() == "yongheng") {
                adjustHandcardNum(player, 4, objectName());
        } else if (triggerEvent == MarkChanged) {
                adjustHandcardNum(player, 4, objectName());
        }
        return false;
    }
};

class Zhuqu : public TriggerSkill
{
public:
    Zhuqu() : TriggerSkill("zhuqu$")
    {
        events << FinishJudge;
    }

	virtual QStringList triggerable(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
        JudgeStar judge = data.value<JudgeStar>();
        if (judge->card->getSuit() != Card::Diamond)
            return QStringList();
			
		QStringList skill_list;
		if (player->isAlive() && player->getKingdom() == "yym"){
			foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
					if (p->hasLordSkill(objectName()) && p->isWounded())
						skill_list << p->objectName() + "'" + objectName();
			}
		}
		return skill_list;
	}
	
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *skill_invoker) const
	{
		skill_invoker->tag["zhuqu-target"] = QVariant::fromValue(target);
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
		skill_invoker->tag.remove("zhuqu-target");
		return false;
	}

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *skill_invoker) const
	{
            
        RecoverStruct recov;
        recov.recover = 1;
        recov.who = skill_invoker;
        room->recover(target, recov);
		skill_invoker->tag.remove("zhuqu-target");	
        return false;
    }
};


class Ruizhi : public TriggerSkill
{
public:
    Ruizhi() : TriggerSkill("ruizhi")
    {
        events << PostCardEffected << CardEffected;
    }

	virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
		if (!TriggerSkill::triggerable(player)) return QStringList();
		if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->isNDTrick()) {
                player->tag["ruizhi_effect"] = data;//for ai need damage
                //when triggerEvent  postcardeffected,the effect card information which is transformed willbe cleared.
                //we can not find the real name in cardused,if this card is transformed
                player->setFlags("ruizhi_effect");
            }
        } else if (triggerEvent == PostCardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->isNDTrick() && effect.to == player
                && player->isWounded() && player->hasFlag("ruizhi_effect")) {
				return QStringList(objectName());
			}
		}
		return QStringList();
	}
	
	virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
	{
		CardEffectStruct effect = data.value<CardEffectStruct>();
		player->setFlags("-ruizhi_effect");
        QString prompt = "invoke:" + effect.card->objectName();
        return room->askForSkillInvoke(player, objectName(), prompt);
	}
	
	virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
	{
        if (triggerEvent == PostCardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
 
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
        return false;
    }
};

MiyaoCard::MiyaoCard()
{
    mute = true;
}
bool MiyaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;
    return !to_select->isKongcheng();
}
void MiyaoCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    if (effect.to->canDiscard(effect.to, "h")) {
        const Card *cards = room->askForExchange(effect.to, "miyao", 1, false, "miyao_cardchosen");
        room->throwCard(cards, effect.to);
    }
    if (effect.to->isWounded()) {
        RecoverStruct     recover;
        recover.recover = 1;
        recover.who = effect.from;
        room->recover(effect.to, recover);
    }


}
class Miyao : public ZeroCardViewAsSkill
{
public:
    Miyao() : ZeroCardViewAsSkill("miyao")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MiyaoCard");
    }

    virtual const Card *viewAs() const
    {
        return new MiyaoCard;
    }
};

class Bumie : public TriggerSkill
{
public:
    Bumie() : TriggerSkill("bumie")
    {
        events << DamageInflicted << PreHpLost;
        frequency = Compulsory;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return -100;
    }

	virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
		if (!TriggerSkill::triggerable(player)) return QStringList();
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
        if (will_losehp < original_damage)
			return QStringList(objectName());
        return QStringList();   
	}
	
	
	virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
	{
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
        if (will_losehp < original_damage) {
            room->touhouLogmessage("#bumie01", player, "bumie", QList<ServerPlayer *>(), QString::number(will_losehp));
            room->notifySkillInvoked(player, objectName());
        }
        if (will_losehp <= 0)
            return true;

        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            damage.damage = will_losehp;
            data = QVariant::fromValue(damage);
        } else
            data = QVariant::fromValue(will_losehp);
        return false;
    }
};

class BumieMaxhp : public TriggerSkill
{
public:
    BumieMaxhp() : TriggerSkill("#bumie")
    {
        events << HpChanged << CardsMoveOneTime << EventPhaseChanging << EventAcquireSkill;
        frequency = Compulsory;
    }
	
	
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
		//if (!TriggerSkill::triggerable(player)) return QStringList();
		bool need_judge = false;
        ServerPlayer *src = room->findPlayerBySkillName("bumie");
        if (src == NULL)
            return QStringList();
        if (triggerEvent == HpChanged) {
            if (player == src && player->getHp() == 1 && player->isKongcheng())
                need_judge = true;
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (player == src && player->getHp() == 1 && move.from && move.from == player
                && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard)
                need_judge = true;
        } else if (triggerEvent == EventPhaseChanging) {
            if (src->getHp() == 1 && src->isKongcheng())
                need_judge = true;
        } else if (triggerEvent == EventAcquireSkill && data.toString() == "bumie") {
            if (src->getHp() == 1 && src->isKongcheng())
                need_judge = true;
        }

        if (need_judge) 
			return QStringList(objectName());
		return QStringList();
	}

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
	{
        
        ServerPlayer *src = room->findPlayerBySkillName("bumie");
        
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
            if (src->isWounded()) {
                RecoverStruct recover;
                recover.recover = src->getLostHp();
                room->recover(src, recover);
            }
        }
        return false;
    }
};

class Lizhan : public TriggerSkill
{
public:
    Lizhan() : TriggerSkill("lizhan")
    {
        events << EventPhaseEnd << CardsMoveOneTime << EventPhaseChanging;
    }


	virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *mokou, QVariant &data) const
    {
        if (!mokou || !mokou->isAlive() || !mokou->hasSkill("lizhan") || triggerEvent != CardsMoveOneTime) return;
        ServerPlayer *current = room->getCurrent();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        if (mokou == current)
            return;

        if (current->getPhase() == Player::Discard) {
            QVariantList LizhanToGet = mokou->tag["lizhan"].toList();
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                foreach (int card_id, move.card_ids) {
                    if (move.from == current) {
                        if (!LizhanToGet.contains(card_id) && Sanguosha->getCard(card_id)->isKindOf("Slash"))
                            LizhanToGet << card_id;
                    }
                }
            }

            mokou->tag["lizhan"] = LizhanToGet;
        }

    }
	
	
	virtual TriggerList triggerable(TriggerEvent e, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Discard)
                player->tag.remove("lizhan");
        }
		TriggerList skill_list;
        if (player == NULL || e != EventPhaseEnd || player->getPhase() != Player::Discard) return skill_list;
		QList<ServerPlayer *> mokous = room->findPlayersBySkillName(objectName());
		foreach (ServerPlayer *mokou, mokous) {
            QVariantList ids = mokou->tag["lizhan"].toList();
			if (ids.length() == 0)
                return TriggerList();
				
            QList<int> all;
            foreach (QVariant card_data, ids) {
                if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
                    all << card_data.toInt();
            }
            if (all.length() == 0)
                return TriggerList();
			
			skill_list.insert(mokou, QStringList(objectName()));
		}

		return skill_list;
	}
	
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *source) const
	{
		ServerPlayer *current = room->getCurrent();
		source->tag["lizhan_target"] = QVariant::fromValue(current);
        QString prompt = "target:" + current->objectName();
        
		return room->askForSkillInvoke(source, objectName(), prompt);
		
	}
	
	virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *source) const
	{
        ServerPlayer *current = room->getCurrent();
        
		QString prompt1 = "@lizhan_slash:" + current->objectName();
        QVariantList ids = source->tag["lizhan"].toList();
            
        QList<int> all;
        foreach (QVariant card_data, ids) {
            if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
                all << card_data.toInt();
		}

            
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
            
        return false;
    }
};


KuangzaoCard::KuangzaoCard()
{
    mute = true;
}
bool KuangzaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() == 0 && to_select->inMyAttackRange(Self) && to_select != Self;
}
void KuangzaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QString prompt = "@kuangzao-slash:" + source->objectName();
    const Card *card = room->askForUseSlashTo(targets.first(), source, prompt);
    if (card == NULL)
        room->damage(DamageStruct("kuangzao", NULL, targets.first()));
}


class Kuangzao : public ZeroCardViewAsSkill
{
public:
    Kuangzao() : ZeroCardViewAsSkill("kuangzao")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("KuangzaoCard");
    }

    virtual const Card *viewAs() const
    {
        return new KuangzaoCard;
    }
};


class Huanshi : public TriggerSkill
{
public:
    Huanshi() : TriggerSkill("huanshi")
    {
        events << TargetConfirming;
    }

	 virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
		if (!TriggerSkill::triggerable(player)) return QStringList();
		CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.from != player && use.to.contains(player)) {
            QList<ServerPlayer *> listt;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (use.from->canSlash(p, use.card, true) && !use.to.contains(p)
                    && use.from->inMyAttackRange(p)) //need check attackrange
                    listt << p;
			}
			if (!listt.isEmpty())
				return QStringList(objectName());
		}
		return QStringList();
	}
	
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
	{
        CardUseStruct use = data.value<CardUseStruct>();
        
        QList<ServerPlayer *> listt;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (use.from->canSlash(p, use.card, true) && !use.to.contains(p)
                    && use.from->inMyAttackRange(p)) //need check attackrange
                listt << p;
        } 
        player->tag["huanshi_source"] = data;
            
        ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@huanshi:" + use.from->objectName(), true, true);

            
        if (target) {
            use.to << target;
            room->sortByActionOrder(use.to);
            data = QVariant::fromValue(use);
            room->getThread()->trigger(TargetConfirming, room, target, data);
        }
        return false;
    }
};

class Shishi : public TriggerSkill
{
public:
    Shishi() : TriggerSkill("shishi")
    {
        events << CardUsed << SlashEffected << CardEffected;
    }

	virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {  
	    TriggerList skill_list;
		if (triggerEvent == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("shishiSkillNullify")) {
                skill_list.insert(effect.to, QStringList(objectName()));
            }
        } else if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->hasFlag("shishiSkillNullify")) {
                skill_list.insert(effect.to, QStringList(objectName()));
            }
        }else if (triggerEvent == CardUsed) {
			
			CardUseStruct use = data.value<CardUseStruct>();
			if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
				QList<ServerPlayer *> keines = room->findPlayersBySkillName(objectName());
				foreach (ServerPlayer *keine, keines) {
					if (use.from != keine && keine->getPile("lishi").length() == 0)
						skill_list.insert(keine, QStringList(objectName()));
				}
			}
			
		}
		return skill_list;
		
	}
    
	virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *src) const
	{
		if (triggerEvent == CardUsed) {
			CardUseStruct use = data.value<CardUseStruct>();
			QString    prompt = "use:" + use.from->objectName() + ":" + use.card->objectName();
			src->tag["shishi_use"] = data;
			return room->askForSkillInvoke(src, objectName(), prompt);
		}
		return true;
	}
	
	virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *src) const
	{
        if (triggerEvent == CardUsed) {
            
            CardUseStruct use = data.value<CardUseStruct>();
              
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, src->objectName(), use.from->objectName());

            if (use.card && room->getCardPlace(use.card->getEffectiveId()) == Player::PlaceTable)
                src->addToPile("lishi", use.card, true);
            //addtopile will clear cardflag(especially  use.from is robot ai )

            if (use.card->isKindOf("Nullification")) {
                room->touhouLogmessage("#weiya", use.from, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                room->setPlayerFlag(use.from, "nullifiationNul");
            } else
                room->setCardFlag(use.card, "shishiSkillNullify");
            
        } else if (triggerEvent == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            
            room->touhouLogmessage("#LingqiAvoid", effect.to, effect.slash->objectName(), QList<ServerPlayer *>(), objectName());
            room->setEmotion(effect.to, "skill_nullify");
            return true;
            
        } else if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            room->touhouLogmessage("#LingqiAvoid", effect.to, effect.card->objectName(), QList<ServerPlayer *>(), objectName());
            room->setEmotion(effect.to, "skill_nullify");
            return true;
        }
        return false;
    }
};

class Shouye : public TriggerSkill
{
public:
    Shouye() : TriggerSkill("shouye")
    {
        events << EventPhaseStart << Damaged;
    }

	virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
		if (!TriggerSkill::triggerable(player)) return QStringList();
		if ((triggerEvent == EventPhaseStart && player->getPhase() == Player::Start)
            || (triggerEvent == Damaged)){
			QList<int> pile = player->getPile("lishi");
			if (pile.length() > 0) 
				return QStringList(objectName());
		}
		return QStringList();
	}
	
	
	virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
	{

        QList<int> pile = player->getPile("lishi");
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@" + objectName(), true, true);
        DummyCard *dummy = new DummyCard(pile);
        if (target)
            room->obtainCard(target, dummy);
        return false;
    }
};



BuxianCard::BuxianCard()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}
bool BuxianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < 2 && !to_select->isKongcheng() && to_select != Self;
}
bool BuxianCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    return targets.length() == 2;
}
void BuxianCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->moveCardTo(Sanguosha->getCard(subcards.first()), NULL, Player::DrawPile);
    targets.first()->pindian(targets.last(), "buxian");
}


class BuxianVS : public OneCardViewAsSkill
{
public:
    BuxianVS() : OneCardViewAsSkill("buxian")
    {
        filter_pattern = ".|.|.|hand";
        response_pattern = "@@buxian";
    }


    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard) {
            BuxianCard *card = new BuxianCard;
            card->addSubcard(originalCard);
            return card;
        }
        return NULL;
    }
};
class Buxian : public TriggerSkill
{
public:
    Buxian() : TriggerSkill("buxian")
    {
        events << Damaged << EventPhaseStart;
        view_as_skill = new BuxianVS;
    }

	 virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
		if (!TriggerSkill::triggerable(player)) return QStringList();
		if (triggerEvent == EventPhaseStart && player->getPhase() != Player::Play)
            return QStringList();
        if (player->isKongcheng())
			return QStringList();
		return QStringList(objectName());
	}
	
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
	{
        room->askForUseCard(player, "@@buxian", "@buxian");
        return false;
    }
};
class BuxianEffect : public TriggerSkill
{
public:
    BuxianEffect() : TriggerSkill("#buxian")
    {
        events << Pindian;
    }

     virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
		PindianStar pindian = data.value<PindianStar>();
        if (pindian->reason != "buxian")
            return QStringList();
		return QStringList(objectName());
	}

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
	{
        PindianStar pindian = data.value<PindianStar>();
        ServerPlayer *bigger = NULL;
        if (pindian->from_number > pindian->to_number)
            bigger = pindian->from;
        else if (pindian->to_number > pindian->from_number)
            bigger = pindian->to;
        if (bigger != NULL && (bigger == pindian->to || bigger == pindian->from)) {
            bigger->drawCards(1);
            room->damage(DamageStruct("buxian", NULL, bigger));
        }
        return false;
    }
};

class Xingyun : public TriggerSkill
{
public:
    Xingyun() : TriggerSkill("xingyun")
    {
        events << CardsMoveOneTime;
    }

	
	 virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
		if (!TriggerSkill::triggerable(player)) return QStringList();
		if (room->getTag("FirstRound").toBool())
            return QStringList();
		CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        if (move.to != NULL && move.to == player && move.to_place == Player::PlaceHand) {
            foreach (int id, move.card_ids) {
                if (Sanguosha->getCard(id)->getSuit() == Card::Heart)
					return QStringList(objectName());
			}
		}
		return QStringList();
	}
	
	virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
	{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        
        foreach (int id, move.card_ids) {
            if (Sanguosha->getCard(id)->getSuit() == Card::Heart &&
                    player->askForSkillInvoke(objectName(), data)) {
                QString choice = "letdraw";
                room->showCard(player, id);
                if (player->isWounded())
                        choice = room->askForChoice(player, objectName(), "letdraw+recover", data);
                if (choice == "letdraw") {
                    ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@xingyun-select");
                    target->drawCards(1);
                } else if (choice == "recover") {
                    RecoverStruct recover;
                    recover.who = player;
                    room->recover(player, recover);
                }
            }

        }
        
        return false;
    }
};



GeshengCard::GeshengCard()
{
    handling_method = Card::MethodNone;
}


bool GeshengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty()) return false;
    if (to_select->getPhase() != Player::Judge) return false;
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


const Card *GeshengCard::validate(CardUseStruct &cardUse) const
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




class GeshengVS : public OneCardViewAsSkill
{
public:
    GeshengVS() : OneCardViewAsSkill("gesheng")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (pattern == "@@gesheng")
            return true;
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard) {
            //Indulgence *indl = new Indulgence(originalCard->getSuit(), originalCard->getNumber());
            GeshengCard *indl = new GeshengCard();
            indl->addSubcard(originalCard);
            indl->setSkillName(objectName());
            return indl;
        }
        return NULL;
    }
};

class Gesheng : public TriggerSkill
{
public:
    Gesheng() : TriggerSkill("gesheng")
    {
        events << EventPhaseChanging << EventPhaseStart << EventPhaseEnd;
        view_as_skill = new GeshengVS;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {  
	    TriggerList skill_list;
		if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Judge) {
			foreach (ServerPlayer *p, room->getAlivePlayers()) {
                foreach (const Card *c, p->getCards("j")) {
                    if (c->isKindOf("Indulgence") && c->getSuit() != Card::Diamond)
                        return TriggerList();
                }
            }

			QList<ServerPlayer *> srcs = room->findPlayersBySkillName(objectName());
			foreach (ServerPlayer *p, srcs)
				skill_list.insert(p, QStringList(objectName()));
			return skill_list;
		}else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Judge) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("gesheng"))
                        room->setPlayerFlag(p, "-gesheng");
                }
            }
        }else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Judge) {
			foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasFlag("gesheng")) {
                    skill_list.insert(p, QStringList(objectName()));
                }
            }
			return skill_list;
		}
		return TriggerList();
		
	}

	virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *src) const
	{
		if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Judge) {
			return room->askForSkillInvoke(src, objectName(), QVariant::fromValue(player));
		}
		return true;
	}
	
	virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *src) const
	{

        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Judge) {
            room->setPlayerFlag(src, "gesheng");
            src->drawCards(1);
        } else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Judge) {
            bool canuse = true;
            if (player->isDead())
                canuse = false;
            Indulgence *indl = new Indulgence(Card::NoSuit, 0);
            indl->deleteLater();
            if (src->isCardLimited(indl, Card::MethodUse, true))
                canuse = false;
            if (player->containsTrick("indulgence") || src->isProhibited(player, indl))
                canuse = false;
            if (canuse) {
                QString prompt = "@gesheng:" + player->objectName();
                const Card *card = room->askForUseCard(src, "@@gesheng", prompt);
                if (!card)
                    room->loseHp(src, 2);
            } else
                room->loseHp(src, 2);
        } 
        return false;
    }
};

class Yemang : public ProhibitSkill
{
public:
    Yemang() : ProhibitSkill("yemang")
    {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        if (card->isKindOf("Slash")) {
            bool offensive_horse = false;
            foreach (int id, card->getSubcards()) {
                if (from->getOffensiveHorse() && from->getOffensiveHorse()->getId() == id) {
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


class Yinghuo : public TriggerSkill
{
public:
    Yinghuo() : TriggerSkill("yinghuo")
    {
        events << CardAsked;
    }

	 virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
		if (!TriggerSkill::triggerable(player)) return QStringList();
		QString pattern = data.toStringList().first();
        if (pattern == "jink") {
            Jink *jink = new Jink(Card::NoSuit, 0);
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
        const Card *card = room->askForCard(player, "Jink", "@yinghuo", data, Card::MethodNone, NULL, false, objectName());
        if (card) {
            room->notifySkillInvoked(player, objectName());
            room->showCard(player, card->getId());
            room->getThread()->delay();
			Jink *jink = new Jink(Card::NoSuit, 0);
            jink->setSkillName("_yinghuo");
            room->provide(jink);
        }
        return false;
    }
};

class Chongqun : public TriggerSkill
{
public:
    Chongqun() : TriggerSkill("chongqun")
    {
        events << CardResponded << CardUsed;
    }

	 virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
		if (!TriggerSkill::triggerable(player)) return QStringList();
		bool can = false;
        if (triggerEvent == CardResponded) {
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isKindOf("BasicCard"))
                can = true;
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("BasicCard"))
                can = true;
        }
        if (can) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!p->isNude() && p->canDiscard(p, "he"))
                    targets << p;
            }
            if (!targets.isEmpty())
                return QStringList(objectName());	
		}
		return QStringList();
	}
	
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
	{
      
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (!p->isNude() && p->canDiscard(p, "he"))
                targets << p;
        }
            
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@chongqun_target", true, true);
        if (target)
        room->askForDiscard(target, "chongqun", 1, 1, false, true, "chongqun_discard:" + player->objectName());
        return false;
    }
};



//for using dialog while responsing, viewas Skill should be the main skill.
class ChuangshiVS : public ZeroCardViewAsSkill
{
public:
    ChuangshiVS() : ZeroCardViewAsSkill("chuangshi")
    {
        response_pattern = "@@chuangshi";
    }

    virtual const Card *viewAs() const
    {

        CardStar c = Self->tag.value("chuangshi").value<CardStar>();
        //we need get the real subcard.
        if (c) {
            ChuangshiCard *card = new ChuangshiCard;
            card->setUserString(c->objectName());
            return card;
        } else {
            return NULL;
        }
    }

    virtual QDialog *getDialog() const
    {
        return QijiDialog::getInstance("chuangshi");
    }

};



class Chuangshi : public TriggerSkill
{
public:
    Chuangshi() : TriggerSkill("#chuangshi")
    {
        events << EventPhaseStart << DrawNCards << EventPhaseChanging;
        view_as_skill = new ChuangshiVS;
    }

    static bool use_chuangshi(Room *room, ServerPlayer *player)
    {

        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), "chuangshi", "@chuangshi_target", true, true);
        if (target != NULL) {
            target->gainMark("chuangshi_user");//need use gainMark to notify the client player.

            room->setPlayerProperty(player, "chuangshi_user", target->objectName());
            const Card *card = room->askForUseCard(player, "@@chuangshi", "@chuangshi_prompt:" + target->objectName());
            return card != NULL;
        }
        return false;
    }
    static const Player *getChuangshiUser(const Player *player)
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->getMark("chuangshi_user") > 0)
                return p;
            //if (p->objectName()==player->property("chuangshi_user").toString())
            //    return p;
        }
        return NULL;
    }

    static ServerPlayer *getChuangshiUser1(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getMark("chuangshi_user") > 0)
                return p;
            //if p->objectName()==player->property("chuangshi_user").toString();
            //    return p;
        }
        return NULL;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
		if (!TriggerSkill::triggerable(player)) return QStringList();
		if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Draw) {
                return QStringList(objectName());
            }
        } else if (triggerEvent == DrawNCards) {
            if (player->hasFlag("chuangshi")) {
                data = QVariant::fromValue(data.toInt() - 1);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            if (player->hasFlag("chuangshi"))
                player->setFlags("-chuangshi");
        }
		return QStringList();
	}
	
	
	virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
	{
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Draw) {
                if (use_chuangshi(room, player))
                    player->setFlags("chuangshi");
            }
        }
        return false;
    }
};

ChuangshiCard::ChuangshiCard()
{
    mute = true;
    will_throw = false;
}
bool ChuangshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Player *user = Chuangshi::getChuangshiUser(Self);
    const Card *card = Self->tag.value("chuangshi").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName());
    new_card->setSkillName("chuangshi");

    if (new_card->targetFixed())
        return false;
    if (new_card->isKindOf("FireAttack"))
        return new_card && (new_card->targetFilter(targets, to_select, user) || (to_select == user && !user->isKongcheng()))
        && !user->isProhibited(to_select, new_card, targets);
    else
        return new_card && new_card->targetFilter(targets, to_select, user) && !user->isProhibited(to_select, new_card, targets);
}

bool ChuangshiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Player *user = Chuangshi::getChuangshiUser(Self);
    const Card *card = Self->tag.value("chuangshi").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName());
    new_card->setSkillName("chuangshi");
    if (card->isKindOf("IronChain") && targets.length() == 0)
        return false;
    return new_card && new_card->targetsFeasible(targets, user);
}

void ChuangshiCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *from = card_use.from;
    ServerPlayer *to1 = card_use.to.at(0);
    ServerPlayer *to2 = card_use.to.at(1);
    QList<ServerPlayer *>logto;
    logto << to1 << to2;

    Card *card = Sanguosha->cloneCard(user_string);
    if (card->isKindOf("Collateral")) {
        ServerPlayer *chuangshi_user = Chuangshi::getChuangshiUser1(from);
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
void ChuangshiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    //const Card *card = Self->tag.value("chuangshi").value<const Card *>();
    Card *card = Sanguosha->cloneCard(user_string);
    if (card->isKindOf("Collateral"))
        return;
    ServerPlayer *user = Chuangshi::getChuangshiUser1(source);
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



class Wangyue : public MasochismSkill
{
public:
    Wangyue() : MasochismSkill("wangyue")
    {
    }

	virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
		if (!TriggerSkill::triggerable(player)) return QStringList();
		DamageStruct damage = data.value<DamageStruct>();
		if (damage.from && damage.from->isAlive()
            && damage.from->getHandcardNum() > player->getHp()) {
			return QStringList(objectName());
		}
		return QStringList();
	}
	
	virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
	{
		DamageStruct damage = data.value<DamageStruct>();
		player->tag["wangyue_target"] = QVariant::fromValue(damage.from);
        QString prompt = "target:" + damage.from->objectName() + "::" + QString::number(player->getHp());
		return room->askForSkillInvoke(player, objectName(), prompt);
	}
	
    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const
    {
        Room *room = player->getRoom();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());
        int x = damage.from->getHandcardNum() - player->getHp();
        room->askForDiscard(damage.from, "wangyue", x, x, false, false);
    }
};


HuweiCard::HuweiCard()
{
    mute = true;
    target_fixed = true;
}
const Card *HuweiCard::validate(CardUseStruct &cardUse) const
{
    Room *room = cardUse.from->getRoom();
    room->touhouLogmessage("#InvokeSkill", cardUse.from, "huwei");
    room->notifySkillInvoked(cardUse.from, "huwei");
    cardUse.from->drawCards(2);
    room->setPlayerFlag(cardUse.from, "Global_huweiFailed");
    return NULL;
}


class HuweiVS : public ZeroCardViewAsSkill
{
public:
    HuweiVS() : ZeroCardViewAsSkill("huwei")
    {
    }

    virtual const Card *viewAs() const
    {
        return new HuweiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return false;
    }
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        Slash *tmpslash = new Slash(Card::NoSuit, 0);
        tmpslash->deleteLater();
        if (player->isCardLimited(tmpslash, Card::MethodUse))
            return false;
        return (!player->hasFlag("Global_huweiFailed") && pattern == "slash" && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE);
    }
};
class Huwei : public TriggerSkill
{
public:
    Huwei() : TriggerSkill("huwei")
    {
        events << CardAsked << CardUsed << CardsMoveOneTime  << CardResponded; //<< BeforeCardsMove
        view_as_skill = new HuweiVS;
    }
    static bool targetChoiceForHuwei(Room *room, ServerPlayer *player, QString skillname, int num)
    {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), skillname, "@huwei_targetdraw:" + QString::number(num), true, true);
        if (target) {
            target->drawCards(2);
            return true;
        } else
            return false;
    }
    
	virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
	{
		if (!TriggerSkill::triggerable(player)) return QStringList();
		if (player->getPhase() == Player::Play)
            return QStringList();
		if (triggerEvent == CardAsked) {
            QString pattern = data.toStringList().first();
            if (pattern == "slash") {
                Slash *tmpslash = new Slash(Card::NoSuit, 0);
                if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
                    if (player->isCardLimited(tmpslash, Card::MethodResponse))
                        return QStringList();
                }
				return QStringList(objectName());
            }
        }else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash"))
				return QStringList(objectName());
		}else if (triggerEvent == CardResponded) {
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isKindOf("Slash"))
				return QStringList(objectName());
		}else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && move.to_place == Player::DiscardPile && move.from == player) {
                if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                    foreach (int id, move.card_ids) {
                        Card *card = Sanguosha->getCard(id);
						if (card->isKindOf("Slash"))
							return QStringList(objectName());
                    }
                }
            }
        }
		return QStringList();
	}
	
	virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
	{
		if (triggerEvent == CardAsked) {
			return room->askForSkillInvoke(player, objectName(), data);
		}
		return true;
	}
	
	virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
	{
        
        if (triggerEvent == CardAsked) {
            player->drawCards(2);
        } else if (triggerEvent == CardUsed) {
            targetChoiceForHuwei(room, player, objectName(), 1);
        } else if (triggerEvent == CardResponded) {
			targetChoiceForHuwei(room, player, objectName(), 1);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            int n = 0;
			foreach (int id, move.card_ids) {
                Card *card = Sanguosha->getCard(id);
                if (card->isKindOf("Slash"))
                    n++;		
            }
			while (n > 0) {
				if (targetChoiceForHuwei(room, player, objectName(), n))
					n--;
				else
					break;
			}
        }
        return false;
    }
};


JinxiCard::JinxiCard()
{
    mute = true;
    target_fixed = true;
}
void JinxiCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->doLightbox("$jinxiAnimate", 4000);
    SkillCard::onUse(room, card_use);
}
void JinxiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->removePlayerMark(source, "@jinxi");
    RecoverStruct recov;
    recov.recover = source->getMaxHp() - source->getHp();
    recov.who = source;
    room->recover(source, recov);
    if (source->getHandcardNum() < 4)
        source->drawCards(4 - source->getHandcardNum());
}
class Jinxi : public ZeroCardViewAsSkill
{
public:
    Jinxi() : ZeroCardViewAsSkill("jinxi")
    {
        frequency = Limited;
        limit_mark = "@jinxi";
    }

    virtual const Card *viewAs() const
    {
        return new JinxiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@jinxi") >= 1 && player->isWounded();
    }
};


TH08Package::TH08Package()
    : Package("th08")
{
    General *yyc001 = new General(this, "yyc001$", "yyc", 4, false);
    yyc001->addSkill(new Yongheng);
    yyc001->addSkill(new Zhuqu);


    General *yyc002 = new General(this, "yyc002", "yyc", 4, false);
    yyc002->addSkill(new Ruizhi);
    yyc002->addSkill(new Miyao);

    General *yyc003 = new General(this, "yyc003", "yyc", 4, false);
    yyc003->addSkill(new Bumie);
    yyc003->addSkill(new BumieMaxhp);
    yyc003->addSkill(new Lizhan);
    related_skills.insertMulti("bumie", "#bumie");


    General *yyc004 = new General(this, "yyc004", "yyc", 4, false);
    yyc004->addSkill(new Kuangzao);
    yyc004->addSkill(new Huanshi);

    General *yyc005 = new General(this, "yyc005", "yyc", 3, false);
    yyc005->addSkill(new Shishi);
    yyc005->addSkill(new Shouye);

    General *yyc006 = new General(this, "yyc006", "yyc", 3, false);
    yyc006->addSkill(new Buxian);
    yyc006->addSkill(new BuxianEffect);
    yyc006->addSkill(new Xingyun);
    related_skills.insertMulti("buxian", "#buxian");

    General *yyc007 = new General(this, "yyc007", "yyc", 3, false);
    yyc007->addSkill(new Gesheng);
    yyc007->addSkill(new Yemang);

    General *yyc008 = new General(this, "yyc008", "yyc", 3, false);
    yyc008->addSkill(new Yinghuo);
    yyc008->addSkill(new Chongqun);

    General *yyc009 = new General(this, "yyc009", "yyc", 3, false);
    yyc009->addSkill(new ChuangshiVS);//for using dialog while responsing, viewas Skill should be the main skill.
    yyc009->addSkill(new Chuangshi);
    yyc009->addSkill(new Wangyue);
    related_skills.insertMulti("chuangshi", "#chuangshi");

    General *yyc010 = new General(this, "yyc010", "yyc", 4, false);
    yyc010->addSkill(new Huwei);
    yyc010->addSkill(new Jinxi);


    addMetaObject<MiyaoCard>();
    addMetaObject<KuangzaoCard>();
    addMetaObject<BuxianCard>();
    addMetaObject<GeshengCard>();
    addMetaObject<ChuangshiCard>();
    addMetaObject<HuweiCard>();
    addMetaObject<JinxiCard>();
}

ADD_PACKAGE(TH08)

