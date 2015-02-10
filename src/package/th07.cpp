#include "th07.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"




class sidie : public TriggerSkill {
public:
    sidie() : TriggerSkill("sidie") {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        if (player->getPhase() != Player::Play)
            return false;
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user)
            return false;
        if (damage.from == NULL || damage.to == NULL || damage.from == damage.to)
            return false;

        if (player->hasFlag("sidie_used"))
            return false;

        if (damage.card != NULL && damage.card->isKindOf("Slash")){
            Slash *slash = new Slash(Card::NoSuit, 0);
            if (damage.to->isCardLimited(slash, Card::MethodUse))
                return false;
            QList<ServerPlayer *> listt;
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (damage.to->canSlash(p, slash, false))
                    listt << p;
            }
            if (listt.length() <= 0)
                return false;


            player->tag["sidie_target"] = QVariant::fromValue(damage.to);

            ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@sidie:" + damage.to->objectName(), true, true);

            if (target != NULL){
                room->setPlayerFlag(player, "sidie_used");
                QList<ServerPlayer *> logto;
                logto << damage.to;
                room->touhouLogmessage("#Dongjie", player, "sidie", logto);
                player->drawCards(2);
                slash->setSkillName("_" + objectName());

                room->useCard(CardUseStruct(slash, damage.to, target), false);

                return true;

            }
            player->tag.remove("sidie_target");

        }
        return false;
    }
};

class sidie_clear : public TriggerSkill {
public:
    sidie_clear() : TriggerSkill("#sidie_clear") {
        events << EventPhaseChanging;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        foreach(ServerPlayer *p, room->getAlivePlayers()){
            if (p->hasFlag("sidie_used"))
                room->setPlayerFlag(player, "-sidie_used");
        }
        return false;
    }
};

class wangxiang : public TriggerSkill {
public:
    wangxiang() : TriggerSkill("wangxiang$") {
        events << Damaged << FinishJudge;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL && target->isAlive() && target->getKingdom() == "yym");
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Damaged) {
            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasLordSkill(objectName()))
                    targets << p;
            }

            while (!targets.isEmpty()){
                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@@wangxiang", true, true);
                if (target != NULL){
                    room->notifySkillInvoked(target, objectName());
                    targets.removeOne(target);

                    player->tag["uuz_wangxiang"] = QVariant::fromValue(target);

                    JudgeStruct judge;
                    judge.pattern = ".|black";
                    judge.who = player;
                    judge.reason = objectName();
                    judge.good = true;

                    room->judge(judge);

                    player->tag.remove("uuz_wangxiang");
                } else
                    break;

            }

        } else if (triggerEvent == FinishJudge) {
            JudgeStar judge = data.value<JudgeStar>();
            if (judge->reason == objectName() && judge->isGood()){
                ServerPlayer *uuz = judge->who->tag["uuz_wangxiang"].value<ServerPlayer *>();
                if (uuz != NULL)
                    uuz->obtainCard(judge->card);
            }
        }

        return false;
    }
};

class jingjie : public TriggerSkill {
public:
    jingjie() : TriggerSkill("jingjie") {
        frequency = Frequent;
        events << Damaged << AfterDrawNCards;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        int num = 1;
        if (triggerEvent == AfterDrawNCards)
            num = 1;
        else if (triggerEvent == Damaged)
            num = data.value<DamageStruct>().damage;

        for (int i = 0; i < num; i++) {
            if (!player->askForSkillInvoke(objectName(), data))
                break;
            player->drawCards(1);
            if (!player->isKongcheng()){
                const Card *cards = room->askForExchange(player, objectName(), 1, false, "jingjie_exchange");
                int id = cards->getSubcards().first();
                player->addToPile("jingjie", id);
            }
        }

        return false;
    }
};


class sisheng : public TriggerSkill {
public:
    sisheng() : TriggerSkill("sisheng") {
        events << Dying;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *who = room->getCurrentDyingPlayer();
        if (who->isNude())
            return false;

        ServerPlayer *yukari = room->findPlayerBySkillName(objectName());
        if (yukari == NULL)
            return false;
        QList<int> pile = yukari->getPile("jingjie");
        if (pile.length() > 0 && yukari->canDiscard(who, "he") && who->getHp() < 1){
            //while (yukari->canDiscard(who, "he") && yukari->getPile("jingjie").length()>0 && who->getHp()<1 ) {
            if (yukari->askForSkillInvoke(objectName(), data)) {
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
            }
            //    else
            //        break;
            //}

        }
        return false;
    }
};

