#include "formation.h"
#include "general.h"
#include "standard.h"
#include "standard-equips.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "serverplayer.h"
#include "room.h"
#include "ai.h"
#include "settings.h"
#include "maneuvering.h"

class Ziliang: public TriggerSkill {
public:
    Ziliang(): TriggerSkill("ziliang") {
        events << Damaged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QList<ServerPlayer *> dengais = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *dengai, dengais) {
            if (!player->isAlive()) break;
            if (dengai->getPile("field").isEmpty()) continue;
            if (!room->askForSkillInvoke(dengai, objectName(), data)) continue;
            room->fillAG(dengai->getPile("field"), dengai);
            int id = room->askForAG(dengai, dengai->getPile("field"), false, objectName());
            room->clearAG(dengai);
            if (player == dengai) {
                LogMessage log;
                log.type = "$MoveCard";
                log.from = player;
                log.to << player;
                log.card_str = QString::number(id);
                room->sendLog(log);
            }
            room->obtainCard(player, id);
        }
        return false;
    }
};

HuyuanCard::HuyuanCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool HuyuanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    if (!targets.isEmpty())
        return false;

    const Card *card = Sanguosha->getCard(subcards.first());
    const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
    int equip_index = static_cast<int>(equip->location());
    return to_select->getEquip(equip_index) == NULL;
}

void HuyuanCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *caohong = effect.from;
    Room *room = caohong->getRoom();
    room->moveCardTo(this, caohong, effect.to, Player::PlaceEquip,
                     CardMoveReason(CardMoveReason::S_REASON_PUT, caohong->objectName(), "huyuan", QString()));

    const Card *card = Sanguosha->getCard(subcards.first());

    LogMessage log;
    log.type = "$ZhijianEquip";
    log.from = effect.to;
    log.card_str = QString::number(card->getEffectiveId());
    room->sendLog(log);

    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getAllPlayers()) {
        if (effect.to->distanceTo(p) == 1 && caohong->canDiscard(p, "he"))
            targets << p;
    }
    if (!targets.isEmpty()) {
        ServerPlayer *to_dismantle = room->askForPlayerChosen(caohong, targets, "huyuan", "@huyuan-discard:" + effect.to->objectName());
        int card_id = room->askForCardChosen(caohong, to_dismantle, "he", "huyuan", false, Card::MethodDiscard);
        room->throwCard(Sanguosha->getCard(card_id), to_dismantle, caohong);
    }
}

