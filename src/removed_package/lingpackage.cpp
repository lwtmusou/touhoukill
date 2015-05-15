#include "lingpackage.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "engine.h"
#include "maneuvering.h"
#include "settings.h"
#include "hegemony.h"

Neo2013XinzhanCard::Neo2013XinzhanCard(): XinzhanCard(){
    setObjectName("Neo2013XinzhanCard");
}


class Neo2013Xinzhan: public ZeroCardViewAsSkill{
public:
    Neo2013Xinzhan(): ZeroCardViewAsSkill("neo2013xinzhan"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return (!player->hasUsed("Neo2013XinzhanCard")) && (player->getHandcardNum() > player->getLostHp());
    }

    virtual const Card *viewAs() const{
        return new Neo2013XinzhanCard;
    }

};


class Neo2013Huilei: public TriggerSkill {
public:
    Neo2013Huilei():TriggerSkill("neo2013huilei") {
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;
        ServerPlayer *killer = death.damage ? death.damage->from : NULL;
        if (killer) {
            LogMessage log;
            log.type = "#HuileiThrow";
            log.from = player;
            log.to << killer;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(player, objectName());

            QString killer_name = killer->getGeneralName();
            if (killer_name.contains("zhugeliang") || killer_name == "wolong")
                room->broadcastSkillInvoke(objectName(), 1);
            else
                room->broadcastSkillInvoke(objectName(), 2);

            killer->throwAllHandCards();
            room->setPlayerMark(killer, "@HuileiDecrease", 1);
            room->acquireSkill(killer, "#neo2013huilei-maxcards", false);
        }

        return false;
    }
};

class Neo2013HuileiDecrease: public MaxCardsSkill{
public:
    Neo2013HuileiDecrease(): MaxCardsSkill("#neo2013huilei-maxcards"){

    }

    virtual int getExtra(const Player *target) const{
        if (target->getMark("@HuileiDecrease") > 0)
            return -1;
        return 0;
    }
};


class Neo2013Yishi: public TriggerSkill {
public:
    Neo2013Yishi(): TriggerSkill("neo2013yishi") {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        if (damage.card && damage.card->isKindOf("Slash") && !player->isAllNude()
        && damage.by_user && !damage.chain && !damage.transfer
            && player->askForSkillInvoke(objectName(), data)) {
                room->broadcastSkillInvoke(objectName(), 1);
                LogMessage log;
                log.type = "#Yishi";
                log.from = player;
                log.arg = objectName();
                log.to << damage.to;
                room->sendLog(log);
                if (!damage.to->isAllNude()) {
                    int card_id = room->askForCardChosen(player, damage.to, "hej", objectName());
                    if(room->getCardPlace(card_id) == Player::PlaceDelayedTrick)
                        room->broadcastSkillInvoke(objectName(), 2);
                    else if(room->getCardPlace(card_id) == Player::PlaceEquip)
                        room->broadcastSkillInvoke(objectName(), 3);
                    else
                        room->broadcastSkillInvoke(objectName(), 4);
                    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
                    room->obtainCard(player, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
                }
                return true;
        }
        return false;
    }
};

class Tannang: public DistanceSkill {
public:
    Tannang(): DistanceSkill("tannang") {
    }

    virtual int getCorrect(const Player *from, const Player *) const{
        if (from->hasSkill(objectName()))
            return -from->getLostHp();
        else
            return 0;
    }
};

class Neo2013HaoyinVS: public ZeroCardViewAsSkill{
public:
    Neo2013HaoyinVS(): ZeroCardViewAsSkill("neo2013haoyin"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player);
    }

    virtual const Card *viewAs() const{
        Analeptic *a = new Analeptic(Card::NoSuit, 0);
        a->setSkillName(objectName());
        return a;
    }

};

class Neo2013Haoyin: public TriggerSkill{
public:
    Neo2013Haoyin(): TriggerSkill("neo2013haoyin"){
        view_as_skill = new Neo2013HaoyinVS;
        events << PreCardUsed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Analeptic") && use.from == player && use.card->getSkillName() == objectName())
            room->loseHp(player);

        return false;
    }

};

class Neo2013Zhulou: public PhaseChangeSkill {
public:
    Neo2013Zhulou(): PhaseChangeSkill("neo2013zhulou") {
    }

    virtual bool onPhaseChange(ServerPlayer *gongsun) const{
        Room *room = gongsun->getRoom();
        if (gongsun->getPhase() == Player::Finish && gongsun->askForSkillInvoke(objectName())) {
            gongsun->drawCards(2);
            room->broadcastSkillInvoke(objectName());
            if (!room->askForCard(gongsun, "^BasicCard", "@zhulou-discard"))
                room->loseHp(gongsun);
        }
        return false;
    }
};

Neo2013FanjianCard::Neo2013FanjianCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

void Neo2013FanjianCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *zhouyu = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhouyu->getRoom();

    const Card *card = Sanguosha->getCard(getSubcards().first());
    int card_id = card->getEffectiveId();
    Card::Suit suit = room->askForSuit(target, "neo2013fanjian");

    LogMessage log;
    log.type = "#ChooseSuit";
    log.from = target;
    log.arg = Card::Suit2String(suit);
    room->sendLog(log);

    room->getThread()->delay();
    target->obtainCard(this);
    room->showCard(target, card_id);

    if (card->getSuit() != suit){
        zhouyu->setFlags("Neo2013Fanjian_InTempMoving");
        int card_id1 = room->askForCardChosen(zhouyu, target, "he", "neo2013fanjian");
        Player::Place place1 = room->getCardPlace(card_id1);
        target->addToPile("#fanjian", card_id1);
        int card_id2 = -1;
        if (!target->isNude()){
            card_id2 = room->askForCardChosen(zhouyu, target, "he", "neo2013fanjian");
        }
        room->moveCardTo(Sanguosha->getCard(card_id1), target, place1, CardMoveReason(CardMoveReason::S_REASON_GOTCARD, zhouyu->objectName()));
        zhouyu->setFlags("-Neo2013Fanjian_InTempMoving");
        DummyCard dummy;
        dummy.addSubcard(card_id1);
        if (card_id2 != -1)
            dummy.addSubcard(card_id2);

        room->obtainCard(zhouyu, &dummy);
    }
}

class Neo2013Fanjian: public OneCardViewAsSkill{
public:
    Neo2013Fanjian(): OneCardViewAsSkill("neo2013fanjian"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return (!player->isKongcheng() && !player->hasUsed("Neo2013FanjianCard"));
    }

    virtual bool viewFilter(const Card *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Neo2013FanjianCard *c = new Neo2013FanjianCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class Neo2013Fankui: public MasochismSkill{
public:
    Neo2013Fankui(): MasochismSkill("neo2013fankui"){

    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        Room *room = target->getRoom();
        for (int i = 0; i < damage.damage; i++){
            QList<ServerPlayer *> players;
            foreach (ServerPlayer *p, room->getOtherPlayers(target)){
                if (!p->isNude())
                    players << p;
            }

            if (players.isEmpty())
                return;

            ServerPlayer *victim;
            if ((victim = room->askForPlayerChosen(target, players, objectName(), "@neo2013fankui", true, true)) != NULL){
                int card_id = room->askForCardChosen(target, victim, "he", objectName());
                room->obtainCard(target, Sanguosha->getCard(card_id), room->getCardPlace(card_id) != Player::PlaceHand);

                room->broadcastSkillInvoke(objectName());
            }
        }
    }
};

class Neo2013Xiezun: public MaxCardsSkill{
public:
    Neo2013Xiezun(): MaxCardsSkill("neo2013xiezun"){

    }

    virtual int getFixed(const Player *target) const{
        if (target->hasSkill(objectName())) {
            int maxcards = 0;
            QList<const Player *> players = target->getAliveSiblings();
            players << target;
            foreach(const Player *p, players){
                int maxcard = p->getMaxCards(objectName());
                if (maxcards < maxcard)
                    maxcards = maxcard;
            }

            return maxcards;
        }
        if ( target->hasFlag("bllmshuiyu"))
            return 4;
        else{
            /*if (target->hasSkill("banling")){
                if (target->getHp() > target->getMark("@lingtili"))
                    return  target->getMark("@lingtili");
                else
                    return target->getHp();
            }
            else*/
            return -1;
        }
    }
};

class Neo2013XiezunEffect: public TriggerSkill{
public:
    Neo2013XiezunEffect(): TriggerSkill("#neo2013xiezun-effect"){
        events << EventPhaseEnd;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Discard && player->getHandcardNum() > player->getHp() && player->hasSkill("neo2013xiezun")){
            room->broadcastSkillInvoke("neo2013xiezun");
            room->notifySkillInvoked(player, "neo2013xiezun");
        }
        return false;
    }
};

/*
class Neo2013RenWang: public TriggerSkill{
public:
    Neo2013RenWang():TriggerSkill("neo2013renwang"){
        events << EventPhaseStart << TargetConfirming << SlashEffected << CardEffected << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *selfplayer = room->findPlayerBySkillName(objectName());
        if (selfplayer == NULL)
            return false;

        switch (triggerEvent){
            case (EventPhaseStart):{
                if (player->getPhase() == Player::RoundStart){
                    QStringList emptylist;
                    selfplayer->tag["neorenwang"] = emptylist;
                }
            }
            case (TargetConfirming):{
                if (!TriggerSkill::triggerable(player))
                    return false;
                CardUseStruct use = data.value<CardUseStruct>();
                ServerPlayer *source = use.from;
                const Card *card = use.card;

                if (card != NULL && (card->isKindOf("Slash") || card->isNDTrick())){
                    if (!source->hasFlag("YiRenwangFirst"))
                        room->setPlayerFlag(source, "YiRenwangFirst");
                    else
                        if (room->askForSkillInvoke(player, objectName(), data)){
                            room->broadcastSkillInvoke(objectName());
                            if (!room->askForDiscard(source, objectName(), 1, 1, true, true, "@neo2013renwang-discard")){
                                QStringList neorenwanginvalid = player->tag["neorenwang"].toStringList();
                                neorenwanginvalid << card->toString();
                                player->tag["neorenwang"] = neorenwanginvalid;
                            }
                        }
                }
                break;
            }
            case (CardEffected):{
                CardEffectStruct effect = data.value<CardEffectStruct>();
                if (effect.card->isNDTrick() && effect.to == selfplayer){
                    QStringList neorenwanginvalid = selfplayer->tag["neorenwang"].toStringList();
                    if (neorenwanginvalid.contains(effect.card->toString())){
                        neorenwanginvalid.removeOne(effect.card->toString());
                        selfplayer->tag["neorenwang"] = neorenwanginvalid;
                        return true;
                    }
                }
                break;
            }
            case (SlashEffected):{
                SlashEffectStruct effect = data.value<SlashEffectStruct>();
                if (effect.to != selfplayer)
                    return false;

                QStringList neorenwanginvalid = selfplayer->tag["neorenwang"].toStringList();
                if (neorenwanginvalid.contains(effect.slash->toString())){
                    neorenwanginvalid.removeOne(effect.slash->toString());
                    selfplayer->tag["neorenwang"] = neorenwanginvalid;
                    return true;
                }
                break;
            }
            case (CardFinished):{
                const Card *c = data.value<CardUseStruct>().card;
                QStringList neorenwanginvalid = selfplayer->tag["neorenwang"].toStringList();
                if (neorenwanginvalid.contains(c->toString())){
                    neorenwanginvalid.removeOne(c->toString());
                    selfplayer->tag["neorenwang"] = neorenwanginvalid;
                }
            }
            default:
                Q_ASSERT(false);
        }
        return false;
    }
};
*/

class Neo2013Renwang: public TriggerSkill{
public:
    Neo2013Renwang(): TriggerSkill("neo2013renwang"){
        events << CardUsed;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive() && target->getPhase() == Player::Play;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") || use.card->isNDTrick()){
            foreach(ServerPlayer *p, use.to){
                if (TriggerSkill::triggerable(p)){
                    if (!player->hasFlag(objectName()))
                        room->setPlayerFlag(player, objectName());
                    else if (p->canDiscard(player, "he") && p->askForSkillInvoke(objectName(), QVariant::fromValue(player))){
                        int card_id = room->askForCardChosen(p, player, "he", objectName(), false, Card::MethodDiscard);

                        room->throwCard(card_id, player, p);
                    }
                }
            }
        }
        return false;
    }
};

