#include "protagonist.h"

#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"

class Qixiang : public TriggerSkill
{
public:
    Qixiang()
        : TriggerSkill("qixiang")
    {
        events << CardsMoveOneTime;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<SkillInvokeDetail> d;

        if (move.to_place == Player::DiscardPile) {
            int reason = move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON;
            switch (reason) {
            case CardMoveReason::S_REASON_USE:
            case CardMoveReason::S_REASON_RESPONSE:
            case CardMoveReason::S_REASON_DISCARD:
                break;
            default: {
                foreach (int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->getSuit() == Card::Heart && room->getCardPlace(id) == Player::DiscardPile) {
                        auto players = room->findPlayersBySkillName(objectName(), true);
                        foreach (auto reimu, players)
                            d << SkillInvokeDetail(this, reimu, reimu);
                        return d;
                    }
                }
                break;
            }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        auto t = room->askForPlayerChosen(invoke->invoker, room->getAllPlayers(), objectName(), "@qixiang-select", true, true);
        if (t != nullptr) {
            invoke->targets << t;
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->targets.first()->drawCards(1);
        return false;
    }
};

class Fengmo : public TriggerSkill
{
public:
    Fengmo()
        : TriggerSkill("fengmo")
    {
        events << CardResponded << CardUsed << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct c = data.value<PhaseChangeStruct>();
            if (c.to == Player::NotActive) {
                foreach (auto p, room->getAllPlayers())
                    p->setMark("fengmoRecord", 0);
            }
        } else {
            ServerPlayer *player = nullptr;
            const Card *card = nullptr;
            if (triggerEvent == CardUsed) {
                player = data.value<CardUseStruct>().from;
                card = data.value<CardUseStruct>().card;
            } else if (triggerEvent == CardResponded) {
                CardResponseStruct response = data.value<CardResponseStruct>();
                player = response.m_from;
                if (response.m_isUse)
                    card = response.m_card;
            }

            if (player != nullptr && card != nullptr && card->isKindOf("BasicCard") && room->getCurrent() != nullptr && room->getCurrent()->isAlive()
                && room->getCurrent()->getPhase() != Player::NotActive) {
                int m = player->getMark("fengmoRecord");
                player->setMark("fengmoRecord", m + 1);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;

        ServerPlayer *player = nullptr;
        const Card *card = nullptr;
        if (triggerEvent == CardUsed) {
            player = data.value<CardUseStruct>().from;
            card = data.value<CardUseStruct>().card;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct response = data.value<CardResponseStruct>();
            player = response.m_from;
            if (response.m_isUse)
                card = response.m_card;
        } else
            return d;

        if (player == nullptr || card == nullptr)
            return d;

        if (room->getCurrent() != nullptr && room->getCurrent()->isAlive() && room->getCurrent()->getPhase() != Player::NotActive && player != nullptr
            && card->isKindOf("BasicCard") && player->getMark("fengmoRecord") == 1) {
            foreach (ServerPlayer *reimu, room->findPlayersBySkillName(objectName())) {
                if (reimu != player)
                    d << SkillInvokeDetail(this, reimu, reimu, nullptr, false, player);
            }
        }

        return d;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        invoke->invoker->tag["fengmo_target"] = QVariant::fromValue(invoke->preferredTarget);
        const Card *usedcard = nullptr;
        if (triggerEvent == CardUsed) {
            usedcard = data.value<CardUseStruct>().card;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct response = data.value<CardResponseStruct>();
            usedcard = response.m_card;
        }
        QString prompt = "@fengmo:" + invoke->preferredTarget->objectName() + ":" + usedcard->objectName();
        invoke->invoker->tag["fengmo_target"] = QVariant::fromValue(invoke->preferredTarget);
        const Card *card = room->askForCard(invoke->invoker, ".|.|.|hand", prompt, data, Card::MethodDiscard, nullptr, false, objectName());
        return card != nullptr;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use;
        CardResponseStruct resp;
        ServerPlayer *p = nullptr;

        if (triggerEvent == CardUsed) {
            use = data.value<CardUseStruct>();
            p = use.from;
        } else {
            resp = data.value<CardResponseStruct>();
            p = resp.m_from; //m_who
        }

        if (p == nullptr)
            return false;

        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), p->objectName());

        JudgeStruct j;
        j.who = p;
        j.pattern = ".|red";
        j.reason = objectName();
        j.negative = true;
        j.good = false;

        room->judge(j);
        if (j.isBad() && !j.ignore_judge) {
            if (triggerEvent == CardUsed) {
                use.nullified_list << "_ALL_TARGETS";
                data = QVariant::fromValue(use);
            } else {
                resp.m_isNullified = true;
                data = QVariant::fromValue(resp);
            }
        }
        return false;
    }
};

class Boli : public TriggerSkill
{
public:
    Boli()
        : TriggerSkill("boli$")
    {
        events << AskForRetrial;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if ((judge->who == nullptr) || !judge->who->isAlive() || judge->card->getSuit() == Card::Heart)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> details;
        foreach (ServerPlayer *reimu, room->getAllPlayers()) {
            if (reimu->hasLordSkill(objectName())) {
                foreach (ServerPlayer *p, room->getOtherPlayers(reimu)) {
                    if (!p->isKongcheng()) {
                        details << SkillInvokeDetail(this, reimu, reimu);
                        break;
                    }
                }
            }
        }
        return details;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *reimu = invoke->invoker;
        JudgeStruct *judge = data.value<JudgeStruct *>();
        QString prompt = "judge:" + judge->who->objectName() + ":" + judge->reason;
        return room->askForSkillInvoke(reimu, "boli", data, prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *reimu = invoke->invoker;
        JudgeStruct *judge = data.value<JudgeStruct *>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, reimu->objectName(), judge->who->objectName());

        foreach (ServerPlayer *p, room->getOtherPlayers(reimu)) {
            QStringList prompts;
            prompts << "@boli-retrial" << judge->who->objectName() << reimu->objectName() << judge->reason;

            const Card *heartcard = room->askForCard(p, ".H", prompts.join(":"), data, Card::MethodResponse, judge->who, true, objectName());
            if (heartcard != nullptr) {
                room->retrial(heartcard, p, judge, objectName(), false);
                break;
            }
        }
        return false;
    }
};

MofaCard::MofaCard()
{
    will_throw = true;
    target_fixed = true;
    handling_method = Card::MethodDiscard;
}

void MofaCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    room->sendLog("#mofa_notice", source, "mofa");
    source->setFlags("mofa_invoked");
}

class MofaVS : public OneCardViewAsSkill
{
public:
    MofaVS()
        : OneCardViewAsSkill("mofa")
    {
        filter_pattern = ".!";
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("MofaCard") && !player->isNude();
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        MofaCard *card = new MofaCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Mofa : public TriggerSkill
{
public:
    Mofa()
        : TriggerSkill("mofa")
    {
        events << PreCardUsed << ConfirmDamage;
        view_as_skill = new MofaVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if ((use.from != nullptr) && use.from->hasFlag("mofa_invoked") && (use.card->isKindOf("Slash") || use.card->isNDTrick()))
                room->setCardFlag(use.card, "mofa_card");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if ((damage.card != nullptr) && damage.card->hasFlag("mofa_card"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, nullptr, damage.from, nullptr, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *marisa = invoke->invoker;
            if (marisa != nullptr) {
                room->sendLog("#TouhouBuff", marisa, objectName());
                QList<ServerPlayer *> logto;
                logto << damage.to;
                room->sendLog("#mofa_damage", marisa, QString::number(damage.damage + 1), logto, QString::number(damage.damage));
            }
            damage.damage = damage.damage + 1;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

class Qinmian : public TriggerSkill
{
public:
    Qinmian()
        : TriggerSkill("qinmian")
    {
        events = {CardsMoveOneTime};
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
        if (from != nullptr && from->isAlive() && from->hasSkill(this)) {
            for (int i = 0; i < move.from_places.length(); ++i) {
                const Card *c = Sanguosha->getCard(move.card_ids[i]);
                if (c->getSuit() == Card::Spade) {
                    if (move.from_places[i] == Player::PlaceEquip
                        || (move.from_places[i] == Player::PlaceHand && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)))
                        return {SkillInvokeDetail(this, from, from)};
                }
            }
        }

        return {};
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->drawCards(1, objectName());
        return false;
    }
};

WuyuCard::WuyuCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "wuyu_attach";
}

void WuyuCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    const QList<ServerPlayer *> &targets = card_use.to;

    ServerPlayer *marisa = targets.first();
    room->setPlayerFlag(marisa, "wuyuInvoked");

    room->notifySkillInvoked(marisa, "wuyu");
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), marisa->objectName(), "wuyu", QString());
    room->obtainCard(marisa, this, reason);
}

bool WuyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasLordSkill("wuyu") && to_select != Self && !to_select->hasFlag("wuyuInvoked");
}