class jingdong : public TriggerSkill {
public:
    jingdong() : TriggerSkill("jingdong") {
        events << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();

        if (change.to == Player::Discard){
            ServerPlayer *s = room->findPlayerBySkillName(objectName());
            if (s == NULL)
                return false;
            QList<int> pile = s->getPile("jingjie");
            if (pile.isEmpty())
                return false;
            s->tag["jingdong_target"] = QVariant::fromValue(player);
            QString prompt = "target:" + player->objectName();
            if (room->askForSkillInvoke(s, objectName(), prompt)) {
                room->fillAG(pile, s);
                int id = room->askForAG(s, pile, false, objectName());
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
                room->throwCard(Sanguosha->getCard(id), reason, NULL);
                room->clearAG(s);
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, s->objectName(), player->objectName());
            
                player->skip(Player::Discard);
            }
            s->tag.remove("jingdong_target");
        }
        return false;
    }
};


zhaoliaoCard::zhaoliaoCard() {
    //mute = true;
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void zhaoliaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{

}

class zhaoliaovs : public ViewAsSkill {
public:
    zhaoliaovs() : ViewAsSkill("zhaoliao") {
        response_pattern = "@@zhaoliao";
    }
    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() > 0) {
            zhaoliaoCard *card = new zhaoliaoCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;

    }
};

class zhaoliao : public TriggerSkill {
public:
    zhaoliao() : TriggerSkill("zhaoliao") {
        events << Damaged;
        view_as_skill = new zhaoliaovs;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *s = room->findPlayerBySkillName(objectName());
        if (s == NULL)
            return false;
        ServerPlayer *a = data.value<DamageStruct>().to;
        if (a->isAlive() && a != s && !s->isNude()){
            s->tag["zhaoliao_target"] = QVariant::fromValue(a);

            const Card *cards = room->askForCard(s, "@@zhaoliao", "@zhaoliao:" + a->objectName(), data, Card::MethodNone);

            if (cards != NULL){
                room->notifySkillInvoked(s, objectName());
                QList<ServerPlayer *> logto;
                logto << player;
                room->touhouLogmessage("#ChoosePlayerWithSkill", s, "zhaoliao", logto);

                room->obtainCard(player, cards, false);
                QString choice;
                ExNihilo *exnihilo = new ExNihilo(Card::SuitToBeDecided, -1);
                if (player->isKongcheng()){
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

        }
        return false;
    }
};


class jiaoxia : public TriggerSkill {
public:
    jiaoxia() : TriggerSkill("jiaoxia") {
        events << Dying;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{


        ServerPlayer *who = room->getCurrentDyingPlayer();
        if (who != player)
            return false;

        if (player->getEquips().length() > 0)
            return false;

        if (player->askForSkillInvoke(objectName(), data)){
            JudgeStruct judge;
            judge.reason = objectName();
            judge.who = player;
            judge.good = false;
            judge.pattern = ".|heart";

            room->judge(judge);
            if (judge.isGood()){
                RecoverStruct recover;
                recover.recover = 1;
                recover.who = player;
                room->recover(player, recover);
            }
        }
        return false;
    }
};


class jianshu : public FilterSkill {
public:
    jianshu() : FilterSkill("jianshu") {
    }

    virtual bool viewFilter(const Card *to_select) const{
        Room *room = Sanguosha->currentRoom();
        if (room->getCardPlace(to_select->getEffectiveId()) == Player::PlaceHand){
            ServerPlayer *youmu = room->getCardOwner(to_select->getEffectiveId());
            if (youmu != NULL && youmu->hasSkill(objectName())){
                return  to_select->isKindOf("Weapon");
            }
        }
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class jianshuTargetMod : public TargetModSkill {
public:
    jianshuTargetMod() : TargetModSkill("#jianshuTargetMod") {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *card) const{
        if (from->hasSkill("jianshu") && card->isKindOf("NatureSlash"))
            return 1000;
        else
            return 0;
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (from->hasSkill("jianshu") && !card->isKindOf("NatureSlash"))
            return 1000;
        else
            return 0;
    }
};

class jianshuWeapon : public TriggerSkill {
public:
    jianshuWeapon() : TriggerSkill("#jianshu") {
        frequency = Compulsory;
        events << CardsMoveOneTime << EventAcquireSkill;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QList<int> t_ids;
            if (move.to != NULL && move.to == player && move.to_place == Player::PlaceEquip){
                foreach(int id, move.card_ids){
                    if (Sanguosha->getCard(id)->isKindOf("Weapon"))
                        t_ids << id;
                }
                if (t_ids.length() > 0){
                    room->touhouLogmessage("#JianshuUninstall", player, "jianshu");
                    foreach(int id, t_ids){
                        room->throwCard(id, player, player);
                    }
                }
            }
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != "jianshu")
                return false;
            if (player->hasSkill(objectName())){
                QList<int> weapon1;
                foreach(const Card *card, player->getCards("e")) {
                    if (card->isKindOf("Weapon"))
                        weapon1 << card->getId();
                }
                if (weapon1.length() > 0){
                    room->touhouLogmessage("#JianshuUninstall", player, "jianshu");
                    foreach(int id, weapon1){
                        room->throwCard(id, player, player);
                    }
                }
            }

        }
        return false;
    }
};


class louguan : public TriggerSkill {
public:
    louguan() : TriggerSkill("louguan") {
        frequency = Compulsory;
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from == player && use.card != NULL && use.card->isKindOf("Slash") && !use.card->isRed()){
            foreach(ServerPlayer *p, use.to) {
                p->addQinggangTag(use.card);
            }
            room->touhouLogmessage("#TriggerSkill", player, "louguan");
            room->notifySkillInvoked(player, objectName());
            room->setEmotion(player, "weapon/qinggang_sword");

        }
        return false;
    }
};


class bailou : public TriggerSkill {
public:
    bailou() : TriggerSkill("bailou") {
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from == player && use.card != NULL && use.card->isKindOf("Slash") && !use.card->isBlack()){
            foreach(ServerPlayer *p, use.to) {
                QVariant _data = QVariant::fromValue(p);
                if (player->canDiscard(p, "h") && room->askForSkillInvoke(player, objectName(), _data)){
                    room->setEmotion(player, "weapon/ice_sword");
                    room->throwCard(room->askForCardChosen(player, p, "h", objectName(), false, Card::MethodDiscard), p, player);
                }
            }
        }
        return false;
    }
};



class xiezouvs : public ZeroCardViewAsSkill {
public:
    xiezouvs() : ZeroCardViewAsSkill("xiezou") {
        response_pattern = "@@xiezou";
    }

