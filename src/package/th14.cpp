#include "th14.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"

class Baochui : public TriggerSkill
{
public:
    Baochui()
        : TriggerSkill("baochui")
    {
        events << EventPhaseStart;
        ;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Start && player->getHandcardNum() < 3 && player->isAlive()) {
            foreach (ServerPlayer *src, room->findPlayersBySkillName(objectName())) {
                if (src->canDiscard(src, "hes"))
                    d << SkillInvokeDetail(this, src, src, NULL, false, player);
            }
        } else if (player->getPhase() == Player::Discard && player->getHandcardNum() < 3 && player->hasFlag(objectName())) {
            d << SkillInvokeDetail(this, NULL, player, NULL, true);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Start) {
            return room->askForCard(invoke->invoker, ".|.|.|hand,equipped", "@baochui:" + player->objectName(), QVariant::fromValue(player), Card::MethodDiscard, NULL, false,
                                    objectName());
        }
        return true;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Start) {
            player->drawCards(3 - player->getHandcardNum());
            room->setPlayerFlag(player, objectName());
        } else if (player->getPhase() == Player::Discard) {
            room->setPlayerFlag(player, "-" + objectName());
            room->touhouLogmessage("#BaochuiBuff", player, objectName(), QList<ServerPlayer *>(), objectName());
            room->loseHp(player, 1);
        }
        return false;
    }
};

class Yicun : public TriggerSkill
{
public:
    Yicun()
        : TriggerSkill("yicun")
    {
        frequency = Compulsory;
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->isKindOf("Slash") && use.from->isAlive()) {
            foreach (ServerPlayer *p, use.to) {
                if (p->hasSkill(this) && use.from->getHandcardNum() > p->getHandcardNum())
                    d << SkillInvokeDetail(this, p, p, NULL, true);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        use.nullified_list << invoke->invoker->objectName();
        data = QVariant::fromValue(use);
        return false;
    }
};

class Moyi : public TriggerSkill
{
public:
    Moyi()
        : TriggerSkill("moyi$")
    {
        events << EventPhaseEnd << CardsMoveOneTime << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            ServerPlayer *current = room->getCurrent();
            if (!current || current->getKingdom() != "hzc" || current->getPhase() != Player::Discard)
                return;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile) {
                QVariantList ids = current->tag["moyi_basics"].toList();
                foreach (int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->isKindOf("BasicCard") && !ids.contains(id))
                        ids << id;
                }
                current->tag["moyi_basics"] = ids;
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Discard)
                change.player->tag.remove("moyi_basics");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            ;
            if (!current || current->getKingdom() != "hzc" || current->getPhase() != Player::Discard || !current->isAlive())
                return d;

            bool invoke = false;
            QVariantList ids = current->tag["moyi_basics"].toList();
            foreach (QVariant card_data, ids) {
                if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile) {
                    invoke = true;
                    break;
                }
            }
            if (!invoke)
                return d;

            foreach (ServerPlayer *p, room->getOtherPlayers(current)) {
                if (p->hasLordSkill(objectName()))
                    d << SkillInvokeDetail(this, p, current);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->tag["moyi-target"] = QVariant::fromValue(invoke->owner);
        if (invoke->invoker->askForSkillInvoke(objectName(), QVariant::fromValue(invoke->owner))) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(invoke->owner, objectName());
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = invoke->invoker;
            log.to << invoke->owner;
            log.arg = objectName();
            room->sendLog(log);

            QVariantList ids = invoke->invoker->tag["moyi_basics"].toList();
            QList<int> all;
            foreach (QVariant card_data, ids) {
                if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
                    all << card_data.toInt();
            }

            room->fillAG(all, invoke->invoker);
            int moyiId = room->askForAG(invoke->invoker, all, false, objectName());
            room->clearAG(invoke->invoker);
            all.removeOne(moyiId);
            invoke->invoker->tag["moyi_basics"] = IntList2VariantList(all);
            invoke->invoker->tag["moyi_id"] = QVariant::fromValue(moyiId);
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        int moyiId = invoke->invoker->tag["moyi_id"].toInt();
        invoke->invoker->tag.remove("moyi_id");
        room->obtainCard(invoke->owner, moyiId, true);
        return false;
    }
};

