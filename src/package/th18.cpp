
#include "th18.h"

#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "roomthread.h"
#include "skill.h"
#include "structs.h"
#include "th10.h"

#include <algorithm>
#include <cmath>

SimaoCard::SimaoCard()
{
}

bool SimaoCard::targetsFeasible(const QList<const Player *> &targets, const Player * /*Self*/) const
{
    return targets.length() >= 2 && targets.length() <= 4;
}

bool SimaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player * /*Self*/) const
{
    return targets.length() < 4 && !to_select->isNude();
}

void SimaoCard::use(Room *room, const CardUseStruct &use) const
{
    QList<int> discard;

    foreach (ServerPlayer *p, use.to) {
        if (!p->isNude()) {
            const Card *c = room->askForCard(p, "..!", "simao-discard1");
            if (c != nullptr) {
                discard << c->getEffectiveId();
            } else {
                QList<int> i = p->forceToDiscard(1, true);
                if (i.isEmpty())
                    room->showAllCards(p);
                else
                    room->throwCard(i.first(), p);
                discard << i;
            }
        }
    }

    QList<int> toget;
    foreach (int id, discard) {
        if (room->getCardPlace(id) == Player::DiscardPile)
            toget << id;
    }

    if (toget.isEmpty())
        return;

    room->fillAG(toget);

    ServerPlayer *start = room->askForPlayerChosen(use.from, use.to, getSkillName(), "simao-starter");

    QList<ServerPlayer *> correctSeq;
    bool b = false;
    foreach (ServerPlayer *p, use.to) {
        if (b || (p == start)) {
            b = true;
            correctSeq << p;
        }
    }

    foreach (ServerPlayer *p, use.to) {
        if (p == start)
            break;
        correctSeq << p;
    }

    try {
        foreach (ServerPlayer *p, correctSeq) {
            int id = room->askForAG(p, toget, false, getSkillName());
            room->takeAG(p, id, true, {}, Player::DiscardPile);
            toget.removeAll(id);
            discard = toget;
            foreach (int id, discard) {
                if (room->getCardPlace(id) != Player::DiscardPile) {
                    room->takeAG(nullptr, id, false);
                    toget.removeAll(id);
                }
            }

            if (toget.isEmpty())
                break;
        }
    } catch (TriggerEvent) {
        room->clearAG();
        throw;
    }
    room->clearAG();
}

class Simao : public ZeroCardViewAsSkill
{
public:
    Simao()
        : ZeroCardViewAsSkill("simao")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("SimaoCard");
    }

    const Card *viewAs() const override
    {
        return new SimaoCard;
    }
};

ShirongCard::ShirongCard()
{
    will_throw = true;
    m_skillName = "shirong_attach";
}

bool ShirongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && Self != to_select && to_select->hasLordSkill("shirong") && !to_select->hasFlag("shirongInvoked")
        && to_select->getHandcardNum() < to_select->getMaxCards();
}

void ShirongCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->setPlayerFlag(effect.to, "shirongInvoked");
    effect.to->drawCards(1, "shirong");
}

class ShirongVS : public OneCardViewAsSkill
{
public:
    ShirongVS()
        : OneCardViewAsSkill("shirong_attach")
    {
        attached_lord_skill = true;
        filter_pattern = ".!";
    }

    bool shouldBeVisible(const Player *Self) const override
    {
        return (Self != nullptr) && Self->getKingdom() == "hld";
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        if (player->isNude() || !shouldBeVisible(player))
            return false;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasLordSkill("shirong") && !p->hasFlag("shirongInvoked"))
                return true;
        }
        return false;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        ShirongCard *sr = new ShirongCard;
        sr->addSubcard(originalCard);
        return sr;
    }
};

class Shirong : public TriggerSkill
{
public:
    Shirong()
        : TriggerSkill("shirong$")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging << Revive;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent != EventPhaseChanging) {
            static QString attachName = "shirong_attach";
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
                if (p->hasFlag("shirongInvoked"))
                    room->setPlayerFlag(p, "-shirongInvoked");
            }
        }
    }
};

namespace {
const QString LiunengBeforeTagName = "liuneng" + QString::number(static_cast<int>(BeforeCardsMove));
const QString LiunengAfterTagName = "liuneng" + QString::number(static_cast<int>(CardsMoveOneTime));
const QString LiunengSetProperty = "liuneng" + QString::number(static_cast<int>(EventAcquireSkill));
} // namespace