    virtual const Card *viewAs() const{
        QString cardname = Self->property("xiezou_card").toString();
        Card *card = Sanguosha->cloneCard(cardname);

        card->setSkillName("xiezou");
        return card;
    }
};


class xiezou : public TriggerSkill {
public:
    xiezou() : TriggerSkill("xiezou") {
        events << EventPhaseEnd << PreCardUsed << CardResponded << EventPhaseChanging;
        view_as_skill = new xiezouvs;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseEnd){
            if (player->getPhase() == Player::Play){
                QString cardname = player->property("xiezou_card").toString();
                Card *card = Sanguosha->cloneCard(cardname);
                if (card == NULL)
                    return false;
                if (card->isKindOf("Slash") || card->isKindOf("Peach") || card->isNDTrick()){
                    if (player->isCardLimited(card, Card::MethodUse))
                        return false;
                    if (card->isKindOf("Peach") && !player->isWounded())
                        return false;
                    QString prompt = "@xiezou:" + card->objectName();
                    room->askForUseCard(player, "@@xiezou", prompt);
                }
            }
        } else if (triggerEvent == PreCardUsed || triggerEvent == CardResponded){
            if (player->getPhase() == Player::Play){
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
        } else if (triggerEvent == EventPhaseChanging){
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play){
                room->setPlayerProperty(player, "xiezou_card", QVariant());
            }
        }
        return false;
    }
};


class hesheng : public TriggerSkill {
public:
    hesheng() : TriggerSkill("hesheng") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        bool can = false;
        foreach(ServerPlayer *p, (room->getAlivePlayers())){
            if (p->getCards("j").length() > 0){
                can = true;
                break;
            }
        }
        if (can){
            room->touhouLogmessage("#hesheng", player, "hesheng", QList<ServerPlayer *>(), QString::number(damage.damage));
            room->notifySkillInvoked(player, objectName());
            return true;
        }
        return false;
    }
};


class renou : public TriggerSkill {
public:
    renou() : TriggerSkill("renou") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer  *current = room->getCurrent();
        if (!current || current->getPhase() != Player::Finish)
            return false;
        ServerPlayer *source = room->findPlayerBySkillName(objectName());

