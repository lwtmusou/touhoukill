#include "th99.h"
#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"
#include "maneuvering.h"



QiuwenCard::QiuwenCard()
{
    mute = true;
    target_fixed = true;
}
void QiuwenCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->drawCards(source, 3);
}
class Qiuwen : public ZeroCardViewAsSkill
{
public:
    Qiuwen() : ZeroCardViewAsSkill("qiuwen")
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
    Zaozu() : TriggerSkill("zaozu")
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
    mute = true;
}
bool DangjiaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasLordSkill("dangjia")
        && to_select != Self && !to_select->hasFlag("dangjiaInvoked")
        && !to_select->isKongcheng() && to_select->isWounded();
}
void DangjiaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *akyu = targets.first();
    if (akyu->hasLordSkill("dangjia")) {
        room->setPlayerFlag(akyu, "dangjiaInvoked");

        room->notifySkillInvoked(akyu, "dangjia");
        if (!source->pindian(akyu, "dangjia", NULL)) {
            RecoverStruct recov;
            recov.who = source;
            room->recover(akyu, recov);
        }

        QList<ServerPlayer *> akyus;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach (ServerPlayer *p, players) {
            if (p->hasLordSkill("dangjia") && !p->hasFlag("dangjiaInvoked"))
                akyus << p;
        }
        if (akyus.isEmpty())
            room->setPlayerFlag(source, "Forbiddangjia");
    }
}
class DangjiaVS : public ZeroCardViewAsSkill
{
public:
    DangjiaVS() :ZeroCardViewAsSkill("dangjia_attach")
    {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return  shouldBeVisible(player) && !player->hasFlag("Forbiddangjia") && !player->isKongcheng();
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
    Dangjia() : TriggerSkill("dangjia$")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
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
            if (phase_change.player->hasFlag("Forbiddangjia"))
                room->setPlayerFlag(phase_change.player, "-Forbiddangjia");
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
    move.card_ids = equips;

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
        if (!putToPile(room, mori))
            return;

        bool used = room->askForUseCard(mori, "@@xiufumove", "@xiufu-move", -1, Card::MethodNone, true, "xiufu");

        cleanUp(room, mori);
        if (!used)
            return;
    } else {
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
    if (equipped_id != -1) {
        CardsMoveStruct move2(equipped_id, NULL, Player::DiscardPile, CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, target->objectName()));
        exchangeMove.push_back(move2);
    }

    LogMessage zhijian;
    zhijian.type = "$ZhijianEquip";
    zhijian.from = target;
    zhijian.card_str = QString::number(equip->getId());
    room->sendLog(zhijian);

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
    XiufuMove() : OneCardViewAsSkill("xiufumove")
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
    Xiufu() : ZeroCardViewAsSkill("xiufu")
    {

    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("XiufuCard") || player->getMark("@xiufudebug") > 0;
    }

    const Card *viewAs() const
    {
        return new XiufuCard;
    }
};

class XiufuDebug : public TriggerSkill
{
public:
    XiufuDebug() : TriggerSkill("xiufu")
    {
        events << MarkChanged;
        view_as_skill = new Xiufu;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const
    {
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
};

class Fandu : public TriggerSkill
{
public:
    Fandu() : TriggerSkill("fandu")
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
            if (p->canDiscard(invoke->invoker, "h"))
                listt << p;
        }
        if (listt.isEmpty())
            return false;
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, listt, "fandu", "@fandu-select", false, true);
        int to_throw = room->askForCardChosen(target, invoke->invoker, "h", "fandu", false, Card::MethodDiscard);
        room->throwCard(to_throw, invoke->invoker, target);

        return false;
    }
};

#pragma message WARN("todo_fs: split ServerPlayer::pindian to 2 parts")

class Taohuan : public TriggerSkill
{
public:
    Taohuan() : TriggerSkill("taohuan")
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
                if (thrower == NULL || thrower->isDead() || thrower->isKongcheng())
                    return QList<SkillInvokeDetail>();

