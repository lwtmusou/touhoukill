#include "th99.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"

QiuwenCard::QiuwenCard()
{
    target_fixed = true;
}

void QiuwenCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->drawCards(source, 3);
}

class Qiuwen : public ZeroCardViewAsSkill
{
public:
    Qiuwen()
        : ZeroCardViewAsSkill("qiuwen")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("QiuwenCard");
    }

    virtual const Card *viewAs() const
    {
        return new QiuwenCard;
    }
};

class Zaozu : public TriggerSkill
{
public:
    Zaozu()
        : TriggerSkill("zaozu")
    {
        events << EventPhaseChanging << EventPhaseStart;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->isAlive() && change.player->hasSkill(this) && change.to == Player::Discard)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, NULL, true);
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *aq = data.value<ServerPlayer *>();
            if (aq->isAlive() && aq->hasSkill(this) && aq->getPhase() == Player::Finish && aq->getHandcardNum() > aq->getMaxHp())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, aq, aq, NULL, true);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        if (triggerEvent == EventPhaseChanging)
            invoke->invoker->skip(Player::Discard);
        else if (triggerEvent == EventPhaseStart)
            room->loseHp(invoke->invoker);

        return false;
    }
};

DangjiaCard::DangjiaCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "dangjia_attach";
}

bool DangjiaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasLordSkill("dangjia") && to_select != Self && !to_select->hasFlag("dangjiaInvoked") && !to_select->isKongcheng()
        && to_select->isWounded();
}

void DangjiaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *akyu = targets.first();
    room->setPlayerFlag(akyu, "dangjiaInvoked");

    room->notifySkillInvoked(akyu, "dangjia");
    if (!source->pindian(akyu, "dangjia", NULL)) {
        RecoverStruct recov;
        recov.who = source;
        room->recover(akyu, recov);
    }
}

class DangjiaVS : public ZeroCardViewAsSkill
{
public:
    DangjiaVS()
        : ZeroCardViewAsSkill("dangjia_attach")
    {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->isKongcheng() || !shouldBeVisible(player))
            return false;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasLordSkill("dangjia") && !p->hasFlag("dangjiaInvoked"))
                return true;
        }
        return false;
    }

    virtual bool shouldBeVisible(const Player *Self) const
    {
        return Self && Self->getKingdom() == "wai";
    }

    virtual const Card *viewAs() const
    {
        return new DangjiaCard;
    }
};

class Dangjia : public TriggerSkill
{
public:
    Dangjia()
        : TriggerSkill("dangjia$")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging << Revive;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent != EventPhaseChanging) {
            static QString attachName = "dangjia_attach";
            QList<ServerPlayer *> aqs;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasLordSkill(this, true))
                    aqs << p;
            }

            if (aqs.length() > 1) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->hasLordSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else if (aqs.length() == 1) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasLordSkill(this, true) && p->hasLordSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                    else if (!p->hasLordSkill(this, true) && !p->hasLordSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else { // the case that aqs is empty
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasLordSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                }
            }
        } else {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                return;
            QList<ServerPlayer *> players = room->getOtherPlayers(phase_change.player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("dangjiaInvoked"))
                    room->setPlayerFlag(p, "-dangjiaInvoked");
            }
        }
    }
};

XiufuCard::XiufuCard()
{
    target_fixed = true;
}

bool XiufuCard::putToPile(Room *room, ServerPlayer *mori)
{
    QList<int> discardpile = room->getDiscardPile();
    QList<int> equips;
    foreach (int id, discardpile) {
        if (Sanguosha->getCard(id)->isKindOf("EquipCard"))
            equips << id;
    }

    if (equips.isEmpty())
        return false;

    CardsMoveStruct move;
    move.from_place = Player::DiscardPile;
    move.to = mori;
    move.to_player_name = mori->objectName();
    move.to_pile_name = "#xiufu_temp";
    move.card_ids = equips;
    move.to_place = Player::PlaceSpecial;
    move.open = true;

    QList<CardsMoveStruct> _moves = QList<CardsMoveStruct>() << move;
    QList<ServerPlayer *> _mori = QList<ServerPlayer *>() << mori;
    room->setPlayerFlag(mori, "xiufu_InTempMoving");
    room->notifyMoveCards(true, _moves, true, _mori);
    room->notifyMoveCards(false, _moves, true, _mori);
    room->setPlayerFlag(mori, "-xiufu_InTempMoving");

    QVariantList tag = IntList2VariantList(equips);
    mori->tag["xiufu_tempcards"] = tag;
    return true;
}

void XiufuCard::cleanUp(Room *room, ServerPlayer *mori)
{
    QList<int> equips = VariantList2IntList(mori->tag.value("xiufu_tempcards", QVariantList()).toList());
    mori->tag.remove("xiufu_tempcards");
    if (equips.isEmpty())
        return;

    CardsMoveStruct move;
    move.from = mori;
    move.from_player_name = mori->objectName();
    move.from_place = Player::PlaceSpecial;
    move.from_pile_name = "#xiufu_temp";
    move.to_place = Player::DiscardPile;
    move.open = true;
    move.card_ids = equips;

    QList<CardsMoveStruct> _moves = QList<CardsMoveStruct>() << move;
    QList<ServerPlayer *> _mori = QList<ServerPlayer *>() << mori;
    room->setPlayerFlag(mori, "xiufu_InTempMoving");
    room->notifyMoveCards(true, _moves, true, _mori);
    room->notifyMoveCards(false, _moves, true, _mori);
    room->setPlayerFlag(mori, "-xiufu_InTempMoving");
}

void XiufuCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *mori = card_use.from;
    if (mori->isOnline()) {
        if (!putToPile(room, mori)) {
            room->setPlayerFlag(mori, "Global_xiufuFailed");
            return;
        }

        bool used = room->askForUseCard(mori, "@@xiufumove", "@xiufu-move", -1, Card::MethodNone, true, "xiufu");

        cleanUp(room, mori);
        if (!used) {
            room->setPlayerFlag(mori, "Global_xiufuFailed");
            return;
        }
    } else {
        room->setPlayerFlag(mori, "xiufu_used");
        // we use askforag and askforplayerchosen for AI
        QList<int> discardpile = room->getDiscardPile();
        QList<int> equips;
        foreach (int id, discardpile) {
            if (Sanguosha->getCard(id)->isKindOf("EquipCard"))
                equips << id;
        }

        int id = room->askForAG(mori, equips, true, "xiufu");
        if (id == -1)
            return;

        ServerPlayer *target = room->askForPlayerChosen(mori, room->getAllPlayers(), "xiufu", QString(), true);
        if (target == NULL)
            return;

        mori->tag["xiufu_id"] = id;
        mori->tag["xiufu_to"] = QVariant::fromValue(target);
    }

    SkillCard::onUse(room, card_use);
}

