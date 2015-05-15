#include "th12.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"
#include "maneuvering.h"




puduCard::puduCard() {
    mute = true;
}
bool puduCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return (targets.isEmpty() && to_select != Self && to_select->isWounded());
}
void puduCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    RecoverStruct recover;
    room->recover(effect.to, recover);
    room->loseHp(effect.from, 1);
}
class pudu : public ZeroCardViewAsSkill {
public:
    pudu() : ZeroCardViewAsSkill("pudu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("puduCard");
    }

    virtual const Card *viewAs() const{
        return new puduCard;
    }
};


class jiushu : public TriggerSkill {
public:
    jiushu() : TriggerSkill("jiushu") {
        events << EventPhaseStart;
        frequency = Frequent;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Finish && player->isWounded()){
            if (room->askForSkillInvoke(player, "jiushu", data))
                player->drawCards(player->getLostHp());
        }
        return false;
    }
};


class fahua : public TriggerSkill {
public:
    fahua() : TriggerSkill("fahua$") {
        events << TargetConfirming;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("TrickCard") && use.from != NULL &&  use.from != player && use.to.contains(player)
            && player->hasLordSkill(objectName())){
            if (use.card->isKindOf("Lightning")) return false;
            room->setCardFlag(use.card, "fahua"); //for rangefix in target filter
            QList<ServerPlayer *> lieges = room->getLieges("xlc", player);
            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, lieges){
                if (p == use.from)
                    continue;
                if (use.to.contains(p) || use.from->isProhibited(p, use.card)){
                    continue;
                }
                if (!use.card->targetFilter(QList<const Player *>(), p, use.from))
                    continue;
                targets << p;
            }
            if (targets.isEmpty())
                return false;

            if (player->askForSkillInvoke(objectName(), data)){
                room->setTag("fahua_target", QVariant::fromValue(player));
                room->setTag("fahua_use", data);
                foreach(ServerPlayer *p, targets){
                    QString prompt = "tricktarget:" + use.from->objectName() + ":" + player->objectName() + ":" + use.card->objectName();
                    if (p->askForSkillInvoke("fahua_change", prompt)) {
                        use.to << p;
                        use.to.removeOne(player);
                        data = QVariant::fromValue(use);

                        QList<ServerPlayer *> logto;
                        logto << player;
                        room->touhouLogmessage("$CancelTarget", use.from, use.card->objectName(), logto);
                        logto << p;
                        logto.removeOne(player);
                        room->touhouLogmessage("#fahua_change", use.from, use.card->objectName(), logto);

                        if (use.card->isKindOf("DelayedTrick")){
                            CardsMoveStruct move;
                            move.card_ids << use.card->getId();
                            move.to_place = Player::PlaceDelayedTrick;
                            move.to = p;
                            room->moveCardsAtomic(move, true);
                        }


                        room->getThread()->trigger(TargetConfirming, room, p, data);
                        break;
                    }
                }
            }
        }
        return false;
    }
};


weizhiCard::weizhiCard() {
    target_fixed = true;
    mute = true;
}

void weizhiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    if (source->isAlive())
        room->drawCards(source, subcards.length() + 1);
}

class weizhi : public ViewAsSkill {
public:
    weizhi() : ViewAsSkill("weizhi") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return  !player->hasUsed("weizhiCard") && !player->isNude();
    }


    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return !to_select->isKindOf("TrickCard") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() > 0) {
            weizhiCard *card = new weizhiCard;
            card->addSubcards(cards);

            return card;
        }
        else
            return NULL;

    }
};

class weizhuang : public TriggerSkill {
public:
    weizhuang() : TriggerSkill("weizhuang") {
        frequency = Compulsory;
        events << TargetConfirming << CardEffected;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirming){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isNDTrick() && use.to.contains(player) && use.from != player){
                room->notifySkillInvoked(player, objectName());
                room->touhouLogmessage("#TriggerSkill", player, "weizhuang");

                use.from->tag["weizhuang_target"] = QVariant::fromValue(player);
                QString prompt = "@weizhuang-discard:" + player->objectName() + ":" + use.card->objectName();
                const Card *card = room->askForCard(use.from, ".Basic", prompt, data, Card::MethodDiscard);
                if (card == NULL) {
                    room->setCardFlag(use.card, "weizhuang" + player->objectName());
                }
            }
        }
        else if (triggerEvent == CardEffected){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->isNDTrick() && effect.card->hasFlag("weizhuang" + effect.to->objectName())){
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.card->objectName(), QList<ServerPlayer *>(), objectName());

                room->setEmotion(effect.to, "skill_nullify");
                return true;
            }
        }
        return false;
    }
};