        if (source != NULL && source->getEquips().length() < 4){
            if (!room->askForSkillInvoke(source, objectName(), data))
                return false;
            room->notifySkillInvoked(source, objectName());
            QList<int> list = room->getNCards(5);
            QList<int> able;
            QList<int> disabled;
            foreach(int id, list){
                Card *tmp_card = Sanguosha->getCard(id);
                if (tmp_card->isKindOf("EquipCard")){
                    const EquipCard *tmp_equip = qobject_cast<const EquipCard *>(tmp_card->getRealCard());
                    if (source->getEquip(tmp_equip->location()))
                        disabled << id;
                    else
                        able << id;
                }
                else
                    disabled << id;
            }
            room->fillAG(list, NULL, disabled);

            QStringList cardinfo;
            foreach(int id, list){
                cardinfo << Sanguosha->getCard(id)->toString();
            }

            LogMessage mes;
            mes.type = "$TurnOver";
            mes.from = source;
            mes.card_str = cardinfo.join("+");
            room->sendLog(mes);
            int equipid = -1;
            if (!able.isEmpty())
                equipid = room->askForAG(source, able, true, objectName());

            if (equipid != -1){
                CardsMoveStruct move(equipid, source, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_PUT, objectName()));
                room->moveCardsAtomic(move, true);

                LogMessage mes2;
                mes2.type = "$renou_put";
                mes2.from = source;
                mes2.card_str = Sanguosha->getCard(equipid)->toString();
                room->sendLog(mes2);

                list.removeOne(equipid);
                cardinfo.removeOne(mes2.card_str);
            }
            qShuffle(list);
            room->askForGuanxing(source, list, Room::GuanxingDownOnly, objectName());
            LogMessage mes1;
            mes1.type = "$renou_movedown";
            mes1.from = source;
            mes1.arg = QString::number(list.length());
            mes1.card_str = cardinfo.join("+");
            room->sendLog(mes1);

            room->getThread()->delay(1000);
            room->clearAG();
        }
        return false;
    }
};


class junshi : public OneCardViewAsSkill {
public:
    junshi() : OneCardViewAsSkill("junshi") {
        filter_pattern = "EquipCard";
        response_or_use = true;
    }


    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "jink" || pattern == "slash";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard != NULL){
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "jink"){
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
        }
        else
            return NULL;
    }
};

class shishen : public TriggerSkill {
public:
    shishen() : TriggerSkill("shishen") {
        events << EventPhaseStart << Damaged;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if ((triggerEvent == EventPhaseStart &&  player->getPhase() == Player::Start) || triggerEvent == Damaged) {
            if (player->getMark("@shi") > 0){
                if (triggerEvent == Damaged)
                    player->setFlags("shishen_choice");//for ai
                QString choice = room->askForChoice(player, objectName(), "shishen1+cancel");
                player->setFlags("-shishen_choice");
                if (choice == "shishen1") {
                    player->loseMark("@shi");
                    room->notifySkillInvoked(player, objectName());
                }
            }
        } else if (triggerEvent == EventPhaseStart &&  player->getPhase() == Player::Play) {
            if (player->getMark("@shi") > 0)
                return false;
            QString choice = room->askForChoice(player, objectName(), "shishen2+cancel");
            if (choice == "shishen2"){
                player->gainMark("@shi", 1);
                room->notifySkillInvoked(player, objectName());
            }
        }
        return false;
    }
};

class yexing : public AttackRangeSkill{
public:
    yexing() : AttackRangeSkill("yexing"){

    }

    virtual int getExtra(const Player *target, bool) const{
        if (target->hasSkill(objectName()) && target->getMark("@shi") == 0)
            return 1;
        return 0;
    }
};

class yexing_effect : public TriggerSkill {
public:
    yexing_effect() : TriggerSkill("#yexing") {
        frequency = Compulsory;
        events << GameStart << PreMarkChange << CardEffected << SlashEffected << EventAcquireSkill << EventLoseSkill << MarkChanged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == GameStart || (triggerEvent == EventAcquireSkill && data.toString() == "yexing")){
            if (player->hasSkill("yexing") && player->getMark("yexing_limit") == 0) {
                room->setPlayerMark(player, "yexing_limit", 1);
                room->setPlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick", false);
            }
        }
        if (triggerEvent == EventLoseSkill && data.toString() == "yexing"){
            if (!player->hasSkill("yexing") && player->getMark("yexing_limit") > 0){
                room->setPlayerMark(player, "yexing_limit", 0);
                room->removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0");
            }
        }
		else if (triggerEvent == MarkChanged){
			MarkChangeStruct change = data.value<MarkChangeStruct>();
			if (change.name == "@changshi"||change.name == "@pingyi" ) {
				if (!player->hasSkill("yexing") && player->getMark("yexing_limit") > 0) {
					room->setPlayerMark(player, "yexing_limit", 0);
					room->removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0");
				}
				else if (player->hasSkill("yexing")) {
					if (player->getMark("@shi")>0 && player->getMark("yexing_limit") > 0){
						room->setPlayerMark(player, "yexing_limit", 0);
						room->removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0");
					}
					else if (player->getMark("@shi")==0 && player->getMark("yexing_limit") == 0){
						room->setPlayerMark(player, "yexing_limit", 1);
						room->setPlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick", false);
					}
				}
			}
		}
        if (!player->hasSkill("yexing"))
            return false;
        if (triggerEvent == PreMarkChange) {
            MarkChangeStruct change = data.value<MarkChangeStruct>();
            if (change.name != "@shi")
                return false;
            int mark = player->getMark("@shi");

            if (mark > 0 && (mark + change.num == 0) && player->getMark("yexing_limit") == 0) {
                room->setPlayerMark(player, "yexing_limit", 1);
                room->setPlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick", false);
            }
            else if (mark == 0 && (mark + change.num > 0) && player->getMark("yexing_limit") > 0){
                room->setPlayerMark(player, "yexing_limit", 0);
                room->removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0");
            }
        }
        if (triggerEvent == SlashEffected){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.nature == DamageStruct::Normal && player->getMark("@shi") == 0){
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.slash->objectName(), QList<ServerPlayer *>(), "yexing");
                room->notifySkillInvoked(player, "yexing");
                room->setEmotion(effect.to, "armor/vine");
                return true;
            }
        }
        if (triggerEvent == CardEffected){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if ((effect.card->isKindOf("SavageAssault") || effect.card->isKindOf("ArcheryAttack")) && player->getMark("@shi") == 0) {
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.card->objectName(), QList<ServerPlayer *>(), "yexing");
                room->notifySkillInvoked(player, "yexing");
                room->setEmotion(effect.to, "armor/vine");
                return true;
            }
        }

        return false;
    }
};


