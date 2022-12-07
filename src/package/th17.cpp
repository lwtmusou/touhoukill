#include "th17.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "skill.h"
#include "testCard.h"

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
        RecoverStruct recover;
        recover.who = invoke->invoker;
        room->recover(invoke->targets.first(), recover);

        invoke->targets.first()->drawCards(1, objectName());
        if (invoke->targets.first() != invoke->invoker) {
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
        QList<int> selectedIds = StringList2IntList(Self->property("lingshouSelected").toString().split("+"));
        return selectedIds.contains(to_select->getId());
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        Slash *s = new Slash(Card::SuitToBeDecided, -1);
        s->addSubcards(cards);
        s->setSkillName("_lingshou");

        if (s->getSubcards().toSet() == StringList2IntList(Self->property("lingshouSelected").toString().split("+")).toSet())
            return s;

        delete s;
        return nullptr;
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

            QList<int> disable_ids;
            QList<int> ids2;
            foreach (int id, ids) {
                if ((Sanguosha->getCard(id)->getSuit() == Sanguosha->getCard(id1)->getSuit()) && (id != id1))
                    ids2 << id;
                else
                    disable_ids << id;
            }

            int id2 = -1;
            if (!ids2.isEmpty()) {
                room->fillAG(ids, invoke->invoker, disable_ids);
                id2 = room->askForAG(invoke->invoker, ids2, true, "lingshou");
                room->clearAG(invoke->invoker);
            }

            room->showCard(target, id1);
            QList<int> selectedIds {id1};
            if (id2 != -1) {
                selectedIds << id2;
                room->showCard(target, id2);
            }

            invoke->tag["lingshou"] = IntList2VariantList(selectedIds);
            invoke->targets << target;
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<int> selectedIds = VariantList2IntList(invoke->tag.value("lingshou").toList());
        room->setPlayerProperty(invoke->targets.first(), "lingshouSelected", IntList2StringList(selectedIds).join("+"));
        if (room->askForUseCard(invoke->targets.first(), "@@LingshouOtherVS", "@lingshou-slash", -1, Card::MethodUse, true, "_lingshou") == nullptr) {
            DummyCard d(selectedIds);

            LogMessage log;
            log.type = "#Card_Recast";
            log.from = invoke->targets.first();
            log.card_str = IntList2StringList(selectedIds).join("+");
            room->sendLog(log);

            CardMoveReason reason(CardMoveReason::S_REASON_RECAST, invoke->targets.first()->objectName());
            room->moveCardTo(&d, invoke->targets.first(), nullptr, Player::DiscardPile, reason);
            invoke->targets.first()->broadcastSkillInvoke("@recast");

            invoke->targets.first()->drawCards(selectedIds.length());
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
            if (change->getPhase() == Player::Play) {
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
    const EquipCard *c = qobject_cast<const EquipCard *>(Sanguosha->getCard(effect.card->getSubcards().first())->getRealCard());
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
        filter_pattern = "EquipCard";
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
        filter_pattern = ".|.|.|#liaoguTemp";
        expand_pile = "#liaoguTemp";
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
    // notify liaogu card move
    static void liaoguTempMove(Room *room, ServerPlayer *to, QList<int> inCards)
    {
        QList<int> inCardsCopy = inCards;
        QList<int> outCards = VariantList2IntList(to->tag.value("liaogu_tempmove", QVariantList()).toList());
        QList<CardsMoveStruct> moves;

        // remove duplicate
        foreach (int c, inCardsCopy) {
            if (outCards.contains(c)) {
                inCards.removeAll(c);
                outCards.removeAll(c);
            }
        }

        if (!outCards.isEmpty()) {
            CardsMoveStruct move;
            move.card_ids = outCards;
            move.open = true;
            move.from = to;
            move.from_player_name = to->objectName();
            move.from_place = Player::PlaceSpecial;
            move.from_pile_name = "#liaoguTemp";
            move.to = nullptr;
            move.to_place = Player::DiscardPile;

            moves << move;
        }

        if (!inCards.isEmpty()) {
            CardsMoveStruct move;
            move.card_ids = inCards;
            move.open = true;
            move.from = nullptr;
            move.from_place = Player::DiscardPile;
            move.to = to;
            move.to_player_name = to->objectName();
            move.to_place = Player::PlaceSpecial;
            move.to_pile_name = "#liaoguTemp";

            moves << move;
        }

        if (!inCardsCopy.isEmpty())
            to->tag["liaogu_tempmove"] = IntList2VariantList(inCardsCopy);
        else
            to->tag.remove("liaogu_tempmove");

        if (moves.isEmpty())
            return;

        room->setPlayerFlag(to, "liaogu_InTempMoving");
        room->notifyMoveCards(true, moves, true, {to});
        room->notifyMoveCards(false, moves, true, {to});
        room->setPlayerFlag(to, "-liaogu_InTempMoving");
    }

    Liaogu()
        : TriggerSkill("liaogu")
    {
        view_as_skill = new LiaoguVS;
        events << CardsMoveOneTime << EventPhaseChanging;
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
        } else if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                p->setFlags("-liaogulost");
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

        liaoguTempMove(room, invoke->invoker, discardIds);

        if (!room->askForUseCard(invoke->invoker, "@@liaogu", "@liaogu-use"))
            liaoguTempMove(room, invoke->invoker, {});

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

void LiaoguCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    Liaogu::liaoguTempMove(room, card_use.from, {});
    SkillCard::onUse(room, card_use);
}

void LiaoguCard::use(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct new_use = card_use;
    new_use.card = Sanguosha->getCard(subcards.first());
    room->useCard(new_use);

    if (!card_use.from->hasFlag("liaogulost") && card_use.from->canDiscard(card_use.from, "hes"))
        room->askForDiscard(card_use.from, "liaogu", 1, 1, false, true, "@liaogu-discard");
}

class Yvshou : public TriggerSkill
{
public:
    Yvshou()
        : TriggerSkill("yvshou")
    {
        events << CardUsed << CardResponded << EventPhaseStart << TurnStart;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const override
    {
        if (triggerEvent == TurnStart) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                p->setMark("yvshou", 0);
                p->setFlags("-yvshouFirst");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;

        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current->isAlive() && current->getPhase() == Player::Start) {
                foreach (ServerPlayer *p, room->getOtherPlayers(current)) {
                    if (p->isAlive() && p->hasSkill(this) && !p->isKongcheng())
                        r << SkillInvokeDetail(this, p, p, current);
                }
            }
        } else if ((triggerEvent == CardUsed) || (triggerEvent == CardResponded)) {
            const Card *card = nullptr;
            ServerPlayer *from = nullptr;
            if (triggerEvent == CardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                card = use.card;
                from = use.from;
            } else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse) {
                    card = resp.m_card;
                    from = resp.m_from;
                }
            }

            if (card != nullptr && card->getNumber() > 0 && (!card->isVirtualCard() || card->getSubcards().length() == 1)
                && ((card->getTypeId() == Card::TypeBasic) || card->isNDTrick()) && from != nullptr && from->isInMainPhase() && !from->hasFlag("yvshouFirst")) {
                SkillInvokeDetail d(this, nullptr, nullptr, from, true);
                d.tag["yvshou"] = QVariant::fromValue<const Card *>(card);
                foreach (ServerPlayer *p, room->getOtherPlayers(from)) {
                    if (p->isAlive() && p->getMark("yvshou") > 0 && p->hasSkill(this)) {
                        d.invoker = p;
                        d.owner = p;
                        r << d;
                    }
                }
            }
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            const Card *c = room->askForCard(invoke->invoker, "..", "@yvshou-discard:" + invoke->targets.first()->objectName(), data, "yvshou");
            if (c != nullptr) {
                invoke->tag["yvshou"] = c->getEffectiveId();
                return true;
            }
        } else {
            invoke->targets.first()->setFlags("yvshouFirst");
            return TriggerSkill::cost(triggerEvent, room, invoke, data);
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            invoke->invoker->setMark("yvshou", Sanguosha->getCard(invoke->tag["yvshou"].toInt())->getNumber());
        } else {
            int mark = invoke->invoker->getMark("yvshou");
            const Card *c = invoke->tag.value("yvshou").value<const Card *>();
            int number = c->getNumber();

            LogMessage l;
            l.from = invoke->targets.first();
            l.arg = objectName();
            l.arg2 = c->objectName();

            if (number <= mark) {
                l.type = "#shenwei";
                if (triggerEvent == CardUsed) {
                    CardUseStruct use = data.value<CardUseStruct>();
                    if (use.card->isKindOf("Nullification"))
                        room->setPlayerFlag(use.from, "nullifiationNul");
                    else
                        use.nullified_list << "_ALL_TARGETS";
                    data = QVariant::fromValue<CardUseStruct>(use);
                } else {
                    CardResponseStruct resp = data.value<CardResponseStruct>();
                    resp.m_isNullified = true;
                    data = QVariant::fromValue<CardResponseStruct>(resp);
                }
            } else {
                l.type = "#fuchou";
                if (triggerEvent == CardUsed) {
                    CardUseStruct use = data.value<CardUseStruct>();
                    if (use.m_addHistory) {
                        room->addPlayerHistory(invoke->targets.first(), c->getClassName(), -1);
                        use.m_addHistory = false;
                        data = QVariant::fromValue<CardUseStruct>(use);
                    }
                }
            }

            room->sendLog(l);
        }

        return false;
    }
};

