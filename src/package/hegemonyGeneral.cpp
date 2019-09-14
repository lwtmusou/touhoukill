#include "hegemonyGeneral.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"
//#include "th10.h"
//#include "th08.h"

class YonghengHegemony : public TriggerSkill
{
public:
    YonghengHegemony()
        : TriggerSkill("yongheng_hegemony")
    {
        events << EventPhaseChanging << CardsMoveOneTime;
        frequency = Compulsory;
    }

    static void adjustHandcardNum(ServerPlayer *player, QString reason)
    {
        int card_num = qMax(player->getHp(), 1);
        Room *room = player->getRoom();
        int hc_num = player->getHandcardNum();
        if (card_num != hc_num) {
            room->touhouLogmessage("#TriggerSkill", player, reason);
            room->notifySkillInvoked(player, reason);
        }
        if (card_num > hc_num)
            player->drawCards(card_num - hc_num, reason);
        else if (card_num < hc_num)
            room->askForDiscard(player, reason, hc_num - card_num, hc_num - card_num);
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        
		if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard && change.player->hasSkill(objectName()))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, NULL, true);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QList<ServerPlayer *> kaguyas;
            ServerPlayer *kaguya1 = qobject_cast<ServerPlayer *>(move.from);
            ServerPlayer *kaguya2 = qobject_cast<ServerPlayer *>(move.to);

            if (kaguya1 && kaguya1->isAlive() && kaguya1->hasSkill(this) && move.from_places.contains(Player::PlaceHand) 
                && kaguya1->getHandcardNum() != qMax(kaguya1->getHp(), 1)
                && kaguya1->getPhase() == Player::NotActive)
                kaguyas << kaguya1;
            if (kaguya2 && kaguya2->isAlive() && kaguya2->hasSkill(this) && move.to_place == Player::PlaceHand 
                && kaguya2->getHandcardNum() != qMax(kaguya2->getHp(), 1)
                && kaguya2->getPhase() == Player::NotActive)
                kaguyas << kaguya2;
            if (kaguyas.length() > 1)
                std::sort(kaguyas.begin(), kaguyas.end(), ServerPlayer::CompareByActionOrder);
            if (!kaguyas.isEmpty()) {
                QList<SkillInvokeDetail> d;
                foreach (ServerPlayer *p, kaguyas)
                    d << SkillInvokeDetail(this, p, p, NULL, true);
                return d;
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard) {
                room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
                room->notifySkillInvoked(invoke->invoker, objectName());
                invoke->invoker->skip(change.to);
                adjustHandcardNum(invoke->invoker, objectName());
            }
        } else
            adjustHandcardNum(invoke->invoker, objectName());
        return false;
    }
};

