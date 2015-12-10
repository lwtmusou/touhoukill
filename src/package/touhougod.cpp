#include "touhougod.h"

#include "general.h"
#include "settings.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "generaloverview.h" //for zun?
#include "client.h"
#include "jsonutils.h"
#include "maneuvering.h" // for iceslash
#include "th10.h" //for huaxiang

class Jiexian : public TriggerSkill
{
public:
    Jiexian() : TriggerSkill("jiexian")
    {
        events << DamageInflicted << PreHpRecover;
    }

     virtual TriggerList triggerable(TriggerEvent event, Room *room, ServerPlayer *, QVariant &) const
    {  
        TriggerList skill_list;
        QList<ServerPlayer *> yukaris = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *yukari, yukaris) {
            if (yukari->canDiscard(yukari, "h")) //yukari->canDiscard(yukari, "he")
                skill_list.insert(yukari, QStringList(objectName()));
            else{ //in order to reduce frequency of client request
                foreach (const Card *c, yukari->getCards("e")) {
                    if (yukari->canDiscard(yukari, c->getEffectiveId()) && event == DamageInflicted &&  c->getSuit() == Card::Heart){
                        skill_list.insert(yukari, QStringList(objectName()));
                        break;
                    }
                    else if (yukari->canDiscard(yukari, c->getEffectiveId()) && event == PreHpRecover &&  c->getSuit() == Card::Spade){
                        skill_list.insert(yukari, QStringList(objectName()));
                        break;
                    }
                }
            }
            
        }
        return skill_list;
    }
    
     virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *source) const
    {    
        const Card *card;
        if (triggerEvent == DamageInflicted) {
            source->tag["jiexian_target"] = QVariant::fromValue(player);
            card = room->askForCard(source, "..H", "@jiexiandamage:" + player->objectName(), data, objectName());

        }else if (triggerEvent == PreHpRecover) {
            source->tag["jiexian_target"] = QVariant::fromValue(player);
            card = room->askForCard(source, "..S", "@jiexianrecover:" + player->objectName(), QVariant::fromValue(player), objectName());
        }
        if (card)
            return true;
        return false;
    }

     virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *source) const
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), player->objectName());
        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            room->touhouLogmessage("#jiexiandamage", player, objectName(), QList<ServerPlayer *>(), QString::number(damage.damage));
            if (player->isWounded()) {
                RecoverStruct recover;
                recover.who = source;
                recover.reason = objectName();
                room->recover(player, recover);
            }
            return true;
            
        } else if (triggerEvent == PreHpRecover) {
            room->touhouLogmessage("#jiexianrecover", player, objectName(), QList<ServerPlayer *>(), QString::number(data.value<RecoverStruct>().recover));
            room->damage(DamageStruct(objectName(), NULL, player));
            return true;
        }
        return false;
    }
};

class Zhouye : public TriggerSkill
{
public:
    Zhouye() : TriggerSkill("zhouye")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseStart;

        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!player) return QStringList();
        if (triggerEvent == GameStart || (triggerEvent == EventAcquireSkill && data.toString() == "zhouye")) {
            if (player->getMark("zhouye_limit") == 0 && player->hasSkill("zhouye")) {
                room->setPlayerMark(player, "zhouye_limit", 1);
                room->setPlayerCardLimitation(player, "use", "Slash", false);
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "zhouye") {
            if (player->getMark("zhouye_limit") > 0 && !player->hasSkill("zhouye")) {
                room->setPlayerMark(player, "zhouye_limit", 0);
                room->removePlayerCardLimitation(player, "use", "Slash$0");
            }
        }else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Start
            && player->hasSkill("zhouye")) 
            return QStringList( objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Start
            && player->hasSkill("zhouye")) {
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            if (player->getMark("@ye") > 0)
                player->loseAllMarks("@ye");

            QList<int> idlist = room->getNCards(1);
            int cd_id = idlist.first();
            room->fillAG(idlist, NULL);
            room->getThread()->delay();

            room->clearAG();
            Card *card = Sanguosha->getCard(cd_id);
            if (card->isBlack())
                player->gainMark("@ye", 1);
            room->throwCard(cd_id, player);
        }
        return false;
    }
};

class ZhouyeChange : public TriggerSkill
{
public:
    ZhouyeChange() : TriggerSkill("#zhouye_change")
    {
        events << PreMarkChange;// << MarkChanged;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        MarkChangeStruct change = data.value<MarkChangeStruct>();
        
        if (change.name == "@ye") {
                int mark = player->getMark("@ye");

                if (mark == 0 && mark + change.num > 0 && player->getMark("zhouye_limit") > 0) {
                    room->setPlayerMark(player, "zhouye_limit", 0);
                    room->removePlayerCardLimitation(player, "use", "Slash$0");
                } else if (mark > 0 && mark + change.num == 0 && player->getMark("zhouye_limit") == 0) {
                    room->setPlayerMark(player, "zhouye_limit", 1);
                    room->setPlayerCardLimitation(player, "use", "Slash", false);
                }
        } 
        else {
                if (change.name == "@changshi" || change.name == "@pingyi") {
                    if (!player->hasSkill("zhouye") && player->getMark("zhouye_limit") > 0) {
                        room->setPlayerMark(player, "zhouye_limit", 0);
                        room->removePlayerCardLimitation(player, "use", "Slash$0");
                    } else if (player->hasSkill("zhouye")) {
                        if (player->getMark("@ye") > 0 && player->getMark("zhouye_limit") > 0) {
                            room->setPlayerMark(player, "zhouye_limit", 0);
                            room->removePlayerCardLimitation(player, "use", "Slash$0");
                        } else if (player->getMark("@ye") == 0 && player->getMark("zhouye_limit") == 0) {
                            room->setPlayerMark(player, "zhouye_limit", 1);
                            room->setPlayerCardLimitation(player, "use", "Slash", false);
                        }
                    }
                }
        }
        return QStringList();
    }
};


HongwuCard::HongwuCard()
{
    mute = true;
    will_throw = true;
    target_fixed = true;
}
void HongwuCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->gainMark("@ye", 1);
}

class Hongwu : public ViewAsSkill
{
public:
    Hongwu() : ViewAsSkill("hongwu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return  player->getMark("@ye") == 0;
    }


    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return to_select->isRed() && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            HongwuCard *card = new HongwuCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;

    }
};

ShenqiangCard::ShenqiangCard()
{
    mute = true;
    will_throw = true;
}
void ShenqiangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    room->damage(DamageStruct("shenqiang", source, target));
}

class Shenqiang : public OneCardViewAsSkill
{
public:
    Shenqiang() :OneCardViewAsSkill("shenqiang")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@ye") > 0;
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return (to_select->isKindOf("Weapon") || to_select->getSuit() == Card::Heart)
            && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ShenqiangCard *card = new ShenqiangCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class Yewang : public TriggerSkill
{
public:
    Yewang() : TriggerSkill("yewang")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (player->getMark("@ye") > 0)
            return QStringList(objectName());
        return QStringList();
    }
    
    

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        
        damage.damage = damage.damage - 1;
        room->touhouLogmessage("#YewangTrigger", player, objectName(), QList<ServerPlayer *>(), QString::number(1));
        room->notifySkillInvoked(player, objectName());
        data = QVariant::fromValue(damage);
        if (damage.damage == 0)
            return true;
        
        return false;
    }
};


class AoyiEffect : public TriggerSkill
{
public:
    AoyiEffect() : TriggerSkill("#aoyi")
    {
        events << GameStart << CardUsed << EventAcquireSkill << EventLoseSkill << MarkChanged;

        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!player) return QStringList();
        if (triggerEvent == GameStart || (triggerEvent == EventAcquireSkill && data.toString() == "aoyi")) {
            if (player->getMark("aoyi_limit") == 0 && player->hasSkill("aoyi")) {
                room->setPlayerMark(player, "aoyi_limit", 1);
                room->setPlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick", false);
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "aoyi") {
            if (player->getMark("aoyi_limit") > 0 && !player->hasSkill("aoyi")) {
                room->setPlayerMark(player, "aoyi_limit", 0);
                room->removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0");
            }
        } else if (triggerEvent == MarkChanged) {
            MarkChangeStruct change = data.value<MarkChangeStruct>();
            if (change.name == "@changshi" || change.name == "@pingyi") {
                if (!player->hasSkill("aoyi") && player->getMark("aoyi_limit") > 0) {
                    room->setPlayerMark(player, "aoyi_limit", 0);
                    room->removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0");
                } else if (player->hasSkill("aoyi") && player->getMark("aoyi_limit") == 0) {
                    room->setPlayerMark(player, "aoyi_limit", 1);
                    room->setPlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick", false);
                }
            }
        } else if (triggerEvent == CardUsed) {
            if (!TriggerSkill::triggerable(player)) return QStringList();
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("IceSlash")){
                return QStringList(objectName());
            }
        }
        
        return QStringList();    
        
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            QStringList choices;
            choices << "aoyi1";
            QList<ServerPlayer *> all;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (player->canDiscard(p, "ej"))
                    all << p;
            }
            if (all.length() > 0)
                choices << "aoyi2";
            QString choice = room->askForChoice(player, "aoyi", choices.join("+"));
            if (choice == "aoyi1") {
                room->touhouLogmessage("#InvokeSkill", player, "aoyi");
                room->notifySkillInvoked(player, objectName());
                player->drawCards(player->getLostHp());
            } else {

                ServerPlayer *s = room->askForPlayerChosen(player, all, "aoyi", "aoyi_chosenplayer", true, true);
                int to_throw = room->askForCardChosen(player, s, "ej", "aoyi", false, Card::MethodDiscard);
                room->throwCard(to_throw, s, player);
            }
        }
        return false;
    }
};

class Aoyi : public FilterSkill
{
public:
    Aoyi() : FilterSkill("aoyi")
    {
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return  to_select->isNDTrick();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IceSlash *slash = new IceSlash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class AoyiTargetMod : public TargetModSkill
{
public:
    AoyiTargetMod() : TargetModSkill("#aoyi_mod")
    {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *card) const
    {
        if (from->hasSkill("aoyi") && card->isKindOf("IceSlash"))
            return 1000;
        else
            return 0;
    }

};

class Shikong : public TargetModSkill
{
public:
    Shikong() : TargetModSkill("shikong")
    {
        pattern = "Slash";
    }

    static int shikong_modNum(const Player *player, const Card *slash)
    {

        int num = 0;
        int rangefix = 0;
        QList<int> ids = slash->getSubcards();
        if (player->getWeapon() != NULL && ids.contains(player->getWeapon()->getId())) {
            if (player->getAttackRange() > player->getAttackRange(false))
                rangefix = rangefix + player->getAttackRange() - player->getAttackRange(false);
        }
        if (player->getOffensiveHorse() != NULL
            && ids.contains(player->getOffensiveHorse()->getId()))
            rangefix = rangefix + 1;

        foreach (const Player *p, player->getAliveSiblings()) {
            if ((player->inMyAttackRange(p) && player->canSlash(p, slash, true, rangefix))
                || Slash::IsSpecificAssignee(p, player, slash))
                num = num + 1;
        }
        return qMax(1, num);
    }

    virtual int getExtraTargetNum(const Player *player, const Card *card) const
    {
        if (player->hasSkill(objectName()) && player->getPhase() == Player::Play && card->isKindOf("Slash"))
            return shikong_modNum(player, card) - 1;
        else
            return 0;
    }
};

class Ronghui : public TriggerSkill
{
public:
    Ronghui() : TriggerSkill("ronghui")
    {
        events << DamageCaused;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() != Player::Play)
            return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user)
            return QStringList();
        if (damage.from == NULL || damage.from == damage.to)
            return QStringList();

        if (damage.card->isKindOf("Slash") && player->canDiscard(damage.to,"e")) 
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        DummyCard *dummy = new DummyCard;
        dummy->deleteLater();
        foreach (const Card *c, damage.to->getCards("e")) {
            if (player->canDiscard(damage.to, c->getEffectiveId())) {
                dummy->addSubcard(c);
            }
        }
        if (dummy->subcardsLength() > 0) {
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            room->throwCard(dummy, damage.to, player);
        }
        
        return false;
    }
};