                QList<SkillInvokeDetail> d;
                int i = 0;
                foreach (int id, move.card_ids) {
                    const Card *c = Sanguosha->getCard(id);
                    Player::Place from_place = move.from_places.value(i++);
                    if (c != NULL && (c->isKindOf("EquipCard") || c->isKindOf("TrickCard") && (from_place == Player::PlaceHand || from_place == Player::PlaceEquip)))
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
            if (invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget)))
                invoke->invoker->pindian(invoke->preferredTarget, objectName());
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "he", objectName());
        room->obtainCard(invoke->invoker, id, room->getCardPlace(id) != Player::PlaceHand);

        return false;
    }
};


class Shitu : public TriggerSkill
{
public:
    Shitu() : TriggerSkill("shitu")
    {
        events << EventPhaseChanging << DamageDone << Death << TurnStart;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const
    {
        switch (triggerEvent) {
            case DamageDone:
            case Death:
                room->setTag("shituDamageOrDeath", true);
                break;
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
        invoke->targets.first()->setMark("shituPhase", 1);
        invoke->targets.first()->gainAnExtraTurn();

        return false;
    }
};

class Mengxian : public TriggerSkill
{
public:
    Mengxian() : TriggerSkill("mengxian")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }
    
    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *merry = data.value<ServerPlayer *>();
        if (merry->hasSkill(this) && merry->getPhase() == Player::Start && merry->isAlive() && merry->getPile("jingjie").length() >= 3 && !merry->getMark("mengxian") == 0)
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

class Luanying : public TriggerSkill
{
public:
    Luanying() : TriggerSkill("luanying")
    {
        events << CardUsed << CardResponded;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *user = NULL;
        const Card *card = NULL;
        bool isUse = false;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
            user = use.from;
            isUse = true;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (!resp.m_isProvision && !resp.m_isRetrial) {
                card = resp.m_card;
                user = resp.m_from;
                isUse = resp.m_isUse;
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
        bool isUse = false;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
            user = use.from;
            isUse = true;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (!resp.m_isProvision && !resp.m_isRetrial) {
                card = resp.m_card;
                user = resp.m_from;
                isUse = resp.m_isUse;
            }
        }

        if (user == NULL || card == NULL)
            return false;

        if (invoke->invoker->askForSkillInvoke(this, data)) {
            QList<int> jingjie = invoke->invoker->getPile("jingjie");
            QList<int> availables, disables;
            foreach (int id, jingjie) {
                if (Sanguosha->getCard(id)->getColor() == card->getColor())
                    availables << id;
                else
                    disables << id;
            }

            room->fillAG(jingjie, invoke->invoker, disables);
            int selected = room->askForAG(invoke->invoker, availables, false, objectName());
            room->clearAG(invoke->invoker);


            if (selected > -1) {
                room->obtainCard(user, selected, true);
                room->touhouLogmessage("#weiya", user, objectName(), QList<ServerPlayer *>(), card->objectName());

                return true;
            }
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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

class LianxiVS : public ZeroCardViewAsSkill
{
public:
    LianxiVS() :ZeroCardViewAsSkill("lianxi")
    {
        response_pattern = "@@lianxi";
    }

    virtual const Card *viewAs() const
    {
        IronChain *ic = new IronChain(Card::NoSuit, 0);
        ic->setSkillName("_lianxi");
        return ic;
    }
};
class Lianxi : public TriggerSkill
{
public:
    Lianxi() : TriggerSkill("lianxi")
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
    Yueshi() : TriggerSkill("yueshi")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }
    
    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *toyo = data.value<ServerPlayer *>();
        if (toyo->hasSkill(this) && toyo->getPhase() == Player::Start && toyo->isAlive() && toyo->isChained() && !toyo->getMark("yueshi") == 0)
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

#pragma message WARN("todo_Fs: develop a better method to deal with skill nullify, the skill Pingyi will be rewritten later")

class Pingyi : public TriggerSkill
{
public:
    Pingyi() : TriggerSkill("pingyi")
    {
        events << Damaged;
    }
    /*     virtual int getPriority(TriggerEvent) const
        {
            return -1;
        } */
    static void skill_comeback(Room *room, ServerPlayer *player)
    {
        ServerPlayer * back;
        QString back_skillname;
        foreach (ServerPlayer *p, room->getOtherPlayers(player, true)) {
            if (p->getMark("@pingyi") > 0) {
                QString pingyi_record = player->objectName() + "pingyi" + p->objectName();
                back_skillname = room->getTag(pingyi_record).toString();
                if (back_skillname != NULL && back_skillname != "") {
                    back = p;
                    room->setTag(pingyi_record, QVariant());
                    break;
                }
            }
        }

        if (back != NULL) {

            room->setPlayerMark(player, "pingyi_steal", 0);
            if (back->isAlive()) {
                //room->setPlayerMark(back, "pingyi", back->getMark("pingyi") - 1);
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), back->objectName());
            }
            room->handleAcquireDetachSkills(player, "-" + back_skillname);

            JsonArray arg;
            arg << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
            arg << player->objectName();
            arg << player->getGeneral()->objectName();
            arg << QString();
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

            if (back->isAlive()) {
                //room->handleAcquireDetachSkills(back, back_skillname);
                room->setPlayerMark(back, "pingyi" + back_skillname, 0);
                JsonArray args;
                args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
                back->loseMark("@pingyi", 1);
                if (back->hasSkill(back_skillname))
                    room->touhouLogmessage("#pingyiReturnSkill", back, back_skillname);
            }
        }

    }
/*

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive() && damage.from != player) {
            if (player->isNude())
                return QStringList();

            foreach (const Skill *skill, damage.from->getVisibleSkillList()) {
                if (skill->isLordSkill() || skill->getFrequency() == Skill::Limited
                    || skill->getFrequency() == Skill::Wake || skill->isAttachedLordSkill()
                    || skill->getFrequency() == Skill::Eternal)
                    continue;
                else {
                    //need check skill both sides
                    if (!player->hasSkill(skill->objectName(), false, true) && damage.from->hasSkill(skill->objectName()))
                        return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QStringList skill_names;
        skill_names << "cancel";

        foreach (const Skill *skill, damage.from->getVisibleSkillList()) {
            if (skill->isLordSkill() || skill->getFrequency() == Skill::Limited
                || skill->getFrequency() == Skill::Wake || skill->isAttachedLordSkill()
                || skill->getFrequency() == Skill::Eternal)
                continue;
            else {
                //need check skill both sides
                if (!player->hasSkill(skill->objectName(), false, true) && damage.from->hasSkill(skill->objectName()))
                    skill_names << skill->objectName();
            }
        }

        player->tag["pingyi_target"] = QVariant::fromValue(damage.from);

        QString skill_name = room->askForChoice(player, "pingyi", skill_names.join("+"));
        if (skill_name == "cancel")
            return false;

        const Card *card = room->askForCard(player, ".|.|.|.!", "@pingyi-discard:" + damage.from->objectName() + ":" + skill_name, data, Card::MethodDiscard, NULL, true, objectName());
        if (card != NULL) {

            if (player->getMark("pingyi_steal") > 0)
                skill_comeback(room, player);
            //record pingyi     relation and content.
            QString pingyi_record = player->objectName() + "pingyi" + damage.from->objectName();
            room->setTag(pingyi_record, skill_name);

            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());


            room->setPlayerMark(player, "pingyi_steal", 1);
            room->setPlayerMark(damage.from, "pingyi" + skill_name, 1); // skill nullify mark, like Qingcheng
            room->handleAcquireDetachSkills(player, skill_name);

            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

            damage.from->gainMark("@pingyi"); // marks could be greater than 1,since it can be stealed any times.
            room->touhouLogmessage("#pingyiLoseSkill", damage.from, skill_name);

            JsonArray arg;
            arg << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
            arg << player->objectName();
            arg << damage.from->getGeneral()->objectName();
            arg << skill_name;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
        }

        return false;
    }*/
};
class PingyiHandler : public TriggerSkill
{
public:
    PingyiHandler() : TriggerSkill("#pingyi_handle")
    {
        events << EventLoseSkill << Death;
    }

    /*     virtual int getPriority(TriggerEvent) const
        { //caution other skills at Death event ,like chuancheng
            return -1;
        } */
/*

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (player->getMark("@pingyi") == 0 && player->getMark("pingyi_steal") == 0)
            return QStringList();

        if (triggerEvent == EventLoseSkill && data.toString() == "pingyi") {
            if (player->getMark("pingyi_steal") > 0 && !player->hasSkill("pingyi"))
                Pingyi::skill_comeback(room, player);

        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            //while yorihime(pingyi owner) die
            if (death.who == player && player->getMark("pingyi_steal") > 0)
                return QStringList(objectName());
            //while others (pingyi victim) die    
            if (death.who == player && player->getMark("@pingyi") > 0) {
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->getMark("pingyi_steal") > 0) {
                        QString pingyi_record = p->objectName() + "pingyi" + player->objectName();
                        QString back_skillname = room->getTag(pingyi_record).toString();
                        if (back_skillname != NULL && back_skillname != "")
                            Pingyi::skill_comeback(room, p);
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        Pingyi::skill_comeback(room, player);
        return false;
    }*/
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
    ZhesheVS() :OneCardViewAsSkill("zheshe")
    {
        filter_pattern = ".|.|.|hand!";//"^EquipCard!" ;
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
    Zheshe() : TriggerSkill("zheshe")
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
            invoke->targets.first()->drawCards(1, objectName());

        return false;
    }
};


class Tanchi : public TriggerSkill
{
public:
    Tanchi() : TriggerSkill("tanchi")
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
    if (choice == "rd" && effect.to->isWounded())
        room->recover(effect.to, recover);
    room->damage(damage);
    if (choice == "dr" && effect.to->isWounded() && effect.to->isAlive())
        room->recover(effect.to, recover);
}

class Zhuonong : public ZeroCardViewAsSkill
{
public:
    Zhuonong() :ZeroCardViewAsSkill("zhuonong")
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
    Jijing() : TriggerSkill("jijing")
    {
        events << Damaged;
        frequency = Compulsory;
    }
    
    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->isDead())
            return QList<SkillInvokeDetail>();

        ServerPlayer *p = room->getCurrent();
        if (!p->isCurrent())
            return QList<SkillInvokeDetail>();

        if (p->hasSkill(this) && damage.to->getMark("@jijing") == 0 && damage.to != p)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, p, p, NULL, true, damage.to);

        return QList<SkillInvokeDetail>();
    }
    
    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->setFixedDistance(invoke->invoker, invoke->targets.first(), 1);
        invoke->targets.first()->gainMark("@jijing");
        room->setPlayerFlag(invoke->invoker, "jijing");
        return false;
    }
};

