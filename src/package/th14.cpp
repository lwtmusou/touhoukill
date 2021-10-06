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
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Start && player->getHandcardNum() < 3 && player->isAlive()) {
            foreach (ServerPlayer *src, room->findPlayersBySkillName(objectName())) {
                if (src->canDiscard(src, "hes"))
                    d << SkillInvokeDetail(this, src, src, nullptr, false, player);
            }
        } else if (player->getPhase() == Player::Discard && player->getHandcardNum() < 3 && player->hasFlag(objectName())) {
            d << SkillInvokeDetail(this, nullptr, player, nullptr, true);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Start) {
            return room->askForCard(invoke->invoker, ".|.|.|hand,equipped", "@baochui:" + player->objectName(), QVariant::fromValue(player), Card::MethodDiscard, nullptr, false,
                                    objectName());
        }
        return true;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->isKindOf("Slash") && use.from->isAlive()) {
            foreach (ServerPlayer *p, use.to) {
                if (p->hasSkill(this) && use.from->getHandcardNum() > p->getHandcardNum())
                    d << SkillInvokeDetail(this, p, p, nullptr, true);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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
    effect.from->drawCards(1);
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

    const Card *cards = room->askForCard(effect.from, ".|.|.|hand,equipped", "@leiting:" + effect.to->objectName(), QVariant::fromValue(effect.to));
    /*if (!cards) {
        //force discard!!!
        int x = qrand() % hc.length();
        cards = hc.value(x);
        room->throwCard(cards, effect.from);
    }*/
    if (cards == nullptr)
        return;
    if (cards->getSuit() == Card::Heart) {
        effect.to->drawCards(1);
        room->damage(DamageStruct("leiting", nullptr, effect.to, 1, DamageStruct::Thunder));
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

        if (target != nullptr) {
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

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("LeitingCard");
    }

    const Card *viewAs() const override
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
        events << TargetConfirmed << TargetSpecified << CardFinished << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *r, QVariant &d) const override
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct c = d.value<PhaseChangeStruct>();
            switch (c.from) {
            case Player::Start:
            case Player::Judge:
            case Player::Draw:
            case Player::Play:
            case Player::Discard:
            case Player::Finish: {
                foreach (ServerPlayer *p, r->getAllPlayers()) {
                    if (p->hasFlag("nizhuanUsed"))
                        p->setFlags("-nizhuanUsed");
                }
            }
            case Player::NotActive:
                foreach (const QString &n, r->getTagNames()) {
                    if (n.startsWith("nizhuan"))
                        r->removeTag(n);
                }
            default:
                break;
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (e == EventPhaseChanging)
            return d;

        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card == nullptr || !use.card->isKindOf("Slash"))
            return d;

        if (e == CardFinished) {
            if (use.card->hasFlag("nizhuan")) {
                QVariantList l = room->getTag("nizhuan" + use.card->toString()).toList();
                if (l.length() == 2) {
                    ServerPlayer *p1 = l.first().value<ServerPlayer *>();
                    ServerPlayer *p2 = l.last().value<ServerPlayer *>();
                    if (p1 != p2 && p1->isAlive() && p2->isAlive())
                        d << SkillInvokeDetail(this, p1, p1, p2, true, nullptr, false);
                }
            }
        } else if (use.to.length() == 1) {
            ServerPlayer *target = use.to.first();
            ServerPlayer *p1 = nullptr;
            ServerPlayer *p2 = nullptr;
            if (e == TargetSpecified && use.from != nullptr && use.from->isAlive() && !use.from->hasFlag("nizhuanUsed") && use.from->hasSkill(this) && target->isAlive()) {
                p1 = use.from;
                p2 = target;
            } else if (e == TargetConfirmed && target->isAlive() && !target->hasFlag("nizhuanUsed") && target->hasSkill(this) && use.from != nullptr && use.from->isAlive()) {
                p1 = target;
                p2 = use.from;
            }

            if (p1 != nullptr && p2 != nullptr && !(p1->isKongcheng() && p2->isKongcheng()))
                d << SkillInvokeDetail(this, p1, p1, nullptr, false, p2);
        }

        return d;
    }

    static void swapHandCard(ServerPlayer *p1, ServerPlayer *p2, Room *room)
    {
        QList<int> h1 = p1->handCards();
        QList<int> h2 = p2->handCards();

        QList<CardsMoveStruct> moves;
        CardsMoveStruct move1, move2;
        move1.card_ids = h1;
        move1.to = p2;
        move1.to_place = Player::PlaceHand;
        move2.card_ids = h2;
        move2.to = p1;
        move2.to_place = Player::PlaceHand;
        moves << move1 << move2;

        room->moveCardsAtomic(moves, false);
    }

    bool cost(TriggerEvent e, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (e == CardFinished)
            return true;
        invoke->invoker->tag["nizhuan_carduse"] = data; //for ai
        return invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget));
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (e == CardFinished) {
            ServerPlayer *p1 = invoke->invoker;
            ServerPlayer *p2 = invoke->targets.first();
            swapHandCard(p1, p2, room);
            room->removeTag("nizhuan" + use.card->toString());
        } else {
            room->setPlayerFlag(invoke->invoker, "nizhuanUsed");
            use.card->setFlags("nizhuan");
            ServerPlayer *p1 = invoke->invoker;
            ServerPlayer *p2 = invoke->targets.first();
            swapHandCard(p1, p2, room);
            room->setTag("nizhuan" + use.card->toString(), QVariantList() << QVariant::fromValue<ServerPlayer *>(p1) << QVariant::fromValue<ServerPlayer *>(p2));
        }

        return false;
    }
};

