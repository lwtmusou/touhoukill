#include "th13.h"

#include "th10.h"
#include "general.h"
#include "settings.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"
#include "jsonutils.h"
#include "maneuvering.h"

class Shengge : public TriggerSkill
{
public:
    Shengge() : TriggerSkill("shengge")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        
        if (player->getMark(objectName()) == 0 && player->getPhase() == Player::Start && player->isKongcheng())
            return QStringList(objectName());
        return QStringList();
    }
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->addPlayerMark(player, objectName());
        room->doLightbox("$shenggeAnimate", 4000);
        room->touhouLogmessage("#ShenggeWake", player, objectName());
        room->notifySkillInvoked(player, objectName());
        if (room->changeMaxHpForAwakenSkill(player))
            player->drawCards(3);
        return false;
    }
};

QingtingCard::QingtingCard()
{
    mute = true;
    target_fixed = true;
}
void QingtingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
        if (p->isKongcheng())
            continue;
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), p->objectName());
    }
    foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
        if (p->isKongcheng())
            continue;
        const Card *card;
        if (source->getMark("shengge") == 0) {
            p->tag["qingting_give"] = QVariant::fromValue(source);
            card = room->askForExchange(p, "qingting", 1, false, "qingtingGive:" + source->objectName());
            p->tag.remove("qingting_give");
        } else
            card = Sanguosha->getCard(room->askForCardChosen(source, p, "h", "qingting"));

        source->obtainCard(card, false);
        room->setPlayerMark(p, "@qingting", 1);

    }
    foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
        if (p->getMark("@qingting") == 0)
            continue;
        room->setPlayerMark(p, "@qingting", 0);
        if (source->isKongcheng())
            continue;

        source->tag["qingting_return"] = QVariant::fromValue(p);
        const Card *card = room->askForExchange(source, "qingting", 1, false, "qingtingReturn:" + p->objectName());
        source->tag.remove("qingting_return");
        p->obtainCard(card, false);
    }

}

class Qingting : public ZeroCardViewAsSkill
{
public:
    Qingting() : ZeroCardViewAsSkill("qingting")
    {
    }
    static bool checkQingting(const Player *player)
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (!p->isKongcheng())
                return true;
        }
        return false;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("QingtingCard") && checkQingting(player);
    }

    virtual const Card *viewAs() const
    {
        return new QingtingCard;
    }
};

class Chiling : public TriggerSkill
{
public:
    Chiling() : TriggerSkill("chiling$")
    {
        events << CardsMoveOneTime;
    }
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        if (move.from && move.from->hasLordSkill(objectName()) &&
            move.from == player && move.from_places.contains(Player::PlaceHand) 
            && (move.to_place == Player::PlaceHand && move.to && move.to != player
            && move.to->getKingdom() == "slm")) {
            return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        //transfer player to serverplayer
        ServerPlayer *target;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->objectName() == move.to->objectName())
               target = p;
        }

        
        bool isSlash = false;
        foreach (int id, move.card_ids) {
            if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand){
                if (Sanguosha->getCard(id)->isKindOf("Slash")){
                    isSlash = true;
                    break;
                }
            }
        }

        if (isSlash)
            player->tag["chiling_showslash"] = QVariant::fromValue(1);
        else
            player->tag["chiling_showslash"] = QVariant::fromValue(0);
        
        player->tag["chiling_givener"] = QVariant::fromValue(target);
        return room->askForSkillInvoke(player, objectName(), "showcard:" + target->objectName());
    }
        
                            
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        //transfer player to serverplayer
        ServerPlayer *target;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->objectName() == move.to->objectName())
                target = p;
        }
            
        foreach (int id, move.card_ids) {
            if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand)
                room->showCard(target, id);
        }

                
        int  can_slash = player->tag["chiling_showslash"].toInt();
        if (can_slash && can_slash > 0)
            room->askForUseCard(target, "slash", "@chiling:" + player->objectName(), -1, Card::MethodUse, false);
        return false;
    }
};



