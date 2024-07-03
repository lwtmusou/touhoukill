#include "th17.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "skill.h"
#include "washout.h"

class Shanxing : public TriggerSkill
{
public:
    Shanxing()
        : TriggerSkill("shanxing")
    {
        events << CardsMoveOneTime;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *ko = qobject_cast<ServerPlayer *>(move.from);
        if (ko != nullptr && ko->isAlive() && move.from_places.contains(Player::PlaceEquip)) {
            QList<SkillInvokeDetail> d;

            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                d << SkillInvokeDetail(this, p, p, nullptr, false, ko);
            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->targets.first()->drawCards(1, objectName());
        if (invoke->targets.first() != invoke->invoker) {
            invoke->targets.first()->tag[objectName()] = QVariant::fromValue(invoke->invoker);
            const Card *c = room->askForExchange(invoke->targets.first(), objectName(), 1, 1, false, "@shanxing-exchange:" + invoke->invoker->objectName(), true);
            if (c != nullptr) {
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, invoke->targets.first()->objectName(), objectName(), QString());
                room->obtainCard(invoke->invoker, c, reason, false);
            }
        }
        return false;
    }
};

class LingshouOtherVS : public ViewAsSkill
{
public:
    LingshouOtherVS()
        : ViewAsSkill("LingshouOtherVS")
    {
        response_or_use = true;
        response_pattern = "@@LingshouOtherVS";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const override
    {
        bool ok = false;
        int selectedId = Self->property("lingshouSelected").toString().toInt(&ok);
        if (ok && to_select->getId() == selectedId)
            return true;

        const Card *c = Sanguosha->getCard(selectedId);
        return c->getSuit() == to_select->getSuit() && to_select->isEquipped();
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        bool ok = false;
        int selectedId = Self->property("lingshouSelected").toString().toInt(&ok);
        if (!ok)
            return nullptr;

        const Card *originalCard = Sanguosha->getCard(selectedId);
        bool containsOriginal = false;

        foreach (const Card *c, cards) {
            if (c->getId() == originalCard->getId()) {
                containsOriginal = true;
                break;
            }
        }
        if (!containsOriginal)
            return nullptr;

        QList<const Card *> equips = Self->getEquips();
        foreach (const Card *equip, equips) {
            if (equip->getSuit() == originalCard->getSuit()) {
                bool containsEquip = false;
                foreach (const Card *c, cards) {
                    if (c->getId() == equip->getId()) {
                        containsEquip = true;
                        break;
                    }
                }
                if (!containsEquip)
                    return nullptr;
            }
        }

        Slash *s = new Slash(Card::SuitToBeDecided, -1);
        s->addSubcards(cards);
        s->setSkillName("_lingshou");

        return s;
    }
};

class Lingshou : public TriggerSkill
{
public:
    Lingshou()
        : TriggerSkill("lingshou")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        if (p->isAlive() && p->hasSkill(this) && p->getPhase() == Player::Finish) {
            foreach (ServerPlayer *ps, room->getOtherPlayers(p)) {
                if (!ps->isKongcheng())
                    return {SkillInvokeDetail(this, p, p)};
            }
        }

        return {};
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *ps, room->getOtherPlayers(invoke->invoker)) {
            if (!ps->isKongcheng())
                targets << ps;
        }

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, "lingshou", "@lingshou-kill", true, true);
        if (target != nullptr) {
            QList<int> ids = target->handCards();

            room->fillAG(ids, invoke->invoker);
            int id1 = room->askForAG(invoke->invoker, ids, false, "lingshou");
            if (id1 == -1)
                id1 = ids.first();
            room->clearAG(invoke->invoker); // re-fill AG since the disabled cards changed
            room->showCard(target, id1);

            invoke->tag["lingshou"] = id1;
            invoke->targets << target;
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        int selectedIds = invoke->tag.value("lingshou").toInt();
        room->setPlayerProperty(invoke->targets.first(), "lingshouSelected", QString::number(selectedIds));
        if (room->askForUseCard(invoke->targets.first(), "@@LingshouOtherVS", "@lingshou-slash", -1, Card::MethodUse, true, "_lingshou") == nullptr) {
            DummyCard d({selectedIds});
            const Card *originalCard = Sanguosha->getCard(selectedIds);

            foreach (const Card *c, invoke->targets.first()->getCards("e")) {
                if (c->getSuit() == originalCard->getSuit())
                    d.addSubcard(c);
            }

            LogMessage log;
            log.type = "#Card_Recast";
            log.from = invoke->targets.first();
            log.card_str = IntList2StringList(d.getSubcards()).join("+");
            room->sendLog(log);

            CardMoveReason reason(CardMoveReason::S_REASON_RECAST, invoke->targets.first()->objectName());
            room->moveCardTo(&d, invoke->targets.first(), nullptr, Player::DiscardPile, reason);
            invoke->targets.first()->broadcastSkillInvoke("@recast");

            invoke->targets.first()->drawCards(d.getSubcards().length());
        }

        return false;
    }
};

