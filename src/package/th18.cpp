
#include "th18.h"

#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "skill.h"
#include "structs.h"

#include <algorithm>

#undef DESCRIPTION

#ifdef DESCRIPTION
["chimata"] = "天弓千亦",
    ["#chimata"] = "无主物之神", ["simao"] = "司贸",
    [":simao"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>你可以令二至四名有牌的角色各弃置一张牌，然后以其中一名角色为起点，这些角色各获得这些弃置的牌中一张牌。",
    ["liuneng"] = "流能", [":liuneng"] = "你可以视为使用一张其他角色于当前回合内因使用、打出或弃置而失去的基本牌或普通锦囊牌，<font color=\"green\"><b>每回合限一次。</b></font>",
    ["shirong"] = "市荣",
    [":shirong"] = "<font color=\"orange\"><b>主公技，</b></font><font "
                   "color=\"green\"><b>其他龙势力角色的出牌阶段限一次，</b></font>若你手牌数小于你的手牌上限，其可以弃置一张牌，然后你摸一张牌。",
    ["shirong_attach"] = "市荣",
    [":shirong_attach"] = "<font color=\"green\"><b>出牌阶段限一次，</b></font>若你属于龙势力且拥有主公技“市荣”的角色的手牌数小于其手牌上限，你可以弃置一张牌，然后其摸一张牌。",
    ;
;
#endif

class Cizhao : public TriggerSkill
{
public:
    Cizhao()
        : TriggerSkill("cizhao")
    {
        events = {EventPhaseStart};
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        QList<SkillInvokeDetail> d;

        if (p->getPhase() == Player::Play && p->isAlive()) {
            foreach (ServerPlayer *mike, room->getAllPlayers()) {
                if (mike != p && mike->hasSkill(this) && !mike->isNude())
                    d << SkillInvokeDetail(this, mike, mike, p);
            }
        }

        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        const Card *c
            = room->askForExchange(invoke->invoker, objectName() + QStringLiteral("1"), 1, 1, true, objectName() + "-prompt1:" + invoke->targets.first()->objectName(), true);
        if (c != nullptr) {
            LogMessage l;
            l.type = "#ChoosePlayerWithSkill";
            l.from = invoke->invoker;
            l.to = invoke->targets;
            l.arg = objectName();
            room->sendLog(l);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());

            CardMoveReason m(CardMoveReason::S_REASON_GIVE, invoke->invoker->objectName(), objectName(), {});
            room->obtainCard(invoke->targets.first(), c, m, false);
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *t = invoke->targets.first();

        bool discarded = (!t->isNude()) && room->askForDiscard(t, objectName() + QStringLiteral("2"), 1, 1, true, true, objectName() + "-prompt2:" + invoke->invoker->objectName());

        LogMessage l;
        l.from = t;

        if (!discarded) {
            room->drawCards(t, 1, objectName());
            room->setPlayerFlag(t, objectName() + QStringLiteral("plus2"));
            l.type = "#cizhao-log1";
        } else {
            room->setPlayerFlag(t, objectName() + QStringLiteral("minus1"));
            l.type = "#cizhao-log2";
        }

        room->sendLog(l);

        return false;
    }
};

class CizhaoDistance : public DistanceSkill
{
public:
    CizhaoDistance(const QString &baseSkill)
        : DistanceSkill("#" + baseSkill + "-distance")
        , b(baseSkill)
    {
    }

    int getCorrect(const Player *from, const Player *) const override
    {
        if (from->hasFlag(b + QStringLiteral("plus2")))
            return 2;
        if (from->hasFlag(b + QStringLiteral("minus1")))
            return -1;

        return 0;
    }

private:
    QString b;
};

class DanranVS : public ViewAsSkill
{
public:
    DanranVS(const QString &objectName)
        : ViewAsSkill(objectName)
    {
        response_pattern = "jink";
        response_or_use = true;
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const override
    {
        return !to_select->isEquipped();
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() == Self->getHandcardNum()) {
            Card *j = Sanguosha->cloneCard("jink");
            j->addSubcards(cards);
            j->setSkillName(objectName());
            j->setShowSkill(objectName());
            return j;
        }

        return nullptr;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        if (player->isKongcheng())
            return false;

        Card::Suit s = Card::SuitToBeDecided;
        foreach (const Card *c, player->getHandcards()) {
            if (c->isRed()) {
                if (s == Card::SuitToBeDecided)
                    s = Card::NoSuitRed;
                else if (s != Card::NoSuitRed)
                    return false;
            } else if (c->isBlack()) {
                if (s == Card::SuitToBeDecided)
                    s = Card::NoSuitBlack;
                else if (s != Card::NoSuitBlack)
                    return false;
            } else {
                if (s == Card::SuitToBeDecided)
                    s = Card::NoSuit;
                else if (s != Card::NoSuit)
                    return false;
            }
        }

        return ViewAsSkill::isEnabledAtResponse(player, pattern);
    }
};

class Danran : public TriggerSkill
{
public:
    Danran()
        : TriggerSkill("danran")
    {
        view_as_skill = new DanranVS(objectName());
        events = {CardFinished, CardResponded};
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *from = nullptr;
        const Card *card = nullptr;

        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            from = use.from;
            card = use.card;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (!resp.m_isUse) {
                from = resp.m_from;
                card = resp.m_card;
            }
        }

        if (from != nullptr && card != nullptr && card->getSkillName() == objectName() && card->isKindOf("Jink"))
            return {SkillInvokeDetail(this, from, from, from, true, nullptr, false)};

        return {};
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ExNihilo *sheng = new ExNihilo(Card::NoSuit, -1);
        sheng->setSkillName("_" + objectName());
        CardUseStruct use(sheng, invoke->invoker, invoke->invoker, false);
        room->useCard(use);
        return false;
    }
};