class LingduVS : public OneCardViewAsSkill
{
public:
    LingduVS()
        : OneCardViewAsSkill("lingdu")
    {
        expand_pile = "#judging_area";
        response_pattern = "@@lingdu";
    }

    bool viewFilter(const Card *) const override
    {
        return true;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        return new DummyCard({originalCard->getEffectiveId()});
    }
};

class Lingdu : public TriggerSkill
{
public:
    Lingdu()
        : TriggerSkill("lingdu")
    {
        events << CardsMoveOneTime;
        view_as_skill = new LingduVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *current = room->getCurrent();
        if (current == nullptr || current->isDead() || current->getPhase() == Player::NotActive || current->hasFlag("lingdu"))
            return {};

        // move.from is filled with card user if the move is because of Use or Response.
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == nullptr || !move.from->hasSkill(this) || move.from->isDead() || (move.to_place != Player::DiscardPile))
            return {};

        QList<int> card_ids;

        for (int i = 0; i < move.card_ids.length(); ++i) {
            if (room->getCardPlace(move.card_ids.at(i)) != Player::DiscardPile)
                continue;

            Player::Place fromPlace = move.from_places.at(i);
            switch (fromPlace) {
            case Player::PlaceHand:
            case Player::PlaceEquip:
            case Player::PlaceDelayedTrick: {
                card_ids << move.card_ids.at(i);
                break;
            }
            case Player::PlaceTable: {
                if (((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE)
                    || ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE))
                    card_ids << move.card_ids.at(i);
                break;
            }
            default: {
                break;
            }
            }
        }

