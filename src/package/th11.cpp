#include "th11.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"
#include "maneuvering.h"




class xiangqi : public TriggerSkill {
public:
    xiangqi() : TriggerSkill("xiangqi") {
        events << Damaged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL && target->isAlive());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (source == NULL || damage.from == NULL || damage.from == source || damage.card == NULL || damage.from->isKongcheng()
            || damage.to == damage.from || damage.to->isDead()
            || !source->inMyAttackRange(damage.to))
            return false;
        QString prompt = "show:" + damage.from->objectName() + ":" + damage.to->objectName() + ":" + damage.card->objectName();
        source->tag["xiangqi_from"] = QVariant::fromValue(damage.from);
        source->tag["xiangqi_to"] = QVariant::fromValue(damage.to);
        source->tag["xiangqi_card"] = QVariant::fromValue(damage.card);


        if (source->askForSkillInvoke("xiangqi", prompt)){

            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), damage.from->objectName());

            int id = room->askForCardChosen(source, damage.from, "h", objectName());
            room->showCard(damage.from, id);
            Card *showcard = Sanguosha->getCard(id);
            bool same = false;
            if (showcard->getTypeId() == damage.card->getTypeId())
                same = true;

            if (same && damage.to != source){
                room->throwCard(id, damage.from, source);
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), damage.to->objectName());

                room->damage(DamageStruct("xiangqi", source, damage.to));
            }
            else
                room->obtainCard(damage.to, showcard);
        }
        return false;
    }
};



class huzhu : public TriggerSkill {
public:
    huzhu() : TriggerSkill("huzhu$") {
        events << TargetConfirming;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        CardUseStruct use = data.value<CardUseStruct>();
        if (!player->hasLordSkill(objectName()))
            return false;
        if (use.card == NULL || use.from == NULL || !use.card->isKindOf("Slash"))
            return false;
        QList<ServerPlayer *> lieges = room->getLieges("dld", player);
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, lieges){
            if (p == use.from)
                continue;
            if (use.from->canSlash(p, use.card, false) && !use.to.contains(p))
                targets << p;
        }
        if (targets.isEmpty())
            return false;
        if (player->askForSkillInvoke(objectName(), data)){
            foreach(ServerPlayer *p, targets){
                room->setTag("huzhu_target", QVariant::fromValue(player));
                QString prompt = "slashtarget:" + use.from->objectName() + ":" + player->objectName() + ":" + use.card->objectName();
                if (p->askForSkillInvoke("huzhu_change", prompt)) {
                    room->removeTag("huzhu_target");
                    use.to << p;
                    use.to.removeOne(player);
                    data = QVariant::fromValue(use);

                    QList<ServerPlayer *> logto;
                    logto << player;
                    room->touhouLogmessage("$CancelTarget", use.from, use.card->objectName(), logto);
                    logto << p;
                    logto.removeOne(player);
                    room->touhouLogmessage("#huzhu_change", use.from, use.card->objectName(), logto);

                    room->getThread()->trigger(TargetConfirming, room, p, data);
                    break;


                }
            }
        }


        return false;
    }
};



maihuoCard::maihuoCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
    mute = true;
}
void maihuoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->moveCardTo(Sanguosha->getCard(subcards.first()), NULL, Player::DrawPile);
    QList<int> card_to_show = room->getNCards(2, false);
    CardsMoveStruct move(card_to_show, NULL, Player::PlaceTable,
        CardMoveReason(CardMoveReason::S_REASON_TURNOVER, targets.first()->objectName()));
    room->moveCardsAtomic(move, true);
    room->getThread()->delay();
    bool bothred = true;
    DummyCard *dummy = new DummyCard;
    dummy->deleteLater();
    foreach(int id, card_to_show){
        dummy->addSubcard(id);
        if (!Sanguosha->getCard(id)->isRed())
            bothred = false;
    }

    room->obtainCard(targets.first(), dummy);
    if (bothred){
        QString choice = "draw";
        if (source->isWounded())
            choice = room->askForChoice(source, "maihuo", "draw+recover");
        if (choice == "draw")
            source->drawCards(2);
        else{
            RecoverStruct recover;
            recover.who = source;
            room->recover(source, recover);
        }
    }
}

