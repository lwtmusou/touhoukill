#include "thxwm.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"


class Kongpiao : public TriggerSkill
{
public:
    Kongpiao() : TriggerSkill("kongpiao")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {   
        TriggerList skill_list;
        if (player->getPhase() == Player::Play) {
            QList<ServerPlayer *> satoris = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *satori, satoris) {
                if (satori->getHandcardNum() < 5 && player->getHandcardNum() > satori->getHandcardNum())
                    skill_list.insert(satori, QStringList(objectName()));
            }
        }
        return skill_list;
    }
    
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *satori) const
    {
        room->touhouLogmessage("#TriggerSkill", satori, objectName());
        room->notifySkillInvoked(player, objectName());
        satori->drawCards(1);
        return false;
    }
};

ShouhuiCard::ShouhuiCard()
{
    mute = true;
    target_fixed = true;
    will_throw = true;
}
void ShouhuiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    if (source->isAlive())
        room->drawCards(source, subcards.length());
}
class Shouhui : public ViewAsSkill
{
public:
    Shouhui() : ViewAsSkill("shouhui")
    {
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return (pattern.contains("peach") && player->hasFlag("Global_Dying"));
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0) {
            ShouhuiCard *card = new ShouhuiCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;

    }
};


WoyuCard::WoyuCard()
{
    mute = true;
}
void WoyuCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->doLightbox("$woyuAnimate", 4000);
    SkillCard::onUse(room, card_use);
}
bool WoyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return(targets.isEmpty() && !to_select->hasShownRole() && to_select != Self);
}
void WoyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->removePlayerMark(source, "@woyu");
    ServerPlayer *target = targets.first();
    QString role = target->getRole();
    room->touhouLogmessage("#WoyuAnnounce", source, role, room->getAllPlayers(), target->getGeneralName());
	room->broadcastProperty(target, "role");
	room->setPlayerProperty(target, "role_shown", true); //important! to notify client
	
    if (role == "rebel")
        source->drawCards(3);
    else if (role == "loyalist")
        source->throwAllHandCardsAndEquips();
}
class Woyu : public ZeroCardViewAsSkill
{
public:
    Woyu() : ZeroCardViewAsSkill("woyu$")
    {
        frequency = Limited;
        limit_mark = "@woyu";
    }

    virtual const Card *viewAs() const
    {
        return new WoyuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasLordSkill(objectName(), false))
            return player->getMark("@woyu") >= 1;
        return false;
    }
};


class Beisha : public PhaseChangeSkill
{
public:
    Beisha() :PhaseChangeSkill("beisha")
    {
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Start) {
            if (player->isKongcheng())
                return QStringList();
            int num = player->getHandcardNum();
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->getHandcardNum() <= num && player->canSlash(p, NULL, false)) {
                    return QStringList(objectName());
                }
                if (p->getHandcardNum() <= (num / 2)) {
                    return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }
    
    virtual bool onPhaseChange(ServerPlayer *player) const
    { 
        Room *room = player->getRoom();
        QList<ServerPlayer *> slashlist;
        QList<ServerPlayer *> losehplist;
        QList<ServerPlayer *> listt;
        int num = player->getHandcardNum();
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getHandcardNum() <= num && player->canSlash(p, NULL, false)) {
                    slashlist << p;
                    listt << p;
            }
            if (p->getHandcardNum() <= (num / 2)) {
                losehplist << p;
                if (!listt.contains(p))
                   listt << p;
            }
        }
        
        ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@beisha", true, true);
        if (target == NULL)
            return false;
            
        QStringList choices;
        if (slashlist.contains(target))
            choices << "useslash";
        if (losehplist.contains(target))
            choices << "losehp";
        QString choice = room->askForChoice(player, objectName(), choices.join("+"));
        if (choice == "useslash") {
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_" + objectName());
            room->useCard(CardUseStruct(slash, player, target));
        } else
            room->loseHp(target);
        
        return false;
    }
};