Neo2013YongyiCard::Neo2013YongyiCard(){
    mute = true;
}

bool Neo2013YongyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    QList<int> arrows = Self->getPile("neoarrow");
    QList<const Card *> arrowcards;

    foreach(int id, arrows)
        arrowcards << Sanguosha->getCard(id);

    QList<const Player *> players = Self->getAliveSiblings();

    QList<const Player *> tars;

    foreach(const Card *c, arrowcards){
        Slash *slash = new Slash(c->getSuit(), c->getNumber());
        slash->addSubcard(c);
        slash->setSkillName("neo2013yongyi");
        QList<const Player *> oldplayers = players;
        foreach(const Player *p, oldplayers)
            if (slash->targetFilter(targets, p, Self)){
                players.removeOne(p);
                tars << p;
            }
        delete slash;
        slash = NULL;
        if (players.isEmpty())
            break;
    }

    return tars.contains(to_select);
}

const Card *Neo2013YongyiCard::validate(CardUseStruct &cardUse) const{
    QList<int> arrows = cardUse.from->getPile("neoarrow");
    QList<int> arrowsdisabled;
    QList<const Card *> arrowcards;

    foreach(int id, arrows)
        arrowcards << Sanguosha->getCard(id);

    foreach(const Card *c, arrowcards){
        Slash *slash = new Slash(c->getSuit(), c->getNumber());
        slash->addSubcard(c);
        slash->setSkillName("neo2013yongyi");
        foreach(ServerPlayer *to, cardUse.to)
            if (cardUse.from->isProhibited(to, slash)){
                arrows.removeOne(slash->getSubcards()[0]);
                arrowsdisabled << slash->getSubcards()[0];
                break;
            }
        delete slash;
        slash = NULL;
    }

    if (arrows.isEmpty())
        return NULL;

    Room *room = cardUse.from->getRoom();

    int or_aidelay = Config.AIDelay;
    Config.AIDelay = 0;

    room->fillAG(arrows + arrowsdisabled, cardUse.from, arrowsdisabled);

    int slashcard = room->askForAG(cardUse.from, arrows, false, "neo2013yongyi");

    room->clearAG(cardUse.from);
    Config.AIDelay = or_aidelay;

    const Card *c = Sanguosha->getCard(slashcard);
    Slash *realslash = new Slash(c->getSuit(), c->getNumber());
    realslash->addSubcard(c);
    realslash->setSkillName("neo2013yongyi");
    return realslash;
}

class Neo2013YongyiVS: public ZeroCardViewAsSkill{
public:
    Neo2013YongyiVS(): ZeroCardViewAsSkill("neo2013yongyi"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "slash" && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    }

    virtual const Card *viewAs() const{
        return new Neo2013YongyiCard;
    }
};

class Neo2013Yongyi: public TriggerSkill{
public:
    Neo2013Yongyi(): TriggerSkill("neo2013yongyi"){
        events << TargetConfirmed << Damage;
        view_as_skill = new Neo2013YongyiVS;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed){
            CardUseStruct use = data.value<CardUseStruct>();
            if ((use.card->isKindOf("Slash") || use.card->isNDTrick()) && use.to.length() == 1 && use.to.contains(player)){
                const Card *c = room->askForExchange(player, objectName(), 1, false, "@neo2013yongyiput", true);
                if (c != NULL){
                    room->notifySkillInvoked(player, "neo2013yongyi");
                    room->broadcastSkillInvoke("neo2013yongyi", 1);
                    player->addToPile("neoarrow", c, false);
                }
            }
        }
        else {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != NULL && damage.card->isKindOf("Slash") && damage.card->getSkillName() == objectName()
                    && !damage.chain && !damage.transfer && damage.by_user)
                if (damage.card->isRed()){
                    if (player->askForSkillInvoke(objectName(), data)){
                        room->broadcastSkillInvoke(objectName(), 3);
                        player->drawCards(1);
                    }
                }
                else if (damage.card->isBlack()){
                    if (damage.to->isAlive() && !damage.to->hasFlag("Global_DebutFlag") && !damage.to->isNude() && player->askForSkillInvoke(objectName(), data)){
                        int card_id = room->askForCardChosen(player, damage.to, "he", objectName(), false, Card::MethodDiscard);
                        room->throwCard(card_id, damage.to, player);
                        room->broadcastSkillInvoke(objectName(), 4);
                    }
                }
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        if (card->isKindOf("Slash"))
            return 2;
        return -1;
    }
};


class Neo2013Duoyi: public TriggerSkill{
public:
    Neo2013Duoyi(): TriggerSkill("neo2013duoyi"){
        events << EventPhaseStart << CardUsed << CardResponded << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive() && !target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *selfplayer = room->findPlayerBySkillName(objectName());
        if (selfplayer == NULL || selfplayer->isKongcheng())
            return false;
        static QStringList types;
        if (types.isEmpty())
            types << "BasicCard" << "EquipCard" << "TrickCard";

        if (triggerEvent == EventPhaseStart){
            if (player->getPhase() != Player::RoundStart)
                return false;

            if (room->askForDiscard(selfplayer, objectName(), 1, 1, true, true, "@neo2013duoyi")){
                QString choice = room->askForChoice(selfplayer, objectName(), types.join("+"), data);
                room->notifySkillInvoked(selfplayer, objectName());
                room->broadcastSkillInvoke(objectName(), 1);
                room->setPlayerMark(player, "YiDuoyiType", types.indexOf(choice) + 1);
            }
        }
        else if (triggerEvent == CardUsed || triggerEvent == CardResponded){
            if (player->getMark("YiDuoyiType") == 0)
                return false;

            std::string t = types[player->getMark("YiDuoyiType") - 1].toStdString();   //QString 2 char * is TOO complicated!

            const Card *c = NULL;

            if (triggerEvent == CardUsed)
                c = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    c = resp.m_card;
            }
            if (c == NULL)
                return false;

            if (c->isKindOf(t.c_str())){
                selfplayer->drawCards(1);
                room->broadcastSkillInvoke(objectName(), 2);
            }
        }
        else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(player, "YiDuoyiType", 0);
        }
        return false;
    }
};

Neo2013PujiCard::Neo2013PujiCard(){
}

bool Neo2013PujiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() == 0 && !to_select->isNude() && to_select != Self;
}

void Neo2013PujiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets[0];
    const Card *card = Sanguosha->getCard(room->askForCardChosen(source, target, "he", "neo2013puji", false, Card::MethodDiscard));

    QList<ServerPlayer *> beneficiary;
    if (Sanguosha->getCard(getSubcards()[0])->isBlack())
        beneficiary << source;

    if (card->isBlack())
        beneficiary << target;

    room->throwCard(card, target, source);

    if (beneficiary.length() != 0)
        foreach(ServerPlayer *p, beneficiary){
            QStringList choicelist;
            choicelist << "draw";
            if (p->isWounded())
                choicelist << "recover";

            QString choice = room->askForChoice(p, "neo2013puji", choicelist.join("+"));

            if (choice == "draw")
                p->drawCards(1);
            else {
                RecoverStruct r;
                r.who = p;
                room->recover(p, r);
            }
        }
}

class Neo2013Puji: public OneCardViewAsSkill{
public:
    Neo2013Puji(): OneCardViewAsSkill("neo2013puji"){

    }

    virtual bool viewFilter(const Card *to_select) const{
        return !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Neo2013PujiCard *c = new Neo2013PujiCard;
        c->addSubcard(originalCard);
        return c;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("Neo2013PujiCard");
    }
};

class Neo2013Sijian: public Sijian{
public:
    Neo2013Sijian():Sijian(){
        setObjectName("neo2013sijian");
    }

    virtual QString getCardChosenFlag() const{
        return "hej";
    }
};

class Neo2013Suishi: public TriggerSkill{
public:
    Neo2013Suishi(): TriggerSkill("neo2013suishi"){
        events << Damage << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *selfplayer = room->findPlayerBySkillName(objectName());
        if (selfplayer == NULL || selfplayer->isDead())
            return false;
        if (triggerEvent == Damage){
            if (selfplayer->hasFlag("YiSuishiUsed"))
                return false;
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.from->hasSkill(objectName()))
                if (room->askForSkillInvoke(player, objectName(), data)){
                    room->broadcastSkillInvoke(objectName(), 1);
                    room->notifySkillInvoked(selfplayer, objectName());
                    LogMessage l;
                    l.type = "#InvokeOthersSkill";
                    l.from = player;
                    l.to << selfplayer;
                    l.arg = objectName();
                    room->sendLog(l);
                    selfplayer->drawCards(1);
                    room->setPlayerFlag(selfplayer, "YiSuishiUsed");
                }
        }
        else if (TriggerSkill::triggerable(player)){
            if (player->isKongcheng())
                return false;
            ServerPlayer *target = NULL;
            DeathStruct death = data.value<DeathStruct>();
            if (death.who->hasSkill(objectName()))
                return false;
            if (death.damage && death.damage->from)
                target = death.damage->from;

            if (target != NULL && room->askForSkillInvoke(target, objectName(), data)){
                room->broadcastSkillInvoke(objectName(), 2);
                if (target != player){
                    LogMessage l;
                    l.type = "#InvokeOthersSkill";
                    l.from = target;
                    l.to << player;
                    l.arg = objectName();
                    room->sendLog(l);
                }
                room->askForDiscard(player, objectName(), 1, 1, false, false, "@neo2013suishi");
            }
        }
        return false;
    }
};

class Neo2013Shushen: public TriggerSkill{
public:
    Neo2013Shushen(): TriggerSkill("neo2013shushen"){
        events << HpRecover;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        RecoverStruct recover_struct = data.value<RecoverStruct>();
        int recover = recover_struct.recover;
        for (int i = 1; i <= recover; i++){
            ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@neo2013shushen", true, true);
            if (target != NULL){
                room->broadcastSkillInvoke(objectName());
                room->drawCards(target, 1);
            }
            else
                break;
        }
        return false;
    }
};

class Neo2013Shenzhi: public PhaseChangeSkill{
public:
    Neo2013Shenzhi(): PhaseChangeSkill("neo2013shenzhi"){
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if (player->getPhase() != Player::Start || player->isKongcheng())
            return false;

        foreach (const Card *card, player->getHandcards()){
            if (player->isJilei(card))
                return false;
        }

        Room *room = player->getRoom();
        if (room->askForSkillInvoke(player, objectName())){
            room->broadcastSkillInvoke(objectName());
            player->throwAllHandCards();
            RecoverStruct recover;
            recover.who = player;
            recover.recover = 1;
            room->recover(player, recover);

            QList<ServerPlayer *> DelayedTricks;
            foreach(ServerPlayer *p, room->getAlivePlayers()){
                if (!p->getJudgingArea().isEmpty())
                    DelayedTricks << p;
            }
            if (!DelayedTricks.isEmpty()){
                ServerPlayer *to_throw = room->askForPlayerChosen(player, DelayedTricks, objectName(), "@neo2013shenzhi-throwjudge", true, true);
                if (to_throw != NULL){
                    DummyCard dummy;
                    dummy.addSubcards(to_throw->getJudgingArea());
                    CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, player->objectName(), objectName(), QString());
                    room->moveCardTo(&dummy, NULL, Player::DiscardPile, reason, true);
                }
            }

        }
        return false;
    }
};

class Neo2013Longyin: public TriggerSkill{
public:
    Neo2013Longyin(): TriggerSkill("neo2013longyin"){
        events << CardUsed;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getPhase() == Player::Play;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")){
            ServerPlayer *selfplayer = room->findPlayerBySkillName(objectName());
            if (selfplayer != NULL && selfplayer->canDiscard(selfplayer, "he")){
                const Card *c = NULL;
                if (use.card->isRed())
                    c = room->askForCard(selfplayer, "..red", "@neo2013longyin", data, objectName());
                else if (use.card->isBlack())
                    c = room->askForCard(selfplayer, "..black", "@neo2013longyin", data, objectName());
                if (c != NULL){
                    if (use.m_addHistory)
                        room->addPlayerHistory(player, use.card->getClassName(), -1);
                    selfplayer->drawCards(1);
                }
            }
        }
        return false;
    }

};

