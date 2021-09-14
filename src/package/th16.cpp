#include "th16.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "skill.h"
#include "testCard.h"

MishenCard::MishenCard()
{
    will_throw = false;
}

bool MishenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    LureTiger *lt = new LureTiger(Card::NoSuit, 0);
    DELETE_OVER_SCOPE(LureTiger, lt)
    return lt->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, lt);
}

void MishenCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    if (subcards.length() > 0)
        card_use.from->addBrokenEquips(subcards);
    else
        room->setPlayerProperty(card_use.from, "chained", !card_use.from->isChained());

    LureTiger *lt = new LureTiger(Card::NoSuit, 0);
    lt->setSkillName("mishen");
    if (lt->isAvailable(card_use.from) && !card_use.from->isProhibited(card_use.to.first(), lt)) {
        CardUseStruct u = card_use;
        u.card = lt;
        room->useCard(u);
    } else
        delete lt;
}

class Mishen : public ViewAsSkill
{
public:
    Mishen()
        : ViewAsSkill("mishen")
    {
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const override
    {
        if (selected.length() > 0)
            return false;
        return to_select->isEquipped(Self) && !Self->isBrokenEquip(to_select->getEffectiveId());
    }

    const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const override
    {
        if (Self->isChained() && cards.isEmpty())
            return nullptr;

        MishenCard *c = new MishenCard;
        c->addSubcards(cards);
        return c;
    }
};

class Liji : public ZeroCardViewAsSkill
{
public:
    static QStringList CardNameList;

    static bool canUse(const Player *from, const Player *to, const Card *c, int i = -1)
    {
        Q_UNUSED(i);
        if (from->getRoomObject()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            bool r = false;
            c->setFlags("IgnoreFailed");
            c->setFlags("houhu");
            r = c->isAvailable(from) && !from->isProhibited(to, c) && c->targetFilter(QList<const Player *>(), to, from);
            c->setFlags("-houhu");
            c->setFlags("-IgnoreFailed");
            return r;
        }
#if 0
        // todo? or won't do?
        else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            if (i == 2) {
                if (Sanguosha->getCurrentCardUsePattern().contains("peach")) {
                }
            } else if (i == 3) {
                // deal with AskForUseSlashTo
            }
        }
#endif
        return false;
    }

    Liji()
        : ZeroCardViewAsSkill("liji")
    {
    }

    const Card *viewAs(const Player * /*Self*/) const override
    {
        return new LijiCard;
    }
};

// beware!! this matches EquipCard::Location
QStringList Liji::CardNameList = {
    "known_both", "iron_chain", "peach", "fire_slash", "lure_tiger",
};

LijiCard::LijiCard()
{
}

bool LijiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (targets.length() > 0)
        return false;

    if ((Self != to_select) && ((Self->getNextAlive() == to_select) || (Self->getLastAlive() == to_select))) {
        for (int i = static_cast<int>(EquipCard::WeaponLocation); i <= static_cast<int>(EquipCard::TreasureLocation); ++i) {
            const EquipCard *equip = to_select->getEquip(i);
            if (equip != nullptr && !to_select->isBrokenEquip(equip->getId())) {
                Card *c = Self->getRoomObject()->cloneCard(Liji::CardNameList.at(i), Card::NoSuit, 0);
                if (Liji::canUse(Self, to_select, c, i))
                    return true;
            }
        }
    } else
        return false;

    return false;
}

void LijiCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    auto equips = card_use.to.first()->getEquips();
    QList<int> ids;
    foreach (auto c, equips) {
        int id = c->getEffectiveId();
        if (card_use.to.first()->isBrokenEquip(id))
            ids << id;
        else {
            int l = static_cast<int>(qobject_cast<const EquipCard *>(c->getRealCard())->location());
            Card *e = room->cloneCard(Liji::CardNameList.at(l), Card::NoSuit, 0);
            if (!Liji::canUse(card_use.from, card_use.to.first(), e, l))
                ids << id;
        }
    }

    if (ids.length() == equips.length())
        return;

    int id = room->askForCardChosen(card_use.from, card_use.to.first(), "e", "liji", false, Card::MethodNone, ids);
    card_use.to.first()->addBrokenEquips(QList<int>() << id);

    const EquipCard *e = qobject_cast<const EquipCard *>(room->getCard(id)->getRealCard());
    if (e == nullptr)
        return;

    Card *c = room->cloneCard(Liji::CardNameList.at(static_cast<int>(e->location())), Card::NoSuit, 0);
    if (c != nullptr) {
        c->setSkillName("liji");
        CardUseStruct cardUse = card_use;
        cardUse.card = c;
        room->useCard(cardUse);
    }
}