LeitingCard::LeitingCard()
{
}

void LeitingCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    effect.from->drawCards(2);
    if (effect.from->isKongcheng())
        return;
    QList<const Card *> hc = effect.from->getHandcards();
    foreach (const Card *c, hc) {
        if (effect.from->isJilei(c))
            hc.removeOne(c);
    }
    if (hc.length() == 0) {
        // jilei show all cards
        room->doJileiShow(effect.from, effect.from->handCards());
        return;
    }

    const Card *cards = room->askForCard(effect.from, ".|.|.|hand!", "@leiting:" + effect.to->objectName(), QVariant::fromValue(effect.to), Card::MethodDiscard);
    if (!cards) {
        //force discard!!!
        int x = qrand() % hc.length();
        cards = hc.value(x);
        room->throwCard(cards, effect.from);
    }

    if (cards->getSuit() == Card::Heart) {
        effect.to->drawCards(1);
        room->damage(DamageStruct("leiting", NULL, effect.to, 1, DamageStruct::Thunder));
    } else if (cards->getSuit() == Card::Spade) {
        ThunderSlash *slash = new ThunderSlash(Card::NoSuit, 0);
        // if return without usecard,we need delete this new thunderslash?
        if (effect.to->isCardLimited(slash, Card::MethodUse))
            return;
        QList<ServerPlayer *> listt;
        foreach (ServerPlayer *p, room->getOtherPlayers(effect.to)) {
            if (effect.to->inMyAttackRange(p) && effect.to->canSlash(p, slash, true))
                listt << p;
        }
        if (listt.length() <= 0)
            return;
        ServerPlayer *target = room->askForPlayerChosen(effect.to, listt, "leiting", "@leiting_chosen:" + effect.from->objectName(), false);

        if (target != NULL) {
            slash->setSkillName("_leiting");
            room->useCard(CardUseStruct(slash, effect.to, target), false);
        }
    }
}

class Leiting : public ZeroCardViewAsSkill
{
public:
    Leiting()
        : ZeroCardViewAsSkill("leiting")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("LeitingCard");
    }

    virtual const Card *viewAs() const
    {
        return new LeitingCard;
    }
};