class YingjiRecord : public TriggerSkill
{
public:
    YingjiRecord(const QString &yingji)
        : TriggerSkill("#" + yingji + "-record")
        , b(yingji)
    {
        events = {EventPhaseStart, PreCardUsed, CardResponded, EventPhaseEnd, CardUsed, EventPhaseChanging};
        global = true;
    }

    void record(TriggerEvent triggerEvent, Room *, QVariant &data) const override
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isNDTrick())
                use.from->tag[b] = use.card->toString();
            else
                use.from->tag.remove(b);
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse) {
                if (resp.m_card->isNDTrick())
                    resp.m_from->tag[b] = resp.m_card->toString();
                else
                    resp.m_from->tag.remove(b);
            }
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->getPhase() == Player::Play)
                p->tag.remove(b);
        }

        if (triggerEvent == CardUsed || triggerEvent == CardResponded) {
            ServerPlayer *player = nullptr;
            const Card *card = nullptr;
            if (triggerEvent == CardUsed) {
                player = data.value<CardUseStruct>().from;
                card = data.value<CardUseStruct>().card;
            } else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                player = response.m_from;
                if (response.m_isUse)
                    card = response.m_card;
            }
            if ((player != nullptr) && player->getPhase() == Player::Play && (card != nullptr)) {
                if (player->hasFlag(objectName() + "_first"))
                    player->setFlags(objectName() + "_second");
                else
                    player->setFlags(objectName() + "_first");
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                change.player->setFlags("-" + objectName() + "_first");
                change.player->setFlags("-" + objectName() + "_second");
            }
        }
    }

private:
    QString b;
};

class YingJiVS : public OneCardViewAsSkill
{
public:
    YingJiVS(const QString &base)
        : OneCardViewAsSkill(base)
    {
        expand_pile = "+goods";
        response_pattern = "@@yingji!";
    }

    bool viewFilter(const Card *to_select) const override
    {
        QString name = Self->property(objectName().toUtf8().constData()).toString();

        QList<const Player *> ps = Self->getAliveSiblings();
        ps << Self;

        const Player *taka = nullptr;

        foreach (const Player *p, ps) {
            if (p->objectName() == name) {
                taka = p;
                break;
            }
        }

        if (taka != nullptr)
            return taka->getPile("goods").contains(to_select->getId());

        return false;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        return new DummyCard({originalCard->getId()});
    }
};