class WuyuVS : public OneCardViewAsSkill
{
public:
    WuyuVS()
        : OneCardViewAsSkill("wuyu_attach")
    {
        attached_lord_skill = true;
        filter_pattern = ".|spade|.|."; //".|spade|.|hand"
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasLordSkill("wuyu") && !p->hasFlag("wuyuInvoked"))
                return true;
        }
        return false;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        WuyuCard *card = new WuyuCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class Wuyu : public TriggerSkill
{
public:
    Wuyu()
        : TriggerSkill("wuyu$")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Death << Revive << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent != EventPhaseChanging) { //the case operating attach skill
            static QString attachName = "wuyu_attach";
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasLordSkill(this, true))
                    lords << p;
            }

            if (lords.length() > 1) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->hasSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else if (lords.length() == 1) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasLordSkill(this, true) && p->hasSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                    else if (!p->hasLordSkill(this, true) && !p->hasSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                }
            }
        } else {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from == Player::Play) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("wuyuInvoked"))
                        room->setPlayerFlag(p, "-wuyuInvoked");
                }
            }
        }
    }
};

SaiqianCard::SaiqianCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "saiqian_attach";
}

bool SaiqianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasSkill("saiqian", false, false) && to_select != Self && !to_select->hasFlag("saiqianInvoked");
}

void SaiqianCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    const QList<ServerPlayer *> &targets = card_use.to;

    ServerPlayer *reimu = targets.first();
    room->setPlayerFlag(reimu, "saiqianInvoked");
    room->notifySkillInvoked(reimu, "saiqian");
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), reimu->objectName(), "saiqian", QString());
    room->obtainCard(reimu, this, reason, false);

    //start effect
    QStringList saiqian_str;
    saiqian_str << "losehp_saiqian"
                << "discard_saiqian"
                << "cancel_saiqian";
    RecoverStruct recov;
    recov.who = reimu;
    recov.reason = "saiqian";
    while (!saiqian_str.isEmpty() && reimu->isAlive()) {
        QString choice = room->askForChoice(reimu, "saiqian", saiqian_str.join("+"));
        if (choice == "cancel_saiqian")
            return;
        else if (choice == "losehp_saiqian") {
            room->sendLog("#saiqian_lose", reimu, "saiqian");
            room->loseHp(reimu);
            room->recover(source, recov);
        } else if (choice == "discard_saiqian") {
            const Card *heartcard = room->askForCard(reimu, ".H", "@saiqian-discard:" + source->objectName(), QVariant::fromValue(source));
            if (heartcard != nullptr)
                room->recover(source, recov);
        }
        saiqian_str.removeOne(choice);
    }
}

class SaiqianVS : public ViewAsSkill
{
public:
    SaiqianVS()
        : ViewAsSkill("saiqian_attach")
    {
        attached_lord_skill = true;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasSkill("saiqian") && !p->hasFlag("saiqianInvoked"))
                return true;
        }
        return false;
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        return !to_select->isEquipped() && selected.length() < 3;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() > 0 && cards.length() <= 3) {
            SaiqianCard *card = new SaiqianCard;
            card->addSubcards(cards);
            return card;
        } else
            return nullptr;
    }
};

class Saiqian : public TriggerSkill
{
public:
    Saiqian()
        : TriggerSkill("saiqian")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging << Death << Debut << Revive << GeneralShown;
        show_type = "static";
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent != EventPhaseChanging) { //the case operating attach skill
            QList<ServerPlayer *> reimus;
            static QString attachName = "saiqian_attach";
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(this, true))
                    reimus << p;
            }

            if (reimus.length() > 1) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->hasSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else if (reimus.length() == 1) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasSkill(this, true) && p->hasSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                    else if (!p->hasSkill(this, true) && !p->hasSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                }
            }

        } else {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from == Player::Play) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("saiqianInvoked"))
                        room->setPlayerFlag(p, "-saiqianInvoked");
                }
            }
        }
    }
};

class JiezouVS : public OneCardViewAsSkill
{
public:
    JiezouVS()
        : OneCardViewAsSkill("jiezou")
    {
        filter_pattern = ".";
        response_or_use = true;
    }

    const Card *viewAs(const Card *c) const override
    {
        Snatch *sn = new Snatch(Card::SuitToBeDecided, -1);
        sn->addSubcard(c);
        sn->setSkillName("jiezou");
        return sn;
    }
};

class Jiezou : public TriggerSkill
{
public:
    Jiezou()
        : TriggerSkill("jiezou")
    {
        events << CardUsed << BeforeCardsMove << CardsMoveOneTime;
        view_as_skill = new JiezouVS;
    }

    // judgement:
    // BeforeCardsMove: All the spade cards which moves from Equip area by the movement of snatch generated by jiezou are recorded as flag "jiezou"
    // CardsMoveOneTime: If the movement contains a card marked as flag "jiezou", judge if it is moved to someone's Hand area and the movement is of snatch generated by jiezou. Must clear flag "jiezou" of all cards after card move.
    // Since nothing should be triggered during record, we can only set a flag on the user of Jiezou and judge it during triggerable
    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_GOTCARD) && move.reason.m_skillName == objectName()
                && move.reason.m_eventName == "snatch") {
                for (int i = 0; i < move.card_ids.length(); ++i) {
                    int id = move.card_ids.value(i);
                    const Card *c = Sanguosha->getCard(id);
                    Player::Place fromPlace = move.from_places.value(i);
                    if (c->getSuit() == Card::Spade && fromPlace == Player::PlaceEquip)
                        c->setFlags("jiezou");
                }
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            for (int i = 0; i < move.card_ids.length(); ++i) {
                int id = move.card_ids.value(i);
                const Card *c = Sanguosha->getCard(id);
                if (c->hasFlag("jiezou")) {
                    if (((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_GOTCARD) && move.reason.m_skillName == objectName()
                        && move.reason.m_eventName == "snatch" && move.to != nullptr && move.to_place == Player::PlaceHand && room->getCurrent() == move.to
                        && room->getCurrent()->isAlive() && room->getCurrent()->getPhase() == Player::Play) {
                        move.to->setFlags("jiezouSnatchedSpadeEquip");
                        move.to->setMark("jiezouSnatchedSpadeEquip", id);
                    }

                    c->setFlags("-jiezou");
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == BeforeCardsMove) {
            return {};
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Snatch") && use.card->getSkillName() == objectName() && use.card->getSuit() != Card::Spade && use.from != nullptr
                && use.from->getPhase() == Player::Play)
                return {SkillInvokeDetail(this, use.from, use.from, nullptr, true, nullptr, false)};

        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to != nullptr && move.to->hasFlag("jiezouSnatchedSpadeEquip") && move.to->isAlive() && move.to->isCurrent() && move.to->getPhase() == Player::Play) {
                ServerPlayer *sp = room->findPlayerByObjectName(move.to->objectName());
                return {SkillInvokeDetail(this, sp, sp, nullptr, true, nullptr, false)};
            }
        }

        return {};
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        int via = 1;
        int card_id = -1;
        if (invoke->invoker->hasFlag("jiezouSnatchedSpadeEquip")) {
            card_id = invoke->invoker->getMark("jiezouSnatchedSpadeEquip");
            via = 2;
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card_id = use.card->getEffectiveId();
        }

        if (card_id != -1) {
            LogMessage l;
            l.type = QString("$jiezou-skip%1").arg(via);
            l.from = invoke->invoker;
            l.arg = objectName();
            l.card_str = QString::number(card_id);
            room->sendLog(l);
        }

        invoke->invoker->setFlags("Global_PlayPhaseTerminated");

        return false;
    }
};