class LiunengVS : public ZeroCardViewAsSkill
{
public:
    LiunengVS(const QString &base)
        : ZeroCardViewAsSkill(base)
    {
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        if (player->hasFlag(objectName()))
            return false;

        QStringList classNames = player->property(objectName().toUtf8().constData()).toString().split("+");
        foreach (const QString &cl, classNames) {
            Card *c = Sanguosha->cloneCard(cl);
            DELETE_OVER_SCOPE(Card, c);
            c->setSkillName(objectName());
            if (c->isAvailable(player))
                return true;
        }

        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        if (player->hasFlag(objectName()))
            return false;

        if (Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;

        if (player->isCurrent()) {
            if (!player->isInMainPhase())
                return false;
        } else {
            foreach (const Player *p, player->getSiblings()) {
                if (p->isCurrent()) {
                    if (!p->isInMainPhase())
                        return false;
                    break;
                }
            }
        }

        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);
        if (cardPattern == nullptr)
            return false;

        QStringList classNames = player->property(objectName().toUtf8().constData()).toString().split("+");
        foreach (const QString &cl, classNames) {
            Card *c = Sanguosha->cloneCard(cl);
            DELETE_OVER_SCOPE(Card, c)
            c->setSkillName(objectName());
            if (cardPattern->match(player, c))
                return true;
        }

        return false;
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const override
    {
        if (player->hasFlag(objectName()))
            return false;

        if (player->isCurrent()) {
            if (!player->isInMainPhase())
                return false;
        } else {
            foreach (const Player *p, player->getSiblings()) {
                if (p->isCurrent()) {
                    if (!p->isInMainPhase())
                        return false;
                    break;
                }
            }
        }

        return player->property(objectName().toUtf8().constData()).toString().contains("Nullification");
    }

    const Card *viewAs() const override
    {
        Card *c = Sanguosha->cloneCard(Self->tag.value(objectName()).toString());
        if (c != nullptr) {
            c->setSkillName(objectName());
            c->setShowSkill(objectName());
            return c;
        }

        return nullptr;
    }
};

class Liuneng : public TriggerSkill
{
public:
    Liuneng()
        : TriggerSkill("liuneng")
    {
        events = {BeforeCardsMove, CardsMoveOneTime, EventPhaseChanging, EventAcquireSkill, EventLoseSkill, PreCardUsed};
        global = true;
        view_as_skill = new LiunengVS(objectName());
    }

    QDialog *getDialog() const override
    {
        return QijiDialog::getInstance(objectName());
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct st = data.value<PhaseChangeStruct>();
            if (st.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    p->tag.remove(LiunengBeforeTagName);
                    p->tag.remove(LiunengAfterTagName);
                    if (p->tag.contains(LiunengSetProperty)) {
                        p->tag.remove(LiunengSetProperty);
                        room->setPlayerProperty(p, objectName().toUtf8().constData(), QString());
                    }

                    room->setPlayerFlag(p, "-" + objectName());
                }
            }

            return;
        }

        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == objectName())
                room->setPlayerFlag(use.from, objectName());

            return;
        }

        bool modified = false;
        if (triggerEvent == BeforeCardsMove || triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            for (int i = 0; i < move.card_ids.length(); ++i) {
                const Card *card = room->getCard(move.card_ids.at(i));
                if ((card->getTypeId() == Card::TypeBasic || card->isNDTrick()) && move.from != nullptr
                    && (move.from_places.at(i) == Player::PlaceHand || move.from_places.at(i) == Player::PlaceEquip)
                    && (((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE)
                        || ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE)
                        || ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD))) {
                    QVariantMap m;
                    if (move.from->tag.contains(LiunengBeforeTagName))
                        m = move.from->tag[LiunengBeforeTagName].toMap();
                    QString cardKey = QString::number(move.card_ids.at(i));

                    if (triggerEvent == BeforeCardsMove) {
                        m[cardKey] = card->objectName();
                    } else {
                        QString className = card->objectName();

                        if (m.contains(cardKey)) {
                            className = m[cardKey].toString();
                            m.remove(cardKey);
                        }

                        QStringList c;
                        if (move.from->tag.contains(LiunengAfterTagName))
                            c = move.from->tag[LiunengAfterTagName].toStringList();
                        if (!c.contains(className))
                            c << className;
                        move.from->tag[LiunengAfterTagName] = c;
                        modified = true;
                    }

                    move.from->tag[LiunengBeforeTagName] = m;
                }
            }
        }

        if (triggerEvent == EventAcquireSkill || triggerEvent == EventLoseSkill) {
            SkillAcquireDetachStruct st = data.value<SkillAcquireDetachStruct>();
            if (st.skill == this)
                modified = true;
        }

        if (modified) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (!p->hasSkill(this)) {
                    if (p->tag.contains(LiunengSetProperty)) {
                        p->tag.remove(LiunengSetProperty);
                        room->setPlayerProperty(p, objectName().toUtf8().constData(), QString());
                    }
                } else {
                    QStringList prop1;
                    QStringList prop;
                    if (p->tag.contains(LiunengSetProperty))
                        prop1 = p->tag[LiunengSetProperty].toStringList();

                    foreach (ServerPlayer *p2, room->getOtherPlayers(p)) {
                        QStringList c;
                        if (p2->tag.contains(LiunengAfterTagName))
                            c = p2->tag[LiunengAfterTagName].toStringList();
                        foreach (const QString &cn, c)
                            prop << cn;
                    }

                    if (prop1.toSet() != prop.toSet()) {
                        prop1 = prop.toSet().toList();
                        p->tag[LiunengSetProperty] = prop1;
                        room->setPlayerProperty(p, objectName().toUtf8().constData(), prop1.join("+"));
                    }
                }
            }
        }
    }
};

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
    CizhaoDistance(const QString &baseSkill = "cizhao")
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
        room->getThread()->delay();

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
    YingjiRecord(const QString &yingji = "yingji")
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
                if (player->hasFlag(b + "_first"))
                    player->setFlags(b + "_second");
                else
                    player->setFlags(b + "_first");
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                change.player->setFlags("-" + b + "_first");
                change.player->setFlags("-" + b + "_second");
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
        response_pattern = "@@" + base + "!";
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

                bool invoke = !ids.isEmpty();

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
            return invoke->invoker->askForSkillInvoke(objectName(), card_v, "e:::" + card_v.value<const Card *>()->objectName());
        } else {
            const Card *card = nullptr;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else
                card = data.value<CardResponseStruct>().m_card;

            return invoke->invoker->askForSkillInvoke(objectName(), data, "a:" + invoke->targets.first()->objectName() + "::" + card->objectName());
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
            const Card *obtainedGood = room->askForCard(player, "@@yingji!", "yingji-get:" + invoke->invoker->objectName(), {}, Card::MethodNone, nullptr, false, {}, false, 0);
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
        if (p->hasSkill(this) && p->getPile("goods").length() > p->getMaxHp() && p->getPhase() == Player::Draw)
            return {SkillInvokeDetail(this, p, p, nullptr, true)};

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
    const CardUseStruct &card_use = _use;

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

    QString cl = "cw";
    if (card_use.from->getNextAlive(1, false) == card_use.to.first())
        cl = "ccw";

    LogMessage logCl;
    logCl.from = _use.from;
    logCl.type = "#boxiCl";
    logCl.arg = "boxi";
    logCl.arg2 = cl;
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
        if (!c->isNude()) {
            c->tag["boxi"] = generateBoxiAiTag(idMap);
            const Card *shown = room->askForExchange(c, getSkillName(), 1, 1, true, "boxi-show:" + card_use.from->objectName(), false);
            c->tag.remove("boxi");
            room->showCard(c, shown->getEffectiveId());
            idMap[shown->getEffectiveId()] = c;
            delete shown;
        }

        if (cw)
            c = qobject_cast<ServerPlayer *>(c->getLastAlive(1, false));
        else
            c = qobject_cast<ServerPlayer *>(c->getNextAlive(1, false));

        if (c == nullptr)
            Q_UNREACHABLE();
    }

    if (!card_use.from->isNude()) {
        card_use.from->tag["boxi"] = generateBoxiAiTag(idMap);
        const Card *shown = room->askForExchange(card_use.from, getSkillName(), 1, 1, true, "boxi-show-self", true);
        card_use.from->tag.remove("boxi");
        if (shown != nullptr) {
            room->showCard(card_use.from, shown->getEffectiveId());
            idMap[shown->getEffectiveId()] = card_use.from;
            delete shown;
        }
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
    return oc->isAvailable(Self) && !Self->isCardLimited(oc, Card::MethodUse) && oc->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, oc, targets);
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
    return oc->isAvailable(Self) && !Self->isCardLimited(oc, Card::MethodUse) && oc->targetsFeasible(targets, Self);
}