class Nizhuan : public TriggerSkill
{
public:
    Nizhuan()
        : TriggerSkill("nizhuan")
    {
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->isKindOf("Slash") && use.to.length() == 1) {
            ServerPlayer *target = use.to.first();
            if (use.from == NULL || use.from->isDead() || use.from->getLostHp() >= target->getLostHp())
                return d;

            foreach (ServerPlayer *src, room->findPlayersBySkillName(objectName())) {
                if (src->canDiscard(target, "hs")) {
                    Slash *slash = new Slash(Card::NoSuit, 0);
                    slash->setSkillName("_" + objectName());
                    slash->deleteLater();
                    if (!target->isCardLimited(slash, Card::MethodUse) && target->canSlash(use.from, slash, false))
                        d << SkillInvokeDetail(this, src, src);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *target = use.to.first();
        invoke->invoker->tag["nizhuan_carduse"] = data;
        if (target == invoke->invoker) {
            if (room->askForCard(target, ".", "@nizhuan-self:" + use.from->objectName(), data, objectName()))
                return true;
        } else {
            QString prompt = "target:" + use.from->objectName() + ":" + target->objectName();
            if (invoke->invoker->askForSkillInvoke(objectName(), prompt)) {
                invoke->invoker->showHiddenSkill(objectName());
                int id = room->askForCardChosen(invoke->invoker, target, "hs", objectName(), false, Card::MethodDiscard);
                room->throwCard(id, target, invoke->invoker);
                return true;
            }
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *target = use.to.first();
        use.nullified_list << target->objectName();
        data = QVariant::fromValue(use);

        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_" + objectName());
        room->useCard(CardUseStruct(slash, target, use.from), false);
        return false;
    }
};

class Guizha : public TriggerSkill
{
public:
    Guizha()
        : TriggerSkill("guizha")
    {
        events << AskForPeaches;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.who->hasSkill(this) && !dying.nowAskingForPeaches->isKongcheng() && dying.nowAskingForPeaches != dying.who)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dying.who, dying.who, NULL, true, dying.nowAskingForPeaches);
        return QList<SkillInvokeDetail>();
    }

    static int guizhaPeach(ServerPlayer *player, ServerPlayer *victim)
    {
        int peachId = -1; //force to use
        foreach (const Card *card, player->getHandcards()) {
            if (card->isKindOf("Peach")) {
                if (!player->isCardLimited(card, Card::MethodUse) && !player->isProhibited(victim, card)) {
                    peachId = card->getEffectiveId();
                    break;
                }
            }
        }
        Room *room = player->getRoom();
        room->touhouLogmessage("#TriggerSkill", victim, "guizha");
        room->notifySkillInvoked(victim, "guizha");
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, victim->objectName(), player->objectName());

        room->showAllCards(player);
        room->getThread()->delay(1000);
        room->clearAG();
        return peachId;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = invoke->targets.first();
        ServerPlayer *victim = invoke->invoker;

        int peachId = guizhaPeach(player, victim);

        while (peachId > -1 && victim->getHp() < victim->dyingThreshold()) {
            const Card *supply_card = room->askForCard(player, "Peach|.|.|hand!", "@guizha:" + victim->objectName(), data, Card::MethodUse, victim, false, objectName(), false);
            //force to supply!
            peachId = (supply_card != NULL) ? supply_card->getEffectiveId() : peachId;
            Peach *peach = new Peach(Card::SuitToBeDecided, -1);
            peach->addSubcard(peachId);
            peach->setSkillName("_guizha");
            room->useCard(CardUseStruct(peach, player, victim), false);
            if (victim->getHp() >= victim->dyingThreshold()) {
                room->setPlayerFlag(victim, "-Global_Dying");
                return true; //avoid triggering askforpeach
            }

            peachId = guizhaPeach(player, victim);
        }

        return false;
    }
};

class Yuyin : public TriggerSkill
{
public:
    Yuyin()
        : TriggerSkill("yuyin")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->hasSkill(this) && damage.to->isAlive()) {
            foreach (ServerPlayer *p, room->getOtherPlayers(damage.to)) {
                if (p->getHp() > damage.to->getHp() && !p->isNude())
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        QList<ServerPlayer *> listt;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getHp() > player->getHp() && !p->isNude())
                listt << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@" + objectName(), true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hes", objectName());
        room->obtainCard(invoke->invoker, id, room->getCardPlace(id) != Player::PlaceHand);
        return false;
    }
};