        if (card_ids.isEmpty())
            return {};

        ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
        if (from == nullptr || from->getPhase() != Player::NotActive)
            return {};

        SkillInvokeDetail d(this, from, from);
        d.tag["lingdu"] = IntList2VariantList(card_ids);
        return {d};
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        const Card *l = room->askForCard(invoke->invoker, "@@lingdu", "@lingdu-discard", invoke->tag.value("lingdu"), Card::MethodNone, nullptr, false, "lingdu");
        if (l != nullptr) {
            // need #InvokeSkill log? waiting for test result

            room->getCurrent()->setFlags("lingdu");
            QList<int> card_ids = VariantList2IntList(invoke->tag.value("lingdu").toList());
            int id = -1;
            if (card_ids.length() == 1)
                id = card_ids.first();
            else {
                room->fillAG(card_ids, invoke->invoker);
                id = room->askForAG(invoke->invoker, card_ids, false, "lingdu");
                room->clearAG(invoke->invoker);
                if (id == -1)
                    id = card_ids.first();
            }

            bool isLastCard = false;
            switch (room->getCardPlace(l->getEffectiveId())) {
            case Player::PlaceHand: {
                isLastCard = (invoke->invoker->getHandcardNum() == 1 && invoke->invoker->getHandcards().first()->getEffectiveId() == l->getEffectiveId());
                break;
            }
            case Player::PlaceEquip: {
                isLastCard = (invoke->invoker->getEquips().length() == 1 && invoke->invoker->getEquips().first()->getEffectiveId() == l->getEffectiveId());
                break;
            }
            case Player::PlaceDelayedTrick: {
                isLastCard = (invoke->invoker->getJudgingAreaID().length() == 1 && invoke->invoker->getJudgingAreaID().first() == l->getEffectiveId());
                break;
            }
            default: {
                break;
            }
            }

            int card1 = l->getEffectiveId();
            int card2 = id;
            invoke->tag["isLast"] = isLastCard;

            LogMessage l;
            l.type = "$lingdulost";
            l.from = invoke->invoker;
            l.card_str = QString::number(card1);
            l.arg = objectName();
            room->sendLog(l);

            CardsMoveStruct move1;
            move1.card_ids << card1;
            move1.to = nullptr;
            move1.to_place = Player::DiscardPile;
            move1.reason.m_reason = CardMoveReason::S_REASON_PUT;
            move1.reason.m_playerId = invoke->invoker->objectName();
            move1.reason.m_skillName = "lingdu";
            CardsMoveStruct move2;
            move2.card_ids << card2;
            move2.to = invoke->invoker;
            move2.to_place = Player::PlaceHand;
            move2.reason.m_reason = CardMoveReason::S_REASON_RECYCLE;
            move2.reason.m_playerId = invoke->invoker->objectName();
            move2.reason.m_skillName = "lingdu";

            room->moveCardsAtomic({move1, move2}, true);

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent /*triggerEvent*/, Room * /*room*/, QSharedPointer<SkillInvokeDetail> invoke, QVariant & /*data*/) const override
    {
        if (invoke->tag.value("isLast").toBool())
            invoke->invoker->drawCards(1, "lingdu");

        return false;
    }
};