class Qijue : public TriggerSkill
{
public:
    Qijue()
        : TriggerSkill("qijue$")
    {
        events << Death << BuryVictim;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        DeathStruct death = data.value<DeathStruct>();
        QList<SkillInvokeDetail> details;
        if (e == Death) {
            if (death.who->getKingdom() != "gxs")
                return details;

            foreach (ServerPlayer *p, room->getOtherPlayers(death.who)) {
                if (p->hasLordSkill(objectName()) && p->isWounded())
                    details << SkillInvokeDetail(this, p, death.who);
            }
        } else if (e == BuryVictim) {
            ServerPlayer *killer = death.damage->from;
            if (killer == nullptr || killer->getKingdom() != "gxs")
                return details;

            foreach (ServerPlayer *p, room->getOtherPlayers(killer)) {
                if (p->hasLordSkill(objectName()) && p->isWounded())
                    details << SkillInvokeDetail(this, p, killer);
            }
        }
        return details;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        bool result = invoke->invoker->askForSkillInvoke(objectName(), QVariant::fromValue(invoke->owner));
        if (result) {
            room->broadcastSkillInvoke(objectName());

            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->owner->objectName());
            room->notifySkillInvoked(invoke->owner, objectName());
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = invoke->invoker;
            log.to << invoke->owner;
            log.arg = objectName();
            room->sendLog(log);
        }
        return result;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        RecoverStruct recover;
        recover.who = invoke->invoker;
        room->recover(invoke->owner, recover);

        return false;
    }
};

class Shanlei : public TriggerSkill
{
public:
    Shanlei()
        : TriggerSkill("shanlei")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;

        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Start) {
            if (player->isAlive() && player->hasSkill(this) && player->getHandcardNum() > player->getMaxCards())
                r << SkillInvokeDetail(this, player, player, nullptr, true);
        } else if (player->getPhase() == Player::Finish) {
            if (player->isAlive() && player->hasSkill(this)) {
                bool flag = false;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->getHandcardNum() >= player->getHandcardNum()) {
                        flag = true;
                        break;
                    }
                }
                if (flag)
                    r << SkillInvokeDetail(this, player, player, nullptr, true);
            }
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
            if (invoke->invoker->hasShownSkill(this)) {
                LogMessage l;
                l.type = "#TriggerSkill";
                l.from = invoke->invoker;
                l.arg = objectName();
                room->sendLog(l);
            }
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (invoke->invoker->getPhase() == Player::Start) {
            // it is certain that p->getHandcardNum() > p->getMaxCards() equals true
            int n = invoke->invoker->getHandcardNum() - invoke->invoker->getMaxCards();
            room->askForDiscard(invoke->invoker, objectName(), n, n, false, false, "@shanlei-discard");
        } else {
            int cardnumMost = 0;
            foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
                if (p->getHandcardNum() >= cardnumMost)
                    cardnumMost = p->getHandcardNum();
            }
            int n = cardnumMost - invoke->invoker->getHandcardNum() + 1;
            room->drawCards(invoke->invoker, n, "shanlei");
        }

        return false;
    }
};

class Bengluo : public TriggerSkill
{
public:
    Bengluo()
        : TriggerSkill("bengluo")
    {
        events << CardsMoveOneTime << EventPhaseStart;
        global = true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if ((move.from != nullptr) && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_USE)
                && ((move.to != move.from) || ((move.to_place != Player::PlaceHand) && (move.to_place != Player::PlaceEquip))))
                move.from->setFlags("bengluo");
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *change = data.value<ServerPlayer *>();
            if (change->getPhase() == Player::RoundStart) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    p->setFlags("-bengluo");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *change = data.value<ServerPlayer *>();
            if (change->getPhase() == Player::Finish) {
                QList<SkillInvokeDetail> r;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasSkill(this) && p->hasFlag("bengluo"))
                        r << SkillInvokeDetail(this, p, p);
                }
                return r;
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == EventPhaseStart)
            return room->askForUseCard(invoke->invoker, "BasicCard+^Jink,EquipCard|.|.|sqchuangshi", "@bengluo-use", -1, Card::MethodUse, false) != nullptr;

        return false;
    }
};