class Wuchang : public TriggerSkill
{
public:
    Wuchang()
        : TriggerSkill("wuchang")
    {
        events << EventPhaseEnd << CardsMoveOneTime << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            ServerPlayer *current = room->getCurrent();
            if (!current || current->getPhase() != Player::Discard)
                return;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == current && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                foreach (int id, move.card_ids) {
                    if ((move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand) && Sanguosha->getCard(id)->isRed()) {
                        room->setTag("wuchang", false);
                        break;
                    }
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Discard)
                room->setTag("wuchang", true);
            if (change.to == Player::Discard)
                room->setTag("wuchang", true);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (!current || current->getPhase() != Player::Discard || !room->getTag("wuchang").toBool())
                return d;
            if (current->canDiscard(current, "hs")) {
                foreach (ServerPlayer *src, room->findPlayersBySkillName(objectName())) {
                    if (src != current)
                        d << SkillInvokeDetail(this, src, src, NULL, false, current);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString prompt = "target:" + invoke->preferredTarget->objectName();
        return room->askForSkillInvoke(invoke->invoker, objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        room->askForDiscard(invoke->targets.first(), objectName(), 1, 1, false, false, "wuchang_discard");
        return false;
    }
};

class Canxiang : public TriggerSkill
{
public:
    Canxiang()
        : TriggerSkill("canxiang")
    {
        events << Damage;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive() && damage.from->hasSkill(this)) {
            foreach (ServerPlayer *p, room->getOtherPlayers(damage.from)) {
                if (p->getHp() > damage.from->getHp() && !p->isNude())
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        QList<ServerPlayer *> listt;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getHp() > player->getHp() && !p->isNude())
                listt << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@" + objectName(), true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hes", objectName());
        room->obtainCard(invoke->invoker, id, room->getCardPlace(id) != Player::PlaceHand);
        return false;
    }
};

class Juwang : public TriggerSkill
{
public:
    Juwang()
        : TriggerSkill("juwang")
    {
        events << CardAsked;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *current = room->getCurrent();
        CardAskedStruct s = data.value<CardAskedStruct>();
        if (!current || !current->isAlive() || current == s.player || !s.player->hasSkill(this))
            return QList<SkillInvokeDetail>();

        if (matchAvaliablePattern("jink", s.pattern)) {
            Jink *jink = new Jink(Card::NoSuit, 0);
            jink->deleteLater();
            if (s.player->isCardLimited(jink, s.method))
                return QList<SkillInvokeDetail>();
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, s.player, s.player, NULL, false, current);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        return invoke->invoker->askForSkillInvoke(objectName(), "throw:" + invoke->preferredTarget->objectName());
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        const Card *card
            = room->askForCard(invoke->targets.first(), ".|red|.|hand", "@juwang:" + invoke->invoker->objectName(), data, Card::MethodDiscard, NULL, false, objectName());
        if (card == NULL)
            room->damage(DamageStruct(objectName(), invoke->invoker, invoke->targets.first(), 1));
        return false;
    }
};

class Langying : public TriggerSkill
{
public:
    Langying()
        : TriggerSkill("langying")
    {
        events << CardAsked;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardAskedStruct s = data.value<CardAskedStruct>();
        if (s.pattern == "jink" && s.player->hasSkill(this) && s.player->getEquips().length() > 0) {
            Jink *jink = new Jink(Card::NoSuit, 0);
            jink->deleteLater();
            if (s.player->isCardLimited(jink, s.method))
                return QList<SkillInvokeDetail>();

            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, s.player, s.player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        return invoke->invoker->askForSkillInvoke(objectName(), data);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        QList<int> equips;
        foreach (const Card *e, (player->getEquips()))
            equips << e->getId();

        CardsMoveStruct move;
        move.card_ids = equips;
        move.from_place = Player::PlaceEquip;
        move.to_place = Player::PlaceHand;
        move.from = player;
        move.to = player;
        room->moveCardsAtomic(move, true);

        Jink *card = new Jink(Card::NoSuit, 0);
        card->setSkillName("_langying");
        room->provide(card);
        return true;
    }
};

YuanfeiCard::YuanfeiCard()
{
}

bool YuanfeiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (getSubcards().isEmpty())
        return targets.isEmpty() && !Self->inMyAttackRange(to_select) && to_select != Self;
    else
        return targets.isEmpty() && Self->inMyAttackRange(to_select) && to_select != Self;
}

void YuanfeiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    ServerPlayer *target = effect.to;
    room->setPlayerCardLimitation(target, "use,response", ".|.|.|.", "yuanfei", true);
    room->touhouLogmessage("#yuanfei", target, "yuanfei");
}

class Yuanfei : public ViewAsSkill
{
public:
    Yuanfei()
        : ViewAsSkill("yuanfei")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("YuanfeiCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() >= 1)
            return false;

        if (to_select->isEquipped())
            return false;

        if (Self->isJilei(to_select))
            return false;

        return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 0)
            return new YuanfeiCard;
        if (cards.length() != 1)
            return NULL;