class maihuo : public OneCardViewAsSkill {
public:
    maihuo() :OneCardViewAsSkill("maihuo") {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("maihuoCard");
    }


    virtual const Card *viewAs(const Card *originalCard) const{
        maihuoCard *card = new maihuoCard;
        card->addSubcard(originalCard);

        return card;
    }
};


class wunian : public ProhibitSkill {
public:
    wunian() : ProhibitSkill("wunian") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const{
        return from != to &&  to->hasSkill(objectName()) && to->isWounded() && card->isKindOf("TrickCard");
    }
};
class wuniantr : public TriggerSkill {
public:
    wuniantr() : TriggerSkill("#wuniantr") {
        events << Predamage;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        damage.from = NULL;
        damage.by_user = false;

        room->touhouLogmessage("#TriggerSkill", player, "wunian");
        room->notifySkillInvoked(player, objectName());
        data = QVariant::fromValue(damage);
        return false;
    }
};



yaobanCard::yaobanCard() {
    will_throw = true;
    handling_method = Card::MethodUse;
    m_skillName = "yaoban";
    mute = true;
}
void yaobanCard::onEffect(const CardEffectStruct &effect) const{
    effect.from->getRoom()->damage(DamageStruct("yaoban", effect.from, effect.to));
}
bool yaobanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    QString str = Self->property("yaoban").toString();
    QStringList yaoban_targets = str.split("+");
    return  targets.isEmpty() && yaoban_targets.contains(to_select->objectName());
}
class yaobanvs : public OneCardViewAsSkill {
public:
    yaobanvs() :OneCardViewAsSkill("yaoban") {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@yaoban";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        yaobanCard *card = new yaobanCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class yaoban : public MasochismSkill {
public:
    yaoban() : MasochismSkill("yaoban") {
        view_as_skill = new yaobanvs;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        Room *room = player->getRoom();
        ServerPlayer *ldlk = room->findPlayerBySkillName(objectName());
        if (ldlk != NULL && !ldlk->isKongcheng() && damage.nature == DamageStruct::Fire){
            QStringList    yaobanTargets;
            foreach(ServerPlayer *p, room->getOtherPlayers(player))
                yaobanTargets << p->objectName();

            if (yaobanTargets.isEmpty())
                return;
            ldlk->tag["yaoban_damage"] = QVariant::fromValue(damage);
            room->setPlayerProperty(ldlk, "yaoban", yaobanTargets.join("+"));
            room->askForUseCard(ldlk, "@@yaoban", "@yaoban:" + damage.to->objectName());
            room->setPlayerProperty(ldlk, "yaoban", QVariant());
            ldlk->tag.remove("yaoban_damage");
        }
    }
};


class here : public TriggerSkill {
public:
    here() : TriggerSkill("here") {
        events << TargetConfirming;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();

        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (source == NULL)
            return false;
        if ((use.from != NULL && use.from == source) || use.to.contains(source)){
            if (use.card->isKindOf("Slash") && !use.card->isKindOf("FireSlash")){

                room->touhouLogmessage("#TriggerSkill", source, "here");
                room->notifySkillInvoked(source, objectName());
                if (use.from != NULL)
                    room->touhouLogmessage("#HereFilter", use.from, "here");
                FireSlash *new_slash = new FireSlash(use.card->getSuit(), use.card->getNumber());
                if (use.card->getSubcards().length() > 0)
                    new_slash->addSubcards(use.card->getSubcards());
                else {//use.from is ai...
                    int id = use.card->getEffectiveId();
                    if (id > -1)
                        new_slash->addSubcard(id);
                }

                new_slash->setSkillName(use.card->getSkillName());

                QStringList flags = use.card->getFlags();
                foreach(QString flag, flags)
                    new_slash->setFlags(flag);
                use.card = new_slash;
                data = QVariant::fromValue(use);
            }
        }
        return false;
    }
};





class yuanling : public TriggerSkill {
public:
    yuanling() : TriggerSkill("yuanling") {
        events << Damaged;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from != NULL && damage.from->isAlive()
            && damage.from != player){
            FireSlash *slash = new FireSlash(Card::NoSuit, 0);
            if (player->isCardLimited(slash, Card::MethodUse))
                return false;
            player->tag["yuanling"] = QVariant::fromValue(damage.from);
            QString prompt = "target:" + damage.from->objectName();
            if (room->askForSkillInvoke(player, objectName(), prompt)){
                slash->setSkillName("_" + objectName());
                room->useCard(CardUseStruct(slash, player, damage.from));
            }
        }
        return false;
    }
};

