#include "th99.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"
#include "jsonutils.h"
#include "maneuvering.h" //for skill lianxi



qiuwenCard::qiuwenCard() {
    mute = true;
    target_fixed = true;
}
void qiuwenCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->drawCards(source, 3);
}
class qiuwen : public ZeroCardViewAsSkill {
public:
    qiuwen() : ZeroCardViewAsSkill("qiuwen") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("qiuwenCard");
    }

    virtual const Card *viewAs() const{
        return new qiuwenCard;
    }
};
class zaozu : public TriggerSkill {
public:
    zaozu() : TriggerSkill("zaozu") {
        events << EventPhaseChanging << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging){
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard){
                room->notifySkillInvoked(player, objectName());
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                player->skip(Player::Discard);
            }
        }
        else if (triggerEvent == EventPhaseStart){
            if (player->getPhase() == Player::Finish &&player->getHandcardNum() > player->getMaxHp()){
                room->notifySkillInvoked(player, objectName());
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->loseHp(player);
            }
        }
        return false;
    }
};

dangjiaCard::dangjiaCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "dangjiavs";
    mute = true;
}
bool dangjiaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasLordSkill("dangjia")
        && to_select != Self && !to_select->hasFlag("dangjiaInvoked")
        && !to_select->isKongcheng() && to_select->isWounded();
}
void dangjiaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *akyu = targets.first();
    if (akyu->hasLordSkill("dangjia")) {
        room->setPlayerFlag(akyu, "dangjiaInvoked");

        room->notifySkillInvoked(akyu, "dangjia");
        if (!source->pindian(akyu, "dangjia", NULL)){
            RecoverStruct recov;
            recov.who = source;
            room->recover(akyu, recov);
        }

        QList<ServerPlayer *> akyus;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach(ServerPlayer *p, players) {
            if (p->hasLordSkill("dangjia") && !p->hasFlag("dangjiaInvoked"))
                akyus << p;
        }
        if (akyus.isEmpty())
            room->setPlayerFlag(source, "Forbiddangjia");
    }
}
class dangjiavs : public ZeroCardViewAsSkill {
public:
    dangjiavs() :ZeroCardViewAsSkill("dangjiavs") {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return  player->getKingdom() == "wai"  && !player->hasFlag("Forbiddangjia") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const{
        return new dangjiaCard;
    }
};
class dangjia : public TriggerSkill {
public:
    dangjia() : TriggerSkill("dangjia$") {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "dangjia")) {
            QList<ServerPlayer *> lords;
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty()) return false;

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach(ServerPlayer *p, players) {
                if (!p->hasSkill("dangjiavs"))
                    room->attachSkillToPlayer(p, "dangjiavs");
            }
        }
        else if (triggerEvent == EventLoseSkill && data.toString() == "dangjia") {
            QList<ServerPlayer *> lords;
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.length() > 2) return false;

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach(ServerPlayer *p, players) {
                if (p->hasSkill("dangjiavs"))
                    room->detachSkillFromPlayer(p, "dangjiavs", true);
            }
        }
        else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                return false;
            if (player->hasFlag("Forbiddangjia"))
                room->setPlayerFlag(player, "-Forbiddangjia");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach(ServerPlayer *p, players) {
                if (p->hasFlag("dangjiaInvoked"))
                    room->setPlayerFlag(p, "-dangjiaInvoked");
            }
        }
        return false;
    }
};



xiufuCard::xiufuCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "xiufu";
    mute = true;
    target_fixed = true;
}
void xiufuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{

    QList<int> able;
    QList<int> discardpile = room->getDiscardPile();
    foreach(int id, discardpile){
        Card *tmp_card = Sanguosha->getCard(id);
        if (tmp_card->isKindOf("EquipCard"))
            able << id;
    }
    if (able.isEmpty())
        return;
    room->fillAG(able, source);
    int equipid = room->askForAG(source, able, false, "xiufu");
    const EquipCard *equip = qobject_cast<const EquipCard *>(Sanguosha->getCard(equipid)->getRealCard());
    ServerPlayer *target = room->askForPlayerChosen(source, room->getAlivePlayers(), "xiufu", "@xiufu-select:" + equip->objectName(), false, true);
    room->clearAG(source);

    int equipped_id = -1;
    if (target->getEquip(equip->location()) != NULL)
        equipped_id = target->getEquip(equip->location())->getEffectiveId();
    QList<CardsMoveStruct> exchangeMove;
    if (equipped_id != -1){
        CardsMoveStruct move2(equipped_id, NULL, Player::DiscardPile,
            CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, target->objectName()));
        exchangeMove.push_back(move2);
    }
    CardsMoveStruct move1(equipid, target, Player::PlaceEquip,
        CardMoveReason(CardMoveReason::S_REASON_TRANSFER, source->objectName()));

    exchangeMove.push_back(move1);

    room->moveCardsAtomic(exchangeMove, true);

}