const Card *BoxiUseOrObtainCard::validate(CardUseStruct &use) const
{
    // AI: ensure an empty use.to here!

    Room *room = use.from->getRoom();
    const Card *card = Sanguosha->getCard(subcards.first());
    QString method = "obtain";

    if (!use.to.isEmpty())
        method = "use";
    else if (card->isAvailable(use.from) && !use.from->isCardLimited(card, Card::MethodUse) && (card->targetFixed(use.from) || card->targetsFeasible({}, use.from))
             && use.from->isOnline())
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

namespace {
bool zhuyuUsable(const Player *p, const Card *c)
{
    if (c->isVirtualCard())
        return false;
    if (!StringList2IntList(p->property("zhuyu").toString().split("+")).contains(c->getId()))
        return false;
    if (c->getSuitString() != p->property("zhuyuSuit").toString())
        return false;
    if (p->isCardLimited(c, Card::MethodUse))
        return false;
    if (c->isKindOf("Jink") || c->isKindOf("Nullification"))
        return false;
    if (c->targetFixed(p))
        return true;

    bool targetSelectable = false;
    QList<const Player *> ps = p->getAliveSiblings();
    ps << p;
    foreach (const Player *o, ps) {
        if (c->targetFilter({}, o, p) && !p->isProhibited(o, c)) {
            targetSelectable = true;
            break;
        }
    }
    return targetSelectable;
}
} // namespace

ZhuyuUseDiscardPileCard::ZhuyuUseDiscardPileCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "_zhuyu";
}

bool ZhuyuUseDiscardPileCard::targetFixed(const Player *Self) const
{
    if (subcardsLength() > 1)
        return true;

    const Card *c = Sanguosha->getCard(subcards.first());
    if (!zhuyuUsable(Self, c))
        return true;

    return c->targetFixed(Self);
}

bool ZhuyuUseDiscardPileCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (subcardsLength() > 1)
        return targets.isEmpty();

    const Card *c = Sanguosha->getCard(subcards.first());
    if (!zhuyuUsable(Self, c))
        return targets.isEmpty();

    return c->targetsFeasible(targets, Self);
}

bool ZhuyuUseDiscardPileCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (subcardsLength() > 1)
        return false;

    const Card *c = Sanguosha->getCard(subcards.first());
    if (!zhuyuUsable(Self, c))
        return false;

    return c->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, c, targets);
}

const Card *ZhuyuUseDiscardPileCard::validate(CardUseStruct &use) const
{
    Room *room = use.from->getRoom();
    QList<int> il = StringList2IntList(use.from->property("zhuyuUsed").toString().split("+"));
    il << subcards;
    room->setPlayerProperty(use.from, "zhuyuUsed", IntList2StringList(il).join("+"));

    if (subcardsLength() > 1) {
        Q_ASSERT(use.to.isEmpty());
        return use.card;
    }

    const Card *card = Sanguosha->getCard(subcards.first());
    if (!zhuyuUsable(use.from, card)) {
        Q_ASSERT(use.to.isEmpty());
        return use.card;
    }

    return card;
}

void ZhuyuUseDiscardPileCard::use(Room *room, const CardUseStruct &card_use) const
{
    room->throwCard(card_use.card, nullptr);
}

ZhuyuSlashCard::ZhuyuSlashCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "_zhuyu";
}

bool ZhuyuSlashCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (targets.length() == 1)
        return true;

    Slash s(Card::SuitToBeDecided, -1);
    s.addSubcards(subcards);
    return s.isAvailable(Self) && s.targetsFeasible(targets, Self);
}