class Duozhi : public TriggerSkill
{
public:
    Duozhi()
        : TriggerSkill("duozhi")
    {
        events << CardFinished << EventPhaseStart;
        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *c = data.value<ServerPlayer *>();
            if (c->getPhase() == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->isCardLimited("use,response", "duozhi"))
                        room->removePlayerCardLimitation(p, "use,response", ".$1", "duozhi", true);
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from != nullptr && use.from->isAlive() && use.from->hasSkill(this)) {
                ServerPlayer *current = room->getCurrent();
                if (current != nullptr && current->getPhase() != Player::NotActive && current->isAlive()) {
                    foreach (ServerPlayer *p, room->getOtherPlayers(use.from)) {
                        if (!p->isCardLimited("use,response", "duozhi"))
                            return {SkillInvokeDetail(this, use.from, use.from, nullptr, true)};
                    }
                }
            }
        }

        return {};
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
        LogMessage l;

        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (!p->isCardLimited("use,response", "duozhi")) {
                room->setPlayerCardLimitation(p, "use,response", ".", "duozhi", true);
                l.to << p;
            }
        }

        if (!l.to.isEmpty()) {
            l.type = "#duozhi";
            room->sendLog(l);
        }

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
            return to_select->isEquipped();
        } else if (pattern == "@@junzhen-card2") {
            return selected.isEmpty();
        }

        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
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
        events << EventPhaseEnd << Death << BuryVictim;
        view_as_skill = new JunzhenVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;
#ifdef JUNZHEN_DEATH
        if (triggerEvent == BuryVictim) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->tag.contains("junzhenTo"))
                    r << SkillInvokeDetail(this, nullptr, p, nullptr, true, nullptr, false);
            }
        } else