class xiufu : public ZeroCardViewAsSkill {
public:
    xiufu() :ZeroCardViewAsSkill("xiufu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("xiufuCard") && player->getMark("can_xiufu") > 0;
    }

    virtual const Card *viewAs() const{
        return new xiufuCard;
    }
};
//check discardpile
class xiufucheck : public TriggerSkill {
public:
    xiufucheck() : TriggerSkill("#xiufu") {
        events << CardsMoveOneTime << EventAcquireSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    static bool findEquip(Room *room){
        QList<int> discardpile = room->getDiscardPile();
        foreach(int id, discardpile){
            Card *tmp_card = Sanguosha->getCard(id);
            if (tmp_card->isKindOf("EquipCard"))
                return true;
        }
        return false;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QList<ServerPlayer *> srcs = room->getAlivePlayers();
        if (triggerEvent == CardsMoveOneTime && player != NULL && player->isAlive()) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (player->getMark("can_xiufu") > 0){
                if (move.from_places.contains(Player::DrawPile) || move.from_places.contains(Player::DiscardPile)){
                    if (!findEquip(room)){
                        foreach(ServerPlayer *p, srcs)
                            room->setPlayerMark(p, "can_xiufu", 0);
                    }
                }
            }
            else{
                if (player->getMark("can_xiufu") == 0 && move.to_place == Player::DiscardPile){
                    if (findEquip(room)){
                        foreach(ServerPlayer *p, srcs)
                            room->setPlayerMark(p, "can_xiufu", 1);
                    }
                }
            }
        }
        else if ((triggerEvent == EventAcquireSkill && data.toString() == "xiufu")){
            if (findEquip(room)){
                foreach(ServerPlayer *p, srcs)
                    room->setPlayerMark(p, "can_xiufu", 1);
            }
            else{
                foreach(ServerPlayer *p, srcs)
                    room->setPlayerMark(p, "can_xiufu", 0);
            }
        }
        return false;
    }
};

class fandu : public TriggerSkill {
public:
    fandu() : TriggerSkill("fandu") {
        events << EventPhaseStart << Damaged;
    }
    static void do_fandu(ServerPlayer *player){
        player->drawCards(2); 
        Room *room = player->getRoom();
        QList<ServerPlayer *> listt;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)){
            if (p->canDiscard(player, "h"))
                listt << p;
        }
        if (listt.isEmpty())
            return;
        ServerPlayer *target = room->askForPlayerChosen(player, listt, "fandu", "@fandu-select",false,true);
        int to_throw = room->askForCardChosen(target, player, "h", "fandu", false, Card::MethodDiscard);
        room->throwCard(to_throw, player, target);
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart){
            if (player->getPhase() == Player::Start && room->askForSkillInvoke(player, "fandu")){
                do_fandu(player);
            }
        }
        else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            for (int var = 0; var < damage.damage; var++) {
                if (!room->askForSkillInvoke(player, "fandu"))
                    break;
                do_fandu(player);
            }
        }
        return false;
    }
};


class taohuan : public TriggerSkill {
public:
    taohuan() : TriggerSkill("taohuan") {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        if (player->hasFlag("pindian") || !player->hasSkill(objectName()))
            return false;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        //case 1:discard
        int count1 = 0;
        ServerPlayer *thrower;
        if (move.from != NULL && move.to_place == Player::DiscardPile && move.from == player
            && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {

            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->objectName() == move.reason.m_playerId) {
                    thrower = p;
                    break;
                }
            }
            if (thrower != NULL && thrower != player){
                foreach(int id, move.card_ids) {
                    if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand ||
                        move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceEquip){
                        if (!Sanguosha->getCard(id)->isKindOf("BasicCard")) {
                            count1++;
                        }
                    }
                }
            }
        }
        //case 2:obtain
        //need check  move.to??
        int count2 = 0;
        ServerPlayer *obtainer;
        if (move.from != NULL && move.from == player){
            if ((move.origin_to && move.origin_to != move.from && move.origin_to_place == Player::PlaceHand)
                || (move.to && move.to != move.from && move.to_place == Player::PlaceHand)){
                Player *temp = move.to ? move.to : move.origin_to;
                foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->objectName() == temp->objectName()) {
                        obtainer = p;
                        break;
                    }
                }
                if (obtainer != NULL && obtainer != player){
                    foreach(int id, move.card_ids) {
                        if (room->getCardPlace(id) == Player::PlaceHand &&
                            (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceTable
                            || move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand
                            || move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceEquip))
                            count2++;
                    }
                }
            }

        }
        ServerPlayer *target;
        int count = 0;
        if (thrower != NULL && count1 > 0){
            target = thrower;
            count = count1;
        }
        if (obtainer != NULL && count2 > 0){
            target = obtainer;
            count = count2;
        }
        if (target == NULL)
            return false;
        player->tag["taohuantarget"] = QVariant::fromValue(target);
        while (count > 0) {
            if (player->isKongcheng() || target->isKongcheng())
                break;
            if (!room->askForSkillInvoke(player, "taohuan", QVariant::fromValue(target)))
                break;
            player->setFlags("pindian");
            count--;
            if (player->pindian(target, "taohuan", NULL) && !target->isNude()){
                int id = room->askForCardChosen(player, target, "he", objectName());
                room->obtainCard(player, id, room->getCardPlace(id) != Player::PlaceHand);
            }
        }
        player->setFlags("-pindian");
        return false;
    }
};