class JiezouNdl : public TargetModSkill
{
public:
    JiezouNdl()
        : TargetModSkill("#jiezou-ndl")
    {
        pattern = "Snatch";
    }

    int getDistanceLimit(const Player *from, const Card *card) const override
    {
        if (from->hasSkill("jiezou") && card->getSuit() == Card::Spade && card->getSkillName() == "jiezou")
            return 1000;

        return 0;
    }
};

ShoucangCard::ShoucangCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
    m_skillName = "shoucang";
}

void ShoucangCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;

    foreach (int id, subcards)
        room->showCard(source, id);
    room->sendLog("#shoucang_max", source, "shoucang", QList<ServerPlayer *>(), QString::number(subcards.length()));
    source->tag["shoucang"] = QVariant::fromValue(subcards.length());
}

class ShoucangVS : public ViewAsSkill
{
public:
    ShoucangVS()
        : ViewAsSkill("shoucang")
    {
        response_pattern = "@@shoucang";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        if (to_select->isEquipped())
            return false;
        if (selected.isEmpty())
            return !to_select->isEquipped();
        else if (selected.length() < 4) {
            foreach (const Card *c, selected) {
                if (to_select->getSuit() == c->getSuit())
                    return false;
            }
            return true;
        } else
            return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() > 0 && cards.length() < 5) {
            ShoucangCard *card = new ShoucangCard;
            card->addSubcards(cards);

            return card;
        } else
            return nullptr;
    }
};
class Shoucang : public TriggerSkill
{
public:
    Shoucang()
        : TriggerSkill("shoucang")
    {
        events << EventPhaseStart << EventPhaseChanging;
        view_as_skill = new ShoucangVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && (change.player != nullptr) && change.player->isAlive())
                room->setPlayerMark(change.player, "shoucang", 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *marisa = data.value<ServerPlayer *>();
            if ((marisa != nullptr) && marisa->hasSkill(this) && marisa->isAlive() && marisa->getPhase() == Player::Discard && !marisa->isKongcheng())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, marisa, marisa);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        return room->askForUseCard(invoke->invoker, "@@shoucang", "@shoucang") != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *marisa = invoke->invoker;
        int num = marisa->tag["shoucang"].toInt();
        marisa->tag.remove("shoucang");
        if (num > 0)
            room->setPlayerMark(marisa, "shoucang", num);
        return false;
    }
};
class ShoucangMax : public MaxCardsSkill
{
public:
    ShoucangMax()
        : MaxCardsSkill("#shoucang")
    {
    }

    int getExtra(const Player *target) const override
    {
        return target->getMark("shoucang");
    }
};

BaoyiCard::BaoyiCard()
{
    will_throw = true;
    target_fixed = true;
    handling_method = Card::MethodDiscard;
}

void BaoyiCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;

    foreach (int id, subcards) {
        Card *c = Sanguosha->getCard(id);
        if (c->getSuit() == Card::Spade) {
            room->setPlayerFlag(source, "baoyi");
            break;
        }
    }
    source->tag["baoyi"] = QVariant::fromValue(subcards.length());
}
class BaoyiVS : public ViewAsSkill
{
public:
    BaoyiVS()
        : ViewAsSkill("baoyi")
    {
        response_pattern = "@@baoyi";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const override
    {
        return to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        bool has_delaytrick = false;
        BaoyiCard *card = new BaoyiCard;
        foreach (int id, Self->getJudgingAreaID()) {
            card->addSubcard(id);
            has_delaytrick = true;
        }
        if (cards.length() == 0 && !has_delaytrick) {
            delete card;
            return nullptr;
        }
        if (cards.length() > 0)
            card->addSubcards(cards);
        return card;
    }
};
class Baoyi : public TriggerSkill
{
public:
    Baoyi()
        : TriggerSkill("baoyi")
    {
        events << EventPhaseStart;
        view_as_skill = new BaoyiVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *marisa = data.value<ServerPlayer *>();
        if ((marisa != nullptr) && marisa->isAlive() && marisa->getPhase() == Player::Start && !marisa->isAllNude() && marisa->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, marisa, marisa);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->setPlayerFlag(invoke->invoker, "Global_InstanceUse_Failed");
        return room->askForUseCard(invoke->invoker, "@@baoyi", "@baoyi") != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *marisa = invoke->invoker;
        int num = marisa->tag["baoyi"].toInt();
        marisa->tag.remove("baoyi");

        while ((num--) != 0) {
            QList<ServerPlayer *> listt;
            foreach (ServerPlayer *p, room->getOtherPlayers(marisa)) {
                if (marisa->canSlash(p, nullptr, false))
                    listt << p;
            }
            if (listt.isEmpty())
                break;
            ServerPlayer *target = room->askForPlayerChosen(marisa, listt, "baoyi", "@@baoyi_chosen:" + QString::number(num + 1), true, true);
            if (target == nullptr)
                break;
            CardUseStruct carduse;
            Slash *slash = new Slash(Card::NoSuit, 0);
            //slash->deleteLater();
            slash->setSkillName("baoyi");
            carduse.card = slash;
            carduse.from = marisa;
            carduse.to << target;
            room->useCard(carduse);
        }
        if (marisa->hasFlag("baoyi")) {
            marisa->setFlags("-baoyi");
            marisa->drawCards(2);
        }
        return false;
    }
};

class Zhize : public TriggerSkill
{
public:
    Zhize()
        : TriggerSkill("zhize")
    {
        frequency = Compulsory;
        events << DrawNCards << AfterDrawNCards;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        DrawNCardsStruct d = data.value<DrawNCardsStruct>();
        if (d.player != nullptr && d.player->isAlive() && !d.isInitial) {
            if (triggerEvent == DrawNCards) {
                if (d.player->hasSkill(this) && d.n >= 1)
                    return {SkillInvokeDetail(this, d.player, d.player, nullptr, true)};
            } else if (triggerEvent == AfterDrawNCards) {
                if (d.player->hasFlag(objectName())) {
                    foreach (ServerPlayer *p, room->getOtherPlayers(d.player)) {
                        if (!p->isKongcheng())
                            return {SkillInvokeDetail(this, d.player, d.player, nullptr, true, nullptr, false)};
                    }
                }
            }
        }

        return {};
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
            if (triggerEvent == DrawNCards) {
                DrawNCardsStruct d = data.value<DrawNCardsStruct>();
                --d.n;
                data = QVariant::fromValue(d);
            }

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == DrawNCards) {
            invoke->invoker->setFlags(objectName());
        } else {
            QList<ServerPlayer *> players;
            foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
                if (!p->isKongcheng())
                    players << p;
            }
            if (players.isEmpty())
                return false;

            ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, players, objectName(), "@zhize", false, true);

            QList<int> ids = target->handCards();
            int id = room->doGongxin(invoke->invoker, target, ids, objectName());
            invoke->invoker->tag.remove(objectName());
            room->obtainCard(invoke->invoker, id, false);
        }

        return false;
    }
};

ChunxiCard::ChunxiCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "chunxi";
}

bool ChunxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void ChunxiCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    LogMessage log;
    log.from = card_use.from;
    log.to = card_use.to;
    log.type = "#UseCard";
    log.card_str = card_use.card->toString(true);
    room->sendLog(log);

    room->showCard(card_use.from, subcards.first());

    // TODO : set target flag
    card_use.to.first()->setMark("chunxi", 1);
}

class ChunxiVS : public OneCardViewAsSkill
{
public:
    ChunxiVS()
        : OneCardViewAsSkill("chunxi")
    {
        response_pattern = "@@chunxi";
    }

    bool viewFilter(const Card *to_select) const override
    {
        return to_select->hasFlag("chunxi");
    }

    const Card *viewAs(const Card *card) const override
    {
        ChunxiCard *c = new ChunxiCard;
        c->addSubcard(card);
        return c;
    }
};