class jinghua : public TriggerSkill {
public:
    jinghua() : TriggerSkill("jinghua") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (source == NULL)
            return false;
        if (player->getPhase() == Player::Start){
            QList<ServerPlayer *> xx;
            foreach(ServerPlayer *p, room->getAlivePlayers()){
                if (p->getCards("j").length() > 0)
                    xx << p;
            }
            if (xx.length() > 0){
                ServerPlayer * target = room->askForPlayerChosen(source, xx, objectName(), "@targetchoose", true, true);
                if (target != NULL){
                    int card_id = room->askForCardChosen(source, target, "j", objectName());
                    QList<int> card_ids;
                    card_ids << card_id;
                    room->moveCardsToEndOfDrawpile(card_ids);
                    if (player != source)
                        room->loseHp(source, 1);
                }
            }
        }
        return false;
    }
};


class zhengyiEffect : public TriggerSkill {
public:
    zhengyiEffect() : TriggerSkill("#zhengyi") {
        frequency = Compulsory;
        events << TargetConfirming << CardEffected << SlashEffected;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirming){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isBlack() && (use.card->isNDTrick() || use.card->isKindOf("Slash"))
                && use.to.contains(player)){
                room->setCardFlag(use.card, "zhengyi" + player->objectName());
                if (use.from->isAlive()){
                    room->touhouLogmessage("#TriggerSkill", player, "zhengyi");
                    CardsMoveStruct move;
                    move.to = use.from;
                    move.to_place = Player::PlaceHand;
                    move.card_ids << (room->drawCard(true));//(room->getDrawPile().last());
                    room->moveCardsAtomic(move, false);
                }
            }
        }
        else if (triggerEvent == CardEffected){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->isNDTrick() && effect.card->hasFlag("zhengyi" + effect.to->objectName())){
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.card->objectName(), QList<ServerPlayer *>(), "zhengyi");
                room->notifySkillInvoked(effect.to, "zhengyi");
                room->setEmotion(effect.to, "skill_nullify");
                return true;
            }
        }
        else if (triggerEvent == SlashEffected){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("zhengyi" + effect.to->objectName())){
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.slash->objectName(), QList<ServerPlayer *>(), "zhengyi");
                room->notifySkillInvoked(effect.to, "zhengyi");
                room->setEmotion(effect.to, "skill_nullify");
                return true;

            }
        }
        return false;
    }
};

class zhengyiArmor : public TriggerSkill {
public:
    zhengyiArmor() : TriggerSkill("#zhengyiArmor") {
        frequency = Compulsory;
        events << CardsMoveOneTime << EventAcquireSkill;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QList<int> t_ids;
            if (move.to != NULL && move.to == player && move.to_place == Player::PlaceEquip){
                foreach(int id, move.card_ids){
                    if (Sanguosha->getCard(id)->isKindOf("Armor")) {
                        t_ids << id;
                    }
                }
                if (t_ids.length() > 0){
                    room->touhouLogmessage("#ZhengyiUninstall", player, "zhengyi");
                    foreach(int id, t_ids){
                        room->throwCard(id, player, player);

                    }
                }
            }
        }
        else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != "zhengyi")
                return false;
            if (player->hasSkill(objectName())){
                QList<int> t_ids;
                foreach(const Card *card, player->getCards("e")) {
                    if (card->isKindOf("Armor"))
                        t_ids << card->getId();
                }
                if (t_ids.length() > 0){
                    room->touhouLogmessage("#ZhengyiUninstall", player, "zhengyi");
                    foreach(int id, t_ids){
                        room->throwCard(id, player, player);
                    }
                }
            }

        }
        return false;
    }
};

class zhengyi : public FilterSkill {
public:
    zhengyi() : FilterSkill("zhengyi") {
    }