class shituDetect : public TriggerSkill {
public:
    shituDetect() : TriggerSkill("#shituDetect") {
        events << DamageDone << Death;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *source = room->findPlayerBySkillName("shitu");

        if (source == NULL)
            return false;
        if (room->getCurrent() == source){
            if (source->getMark("touhou-extra") > 0)
                return false;
            room->setPlayerFlag(source, "shituDamage");
        }
        return false;
    }
};

class shitu : public TriggerSkill {
public:
    shitu() : TriggerSkill("shitu") {
        events << EventPhaseChanging << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging){
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive){
                if (player->hasFlag("shituDamage")) {
                    room->setPlayerFlag(player, "-shituDamage");
                    return false;
                }

                if (player->getMark("touhou-extra") > 0){
                    player->removeMark("touhou-extra");
                    return false;
                }
                if (room->canInsertExtraTurn())
                    player->tag["ShituInvoke"] = QVariant::fromValue(true);

            }
        }
        else if (triggerEvent == EventPhaseStart &&  player->getPhase() == Player::NotActive){
            bool shitu = player->tag["ShituInvoke"].toBool();
            player->tag["ShituInvoke"] = QVariant::fromValue(false);
            if (shitu) {
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@shitu-playerchoose", true, true);

                if (target != NULL){
                    room->notifySkillInvoked(player, objectName());
                    room->setPlayerMark(target, "shitu", 1);
                    target->gainAnExtraTurn();
                    room->setPlayerMark(target, "shitu", 0);
                    //just add a phase
                    /*QList<Player::Phase> phases;
                    room->setPlayerMark(target,"touhou-extra",1);
                    phases<<Player::Draw;
                    ServerPlayer *current=room->getCurrent();
                    room->setCurrent(target);
                    target->play(phases);
                    room->setCurrent(current);
                    room->setPlayerMark(target,"touhou-extra",0);*/
                }
            }
        }
        return false;
    }
};

class mengxian : public TriggerSkill {
public:
    mengxian() : TriggerSkill("mengxian") {
        events << EventPhaseStart;
        frequency = Wake;
    }
    virtual bool triggerable(const ServerPlayer *player) const{
        return (player != NULL &&  player->hasSkill(objectName())
            && player->getMark("mengxian") == 0 && player->faceUp()
            && player->getPhase() == Player::Start);
    }
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        int n = player->getPile("jingjie").length();
        if (n >= 3){
            room->doLightbox("$mengxianAnimate", 4000);
            room->touhouLogmessage("#MengxianWake", player, objectName(), QList<ServerPlayer *>(), QString::number(n));
            room->notifySkillInvoked(player, objectName());
            room->addPlayerMark(player, objectName());
            if (room->changeMaxHpForAwakenSkill(player)){
                player->drawCards(4);
                room->handleAcquireDetachSkills(player, "luanying");
            }
        }
        return false;
    }
};