class Chunxi : public TriggerSkill
{
public:
    Chunxi()
        : TriggerSkill("chunxi")
    {
        events << CardsMoveOneTime;
        view_as_skill = new ChunxiVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e == CardsMoveOneTime) {
            if (room->getTag("FirstRound").toBool())
                return QList<SkillInvokeDetail>();

            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *reimu = qobject_cast<ServerPlayer *>(move.to);
            if (reimu != nullptr && reimu->hasSkill(this) && move.to_place == Player::PlaceHand && move.reason.m_skillName != objectName()) {
                foreach (int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->getSuit() == Card::Heart && room->getCardPlace(id) == Player::PlaceHand) {
                        ServerPlayer *owner = room->getCardOwner(id);
                        if ((owner != nullptr) && owner == reimu)
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, reimu, reimu);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *reimu = invoke->invoker;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        foreach (int id, move.card_ids) {
            if (Sanguosha->getCard(id)->getSuit() == Card::Heart && room->getCardPlace(id) == Player::PlaceHand) {
                ServerPlayer *owner = room->getCardOwner(id);
                if ((owner != nullptr) && owner == reimu)
                    room->setCardFlag(id, "chunxi");
            }
        }
        invoke->invoker->tag["chunxi_move"] = data;
        const Card *c = room->askForUseCard(reimu, "@@chunxi", "@chunxi");
        foreach (int id, move.card_ids)
            room->setCardFlag(id, "-chunxi");

        if (c != nullptr) {
            foreach (ServerPlayer *p, room->getOtherPlayers(reimu)) {
                if (p->getMark("chunxi") > 0) {
                    invoke->targets << p;
                    break;
                }
            }
        }

        foreach (ServerPlayer *p, room->getOtherPlayers(reimu))
            p->setMark("chunxi", 0);

        return c != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        int obtainId = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hs", objectName());
        CardMoveReason r(CardMoveReason::S_REASON_ROB, invoke->invoker->objectName(), objectName(), QString());
        room->obtainCard(invoke->invoker, Sanguosha->getCard(obtainId), r, false);

        return false;
    }
};

#pragma message WARN("todo_lwtmusou: find a new method for WuyuCost which could help player and AI to operate this skill more esaily")
BllmWuyuCard::BllmWuyuCard()
{
    mute = true;
    target_fixed = true;
}

void BllmWuyuCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *bllm = card_use.from;

    QStringList uselist;
    if (Analeptic::IsAvailable(bllm))
        uselist << "bllmshiyu";

    uselist << "bllmseyu";
    uselist << "dismiss";
    QString choice = room->askForChoice(bllm, "bllmwuyu", uselist.join("+"));
    if (choice == "dismiss")
        return;
    else if (choice == "bllmshiyu")
        room->useCard(CardUseStruct(new BllmShiyuCard, bllm));
    else if (choice == "bllmseyu")
        room->useCard(CardUseStruct(new BllmSeyuCard, bllm));
}

class BllmWuyuVS : public ViewAsSkill
{
public:
    BllmWuyuVS()
        : ViewAsSkill("bllmwuyu")
    {
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        if (player->getMark("@yu") == 0 && player->isKongcheng())
            return false;
        if (Analeptic::IsAvailable(player))
            return true;
        return true;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        if (pattern.contains("@@bllmwuyu"))
            return true;
        Analeptic card(Card::NoSuit, 0);
        if (player->isCardLimited(&card, Card::MethodUse))
            return false;

        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);
        return cardPattern != nullptr && cardPattern->match(player, &card);
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@bllmwuyu")
            return selected.isEmpty() && !to_select->isEquipped();
        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@bllmwuyu") {
            if (cards.length() != 1)
                return nullptr;
            BllmShiyuDummy *shiyu = new BllmShiyuDummy;
            shiyu->addSubcards(cards);
            return shiyu;
        } else {
            Analeptic card(Card::NoSuit, 0);
            if (Self->isCardLimited(&card, Card::MethodUse))
                return nullptr;

            const CardPattern *cardPattern = Sanguosha->getPattern(pattern);
            if (cardPattern != nullptr && cardPattern->match(Self, &card))
                return new BllmShiyuCard;
            else
                return new BllmWuyuCard;
        }
    }
};

class BllmWuyu : public PhaseChangeSkill
{
public:
    BllmWuyu()
        : PhaseChangeSkill("bllmwuyu")
    {
        view_as_skill = new BllmWuyuVS;
        related_mark = "@yu";
    }

    bool canPreshow() const override
    {
        return true;
    }

    static bool BllmWuyuCost(Room *room, ServerPlayer *bllm, QString prompt)
    {
        //tag for AI
        bllm->tag["wuyu_prompt"] = QVariant::fromValue(prompt);
        if (bllm->getMark("@yu") > 0 && bllm->askForSkillInvoke("bllmwuyu", "useyu:" + prompt)) {
            bllm->loseMark("@yu");
            room->sendLog("#InvokeSkill", bllm, prompt);
            room->notifySkillInvoked(bllm, prompt);
            return true;
        } else {
            const Card *card = room->askForCard(bllm, "..H", "@bllm-discard:" + prompt, QVariant(), "bllmwuyu");
            if (card != nullptr) {
                room->sendLog("#InvokeSkill", bllm, prompt);
                room->notifySkillInvoked(bllm, prompt);
                return true;
            }
        }
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *reimu = data.value<ServerPlayer *>();
        if ((reimu != nullptr) && reimu->hasSkill(this) && reimu->isAlive() && reimu->getPhase() == Player::Start) {
            int z = (reimu->getLostHp() + 1) - reimu->getMark("@yu");
            if (z > 0)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, reimu, reimu);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        //tag for AI
        invoke->invoker->tag["wuyu_prompt"] = QVariant::fromValue(objectName());
        return invoke->invoker->askForSkillInvoke(objectName());
    }

    bool onPhaseChange(ServerPlayer *player) const override
    {
        int z = (player->getLostHp() + 1) - player->getMark("@yu");
        player->gainMark("@yu", z);
        return false;
    }
};

class BllmCaiyu : public PhaseChangeSkill
{
public:
    BllmCaiyu()
        : PhaseChangeSkill("#bllmcaiyu")
    {
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *reimu = data.value<ServerPlayer *>();
        if ((reimu != nullptr) && reimu->hasSkill("bllmwuyu") && reimu->isAlive() && reimu->getPhase() == Player::Draw) {
            if (reimu->getMark("@yu") > 0 || reimu->canDiscard(reimu, "hs"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, reimu, reimu);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        return BllmWuyu::BllmWuyuCost(room, invoke->invoker, "bllmcaiyu");
    }

    bool onPhaseChange(ServerPlayer *player) const override
    {
        player->drawCards(1);
        return false;
    }
};

BllmSeyuCard::BllmSeyuCard()
{
    will_throw = false;
    target_fixed = true;
}

void BllmSeyuCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;

    room->setPlayerMark(source, "bllmseyu", source->getMark("bllmseyu") + 1);
}

const Card *BllmSeyuCard::validate(CardUseStruct &cardUse) const
{
    ServerPlayer *bllm = cardUse.from;
    Room *room = bllm->getRoom();
    if (BllmWuyu::BllmWuyuCost(room, bllm, "bllmseyu"))
        return cardUse.card;
    return nullptr;
}

class BllmSeyu : public TargetModSkill
{
public:
    BllmSeyu()
        : TargetModSkill("#bllmseyu")
    {
        frequency = NotFrequent;
        pattern = "Slash";
    }