    virtual bool viewFilter(const Card *to_select) const{
        Room *room = Sanguosha->currentRoom();
        if (room->getCardPlace(to_select->getEffectiveId()) == Player::PlaceHand){
            ServerPlayer *xing = room->getCardOwner(to_select->getEffectiveId());
            if (xing != NULL && xing->hasSkill(objectName())){
                return to_select->isKindOf("Armor");
            }
        }
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Nullification *nul = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        nul->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(nul);
        return card;
    }
};

chuannanCard::chuannanCard() {
    handling_method = Card::MethodNone;
}

bool chuannanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty()) return false;
    int id = getEffectiveId();

    SupplyShortage *supply = new SupplyShortage(getSuit(), getNumber());
    supply->addSubcard(id);
    supply->setSkillName("chuannan");
    supply->deleteLater();

    QString str = Self->property("chuannan").toString();
    QStringList chuannan_targets = str.split("+");
    bool canUse = !Self->isLocked(supply);
    if (canUse && chuannan_targets.contains(to_select->objectName()) && to_select != Self
        && !to_select->containsTrick("supply_shortage") && !Self->isProhibited(to_select, supply))
        return true;
    return false;
}

const Card *chuannanCard::validate(CardUseStruct &cardUse) const{
    ServerPlayer *to = cardUse.to.first();
    if (!to->containsTrick("supply_shortage")) {
        SupplyShortage *supply = new SupplyShortage(getSuit(), getNumber());
        supply->addSubcard(getEffectiveId());
        supply->setSkillName("chuannan");
        cardUse.from->getRoom()->setPlayerProperty(cardUse.from, "chuanan", QVariant());
        return supply;
    }
    return NULL;
}


class chuannanvs : public OneCardViewAsSkill {
public:
    chuannanvs() : OneCardViewAsSkill("chuannan") {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
        response_pattern = "@@chuannan";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        chuannanCard *card = new chuannanCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};
class chuannan : public TriggerSkill {
public:
    chuannan() : TriggerSkill("chuannan") {
        events << Damaged << Damage;
        view_as_skill = new chuannanvs;
        skill_property = "use_delayed_trick";
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target;
        if (triggerEvent == Damaged){
            if (damage.from  && damage.from->isAlive() && player != damage.from)
                target = damage.from;
            else
                return false;
        }
        else if (triggerEvent == Damage){
            if (damage.to && damage.to->isAlive() && player != damage.to)
                target = damage.to;
            else
                return false;
        }


        if (triggerEvent == Damaged){
            if (!room->askForSkillInvoke(player, objectName(), data))
                return false;
            player->drawCards(1);
        }

        SupplyShortage *supply = new SupplyShortage(Card::NoSuit, 0);
        supply->deleteLater();
        if (player->isCardLimited(supply, Card::MethodUse, true))
            return false;
        if (target->containsTrick("supply_shortage") || player->isProhibited(target, supply))
            return false;


        player->tag["chuannan_damage"] = QVariant::fromValue(damage);
        QStringList    chuannanTargets;
        chuannanTargets << target->objectName();
        room->setPlayerProperty(player, "chuannan", chuannanTargets.join("+"));
        room->askForUseCard(player, "@@chuannan", "@chuannan:" + target->objectName());
        room->setPlayerProperty(player, "chuanan", QVariant());
        player->tag.remove("chuannan_damage");
        return false;
    }
};



class lizhi : public TriggerSkill {
public:
    lizhi() : TriggerSkill("lizhi") {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        player->tag["lizhi_damage"] = data;
        if (player->askForSkillInvoke(objectName(), QVariant::fromValue(damage.to))){
            player->drawCards(2);
            return true;
        }
        return false;
    }
};
class yunshang : public TriggerSkill {
public:
    yunshang() : TriggerSkill("yunshang") {
        frequency = Compulsory;
        events << TargetConfirming << CardEffected;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirming){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isNDTrick() && use.to.contains(player) && use.from != player && !use.from->inMyAttackRange(player)){
                room->setCardFlag(use.card, "yunshang" + player->objectName());
                room->notifySkillInvoked(player, objectName());
            }
        }
        else if (triggerEvent == CardEffected){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->isNDTrick() && effect.card->hasFlag("yunshang" + effect.to->objectName())){
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.card->objectName(), QList<ServerPlayer *>(), objectName());
                room->setEmotion(effect.to, "skill_nullify");
                return true;
            }
        }
        return false;
    }
};