#endif
            if (triggerEvent == EventPhaseEnd
#ifdef JUNZHEN_DEATH
                || triggerEvent == Death
#endif
            ) {
            ServerPlayer *invoker = nullptr;
            if (triggerEvent == EventPhaseEnd) {
                ServerPlayer *c = data.value<ServerPlayer *>();
                if (c->getPhase() == Player::Play && c->isAlive())
                    invoker = c;
            }
#ifdef JUNZHEN_DEATH
            else if (triggerEvent == Death) {
                DeathStruct death = data.value<DeathStruct>();
                invoker = death.who;
            }
#endif
            if (invoker != nullptr && !invoker->isNude()) {
                foreach (ServerPlayer *owner, room->findPlayersBySkillName(objectName()))
                    r << SkillInvokeDetail(this, owner, invoker);
            }

        } else {
            // ??
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
#ifdef JUNZHEN_DEATH
        if (triggerEvent == BuryVictim) {
            return true;
        } else
#endif
            if (triggerEvent == EventPhaseEnd
#ifdef JUNZHEN_DEATH
                || triggerEvent == Death
#endif
            ) {
            room->setPlayerFlag(invoke->owner, "junzhen_owner");
            invoke->invoker->tag.remove("junzhenTo");
            QString prompt = (invoke->invoker == invoke->owner) ? QStringLiteral("@junzhen-prompt2") : (QString(QStringLiteral("@junzhen-prompt1:")) + invoke->owner->objectName());
            int noticeIndex = (invoke->invoker == invoke->owner) ? 2 : 1;
            bool invoked = room->askForUseCard(invoke->invoker, "@@junzhen-card1", prompt, noticeIndex, Card::MethodNone);
            if (!invoked)
                room->setPlayerFlag(invoke->owner, "-junzhen_owner");

            return invoked;
        } else {
            // ??
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (
#ifdef JUNZHEN_DEATH
            triggerEvent == BuryVictim ||
#endif
            triggerEvent == EventPhaseEnd) {
            ServerPlayer *target = invoke->invoker->tag["junzhenTo"].value<ServerPlayer *>();
            invoke->invoker->tag.remove("junzhenTo");

            if (target != nullptr)
                room->askForUseCard(target, "@@junzhen-card2", "@junzhen-prompt3", 3, Card::MethodUse);
        }

        return false;
    }
};

class Ciou : public TriggerSkill
{
public:
    Ciou()
        : TriggerSkill("ciou")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->isAlive() && damage.to->hasSkill(this)) {
            if (!(damage.card != nullptr
                  && (!damage.card->isVirtualCard()
                      || (damage.card->subcardsLength() == 1 && Sanguosha->getCard(damage.card->getEffectiveId())->getClassName() == damage.card->getClassName()))
                  && damage.nature == DamageStruct::Normal))
                return {SkillInvokeDetail(this, damage.to, damage.to, nullptr, true)};
        }

        return {};
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        LogMessage l;
        l.from = invoke->invoker;
        l.arg = objectName();
        l.type = "#micai01";
        l.arg2 = QString::number(1);
        damage.damage -= 1;

        room->sendLog(l);
        room->notifySkillInvoked(invoke->invoker, "ciou");
        data = QVariant::fromValue<DamageStruct>(damage);
        return damage.damage == 0;
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
        events << EventPhaseChanging << CardUsed;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() == Card::TypeBasic && use.from != nullptr && use.from->hasSkill("jinji") && use.from->isAlive())
                room->setPlayerFlag(use.from, "jinji_basic");
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
            if (c->getPhase() == Player::Play && c->isAlive() && c->hasSkill(this)) {
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
    saki->addSkill(new Tianxing);
    saki->addSkill(new SlashNoDistanceLimitSkill("tianxing"));
    related_skills.insertMulti("tianxing", "#tianxing-slash-ndl");

    General *mayumi = new General(this, "mayumi", "gxs");
    mayumi->addSkill(new Junzhen);
    mayumi->addSkill(new Ciou);

    General *yachie = new General(this, "yachie", "gxs");
    yachie->addSkill(new Duozhi);

    General *kutaka = new General(this, "kutaka", "gxs");
    kutaka->addSkill(new Yvshou);
    kutaka->addSkill(new Lingdu);

    General *urumi = new General(this, "urumi", "gxs");
    urumi->addSkill(new Lunni);
    urumi->addSkill(new Liaogu);

    General *eika = new General(this, "eika", "gxs", 3);
    eika->addSkill(new Shanlei);
    eika->addSkill(new Bengluo);

    addMetaObject<LunniCard>();
    addMetaObject<JunzhenCard>();

    skills << new LingshouOtherVS;
}

ADD_PACKAGE(TH17)