class HuyuanViewAsSkill: public OneCardViewAsSkill {
public:
    HuyuanViewAsSkill(): OneCardViewAsSkill("huyuan") {
        response_pattern = "@@huyuan";
        filter_pattern = "EquipCard";
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        HuyuanCard *first = new HuyuanCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Huyuan: public PhaseChangeSkill {
public:
    Huyuan(): PhaseChangeSkill("huyuan") {
        view_as_skill = new HuyuanViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        if (target->getPhase() == Player::Finish && !target->isNude())
            room->askForUseCard(target, "@@huyuan", "@huyuan-equip", -1, Card::MethodNone);
        return false;
    }
};

/*
//Para's version
HeyiCard::HeyiCard() {
}

bool HeyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() < 2;
}

bool HeyiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

void HeyiCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *caohong = card_use.from;

    LogMessage log;
    log.from = caohong;
    log.to << card_use.to;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    QVariant data = QVariant::fromValue(card_use);
    RoomThread *thread = room->getThread();

    thread->trigger(PreCardUsed, room, caohong, data);
    thread->trigger(CardUsed, room, caohong, data);
    thread->trigger(CardFinished, room, caohong, data);
}

void HeyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->setTag("HeyiSource", QVariant::fromValue((PlayerStar)source));
    QList<ServerPlayer *> players = room->getAllPlayers();
    int index1 = players.indexOf(targets.first()), index2 = players.indexOf(targets.last());
    int index_self = players.indexOf(source);
    QList<ServerPlayer *> cont_targets;
    if (index1 == index_self || index2 == index_self) {
        forever {
            cont_targets.append(players.at(index1));
            if (index1 == index2) break;
            index1++;
            if (index1 >= players.length())
                index1 -= players.length();
        }
    } else {
        if (index1 > index2)
            qSwap(index1, index2);
        if (index_self > index1 && index_self < index2) {
            for (int i = index1; i <= index2; i++)
                cont_targets.append(players.at(i));
        } else {
            forever {
                cont_targets.append(players.at(index2));
                if (index1 == index2) break;
                index2++;
                if (index2 >= players.length())
                    index2 -= players.length();
            }
        }
    }
    cont_targets.removeOne(source);
    QStringList list;
    foreach(ServerPlayer *p, cont_targets) {
        if (!p->isAlive()) continue;
        list.append(p->objectName());
        source->tag["heyi"] = QVariant::fromValue(list);
        room->acquireSkill(p, "feiying");
    }
}

class HeyiViewAsSkill: public ZeroCardViewAsSkill {
public:
    HeyiViewAsSkill(): ZeroCardViewAsSkill("heyi") {
        response_pattern = "@@heyi";
    }

    virtual const Card *viewAs() const{
        return new HeyiCard;
    }
};

class Heyi: public TriggerSkill {
public:
    Heyi(): TriggerSkill("heyi") {
        events << EventPhaseChanging << Death;
        view_as_skill = new HeyiViewAsSkill;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        }
        if (room->getTag("HeyiSource").value<PlayerStar>() == player) {
            room->removeTag("HeyiSource");
            QStringList list = player->tag[objectName()].toStringList();
            player->tag.remove(objectName());
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (list.contains(p->objectName()))
                    room->detachSkillFromPlayer(p, "feiying", false, true);
            }
        }
        if (TriggerSkill::triggerable(player) && triggerEvent == EventPhaseChanging)
            room->askForUseCard(player, "@@heyi", "@heyi");
        return false;
    }
};
*/

HeyiCard::HeyiCard(){

}

bool HeyiCard::targetFilter(const QList<const Player *> &, const Player *to_select, const Player *Self) const{
    return to_select->isAdjacentTo(Self);
}

void HeyiCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->gainMark("@heyi_feiying");
}

class HeyiVS: public ZeroCardViewAsSkill{
public:
    HeyiVS(): ZeroCardViewAsSkill("heyi"){
        response_pattern = "@@heyi";
    }

    virtual const Card *viewAs() const{
        return new HeyiCard;
    }
};

class Heyi: public TriggerSkill{
public:
    Heyi(): TriggerSkill("heyi"){
        view_as_skill = new HeyiVS;
        events << EventPhaseStart << Death << EventLoseSkill;
    }

private:
    static void loseEffect(Room *room){
        foreach (ServerPlayer *p, room->getAlivePlayers()){
            if (p->getMark("@heyi_feiying") > 0)
                p->loseAllMarks("@heyi_feiying");
        }
    }

public:
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player)){
            if (player->getPhase() == Player::RoundStart)
                loseEffect(room);
            else if (player->getPhase() == Player::Start)
                room->askForUseCard(player, "@@heyi", "@heyi");
        }
        else {
            bool loseeffect = false;
            if (triggerEvent == Death){
                if (data.value<DeathStruct>().who == player)
                    loseeffect = true;
            }
            else if (triggerEvent == EventLoseSkill){
                if (data.toString() == objectName())
                    loseeffect = true;
            }
            if (loseeffect)
                loseEffect(room);
        }
        return false;
    }
};
class HeyiAcquireDetach: public TriggerSkill{
public:
    HeyiAcquireDetach(): TriggerSkill("#heyi-acquire-detach"){
        events << MarkChanged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        MarkChangeStruct change = data.value<MarkChangeStruct>();
        if (change.name == "@heyi_feiying"){
            int kanpo = player->getMark("@heyi_feiying");
            if (kanpo > 0 && kanpo - change.num == 0){
                room->acquireSkill(player, "feiying");
            }
            else if (kanpo == 0 && change.num < 0){
                if (!player->hasInnateSkill("feiying"))
                    room->detachSkillFromPlayer(player, "feiying");
            }
        }
        return false;
    }
};