class souji : public TriggerSkill {
public:
    souji() : TriggerSkill("souji") {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        if (player->isCurrent() && move.from  && move.from != player
                && move.to_place == Player::DiscardPile){
            //check provider
            ServerPlayer *provider = move.reason.m_provider.value<ServerPlayer *>();
            if (provider && provider == player)
                return false;

            QList<int> obtain_ids;
            foreach(int id, move.card_ids) {
                if (room->getCardPlace(id) != Player::DiscardPile)
                    continue;

                switch (move.from_places.at(move.card_ids.indexOf(id))) {
                case Player::PlaceHand: obtain_ids << id; break;
                case Player::PlaceEquip: obtain_ids << id; break;
                case Player::PlaceTable: {
                    //if (move.reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE ||
                        //move.reason.m_reason == CardMoveReason::S_REASON_USE || move.reason.m_reason == CardMoveReason::S_REASON_RESPONSE)
                            QVariantList record_ids = room->getTag("UseOrResponseFromPile").toList();
                            bool ocurrence = false;
                            foreach(QVariant card_data, record_ids) {
                                int card_id = card_data.toInt();
                                if (card_id == id){
                                    ocurrence = true;
                                    break;
                                }
                            }
                            if (!ocurrence)
                                obtain_ids << id;

                        break;
                    }
                default:
                    break;
                }
            }

            if (obtain_ids.length() > 0 && room->askForSkillInvoke(player, objectName(), data)) {
                CardsMoveStruct mo;
                mo.card_ids = obtain_ids;
                mo.to = player;
                mo.to_place = Player::PlaceHand;
                room->moveCardsAtomic(mo, true);
            }

        }


        return false;
    }
};

//record card from pile by using  //will be gamerule?
class soujiRecord : public TriggerSkill {
public:
    soujiRecord() : TriggerSkill("#souji") {
        events << CardsMoveOneTime << BeforeCardsMove;
    }

    virtual int getPriority(TriggerEvent) const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (triggerEvent == BeforeCardsMove){
            if (move.to_place != Player::PlaceTable)
                return false;

            if  (move.reason.m_reason == CardMoveReason::S_REASON_RETRIAL || move.reason.m_reason == CardMoveReason::S_REASON_USE
                    ||    move.reason.m_reason == CardMoveReason::S_REASON_LETUSE || move.reason.m_reason == CardMoveReason::S_REASON_RESPONSE)
            {
                QVariantList record_ids = room->getTag("UseOrResponseFromPile").toList();
                QVariantList tmp_ids = room->getTag("UseOrResponseFromPile").toList();
                //QList<int> tmp_ids
                QList<int> pile_ids;
                foreach(int id, move.card_ids){
                    if (move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceHand && move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceEquip)
                        pile_ids << id;
                }
                foreach (int id, pile_ids) {
                    //check ocurrence
                    bool ocurrence = false;
                    foreach(QVariant card_data, record_ids) {
                            int card_id = card_data.toInt();
                            if (card_id == id){
                                ocurrence = true;
                                break;
                            }
                    }
                    if (!ocurrence)
                        tmp_ids << id;
                }
                //room->setTag("UseOrResponseFromPile", IntList2VariantList(tmp_ids));
                room->setTag("UseOrResponseFromPile", tmp_ids);
            }
        }
        else if (triggerEvent == CardsMoveOneTime){
            QVariantList record_ids = room->getTag("UseOrResponseFromPile").toList();

            QVariantList tmp_ids = room->getTag("UseOrResponseFromPile").toList();
            foreach(int id, move.card_ids){
                if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceTable){
                    foreach(QVariant card_data, record_ids){
                        int card_id = card_data.toInt();
                        if (card_id == id)
                            tmp_ids.removeOne(id);
                    }
                }
            }
            room->setTag("UseOrResponseFromPile", tmp_ids);
        }
        return false;
    }
};

