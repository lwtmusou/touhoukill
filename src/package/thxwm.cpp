#include "thxwm.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"

//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"





class kongpiao : public TriggerSkill {
public:
    kongpiao() : TriggerSkill("kongpiao") {
        events << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *satori = room->findPlayerBySkillName(objectName());
        if (satori == NULL)
            return false;
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Play && satori->getPhase() == Player::NotActive) {

            ServerPlayer *current = room->getCurrent();
            if (satori->getHandcardNum() < 5 && current->getHandcardNum() > satori->getHandcardNum()){
                room->touhouLogmessage("#TriggerSkill", satori, objectName());
                room->notifySkillInvoked(player, objectName());
                satori->drawCards(1);
            }
        }
        return false;
    }
};

shouhuiCard::shouhuiCard() {
    mute = true;
    target_fixed = true;
    will_throw = true;
}
void shouhuiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    if (source->isAlive())
        room->drawCards(source, subcards.length());
}
class shouhui : public ViewAsSkill {
public:
    shouhui() : ViewAsSkill("shouhui") {
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return (pattern.contains("peach") && player->hasFlag("Global_Dying"));
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() > 0) {
            shouhuiCard *card = new shouhuiCard;
            card->addSubcards(cards);

            return card;
        }
        else
            return NULL;

    }
};


woyuCard::woyuCard() {
    mute = true;
}
void woyuCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *to = card_use.to.at(0);
    room->doLightbox("$woyuAnimate", 4000);
    //this mark is for displaying the revealed true role
    //UI端 photo内的m_player getrole 返回为空。。。 为什么呢? 只有这里具体给出了。。。
    room->setPlayerMark(to, "woyuVictim_" + to->getRole(), 1);
    SkillCard::onUse(room, card_use);
}

void woyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->removePlayerMark(source, "@woyu");
    ServerPlayer *target = targets.first();
    QString role = target->getRole();
    room->touhouLogmessage("#WoyuAnnounce", source, role, room->getAllPlayers(), target->getGeneralName());
    if (role == "rebel")
        source->drawCards(3);
    else if (role == "loyalist")
        source->throwAllHandCardsAndEquips();
}
class woyu : public ZeroCardViewAsSkill {
public:
    woyu() : ZeroCardViewAsSkill("woyu$") {
        frequency = Limited;
        limit_mark = "@woyu";
    }

    virtual const Card *viewAs() const{
        return new woyuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (player->hasLordSkill(objectName(), false))
            return player->getMark("@woyu") >= 1;
        return false;
    }
};


class beisha : public PhaseChangeSkill {
public:
    beisha() :PhaseChangeSkill("beisha") {

    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if (player->getPhase() == Player::Start) {
            if (player->isKongcheng())
                return false;
            Room *room = player->getRoom();
            QList<ServerPlayer *> slashlist;
            QList<ServerPlayer *> losehplist;
            QList<ServerPlayer *> listt;
            int num = player->getHandcardNum();
            foreach(ServerPlayer *p, room->getAllPlayers()){
                if (p->getHandcardNum() <= num && player->canSlash(p, NULL, false)){
                    slashlist << p;
                    listt << p;
                }
                if (p->getHandcardNum() <= (num / 2)){
                    losehplist << p;
                    if (!listt.contains(p))
                        listt << p;
                }
            }
            if (listt.isEmpty())
                return false;
            ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@beisha", true, true);
            if (target == NULL)
                return false;
            QStringList choices;
            if (slashlist.contains(target))
                choices << "useslash";
            if (losehplist.contains(target))
                choices << "losehp";
            QString choice = room->askForChoice(player, objectName(), choices.join("+"));
            if (choice == "useslash"){
                Card *slash = Sanguosha->cloneCard("slash");
                slash->setSkillName("_" + objectName());
                room->useCard(CardUseStruct(slash, player, target));
            }
            else
                room->loseHp(target);
        }
        return false;
    }
};


class xisan : public TriggerSkill {
public:
    xisan() : TriggerSkill("xisan") {
        events << EventPhaseStart;

    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Start){
            ServerPlayer *selfplayer = room->findPlayerBySkillName(objectName());
            if (selfplayer == NULL)
                return false;
            int hd_n = selfplayer->getHandcardNum();
            if (room->askForSkillInvoke(selfplayer, objectName(), data)) {
                QString choice = room->askForChoice(selfplayer, objectName(), "a+b");
                if (choice == "a")
                    selfplayer->drawCards(3 - hd_n);
                else{
                    room->askForDiscard(selfplayer, objectName(), hd_n, 1, true, false, "@xisan-discard");
                    int newhd_n = selfplayer->getHandcardNum();
                    selfplayer->drawCards(2 - newhd_n);
                }
            }
        }
        return false;
    }
};

class jubao : public MasochismSkill {
public:
    jubao() : MasochismSkill("jubao") {

    }


    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getAllPlayers()){
            if (player->canDiscard(p, "h"))
                targets << p;
        }
        if (targets.isEmpty())
            return;
        ServerPlayer *victim = room->askForPlayerChosen(player, targets, objectName(), "@jubao-select", true, true);
        if (victim != NULL){
            int id = room->askForCardChosen(player, victim, "h", objectName(), false, Card::MethodDiscard);
            room->throwCard(id, victim, player);
            Card *dummy = Sanguosha->cloneCard("Slash");
            int count = 0;
            foreach(const Card *c, victim->getEquips()){

                if (player->canDiscard(victim, c->getEffectiveId()) && c->isRed() == Sanguosha->getCard(id)->isRed()){
                    dummy->addSubcard(c);
                    count = count + 1;
                }
            }
            if (count > 0)
                room->throwCard(dummy, victim, player);
        }
    }
};