/*songzangCard::songzangCard() {
    will_throw = true;
    target_fixed = true;
    handling_method = Card::MethodDiscard;
    mute = true;
    }

    void songzangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *who=room->getCurrentDyingPlayer();
    DamageStruct damage;
    damage.from = source;
    room->killPlayer(who, &damage);
    }


    class songzangvs: public OneCardViewAsSkill {
    public:
    songzangvs():OneCardViewAsSkill("songzang") {
    filter_pattern = ".|spade|.|.!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
    return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
    if  (pattern.contains("peach") ){
    if (player->getHp()<1 && player->isWounded())
    return false;
    foreach (const Player *p, player->getAliveSiblings()){
    if (p->getHp() < 1 && p->isWounded())
    return true;
    }
    }
    return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
    songzangCard *card = new songzangCard;
    card->addSubcard(originalCard);

    return card;
    }
    };*/

class songzang : public TriggerSkill {
public:
    songzang() : TriggerSkill("songzang") {
        events << AskForPeaches;
        //view_as_skill=new songzangvs;
    }

    virtual int getPriority(TriggerEvent) const{
        return 10;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        //just for ai
        ServerPlayer *victim = room->getCurrentDyingPlayer();
        if (victim == NULL || player == victim)
            return false;
        player->tag["songzang_dying"] = data;

        const Card *c = room->askForCard(player, ".|spade", "@songzang:" + victim->objectName(), data, objectName());
        if (c){
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), victim->objectName());

            DamageStruct damage;
            damage.from = player;
            room->killPlayer(victim, &damage);
            room->setPlayerFlag(victim, "-Global_Dying");
            return true; //avoid triggering askforpeach
        }
        return false;
    }
};


class cuiji : public DrawCardsSkill {
public:
    cuiji() : DrawCardsSkill("cuiji") {

    }

    static bool do_cuiji(ServerPlayer *player) {
        Room *room = player->getRoom();
        QString choice = room->askForChoice(player, "cuiji", "red+black+cancel");
        if (choice == "cancel")
            return false;
        bool isred = (choice == "red");
        room->touhouLogmessage("#cuiji_choice", player, "cuiji", QList<ServerPlayer *>(), choice);
        room->notifySkillInvoked(player, "cuiji");
        int acquired = 0;
        while (acquired < 1){
            int id = room->drawCard();
            CardsMoveStruct move(id, NULL, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName()));
            move.reason.m_skillName = "cuiji";
            room->moveCardsAtomic(move, true);
            room->getThread()->delay();
            Card *card = Sanguosha->getCard(id);
            if (card->isRed() == isred){
                acquired = acquired + 1;
                CardsMoveStruct move2(id, player, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_GOTBACK, player->objectName()));
                room->moveCardsAtomic(move2, false);
            }
            else{
                CardsMoveStruct move3(id, NULL, Player::DiscardPile, CardMoveReason(CardMoveReason::S_REASON_NATURAL_ENTER, ""));
                room->moveCardsAtomic(move3, true);
            }
        }
        return true;

    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        if (do_cuiji(player)){
            n = n - 1;
            if (do_cuiji(player))
                n = n - 1;
        }
        return n;
    }
};