class JijingClear : public TriggerSkill
{
public:
    JijingClear() : TriggerSkill("#jijing")
    {
        events << EventPhaseChanging;
    }
    
    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive && change.player->hasFlag("jijing")) {
            foreach (ServerPlayer *p, room->getOtherPlayers(change.player)) {
                if (p->getMark("@jijing") > 0)
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, NULL, true);
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (p->getMark("@jijing") > 0)
                invoke->targets << p;
        }
        return true;
    }
    
    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        foreach (ServerPlayer *p, invoke->targets) {
            p->loseAllMarks("@jijing");
            room->setFixedDistance(invoke->invoker, p, -1);
        }

        return false;
    }
};


class Ganying;
namespace
{
    Ganying *ganying_instance;
}

class Ganying : public TriggerSkill
{
public:
    Ganying() : TriggerSkill("ganying")
    {
        ganying_instance = this;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1, objectName());
        QStringList distance_changed = invoke->invoker->tag.value("distance_changed_" + QString::number(triggerEvent), QStringList()).toStringList();
        if (distance_changed.isEmpty())
            return false;

        QList<ServerPlayer *> targets;
        foreach (const QString &c, distance_changed) {
            ServerPlayer *p = room->findPlayerByObjectName(c);
            if (p != NULL)
                targets << p;
        }