class luanying : public TriggerSkill {
public:
    luanying() : TriggerSkill("luanying") {
        frequency = Compulsory;
        events << CardUsed << SlashEffected << CardEffected << CardResponded;
    }


    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *merry = room->findPlayerBySkillName(objectName());
        if (triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (merry == NULL || merry->getPile("jingjie").length() == 0)
                return false;
            if (!use.card->isKindOf("BasicCard"))
                return false;
            if (use.from == NULL || use.from == merry || use.from->isDead())
                return false;

            QList<int> jingjies = merry->getPile("jingjie");
            room->setTag("luanying_target", QVariant::fromValue(use.from));
            merry->tag["luanying_use"] = data;
            if (room->askForSkillInvoke(merry, objectName(), data)) {
                room->removeTag("luanying_target");

                room->fillAG(jingjies, merry);//, disabled
                int card_id = -1;
                card_id = room->askForAG(merry, jingjies, true, "luanying");
                room->clearAG(merry);
                if (card_id > -1){
                    room->obtainCard(use.from, card_id, true);
                    room->touhouLogmessage("#weiya", use.from, objectName(), QList<ServerPlayer *>(), use.card->objectName());

                    room->setCardFlag(use.card, "luanyingSkillNullify");
                }
            }
        }
        else if (triggerEvent == SlashEffected){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash != NULL && effect.slash->hasFlag("luanyingSkillNullify"))
                return true;
        }
        else if (triggerEvent == CardEffected){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card != NULL && effect.card->hasFlag("luanyingSkillNullify"))
                return true;
        }
        else if (triggerEvent == CardResponded){
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if (!card_star->isKindOf("BasicCard") || data.value<CardResponseStruct>().m_isRetrial
                || data.value<CardResponseStruct>().m_isProvision)
                return false;
            if (merry == NULL || merry->getPile("jingjie").length() == 0)
                return false;
            if (player == NULL || player == merry || player->isDead())
                return false;
            QList<int> jingjies = merry->getPile("jingjie");
            room->setTag("luanying_target", QVariant::fromValue(player));
            if (room->askForSkillInvoke(merry, objectName(), data)) {
                room->removeTag("luanying_target");
                room->fillAG(jingjies, merry);
                int card_id = -1;
                card_id = room->askForAG(merry, jingjies, true, "luanying");
                room->clearAG(merry);
                if (card_id > -1){
                    room->obtainCard(player, card_id, true);
                    room->touhouLogmessage("#weiya", player, objectName(), QList<ServerPlayer *>(), card_star->objectName());
                    room->setPlayerFlag(player, "respNul");
                }
            }
        }
        return false;
    }
};




lianxiCard::lianxiCard() {
    will_throw = false;
    m_skillName = "lianxi";
    mute = true;
}
bool lianxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (targets.length() <= 1){
        QList<const Player *> &targets2 = QList<const Player *>();
        IronChain *card = new IronChain(Card::NoSuit, 0);
        card->deleteLater();
        return (card->isAvailable(Self) && !Self->isProhibited(to_select, card) && card->targetFilter(targets2, to_select, Self));
    }
    else
        return false;

}
bool lianxiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() <= 2;
}
const Card *lianxiCard::validate(CardUseStruct &use) const{
    if (use.to.length() <= 2){
        IronChain *card = new IronChain(Card::NoSuit, 0);
        card->setSkillName("lianxi");
        return card;
    }
    return use.card;
}


class lianxivs : public ZeroCardViewAsSkill {
public:
    lianxivs() :ZeroCardViewAsSkill("lianxi") {
        response_pattern = "@@lianxi";
    }

    virtual const Card *viewAs() const{
        return new lianxiCard;
    }
};
class lianxi : public TriggerSkill {
public:
    lianxi() : TriggerSkill("lianxi") {
        events << CardResponded << CardUsed;
        view_as_skill = new lianxivs;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        bool can = false;
        if (triggerEvent == CardResponded){
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isKindOf("Slash"))
                can = true;
        }
        else if (triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash"))
                can = true;
        }
        if (can)
            room->askForUseCard(player, "@@lianxi", "@lianxi");
        return false;
    }
};

class yueshi : public TriggerSkill {
public:
    yueshi() : TriggerSkill("yueshi") {
        events << EventPhaseStart;
        frequency = Wake;
    }
    virtual bool triggerable(const ServerPlayer *player) const{
        return (player != NULL &&  player->hasSkill(objectName())
            && player->getMark("yueshi") == 0 && player->faceUp()
            && player->getPhase() == Player::Start);
    }
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->isChained()){
            room->doLightbox("$yueshiAnimate", 4000);
            room->touhouLogmessage("#YueshiWake", player, "yueshi");
            room->notifySkillInvoked(player, objectName());
            room->addPlayerMark(player, objectName());
            if (room->changeMaxHpForAwakenSkill(player, 1))
                room->handleAcquireDetachSkills(player, "ruizhi");
        }
        return false;
    }
};