class Guizha : public TriggerSkill
{
public:
    Guizha()
        : TriggerSkill("guizha")
    {
        events << EnterDying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *r, const QVariant &data) const override
    {
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.who->hasSkill(this) && dying.who->isAlive()) {
            foreach (ServerPlayer *p, r->getOtherPlayers(dying.who)) {
                if (!p->isKongcheng())
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dying.who, dying.who);
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        QList<ServerPlayer *> ts;
        DyingStruct dying = data.value<DyingStruct>();
        foreach (ServerPlayer *p, room->getOtherPlayers(dying.who)) {
            if (!p->isKongcheng())
                ts << p;
        }

        ServerPlayer *t = room->askForPlayerChosen(invoke->invoker, ts, "guizha", "@guizha-steal", true, true);
        if (t != nullptr) {
            invoke->targets << t;
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = invoke->targets.first();
        QList<int> ids;
        foreach (int id, target->handCards()) {
            if (Sanguosha->getCard(id)->isKindOf("Peach") || Sanguosha->getCard(id)->isKindOf("Analeptic"))
                ids << id;
        }

        int selected = room->doGongxin(invoke->invoker, target, ids, "guizha");
        invoke->invoker->tag.remove("guizha");
        if (selected != -1)
            room->obtainCard(invoke->invoker, selected);

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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->hasSkill(this) && damage.to->isAlive()) {
            foreach (ServerPlayer *p, room->getOtherPlayers(damage.to)) {
                if (p->getHp() >= damage.to->getHp() && !p->isNude())
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *player = invoke->invoker;
        QList<ServerPlayer *> listt;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getHp() >= player->getHp() && !p->isNude())
                listt << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@" + objectName(), true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hes", objectName());
        room->obtainCard(invoke->invoker, id, false);
        return false;
    }
};