class Jubian : public TriggerSkill
{
public:
    Jubian() : TriggerSkill("jubian")
    {
        events << Damage << CardFinished;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() != Player::Play)
            return QStringList();    
        if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card == NULL)
                return QStringList();
            if (damage.card->hasFlag("jubian_card")) {
                if (!damage.card->hasFlag("jubian_used"))
                    room->setCardFlag(damage.card, "jubian_used");
            } else
                room->setCardFlag(damage.card, "jubian_card");
        }else if (triggerEvent == CardFinished) {
            const Card *card = data.value<CardUseStruct>().card;
            if (card->hasFlag("jubian_card") && card->hasFlag("jubian_used"))
                return QStringList(objectName());
        }
        return QStringList();
        
    }


    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == CardFinished) {
            const Card *card = data.value<CardUseStruct>().card;
            if (card->hasFlag("jubian_card") && card->hasFlag("jubian_used")) {
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());

                RecoverStruct recov;
                recov.who = player;
                recov.recover = 1;
                room->recover(player, recov);
            }
        }
        return false;
    }
};

class Hengxing : public TriggerSkill
{
public:
    Hengxing() : TriggerSkill("hengxing")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Finish)
            return QStringList(objectName());
        return QStringList();
    }
    
    
    
     virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        room->loseHp(player);
        player->drawCards(3);
        return false;
    }
};


class Huanmeng : public TriggerSkill
{
public:
    Huanmeng() : TriggerSkill("huanmeng")
    {
        events << GameStart << PreHpLost << EventPhaseStart << EventPhaseChanging;
        frequency = Eternal;
    }

    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!player) return QStringList();
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == GameStart && player->isLord()) {
            room->setPlayerProperty(player, "maxhp", 0);
            room->setPlayerProperty(player, "hp", 0);
        } else if (triggerEvent == PreHpLost) {
            return QStringList(objectName());
        }else if (triggerEvent == EventPhaseStart
            && player->getPhase() == Player::RoundStart) {
            if (player->getHandcardNum() == 0) {
                return QStringList(objectName());
            }
        }  
        else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Draw) {
                return QStringList(objectName());
            }
            if (change.to == Player::Discard) {
                return QStringList(objectName());
            }
        }
        return QStringList();
    }

    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == PreHpLost) {
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            return true;
        } else if (triggerEvent == EventPhaseStart
            && player->getPhase() == Player::RoundStart) {
            if (player->getHandcardNum() == 0) {
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
                room->killPlayer(player);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Draw) {
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
                player->skip(Player::Draw);
            }
            if (change.to == Player::Discard) {
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
                player->skip(Player::Discard);
            }
        }
        return false;
    }
};
class Cuixiang : public TriggerSkill
{
public:
    Cuixiang() : TriggerSkill("cuixiang")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }
    
    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Start)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        QList<int> idlist;
        foreach(ServerPlayer *p, room->getOtherPlayers(player))
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());

        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->canDiscard(p, "h")) {
                int id = -1;
                //auto throw
                if (p->getHandcardNum() == 1)
                    id = p->getCards("h").first()->getEffectiveId();
                else {
                    const Card *cards = room->askForExchange(p, objectName(), 1, false, "cuixiang-exchange:" + player->objectName() + ":" + objectName());
                    id = cards->getSubcards().first();

                }
                room->throwCard(id, p, p);
                //we need id to check cardplace,
                //since skill "jinian",  the last handcard will be return.
                if (room->getCardPlace(id) == Player::DiscardPile)
                    idlist << id;
            } else {
                QList<int> cards = room->getNCards(1);
                room->throwCard(cards.first(), NULL, p);
                idlist << cards.first();
            }
        }

        int x = qMin(idlist.length(), 2);
        if (x == 0)
            return false;
        room->fillAG(idlist, NULL);
        for (int i = 0; i < x; i++) {

            int card_id = room->askForAG(player, idlist, false, "cuixiang");
            //just for displaying chosen card in ag container
            room->takeAG(player, card_id, false);
            room->obtainCard(player, card_id, true);
            idlist.removeOne(card_id);
        }
        room->clearAG();
        return false;
    }
};
class Xuying : public TriggerSkill
{
public:
    Xuying() : TriggerSkill("xuying")
    {
        events << SlashHit << SlashMissed;
        frequency = Eternal;
    }
    
    
   virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer* &) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (triggerEvent == SlashHit && effect.to->hasSkill(objectName())) {
            if (effect.to->getHandcardNum() > 0) 
                return QStringList(objectName());
        }if (triggerEvent == SlashMissed && effect.to->hasSkill(objectName())) {
            if (effect.from && effect.from->hasFlag("hitAfterMissed"))
                return QStringList();
            return QStringList(objectName());
        }
        
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (triggerEvent == SlashHit && effect.to->hasSkill(objectName())) {
            if (effect.to->getHandcardNum() > 0) {
                int x = qMax(effect.to->getHandcardNum() / 2, 1);
                room->touhouLogmessage("#TriggerSkill", effect.to, objectName());
                room->notifySkillInvoked(effect.to, objectName());
                room->askForDiscard(effect.to, "xuying", x, x, false, false, "xuying_discard:" + QString::number(x));
            }
        }
        if (triggerEvent == SlashMissed && effect.to->hasSkill(objectName())) {
            room->touhouLogmessage("#TriggerSkill", effect.to, objectName());
            room->notifySkillInvoked(effect.to, objectName());
            effect.to->drawCards(1);
        }
        return false;
    }
};


class Kuangyan : public TriggerSkill
{
public:
    Kuangyan() : TriggerSkill("kuangyan")
    {
        events << Dying;
    }

    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        ServerPlayer *who = room->getCurrentDyingPlayer();
        if (who != player)
            return QStringList();
        ServerPlayer *current = room->getCurrent();
        if (!current || !current->isAlive() || player == current)
            return QStringList();
        return QStringList(objectName());
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {    
        ServerPlayer *current = room->getCurrent();
        return player->askForSkillInvoke(objectName(), "recover:" + current->objectName());
    }
    
    
     virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        
        ServerPlayer *current = room->getCurrent();
        
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), current->objectName());

        player->gainMark("@kinki");
        RecoverStruct recover;
        recover.recover = 1 - player->getHp();
        room->recover(player, recover);

        room->damage(DamageStruct(objectName(), player, current));
        
        return false;
    }
};

HuimieCard::HuimieCard()
{
    mute = true;
}
bool HuimieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return(targets.isEmpty() && to_select != Self);
}
void HuimieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    source->gainMark("@kinki");
    if (!target->isChained()) {
        target->setChained(!target->isChained());
        Sanguosha->playSystemAudioEffect("chained");
        room->broadcastProperty(target, "chained");
        room->setEmotion(target, "chain");
    }
    room->damage(DamageStruct("huimie", source, target, 1, DamageStruct::Fire));

}


class Huimie : public ZeroCardViewAsSkill
{
public:
    Huimie() : ZeroCardViewAsSkill("huimie")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("HuimieCard");
    }

    virtual const Card *viewAs() const
    {
        return new HuimieCard;
    }
};

class Jinguo : public TriggerSkill
{
public:
    Jinguo() : TriggerSkill("jinguo")
    {
        events << EventPhaseEnd;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Play) 
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->getPhase() == Player::Play) {
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());

            JudgeStruct judge;
            judge.who = player;
            judge.good = true;
            judge.pattern = ".|spade";
            judge.negative = false;
            judge.play_animation = true;
            judge.reason = objectName();
            room->judge(judge);

            if (!judge.isGood()) {
                int x = player->getMark("@kinki");
                if (x == 0)
                    return false;
                int y = x / 2;
                if (x > player->getCards("he").length())
                    room->loseHp(player, y);
                else {
                    if (!room->askForDiscard(player, objectName(), x, x, true, true, "@jinguo:" + QString::number(x) + ":" + QString::number(y)))
                        room->loseHp(player, y);
                }
            }
        }
        return false;
    }
};


class Shicao : public TriggerSkill
{
public:
    Shicao() : TriggerSkill("shicao")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }
    
    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Start && player->getMark("touhou-extra") == 0)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        player->gainMark("@clock", 1);
        return false;
    }
};

class Shiting : public TriggerSkill
{
public:
    Shiting() : TriggerSkill("shiting")
    {
        events << TurnStart;
    }
    
    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    { 
        TriggerList skill_list;
        ServerPlayer *current = room->getCurrent();
        QList<ServerPlayer *> sakuyas = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *sakuya, sakuyas) {
            if (current && current == player &&current->isAlive()
                && sakuya->isAlive()
                && room->canInsertExtraTurn() && sakuya->getMark("@clock") > 0)
                skill_list.insert(sakuya, QStringList(objectName()));
        }
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *, QVariant &, ServerPlayer *skillowner) const
    {    
        ServerPlayer *current = room->getCurrent();
        QString prompt = "extraturn:" + current->objectName();
        return room->askForSkillInvoke(skillowner, objectName(), prompt);
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *skillowner) const
    {
        skillowner->loseAllMarks("@clock");
        room->touhouLogmessage("#touhouExtraTurn", skillowner, NULL);
        skillowner->gainAnExtraTurn();
        while (true) {// for limit skill combo in same turn start of the same current player
            ServerPlayer *current = room->getCurrent();
            if (current && current == player &&current->isAlive()
                && skillowner &&  skillowner->isAlive()
                && room->canInsertExtraTurn() && skillowner->getMark("@clock") > 0) {
                QString prompt = "extraturn:" + current->objectName();
                if (room->askForSkillInvoke(skillowner, objectName(), prompt)) {
                    skillowner->loseAllMarks("@clock");
                    room->touhouLogmessage("#touhouExtraTurn", skillowner, NULL);

                    skillowner->gainAnExtraTurn();
                } else
                    break;
            } else
                break;
        }
        //room->getThread()->trigger(TurnStart, room, player, data); 
        //do not trigger tun start, since it will give the current player more than one turn.  
        return false;
    }
};

class Huanzai : public TriggerSkill
{
public:
    Huanzai() : TriggerSkill("huanzai")
    {
        events << EventPhaseStart;
        frequency = Limited;
        limit_mark = "@huanzai";
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Finish) {
            if (player->getMark("@huanzai") > 0 && player->getMark("@clock") == 0)
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForSkillInvoke(player, objectName());
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->doLightbox("$huanzaiAnimate", 4000);
        player->gainMark("@clock", 1);
        room->removePlayerMark(player, "@huanzai");
 
        return false;
    }
};
class Shanghun : public TriggerSkill
{
public:
    Shanghun() : TriggerSkill("shanghun")
    {
        events << Damaged;
        frequency = Limited;
        limit_mark = "@shanghun";
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getMark("@shanghun") > 0 && player->getMark("@clock") == 0)
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForSkillInvoke(player, objectName());
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->doLightbox("$shanghunAnimate", 4000);
        player->gainMark("@clock", 1);
        room->removePlayerMark(player, "@shanghun");
        return false;
    }
};