bool ZhuyuSlashCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (targets.isEmpty())
        return true;

    Slash s(Card::SuitToBeDecided, -1);
    s.addSubcards(subcards);
    return s.isAvailable(Self) && s.targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, &s);
}

const Card *ZhuyuSlashCard::validate(CardUseStruct &cardUse) const
{
    Slash *s = new Slash(Card::SuitToBeDecided, -1);
    s->addSubcards(subcards);
    s->setSkillName("_zhuyu");

    QList<const Player *> ps;
    foreach (ServerPlayer *p, cardUse.to)
        ps << p;

    bool canSlash = false;

    if (s->isAvailable(cardUse.from) && s->targetsFeasible(ps, cardUse.from))
        canSlash = true;

    QString choice = "slash";

    if (cardUse.to.length() == 1) {
        if ((!canSlash) || (cardUse.to.first() == cardUse.from) || !cardUse.from->isOnline())
            choice = "give";
        else
            choice = cardUse.from->getRoom()->askForChoice(cardUse.from, "zhuyu", "slash+give", QVariant::fromValue(cardUse));
    }

    if (choice == "slash")
        return s;

    delete s;
    return cardUse.card;
}

void ZhuyuSlashCard::onEffect(const CardEffectStruct &effect) const
{
    CardMoveReason r(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), getSkillName(), {});
    effect.to->getRoom()->obtainCard(effect.to, this, r);
}

class ZhuyuVS : public ViewAsSkill
{
public:
    ZhuyuVS(const QString &base)
        : ViewAsSkill(base)
    {
        expand_pile = "*" + objectName();
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if ((pattern == "@@zhuyu-card1!" || pattern == "@@zhuyu-card2!")) {
            if (selected.isEmpty()) {
                if (zhuyuUsable(Self, to_select))
                    return true;
            } else if (selected.length() == 1) {
                if (zhuyuUsable(Self, selected.first()))
                    return false;
            } // else if selected.length > 1 - impossible
        }

        if (pattern == "@@zhuyu-card1!") {
            if (to_select->getSuitString() != Self->property("zhuyuSuit").toString())
                return true;
        }

        if (pattern == "@@zhuyu-card3!") {
            QList<int> ids = StringList2IntList(Self->property("zhuyu").toString().split("+"));
            QList<int> usedIds = StringList2IntList(Self->property("zhuyuUsed").toString().split("+"));
            if (ids.contains(to_select->getId()) && !usedIds.contains(to_select->getId()))
                return true;
        }
        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if ((pattern == "@@zhuyu-card1!" || pattern == "@@zhuyu-card2!") && cards.length() == 1 && zhuyuUsable(Self, cards.first())) {
            ZhuyuUseDiscardPileCard *c = new ZhuyuUseDiscardPileCard;
            c->addSubcards(cards);
            return c;
        }

        if (pattern == "@@zhuyu-card1!") {
            QList<int> ids = StringList2IntList(Self->property("zhuyu").toString().split("+"));
            foreach (const Card *c, cards)
                ids.removeAll(c->getId());
            foreach (int id, ids) {
                const Card *c = Sanguosha->getCard(id);
                if (c->getSuitString() != Self->property("zhuyuSuit").toString())
                    return nullptr;
            }

            ZhuyuUseDiscardPileCard *c = new ZhuyuUseDiscardPileCard;
            c->addSubcards(cards);
            return c;
        }

        if (pattern == "@@zhuyu-card3!") {
            QList<int> ids = StringList2IntList(Self->property("zhuyu").toString().split("+"));
            QList<int> usedIds = StringList2IntList(Self->property("zhuyuUsed").toString().split("+"));
            foreach (int id, usedIds)
                ids.removeAll(id);
            foreach (const Card *card, cards)
                ids.removeAll(card->getId());

            if (ids.isEmpty()) {
                ZhuyuSlashCard *c = new ZhuyuSlashCard;
                c->addSubcards(cards);
                return c;
            }
        }

        return nullptr;
    }

    bool isEnabledAtPlay(const Player *) const override
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const override
    {
        return pattern.startsWith("@@zhuyu-card");
    }
};