class XihuaClear : public TriggerSkill
{
public:
    XihuaClear() : TriggerSkill("#xihua_clear")
    {
        events << EventPhaseChanging;
    }
    static QString xihua_pattern_trans(QString pattern)
    {
        QString pattern1 = "";
        if (pattern.contains("slash"))
            pattern1 = "Slash";
        else if (pattern == "jink")
            pattern1 = "Jink";
        else if (pattern == "peach")
            pattern1 = "Peach";
        else if (pattern == "analeptic")
            pattern1 = "Analeptic";
        else if (pattern == "amazing_grace")
            pattern1 = "AmazingGrace";
        else if (pattern == "god_salvation")
            pattern1 = "GodSalvation";
        else if (pattern == "savage_assault")
            pattern1 = "SavageAssault";
        else if (pattern == "archery_attack")
            pattern1 = "ArcheryAttack";
        else if (pattern == "duel")
            pattern1 = "Duel";
        else if (pattern == "ex_nihilo")
            pattern1 = "ExNihilo";
        else if (pattern == "snatch")
            pattern1 = "Snatch";
        else if (pattern == "dismantlement")
            pattern1 = "Dismantlement";
        else if (pattern == "collateral")
            pattern1 = "Collateral";
        else if (pattern == "iron_chain")
            pattern1 = "IronChain";
        else if (pattern == "fire_attack")
            pattern1 = "FireAttack";
        else if (pattern == "nullification")
            pattern1 = "Nullification";
        return pattern1;
    }
    static void xihua_limit(Room *room, ServerPlayer *player, QString pattern)
    {
        QString upper_pattern = xihua_pattern_trans(pattern);
        QString markName = "xihua_limit_" + upper_pattern;
        room->setPlayerMark(player, markName, 1);
        room->setPlayerCardLimitation(player, "use,response", upper_pattern, false);
    }
    static bool xihua_choice_limit(const Player *player, QString pattern, Card::HandlingMethod method)
    {
        QString upper_pattern = xihua_pattern_trans(pattern);
        QString markName = "xihua_limit_" + upper_pattern;
        if (method == NULL)
            method = Card::MethodUse;
        if (player->getMark(markName) > 0 ||
            player->isCardLimited(Sanguosha->cloneCard(pattern), method, true))
            return true;
        else
            return false;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            QStringList patterns;
            QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
            foreach (const Card *card, cards) {
                QString name;
                if (card->isKindOf("Slash"))
                    name = "slash";
                else
                    name = card->objectName();

                patterns << name;
            }

            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                foreach (QString pattern, patterns) {
                    QString upper_pattern = xihua_pattern_trans(pattern);
                    QString markName = "xihua_limit_" + upper_pattern;
                    if (p->getMark(markName) > 0) {
                        room->setPlayerMark(p, markName, 0);
                        room->removePlayerCardLimitation(p, "use,response", upper_pattern + "$0");
                    }
                }
            }
        }
        return QStringList();
    }
};

XihuaCard::XihuaCard()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool XihuaCard::do_xihua(ServerPlayer *tanuki) const
{
    Room *room = tanuki->getRoom();
    Card *xihuacard = Sanguosha->cloneCard(tanuki->tag["xihua_choice"].toString());

    ServerPlayer *target = room->askForPlayerChosen(tanuki, room->getOtherPlayers(tanuki), "xihua", "@xihua_chosen:" + xihuacard->objectName(), false, true);
    int to_show = room->askForCardChosen(target, tanuki, "h", "xihua");
    room->showCard(tanuki, to_show);

    room->getThread()->delay();
    Card *card = Sanguosha->getCard(to_show);

    bool success = false;
    if (card->getNumber() > 10)
        success = true;
    else if (card->isKindOf("TrickCard") && xihuacard->isKindOf("TrickCard"))
        success = true;
    else if (card->isKindOf("BasicCard") && xihuacard->isKindOf("BasicCard"))
        success = true;

    tanuki->tag["xihua_id"] = QVariant::fromValue(to_show);
    if (!success) {
        room->throwCard(to_show, tanuki);
        ServerPlayer *current = room->getCurrent();
        if (current && current->isAlive() && current->getPhase() != Player::NotActive)
            XihuaClear::xihua_limit(room, tanuki, xihuacard->objectName());
        room->touhouLogmessage("#Xihua_failed", tanuki, "xihua", QList<ServerPlayer *>(), xihuacard->objectName());
    }


    return success;
}

bool XihuaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{

    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            const Card *oc = Sanguosha->getCard(subcards.first());
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), oc->getSuit(), oc->getNumber());
            card->setSkillName("xihua");
            return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *card = Self->tag.value("xihua").value<const Card *>();
    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(card->objectName(), oc->getSuit(), oc->getNumber());
    new_card->addSubcard(oc);
    new_card->setSkillName("xihua");
    if (new_card->targetFixed())
        return false;
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool XihuaCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            const Card *oc = Sanguosha->getCard(subcards.first());
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), oc->getSuit(), oc->getNumber());
            card->setSkillName("xihua");
            return card && card->targetFixed();
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("xihua").value<const Card *>();
    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(card->objectName(), oc->getSuit(), oc->getNumber());
    new_card->addSubcard(oc);
    new_card->setSkillName("xihua");
    //return false defaultly
    //we need a confirming chance to pull back, since  this is a zero cards viewas Skill.
    return false;//new_card && new_card->targetFixed();
}

bool XihuaCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            const Card *oc = Sanguosha->getCard(subcards.first());
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), oc->getSuit(), oc->getNumber());
            card->setSkillName("xihua");
            return card && card->targetsFeasible(targets, Self);
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("xihua").value<const Card *>();
    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(card->objectName(), oc->getSuit(), oc->getNumber());
    new_card->addSubcard(oc);
    new_card->setSkillName("xihua");
    if (card->isKindOf("IronChain") && targets.length() == 0)
        return false;
    return new_card && new_card->targetsFeasible(targets, Self);
}

const Card *XihuaCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *xihua_general = card_use.from;

    Room *room = xihua_general->getRoom();
    QString to_use = user_string;

    LogMessage log;
    log.type = card_use.to.isEmpty() ? "#XihuaNoTarget" : "#Xihua";
    log.from = xihua_general;
    log.to = card_use.to;
    log.arg = to_use;
    log.arg2 = "xihua";
    room->sendLog(log);

    xihua_general->tag["xihua_choice"] = QVariant::fromValue(to_use);
    bool success = do_xihua(xihua_general);
    if (success) {
        Card *use_card = Sanguosha->cloneCard(to_use);
        use_card->setSkillName("xihua");
        use_card->addSubcard(xihua_general->tag["xihua_id"].toInt());
        use_card->deleteLater();

        return use_card;
    } else
        return NULL;
}