void XiufuCard::use(Room *room, ServerPlayer *mori, QList<ServerPlayer *> &) const
{
    room->setPlayerFlag(mori, "xiufu_used");
    //process move
    int xiufu_id = mori->tag.value("xiufu_id", -1).toInt();
    if (xiufu_id == -1)
        return;

    const EquipCard *equip = qobject_cast<const EquipCard *>(Sanguosha->getCard(xiufu_id)->getRealCard());
    ServerPlayer *target = mori->tag.value("xiufu_to", QVariant::fromValue((ServerPlayer *)(NULL))).value<ServerPlayer *>();

    int equipped_id = -1;
    if (target->getEquip(equip->location()) != NULL)
        equipped_id = target->getEquip(equip->location())->getEffectiveId();
    QList<CardsMoveStruct> exchangeMove;
    CardsMoveStruct move1(equip->getId(), target, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_TRANSFER, mori->objectName()));
    exchangeMove.push_back(move1);
    //since new equip could not move into gaoao's area, the equipped one will maintain.
    if (equipped_id != -1 && !target->hasSkill("gaoao")) {
        CardsMoveStruct move2(equipped_id, NULL, Player::DiscardPile, CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, target->objectName()));
        exchangeMove.push_back(move2);
    }

    if (!target->hasSkill("gaoao")) {
        LogMessage zhijian;
        zhijian.type = "$ZhijianEquip";
        zhijian.from = target;
        zhijian.card_str = QString::number(equip->getId());
        room->sendLog(zhijian);
    }

    room->moveCardsAtomic(exchangeMove, true);
}

XiufuMoveCard::XiufuMoveCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool XiufuMoveCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void XiufuMoveCard::onUse(Room *, const CardUseStruct &card_use) const
{
    card_use.from->tag["xiufu_id"] = subcards.first();
    card_use.from->tag["xiufu_to"] = QVariant::fromValue(card_use.to.first());
}

class XiufuMove : public OneCardViewAsSkill
{
public:
    XiufuMove()
        : OneCardViewAsSkill("xiufumove")
    {
        response_pattern = "@@xiufumove";
        expand_pile = "#xiufu_temp";
    }

    bool viewFilter(const Card *to_select) const
    {
        return Self->getPile("#xiufu_temp").contains(to_select->getId());
    }

    const Card *viewAs(const Card *originalCard) const
    {
        XiufuMoveCard *move = new XiufuMoveCard;
        move->addSubcard(originalCard);
        return move;
    }
};

class Xiufu : public ZeroCardViewAsSkill
{
public:
    Xiufu()
        : ZeroCardViewAsSkill("xiufu")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return (!player->hasFlag("Global_xiufuFailed") && !player->hasFlag("xiufu_used")) || player->getMark("@xiufudebug") > 0;
    }

    const Card *viewAs() const
    {
        return new XiufuCard;
    }
};

class XiufuDebug : public TriggerSkill
{
public:
    XiufuDebug()
        : TriggerSkill("xiufu")
    {
        events << MarkChanged << EventPhaseChanging;
        view_as_skill = new Xiufu;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->isAlive() && change.player->hasFlag("xiufu_used") && change.to == Player::Play)
                room->setPlayerFlag(change.player, "-xiufu_used");
        } else {
            MarkChangeStruct change = data.value<MarkChangeStruct>();
            if (change.name == "@xiufudebug") {
                QList<int> equips;
                foreach (int id, Sanguosha->getRandomCards()) {
                    const Card *card = Sanguosha->getEngineCard(id);
                    if (card->isKindOf("EquipCard") && room->getCardPlace(id) != Player::DiscardPile)
                        equips << id;
                }

                DummyCard dummy(equips);
                room->moveCardTo(&dummy, NULL, Player::DiscardPile);
            }
        }
    }
};

class Fandu : public TriggerSkill
{
public:
    Fandu()
        : TriggerSkill("fandu")
    {
        events << EventPhaseStart << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->isAlive() && p->hasSkill(this) && p->getPhase() == Player::Start)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, p, p);
        } else {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->isAlive() && damage.to->hasSkill(this)) {
                QList<SkillInvokeDetail> d;
                for (int i = 0; i < damage.damage; ++i)
                    d << SkillInvokeDetail(this, damage.to, damage.to);
                return d;
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(2);
        QList<ServerPlayer *> listt;
        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (p->canDiscard(invoke->invoker, "hs"))
                listt << p;
        }
        if (listt.isEmpty())
            return false;
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, listt, "fandu", "@fandu-select", false, true);
        int to_throw = room->askForCardChosen(target, invoke->invoker, "hs", "fandu", false, Card::MethodDiscard);
        room->throwCard(to_throw, invoke->invoker, target);

        return false;
    }
};

#pragma message WARN("todo_fs: split ServerPlayer::pindian to 2 parts")

class Taohuan : public TriggerSkill
{
public:
    Taohuan()
        : TriggerSkill("taohuan")
    {
        events << CardsMoveOneTime << Pindian;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!move.from_places.contains(Player::PlaceHand) && !move.from_places.contains(Player::PlaceEquip))
                return QList<SkillInvokeDetail>();
            ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
            if (from == NULL || from->isDead() || !from->hasSkill(this) || from->isKongcheng())
                return QList<SkillInvokeDetail>();

            // move from is now certain, we can now only check the card movement case.
            if (move.to_place == Player::DiscardPile) {
                // cae 1: discard
                if (move.reason.m_reason != CardMoveReason::S_REASON_DISMANTLE)
                    return QList<SkillInvokeDetail>();

                ServerPlayer *thrower = room->findPlayerByObjectName(move.reason.m_playerId);
                if (thrower == NULL || thrower->isDead() || thrower->isKongcheng() || from == thrower)
                    return QList<SkillInvokeDetail>();

                QList<SkillInvokeDetail> d;
                int i = 0;
                foreach (int id, move.card_ids) {
                    const Card *c = Sanguosha->getCard(id);
                    Player::Place from_place = move.from_places.value(i++);
                    if (c != NULL && (c->isKindOf("EquipCard") || c->isKindOf("TrickCard")) && (from_place == Player::PlaceHand || from_place == Player::PlaceEquip))
                        d << SkillInvokeDetail(this, from, from, NULL, false, thrower);
                }
                return d;
            } else if (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip) {
                ServerPlayer *obtainer = qobject_cast<ServerPlayer *>(move.to);
                if (obtainer == NULL || obtainer->isDead() || obtainer == from || obtainer->isKongcheng())
                    return QList<SkillInvokeDetail>();

                QList<SkillInvokeDetail> d;
                for (int i = 0; i < move.card_ids.length(); ++i) {
                    Player::Place from_place = move.from_places.value(i);
                    if (from_place == Player::PlaceHand || from_place == Player::PlaceEquip)
                        d << SkillInvokeDetail(this, from, from, NULL, false, obtainer);
                }
                return d;
            }
        } else {
            PindianStruct *pindian = data.value<PindianStruct *>();
            if (pindian->isSuccess() && !pindian->to->isNude() && pindian->reason == objectName())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, pindian->from, pindian->from, NULL, true, pindian->to);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == Pindian)
            return true;
        else {
            //for intention ai
            invoke->invoker->tag["taohuantarget"] = QVariant::fromValue(invoke->preferredTarget);
            return invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget));
        }
        return false;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (e == Pindian) {
            int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hes", objectName());
            room->obtainCard(invoke->invoker, id, room->getCardPlace(id) != Player::PlaceHand);
        } else {
            invoke->invoker->pindian(invoke->preferredTarget, objectName());
        }

        return false;
    }
};