class Zhuyu : public TriggerSkill
{
public:
    Zhuyu()
        : TriggerSkill("zhuyu")
    {
        events = {EventPhaseEnd};
        view_as_skill = new ZhuyuVS(objectName());
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        if (p->isAlive() && p->hasSkill(this) && p->getPhase() == Player::Draw)
            return {SkillInvokeDetail(this, p, p)};

        return {};
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<int> ids = room->getNCards(3, false, true);
        room->returnToDrawPile(ids, true);

        LogMessage logShowCard;
        logShowCard.type = "$ShowCard";
        logShowCard.from = invoke->invoker;
        logShowCard.card_str = IntList2StringList(ids).join("+");
        room->sendLog(logShowCard);

        room->fillAG(ids);

        room->setPlayerProperty(invoke->invoker, objectName().toUtf8().constData(), IntList2StringList(ids).join("+"));
        const Card *c = room->askForCard(invoke->invoker, "..", "zhuyu-discard1", {}, QString(), 0);

        // TODO: record this card in event BeforeCardsMove?
        // Since there are no FilterSkill which changes the suit now, temporarily ignore it
        if (c != nullptr) {
            c = Sanguosha->getCard(c->getEffectiveId());
            room->setPlayerProperty(invoke->invoker, "zhuyuSuit", c->getSuitString());

            QList<int> usedIds;

            try {
                forever {
                    bool usable = false;
                    foreach (int id, ids) {
                        if (!usedIds.contains(id)) {
                            const Card *idCard = Sanguosha->getCard(id);
                            if (zhuyuUsable(invoke->invoker, idCard)) {
                                usable = true;
                                break;
                            }
                        }
                    }
                    if (!usable) {
                        if (usedIds.isEmpty()) {
                            foreach (int id, ids) {
                                const Card *idCard = Sanguosha->getCard(id);
                                if (idCard->getSuit() != c->getSuit())
                                    usedIds << id;
                            }

                            if (!usedIds.isEmpty()) {
                                DummyCard d(usedIds);
                                room->throwCard(&d, nullptr);
                            }
                        }
                        break;
                    }

                    QString pattern = "@@zhuyu-card2";
                    QString prompt = "zhuyu-use2";
                    int notifyIndex = 2;
                    if (usedIds.isEmpty()) {
                        pattern = "@@zhuyu-card1";
                        prompt = "zhuyu-use1";
                        notifyIndex = 1;
                    }

                    // AI: Is it possible directly using the original card?
                    // need deduplicate items during setPlayerProperty
                    if (!room->askForUseCard(invoke->invoker, pattern + "!", prompt + ":::" + c->getSuitString(), notifyIndex)) {
                        if (usedIds.isEmpty()) {
                            foreach (int id, ids) {
                                const Card *idCard = Sanguosha->getCard(id);
                                if (idCard->getSuit() != c->getSuit())
                                    usedIds << id;
                            }

                            if (!usedIds.isEmpty()) {
                                DummyCard d(usedIds);
                                room->throwCard(&d, nullptr);
                                break;
                            }
                        }

                        // code reaches here, either there are card used, or there are no card to discard. So force use
                        {
                            const Card *useCard = nullptr;
                            ServerPlayer *target = nullptr;
                            foreach (int id, ids) {
                                if (!usedIds.contains(id)) {
                                    const Card *idCard = Sanguosha->getCard(id);
                                    if (zhuyuUsable(invoke->invoker, idCard)) {
                                        useCard = idCard;
                                        if (!idCard->targetFixed(invoke->invoker)) {
                                            foreach (ServerPlayer *t, room->getAlivePlayers()) {
                                                if (idCard->targetFilter({}, t, invoke->invoker) && !invoke->invoker->isProhibited(t, idCard)) {
                                                    target = t;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                                if (useCard != nullptr)
                                    break;
                            }

                            if (useCard == nullptr) {
                                Q_UNREACHABLE();

                                // Make it more ... somewhat ... clear?
                                LogMessage l;
                                l.type = "#ZhuyuForceUseBug";
                                l.from = invoke->invoker;
                                room->sendLog(l);

                                JsonArray skillCommand;
                                skillCommand << (objectName() + "Bug") << ("forceUseBug:" + invoke->invoker->objectName());
                                QList<ServerPlayer *> notified;
                                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                                    if (p->isOnline()) {
                                        notified << p;
                                        p->m_commandArgs = skillCommand;
                                    }
                                }
                                room->doBroadcastRequest(notified, QSanProtocol::S_COMMAND_INVOKE_SKILL);

                                // we have no choice but...
                                break;
                            } else {
                                usedIds << useCard->getId();

                                CardUseStruct use;
                                use.from = invoke->invoker;
                                use.card = useCard;
                                if (!use.card->targetFixed(use.from))
                                    use.to << target;
                                room->useCard(use);
                            }
                        }

                        room->setPlayerProperty(invoke->invoker, "zhuyuUsed", IntList2StringList(usedIds).join("+"));
                    } else {
                        usedIds = StringList2IntList(invoke->invoker->property("zhuyuUsed").toString().split("+"));
                    }

                    if (usedIds.length() == ids.length())
                        break;
                }

                if (usedIds.length() < ids.length()) {
                    Card::Suit s = Card::NoSuitBlack; // there must be no actual card be NoSuitBlack. but may be NoSuit.
                    bool same = true;
                    foreach (int id, ids) {
                        if (!usedIds.contains(id)) {
                            const Card *idCard = Sanguosha->getCard(id);
                            if (s == Card::NoSuitBlack) {
                                s = idCard->getSuit();
                            } else if (s != idCard->getSuit()) {
                                same = false;
                                break;
                            }
                        }
                    }

                    if (same) {
                        // AI: use original card for Slash
                        if (!room->askForUseCard(invoke->invoker, "@@zhuyu-card3!", "zhuyu-slash1", 3)) {
                            DummyCard d;
                            foreach (int id, ids) {
                                if (!usedIds.contains(id))
                                    d.addSubcard(id);
                            }

                            CardMoveReason r(CardMoveReason::S_REASON_GIVE, invoke->invoker->objectName(), invoke->invoker->objectName(), objectName(), {});
                            room->obtainCard(invoke->invoker, &d, r);
                        }
                    }
                }
            } catch (TriggerEvent) {
                room->setPlayerProperty(invoke->invoker, objectName().toUtf8().constData(), QString());
                room->setPlayerProperty(invoke->invoker, "zhuyuSuit", QString());
                room->setPlayerProperty(invoke->invoker, "zhuyuUsed", QString());
                room->clearAG();
                throw;
            }

            room->setPlayerProperty(invoke->invoker, objectName().toUtf8().constData(), QString());
            room->setPlayerProperty(invoke->invoker, "zhuyuSuit", QString());
            room->setPlayerProperty(invoke->invoker, "zhuyuUsed", QString());
            room->clearAG();
        } else {
            room->getThread()->delay();
            room->setPlayerProperty(invoke->invoker, objectName().toUtf8().constData(), QString());
            room->clearAG();
        }

        return false;
    }
};

class Shuzhu : public TriggerSkill
{
public:
    Shuzhu()
        : TriggerSkill("shuzhu")
    {
        events = {EventPhaseStart};
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        QList<SkillInvokeDetail> d;
        if (p->isAlive() && p->getPhase() == Player::Discard && p->getHandcardNum() > p->getMaxCards()) {
            foreach (ServerPlayer *o, room->getOtherPlayers(p)) {
                if (o->hasSkill(this))
                    d << SkillInvokeDetail(this, o, o, p);
            }
        }

        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hs", objectName());
        room->showCard(invoke->targets.first(), id);

        QString choice = "put";
        if (invoke->invoker->canDiscard(invoke->targets.first(), id, objectName()))
            choice = room->askForChoice(invoke->invoker, objectName(), "put+discard", id);
        if (choice == "discard")
            room->throwCard(id, invoke->targets.first(), invoke->invoker);
        else
            room->moveCardsToEndOfDrawpile({id}, true);

        return false;
    }
};

TiaosuoCard::TiaosuoCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    sort_targets = false;
}

bool TiaosuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->isAlive() && to_select->getPhase() == Player::NotActive;
}

void TiaosuoCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *c = room->getCurrent();

    c->obtainCard(this);

    for (ServerPlayer *i : {c, card_use.to.first()})
        room->setPlayerProperty(i, "chained", !i->isChained());

    room->setPlayerMark(c, "tiaosuo1", 1);

    // TODO: Is a log message needed here?
}

class TiaosuoVS : public OneCardViewAsSkill
{
public:
    TiaosuoVS(const QString &base)
        : OneCardViewAsSkill(base)
    {
        filter_pattern = ".|black";
        response_pattern = "@@" + base;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        TiaosuoCard *c = new TiaosuoCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class TiaosuoP : public ProhibitSkill
{
public:
    TiaosuoP(const QString &base = "tiaosuo")
        : ProhibitSkill("#" + base + "-prohibit")
    {
    }

    bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &, bool) const override
    {
        if (from->getMark("tiaosuo1") > 0 && (card->isKindOf("Slash") || card->isKindOf("Duel")))
            return from->isChained() != to->isChained();

        return false;
    }
};

class TiaosuoT : public TargetModSkill
{
public:
    TiaosuoT(const QString &base = "tiaosuo")
        : TargetModSkill("#" + base + "-targetmod")
    {
        pattern = "Slash";
    }