class Yingji : public TriggerSkill
{
public:
    Yingji()
        : TriggerSkill("yingji")
    {
        events = {EventPhaseEnd, CardUsed, CardResponded};
        view_as_skill = new YingJiVS(objectName());
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == CardUsed || triggerEvent == CardResponded) {
            ServerPlayer *player = nullptr;
            const Card *card = nullptr;
            if (triggerEvent == CardUsed) {
                player = data.value<CardUseStruct>().from;
                card = data.value<CardUseStruct>().card;
            } else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                player = response.m_from;
                if (response.m_isUse)
                    card = response.m_card;
            }
            if ((player != nullptr) && player->getPhase() == Player::Play && (card != nullptr) && card->getTypeId() == Card::TypeBasic) {
                if (player->hasFlag(objectName() + "_first") && !player->hasFlag(objectName() + "_second")) {
                    QList<SkillInvokeDetail> d;
                    foreach (ServerPlayer *p, room->getAllPlayers()) {
                        if (p->hasSkill(this) && !p->getPile("goods").isEmpty())
                            d << SkillInvokeDetail(this, p, p, player);
                    }

                    return d;
                }
            }
        } else if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->getPhase() == Player::Play && p->tag.contains(objectName())) {
                QString cardStr = p->tag.value(objectName()).toString();
                const Card *c = Card::Parse(cardStr);
                QList<int> ids;
                if (c->isVirtualCard())
                    ids = c->getSubcards();
                else
                    ids << c->getId();

                bool invoke = true;
                foreach (int id, ids) {
                    if (room->getCardPlace(id) != Player::DiscardPile) {
                        invoke = false;
                        break;
                    }
                }

                if (invoke) {
                    QList<SkillInvokeDetail> d;
                    foreach (ServerPlayer *p, room->getAllPlayers()) {
                        if (p->hasSkill(this)) {
                            SkillInvokeDetail i(this, p, p);
                            i.tag[objectName()] = QVariant::fromValue(c);
                            d << i;
                        }
                    }

                    return d;
                }
            }
        }

        return {};
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseEnd) {
            QVariant card_v = invoke->tag.value(objectName());
            return invoke->invoker->askForSkillInvoke(objectName(), card_v, "e:::" + card_v.value<const Card *>()->getClassName());
        } else {
            return invoke->invoker->askForSkillInvoke(objectName(), data, "a:" + invoke->targets.first()->objectName());
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseEnd) {
            const Card *c = invoke->tag.value(objectName()).value<const Card *>();
            QList<int> ids;
            if (c->isVirtualCard())
                ids = c->getSubcards();
            else
                ids << c->getId();
            invoke->invoker->addToPile("goods", ids);
        } else {
            ServerPlayer *player = nullptr;
            const Card *card = nullptr;
            if (triggerEvent == CardUsed) {
                player = data.value<CardUseStruct>().from;
                card = data.value<CardUseStruct>().card;
            } else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                player = response.m_from;
                if (response.m_isUse)
                    card = response.m_card;
            }

            if (player == nullptr || card == nullptr)
                Q_UNREACHABLE();

            invoke->invoker->obtainCard(card);
            room->setPlayerProperty(player, objectName().toUtf8().constData(), invoke->invoker->objectName());
            const Card *obtainedGood = room->askForCard(player, "@@yingji!", "yingji-get:" + invoke->invoker->objectName(), {}, Card::MethodNone);
            if (obtainedGood == nullptr) {
                QList<int> is = invoke->invoker->getPile("goods");
                obtainedGood = Sanguosha->getCard(is[qrand() % is.length()]);
            }

            player->obtainCard(obtainedGood);
        }

        return false;
    }
};

class Zhixiao : public TriggerSkill
{
public:
    Zhixiao()
        : TriggerSkill("zhixiao")
    {
        events = {EventPhaseStart};
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        if (p->hasSkill(this) && p->getPile("goods").length() > p->getMaxHp())
            return {SkillInvokeDetail(this, p, p, nullptr, true)};

        return {};
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
            LogMessage l;
            l.type = "#TriggerSkill";
            l.from = invoke->invoker;
            l.arg = objectName();
            room->sendLog(l);

            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        DummyCard d(invoke->invoker->getPile("goods"));
        invoke->invoker->obtainCard(&d);

        return true;
    }
};

BoxiCard::BoxiCard()
{
    target_fixed = false;
}

bool BoxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && (Self->getNextAlive(1, false) == to_select || Self->getLastAlive(1, false) == to_select);
}