class haidi : public TriggerSkill {
public:
    haidi() : TriggerSkill("haidi") {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != NULL && move.from == player && move.from_places.contains(Player::PlaceHand)
            && player->isWounded() && move.is_last_handcard){
            if (player->askForSkillInvoke(objectName())){
                RecoverStruct recover;
                recover.who = player;
                player->getRoom()->recover(player, recover);

            }
        }
        return false;
    }
};



class shanji : public TriggerSkill {
public:
    shanji() : TriggerSkill("shanji") {
        events << DrawInitialCards << AfterDrawInitialCards << CardsMoveOneTime;//<<PreCardUsed
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == DrawInitialCards) {
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            data = QVariant::fromValue(data.toInt() + 6);
        }
        else if ((triggerEvent == AfterDrawInitialCards)){
            room->broadcastSkillInvoke("shanji");
            const Card *exchange_card = room->askForExchange(player, "shanji", 6);
            player->addToPile("piao", exchange_card->getSubcards(), false);
        }
        else if (triggerEvent == CardsMoveOneTime){
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!move.from || move.from != player)
                return false;
            if (move.reason.m_skillName == "xijian")
                return false;
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

        return false;
    }
};

yazhiCard::yazhiCard() {
    will_throw = false;
    target_fixed = true;
    mute = true;
}
void yazhiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    source->addToPile("piao", subcards, false);
}
class yazhi : public OneCardViewAsSkill {
public:
    yazhi() :OneCardViewAsSkill("yazhi") {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("yazhiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        yazhiCard *card = new yazhiCard;
        card->addSubcard(originalCard);

        return card;
    }
};


class tymhtianxiang : public TriggerSkill {
public:
    tymhtianxiang() : TriggerSkill("tymhtianxiang$") {
        events << GameStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->hasLordSkill(objectName()) && player->isLord()) {
            room->notifySkillInvoked(player, objectName());
            foreach(ServerPlayer *p, room->getAllPlayers())
                room->setPlayerMark(p, "tianxiang", 1);
        }
        return false;
    }
};


class qingcang : public TriggerSkill {
public:
    qingcang() : TriggerSkill("qingcang") {
        events << DrawNCards << AfterDrawNCards;

    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == DrawNCards) {
            if (player->getCards("j").isEmpty()){
                if (room->askForSkillInvoke(player, "qingcang", data)) {
                    data = QVariant::fromValue(data.toInt() + 4);
                    room->setPlayerFlag(player, "qingcangUsed");
                }
            }
        }
        else if (triggerEvent == AfterDrawNCards && player->hasFlag("qingcangUsed") && !player->isKongcheng()){
            //const Card *card = room->askForCard(player,".|.|.|hand!","@qingcang-card",QVariant(),Card::MethodNone,NULL,false,objectName(),false);
            const Card *card = room->askForExchange(player, objectName(), 1, false, "@qingcang-card");

            Card *supplyshortage = Sanguosha->cloneCard("supply_shortage", card->getSuit(), card->getNumber());
            //WrappedCard *vs_card = Sanguosha->getWrappedCard(card->getId());
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


class changqing : public TriggerSkill {
public:
    changqing() : TriggerSkill("changqing") {
        events << Dying;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{


        ServerPlayer *who = room->getCurrentDyingPlayer();
        if (who != player)
            return false;
        if (room->getAlivePlayers().length() < 5)
            return false;
        room->notifySkillInvoked(player, objectName());
        room->touhouLogmessage("#TriggerSkill", player, objectName());

        room->recover(player, RecoverStruct());
        return false;
    }
};


thxwmPackage::thxwmPackage()
    : Package("thxwm")
{
    General *xwm001 = new General(this, "xwm001$", "dld", 3, false);
    xwm001->addSkill(new kongpiao);
    xwm001->addSkill(new shouhui);
    xwm001->addSkill(new woyu);


    General *xwm002 = new General(this, "xwm002", "yym", 4, false);
    xwm002->addSkill(new beisha);

    General *xwm003 = new General(this, "xwm003", "zhu", 4, false);
    xwm003->addSkill(new xisan);

    General *xwm004 = new General(this, "xwm004", "zhan", 3, false);
    xwm004->addSkill(new jubao);
    xwm004->addSkill(new haidi);

    General *xwm005 = new General(this, "xwm005$", "yyc", 3, false);
    xwm005->addSkill(new shanji);
    xwm005->addSkill(new yazhi);
    xwm005->addSkill(new tymhtianxiang);


    General *xwm006 = new General(this, "xwm006", "hmx", 3, false);
    xwm006->addSkill(new qingcang);
    xwm006->addSkill(new changqing);

    addMetaObject<shouhuiCard>();
    addMetaObject<yazhiCard>();
    addMetaObject<woyuCard>();
}

ADD_PACKAGE(thxwm)