    int getResidueNum(const Player *from, const Card *) const override
    {
        if (from->getMark("tiaosuo1") > 0)
            return 1;

        return 0;
    }

    int getDistanceLimit(const Player *from, const Card *) const override
    {
        if (from->getMark("tiaosuo1") > 0)
            return 1000;

        return 0;
    }
};

class Tiaosuo : public TriggerSkill
{
public:
    Tiaosuo()
        : TriggerSkill("tiaosuo")
    {
        events = {EventPhaseStart, EventPhaseChanging};
        view_as_skill = new TiaosuoVS(objectName());
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct s = data.value<PhaseChangeStruct>();
            if (s.from == Player::Play)
                room->setPlayerMark(s.player, "tiaosuo1", 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->getPhase() == Player::Play) {
                foreach (ServerPlayer *a, room->getAllPlayers()) {
                    if (a->hasSkill(this) && a != p) {
                        bool invoke = !a->isKongcheng();
                        if (!invoke) {
                            foreach (const Card *c, a->getEquips()) {
                                if (c->isBlack()) {
                                    invoke = true;
                                    break;
                                }
                            }
                        }

                        if (invoke)
                            d << SkillInvokeDetail(this, a, a, p);
                    }
                }
            }
        }

        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        return room->askForUseCard(invoke->invoker, "@@tiaosuo", "tiaosuo-ts:" + p->objectName(), -1, Card::MethodNone);
    }
};

class Zuanying : public TriggerSkill
{
public:
    Zuanying()
        : TriggerSkill("zuanying")
    {
        events = {EventPhaseStart};
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        if (p->isAlive() && p->getPhase() == Player::Finish && p->hasSkill(this)) {
            foreach (ServerPlayer *i, room->getAllPlayers()) {
                if (i != p && i->isChained())
                    return {SkillInvokeDetail(this, p, p)};
            }
        }

        return {};
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<ServerPlayer *> ts;
        foreach (ServerPlayer *i, room->getAllPlayers()) {
            if (i != invoke->invoker && i->isChained())
                ts << i;
        }
        if (!ts.isEmpty()) {
            ServerPlayer *p = room->askForPlayerChosen(invoke->invoker, ts, objectName(), "zuanying-zzz", true, true);
            if (p != nullptr) {
                invoke->targets << p;
                return true;
            }
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->targets.first()->drawCards(1, objectName());

        if (invoke->targets.first()->getHandcardNum() > invoke->targets.first()->getMaxCards()) {
            QStringList choices;
            if (invoke->invoker->isWounded())
                choices << "heal";
            if (invoke->targets.first()->getCardCount() >= 2)
                choices << "rob";

            if (choices.isEmpty())
                return false;

            QString choice = choices.first();
            if (choices.length() > 1)
                choice = room->askForChoice(invoke->invoker, objectName(), choices.join("+"), QVariant::fromValue(invoke->targets.first()));

            if (choice == "heal") {
                room->recover(invoke->invoker, {});
            } else {
                QList<int> selected;
                for (int i = 0; i < 2; ++i) {
                    int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hes", objectName(), false, Card::MethodNone, selected);
                    selected << id;
                }

                DummyCard d(selected);
                room->obtainCard(invoke->invoker, &d, false);
            }
        }

        return false;
    }
};

class FgwlShezheng : public TriggerSkill
{
public:
    FgwlShezheng()
        : TriggerSkill("fgwlshezheng")
    {
        events = {PreCardUsed, CardUsed, CardResponded, EventPhaseChanging};
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use;
        if (triggerEvent == CardUsed) {
            use = data.value<CardUseStruct>();
        } else if (triggerEvent == CardResponded) {
            use = CardUseStruct();
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse) {
                use.card = resp.m_card;
                use.from = resp.m_from;
                use.m_addHistory = false;
            }
        } else {
            use = CardUseStruct();
        }