class Banling : public TriggerSkill
{
public:
    Banling() : TriggerSkill("banling")
    {
        events << GameStart << PreHpLost << DamageInflicted << PreHpRecover;
        frequency = Eternal;
    }
    virtual int getPriority(TriggerEvent) const
    { //important?
        return -1;
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!player) return QStringList();
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == GameStart) {
            room->setPlayerMark(player, "lingtili", player->getMaxHp());
            room->setPlayerMark(player, "rentili", player->getMaxHp());
            room->setPlayerProperty(player, "hp", player->getMaxHp());
        }else
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == PreHpLost) {
            int x = player->getMark("lingtili");
            int y = player->getMark("rentili");
            int minus_x = player->getMark("minus_lingtili");
            int minus_y = player->getMark("minus_rentili");
            player->tag["banling_minus"] = data;
            for (int i = 0; i < data.toInt(); i++) {

                QStringList choices;

                choices << "lingtili";
                choices << "rentili";
                QString choice = room->askForChoice(player, "banling_minus", choices.join("+"));
                if (choice == "lingtili") {
                    if (x > 0)
                        x = x - 1;
                    else
                        minus_x = minus_x + 1;
                    room->touhouLogmessage("#lingtilidamage", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                } else if (choice == "rentili") {
                    if (y > 0)
                        y = y - 1;
                    else
                        minus_y = minus_y + 1;
                    room->touhouLogmessage("#rentilidamage", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                }
            }
            room->setPlayerMark(player, "lingtili", x);
            room->setPlayerMark(player, "rentili", y);
            room->setPlayerMark(player, "minus_lingtili", minus_x);
            room->setPlayerMark(player, "minus_rentili", minus_y);
            room->notifySkillInvoked(player, objectName());
            room->setPlayerProperty(player, "hp", qMin(x - minus_x, y - minus_y));


            LogMessage log;
            log.type = "#LoseHp";
            log.from = player;
            log.arg = QString::number(data.toInt());
            room->sendLog(log);
            log.arg = QString::number(player->getHp());
            log.arg2 = QString::number(player->getMaxHp());
            log.type = "#GetHp";
            room->sendLog(log);

            room->getThread()->trigger(PostHpReduced, room, player, data);
            return true;
        } else if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            int x = player->getMark("lingtili");
            int y = player->getMark("rentili");
            int minus_x = player->getMark("minus_lingtili");
            int minus_y = player->getMark("minus_rentili");
            player->tag["banling_minus"] = QVariant::fromValue(damage.damage);
            for (int i = 0; i < damage.damage; i++) {

                QStringList choices;

                choices << "lingtili";
                choices << "rentili";
                QString choice = room->askForChoice(player, "banling_minus", choices.join("+"));
                if (choice == "lingtili") {
                    if (x > 0)
                        x = x - 1;
                    else
                        minus_x = minus_x + 1;
                    room->touhouLogmessage("#lingtilidamage", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                }
                if (choice == "rentili") {
                    if (y > 0)
                        y = y - 1;
                    else
                        minus_y = minus_y + 1;
                    room->touhouLogmessage("#rentilidamage", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                }
            }
            room->setPlayerMark(player, "lingtili", x);
            room->setPlayerMark(player, "rentili", y);
            room->setPlayerMark(player, "minus_lingtili", minus_x);
            room->setPlayerMark(player, "minus_rentili", minus_y);

            QVariant qdata = QVariant::fromValue(damage);
            room->getThread()->trigger(PreDamageDone, room, damage.to, qdata);

            room->getThread()->trigger(DamageDone, room, damage.to, qdata);


            room->notifySkillInvoked(player, objectName());
            int z = qMin(x - minus_x, y - minus_y);
            room->setPlayerProperty(damage.to, "hp", z);

            room->getThread()->trigger(Damage, room, damage.from, qdata);

            room->getThread()->trigger(Damaged, room, damage.to, qdata);

            room->getThread()->trigger(DamageComplete, room, damage.to, qdata);

            return true;
        } else if (triggerEvent == PreHpRecover) {
            RecoverStruct recov = data.value<RecoverStruct>();
            for (int i = 0; i < recov.recover; i++) {
                int x = player->getMark("lingtili");
                int y = player->getMark("rentili");
                int minus_x = player->getMark("minus_lingtili");
                int minus_y = player->getMark("minus_rentili");
                QString choice = "rentili";

                if (x < player->getMaxHp() && y < player->getMaxHp())
                    choice = room->askForChoice(player, "banling_plus", "lingtili+rentili");
                else {
                    if (x == player->getMaxHp())
                        choice = "rentili";
                    else
                        choice = "lingtili";
                }
                if (choice == "rentili") {
                    if (minus_y == 0)
                        room->setPlayerMark(player, "rentili", y + 1);
                    else {
                        minus_y = minus_y - 1;
                        room->setPlayerMark(player, "minus_rentili", minus_y);
                    }
                    room->touhouLogmessage("#rentilirecover", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                    continue;
                }
                if (choice == "lingtili") {
                    if (minus_x == 0)
                        room->setPlayerMark(player, "lingtili", x + 1);
                    else {
                        minus_x = minus_x - 1;
                        room->setPlayerMark(player, "minus_lingtili", minus_x);
                    }
                    room->touhouLogmessage("#lingtilirecover", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                    continue;
                }
            }
            room->notifySkillInvoked(player, objectName());
        }
        return false;
    }
};

class Rengui : public PhaseChangeSkill
{
public:
    Rengui() :PhaseChangeSkill("rengui")
    {
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Start && player->hasSkill("banling")){ 
            if (player->getLingHp() < player->getMaxHp())
                return QStringList(objectName());
            if (player->getRenHp() < player->getMaxHp())
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    
    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        if (player->getPhase() == Player::Start && player->hasSkill("banling")) {
            if (player->getLingHp() < player->getMaxHp()) {
                int  x = player->getMaxHp() - player->getLingHp();
                x = qMin(x, 2);
                ServerPlayer *s = room->askForPlayerChosen(player, room->getAlivePlayers(), "renguidraw", "@rengui-draw:" + QString::number(x), true, true);
                if (s){
                    room->notifySkillInvoked(player, objectName());
                    s->drawCards(x);
                }
            }
            if (player->getRenHp() < player->getMaxHp()) {
                int y = player->getMaxHp() - player->getRenHp();
                y = qMin(y, 2);
                QList<ServerPlayer *> all;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (player->canDiscard(p, "he"))
                        all << p;
                }
                if (!all.isEmpty()) {
                    ServerPlayer *s = room->askForPlayerChosen(player, all, "renguidiscard", "@rengui-discard:" + QString::number(y), true, true);
                    if (s) {
                        room->notifySkillInvoked(player, objectName());
                        DummyCard *dummy = new DummyCard;
                        dummy->deleteLater();
                        QList<int> card_ids;
                        QList<Player::Place> original_places;
                        s->setFlags("rengui_InTempMoving");
                        for (int i = 0; i < y; i++) {
                            if (!player->canDiscard(s, "he"))
                                break;
                            int id = room->askForCardChosen(player, s, "he", objectName(), false, Card::MethodDiscard);
                            card_ids << id;
                            original_places << room->getCardPlace(id);
                            dummy->addSubcard(id);
                            s->addToPile("#rengui", id, false);
                        }
                        for (int i = 0; i < dummy->subcardsLength(); i++) {
                            Card *c = Sanguosha->getCard(card_ids.at(i));
                            room->moveCardTo(c, s, original_places.at(i), false);
                        }
                        s->setFlags("-rengui_InTempMoving");
                        if (dummy->subcardsLength() > 0)
                            room->throwCard(dummy, s, player);

                    }
                }
            }
        }
        return false;
    }
};

class Ningshi : public TriggerSkill
{
public:
    Ningshi() : TriggerSkill("ningshi")
    {
        events << TargetSpecified;
        frequency = Compulsory;
    }
    
    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (player->getPhase() == Player::Play
            && use.from == player && use.to.length() == 1 && !use.to.contains(player)) {
            if (use.card->isKindOf("Slash") || use.card->isKindOf("TrickCard"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *target = use.to.first();
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        room->loseHp(target);
        return false;
    }
};

class Gaoao : public TriggerSkill
{
public:
    Gaoao() : TriggerSkill("gaoao")
    {
        events << BeforeCardsMove;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->isCurrent())
            return QStringList();
        if (room->getTag("FirstRound").toBool())
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to != NULL && move.to == player)
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        
        //need special process when processing takeAG and askforrende
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to != NULL && move.to == player) {
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            //Card *dummy = Sanguosha->cloneCard("Slash");
            //foreach (int id, move.card_ids)
            //    dummy->addSubcard(Sanguosha->getCard(id));
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), "");
            //room->throwCard(dummy,reason,NULL);

            move.to = NULL;
            move.origin_to = NULL;
            move.origin_to_place = Player::DiscardPile;
            move.reason = reason;
            move.to_place = Player::DiscardPile;


            data = QVariant::fromValue(move);
            foreach(ServerPlayer *p, room->getAllPlayers())
                room->getThread()->trigger(BeforeCardsMove, room, p, data);
            //return true;
        }
        return false;
    }
};



ShenshouCard::ShenshouCard()
{
    mute = true;
    will_throw = false;
}
void ShenshouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();

    Card *card = Sanguosha->getCard(subcards.first());

    int x = 0;
    int y = 0;
    int z = 0;
    if (card->isKindOf("Slash"))
        x = 1;
    if (card->getSuit() == Card::Spade)
        y = 1;
    if (card->getNumber() > 4 && card->getNumber() < 10)
        z = 1;
    if (source->hasSkill("shenshou") && source->getHandcardNum() == 1) {
        x = 1;
        y = 1;
        z = 1;
    }
    QStringList choices;

    if (x == 1) {
        choices << "shenshou_slash";
    }
    if (y == 1)
        choices << "shenshou_obtain";
    if (z == 1)
        choices << "shenshou_draw";
    choices << "cancel";

    room->showCard(source, subcards.first());

    //    tempcard=sgs.Sanguosha:cloneCard("slash",sgs.Card_Spade,5)

    //    local mes=sgs.LogMessage()
    //    mes.type="$ShenshouTurnOver"
    //    mes.from=source
    //    mes.arg="shenshou"
    //    mes.card_str=tempcard:toString()
    //    room:sendLog(mes)

    room->moveCardTo(card, target, Player::PlaceHand, true);

    while (!choices.isEmpty()) {
        source->tag["shenshou_x"] = QVariant::fromValue(x);
        source->tag["shenshou_y"] = QVariant::fromValue(y);
        source->tag["shenshou_z"] = QVariant::fromValue(z);
        source->tag["shenshou_target"] = QVariant::fromValue(target);
        QString choice = room->askForChoice(source, "shenshou", choices.join("+"));
        choices.removeOne(choice);
        if (choice == "cancel")
            break;
        if (choice == "shenshou_slash") {
            x = 0;
            QList<ServerPlayer *>listt;
            foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
                if (target->inMyAttackRange(p) && target->canSlash(p, NULL, false))
                    listt << p;
            }
            if (listt.length() > 0) {
                ServerPlayer *slashtarget = room->askForPlayerChosen(source, listt, "shenshou", "@shenshou-slash:" + target->objectName());
                if (slashtarget != NULL) {
                    Slash *slash = new Slash(Card::NoSuit, 0);
                    slash->setSkillName("shenshou");
                    room->useCard(CardUseStruct(slash, target, slashtarget));
                }
            }
        }
        if (choice == "shenshou_obtain") {
            y = 0;
            QList<ServerPlayer *>listt;
            foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
                if (target->inMyAttackRange(p) && !p->isKongcheng())
                    listt << p;
            }
            if (listt.length() > 0) {
                ServerPlayer *cardtarget = room->askForPlayerChosen(source, listt, "shenshou", "@shenshou-obtain:" + target->objectName());
                if (cardtarget != NULL) {
                    int card1 = room->askForCardChosen(target, cardtarget, "h", "shenshou");
                    room->obtainCard(target, card1, false);
                }
            }
        }
        if (choice == "shenshou_draw") {
            z = 0;
            source->drawCards(1);
        }
        source->tag.remove("shenshou_x");
        source->tag.remove("shenshou_y");
        source->tag.remove("shenshou_z");

        source->tag.remove("shenshou_target");
    }
}