LunniCard::LunniCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void LunniCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *current = room->getCurrent();

    Q_ASSERT(current != nullptr);
    Q_ASSERT((current->isAlive() && current->getPhase() == Player::Play));

    use.to << current;
    SkillCard::onUse(room, use);
}

void LunniCard::onEffect(const CardEffectStruct &effect) const
{
    const EquipCard *c = qobject_cast<const EquipCard *>(Sanguosha->getCard(effect.card->getSubcards().constFirst())->getRealCard());
    if (c == nullptr)
        return;

    EquipCard::Location location = c->location();

    int equipped_id = Card::S_UNKNOWN_CARD_ID;
    ServerPlayer *target = effect.to;
    Room *room = target->getRoom();
    if (target->getEquip(location) != nullptr)
        equipped_id = target->getEquip(location)->getEffectiveId();

    QList<CardsMoveStruct> exchangeMove;
    CardsMoveStruct move1(subcards.first(), target, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_PUT, target->objectName()));
    exchangeMove.push_back(move1);
    if (equipped_id != Card::S_UNKNOWN_CARD_ID) {
        CardsMoveStruct move2(equipped_id, nullptr, Player::DiscardPile, CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, target->objectName()));
        exchangeMove.push_back(move2);
    }
    room->moveCardsAtomic(exchangeMove, true);

    effect.to->setFlags("lunni");
}

class LunniVS : public OneCardViewAsSkill
{
public:
    LunniVS()
        : OneCardViewAsSkill("lunni")
    {
        filter_pattern = ".|.|.|equipped";
        response_pattern = "@@lunni";
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        LunniCard *c = new LunniCard;
        c->addSubcard(originalCard);
        c->setShowSkill("lunni");
        return c;
    }
};

class Lunni : public TriggerSkill
{
public:
    Lunni()
        : TriggerSkill("lunni")
    {
        events << EventPhaseStart << EventPhaseEnd;
        view_as_skill = new LunniVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;

        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->getPhase() == Player::Play) {
                QList<ServerPlayer *> players = room->getOtherPlayers(p);
                foreach (ServerPlayer *bull, players) {
                    if (bull->isAlive() && bull->hasSkill(this) && !bull->isAllNude())
                        r << SkillInvokeDetail(this, bull, bull, p);
                }
            }
        } else if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *change = data.value<ServerPlayer *>();
            if (change->hasFlag("lunni")) {
                if (change->getPhase() == Player::Play && !change->getCards("e").isEmpty())
                    r << SkillInvokeDetail(this, change, change, nullptr, true, nullptr, false);
            }
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            return room->askForUseCard(invoke->invoker, "@@lunni", "@lunni-discard:" + invoke->targets.first()->objectName(), -1, Card::MethodNone) != nullptr;
        } else {
            ServerPlayer *change = data.value<ServerPlayer *>();
            if (change->hasFlag("lunni") && invoke->invoker == change)
                return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *change = data.value<ServerPlayer *>();
            if (change->hasFlag("lunni")) {
                LogMessage l;
                l.type = "#lunni-eff4";
                l.from = change;
                room->sendLog(l);

                DummyCard d;
                d.addSubcards(change->getCards("e"));
                change->obtainCard(&d);
            }
        }

        return false;
    }
};

class LiaoguVS : public OneCardViewAsSkill
{
public:
    LiaoguVS()
        : OneCardViewAsSkill("liaogu")
    {
        response_pattern = "@@liaogu";
        expand_pile = "*liaogu_temp";
    }

    bool viewFilter(const Card *to_select) const override
    {
        return StringList2IntList(Self->property("liaogu_temp").toString().split("+")).contains(to_select->getEffectiveId()) && to_select->isAvailable(Self);
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        LiaoguCard *c = new LiaoguCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class Liaogu : public TriggerSkill
{
public:
    Liaogu()
        : TriggerSkill("liaogu")
    {
        view_as_skill = new LiaoguVS;
        events << CardsMoveOneTime << EventPhaseStart;
        global = true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != nullptr && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                && ((move.to != move.from) || (move.to_place != Player::PlaceHand && move.to_place != Player::PlaceEquip)))
                if (room->getCurrent() != nullptr && room->getCurrent()->isAlive() && room->getCurrent()->getPhase() != Player::NotActive)
                    move.from->setFlags("liaogulost");
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current->getPhase() == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    p->setFlags("-liaogulost");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.reason.m_reason == CardMoveReason::S_REASON_RULEDISCARD && move.from != nullptr && move.from->isCurrent() && move.from->getPhase() == Player::Discard) {
                bool inDiscardPile = false;
                foreach (int cardid, move.card_ids) {
                    if (room->getCardPlace(cardid) == Player::DiscardPile) {
                        inDiscardPile = true;
                        break;
                    }
                }

                if (!inDiscardPile)
                    return {};

                QList<SkillInvokeDetail> d;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p != move.from && p->hasSkill(this))
                        d << SkillInvokeDetail(this, p, p);
                }