        if (targets.isEmpty())
            return false;

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, "ganying", "@ganying", true);
        if (target == NULL)
            return false;

        int id = room->askForCardChosen(invoke->invoker, target, "h", "ganying", false, Card::MethodDiscard);
        room->throwCard(id, target, invoke->invoker == target ? NULL : invoke->invoker);

        return false;
    }
};

class GanyingHandler : public TriggerSkill
{
public:
    GanyingHandler() : TriggerSkill("ganying_handle")
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

            QStringList distance_changed = p->tag.value("distance_changed_" + QString::number(triggerEvent), QStringList()).toStringList();
            if (!distance_changed.isEmpty())
                d << SkillInvokeDetail(ganying_instance, p, p);
        }
    }
};

class Zhujiu : public TriggerSkill
{
public:
    Zhujiu() : TriggerSkill("zhujiu")
    {
        events << TargetConfirmed;
    }
/*

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Peach") || use.card->isKindOf("Analeptic")) {
            foreach (ServerPlayer *to, use.to) {
                if (player != to && !player->isNude())
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }



    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *to, use.to) {
            if (player != to && !player->isNude()) {
                player->tag["zhujiu_target"] = QVariant::fromValue(to);
                const Card *card = room->askForCard(player, "..", "@zhujiu:" + to->objectName(), data, Card::MethodNone);
                if (card) {
                    room->notifySkillInvoked(player, objectName());
                    room->touhouLogmessage("#InvokeSkill", player, objectName());
                    room->obtainCard(to, card->getEffectiveId(), room->getCardPlace(card->getEffectiveId()) != Player::PlaceHand);
                    player->drawCards(1);
                }
            }
        }
        return false;
    }*/
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
    QList<ServerPlayer *>logto;
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
    to1->tag["yushou_target"] = QVariant::fromValue(to2);
    int card_id = room->askForCardChosen(from, to1, "e", "yushou", false, Card::MethodNone, disable);