void BoxiCard::onUse(Room *room, const CardUseStruct &_use) const
{
    CardUseStruct card_use = _use;

    // GameRule::effect (PreCardUsed)
    {
        if (card_use.from->hasFlag("Global_ForbidSurrender")) {
            card_use.from->setFlags("-Global_ForbidSurrender");
            room->doNotify(card_use.from, QSanProtocol::S_COMMAND_ENABLE_SURRENDER, QVariant(true));
        }

        card_use.from->broadcastSkillInvoke(card_use.card);
        if (!card_use.card->getSkillName().isNull() && card_use.card->getSkillName(true) == card_use.card->getSkillName(false) && card_use.m_isOwnerUse
            && card_use.from->hasSkill(card_use.card->getSkillName()))
            room->notifySkillInvoked(card_use.from, card_use.card->getSkillName());
    }

    LogMessage log;
    log.from = _use.from;
    log.type = "#UseCard";
    log.card_str = card_use.card->toString(!card_use.card->willThrow());
    room->sendLog(log);

    QString cl = "cw";
    if (card_use.from->getNextAlive(1, false) == card_use.to.first())
        cl = "ccw";

    LogMessage logCl;
    logCl.from = _use.from;
    logCl.type = "#boxiCl";
    log.arg = "boxi";
    log.arg2 = cl;
    room->sendLog(logCl);

    // show general // Fs: why not use getShowSkill?
    _use.from->showHiddenSkill(card_use.card->getSkillName(false));

    use(room, card_use);
}

namespace {
QStringList generateBoxiAiTag(const QMap<int, ServerPlayer *> &map)
{
    QStringList l;
    foreach (int id, map.keys())
        l << (map.value(id)->objectName() + ":" + QString::number(id));

    return l;
}
} // namespace

void BoxiCard::use(Room *room, const CardUseStruct &card_use) const
{
    bool cw = true;
    if (card_use.from->getNextAlive(1, false) == card_use.to.first())
        cw = false;

    ServerPlayer *c = card_use.to.first();
    QMap<int, ServerPlayer *> idMap;
    while (c != card_use.from) {
        // TODO: currently askForCardShow do not support show equip card. To be resolved.
        if (!c->isKongcheng()) {
            c->tag["boxi"] = generateBoxiAiTag(idMap);
            const Card *shown = room->askForCardShow(c, card_use.from, "boxi");
            c->tag.remove("boxi");
            idMap[shown->getEffectiveId()] = c;
        }

        if (cw)
            c = qobject_cast<ServerPlayer *>(c->getLastAlive(1, false));
        else
            c = qobject_cast<ServerPlayer *>(c->getNextAlive(1, false));

        if (c == nullptr)
            Q_UNREACHABLE();
    }

    if (!card_use.from->isKongcheng()) {
        card_use.from->tag["boxi"] = generateBoxiAiTag(idMap);
        // TODO: cancelable askForCardShow
        const Card *shown = room->askForCardShow(card_use.from, card_use.from, "boxi");
        card_use.from->tag.remove("boxi");
        // idMap[card_use.from] = shown->getEffectiveId();
        idMap[shown->getEffectiveId()] = card_use.from;
    }

    QList<int> ids = idMap.keys();

    QMap<Card::Suit, int> numbers;
    int most = 0;
    Card::Suit uniqueMost = Card::NoSuit;
    foreach (int id, ids) {
        const Card *z = Sanguosha->getCard(id);
        if (!numbers.contains(z->getSuit()))
            numbers[z->getSuit()] = 0;

        int current = ++(numbers[z->getSuit()]);
        if (most < current) {
            most = current;
            uniqueMost = z->getSuit();
        } else if (most == current) {
            uniqueMost = Card::NoSuit;
        } else {
            //
        }
    }

    int least = ids.length();
    foreach (int number, numbers)
        least = std::min(least, number);

    QList<int> to_discard;

    if (uniqueMost != Card::NoSuit) {
        foreach (int id, ids) {
            const Card *z = Sanguosha->getCard(id);
            if (z->getSuit() == uniqueMost)
                to_discard << id;
        }

        CardMoveReason r;
        r.m_reason = CardMoveReason::S_REASON_THROW;
        r.m_playerId = card_use.from->objectName();

        LogMessage l;
        l.type = "$DiscardCard";
        l.from = card_use.from;
        foreach (int id, to_discard)
            l.to << idMap.value(id);
        l.card_str = IntList2StringList(to_discard).join("+");
        room->sendLog(l);

        CardsMoveStruct move(to_discard, nullptr, Player::DiscardPile, r);
        room->moveCardsAtomic({move}, true);
    }

    QList<ServerPlayer *> draws;
    foreach (int id, ids) {
        const Card *z = Sanguosha->getCard(id);
        if (numbers.value(z->getSuit()) == least)
            draws << idMap.value(id);
    }

    room->sortByActionOrder(draws);
    room->drawCards(draws, 1, "boxi");

    if (!to_discard.isEmpty()) {
        QList<int> to_discard2;
        foreach (int id, to_discard) {
            if (room->getCardPlace(id) == Player::DiscardPile)
                to_discard2 << id;
        }

        if (!to_discard2.isEmpty()) {
            // AI: do not use BoxiUseOrObtainCard for this askForUseCard when using, instead:
            // use setPlayerProperty to clear this property, then return real card for this CardUseStruct
            // use BoxiUseOrObtainCard only for obtaining
            room->setPlayerProperty(card_use.from, "boxi", IntList2StringList(to_discard2).join("+"));
            try {
                if (!room->askForUseCard(card_use.from, "@@boxi!", "boxi-use-or-obtain")) {
                    // randomly get a card
                    int r = to_discard2.at(qrand() % to_discard2.length());
                    room->obtainCard(card_use.from, r);
                }
            } catch (TriggerEvent) {
                room->setPlayerProperty(card_use.from, "boxi", QString());
                throw;
            }
            room->setPlayerProperty(card_use.from, "boxi", QString());
        }
    }
}