class Neo2013Duoshi: public OneCardViewAsSkill{
public:
    Neo2013Duoshi(): OneCardViewAsSkill("neo2013duoshi"){
        filter_pattern = ".|red|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->usedTimes("NeoDuoshiAE") <= 4;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        AwaitExhausted *ae = new AwaitExhausted(originalCard->getSuit(), originalCard->getNumber());
        ae->addSubcard(originalCard);
        ae->setSkillName(objectName());
        return ae;
    }
};

class Neo2013Danji: public PhaseChangeSkill{
public:
    Neo2013Danji(): PhaseChangeSkill("neo2013danji"){
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::RoundStart
            && target->getMark(objectName()) == 0
            && target->getHandcardNum() > target->getHp();
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, objectName());
        LogMessage l;
        l.type = "#NeoDanjiWake";
        l.from = player;
        l.arg = QString::number(player->getHandcardNum());
        l.arg2 = QString::number(player->getHp());
        room->sendLog(l);
        room->broadcastSkillInvoke(objectName());
        room->doLightbox("$DanjiAnimate", 5000);
        if (room->changeMaxHpForAwakenSkill(player)){
            room->setPlayerProperty(player, "kingdom", "shu");
            room->setPlayerMark(player, objectName(), 1);
            room->handleAcquireDetachSkills(player, "mashu|zhongyi|neo2013huwei");
        }

        return false;
    }
};

class Neo2013Huwei: public PhaseChangeSkill{
public:
    Neo2013Huwei(): PhaseChangeSkill("neo2013huwei"){
        frequency = Limited;
        limit_mark = "@yihuwei";
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        if (player->getPhase() != Player::RoundStart)
            return false;
        if (player->getMark("@yihuwei") == 0)
            return false;

        NeoDrowning *dr = new NeoDrowning(Card::NoSuit, 0);
        dr->setSkillName(objectName());

        if (!dr->isAvailable(player)){
            delete dr;
            return false;
        }
        if (player->askForSkillInvoke(objectName())){
            room->doLightbox("$HuweiAnimate", 4000);
            room->setPlayerMark(player, "@yihuwei", 0);
            room->useCard(CardUseStruct(dr, player, room->getOtherPlayers(player)), false);
        }
        return false;

    }
};


class Neo2013Huoshui: public TriggerSkill{
public:
    Neo2013Huoshui(): TriggerSkill("neo2013huoshui") {
        events << EventPhaseStart << Death
            << EventLoseSkill << EventAcquireSkill
            << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual int getPriority(TriggerEvent) const{
        return 5;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart) {
            if (!TriggerSkill::triggerable(player)
                || (player->getPhase() != Player::RoundStart || player->getPhase() != Player::NotActive)) return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || !player->hasSkill(objectName())) return false;
        } else if (triggerEvent == EventLoseSkill) {
            if (data.toString() != objectName() || player->getPhase() == Player::NotActive) return false;
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != objectName() || !player->hasSkill(objectName()) || player->getPhase() == Player::NotActive)
                return false;
        } else if (triggerEvent == CardsMoveOneTime) {  //this can fix filter skill?
            if (!room->getCurrent() || !room->getCurrent()->hasSkill(objectName())) return false;
        }

        if (player->getPhase() == Player::RoundStart || triggerEvent == EventAcquireSkill)
            room->broadcastSkillInvoke(objectName(), 1);
        else if (player->getPhase() == Player::NotActive || triggerEvent == EventLoseSkill)
            room->broadcastSkillInvoke(objectName(), 2);

        foreach (ServerPlayer *p, room->getAllPlayers())
            room->filterCards(p, p->getCards("he"), true);
        Json::Value args;
        args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

        return false;
    }
};

class Neo2013Qingcheng: public TriggerSkill{
public:
    Neo2013Qingcheng(): TriggerSkill("neo2013qingcheng"){
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual int getPriority(TriggerEvent) const{
        return 6;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() == Player::RoundStart){
            ServerPlayer *zou = room->findPlayerBySkillName(objectName());
            if (zou == NULL || zou->isNude() || zou == player)
                return false;

            if (room->askForDiscard(zou, "neo2013qingcheng", 1, 1, true, true, "@neo2013qingcheng-discard")){
                room->setPlayerFlag(player, "neo2013qingcheng");
                room->broadcastSkillInvoke(objectName(), 1);
            }
        }
        else if (player->getPhase() == Player::NotActive){
            if (player->hasFlag("neo2013qingcheng")){
                room->setPlayerFlag(player, "-neo2013qingcheng");
                room->broadcastSkillInvoke(objectName(), 2);
            }
        }

        foreach (ServerPlayer *p, room->getAllPlayers())
            room->filterCards(p, p->getCards("he"), true);

        Json::Value args;
        args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

        return false;
    }
};

LuoyiCard::LuoyiCard() {
    target_fixed = true;
}

void LuoyiCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const{
    source->setFlags("neoluoyi");
}

class NeoLuoyi: public OneCardViewAsSkill {
public:
    NeoLuoyi(): OneCardViewAsSkill("neoluoyi") {
        filter_pattern = "EquipCard!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("LuoyiCard") && player->canDiscard(player, "he");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        LuoyiCard *card = new LuoyiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class NeoLuoyiBuff: public TriggerSkill {
public:
    NeoLuoyiBuff(): TriggerSkill("#neoluoyi") {
        events << DamageCaused;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasFlag("neoluoyi") && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *xuchu, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user) return false;
        const Card *reason = damage.card;
        if (reason && (reason->isKindOf("Slash") || reason->isKindOf("Duel"))) {
            LogMessage log;
            log.type = "#LuoyiBuff";
            log.from = xuchu;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

Neo2013XiechanCard::Neo2013XiechanCard(){

}

void Neo2013XiechanCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->setPlayerMark(effect.from, "@neo2013xiechan", 0);
    bool success = effect.from->pindian(effect.to, "neo2013xiechan", NULL);
    Card *card = NULL;
    if (success)
        card = new Slash(Card::NoSuit, 0);
    else
        card = new Duel(Card::NoSuit, 0);

    card->setSkillName("_neo2013xiechan");
    room->useCard(CardUseStruct(card, effect.from, effect.to), false);
}

class Neo2013Xiechan: public ZeroCardViewAsSkill{
public:
    Neo2013Xiechan(): ZeroCardViewAsSkill("neo2013xiechan"){
        frequency = Limited;
        limit_mark = "@neo2013xiechan";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@neo2013xiechan") > 0;
    }

    virtual const Card *viewAs() const{
        return new Neo2013XiechanCard;
    }
};

class Neo2013Chengxiang: public MasochismSkill {
public:
    Neo2013Chengxiang(): MasochismSkill("neo2013chengxiang") {
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        Room *room = target->getRoom();
        for (int i = 1; i <= damage.damage; i++) {
            if (!target->askForSkillInvoke(objectName(), QVariant::fromValue(damage)))
                return;
            room->broadcastSkillInvoke(objectName());

            QList<int> card_ids = room->getNCards(4);
            room->fillAG(card_ids);

            QList<int> to_get, to_throw;
            while (true) {
                int sum = 0;
                foreach (int id, to_get)
                    sum += Sanguosha->getCard(id)->getNumber();
                foreach (int id, card_ids) {
                    if (sum + Sanguosha->getCard(id)->getNumber() > 13) {
                        room->takeAG(NULL, id, false);
                        card_ids.removeOne(id);
                        to_throw << id;
                    }
                }
                if (card_ids.isEmpty()) break;

                int card_id = room->askForAG(target, card_ids, card_ids.length() < 4, objectName());
                if (card_id == -1) break;
                card_ids.removeOne(card_id);
                to_get << card_id;
                room->takeAG(target, card_id, false);
                if (card_ids.isEmpty()) break;
            }
            DummyCard *dummy = new DummyCard;
            if (!to_get.isEmpty()) {
                dummy->addSubcards(to_get);
                target->obtainCard(dummy);
            }
            dummy->clearSubcards();
            if (!to_throw.isEmpty() || !card_ids.isEmpty()) {
                dummy->addSubcards(to_throw + card_ids);
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, target->objectName(), objectName(), QString());
                room->throwCard(dummy, reason, NULL);
            }
            delete dummy;

            room->clearAG();
        }
    }
};

class Neo2013Xiangxue: public PhaseChangeSkill{
public:
    Neo2013Xiangxue(): PhaseChangeSkill("neo2013xiangxue"){
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target) && target->getMark(objectName()) == 0;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() != Player::Start)
            return false;

        Room *room = target->getRoom();
        bool invoke = true;
        foreach (ServerPlayer *p, room->getOtherPlayers(target))
            if (p->getHandcardNum() >= target->getHandcardNum()){
                invoke = false;
                break;
            }

        if (!invoke)
            return false;

        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(target, objectName());
        room->doLightbox("$neo2013xiangxue");

        if (room->changeMaxHpForAwakenSkill(target)){
            room->setPlayerMark(target, objectName(), 1);
            room->drawCards(target, 2);
            QStringList l;
            l << "neo2013tongwu" << "neo2013bingyin";
            room->handleAcquireDetachSkills(target, l);
        }
        return false;
    }
};

class Neo2013TongwuVS: public ViewAsSkill{
public:
    Neo2013TongwuVS(): ViewAsSkill("neo2013tongwu"){

    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY
                || (Sanguosha->currentRoomState()->getCurrentCardUsePattern() == "slash"
                && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE))
            return selected.isEmpty() && to_select->isNDTrick();
        else if (Sanguosha->currentRoomState()->getCurrentCardUsePattern() == "@@neo2013tongwu")
            return false;

        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY
                || (Sanguosha->currentRoomState()->getCurrentCardUsePattern() == "slash"
                && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)){
            if (cards.length() == 0)
                return NULL;
            Slash *slash = new Slash(cards[0]->getSuit(), cards[0]->getNumber());
            slash->setSkillName(objectName());
            slash->addSubcards(cards);
            return slash;
        }
        else if (Sanguosha->currentRoomState()->getCurrentCardUsePattern() == "@@neo2013tongwu"){
            const Card *card = Sanguosha->getCard(Self->property("tongwucard").toInt());
            Card *card_to_use = Sanguosha->cloneCard(card->objectName(), Card::NoSuit, 0);
            card_to_use->setSkillName(objectName());
            return card_to_use;
        }

        return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return (pattern == "slash" && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            || (pattern == "@@neo2013tongwu");
    }
};

class Neo2013Tongwu: public TriggerSkill{
public:
    Neo2013Tongwu(): TriggerSkill("neo2013tongwu"){
        view_as_skill = new Neo2013TongwuVS;
        events << DamageDone << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == DamageDone){
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->getSkillName() == objectName())
                room->setCardFlag(damage.card, "tongwucaninvoke");
        }
        else if (TriggerSkill::triggerable(player)){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->isKindOf("Slash") && use.card->hasFlag("tongwucaninvoke")){
                room->setCardFlag(use.card, "-tongwucaninvoke");
                const Card *card_to_use = Sanguosha->getCard(use.card->getSubcards().first());
                if (!card_to_use->isAvailable(player))
                    return false;
                else if (card_to_use->targetFixed()){
                    if (!player->askForSkillInvoke(objectName(), QVariant::fromValue(card_to_use)))
                        return false;
                    Card *real_use = Sanguosha->cloneCard(card_to_use->objectName(), Card::NoSuit, 0);
                    real_use->setSkillName(objectName());
                    room->useCard(CardUseStruct(real_use, player, QList<ServerPlayer *>()));
                }
                else {
                    room->setPlayerProperty(player, "tongwucard", card_to_use->getId());
                    room->askForUseCard(player, "@@neo2013tongwu", "@neo2013tongwu");
                    room->setPlayerProperty(player, "tongwucard", QVariant());
                }
            }
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *card) const{
        if (card->isKindOf("Slash"))
            return 1;
        else
            return 2;
        return Skill::getEffectIndex(player, card);
    }
};