class Tianfu: public TriggerSkill {
public:
    Tianfu(): TriggerSkill("tianfu") {
        events << EventPhaseStart << EventLoseSkill;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart){
            if (player->getPhase() == Player::RoundStart){
                QList<ServerPlayer *> jiangweis = room->findPlayersBySkillName(objectName());
                foreach (ServerPlayer *jiangwei, jiangweis){
                    if (jiangwei->isAdjacentTo(player) || jiangwei == player)
                        jiangwei->gainMark("@tianfu_kanpo");
                }
            }
            else if (player->getPhase() == Player::NotActive){
                foreach (ServerPlayer *p, room->getAllPlayers()){
                    if (p->getMark("@tianfu_kanpo") > 0){
                        p->loseAllMarks("@tianfu_kanpo");
                    }
                }
            }
        }
        else {
            if (data.toString() == "tianfu" && player->getMark("@tianfu_kanpo") > 0){
                player->loseAllMarks("@tianfu_kanpo");
            }
        }
        return false;
    }
};
class TianfuAcquireDetach: public TriggerSkill{
public:
    TianfuAcquireDetach(): TriggerSkill("#tianfu-acquire-detach"){
        events << MarkChanged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        MarkChangeStruct change = data.value<MarkChangeStruct>();
        if (change.name == "@tianfu_kanpo"){
            int kanpo = player->getMark("@tianfu_kanpo");
            if (kanpo > 0 && kanpo - change.num == 0){
                room->acquireSkill(player, "kanpo");
            }
            else if (kanpo == 0 && change.num < 0){
                if (!player->hasInnateSkill("kanpo"))
                    room->detachSkillFromPlayer(player, "kanpo");
            }
        }
        return false;
    }
};

class Yicheng: public TriggerSkill {
public:
    Yicheng(): TriggerSkill("yicheng") {
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash")) return false;
        foreach (ServerPlayer *p, use.to) {
            if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(p))) {
                p->drawCards(1);
                if (p->isAlive() && p->canDiscard(p, "he"))
                    room->askForDiscard(p, objectName(), 1, 1, false, true);
            }
            if (!player->isAlive())
                break;
        }
        return false;
    }
};

class Qianhuan: public TriggerSkill {
public:
    Qianhuan(): TriggerSkill("qianhuan") {
        events << Damaged << TargetConfirming;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Damaged && player->isAlive()) {
            ServerPlayer *yuji = room->findPlayerBySkillName(objectName());
            if (yuji && room->askForSkillInvoke(player, objectName(), "choice:" + yuji->objectName())) {
                room->broadcastSkillInvoke(objectName());
                if (yuji != player) {
                    room->notifySkillInvoked(yuji, objectName());
                    LogMessage log;
                    log.type = "#InvokeOthersSkill";
                    log.from = player;
                    log.to << yuji;
                    log.arg = objectName();
                    room->sendLog(log);
                }

                int id = room->drawCard();
                Card::Suit suit = Sanguosha->getCard(id)->getSuit();
                bool duplicate = false;
                foreach (int card_id, yuji->getPile("sorcery")) {
                    if (Sanguosha->getCard(card_id)->getSuit() == suit) {
                        duplicate = true;
                        break;
                    }
                }
                yuji->addToPile("sorcery", id);
                if (duplicate) {
                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
                    room->throwCard(Sanguosha->getCard(id), reason, NULL);
                }
            }
        } else if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card || use.card->getTypeId() == Card::TypeEquip || use.card->getTypeId() == Card::TypeSkill)
                return false;
            if (use.to.length() != 1) return false;
            ServerPlayer *yuji = room->findPlayerBySkillName(objectName());
            if (!yuji || yuji->getPile("sorcery").isEmpty()) return false;
            if (room->askForSkillInvoke(yuji, objectName(), data)) {
                room->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(yuji, objectName());
                QList<int> ids = yuji->getPile("sorcery");
                int id = -1;
                if (ids.length() > 1) {
                    room->fillAG(ids, yuji);
                    id = room->askForAG(yuji, ids, false, objectName());
                    room->clearAG(yuji);
                } else {
                    id = ids.first();
                }
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
                room->throwCard(Sanguosha->getCard(id), reason, NULL);

                LogMessage log;
                if (use.from) {
                    log.type = "$CancelTarget";
                    log.from = use.from;
                } else {
                    log.type = "$CancelTargetNoUser";
                }
                log.to = use.to;
                log.arg = use.card->objectName();
                room->sendLog(log);

                use.to.clear();
                data = QVariant::fromValue(use);
            }
        }
        return false;
    }
};