class Shitu : public TriggerSkill
{
public:
    Shitu()
        : TriggerSkill("shitu")
    {
        events << EventPhaseChanging << DamageDone << TurnStart;
        global = true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        switch (triggerEvent) {
        case DamageDone: {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isCurrent())
                room->setTag("shituDamageOrDeath", true);
            break;
        }
        case TurnStart:
            room->setTag("shituDamageOrDeath", false);
            break;
        default:
            break;
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->hasSkill(this) && change.player->isAlive() && change.to == Player::NotActive) {
                QVariant extraTag = room->getTag("touhou-extra");
                QVariant damageOrDeathTag = room->getTag("shituDamageOrDeath");

                bool nowExtraTurn = extraTag.canConvert(QVariant::Bool) && extraTag.toBool();
                bool damageOrDeath = damageOrDeathTag.canConvert(QVariant::Bool) && damageOrDeathTag.toBool();
                bool extraTurnExist = room->getThread()->hasExtraTurn();

                if (!nowExtraTurn && !damageOrDeath && !extraTurnExist)
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getAllPlayers(), objectName(), "@shitu-playerchoose", true, true);
        if (target != NULL) {
            invoke->targets << target;
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->notifySkillInvoked(invoke->invoker, objectName());

        ExtraTurnStruct extra;
        extra.set_phases << Player::RoundStart << Player::Draw << Player::NotActive;
        invoke->targets.first()->tag["ExtraTurnInfo"] = QVariant::fromValue(extra);
        invoke->targets.first()->gainAnExtraTurn();

        return false;
    }
};

class Mengxian : public TriggerSkill
{
public:
    Mengxian()
        : TriggerSkill("mengxian")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *merry = data.value<ServerPlayer *>();
        if (merry->hasSkill(this) && merry->getPhase() == Player::Start && merry->isAlive() && merry->getPile("jingjie").length() >= 3 && merry->getMark("mengxian") == 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, merry, merry, NULL, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->doLightbox("$mengxianAnimate", 4000);
        room->touhouLogmessage("#MengxianWake", invoke->invoker, objectName(), QList<ServerPlayer *>(), QString::number(invoke->invoker->getPile("jingjie").length()));
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->addPlayerMark(invoke->invoker, "mengxian");
        if (room->changeMaxHpForAwakenSkill(invoke->invoker) && invoke->invoker->getMark("mengxian") > 0) {
            room->recover(invoke->invoker, RecoverStruct());
            room->handleAcquireDetachSkills(invoke->invoker, "luanying");
        }
        return false;
    }
};

class LuanyingVS : public OneCardViewAsSkill
{
public:
    LuanyingVS()
        : OneCardViewAsSkill("luanying")
    {
        response_pattern = "@@luanying";
        expand_pile = "jingjie";
    }

    bool viewFilter(const Card *to_select) const
    {
        if (!Self->getPile("jingjie").contains(to_select->getId()))
            return false;

        QString property = Self->property("luanying").toString();
        if (property == "black")
            return to_select->isBlack();
        else
            return to_select->isRed();
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};

class Luanying : public TriggerSkill
{
public:
    Luanying()
        : TriggerSkill("luanying")
    {
        events << CardUsed << CardResponded;
        view_as_skill = new LuanyingVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *user = NULL;
        const Card *card = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
            user = use.from;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse) { //!resp.m_isProvision && !resp.m_isRetrial
                card = resp.m_card;
                user = resp.m_from;
            }
        }

        if (user == NULL || card == NULL)
            return QList<SkillInvokeDetail>();

        if (card->isKindOf("BasicCard")) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, room->getOtherPlayers(user)) {
                if (p->hasSkill(this) && p->isAlive()) {
                    QList<int> jingjie = p->getPile("jingjie");
                    bool flag = false;
                    foreach (int id, jingjie) {
                        if (Sanguosha->getCard(id)->getColor() == card->getColor()) {
                            flag = true;
                            break;
                        }
                    }
                    if (flag)
                        d << SkillInvokeDetail(this, p, p);
                }
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *user = NULL;
        const Card *card = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
            user = use.from;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            card = resp.m_card;
            user = resp.m_from;
        }

        if (user == NULL || card == NULL)
            return false;

        if (card->isRed()) {
            invoke->invoker->setProperty("luanying", "red");
            room->notifyProperty(invoke->invoker, invoke->invoker, "luanying");
        } else {
            invoke->invoker->setProperty("luanying", "black");
            room->notifyProperty(invoke->invoker, invoke->invoker, "luanying");
        }

        QString prompt = "@luanying-invoke:" + user->objectName() + ":" + card->objectName();
        const Card *c = room->askForCard(invoke->invoker, "@@luanying", prompt, data, Card::MethodNone, NULL, false, "luanying");

        if (c != NULL) {
            room->obtainCard(user, c, true);
            room->touhouLogmessage("#weiya", user, objectName(), QList<ServerPlayer *>(), card->objectName());

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.nullified_list << "_ALL_TARGETS";
            data = QVariant::fromValue(use);
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct s = data.value<CardResponseStruct>();
            s.m_isNullified = true;
            data = QVariant::fromValue(s);
        }

        return false;
    }
};

LianxiCard::LianxiCard()
{
    will_throw = false;
    m_skillName = "lianxi";
    can_recast = true;
}
bool LianxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    IronChain *card = new IronChain(Card::NoSuit, 0);
    card->setSkillName("lianxi");
    card->deleteLater();
    int total_num = 2 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, card);
    return targets.length() < total_num && !Self->isProhibited(to_select, card) && !Self->isCardLimited(card, Card::MethodUse);
}
bool LianxiCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    IronChain *card = new IronChain(Card::NoSuit, 0);
    card->setSkillName("lianxi");
    card->deleteLater();
    int total_num = 2 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, card);
    return targets.length() <= total_num;
}
const Card *LianxiCard::validate(CardUseStruct &) const
{
    IronChain *card = new IronChain(Card::NoSuit, 0);
    card->setSkillName("lianxi");
    return card;
}

class LianxiVS : public ZeroCardViewAsSkill
{
public:
    LianxiVS()
        : ZeroCardViewAsSkill("lianxi")
    {
        response_pattern = "@@lianxi";
    }

    virtual const Card *viewAs() const
    {
        return new LianxiCard;
    }
};

class Lianxi : public TriggerSkill
{
public:
    Lianxi()
        : TriggerSkill("lianxi")
    {
        events << CardResponded << CardUsed;
        view_as_skill = new LianxiVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *user = NULL;
        const Card *card = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
            user = use.from;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            card = resp.m_card;
            user = resp.m_from;
        }

        if (user != NULL && user->hasSkill(this) && user->isAlive() && card != NULL && card->isKindOf("Slash"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, user, user);

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->askForUseCard(invoke->invoker, "@@lianxi", "@lianxi");
        return false;
    }
};

