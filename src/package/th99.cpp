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

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("QiuwenCard");
    }

    const Card *viewAs(const Player * /*Self*/) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->isAlive() && change.player->hasSkill(this) && change.to == Player::Discard)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, nullptr, true);
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *aq = data.value<ServerPlayer *>();
            if (aq->isAlive() && aq->hasSkill(this) && aq->getPhase() == Player::Finish && aq->getHandcardNum() > aq->getMaxHp())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, aq, aq, nullptr, true);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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
    if (!source->pindian(akyu, "dangjia", nullptr)) {
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

    bool isEnabledAtPlay(const Player *player) const override
    {
        if (player->isKongcheng() || !shouldBeVisible(player))
            return false;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasLordSkill("dangjia") && !p->hasFlag("dangjiaInvoked"))
                return true;
        }
        return false;
    }

    bool shouldBeVisible(const Player *Self) const override
    {
        return Self && Self->getKingdom() == "wai";
    }

    const Card *viewAs(const Player * /*Self*/) const override
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
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
        if (room->getCard(id)->isKindOf("EquipCard"))
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
            if (room->getCard(id)->isKindOf("EquipCard"))
                equips << id;
        }

        int id = room->askForAG(mori, equips, true, "xiufu");
        if (id == -1)
            return;

        ServerPlayer *target = room->askForPlayerChosen(mori, room->getAllPlayers(), "xiufu", QString(), true);
        if (target == nullptr)
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

    const EquipCard *equip = qobject_cast<const EquipCard *>(room->getCard(xiufu_id)->getRealCard());
    ServerPlayer *target = mori->tag.value("xiufu_to", QVariant::fromValue((ServerPlayer *)nullptr)).value<ServerPlayer *>();

    int equipped_id = -1;
    if (target->getEquip(equip->location()) != nullptr)
        equipped_id = target->getEquip(equip->location())->getEffectiveId();
    QList<CardsMoveStruct> exchangeMove;
    CardsMoveStruct move1(equip->getId(), target, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_TRANSFER, mori->objectName()));
    exchangeMove.push_back(move1);
    //since new equip could not move into gaoao's area, the equipped one will maintain.
    if (equipped_id != -1 && !target->hasSkill("gaoao")) {
        CardsMoveStruct move2(equipped_id, nullptr, Player::DiscardPile, CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, target->objectName()));
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

    bool viewFilter(const Card *to_select, const Player *Self) const override
    {
        return Self->getPile("#xiufu_temp").contains(to_select->getId());
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
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

    bool isEnabledAtPlay(const Player *player) const override
    {
        return (!player->hasFlag("Global_xiufuFailed") && !player->hasFlag("xiufu_used")) || player->getMark("@xiufudebug") > 0;
    }

    const Card *viewAs(const Player * /*Self*/) const override
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

    void record(TriggerEvent e, Room *room, QVariant &data) const override
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
                room->moveCardTo(&dummy, nullptr, Player::DiscardPile);
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!move.from_places.contains(Player::PlaceHand) && !move.from_places.contains(Player::PlaceEquip))
                return QList<SkillInvokeDetail>();
            ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
            if (from == nullptr || from->isDead() || !from->hasSkill(this) || from->isKongcheng())
                return QList<SkillInvokeDetail>();

            // move from is now certain, we can now only check the card movement case.
            if (move.to_place == Player::DiscardPile) {
                // cae 1: discard
                if (move.reason.m_reason != CardMoveReason::S_REASON_DISMANTLE)
                    return QList<SkillInvokeDetail>();

                ServerPlayer *thrower = room->findPlayerByObjectName(move.reason.m_playerId);
                if (thrower == nullptr || thrower->isDead() || thrower->isKongcheng() || from == thrower)
                    return QList<SkillInvokeDetail>();

                QList<SkillInvokeDetail> d;
                int i = 0;
                foreach (int id, move.card_ids) {
                    const Card *c = room->getCard(id);
                    Player::Place from_place = move.from_places.value(i++);
                    if (c != nullptr && (c->isKindOf("EquipCard") || c->isKindOf("TrickCard")) && (from_place == Player::PlaceHand || from_place == Player::PlaceEquip))
                        d << SkillInvokeDetail(this, from, from, nullptr, false, thrower);
                }
                return d;
            } else if (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip) {
                ServerPlayer *obtainer = qobject_cast<ServerPlayer *>(move.to);
                if (obtainer == nullptr || obtainer->isDead() || obtainer == from || obtainer->isKongcheng())
                    return QList<SkillInvokeDetail>();

                QList<SkillInvokeDetail> d;
                for (int i = 0; i < move.card_ids.length(); ++i) {
                    Player::Place from_place = move.from_places.value(i);
                    if (from_place == Player::PlaceHand || from_place == Player::PlaceEquip)
                        d << SkillInvokeDetail(this, from, from, nullptr, false, obtainer);
                }
                return d;
            }
        } else {
            PindianStruct *pindian = data.value<PindianStruct *>();
            if (pindian->isSuccess() && !pindian->to->isNude() && pindian->reason == objectName())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, pindian->from, pindian->from, nullptr, true, pindian->to);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == Pindian)
            return true;
        else {
            return invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget));
        }
        return false;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (e == Pindian) {
            int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hes", objectName());
            room->obtainCard(invoke->invoker, id, false);
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

    bool canPreshow() const override
    {
        return true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getAllPlayers(), objectName(), "@shitu-playerchoose", true, true);
        if (target != nullptr) {
            invoke->targets << target;
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *merry = data.value<ServerPlayer *>();
        if (merry->hasSkill(this) && merry->getPhase() == Player::Start && merry->isAlive() && merry->getPile("jingjie").length() >= 3 && merry->getMark("mengxian") == 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, merry, merry, nullptr, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    bool viewFilter(const Card *to_select, const Player *Self) const override
    {
        if (!Self->getPile("jingjie").contains(to_select->getId()))
            return false;

        QString property = Self->property("luanying").toString();
        if (property == "black")
            return to_select->isBlack();
        else
            return to_select->isRed();
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
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

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *user = nullptr;
        const Card *card = nullptr;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
            user = use.from;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse) {
                card = resp.m_card;
                user = resp.m_from;
            }
        }

        if (user == nullptr || card == nullptr)
            return QList<SkillInvokeDetail>();

        if (card->isKindOf("BasicCard")) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, room->getOtherPlayers(user)) {
                if (p->hasSkill(this) && p->isAlive()) {
                    QList<int> jingjie = p->getPile("jingjie");
                    bool flag = false;
                    foreach (int id, jingjie) {
                        if (room->getCard(id)->getColor() == card->getColor()) {
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

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *user = nullptr;
        const Card *card = nullptr;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
            user = use.from;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            card = resp.m_card;
            user = resp.m_from;
        }

        if (user == nullptr || card == nullptr)
            return false;

        if (card->isRed()) {
            invoke->invoker->setProperty("luanying", "red");
            room->notifyProperty(invoke->invoker, invoke->invoker, "luanying");
        } else {
            invoke->invoker->setProperty("luanying", "black");
            room->notifyProperty(invoke->invoker, invoke->invoker, "luanying");
        }

        QString prompt = "@luanying-invoke:" + user->objectName() + ":" + card->objectName();
        const Card *c = room->askForCard(invoke->invoker, "@@luanying", prompt, data, Card::MethodNone, nullptr, false, "luanying");

        if (c != nullptr) {
            room->obtainCard(user, c, true);
            room->touhouLogmessage("#weiya", user, objectName(), QList<ServerPlayer *>(), card->objectName());

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
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

bool LianxiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    IronChain *card = new IronChain(Card::NoSuit, 0);
    card->setSkillName("lianxi");
    card->deleteLater();
    int total_num = 2 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, card);
    return targets.length() <= total_num;
}

const Card *LianxiCard::validate(CardUseStruct &card_use) const
{
    card_use.from->showHiddenSkill("lianxi");
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

    const Card *viewAs(const Player * /*Self*/) const override
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

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *user = nullptr;
        const Card *card = nullptr;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
            user = use.from;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            card = resp.m_card;
            user = resp.m_from;
        }

        if (user != nullptr && user->hasSkill(this) && user->isAlive() && card != nullptr && card->isKindOf("Slash"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, user, user);

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *toyo = data.value<ServerPlayer *>();
        if (toyo->hasSkill(this) && toyo->getPhase() == Player::Start && toyo->isAlive() && toyo->isDebuffStatus() && toyo->getMark("yueshi") == 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, toyo, toyo, nullptr, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    void record(TriggerEvent e, Room *room, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        if (e == EventPhaseChanging)
            return QList<SkillInvokeDetail>();
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *yori = (e == Damage) ? damage.from : damage.to;
        ServerPlayer *target = (e == Damage) ? damage.to : damage.from;

        if (yori == nullptr || yori->isDead() || !yori->hasSkill(this) || yori->hasFlag("pingyi_used") || target == nullptr || target->isDead() || yori == target)
            return QList<SkillInvokeDetail>();

        foreach (const Skill *skill, target->getVisibleSkillList()) {
            if (skill->isLordSkill() || skill->isAttachedLordSkill() || skill->getFrequency() == Skill::Limited || skill->getFrequency() == Skill::Wake
                || skill->getFrequency() == Skill::Eternal)
                continue;
            if (!yori->hasSkill(skill, true) && target->hasSkill(skill))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, yori, yori, nullptr, false, target);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    static void skillProcess(Room *room, ServerPlayer *yori, ServerPlayer *skill_owner = nullptr, const Skill *skill = nullptr)
    {
        JsonArray arg;
        arg << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
        arg << yori->objectName();
        if (skill_owner == nullptr || skill == nullptr) {
            arg << QString() << QString() << QString() << QString();
        } else {
            if (yori->inHeadSkills("pingyi")) {
                arg << skill_owner->getGeneral()->objectName();
                arg << skill->objectName();
            }
            arg << QString() << QString();
            if (!yori->inHeadSkills("pingyi")) {
                arg << skill_owner->getGeneral()->objectName();
                arg << skill->objectName();
            }
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

        if (skill != nullptr && skill_owner != nullptr) {
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who->tag.contains("pingyi_skill"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, death.who, death.who, nullptr, true);
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.to == Player::NotActive) {
                ServerPlayer *yori = phase_change.player;
                if (yori != nullptr) {
                    ServerPlayer *who = yori->tag.value("pingyi_originalOwner").value<ServerPlayer *>();
                    if (who != nullptr)
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, yori, yori, nullptr, true);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
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

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageInflicted) {
            if (damage.to->hasSkill(this) && damage.to->isAlive() && !damage.to->isKongcheng() && damage.to != damage.from)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        } else if (triggerEvent == DamageComplete) {
            if (damage.to->isAlive() && damage.to->getMark("zheshetransfer") > 0 && damage.reason == "zheshe")
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, true, damage.to);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == DamageInflicted) {
            if (room->askForUseCard(invoke->invoker, "@@zheshe", "@zheshe", -1, Card::MethodDiscard) && invoke->invoker->tag.contains("zheshe_target")) {
                ServerPlayer *target = invoke->invoker->tag.value("zheshe_target", QVariant::fromValue((ServerPlayer *)nullptr)).value<ServerPlayer *>();
                if (target != nullptr) {
                    invoke->targets << target;
                    return true;
                }
            }
        } else if (triggerEvent == DamageComplete) {
            invoke->preferredTarget->removeMark("zheshetransfer");
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == DamageInflicted) {
            invoke->targets.first()->addMark("zheshetransfer");

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

class Zhuxi : public TriggerSkill
{
public:
    Zhuxi()
        : TriggerSkill("zhuxi")
    {
        events << HpRecover;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        RecoverStruct recover = data.value<RecoverStruct>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            d << SkillInvokeDetail(this, p, p, recover.to, false, recover.to);
        }
        return d;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->targets.first()->drawCards(1, objectName());
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

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("ZhuonongCard");
    }

    const Card *viewAs(const Player * /*Self*/) const override
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
        events << Damage << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark("@jijing") > 0) {
                        p->loseAllMarks("@jijing");
                        room->setFixedDistance(change.player, p, -1);
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e == EventPhaseChanging)
            return QList<SkillInvokeDetail>();

        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from == nullptr || room->getCurrent() == nullptr || room->getCurrent()->isDead())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *lunar, room->findPlayersBySkillName(objectName())) {
            if (lunar->isCurrent() && lunar == damage.from)
                d << SkillInvokeDetail(this, lunar, lunar);
            else if (damage.from->isCurrent() && lunar != damage.from)
                d << SkillInvokeDetail(this, lunar, lunar);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QString prompt = invoke->invoker->isCurrent() ? "@jijing-41" : ("@jijing-42:" + room->getCurrent()->objectName());
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(room->getCurrent()), objectName(), prompt, true, true);
        if (target != nullptr)
            invoke->targets << target;
        return !invoke->targets.isEmpty();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        foreach (ServerPlayer *p, invoke->targets) {
            room->setFixedDistance(room->getCurrent(), p, (invoke->invoker->getPhase() == Player::NotActive ? 1000 : 1));
            p->gainMark("@jijing");
        }

        return false;
    }
};

class Ganying : public TriggerSkill
{
public:
    Ganying()
        : TriggerSkill("ganying")
    {
        events << Damaged << HpRecover;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (e == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->isDead())
                return QList<SkillInvokeDetail>();
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p != damage.to && damage.to->getHp() == p->getHp())
                    d << SkillInvokeDetail(this, p, p, nullptr, false, damage.to);
            }
        }

        if (e == HpRecover) {
            RecoverStruct recover = data.value<RecoverStruct>();
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p != recover.to && recover.to->getHp() == p->getHp())
                    d << SkillInvokeDetail(this, p, p, nullptr, false, recover.to);
            }
        }

        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->drawCards(1, objectName());
        if (invoke->invoker->canDiscard(invoke->targets.first(), "hs")) {
            ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, invoke->targets, "ganying", QString("@ganying:%1").arg(invoke->targets.first()->objectName()), true);
            if (target == nullptr)
                return false;
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), target->objectName());
            int id = room->askForCardChosen(invoke->invoker, target, "hs", objectName(), false, Card::MethodDiscard);
            room->throwCard(id, target, invoke->invoker == target ? nullptr : invoke->invoker);
        }
        return false;
    }
};