class yaoshuvs : public ZeroCardViewAsSkill {
public:
    yaoshuvs() : ZeroCardViewAsSkill("yaoshu") {
        response_pattern = "@@yaoshu";
    }

    virtual const Card *viewAs() const{
        QString cardname = Self->property("yaoshu_card").toString();
        Card *card = Sanguosha->cloneCard(cardname);

        card->setSkillName("yaoshu");
        return card;
    }
};

class yaoshu : public TriggerSkill {
public:
    yaoshu() : TriggerSkill("yaoshu") {
        events << CardFinished;
        view_as_skill = new yaoshuvs;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card != NULL && use.card->isNDTrick() && !use.card->isKindOf("Nullification")){
            if (use.card->getSkillName() == "yaoshu"){
                room->setPlayerProperty(player, "yaoshu_card", QVariant());
                return false;
            }
            if (use.card->isVirtualCard() && use.card->getSubcards().length() == 0)
                return false;



            if (player->isCardLimited(Sanguosha->cloneCard(use.card->objectName()), Card::MethodUse))
                return false;

            room->setPlayerProperty(player, "yaoshu_card", use.card->objectName());
            room->askForUseCard(player, "@@yaoshu", "@yaoshu:" + use.card->objectName());

        }
        return false;
    }
};




class jiyi : public TriggerSkill {
public:
    jiyi() : TriggerSkill("jiyi") {
        events << TurnedOver;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        if (player->askForSkillInvoke(objectName(), data)){
            player->drawCards(2);
            room->askForRende(player, player->handCards(), objectName(), false, true, qMin(2, player->getHandcardNum()));
        }
        return false;
    }
};

class chunmian : public TriggerSkill {
public:
    chunmian() : TriggerSkill("chunmian") {
        frequency = Compulsory;
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Finish){
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            player->turnOver();
        }

        return false;
    }
};



class baochun : public TriggerSkill {
public:
    baochun() : TriggerSkill("baochun") {
        events << Damaged;
    }


    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@" + objectName() + ":" + QString::number(player->getLostHp()), true, true);

        if (target)
            target->drawCards(player->getLostHp());

        return false;
    }
};


class chunyi : public TriggerSkill {
public:
    chunyi() : TriggerSkill("chunyi") {
        frequency = Compulsory;
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Start && player->getMaxHp() < 6){
            room->notifySkillInvoked(player, objectName());

            room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->touhouLogmessage("#GainMaxHp", player, QString::number(1));
            room->touhouLogmessage("#GetHp", player, QString::number(player->getHp()), QList<ServerPlayer *>(), QString::number(player->getMaxHp()));


        }
        return false;
    }
};