class Shenshou : public OneCardViewAsSkill
{
public:
    Shenshou() :OneCardViewAsSkill("shenshou")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ShenshouCard");
    }


    virtual const Card *viewAs(const Card *originalCard) const
    {
        ShenshouCard *card = new ShenshouCard;
        card->addSubcard(originalCard);

        return card;
    }
};



 class Yibian : public TriggerSkill
{
public:
    Yibian() : TriggerSkill("yibian")
    {
        events << EventPhaseStart;
        //view_as_skill = new YibianVS;
    }

    static bool sameRole(ServerPlayer *player1, ServerPlayer *player2)
    {
        QString role1 = (player1->isLord()) ? "loyalist" : player1->getRole();
        QString role2 = (player2->isLord()) ? "loyalist" : player2->getRole();
        if (role1 == role2 && role1 == "renegade")
            return false;
        return role1 == role2;
    }
    
    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        //if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Start){
            if (!player->hasShownRole())
                return QStringList(objectName());
            else if (!player->isNude()) {
                foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                    if (p->hasShownRole() && sameRole(player, p)){
                        return QStringList(objectName());
                    }
                }
            }
        }
        return QStringList();
    }
    
     virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (!player->hasShownRole()){
            QString prompt = "yibian_notice";
            return room->askForSkillInvoke(player, objectName(), prompt); 
        }
        else{
            return true;
        }
        return false;
    }
    
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (!player->hasShownRole()){
            QString role = player->getRole();
            room->touhouLogmessage("#YibianShow", player, role, room->getAllPlayers());
            room->broadcastProperty(player, "role");
            //room->setPlayerProperty(player, "role",player->getRole());
            room->setPlayerProperty(player, "role_shown", true); //important! to notify client 
            
            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                if (p->hasShownRole() && !sameRole(player, p))
                    targets << p;
            }
            if (targets.length() > 0){
                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@yibian", false, true);
                target->drawCards(1);
            }
        }
        else
        {
            QList<ServerPlayer *> targets;
            QList<int> ids;
            foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                if (p->hasShownRole() && sameRole(player, p)){
                    targets << p;
                }
            }
            foreach (const Card *c, player->getCards("he")) {
                ids << c->getEffectiveId();
            }
            QString prompt = "yibian_give"; 
            room->askForYiji(player, ids, objectName(), false, false, true, 1, targets, CardMoveReason(),  prompt); 
        }
        
        return false;
    }
    
};

FengyinCard::FengyinCard()
{
    will_throw = true;
}
bool FengyinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return(targets.isEmpty() && to_select->hasShownRole());//
}
void FengyinCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const
{

    ServerPlayer *target = targets.first();
    QString role = target->getRole();
    //room->broadcastProperty(target, "role", "unknown");
    //target->setRole(role);
    //room->notifyProperty(target, target, "role", role);
    room->broadcastProperty(target, "role");
    room->setPlayerProperty(target, "role_shown", false); //important! to notify client

    room->touhouLogmessage("#FengyinHide", target, role, room->getAllPlayers());
    target->turnOver();
}

class Fengyin : public OneCardViewAsSkill
{
public:
    Fengyin() : OneCardViewAsSkill("fengyin")
    {
        filter_pattern = ".|heart|.|.!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("FengyinCard");
    }
    
    
    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            FengyinCard *card = new FengyinCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }
};


class Huanxiang : public TriggerSkill
{
public:
    Huanxiang() : TriggerSkill("huanxiang")
    {
        events <<  EventPhaseStart;
        frequency = Compulsory;
    }
    
    static bool hasSameNumberRoles(Room *room)
    {
        //step1: loyalists vs. rebels
        int loyal_num = 0;
        int loyal_shown_num = 0;
        int rebel_num = 0;
        int rebel_shown_num = 0;
        
        foreach(ServerPlayer *p, room->getAlivePlayers()){
            QString role = (p->isLord()) ? "loyalist" : p->getRole();
            if (role == "loyalist"){
                loyal_num++;
                if (p->hasShownRole())
                    loyal_shown_num++;
            }
            else if (role == "rebel"){
                rebel_num++;
                if (p->hasShownRole())
                    rebel_shown_num++;
            }
        }
        if (loyal_num > 0 && rebel_num > 0 && loyal_shown_num == rebel_shown_num)
            return true;
    
        //step2: compare renegades
        bool find_shown_renegde = false;
        bool find_hide_renegde = false;
        int renegde_num = 0;
        QStringList roles;
        foreach(ServerPlayer *p1, room->getAlivePlayers()){
            if (p1->getRole() == "renegade"){
                renegde_num++;
                if (p1->hasShownRole())
                    find_shown_renegde = true;
                else
                    find_hide_renegde = true;
                    
                foreach(ServerPlayer *p2, room->getOtherPlayers(p1)){
                    if (p2->getRole() == "renegade"){
                        if (p1->hasShownRole() == p2->hasShownRole())
                            return true;
                    }
                }
            }
        }
        
        //step3: renegades vs. loyal or rebel
        if (find_shown_renegde && (loyal_shown_num == 1 || rebel_shown_num ==1 )){
            return true;
        }
        else if (find_hide_renegde && (loyal_shown_num == 0 || rebel_shown_num == 0 )){
            return true;
        }
        
        
        return false;         
    }
    
    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Finish){
            return QStringList(objectName());
        }
        return QStringList();
    }
    
     virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->notifySkillInvoked(player, objectName());
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        if  (!hasSameNumberRoles(room))
            room->askForDiscard(player, objectName(), 1, 1, false, true, "huanxiang_discard");
        else
            player->drawCards(1);
        return false;
    }
    
};


class RoleShownHandler : public TriggerSkill
{
public:
    RoleShownHandler() : TriggerSkill("#roleShownHandler")
    {
        events << GameStart << EventAcquireSkill; // << EventLoseSkill << EventPhaseChanging << Death << Debut;
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!player) return QStringList();
        if (triggerEvent == GameStart) {
            foreach(ServerPlayer *p, room->getAlivePlayers())
                room->setPlayerProperty(p, "role_shown",  p->isLord() ? true : false );    
        }
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == EventAcquireSkill){
            QString skillName = data.toString();
            if (skillName == "fengyin" || skillName == "yibian" || skillName == "huanxiang"){
                foreach(ServerPlayer *p, room->getAlivePlayers())
                    room->setPlayerProperty(p, "role_shown", p->hasShownRole() ? true : false);                
            }
        }
        
        return QStringList();
    }
};



class Quanjie : public TriggerSkill
{
public:
    Quanjie() :TriggerSkill("quanjie")
    {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {   
        TriggerList skill_list;
        if (player->getPhase() == Player::Play){
            QList<ServerPlayer *> srcs = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *src, srcs) {
            if (player != src)
                skill_list.insert(src, QStringList(objectName()));
            }
        }
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ymsnd) const
    {
        room->setPlayerMark(player, objectName(), 0);
        return ymsnd->askForSkillInvoke(objectName(), QVariant::fromValue(player));
    }
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *ymsnd) const
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, ymsnd->objectName(), player->objectName());

        const Card *card = room->askForCard(player, "%slash,%thunder_slash,%fire_slash", "@quanjie-discard");
        if (card == NULL) {
            player->drawCards(1);
            room->setPlayerMark(player, objectName(), 1);
            room->setPlayerCardLimitation(player, "use", "Slash", true);
        }
        return false;
    }
};



class Duanzui : public TriggerSkill
{
public:
    Duanzui() : TriggerSkill("duanzui")
    {
        events << EventPhaseStart << Death << TurnStart;
    }
    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }
    
    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player)
                room->setTag("duanzui", true);
        } else if (triggerEvent == TurnStart)
            room->setTag("duanzui", false);

        return ;
    }
    
    
    virtual TriggerList triggerable(TriggerEvent e, Room *room, ServerPlayer *, QVariant &) const
    { 
        TriggerList skill_list;
        ServerPlayer *current = room->getCurrent();
        if (e ==EventPhaseStart && 
            current && current->getPhase() == Player::NotActive) {
            if (!room->getTag("duanzui").toBool())
                return skill_list;

            room->setTag("duanzui", false);
            QList<ServerPlayer *> srcs = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *ymsnd, srcs) {
                if (current != ymsnd && (room->canInsertExtraTurn()|| !ymsnd->faceUp() ))
                    skill_list.insert(ymsnd, QStringList(objectName()));
            }
        }
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent , Room *, ServerPlayer *, QVariant &, ServerPlayer *ymsnd) const
    {
        return    ymsnd->askForSkillInvoke(objectName());
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *, QVariant &, ServerPlayer *ymsnd) const
    {
        ServerPlayer * current = room->getCurrent(); 
        if (!ymsnd->faceUp())
            ymsnd->turnOver();
        if (room->canInsertExtraTurn()){
            ymsnd->gainMark("@duanzui-extra");
            QList<ServerPlayer *> logto;
            logto << current->getNext();
            room->touhouLogmessage("#touhouExtraTurn", ymsnd, NULL, logto);
            ymsnd->gainAnExtraTurn();
            ymsnd->loseMark("@duanzui-extra");
        }
        
        return false;
    }
};
class DuanzuiShenpan : public TriggerSkill
{
public:
    DuanzuiShenpan() : TriggerSkill("#duanzui-shenpan")
    {
        events << PreMarkChange;
    }
    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if ((player->getMark("@duanzui-extra") > 0 || player->hasSkill("duanzui")) && !player->isCurrent()){
            MarkChangeStruct change = data.value<MarkChangeStruct>();
            if (change.name == "@duanzui-extra") {
                if (change.num > 0 && player->getMark("@duanzui-extra") == 0 && !player->hasSkill("shenpan"))
                    room->handleAcquireDetachSkills(player, "shenpan");
                else if (change.num < 0 && player->getMark("@duanzui-extra") + change.num <= 0 && player->hasSkill("shenpan"))
                    room->handleAcquireDetachSkills(player, "-shenpan");
            }
        }
        return QStringList();
    }
};


HuaxiangCard::HuaxiangCard()
{
    mute = true;
    will_throw = false;
}

bool HuaxiangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), Card::NoSuit, 0);
            return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *card = Self->tag.value("huaxiang").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName(), Card::NoSuit, 0);
    new_card->setSkillName("huaxiang");
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool HuaxiangCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), Card::NoSuit, 0);
            return card && card->targetFixed();
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("huaxiang").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName(), Card::NoSuit, 0);
    new_card->setSkillName("huaxiang");
    return new_card && new_card->targetFixed();
}

bool HuaxiangCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), Card::NoSuit, 0);
            return card && card->targetsFeasible(targets, Self);
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("huaxiang").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName(), Card::NoSuit, 0);
    new_card->setSkillName("huaxiang");
    return new_card && new_card->targetsFeasible(targets, Self);
}

const Card *HuaxiangCard::validate(CardUseStruct &card_use) const
{
    QString to_use = user_string;
    card_use.from->getRoom()->touhouLogmessage("#InvokeSkill", card_use.from, "huaxiang");    
    card_use.from->addToPile("rainbow", subcards.first());

    Card *use_card = Sanguosha->cloneCard(to_use, Card::NoSuit, 0);
    use_card->setSkillName("huaxiang");
    use_card->deleteLater();
    return use_card;
}

const Card *HuaxiangCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();
        
    QString to_use;
    if (user_string == "peach+analeptic") {
        QStringList use_list;
        Card *peach = Sanguosha->cloneCard("peach");
        if (!user->isCardLimited(peach, Card::MethodResponse, true) && user->getMaxHp() <= 2 )
            use_list << "peach";
        Card *ana = Sanguosha->cloneCard("analeptic");
        if (!Config.BanPackages.contains("maneuvering") && !user->isCardLimited(ana, Card::MethodResponse, true))
            use_list << "analeptic";
        to_use = room->askForChoice(user, "huaxiang_skill_saveself", use_list.join("+"));
    } else
        to_use = user_string;
        
    room->touhouLogmessage("#InvokeSkill", user, "huaxiang");    
    user->addToPile("rainbow", subcards.first());
    Card *use_card = Sanguosha->cloneCard(to_use, Card::NoSuit, 0);
    use_card->setSkillName("huaxiang");
    use_card->deleteLater();

    return use_card;
}