class baigui : public OneCardViewAsSkill {
public:
    baigui() : OneCardViewAsSkill("baigui") {
        filter_pattern = ".|spade|.|hand";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard != NULL){
            SavageAssault *sa = new SavageAssault(Card::SuitToBeDecided, -1);
            sa->addSubcard(originalCard);
            sa->setSkillName(objectName());
            return sa;
        }
        else
            return NULL;
    }
};

class jiuchong : public OneCardViewAsSkill {
public:
    jiuchong() : OneCardViewAsSkill("jiuchong") {
        filter_pattern = ".|heart|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern.contains("analeptic") && Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard != NULL){
            Analeptic *ana = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
            ana->addSubcard(originalCard);
            ana->setSkillName(objectName());
            return ana;
        }
        else
            return NULL;
    }
};


class guaili : public TriggerSkill {
public:
    guaili() : TriggerSkill("guaili") {
        events << TargetConfirmed << SlashProceed << CardUsed;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (!player->hasSkill("guaili"))
            return false;
        if (triggerEvent == TargetConfirmed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (player == use.from && use.card->isKindOf("Slash") && use.card->isRed()){
                foreach(ServerPlayer *target, use.to){
                    player->tag["guaili_target"] = QVariant::fromValue(target);
                    if (player->askForSkillInvoke(objectName(), "cannotjink:" + target->objectName()))
                        room->setCardFlag(use.card, "guailiTarget" + target->objectName());

                    player->tag.remove("guaili_target");
                }
            }
        }
        else if (triggerEvent == SlashProceed){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("guailiTarget" + effect.to->objectName())){
                room->slashResult(effect, NULL);
                return true;
            }
        }
        else if (triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && !use.card->isRed()){
                const Card *ask_card = room->askForCard(player, ".|.|.|hand", "@guaili", data, Card::MethodDiscard, NULL, true, objectName());
                if (ask_card){
                    Card *slash = Sanguosha->cloneCard(use.card->objectName(), Card::NoSuitRed, use.card->getNumber());
                    slash->addSubcard(use.card->getId());
                    slash->setSkillName("guaili");
                    use.card = slash;
                    room->touhouLogmessage("#guaili1", use.from, "guaili");
                    data = QVariant::fromValue(use);
                }
            }
        }
        return false;
    }
};

class haoyin : public TriggerSkill {
public:
    haoyin() : TriggerSkill("haoyin") {
        events << CardEffected;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.card->isKindOf("Analeptic") && effect.from == player){
            room->setEmotion(effect.to, "analeptic");
            if (effect.to->hasFlag("Global_Dying") && Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY){
                room->touhouLogmessage("#haoyin1", effect.from, "haoyin");
                room->notifySkillInvoked(effect.from, objectName());
                RecoverStruct recover;
                recover.card = effect.card;
                recover.recover = 2;
                recover.who = effect.from;

                room->recover(effect.to, recover);
                return true;
            }
            else{
                room->touhouLogmessage("#haoyin2", effect.to, "haoyin");
                room->notifySkillInvoked(effect.to, objectName());
                room->addPlayerMark(effect.to, "drank", 2);
                return true;
            }
        }

        return false;
    }
};
class haoyin_draw : public TriggerSkill {
public:
    haoyin_draw() : TriggerSkill("#haoyin_draw") {
        events << CardUsed;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (source == NULL)
            return false;

        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Analeptic")){

            room->touhouLogmessage("#TriggerSkill", source, "haoyin");
            room->notifySkillInvoked(source, "haoyin");
            source->drawCards(1);
        }
        return false;
    }
};