class zhancao : public TriggerSkill {
public:
    zhancao() : TriggerSkill("zhancao") {
        events << TargetConfirmed << SlashEffected; //TargetConfirming 
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL && target->isAlive());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                foreach(ServerPlayer *to, use.to) {
                    if (!player->isAlive()) break;
                    if (player->inMyAttackRange(to) && player->hasSkill("zhancao")){
                        player->tag["zhancao_carduse"] = data;
                        player->tag["zhancao_target"] = QVariant::fromValue(to);
                        QString prompt = "target:" + use.from->objectName() + ":" + to->objectName();
                        if (room->askForSkillInvoke(player, objectName(), prompt)){
                            room->setCardFlag(use.card, "zhancao" + to->objectName());
                            if (room->askForCard(player, ".Equip", "@zhancao-discard", data, objectName()) == NULL)
                                room->loseHp(player);
                            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), to->objectName());
            
                        }
                        player->tag.remove("zhancao_carduse");
                    }
                }
            }
        }

        /*if (triggerEvent == TargetConfirming ) {
            CardUseStruct use = data.value<CardUseStruct>();
            ServerPlayer *shrx =room->findPlayerBySkillName(objectName());
            if (shrx != NULL && shrx->inMyAttackRange(player) && use.card !=NULL && use.card->isKindOf("Slash")){
            shrx->tag["zhancao_carduse"] =data;
            shrx->tag["zhancao_target"] =QVariant::fromValue(player);
            QString prompt="target:"+use.from->objectName()+":"+player->objectName();
            if (room->askForSkillInvoke(shrx,objectName(), prompt) ){
            room->setCardFlag(use.card, "zhancao"+player->objectName());
            if (room->askForCard(shrx, ".Equip", "@zhancao-discard") ==NULL)
            room->loseHp(shrx);
            }
            shrx->tag.remove("zhancao_carduse");
            }

            }*/
        else if (triggerEvent == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash != NULL && effect.slash->hasFlag("zhancao" + effect.to->objectName())){
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.slash->objectName(), QList<ServerPlayer *>(), objectName());
                room->setEmotion(effect.to, "skill_nullify");
                return true;
            }

        }
        return false;
    }
};



mocaoCard::mocaoCard() {
    mute = true;
}

bool mocaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || to_select == Self)
        return false;
    return !to_select->getEquips().isEmpty();
}
void mocaoCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    int card_id = room->askForCardChosen(effect.from, effect.to, "e", "mocao");
    room->obtainCard(effect.from, card_id);
    if (effect.to->isWounded())
        effect.to->drawCards(effect.to->getLostHp());
}


class mocao : public ZeroCardViewAsSkill {
public:
    mocao() : ZeroCardViewAsSkill("mocao") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("mocaoCard");
    }

    virtual const Card *viewAs() const{
        return new mocaoCard;
    }
};



class shenyin : public TriggerSkill {
public:
    shenyin() : TriggerSkill("shenyin") {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *to = damage.to;
        if (!to->isNude()){
            QVariant _data = QVariant::fromValue(to);
            player->tag["shenyin_damage"] = data;
            if (room->askForSkillInvoke(player, objectName(), _data)) {
                int to_throw = room->askForCardChosen(player, to, "he", objectName());
                player->addToPile("yin_mark", to_throw);
                if (!to->isNude()){
                    int to_throw1 = room->askForCardChosen(player, to, "he", objectName());
                    player->addToPile("yin_mark", to_throw1);
                }
                return true;
            }
        }
        return false;
    }
};

class xijian : public TriggerSkill {
public:
    xijian() : TriggerSkill("xijian") {
        events << EventPhaseStart << Damaged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        ServerPlayer  *current = room->getCurrent();
        bool can_invoke = false;

        if (triggerEvent == EventPhaseStart && current && current->getPhase() == Player::Finish){
            if (player != current)
                return false;
            can_invoke = true;
        } else if (triggerEvent == Damaged){
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.to || !damage.to->isAlive())
                return false;
            current = damage.to;
            can_invoke = true;
        }
        ServerPlayer *yukari = room->findPlayerBySkillName(objectName());
        if (yukari == NULL || !can_invoke)
            return false;
        QList<ServerPlayer *> plist;
        foreach(ServerPlayer *liege, room->getAlivePlayers()) {
            int num = 0;

            foreach(QString pile, liege->getPileNames()){
                if (pile != "suoding_cards" && pile != "jiejie_right" &&  pile != "jiejie_left" && pile != "wooden_ox")
                    num = num + liege->getPile(pile).length();
            }
            if (num > 0)
                plist << liege;
        }
        yukari->tag["xijian_target"] = QVariant::fromValue(current);