class Houguang : public TriggerSkill
{
public:
    Houguang()
        : TriggerSkill("houguang$")
    {
        events << CardFinished;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.from->hasLordSkill(this) && use.card != nullptr && !use.card->isKindOf("SkillCard")) {
            foreach (auto p, use.to) {
                if (p->getKingdom() == "tkz" && p->isAlive() && p != use.from && !p->isRemoved())
                    d << SkillInvokeDetail(this, use.from, use.from, nullptr, false, p);
            }
        }

        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (room->askForSkillInvoke(invoke->targets.first(), "houguanghide", "hide")) {
            room->touhouLogmessage("#Shenyin1", invoke->targets.first(), objectName(), QList<ServerPlayer *>());

            room->setPlayerCardLimitation(invoke->targets.first(), "use", ".", "lure_tiger", true);
            room->setPlayerProperty(invoke->targets.first(), "removed", true);
        }

        return false;
    }
};

MenfeiCard::MenfeiCard()
{
}

bool MenfeiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void MenfeiCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->gainMark("@door");
}

class MenfeiVS : public ZeroCardViewAsSkill
{
public:
    MenfeiVS()
        : ZeroCardViewAsSkill("menfei")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        if (player->getMark("@door") > 0)
            return false;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->getMark("@door") > 0)
                return false;
        }
        return true;
    }

    const Card *viewAs(const Player * /*Self*/) const override
    {
        return new MenfeiCard;
    }
};

class Menfei : public TriggerSkill
{
public:
    Menfei()
        : TriggerSkill("menfei")
    {
        events << CardFinished;
        view_as_skill = new MenfeiVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeSkill)
            return QList<SkillInvokeDetail>();

        ServerPlayer *player = use.from;

        if (player && player->isAlive() && player->hasSkill(this)) {
            ServerPlayer *target = nullptr;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getMark("@door") > 0) {
                    target = p;
                    break;
                }
            }
            if (target) {
                ServerPlayer *next = qobject_cast<ServerPlayer *>(target->getNextAlive(1));
                if (next && next != target) {
                    SkillInvokeDetail r(this, player, player, nullptr, true);
                    r.tag["door"] = QVariant::fromValue(next);
                    return QList<SkillInvokeDetail>() << r;
                }
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->getMark("@door") > 0) {
                room->setPlayerMark(p, "@door", 0);
                break;
            }
        }
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        ServerPlayer *next = invoke->tag["door"].value<ServerPlayer *>();
        if (next)
            next->gainMark("@door");
        return false;
    }
};

class Houhu : public TriggerSkill
{
public:
    Houhu()
        : TriggerSkill("houhu")
    {
        events << TargetSpecifying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeBasic || use.card->isNDTrick()) {
            if (use.card->isKindOf("Jink") || use.card->isKindOf("Nullification"))
                return QList<SkillInvokeDetail>();
            ServerPlayer *player = use.from;
            if (player && player->isAlive() && player->hasSkill(this)) {
                ServerPlayer *target = nullptr;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getMark("@door") > 0) {
                        target = p;
                        break;
                    }
                }
                if (target) {
                    if (use.to.contains(target)) {
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, nullptr, false, target);
                    } else if (!player->isProhibited(target, use.card)) {
                        use.card->setFlags("IgnoreFailed");
                        use.card->setFlags("houhu");
                        bool can = use.card->targetFilter(QList<const Player *>(), target, player);
                        if (use.card->isKindOf("Peach") && target->isWounded())
                            can = true;
                        use.card->setFlags("-houhu");
                        use.card->setFlags("-IgnoreFailed");
                        if (can)
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, nullptr, false, target);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.contains(invoke->targets.first())) {
            invoke->invoker->drawCards(1);
        } else {
            CardUseStruct use = data.value<CardUseStruct>();
            QList<ServerPlayer *> logto;
            logto << invoke->targets.first();
            room->touhouLogmessage("#houhu", invoke->invoker, use.card->objectName(), logto, objectName());

            use.to << invoke->targets.first();
            room->sortByActionOrder(use.to);
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

class HouhuDistance : public TargetModSkill
{
public:
    HouhuDistance()
        : TargetModSkill("houhu-dist")
    {
        pattern = "BasicCard,TrickCard";
    }

    int getDistanceLimit(const Player *, const Card *card) const override
    {
        if (card->hasFlag("houhu"))
            return 1000;

        return 0;
    }
};

class Diexing : public TriggerSkill
{
public:
    Diexing()
        : TriggerSkill("diexing")
    {
        events << HpChanged << CardsMoveOneTime;
        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *, QVariant &data) const override
    {
        if (triggerEvent == HpChanged) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p != nullptr) {
                if (!p->tag.contains("diexingHp"))
                    p->tag["diexingHp"] = p->getMaxHp(); //p->getGeneral()->getMaxHp();

                p->tag["diexingHp2"] = p->tag.value("diexingHp");
                p->tag["diexingHp"] = p->getHp();
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;
        if (triggerEvent == HpChanged) {
            ServerPlayer *player = data.value<ServerPlayer *>();

            if (player != nullptr && player->isAlive() && player->hasSkill(this)) {
                // judge if hp is deduced
                bool ok = false;
                if (player->tag.value("diexingHp2", -1).toInt(&ok) > player->getHp() && ok)
                    r << SkillInvokeDetail(this, player, player, nullptr, true);
            }
        } else {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != nullptr && move.from->isAlive() && move.from->hasSkill(this)
                && ((move.to != move.from) || (move.to_place == Player::PlaceTable) || (move.to_place == Player::DiscardPile) || (move.to_place == Player::DrawPile))) {
                int n = 0;
                for (int i = 0; i < move.card_ids.length(); ++i) {
                    if (((move.from_places.at(i) == Player::PlaceHand) || (move.from_places.at(i) == Player::PlaceEquip)))
                        ++n;
                }
                if (n >= 2) {
                    ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
                    if (player != nullptr)
                        r << SkillInvokeDetail(this, player, player, nullptr, true);
                }
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

                room->notifySkillInvoked(invoke->invoker, objectName());
            }

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == HpChanged)
            invoke->invoker->drawCards(2, objectName());
        else
            room->recover(invoke->invoker, RecoverStruct());

        return false;
    }
};