class Dubi : public TriggerSkill
{
public:
    Dubi()
        : TriggerSkill("dubi")
    {
        events << TargetSpecifying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.from != nullptr && use.card != nullptr && use.card->getTypeId() == Card::TypeTrick && !use.to.isEmpty()) {
            foreach (ServerPlayer *p, use.to) {
                if (p->hasSkill(this) && p != use.from)
                    d << SkillInvokeDetail(this, p, p);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QStringList prompt_list;
        prompt_list << "@dubi-cancel" << use.from->objectName() << use.card->objectName();
        QString prompt = prompt_list.join(":");
        invoke->invoker->tag["dubi_use"] = data; //for ai
        ServerPlayer *p = room->askForPlayerChosen(invoke->invoker, use.to, objectName(), prompt, true, true);
        if (p != nullptr) {
            room->loseHp(invoke->invoker);
            invoke->targets << p;
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.to.removeOne(invoke->targets.first());
        room->sortByActionOrder(use.to);
        data = QVariant::fromValue<CardUseStruct>(use);

        LogMessage l;
        l.type = "#XushiHegemonySkillAvoid";
        l.from = invoke->targets.first();
        l.arg = objectName();
        l.arg2 = use.card->objectName();

        room->sendLog(l);

        return false;
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        invoke->owner->tag["zhujiu_target"] = QVariant::fromValue(invoke->invoker);
        const Card *c = room->askForCard(invoke->owner, "..", "@zhujiu:" + invoke->invoker->objectName(), data, Card::MethodNone);
        if (c) {
            invoke->owner->showHiddenSkill(objectName());
            CardMoveReason r(CardMoveReason::S_REASON_GIVE, invoke->owner->objectName(), objectName(), QString());
            room->obtainCard(invoke->invoker, c, r, false);

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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
    const Card *card = room->getCard(card_id);
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

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("YushouCard");
    }

    const Card *viewAs(const Player * /*Self*/) const override
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
    Card *showcard = room->getCard(card_id);
    if (showcard->isKindOf("Slash")) {
        if (!target->isCardLimited(showcard, Card::MethodUse)) {
            room->setCardFlag(showcard, "chosenExtraSlashTarget");
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

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("PanduCard");
    }

    const Card *viewAs(const Player * /*Self*/) const override
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerMark(p, objectName(), 0);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
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
                    if (kos == nullptr || kos->isDead())
                        continue;

                    SkillInvokeDetail s(this, kos, p, nullptr, true);
                    d << s;
                }
            }
            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->isKindOf("Peach") || use.card->isKindOf("Slash") || use.card->isNDTrick()) {
            // Fs: when modifiying this skill, check skill "GakungWu" (Guwu & Kuangwu) in th16
            use.card->setFlags("xunshi");
            use.card->setFlags("IgnoreFailed");
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (use.from->isAlive() && p != use.from && !use.to.contains(p) && !use.to.isEmpty()
                    && (p->getHandcardNum() < use.from->getHandcardNum() || p->getHp() < use.from->getHp()) && !use.from->isProhibited(p, use.card)) {
                    if (use.card->isKindOf("Peach")) {
                        if (p->isWounded())
                            d << SkillInvokeDetail(this, p, p, nullptr, true);
                    } else if (use.card->targetFilter(QList<const Player *>(), p, use.from))
                        d << SkillInvokeDetail(this, p, p, nullptr, true);
                }
            }
            use.card->setFlags("-IgnoreFailed");
            use.card->setFlags("-xunshi");
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();

        room->notifySkillInvoked(invoke->invoker, objectName());
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), use.from->objectName());
        QList<ServerPlayer *> logto;
        logto << invoke->invoker;
        room->touhouLogmessage("#xunshi", use.from, use.card->objectName(), logto, objectName());

        invoke->invoker->drawCards(1);
        use.to << invoke->invoker;
        room->sortByActionOrder(use.to);
        data = QVariant::fromValue(use);

        return false;
    }
};