class pingyi : public TriggerSkill {
public:
    pingyi() : TriggerSkill("pingyi") {
        events << Damaged;
    }
    virtual int getPriority(TriggerEvent) const{
        return -1;
    }
    static void skill_comeback(Room *room, ServerPlayer *player){
        ServerPlayer * back;
        QString back_skillname;
        foreach(ServerPlayer *p, room->getOtherPlayers(player, true)){
            if (p->getMark("pingyi") > 0){
                QString pingyi_record = player->objectName() + "pingyi" + p->objectName();
                back_skillname = room->getTag(pingyi_record).toString();
                if (back_skillname != NULL && back_skillname != ""){
                    back = p;
                    room->setTag(pingyi_record, QVariant());
                    break;
                }
            }
        }

        if (back != NULL){

            room->setPlayerMark(player, "pingyi_steal", 0);
            if (back->isAlive())
                room->setPlayerMark(back, "pingyi", back->getMark("pingyi") - 1);

            room->handleAcquireDetachSkills(player, "-" + back_skillname);

            Json::Value arg(Json::arrayValue);
            arg[0] = (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
            arg[1] = QSanProtocol::Utils::toJsonString(player->objectName());
            arg[2] = QSanProtocol::Utils::toJsonString(player->getGeneral()->objectName());
            arg[3] = QSanProtocol::Utils::toJsonString("clear");
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

            if (back->isAlive())
                room->handleAcquireDetachSkills(back, back_skillname);
        }

    }
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from != NULL && damage.from->isAlive() && damage.from != player){
            if (player->isNude())
                return false;
            //if damage.from:hasSkill("chuanghuan") then return false end
            QStringList skill_names;
            skill_names << "cancel";

            foreach(const Skill *skill, damage.from->getVisibleSkillList()) {
                if (skill->isLordSkill() || skill->getFrequency() == Skill::Limited
                    || skill->getFrequency() == Skill::Wake || skill->isAttachedLordSkill()
                    || skill->getFrequency() == Skill::Eternal)
                    continue;
                else{

                    if (!player->hasSkill(skill->objectName()))
                        skill_names << skill->objectName();
                }
            }

            player->tag["pingyi_target"] = QVariant::fromValue(damage.from);

            QString skill_name = room->askForChoice(player, "pingyi", skill_names.join("+"));
            if (skill_name == "cancel")
                return false;

            const Card *card = room->askForCard(player, ".|.|.|.!", "@pingyi:" + damage.from->objectName() + ":" + skill_name, data, Card::MethodDiscard, NULL, true, objectName());
            if (card != NULL){

                if (player->getMark("pingyi_steal") > 0)
                    skill_comeback(room, player);
                //record pingyi     relation and content.
                QString pingyi_record = player->objectName() + "pingyi" + damage.from->objectName();
                room->setTag(pingyi_record, skill_name);

                room->setPlayerMark(player, "pingyi_steal", 1);
                room->setPlayerMark(damage.from, "pingyi", damage.from->getMark("pingyi") + 1);//can be stealed any times.
                room->handleAcquireDetachSkills(damage.from, "-" + skill_name);
                room->handleAcquireDetachSkills(player, skill_name);


                Json::Value arg(Json::arrayValue);
                arg[0] = (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
                arg[1] = QSanProtocol::Utils::toJsonString(player->objectName());
                arg[2] = QSanProtocol::Utils::toJsonString(damage.from->getGeneral()->objectName());
                arg[3] = QSanProtocol::Utils::toJsonString(skill_name);
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
            }
        }
        return false;
    }
};
class pingyi_handle : public TriggerSkill {
public:
    pingyi_handle() : TriggerSkill("#pingyi_handle") {
        events << EventLoseSkill << Death;
    }

    virtual int getPriority(TriggerEvent) const{ //caution other skills at Death event ,like chuancheng
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return player->getMark("pingyi") > 0 || player->getMark("pingyi_steal") > 0;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventLoseSkill && data.toString() == "pingyi"){
            if (player->getMark("pingyi_steal") > 0 && !player->hasSkill("pingyi"))
                pingyi::skill_comeback(room, player);

        }
        else if (triggerEvent == Death){
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player && player->getMark("pingyi_steal") > 0)
                pingyi::skill_comeback(room, player);
            if (death.who == player && player->getMark("pingyi") > 0){

                foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                    if (p->getMark("pingyi_steal") > 0){
                        QString pingyi_record = p->objectName() + "pingyi" + player->objectName();
                        QString back_skillname = room->getTag(pingyi_record).toString();
                        if (back_skillname != NULL && back_skillname != "")
                            pingyi::skill_comeback(room, p);
                    }
                }
            }
        }
        return false;
    }
};