                return d;
            }
        }

        return {};
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> discardIds;
        foreach (int cardid, move.card_ids) {
            if (room->getCardPlace(cardid) == Player::DiscardPile)
                discardIds << cardid;
        }

        invoke->invoker->tag["liaogu_tempmove"] = IntList2VariantList(discardIds);
        room->setPlayerProperty(invoke->invoker, "liaogu_temp", IntList2StringList(discardIds).join("+"));
        room->askForUseCard(invoke->invoker, "@@liaogu", "@liaogu-use");
        room->setPlayerProperty(invoke->invoker, "liaogu_temp", QString());
        return false;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &) const override
    {
        return false;
    }
};

LiaoguCard::LiaoguCard()
{
    will_throw = false;
}

bool LiaoguCard::targetFixed(const Player *Self) const
{
    return Sanguosha->getCard(subcards.first())->targetFixed(Self);
}

bool LiaoguCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *oc = Sanguosha->getCard(subcards.first());
    return oc->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, oc, targets);
}

bool LiaoguCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Card *oc = Sanguosha->getCard(subcards.first());
    if (oc->canRecast() && targets.length() == 0)
        return false;
    return oc->targetsFeasible(targets, Self);
}

void LiaoguCard::use(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct new_use = card_use;
    new_use.card = Sanguosha->getCard(subcards.first());
    room->useCard(new_use);

    if (!card_use.from->hasFlag("liaogulost") && card_use.from->canDiscard(card_use.from, "hes"))
        room->askForDiscard(card_use.from, "liaogu", 1, 1, false, true, "@liaogu-discard");
}

class XuelingVS : public OneCardViewAsSkill
{
public:
    XuelingVS()
        : OneCardViewAsSkill("xueling")
    {
        expand_pile = "#judging_area";
        response_pattern = "@@xueling";
    }

    bool viewFilter(const Card *to_select) const override
    {
        if (Self->canDiscard(Self, to_select->getEffectiveId()))
            return true;

        if (Self->hasEquip(to_select))
            return !Self->isBrokenEquip(to_select->getEffectiveId());

        if (!Self->getJudgingAreaID().contains(to_select->getEffectiveId()))
            return !Self->isShownHandcard(to_select->getEffectiveId());

        return false;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        return new DummyCard({originalCard->getEffectiveId()});
    }
};

class Xueling : public TriggerSkill
{
public:
    Xueling()
        : TriggerSkill("xueling")
    {
        events = {CardsMoveOneTime};
        view_as_skill = new XuelingVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
        if (from != nullptr && from->isAlive() && from->hasSkill(this) && from->getPhase() != Player::Play && move.reason.m_skillName != objectName()
            && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip)) && from->getCardCount(true, true) > 0)
            return {SkillInvokeDetail(this, from, from)};

        return {};
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        const Card *c = room->askForCard(invoke->invoker, "@@xueling", "@xueling-select", {}, Card::MethodNone, nullptr, false, objectName(), false, 1);
        if (c != nullptr) {
            invoke->tag[objectName()] = c->getEffectiveId();
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        bool ok = false;
        int id = invoke->tag.value(objectName()).toInt(&ok);
        if (ok) {
            const Card *originalCard = Sanguosha->getCard(id);
            QStringList choices;

            if (invoke->invoker->canDiscard(invoke->invoker, id))
                choices << "discard";
            if (invoke->invoker->hasEquip(originalCard) && !invoke->invoker->isBrokenEquip(id))
                choices << "breakEquip";
            if (invoke->invoker->handCards().contains(id) && !invoke->invoker->isShownHandcard(id))
                choices << "showHandcard";

            QString choice = choices.first();
            if (choices.length() > 1)
                choice = room->askForChoice(invoke->invoker, objectName(), choices.join("+"));

            if (choice == "breakEquip" && invoke->invoker->hasEquip(originalCard))
                invoke->invoker->addBrokenEquips({id});
            else if (choice == "showHandcard" && invoke->invoker->handCards().contains(id))
                invoke->invoker->addToShownHandCards({id});
            else if (choice == "discard" && invoke->invoker->canDiscard(invoke->invoker, id)) // Skill name of the reason counts. It is got judged in triggerable
                room->throwCard(originalCard, CardMoveReason(CardMoveReason::S_REASON_DISCARD, invoke->invoker->objectName(), objectName(), {}), invoke->invoker);

            int h = invoke->invoker->getHandcardNum() - invoke->invoker->getShownHandcards().length();
            int e = invoke->invoker->getEquips().length() - invoke->invoker->getBrokenEquips().length();
            int j = invoke->invoker->getJudgingAreaID().length();
            if (h <= 1 && e <= 1 && j <= 1)
                room->drawCards(invoke->invoker, 2, objectName());
        }

        return false;
    }
};