class jiduvs : public OneCardViewAsSkill {
public:
    jiduvs() : OneCardViewAsSkill("jidu") {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }


    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard != NULL){
            Duel *duel = new Duel(Card::SuitToBeDecided, -1);
            duel->addSubcard(originalCard);
            duel->setSkillName(objectName());
            return duel;
        }
        else
            return NULL;
    }
};

class jidu : public MasochismSkill {
public:
    jidu() : MasochismSkill("jidu") {
        view_as_skill = new jiduvs;
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        if (damage.card && damage.card->isKindOf("Duel")){
            if (player->askForSkillInvoke(objectName(), QVariant::fromValue(damage)))
                player->drawCards(1);
        }
    }
};


class jiduprevent : public ProhibitSkill {
public:
    jiduprevent() : ProhibitSkill("#jiduprevent") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const{
        return card->getSkillName() == "jidu" && from->getHp() > to->getHp();
    }
};


class gelong : public TriggerSkill {
public:
    gelong() : TriggerSkill("gelong") {
        events << TargetConfirming;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.to.contains(player)){
            if (use.from  && use.from->isAlive()){

                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.from->objectName());

                SupplyShortage *supply = new SupplyShortage(Card::NoSuit, 0);
                QString choice;
                bool canchoice = true;

                if (player->isProhibited(use.from, supply) || use.from->containsTrick("supply_shortage"))
                    canchoice = false;
                if (canchoice)
                    choice = room->askForChoice(use.from, objectName(), "gelong1+gelong2");
                else
                    choice = "gelong1";

                if (choice == "gelong1"){
                    room->loseHp(use.from);
                    if (use.from->isAlive())
                        use.from->drawCards(1);
                }
                else{
                    Card *first = Sanguosha->getCard(room->drawCard());
                    SupplyShortage *supplyshortage = new SupplyShortage(first->getSuit(), first->getNumber());
                    WrappedCard *vs_card = Sanguosha->getWrappedCard(first->getId());
                    vs_card->setSkillName("_gelong");
                    vs_card->takeOver(supplyshortage);
                    room->broadcastUpdateCard(room->getAlivePlayers(), vs_card->getId(), vs_card);
                    CardsMoveStruct move;
                    move.card_ids << vs_card->getId();
                    move.to = use.from;
                    move.to_place = Player::PlaceDelayedTrick;
                    room->moveCardsAtomic(move, true);

                    LogMessage mes;
                    mes.type = "$PasteCard";
                    mes.from = use.from;
                    mes.to << use.from;
                    mes.arg = objectName();
                    mes.card_str = vs_card->toString();

                    room->sendLog(mes);
                }
            }
        }
        return false;
    }
};

chuanranCard::chuanranCard() {
    //mute = true;
    will_throw = true;
    handling_method = Card::MethodNone;
}
bool chuanranCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    QString str = Self->property("chuanran").toString();
    QStringList chuanran_targets = str.split("+");
    return  targets.isEmpty() && chuanran_targets.contains(to_select->objectName());
}
void chuanranCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    int id = source->tag["chuanran_id"].toInt();
    Card *card = Sanguosha->getCard(id);
    CardsMoveStruct  move;
    move.to = target;
    move.to_place = Player::PlaceDelayedTrick;
    QString trick_name = card->objectName();
    if (!card->isKindOf("DelayedTrick")){
        SupplyShortage *supplyshortage = new SupplyShortage(card->getSuit(), card->getNumber());
        trick_name = supplyshortage->objectName();
        WrappedCard *vs_card = Sanguosha->getWrappedCard(card->getId());
        vs_card->setSkillName("_chuanran");
        vs_card->takeOver(supplyshortage);
        room->broadcastUpdateCard(room->getAlivePlayers(), vs_card->getId(), vs_card);
    }

    move.card_ids << id;
    room->moveCardsAtomic(move, true);
    room->touhouLogmessage("#chuanran_move", target, trick_name);
}