const Card *XihuaCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();

    QString to_use;
    if (user_string == "peach+analeptic") {
        QStringList use_list;
        if (!XihuaClear::xihua_choice_limit(user, "peach", Card::MethodResponse))
            use_list << "peach";
        if (!Config.BanPackages.contains("maneuvering")
            && !XihuaClear::xihua_choice_limit(user, "analeptic", Card::MethodResponse))
            use_list << "analeptic";
        to_use = room->askForChoice(user, "xihua_skill_saveself", use_list.join("+"));
    } else
        to_use = user_string;


    LogMessage log;
    log.type = "#XihuaNoTarget";
    log.from = user;
    log.arg = to_use;
    log.arg2 = "xihua";
    room->sendLog(log);


    user->tag["xihua_choice"] = QVariant::fromValue(to_use);
    bool success = do_xihua(user);
    if (success) {
        Card *use_card = Sanguosha->cloneCard(to_use);
        use_card->setSkillName("xihua");
        use_card->addSubcard(user->tag["xihua_id"].toInt());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}



class Xihua : public ZeroCardViewAsSkill
{
public:
    Xihua() : ZeroCardViewAsSkill("xihua")
    {
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->isKongcheng() || pattern.startsWith(".") || pattern.startsWith("@")) return false;
        if (pattern == "peach" && player->getMark("Global_PreventPeach") > 0) return false;
        for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false; // This is an extremely dirty hack!! For we need to prevent patterns like 'BasicCard'
        }
        //just check basic card
        if (pattern == "slash")
            return !(XihuaClear::xihua_choice_limit(player, "slash", Card::MethodResponse));
        if (pattern == "jink")
            return !XihuaClear::xihua_choice_limit(player, "jink", Card::MethodResponse);
        if (pattern.contains("peach") && pattern.contains("analeptic"))
            return (!XihuaClear::xihua_choice_limit(player, "peach", Card::MethodResponse) ||
            !XihuaClear::xihua_choice_limit(player, "analeptic", Card::MethodResponse));
        else if (pattern == "peach")
            return !XihuaClear::xihua_choice_limit(player, "peach", Card::MethodResponse);
        else if (pattern == "analeptic")
            return !XihuaClear::xihua_choice_limit(player, "analeptic", Card::MethodResponse);
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern.contains("slash")) {
                const Card *c = Self->tag.value("xihua").value<const Card *>();
                if (c)
                    pattern = c->objectName();
                else
                    return NULL;
            }
            XihuaCard *card = new XihuaCard;
            card->setUserString(pattern);
            return card;
        }

        CardStar c = Self->tag.value("xihua").value<CardStar>();
        //we need get the real subcard.
        if (c) {
            XihuaCard *card = new XihuaCard;
            card->setUserString(c->objectName());
            return card;
        } else
            return NULL;
    }

    virtual QDialog *getDialog() const
    {
        return QijiDialog::getInstance("xihua");
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        if (player->isKongcheng())
            return false;
        QString pattern = "nullification";
        if (XihuaClear::xihua_choice_limit(player, pattern, Card::MethodResponse))
            return false;
        return true;
    }
};

ShijieCard::ShijieCard()
{
    will_throw = true;
    target_fixed = true;
    handling_method = Card::MethodDiscard;
    mute = true;
}
void ShijieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *who = room->getCurrentDyingPlayer();
    JudgeStruct judge;
    judge.reason = "shijie";
    judge.who = who;
    judge.good = true;
    judge.pattern = ".|.|.";
    room->judge(judge);
    QList<ServerPlayer *> listt;
    foreach (ServerPlayer *p, room->getAlivePlayers()) {
        foreach (const Card *c, p->getCards("e")) {
            if (judge.card->getSuit() == c->getSuit() && source->canDiscard(p, c->getEffectiveId()))
                listt << p;
        }
    }
    if (!listt.isEmpty()) {
        ServerPlayer * target = room->askForPlayerChosen(source, listt, "shijie", "@@shijie_chosen", false, true);
        if (target == NULL)
            return;
        QList<int> disabled_ids;
        foreach (const Card *c, target->getCards("e")) {
            if (judge.card->getSuit() != c->getSuit() || !source->canDiscard(target, c->getEffectiveId()))
                disabled_ids << c->getEffectiveId();
        }
        //for ai
        int suit_id;
        if (judge.card->getSuit() == Card::Spade)
            suit_id = 1;
        else if (judge.card->getSuit() == Card::Heart)
            suit_id = 2;
        else if (judge.card->getSuit() == Card::Club)
            suit_id = 3;
        else if (judge.card->getSuit() == Card::Diamond)
            suit_id = 4;
        source->tag["shijie_suit"] = QVariant::fromValue(suit_id);

        int id = room->askForCardChosen(source, target, "e", "shijie",
            false, Card::MethodDiscard, disabled_ids);
        room->throwCard(id, target, source);
        RecoverStruct recover;
        recover.recover = 1;
        recover.who = source;
        room->recover(who, recover);
    }
}

class Shijie : public OneCardViewAsSkill
{
public:
    Shijie() : OneCardViewAsSkill("shijie")
    {
        filter_pattern = ".|.|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern.contains("peach") && !player->isKongcheng();
    }


    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            ShijieCard *card = new ShijieCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }
};