class Xisan : public TriggerSkill
{
public:
    Xisan() : TriggerSkill("xisan")
    {
        events << EventPhaseStart;

    }

    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {   
        if (player->getPhase() != Player::Start)
            return TriggerList();

        TriggerList skill_list;
        QList<ServerPlayer *> marisas = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *marisa, marisas) {
                skill_list.insert(marisa, QStringList(objectName()));
        }
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *marisa) const
    {
        return room->askForSkillInvoke(marisa, objectName(), data);
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *marisa) const
    {
        int hd_n = marisa->getHandcardNum();
        QString choice = room->askForChoice(marisa, objectName(), "a+b");
        if (choice == "a")
            marisa->drawCards(3 - hd_n);
        else {
            room->askForDiscard(marisa, objectName(), hd_n, 1, true, false, "@xisan-discard");
            int newhd_n = marisa->getHandcardNum();
            marisa->drawCards(2 - newhd_n);
        }
        return false;
    }
};

class Jubao : public MasochismSkill
{
public:
    Jubao() : MasochismSkill("jubao")
    {

    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (player->canDiscard(p, "h"))
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    
    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const
    {
        Room *room = player->getRoom();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (player->canDiscard(p, "h"))
                targets << p;
        }
        
        ServerPlayer *victim = room->askForPlayerChosen(player, targets, objectName(), "@jubao-select", true, true);
        if (victim) {
            int id = room->askForCardChosen(player, victim, "h", objectName(), false, Card::MethodDiscard);
            room->throwCard(id, victim, player);
            Card *dummy = Sanguosha->cloneCard("Slash");
            int count = 0;
            foreach (const Card *c, victim->getEquips()) {

                if (player->canDiscard(victim, c->getEffectiveId()) && c->isRed() == Sanguosha->getCard(id)->isRed()) {
                    dummy->addSubcard(c);
                    count = count + 1;
                }
            }
            if (count > 0)
                room->throwCard(dummy, victim, player);
        }
    }
};


class Haidi : public TriggerSkill
{
public:
    Haidi() : TriggerSkill("haidi")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != NULL && move.from == player && move.from_places.contains(Player::PlaceHand)
            && player->isWounded() && move.is_last_handcard) 
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName());
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        RecoverStruct recover;
        recover.who = player;
        player->getRoom()->recover(player, recover);
        return false;
    }
};



class Shanji : public TriggerSkill
{
public:
    Shanji() : TriggerSkill("shanji")
    {
        events << DrawInitialCards << AfterDrawInitialCards << CardsMoveOneTime;//<<PreCardUsed
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == DrawInitialCards || triggerEvent == AfterDrawInitialCards) 
            return QStringList(objectName());
        else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!move.from || move.from != player)
                return QStringList();
            if (move.reason.m_skillName == "xijian")
                return QStringList();
            if (player->hasSkill("shanji")) {
                int count = 0;
                for (int i = 0; i < move.card_ids.size(); i++) {
                    if (move.from_pile_names[i] == "piao") count++;
                }
                if (count > 0) {
                    room->notifySkillInvoked(player, "shanji");
                    LogMessage log;
                    log.type = "#shanji";
                    log.from = player;
                    log.arg = QString::number(count);
                    log.arg2 = "piao";
                    room->sendLog(log);
                }
            }
        }
        return QStringList();
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == DrawInitialCards) {
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            data = QVariant::fromValue(data.toInt() + 6);
        } else if ((triggerEvent == AfterDrawInitialCards)) {
            room->broadcastSkillInvoke("shanji");
            const Card *exchange_card = room->askForExchange(player, "shanji", 6);
            player->addToPile("piao", exchange_card->getSubcards(), false);
        } 
        return false;
    }
};

YazhiCard::YazhiCard()
{
    will_throw = false;
    target_fixed = true;
    mute = true;
}
void YazhiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    source->addToPile("piao", subcards, false);
}
class Yazhi : public OneCardViewAsSkill
{
public:
    Yazhi() :OneCardViewAsSkill("yazhi")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("YazhiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        YazhiCard *card = new YazhiCard;
        card->addSubcard(originalCard);

        return card;
    }
};