/*
class Tianbian : public TriggerSkill
{
public:
	Tianbian()
		: TriggerSkill("tianbian")
	{
		events << EventPhaseProceeding;
		frequency = Enteral;
	}


	QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
	{
		ServerPlayer *current = data.value<ServerPlayer *>();
		if (current == NULL || current->isDead())
			return QList<SkillInvokeDetail>();
		if (current->getPhase() == Player::Judge) {
			QList<const Card *> tricks = current->getJudgingArea();
			if (tricks.isEmpty())
				return QList<SkillInvokeDetail>();
			QList<ServerPlayer *> tenshis = room->findPlayersBySkillName(objectName());
			QList<SkillInvokeDetail> d;
			foreach(ServerPlayer *tenshi, tenshis)
				d << SkillInvokeDetail(this, tenshi, tenshi, NULL, true);
			return d;
		}
		return QList<SkillInvokeDetail>();
	}

	bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
	{
		ServerPlayer *current = data.value<ServerPlayer *>();
		int maxchoice = qMax(current->getJudgingAreaID().length(), 2);
		//step 1 choose card which will not be used
		QList<int> disable;
		for (int i = 0; i < maxchoice; i += 1) {
			int card_id = room->askForCardChosen(invoke->invoker, current, "j", objectName(), visible, Card::MethodDiscard, disable); //refuasable??
			if (card_id <= -1)
				break;
			disable << card_id;

		}
		//step 2 use card which is avaliable
		QList<int> tricks = current->getJudgingAreaID();
		QList<int> effected;
		while (!tricks.isEmpty() && current->isAlive()) {
			tricks = current->getJudgingAreaID();
			//foreach(int id, effected) {
			//	if (tricks.contains(id))
			//		tricks.removeOne(id);
			//}

			QList<int> use_ids, disable_use_ids;
			foreach(int id, tricks) {
				const Card *c = Sanguosha->getEngineCard(id) //deletelater?
					c->setFlags("tianbian");
					c->setFlags("IgnoreFailed");
					bool can = !disable.contains(id) && usecheck (invoke->invoker, current, c);
					c->setFlags("-IgnoreFailed");
					c->setFlags("-tianbian");
					if (can)
						use_ids << id; 
					else
						disable_use_ids << id;
				  
			}
			if (use_ids.length() == 0)
				break;
			int card_id = room->askForCardChosen(invoke->invoker, current, "j", objectName(), visible, Card::MethodDiscard, disable_use_ids);
			const Card *c = Sanguosha->getEngineCard(card_id)
			//real delaycard??
			room->useCard(CardUseStruct(c, invoke->invoker, current));
			effected << card_id;
		}
		//step 3 throw all card, and effect delay
		if 	(current->isAlive() &&  !current->getJudgingAreaID().isEmpty()){
			int num = (current->getJudgingAreaID().length() / 2;
			DummyCard *dummy = new DummyCard;
			dummy->addSubcards(current->getJudgingAreaID());
			room->throwCard(dummy, current, invoke->invoker);
			QStringList names;
        names << "lightning"
              << "indulgence"
              << "supply_shortage"
              << "saving_energy"
              << "spring_breath";
			
			//if (num > 0){
				for (int i = 0; i < num; i += 1) {
					QString choice = room->askForChoice(invoke->invoker, "qizhi", names.join("+"), QVariant());
					//if (exclude == "lightning") {
						Lightning *c = new Lightning(Card::NoSuit, 0);
						 bool on_effect = room->cardEffect(c, NULL, player);  // iscanceled  ->  DelayedTrick::onEffect
						//effected << trick;
						//if (!on_effect)
						//	trick->onNullified(player); just for move delaytrick card
						
						JudgeStruct judge_struct = c->getJudge();
						judge_struct.who = current;
						room->judge(judge_struct);

						if (judge_struct.negative == judge_struct.isBad()) {
							if (judge_struct.who->isAlive() && !judge_struct.ignore_judge)
								c->takeEffect(judge_struct.who);
						}
						delete c;
					//}
				}
			//}
		}

		return false;
	}
	
	static bool usecheck (ServerPlayer *from ,  ServerPlayer *to, const Card *c){
		if (c->isKindOf("Jink") || c->isKindOf("Nullification"))//using matchpattern is better?
			return false
		if (from->isCardLimited(c, Card::MethodUse) || from->isProhibited(to, c))
			return false
		if (c->isKindOf("Peach")) {
			if (to->isWounded())
                return true
        } else if (c->targetFilter(QList<const Player *>(), to, from))
                return true
		return false
	}
};

class TianbianDistance : public TargetModSkill
{
public:
    TianbianDistance()
        : TargetModSkill("tianbian-dist")
    {
        pattern = "Slash,TrickCard+^DelayedTrick";
    }

    int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->hasFlag("tianbian"))
            return 1000;

        return 0;
    }
};
*/
//skills  << new TianbianDistance


HegemonyGeneralPackage::HegemonyGeneralPackage()
    : Package("hegemonyGeneral")
{
    General *kaguya_hegemony = new General(this, "kaguya_hegemony", "yyc", 4);
    kaguya_hegemony->addSkill(new YonghengHegemony);

    General *eirin_hegemony = new General(this, "eirin_hegemony", "yyc", 4);
    eirin_hegemony->addSkill("ruizhi");
    eirin_hegemony->addSkill("miyao");

    General *reisen_hegemony = new General(this, "reisen_hegemony", "yyc", 4);
    reisen_hegemony->addSkill("kuangzao");
    reisen_hegemony->addSkill("huanshi");
    //skills << new MingmuVS << new YemangRange << new MingmuRange;
}

ADD_PACKAGE(HegemonyGeneral)