LinsaCard::LinsaCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void LinsaCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();

    // get the card detail before give
    Card::Suit suit = room->getCard(getSubcards().first())->getSuit();
    room->showCard(effect.from, getEffectiveId());

    CardMoveReason r(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), "linsa", QString());
    room->obtainCard(effect.to, this, r);

    if (suit == Card::Heart)
        room->loseHp(effect.to);
    else if (suit == Card::Spade) {
        LogMessage l;
        l.type = "#LinsaNullifyPre";
        l.from = effect.from;
        l.to << effect.to;
        room->sendLog(l);

        effect.to->tag["linsaNullifyFrom"] = QVariant::fromValue<ServerPlayer *>(effect.from);
    }
}

class LinsaVS : public OneCardViewAsSkill
{
public:
    LinsaVS()
        : OneCardViewAsSkill("linsa")
    {
        filter_pattern = ".|.|.|hand";
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("LinsaCard");
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
    {
        LinsaCard *c = new LinsaCard;
        c->addSubcard(originalCard);
        c->setShowSkill(objectName());
        return c;
    }
};

class Linsa : public TriggerSkill
{
public:
    Linsa()
        : TriggerSkill("linsa")
    {
        events << CardUsed << CardResponded << EventPhaseChanging;
        view_as_skill = new LinsaVS;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->tag.contains("linsaNullifyFrom")) {
                        ServerPlayer *from = p->tag.value("linsaNullifyFrom").value<ServerPlayer *>();
                        if (from != nullptr) {
                            LogMessage l;
                            l.type = "#LinsaNullifyPreRoundOver";
                            l.from = from;
                            l.to << p;
                            room->sendLog(l);
                        }

                        p->tag.remove("linsaNullifyFrom");
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *p = nullptr;
        CardUseStruct use;
        if (triggerEvent == CardUsed) {
            use = data.value<CardUseStruct>();
            if (use.card->getTypeId() == Card::TypeSkill)
                return QList<SkillInvokeDetail>();
            p = use.from;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card && resp.m_card->getTypeId() == Card::TypeSkill)
                return QList<SkillInvokeDetail>();
            if (resp.m_isUse) {
                use.from = resp.m_from;
                use.card = resp.m_card;
                use.m_isHandcard = resp.m_isHandcard;
                p = use.from;
            }
        }

        if (p != nullptr && use.m_isHandcard) {
            ServerPlayer *e = p->tag.value("linsaNullifyFrom").value<ServerPlayer *>();
            if (e != nullptr)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, e, p, nullptr, true, nullptr, false);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        const Card *nullifiedCard = nullptr;

        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.nullified_list << "_ALL_TARGETS";
            data = QVariant::fromValue<CardUseStruct>(use);
            nullifiedCard = use.card;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            resp.m_isNullified = true;
            data = QVariant::fromValue<CardResponseStruct>(resp);
            nullifiedCard = resp.m_card;
        }

        invoke->invoker->tag.remove("linsaNullifyFrom");

        if (nullifiedCard != nullptr) {
            LogMessage l;
            l.type = "#LinsaNullify";
            l.from = invoke->owner;
            l.to << invoke->invoker;
            l.arg = nullifiedCard->objectName();
            room->sendLog(l);
        }

        return false;
    }
};