class Weiling : public TriggerSkill
{
public:
    Weiling()
        : TriggerSkill("weiling")
    {
        events = {BrokenEquipChanged, ShownCardChanged};
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == BrokenEquipChanged) {
            BrokenEquipChangedStruct change = data.value<BrokenEquipChangedStruct>();
            if (!change.moveFromEquip && !change.broken && change.player->isAlive() && change.player->hasSkill(this)) {
                SkillInvokeDetail d(this, change.player, change.player);
                d.tag[objectName()] = IntList2VariantList(change.ids);
                return {d};
            }
        } else if (triggerEvent == ShownCardChanged) {
            ShownCardChangedStruct change = data.value<ShownCardChangedStruct>();
            if (!change.moveFromHand && !change.shown && change.player->isAlive() && change.player->hasSkill(this)) {
                SkillInvokeDetail d(this, change.player, change.player);
                d.tag[objectName()] = IntList2VariantList(change.ids);
                return {d};
            }
        }

        return {};
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QVariantList ids_v = invoke->tag.value(objectName()).toList();
        QList<int> ids = VariantList2IntList(ids_v);
        if (ids.isEmpty())
            return false;

        invoke->invoker->tag[objectName()] = ids_v;
        ServerPlayer *p = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(),
                                                   "@weiling-exchange" + QString::number(static_cast<int>(triggerEvent)), true, true);
        if (p != nullptr) {
            invoke->targets << p;
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QVariantList ids_v = invoke->tag.value(objectName()).toList();
        QList<int> ids = VariantList2IntList(ids_v);
        if (ids.isEmpty())
            return false;

        DummyCard d(ids);
        room->obtainCard(invoke->targets.first(), &d, CardMoveReason(CardMoveReason::S_REASON_GIVE, invoke->invoker->objectName(), objectName(), {}));

        return false;
    }
};

WeiyiCard::WeiyiCard()
{
    sort_targets = false;
}

bool WeiyiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    return targets.length() == 2 && targetFilter({}, targets.first(), Self) && targetFilter({targets.first()}, targets.last(), Self);
}

bool WeiyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (targets.length() == 0)
        return !to_select->isNude() && to_select != Self;

    if (targets.length() == 1)
        return targets.first()->canSlash(to_select, false);

    return false;
}

void WeiyiCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *a = card_use.to.first();
    ServerPlayer *b = card_use.to.last();

    QStringList prompt = {"@weiyi-askforuseslashto"};
    prompt << card_use.from->objectName() << b->objectName();

    if (!room->askForUseSlashTo(a, b, prompt.join(":"), false)) {
        if (!a->isNude() && a != card_use.from) {
            QStringList prompt = {"@weiyi-askforexchange"};
            prompt << card_use.from->objectName();
            const Card *c = room->askForExchange(a, "weiyi", 1, 1, true, prompt.join(":"));
            if (c == nullptr) {
                QList<const Card *> cs = a->getCards("hes");
                if (!cs.isEmpty())
                    c = cs.first();
            }
            if (c == nullptr)
                return;

            card_use.from->tag["weiyiSelected"] = c->getEffectiveId(); // for AI and skill event trigger
            card_use.from->tag["weiyiFrom"] = QVariant::fromValue(a);
            room->setPlayerProperty(card_use.from, "weiyiSelected", QString::number(c->getEffectiveId())); // for client use
            room->obtainCard(card_use.from, c, false);
            card_use.from->tag.remove("weiyiFrom");
            card_use.from->tag.remove("weiyiSelected");
        }
    }
}

class WeiyiVS : public ViewAsSkill
{
public:
    WeiyiVS()
        : ViewAsSkill("weiyi")
    {
        response_pattern = "@@weiyi";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY)
            return false;
        if (selected.isEmpty()) {
            bool ok = false;
            int id = Self->property("weiyiSelected").toString().toInt(&ok);
            return ok && to_select->getId() == id;
        }
        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY)
            return new WeiyiCard;

        Slash *s = new Slash(Card::SuitToBeDecided, -1);
        s->addSubcards(cards);
        s->setSkillName("_" + objectName());
        return s;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("WeiyiCard");
    }

    bool isResponseOrUse() const override
    {
        return Sanguosha->getCurrentCardUsePattern() == "@@weiyi";
    }
};