    int getResidueNum(const Player *from, const Card *) const override
    {
        return from->getMark("bllmseyu");
    }
};

class BllmSeyuClear : public TriggerSkill
{
public:
    BllmSeyuClear()
        : TriggerSkill("#bllmseyu_clear")
    {
        events << EventPhaseChanging;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const override
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if ((change.player != nullptr) && change.player->getMark("bllmseyu") > 0)
            room->setPlayerMark(change.player, "bllmseyu", 0);
    }
};

class BllmMingyu : public TriggerSkill
{
public:
    BllmMingyu()
        : TriggerSkill("#bllmmingyu")
    {
        events << EventPhaseChanging;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        ServerPlayer *reimu = change.player;
        if ((reimu != nullptr) && reimu->hasSkill("bllmwuyu") && change.to == Player::Judge && !reimu->isSkipped(change.to)) {
            if (reimu->getMark("@yu") > 0 || reimu->canDiscard(reimu, "hs"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, reimu, reimu);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        return BllmWuyu::BllmWuyuCost(room, invoke->invoker, "bllmmingyu");
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->skip(Player::Judge);
        return false;
    }
};

BllmShiyuDummy::BllmShiyuDummy()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void BllmShiyuDummy::use(Room *, const CardUseStruct &) const
{
}

BllmShiyuCard::BllmShiyuCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodUse;
}

const Card *BllmShiyuCard::validate(CardUseStruct &cardUse) const
{
    ServerPlayer *bllm = cardUse.from;
    Room *room = bllm->getRoom();
    if (BllmWuyu::BllmWuyuCost(room, bllm, "bllmshiyu")) {
        room->setPlayerFlag(bllm, "Global_expandpileFailed");
        const Card *dummy = room->askForCard(bllm, "@@bllmwuyu", "@bllmshiyu-basics", QVariant(), Card::MethodNone);
        room->setPlayerFlag(bllm, "-Global_expandpileFailed");
        if (dummy != nullptr) {
            Analeptic *ana = new Analeptic(Card::SuitToBeDecided, -1);
            foreach (int id, dummy->getSubcards())
                ana->addSubcard(id);
            ana->setSkillName("bllmshiyu");
            ana->setFlags("Add_History"); //because the use reason is not equal CardUseStruct::CARD_USE_REASON_PLAY
            return ana;
        }
    }
    return nullptr;
}

const Card *BllmShiyuCard::validateInResponse(ServerPlayer *bllm) const
{
    Room *room = bllm->getRoom();
    if (BllmWuyu::BllmWuyuCost(room, bllm, "bllmshiyu")) {
        room->setPlayerFlag(bllm, "Global_expandpileFailed");
        const Card *dummy = room->askForCard(bllm, "@@bllmwuyu", "@bllmshiyu-basics", QVariant(), Card::MethodNone);
        room->setPlayerFlag(bllm, "-Global_expandpileFailed");
        if (dummy != nullptr) {
            Analeptic *ana = new Analeptic(Card::SuitToBeDecided, -1);
            foreach (int id, dummy->getSubcards())
                ana->addSubcard(id);
            ana->setSkillName("bllmshiyu");
            return ana;
        }
    }
    return nullptr;
}

class BllmShuiyu : public PhaseChangeSkill
{
public:
    BllmShuiyu()
        : PhaseChangeSkill("#bllmshuiyu")
    {
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *reimu = data.value<ServerPlayer *>();
        if ((reimu != nullptr) && reimu->hasSkill("bllmwuyu") && reimu->isAlive() && reimu->getPhase() == Player::Discard) {
            if (reimu->getMark("@yu") > 0 || reimu->canDiscard(reimu, "hs"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, reimu, reimu);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        return BllmWuyu::BllmWuyuCost(room, invoke->invoker, "bllmshuiyu");
    }

    bool onPhaseChange(ServerPlayer *player) const override
    {
        player->getRoom()->setPlayerFlag(player, "bllmshuiyu");
        return false;
    }
};

class BllmShuiyuMax : public MaxCardsSkill
{
public:
    BllmShuiyuMax()
        : MaxCardsSkill("#bllmshuiyu2")
    {
    }

    int getFixed(const Player *target) const override
    {
        if (target->hasFlag("bllmshuiyu"))
            return 4;
        return -1;
    }
};

class QiangyuVS : public ViewAsSkill
{
public:
    QiangyuVS()
        : ViewAsSkill("qiangyu")
    {
        response_pattern = "@@qiangyu!";
    }

    static int getQiangyuDiscardNum(const Player *p)
    {
        // qiangyu mark stores current value plus 1 since skill trigger is judged by the time cost is done
        // Do not break when turn broken before Qiangyu effect
        return p->getMark("qiangyu") + 1;
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        if (to_select->isEquipped() || Self->isJilei(to_select))
            return false;
        if (selected.length() >= getQiangyuDiscardNum(Self))
            return false;

        return true;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        foreach (const Card *card, cards) {
            if (Self->isJilei(card))
                return nullptr;
        }

        bool ok = cards.length() == getQiangyuDiscardNum(Self);
        if (cards.length() == 1 && cards.first()->getSuit() == Card::Spade)
            ok = true;

        if (ok) {
            DummyCard *dc = new DummyCard;
            dc->addSubcards(cards);
            return dc;
        }

        return nullptr;
    }
};

class Qiangyu : public TriggerSkill
{
public:
    Qiangyu()
        : TriggerSkill("qiangyu")
    {
        events << CardsMoveOneTime << BeforeCardsMove << TurnStart;
        view_as_skill = new QiangyuVS;
    }

    void record(TriggerEvent e, Room *room, QVariant &) const override
    {
        if (e == TurnStart) {
            foreach (ServerPlayer *ps, room->getAllPlayers()) {
                if (ps->getMark(objectName()) > 0)
                    room->setPlayerMark(ps, objectName(), 0);
            }
        }
    }

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (room->getTag("FirstRound").toBool())
            return {};
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        if (triggerEvent == BeforeCardsMove) {
            if (move.reason.m_reason == CardMoveReason::S_REASON_DRAW && move.reason.m_extraData.toString() != "initialDraw") {
                ServerPlayer *marisa = qobject_cast<ServerPlayer *>(move.to);
                if (marisa != nullptr && marisa->hasSkill(this))
                    return {SkillInvokeDetail(this, marisa, marisa)};
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            if (move.reason.m_reason == CardMoveReason::S_REASON_DRAW && move.reason.m_extraData.toString() != "initialDraw") {
                ServerPlayer *marisa = qobject_cast<ServerPlayer *>(move.to);
                if (marisa != nullptr && marisa->hasFlag("qiangyu"))
                    return {SkillInvokeDetail(this, marisa, marisa, nullptr, true)};
            }
        }
        return {};
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        bool r = TriggerSkill::cost(triggerEvent, room, invoke, data);
        if (r && triggerEvent == BeforeCardsMove)
            room->setPlayerMark(invoke->invoker, objectName(), invoke->invoker->getMark(objectName()) + 1);

        return r;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            //notice that we need keep these lists with same length
            move.card_ids << room->getNCards(2);
            move.from_places << Player::DrawPile << Player::DrawPile;
            move.from_pile_names << QString() << QString();
            move.origin_from_places << move.origin_from_places.first() << move.origin_from_places.first();
            move.origin_from_pile_names << move.origin_from_pile_names.first() << move.origin_from_pile_names.first();
            move.open << move.open.first() << move.open.first();

            data = QVariant::fromValue(move);
            invoke->invoker->setFlags("qiangyu");
        } else {
            invoke->invoker->setFlags("-qiangyu");
            if (invoke->invoker->isKongcheng())
                return false;
            QList<const Card *> hc = invoke->invoker->getHandcards();
            const Card *spade = nullptr;
            foreach (const Card *c, hc) {
                if (invoke->invoker->isJilei(c))
                    hc.removeOne(c);
                else if (c->getSuit() == Card::Spade)
                    spade = c;
            }

            if (spade == nullptr && hc.length() < QiangyuVS::getQiangyuDiscardNum(invoke->invoker)) {
                // discard all other cards, and jilei show remaining cards
                DummyCard dc;
                dc.addSubcards(hc);
                room->throwCard(&dc, invoke->invoker);
                room->doJileiShow(invoke->invoker, invoke->invoker->handCards());
                return false;
            }

            const Card *card = room->askForCard(invoke->invoker, "@@qiangyu!", "@qiangyu-discard:::" + QString::number(QiangyuVS::getQiangyuDiscardNum(invoke->invoker)), data);
            if (card == nullptr) {
                // force discard!!!
                DummyCard dc;

                if (hc.length() > QiangyuVS::getQiangyuDiscardNum(invoke->invoker)) {
                    qShuffle(hc);
                    dc.addSubcards(hc.mid(0, QiangyuVS::getQiangyuDiscardNum(invoke->invoker)));
                } else if (spade != nullptr) {
                    dc.addSubcard(spade);
                } else {
                    // ?
                }

                room->throwCard(&dc, invoke->invoker);
            }
        }
        return false;
    }
};

class Mokai : public TriggerSkill
{
public:
    Mokai()
        : TriggerSkill("mokai")
    {
        events << CardUsed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        if (e == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("TrickCard") && use.card->isBlack() && use.from->hasSkill(this) && use.from->getPhase() == Player::Play)
                return {SkillInvokeDetail(this, use.from, use.from)};
        }
        return {};
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        return room->askForCard(invoke->invoker, "EquipCard", "@mokai", data, Card::MethodDiscard, nullptr, false, objectName()) != nullptr;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->drawCards(2);
        return false;
    }
};

DfgzmSiyuCard::DfgzmSiyuCard()
{
    will_throw = false;
}

void DfgzmSiyuCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    const QList<ServerPlayer *> &targets = card_use.to;