class Shengyu : public TriggerSkill
{
public:
    Shengyu()
        : TriggerSkill("shengyu")
    {
        events << TargetSpecified << CardFinished;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetSpecified) {
            if (use.card != nullptr && (use.card->isKindOf("Slash") || use.card->isNDTrick()) && (use.card->isRed() || use.card->isBlack()) && use.from != nullptr
                && use.from->hasSkill(this) && use.from->isAlive()) {
                foreach (ServerPlayer *p, use.to) {
                    if (p != use.from)
                        r << SkillInvokeDetail(this, use.from, use.from, nullptr, true, p);
                }
            }
        } else {
            if (use.card != nullptr) {
                foreach (ServerPlayer *p, use.to) {
                    if (p->hasFlag("shengyu_" + use.card->toString()))
                        r << SkillInvokeDetail(this, use.from, use.from, nullptr, true, p, false);
                }
            }
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
            if (invoke->invoker->hasShownSkill(this) && triggerEvent == TargetSpecified) {
                LogMessage l;
                l.type = "#TriggerSkill";
                l.arg = objectName();
                l.from = invoke->invoker;
                room->sendLog(l);

                room->notifySkillInvoked(invoke->invoker, objectName());
            }
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString colorstring = "red";
        if (use.card->isBlack())
            colorstring = "black";
        else if (!use.card->isRed())
            return false;

        if (triggerEvent == TargetSpecified) {
            QList<int> ids;
            foreach (const Card *eq, invoke->targets.first()->getEquips()) {
                if (eq->sameColorWith(use.card) && !invoke->targets.first()->getBrokenEquips().contains(eq->getId()))
                    ids << eq->getId();
            }

            if (ids.isEmpty() || invoke->targets.first()->askForSkillInvoke("#shengyu-select", QString("mow:%1::%2").arg(invoke->invoker->objectName()).arg(colorstring))) {
                room->setPlayerCardLimitation(invoke->targets.first(), "use,response", ".|" + colorstring + "|.|.", "shengyu", false);
                invoke->targets.first()->setFlags("shengyu_" + use.card->toString());
            } else
                invoke->targets.first()->addBrokenEquips(ids);
        } else
            room->removePlayerCardLimitation(invoke->targets.first(), "use,response", ".|" + colorstring + "|.|.$0", "shengyu");

        return false;
    }
};

class Modao : public OneCardViewAsSkill
{
public:
    Modao()
        : OneCardViewAsSkill("modao")
    {
        filter_pattern = ".|club";
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        BoneHealing *bh = new BoneHealing(Card::SuitToBeDecided, -1);
        DELETE_OVER_SCOPE(BoneHealing, bh)
        return bh->isAvailable(player);
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        BoneHealing *card = new BoneHealing(Card::SuitToBeDecided, -1);
        DELETE_OVER_SCOPE(BoneHealing, card)
        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);

        return cardPattern != nullptr && cardPattern->match(player, card);
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
    {
        BoneHealing *bh = new BoneHealing(Card::SuitToBeDecided, -1);
        bh->addSubcard(originalCard);
        bh->setSkillName(objectName());
        bh->setShowSkill(objectName());
        return bh;
    }
};

class Xunfo : public TriggerSkill
{
public:
    Xunfo()
        : TriggerSkill("xunfo")
    {
        events << CardsMoveOneTime << HpRecover;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        bool triggerFlag = false;

        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DRAW) && move.reason.m_extraData.toString() != "xunfo"
                && move.reason.m_extraData.toString() != "initialDraw" && move.to->isLord())
                triggerFlag = true;
        } else {
            RecoverStruct recover = data.value<RecoverStruct>();
            if (recover.to->isLord())
                triggerFlag = true;
        }

        QList<SkillInvokeDetail> r;

        if (!triggerFlag)
            return r;

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->isAlive() && p->hasSkill(this) && p->getHandcardNum() < 5)
                r << SkillInvokeDetail(this, p, p, nullptr, true);
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
            if (invoke->invoker->hasShownSkill(this)) {
                LogMessage log;
                log.type = "#TriggerSkill";
                log.from = invoke->invoker;
                log.arg = objectName();
                room->sendLog(log);

                room->notifySkillInvoked(invoke->invoker, objectName());
            }
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->drawCards(1, "xunfo");
        return false;
    }
};

class ZhensheVS : public OneCardViewAsSkill
{
public:
    ZhensheVS()
        : OneCardViewAsSkill("zhenshe")
    {
        filter_pattern = ".|heart";
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasFlag("zhenshe");
    }

    /*bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        return false;
        SavingEnergy *card = new SavingEnergy(Card::SuitToBeDecided, -1);
        DELETE_OVER_SCOPE(SavingEnergy, card)
        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);

        return cardPattern != nullptr && cardPattern->match(player, card);
    }*/

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
    {
        SavingEnergy *se = new SavingEnergy(Card::SuitToBeDecided, -1);
        se->addSubcard(originalCard);
        se->setSkillName(objectName());
        se->setShowSkill(objectName());
        return se;
    }
};

class Zhenshe : public TriggerSkill
{
public:
    Zhenshe()
        : TriggerSkill("zhenshe")
    {
        events << PreCardUsed << EventPhaseChanging;
        view_as_skill = new ZhensheVS;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play && change.player->hasFlag("zhenshe")) {
                room->setPlayerFlag(change.player, "-zhenshe");
            }
        }

        if (e == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == objectName())
                room->setPlayerFlag(use.from, "zhenshe");
        }
    }
};