        YuanfeiCard *card = new YuanfeiCard;
        card->addSubcards(cards);

        return card;
    }
};

class FeitouVS : public OneCardViewAsSkill
{
public:
    FeitouVS()
        : OneCardViewAsSkill("feitou")
    {
        filter_pattern = ".|.|.|feitou";
        expand_pile = "feitou";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        //need check limitation?
        return player->getPile("feitou").length() > 0;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return matchAvaliablePattern("slash", pattern) && player->getPile("feitou").length() > 0;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName("feitou");
            return slash;
        } else
            return NULL;
    }
};

class Feitou : public TriggerSkill
{
public:
    Feitou()
        : TriggerSkill("feitou")
    {
        events << EventPhaseStart << PreCardUsed;
        view_as_skill = new FeitouVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        //clear histroy
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.m_addHistory && use.card->getSkillName() == objectName()) {
                room->addPlayerHistory(use.from, use.card->getClassName(), -1);
                use.m_addHistory = false;
                data = QVariant::fromValue(use);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent != EventPhaseStart)
            return QList<SkillInvokeDetail>();
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Finish && player->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        //invoke->invoker->addToPile("feitou", room->getNCards(1));
        invoke->invoker->drawCards(1);
        if (!invoke->invoker->isKongcheng()) {
            const Card *cards = room->askForExchange(invoke->invoker, objectName(), 1, 1, false, "feitou-exchange");
            DELETE_OVER_SCOPE(const Card, cards)
            invoke->invoker->addToPile("feitou", cards->getSubcards().first());
        }
        return false;
    }
};

class FeitouTargetMod : public TargetModSkill
{
public:
    FeitouTargetMod()
        : TargetModSkill("#feitoumod")
    {
        pattern = "Slash";
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const
    {
        if (from->hasSkill("feitou") && card->getSkillName() == "feitou")
            return 1000;
        else
            return 0;
    }

    virtual int getResidueNum(const Player *from, const Card *card) const
    {
        if (from->hasSkill("feitou") && card->getSkillName() == "feitou")
            return 1000;
        else
            return 0;
    }
};

class Shizhu : public TriggerSkill
{
public:
    Shizhu()
        : TriggerSkill("shizhu")
    {
        events << EventPhaseStart << EventPhaseChanging << CardsMoveOneTime;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile) {
                QList<int> temp_ids;
                QVariantList shizhu_ids = room->getTag("shizhuPeach").toList();
                foreach (QVariant card_data, shizhu_ids)
                    temp_ids << card_data.toInt();

                foreach (int id, move.card_ids) {
                    Card *card = Sanguosha->getCard(id);
                    if (card->isKindOf("Peach") && !temp_ids.contains(id) && room->getCardPlace(id) == Player::DiscardPile)
                        shizhu_ids << id;
                }
                room->setTag("shizhuPeach", shizhu_ids);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->removeTag("shizhuPeach");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() != Player::Finish)
                return QList<SkillInvokeDetail>();

            bool hasPeach = false;
            QVariantList shizhu_ids = room->getTag("shizhuPeach").toList();
            foreach (QVariant card_data, shizhu_ids) {
                if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile) {
                    hasPeach = true;
                    break;
                }
            }
            if (!hasPeach)
                return QList<SkillInvokeDetail>();

            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *src, room->findPlayersBySkillName(objectName())) {
                if (!src->isCurrent())
                    d << SkillInvokeDetail(this, src, src);
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        return invoke->invoker->askForSkillInvoke(objectName(), data);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *source = invoke->invoker;
        QList<int> temp_ids;
        QVariantList shizhu_ids = room->getTag("shizhuPeach").toList();
        foreach (QVariant card_data, shizhu_ids) {
            if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
                temp_ids << card_data.toInt();
        }

        room->fillAG(temp_ids, source);
        int id = room->askForAG(source, temp_ids, false, objectName());
        room->clearAG(source);
        if (id > -1) {
            room->showAllCards(source);
            room->getThread()->delay(1000);
            room->clearAG();
            bool hand_peach = false;
            foreach (const Card *card, source->getHandcards()) {
                if (card->isKindOf("Peach")) {
                    hand_peach = true;
                    break;
                }
            }
            if (!hand_peach)
                source->obtainCard(Sanguosha->getCard(id), true);
        }
        return false;
    }
};