class Fengshui : public TriggerSkill
{
public:
    Fengshui() : TriggerSkill("fengshui")
    {
        events << AskForRetrial;
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName(), data);
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        JudgeStar judge = data.value<JudgeStar>();
        QList<int> list = room->getNCards(2);
        if (judge->reason == "shijie")
            player->setFlags("shijie_judge");
        room->askForGuanxing(player, list, Room::GuanxingBothSides, objectName());

        if (player->askForSkillInvoke("fengshui_retrial", data)) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), judge->who->objectName());

            player->setFlags("-shijie_judge");
            QList<int> list1 = room->getNCards(1);
            Card *card = Sanguosha->getCard(list1.first());
            room->retrial(card, player, judge, objectName());
        }
        player->setFlags("-shijie_judge");
        return false;
    }
};


LeishiCard::LeishiCard()
{
    handling_method = Card::MethodUse;
    mute = true;
}
bool LeishiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    ThunderSlash *slash = new ThunderSlash(Card::NoSuit, 0);
    slash->deleteLater();
    return targets.length() == 0 && !to_select->isKongcheng() && Self->canSlash(to_select, slash, false);
}
void LeishiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    ThunderSlash *slash = new ThunderSlash(Card::NoSuit, 0);
    slash->setFlags("leishislash");
    slash->setSkillName("_leishi");
    room->useCard(CardUseStruct(slash, effect.from, effect.to), false);
}

class LeishiVS : public ZeroCardViewAsSkill
{
public:
    LeishiVS() : ZeroCardViewAsSkill("leishi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        ThunderSlash *slash = new ThunderSlash(Card::NoSuit, 0);
        slash->deleteLater();
        return !player->hasUsed("LeishiCard") && !player->isCardLimited(slash, Card::MethodUse);
    }

    virtual const Card *viewAs() const
    {
        return new LeishiCard;
    }
};
class Leishi : public TriggerSkill
{
public:
    Leishi() : TriggerSkill("leishi")
    {
        events << SlashMissed;
        view_as_skill = new LeishiVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        if (effect.slash->hasFlag("leishislash")) 
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        room->loseHp(effect.from, 1);
        return false;
    }
};

class Fenyuan : public TriggerSkill
{
public:
    Fenyuan() : TriggerSkill("fenyuan")
    {
        events << Dying;
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
        ServerPlayer *current = room->getCurrent();
        ServerPlayer *who = room->getCurrentDyingPlayer();
        if (!current || !current->isAlive() || who != player || current == player)
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        ServerPlayer *current = room->getCurrent();
        player->tag["fenyuanDying"] = data;
        QString prompt = "invoke:" + current->objectName();
        return room->askForSkillInvoke(player, objectName(), prompt);
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        ServerPlayer *current = room->getCurrent();
        room->killPlayer(player);
        DamageStruct damage;
        damage.to = current;
        damage.damage = 2;
        damage.nature = DamageStruct::Thunder;
        room->damage(damage);
        return false;
    }
};

XiefaCard::XiefaCard()
{
    will_throw = false;
    mute = true;
}
bool XiefaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{

    if (targets.length() == 0) {
        if (to_select == Self)
            return false;
        //Card *slash = Sanguosha->cloneCard("slash");
        //slash->deleteLater();
        //pingyi yuanfei then use yuanfei. now pingyi xiefa and use xiefa in this round
        //can select yuanfei target...
        //if (to_select->isCardLimited(slash,Card::MethodUse))
        //    return false;
        //isCardLimited() did not worked...
        if (to_select->hasSkill("zhouye") && to_select->getMark("@ye") == 0)
            return false;

        if (to_select->hasSkill("xihua") && to_select->getMark("xihua_limit_Slash") > 0)
            return false;
        return true;
    } else if (targets.length() == 1) {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->deleteLater();
        if (!targets.first()->canSlash(to_select, slash, true))
            return false;
        return targets.first()->inMyAttackRange(to_select);
    }
    return (targets.length() < 2);
}
bool XiefaCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    return targets.length() == 2;
}

void XiefaCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *from = card_use.from;
    ServerPlayer *to1 = card_use.to.at(0);
    ServerPlayer *to2 = card_use.to.at(1);
    QList<ServerPlayer *>logto;
    logto << to1 << to2;
    room->touhouLogmessage("#ChoosePlayerWithSkill", from, "xiefa", logto, "");
    room->notifySkillInvoked(card_use.from, "xiefa");

    Card *card = Sanguosha->getCard(subcards.first());
    to1->obtainCard(card, false);

    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->setSkillName("_xiefa");
    CardUseStruct use;
    use.from = to1;
    use.to << to2;
    use.card = slash;
    room->useCard(use);

}
//about_to_use = function(self, room, cardUse)
class XiefaVS : public OneCardViewAsSkill
{
public:
    XiefaVS() :OneCardViewAsSkill("xiefa")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("XiefaCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        XiefaCard *card = new XiefaCard;
        card->addSubcard(originalCard);

        return card;
    }
};
class Xiefa : public TriggerSkill
{
public:
    Xiefa() : TriggerSkill("xiefa")
    {
        events << SlashMissed << Damaged;
        view_as_skill = new XiefaVS;
    }
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (triggerEvent == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->getSkillName() == objectName() && player == effect.from) 
                return QStringList(objectName());
        }else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();

            if (damage.card && damage.card->getSkillName() == objectName()) {
                if (damage.chain || damage.transfer)
                    return QStringList();
                if (damage.from == damage.to)
                    return QStringList();
                ServerPlayer *source = room->findPlayerBySkillName(objectName());
                if (source != NULL)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }
    
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == SlashMissed) {
            room->loseHp(player);
        } else if (triggerEvent == Damaged) {
            ServerPlayer *source = room->findPlayerBySkillName(objectName());
            source->drawCards(1);
        }
        return false;
    }
};