// Fs: when modifiying this skill, check skill "GakungWu" (Guwu & Kuangwu) in th16
class XunshiDistance : public TargetModSkill
{
public:
    XunshiDistance()
        : TargetModSkill("xunshi-dist")
    {
        pattern = "Slash,TrickCard+^DelayedTrick";
    }

    int getDistanceLimit(const Player *, const Card *card) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->isKindOf("Slash") || use.card->isKindOf("Duel")) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (use.from->isAlive() && p != use.from && use.to.contains(p) && p->canDiscard(p, "hs"))
                    d << SkillInvokeDetail(this, p, p, nullptr);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QStringList prompt_list;
        prompt_list << "jidong-discard" << use.card->objectName() << use.from->objectName() << "basic";
        QString prompt = prompt_list.join(":");

        const Card *card = room->askForCard(invoke->invoker, ".Basic", prompt, data, Card::MethodDiscard, nullptr, false, objectName());
        if (card)
            invoke->invoker->tag["jidong_card"] = QVariant::fromValue(card);
        return card != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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
        related_pile = "zhang";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
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
                        d << SkillInvokeDetail(this, p, p, nullptr, true);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent e, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (e == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            QString prompt = "target:" + use.from->objectName() + ":" + use.card->objectName();
            invoke->invoker->tag["zhangmu"] = data;
            return invoke->invoker->askForSkillInvoke(objectName(), prompt);
        }
        return true;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", nullptr, objectName(), "");
                room->throwCard(&dummy, reason, nullptr);
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, invoke->targets, objectName(), "@liyou", true, true);
        if (target != nullptr)
            invoke->targets << target;
        return target != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    int getDistanceLimit(const Player *, const Card *card) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->getPhase() == Player::Start) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (!p->isChained())
                    d << SkillInvokeDetail(this, p, p, nullptr, true);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &) const override
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *t, room->getAlivePlayers()) {
            if ((t->isChained() == invoke->invoker->isChained() && t != invoke->invoker) || (t->isCurrent() && t->canDiscard(t, "hes")))
                targets << t;
        }
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@daoyao", true, true);
        if (target)
            invoke->targets << target;
        return target != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QStringList choices;
        ServerPlayer *target = invoke->targets.first();
        if (target->isCurrent() && target->canDiscard(target, "hes"))
            choices << "discard";
        if (target != invoke->invoker && target->isChained() == invoke->invoker->isChained())
            choices << "draw";
        QString choice = room->askForChoice(invoke->invoker, objectName(), choices.join("+"), QVariant::fromValue(target));
        if (choice == "discard")
            room->askForDiscard(target, objectName(), 1, 1, false, true, "daoyao_discard:" + invoke->invoker->objectName());
        else
            target->drawCards(1);

        return false;
    }
};