class Yueshi : public TriggerSkill
{
public:
    Yueshi()
        : TriggerSkill("yueshi")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *toyo = data.value<ServerPlayer *>();
        if (toyo->hasSkill(this) && toyo->getPhase() == Player::Start && toyo->isAlive() && toyo->isChained() && toyo->getMark("yueshi") == 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, toyo, toyo, NULL, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->doLightbox("$yueshiAnimate", 4000);
        ServerPlayer *toyo = invoke->invoker;
        room->touhouLogmessage("#YueshiWake", toyo, "yueshi");
        room->notifySkillInvoked(toyo, objectName());
        room->addPlayerMark(toyo, objectName());
        if (room->changeMaxHpForAwakenSkill(toyo, 1) && toyo->getMark("yueshi") > 0)
            room->handleAcquireDetachSkills(toyo, "ruizhi");

        return false;
    }
};

// this skill is like this:
// cost: You discard one card.
// target: a skill of damage.from.
// effect:
// if you own a skill that is acquired by this method, you should lose this skill, and make the original skill become valid.
// and you make the selected skill become invalid, and you acquire this skill.

// if you or the original skill owner die, or the original skill owner lose the skill, you you should lose this skill, and make the original skill become valid.

class Pingyi : public TriggerSkill
{
public:
    Pingyi()
        : TriggerSkill("pingyi")
    {
        events << Damage << Damaged << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("pingyi_used"))
                        room->setPlayerFlag(p, "-pingyi_used");
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e == EventPhaseChanging)
            return QList<SkillInvokeDetail>();
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *yori = (e == Damage) ? damage.from : damage.to;
        ServerPlayer *target = (e == Damage) ? damage.to : damage.from;

        if (yori == NULL || yori->isDead() || !yori->hasSkill(this) || yori->hasFlag("pingyi_used") || target == NULL || target->isDead() || yori == target)
            return QList<SkillInvokeDetail>();

        foreach (const Skill *skill, target->getVisibleSkillList()) {
            if (skill->isLordSkill() || skill->isAttachedLordSkill() || skill->getFrequency() == Skill::Limited || skill->getFrequency() == Skill::Wake
                || skill->getFrequency() == Skill::Eternal)
                continue;
            if (!yori->hasSkill(skill, true) && target->hasSkill(skill))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, yori, yori, NULL, false, target);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *yori = invoke->invoker;

        QStringList skill_names;

        foreach (const Skill *skill, invoke->preferredTarget->getVisibleSkillList()) {
            if (skill->isLordSkill() || skill->isAttachedLordSkill() || skill->getFrequency() == Skill::Limited || skill->getFrequency() == Skill::Wake
                || skill->getFrequency() == Skill::Eternal)
                continue;

            if (!yori->hasSkill(skill, true) && invoke->preferredTarget->hasSkill(skill))
                skill_names << skill->objectName();
        }
        skill_names << "cancel";
        yori->tag["pingyi_target"] = QVariant::fromValue(invoke->preferredTarget);

        QString skill_name = room->askForChoice(yori, objectName(), skill_names.join("+"));
        invoke->tag["selected_skill"] = skill_name;
        return skill_name != "cancel";
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString skill_name = invoke->tag.value("selected_skill", QString()).toString();
        if (skill_name.isEmpty())
            return false;

        room->notifySkillInvoked(invoke->invoker, objectName());
        room->touhouLogmessage("#InvokeSkill", invoke->invoker, objectName());
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());

        const Skill *skill = Sanguosha->getSkill(skill_name);
        room->setPlayerFlag(invoke->invoker, "pingyi_used");
        skillProcess(room, invoke->invoker, invoke->targets.first(), skill);
        return false;
    }

    static void skillProcess(Room *room, ServerPlayer *yori, ServerPlayer *skill_owner = NULL, const Skill *skill = NULL)
    {
        JsonArray arg;
        arg << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
        arg << yori->objectName();
        if (skill_owner == NULL || skill == NULL) {
            arg << QString() << QString();
        } else {
            arg << skill_owner->getGeneral()->objectName();
            arg << skill->objectName();
        }
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

        if (yori->tag.contains("pingyi_skill") && yori->tag.contains("pingyi_originalOwner")) {
            QString originalSkillName = yori->tag.value("pingyi_skill", QString()).toString();
            const Skill *originalSkill = Sanguosha->getSkill(originalSkillName);
            ServerPlayer *originalOwner = yori->tag["pingyi_originalOwner"].value<ServerPlayer *>();

            // 1. yorihime lost the acquired skill
            if (!originalSkillName.isEmpty())
                room->handleAcquireDetachSkills(yori, "-" + originalSkillName, true);

            yori->tag.remove("pingyi_skill");
            yori->tag.remove("pingyi_originalOwner");
            yori->tag.remove("Huashen_skill");
            yori->tag.remove("Huashen_target");
            originalOwner->tag.remove("pingyi_from");
            originalOwner->tag.remove("pingyi_invalidSkill");
            room->setPlayerSkillInvalidity(originalOwner, originalSkill, false);
        }

        if (skill != NULL && skill_owner != NULL) {
            yori->tag["pingyi_skill"] = skill->objectName();
            yori->tag["pingyi_originalOwner"] = QVariant::fromValue(skill_owner);
#pragma message WARN("todo_fs: pingyi_from and pingyi_invalidSkill could large than 1")
            skill_owner->tag["pingyi_from"] = QVariant::fromValue(yori);
            skill_owner->tag["pingyi_invalidSkill"] = skill->objectName();

            // 2.let the skill become invalid
            room->setPlayerSkillInvalidity(skill_owner, skill, true);

            // 3. acquire the skill
            yori->tag["Huashen_skill"] = skill->objectName();
            yori->tag["Huashen_target"] = skill_owner->getGeneralName();
            room->handleAcquireDetachSkills(yori, skill->objectName(), true);
        }
    }
};

class PingyiHandler : public TriggerSkill
{
public:
    PingyiHandler()
        : TriggerSkill("#pingyi_handle")
    {
        events << EventPhaseChanging << Death;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who->tag.contains("pingyi_skill"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, death.who, death.who, NULL, true);
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.to == Player::NotActive) {
                ServerPlayer *yori = phase_change.player;
                ServerPlayer *who = yori->tag.value("pingyi_originalOwner").value<ServerPlayer *>();
                if (yori != NULL && who != NULL)
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, yori, yori, NULL, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        Pingyi::skillProcess(room, invoke->invoker);
        return false;
    }
};

ZhesheCard::ZhesheCard()
{
    will_throw = true;
}

bool ZhesheCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.length() == 0;
}

void ZhesheCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->tag["zheshe_target"] = QVariant::fromValue(effect.to);
}