zhesheCard::zhesheCard() {
    will_throw = true;
    m_skillName = "zheshe";
    mute = true;
}
bool zhesheCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() == 0;
}
void zhesheCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    room->setPlayerMark(effect.to, "zheshetransfer", 1);

    DamageStruct damage; //effect.from->tag["zhesheDamage"].value<DamageStruct>();
    damage.from = effect.from;
    damage.to = effect.to;
    //damage.transfer = true;
    damage.reason = "zheshe";
    room->damage(damage);
}


class zheshevs : public OneCardViewAsSkill {
public:
    zheshevs() :OneCardViewAsSkill("zheshe") {
        filter_pattern = ".|.|.|hand!";//"^EquipCard!" ;
        response_pattern = "@@zheshe";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        zhesheCard *card = new zhesheCard;
        card->addSubcard(originalCard);

        return card;
    }
};
class zheshe : public TriggerSkill {
public:
    zheshe() : TriggerSkill("zheshe") {
        events << DamageInflicted;
        view_as_skill = new zheshevs;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (player == damage.from)
            return false;
        //if (damage.transfer)
        //return false;
        player->tag["zhesheDamage"] = data;
        const Card *card = room->askForUseCard(player, "@@zheshe", "@zheshe", -1, Card::MethodDiscard);
        if (card != NULL)
            return true;
        return false;
    }
};
class zheshedraw : public TriggerSkill {
public:
    zheshedraw() : TriggerSkill("#zheshedraw") {
        events << DamageComplete;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        //&& damage.transfer
        if (damage.to->isAlive() && damage.to->getMark("zheshetransfer") > 0 && damage.reason == "zheshe"){
            room->setPlayerMark(damage.to, "zheshetransfer", 0);
            damage.to->drawCards(damage.to->getLostHp());
        }
        return false;
    }
};



class tanchi : public TriggerSkill {
public:
    tanchi() : TriggerSkill("tanchi") {
        events << PreHpRecover;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        RecoverStruct recover = data.value<RecoverStruct>();
        recover.recover = recover.recover + 1;
        data = QVariant::fromValue(recover);
        return false;
    }
};

zhuonongCard::zhuonongCard() {
    m_skillName = "zhuonong";
    mute = true;
}
bool zhuonongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() == 0;
}
void zhuonongCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    effect.from->tag["zhuonong_target"] = QVariant::fromValue(effect.to);
    QString choice = room->askForChoice(effect.from, "zhuonong", "rd+dr", QVariant::fromValue(effect.to));
    RecoverStruct recover;
    recover.who = effect.from;
    DamageStruct damage(objectName(), effect.from, effect.to, 1, DamageStruct::Fire);
    if (choice == "rd" && effect.to->isWounded())
        room->recover(effect.to, recover);
    room->damage(damage);
    if (choice == "dr" && effect.to->isWounded())
        room->recover(effect.to, recover);
}
class zhuonong : public ZeroCardViewAsSkill {
public:
    zhuonong() :ZeroCardViewAsSkill("zhuonong") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("zhuonongCard");
    }

    virtual const Card *viewAs() const{
        return new zhuonongCard;
    }
};

class jijing : public TriggerSkill {
public:
    jijing() : TriggerSkill("jijing") {
        events << Damaged;
        frequency = Compulsory;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *luna = room->findPlayerBySkillName(objectName());
        if (luna == NULL || luna->getPhase() == Player::NotActive)
            return false;
        if (damage.to == NULL || damage.to->isDead() || damage.to == luna)
            return false;
        if (damage.to->getMark("@jijing") == 0){
            room->notifySkillInvoked(luna, objectName());
            room->setFixedDistance(luna, damage.to, 1);
            damage.to->gainMark("@jijing", 1);
            room->setPlayerFlag(luna, "jijing");
        }
        return false;
    }
};
class jijing_clear : public TriggerSkill {
public:
    jijing_clear() : TriggerSkill("#jijing") {
        events << EventPhaseChanging;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasFlag("jijing");
    }
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive){
            foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                if (p->getMark("@jijing") > 0){
                    room->setFixedDistance(player, p, -1);
                    p->loseAllMarks("@jijing");
                }
            }
        }

        return false;
    }
};