class Puti : public TriggerSkill
{
public:
    Puti()
        : TriggerSkill("puti")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                if (change.player->isCardLimited("use", "puti"))
                    room->removePlayerCardLimitation(change.player, "use", "Slash$1", "puti");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        if (e == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (!player->hasSkill(this) || player->isDead() || player->getPhase() != Player::Play)
                return QList<SkillInvokeDetail>();
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *player = invoke->invoker;
        int acquired = 0;
        QList<int> throwIds;
        while (acquired < 1) {
            int id = room->drawCard();
            CardsMoveStruct move(id, nullptr, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName()));
            move.reason.m_skillName = "puti";
            room->moveCardsAtomic(move, true);
            room->getThread()->delay();
            Card *card = room->getCard(id);
            if (card->getTypeId() == Card::TypeTrick) {
                acquired = acquired + 1;
                CardsMoveStruct move2(id, player, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_GOTBACK, player->objectName()));
                room->moveCardsAtomic(move2, true);

                if (!room->askForDiscard(player, "puti", 1, 1, true, false, "@puti-discard1"))
                    room->setPlayerCardLimitation(player, "use", "Slash", "puti", true);

                if (!throwIds.isEmpty()) {
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "sishu", QString());
                    DummyCard dummy(throwIds);
                    room->throwCard(&dummy, reason, nullptr);
                    throwIds.clear();
                }
            } else
                throwIds << id;
        }

        return false;
    }
};

class Zangfa : public TriggerSkill
{
public:
    Zangfa()
        : TriggerSkill("zangfa")
    {
        events << TargetSpecifying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isNDTrick() || use.card->isKindOf("Nullification"))
            return QList<SkillInvokeDetail>();

        bool invoke = false;
        use.card->setFlags("IgnoreFailed");
        use.card->setFlags("zangfa");
        foreach (ServerPlayer *q, room->getAlivePlayers()) {
            if (!use.to.contains(q) && !use.from->isProhibited(q, use.card) && use.card->targetFilter(QList<const Player *>(), q, use.from)) {
                invoke = true;
                break;
            }
        }
        use.card->setFlags("-zangfa");
        use.card->setFlags("-IgnoreFailed");
        if (!invoke)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        QList<ServerPlayer *> invokers = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *p, invokers) {
            if (p->isAlive() && (p == use.from || use.to.contains(p)))
                d << SkillInvokeDetail(this, p, p);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        QList<ServerPlayer *> listt;
        CardUseStruct use = data.value<CardUseStruct>();
        use.card->setFlags("IgnoreFailed");
        use.card->setFlags("zangfa");
        foreach (ServerPlayer *q, room->getAlivePlayers()) {
            if (!use.to.contains(q) && !use.from->isProhibited(q, use.card) && use.card->targetFilter(QList<const Player *>(), q, use.from))
                listt << q;
        }
        use.card->setFlags("-zangfa");
        use.card->setFlags("-IgnoreFailed");
        invoke->invoker->tag["zangfa_use"] = data;
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, listt, objectName(), "@zangfa:" + use.card->objectName(), true, true);
        //player->tag.remove("huanshi_source");
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.to << invoke->targets.first();
        room->sortByActionOrder(use.to);
        data = QVariant::fromValue(use);
        return false;
    }
};

class ZangfaDistance : public TargetModSkill
{
public:
    ZangfaDistance()
        : TargetModSkill("zangfa-dist")
    {
        pattern = "TrickCard";
    }

    int getDistanceLimit(const Player *, const Card *card) const override
    {
        if (card->hasFlag("zangfa"))
            return 1000;

        return 0;
    }
};

GakungWuCard::GakungWuCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool GakungWuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QStringList l = Self->property((m_skillName + "availability").toLatin1().constData()).toString().split(QLatin1Char('+');
    return targets.isEmpty() && l.contains(to_select->objectName());
}

void GakungWuCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    if (subcards.length() > 0)
        card_use.from->addBrokenEquips(subcards);
    else
        room->setPlayerProperty(card_use.from, "chained", !card_use.from->isChained());

    card_use.from->tag[m_skillName] = QVariant::fromValue<ServerPlayer *>(card_use.to.first());
}

GuwuCard::GuwuCard()
{
    m_skillName = "guwu";
}

class GakungWuVs : public ViewAsSkill
{
public:
    GakungWuVs(const QString &name, const QString &cardName)
        : ViewAsSkill(name)
        , cardName(cardName)
    {
        response_pattern = "@@" + name;
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const override
    {
        if (selected.length() > 0)
            return false;
        return to_select->isEquipped(Self) && !Self->isBrokenEquip(to_select->getEffectiveId());
    }

    const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const override
    {
        if (Self->isChained() && cards.isEmpty())
            return nullptr;

        Card *c = Self->getRoomObject()->cloneSkillCard(cardName);
        if (c != nullptr) {
            c->addSubcards(cards);
            return c;
        }

        return nullptr;
    }

private:
    QString cardName;
};

class GakungWu : public TriggerSkill
{
public:
    GakungWu(const QString &name, const QString &cardName)
        : TriggerSkill(name)
    {
        events << CardUsed; //TargetSpecifying;
        view_as_skill = new GakungWuVs(name, cardName);
    }

    virtual bool cardMatch(const Card *c) const = 0;