class ZhesheVS : public OneCardViewAsSkill
{
public:
    ZhesheVS()
        : OneCardViewAsSkill("zheshe")
    {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@zheshe";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ZhesheCard *card = new ZhesheCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class Zheshe : public TriggerSkill
{
public:
    Zheshe()
        : TriggerSkill("zheshe")
    {
        events << DamageInflicted << DamageComplete;
        view_as_skill = new ZhesheVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageInflicted) {
            if (damage.to->hasSkill(this) && damage.to->isAlive() && !damage.to->isKongcheng() && damage.to != damage.from)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        } else if (triggerEvent == DamageComplete) {
            if (damage.to->isAlive() && damage.to->getMark("zheshetransfer") > 0 && damage.reason == "zheshe")
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true, damage.to);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == DamageInflicted) {
            if (room->askForUseCard(invoke->invoker, "@@zheshe", "@zheshe", -1, Card::MethodDiscard) && invoke->invoker->tag.contains("zheshe_target")) {
                ServerPlayer *target = invoke->invoker->tag.value("zheshe_target", QVariant::fromValue((ServerPlayer *)NULL)).value<ServerPlayer *>();
                if (target != NULL) {
                    invoke->targets << target;
                    return true;
                }
            }
        } else if (triggerEvent == DamageComplete) {
            invoke->preferredTarget->setMark("zheshetransfer", 0);
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == DamageInflicted) {
            invoke->targets.first()->setMark("zheshetransfer", 1);

            DamageStruct damage;
            damage.from = invoke->invoker;
            damage.to = invoke->targets.first();
            damage.reason = "zheshe";
            room->damage(damage);
            return true;
        } else if (triggerEvent == DamageComplete)
            invoke->targets.first()->drawCards(invoke->targets.first()->getLostHp(), objectName());

        return false;
    }
};

class Tanchi : public TriggerSkill
{
public:
    Tanchi()
        : TriggerSkill("tanchi")
    {
        events << PreHpRecover;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        RecoverStruct recover = data.value<RecoverStruct>();
        if (recover.to->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, recover.to, recover.to, NULL, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        RecoverStruct recover = data.value<RecoverStruct>();
        recover.recover = recover.recover + 1;
        data = QVariant::fromValue(recover);
        return false;
    }
};

ZhuonongCard::ZhuonongCard()
{
    m_skillName = "zhuonong";
}

bool ZhuonongCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.length() == 0;
}

void ZhuonongCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    effect.from->tag["zhuonong_target"] = QVariant::fromValue(effect.to);
    QString choice = room->askForChoice(effect.from, "zhuonong", "rd+dr", QVariant::fromValue(effect.to));
    RecoverStruct recover;
    recover.who = effect.from;
    DamageStruct damage(objectName(), effect.from, effect.to, 1, DamageStruct::Fire);

    QString logtype = (choice == "rd") ? "#zhuonong_rd" : "#zhuonong_dr";
    QList<ServerPlayer *> logto;
    logto << effect.to;
    room->touhouLogmessage(logtype, effect.from, "zhuonong", logto, QString::number(1));
    if (choice == "rd" && effect.to->isWounded())
        room->recover(effect.to, recover);
    room->damage(damage);
    if (choice == "dr" && effect.to->isWounded() && effect.to->isAlive())
        room->recover(effect.to, recover);
}

class Zhuonong : public ZeroCardViewAsSkill
{
public:
    Zhuonong()
        : ZeroCardViewAsSkill("zhuonong")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ZhuonongCard");
    }

    virtual const Card *viewAs() const
    {
        return new ZhuonongCard;
    }
};

class Jijing : public TriggerSkill
{
public:
    Jijing()
        : TriggerSkill("jijing")
    {
        events << Damaged << EventPhaseChanging;
        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && change.player->hasFlag("jijing")) {
                foreach (ServerPlayer *p, room->getOtherPlayers(change.player)) {
                    if (p->getMark("@jijing") > 0) {
                        room->setPlayerMark(p, "@jijing", 0);
                        room->setFixedDistance(change.player, p, -1);
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e != Damaged)
            return QList<SkillInvokeDetail>();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->isDead())
            return QList<SkillInvokeDetail>();

        ServerPlayer *p = room->getCurrent();
        if (!p || !p->isCurrent() || p->isDead())
            return QList<SkillInvokeDetail>();

        if (p->hasSkill(this) && damage.to->getMark("@jijing") == 0 && damage.to != p)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, p, p, NULL, true, damage.to);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->setPlayerFlag(invoke->invoker, "jijing");
        room->setFixedDistance(invoke->invoker, invoke->targets.first(), 1);
        invoke->targets.first()->gainMark("@jijing"); // trigger ganying
        return false;
    }
};

class Ganying;
namespace {
Ganying *ganying_instance;
}

class Ganying : public TriggerSkill
{
public:
    Ganying()
        : TriggerSkill("ganying")
    {
        ganying_instance = this;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1, objectName());
        if (invoke->targets.isEmpty())
            return false;

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, invoke->targets, "ganying", "@ganying", true);
        if (target == NULL)
            return false;

        int id = room->askForCardChosen(invoke->invoker, target, "hs", "ganying", false, Card::MethodDiscard);
        room->throwCard(id, target, invoke->invoker == target ? NULL : invoke->invoker);

        return false;
    }
};

class GanyingHandler : public TriggerSkill
{
public:
    GanyingHandler()
        : TriggerSkill("ganying_handle")
    {
        events << NumOfEvents;
        global = true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const
    {
        if (triggerEvent == GameStart) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                QVariantMap distance_map;
                foreach (ServerPlayer *q, room->getOtherPlayers(p))
                    distance_map[q->objectName()] = p->distanceTo(q);

                p->tag["distance_map"] = distance_map;
            }
            return;
        }

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            p->tag.remove("distance_changed_" + QString::number(triggerEvent));
            QVariantMap distance_map;
            foreach (ServerPlayer *q, room->getOtherPlayers(p))
                distance_map[q->objectName()] = p->distanceTo(q);

            QStringList distance_changed;
            QVariantMap distance_map_old = p->tag.value("distance_map", QVariantMap()).toMap();

            foreach (const QString &key, distance_map_old.keys()) {
                bool ok = false;
                if (distance_map.value(key).toInt(&ok) != -1 && ok) {
                    int old_distance = distance_map_old.value(key).toInt();
                    int new_distance = distance_map.value(key).toInt();
                    if (old_distance != new_distance)
                        distance_changed << key;
                } else
                    distance_map[key] = -1;
            }

            p->tag["distance_changed_" + QString::number(triggerEvent)] = distance_changed;
            p->tag["distance_map"] = distance_map;
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &) const
    {
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (!p->hasSkill("ganying"))
                continue;
            bool invoke = false;
            QList<ServerPlayer *> targets;
            //case1: distance changed (self to other)
            QStringList from_distance_changed = p->tag.value("distance_changed_" + QString::number(triggerEvent), QStringList()).toStringList();
            if (!from_distance_changed.isEmpty()) {
                invoke = true;
                foreach (const QString &c, from_distance_changed) {
                    ServerPlayer *target = room->findPlayerByObjectName(c);
                    if (target != NULL && p->canDiscard(target, "hs") && !targets.contains(target))
                        targets << target;
                }
            }
            //case2:distance changed(other to self)
            foreach (ServerPlayer *q, room->getOtherPlayers(p)) {
                QStringList to_distance_changed = q->tag.value("distance_changed_" + QString::number(triggerEvent), QStringList()).toStringList();
                foreach (const QString &c, to_distance_changed) {
                    ServerPlayer *to = room->findPlayerByObjectName(c);
                    if (to != NULL && p == to)
                        invoke = true;
                    if (to != NULL && p == to && p->canDiscard(q, "hs") && !targets.contains(q)) {
                        targets << q;
                        break;
                    }
                }
            }

            if (invoke)
                d << SkillInvokeDetail(ganying_instance, p, p, targets);
        }
        return d;
    }
};