        if (plist.length() > 0){
            ServerPlayer *player_haspile = room->askForPlayerChosen(yukari, plist, "xijian", "@xijian:" + current->objectName(), true, true);
            if (player_haspile != NULL){
                QList<int> idlist;
                foreach(QString pile, player_haspile->getPileNames()){
                    if (pile != "suoding_cards" && pile != "jiejie_right" && pile != "jiejie_left" && pile != "wooden_ox"){
                        foreach(int id, player_haspile->getPile(pile)){
                            idlist << id;
                        }
                    }
                }

                room->fillAG(idlist, yukari);
                int card_id = room->askForAG(yukari, idlist, true, objectName());
                room->clearAG(yukari);
                bool can_open = false;
                QString pile_name;
                foreach(QString pile, player_haspile->getPileNames()){
                    if (player_haspile->getPile(pile).contains(card_id)){
                        pile_name = pile;
                        break;
                    }
                }
                if (pile_name != NULL && player_haspile->pileOpen(pile_name, yukari->objectName()))
                    can_open = true;
                //need reason to slove the disambugation of the move of piao is
                //xijian a "piao" or using a "piao"
                CardMoveReason reason(CardMoveReason::S_REASON_GOTBACK, current->objectName(), "xijian", "");
                room->obtainCard(current, Sanguosha->getCard(card_id), reason, can_open);
            }
        }
        yukari->tag.remove("xijian_target");
        return false;
    }
};


class youqu : public TriggerSkill {
public:
    youqu() : TriggerSkill("youqu") {
        frequency = Compulsory;
        events << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{


        if (player->getPhase() == Player::Start){
            QList<int> sl = player->getPile("siling");
            if (sl.length() >= 2){
                room->notifySkillInvoked(player, objectName());

                room->touhouLogmessage("#TriggerSkill", player, objectName());

                CardsMoveStruct move(sl, player, player, Player::PlaceSpecial, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_UNKNOWN, QString()));
                room->moveCardsAtomic(move, true);

                room->touhouLogmessage("#silinggain", player, objectName(), QList<ServerPlayer *>(), QString::number(sl.length()));
                room->damage(DamageStruct("siling", player, player));
            }
        } else if (player->getPhase() == Player::Finish){
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

class wangwu : public TriggerSkill {
public:
    wangwu() : TriggerSkill("wangwu") {
        events << TargetConfirming;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card == NULL)
            return false;
        if (use.card->isKindOf("Slash") || use.card->isNDTrick()){
            if (!use.card->isRed() && !use.card->isBlack())
                return false;
            if (use.to.contains(player)){
                QList<int> list = player->getPile("siling");
                if (!list.isEmpty() && use.from->isAlive()){
                    player->tag["wangwu_use"] = data;
                    QString prompt = "invoke:" + use.from->objectName() + ":" + use.card->objectName();
                    if (player->askForSkillInvoke(objectName(), prompt)){
                        QList<int> same;
                        QList<int> disabled;
                        foreach(int id, list){
                            if (Sanguosha->getCard(id)->sameColorWith(use.card))
                                same << id;
                            else
                                disabled << id;
                        }
                        room->fillAG(list, player, disabled);
                        player->tag["wangwu_card"] = QVariant::fromValue(use.card);
                        if (same.isEmpty()){
                            //give a delay to avoid lacking "siling" shortage  
                            room->getThread()->delay(1000);
                            room->clearAG(player);
                        } else {
                            int id = room->askForAG(player, same, true, objectName());
                            room->clearAG(player);
                            if (id > -1){
                                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
                                room->throwCard(Sanguosha->getCard(id), reason, NULL);
                                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.from->objectName());
            
                                player->drawCards(1);

                                room->damage(DamageStruct("wangwu", player, use.from));
                            }
                        }
                        player->tag.remove("wangwu_card");
                    }
                }
            }

        }
        return false;
    }
};


class hpymsiyu : public TriggerSkill {
public:
    hpymsiyu() : TriggerSkill("hpymsiyu") {
        events << PostHpReduced;
        frequency = Compulsory;
    }

    static void touhou_siyu_clear(ServerPlayer *player) {
        Room *room = player->getRoom();
        room->clearAG();

        QVariantList ag_list = room->getTag("AmazingGrace").toList();
        room->removeTag("AmazingGrace");
        if (!ag_list.isEmpty()){
            DummyCard *dummy = new DummyCard(VariantList2IntList(ag_list));
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), "amazing_grace", QString());
            room->throwCard(dummy, reason, NULL);
            delete dummy;
        }