BoxiUseOrObtainCard::BoxiUseOrObtainCard()
{
    will_throw = false;
    m_skillName = "_boxi";
    handling_method = Card::MethodNone;
}

bool BoxiUseOrObtainCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *oc = Sanguosha->getCard(subcards.first());
    return oc->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, oc, targets);
}

bool BoxiUseOrObtainCard::targetFixed(const Player *Self) const
{
    const Card *oc = Sanguosha->getCard(subcards.first());
    return oc->targetFixed(Self);
}

bool BoxiUseOrObtainCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (targets.isEmpty())
        return true;

    const Card *oc = Sanguosha->getCard(subcards.first());
    return oc->targetsFeasible(targets, Self);
}

const Card *BoxiUseOrObtainCard::validate(CardUseStruct &use) const
{
    // AI: ensure an empty use.to here!

    Room *room = use.from->getRoom();
    const Card *card = Sanguosha->getCard(subcards.first());
    QString method = "obtain";

    if (!use.to.isEmpty())
        method = "use";
    else if ((card->targetFixed(use.from) || card->targetsFeasible({}, use.from)) && use.from->isOnline())
        method = room->askForChoice(use.from, "boxi", "use+obtain");

    room->setPlayerProperty(use.from, "boxi", QString());

    if (method == "use")
        return card;

    return use.card;
}

void BoxiUseOrObtainCard::use(Room *room, const CardUseStruct &card_use) const
{
    room->obtainCard(card_use.from, card_use.card);
}

class Boxi : public ViewAsSkill
{
public:
    Boxi()
        : ViewAsSkill("boxi")
    {
        expand_pile = "*boxi";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY)
            return false;
        else if (Sanguosha->getCurrentCardUsePattern() == "@@boxi!")
            return selected.isEmpty() && StringList2IntList(Self->property("boxi").toString().split("+")).contains(to_select->getId());

        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            return new BoxiCard;
        } else if (Sanguosha->getCurrentCardUsePattern() == "@@boxi!") {
            if (cards.length() == 1) {
                BoxiUseOrObtainCard *c = new BoxiUseOrObtainCard;
                c->addSubcards(cards);
                return c;
            }
        }

        return nullptr;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("BoxiCard");
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const override
    {
        return pattern == "@@boxi!";
    }
};

#ifdef DESCRIPTION
["misumaru"] = "玉造魅须丸", ["#misumaru"] = "真正的勾玉匠人", ["zhuyu"] = "铸玉",
                             [":zhuyu"]
    = "摸牌阶段结束时，你可以展示牌堆底的三张牌，然后可以弃置一张牌并选择一项：依次使用其中与之花色相同的能使用的牌；或将其中与之花色不同的牌置入弃牌堆。选择后若余下的"
      "牌花色相同，你将之当【杀】使用或交给一名角色。",
                             ["shuzhu"] = "戍珠", [":shuzhu"] = "其他角色的弃牌阶段开始时，若其手牌数大于其手牌上限，你可以展示其一张手牌并将之置于牌堆底或弃置之。",
                             ;
