#include "th13.h"

#include "th10.h"
#include "general.h"
#include "settings.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"
#include "maneuvering.h"

class Shengge : public TriggerSkill
{
public:
    Shengge() : TriggerSkill("shengge")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getMark(objectName()) == 0 && player->getPhase() == Player::Start && player->isKongcheng())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->addPlayerMark(invoke->invoker, objectName());
        room->doLightbox("$shenggeAnimate", 4000);
        room->touhouLogmessage("#ShenggeWake", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        if (room->changeMaxHpForAwakenSkill(invoke->invoker))
            invoke->invoker->drawCards(3);
        return false;
    }
};

QingtingCard::QingtingCard()
{
    target_fixed = true;
}

void QingtingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
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
        if (source->getMark("shengge") > 0 || p->getHandcardNum() == 1)
            card = new DummyCard(QList<int>() << room->askForCardChosen(source, p, "hs", "qingting"));
        else {
            p->tag["qingting_give"] = QVariant::fromValue(source);
            card = room->askForExchange(p, "qingting", 1, 1, false, "qingtingGive:" + source->objectName());
            p->tag.remove("qingting_give");
        }
        DELETE_OVER_SCOPE(const Card, card)

                source->obtainCard(card, false);
        room->setPlayerMark(p, "@qingting", 1);

    }

    //get delay
    if (source->isOnline())
        room->getThread()->delay(2000);

    foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
        if (p->getMark("@qingting") == 0)
            continue;
        room->setPlayerMark(p, "@qingting", 0);
        if (source->isKongcheng())
            continue;

        source->tag["qingting_return"] = QVariant::fromValue(p);
        const Card *card = room->askForExchange(source, "qingting", 1, 1, false, "qingtingReturn:" + p->objectName());
        DELETE_OVER_SCOPE(const Card, card)
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *miko = qobject_cast<ServerPlayer *>(move.from);
        if (miko != NULL && miko->isAlive() && miko->hasLordSkill(objectName()) && move.from_places.contains(Player::PlaceHand)
                && (move.to_place == Player::PlaceHand && move.to && move.to != miko
                    && move.to->getKingdom() == "slm"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, miko, miko);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *target = qobject_cast<ServerPlayer *>(move.to);

        bool isSlash = false;
        foreach (int id, move.card_ids) {
            if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand) {
                if (Sanguosha->getCard(id)->isKindOf("Slash")) {
                    isSlash = true;
                    break;
                }
            }
        }
        if (isSlash)
            invoke->invoker->tag["chiling_showslash"] = QVariant::fromValue(1);
        else
            invoke->invoker->tag["chiling_showslash"] = QVariant::fromValue(0);

        invoke->invoker->tag["chiling_givener"] = QVariant::fromValue(target);
        return invoke->invoker->askForSkillInvoke(objectName(), "showcard:" + target->objectName());
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *target = qobject_cast<ServerPlayer *>(move.to);

        foreach (int id, move.card_ids) {
            if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand)
                room->showCard(target, id);
        }


        int  can_slash = invoke->invoker->tag["chiling_showslash"].toInt();
        if (can_slash && can_slash > 0)
            room->askForUseCard(target, "slash", "@chiling:" + invoke->invoker->objectName(), -1, Card::MethodUse, false);
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

    static void xihua_record(Room *room, ServerPlayer *player, QString pattern)
    {
        if (pattern.contains("slash"))
            pattern = "slash";
        QString markName = "xihua_record_" + pattern;
        room->setPlayerMark(player, markName, 1);
    }
    static bool xihua_choice_limit(const Player *player, QString pattern, Card::HandlingMethod method)
    {
        QString markName = "xihua_record_" + pattern;
        Card *c = Sanguosha->cloneCard(pattern);
        DELETE_OVER_SCOPE(Card, c)
                if (method == Card::MethodNone)
                method = Card::MethodUse;
        if (player->getMark(markName) > 0 || player->isCardLimited(c, method, true))
            return true;
        else
            return false;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const
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
                    QString markName = "xihua_record_" + pattern;
                    if (p->getMark(markName) > 0)
                        room->setPlayerMark(p, markName, 0);
                }
            }
        }
    }
};