class chuanranvs : public OneCardViewAsSkill {
public:
    chuanranvs() : OneCardViewAsSkill("chuanran") {
        filter_pattern = ".|black|.|.!";
        response_pattern = "@@chuanran";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard != NULL){
            chuanranCard *indl = new chuanranCard;
            indl->addSubcard(originalCard);
            return indl;
        }
        else
            return NULL;
    }
};

class chuanran : public TriggerSkill {
public:
    chuanran() : TriggerSkill("chuanran") {
        events << CardsMoveOneTime << BeforeCardsMove << EventPhaseChanging;
        view_as_skill = new chuanranvs;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        //need check null current?
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (source == NULL || room->getCurrent()->getPhase() != Player::Judge || player != room->getCurrent())
            return false;
        if (triggerEvent == BeforeCardsMove) {
            if (move.from == NULL || move.from != room->getCurrent())
                return false;
            if ((move.from_places.contains(Player::PlaceDelayedTrick) || move.origin_from_places.contains(Player::PlaceDelayedTrick))
                ){
                QVariantList ids1 = source->tag["chuanran"].toList();
                foreach(int id, move.card_ids){
                    if (room->getCardPlace(id) == Player::PlaceDelayedTrick && !ids1.contains(id)) {
                        ids1 << id;
                    }
                }
                source->tag["chuanran"] = ids1;
            }
        }
        else if (triggerEvent == CardsMoveOneTime){
            QVariantList chuanran_ids = source->tag["chuanran"].toList();
            QList<int> all;
            foreach(QVariant card_data, chuanran_ids){
                int id = card_data.toInt();
                if (room->getCardPlace(id) == Player::DiscardPile && move.card_ids.contains(id)){
                    all << id;
                    chuanran_ids.removeOne(card_data);
                }
            }
            if (all.length() == 0)
                return false;
            source->tag["chuanran"] = chuanran_ids;
            foreach(int id, all){
                //if (room->askForSkillInvoke(source,objectName(),data)) {
                QList<ServerPlayer *>others;
                QStringList chuanranTargets;
                QString trickname;
                if (Sanguosha->getCard(id)->isKindOf("DelayedTrick"))
                    trickname = Sanguosha->getCard(id)->objectName();
                else
                    trickname = "supply_shortage";
                foreach(ServerPlayer *p, room->getOtherPlayers(room->getCurrent())){

                    if (!p->containsTrick(trickname)) {
                        others << p;
                        chuanranTargets << p->objectName();
                    }
                }
                if (!chuanranTargets.isEmpty()){
                    room->setPlayerProperty(source, "chuanran", chuanranTargets.join("+"));
                    source->tag["chuanran_cardname"] == QVariant::fromValue(trickname);
                    source->tag["chuanran_id"] = QVariant::fromValue(id);
                    //const Card *dummy =
                    room->askForUseCard(source, "@@chuanran", "@chuanran:" + trickname);
                    room->setPlayerProperty(source, "chuanran", QVariant());
                    //if (dummy==NULL)
                    //     room->obtainCard(source,id,true);
                }
                //else
                //    room->obtainCard(source,id,true);
                //}
            }
        }
        else if (triggerEvent == EventPhaseChanging){
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Judge) {
                source->tag.remove("chuanran");
            }
        }
        return false;
    }
};

class rebing : public MasochismSkill {
public:
    rebing() : MasochismSkill("rebing") {
    }


    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> listt;
        foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
            if (!p->isKongcheng() && p->getCards("j").length() > 0)
                listt << p;
        }
        if (listt.length() <= 0)
            return;
        ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@rebing", true, true);

        if (target != NULL){
            int id = room->askForCardChosen(player, target, "h", objectName());
            room->obtainCard(player, id, false);
        }
    }
};