class ganying : public TriggerSkill {
public:
    ganying() : TriggerSkill("ganying") {
        events << GameStart << EventAcquireSkill; //<< EventLoseSkill
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == GameStart || (triggerEvent == EventAcquireSkill && data.toString() == "ganying")) {
            if (player->hasSkill(objectName())){
                //reset distance record.
                room->setPlayerMark(player, "ganying_owner", 1);
                foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                    QString mark = player->objectName() + "_ganying_" + p->objectName();
                    //this data will be recorded by from
                    room->setPlayerMark(player, mark, player->distanceTo(p));
                    QString mark1 = p->objectName() + "_ganying_" + player->objectName();
                    room->setPlayerMark(p, mark1, p->distanceTo(player));
                }
                foreach(ServerPlayer *p, room->getAlivePlayers()){
                    if (!p->hasSkill("#ganying_handle"))
                        room->acquireSkill(p, "#ganying_handle");
                }
            }
        }

        return false;
    }
};

class ganying_handle : public TriggerSkill {
public:
    ganying_handle() : TriggerSkill("#ganying_handle") {
        events << EventAcquireSkill << HpChanged << Death << CardsMoveOneTime << EventPhaseChanging;
        //<< MarkChanged
    }

    static void ganying_effect(ServerPlayer *player, QList<ServerPlayer *> targets){
        Room *room = player->getRoom();
        ServerPlayer *target = room->askForPlayerChosen(player, targets, "ganying", "@ganying", true, true);
        if (target == NULL)
            return;
        int id = room->askForCardChosen(player, target, "h", "ganying", false, Card::MethodDiscard);
        room->throwCard(id, target, player);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventAcquireSkill || triggerEvent == EventLoseSkill){
            if (data.toString() == "ganying")
                return false;
        }
        if (triggerEvent == CardsMoveOneTime){
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != NULL && move.from->hasFlag("rengui_InTempMoving"))
                return false;
        }
        ;
        foreach(ServerPlayer *p, room->getAlivePlayers()){
            if (p->getMark("ganying_owner") > 0){//cause skill "changshi", we need record distance any time.
                bool distance_change_one_time = false;
                QList<ServerPlayer *> targets;

                foreach(ServerPlayer *target, room->getOtherPlayers(p)){
                    QString markname = p->objectName() + "_ganying_" + target->objectName();
                    QString markname1 = target->objectName() + "_ganying_" + p->objectName();
                    int n = p->getMark(markname);
                    int n1 = target->getMark(markname1);
                    if ((n > 0 && n != p->distanceTo(target)) ||
                        n1 > 0 && n1 != target->distanceTo(p)){
                        distance_change_one_time = true;
                        if (p->hasSkill("ganying") && p->canDiscard(target, "h"))
                            targets << target;
                    }
                    room->setPlayerMark(p, markname, p->distanceTo(target));
                    room->setPlayerMark(target, markname1, target->distanceTo(p));
                }
                if (distance_change_one_time && p->hasSkill("ganying")){
                    if (room->askForSkillInvoke(p, "ganying", data)){
                        p->drawCards(1);
                        //targets<<p;
                        ganying_effect(p, targets);
                    }
                }
            }
        }
        return false;
    }
};




panduCard::panduCard() {
    handling_method = Card::MethodUse;
    mute = true;
}
bool panduCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() == 0 && !to_select->isKongcheng() && to_select != Self;
}
void panduCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    int card_id = room->askForCardChosen(source, target, "h", "pandu");
    room->showCard(target, card_id);
    Card *showcard = Sanguosha->getCard(card_id);
    if (showcard->isKindOf("Slash")){
        if (!target->isCardLimited(showcard, Card::MethodUse))
            room->useCard(CardUseStruct(showcard, target, source), false);
    }
    else if (!showcard->isKindOf("BasicCard"))
        room->obtainCard(source, showcard, true);

}
class pandu : public ZeroCardViewAsSkill {
public:
    pandu() : ZeroCardViewAsSkill("pandu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("panduCard");
    }

    virtual const Card *viewAs() const{
        return new panduCard;
    }
};

class bihuo : public TriggerSkill {
public:
    bihuo() : TriggerSkill("bihuo") {
        events << TargetConfirming;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && !player->isKongcheng()){
            QList<ServerPlayer *> targets = room->getOtherPlayers(player);
            foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                if (use.from != NULL && p == use.from){
                    targets.removeOne(use.from);
                    continue;
                }
                if (!use.from->canSlash(p, use.card, false) || use.to.contains(p))
                    targets.removeOne(p);
            }
            if (targets.isEmpty())  return false;
            QString prompt = "@bihuo-playerchosen:" + use.from->objectName();
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), prompt, true, true);
            if (target != NULL){
                DummyCard *dummy = new DummyCard;
                dummy->deleteLater();
                dummy->addSubcards(player->getCards("h"));
                int num = dummy->subcardsLength();
                room->obtainCard(target, dummy, false);

                int tmp = player->tag["bihuo_num_" + player->objectName()].toInt();
                if (tmp == NULL)  tmp = 0;
                target->tag["bihuo_num_" + player->objectName()] = QVariant::fromValue(tmp + num);

                room->setPlayerFlag(target, "bihuo_" + player->objectName());


                use.to << target;
                use.to.removeOne(player);
                data = (QVariant::fromValue(use));

                QList<ServerPlayer *> logto;
                logto << player;
                QList<ServerPlayer *> logto1;
                logto1 << target;
                room->touhouLogmessage("$CancelTarget", use.from, use.card->objectName(), logto);
                room->touhouLogmessage("#huzhu_change", use.from, use.card->objectName(), logto1);

                room->getThread()->trigger(TargetConfirming, room, target, data);
            }
        }
        return false;
    }
};