class Weiyi : public TriggerSkill
{
public:
    Weiyi()
        : TriggerSkill("weiyi")
    {
        events = {CardsMoveOneTime};
        view_as_skill = new WeiyiVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *to = qobject_cast<ServerPlayer *>(move.to);
        if (to != nullptr && to->tag.contains("weiyiSelected") && move.to_place == Player::PlaceHand) {
            bool ok = false;
            int id = to->tag.value("weiyiSelected").toInt(&ok);
            if (ok && move.card_ids.contains(id) && room->getCardOwner(id) == to && room->getCardPlace(id) == Player::PlaceHand) {
                ServerPlayer *a = to->tag.value("weiyiFrom").value<ServerPlayer *>();
                if (a != nullptr && to->canSlash(a))
                    return {SkillInvokeDetail(this, to, to, a, true, nullptr, false)};
            }
        }

        return {};
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        // copy & paste Room::askForUseSlashTo

        ServerPlayer *slasher = invoke->invoker;
        ServerPlayer *victim = invoke->targets.first();

        room->setPlayerFlag(slasher, "slashTargetFix");
        room->setPlayerFlag(slasher, "slashTargetFixToOne");
        room->setPlayerFlag(victim, "SlashAssignee");

        const Card *slash = room->askForUseCard(slasher, "@@weiyi", "@weiyi:" + victim->objectName(), 0, Card::MethodUse, false);
        if (slash == nullptr) {
            room->setPlayerFlag(slasher, "-slashTargetFix");
            room->setPlayerFlag(slasher, "-slashTargetFixToOne");
            room->setPlayerFlag(victim, "-SlashAssignee");
        }

        return slash != nullptr;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &) const override
    {
        return false;
    }
};

class Duozhi : public TriggerSkill
{
public:
    Duozhi()
        : TriggerSkill("duozhi")
    {
        events = {EventPhaseStart, EventPhaseChanging};
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->isCardLimited("use,response", objectName()))
                        room->removePlayerCardLimitation(p, "use,response", "Jink$1", objectName(), true);
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->isAlive() && p->getPhase() == Player::Start) {
                foreach (ServerPlayer *yachie, room->getAlivePlayers()) {
                    if (yachie->isAlive() && yachie->hasSkill(this) && p->getHandcardNum() >= yachie->getHandcardNum()) {
                        foreach (ServerPlayer *others, room->getOtherPlayers(yachie)) {
                            if (others->getHandcardNum() < yachie->getHandcardNum()) {
                                d << SkillInvokeDetail(this, yachie, yachie);
                                break;
                            }
                        }
                    }
                }
            }
        }

        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *others, room->getOtherPlayers(invoke->invoker)) {
            if (others->getHandcardNum() < invoke->invoker->getHandcardNum())
                targets << others;
        }

        ServerPlayer *target
            = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@duozhi-yingyingying:::" + QString::number(invoke->invoker->getHandcardNum()), true, true);
        if (target != nullptr) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->targets.first()->drawCards(1, objectName());
        room->setPlayerCardLimitation(invoke->targets.first(), "use,response", "Jink", objectName(), true);

        return false;
    }
};

JunzhenCard::JunzhenCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool JunzhenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty())
        return false;

    // if Self is owner of Junzhen, the judgement happens here
    return feasible(Self, to_select);
}

bool JunzhenCard::targetFixed(const Player *Self) const
{
    return !Self->hasFlag("junzhen_owner");
}

void JunzhenCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.from->hasFlag("junzhen_owner")) {
        room->setPlayerFlag(use.from, "-junzhen_owner");
    } else {
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->hasFlag("junzhen_owner")) {
                use.to = {p};
                room->setPlayerFlag(p, "-junzhen_owner");
            }
        }
    }

    SkillCard::onUse(room, use);
}

void JunzhenCard::use(Room *room, const CardUseStruct &use) const
{
    CardsMoveStruct move;
    move.card_ids = subcards;
    move.to = use.to.first();
    move.to_place = Player::PlaceEquip;
    move.reason = CardMoveReason(CardMoveReason::S_REASON_PUT, use.from->objectName(), "junzhen", QString());
    room->moveCardsAtomic(move, true);

    // should record use.to here....
    use.from->tag["junzhenTo"] = QVariant::fromValue<ServerPlayer *>(use.to.first());
}