class diaoping : public TriggerSkill {
public:
    diaoping() : TriggerSkill("diaoping") {
        events << TargetConfirmed << SlashEffected;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed){
            CardUseStruct use = data.value<CardUseStruct>();
            ServerPlayer *skillowner = room->findPlayerBySkillName(objectName());
            if (skillowner == NULL)
                return false;
            if (!use.card->isKindOf("Slash"))
                return false;
            if (use.from == NULL || player != use.from
                || skillowner == use.from)
                return false;
            bool find = false;
            foreach(ServerPlayer *p, use.to){
                if (skillowner->inMyAttackRange(p))
                    find = true;
            }
            if (!find)
                return false;
            if (skillowner->getHandcardNum() > 0 && use.from->getHandcardNum() > 0){
                bool good_result = false;
                QString prompt = "slashtarget:" + use.from->objectName() + ":" + use.card->objectName();
                skillowner->tag["diaoping_slash"] = data;
                while (use.from->isAlive() && good_result == false && skillowner->getHandcardNum() > 0 && use.from->getHandcardNum() > 0){

                    if (!room->askForSkillInvoke(skillowner, "diaoping", prompt))
                        return false;
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, skillowner->objectName(), use.from->objectName());

                    if (skillowner->pindian(use.from, "diaoping", NULL)){
                        use.from->turnOver();
                        good_result = true;
                        room->setCardFlag(use.card, "diaopingSkillNullify");
                        break;
                    }
                }
            }
        }
        else if (triggerEvent == SlashEffected){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("diaopingSkillNullify")){
                LogMessage log;
                log.type = "#LingqiAvoid";
                log.from = effect.to;
                log.arg = effect.slash->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->setEmotion(effect.to, "skill_nullify");
                return true;
            }
        }
        return false;
    }
};


class tongju : public ProhibitSkill {
public:
    tongju() : ProhibitSkill("tongju") {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return to->hasSkill(objectName()) && ((card->isKindOf("SavageAssault") || card->isKindOf("IronChain")) || card->isKindOf("ArcheryAttack"));
    }
};



th11Package::th11Package()
    : Package("th11")
{
    General *dld001 = new General(this, "dld001$", "dld", 3, false);
    dld001->addSkill(new xiangqi);
    //Room::askForCardChosen
    dld001->addSkill(new Skill("duxin", Skill::Compulsory));
    dld001->addSkill(new huzhu);

    General *dld002 = new General(this, "dld002", "dld", 3, false);
    dld002->addSkill(new maihuo);
    dld002->addSkill(new wunian);
    dld002->addSkill(new wuniantr);
    related_skills.insertMulti("wunian", "#wuniantr");

    General *dld003 = new General(this, "dld003", "dld", 4, false);
    dld003->addSkill(new yaoban);
    dld003->addSkill(new here);


    General *dld004 = new General(this, "dld004", "dld", 4, false);
    dld004->addSkill(new yuanling);
    dld004->addSkill(new songzang);

    General *dld005 = new General(this, "dld005", "dld", 3, false);
    dld005->addSkill(new cuiji);
    dld005->addSkill(new baigui);
    dld005->addSkill(new jiuchong);

    General *dld006 = new General(this, "dld006", "dld", 4, false);
    dld006->addSkill(new guaili);
    dld006->addSkill(new haoyin);
    dld006->addSkill(new haoyin_draw);
    related_skills.insertMulti("haoyin", "#haoyin_draw");

    General *dld007 = new General(this, "dld007", "dld", 3, false);
    dld007->addSkill(new jidu);
    dld007->addSkill(new jiduprevent);
    dld007->addSkill(new gelong);
    related_skills.insertMulti("jidu", "#jiduprevent");

    General *dld008 = new General(this, "dld008", "dld", 4, false);
    dld008->addSkill(new chuanran);
    dld008->addSkill(new rebing);

    General *dld009 = new General(this, "dld009", "dld", 3, false);
    dld009->addSkill(new diaoping);
    dld009->addSkill(new tongju);

    addMetaObject<maihuoCard>();
    addMetaObject<yaobanCard>();
    //addMetaObject<songzangCard>();
    addMetaObject<chuanranCard>();
}

ADD_PACKAGE(th11)