class Huaxiang : public ViewAsSkill
{
public:
    Huaxiang() : ViewAsSkill("huaxiang")
    {
        //response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getPile("rainbow").length() > 3) return false;
        if (player->isKongcheng()) return false;
        if  (Slash::IsAvailable(player) || Analeptic::IsAvailable(player))
            return true;
        if (player->getMaxHp() <= 2){
            Card *card = Sanguosha->cloneCard("peach", Card::NoSuit, 0);
            card->deleteLater();
            return card->isAvailable(player);
        }
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getPile("rainbow").length() > 3) return false;
        if (player->isKongcheng()) return false;
        QStringList validPatterns;
        validPatterns << "slash" <<  "analeptic";
        if (player->getMaxHp() <= 3)
            validPatterns << "jink";
        if (player->getMaxHp() <= 2)
            validPatterns << "peach";
        if (player->getMaxHp() <= 1)
            validPatterns << "nullification";
        bool valid = false;
        foreach (QString str, validPatterns) {
            if (pattern.contains(str)){
                valid = true;
                break;
            }
        }    
        if (!valid) return false;
        
        if (pattern == "peach" && player->hasFlag("Global_PreventPeach")) return false;
        for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false;
        }
        
        if (pattern == "slash" || pattern == "jink") {
            Card *card = Sanguosha->cloneCard(pattern);
            return !player->isCardLimited(card, Card::MethodResponse, true);
        }
        
        
            
        if (pattern.contains("peach") && pattern.contains("analeptic")) {
            Card *peach = Sanguosha->cloneCard("peach");
            Card *ana = Sanguosha->cloneCard("analeptic");
            return !player->isCardLimited(peach, Card::MethodResponse, true) ||
                !player->isCardLimited(ana, Card::MethodResponse, true);
        } else if (pattern == "peach") {
            Card *peach = Sanguosha->cloneCard("peach");
            return !player->isCardLimited(peach, Card::MethodResponse, true);
        } else if (pattern == "analeptic") {
            Card *ana = Sanguosha->cloneCard("analeptic");
            return !player->isCardLimited(ana, Card::MethodResponse, true);
        }

        return true;
    }

    
    
    
    
    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (to_select->isEquipped() || !selected.isEmpty()) return false;
        
        foreach(int id, Self->getPile("rainbow")) {
            Card *card = Sanguosha->getCard(id);
            if (card->getSuit() == to_select->getSuit())
                return false;
        }
        
        return selected.isEmpty();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 1) return NULL;
        
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern.contains("slash")) {
                const Card *c = Self->tag.value("huaxiang").value<const Card *>();
                if (c)
                    pattern = c->objectName();
                else
                    return NULL;
            }
            HuaxiangCard *card = new HuaxiangCard;
            card->setUserString(pattern);
            card->addSubcards(cards);
            return card;

        }

        const Card *c = Self->tag.value("huaxiang").value<const Card *>();
        if (c) {
            HuaxiangCard *card = new HuaxiangCard;
            card->setUserString(c->objectName());
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }
    
    virtual QDialog *getDialog() const
    {
        return QijiDialog::getInstance("huaxiang", true, false);
    }
    
    
    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        if (player->getPile("rainbow").length() > 3) return false;
        if (player->isKongcheng()) return false;
        if (player->isCardLimited(Sanguosha->cloneCard("nullification"), Card::MethodResponse, true))
            return false;
        return player->getMaxHp() <= 1;
    }
};


class Caiyu : public TriggerSkill
{
public:
    Caiyu() : TriggerSkill("caiyu")
    {
        events << EventPhaseStart;
    }

    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    { 
        TriggerList skill_list;
        if (player->getPhase() == Player::Finish) {
            QList<ServerPlayer *> merins = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *merin, merins) {
                if (merin->getPile("rainbow").length() > 3)
                    skill_list.insert(merin, QStringList(objectName()));
            }
        }
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *, QVariant &, ServerPlayer *merin) const
    {
        return room->askForSkillInvoke(merin, objectName(), "discard");
    }
    
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *, QVariant &, ServerPlayer *merin) const
    {
        
        QList<int>  idlist = merin->getPile("rainbow");
        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, merin->objectName(), NULL, objectName(), "");
        CardsMoveStruct move(idlist, merin, Player::DiscardPile,
                        reason);
        room->moveCardsAtomic(move, true);
                
        merin->drawCards(2);
                    
        if (room->askForSkillInvoke(merin, objectName(), "loseMaxHp")) {
            room->loseMaxHp(merin, 1);
        }
        return false;
    }
};


class Xuanlan : public TriggerSkill
{
public:
    Xuanlan() : TriggerSkill("xuanlan")
    {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Discard &&
            !player->isWounded())
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return room->askForSkillInvoke(player, objectName());
    }
    virtual bool effect(TriggerEvent , Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
       
        player->skip(Player::Discard);
        return false;
    }
};



class QiannianMax : public MaxCardsSkill
{
public:
    QiannianMax() : MaxCardsSkill("#qiannian_max")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        int n = target->getMark("@qiannian");
        if (n > 0 && target->hasSkill("qiannian"))
            return 2 * n;
        else
            return 0;
    }
};
class Qiannian : public TriggerSkill
{
public:
    Qiannian() : TriggerSkill("qiannian")
    {
        events << GameStart  << DrawNCards << DrawPileSwaped ;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!player) return QStringList();
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == GameStart || triggerEvent == DrawPileSwaped)
            return QStringList(objectName());
        else if (triggerEvent == DrawNCards){
            if (player->getMark("@qiannian") > 0) 
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();    
        if (triggerEvent == GameStart || triggerEvent == DrawPileSwaped) {  
            room->sendLog(log);
            room->notifySkillInvoked(player, objectName());

            player->gainMark("@qiannian", 1);
        }
        else if (triggerEvent == DrawNCards){
            if (player->getMark("@qiannian") > 0) {
                data = QVariant::fromValue(data.toInt() + player->getMark("@qiannian"));

                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
            }
        }
        return false;
    }
};


class Qinlue : public TriggerSkill
{
public:
    Qinlue() : TriggerSkill("qinlue")
    {
        events << EventPhaseChanging;
    }
    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *, QVariant &data, ServerPlayer* &) const
    {
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (!source) return QStringList();
        ServerPlayer *current = room->getCurrent();
        if (!current || current->getMark("touhou-extra") > 0)  return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Play && current != source && !current->isSkipped(Player::Play)) 
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        ServerPlayer *current = room->getCurrent();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Play && current != source && !current->isSkipped(Player::Play)) {
            QString  prompt = "@qinlue-discard:" + current->objectName();
            const Card *card = room->askForCard(source, "Slash,EquipCard", prompt, QVariant::fromValue(current), Card::MethodDiscard, current, false, "qinlue");
            if (card != NULL) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), current->objectName());

                QString  prompt = "@qinlue-discard1:" + source->objectName();
                const Card *card1 = room->askForCard(current, "Jink", prompt, QVariant::fromValue(source), Card::MethodDiscard);
                if (!card1) {
                    current->skip(Player::Play);

                    source->tag["qinlue_current"] = QVariant::fromValue(current);
                    //room->setPlayerMark(source, "touhou-extra", 1);
                    source->setFlags("qinlue");
                    source->setPhase(Player::Play);
                    //current->setPhase(Player::NotActive);
                    current->setPhase(Player::PhaseNone);
                    //room->setCurrent(source);
                    room->broadcastProperty(source, "phase");
                    room->broadcastProperty(current, "phase");
                    RoomThread *thread = room->getThread();
                    if (!thread->trigger(EventPhaseStart, room, source))
                        thread->trigger(EventPhaseProceeding, room, source);

                    thread->trigger(EventPhaseEnd, room, source);
                    //room->setCurrent(current);

                    source->changePhase(Player::Play, Player::NotActive);

                    //room->setPlayerMark(source, "touhou-extra", 0);
                    current->setPhase(Player::PhaseNone);

                    source->setFlags("-qinlue");
                    room->broadcastProperty(source, "phase");
                }
            }
        }

        return false;
    }
};