        if (use.card != nullptr && use.from != nullptr) {
            if (use.from->getPhase() == Player::Play && use.from->hasSkill(this)) {
                SkillInvokeDetail d(this, use.from, use.from);
                d.tag["originalUse"] = QVariant::fromValue(use);
                return {d};
            }
        }

        return {};
    }

    static QString colorStr(const Card *c)
    {
        if (c->isRed())
            return "no_suit_red";
        if (c->isBlack())
            return "no_suit_black";
        return "no_suit";
    }

    bool cost(TriggerEvent, Room *r, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        CardUseStruct use = invoke->tag["originalUse"].value<CardUseStruct>();

        if (invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(use.card), "i:::" + colorStr(use.card) + ":" + use.card->objectName())) {
            QList<int> ids = r->getNCards(1, true, true);
            CardMoveReason reason(CardMoveReason::S_REASON_PUT, invoke->invoker->objectName(), objectName(), {});
            r->throwCard(Sanguosha->getCard(ids.first()), reason, nullptr);
            invoke->tag["realId"] = ids.first();
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct originalUse = invoke->tag["originalUse"].value<CardUseStruct>();
        const Card *putCard = Sanguosha->getCard(invoke->tag["realId"].toInt());

        if ((originalUse.card->isBlack() == putCard->isBlack()) && (originalUse.card->isRed() == putCard->isRed())) {
            originalUse.from->setFlags(objectName());
            bool reducedHistory = false;

            if (triggerEvent == CardUsed) {
                CardUseStruct realUse = data.value<CardUseStruct>();
                if (realUse.m_addHistory) {
                    realUse.m_addHistory = false;
                    data = QVariant::fromValue(realUse);
                    room->addPlayerHistory(realUse.from, realUse.card->getClassName(), -1);
                    reducedHistory = true;
                }
            }

            LogMessage l;
            l.type = "#fgwlshezheng1";
            if (!reducedHistory)
                l.type = "#fgwlshezheng2";
            l.from = originalUse.from;
            l.arg = objectName();
            l.arg2 = originalUse.card->objectName();
            room->sendLog(l);
        } else {
            if (!invoke->invoker->isNude())
                room->askForDiscard(invoke->invoker, objectName(), 1, 1, false, true);
        }

        return false;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                change.player->setFlags("-" + objectName());
        } else if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from->hasFlag(objectName()) && use.from->getPhase() == Player::Play) {
                use.from->setFlags("-" + objectName());
                if (use.m_addHistory) {
                    use.m_addHistory = false;
                    data = QVariant::fromValue(use);
                    room->addPlayerHistory(use.from, use.card->getClassName(), -1);

                    LogMessage l;
                    l.type = "#yvshou";
                    l.from = use.from;
                    l.arg = objectName();
                    l.arg2 = use.card->objectName();
                    room->sendLog(l);
                }
            }
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse) {
                if (resp.m_from->hasFlag(objectName()) && resp.m_from->getPhase() == Player::Play)
                    resp.m_from->setFlags("-" + objectName());
            }
        }
    }
};

class MijiRecord : public TriggerSkill
{
public:
    MijiRecord(const QString &base = "miji")
        : TriggerSkill("#" + base)
        , b(base)
    {
        events = {PreCardUsed, CardResponded};
        global = true;
    }

    void record(TriggerEvent triggerEvent, Room *, QVariant &data) const override
    {
        const Card *c = nullptr;
        ServerPlayer *p = nullptr;

        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            c = use.card;
            p = use.from;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse) {
                c = resp.m_card;
                p = resp.m_from;
            }
        }

        if (c != nullptr && p != nullptr) {
            int t = p->tag[b].toInt();
            int type = static_cast<int>(c->getTypeId());
            t = t | (1 << type);
            p->tag[b] = t;
        }
    }

private:
    QString b;
};

class Miji : public TriggerSkill
{
public:
    Miji()
        : TriggerSkill("miji")
    {
        events = {CardsMoveOneTime, EventPhaseStart};
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *invoker = nullptr;

        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->getPhase() == Player::Play && p->hasSkill(this) && p->isAlive())
                invoker = p;
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != nullptr && move.from->hasSkill(this) && move.from->isAlive()
                && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)) {
                for (int i = 0; i < move.card_ids.length(); ++i) {
                    if ((move.from_places[i] == Player::PlaceEquip || move.from_places[i] == Player::PlaceHand)) {
                        const Card *c = Sanguosha->getCard(move.card_ids[i]);
                        if (c->getTypeId() == Card::TypeBasic) {
                            invoker = qobject_cast<ServerPlayer *>(move.from);
                            break;
                        }
                    }
                }
            }
        }

        if (invoker != nullptr)
            return {SkillInvokeDetail(this, invoker, invoker)};

        return {};
    }

    // bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override;

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<int> ids = room->getNCards(3, false, true);
        room->returnToDrawPile(ids, true);

        QList<int> disabled;
        QList<int> enabled;
        foreach (int id, ids) {
            const Card *c = Sanguosha->getCard(id);
            int type = static_cast<int>(c->getTypeId());
            int t = invoke->invoker->tag[objectName()].toInt();
            if (t & (1 << type))
                disabled << id;
            else
                enabled << id;
        }

        room->fillAG(ids, invoke->invoker, disabled);
        int selected = -1;
        if (!enabled.isEmpty())
            selected = room->askForAG(invoke->invoker, enabled, false, objectName());

        try {
            if (selected != -1)
                room->takeAG(invoke->invoker, selected, true, {invoke->invoker});
        } catch (TriggerEvent) {
            room->clearAG(invoke->invoker);
            throw;
        }

        if (selected == -1)
            room->getThread()->delay();

        room->clearAG(invoke->invoker);

        return false;
    }
};