class Zhendu: public TriggerSkill {
public:
    Zhendu(): TriggerSkill("zhendu") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::Play)
            return false;
        QList<ServerPlayer *> hetaihous = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *hetaihou, hetaihous) {
            if (!hetaihou->isAlive() || !hetaihou->canDiscard(hetaihou, "h") || hetaihou->getPhase() == Player::Play)
                continue;
            if (room->askForCard(hetaihou, ".", "@zhendu-discard", QVariant(), objectName())) {
                Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
                analeptic->setSkillName("_zhendu");
                room->useCard(CardUseStruct(analeptic, player, QList<ServerPlayer *>(), true));
                if (player->isAlive())
                    room->loseHp(player);
                if (player->getHp() < hetaihou->getHp())
                    hetaihou->drawCards(1);
            }
        }
        return false;
    }
};

class Qiluan: public TriggerSkill {
public:
    Qiluan(): TriggerSkill("qiluan") {
        events << Death << EventPhaseStart;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;

            ServerPlayer *current = room->getCurrent();
            if (current && (current->isAlive() || death.who == current) && current->getPhase() != Player::NotActive){
                foreach(ServerPlayer *p, room->getAllPlayers())
                    if (TriggerSkill::triggerable(p))
                        room->setPlayerMark(p, objectName(), 1);
            }
        } else {
            if (player->getPhase() == Player::NotActive) {
                QList<ServerPlayer *> hetaihous;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark(objectName()) > 0 && TriggerSkill::triggerable(p))
                        hetaihous << p;
                    room->setPlayerMark(p, objectName(), 0);
                }

                foreach (ServerPlayer *p, hetaihous) {
                    if (p->isAlive() && room->askForSkillInvoke(p, objectName())){
                        QStringList kingdoms;
                        ServerPlayer *lord = room->getLord();
                        foreach (ServerPlayer *other, room->getAlivePlayers()){
                            if (!kingdoms.contains(other->getKingdom()) && other->getKingdom() != lord->getKingdom())
                                kingdoms << other->getKingdom();
                        }
                        p->drawCards(kingdoms.length());
                    }
                }
            }
        }
        return false;
    }
};


class Shengxi: public TriggerSkill {
public:
    Shengxi(): TriggerSkill("shengxi") {
        events << DamageDone << EventPhaseStart;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart) {
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish) {
                if (!player->hasFlag("ShengxiDamageInPlayPhase") && player->askForSkillInvoke(objectName()))
                    player->drawCards(2);
            }
            if (player->hasFlag("ShengxiDamageInPlayPhase"))
                player->setFlags("-ShengxiDamageInPlayPhase");
        } else if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->getPhase() == Player::Play && !damage.from->hasFlag("ShengxiDamageInPlayPhase"))
                damage.from->setFlags("ShengxiDamageInPlayPhase");
        }
        return false;
    }
};

class Shoucheng: public TriggerSkill {
public:
    Shoucheng(): TriggerSkill("shoucheng") {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from && move.from->isAlive() && move.from->getPhase() == Player::NotActive
            && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue((ServerPlayer *)move.from))) {
                room->broadcastSkillInvoke(objectName());
                ServerPlayer *from = (ServerPlayer *)move.from;
                from->drawCards(1);
            }
        }
        return false;
    }
};