ZhuozhiCard::ZhuozhiCard()
{
    target_fixed = true;
    will_throw = true;
}

void ZhuozhiCard ::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QList<int> ids = room->getNCards(4);

    CardsMoveStruct move(ids, nullptr, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, source->objectName(), "zhuozhi", QString()));
    room->moveCardsAtomic(move, true);
    room->fillAG(ids);

    int n = 1;
    foreach (int id, ids) {
        if (room->getCard(id)->isKindOf("Slash") || room->getCard(id)->getTypeId() == Card::TypeEquip) {
            n = 2;
            break;
        }
    }

    QList<int> obtain_ids;
    while (n--) {
        int id = room->askForAG(source, ids, false, "zhuozhi");
        ids.removeOne(id);
        obtain_ids << id;
        room->takeAG(source, id, false, QList<ServerPlayer *>(), Player::PlaceTable);
    }

    room->clearAG();

    DummyCard obtainDummy;
    obtainDummy.addSubcards(obtain_ids);
    room->obtainCard(source, &obtainDummy);

    DummyCard discardDummy;
    discardDummy.addSubcards(ids);
    room->throwCard(&discardDummy, nullptr);
}

class Zhuozhi : public OneCardViewAsSkill
{
public:
    Zhuozhi()
        : OneCardViewAsSkill("zhuozhi")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("ZhuozhiCard");
    }

    bool viewFilter(const Card *to_select, const Player *Self) const override
    {
        return !to_select->isEquipped(Self);
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
    {
        ZhuozhiCard *c = new ZhuozhiCard;
        c->addSubcard(originalCard);
        return c;
    }
};