class bihuo_return : public TriggerSkill {
public:
    bihuo_return() : TriggerSkill("#bihuo") {
        events << EventPhaseChanging;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive){
            foreach(ServerPlayer *s, room->getAlivePlayers()){
                QString flag = "bihuo_" + s->objectName();
                foreach(ServerPlayer *p, room->getOtherPlayers(s)){
                    if (p->hasFlag(flag)){
                        p->setFlags("-" + flag);
                        int num = p->tag["bihuo_num_" + s->objectName()].toInt();
                        p->tag.remove("bihuo_num_" + s->objectName());
                        if (!p->isKongcheng()){
                            int count = qMin(p->getHandcardNum(), num);
                            const Card *cards = room->askForExchange(p, "bihuo", count, false, "bihuo_exchange:" + QString::number(count) + ":" + s->objectName());
                            room->obtainCard(s, cards, false);
                        }
                    }
                }

            }
        }
        return false;
    }
};



th99Package::th99Package()
    : Package("th99")
{
    General *wai001 = new General(this, "wai001$", "wai", 3, false);
    wai001->addSkill(new qiuwen);
    wai001->addSkill(new zaozu);
    wai001->addSkill(new dangjia);

    General *wai002 = new General(this, "wai002", "wai", 4, true);
    wai002->addSkill(new xiufu);
    wai002->addSkill(new xiufucheck);
    related_skills.insertMulti("xiufu", "#xiufu");

    General *wai003 = new General(this, "wai003", "wai", 3, false);
    wai003->addSkill(new fandu);
    wai003->addSkill(new taohuan);

    General *wai004 = new General(this, "wai004", "wai", 4, false);
    wai004->addSkill(new shitu);
    wai004->addSkill(new shituDetect);
    related_skills.insertMulti("shitu", "#shituDetect");

    General *wai005 = new General(this, "wai005", "wai", 4, false);
    wai005->addSkill("jingjie");
    wai005->addSkill(new mengxian);
    wai005->addRelateSkill("luanying");

    General *wai006 = new General(this, "wai006", "wai", 3, false);
    wai006->addSkill(new lianxi);
    wai006->addSkill(new yueshi);

    General *wai007 = new General(this, "wai007", "wai", 4, false);
    wai007->addSkill(new pingyi);
    wai007->addSkill(new pingyi_handle);
    related_skills.insertMulti("pingyi", "#pingyi_handle");


    General *wai008 = new General(this, "wai008", "wai", 4, false);
    wai008->addSkill(new zheshe);
    wai008->addSkill(new zheshedraw);
    wai008->addSkill(new tanchi);
    related_skills.insertMulti("zheshe", "#zheshedraw");

    General *wai009 = new General(this, "wai009", "wai", 4, false);
    wai009->addSkill(new zhuonong);
    wai009->addSkill(new jijing);
    wai009->addSkill(new jijing_clear);
    related_skills.insertMulti("jijing", "#jijing");

    General *wai010 = new General(this, "wai010", "wai", 4, false);
    wai010->addSkill(new ganying);

    General *wai011 = new General(this, "wai011", "wai", 4, false);

    General *wai012 = new General(this, "wai012", "wai", 3, false);
    wai012->addSkill(new pandu);
    wai012->addSkill(new bihuo);
    wai012->addSkill(new bihuo_return);
    related_skills.insertMulti("bihuo", "#bihuo");

    addMetaObject<qiuwenCard>();
    addMetaObject<dangjiaCard>();
    addMetaObject<xiufuCard>();
    addMetaObject<lianxiCard>();
    addMetaObject<zhesheCard>();
    addMetaObject<zhuonongCard>();
    addMetaObject<panduCard>();
    skills << new dangjiavs << new luanying << new ganying_handle;
}

ADD_PACKAGE(th99)