class Chuanbi : public TriggerSkill
{
public:
    Chuanbi() : TriggerSkill("chuanbi")
    {
        events << CardAsked;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    { 
        if (!TriggerSkill::triggerable(player)) return QStringList();
        QString pattern = data.toStringList().first();
        if (pattern == "jink") {
            ServerPlayer *current = room->getCurrent();
            if (!current && !current->isAlive() || (current->getWeapon() != NULL))
                return QStringList();
            Jink *jink = new Jink(Card::NoSuit, 0);
            jink->deleteLater();
            //need check
            if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
                if (player->isCardLimited(jink, Card::MethodResponse))
                    return QStringList();
            } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
                if (player->isCardLimited(jink, Card::MethodUse))
                    return QStringList();
            }
            return QStringList(objectName());
        }
        return QStringList();
    }
    
     virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName(), data); 
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        Jink *jink = new Jink(Card::NoSuit, 0);
        jink->setSkillName("_chuanbi");
        room->provide(jink);
        return false;
    }
};




class DuzhuaVS : public OneCardViewAsSkill
{
public:
    DuzhuaVS() : OneCardViewAsSkill("duzhua")
    {
        filter_pattern = ".|red|.|hand";
        response_or_use = true;
    }


    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return  !player->hasFlag("duzhua");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName("duzhua");
            return slash;
        } else
            return NULL;
    }
};

class Duzhua : public TriggerSkill
{
public:
    Duzhua() : TriggerSkill("duzhua")
    {
        events << PreCardUsed;
        view_as_skill = new DuzhuaVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    { 
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getSkillName() == "duzhua") {
            room->setPlayerFlag(player, objectName());
        }
        return QStringList();
    }
};

class DuzhuaTargetMod : public TargetModSkill
{
public:
    DuzhuaTargetMod() : TargetModSkill("#duzhuaTargetMod")
    {
        pattern = "Slash";
    }

    virtual int getResidueNum(const Player *from, const Card *card) const
    {
        if (from->hasSkill("duzhua") && card->getSkillName() == "duzhua")
            return 1;
        else
            return 0;
    }

};


class Taotie : public TriggerSkill
{
public:
    Taotie() : TriggerSkill("taotie")
    {
        events << CardResponded;
    }
    
    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {   
        TriggerList skill_list;
        QList<ServerPlayer *> srcs = room->findPlayersBySkillName(objectName());
        CardStar card_star = data.value<CardResponseStruct>().m_card;
        foreach (ServerPlayer *src, srcs) {
            if (card_star->isKindOf("Jink") && player != src  &&
            data.value<CardResponseStruct>().m_isUse &&  src->isWounded()) 
                skill_list.insert(src, QStringList(objectName()));
        }
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *source) const
    {
        return source->askForSkillInvoke(objectName(), data);
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *source) const
    {
        JudgeStruct judge;
        judge.reason = objectName();
        judge.who = source;
        judge.good = true;
        judge.pattern = ".|black";
        //judge.negative = false
        room->judge(judge);
        if (judge.isGood()) {
            room->recover(source, RecoverStruct());
        }
        return false;
    }
};

HuishengCard::HuishengCard()
{
    mute = true;
    will_throw = false;
}

bool HuishengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QString cardname = Self->property("huisheng_card").toString();
    QString str = Self->property("huisheng_target").toString();
    Card *new_card = Sanguosha->cloneCard(cardname);
    new_card->setSkillName("huisheng");
    if (new_card->isKindOf("Peach"))
        return to_select->objectName() == str && new_card->isAvailable(to_select);
    if (new_card->targetFixed() && !targets.isEmpty())
        return false;
    if (targets.isEmpty() && to_select->objectName() != str)
        return false;
    return new_card
        && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}
bool HuishengCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    QString cardname = Self->property("huisheng_card").toString();
    Card *new_card = Sanguosha->cloneCard(cardname);
    new_card->setSkillName("huisheng");
    //if ((new_card->isKindOf("IronChain")|| new_card->isKindOf("Peach"))&& targets.length()!=1)
    //    return false;
    if (targets.length() < 1)
        return false;
    return new_card && new_card->targetsFeasible(targets, Self);
}
const Card *HuishengCard::validate(CardUseStruct &card_use) const
{
    QString cardname = card_use.from->property("huisheng_card").toString();
    Card *card = Sanguosha->cloneCard(cardname);
    card->setSkillName("huisheng");
    return card;
}
class HuishengVS : public ZeroCardViewAsSkill
{
public:
    HuishengVS() : ZeroCardViewAsSkill("huisheng")
    {
        response_pattern = "@@huisheng";
    }