LiangeCard::LiangeCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void LiangeCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
    room->moveCardTo(Sanguosha->getCard(subcards.first()), NULL, Player::DrawPile);
    QList<int> idlist = room->getNCards(2);

    room->fillAG(idlist, targets.first());
    int card_id = room->askForAG(targets.first(), idlist, false, "liange");
    room->clearAG(targets.first());
    room->obtainCard(targets.first(), card_id, false);
    idlist.removeOne(card_id);

    DummyCard *dummy = new DummyCard;
    foreach (int id, idlist)
        dummy->addSubcard(id);
    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, targets.first()->objectName(), objectName(), "");
    room->throwCard(dummy, reason, NULL);
}

class Liange : public OneCardViewAsSkill
{
public:
    Liange()
        : OneCardViewAsSkill("liange")
    {
        filter_pattern = ".|.|.|.";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("LiangeCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        LiangeCard *card = new LiangeCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Tianxie : public TriggerSkill
{
public:
    Tianxie()
        : TriggerSkill("tianxie")
    {
        events << SlashHit << PostCardEffected; //<< CardFinished
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == SlashHit) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            room->setCardFlag(effect.slash, "tianxieEffected_" + effect.to->objectName());
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e != PostCardEffected)
            return QList<SkillInvokeDetail>();
        //CardUseStruct use = data.value<CardUseStruct>();
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.card->isKindOf("EquipCard") || effect.card->isKindOf("SkillCard"))
            return QList<SkillInvokeDetail>();
        if (effect.to->hasSkill(this) && effect.to->isAlive()) {
            if (effect.card->hasFlag("tianxieEffected_" + effect.to->objectName())) {
                if (effect.card->isKindOf("DelayedTrick") || !effect.from || !effect.from->isAlive() || !effect.from->canDiscard(effect.from, "hes"))
                    return QList<SkillInvokeDetail>();
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to);
            } else
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        QString prompt = "target2";
        if (effect.card->hasFlag("tianxieEffected_" + effect.to->objectName()))
            prompt = "target1:" + effect.from->objectName();
        ;
        invoke->invoker->tag[objectName()] = data;
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.card->hasFlag("tianxieEffected_" + effect.to->objectName())) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), effect.from->objectName());
            room->askForDiscard(effect.from, objectName(), 1, 1, false, true);
        } else
            invoke->invoker->drawCards(1);
        return false;
    }
};

class Huobao : public TriggerSkill
{
public:
    Huobao()
        : TriggerSkill("huobao")
    {
        events << EventPhaseStart << EventPhaseChanging << Damage;
    }