ShangyiCard::ShangyiCard(){
}

void ShangyiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    if (effect.from->isKongcheng() || effect.to->isKongcheng())
        return ;

    room->showAllCards(effect.from, effect.to);

    QList<int> ids;
    foreach (int card_id, effect.to->handCards()){
        if (Sanguosha->getCard(card_id)->isBlack())
            ids << card_id;
    }

    int to_throw = room->doGongxin(effect.from, effect.to, ids, "shangyi");
    effect.from->tag.remove("shangyi");

    if (to_throw != -1)
        room->throwCard(to_throw, effect.to, effect.from);
    else {
        const Card *card1 = room->askForExchange(effect.from, "shangyi", 1, false, "@shangyi-swap");
        const Card *card2 = room->askForExchange(effect.to, "shangyi", 1, false, "@shangyi-swap");
        CardsMoveStruct move1(card1->getEffectiveId(), effect.to, Player::PlaceHand, 
            CardMoveReason(CardMoveReason::S_REASON_SWAP, effect.from->objectName(), effect.to->objectName(), "shangyi", QString()));
        CardsMoveStruct move2(card2->getEffectiveId(), effect.from, Player::PlaceHand, 
            CardMoveReason(CardMoveReason::S_REASON_SWAP, effect.to->objectName(), effect.from->objectName(), "shangyi", QString()));

        QList<CardsMoveStruct> exchangeMove;
        exchangeMove << move1 << move2;

        room->moveCards(exchangeMove, false);
    }
}

class Shangyi: public ZeroCardViewAsSkill{
public:
    Shangyi(): ZeroCardViewAsSkill("shangyi"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ShangyiCard");
    }

    virtual const Card *viewAs() const{
        return new ShangyiCard;
    }
};

class Niaoxiang: public TriggerSkill{
public:
    Niaoxiang(): TriggerSkill("niaoxiang"){
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent , Room *, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card != NULL && use.card->isKindOf("Slash") && use.from != NULL){
            QVariantList jink_list = use.from->tag["Jink_" + use.card->toString()].toList();
            foreach(ServerPlayer *p, use.to){
                if (player->isAdjacentTo(p) && player->askForSkillInvoke(objectName(), QVariant::fromValue(p))){
                    if (jink_list[use.to.indexOf(p)].toInt() == 1)
                        jink_list[use.to.indexOf(p)] = 2;
                }
            }
            use.from->tag["Jink_" + use.card->toString()] = jink_list;
        }
        return false;
    }
};

class Zhangwu: public TriggerSkill{
public:
    Zhangwu(): TriggerSkill("zhangwu"){
        events << CardsMoveOneTime << BeforeCardsMove;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to_place == Player::DrawPile)
            return false;
        int fldfid = -1;
        foreach (int id, move.card_ids)
            if (Sanguosha->getCard(id)->isKindOf("DragonPhoenix")){
                fldfid = id;
                break;
            }

        if (fldfid == -1)
            return false;

        if (triggerEvent == CardsMoveOneTime){
            if (move.to_place == Player::DiscardPile || (move.to_place == Player::PlaceEquip && move.to != player))
                player->obtainCard(Sanguosha->getCard(fldfid));
        }
        else {
            if ((move.from == player && (move.from_places[move.card_ids.indexOf(fldfid)] == Player::PlaceHand || move.from_places[move.card_ids.indexOf(fldfid)] == Player::PlaceEquip))
                    && (move.to != player || (move.to_place != Player::PlaceHand && move.to_place != Player::PlaceEquip && move.to_place != Player::DrawPile))
                    /*&& player->askForSkillInvoke(objectName())*/){ // infinity loop
                room->showCard(player, fldfid);
                move.from_places.removeAt(move.card_ids.indexOf(fldfid));
                move.card_ids.removeOne(fldfid);
                data = QVariant::fromValue(move);
                QList<int> to_move;
                to_move << fldfid;
                room->moveCardsToEndOfDrawpile(to_move);
                room->drawCards(player, 2);
            }
        }
        return false;
    }
};