bool JunzhenCard::feasible(const Player *Self, const Player *target) const
{
    if (target == Self)
        return false;

    if (!target->isAlive())
        return false;

    if (!(Self->hasFlag("junzhen_owner") || target->hasFlag("junzhen_owner")))
        return false;

    foreach (int id, subcards) {
        const EquipCard *c = qobject_cast<const EquipCard *>(Sanguosha->getCard(id)->getRealCard());
        if (c == nullptr)
            return false;

        if (target->getEquip(c->location()) != nullptr)
            return false;
    }

    return true;
}

class JunzhenVS : public ViewAsSkill
{
public:
    JunzhenVS()
        : ViewAsSkill("junzhen")
    {
    }

    bool isEnabledAtPlay(const Player *) const override
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const override
    {
        return pattern.startsWith("@@junzhen-card");
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@junzhen-card1") {
            return to_select->isKindOf("EquipCard");
        } else if (pattern == "@@junzhen-card2") {
            return selected.isEmpty();
        }

        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.isEmpty())
            return nullptr;

        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@junzhen-card1") {
            JunzhenCard *dc = new JunzhenCard;
            dc->addSubcards(cards);
            dc->setShowSkill("junzhen");

            // Wait! Since we decided to make JunzhenCard targetFixed = true when Self is not owner of junzhen
            // The judgement should be here
            bool feasible = false;
            if (Self->hasFlag("junzhen_owner")) {
                feasible = true;
            } else {
                foreach (const Player *p, Self->getAliveSiblings()) {
                    if (p->hasFlag("junzhen_owner")) {
                        feasible = dc->feasible(Self, p);
                        break;
                    }
                }
            }

            if (feasible)
                return dc;
            else
                delete dc;
        } else if (pattern == "@@junzhen-card2") {
            Slash *sl = new Slash(Card::SuitToBeDecided, -1);
            sl->addSubcards(cards);
            sl->setSkillName("_junzhen");
            return sl;
        }

        return nullptr;
    }
};

class Junzhen : public TriggerSkill
{
public:
    Junzhen()
        : TriggerSkill("junzhen")
    {
        events << EventPhaseEnd;
        view_as_skill = new JunzhenVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;

        ServerPlayer *c = data.value<ServerPlayer *>();
        if (c->getPhase() == Player::Play && c->isAlive() && !c->isNude()) {
            foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                r << SkillInvokeDetail(this, owner, c);
        }

        return r;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->setPlayerFlag(invoke->owner, "junzhen_owner");
        invoke->invoker->tag.remove("junzhenTo");
        QString prompt = (invoke->invoker == invoke->owner) ? QStringLiteral("@junzhen-prompt2") : (QString(QStringLiteral("@junzhen-prompt1:")) + invoke->owner->objectName());
        int noticeIndex = (invoke->invoker == invoke->owner) ? 2 : 1;
        bool invoked = room->askForUseCard(invoke->invoker, "@@junzhen-card1", prompt, noticeIndex, Card::MethodNone);
        if (!invoked)
            room->setPlayerFlag(invoke->owner, "-junzhen_owner");

        return invoked;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = invoke->invoker->tag["junzhenTo"].value<ServerPlayer *>();
        invoke->invoker->tag.remove("junzhenTo");

        if (target != nullptr && !target->isNude())
            room->askForUseCard(target, "@@junzhen-card2", "@junzhen-prompt3", 3, Card::MethodUse);

        return false;
    }
};

class CiouRecord : public TriggerSkill
{
public:
    CiouRecord()
        : TriggerSkill("#ciou")
    {
        events = {DamageDone, EventPhaseChanging};
        global = true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();

            if (damage.to->isAlive()) {
                if ((damage.card != nullptr
                     && (damage.card->isVirtualCard()
                         && !(damage.card->subcardsLength() == 1 && Sanguosha->getCard(damage.card->getEffectiveId())->getClassName() == damage.card->getClassName())))
                    || (damage.nature != DamageStruct::Normal))
                    damage.to->setFlags("ciou");
            }

        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct s = data.value<PhaseChangeStruct>();
            if (s.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    p->setFlags("-ciou");
            }
        }
    }
};

class Ciou : public TriggerSkill
{
public:
    Ciou()
        : TriggerSkill("ciou")
    {
        events = {EventPhaseStart};
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        if (p->isAlive() && p->hasSkill(this) && p->hasFlag(objectName()))
            return {SkillInvokeDetail(this, p, p)};

        return {};
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        RecoverStruct recover;
        recover.reason = objectName();
        room->recover(invoke->invoker, recover);

        return false;
    }
};

class Jinji : public DistanceSkill
{
public:
    Jinji()
        : DistanceSkill("jinji")
    {
    }