    static QList<int> huobaoProhibitCards(ServerPlayer *src, ServerPlayer *target)
    {
        QList<int> cards;
        foreach (const Card *e, target->getEquips()) {
            const EquipCard *equip = qobject_cast<const EquipCard *>(e->getRealCard());
            if (src->getEquip(equip->location()))
                cards << e->getId();
        }
        return cards;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getOtherPlayers(change.player)) {
                    if (p->hasFlag("huobao")) {
                        room->setPlayerFlag(p, "-huobao");
                        room->setFixedDistance(change.player, p, -1);
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() != Player::Play)
                return QList<SkillInvokeDetail>();

            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *src, room->findPlayersBySkillName(objectName())) {
                if (!src->isCurrent() && src->getEquips().length() < player->getEquips().length() && huobaoProhibitCards(src, player).length() < player->getEquips().length())
                    d << SkillInvokeDetail(this, src, src);
            }
            return d;
        }
        if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.from && damage.from->isAlive() && damage.from->isCurrent() && damage.to->isAlive()
                && damage.to->hasFlag("huobao") && !damage.to->getEquips().isEmpty())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.from, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == EventPhaseStart) {
            ServerPlayer *current = room->getCurrent();
            QList<int> disable = huobaoProhibitCards(invoke->invoker, current);
            int id = room->askForCardChosen(invoke->invoker, current, "e", objectName(), false, Card::MethodNone, disable);
            const Card *card = Sanguosha->getCard(id);
            if (!invoke->invoker->hasFlag("huobao")) {
                room->setPlayerFlag(invoke->invoker, "huobao");
                room->setFixedDistance(current, invoke->invoker, 1);
            }
            room->moveCardTo(card, current, invoke->invoker, Player::PlaceEquip,
                             CardMoveReason(CardMoveReason::S_REASON_TRANSFER, invoke->invoker->objectName(), objectName(), QString()));
        } else if (e == Damage) {
            DamageStruct damage = data.value<DamageStruct>();

            room->touhouLogmessage("#TriggerSkill", damage.to, objectName());
            room->notifySkillInvoked(damage.to, objectName());
            int id = room->askForCardChosen(invoke->invoker, damage.to, "e", objectName());

            room->obtainCard(invoke->invoker, id);
        }
        return false;
    }
};

class Duobao : public TriggerSkill
{
public:
    Duobao()
        : TriggerSkill("duobao")
    {
        events << Damage << Damaged;
        frequency = Skill::Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.from && damage.from->isAlive() && damage.by_user && damage.to->isAlive() && damage.from != damage.to && !damage.to->getEquips().isEmpty()
            && (damage.from->hasSkill(this) || damage.to->hasSkill(this))) {
            if (triggerEvent == Damage && damage.from->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true);
            else if (triggerEvent == Damaged && damage.to->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        room->touhouLogmessage("#TriggerSkill", invoke->owner, objectName());
        room->notifySkillInvoked(invoke->owner, objectName());
        int id = room->askForCardChosen(damage.from, damage.to, "e", objectName());
        room->obtainCard(damage.from, id);
        return false;
    }
};

TH14Package::TH14Package()
    : Package("th14")
{
    General *shinmyoumaru = new General(this, "shinmyoumaru$", "hzc", 3);
    shinmyoumaru->addSkill(new Baochui);
    shinmyoumaru->addSkill(new Yicun);
    shinmyoumaru->addSkill(new Moyi);

    General *raiko = new General(this, "raiko", "hzc", 4);
    raiko->addSkill(new Leiting);

    General *seija = new General(this, "seija", "hzc", 3);
    seija->addSkill(new Nizhuan);
    seija->addSkill(new Guizha);

    General *benben = new General(this, "benben", "hzc", 3);
    benben->addSkill(new Yuyin);
    benben->addSkill(new Wuchang);

    General *yatsuhashi = new General(this, "yatsuhashi", "hzc", 3);
    yatsuhashi->addSkill(new Canxiang);
    yatsuhashi->addSkill(new Juwang);

    General *kagerou = new General(this, "kagerou", "hzc", 4);
    kagerou->addSkill(new Langying);
    kagerou->addSkill(new Yuanfei);

    General *sekibanki = new General(this, "sekibanki", "hzc", 4);
    sekibanki->addSkill(new Feitou);
    sekibanki->addSkill(new FeitouTargetMod);
    related_skills.insertMulti("feitou", "#feitoumod");

    General *wakasagihime = new General(this, "wakasagihime", "hzc", 3);
    wakasagihime->addSkill(new Shizhu);
    wakasagihime->addSkill(new Liange);

    General *seija_sp = new General(this, "seija_sp", "hzc", 3);
    seija_sp->addSkill(new Tianxie);
    //seija_sp->addSkill(new Huobao);
    seija_sp->addSkill(new Duobao);

    addMetaObject<LeitingCard>();
    addMetaObject<YuanfeiCard>();
    addMetaObject<LiangeCard>();
}

ADD_PACKAGE(TH14)