    ServerPlayer *target = targets.first();
    room->obtainCard(target, this, false);
    QVariantMap m = source->tag.value("dfgzmsiyu_selected", QVariantMap()).toMap();
    m[target->objectName()] = subcardsLength();
    source->tag["dfgzmsiyu_selected"] = m;
}

class DfgzmsiyuVS : public ViewAsSkill
{
public:
    DfgzmsiyuVS()
        : ViewAsSkill("dfgzmsiyu")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("DfgzmSiyuCard") && !player->isKongcheng();
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        if (selected.length() >= 2)
            return false;
        return !to_select->isEquipped();
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (!cards.isEmpty()) {
            DfgzmSiyuCard *card = new DfgzmSiyuCard;
            card->addSubcards(cards);
            return card;
        }

        return nullptr;
    }
};

class DfgzmSiyu : public TriggerSkill
{
public:
    DfgzmSiyu()
        : TriggerSkill("dfgzmsiyu")
    {
        events << EventPhaseChanging;
        view_as_skill = new DfgzmsiyuVS;
    }

    void record(TriggerEvent, Room *, QVariant &data) const override
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::RoundStart)
            change.player->tag.remove("dfgzmsiyu_selected");
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            if (change.player->isAlive()) {
                QList<SkillInvokeDetail> d;
                QVariantMap siyu = change.player->tag.value("dfgzmsiyu_selected", QVariantMap()).toMap();
                foreach (const QString &sy, siyu.keys()) {
                    ServerPlayer *syP = room->findPlayerByObjectName(sy);
                    if (syP != nullptr && syP->isAlive() && !syP->isKongcheng())
                        d << SkillInvokeDetail(this, change.player, change.player, nullptr, true, syP, false);
                }

                return d;
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->tag["dfgzmsiyu_invokeTarget"] = QVariant::fromValue(invoke->preferredTarget);
        return true;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QVariantMap siyu = invoke->invoker->tag.value("dfgzmsiyu_selected", QVariantMap()).toMap();
        int syNum = siyu.value(invoke->targets.first()->objectName(), 0).toInt();
        if (syNum > 0) {
            QList<int> enabled_ids = invoke->targets.first()->handCards();
            int id = room->doGongxin(invoke->invoker, invoke->targets.first(), enabled_ids, objectName(), false);
            room->obtainCard(invoke->invoker, id, false);
            invoke->invoker->tag.remove(objectName());
        }
        return false;
    }
};

class QishuTargetMod : public TargetModSkill
{
public:
    QishuTargetMod()
        : TargetModSkill("#qishu-mod")
    {
        pattern = ".";
    }

    static bool isLastHandCard(const Player *player, const Card *card)
    {
        QList<int> subcards = card->getSubcards();
        if (subcards.length() == 0 || player->isKongcheng())
            return false;
        int handnum = 0;
        foreach (const Card *c, player->getHandcards()) {
            if (subcards.contains(c->getEffectiveId()))
                handnum++;
            else {
                handnum = 0;
                break;
            }
        }
        if (handnum >= player->getHandcardNum())
            return true;
        else
            return false;
    }

    int getDistanceLimit(const Player *from, const Card *card) const override
    {
        if (from->hasSkill("qishu") && isLastHandCard(from, card))
            return 1000;
        else
            return 0;
    }

    int getExtraTargetNum(const Player *player, const Card *card) const override
    {
        if (player->hasSkill("qishu") && player->getPhase() == Player::Play && isLastHandCard(player, card) && (card->isKindOf("Slash") || card->isNDTrick()))
            return 1000;
        else
            return 0;
    }
};

class Qishu : public TriggerSkill
{
public:
    Qishu()
        : TriggerSkill("qishu")
    {
        events << PreCardUsed;
    }

    static bool isQishu(QList<ServerPlayer *> players, const Card *card)
    {
        if (card->isKindOf("GlobalEffect") || card->isKindOf("AOE"))
            return false;
        if (card->isKindOf("IronChain"))
            return players.length() > 2;
        return true;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        //for log
        if (use.to.length() > 1 && QishuTargetMod::isLastHandCard(use.from, use.card)) {
            if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
                if (isQishu(use.to, use.card))
                    room->notifySkillInvoked(use.from, "qishu");
            }
        }
    }
};

// #pragma message WARN("todo_lwtmusou: rewrite siyu, notice that skill records (flag, tag, marks, etc.) should be updated while siyu TurnBroken")
// Fs: should check in every skill, better write the most records clear into the eventphasechanging(to = notactive) event
// Fs: it's no need to check at here now, the extra turn is inserted after the whole round finished
// Fs: seems like the only skill that need clean up in this skill is 'shitu' in th99 and 'qinlue' in touhougod.....
class HpymSiyu : public TriggerSkill
{
public:
    HpymSiyu()
        : TriggerSkill("hpymsiyu")
    {
        events << EnterDying << EventPhaseChanging; //<< PostHpReduced
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == EnterDying) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.who->hasSkill(this) && !dying.who->isCurrent() && dying.who->getMark("siyuinvoke") == 0)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dying.who, dying.who, nullptr, true);
        } else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->getMark("siyuinvoke") > 0 && change.to == Player::NotActive)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, nullptr, true);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == EnterDying) {
            if (!invoke->invoker->faceUp())
                invoke->invoker->turnOver();

            QList<const Card *> tricks = invoke->invoker->getJudgingArea();
            foreach (const Card *trick, tricks) {
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, invoke->invoker->objectName());
                room->throwCard(trick, reason, nullptr);
            }

            // AmazingGrace::clearRestCards();  private function
            //******************************
            room->clearAG();
            QVariantList ag_list = room->getTag("AmazingGrace").toList();
            room->removeTag("AmazingGrace");
            if (!ag_list.isEmpty()) {
                DummyCard *dummy = new DummyCard(VariantList2IntList(ag_list));
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), "amazing_grace", QString());
                room->throwCard(dummy, reason, nullptr);
                delete dummy;
            }
            //******************************

            foreach (int id, Sanguosha->getRandomCards()) {
                if (room->getCardPlace(id) == Player::PlaceTable)
                    room->moveCardTo(Sanguosha->getCard(id), nullptr, Player::DiscardPile, true);
                if (Sanguosha->getCard(id)->hasFlag("using"))
                    room->setCardFlag(Sanguosha->getCard(id), "-using");
            }

            invoke->invoker->addMark("siyuinvoke");
            room->sendLog("#TriggerSkill", invoke->invoker, objectName());
            room->notifySkillInvoked(invoke->invoker, objectName());

            room->sendLog("#touhouExtraTurn", invoke->invoker, objectName());

            invoke->invoker->gainAnExtraTurn();

            //only for UI //While EnterDying, need QuitDying to change '_m_saveMeIcon'
            JsonArray arg;
            arg << QSanProtocol::S_GAME_EVENT_PLAYER_QUITDYING << invoke->invoker->objectName();
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

            //clear currentDying tag
            QStringList currentdying = room->getTag("CurrentDying").toStringList();
            currentdying.removeOne(invoke->invoker->objectName());
            room->setTag("CurrentDying", QVariant::fromValue(currentdying));

            room->setPlayerFlag(invoke->invoker, "-Global_Dying");

            // This is a bad design.
            // A better way may be set turnbreak flag and throw a ProcessBroken.
            throw TurnBroken;

            Q_UNREACHABLE();
            //return true; // prevent enterdying
        } else if (triggerEvent == EventPhaseChanging) {
            invoke->invoker->setMark("siyuinvoke", 0);
            if (invoke->invoker->getHp() < invoke->invoker->dyingThreshold()) {
                room->notifySkillInvoked(invoke->invoker, "hpymsiyu");
                room->enterDying(invoke->invoker, nullptr);
            }
        }

        return false;
    }
};