    virtual const Card *viewAs() const
    {

        return new HuishengCard;
    }
};
class Huisheng : public TriggerSkill
{
public:
    Huisheng() : TriggerSkill("huisheng")
    {
        events << CardFinished << PreCardUsed;
        view_as_skill = new HuishengVS;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == PreCardUsed) {//change aoe target for huisheng
            if (use.card->getSkillName() == "huisheng") {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (use.card->isKindOf("AOE") || use.card->isKindOf("GlobalEffect")) {
                        QString str = player->property("huisheng_target").toString();
                        if (use.to.contains(p) && p->objectName() != str)
                            use.to.removeOne(p);
                    }
                }
                data = QVariant::fromValue(use);
            }
        }
        else if (triggerEvent == CardFinished) {
            if (use.card->isKindOf("Jink"))
                return TriggerList();
            //for turnbroken
            if (use.from->hasFlag("Global_ProcessBroken") || !use.from->isAlive())
                return TriggerList();
            TriggerList skill_list;    
            if (use.from  && use.to.length() == 1
                &&  (use.card->isKindOf("BasicCard") || use.card->isNDTrick())){
                QList<ServerPlayer *> sources = room->findPlayersBySkillName(objectName());
                foreach (ServerPlayer *source, sources) {
                    if (use.from != source  && use.to.contains(source)){
                        Card *card = Sanguosha->cloneCard(use.card->objectName());
                        if (use.card->isNDTrick() && use.from->hasSkill("aoyi")) {//for a bug in filtersviewkill, then ai has this skill
                            if (use.from->getAI())
                                card = Sanguosha->cloneCard("ice_slash");
                            }
                        if (source->isCardLimited(card, Card::MethodUse))
                            continue;
                        if (!source->isProhibited(use.from, card))
                            skill_list.insert(source, QStringList(objectName()));
                    }
                }
            }
            return skill_list;
        }
        
        return TriggerList();
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *source) const
    {
        room->setTag("huisheng_use", data);
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "@huisheng-use:" + use.from->objectName() + ":" + use.card->objectName();
        room->setPlayerProperty(source, "huisheng_card", use.card->objectName());
        room->setPlayerProperty(source, "huisheng_target", use.from->objectName());
        room->askForUseCard(source, "@@huisheng", prompt);
        room->setPlayerProperty(source, "huisheng_target", QVariant());
        return false;
    }
};
class HuishengTargetMod : public TargetModSkill
{
public:
    HuishengTargetMod() : TargetModSkill("#huisheng_effect")
    {
        pattern = "BasicCard,TrickCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const
    {
        if (card->getSkillName() == "huisheng")
            return 1000;
        else
            return 0;
    }
};


class Songjing : public TriggerSkill
{
public:
    Songjing() : TriggerSkill("songjing")
    {
        events << CardUsed;
        frequency = Frequent;
    }
    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {   
        TriggerList skill_list;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("DelayedTrick") ){
            QList<ServerPlayer *> srcs = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *src, srcs) {
                skill_list.insert(src, QStringList(objectName()));
            }
        }    
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *source) const
    {    
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "use:" + use.from->objectName() + ":" + use.card->objectName();
        return room->askForSkillInvoke(source, objectName(), prompt);
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *source) const
    {
        source->drawCards(2);
        return false;
    }
};


class Chuixue : public TriggerSkill
{
public:
    Chuixue() : TriggerSkill("chuixue")
    {
        events << CardsMoveOneTime << EventPhaseEnd << EventPhaseChanging;
    }
    static void recordChuixue(ServerPlayer *player, Card *card)
    {
        Room *room = player->getRoom();
        if (card->getSuit() == Card::Spade)
            room->setPlayerMark(player, "chuixuespade", 1);
        else if (card->getSuit() == Card::Heart)
            room->setPlayerMark(player, "chuixueheart", 1);
        else if (card->getSuit() == Card::Club)
            room->setPlayerMark(player, "chuixueclub", 1);
        else if (card->getSuit() == Card::Diamond)
            room->setPlayerMark(player, "chuixuediamond", 1);
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from  && move.from == player && player->getPhase() == Player::Discard
                && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)) {
                foreach (int id, move.card_ids) {
                    if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand) {
                        Card *card = Sanguosha->getCard(id);
                        recordChuixue(player, card);
                    }
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Discard) {
                QStringList allsuits;
                allsuits << "spade" << "heart" << "club" << "diamond";
                foreach (QString suit, allsuits) {
                    room->setPlayerMark(player, "chuixue" + suit, 0);
                }
            }
        }
        else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Discard) {
            int count = 0;
            QStringList allsuits;
            allsuits << "spade" << "heart" << "club" << "diamond";
            foreach (QString suit, allsuits) {
                if (player->getMark("chuixue" + suit) == 0)
                    count++;
            }
            if (count < 4)
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QStringList allsuits;
        allsuits << "spade" << "heart" << "club" << "diamond";
        QStringList suits;
        foreach (QString suit, allsuits) {
            if (player->getMark("chuixue" + suit) == 0)
                suits << suit;
            room->setPlayerMark(player, "chuixue" + suit, 0);
        }

        QString pattern = "";
        if (suits.length() == 4)
            return false;
        if (suits.isEmpty())
            pattern = "";
        else
            pattern = ".|" + suits.join(",") + "|.|hand";

        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), "chuixue", "@chuixue-select", true, true);
        if (target != NULL) {
            if (suits.isEmpty())
                    room->loseHp(target, 1);
            else {
                const Card *card = room->askForCard(target, pattern, "@chuixue-discard:" + player->objectName());
                if (card == NULL)
                    room->loseHp(target, 1);
            }
        }
        return false;
    }
};