;
#endif
#ifdef DESCRIPTION
["tsukasa"] = "菅牧典", ["#tsukasa"] = "耳边低语的邪恶白狐", ["tiaosuo"] = "挑唆",
                        [":tiaosuo"] = "其他角色的出牌阶段开始时，你可以将一张黑色牌交给其并横置或重置其和另一名角色，然后其于此阶段内：使用【杀】的次数+"
                                       "1且无距离限制；使用【杀】或【决斗】不能选择与其人物牌横竖放置状态不同的角色为目标。",
                        ["zuanying"] = "钻营",
                        [":zuanying"] = "结束阶段开始时，你可以令一名处于连环状态的其他角色摸一张牌，然后若其手牌数大于其手牌上限，你回复1点体力或获得其两张牌。",
                        ;
;
#endif
#ifdef DESCRIPTION
["megumu"] = "饭纲丸龙", ["#megumu"] = "鸦天狗的首领", ["fgwlshezheng"] = "涉政",
                         [":fgwlshezheng"]
    = "当你于出牌阶段内使用牌时，你可以将牌堆底的一张牌置入弃牌堆，若两张牌颜色：相同，你于此阶段内使用的本牌和下一张牌不计入使用次数限制；不同，你弃置一张牌。",
                         ["miji"] = "觅机", [":miji"] = "出牌阶段开始时或当你因弃置而失去基本牌后，你可以观看牌堆底的三张牌并获得其中一张当前回合未使用过的类别牌。",
                         ;
;
#endif
#ifdef DESCRIPTION
["momoyo"] = "姬虫百百世", ["#momoyo"] = "漆黑的噬龙者", ["juezhu"] = "掘珠",
                           [":juezhu"] = "<font "
                                         "color=\"green\"><b>出牌阶段限一次，</b></"
                                         "font>你可以选择所有手牌数不小于你的其他角色，令其各选择一项：1.令你摸一张牌；2.摸一张牌，然后你可以终止此流程并视为对其使用【决斗】。",
                           ["zhanyi"] = "战意", [":zhanyi"] = "你可以将X张牌（X为你的体力值与手牌数之差且至少为1）当【杀】使用或打出。",
                           ;
;
;
#endif

TH18Package::TH18Package()
    : Package("th18")
{
    General *chimata = new General(this, "chimata$", "hld");
    chimata->addSkill(new Skill("simao"));
    chimata->addSkill(new Skill("liuneng"));
    chimata->addSkill(new Skill("shirong$"));

    General *mike = new General(this, "mike", "hld", 3);
    mike->addSkill(new Cizhao);
    mike->addSkill(new CizhaoDistance("cizhao"));
    mike->addSkill(new Danran);
    related_skills.insertMulti("cizhao", "#cizhao-distance");

    General *takane = new General(this, "takane", "hld");
    takane->addSkill(new Yingji);
    takane->addSkill(new YingjiRecord("yingji"));
    takane->addSkill(new Zhixiao);
    related_skills.insertMulti("yingji", "#yingji-record");

    General *sannyo = new General(this, "sannyo", "hld");
    sannyo->addSkill(new Boxi);

    General *misumaru = new General(this, "misumaru", "hld");
    misumaru->addSkill(new Skill("zhuyu"));
    misumaru->addSkill(new Skill("shuzhu"));

    General *tsukasa = new General(this, "tsukasa", "hld", 3);
    tsukasa->addSkill(new Skill("tiaosuo"));
    tsukasa->addSkill(new Skill("zuanying"));

    General *megumu = new General(this, "megumu", "hld");
    megumu->addSkill(new Skill("fgwlshezheng"));
    megumu->addSkill(new Skill("miji"));

    General *momoyo = new General(this, "momoyo", "hld");
    momoyo->addSkill(new Skill("juezhu"));
    momoyo->addSkill(new Skill("zhanyi"));

    addMetaObject<BoxiCard>();
    addMetaObject<BoxiUseOrObtainCard>();
}

ADD_PACKAGE(TH18)