class QinlueEffect : public TriggerSkill
{
public:
    QinlueEffect() : TriggerSkill("#qinlue_effect")
    {
        events << EventPhaseChanging << EventPhaseStart;
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if ( !player->hasFlag("qinlue"))
            return QStringList();
        
    
        if (triggerEvent == EventPhaseStart) {
            if (player->hasFlag("qinlue") && player->getPhase() == Player::Play) {

                ServerPlayer *target = player->tag["qinlue_current"].value<ServerPlayer *>();
                if (target != NULL && target->isAlive()) {
                    QList<int> ids;
                    foreach(const Card *c, player->getHandcards())
                        ids << (c->getId());
                    player->addToPile("qinlue", ids, false);
                    DummyCard *dummy = new DummyCard;
                    dummy->deleteLater();
                    dummy->addSubcards(target->getHandcards());
                    room->obtainCard(player, dummy, false);
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            if (player->hasFlag("qinlue")) {
                ServerPlayer *target = player->tag["qinlue_current"].value<ServerPlayer *>();
                if (target != NULL && target->isAlive()) {
                    DummyCard *dummy = new DummyCard;
                    dummy->deleteLater();
                    dummy->addSubcards(player->getHandcards());
                    room->obtainCard(target, dummy, false);
                }
                if (!player->getPile("qinlue").isEmpty()) {
                    DummyCard *dummy1 = new DummyCard;
                    dummy1->deleteLater();
                    foreach(int c, player->getPile("qinlue"))
                        dummy1->addSubcard(c);
                    room->obtainCard(player, dummy1, false);
                }
                //if (target != NULL)  //for siyu broken
                //    room->setCurrent(target);

            }
        }

        return QStringList();
    }
};



/* ChaorenPreventRecast::ChaorenPreventRecast()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodUse;
}
bool ChaorenPreventRecast::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Card *card = Sanguosha->getCard(Self->property("chaoren").toInt());
    if (Sanguosha->isProhibited(Self, to_select, card))
        return false;
    return card->targetFilter(targets, to_select, Self);

}
bool ChaorenPreventRecast::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (targets.isEmpty())
        return false;
    Card *card = Sanguosha->getCard(Self->property("chaoren").toInt());
    foreach (const Player *p, targets) {
        if (Sanguosha->isProhibited(Self, p, card))
            return false;
    }
    return card->targetsFeasible(targets, Self);

}
const Card *ChaorenPreventRecast::validate(CardUseStruct &card_use) const
{
    Card *card = Sanguosha->getCard(card_use.from->property("chaoren").toInt());
    card->setSkillName("chaoren");
    return card;
}

 */
/* class ChaorenVS : public ZeroCardViewAsSkill
{
public:
    ChaorenVS() : ZeroCardViewAsSkill("chaoren")
    {
    }
    static bool cardCanRecast(Card *card)
    {
        return card->canRecast() && !card->isKindOf("Weapon");
    }
    virtual bool isEnabledAtPlay(const Player *player) const
    {
        Card *card = Sanguosha->getCard(player->property("chaoren").toInt());
        if (card != NULL && card->isAvailable(player)) {
            if (cardCanRecast(card) && player->isCardLimited(card, Card::MethodUse))
                return false;
            return true;
        }
        return false;
    }
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        Card *card = Sanguosha->getCard(player->property("chaoren").toInt());
        if (card == NULL)
            return false;
        QStringList realpattern = pattern.split("+");
        if (player->hasFlag("Global_PreventPeach"))
            realpattern.removeOne("peach");
        Card::HandlingMethod handlingmethod = (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) ? Card::MethodUse : Card::MethodResponse;
        if (realpattern.contains("slash")) {
            realpattern << "fire_slash";
            realpattern << "thunder_slash";
        }
        return realpattern.contains(card->objectName()) && !player->isCardLimited(card, handlingmethod);
    }
    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        Card *card = Sanguosha->getCard(player->property("chaoren").toInt());
        if (card == NULL)
            return false;
        return card->isKindOf("Nullification") && !player->isCardLimited(card, Card::MethodUse);

    }


    virtual const Card *viewAs() const
    {
        Card *card = Sanguosha->getCard(Self->property("chaoren").toInt());
        if (card != NULL) {
            if (cardCanRecast(card))
                return new ChaorenPreventRecast;
            else {
                card->setSkillName("chaoren");
                return card;
            }
        }
        return NULL;
    }
};
 */
/* class Chaoren : public TriggerSkill
{
public:
    Chaoren() : TriggerSkill("chaoren")
    {
        events << PreCardUsed << CardResponded << CardUsed << CardFinished << CardsMoveOneTime << TargetConfirming;
        view_as_skill = new ChaorenVS;
    }


    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isVirtualCard() && use.card->subcardsLength() != 1)
                return QStringList();
            if (player == use.from && player->hasSkill("chaoren") &&
                use.card->getId() == player->property("chaoren").toInt()) {
                room->touhouLogmessage("#InvokeSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
            }
            return QStringList();
        }
        if (triggerEvent == CardResponded) {
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isVirtualCard() && card_star->subcardsLength() != 1)
                return QStringList();
            if (player == data.value<CardResponseStruct>().m_who && player->hasSkill("chaoren")
                && card_star->getId() == player->property("chaoren").toInt()) {
                room->touhouLogmessage("#InvokeSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
            }
            return QStringList();
        }
        ServerPlayer *sbl = room->findPlayerBySkillName(objectName());
        if (sbl == NULL || sbl->isDead())
            return QStringList();
        QList<int> drawpile = room->getDrawPile();
        int firstcard = -1;
        if (!drawpile.isEmpty())
            firstcard = drawpile.first();
        //deal the amazinggrace
        //update firstcard¡£¡£¡£
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("AmazingGrace"))
                return QStringList();
        }

        room->setPlayerProperty(sbl, "chaoren", firstcard);

        if (room->getTag("FirstRound").toBool())
            return QStringList();
        if (triggerEvent == CardUsed || triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("AmazingGrace"))
                sbl->setFlags((triggerEvent == CardUsed) ? "agusing" : "-agusing");
        }
        bool invoke = (triggerEvent == CardUsed || triggerEvent == TargetConfirming);
        if (triggerEvent == CardsMoveOneTime && player != NULL && player->isAlive() && player->hasSkill(objectName())) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from_places.contains(Player::DrawPile) || (move.to_place == Player::DrawPile))
                invoke = true;
        }
        if (invoke && firstcard != -1) {
            QList<int> watchlist;
            watchlist << firstcard;
            LogMessage l;
            l.type = "$chaorendrawpile";
            l.card_str = IntList2StringList(watchlist).join("+");

            room->doNotify(sbl, QSanProtocol::S_COMMAND_LOG_SKILL, l.toJsonValue());


            if (!sbl->hasFlag("agusing")) {
                Json::Value gongxinArgs(Json::arrayValue);

                gongxinArgs[0] = QSanProtocol::Utils::toJsonString(QString());
                gongxinArgs[1] = false;
                gongxinArgs[2] = QSanProtocol::Utils::toJsonArray(watchlist);

                room->doNotify(sbl, QSanProtocol::S_COMMAND_SHOW_ALL_CARDS, gongxinArgs);
            }
        }
        return QStringList();
    }
};

 */

 
class Chaoren : public TriggerSkill
{
public:
    Chaoren() : TriggerSkill("chaoren")
    {
        events    << CardsMoveOneTime   << TargetConfirming << EventPhaseChanging << DrawPileSwaped 
        << EventAcquireSkill << EventLoseSkill << MarkChanged << AfterGuanXing << Reconnect;
    }


    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        //part1: ignore some cases         
       //for global event, check player has this skill
        if (triggerEvent == CardsMoveOneTime || triggerEvent == DrawPileSwaped){
            if (!player->hasSkill(objectName()))
                return QStringList();
        }
        //deal the amazinggrace
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("AmazingGrace"))
                return QStringList();
        }
        
        
        if (room->getTag("FirstRound").toBool())
            return QStringList();

        if ((triggerEvent == EventAcquireSkill || triggerEvent == EventLoseSkill )&& data.toString() != "chaoren")    
            return QStringList();
            
        // part 2: 
        ServerPlayer *sbl = room->findPlayerBySkillName(objectName());
        bool retract = false;
        bool expand = false;
        if (triggerEvent == EventLoseSkill && data.toString() == "chaoren" && !player->hasSkill("chaoren") && player->hasSkill("chaoren", true)){
             sbl = player;
             retract = true;
        }
        
        //EventPhaseChanging : for skill invalid, such as changshi
        if (triggerEvent == MarkChanged){ 
            
            MarkChangeStruct change = data.value<MarkChangeStruct>();
            if  (change.name != "@changshi"  && change.name != "@pingyi")
                return QStringList();
            if (change.name == "@changshi" && player->getMark(change.name) >0 && !player->hasSkill("chaoren") && player->hasSkill("chaoren", false, true)){
                sbl = player;
                retract = true;
            }
            if(change.name == "@pingyi" && player->getMark("pingyichaoren") >0){
                sbl = player;
                retract = true;
            }
        }
        if (triggerEvent == Reconnect)
            expand = true;
        
        
        
        if (sbl == NULL || sbl->isDead())
            return QStringList();
            
        
        
        QList<int> drawpile = room->getDrawPile();
        
        int old_firstcard = sbl->property("chaoren").toInt();
        if (old_firstcard == NULL|| old_firstcard <0)
            old_firstcard = -1;
        //if (!sbl->getPile("chaoren").isEmpty())
        //    old_firstcard = sbl->getPile("chaoren").first();
        int new_firstcard = -1;
        if (!drawpile.isEmpty())
            new_firstcard = drawpile.first();
        
        

        int changed = (new_firstcard != old_firstcard); 
        if (!changed){
            if (retract){
                room->setPlayerProperty(sbl, "chaoren", -1);
                Json::Value args;
                args[0] = QSanProtocol::S_GAME_EVENT_RETRACT_PILE_CARDS;
                room->doNotify(sbl, QSanProtocol::S_COMMAND_LOG_EVENT, args);
            }
            else if (expand){
                room->setPlayerProperty(sbl, "chaoren", new_firstcard);
                Json::Value args;
                args[0] = QSanProtocol::S_GAME_EVENT_EXPAND_PILE_CARDS;
                room->doNotify(sbl, QSanProtocol::S_COMMAND_LOG_EVENT, args); 
            }
            return QStringList();
        }
        //retract at first, then expand
        if (changed){
            //sbl->clearOnePrivatePile("chaoren");
            
            /* if (old_firstcard > -1){
                Json::Value args;
                args[0] = QSanProtocol::S_GAME_EVENT_RETRACT_PILE_CARDS;
                room->doNotify(sbl, QSanProtocol::S_COMMAND_LOG_EVENT, args);
            } */

            room->setPlayerProperty(sbl, "chaoren", new_firstcard);
            //for displaying the change on dashboard immidately, even  the status is not Playing or Response.
            Json::Value args;
            args[0] = QSanProtocol::S_GAME_EVENT_EXPAND_PILE_CARDS;
            room->doNotify(sbl, QSanProtocol::S_COMMAND_LOG_EVENT, args); 
            
            
            // for client log 
            if (new_firstcard > -1){
                QList<int> watchlist;
                watchlist << new_firstcard;
                LogMessage l;
                l.type = "$chaorendrawpile";
                l.card_str = IntList2StringList(watchlist).join("+");

                room->doNotify(sbl, QSanProtocol::S_COMMAND_LOG_SKILL, l.toJsonValue());
            }
            
        }
        
        return QStringList();
    }
};




class Biaoxiang : public TriggerSkill
{
public:
    Biaoxiang() : TriggerSkill("biaoxiang")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }
    
    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        
        if (player->getMark("biaoxiang") == 0  && player->getPhase() == Player::Start && player->getHandcardNum() < 2)
            return QStringList(objectName());
        return QStringList();
    }

     virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->doLightbox("$biaoxiangAnimate", 4000);
        room->touhouLogmessage("#BiaoxiangWake", player, objectName());
        room->notifySkillInvoked(player, objectName());
        room->addPlayerMark(player, objectName());
        int x = player->getMaxHp();
        int y;
        if (x > 3)
            y = 3 - x;
        else
             y = 4 - x;

        if (room->changeMaxHpForAwakenSkill(player, y))
            room->handleAcquireDetachSkills(player, "ziwo");
        return false;
    }
};

ZiwoCard::ZiwoCard()
{
    mute = true;
    will_throw = true;
    target_fixed = true;
}
void ZiwoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    RecoverStruct recov;
    recov.recover = 1;
    recov.who = source;
    room->recover(source, recov);
}

class Ziwo : public ViewAsSkill
{
public:
    Ziwo() : ViewAsSkill("ziwo")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return  player->isWounded();
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isEquipped() && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            ZiwoCard *card = new ZiwoCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;

    }
};

class Shifang : public TriggerSkill
{
public:
    Shifang() : TriggerSkill("shifang")
    {
        events << CardsMoveOneTime << BeforeCardsMove;
        frequency = Wake;
    }
    static void koishi_removeskill(Room *room, ServerPlayer *player)
    {
        foreach (const Skill *skill, player->getVisibleSkillList()) {
            if (skill->getFrequency() == Skill::Wake || skill->isAttachedLordSkill()
                || skill->getFrequency() == Skill::Eternal)
                continue;
            else {
                QString skill_name = skill->objectName();

                room->handleAcquireDetachSkills(player, "-" + skill_name);

            }
        }
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        
        if (player->getMark("shifang") > 0)
            return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (triggerEvent == BeforeCardsMove) {
            if (move.from != NULL && move.from == player && player->getCards("e").length() == 1) {
                foreach (int id, move.card_ids) {
                    if (room->getCardPlace(id) == Player::PlaceEquip) {
                        room->setCardFlag(Sanguosha->getCard(id), "shifang");
                        room->setPlayerFlag(player, "shifangInvoked");
                        break;
                    }
                }
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            if (move.from != NULL && move.from == player) { //&& move.from_place == Player::Equip
                if (player->hasFlag("shifangInvoked") && player->getMark("shifang") == 0) {
                    foreach (int id, move.card_ids) {
                        if (Sanguosha->getCard(id)->hasFlag("shifang")) {
                            return QStringList(objectName());
                        }
                    }
                }
            }
        }
        return QStringList();
    }
    
    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        room->doLightbox("$shifangAnimate", 4000);
        room->touhouLogmessage("#ShifangWake", player, objectName());
        koishi_removeskill(room, player);

        room->notifySkillInvoked(player, objectName());
        room->addPlayerMark(player, objectName());
        int x = player->getMaxHp();
        if (room->changeMaxHpForAwakenSkill(player, 4 - x)) {
            room->handleAcquireDetachSkills(player, "benwo");
            room->setPlayerFlag(player, "-shifangInvoked");
        }   
        return false;
    }
};

class Benwo : public TriggerSkill
{
public:
    Benwo() : TriggerSkill("benwo")
    {
        events << DamageInflicted;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (player->isWounded() && damage.from != NULL && damage.from->isAlive())
            return QStringList(objectName());
        return QStringList();
    }
    
   virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        player->tag["benwo_target"] = QVariant::fromValue(damage.from);
        int x = player->getLostHp();
        QString prompt = "invoke:" + damage.from->objectName() + ":" + QString::number(x);
        return room->askForSkillInvoke(player, objectName(), QVariant::fromValue(damage.from));
    }
   
   virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        int x = player->getLostHp();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());

        player->drawCards(x);
        room->askForDiscard(damage.from, objectName(), x, x, false, true);
        return false;
    }
};

class Yizhi : public TriggerSkill
{
public:
    Yizhi() : TriggerSkill("yizhi")
    {
        events << Dying;
        frequency = Wake;
    }

    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getMark("yizhi") > 0)
            return QStringList();
        ServerPlayer *victim = room->getCurrentDyingPlayer();
        if (victim != player)
            return QStringList();
        return QStringList(objectName());
    }
    
    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        room->doLightbox("$yizhiAnimate", 4000);
        room->touhouLogmessage("#YizhiWake", player, objectName());
        room->notifySkillInvoked(player, objectName());
        int x = 1 - player->getHp();
        RecoverStruct recov;
        recov.recover = x;
        recov.who = player;
        room->recover(player, recov);



        x = player->getMaxHp();
        if (room->changeMaxHpForAwakenSkill(player, 3 - x)) {
            Shifang::koishi_removeskill(room, player);
            room->addPlayerMark(player, objectName());
            room->handleAcquireDetachSkills(player, "chaowo");
        }

        return false;
    }
};


ChaowoCard::ChaowoCard()
{
    mute = true;
    will_throw = true;
}
bool ChaowoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return(targets.isEmpty() && to_select->getMaxHp() >= Self->getMaxHp());
}
void ChaowoCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{

    targets.first()->drawCards(2);
    if (targets.first()->getMaxHp() == 3)
        source->drawCards(2);
}

class ChaowoVS : public OneCardViewAsSkill
{
public:
    ChaowoVS() : OneCardViewAsSkill("chaowo")
    {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@chaowo";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            ChaowoCard *card = new ChaowoCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }
};

class Chaowo : public TriggerSkill
{
public:
    Chaowo() : TriggerSkill("chaowo")
    {
        events << EventPhaseStart;
        view_as_skill = new ChaowoVS;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Finish) 
            return QStringList(objectName());
        return QStringList();
    }
    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        room->askForUseCard(player, "@@chaowo", "@chaowo");
        return false;
    }
};



class Zuosui : public TriggerSkill
{
public:
    Zuosui() : TriggerSkill("zuosui")
    {
        events << DamageCaused;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.from || !damage.to || damage.from == damage.to)
            return QStringList();
        return QStringList(objectName());
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        player->tag["zuosui_damage"] = data;
         DamageStruct damage = data.value<DamageStruct>();
        return room->askForSkillInvoke(player, objectName(), QVariant::fromValue(damage.to));
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());

        player->gainMark("@xinyang");

        damage.to->tag["zuosui_source"] = QVariant::fromValue(player);
        QString choice = room->askForChoice(damage.to, objectName(), "1+2+3+4");
        int x;
        if (choice == "1")
            x = 1;
        else if (choice == "2")
            x = 2;
        else if (choice == "3")
            x = 3;
        else
            x = 4;
        room->touhouLogmessage("#zuosuichoice", damage.to, objectName(), QList<ServerPlayer *>(), QString::number(x));
        player->tag["zuosui_number"] = QVariant::fromValue(x);
        choice = room->askForChoice(player, objectName(), "losehp+discard");
        if (choice == "losehp") {
            damage.to->drawCards(x);
            room->loseHp(damage.to, x);
        } else {
            int discardNum = damage.to->getCards("he").length() > x ? damage.to->getCards("he").length() - x : 0;
            if (discardNum > 0)
                room->askForDiscard(damage.to, objectName(), discardNum, discardNum, false, true);
        }
        return true;
    }
};

class Worao : public TriggerSkill
{
public:
    Worao() : TriggerSkill("worao")
    {
        events << TargetConfirming;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
         if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from  && player != use.from  &&  use.to.contains(player)
                && (use.card->isKindOf("Slash") || use.card->isNDTrick()))
            return QStringList(objectName());
        return QStringList();
    }
    
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "invoke:" + use.from->objectName() + ":" + use.card->objectName();
        player->tag["worao_use"] = data;
        return room->askForSkillInvoke(player, objectName(), prompt);
    }
    
    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {    
        CardUseStruct use = data.value<CardUseStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.from->objectName());
        player->gainMark("@xinyang");
        player->drawCards(2);
        if (!player->isKongcheng()) {
            const Card *card = room->askForExchange(player, objectName(), 1, false, "woraoGive:" + use.from->objectName());
            use.from->obtainCard(card, false);
        }
        return false;
    }
};


class Shenhua : public TriggerSkill
{
public:
    Shenhua() : TriggerSkill("shenhua")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish)
            return QStringList(objectName());
        return QStringList();
    }
    
     virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->notifySkillInvoked(player, objectName());
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        if (player->getMark("@xinyang") == 0)
            room->loseMaxHp(player, 1);
        else
            player->loseMark("@xinyang", player->getMark("@xinyang"));
        return false;
    }
};


class Hongfo : public TriggerSkill
{
public:
    Hongfo() : TriggerSkill("hongfo")
    {
        events << AfterDrawNCards;
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return room->askForSkillInvoke(player, objectName(), data);
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        
        QList<ServerPlayer *> all = room->getLieges(player->getKingdom(), NULL);
        room->sortByActionOrder(all);
        foreach(ServerPlayer *p, all)
            p->drawCards(1);
        QList<ServerPlayer *> others;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getKingdom() != player->getKingdom())
                others << p;
        }
        if (others.length() <= 0)
            return false;
        ServerPlayer *target = room->askForPlayerChosen(player, others, objectName(), "@hongfo", false, true);


        LogMessage log;
        log.type = "#hongfoChangeKingdom";
        log.from = target;
        log.arg = player->getKingdom();
        room->sendLog(log);
        room->setPlayerProperty(target, "kingdom", player->getKingdom());
        
        return false;
    }
};


class Junwei : public TriggerSkill
{
public:
    Junwei() : TriggerSkill("junwei")
    {
        frequency = Compulsory;
        events << TargetConfirming;
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.to.contains(player)
                && use.from && use.from != player && use.from->getKingdom() != player->getKingdom()) 
                return QStringList(objectName());    
        } 
        return QStringList();
    }


     virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetConfirming) {
            room->notifySkillInvoked(player, objectName());
            room->touhouLogmessage("#TriggerSkill", player, "junwei");
            CardUseStruct use = data.value<CardUseStruct>();
            use.from->tag["junwei_target"] = QVariant::fromValue(player);
            QString prompt = "@junwei-discard:" + player->objectName() + ":" + use.card->objectName();
            const Card *card = room->askForCard(use.from, ".|black|.|hand,equipped", prompt, data, Card::MethodDiscard);
            if (card == NULL) {
                use.nullified_list << player->objectName();
                data = QVariant::fromValue(use);
            }
        }
        return false;
    }
};

class Gaizong : public TriggerSkill
{
public:
    Gaizong() : TriggerSkill("gaizong")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }
    
    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() != Player::Finish)
            return QStringList();
        if (player->getMark("gaizong") > 0)
            return QStringList();
        QStringList kingdoms;
        int num = 0;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (!kingdoms.contains(p->getKingdom())) {
                kingdoms << p->getKingdom();
                num += 1;
            }
        }
        if (num <= 2) 
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        
        room->addPlayerMark(player, objectName());
        room->doLightbox("$gaizongAnimate", 4000);
        room->touhouLogmessage("#GaizongWake", player, objectName());
        room->notifySkillInvoked(player, objectName());
        if (room->changeMaxHpForAwakenSkill(player)) {
            QString kingdom = room->askForKingdom(player);

            LogMessage log;
            log.type = "#ChooseKingdom";
            log.from = player;
            log.arg = kingdom;
            room->sendLog(log);

            room->setPlayerProperty(player, "kingdom", kingdom);
            room->handleAcquireDetachSkills(player, "-hongfo");
            room->handleAcquireDetachSkills(player, "wendao");
        }
        
        return false;
    }
};

WendaoCard::WendaoCard()
{
    mute = true;
}
bool WendaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!Self->canDiscard(to_select, "h"))
        return false;
    if (targets.length() == 0)
        return true;
    else {
        foreach (const Player *p, targets) {
            if (p->getKingdom() == to_select->getKingdom())
                return false;
        }
        return true;
    }
}
void WendaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->sortByActionOrder(targets);
    foreach (ServerPlayer *p, targets) {
        int card_id = room->askForCardChosen(source, p, "h", objectName());
        room->throwCard(card_id, p, source);
        if (Sanguosha->getCard(card_id)->isRed()) {
            RecoverStruct recover;
            room->recover(source, recover);
        }
    }
}

class WendaoVS : public ZeroCardViewAsSkill
{
public:
    WendaoVS() : ZeroCardViewAsSkill("wendao")
    {
        response_pattern = "@@wendao";
    }

    virtual const Card *viewAs() const
    {
        return new WendaoCard;
    }
};
class Wendao : public TriggerSkill
{
public:
    Wendao() : TriggerSkill("wendao")
    {
        events << EventPhaseStart;
        view_as_skill = new WendaoVS;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Play)
            return QStringList(objectName());
        return QStringList();
        
    }
    
     virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->askForUseCard(player, "@@wendao", "@wendao");
        return false;
    }
};


class Shenbao : public AttackRangeSkill
{
public:
    Shenbao() : AttackRangeSkill("shenbao")
    {

    }

    virtual int getExtra(const Player *target, bool) const
    {
        if (target->hasSkill(objectName())){
            int self_weapon_range = 1;
            int extra = 0;
            if (target->getWeapon()){
                WrappedCard *weapon = target->getWeapon();
                const Weapon *card = qobject_cast<const Weapon *>(weapon->getRealCard());
                self_weapon_range = card->getRange();
            }
            foreach (const Player *p, target->getAliveSiblings()) {
                if (p->getWeapon()){
                    WrappedCard *weapon = p->getWeapon();
                    const Weapon *card = qobject_cast<const Weapon *>(weapon->getRealCard());
                    int other_weapon_range = card->getRange();
                    int new_extra = other_weapon_range - self_weapon_range;
                    if (new_extra > extra)
                        extra = new_extra;                    
                }
            }            
            return extra;
        }
        return 0;
    }
};


class ShenbaoDistance : public DistanceSkill
{
public:
    ShenbaoDistance() : DistanceSkill("#shenbao_distance")
    {
    }

    virtual int getCorrect(const Player *from, const Player *to) const
    {
        int correct = 0;
        if (from->hasSkill("shenbao")){
            foreach (const Player *p, from->getAliveSiblings()) {
                if (p->getOffensiveHorse()){
                    correct = correct - 1;                
                }
            }        
        }
            

        if (to->hasSkill("shenbao")){
            foreach (const Player *p, to->getAliveSiblings()) {
                if (p->getDefensiveHorse()){
                    correct = correct + 1;                
                }
            }    
        }

        return correct;
    }
};





class ShenbaoSpear: public ViewAsSkill {
public:
    ShenbaoSpear(): ViewAsSkill("shenbao_spear") {
        attached_lord_skill = true;
    }