class ShouYue: public AttackRangeSkill{
public:
    ShouYue(): AttackRangeSkill("shouyue$"){

    }

    virtual int getExtra(const Player *target, bool ) const{
        QList<const Player *> players = target->getAliveSiblings();
        //players << target;
        foreach(const Player *p, players){
            if (p->hasLordSkill(objectName()) && target->getKingdom() == "shu")
                return 1;
        }
        return 0;
    }
};

class Jizhao: public TriggerSkill{
public:
    Jizhao(): TriggerSkill("jizhao"){
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@jizhao";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getMark(limit_mark) > 0;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.who != player)
            return false;

        if (player->askForSkillInvoke(objectName(), data)){
            player->loseMark(limit_mark);
            if (player->getHandcardNum() < player->getMaxHp())
                room->drawCards(player, player->getMaxHp() - player->getHandcardNum());

            if (player->getHp() < 2){
                RecoverStruct rec;
                rec.recover = 2 - player->getHp();
                rec.who = player;
                room->recover(player, rec);
            }
            
            room->handleAcquireDetachSkills(player, "-shouyue|rende|neo2013renwang");
            if (player->isLord())
                room->handleAcquireDetachSkills(player, "jijiang");
        }
        return false;
    }
};

FormationPackage::FormationPackage()
    : Package("formation")
{
    General *heg_jiangwei = new General(this, "heg_jiangwei", "shu"); // SHU 012 G
    heg_jiangwei->addSkill("tiaoxin");
    heg_jiangwei->addSkill("zhiji");
    heg_jiangwei->addSkill(new Tianfu);
    heg_jiangwei->addSkill(new TianfuAcquireDetach);
    related_skills.insertMulti("tianfu", "#tianfu-acquire-detach");

    General *heg_dengai = new General(this, "heg_dengai", "wei"); // WEI 015 G
    heg_dengai->addSkill("tuntian");
    heg_dengai->addSkill("zaoxian");
    heg_dengai->addRelateSkill("jixi");
    heg_dengai->addSkill(new Ziliang);

    General *heg_caohong = new General(this, "heg_caohong", "wei"); // WEI 018
    heg_caohong->addSkill("yuanhu");
    heg_caohong->addSkill(new Heyi);
    heg_caohong->addSkill(new HeyiAcquireDetach);
    related_skills.insertMulti("heyi", "#heyi-acquire-detach");

    General *jiangwanfeiyi = new General(this, "jiangwanfeiyi", "shu", 3); // SHU 018
    jiangwanfeiyi->addSkill(new Shengxi);
    jiangwanfeiyi->addSkill(new Shoucheng);

    General *jiangqin = new General(this, "jiangqin", "wu", 4);
    jiangqin->addSkill(new Shangyi);
    jiangqin->addSkill(new Niaoxiang);

    General *heg_xusheng = new General(this, "heg_xusheng", "wu"); // WU 020
    heg_xusheng->addSkill("pojun");
    heg_xusheng->addSkill(new Yicheng);

    General *heg_yuji = new General(this, "heg_yuji", "qun", 3); // QUN 011 G
    heg_yuji->addSkill(new Qianhuan);

    General *hetaihou = new General(this, "hetaihou", "qun", 3, false); // QUN 020
    hetaihou->addSkill(new Zhendu);
    hetaihou->addSkill(new Qiluan);

    General *heg_liubei = new General(this, "heg_liubei$", "shu", 4);
    heg_liubei->addSkill(new Zhangwu);
    heg_liubei->addSkill(new ShouYue);
    heg_liubei->addSkill(new Jizhao);

    addMetaObject<HuyuanCard>();
    addMetaObject<HeyiCard>();
    addMetaObject<ShangyiCard>();

    skills << new Huyuan; // for future use
}

ADD_PACKAGE(Formation)