XihuaCard::XihuaCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool XihuaCard::do_xihua(ServerPlayer *tanuki) const
{
    Room *room = tanuki->getRoom();
    Card *xihuacard = Sanguosha->cloneCard(tanuki->tag["xihua_choice"].toString());
    DELETE_OVER_SCOPE(Card, xihuacard)
            //record xihua cardname which has used
            ServerPlayer *current = room->getCurrent();
    if (current && current->isAlive() && current->isCurrent())
        XihuaClear::xihua_record(room, tanuki, xihuacard->objectName());


    ServerPlayer *target = room->askForPlayerChosen(tanuki, room->getOtherPlayers(tanuki), "xihua", "@xihua_chosen:" + xihuacard->objectName(), false, true);
    int to_show = room->askForCardChosen(target, tanuki, "hs", "xihua");
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
        room->touhouLogmessage("#Xihua_failed", tanuki, "xihua", QList<ServerPlayer *>(), xihuacard->objectName());
    }
    return success;
}

bool XihuaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{

    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), Card::NoSuit, 0);
            DELETE_OVER_SCOPE(Card, card)
                    card->setSkillName("xihua");
            return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *card = Self->tag.value("xihua").value<const Card *>();

    Card *new_card = Sanguosha->cloneCard(card->objectName(), Card::NoSuit, 0);
    DELETE_OVER_SCOPE(Card, new_card)
            new_card->setSkillName("xihua");
    if (new_card->targetFixed())
        return false;
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool XihuaCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), Card::NoSuit, 0);
            DELETE_OVER_SCOPE(Card, card)
                    card->setSkillName("xihua");
            return card && card->targetFixed();
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("xihua").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName(), Card::NoSuit, 0);
    DELETE_OVER_SCOPE(Card, new_card)
            new_card->setSkillName("xihua");
    //return false defaultly
    //we need a confirming chance to pull back, since  this is a zero cards viewas Skill.
    return false;//new_card && new_card->targetFixed();
}

bool XihuaCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), Card::NoSuit, 0);
            card->setSkillName("xihua");
            return card && card->targetsFeasible(targets, Self);
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("xihua").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName(), Card::NoSuit, 0);
    DELETE_OVER_SCOPE(Card, new_card)
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

    static QStringList responsePatterns() {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        Card::HandlingMethod method;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            method = Card::MethodResponse;
        else
            method = Card::MethodUse;

        QStringList validPatterns;
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        foreach(const Card *card, cards) {
            if ((card->isNDTrick() || card->isKindOf("BasicCard"))
                    && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
                QString pattern = card->objectName();
                if (card->isKindOf("Slash"))
                    pattern = "slash";
                if (!validPatterns.contains(pattern) && !XihuaClear::xihua_choice_limit(Self, pattern, method))
                    validPatterns << card->objectName();
            }
        }

        QStringList checkedPatterns;
        foreach(QString str, validPatterns) {
            const Skill *skill = Sanguosha->getSkill("xihua");
            if (skill->matchAvaliablePattern(str, pattern))
                checkedPatterns << str;
        }
        return checkedPatterns;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &) const
    {
        if (player->isKongcheng()) return false;
        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.contains("peach") && checkedPatterns.length() == 1 && player->getMark("Global_PreventPeach") > 0) return false;

        return !checkedPatterns.isEmpty();
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
            QStringList checkedPatterns = responsePatterns();
            if (checkedPatterns.length() > 1 || checkedPatterns.contains("slash")) {
                const Card *c = Self->tag.value("xihua").value<const Card *>();
                if (c)
                    pattern = c->objectName();
                else
                    return NULL;
            } else
                pattern = checkedPatterns.first();
            XihuaCard *card = new XihuaCard;
            card->setUserString(pattern);
            return card;
        }

        const Card * c = Self->tag.value("xihua").value<const Card *>();
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
}

void ShijieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    ServerPlayer *who = room->getCurrentDyingPlayer();
    JudgeStruct judge;
    judge.reason = "shijie";
    judge.who = who;
    judge.good = true;
    judge.pattern = ".";
    judge.play_animation = false;
    room->judge(judge);
    QList<ServerPlayer *> listt;
    foreach (ServerPlayer *p, room->getAlivePlayers()) {
        foreach (const Card *c, p->getCards("e")) {
            if (judge.pattern == c->getSuitString() && source->canDiscard(p, c->getEffectiveId()))
                listt << p;
        }
    }
    if (!listt.isEmpty()) {
        ServerPlayer * target = room->askForPlayerChosen(source, listt, "shijie", "@@shijie_chosen", true, true);
        if (target == NULL)
            return;
        QList<int> disabled_ids;
        foreach (const Card *c, target->getCards("e")) {
            if (judge.pattern != c->getSuitString() || !source->canDiscard(target, c->getEffectiveId()))
                disabled_ids << c->getEffectiveId();
        }
        //for ai
        source->tag["shijie_suit"] = QVariant::fromValue(judge.pattern);

        int id = room->askForCardChosen(source, target, "e", "shijie", false, Card::MethodDiscard, disabled_ids);
        room->throwCard(id, target, source == target ? NULL : source);
        RecoverStruct recover;
        recover.recover = 1;
        recover.who = source;
        room->recover(who, recover);
    }
}