JuezhuCard::JuezhuCard()
{
    target_fixed = true;
}

void JuezhuCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    use.to.clear();

    foreach (ServerPlayer *p, room->getOtherPlayers(card_use.from)) {
        if (p->getHandcardNum() >= card_use.from->getHandcardNum())
            use.to << p;
    }

    SkillCard::onUse(room, use);
}

void JuezhuCard::use(Room *room, const CardUseStruct &card_use) const
{
    foreach (ServerPlayer *p, card_use.to) {
        QString draw = room->askForChoice(p, getSkillName(), "letdraw+draw", QVariant::fromValue(card_use));

        if (draw == "letdraw") {
            card_use.from->drawCards(1, getSkillName());
        } else {
            p->drawCards(1, getSkillName());
            if (card_use.from->askForSkillInvoke(getSkillName(), QVariant::fromValue(p), "d:" + p->objectName())) {
                Duel *d = new Duel(Card::NoSuit, -1);
                d->setSkillName("_" + getSkillName());
                CardUseStruct newUse;
                newUse.card = d;
                newUse.from = card_use.from;
                newUse.to << p;
                room->useCard(newUse);

                return;
            }
        }
    }
}

class Juezhu : public ZeroCardViewAsSkill
{
public:
    Juezhu()
        : ZeroCardViewAsSkill("juezhu")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("JuezhuCard");
    }

    const Card *viewAs() const override
    {
        return new JuezhuCard;
    }
};

class Zhanyi : public ViewAsSkill
{
public:
    Zhanyi()
        : ViewAsSkill("zhanyi")
    {
        response_or_use = true;
    }

    static int num(const Player *p)
    {
        int hp = p->getHp();
        int hcnum = p->getHandcardNum();

        return std::max(1, std::abs(hp - hcnum));
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *) const override
    {
        return selected.length() < num(Self);
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() == num(Self)) {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcards(cards);
            slash->setSkillName(objectName());
            slash->setShowSkill(objectName());

            bool usable = true;

            if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY)
                usable = slash->isAvailable(Self);

            if (usable)
                return slash;
            else
                delete slash;
        }

        return nullptr;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return Slash::IsAvailable(player);
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        Slash *card = new Slash(Card::SuitToBeDecided, -1);
        DELETE_OVER_SCOPE(Slash, card)
        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);

        return cardPattern != nullptr && cardPattern->match(player, card);
    }
};

TH18Package::TH18Package()
    : Package("th18")
{
    General *chimata = new General(this, "chimata$", "hld");
    chimata->addSkill(new Simao);
    chimata->addSkill(new Liuneng);
    chimata->addSkill(new Shirong);

    General *mike = new General(this, "mike", "hld", 3);
    mike->addSkill(new Cizhao);
    mike->addSkill(new CizhaoDistance);
    mike->addSkill(new Danran);
    related_skills.insertMulti("cizhao", "#cizhao-distance");

    General *takane = new General(this, "takane", "hld");
    takane->addSkill(new Yingji);
    takane->addSkill(new YingjiRecord);
    takane->addSkill(new Zhixiao);
    related_skills.insertMulti("yingji", "#yingji-record");

    General *sannyo = new General(this, "sannyo", "hld");
    sannyo->addSkill(new Boxi);

    General *misumaru = new General(this, "misumaru", "hld");
    misumaru->addSkill(new Zhuyu);
    misumaru->addSkill(new Shuzhu);

    General *tsukasa = new General(this, "tsukasa", "hld", 3);
    tsukasa->addSkill(new Tiaosuo);
    tsukasa->addSkill(new TiaosuoT);
    tsukasa->addSkill(new TiaosuoP);
    tsukasa->addSkill(new Zuanying);
    related_skills.insertMulti("tiaosuo", "#tiaosuo-distance");
    related_skills.insertMulti("tiaosuo", "#tiaosuo-targetmod");

    General *megumu = new General(this, "megumu", "hld");
    megumu->addSkill(new FgwlShezheng);
    megumu->addSkill(new Miji);
    megumu->addSkill(new MijiRecord);
    related_skills.insertMulti("miji", "#miji");

    General *momoyo = new General(this, "momoyo", "hld");
    momoyo->addSkill(new Juezhu);
    momoyo->addSkill(new Zhanyi);

    addMetaObject<BoxiCard>();
    addMetaObject<BoxiUseOrObtainCard>();
    addMetaObject<TiaosuoCard>();
    addMetaObject<JuezhuCard>();
    addMetaObject<ZhuyuSlashCard>();
    addMetaObject<ZhuyuUseDiscardPileCard>();
    addMetaObject<SimaoCard>();
    addMetaObject<ShirongCard>();

    skills << new ShirongVS;
}

ADD_PACKAGE(TH18)