WanshenCard::WanshenCard()
{
    target_fixed = true;
}

void WanshenCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->doLightbox("$WanshenAnimate", 4000);
    room->setPlayerMark(source, "@kazenwanshen", 0);

    if (room->changeMaxHpForAwakenSkill(source)) {
        source->gainAnExtraTurn();
        room->handleAcquireDetachSkills(source, "xieli");
    }
}

class Wanshen : public ZeroCardViewAsSkill
{
public:
    Wanshen()
        : ZeroCardViewAsSkill("wanshen")
    {
        frequency = Limited;
        limit_mark = "@kazenwanshen";
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return player->getMark("@kazenwanshen") > 0 && player->getEquips().count() > 2;
    }

    const Card *viewAs(const Player * /*Self*/) const override
    {
        return new WanshenCard;
    }
};

XieliCard::XieliCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void XieliCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->addBrokenEquips(subcards);

    Analeptic *ana = new Analeptic(Card::NoSuit, -1);
    ana->setFlags("Add_History");
    room->useCard(CardUseStruct(ana, source));
}

class Xieli : public OneCardViewAsSkill
{
public:
    Xieli()
        : OneCardViewAsSkill("xieli")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        if (!Analeptic::IsAvailable(player))
            return false;

        foreach (const Card *card, player->getEquips()) {
            if (!player->isBrokenEquip(card->getId()))
                return true;
        }