class ShijieVS : public OneCardViewAsSkill
{
public:
    ShijieVS() : OneCardViewAsSkill("shijie")
    {
        filter_pattern = ".|.|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *) const
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

class Shijie : public TriggerSkill
{
public:
    Shijie() : TriggerSkill("shijie")
    {
        events << FinishJudge;
        view_as_skill = new ShijieVS;
    }

    void record(TriggerEvent, Room *, QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (judge->reason == "shijie")
            judge->pattern = judge->card->getSuitString();
    }
};

class Fengshui : public TriggerSkill
{
public:
    Fengshui() : TriggerSkill("fengshui")
    {
        events << AskForRetrial;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        JudgeStruct * judge = data.value<JudgeStruct *>();
        if (!judge->who || !judge->who->isAlive())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach(ServerPlayer *p, room->getAllPlayers()) {
            if (p->hasSkill(this))
                d << SkillInvokeDetail(this, p, p);
        }

        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        JudgeStruct * judge = data.value<JudgeStruct *>();
        ServerPlayer *player = invoke->invoker;
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.slash->hasFlag("leishislash"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.from, effect.from, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->loseHp(invoke->invoker, 1);
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *current = room->getCurrent();
        ServerPlayer *who = data.value<DyingStruct>().who;
        if (!who->hasSkill(this) || !current || !current->isAlive() || current == who)
            return QList<SkillInvokeDetail>();
        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, who, who, NULL, false, current);
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        invoke->invoker->tag["fenyuanDying"] = data;
        QString prompt = "invoke:" + invoke->preferredTarget->objectName();
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->killPlayer(invoke->invoker);
        DamageStruct damage;
        damage.to = invoke->targets.first();
        damage.damage = 2;
        damage.nature = DamageStruct::Thunder;
        room->damage(damage);
        return false;
    }
};


#pragma message WARN("todo_lwtmusou:target filter can not get useCardLimit, such as skill zhouye")
XiefaCard::XiefaCard()
{
    will_throw = false;
}

bool XiefaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->deleteLater();
    if (targets.length() == 0) {
        if (to_select == Self)
            return false;
        if (to_select->isCardLimited(slash, Card::MethodUse))
            return false;

        return true;
    } else if (targets.length() == 1) {
        if (!targets.first()->canSlash(to_select, slash, true))
            return false;
        return targets.first()->inMyAttackRange(to_select);
    }
    return (targets.length() < 2);
}

bool XiefaCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void XiefaCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    QVariant data = QVariant::fromValue(use);
    RoomThread *thread = room->getThread();

    thread->trigger(PreCardUsed, room, data);
    use = data.value<CardUseStruct>();

    ServerPlayer *from = card_use.from;
    ServerPlayer *to1 = card_use.to.at(0);
    ServerPlayer *to2 = card_use.to.at(1);
    QList<ServerPlayer *>logto;
    logto << to1 << to2;
    room->touhouLogmessage("#ChoosePlayerWithSkill", from, "xiefa", logto, "");
    room->notifySkillInvoked(card_use.from, "xiefa");

    thread->trigger(CardUsed, room, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, data);
}

void XiefaCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *to1 = targets.first();
    ServerPlayer *to2 = targets.last();
    Card *card = Sanguosha->getCard(subcards.first());
    to1->obtainCard(card, false);

    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->setSkillName("_xiefa");
    room->setCardFlag(slash, "xiefa");
    CardUseStruct use;
    use.from = to1;
    use.to << to2;
    use.card = slash;
    room->useCard(use);
}

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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag(objectName()) && effect.from->isAlive())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, NULL, effect.from, NULL, true);
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->hasFlag(objectName())) {
                if (!damage.chain && !damage.transfer && damage.from != damage.to) {
                    ServerPlayer *source = room->getCurrent();
                    if (source != NULL && source->isAlive())
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, source, source, NULL, true);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }


    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == SlashMissed)
            room->loseHp(invoke->invoker);
        else if (triggerEvent == Damaged)
            invoke->invoker->drawCards(1);
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardAskedStruct s = data.value<CardAskedStruct>();
        if (matchAvaliablePattern("jink", s.pattern)) {
            ServerPlayer *current = room->getCurrent();
            if (!s.player->hasSkill(this) || !current && !current->isAlive() || (current->getWeapon() != NULL))
                return QList<SkillInvokeDetail>();

            Jink *jink = new Jink(Card::NoSuit, 0);
            jink->deleteLater();
            if (s.player->isCardLimited(jink, s.method))
                return QList<SkillInvokeDetail>();

            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, s.player, s.player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        Jink *jink = new Jink(Card::NoSuit, 0);
        jink->setSkillName("_chuanbi");
        room->provide(jink);
        return true;
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
        return !player->hasFlag("duzhua") && Slash::IsAvailable(player);
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

    void record(TriggerEvent, Room *room, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getSkillName() == "duzhua") {
            room->setPlayerFlag(use.from, objectName());
            if (use.m_addHistory) {
                room->addPlayerHistory(use.from, use.card->getClassName(), -1);
                use.m_addHistory = false;
                data = QVariant::fromValue(use);
            }
        }
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
        if (from->hasSkill("duzhua") && ((card->getSkillName() == "duzhua") || card->hasFlag("Global_SlashAvailabilityChecker")))
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        CardResponseStruct resp = data.value<CardResponseStruct>();
        if (resp.m_card->isKindOf("Jink") &&  resp.m_isUse) {
            foreach(ServerPlayer *src, room->findPlayersBySkillName(objectName())) {
                if (resp.m_from != src && src->isWounded())
                    d << SkillInvokeDetail(this, src, src);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        JudgeStruct judge;
        judge.reason = objectName();
        judge.who = invoke->invoker;
        judge.good = true;
        judge.pattern = ".|black";
        room->judge(judge);

        if (judge.isGood())
            room->recover(invoke->invoker, RecoverStruct());
        return false;
    }
};



HuishengCard::HuishengCard()
{
    will_throw = false;
}

bool HuishengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QString cardname = Self->property("huisheng_card").toString();
    QString str = Self->property("huisheng_target").toString();
    Card *new_card = Sanguosha->cloneCard(cardname);
    DELETE_OVER_SCOPE(Card, new_card)
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
    DELETE_OVER_SCOPE(Card, new_card)
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == PreCardUsed && use.card->getSkillName() == "huisheng") {//change aoe target for huisheng
            if (use.card->isKindOf("AOE") || use.card->isKindOf("GlobalEffect")) {
                ServerPlayer *target = use.from->tag["huisheng_target"].value<ServerPlayer *>();
                foreach(ServerPlayer *p, use.to) {
                    if (p != target)
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, true);
                }
            }
        } else if (triggerEvent == CardFinished) {
            if (use.card->isKindOf("Jink") || use.from->hasFlag("Global_ProcessBroken") || !use.from->isAlive())
                return QList<SkillInvokeDetail>();
            if (use.from  && use.to.length() == 1
                    && (use.card->isKindOf("BasicCard") || use.card->isNDTrick())) {
                ServerPlayer *source = use.to.first();
                if (use.from != source  && source->hasSkill(this) && source->isAlive() && use.from->isAlive()) {
                    Card *card = Sanguosha->cloneCard(use.card->objectName());
                    DELETE_OVER_SCOPE(Card, card)
                            if (!source->isCardLimited(card, Card::MethodUse) && !source->isProhibited(use.from, card))
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, source, source);
                }
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == CardFinished) {
            room->setTag("huisheng_use", data);
            CardUseStruct use = data.value<CardUseStruct>();
            Card *card = Sanguosha->cloneCard(use.card->objectName());

            QString prompt = "@huisheng-use:" + use.from->objectName() + ":" + card->objectName();
            room->setPlayerProperty(invoke->invoker, "huisheng_card", card->objectName());
            delete card;
            room->setPlayerProperty(invoke->invoker, "huisheng_target", use.from->objectName());
            invoke->invoker->tag["huisheng_target"] = QVariant::fromValue(use.from);
            room->askForUseCard(invoke->invoker, "@@huisheng", prompt);
            room->setPlayerProperty(invoke->invoker, "huisheng_target", QVariant());
            return false;
        }

        return true;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *target = use.from->tag["huisheng_target"].value<ServerPlayer *>();
        foreach(ServerPlayer *p, room->getAlivePlayers()) {
            if (use.to.contains(p) && p != target)
                use.to.removeOne(p);
        }
        invoke->invoker->tag.remove("huisheng_target");
        data = QVariant::fromValue(use);
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

    virtual int getDistanceLimit(const Player *, const Card *card) const
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("DelayedTrick")) {
            foreach (ServerPlayer *src, room->findPlayersBySkillName(objectName())) {
                d << SkillInvokeDetail(this, src, src);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "use:" + use.from->objectName() + ":" + use.card->objectName();
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(2);
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
            if (player != NULL && player->getPhase() == Player::Discard
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
                    room->setPlayerMark(change.player, "chuixue" + suit, 0);
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() != Player::Discard || !player->hasSkill(this))
                return QList<SkillInvokeDetail>();

            int count = 0;
            QStringList allsuits;
            allsuits << "spade" << "heart" << "club" << "diamond";
            foreach (QString suit, allsuits) {
                if (player->getMark("chuixue" + suit) == 0)
                    count++;
            }
            if (count < 4)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }
        return QList<SkillInvokeDetail>();
    }


    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), "chuixue", "@chuixue-select", true, true);
        if (target != NULL) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QStringList allsuits;
        allsuits << "spade" << "heart" << "club" << "diamond";
        QStringList suits;
        foreach(QString suit, allsuits) {
            if (invoke->invoker->getMark("chuixue" + suit) == 0)
                suits << suit;
            room->setPlayerMark(invoke->invoker, "chuixue" + suit, 0);
        }

        QString pattern = "";
        if (suits.isEmpty())
            pattern = "";
        else
            pattern = ".|" + suits.join(",") + "|.|hand";


        if (suits.isEmpty())
            room->loseHp(invoke->targets.first(), 1);
        else {
            if (!room->askForCard(invoke->targets.first(), pattern, "@chuixue-discard:" + invoke->invoker->objectName()))
                room->loseHp(invoke->targets.first(), 1);
        }
        return false;

    }

};