    const Card *card = Sanguosha->getCard(card_id);
    room->moveCardTo(card, to1, to2, Player::PlaceEquip,
        CardMoveReason(CardMoveReason::S_REASON_TRANSFER,
        from->objectName(), "yushou", QString()));

    QString choice = room->askForChoice(to1, "yushou", "damage+cancel");
    if (choice == "damage")
        room->damage(DamageStruct("yushou", to1, to2, 1, DamageStruct::Normal));

}
class Yushou : public ZeroCardViewAsSkill
{
public:
    Yushou() :ZeroCardViewAsSkill("yushou")
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
    handling_method = Card::MethodUse;
    mute = true;
}
bool PanduCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() == 0 && !to_select->isKongcheng() && to_select != Self;
}
void PanduCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    int card_id = room->askForCardChosen(source, target, "h", "pandu");
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
    Pandu() : ZeroCardViewAsSkill("pandu")
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
    Bihuo() : TriggerSkill("bihuo")
    {
        events << TargetConfirming;
    }
/*

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && !player->isKongcheng()) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (use.from->canSlash(p, use.card, false) || !use.to.contains(p))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (use.from->canSlash(p, use.card, false) && !use.to.contains(p))
                targets << p;
        }
        QString prompt = "@bihuo-playerchosen:" + use.from->objectName();
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), prompt, true, true);
        if (target) {
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
        }

        return false;
    }*/
};