class Wushou : public TriggerSkill
{
public:
    Wushou() : TriggerSkill("wushou")
    {
        events << TargetConfirming;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") || use.card->isKindOf("Duel"))  
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName());
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        player->drawCards(player->getMaxHp());
        if (player->getHp() > 0)
            room->askForDiscard(player, objectName(), player->getHp(), player->getHp(), false, false, "wushou_discard:" + QString::number(player->getHp()));
        
        return false;
    }
};


BumingCard::BumingCard()
{
    will_throw = true;
    handling_method = Card::MethodUse;
    m_skillName = "buming";
    mute = true;
}
bool BumingCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (to_select == Self || targets.length() > 0)
        return false;
    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->deleteLater();
    Duel *duel = new Duel(Card::NoSuit, 0);
    duel->deleteLater();
    int rangefix = 0;
    if (Self->getWeapon() != NULL && Self->getWeapon()->getId() == subcards.first()) {
        if (Self->getAttackRange() > Self->getAttackRange(false))
            rangefix = rangefix + Self->getAttackRange() - Self->getAttackRange(false);
    }
    if (Self->getOffensiveHorse() && Self->getOffensiveHorse()->getId() == subcards.first())
        rangefix = rangefix + 1;

    if (subcards.length() > 0) {
        slash->addSubcard(subcards.first());//need add subcard,since we need  check rangefix
        QList<const Player *> targets2;
        return (slash->targetFilter(targets2, to_select, Self) && !(Self->isCardLimited(slash, Card::MethodUse)))
            || (Self->distanceTo(to_select, rangefix) <= Self->getAttackRange() &&
            !Self->isProhibited(to_select, duel) && !Self->isCardLimited(duel, Card::MethodUse));
    }
    return false;
}
void BumingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    QStringList    choices;
    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->deleteLater();
    Duel *duel = new Duel(Card::NoSuit, 0);
    duel->deleteLater();
    if (source->canSlash(target, slash, true) && !source->isCardLimited(slash, Card::MethodUse))
        choices << "slash_buming";
    if (!source->isProhibited(target, duel) && !source->isCardLimited(duel, Card::MethodUse))
        choices << "duel_buming";
    if (choices.length() == 0)
        return;
    QString choice = room->askForChoice(target, "buming", choices.join("+"));
    CardUseStruct card_use;
    slash->setSkillName("buming");
    duel->setSkillName("buming");
    if (choice == "slash_buming")
        card_use.card = slash;
    else if (choice == "duel_buming")
        card_use.card = duel;
    room->touhouLogmessage("#buming_choose", target, card_use.card->objectName());
    card_use.to << target;
    card_use.from = source;
    room->useCard(card_use, false);
}


class Buming : public OneCardViewAsSkill
{
public:
    Buming() :OneCardViewAsSkill("buming")
    {
        filter_pattern = ".|.|.|.!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("BumingCard");
    }
    virtual const Card *viewAs(const Card *originalCard) const
    {
        BumingCard *card = new BumingCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class Zhengti : public TriggerSkill
{
public:
    Zhengti() : TriggerSkill("zhengti")
    {
        events << DamageInflicted << Damaged;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == Damaged) {
            if (damage.from && damage.from != player) 
                return QStringList(objectName());
        } else if (triggerEvent == DamageInflicted) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getMark("@zhengti") > 0)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageInflicted) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getMark("@zhengti") > 0)
                    targets << p;
            }
            if (targets.length() > 0) {
                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@zhengti-choose", false, true);
                target->loseMark("@zhengti", 1);
                //for huashen UI
                if (target->getMark("@zhengti") == 0) {

                    room->setTag("zhengti_target", QVariant());
                    QList<ServerPlayer *> owners = room->findPlayersBySkillName(objectName());
                    foreach (ServerPlayer *owner, owners) {
                        if (owner->hasSkill("pingyi"))
                            continue;
                        Json::Value arg(Json::arrayValue);
                        arg[0] = (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
                        arg[1] = QSanProtocol::Utils::toJsonString(owner->objectName());
                        arg[2] = QSanProtocol::Utils::toJsonString(owner->getGeneral()->objectName());
                        arg[3] = QSanProtocol::Utils::toJsonString("clear");//QString()
                        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
                    }
                }

                damage.to = target;
                damage.transfer = true;
                room->damage(damage);

                return true;
            }
        } else if (triggerEvent == Damaged) {
            
            room->notifySkillInvoked(player, objectName());

            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());

            damage.from->gainMark("@zhengti", 1);
            room->setTag("zhengti_target", QVariant::fromValue(damage.from));
            //for huashen UI
            QList<ServerPlayer *> owners = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *owner, owners) {
                if (owner->hasSkill("pingyi"))
                    continue;

                Json::Value arg(Json::arrayValue);
                arg[0] = (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
                arg[1] = QSanProtocol::Utils::toJsonString(owner->objectName());
                arg[2] = QSanProtocol::Utils::toJsonString(damage.from->getGeneral()->objectName());
                arg[3] = QSanProtocol::Utils::toJsonString("zhengti");
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
            }
        }
        return false;
    }
};