    int getCorrect(const Player *from, const Player *to) const override
    {
        if (from == to)
            return 0;
        int i = 0;
        if (from->hasSkill(this) && !from->hasFlag("jinji_basic"))
            i -= 1;
        if (to->hasSkill(this) && !to->hasFlag("jinji_basic"))
            i += 1;

        return i;
    }
};

class JinjiT : public TriggerSkill
{
public:
    JinjiT()
        : TriggerSkill("#jinji-t")
    {
        events << EventPhaseChanging << CardUsed << CardResponded;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() == Card::TypeBasic && use.from != nullptr && use.from->hasSkill("jinji") && use.from->isAlive())
                room->setPlayerFlag(use.from, "jinji_basic");
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card->getTypeId() == Card::TypeBasic && resp.m_from != nullptr && resp.m_from->hasSkill("jinji") && resp.m_from->isAlive())
                room->setPlayerFlag(resp.m_from, "jinji_basic");
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *saki, room->getAllPlayers()) {
                    if (saki->hasFlag("jinji_basic"))
                        room->setPlayerFlag(saki, "-jinji_basic");
                }
            }
        }
    }
};

class TianxingVS : public ZeroCardViewAsSkill
{
public:
    TianxingVS()
        : ZeroCardViewAsSkill("tianxing")
    {
        response_pattern = "@@tianxing";
    }

    const Card *viewAs() const override
    {
        PowerSlash *ps = new PowerSlash(Card::NoSuit, 0);
        ps->setSkillName(objectName());
        ps->setShowSkill(objectName());
        return ps;
    }
};

class Tianxing : public TriggerSkill
{
public:
    Tianxing()
        : TriggerSkill("tianxing")
    {
        events << EventPhaseEnd << EventPhaseStart;
        view_as_skill = new TianxingVS;
        global = true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *c = data.value<ServerPlayer *>();
            if (c->getPhase() == Player::Play) {
                foreach (ServerPlayer *saki, room->getAllPlayers()) {
                    int n = 0;
                    foreach (ServerPlayer *p, room->getOtherPlayers(saki)) {
                        if (saki->inMyAttackRange(p))
                            ++n;
                    }
                    saki->tag["tianxing"] = n;
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;

        if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *c = data.value<ServerPlayer *>();
            if (c->getPhase() == Player::Play) {
                foreach (ServerPlayer *saki, room->findPlayersBySkillName(objectName())) {
                    int n = 0;
                    foreach (ServerPlayer *p, room->getOtherPlayers(saki)) {
                        if (saki->inMyAttackRange(p))
                            ++n;
                    }
                    if (n != saki->tag["tianxing"].toInt())
                        r << SkillInvokeDetail(this, saki, saki);
                }
            }
        }

        return r;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        return room->askForUseCard(invoke->invoker, "@@tianxing", "@tianxing-slash");
    }
};

TH17Package::TH17Package()
    : Package("th17")
{
    General *keiki = new General(this, "keiki$", "gxs");
    keiki->addSkill(new Shanxing);
    keiki->addSkill(new Lingshou);
    keiki->addSkill(new Qijue);

    General *saki = new General(this, "saki", "gxs");
    saki->addSkill(new Jinji);
    saki->addSkill(new JinjiT);
    saki->addSkill(new Tianxing);
    saki->addSkill(new SlashNoDistanceLimitSkill("tianxing"));
    related_skills.insertMulti("jinji", "#jinji-t");
    related_skills.insertMulti("tianxing", "#tianxing-slash-ndl");

    General *mayumi = new General(this, "mayumi", "gxs");
    mayumi->addSkill(new Junzhen);
    mayumi->addSkill(new Ciou);
    mayumi->addSkill(new CiouRecord);
    related_skills.insertMulti("ciou", "#ciou");

    General *yachie = new General(this, "yachie", "gxs");
    yachie->addSkill(new Weiyi);
    yachie->addSkill(new Duozhi);

    General *kutaka = new General(this, "kutaka", "gxs", 3);
    kutaka->addSkill(new Xueling);
    kutaka->addSkill(new Weiling);

    General *urumi = new General(this, "urumi", "gxs");
    urumi->addSkill(new Lunni);
    urumi->addSkill(new Liaogu);

    General *eika = new General(this, "eika", "gxs", 3);
    eika->addSkill(new Shanlei);
    eika->addSkill(new Bengluo);

    addMetaObject<LiaoguCard>();
    addMetaObject<LunniCard>();
    addMetaObject<JunzhenCard>();
    addMetaObject<WeiyiCard>();

    skills << new LingshouOtherVS;
}

ADD_PACKAGE(TH17)