class BihuoReturn : public TriggerSkill
{
public:
    BihuoReturn() : TriggerSkill("#bihuo")
    {
        events << EventPhaseChanging;
    }/*
    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer* &) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            foreach (ServerPlayer *s, room->getAlivePlayers()) {
                QString flag = "bihuo_" + s->objectName();
                foreach (ServerPlayer *p, room->getOtherPlayers(s)) {
                    if (p->hasFlag(flag)) {
                        p->setFlags("-" + flag);
                        int num = p->tag["bihuo_num_" + s->objectName()].toInt();
                        p->tag.remove("bihuo_num_" + s->objectName());
                        if (!p->isKongcheng()) {
                            int count = qMin(p->getHandcardNum(), num);
                            const Card *cards = room->askForExchange(p, "bihuo", count, count, false, "bihuo_exchange:" + QString::number(count) + ":" + s->objectName());
                            DELETE_OVER_SCOPE(const Card, cards)
                            room->obtainCard(s, cards, false);
                        }
                    }
                }

            }
        }
        return QStringList();
    }*/
};



TH99Package::TH99Package()
    : Package("th99")
{
    General *akyuu = new General(this, "akyuu$", "wai", 3, false);
    akyuu->addSkill(new Qiuwen);
    akyuu->addSkill(new Zaozu);
    akyuu->addSkill(new Dangjia);

    General *rinnosuke = new General(this, "rinnosuke", "wai");
    rinnosuke->addSkill(new XiufuDebug);

    General *tokiko = new General(this, "tokiko", "wai", 3, false);
    tokiko->addSkill(new Fandu);
    tokiko->addSkill(new Taohuan);

    General *renko = new General(this, "renko", "wai", 4, false);
    renko->addSkill(new Shitu);

    General *merry = new General(this, "merry", "wai", 4, false);
    merry->addSkill("jingjie");
    merry->addSkill(new Mengxian);
    merry->addRelateSkill("luanying");

    General *toyohime = new General(this, "toyohime", "wai", 3, false);
    toyohime->addSkill(new Lianxi);
    toyohime->addSkill(new Yueshi);

    General *yorihime = new General(this, "yorihime", "wai", 4, false);
    yorihime->addSkill(new Pingyi);
    yorihime->addSkill(new PingyiHandler);
    related_skills.insertMulti("pingyi", "#pingyi_handle");


    General *sunny = new General(this, "sunny", "wai", 4, false);
    sunny->addSkill(new Zheshe);
    sunny->addSkill(new Tanchi);

    General *lunar = new General(this, "lunar", "wai", 4, false);
    lunar->addSkill(new Zhuonong);
    lunar->addSkill(new Jijing);
    lunar->addSkill(new JijingClear);
    related_skills.insertMulti("jijing", "#jijing");

    General *star = new General(this, "star", "wai", 4, false);
    star->addSkill(new Ganying);

    General *kasen = new General(this, "kasen", "wai", 4, false);
    kasen->addSkill(new Zhujiu);
    kasen->addSkill(new Yushou);

    General *kosuzu = new General(this, "kosuzu", "wai", 3, false);
    kosuzu->addSkill(new Pandu);
    kosuzu->addSkill(new Bihuo);
    kosuzu->addSkill(new BihuoReturn);
    related_skills.insertMulti("bihuo", "#bihuo");

    General *mamizou = new General(this, "mamizou_sp", "wai", 4, false);
    Q_UNUSED(mamizou);

    General *reisen2 = new General(this, "reisen2", "wai", 4, false);
    Q_UNUSED(reisen2);

    addMetaObject<QiuwenCard>();
    addMetaObject<DangjiaCard>();
    addMetaObject<XiufuCard>();
    addMetaObject<XiufuMoveCard>();
    addMetaObject<ZhesheCard>();
    addMetaObject<ZhuonongCard>();
    addMetaObject<YushouCard>();
    addMetaObject<PanduCard>();
    skills << new DangjiaVS << new Luanying << new GanyingHandler << new XiufuMove;
}

ADD_PACKAGE(TH99)