        return false;
    }

    bool viewFilter(const Card *to_select, const Player *Self) const override
    {
        return to_select->isEquipped(Self) && !Self->isBrokenEquip(to_select->getId());
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
    {
        XieliCard *c = new XieliCard;
        c->addSubcard(originalCard);
        return c;
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
    related_skills.insert("pingyi", "#pingyi_handle");

    General *sunny = new General(this, "sunny", "wai", 3);
    sunny->addSkill(new Zheshe);
    sunny->addSkill(new Zhuxi);

    General *lunar = new General(this, "lunar", "wai", 3);
    lunar->addSkill(new Zhuonong);
    lunar->addSkill(new Jijing);

    General *star = new General(this, "star", "wai", 3);
    star->addSkill(new Ganying);
    star->addSkill(new Dubi);

    General *kasen = new General(this, "kasen", "wai", 4);
    kasen->addSkill(new Zhujiu);
    kasen->addSkill(new Yushou);

    General *kosuzu = new General(this, "kosuzu", "wai", 3);
    kosuzu->addSkill(new Pandu);
    kosuzu->addSkill(new Bihuo);
    kosuzu->addSkill(new BihuoReturn);
    related_skills.insert("bihuo", "#bihuo");

    General *cirno = new General(this, "cirno_sp", "wai", 3);
    cirno->addSkill(new Xunshi);
    cirno->addSkill(new Jidong);

    General *mamizou = new General(this, "mamizou_sp", "wai", 3);
    mamizou->addSkill(new Zhangmu);
    mamizou->addSkill(new Liyou);

    General *reisen2 = new General(this, "reisen2", "wai", 4);
    reisen2->addSkill(new Sixiang);
    reisen2->addSkill(new Daoyao);

    General *kasensp = new General(this, "kasen_sp", "wai", 5);
    kasensp->addSkill(new Zhuozhi);
    kasensp->addSkill(new Wanshen);
    kasensp->addRelateSkill("xieli");

    General *miyoi = new General(this, "miyoi", "wai", 4, false, true, true);
    Q_UNUSED(miyoi);

    addMetaObject<QiuwenCard>();
    addMetaObject<DangjiaCard>();
    addMetaObject<XiufuCard>();
    addMetaObject<XiufuMoveCard>();
    addMetaObject<LianxiCard>();
    addMetaObject<ZhesheCard>();
    addMetaObject<ZhuonongCard>();
    addMetaObject<YushouCard>();
    addMetaObject<PanduCard>();
    addMetaObject<ZhuozhiCard>();
    addMetaObject<WanshenCard>();
    addMetaObject<XieliCard>();

    skills << new DangjiaVS << new Luanying << new XiufuMove << new XunshiDistance << new LiyouDistance << new Xieli; //<< new GanyingHandler
}

ADD_PACKAGE(TH99)