class ZhengtiUIhandler : public TriggerSkill
{
public:
    ZhengtiUIhandler() : TriggerSkill("#zhengti")
    {
        events << EventLoseSkill << Death << EventAcquireSkill;
    }
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        ServerPlayer *zhengti;
        bool find = false;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->getMark("@zhengti") > 0) {
                zhengti = p;
                find = true;
                break;
            }
        }
        if (!find)
            room->setTag("zhengti_target", QVariant());

        if ((triggerEvent == EventLoseSkill && data.toString() == "zhengti") || triggerEvent == Death) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill("pingyi"))
                    continue;

                if (find && p->hasSkill("zhengti"))
                    continue;

                Json::Value arg(Json::arrayValue);
                arg[0] = (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
                arg[1] = QSanProtocol::Utils::toJsonString(p->objectName());
                arg[2] = QSanProtocol::Utils::toJsonString(p->getGeneral()->objectName());
                arg[3] = QSanProtocol::Utils::toJsonString("clear");
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
            }

        } else if (triggerEvent == EventAcquireSkill && data.toString() == "zhengti") {

            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill("pingyi"))
                    continue;
                if (!find)
                    continue;


                if (p->hasSkill("zhengti")) {

                    Json::Value arg(Json::arrayValue);
                    arg[0] = (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
                    arg[1] = QSanProtocol::Utils::toJsonString(p->objectName());
                    arg[2] = QSanProtocol::Utils::toJsonString(zhengti->getGeneral()->objectName());
                    arg[3] = QSanProtocol::Utils::toJsonString("zhengti");
                    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
                }
            }

        }
        return QStringList();
    }
};


class Qingyu : public MasochismSkill
{
public:
    Qingyu() : MasochismSkill("qingyu")
    {
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getHp() >= player->getHp())
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName());
    }
    
    
    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const
    {
        Room *room = player->getRoom();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getHp() >= player->getHp()){
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
                if (p->canDiscard(p, "he")) {
                    p->tag["qingyu_source"] = QVariant::fromValue(player);
                    
                    const Card *cards = room->askForCard(p, ".|.|.|.", "@qingyu-discard:" + player->objectName(), QVariant::fromValue(player), Card::MethodDiscard);
                    if (cards == NULL)
                        player->drawCards(1);
                    p->tag.remove("qingyu_source");
                } else
                    player->drawCards(1);
            }
        }
    }
};


class Guoke : public TriggerSkill
{
public:
    Guoke() : TriggerSkill("guoke")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != NULL && move.from == player && move.from_places.contains(Player::PlaceDelayedTrick))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName());
    }
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QString choice = "draw";
        if (player->isWounded())
            choice = room->askForChoice(player, objectName(), "draw+recover");
        if (choice == "draw")
            player->drawCards(2);
        else {
            room->recover(player, RecoverStruct());
        }
        return false;
    }
};



TH13Package::TH13Package()
    : Package("th13")
{
    General *slm001 = new General(this, "slm001$", "slm", 4, false);
    slm001->addSkill(new Shengge);
    slm001->addSkill(new Qingting);
    slm001->addSkill(new Chiling);

    General *slm002 = new General(this, "slm002", "slm", 3, false);
    slm002->addSkill(new Xihua);
    slm002->addSkill(new XihuaClear);
    related_skills.insertMulti("xihua", "#xihua_clear");

    General *slm003 = new General(this, "slm003", "slm", 3, false);
    slm003->addSkill(new Shijie);
    slm003->addSkill(new Fengshui);

    General *slm004 = new General(this, "slm004", "slm", 4, false);
    slm004->addSkill(new Leishi);
    slm004->addSkill(new Fenyuan);


    General *slm005 = new General(this, "slm005", "slm", 3, false);
    slm005->addSkill(new Xiefa);
    slm005->addSkill(new Chuanbi);


    General *slm006 = new General(this, "slm006", "slm", 4, false);
    slm006->addSkill(new Duzhua);
    slm006->addSkill(new DuzhuaTargetMod);;
    slm006->addSkill(new Taotie);
    related_skills.insertMulti("duzhua", "#duzhuaTargetMod");

    General *slm007 = new General(this, "slm007", "slm", 3, false);
    slm007->addSkill(new Huisheng);
    slm007->addSkill(new HuishengTargetMod);
    slm007->addSkill(new Songjing);
    related_skills.insertMulti("huisheng_effect", "#huisheng_effect");

    General *slm008 = new General(this, "slm008", "slm", 3, false);
    slm008->addSkill(new Chuixue);
    slm008->addSkill(new Wushou);

    General *slm009 = new General(this, "slm009", "slm", 3, false);
    slm009->addSkill(new Buming);
    slm009->addSkill(new Zhengti);
    slm009->addSkill(new ZhengtiUIhandler);
    related_skills.insertMulti("zhengti", "#zhengti");

    General *slm010 = new General(this, "slm010", "slm", 3, false);
    slm010->addSkill(new Qingyu);
    slm010->addSkill(new Guoke);

    addMetaObject<QingtingCard>();
    addMetaObject<XihuaCard>();
    addMetaObject<ShijieCard>();
    addMetaObject<LeishiCard>();
    addMetaObject<XiefaCard>();
    addMetaObject<HuishengCard>();
    addMetaObject<BumingCard>();

}

ADD_PACKAGE(TH13)