class Tianxiang : public TriggerSkill
{
public:
    Tianxiang() : TriggerSkill("tianxiang$")
    {
        events << GameStart;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!player) return QStringList();
        if (player->hasLordSkill(objectName()) && player->isLord()) {
            room->notifySkillInvoked(player, objectName());
            foreach(ServerPlayer *p, room->getAllPlayers())
                room->setPlayerMark(p, "tianxiang", 1);
        }
        return QStringList();
    }
};


class Qingcang : public TriggerSkill
{
public:
    Qingcang() : TriggerSkill("qingcang")
    {
        events << DrawNCards << AfterDrawNCards;

    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == DrawNCards) {
            if (player->getCards("j").isEmpty()) 
                 return QStringList(objectName());
        }else if (triggerEvent == AfterDrawNCards && player->hasFlag("qingcangUsed") && !player->isKongcheng())
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == DrawNCards)
            return room->askForSkillInvoke(player, "qingcang", data);
        return true;
    }
    
    
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == DrawNCards) {
            data = QVariant::fromValue(data.toInt() + 4);
            room->setPlayerFlag(player, "qingcangUsed");      
        } else if (triggerEvent == AfterDrawNCards) {
            const Card *card = room->askForExchange(player, objectName(), 1, false, "@qingcang-card");

            Card *supplyshortage = Sanguosha->cloneCard("supply_shortage", card->getSuit(), card->getNumber());
            WrappedCard *vs_card = Sanguosha->getWrappedCard(card->getSubcards().first());
            vs_card->setSkillName("_qingcang");
            vs_card->takeOver(supplyshortage);
            room->broadcastUpdateCard(room->getAlivePlayers(), vs_card->getId(), vs_card);
            CardsMoveStruct  move;
            move.card_ids << vs_card->getId();
            move.to = player;
            move.to_place = Player::PlaceDelayedTrick;
            room->moveCardsAtomic(move, true);
        }
        return false;
    }
};


class Changqing : public TriggerSkill
{
public:
    Changqing() : TriggerSkill("changqing")
    {
        events << Dying;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        ServerPlayer *who = room->getCurrentDyingPlayer();
        if (who != player)
            return QStringList();
        if (room->getAlivePlayers().length() < 5)
            return QStringList();
        return QStringList(objectName());
    }
    
     virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->notifySkillInvoked(player, objectName());
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->recover(player, RecoverStruct());
        return false;
    }
};


THXWMPackage::THXWMPackage()
    : Package("thxwm")
{
    General *satori_xwm = new General(this, "satori_xwm$", "dld", 3, false);
    satori_xwm->addSkill(new Kongpiao);
    satori_xwm->addSkill(new Shouhui);
    satori_xwm->addSkill(new Woyu);


    General *yuyuko_xwm = new General(this, "yuyuko_xwm", "yym", 4, false);
    yuyuko_xwm->addSkill(new Beisha);

    General *marisa_xwm = new General(this, "marisa_xwm", "zhu", 4, false);
    marisa_xwm->addSkill(new Xisan);

    General *aya_xwm = new General(this, "aya_xwm", "zhan", 3, false);
    aya_xwm->addSkill(new Jubao);
    aya_xwm->addSkill(new Haidi);

    General *mokou_xwm = new General(this, "mokou_xwm$", "yyc", 3, false);
    mokou_xwm->addSkill(new Shanji);
    mokou_xwm->addSkill(new Yazhi);
    mokou_xwm->addSkill(new Tianxiang);


    General *remilia_xwm = new General(this, "remilia_xwm", "hmx", 3, false);
    remilia_xwm->addSkill(new Qingcang);
    remilia_xwm->addSkill(new Changqing);

    addMetaObject<ShouhuiCard>();
    addMetaObject<YazhiCard>();
    addMetaObject<WoyuCard>();
}

ADD_PACKAGE(THXWM)