        foreach(int id, Sanguosha->getRandomCards()) {
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

        foreach(ServerPlayer *p, room->getAlivePlayers()) {
            p->clearFlags();
            if (p->getMark("@duanzui-extra") > 0)
                p->loseMark("@duanzui-extra");


            QStringList marks;
            marks << "chuangshi" << "xinshang_effect" << "shituDamage" << "shituDeath" << "shitu" << "zheshetransfer";
            marks << "touhou-extra" << "@qianxi_red" << "@qianxi_black" << "sizhai" << "@qingting";
            foreach(QString a, marks) {
                room->setPlayerMark(p, a, 0);
            }
        }
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if ( player->getHp() >= 1 || player->isCurrent()) //player->getPhase() != Player::NotActive ||
            return false;
        if (!player->faceUp())
            player->turnOver();

        QList<const Card *> tricks = player->getJudgingArea();
        foreach(const Card *trick, tricks) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName());
            room->throwCard(trick, reason, NULL);
        }

        player->addMark("siyuinvoke");
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());

        


        room->touhouLogmessage("#touhouExtraTurn", player, objectName());
        //for skill qinlue
        foreach(ServerPlayer *p, room->getAlivePlayers()) {
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
};
class hpymsiyu_dying : public TriggerSkill {
public:
    hpymsiyu_dying() : TriggerSkill("#hpymsiyu") {
        events << EventPhaseEnd;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getMark("siyuinvoke") > 0;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Play){
            player->removeMark("siyuinvoke");
            if (player->getHp() < 1) {
                room->notifySkillInvoked(player, "hpymsiyu");
                room->enterDying(player, NULL);
            }
        }
        return false;
    }
};

class juhe : public DrawCardsSkill {
public:
    juhe() : DrawCardsSkill("juhe") {
        frequency = Frequent;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        if (room->askForSkillInvoke(player, objectName())) {
            room->setPlayerFlag(player, "juheUsed");
            return n + 3;
        } else
            return n;
    }
};

class juhe_effect : public TriggerSkill {
public:
    juhe_effect() : TriggerSkill("#juhe") {
        events << AfterDrawNCards;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->hasFlag("juheUsed") && player->getHp() > 0){
            player->setFlags("-juheUsed");
            room->askForDiscard(player, "juhe", player->getHp(), player->getHp(), false, false, "juhe_discard:" + QString::number(player->getHp()));
        }
        return false;
    }
};




th07Package::th07Package()
    : Package("th07")
{
    General *yym001 = new General(this, "yym001$", "yym", 4, false);
    yym001->addSkill(new sidie);
    yym001->addSkill(new sidie_clear);
    yym001->addSkill(new wangxiang);
    related_skills.insertMulti("sidie", "#sidie_clear");

    General *yym002 = new General(this, "yym002", "yym", 3, false);
    yym002->addSkill(new jingjie);
    yym002->addSkill(new sisheng);
    yym002->addSkill(new jingdong);

    General *yym003 = new General(this, "yym003", "yym", 3, false);
    yym003->addSkill(new zhaoliao);
    yym003->addSkill(new jiaoxia);

    General *yym004 = new General(this, "yym004", "yym", 4, false);
    yym004->addSkill(new jianshu);
    yym004->addSkill(new jianshuWeapon);
    yym004->addSkill(new jianshuTargetMod);
    yym004->addSkill(new louguan);
    yym004->addSkill(new bailou);
    related_skills.insertMulti("jianshu", "#jianshu");
    related_skills.insertMulti("jianshu", "#jianshuTargetMod");


    General *yym005 = new General(this, "yym005", "yym", 3, false);
    yym005->addSkill(new xiezou);
    yym005->addSkill(new hesheng);

    General *yym006 = new General(this, "yym006", "yym", 4, false);
    yym006->addSkill(new renou);
    yym006->addSkill(new junshi);


    General *yym007 = new General(this, "yym007", "yym", 3, false);
    yym007->addSkill(new shishen);
    yym007->addSkill(new yexing);
    yym007->addSkill(new yexing_effect);
    yym007->addSkill(new yaoshu);
    related_skills.insertMulti("yexing", "#yexing");


    General *yym008 = new General(this, "yym008", "yym", 4, false);
    yym008->addSkill(new jiyi);
    yym008->addSkill(new chunmian);

    General *yym009 = new General(this, "yym009", "yym", 3, false);
    yym009->addSkill(new baochun);
    yym009->addSkill(new chunyi);

    General *yym010 = new General(this, "yym010", "yym", 3, false);
    yym010->addSkill(new zhancao);
    yym010->addSkill(new mocao);

    General *yym011 = new General(this, "yym011", "yym", 4, false);
    yym011->addSkill(new shenyin);
    yym011->addSkill(new xijian);

    General *yym012 = new General(this, "yym012", "yym", 3, false);
    yym012->addSkill(new youqu);
    yym012->addSkill(new wangwu);

    General *yym013 = new General(this, "yym013", "yym", 2, false);
    yym013->addSkill(new hpymsiyu);
    yym013->addSkill(new hpymsiyu_dying);
    yym013->addSkill(new juhe);
    yym013->addSkill(new juhe_effect);
    related_skills.insertMulti("hpymsiyu", "#hpymsiyu");
    related_skills.insertMulti("juhe", "#juhe");

    addMetaObject<zhaoliaoCard>();
    addMetaObject<mocaoCard>();
}

ADD_PACKAGE(th07)