class Wushou : public TriggerSkill
{
public:
    Wushou() : TriggerSkill("wushou")
    {
        events << TargetConfirmed;
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->isKindOf("Slash") || use.card->isKindOf("Duel")){
            foreach(ServerPlayer *p, use.to) {
                if (p->hasSkill(this))
                    d << SkillInvokeDetail(this, p, p);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.to->hasSkill(this))
            return QList<SkillInvokeDetail>();

        if (triggerEvent == Damaged) {
            if (damage.from && damage.from != damage.to && damage.to->isAlive())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, true);
        } else if (triggerEvent == DamageInflicted) {
            foreach (ServerPlayer *p, room->getOtherPlayers(damage.to)) {
                if (p->getMark("@zhengti") > 0)
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageInflicted) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(damage.to)) {
                if (p->getMark("@zhengti") > 0)
                    targets << p;
            }
            ServerPlayer *target = room->askForPlayerChosen(damage.to, targets, objectName(), "@zhengti-choose", false, true);
            target->loseMark("@zhengti", 1);

            damage.to = target;
            damage.transfer = true;
            room->damage(damage);
            return true;
        } else if (triggerEvent == Damaged) {
            room->notifySkillInvoked(damage.to, objectName());
            room->touhouLogmessage("#TriggerSkill", damage.to, objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, damage.to->objectName(), damage.from->objectName());

            damage.from->gainMark("@zhengti", 1);
            room->setTag("zhengti_target", QVariant::fromValue(damage.from));
        }
        return false;
    }
};




class Qingyu : public MasochismSkill
{
public:
    Qingyu() : MasochismSkill("qingyu")
    {
    }