class Neo2013Bingyin: public PhaseChangeSkill{
public:
    Neo2013Bingyin(): PhaseChangeSkill("neo2013bingyin"){
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target) && target->getMark(objectName()) == 0;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() != Player::Finish)
            return false;

        Room *room = target->getRoom();
        bool invoke = true;
        foreach (ServerPlayer *p, room->getOtherPlayers(target))
            if (p->getHp() < target->getHp()){
                invoke = false;
                break;
            }

        if (!invoke)
            return false;

        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(target, objectName());
        room->doLightbox("$neo2013bingyin");

        room->setPlayerMark(target, objectName(), 1);
        QStringList l;
        l << "neo2013touxi" << "neo2013muhui";
        room->handleAcquireDetachSkills(target, l);

        return false;
    }

};

class Neo2013Touxi: public TriggerSkill{
public:
    Neo2013Touxi(): TriggerSkill("neo2013touxi"){
        frequency = Compulsory;
        events << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from == NULL || use.card == NULL || !use.card->isKindOf("Slash"))
            return false;

        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
            room->askForUseSlashTo(p, use.from, "@neo2013touxi-slash:" + use.from->objectName(), false);

        return false;
    }
};

class Neo2013TouxiRange: public AttackRangeSkill{
public:
    Neo2013TouxiRange(): AttackRangeSkill("#neo2013touxi"){

    }

    virtual int getFixed(const Player *target, bool ) const{
        if (target->hasSkill("neo2013touxi")) {
            const Player *current = NULL;
            foreach (const Player *p, target->getAliveSiblings()){
                if (p->getPhase() != Player::NotActive){
                    current = p;
                    break;
                }
            }

            if (current != NULL)
                return current->getHp() > 0 ? current->getHp(): 0;
            return -1;
        }


        if (target->hasSkill("duanjiao"))
            return 3;
        else if (target->hasSkill("yicun") && target->getEquips().length())
            return target->getEquips().length();
        else
            return -1;

    }
};

class Neo2013Muhui: public ProhibitSkill{
public:
    Neo2013Muhui(): ProhibitSkill("neo2013muhui"){

    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->hasSkill(objectName()) && (!card->isKindOf("SkillCard") || card->isKindOf("GuhuoCard") || card->isKindOf("QiceCard") || card->isKindOf("GudanCard"));
    }

};

class Neo2013MuhuiDis: public PhaseChangeSkill{
public:
    Neo2013MuhuiDis(): PhaseChangeSkill("#neo2013muhui"){

    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() == Player::NotActive){
            Room *room = target->getRoom();
            room->loseMaxHp(target);
            room->broadcastSkillInvoke("neo2013muhui");
            room->notifySkillInvoked(target, "neo2013muhui");
        }
        return false;
    }
};

Neo2013XiongyiCard::Neo2013XiongyiCard(): XiongyiCard(){
    setObjectName("Neo2013XiongyiCard");
}

int Neo2013XiongyiCard::getDrawNum() const{
    return 1;
}

class Neo2013Xiongyi: public ZeroCardViewAsSkill{
public:
    Neo2013Xiongyi(): ZeroCardViewAsSkill("neo2013xiongyi"){
        frequency = Limited;
        limit_mark = "@arise";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@arise") >= 1;
    }

    virtual const Card *viewAs() const{
        return new Neo2013XiongyiCard;
    }
};

class Neo2013Qijun: public TriggerSkill{
public:
    Neo2013Qijun(): TriggerSkill("neo2013qijun"){
        events << EventPhaseStart << Damage;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        ServerPlayer *mateng = room->findPlayerBySkillName(objectName());

        if (triggerEvent == EventPhaseStart){
            if (player->getPhase() == Player::Play){
                if (mateng == NULL || mateng->isDead() || mateng == player)
                    return false;

                if (!mateng->canDiscard(mateng, "he"))
                    return false;

                if (!room->askForDiscard(mateng, objectName(), 1, 1, true, true, "@qijun-discard"))
                    return false;

                room->setPlayerMark(player, "qijun", 1);
                room->setPlayerFlag(player, "qijun");
                room->handleAcquireDetachSkills(player, "mashu");
            }
            else if (player->getPhase() == Player::NotActive){
                if (player->getMark("qijun") < 1)
                    return false;

                room->handleAcquireDetachSkills(player, "-mashu");
                room->setPlayerMark(player, "qijun", 0);
            }
        }
        else if (player->hasFlag("qijun")){
            room->setPlayerFlag(player, "-qijun");

            if (mateng == NULL || mateng->isDead())
                return false;

            mateng->drawCards(1);

        }
        return false;
    }
};

Neo2013ZhoufuCard::Neo2013ZhoufuCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool Neo2013ZhoufuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    return targets.isEmpty() && to_select->getPile("incantationn").isEmpty();
}

void Neo2013ZhoufuCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    target->tag["Neo2013ZhoufuSource" + QString::number(getEffectiveId())] = QVariant::fromValue((PlayerStar)source);
    target->addToPile("incantationn", this);
}

class Neo2013ZhoufuViewAsSkill: public OneCardViewAsSkill {
public:
    Neo2013ZhoufuViewAsSkill(): OneCardViewAsSkill("neo2013zhoufu") {
        filter_pattern = ".|.|.|hand";
    }

/*
    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("Neo2013ZhoufuCard");
    }
*/

    virtual const Card *viewAs(const Card *originalcard) const{
        Card *card = new Neo2013ZhoufuCard;
        card->addSubcard(originalcard);
        return card;
    }
};

class Neo2013Zhoufu: public TriggerSkill{
public:
    Neo2013Zhoufu(): TriggerSkill("neo2013zhoufu") {
        events << StartJudge << Death;
        view_as_skill = new Neo2013ZhoufuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getPile("incantationn").length() > 0;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == StartJudge) {
            int card_id = player->getPile("incantationn").first();

            JudgeStar judge = data.value<JudgeStar>();
            judge->card = Sanguosha->getCard(card_id);

            LogMessage log;
            log.type = "$ZhoufuJudge";
            log.from = player;
            log.arg = objectName();
            log.card_str = QString::number(judge->card->getEffectiveId());
            room->sendLog(log);

            room->moveCardTo(judge->card, NULL, judge->who, Player::PlaceJudge,
                CardMoveReason(CardMoveReason::S_REASON_JUDGE,
                judge->who->objectName(),
                QString(), QString(), judge->reason), true);
            judge->updateResult();
            room->setTag("SkipGameRule", true);
        } else {
            int id = player->getPile("incantationn").first();
            PlayerStar zhangbao = player->tag["Neo2013ZhoufuSource" + QString::number(id)].value<PlayerStar>();
            if (zhangbao && zhangbao->isAlive()){
                RecoverStruct recover;
                recover.who = zhangbao;
                room->recover(zhangbao, recover);
            }
        }
        return false;
    }
};

class Neo2013Yingbing: public TriggerSkill {
public:
    Neo2013Yingbing(): TriggerSkill("neo2013yingbing") {
        events << StartJudge;
        frequency = Frequent;
    }

    virtual int getPriority(TriggerEvent) const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        int id = judge->card->getEffectiveId();
        PlayerStar zhangbao = player->tag["Neo2013ZhoufuSource" + QString::number(id)].value<PlayerStar>();
        if (zhangbao && TriggerSkill::triggerable(zhangbao)
            && zhangbao->askForSkillInvoke(objectName(), data))
            zhangbao->drawCards(2);
        return false;
    }
};

class Neo2013Yizhong: public TriggerSkill{
public:
    Neo2013Yizhong(): TriggerSkill("neo2013yizhong"){
        events << SlashEffected;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (player->faceUp() && effect.slash->isBlack()){
            LogMessage l;
            l.type = "#DanlaoAvoid";
            l.from = player;
            l.arg = effect.slash->objectName();
            l.arg2 = objectName();
            room->sendLog(l);

            return true;
        }
        return false;
    }
};

class Neo2013Canhui: public ProhibitSkill{ //Room::askForCardChosen()
public:
    Neo2013Canhui(): ProhibitSkill("neo2013canhui"){

    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return (to->hasSkill(objectName()) && !to->faceUp() && card->isRed() && card->getSkillName() != "nosguhuo");
    }
};

class Neo2013CanhuiTr: public TriggerSkill{
public:
    Neo2013CanhuiTr(): TriggerSkill("#neo2013canhui"){
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        if (!player->faceUp()){
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if ((move.from == player || move.to == player) && (move.from_places.contains(Player::PlaceHand) || move.to_place == Player::PlaceHand)){
                room->showAllCards(player);
            }
        }
        return false;
    }
};

class Neo2013Kunxiang: public TriggerSkill{
public:
    Neo2013Kunxiang(): TriggerSkill("neo2013kunxiang"){
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::RoundStart)
            return false;

        if (TriggerSkill::triggerable(player)){
            if (!player->askForSkillInvoke(objectName()))
                return false;

            player->turnOver();

            DummyCard dummy;
            dummy.addSubcards(player->getEquips());
            dummy.addSubcards(player->getJudgingArea());

            if (dummy.subcardsLength() > 0)
                room->moveCardTo(&dummy, player, Player::PlaceHand);

            if (player->getHandcardNum() < 7)
                room->drawCards(player, 7 - player->getHandcardNum());

            throw TurnBroken;
        }
        else {
            ServerPlayer *yujin = room->findPlayerBySkillName(objectName());
            if (yujin == NULL || yujin->isDead() || yujin->faceUp() || yujin->isNude())
                return false;

            if (!player->askForSkillInvoke(objectName()))
                return false;

            int card_id = room->askForCardChosen(player, yujin, "he", objectName());

            player->obtainCard(Sanguosha->getCard(card_id));

            if (room->askForChoice(player, objectName(), "draw+dismiss") == "draw")
                yujin->drawCards(1);

        }
        return false;
    }
};

class Neo2013Zongxuan: public TriggerSkill{
public:
    Neo2013Zongxuan(): TriggerSkill("neo2013zongxuan"){
        events << BeforeCardsMove;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                && move.to_place == Player::DiscardPile){
            QList<int> ids = move.card_ids;
            if (ids.isEmpty() || player->isNude() || !player->askForSkillInvoke(objectName(), data))
                return false;

            while (!ids.isEmpty()){
                room->fillAG(ids, player);
                int id = room->askForAG(player, ids, true, objectName());
                room->clearAG(player);
                if (id == -1)
                    break;

                ServerPlayer *orig_owner = room->getCardOwner(id);
                Player::Place orig_place = room->getCardPlace(id);

                const Card *c = room->askForExchange(player, objectName(), 1, true, "@zongxuan-card", true);
                if (c == NULL)
                    break;

                bool isDrawPileTop = (room->askForChoice(player, objectName(), "zongxuanup+zongxuandown", QVariant::fromValue(c)) == "zongxuanup");
                if (isDrawPileTop){
                    room->moveCardTo(c, NULL, Player::DrawPile);
                }
                else { // temp method for move cards to the bottom of drawpile, bugs exist probably
                    QList<int> to_move;
                    to_move << c->getEffectiveId();
                    room->moveCardsToEndOfDrawpile(to_move);
                }

                ids.removeOne(id);
                move.from_places.removeAt(move.card_ids.at(id));
                move.card_ids.removeOne(id);
                if (room->getCardOwner(id) == orig_owner && room->getCardPlace(id) == orig_place)
                    room->obtainCard(player, id);
            }
            data = QVariant::fromValue(move);
        }
        return false;
    }
};