class Juhe : public TriggerSkill
{
public:
    Juhe()
        : TriggerSkill("juhe")
    {
        events << DrawNCards << AfterDrawNCards;
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        DrawNCardsStruct draw = data.value<DrawNCardsStruct>();

        QList<SkillInvokeDetail> d;
        if (triggerEvent == DrawNCards) {
            if (draw.player->hasSkill(this))
                d << SkillInvokeDetail(this, draw.player, draw.player);
        } else if (triggerEvent == AfterDrawNCards) {
            if (draw.player->hasFlag("juheUsed") && draw.player->getHp() > 0)
                d << SkillInvokeDetail(this, draw.player, draw.player, nullptr, true);
        }
        return d;
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == DrawNCards)
            return invoke->invoker->askForSkillInvoke(objectName());
        else
            invoke->invoker->setFlags("-juheUsed");
        return true;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *player = invoke->invoker;
        if (triggerEvent == DrawNCards) {
            DrawNCardsStruct draw = data.value<DrawNCardsStruct>();
            draw.n = draw.n + 3;
            data = QVariant::fromValue(draw);
            room->setPlayerFlag(player, "juheUsed");
        } else if (triggerEvent == AfterDrawNCards)
            room->askForDiscard(player, "juhe", player->getHp(), player->getHp(), false, false, "juhe_discard:" + QString::number(player->getHp()));
        return false;
    }
};

YinyangCard::YinyangCard()
{
}

bool YinyangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return !to_select->isKongcheng() && targets.isEmpty() && to_select != Self;
}

void YinyangCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    const Card *card1 = nullptr;
    const Card *card2 = nullptr;

    //The First Discard
    QList<const Card *> hc1 = effect.to->getHandcards();
    foreach (const Card *c, hc1) {
        if (effect.to->isJilei(c))
            hc1.removeOne(c);
    }
    if (hc1.length() == 0) {
        // jilei show all cards
        room->doJileiShow(effect.to, effect.to->handCards());
        return;
    }
    card1 = room->askForCard(effect.to, ".|.|.|hand!", "@yinyang_discard");
    if (card1 == nullptr) {
        // force discard!!!
        int x = qrand() % hc1.length();
        card1 = hc1.value(x);
        room->throwCard(card1, effect.to);
    }

    if (card1 != nullptr) { //for AI
        effect.from->tag["yinyang_card"] = QVariant::fromValue(card1);
        effect.from->tag["yinyang_target"] = QVariant::fromValue(effect.to);
    }

    //The Second Discard
    if (effect.from->isKongcheng())
        return;
    QList<const Card *> hc2 = effect.from->getHandcards();
    foreach (const Card *c, hc2) {
        if (effect.from->isJilei(c))
            hc2.removeOne(c);
    }
    if (hc2.length() == 0) {
        // jilei show all cards
        room->doJileiShow(effect.from, effect.from->handCards());
        return;
    }
    card2 = room->askForCard(effect.from, ".|.|.|hand!", "@yinyang_discard");
    if (card2 == nullptr) {
        // force discard!!!
        if (hc2.length() > 0) {
            int x = qrand() % hc2.length();
            card2 = hc2.value(x);
            room->throwCard(card2, effect.from);
        }
    }

    effect.from->tag.remove("yinyang_card");
    effect.from->tag.remove("yinyang_target");
    if ((card1 != nullptr) && (card2 != nullptr)) {
        if (card1->isRed() == card2->isRed()) {
            effect.from->drawCards(2);
            room->loseHp(effect.from);
        } else {
            effect.to->drawCards(1);
            room->recover(effect.to, RecoverStruct());
        }
    }
}

class Yinyang : public ZeroCardViewAsSkill
{
public:
    Yinyang()
        : ZeroCardViewAsSkill("yinyang")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("YinyangCard");
    }

    const Card *viewAs() const override
    {
        return new YinyangCard;
    }
};

class Lingji : public TriggerSkill
{
public:
    Lingji()
        : TriggerSkill("lingji")
    {
        events << CardsMoveOneTime << CardUsed << CardResponded << EventPhaseStart << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *, QVariant &data) const override
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *reimu = qobject_cast<ServerPlayer *>(move.from);
            if ((reimu == nullptr) || !reimu->isCurrent())
                return;
            bool heart = false;
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                foreach (int id, move.card_ids) {
                    if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand || move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceEquip) {
                        if (Sanguosha->getCard(id)->getSuit() == Card::Heart) {
                            heart = true;
                            break;
                        }
                    }
                }
            }
            if (heart)
                reimu->tag["lingji"] = QVariant::fromValue(true);
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.from->isCurrent())
                return;
            if (use.card->getSuit() == Card::Heart)
                use.from->tag["lingji"] = QVariant::fromValue(true);
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (!resp.m_from->isCurrent())
                return;
            if (resp.m_card->getSuit() == Card::Heart)
                resp.m_from->tag["lingji"] = QVariant::fromValue(true);
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                change.player->tag["lingji"] = QVariant::fromValue(false);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->getPhase() == Player::Finish && player->tag["lingji"].toBool()) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(), "@lingji", true, true);
        if (target != nullptr) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        room->damage(DamageStruct(objectName(), invoke->invoker, invoke->targets.first()));
        return false;
    }
};

class ToushiVS : public OneCardViewAsSkill
{
public:
    ToushiVS()
        : OneCardViewAsSkill("toushi")
    {
        response_pattern = "@@toushi";
        response_or_use = true;
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const override
    {
        return !to_select->isKindOf("TrickCard");
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        if (originalCard != nullptr) {
            QString cardname = Self->property("toushi_card").toString();
            Card *card = Sanguosha->cloneCard(cardname, originalCard->getSuit(), originalCard->getNumber());
            card->addSubcard(originalCard);
            card->setSkillName("toushi");
            return card;
        }
        return nullptr;
    }
};

class Toushi : public TriggerSkill
{
public:
    Toushi()
        : TriggerSkill("toushi")
    {
        events << EventPhaseEnd << PreCardUsed << CardResponded << EventPhaseChanging << CardUsed;
        view_as_skill = new ToushiVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == PreCardUsed || triggerEvent == CardResponded) {
            ServerPlayer *player = nullptr;
            const Card *card = nullptr;
            if (triggerEvent == PreCardUsed) {
                player = data.value<CardUseStruct>().from;
                card = data.value<CardUseStruct>().card;
            } else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                player = response.m_from;
                if (response.m_isUse)
                    card = response.m_card;
            }
            if ((player != nullptr) && player->getPhase() == Player::Play && (card != nullptr) && card->getHandlingMethod() == Card::MethodUse)
                room->setPlayerProperty(player, "toushi_card", card->objectName());
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerProperty(p, "toushi_card", QVariant());
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Play) {
                QString cardname = player->property("toushi_card").toString();
                Card *card = Sanguosha->cloneCard(cardname);
                if (card == nullptr)
                    return QList<SkillInvokeDetail>();
                DELETE_OVER_SCOPE(Card, card)

                if (card->isKindOf("Slash") || (card->isNDTrick() && !card->isKindOf("Nullification"))) {
                    foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                        if (p != player && !p->isCardLimited(card, Card::MethodUse))
                            d << SkillInvokeDetail(this, p, p);
                    }
                }
            }
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == objectName() && use.card->getSuit() == Card::Spade && (use.from != nullptr) && use.from->isAlive())
                d << SkillInvokeDetail(this, use.from, use.from, nullptr, true);
        }
        return d;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == CardUsed) {
            return true;
        }

        ServerPlayer *current = data.value<ServerPlayer *>();
        QString cardname = current->property("toushi_card").toString();
        Card *card = Sanguosha->cloneCard(cardname);
        QString prompt = "@toushi:" + card->objectName();
        delete card;

        room->setPlayerProperty(invoke->invoker, "toushi_card", cardname);
        room->askForUseCard(invoke->invoker, "@@toushi", prompt);
        return false;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->drawCards(1);
        return false;
    }
};