class tansuo : public TriggerSkill {
public:
    tansuo() : TriggerSkill("tansuo") {
        events << CardsMoveOneTime << EventPhaseEnd;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::Discard)
            return false;
        if (triggerEvent == CardsMoveOneTime){
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != NULL && move.from == player){
                if (move.to_place == Player::DiscardPile){
                    int count = player->getMark("tansuo");
                    count = count + move.card_ids.length();
                    player->setMark("tansuo", count);
                }
            }
        }
        else if (triggerEvent == EventPhaseEnd){
            if (player->getMark("tansuo") >= player->getHp()){
                if (player->askForSkillInvoke(objectName())){
                    CardsMoveStruct move;
                    move.to = player;
                    move.to_place = Player::PlaceHand;
                    for (int i = 1; i <= 2; i++)
                        move.card_ids << room->drawCard(true);
                    //int len = room->getDrawPile().length();
                    //move.card_ids << (room->getDrawPile().last());
                    //move.card_ids << (room->getDrawPile().at(len - 2));

                    room->moveCardsAtomic(move, false);
                }

            }
            player->setMark("tansuo", 0); //need clear at EventPhaseChanging?
            //like turn broken??
        }
        return false;
    }
};



class yiwang : public TriggerSkill {
public:
    yiwang() : TriggerSkill("yiwang") {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != NULL && move.from == player && move.from_places.contains(Player::PlaceEquip)){
            foreach(Player::Place place, move.from_places) {
                if (place == Player::PlaceEquip){
                    QList<ServerPlayer *> wounded;
                    foreach(ServerPlayer *p, room->getAllPlayers()){
                        if (p->isWounded())
                            wounded << p;
                    }
                    if (wounded.isEmpty())
                        return false;

                    ServerPlayer * recovertarget = room->askForPlayerChosen(player, wounded, objectName(), "@yiwang-recover", true, true);
                    if (recovertarget == NULL)
                        return false;
                    RecoverStruct recover2;
                    recover2.who = player;
                    room->recover(recovertarget, recover2);
                    if (recovertarget != player)
                        player->drawCards(1);
                }
            }
        }
        return false;
    }
};


class jingxia : public MasochismSkill {
public:
    jingxia() : MasochismSkill("jingxia") {

    }


    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const{
        Room *room = player->getRoom();
        for (int i = 0; i < damage.damage; i++) {
            QStringList choices;
            if (damage.from != NULL && player->canDiscard(damage.from, "he"))
                choices << "discard";
            QList<ServerPlayer *> fieldcard;
            foreach(ServerPlayer *p, room->getAllPlayers()){
                if (player->canDiscard(p, "ej"))
                    fieldcard << p;
            }
            if (!fieldcard.isEmpty())
                choices << "discardfield";

            choices << "dismiss";

            player->tag["jingxia"] = QVariant::fromValue(damage);
            QString choice = room->askForChoice(player, objectName(), choices.join("+"), QVariant::fromValue(damage));
            player->tag.remove("jingxia");
            if (choice == "dismiss")
                return;

            room->touhouLogmessage("#InvokeSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            if (choice == "discard"){
                for (int i = 0; i < 2; i++) {
                    if (!player->canDiscard(damage.from, "he"))
                        return;
                    int card_id = room->askForCardChosen(player, damage.from, "he", objectName(), false, Card::MethodDiscard);
                    //objectName() + "-discard"
                    room->throwCard(card_id, damage.from, player);
                }
            }
            else{
                //local aidelay = sgs.GetConfig("AIDelay", 0)
                // sgs.SetConfig("AIDelay", 0)
                //        ---------------AIDELAY == 0-------------------
                ServerPlayer *player1 = room->askForPlayerChosen(player, fieldcard, objectName(), "@jingxia-discardfield");
                int card1 = room->askForCardChosen(player, player1, "ej", objectName(), false, Card::MethodDiscard);
                //objectName()+"-discardfield"
                //sgs.SetConfig("AIDelay", aidelay)
                //        ----------------------------------------------
                room->throwCard(card1, player1, player);
                //       ---------------AIDELAY == 0-------------------
                //sgs.SetConfig("AIDelay", 0)
                QList<ServerPlayer *> fieldcard2;
                foreach(ServerPlayer *p, room->getAllPlayers()){
                    if (player->canDiscard(p, "ej"))
                        fieldcard2 << p;
                }
                if (fieldcard2.length() == 0)
                    return;
                ServerPlayer *player2 = room->askForPlayerChosen(player, fieldcard2, objectName(), "@jingxia-discardfield2", true);
                if (player2 != NULL) {
                    int card2 = room->askForCardChosen(player, player2, "ej", objectName(), false, Card::MethodDiscard);
                    //sgs.SetConfig("AIDelay", aidelay)
                    room->throwCard(card2, player2, player);
                }
                //       ----------------------------------------------
                //sgs.SetConfig("AIDelay", aidelay)
            }

        }
    }
};