Neo2013JiejiCard::Neo2013JiejiCard(){
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void Neo2013JiejiCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const{
    source->addToPile("robbery", this, false);
}

class Neo2013JiejiVS: public OneCardViewAsSkill{
public:
    Neo2013JiejiVS(): OneCardViewAsSkill("neo2013jieji"){
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("Neo2013JiejiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Neo2013JiejiCard *jieji = new Neo2013JiejiCard;
        jieji->addSubcard(originalCard);
        return jieji;
    }
};

class Neo2013Jieji: public TriggerSkill{
public:
    Neo2013Jieji(): TriggerSkill("neo2013jieji"){
        events << CardUsed << CardResponded << JinkEffect << CardsMoveOneTime;
        view_as_skill = new Neo2013JiejiVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *selfplayer = room->findPlayerBySkillName(objectName());
        if (selfplayer == NULL || selfplayer->getPile("robbery").length() == 0)
            return false;

        QList<int> robbery = selfplayer->getPile("robbery");
        QList<int> disable, able;

        if (triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from != NULL && use.from->hasSkill(objectName()))
                return false;

            if (use.card == NULL || (use.card->isKindOf("SkillCard") || use.card->isKindOf("EquipCard")))
                return false;
            foreach(int id, robbery){
                if (Sanguosha->getCard(id)->sameColorWith(use.card))
                    able.append(id);
                else
                    disable.append(id);
            }
            if (able.isEmpty() || !selfplayer->askForSkillInvoke(objectName(), data))
                return false;

            room->fillAG(robbery, selfplayer, disable);
            int card_id = room->askForAG(selfplayer, able, true, objectName());
            room->clearAG(selfplayer);
            if (card_id != -1){
                room->throwCard(Sanguosha->getCard(card_id), CardMoveReason(CardMoveReason::S_REASON_PUT, QString()), NULL);
                room->broadcastSkillInvoke(objectName(), 2);
                room->notifySkillInvoked(selfplayer, objectName());
                room->obtainCard(selfplayer, use.card);
                use.to.clear();
                data = QVariant::fromValue(use);
            }
        }
        else if (triggerEvent == CardResponded){
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card->isKindOf("Jink") && resp.m_isUse){
                foreach(int id, robbery){
                    if (Sanguosha->getCard(id)->sameColorWith(resp.m_card))
                        able.append(id);
                    else
                        disable.append(id);
                }
                if (able.isEmpty() || !selfplayer->askForSkillInvoke(objectName(), data))
                    return false;

                room->fillAG(robbery, selfplayer, disable);
                int card_id = room->askForAG(selfplayer, able, true, objectName());
                room->clearAG(selfplayer);
                if (card_id != -1){
                    room->throwCard(Sanguosha->getCard(card_id), CardMoveReason(CardMoveReason::S_REASON_PUT, QString()), NULL);
                    room->broadcastSkillInvoke(objectName(), 2);
                    room->notifySkillInvoked(selfplayer, objectName());
                    player->tag["jiejijink"] = QVariant::fromValue(resp.m_card);
                    return true;
                }
            }
        }
        else if (triggerEvent == JinkEffect){
            if (player->tag["jiejijink"].value<const Card *>() == data.value<const Card *>()){
                player->tag.remove("jiejijink");
                return true;
            }
        }
        else if (triggerEvent == CardsMoveOneTime){
            if (!player->hasSkill(objectName()))
                return false;

            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == NULL || move.to == NULL)
                return false;

            if (move.card_ids.length() != 1 || !move.from_places.contains(Player::PlaceHand) || move.to_place != Player::PlaceEquip)
                return false;

            const Card *card = Sanguosha->getCard(move.card_ids[0]);
            if (!card->isKindOf("EquipCard"))
                return false;

            foreach(int id, robbery){
                if (Sanguosha->getCard(id)->sameColorWith(card))
                    able.append(id);
                else
                    disable.append(id);
            }
            if (able.isEmpty() || !selfplayer->askForSkillInvoke(objectName(), data))
                return false;

            room->fillAG(robbery, selfplayer, disable);
            int card_id = room->askForAG(selfplayer, able, true, objectName());
            room->clearAG(selfplayer);
            if (card_id != -1){
                room->throwCard(Sanguosha->getCard(card_id), CardMoveReason(CardMoveReason::S_REASON_PUT, QString()), NULL);
                room->broadcastSkillInvoke(objectName(), 2);
                room->notifySkillInvoked(selfplayer, objectName());
                room->obtainCard(selfplayer, card);
            }
        }

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *card) const{
        return 1;
    }
};

class Neo2013Yanyu: public TriggerSkill{
public:
    Neo2013Yanyu(): TriggerSkill("neo2013yanyu"){
        events << BeforeCardsMove << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == BeforeCardsMove && TriggerSkill::triggerable(player)){
            if (room->getCurrent() == NULL || room->getCurrent() == player || room->getCurrent()->getPhase() != Player::Play)
                return false;

            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile){

                QList<int> ids = move.card_ids;

                if (player->getMark("neo2013yanyu") >= 3)
                    return false;

                int aidelay = Config.AIDelay;

                while (!ids.isEmpty() && player->getMark("neo2013yanyu") < 3){
                    room->fillAG(ids, player);
                    Config.AIDelay = 0;
                    int selected = room->askForAG(player, ids, false, objectName());
                    Config.AIDelay = aidelay;
                    room->clearAG(player);

                    ids.removeOne(selected);

                    QStringList choices;
                    choices << "cancel";
                    if (!player->isNude()){
                        choices << "gain";
                        QList<int> cards = player->handCards();
                        foreach (const Card *c, player->getEquips())
                            cards << c->getEffectiveId();

                        foreach (int card_id, cards)
                            if (Sanguosha->getCard(card_id)->getType() == Sanguosha->getCard(selected)->getType()){
                                choices << "give";
                                break;
                            }
                    }
                    if (choices.length() == 1)
                        continue;

                    Config.AIDelay = 0;
                    QString choice = room->askForChoice(player, objectName(), choices.join("+"), QVariant(selected));
                    Config.AIDelay = aidelay;

                    if (choice != "cancel"){
                        room->setPlayerMark(player, "neo2013yanyu", player->getMark("neo2013yanyu") + 1);
                    }

                    if (choice == "cancel")
                        continue;
                    else if (choice == "gain"){
                        Config.AIDelay = 0;
                        const Card *card = room->askForExchange(player, objectName() + "-gain", 1, true, "@neo2013yanyu-gain", false);
                        Config.AIDelay = aidelay;

                        QList<int> to_move;
                        to_move << card->getEffectiveId();
                        room->moveCardsToEndOfDrawpile(to_move);
                        move.from_places.removeAt(move.card_ids.at(selected));
                        move.card_ids.removeOne(selected);
                        room->obtainCard(player, selected);
                    }
                    else if (choice == "give"){
                        QString pattern;
                        switch (Sanguosha->getCard(selected)->getTypeId()){
                            case Card::TypeBasic:
                                pattern = ".Basic";
                                break;
                            case Card::TypeEquip:
                                pattern = ".Equip";
                                break;
                            case Card::TypeTrick:
                                pattern = ".Trick";
                                break;
                            default:
                                Q_ASSERT(false);

                        }
                        Config.AIDelay = 0;
                        const Card *card = room->askForCard(player, pattern, "@neo2013yanyu-give", QVariant(selected), Card::MethodNone, NULL, false, objectName());
                        Config.AIDelay = aidelay;
                        if (card == NULL)
                            continue;
                        Config.AIDelay = 0;
                        QString choice2 = room->askForChoice(player, objectName() + "-moveplace", "up+down", QVariant(QVariantList() << selected << card->getEffectiveId()));
                        ServerPlayer *to_give = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName() + "-give", "@neo2013yanyu-giveplayer", false, true);
                        Config.AIDelay = aidelay;
                        if (choice2 == "up")
                            room->moveCardTo(card, NULL, Player::DrawPile);
                        else {
                            QList<int> to_move;
                            to_move << card->getEffectiveId();
                            room->moveCardsToEndOfDrawpile(to_move);
                        }
                        move.from_places.removeAt(move.card_ids.at(selected));
                        move.card_ids.removeOne(selected);
                        room->obtainCard(to_give, selected);
                    }
                }
                data = QVariant::fromValue(move);
            }
        }
        else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart){
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                room->setPlayerMark(p, "neo2013yanyu", 0);
        }
        return false;
    }
};

class Neo2013Jingce: public TriggerSkill{
public:
    Neo2013Jingce(): TriggerSkill("neo2013jingce"){
        frequency = Frequent;
        events << EventPhaseEnd << CardUsed << CardResponded;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if ((triggerEvent == CardUsed || triggerEvent == CardResponded) /*&& TriggerSkill::triggerable(player)*/){
            ServerPlayer *current = room->getCurrent();
            if (current == NULL || current->getPhase() != Player::Play)
                return false;

            const Card *card = NULL;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else
                card = data.value<CardResponseStruct>().m_card;

            if (card != NULL && !card->isKindOf("SkillCard"))
                room->setPlayerMark(player, objectName(), player->getMark(objectName()) + 1);
        }
        else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Play){
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())){
                if (p->getMark(objectName()) >= p->getHp() && p->askForSkillInvoke(objectName()))
                    p->drawCards(2);
            }
            foreach (ServerPlayer *p, room->getAlivePlayers())
                room->setPlayerMark(p, objectName(), 0);
        }
        return false;
    }
};


Neo2013JinanCard::Neo2013JinanCard(){
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool Neo2013JinanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty())
        return false;

    QStringList tonames = Self->property("neo2013jinan").toStringList();
    if (!tonames.contains(to_select->objectName()))
        return false;

    const Card *realcard = Sanguosha->getCard(getEffectiveId())->getRealCard();
    if (!realcard->isEquipped())
        return true;

    const EquipCard *equip = qobject_cast<const EquipCard *>(realcard->getRealCard());
    if (to_select->getEquip(equip->location()))
        return false;

    return true;
}

void Neo2013JinanCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    int id = getEffectiveId();
    Player::Place cardplace = room->getCardPlace(id);
    room->moveCardTo(Sanguosha->getCard(id), effect.to, cardplace);
}

class Neo2013JinanVS: public OneCardViewAsSkill{
public:
    Neo2013JinanVS(): OneCardViewAsSkill("neo2013jinan"){
        response_pattern = "@@neo2013jinan";
    }

    virtual bool viewFilter(const Card *to_select) const{
        return true;
    }

    virtual const Card *viewAs(const Card *) const{
        Neo2013JinanCard *card = new Neo2013JinanCard;
        card->addSubcard(card);
        return card;
    }
};

class Neo2013Jinan: public TriggerSkill{
public:
    Neo2013Jinan(): TriggerSkill("neo2013jinan"){
        events << TargetConfirmed;
        view_as_skill = new Neo2013JinanVS;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card != NULL && (use.card->isKindOf("Slash") || use.card->isNDTrick())){
            QStringList tonames;
            foreach(ServerPlayer *p, use.to)
                tonames << p->objectName();
            room->setPlayerProperty(player, "neo2013jinan", tonames);
            try {
                room->askForUseCard(player, "@@neo2013jinan", "@neo2013jinan", -1, Card::MethodNone);
                room->setPlayerProperty(player, "neo2013jinan", QVariant());
            }
            catch (TriggerEvent errorevent){
                if (errorevent == StageChange || errorevent == TurnBroken)
                    room->setPlayerProperty(player, "neo2013jinan", QVariant());

                throw errorevent;
            }
        }
        return false;
    }
};

class Neo2013Enyuan: public TriggerSkill{
public:
    Neo2013Enyuan(): TriggerSkill("neo2013enyuan"){
        events << HpRecover << Damaged << CardsMoveOneTime;
    }

private:
    const static int NotInvoke = 0;
    const static int En = 1;
    const static int Yuan = 2;

public:
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *target = NULL;
        int enyuaninvoke = NotInvoke;
        int triggertimes = 0;
        switch (triggerEvent){
            case (HpRecover):{
                RecoverStruct recover = data.value<RecoverStruct>();
                if (recover.who != NULL && recover.who != player){
                    target = recover.who;
                    enyuaninvoke = En;
                    triggertimes = recover.recover;
                }
                break;
            }
            case (Damaged):{
                DamageStruct damage = data.value<DamageStruct>();
                if (damage.from != NULL && damage.from != player){
                    target = damage.from;
                    enyuaninvoke = Yuan;
                    triggertimes = damage.damage;
                }
                break;
            }
            case (CardsMoveOneTime):{
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (move.to_place != Player::PlaceHand && move.to_place != Player::PlaceEquip){
                    enyuaninvoke = NotInvoke;
                    break;
                }
                if (move.from == player && move.to != NULL && move.to != player && player->getPhase() == Player::NotActive){
                    target = (ServerPlayer *)(move.to);
                    enyuaninvoke = Yuan;
                    triggertimes = 1;
                }
                else if (move.to == player && move.from != NULL && move.from != player){
                    target = (ServerPlayer *)(move.from);
                    enyuaninvoke = En;
                    triggertimes = 1;
                }
                break;
            }
            default:
                Q_ASSERT(false);
        }
        room->setPlayerMark(player, objectName(), enyuaninvoke);
        switch (enyuaninvoke){
            case (NotInvoke):{
                break;
            }
            case (En):{
                for (int i = 0; i < triggertimes; i++){
                    if (player->askForSkillInvoke(objectName(), QVariant::fromValue(target)))
                        target->drawCards(1);
                }
                break;
            }
            case (Yuan):{
                for (int i = 0; i < triggertimes; i++){
                    if (player->askForSkillInvoke(objectName(), QVariant::fromValue(target))){
                        const Card *red = room->askForCard(target, ".H", "@enyuanheart", QVariant::fromValue(player), Card::MethodNone);
                        if (red == NULL)
                            room->loseHp(target);
                        else
                            room->obtainCard(player, red);
                    }
                }
                break;
            }
            default:
                Q_ASSERT(false);
        }
        room->setPlayerMark(player, objectName(), 0);
        return false;
    }
};