    virtual bool shouldBeVisible(const Player *Self) const{ 
        return Self;
        //return Self->hasWeapon("Spear") && !Self->hasWeapon("Spear", true);
        // need update dashboard when real weapon moved
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        if (player->hasWeapon("Spear") && !player->hasWeapon("Spear" , true)){
            const ViewAsSkill *spear_skill = Sanguosha->getViewAsSkill("Spear");
            return spear_skill->isEnabledAtPlay(player);
        } 
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        if (player->hasWeapon("Spear") && !player->hasWeapon("Spear" , true)){
            
            const ViewAsSkill *spear_skill = Sanguosha->getViewAsSkill("Spear");
            return spear_skill->isEnabledAtResponse(player, pattern);
        } 
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (Self->hasWeapon("Spear") && !Self->hasWeapon("Spear" , true)){
            const ViewAsSkill *spear_skill = Sanguosha->getViewAsSkill("Spear");
            return spear_skill->viewFilter(selected, to_select);
        } 
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (Self->hasWeapon("Spear") && !Self->hasWeapon("Spear" , true)){
            const ViewAsSkill *spear_skill = Sanguosha->getViewAsSkill("Spear");
            return spear_skill->viewAs(cards);
        } 
        return NULL;        
    }
};




class ShenbaoHandler : public TriggerSkill
{
public:
    ShenbaoHandler() : TriggerSkill("#shenbao")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Death << Debut;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!player) return QStringList();
        if (triggerEvent == GameStart || triggerEvent == Debut
            || (triggerEvent == EventAcquireSkill && data.toString() == "shenbao")) {
            QList<ServerPlayer *> kaguyas;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill("shenbao", false, true))
                    kaguyas << p;
            }
            if (kaguyas.isEmpty()) return QStringList();
            foreach (ServerPlayer *p, kaguyas) {
                if (!p->hasSkill("shenbao_spear"))
                    room->attachSkillToPlayer(p, "shenbao_spear");
            }
        } else if (triggerEvent == Death || (triggerEvent == EventLoseSkill && data.toString() == "shenbao")) {
            if (triggerEvent == Death) {
                DeathStruct death = data.value<DeathStruct>();
                if (!death.who->hasSkill("shenbao", false, true))//deal the case that death in round of changshi?
                    return QStringList();
            }
            
            
            QList<ServerPlayer *> losers;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!p->hasSkill("shenbao", false, true))
                    losers << p;
            }
            if (losers.isEmpty()) return QStringList();
            foreach (ServerPlayer *p, losers) {
                if (p->hasSkill("shenbao_spear"))
                    room->detachSkillFromPlayer(p, "shenbao_spear", true);
            }
        }
        return QStringList();
    }
}; 

class Yindu : public TriggerSkill
{
public:
    Yindu() : TriggerSkill("yindu")
    {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DeathStruct death = data.value<DeathStruct>();
        if (death.who && death.who != player ) 
            return QStringList(objectName());
        return QStringList();    
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DeathStruct death = data.value<DeathStruct>();
        QString prompt = "invoke:" + death.who->objectName();
        player->tag["yindu_death"] = data;
        return room->askForSkillInvoke(player, objectName(), prompt);
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *source, QVariant &data, ServerPlayer *) const
    {

        DeathStruct death = data.value<DeathStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), death.who->objectName());
        source->drawCards(3);
        room->setPlayerFlag(death.who, "skipRewardAndPunish");
            
        return false;
    }
};


class Huanming : public TriggerSkill
{
public:
    Huanming() : TriggerSkill("huanming")
    {
        events << DamageCaused;
        frequency = Limited;
        limit_mark = "@huanming";
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.to || player == damage.to
            || player->getMark("@huanming") == 0
            || damage.to->getHp() <= 0)
            return QStringList();
        return QStringList(objectName());
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        player->tag["huanming_damage"] = data;
        return room->askForSkillInvoke(player, objectName(), QVariant::fromValue(damage.to));
    }
    
    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {

        DamageStruct damage = data.value<DamageStruct>();
        
        room->removePlayerMark(player, "@huanming");
        room->doLightbox("$huanmingAnimate", 4000);
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());

        int source_newHp = qMin(damage.to->getHp(), player->getMaxHp());
        int victim_newHp = qMin(player->getHp(), damage.to->getMaxHp());
        room->setPlayerProperty(player, "hp", source_newHp);
        if (damage.to->hasSkill("banling")) {
            room->setPlayerMark(damage.to, "lingtili", victim_newHp);
            room->setPlayerMark(damage.to, "rentili", victim_newHp);
            //room->setPlayerMark(player, "minus_lingtili", minus_x);
            //room->setPlayerMark(player, "minus_rentili", minus_y);
            room->setPlayerProperty(damage.to, "hp", victim_newHp);
        } else
            room->setPlayerProperty(damage.to, "hp", victim_newHp);

        room->touhouLogmessage("#GetHp", player, QString::number(player->getHp()), QList<ServerPlayer *>(), QString::number(player->getMaxHp()));
        room->touhouLogmessage("#GetHp", damage.to, QString::number(damage.to->getHp()), QList<ServerPlayer *>(), QString::number(damage.to->getMaxHp()));
        return true;
    }
};

//the real distance effect is in  Player::distanceTo()
//this triggerskill is for skilleffect
class Chuanwu : public TriggerSkill
{
public:
    Chuanwu() : TriggerSkill("chuanwu")
    {
        events << HpChanged;
        frequency = Compulsory;
    }

    
    
    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (source && source != player) {
            int right = qAbs(source->getSeat() - player->getSeat());
            int left = room->alivePlayerCount() - right;
            int distance = qMin(left, right);
            distance += Sanguosha->correctDistance(source, player);
            if (distance > player->getHp())
                room->notifySkillInvoked(source, objectName());
        }
        return QStringList();
    }
};







TouhouGodPackage::TouhouGodPackage()
    : Package("touhougod")
{
    General *yukari_god = new General(this, "yukari_god", "touhougod", 4, false);
    yukari_god->addSkill(new Jiexian);

    General *remilia_god = new General(this, "remilia_god", "touhougod", 3, false);
    remilia_god->addSkill(new Zhouye);
    remilia_god->addSkill(new ZhouyeChange);
    remilia_god->addSkill(new Hongwu);
    remilia_god->addSkill(new Shenqiang);
    remilia_god->addSkill(new Yewang);
    related_skills.insertMulti("zhouye", "#zhouye_change");

    General *cirno_god = new General(this, "cirno_god", "touhougod", 4, false);
    cirno_god->addSkill(new Aoyi);
    cirno_god->addSkill(new AoyiEffect);
    cirno_god->addSkill(new AoyiTargetMod);
    related_skills.insertMulti("aoyi", "#aoyi_handle");
    related_skills.insertMulti("aoyi", "#aoyi_mod");
    related_skills.insertMulti("aoyi", "#aoyi");

    General *utsuho_god = new General(this, "utsuho_god", "touhougod", 4, false);
    utsuho_god->addSkill(new Shikong);
    utsuho_god->addSkill(new Ronghui);
    utsuho_god->addSkill(new Jubian);
    utsuho_god->addSkill(new Hengxing);

    General *suika_god = new General(this, "suika_god", "touhougod", 0, false);
    suika_god->addSkill(new Huanmeng);
    suika_god->addSkill(new Cuixiang);
    suika_god->addSkill(new Xuying);


    General *flandre_god = new General(this, "flandre_god", "touhougod", 3, false);
    flandre_god->addSkill(new Kuangyan);
    flandre_god->addSkill(new Huimie);
    flandre_god->addSkill(new Jinguo);

    General *sakuya_god = new General(this, "sakuya_god", "touhougod", 3, false);
    sakuya_god->addSkill(new Shicao);
    sakuya_god->addSkill(new Shiting);
    sakuya_god->addSkill(new Huanzai);
    sakuya_god->addSkill(new Shanghun);


    General *youmu_god = new General(this, "youmu_god", "touhougod", 3, false);
    youmu_god->addSkill(new Banling);
    youmu_god->addSkill(new Rengui);
    youmu_god->addSkill(new FakeMoveSkill("rengui"));
    related_skills.insertMulti("rengui", "#rengui-fake-move");

    General *reisen_god = new General(this, "reisen_god", "touhougod", 4, false);
    reisen_god->addSkill(new Ningshi);
    reisen_god->addSkill(new Gaoao);

    General *sanae_god = new General(this, "sanae_god", "touhougod", 4, false);
    sanae_god->addSkill(new Shenshou);

    General *reimu_god = new General(this, "reimu_god", "touhougod", 4, false);
    reimu_god->addSkill(new Yibian);
    reimu_god->addSkill(new Fengyin);
    reimu_god->addSkill(new Huanxiang);
    reimu_god->addSkill(new RoleShownHandler);
    related_skills.insertMulti("huanxiang", "#roleShownHandler");
    related_skills.insertMulti("fengyin", "#roleShownHandler");
    related_skills.insertMulti("Yibian", "#roleShownHandler");

    General *shikieiki_god = new General(this, "shikieiki_god", "touhougod", 4, false);
    shikieiki_god->addSkill(new Quanjie);
    shikieiki_god->addSkill(new Duanzui);
    shikieiki_god->addSkill(new DuanzuiShenpan);
    related_skills.insertMulti("duanzui", "#duanzui-shenpan");


    General *meirin_god = new General(this, "meirin_god", "touhougod", 4, false);
    meirin_god->addSkill(new Huaxiang);
    meirin_god->addSkill(new Caiyu);
    meirin_god->addSkill(new Xuanlan);


    General *eirin_god = new General(this, "eirin_god", "touhougod", 4, false);
    eirin_god->addSkill(new Qiannian);
    eirin_god->addSkill(new QiannianMax);
    related_skills.insertMulti("qiannian", "#qiannian_max");


    General *kanako_god = new General(this, "kanako_god", "touhougod", 4, false);
    kanako_god->addSkill(new Qinlue);
    kanako_god->addSkill(new QinlueEffect);
    related_skills.insertMulti("qinlue", "#qinlue_effect");

    General *byakuren_god = new General(this, "byakuren_god", "touhougod", 4, false);
    byakuren_god->addSkill(new Chaoren);

    General *koishi_god = new General(this, "koishi_god", "touhougod", 3, false);
    koishi_god->addSkill(new Biaoxiang);
    koishi_god->addSkill(new Shifang);
    koishi_god->addSkill(new Yizhi);
    koishi_god->addRelateSkill("ziwo");
    koishi_god->addRelateSkill("benwo");
    koishi_god->addRelateSkill("chaowo");

    General *suwako_god = new General(this, "suwako_god", "touhougod", 5, false);
    suwako_god->addSkill(new Zuosui);
    suwako_god->addSkill(new Worao);
	suwako_god->addSkill(new Shenhua);

    General *miko_god = new General(this, "miko_god", "touhougod", 4, false);
    miko_god->addSkill(new Hongfo);
    miko_god->addSkill(new Junwei);
    miko_god->addSkill(new Gaizong);
    miko_god->addRelateSkill("wendao");

    General *kaguya_god = new General(this, "kaguya_god", "touhougod", 4, false);
    kaguya_god->addSkill(new Shenbao);
    kaguya_god->addSkill(new ShenbaoDistance);
    kaguya_god->addSkill(new ShenbaoHandler);
    related_skills.insertMulti("shenbao", "#shenbao_distance");
    related_skills.insertMulti("shenbao", "#shenbao");
    
    General *komachi_god = new General(this, "komachi_god", "touhougod", 4, false);
    komachi_god->addSkill(new Yindu);
    komachi_god->addSkill(new Huanming);
    komachi_god->addSkill(new Chuanwu);

    addMetaObject<HongwuCard>();
    addMetaObject<ShenqiangCard>();
    addMetaObject<HuimieCard>();
    addMetaObject<ShenshouCard>();
    addMetaObject<FengyinCard>();
    addMetaObject<HuaxiangCard>();
    //addMetaObject<ChaorenPreventRecast>();
    addMetaObject<ZiwoCard>();
    addMetaObject<ChaowoCard>();
    addMetaObject<WendaoCard>();

    skills << new Ziwo << new Benwo << new Chaowo << new Wendao << new ShenbaoSpear;
}

ADD_PACKAGE(TouhouGod)