    bool canAddTarget(const CardUseStruct &use) const
    {
        bool flag = false;
        use.card->setFlags("IgnoreFailed");
        use.card->setFlags("xunshi");
        foreach (ServerPlayer *p, use.from->getRoom()->getAllPlayers()) {
            if (!use.to.contains(p) && use.card->isKindOf("Peach") && p->isWounded() && !use.from->isProhibited(p, use.card)) {
                flag = true;
                break;
            }
            if (!use.to.contains(p) && use.card->targetFilter(QList<const Player *>(), p, use.from) && !use.from->isProhibited(p, use.card)) {
                flag = true;
                break;
            }
        }
        use.card->setFlags("-xunshi");
        use.card->setFlags("-IgnoreFailed");

        return flag;
    }

    bool canchain(const Player *p) const
    {
        if (!p->isChained())
            return true;

        foreach (const Card *c, p->getEquips()) {
            const EquipCard *ec = qobject_cast<const EquipCard *>(c->getRealCard());
            if (ec != nullptr && !p->isBrokenEquip(ec->getId()))
                return true;
        }

        return false;
    }

    QStringList extraTargetNames(const CardUseStruct &use) const
    {
        QStringList r;

        use.card->setFlags("IgnoreFailed");
        use.card->setFlags("xunshi");
        foreach (ServerPlayer *p, use.from->getRoom()->getAllPlayers()) {
            if (!use.to.contains(p) && use.card->targetFilter(QList<const Player *>(), p, use.from) && !use.from->isProhibited(p, use.card))
                r << p->objectName();
            else if (!use.to.contains(p) && use.card->isKindOf("Peach") && p->isWounded() && !use.from->isProhibited(p, use.card)) {
                r << p->objectName();
            }
        }
        use.card->setFlags("-xunshi");
        use.card->setFlags("-IgnoreFailed");

        return r;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();
        if (room->getCurrent() == use.from && use.from->isAlive() && use.from->getPhase() != Player::NotActive && use.card != nullptr && !use.card->isKindOf("SkillCard")
            && cardMatch(use.card) && canAddTarget(use)) {
            foreach (ServerPlayer *p, use.to) {
                if (use.from->getNextAlive() == p || use.from->getLastAlive() == p) {
                    foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                        if (canchain(p))
                            d << SkillInvokeDetail(this, p, p);
                    }
                }
            }
        }

        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QStringList l = extraTargetNames(use);
        room->setPlayerProperty(invoke->invoker, (objectName() + "availability").toLatin1().constData(), l.join('+'));
        invoke->invoker->tag[objectName()] = data; //for ai
        if (room->askForUseCard(invoke->invoker, "@@" + objectName(), "@" + objectName() + "-invoke", -1, Card::MethodNone, true, objectName())) {
            invoke->targets << invoke->invoker->tag[objectName()].value<ServerPlayer *>();
            invoke->invoker->tag.remove(objectName());
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.to.append(invoke->targets);
        room->sortByActionOrder(use.to);
        data = QVariant::fromValue<CardUseStruct>(use);

        //log notice
        room->touhouLogmessage("#InvokeSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());

        foreach (ServerPlayer *p, invoke->targets)
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), p->objectName());

        LogMessage alog;
        alog.type = "$Kuangwu";
        alog.from = use.from;
        alog.card_str = use.card->toString();
        alog.to = invoke->targets;
        alog.arg = objectName();
        room->sendLog(alog);

        return false;
    }
};

class Guwu : public GakungWu
{
public:
    Guwu()
        : GakungWu("guwu", "GuwuCard")
    {
    }

    bool cardMatch(const Card *c) const override
    {
        return c->isNDTrick();
    }
};

MiZhiungHteiCard::MiZhiungHteiCard()
{
}

bool MiZhiungHteiCard::targetFilter(const QList<const Player *> &, const Player *to_select, const Player *Self) const
{
    AllianceFeast *af = new AllianceFeast(Card::NoSuit, 0);
    DELETE_OVER_SCOPE(AllianceFeast, af)
    af->setSkillName(m_skillName);
    return !Self->isProhibited(to_select, af);
}

void MiZhiungHteiCard::onUse(Room *, const CardUseStruct &card_use) const
{
    AllianceFeast *af = new AllianceFeast(Card::NoSuit, 0);
    af->setSkillName(m_skillName);
    CardUseStruct u = card_use;
    u.card = af;
    card_use.from->tag[m_skillName] = QVariant::fromValue<CardUseStruct>(u);
}

MingheCard::MingheCard()
{
    m_skillName = "minghe";
}

class MiZhiungHteiVs : public ZeroCardViewAsSkill
{
public:
    MiZhiungHteiVs(const QString &name, const QString &cardName)
        : ZeroCardViewAsSkill(name)
        , cardName(cardName)
    {
        response_pattern = "@@" + name;
    }

    const Card *viewAs(const Player *Self) const override
    {
        return Self->getRoomObject()->cloneSkillCard(cardName);
    }

private:
    QString cardName;
};

class MiZhiungHtei : public TriggerSkill
{
public:
    MiZhiungHtei(const QString &name, const QString &cardName)
        : TriggerSkill(name)
    {
        events << Damaged << PostCardEffected;
        view_as_skill = new MiZhiungHteiVs(name, cardName);
    }