class Wuchang : public TriggerSkill
{
public:
    Wuchang()
        : TriggerSkill("wuchang")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Discard) {
                if (change.player->getMark("wuchang_limit") > 0) {
                    room->removePlayerCardLimitation(change.player, "discard", ".|.|.|show$1", objectName());
                    room->setPlayerMark(change.player, "wuchang_limit", 0);
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (!current || current->getPhase() != Player::Discard || current->getCards("h").isEmpty())
                return d;

            foreach (ServerPlayer *src, room->findPlayersBySkillName(objectName())) {
                if (src != current)
                    d << SkillInvokeDetail(this, src, src, nullptr, false, current);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QString prompt = "target:" + invoke->preferredTarget->objectName();
        return room->askForSkillInvoke(invoke->invoker, objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "h", objectName());
        if (id > -1) {
            invoke->targets.first()->addToShownHandCards(QList<int>() << id);
            if (invoke->targets.first()->getMark("wuchang_limit") == 0) {
                room->setPlayerCardLimitation(invoke->targets.first(), "discard", ".|.|.|show", objectName(), true);
                room->setPlayerMark(invoke->targets.first(), "wuchang_limit", 1);
            }
        }
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive() && damage.from->hasSkill(this)) {
            foreach (ServerPlayer *p, room->getOtherPlayers(damage.from)) {
                if (p->getHp() >= damage.from->getHp() && !p->isNude())
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *player = invoke->invoker;
        QList<ServerPlayer *> listt;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getHp() >= player->getHp() && !p->isNude())
                listt << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@" + objectName(), true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hes", objectName());
        room->obtainCard(invoke->invoker, id, false); //room->getCardPlace(id) != Player::PlaceHand
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *current = room->getCurrent();
        CardAskedStruct s = data.value<CardAskedStruct>();
        if (!current || !current->isAlive() || current == s.player || !s.player->hasSkill(this))
            return QList<SkillInvokeDetail>();

        Jink jink(Card::NoSuit, 0);
        const CardPattern *cardPattern = Sanguosha->getPattern(s.pattern);
        if (cardPattern != nullptr && cardPattern->match(s.player, &jink) && !s.player->isCardLimited(&jink, s.method)) // ??? isCardLimited????
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, s.player, s.player, nullptr, false, current);

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        return invoke->invoker->askForSkillInvoke(objectName(), "throw:" + invoke->preferredTarget->objectName());
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        const Card *card = room->askForCard(invoke->targets.first(), ".|red|.|hand", "@juwang:" + invoke->invoker->objectName(), data);
        if (card == nullptr)
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        return invoke->invoker->askForSkillInvoke(objectName(), data);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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
    LogMessage l;
    l.type = "#yuanfei";
    l.from = target;
    room->sendLog(l);
}

class Yuanfei : public ViewAsSkill
{
public:
    Yuanfei()
        : ViewAsSkill("yuanfei")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("YuanfeiCard");
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        if (selected.length() >= 1)
            return false;

        if (to_select->isEquipped())
            return false;

        if (Self->isJilei(to_select))
            return false;

        return true;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() == 0)
            return new YuanfeiCard;
        if (cards.length() != 1)
            return nullptr;

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

    bool isEnabledAtPlay(const Player *player) const override
    {
        //need check limitation?
        return player->getPile("feitou").length() > 0;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        Slash *card = new Slash(Card::SuitToBeDecided, -1);
        DELETE_OVER_SCOPE(Slash, card)
        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);

        return cardPattern != nullptr && cardPattern->match(player, card) && player->getPile("feitou").length() > 0;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->addSubcard(originalCard);
        slash->setSkillName("feitou");
        return slash;
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
        related_pile = "feitou";
    }

    bool canPreshow() const override
    {
        return true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent != EventPhaseStart)
            return QList<SkillInvokeDetail>();
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Finish && player->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
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

    int getDistanceLimit(const Player *from, const Card *card) const override
    {
        if (from->hasSkill("feitou") && card->getSkillName() == "feitou")
            return 1000;
        else
            return 0;
    }

    int getResidueNum(const Player *from, const Card *card) const override
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        return invoke->invoker->askForSkillInvoke(objectName(), data);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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
    room->moveCardTo(Sanguosha->getCard(subcards.first()), nullptr, Player::DrawPile);
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
    room->throwCard(dummy, reason, nullptr);
}

class Liange : public OneCardViewAsSkill
{
public:
    Liange()
        : OneCardViewAsSkill("liange")
    {
        filter_pattern = ".";
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("LiangeCard");
    }

    const Card *viewAs(const Card *originalCard) const override
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

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == SlashHit) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            room->setCardFlag(effect.slash, "tianxieEffected_" + effect.to->objectName());
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        QString prompt = "target2";
        if (effect.card->hasFlag("tianxieEffected_" + effect.to->objectName()))
            prompt = "target1:" + effect.from->objectName();

        invoke->invoker->tag[objectName()] = data;
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
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
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.from, nullptr, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.from && damage.from->isAlive() && damage.by_user && damage.to->isAlive() && damage.from != damage.to && !damage.to->getEquips().isEmpty()
            && (damage.from->hasSkill(this) || damage.to->hasSkill(this))) {
            if (triggerEvent == Damage && damage.from->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, true);
            else if (triggerEvent == Damaged && damage.to->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, nullptr, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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
    seija_sp->addSkill(new Duobao);

    addMetaObject<LeitingCard>();
    addMetaObject<YuanfeiCard>();
    addMetaObject<LiangeCard>();
}

ADD_PACKAGE(TH14)