    QList<SkillInvokeDetail> triggerable(const Room *, const DamageStruct &damage) const
    {
        
        if (damage.to->hasSkill(this) && damage.to->isAlive()
                && !damage.to->isKongcheng() && !damage.to->containsTrick("supply_shortage"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        const Card *card = room->askForCard(invoke->invoker, ".|.|.|hand", "@qingyu", data, Card::MethodNone, NULL, false, objectName());
        if (card) {
            room->notifySkillInvoked(invoke->invoker, objectName());
            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = invoke->invoker;
            log.arg = objectName();
            room->sendLog(log);

            Card *supplyshortage = Sanguosha->cloneCard("supply_shortage", card->getSuit(), card->getNumber());
            WrappedCard *vs_card = Sanguosha->getWrappedCard(card->getSubcards().first());
            vs_card->setSkillName(objectName());
            vs_card->takeOver(supplyshortage);
            room->broadcastUpdateCard(room->getAlivePlayers(), vs_card->getId(), vs_card);
            CardsMoveStruct  move;
            move.card_ids << vs_card->getId();
            move.to = invoke->invoker;
            move.to_place = Player::PlaceDelayedTrick;
            room->moveCardsAtomic(move, true);
        }
        return card != NULL;
    }

    void onDamaged(Room *room, QSharedPointer<SkillInvokeDetail> invoke, const DamageStruct &damage) const
    {
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getOtherPlayers(damage.to)) {
            if (p->getHp() >= damage.to->getHp())
                targets << p;
        }
        foreach (ServerPlayer *p, targets) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, damage.to->objectName(), p->objectName());
            if (p->canDiscard(p, "hes")) {
                p->tag["qingyu_source"] = QVariant::fromValue(damage.to);
                const Card *cards = room->askForCard(p, ".|.|.|.", "@qingyu-discard:" + damage.to->objectName(), QVariant::fromValue(damage.to), Card::MethodDiscard);
                p->tag.remove("qingyu_source");
                if (cards == NULL)
                    damage.to->drawCards(1);
            }
            else
                damage.to->drawCards(1);
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *kogasa = qobject_cast<ServerPlayer *>(move.from);
        if (kogasa != NULL && kogasa->isAlive() && kogasa->hasSkill(this) && move.from_places.contains(Player::PlaceDelayedTrick))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, kogasa, kogasa);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString choice = "draw";
        if (invoke->invoker->isWounded())
            choice = room->askForChoice(invoke->invoker, objectName(), "draw+recover");
        if (choice == "draw")
            invoke->invoker->drawCards(1);
        else
            room->recover(invoke->invoker, RecoverStruct());
        return false;
    }
};



TH13Package::TH13Package()
    : Package("th13")
{
    General *miko = new General(this, "miko$", "slm", 4, false);
    miko->addSkill(new Shengge);
    miko->addSkill(new Qingting);
    miko->addSkill(new Chiling);

    General *mamizou = new General(this, "mamizou", "slm", 4, false);
    mamizou->addSkill(new Xihua);
    mamizou->addSkill(new XihuaClear);
    related_skills.insertMulti("xihua", "#xihua_clear");

    General *futo = new General(this, "futo", "slm", 3, false);
    futo->addSkill(new Shijie);
    futo->addSkill(new Fengshui);

    General *toziko = new General(this, "toziko", "slm", 4, false);
    toziko->addSkill(new Leishi);
    toziko->addSkill(new Fenyuan);


    General *seiga = new General(this, "seiga", "slm", 3, false);
    seiga->addSkill(new Xiefa);
    seiga->addSkill(new Chuanbi);


    General *yoshika = new General(this, "yoshika", "slm", 4, false);
    yoshika->addSkill(new Duzhua);
    yoshika->addSkill(new DuzhuaTargetMod);;
    yoshika->addSkill(new Taotie);
    related_skills.insertMulti("duzhua", "#duzhuaTargetMod");

    General *kyouko = new General(this, "kyouko", "slm", 3, false);
    kyouko->addSkill(new Huisheng);
    kyouko->addSkill(new HuishengTargetMod);
    kyouko->addSkill(new Songjing);
    related_skills.insertMulti("huisheng_effect", "#huisheng_effect");

    General *yuyuko_slm = new General(this, "yuyuko_slm", "slm", 3, false);
    yuyuko_slm->addSkill(new Chuixue);
    yuyuko_slm->addSkill(new Wushou);

    General *nue_slm = new General(this, "nue_slm", "slm", 3, false);
    nue_slm->addSkill(new Buming);
    nue_slm->addSkill(new Zhengti);

    General *kogasa_slm = new General(this, "kogasa_slm", "slm", 3, false);
    kogasa_slm->addSkill(new Qingyu);
    kogasa_slm->addSkill(new Guoke);

    addMetaObject<QingtingCard>();
    addMetaObject<XihuaCard>();
    addMetaObject<ShijieCard>();
    addMetaObject<LeishiCard>();
    addMetaObject<XiefaCard>();
    addMetaObject<HuishengCard>();
    addMetaObject<BumingCard>();

}

ADD_PACKAGE(TH13)