    virtual void afEffect(Room *room, ServerPlayer *target) const = 0;

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->hasSkill(this) && damage.to->isAlive()) {
                AllianceFeast *af = new AllianceFeast(Card::NoSuit, 0);
                DELETE_OVER_SCOPE(AllianceFeast, af)
                if (af->isAvailable(damage.to))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
            }
        } else {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card != nullptr && effect.card->isKindOf("AllianceFeast") && effect.card->getSkillName() == objectName() && effect.from != nullptr
                && !effect.card->hasFlag("MiZhiungHteiUneffected_" + effect.to->objectName()) && effect.to->getHp() > effect.from->getHp())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.from, effect.from, effect.to, true, nullptr, false);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == Damaged)
            return room->askForUseCard(invoke->invoker, "@@" + objectName(), "@" + objectName() + "-invoke", -1, Card::MethodUse, true, objectName());
        else
            return true;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == Damaged) {
            CardUseStruct use = invoke->invoker->tag[objectName()].value<CardUseStruct>();
            room->useCard(use);
        } else
            afEffect(room, invoke->targets.first());

        return false;
    }
};

class Minghe : public MiZhiungHtei
{
public:
    Minghe()
        : MiZhiungHtei("minghe", "MingheCard")
    {
    }

    void afEffect(Room *room, ServerPlayer *target) const override
    {
        room->setPlayerCardLimitation(target, "use", ".", objectName(), true);
    }
};

KuangwuCard::KuangwuCard()
{
    m_skillName = "kuangwu";
}

class Kuangwu : public GakungWu
{
public:
    Kuangwu()
        : GakungWu("kuangwu", "KuangwuCard")
    {
    }

    bool cardMatch(const Card *c) const override
    {
        return c->isKindOf("BasicCard");
    }
};

ZhutiCard::ZhutiCard()
{
    m_skillName = "zhuti";
}

class Zhuti : public MiZhiungHtei
{
public:
    Zhuti()
        : MiZhiungHtei("zhuti", "ZhutiCard")
    {
    }

    void afEffect(Room *room, ServerPlayer *target) const override
    {
        room->loseHp(target);
    }
};

HuazhaoCard::HuazhaoCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    target_fixed = true;
}

void HuazhaoCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->tag["huazhaoid"] = getEffectiveId();
}

class HuazhaoVS : public OneCardViewAsSkill
{
public:
    HuazhaoVS()
        : OneCardViewAsSkill("huazhao")
    {
        response_pattern = "@@huazhao";
        filter_pattern = ".|.|.|spring";
        expand_pile = "spring";
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
    {
        HuazhaoCard *c = new HuazhaoCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class Huazhao : public TriggerSkill
{
public:
    Huazhao()
        : TriggerSkill("huazhao")
    {
        events << AfterDrawInitialCards << EventPhaseStart;
        view_as_skill = new HuazhaoVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;

        ServerPlayer *target = nullptr;
        if (triggerEvent == AfterDrawInitialCards) {
            DrawNCardsStruct st = data.value<DrawNCardsStruct>();
            if (st.isInitial)
                target = st.player;
        } else {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Start)
                target = player;
        }

        if (target != nullptr && target->isAlive() && target->hasSkill(this))
            r << SkillInvokeDetail(this, target, target);

        return r;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (invoke->invoker->getPile("spring").isEmpty()) {
            if (invoke->invoker->askForSkillInvoke(this))
                return true;
        } else {
            if (room->askForUseCard(invoke->invoker, "@@huazhao", "@huazhao", -1, Card::MethodNone, true, objectName()))
                return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (invoke->invoker->getPile("spring").isEmpty()) {
            invoke->invoker->addToPile("spring", room->getNCards(4), false);
        } else {
            bool ok = false;
            int id = invoke->invoker->tag["huazhaoid"].toInt(&ok);
            const Card *card = room->getCard(id);
            if (ok && card != nullptr)
                invoke->invoker->obtainCard(card, false);

            invoke->invoker->tag.remove("huazhaoid");
        }

        return false;
    }
};

ChuntengCard::ChuntengCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void ChuntengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->showCard(source, getEffectiveId());
}

Chunteng2Card::Chunteng2Card()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "_chunteng";
}

void Chunteng2Card::onEffect(const CardEffectStruct &effect) const
{
    CardMoveReason r(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "huazhao", QString());
    Room *room = effect.from->getRoom();
    room->obtainCard(effect.to, this, r, false);

    if (effect.to->getHandcardNum() > effect.from->getPile("spring").length()) {
        if (!room->askForDiscard(effect.to, "chunteng", 1, 1, true, true, "@chunteng-discard")) {
            DummyCard d;
            d.addSubcards(effect.from->getPile("spring"));
            CardMoveReason r2(CardMoveReason::S_REASON_PUT, effect.to->objectName(), QString(), "chunteng", QString());
            room->throwCard(&d, r2, nullptr);
        }
    }
}

class ChuntengVS : public OneCardViewAsSkill
{
public:
    ChuntengVS()
        : OneCardViewAsSkill("chunteng")
    {
        expand_pile = "spring";
    }