class bianhuan : public TriggerSkill {
public:
    bianhuan() : TriggerSkill("bianhuan") {
        events << DamageInflicted;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (room->askForSkillInvoke(player, objectName(), data)) {
            room->loseMaxHp(player, 1);
            return true;
        }
        return false;
    }

};





nuhuoCard::nuhuoCard() {
    mute = true;
}
bool nuhuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return (targets.isEmpty() && to_select != Self);
}
void nuhuoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    room->damage(DamageStruct("nuhuo", target, source));

    QList<ServerPlayer *> all;
    foreach(ServerPlayer *p, room->getOtherPlayers(source)){
        if (source->canSlash(p, NULL, true))
            all << p;
    }
    if (!all.isEmpty()) {
        ServerPlayer *victim = room->askForPlayerChosen(target, all, "nuhuo", "@nuhuo:" + source->objectName(), false);
        QList<ServerPlayer *> logto;
        logto << victim;
        room->touhouLogmessage("#nuhuoChoose", target, "nuhuo", logto);

        Slash *slash = new Slash(Card::NoSuit, 0);
        CardUseStruct carduse;
        slash->setSkillName("_nuhuo");
        carduse.card = slash;
        carduse.from = source;
        carduse.to << victim;
        room->useCard(carduse, false);
    }
}
class nuhuo : public ZeroCardViewAsSkill {
public:
    nuhuo() : ZeroCardViewAsSkill("nuhuo") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->deleteLater();
        if (!player->isCardLimited(slash, Card::MethodUse)
            && !player->hasUsed("nuhuoCard")){
            foreach(const Player *p, player->getAliveSiblings()){
                if (player->canSlash(p, slash, true))
                    return true;
            }
        }
        return false;
    }

    virtual const Card *viewAs() const{
        return new nuhuoCard;
    }
};




th12Package::th12Package()
    : Package("th12")
{
    General *xlc001 = new General(this, "xlc001$", "xlc", 4, false);
    xlc001->addSkill(new pudu);
    xlc001->addSkill(new jiushu);
    xlc001->addSkill(new fahua);

    General *xlc002 = new General(this, "xlc002", "xlc", 3, false);
    xlc002->addSkill(new weizhi);
    xlc002->addSkill(new weizhuang);

    General *xlc003 = new General(this, "xlc003", "xlc", 4, false);
    xlc003->addSkill(new jinghua);
    xlc003->addSkill(new zhengyiEffect);
    xlc003->addSkill(new zhengyiArmor);
    xlc003->addSkill(new zhengyi);
    related_skills.insertMulti("zhengyi", "#zhengyiArmor");
    related_skills.insertMulti("zhengyi", "#zhengyi");

    General *xlc004 = new General(this, "xlc004", "xlc", 4, false);
    xlc004->addSkill(new chuannan);

    General *xlc005 = new General(this, "xlc005", "xlc", 4, false);
    xlc005->addSkill(new lizhi);
    xlc005->addSkill(new yunshang);

    General *xlc006 = new General(this, "xlc006", "xlc", 3, false);
    xlc006->addSkill(new souji);
    xlc006->addSkill(new soujiRecord);
    xlc006->addSkill(new tansuo);
    related_skills.insertMulti("souji", "#souji");

    General *xlc007 = new General(this, "xlc007", "xlc", 3, false);
    xlc007->addSkill(new yiwang);
    xlc007->addSkill(new jingxia);


    General *xlc008 = new General(this, "xlc008", "xlc", 4, true);
    xlc008->addSkill(new bianhuan);
    xlc008->addSkill(new nuhuo);

    addMetaObject<puduCard>();
    addMetaObject<weizhiCard>();
    addMetaObject<chuannanCard>();
    addMetaObject<nuhuoCard>();
}

ADD_PACKAGE(th12)