class Zhujiu : public TriggerSkill
{
public:
    Zhujiu()
        : TriggerSkill("zhujiu")
    {
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Peach") || use.card->isKindOf("Analeptic")) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *to, use.to) {
                foreach (ServerPlayer *p, room->getOtherPlayers(to)) {
                    if (p->hasSkill(this) && !p->isNude())
                        d << SkillInvokeDetail(this, p, to);
                }
            }
            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        invoke->owner->tag["zhujiu_target"] = QVariant::fromValue(invoke->invoker);
        const Card *c = room->askForCard(invoke->owner, "..", "@zhujiu:" + invoke->invoker->objectName(), data, Card::MethodNone);
        if (c) {
            invoke->owner->showHiddenSkill(objectName());
            CardMoveReason r(CardMoveReason::S_REASON_GIVE, invoke->owner->objectName(), objectName(), QString());
            room->obtainCard(invoke->invoker, c, r, room->getCardPlace(c->getEffectiveId()) != Player::PlaceHand);

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->notifySkillInvoked(invoke->owner, objectName());
        invoke->owner->drawCards(1, objectName());

        return false;
    }
};

YushouCard::YushouCard()
{
    m_skillName = "yushou";
    mute = true;
}

bool YushouCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (targets.length() == 0) {
        return !to_select->getEquips().isEmpty();
    } else if (targets.length() == 1) {
        foreach (const Card *e, targets.first()->getEquips()) {
            const EquipCard *equip = qobject_cast<const EquipCard *>(e->getRealCard());
            if (!to_select->getEquip(equip->location()))
                return true;
        }
    }
    return false;
}

bool YushouCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void YushouCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *from = card_use.from;
    ServerPlayer *to1 = card_use.to.at(0);
    ServerPlayer *to2 = card_use.to.at(1);
    QList<ServerPlayer *> logto;
    logto << to1 << to2;
    room->touhouLogmessage("#ChoosePlayerWithSkill", from, "yushou", logto, "");
    room->notifySkillInvoked(card_use.from, "yushou");
    QList<int> disable;

    foreach (const Card *e, to1->getEquips()) {
        const EquipCard *equip = qobject_cast<const EquipCard *>(e->getRealCard());
        if (to2->getEquip(equip->location())) {
            disable << e->getEffectiveId();
        }
    }
    from->tag["yushou_target"] = QVariant::fromValue(to2);
    int card_id = room->askForCardChosen(from, to1, "e", "yushou", false, Card::MethodNone, disable);
    from->showHiddenSkill("yushou");
    const Card *card = Sanguosha->getCard(card_id);
    room->moveCardTo(card, to1, to2, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_TRANSFER, from->objectName(), "yushou", QString()));

    QString prompt = "confirm:" + to2->objectName();
    to1->tag["yushou_target"] = QVariant::fromValue(to2);
    if (to1->askForSkillInvoke("yushou_damage", prompt))
        room->damage(DamageStruct("yushou", to1, to2, 1, DamageStruct::Normal));
}

class Yushou : public ZeroCardViewAsSkill
{
public:
    Yushou()
        : ZeroCardViewAsSkill("yushou")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("YushouCard");
    }

    virtual const Card *viewAs() const
    {
        return new YushouCard;
    }
};

PanduCard::PanduCard()
{
    handling_method = Card::MethodNone;
}

bool PanduCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() == 0 && !to_select->isKongcheng() && to_select != Self;
}

void PanduCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    int card_id = room->askForCardChosen(source, target, "hs", "pandu");
    room->showCard(target, card_id);
    Card *showcard = Sanguosha->getCard(card_id);
    if (showcard->isKindOf("Slash")) {
        if (!target->isCardLimited(showcard, Card::MethodUse)) {
            room->setCardFlag(showcard, "pandu");
            room->useCard(CardUseStruct(showcard, target, source), false);
        }
    } else if (!showcard->isKindOf("BasicCard"))
        room->obtainCard(source, showcard, true);
}

class Pandu : public ZeroCardViewAsSkill
{
public:
    Pandu()
        : ZeroCardViewAsSkill("pandu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("PanduCard");
    }

    virtual const Card *viewAs() const
    {
        return new PanduCard;
    }
};

class Bihuo : public TriggerSkill
{
public:
    Bihuo()
        : TriggerSkill("bihuo")
    {
        events << TargetConfirming << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerMark(p, objectName(), 0);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e != TargetConfirming)
            return QList<SkillInvokeDetail>();
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash"))
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        use.card->setFlags("IgnoreFailed");
        foreach (ServerPlayer *p, use.to) {
            if (!p->hasSkill(this) || p->isKongcheng() || p->getMark(objectName()) > 0)
                continue;

            bool flag = false;
            foreach (ServerPlayer *q, room->getOtherPlayers(p)) {
                if (!(use.from == q || use.to.contains(q)) && use.from->canSlash(q, use.card, false)) {
                    flag = true;
                    break;
                }
            }

            if (flag)
                d << SkillInvokeDetail(this, p, p);
        }
        use.card->setFlags("-IgnoreFailed");
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<ServerPlayer *> targets;
        use.card->setFlags("IgnoreFailed");
        foreach (ServerPlayer *q, room->getOtherPlayers(invoke->invoker)) {
            if (!(use.from == q || use.to.contains(q)) && use.from->canSlash(q, use.card, false))
                targets << q;
        }
        use.card->setFlags("-IgnoreFailed");

        QString prompt = "@bihuo-playerchosen:" + use.from->objectName();
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), prompt, true, true);
        if (target) {
            invoke->invoker->showHiddenSkill(objectName());

            QVariantMap bihuo_list = target->tag.value("bihuo", QVariantMap()).toMap();
            int i = bihuo_list.value(invoke->invoker->objectName(), 0).toInt();
            i += invoke->invoker->getHandcardNum();
            bihuo_list[invoke->invoker->objectName()] = i;
            target->tag["bihuo"] = bihuo_list;
            room->setPlayerMark(invoke->invoker, objectName(), 1);

            const Card *c = invoke->invoker->wholeHandCards();
            CardMoveReason r(CardMoveReason::S_REASON_GIVE, invoke->owner->objectName(), objectName(), QString());
            room->obtainCard(target, c, r, false);
            delete c;

            invoke->targets << target;
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.to << invoke->targets;
        use.to.removeOne(invoke->invoker);
        data = QVariant::fromValue(use);

        QList<ServerPlayer *> logto;
        logto << invoke->invoker;
        QList<ServerPlayer *> logto1;
        logto1 << invoke->targets;
        room->touhouLogmessage("$CancelTarget", use.from, use.card->objectName(), logto);
        room->touhouLogmessage("#huzhu_change", use.from, use.card->objectName(), logto1);

        return false;
    }
};