class Neo2013Ganglie: public MasochismSkill{
public:
    Neo2013Ganglie(): MasochismSkill("neo2013ganglie"){

    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &) const{
        Room *room = player->getRoom();
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@neo2013ganglie", true, true);
        if (target != NULL){
            JudgeStruct judge;
            judge.who = player;
            judge.pattern = ".|heart";
            judge.good = false;
            room->judge(judge);

            if (judge.isGood()){
                QStringList choicelist;
                choicelist << "damage";
                if (target->getHandcardNum() > 1)
                    choicelist << "throw";
                QString choice = room->askForChoice(player, objectName(), choicelist.join("+"));
                if (choice == "damage")
                    room->damage(DamageStruct(objectName(), player, target));
                else
                    room->askForDiscard(target, objectName(), 2, 2);
            }
        }
    }
};

class Neo2013Kuangfu: public TriggerSkill{
public:
    Neo2013Kuangfu(): TriggerSkill("neo2013kuangfu"){
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *panfeng, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if (target->isAlive() && !target->hasFlag("Global_DebutFlag") &&  target->getHp() <= panfeng->getHp() && target->hasEquip()) {
            QStringList equiplist;
            for (int i = 0; i <= 3; i++) {
                if (!target->getEquip(i)) continue;
                if (panfeng->canDiscard(target, target->getEquip(i)->getEffectiveId()) || panfeng->getEquip(i) == NULL)
                    equiplist << QString::number(i);
            }
            if (equiplist.isEmpty() || !panfeng->askForSkillInvoke(objectName(), data))
                return false;
            int equip_index = room->askForChoice(panfeng, "kuangfu_equip", equiplist.join("+"), QVariant::fromValue((PlayerStar)target)).toInt();
            const Card *card = target->getEquip(equip_index);
            int card_id = card->getEffectiveId();

            QStringList choicelist;
            if (panfeng->canDiscard(target, card_id))
                choicelist << "throw";
            if (equip_index > -1 && panfeng->getEquip(equip_index) == NULL)
                choicelist << "move";

            QString choice = room->askForChoice(panfeng, "kuangfu", choicelist.join("+"));

            if (choice == "move") {
                room->broadcastSkillInvoke(objectName(), 1);
                room->moveCardTo(card, panfeng, Player::PlaceEquip);
            } else {
                room->broadcastSkillInvoke(objectName(), 2);
                room->throwCard(card, target, panfeng);
            }
        }

        return false;
    }
};

class Neo2013Qixi: public OneCardViewAsSkill{
public:
    Neo2013Qixi(): OneCardViewAsSkill("neo2013qixi"){
        filter_pattern = ".|black";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        Dismantlement *dis = new Dismantlement(Card::NoSuit, 0);
        dis->deleteLater();
        return dis->isAvailable(player);
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Dismantlement *dis = new Dismantlement(Card::SuitToBeDecided, -1);
        dis->addSubcard(originalCard);
        dis->setSkillName(objectName()); //Dismantlement::onEffect();
        return dis;
    }
};

class Neo2013Jiewei: public TriggerSkill{
public:
    Neo2013Jiewei(): TriggerSkill("neo2013jiewei"){
        events << TurnedOver;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->askForSkillInvoke(objectName())){
            player->drawCards(1);
            if (room->askForCard(player, "^BasicCard", "@neo2013jiewei-discard") != NULL){
                bool invokable = false;
                foreach(ServerPlayer *p, room->getAlivePlayers()){
                    if (!p->getCards("ej").isEmpty()){
                        invokable = true;
                        break;
                    }
                }
                if (invokable){
                    QString choice = room->askForChoice(player, objectName(), "move+obtain");
                    QList<ServerPlayer *> players;
                    foreach(ServerPlayer *p, room->getAlivePlayers()){
                        if (!p->getCards("ej").isEmpty())
                            players << p;
                    }
                    ServerPlayer *target1 = room->askForPlayerChosen(player, players, objectName() + "_" + choice + "1", "@neo2013jiewei-" + choice + "1", true);
                    if (target1 != NULL){
                        int id = room->askForCardChosen(player, target1, "ej", objectName() + "_" + choice + "1");
                        if (choice == "obtain")
                            room->obtainCard(player, id);
                        else {
                            const Card *card = Sanguosha->getCard(id);
                            Player::Place place = room->getCardPlace(id);

                            int equip_index = -1;
                            if (place == Player::PlaceEquip) {
                                const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
                                equip_index = static_cast<int>(equip->location());
                            }

                            QList<ServerPlayer *> tos;
                            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                                if (equip_index != -1) {
                                    if (p->getEquip(equip_index) == NULL)
                                        tos << p;
                                } else {
                                    if (!player->isProhibited(p, card) && !p->containsTrick(card->objectName()))
                                        tos << p;
                                }
                            }

                            room->setTag("QiaobianTarget", QVariant::fromValue(target1));
                            ServerPlayer *to = room->askForPlayerChosen(player, tos, objectName() + "_move2", "@neo2013jiewei-move2:::" + card->objectName());
                            if (to)
                                room->moveCardTo(card, target1, to, place,
                                CardMoveReason(CardMoveReason::S_REASON_TRANSFER, player->objectName(), objectName(), QString()));
                            room->removeTag("QiaobianTarget");
                        }
                    }
                }
            }
        }
        return false;
    }
};

class Neo2013Wangzun: public TriggerSkill{
public:
    Neo2013Wangzun(): TriggerSkill("neo2013wangzun"){
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive() && target->getPhase() == Player::Start;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *yuanshu = room->findPlayerBySkillName(objectName());
        if (yuanshu != NULL && yuanshu != player && player->getHandcardNum() > yuanshu->getHandcardNum()
                && yuanshu->askForSkillInvoke(objectName(), QVariant::fromValue(player))){
            yuanshu->drawCards(1);
            room->setPlayerFlag(player, "neo2013yongsidec");
        }
        return false;
    }
};
class Neo2013WangzunMaxCards: public MaxCardsSkill{
public:
    Neo2013WangzunMaxCards(): MaxCardsSkill("#neo2013wangzun"){

    }

    virtual int getExtra(const Player *target) const{
        if (target->hasFlag("neo2013yongsidec"))
            return -1;
        return 0;
    }
};

class Neo2013Yongsi: public DrawCardsSkill{
public:
    Neo2013Yongsi(): DrawCardsSkill("neo2013yongsi"){

    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        ServerPlayer *lord = room->getLord();
        if (lord == NULL)
            return n;

        int num = 1;
        foreach (ServerPlayer *p, room->getOtherPlayers(lord)){
            if (p->getKingdom() == lord->getKingdom())
                num++;
        }

        return num;
    }
};
class Neo2013YongsiMaxCards: public MaxCardsSkill{
public:
    Neo2013YongsiMaxCards(): MaxCardsSkill("#neo2013yongsi"){

    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill("neo2013yongsi"))
            return -1;
        return 0;
    }
};

class Neo2013Kuanggu: public TriggerSkill{
public:
    Neo2013Kuanggu(): TriggerSkill("neo2013kuanggu"){
        events << Damage;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        for (int i = 0; i < damage.damage; i++){
            if (!player->askForSkillInvoke(objectName()))
                return false;
            JudgeStruct judge;
            judge.who = player;
            judge.pattern = ".|black";
            judge.good = true;
            judge.reason = objectName();
            room->judge(judge);

            if (judge.isGood()){
                RecoverStruct recover;
                recover.who = player;
                room->recover(player, recover);
            }
        }
        return false;
    }
};

class Neo2013Shenju: public MaxCardsSkill{
public:
    Neo2013Shenju(): MaxCardsSkill("neo2013shenju"){

    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName())){
            QList<const Player *> sib = target->getAliveSiblings();
            sib << target;
            int extra = 0;
            foreach (const Player *p, sib){
                if (target->distanceTo(p) == 1)
                    extra += p->getHp();
            }
            return extra;
        }
        return 0;
    }
};

class Neo2013BotuCount: public TriggerSkill{
public:
    Neo2013BotuCount(): TriggerSkill("#neo2013botu-count"){
        events << CardUsed << CardResponded << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

private:
    const static int USED_SPADE = 1;
    const static int USED_CLUB = 2;
    const static int USED_HEART = 4;
    const static int USED_DIAMOND = 8;

public:
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart){
            foreach (ServerPlayer *p, room->getAlivePlayers()){
                p->setMark("neo2013botu", 0);
            }
        }
        else if (triggerEvent != EventPhaseStart && player->getPhase() == Player::Play){
            const Card *c = NULL;
            if (triggerEvent == CardUsed)
                c = data.value<CardUseStruct>().card;
            else{
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    c = resp.m_card;
            }
            if (c == NULL || c->isKindOf("SkillCard"))
                return false;

            int to_add = 0;
            switch (c->getSuit()){
                case Card::Spade:
                    to_add = USED_SPADE;
                    break;
                case Card::Club:
                    to_add = USED_CLUB;
                    break;
                case Card::Heart:
                    to_add = USED_HEART;
                    break;
                case Card::Diamond:
                    to_add = USED_DIAMOND;
                    break;
                default:
                    to_add = 0;
            }

            int botu = player->getMark("neo2013botu");
            botu = botu | to_add;
            player->setMark("neo2013botu", botu);
        }
        return false;
    }
};
class Neo2013Botu: public PhaseChangeSkill{
public:
    Neo2013Botu(): PhaseChangeSkill("neo2013botu"){
        frequency = Frequent;
    }

    virtual int getPriority(TriggerEvent) const{
        return 1;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() == Player::NotActive && target->getMark("neo2013botu") == 15 && target->askForSkillInvoke(objectName())){
            target->gainAnExtraTurn();
        }
        return false;
    }
};