    bool viewFilter(const Card *to_select, const Player *Self) const override
    {
        if (Self->getPile("spring").contains(to_select->getId())) {
            if (Self->getRoomObject()->getCurrentCardUsePattern() == "@@chunteng-card2!")
                return true;

            bool ok = false;
            Card::Suit s = static_cast<Card::Suit>(Self->property("chunteng1").toString().toInt(&ok));
            if (ok)
                return to_select->getSuit() == s;
        }

        return false;
    }

    const Card *viewAs(const Card *originalCard, const Player *Self) const override
    {
        Card *c = nullptr;
        if (Self->getRoomObject()->getCurrentCardUsePattern() == "@@chunteng-card1") {
            c = new ChuntengCard;
            c->setSkillName(objectName());
        } else {
            c = new Chunteng2Card;
            c->setSkillName("_chunteng");
        }

        c->addSubcard(originalCard);

        return c;
    }

    bool isEnabledAtPlay(const Player *) const override
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const override
    {
        return pattern.startsWith("@@chunteng-card");
    }
};

class Chunteng : public TriggerSkill
{
public:
    Chunteng()
        : TriggerSkill("chunteng")
    {
        events << CardUsed << CardResponded;
        view_as_skill = new ChuntengVS;
        related_pile = "spring";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct u;

        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            u.card = use.card;
            u.from = use.from;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse) {
                u.card = resp.m_card;
                u.from = resp.m_from;
            }
        }

        if (u.card != nullptr && !u.card->isKindOf("SkillCard") && u.from != nullptr && u.from->isAlive() && u.from->hasSkill(this) && !u.from->getPile("spring").isEmpty()) {
            SkillInvokeDetail d(this, u.from, u.from);
            d.tag["u"] = QVariant::fromValue<CardUseStruct>(u);

            return QList<SkillInvokeDetail>() << d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        CardUseStruct u = invoke->tag["u"].value<CardUseStruct>();
        room->setPlayerProperty(u.from, "chunteng1", QString::number(static_cast<int>(u.card->getSuit())));
        return room->askForUseCard(u.from, "@@chunteng-card1", "@chunteng1:::" + u.card->getSuitString(), -1, Card::MethodNone, true, objectName());
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        CardUseStruct u = invoke->tag["u"].value<CardUseStruct>();
        if (!room->askForUseCard(u.from, "@@chunteng-card2!", "@chunteng2", -1, Card::MethodNone, true, "_chunteng")) {
            QList<ServerPlayer *> p = room->getAllPlayers();
            p.removeAll(u.from);
            ServerPlayer *target = p[std::random_device()() % p.length()];

            Card *c = new Chunteng2Card;
            c->setSkillName("_chunteng");
            c->addSubcard(u.from->getPile("spring")[std::random_device()() % u.from->getPile("spring").length()]);

            CardUseStruct newUse(c, u.from, target);
            room->useCard(newUse);
        }

        return false;
    }
};

TH16Package::TH16Package()
    : Package("th16")
{
    General *okina = new General(this, "okina$", "tkz", 4);
    okina->addSkill(new Mishen);
    okina->addSkill(new Liji);
    okina->addSkill(new Houguang);

    General *eternity = new General(this, "eternity", "tkz", 3);
    eternity->addSkill(new Diexing);
    eternity->addSkill(new Linsa);

    General *nemuno = new General(this, "nemuno", "tkz", 4);
    nemuno->addSkill(new Shengyu);
    nemuno->addSkill(new Modao);

    General *aun = new General(this, "aun", "tkz", 3);
    aun->addSkill(new Xunfo);
    aun->addSkill(new Zhenshe);

    General *narumi = new General(this, "narumi", "tkz");
    narumi->addSkill(new Puti);
    narumi->addSkill(new Zangfa);

    General *satono = new General(this, "satono", "tkz", 3);
    satono->addSkill(new Guwu);
    satono->addSkill(new Minghe);

    General *mai = new General(this, "mai", "tkz", 3);
    mai->addSkill(new Kuangwu);
    mai->addSkill(new Zhuti);

    General *okinasp = new General(this, "okina_sp", "tkz");
    okinasp->addSkill(new Menfei);
    okinasp->addSkill(new Houhu);

    General *lili = new General(this, "lilywhite_sp", "tkz");
    lili->addSkill(new Chunteng);
    lili->addSkill(new Huazhao);

    addMetaObject<MishenCard>();
    addMetaObject<LijiCard>();
    addMetaObject<MenfeiCard>();
    addMetaObject<HuazhaoCard>();
    addMetaObject<ChuntengCard>();
    addMetaObject<Chunteng2Card>();
    addMetaObject<LinsaCard>();
    addMetaObject<GuwuCard>();
    addMetaObject<MingheCard>();
    addMetaObject<KuangwuCard>();
    addMetaObject<ZhutiCard>();

    skills << new HouhuDistance << new ZangfaDistance;
}

ADD_PACKAGE(TH16)