class Moli : public TriggerSkill
{
public:
    Moli()
        : TriggerSkill("moli")
    {
        events << EventPhaseStart << EventPhaseChanging << DamageDone;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if ((damage.from != nullptr) && damage.from->isAlive())
                room->setPlayerMark(damage.from, objectName(), damage.from->getMark(objectName()) + damage.damage);
            if (damage.to->isAlive())
                room->setPlayerMark(damage.to, objectName(), damage.to->getMark(objectName()) + damage.damage);
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerMark(p, objectName(), 0);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Finish) {
                bool wounded = false;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->isWounded()) {
                        wounded = true;
                        break;
                    }
                }
                if (!wounded)
                    return d;

                foreach (ServerPlayer *marisa, room->findPlayersBySkillName(objectName())) {
                    if (marisa->getMark(objectName()) > 1)
                        d << SkillInvokeDetail(this, marisa, marisa);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->isWounded())
                targets << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@moli", true, true);
        if (target != nullptr) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->recover(invoke->targets.first(), RecoverStruct());
        return false;
    }
};

BodongCard::BodongCard()
{
    will_throw = true;
}

bool BodongCard::targetFilter(const QList<const Player *> &, const Player *, const Player *) const
{
    Q_ASSERT(false);
    return false;
}

bool BodongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *, int &maxVotes) const
{
    if (to_select->getEquips().isEmpty())
        return false;
    int i = 0;

    foreach (const Player *player, targets) {
        if (player == to_select)
            i++;
    }

    maxVotes = qMax(3 - targets.size(), 0) + i;
    return maxVotes > 0;
}

bool BodongCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    if (targets.toSet().size() > 3 || targets.toSet().size() == 0)
        return false;
    QHash<const Player *, int> map;

    foreach (const Player *sp, targets)
        map[sp]++;
    foreach (const Player *sp, map.keys()) {
        int num = sp->getEquips().length() - sp->getBrokenEquips().length();
        if (map[sp] > num || num <= 0)
            return false;
    }

    return true;
}

void BodongCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    const QList<ServerPlayer *> &targets = card_use.to;

    QHash<ServerPlayer *, int> map;
    foreach (ServerPlayer *sp, targets)
        map[sp]++;

    QList<ServerPlayer *> newtargets = map.keys();
    room->sortByActionOrder(newtargets);
    foreach (ServerPlayer *sp, newtargets) {
        //if (source == sp)
        QList<int> ids;
        QList<int> disable;
        foreach (const Card *c, sp->getCards("e")) {
            if (sp->isBrokenEquip(c->getEffectiveId()))
                disable << c->getEffectiveId();
        }
        for (int i = 0; i < map[sp]; i++) {
            int id = room->askForCardChosen(source, sp, "e", objectName(), false, Card::MethodNone, disable);
            ids << id;
            disable << id;
        }
        sp->addBrokenEquips(ids);
    }
}

class Bodong : public OneCardViewAsSkill
{
public:
    Bodong()
        : OneCardViewAsSkill("bodong")
    {
        filter_pattern = ".|.|.|hand!";
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("BodongCard") && !player->isKongcheng();
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        if (originalCard != nullptr) {
            BodongCard *card = new BodongCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return nullptr;
    }
};

class Huanlong : public TriggerSkill
{
public:
    Huanlong()
        : TriggerSkill("huanlong")
    {
        events << Damage << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *player = (triggerEvent == Damage) ? damage.from : damage.to;

        if (player == nullptr || player->isDead() || player->getBrokenEquips().isEmpty())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            //if (p->canDiscard(player, "hs")) //p != player &&
            d << SkillInvokeDetail(this, p, p, nullptr, false, player);
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        QList<int> disable;
        foreach (const Card *c, invoke->targets.first()->getCards("e")) {
            if (!invoke->targets.first()->isBrokenEquip(c->getEffectiveId()))
                disable << c->getEffectiveId();
        }
        int id1 = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "e", objectName(), false, Card::MethodNone, disable);
        invoke->targets.first()->removeBrokenEquips(QList<int>() << id1);
        invoke->invoker->drawCards(1);
        return false;
    }
};

ProtagonistPackage::ProtagonistPackage()
    : Package("protagonist")
{
    General *reimu = new General(this, "reimu$", "zhu", 4);
    //reimu->addSkill(new Lingqi);
    reimu->addSkill(new Qixiang);
    reimu->addSkill(new Fengmo);
    reimu->addSkill(new Boli);

    General *marisa = new General(this, "marisa$", "zhu", 4);
    marisa->addSkill(new Mofa);
    marisa->addSkill(new Qinmian);
    marisa->addSkill(new Wuyu);

    General *reimu_sp = new General(this, "reimu_sp", "zhu", 4);
    reimu_sp->addSkill(new Saiqian);

    General *marisa_sp = new General(this, "marisa_sp", "zhu", 3);
    marisa_sp->addSkill(new Jiezou);
    marisa_sp->addSkill(new JiezouNdl);
    related_skills.insertMulti("jiezou", "#jiezou-ndl");
    marisa_sp->addSkill(new Shoucang);
    marisa_sp->addSkill(new ShoucangMax);
    related_skills.insertMulti("shoucang", "#shoucang");

    General *marisa_sp2 = new General(this, "marisa_sp2", "zhu", 4);
    marisa_sp2->addSkill(new Baoyi);

    General *reimu_yym = new General(this, "reimu_yym", "zhu", 4);
    reimu_yym->addSkill(new Zhize);
    reimu_yym->addSkill(new Chunxi);

    General *reimu_slm = new General(this, "reimu_slm", "zhu", 4);
    reimu_slm->addSkill(new BllmWuyu);
    reimu_slm->addSkill(new BllmCaiyu);
    reimu_slm->addSkill(new BllmSeyu);
    reimu_slm->addSkill(new BllmSeyuClear);
    reimu_slm->addSkill(new BllmMingyu);
    reimu_slm->addSkill(new BllmShuiyu);
    reimu_slm->addSkill(new BllmShuiyuMax);
    related_skills.insertMulti("bllmwuyu", "#bllmcaiyu");
    related_skills.insertMulti("bllmwuyu", "#bllmseyu");
    related_skills.insertMulti("bllmwuyu", "#bllmseyu_clear");
    related_skills.insertMulti("bllmwuyu", "#bllmmingyu");
    related_skills.insertMulti("bllmwuyu", "#bllmshuiyu");
    related_skills.insertMulti("bllmwuyu", "#bllmshuiyu2");

    General *marisa_slm = new General(this, "marisa_slm", "zhu", 3);
    marisa_slm->addSkill(new Qiangyu);
    marisa_slm->addSkill(new Mokai);

    General *sanae_slm = new General(this, "sanae_slm", "zhu", 4);
    sanae_slm->addSkill(new DfgzmSiyu);
    sanae_slm->addSkill(new Qishu);
    sanae_slm->addSkill(new QishuTargetMod);
    related_skills.insertMulti("qishu", "#qishu-mod");

    General *youmu_slm = new General(this, "youmu_slm", "zhu", 2);
    youmu_slm->addSkill(new HpymSiyu);
    youmu_slm->addSkill(new Juhe);

    General *reimu_old = new General(this, "reimu_old", "zhu", 4);
    reimu_old->addSkill(new Yinyang);
    reimu_old->addSkill(new Lingji);

    General *marisa_old = new General(this, "marisa_old", "zhu", 4);
    marisa_old->addSkill(new Toushi);
    marisa_old->addSkill(new Moli);

    General *reisen_gzz = new General(this, "reisen_gzz", "zhu", 4);
    reisen_gzz->addSkill(new Bodong);
    reisen_gzz->addSkill(new Huanlong);

    addMetaObject<MofaCard>();
    addMetaObject<WuyuCard>();
    addMetaObject<SaiqianCard>();
    addMetaObject<ShoucangCard>();
    addMetaObject<BaoyiCard>();
    addMetaObject<ChunxiCard>();
    addMetaObject<BllmSeyuCard>();
    addMetaObject<BllmShiyuDummy>();
    addMetaObject<BllmShiyuCard>();
    addMetaObject<BllmWuyuCard>();
    addMetaObject<DfgzmSiyuCard>();
    addMetaObject<YinyangCard>();
    addMetaObject<BodongCard>();

    skills << new WuyuVS << new SaiqianVS;
}

ADD_PACKAGE(Protagonist)