Ling2013Package::Ling2013Package(): Package("Ling2013"){
    General *neo2013_masu = new General(this, "neo2013_masu", "shu", 3);
    neo2013_masu->addSkill(new Neo2013Xinzhan);
    neo2013_masu->addSkill(new Neo2013Huilei);

    General *neo2013_guanyu = new General(this, "neo2013_guanyu", "shu");
    neo2013_guanyu->addSkill(new Neo2013Yishi);
    neo2013_guanyu->addSkill("wusheng");

    General *neo2013_zhangfei = new General(this, "neo2013_zhangfei", "shu");
    neo2013_zhangfei->addSkill(new Neo2013Haoyin);
    neo2013_zhangfei->addSkill("paoxiao");
    neo2013_zhangfei->addSkill(new Tannang);

    General *neo2013_gongsun = new General(this, "neo2013_gongsunzan", "qun");
    neo2013_gongsun->addSkill(new Neo2013Zhulou);
    neo2013_gongsun->addSkill("yicong");

    General *neo2013_zhouyu = new General(this, "neo2013_zhouyu", "wu", 3);
    neo2013_zhouyu->addSkill(new Neo2013Fanjian);
    neo2013_zhouyu->addSkill(new FakeMoveSkill("Neo2013Fanjian"));
    neo2013_zhouyu->addSkill("yingzi");
    related_skills.insertMulti("neo2013fanjian", "#Neo2013Fanjian-fake-move");

    General *neo2013_sima = new General(this, "neo2013_simayi", "wei", 3);
    neo2013_sima->addSkill(new Neo2013Fankui);
    neo2013_sima->addSkill("guicai");

    General *neo2013_caocao = new General(this, "neo2013_caocao$", "wei");
    neo2013_caocao->addSkill(new Neo2013Xiezun);
    neo2013_caocao->addSkill(new Neo2013XiezunEffect);
    related_skills.insertMulti("neo2013xiezun", "#neo2013xiezun-effect");
    neo2013_caocao->addSkill("jianxiong");
    neo2013_caocao->addSkill("hujia");

    General *neo2013_liubei = new General(this, "neo2013_liubei$", "shu");
    neo2013_liubei->addSkill(new Neo2013Renwang);
    neo2013_liubei->addSkill("jijiang");
    neo2013_liubei->addSkill("rende");

    General *neo2013_huangzhong = new General(this, "neo2013_huangzhong", "shu", 4);
    neo2013_huangzhong->addSkill(new Neo2013Yongyi);
    neo2013_huangzhong->addSkill(new SlashNoDistanceLimitSkill("neo2013yongyi"));
    neo2013_huangzhong->addSkill("liegong");
    related_skills.insertMulti("neo2013yongyi", "#neo2013yongyi-slash-ndl");

    General *neo2013_yangxiu = new General(this, "neo2013_yangxiu", "wei", 3);
    neo2013_yangxiu->addSkill(new Neo2013Duoyi);
    neo2013_yangxiu->addSkill("jilei");
    neo2013_yangxiu->addSkill("danlao");

    General *neo2013_huatuo = new General(this, "neo2013_huatuo", "qun", 3);
    neo2013_huatuo->addSkill(new Neo2013Puji);
    neo2013_huatuo->addSkill("jijiu");

    General *neo2013_xuchu = new General(this, "neo2013_xuchu", "wei", 4);
    neo2013_xuchu->addSkill(new NeoLuoyi);
    neo2013_xuchu->addSkill(new NeoLuoyiBuff);
    related_skills.insertMulti("neoluoyi", "#neoluoyi");
    neo2013_xuchu->addSkill(new Neo2013Xiechan);

    General *neo2013_zhaoyun = new General(this, "neo2013_zhaoyun", "shu", 4);
    neo2013_zhaoyun->addSkill("longdan");
    neo2013_zhaoyun->addSkill("yicong");
    neo2013_zhaoyun->addSkill("jiuzhu");

    General *neo2013_tianfeng = new General(this, "neo2013_tianfeng", "qun", 3);
    neo2013_tianfeng->addSkill(new Neo2013Suishi);
    neo2013_tianfeng->addSkill(new Neo2013Sijian);

    General *neo2013_gan = new General(this, "neo2013_ganfuren", "shu", 3, false);
    neo2013_gan->addSkill(new Neo2013Shushen);
    neo2013_gan->addSkill(new Neo2013Shenzhi);

    General *neo2013_guanping = new General(this, "neo2013_guanping", "shu", 4);
    neo2013_guanping->addSkill(new Neo2013Longyin);

    General *neo2013_luxun = new General(this, "neo2013_luxun", "wu", 3);
    neo2013_luxun->addSkill(new Neo2013Duoshi);
    neo2013_luxun->addSkill("qianxun");

    General *neo2013_spguanyu = new General(this, "neo2013_sp_guanyu", "wei", 4);
    neo2013_spguanyu->addSkill(new Neo2013Danji);
    neo2013_spguanyu->addSkill("wusheng");
    neo2013_spguanyu->addRelateSkill("neo2013huwei");

    General *neo2013_zoushi = new General(this, "neo2013_zoushi", "qun", 3, false);
    neo2013_zoushi->addSkill(new Neo2013Huoshui);
    neo2013_zoushi->addSkill(new Neo2013Qingcheng);

    General *neo2013_caochong = new General(this, "neo2013_caochong", "wei", 3);
    neo2013_caochong->addSkill(new Neo2013Chengxiang);
    neo2013_caochong->addSkill("renxin");

    General *neo2013_sp_lvmeng = new General(this, "neo2013_sp_lvmeng", "wu", 5);
    neo2013_sp_lvmeng->addSkill("keji");
    neo2013_sp_lvmeng->addSkill(new Neo2013Xiangxue);
    neo2013_sp_lvmeng->addRelateSkill("neo2013tongwu");
    neo2013_sp_lvmeng->addRelateSkill("neo2013bingyin");
    neo2013_sp_lvmeng->addRelateSkill("neo2013touxi");
    neo2013_sp_lvmeng->addRelateSkill("#neo2013touxi");
    neo2013_sp_lvmeng->addRelateSkill("neo2013muhui");
    neo2013_sp_lvmeng->addRelateSkill("#neo2013muhui");
    related_skills.insertMulti("neo2013touxi", "#neo2013touxi");
    related_skills.insertMulti("neo2013muhui", "#neo2013muhui");

    General *neo2013_mateng = new General(this, "neo2013_mateng", "qun", 4);
    neo2013_mateng->addSkill("mashu");
    neo2013_mateng->addSkill(new Neo2013Xiongyi);
    neo2013_mateng->addSkill(new Neo2013Qijun);

    General *neo2013_zhangbao = new General(this, "neo2013_zhangbao", "qun", 3);
    neo2013_zhangbao->addSkill(new Neo2013Zhoufu);
    neo2013_zhangbao->addSkill(new Neo2013Yingbing);

    General *neo2013_yujin = new General(this, "neo2013_yujin", "wei", 4);
    neo2013_yujin->addSkill(new Neo2013Yizhong);
    neo2013_yujin->addSkill(new Neo2013Canhui);
    neo2013_yujin->addSkill(new Neo2013CanhuiTr);
    neo2013_yujin->addSkill(new Neo2013Kunxiang);
    related_skills.insertMulti("neo2013canhui", "#neo2013canhui");

    General *neo2013_yufan = new General(this, "neo2013_yufan", "wu", 3);
    neo2013_yufan->addSkill(new Neo2013Zongxuan);
    neo2013_yufan->addSkill("zhiyan");

    General *neo2013_panzmaz = new General(this, "neo2013_panzhangmazhong", "wu", 4);
    neo2013_panzmaz->addSkill(new Neo2013Jieji);
    neo2013_panzmaz->addSkill("anjian");

    General *neo2013_xiahoushi = new General(this, "neo2013_xiahoushi", "shu", 3, false);
    neo2013_xiahoushi->addSkill(new Neo2013Yanyu);
    neo2013_xiahoushi->addSkill("xiaode");

    General *neo2013_guohuai = new General(this, "neo2013_guohuai", "wei", 3);
    neo2013_guohuai->addSkill(new Neo2013Jingce);
    neo2013_guohuai->addSkill(new Neo2013Jinan);

    General *neo2013_fazheng = new General(this, "neo2013_fazheng", "shu", 3);
    neo2013_fazheng->addSkill("nosxuanhuo");
    neo2013_fazheng->addSkill(new Neo2013Enyuan);

    General *neo2013_xiahou = new General(this, "neo2013_xiahoudun", "wei", 4);
    neo2013_xiahou->addSkill(new Neo2013Ganglie);

    General *neo2013_panfeng = new General(this, "neo2013_panfeng", "qun", 4);
    neo2013_panfeng->addSkill(new Neo2013Kuangfu);

    General *neo2013_ganning = new General(this, "neo2013_ganning", "wu", 4);
    neo2013_ganning->addSkill(new Neo2013Qixi);

    General *neo2013_caoren = new General(this, "neo2013_caoren", "wei", 4);
    neo2013_caoren->addSkill(new Neo2013Jiewei);
    neo2013_caoren->addSkill("jushou");

    General *neo2013_yuanshu = new General(this, "neo2013_yuanshu", "qun", 4);
    neo2013_yuanshu->addSkill(new Neo2013Wangzun);
    neo2013_yuanshu->addSkill(new Neo2013WangzunMaxCards);
    neo2013_yuanshu->addSkill(new Neo2013Yongsi);
    neo2013_yuanshu->addSkill(new Neo2013YongsiMaxCards);
    neo2013_yuanshu->addSkill("weidi");
    related_skills.insertMulti("neo2013wangzun", "#neo2013wangzun");
    related_skills.insertMulti("neo2013yongsi", "#neo2013yongsi");

    General *neo2013_weiyan = new General(this, "neo2013_weiyan", "shu", 4);
    neo2013_weiyan->addSkill(new Neo2013Kuanggu);

    General *neo2013_lvmeng = new General(this, "neo2013_lvmeng", "wu", 4);
    neo2013_lvmeng->addSkill(new Neo2013Shenju);
    neo2013_lvmeng->addSkill(new Neo2013BotuCount);
    neo2013_lvmeng->addSkill(new Neo2013Botu);
    neo2013_lvmeng->addSkill("keji");
    related_skills.insertMulti("neo2013botu", "#neo2013botu-count");

    addMetaObject<Neo2013XinzhanCard>();
    addMetaObject<Neo2013FanjianCard>();
    addMetaObject<Neo2013YongyiCard>();
    addMetaObject<Neo2013XiongyiCard>();
    addMetaObject<Neo2013ZhoufuCard>();
    addMetaObject<Neo2013JiejiCard>();
    addMetaObject<Neo2013JinanCard>();
    addMetaObject<Neo2013PujiCard>();
    addMetaObject<Neo2013XiechanCard>();
    addMetaObject<LuoyiCard>();

    skills << new Neo2013HuileiDecrease << new Neo2013Huwei
        << new Neo2013Tongwu << new Neo2013Bingyin << new Neo2013Touxi << new Neo2013TouxiRange << new Neo2013Muhui << new Neo2013MuhuiDis;
}

ADD_PACKAGE(Ling2013)


AwaitExhausted::AwaitExhausted(Card::Suit suit, int number): TrickCard(suit, number){
    setObjectName("await_exhausted");
}


QString AwaitExhausted::getSubtype() const{
    return "await_exhausted";
}

bool AwaitExhausted::targetFilter(const QList<const Player *> &, const Player *to_select, const Player *Self) const {
    return to_select != Self;
}

bool AwaitExhausted::targetsFeasible(const QList<const Player *> &, const Player *) const{
    return true;
}

void AwaitExhausted::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct new_use = card_use;
    if (!card_use.to.contains(card_use.from))
        new_use.to << card_use.from;

    if (getSkillName() == "duoshi")
        room->addPlayerHistory(card_use.from, "DuoshiAE", 1);

    if (getSkillName() == "neo2013duoshi")
        room->addPlayerHistory(card_use.from, "NeoDuoshiAE", 1);

    TrickCard::onUse(room, new_use);
}

void AwaitExhausted::onEffect(const CardEffectStruct &effect) const{
    effect.to->drawCards(2);
    effect.to->getRoom()->askForDiscard(effect.to, objectName(), 2, 2, false, true);
}


BefriendAttacking::BefriendAttacking(Card::Suit suit, int number): SingleTargetTrick(suit, number){
    setObjectName("befriend_attacking");
}

bool BefriendAttacking::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num)
        return false;

    QList<const Player *> siblings = Self->getAliveSiblings();
    int distance = 0;

    foreach(const Player *p, siblings){
        int dist = Self->distanceTo(p);
        if (dist > distance)
            distance = dist;
    }

    return Self->distanceTo(to_select) == distance;
}

bool BefriendAttacking::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return targets.length() > 0;
}

void BefriendAttacking::onEffect(const CardEffectStruct &effect) const{
    effect.to->drawCards(1);
    effect.from->drawCards(3);
}

KnownBoth::KnownBoth(Card::Suit suit, int number): SingleTargetTrick(suit, number){
    setObjectName("known_both");
    can_recast = true;
}

bool KnownBoth::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (Self->isCardLimited(this, Card::MethodUse))
        return false;

    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num || to_select == Self)
        return false;

    return !to_select->isKongcheng();
}