class BihuoReturn : public TriggerSkill
{
public:
    BihuoReturn()
        : TriggerSkill("#bihuo")
    {
        events << EventPhaseChanging;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                QVariantMap bihuo_list = p->tag.value("bihuo", QVariantMap()).toMap();

                foreach (const QString &key, bihuo_list.keys()) {
                    int n = bihuo_list.value(key, 0).toInt();
                    if (n <= 0)
                        continue;

                    ServerPlayer *kos = room->findPlayerByObjectName(key);
                    if (kos == NULL || kos->isDead())
                        continue;

                    SkillInvokeDetail s(this, kos, p, NULL, true);
                    //s.tag["n"] = n;
                    d << s;
                }
            }
            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        //int n = invoke->tag.value("n", 0).toInt();
        QVariantMap bihuo_list = invoke->invoker->tag.value("bihuo", QVariantMap()).toMap();
        int n = bihuo_list.value(invoke->owner->objectName(), 0).toInt();
        bihuo_list[invoke->owner->objectName()] = 0;
        invoke->invoker->tag["bihuo"] = bihuo_list;

        if (n <= 0)
            return false;
        n = qMin(n, invoke->invoker->getHandcardNum());
        const Card *c = room->askForExchange(invoke->invoker, objectName(), n, n, false, "@bihuo-return:" + QString::number(n) + ":" + invoke->owner->objectName());
        CardMoveReason r(CardMoveReason::S_REASON_GIVE, invoke->owner->objectName(), objectName(), QString());
        room->obtainCard(invoke->owner, c, r, false);
        delete c;
        return false;
    }
};

class Xunshi : public TriggerSkill
{
public:
    Xunshi()
        : TriggerSkill("xunshi")
    {
        events << TargetSpecifying;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->isKindOf("Peach") || use.card->isKindOf("Slash") || use.card->isNDTrick()) {
            use.card->setFlags("xunshi");
            use.card->setFlags("IgnoreFailed");
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (use.from->isAlive() && p != use.from && !use.to.contains(p) && !use.to.isEmpty()
                    && (p->getHandcardNum() < use.from->getHandcardNum() || p->getHp() < use.from->getHp()) && !use.from->isProhibited(p, use.card)) {
                    if (use.card->isKindOf("Peach")) {
                        if (p->isWounded())
                            d << SkillInvokeDetail(this, p, p, NULL, true);
                    } else if (use.card->isKindOf("Drowning")) {
                        if (p->canDiscard(p, "e"))
                            d << SkillInvokeDetail(this, p, p, NULL, true);
                    } else if (use.card->targetFilter(QList<const Player *>(), p, use.from))
                        d << SkillInvokeDetail(this, p, p, NULL, true);
                }
            }
            use.card->setFlags("-IgnoreFailed");
            use.card->setFlags("-xunshi");
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();

        room->notifySkillInvoked(invoke->invoker, objectName());
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), use.from->objectName());
        QList<ServerPlayer *> logto;
        logto << invoke->invoker;
        room->touhouLogmessage("#xunshi", use.from, use.card->objectName(), logto, objectName());

        //wtf!?  this flag can not detected in sgs.ai_choicemade_filter.cardChosen.snatch
        room->setCardFlag(use.card, "xunshi");
        invoke->invoker->drawCards(1);
        use.to << invoke->invoker;
        room->sortByActionOrder(use.to);
        data = QVariant::fromValue(use);

        /*if (use.card->isKindOf("Collateral")) {
            QList<ServerPlayer *> col_targets;
            foreach (ServerPlayer *t, room->getOtherPlayers(invoke->invoker)) {
                if (invoke->invoker->canSlash(t))
                    col_targets << t;
            }
            ServerPlayer *victim = room->askForPlayerChosen(use.from, col_targets, objectName(), "@xunshi_col:" + invoke->invoker->objectName(), false);
            invoke->invoker->tag["collateralVictim"] = QVariant::fromValue(victim);
            if (victim) {
                LogMessage log;
                log.type = "#CollateralSlash";
                log.from = use.from;
                log.to << victim;
                room->sendLog(log);
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), victim->objectName());
            }
        }*/
        return false;
    }
};

class XunshiDistance : public TargetModSkill
{
public:
    XunshiDistance()
        : TargetModSkill("xunshi-dist")
    {
        pattern = "Slash,TrickCard+^DelayedTrick";
    }

    int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->hasFlag("xunshi"))
            return 1000;

        return 0;
    }
};

class Jidong : public TriggerSkill
{
public:
    Jidong()
        : TriggerSkill("jidong")
    {
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->isKindOf("Slash") || use.card->isKindOf("Duel")) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (use.from->isAlive() && p != use.from && use.to.contains(p) && p->canDiscard(p, "hs"))
                    d << SkillInvokeDetail(this, p, p, NULL);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QStringList prompt_list;
        prompt_list << "jidong-discard" << use.card->objectName() << use.from->objectName() << "basic";
        QString prompt = prompt_list.join(":");

        const Card *card = room->askForCard(invoke->invoker, ".Basic", prompt, data, Card::MethodDiscard, NULL, false, objectName());
        if (card)
            invoke->invoker->tag["jidong_card"] = QVariant::fromValue(card);
        return card != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        const Card *card = invoke->invoker->tag["jidong_card"].value<const Card *>();
        bool can = (card->getNumber() >= 13 || !use.from->canDiscard(use.from, "hs")) ? true : false;
        if (!can) {
            QString point = QString::number(card->getNumber() + 1);
            QString pattern = QString(".|.|%1~|hand").arg(point);
            QStringList prompt_list;
            prompt_list << "jidong-confirm" << use.card->objectName() << use.from->objectName() << card->getNumberString();
            QString prompt = prompt_list.join(":");
            use.from->tag["jidong_target"] = QVariant::fromValue(invoke->invoker);
            const Card *card = room->askForCard(use.from, pattern, prompt, data);
            if (!card)
                can = true;
        }

        if (can) {
            use.nullified_list << invoke->invoker->objectName();
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

class Zhangmu : public TriggerSkill
{
public:
    Zhangmu()
        : TriggerSkill("zhangmu")
    {
        events << TargetConfirmed << CardFinished;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->isKindOf("Slash") || use.card->isKindOf("Snatch") || use.card->isKindOf("Dismantlement")) {
            if (e == TargetConfirmed) {
                foreach (ServerPlayer *p, use.to) {
                    if (p->hasSkill(this))
                        d << SkillInvokeDetail(this, p, p);
                }
            }
            if (e == CardFinished) {
                foreach (ServerPlayer *p, use.to) {
                    QString flag = "zhangmu_" + p->objectName();
                    if (use.card->hasFlag(flag) && !p->getPile("zhang").isEmpty())
                        d << SkillInvokeDetail(this, p, p, NULL, true);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent e, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            QString prompt = "target:" + use.from->objectName() + ":" + use.card->objectName();
            invoke->invoker->tag["zhangmu"] = data;
            return invoke->invoker->askForSkillInvoke(objectName(), prompt);
        }
        return true;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == TargetConfirmed) {
            if (invoke->invoker->getHandcardNum() < 3)
                invoke->invoker->drawCards(3 - invoke->invoker->getHandcardNum());
            int num = qMin(2, invoke->invoker->getHandcardNum());
            if (num > 0) {
                const Card *cards = room->askForExchange(invoke->invoker, objectName(), num, num, false, "zhangmu_exchange:" + QString::number(num));
                DELETE_OVER_SCOPE(const Card, cards)

                invoke->invoker->addToPile("zhang", cards, false);
                CardUseStruct use = data.value<CardUseStruct>();
                room->setCardFlag(use.card, "zhangmu_" + invoke->invoker->objectName());
            }
        }
        if (e == CardFinished) {
            if (invoke->invoker->isKongcheng()) {
                CardsMoveStruct move;
                move.card_ids = invoke->invoker->getPile("zhang");
                move.to_place = Player::PlaceHand;
                move.to = invoke->invoker;
                room->moveCardsAtomic(move, false);
                room->touhouLogmessage("#zhangmu_return", invoke->invoker, "zhang", QList<ServerPlayer *>(), QString::number(move.card_ids.length()));
            } else {
                DummyCard dummy;
                dummy.addSubcards(invoke->invoker->getPile("zhang"));
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
                room->throwCard(&dummy, reason, NULL);
            }
        }
        return false;
    }
};

class Liyou : public TriggerSkill
{
public:
    Liyou()
        : TriggerSkill("liyou")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (!player->hasSkill(this) || player->isDead() || player->getPhase() != Player::Start)
            return QList<SkillInvokeDetail>();

        QList<ServerPlayer *> targets;
        Snatch *snatch = new Snatch(Card::NoSuit, 0);
        snatch->setSkillName("_liyou");
        snatch->deleteLater();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (!p->isCardLimited(snatch, Card::MethodUse) && !p->isProhibited(p, snatch) && snatch->targetFilter(QList<const Player *>(), player, p))
                targets << p;
        }
        if (targets.isEmpty())
            return QList<SkillInvokeDetail>();

        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, targets);
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, invoke->targets, objectName(), "@liyou", true, true);
        if (target != NULL)
            invoke->targets << target;
        return target != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        Snatch *snatch = new Snatch(Card::NoSuit, 0);
        snatch->setSkillName("_liyou");
        room->useCard(CardUseStruct(snatch, invoke->targets.last(), invoke->invoker), false);
        return false;
    }
};

class LiyouDistance : public TargetModSkill
{
public:
    LiyouDistance()
        : TargetModSkill("liyou-dist")
    {
        pattern = "Snatch";
    }

    int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->getSkillName() == "liyou")
            return 1000;

        return 0;
    }
};

class Sixiang : public TriggerSkill
{
public:
    Sixiang()
        : TriggerSkill("sixiang")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->getPhase() == Player::Start) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (!p->isChained())
                    d << SkillInvokeDetail(this, p, p, NULL, true);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->notifySkillInvoked(invoke->owner, objectName());
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = invoke->owner;
        log.arg = objectName();
        room->sendLog(log);

        room->recover(invoke->invoker, RecoverStruct());
        room->setPlayerProperty(invoke->invoker, "chained", true);
        return false;
    }
};

class Daoyao : public TriggerSkill
{
public:
    Daoyao()
        : TriggerSkill("daoyao")
    {
        events << HpRecover;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &) const
    {
        QList<SkillInvokeDetail> d;
        ServerPlayer *current = room->getCurrent();
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (current && current->canDiscard(current, "hes"))
                d << SkillInvokeDetail(this, p, p);
            else {
                foreach (ServerPlayer *t, room->getOtherPlayers(p)) {
                    if (t->isChained() == p->isChained()) {
                        d << SkillInvokeDetail(this, p, p);
                        break;
                    }
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *t, room->getAlivePlayers()) {
            if ((t->isChained() == invoke->invoker->isChained() && t != invoke->invoker) || (t->isCurrent() && t->canDiscard(t, "hes")))
                targets << t;
        }
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@daoyao", true, true);
        if (target)
            invoke->targets << target;
        return target != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QStringList choices;
        ServerPlayer *target = invoke->targets.first();
        if (target->isCurrent() && target->canDiscard(target, "hes"))
            choices << "discard";
        if (target != invoke->invoker && target->isChained() == invoke->invoker->isChained())
            choices << "draw";
        invoke->invoker->tag["daoyao-target"] = QVariant::fromValue(target);
        QString choice = room->askForChoice(invoke->invoker, objectName(), choices.join("+"), data);
        if (choice == "discard")
            room->askForDiscard(target, objectName(), 1, 1, false, true, "daoyao_discard:" + invoke->invoker->objectName());
        else
            target->drawCards(1);

        return false;
    }
};

TH99Package::TH99Package()
    : Package("th99")
{
    General *akyuu = new General(this, "akyuu$", "wai", 3);
    akyuu->addSkill(new Qiuwen);
    akyuu->addSkill(new Zaozu);
    akyuu->addSkill(new Dangjia);

    General *rinnosuke = new General(this, "rinnosuke", "wai", 4, true);
    rinnosuke->addSkill(new XiufuDebug);

    General *tokiko = new General(this, "tokiko", "wai", 3);
    tokiko->addSkill(new Fandu);
    tokiko->addSkill(new Taohuan);

    General *renko = new General(this, "renko", "wai", 4);
    renko->addSkill(new Shitu);

    General *merry = new General(this, "merry", "wai", 4);
    merry->addSkill("jingjie");
    merry->addSkill(new Mengxian);
    merry->addRelateSkill("luanying");

    General *toyohime = new General(this, "toyohime", "wai", 3);
    toyohime->addSkill(new Lianxi);
    toyohime->addSkill(new Yueshi);

    General *yorihime = new General(this, "yorihime", "wai", 4);
    yorihime->addSkill(new Pingyi);
    yorihime->addSkill(new PingyiHandler);
    related_skills.insertMulti("pingyi", "#pingyi_handle");

    General *sunny = new General(this, "sunny", "wai", 4);
    sunny->addSkill(new Zheshe);
    sunny->addSkill(new Tanchi);

    General *lunar = new General(this, "lunar", "wai", 4);
    lunar->addSkill(new Zhuonong);
    lunar->addSkill(new Jijing);

    General *star = new General(this, "star", "wai", 4);
    star->addSkill(new Ganying);

    General *kasen = new General(this, "kasen", "wai", 4);
    kasen->addSkill(new Zhujiu);
    kasen->addSkill(new Yushou);

    General *kosuzu = new General(this, "kosuzu", "wai", 3);
    kosuzu->addSkill(new Pandu);
    kosuzu->addSkill(new Bihuo);
    kosuzu->addSkill(new BihuoReturn);
    related_skills.insertMulti("bihuo", "#bihuo");

    General *cirno = new General(this, "cirno_sp", "wai", 3);
    cirno->addSkill(new Xunshi);
    cirno->addSkill(new Jidong);

    General *mamizou = new General(this, "mamizou_sp", "wai", 3);
    mamizou->addSkill(new Zhangmu);
    mamizou->addSkill(new Liyou);

    General *reisen2 = new General(this, "reisen2", "wai", 4);
    reisen2->addSkill(new Sixiang);
    reisen2->addSkill(new Daoyao);

    addMetaObject<QiuwenCard>();
    addMetaObject<DangjiaCard>();
    addMetaObject<XiufuCard>();
    addMetaObject<XiufuMoveCard>();
    addMetaObject<LianxiCard>();
    addMetaObject<ZhesheCard>();
    addMetaObject<ZhuonongCard>();
    addMetaObject<YushouCard>();
    addMetaObject<PanduCard>();
    skills << new DangjiaVS << new Luanying << new GanyingHandler << new XiufuMove << new XunshiDistance << new LiyouDistance;
}

ADD_PACKAGE(TH99)