bool KnownBoth::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (Self->isCardLimited(this, Card::MethodUse))
        return targets.length() == 0;

    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
        return targets.length() != 0;

    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (getSkillName().contains("guhuo") || getSkillName() == "qice")  // Dirty hack here!!!
        return targets.length() > 0 && targets.length() <= total_num;
    else
        return targets.length() <= total_num;
}

void KnownBoth::onUse(Room *room, const CardUseStruct &card_use) const{
    if (card_use.to.isEmpty()){

        LogMessage log;
        log.type = "#Card_Recast";
        log.from = card_use.from;
        log.card_str = card_use.card->toString();
        room->sendLog(log);

        CardMoveReason reason(CardMoveReason::S_REASON_RECAST, card_use.from->objectName());
        reason.m_skillName = this->getSkillName();
        room->moveCardTo(this, card_use.from, NULL, Player::DiscardPile, reason);
        card_use.from->broadcastSkillInvoke("@recast");

        card_use.from->drawCards(1);
    }
    else
        SingleTargetTrick::onUse(room, card_use);
}

void KnownBoth::onEffect(const CardEffectStruct &effect) const{
    effect.to->getRoom()->showAllCards(effect.to, effect.from);
}

NeoDrowning::NeoDrowning(Card::Suit suit, int number): AOE(suit, number){
    setObjectName("neo_drowning");
}

void NeoDrowning::onEffect(const CardEffectStruct &effect) const{
    QVariant data = QVariant::fromValue(effect);
    Room *room = effect.to->getRoom();
    QString choice = "";
    if (!effect.to->getEquips().isEmpty() && (choice = room->askForChoice(effect.to, objectName(), "throw+damage", data)) == "throw")
        effect.to->throwAllEquips();
    else{
        ServerPlayer *source = NULL;
        if (effect.from->isAlive())
            source = effect.from;
        if (choice == "")
            room->getThread()->delay();
        room->damage(DamageStruct(this, source, effect.to));
    }
}

SixSwords::SixSwords(Card::Suit suit, int number): Weapon(suit, number, 2){
    setObjectName("SixSwords");
}

void SixSwords::onUninstall(ServerPlayer *player) const{
    Room *room = player->getRoom();
    foreach (ServerPlayer *p, room->getAlivePlayers()){
        if (p->getMark("@SixSwordsBuff") > 0)
            p->loseAllMarks("@SixSwordsBuff");
    }

    Weapon::onUninstall(player);
}

SixSwordsCard::SixSwordsCard(){

}

bool SixSwordsCard::targetFilter(const QList<const Player *> &, const Player *to_select, const Player *Self) const{
    return to_select != Self;
}

void SixSwordsCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->gainMark("@SixSwordsBuff");
}

class SixSwordsSkillVS: public ZeroCardViewAsSkill{
public:
    SixSwordsSkillVS(): ZeroCardViewAsSkill("SixSwords"){
        response_pattern = "@@SixSwords";
    }

    virtual const Card *viewAs() const{
        return new SixSwordsCard;
    }
};

class SixSwordsSkill: public WeaponSkill{
public:
    SixSwordsSkill(): WeaponSkill("SixSwords"){
        events << EventPhaseStart;
        view_as_skill = new SixSwordsSkillVS;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::NotActive){
            foreach(ServerPlayer *p, room->getOtherPlayers(player))
                if (p->getMark("@SixSwordsBuff") > 0)
                    p->loseAllMarks("@SixSwordsBuff");
            room->askForUseCard(player, "@@SixSwords", "@six_swords");
        }
        return false;
    }
};

class SixSwordsSkillRange: public AttackRangeSkill{
public:
    SixSwordsSkillRange(): AttackRangeSkill("#SixSwords"){

    }

    virtual int getExtra(const Player *target, bool ) const{
        if (target->getMark("@SixSwordsBuff") > 0)
            return 1;
        return 0;
    }
};

Triblade::Triblade(Card::Suit suit, int number): Weapon(suit, number, 3){
    setObjectName("Triblade");
}

TribladeCard::TribladeCard(){

}

bool TribladeCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    return targets.length() == 0 && to_select->hasFlag("TribladeFilter");
}

void TribladeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->damage(DamageStruct("Triblade", source, targets[0]));
}

class TribladeSkillVS: public OneCardViewAsSkill{
public:
    TribladeSkillVS(): OneCardViewAsSkill("Triblade"){
        response_pattern = "@@Triblade";
        filter_pattern = ".|.|.|hand!";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        TribladeCard *c = new TribladeCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class TribladeSkill: public WeaponSkill{
public:
    TribladeSkill(): WeaponSkill("Triblade"){
        events << Damage;
        view_as_skill = new TribladeSkillVS;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->isAlive() && damage.card && damage.card->isKindOf("Slash")
                && damage.by_user && !damage.chain && !damage.transfer){
            QList<ServerPlayer *> players;
            foreach(ServerPlayer *p, room->getOtherPlayers(damage.to))
                if (damage.to->distanceTo(p) == 1){
                    players << p;
                    room->setPlayerFlag(p, "TribladeFilter");
                }
            if (players.isEmpty())
                return false;
            room->askForUseCard(player, "@@Triblade", "@triblade");
        }

        foreach(ServerPlayer *p, room->getAllPlayers())
            if (p->hasFlag("TribladeFilter"))
                room->setPlayerFlag(p, "-TribladeFilter");

        return false;
    }
};

DragonPhoenix::DragonPhoenix(Card::Suit suit, int number): Weapon(suit, number, 2){
    setObjectName("DragonPhoenix");
}

class DragonPhoenixSkill: public WeaponSkill{
public:
    DragonPhoenixSkill(): WeaponSkill("DragonPhoenix"){
        events << TargetConfirmed << Death;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from != player)
                return false;

            if (use.card->isKindOf("Slash"))
                foreach(ServerPlayer *to, use.to){
                    if (!to->canDiscard(to, "he"))
                        return false;
                    else if (use.from->askForSkillInvoke(objectName(), data)){
                        QString prompt = "dragon-phoenix-card:" + use.from->objectName();
                        room->askForDiscard(to, objectName(), 1, 1, false, true, prompt);
                    }
                }
        }
        else {
            DeathStruct death = data.value<DeathStruct>();
            if (death.damage != NULL && death.damage->card != NULL && death.damage->card->isKindOf("Slash")
                    && death.damage->from == player && player->askForSkillInvoke(objectName(), data)){
                QString general1 = death.who->getGeneralName(), general2 = death.who->getGeneral2Name();
                QString general3 = player->getGeneralName(), general4 = player->getGeneral2Name();
                int maxhp1 = death.who->getMaxHp(), maxhp2 = player->getMaxHp();
                QList<const Skill *> skills1 = death.who->getVisibleSkillList(), skills2 = player->getVisibleSkillList();

                QStringList detachlist;
                foreach(const Skill *skill, skills1)
                    if (player->hasSkill(skill->objectName()))
                        detachlist.append(QString("-") + skill->objectName());

                if (!detachlist.isEmpty())
                    room->handleAcquireDetachSkills(player, detachlist);

                detachlist.clear();

                foreach(const Skill *skill, skills2)
                    if (death.who->hasSkill(skill->objectName()))
                        detachlist.append(QString("-") + skill->objectName());

                if (!detachlist.isEmpty())
                    room->handleAcquireDetachSkills(player, detachlist);

                room->changeHero(player, general1, false, false, false, true);
                if (general2.length() > 0)
                    room->changeHero(player, general2, false, false, true, true);

                room->changeHero(death.who, general3, false, false, false, true);
                if (general4.length() > 0)
                    room->changeHero(death.who, general4, false, false, true, true);

                if (player->getMaxHp() != maxhp1)
                    room->setPlayerProperty(player, "maxhp", (player->isLord()) ? maxhp1 + 1 : maxhp1);

                if (death.who->getMaxHp() != maxhp2)
                    room->setPlayerProperty(death.who, "maxhp", (death.who->isLord()) ? maxhp2 + 1: maxhp2);

            }
        }
        return false;
    }
};

PeaceSpell::PeaceSpell(Card::Suit suit, int number): Armor(suit, number){
    setObjectName("PeaceSpell");
}

void PeaceSpell::onUninstall(ServerPlayer *player) const{
    if (player->isAlive() && player->hasArmorEffect(objectName()))
        player->setFlags("peacespell_throwing");

    foreach(ServerPlayer *p, player->getRoom()->getAlivePlayers())
        if (p->getMark("@PeaceSpellBuff") > 0)
            p->loseAllMarks("@PeaceSpellBuff");

    Armor::onUninstall(player);
}

PeaceSpellCard::PeaceSpellCard(){

}

bool PeaceSpellCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return true;
}

void PeaceSpellCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->gainMark("@PeaceSpellBuff");
}

class PeaceSpellVS: public ZeroCardViewAsSkill{
public:
    PeaceSpellVS(): ZeroCardViewAsSkill("PeaceSpell"){
        response_pattern = "@@PeaceSpell";
    }

    virtual const Card *viewAs() const{
        return new PeaceSpellCard;
    }
};

class PeaceSpellSkill: public ArmorSkill{
public:
    PeaceSpellSkill(): ArmorSkill("PeaceSpell"){
        events << EventPhaseStart << DamageInflicted << CardsMoveOneTime;
        view_as_skill = new PeaceSpellVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent != CardsMoveOneTime && !ArmorSkill::triggerable(player))
            return false;

        switch(triggerEvent){
            case (EventPhaseStart):{
                if (player->getPhase() == Player::Discard){
                     room->askForUseCard(player, "@@PeaceSpell", "@peacespell");
                }
                else if (player->getPhase() == Player::RoundStart){
                    foreach(ServerPlayer *p, room->getAlivePlayers()){
                        if (p->getMark("@PeaceSpellBuff") > 0)
                            p->loseAllMarks("@PeaceSpellBuff");
                    }
                }
                break;
            }
            case (DamageInflicted):{
                DamageStruct damage = data.value<DamageStruct>();
                if (damage.nature != DamageStruct::Normal)
                    return true;
                break;
            }
            case (CardsMoveOneTime):{
                if (player->hasFlag("peacespell_throwing")){
                    CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                    if (move.from != NULL && move.from == player && move.from_places.contains(Player::PlaceEquip))
                        foreach(int id, move.card_ids){
                            const Card *card = Sanguosha->getEngineCard(id);
                            if (card->getClassName() == "PeaceSpell"){
                                room->loseHp(player);
                                if (player->isAlive())
                                    player->drawCards(2);
                                break;
                            }
                        }
                }
                break;
            }
            default:
                Q_ASSERT(false);
        }
        return false;
    }
};

class PeaceSpellMaxCards: public MaxCardsSkill{
public:
    PeaceSpellMaxCards(): MaxCardsSkill("#PeaceSpell"){

    }

    virtual int getExtra(const Player *target) const{
        if (target->getMark("@PeaceSpellBuff") > 0){
            return 1;
        }
        return 0;
    }
};



LingCardsPackage::LingCardsPackage(): Package("LingCards", Package::CardPack){

    QList<Card *> cards;

    cards << new AwaitExhausted(Card::Diamond, 4);
    cards << new AwaitExhausted(Card::Heart, 11);
    cards << new BefriendAttacking(Card::Heart, 9);
    cards << new KnownBoth(Card::Club, 3);
    cards << new KnownBoth(Card::Club, 4);
    cards << new NeoDrowning(Card::Club, 7);
    cards << new SixSwords(Card::Diamond, 6);
    cards << new Triblade(Card::Diamond, 12);
    cards << new DragonPhoenix(Card::Spade, 2);
    cards << new PeaceSpell(Card::Heart, 3);

    foreach(Card *c, cards)
        c->setParent(this);


    skills << new SixSwordsSkill << new SixSwordsSkillRange << new TribladeSkill << new DragonPhoenixSkill << new PeaceSpellSkill << new PeaceSpellMaxCards;
    related_skills.insertMulti("SixSwords", "#SixSwords");
    related_skills.insertMulti("PeaceSpell", "#PeaceSpell");

    addMetaObject<SixSwordsCard>();
    addMetaObject<TribladeCard>();
    addMetaObject<PeaceSpellCard>();
}

ADD_PACKAGE(LingCards)
