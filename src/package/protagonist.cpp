#include "protagonist.h"

#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"

class Lingqi : public TriggerSkill
{
public:
    Lingqi()
        : TriggerSkill("lingqi")
    {
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash") && !use.card->isNDTrick())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *reimu, use.to) {
            if (reimu->hasSkill(this))
                d << SkillInvokeDetail(this, reimu, reimu);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *reimu = invoke->invoker;
        reimu->tag["lingqicarduse"] = data;
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "target:" + use.from->objectName() + ":" + use.card->objectName();
        return reimu->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *reimu = invoke->invoker;
        JudgeStruct judge;
        judge.reason = "lingqi";
        judge.who = reimu;
        judge.good = true;
        judge.pattern = ".|heart";
        room->judge(judge);

        if (judge.isGood()) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.nullified_list << reimu->objectName();
            data = QVariant::fromValue(use);
        }

        return false;
    }
};

class Qixiang : public TriggerSkill
{
public:
    Qixiang()
        : TriggerSkill("qixiang")
    {
        events << FinishJudge;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (!judge->who || !judge->who->isAlive())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        QList<ServerPlayer *> reimus = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *reimu, reimus) {
            if (judge->who->getHandcardNum() < reimu->getMaxHp())
                d << SkillInvokeDetail(this, reimu, reimu, NULL, false, judge->who);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *reimu = invoke->invoker;
        JudgeStruct *judge = data.value<JudgeStruct *>();
        reimu->tag["qixiang_judge"] = data;
        QString prompt = "target:" + judge->who->objectName() + ":" + judge->reason;
        return room->askForSkillInvoke(reimu, objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), judge->who->objectName());
        judge->who->drawCards(1);
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

    void record(TriggerEvent e, Room *room, QVariant &) const
    {
        if (e == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                room->setPlayerFlag(p, "-fengmo_used");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        ServerPlayer *player = NULL;
        const Card *card = NULL;
        if (triggerEvent == CardUsed) {
            player = data.value<CardUseStruct>().from;
            card = data.value<CardUseStruct>().card;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct response = data.value<CardResponseStruct>();
            player = response.m_from;
            if (response.m_isUse)
                card = response.m_card;
        }
        if (player && card && (card->isKindOf("Jink") || card->isKindOf("Nullification"))) {
            foreach (ServerPlayer *reimu, room->findPlayersBySkillName(objectName())) {
                if (!reimu->hasFlag("fengmo_used"))
                    d << SkillInvokeDetail(this, reimu, reimu, NULL, false, player);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getAlivePlayers(), objectName(), "@fengmo-target", true, true);
        if (target)
            invoke->targets << target;
        return target != NULL;
    }
    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->setPlayerFlag(invoke->invoker, "fengmo_used");
        JudgeStruct judge;
        judge.reason = objectName();
        judge.who = invoke->targets.first();
        judge.good = true;
        judge.pattern = ".|red";
        room->judge(judge);

        ServerPlayer *target = invoke->targets.first();
        if (judge.isGood()) {
            room->setPlayerMark(target, "@fengmo_SingleTurn", 1);
            room->setPlayerCardLimitation(target, "use,response", ".|^heart", objectName(), true);
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (!judge->who || !judge->who->isAlive() || judge->card->getSuit() == Card::Heart)
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *reimu = invoke->invoker;
        JudgeStruct *judge = data.value<JudgeStruct *>();
        reimu->tag["boli_judge"] = data;
        QString prompt = "judge:" + judge->who->objectName() + ":" + judge->reason;
        return room->askForSkillInvoke(reimu, "boli", prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *reimu = invoke->invoker;
        JudgeStruct *judge = data.value<JudgeStruct *>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, reimu->objectName(), judge->who->objectName());

        foreach (ServerPlayer *p, room->getOtherPlayers(reimu)) {
            QStringList prompts;
            prompts << "@boli-retrial" << judge->who->objectName() << reimu->objectName() << judge->reason;

            const Card *heartcard = room->askForCard(p, ".H", prompts.join(":"), data, Card::MethodResponse, judge->who, true, objectName());
            if (heartcard) {
                room->retrial(heartcard, p, judge, objectName(), true);
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

void MofaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    Card *card = Sanguosha->getCard(subcards.first());
    if (card->getSuit() == Card::Spade)
        source->drawCards(1);
    room->touhouLogmessage("#mofa_notice", source, "mofa");
    source->setFlags("mofa_invoked");
}

class MofaVS : public OneCardViewAsSkill
{
public:
    MofaVS()
        : OneCardViewAsSkill("mofa")
    {
        filter_pattern = ".|.|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MofaCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs(const Card *originalCard) const
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.from->hasFlag("mofa_invoked") && (use.card->isKindOf("Slash") || use.card->isNDTrick()))
                room->setCardFlag(use.card, "mofa_card");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->hasFlag("mofa_card"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *marisa = invoke->invoker;
            if (marisa) {
                room->touhouLogmessage("#TouhouBuff", marisa, objectName());
                QList<ServerPlayer *> logto;
                logto << damage.to;
                room->touhouLogmessage("#mofa_damage", marisa, QString::number(damage.damage + 1), logto, QString::number(damage.damage));
            }
            damage.damage = damage.damage + 1;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

WuyuCard::WuyuCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "wuyu_attach";
}

void WuyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
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
        filter_pattern = ".|spade|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasLordSkill("wuyu") && !p->hasFlag("wuyuInvoked"))
                return true;
        }
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
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
        } else if (triggerEvent == EventPhaseChanging) {
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
    return targets.isEmpty() && to_select->hasSkill("saiqian") && to_select != Self && !to_select->hasFlag("saiqianInvoked");
}

void SaiqianCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
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
            room->touhouLogmessage("#saiqian_lose", reimu, "saiqian");
            room->loseHp(reimu);
            room->recover(source, recov);
        } else if (choice == "discard_saiqian") {
            const Card *heartcard
                = room->askForCard(reimu, ".H", "@saiqian-discard:" + source->objectName(), QVariant::fromValue(source), Card::MethodDiscard, NULL, true, "saiqian");
            if (heartcard != NULL)
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

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasSkill("saiqian") && !p->hasFlag("saiqianInvoked"))
                return true;
        }
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return !to_select->isEquipped() && selected.length() < 3;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0 && cards.length() <= 3) {
            SaiqianCard *card = new SaiqianCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }
};

class Saiqian : public TriggerSkill
{
public:
    Saiqian()
        : TriggerSkill("saiqian")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging << Death << Debut << Revive;
        show_type = "static";
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
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

        } else if (triggerEvent == EventPhaseChanging) {
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

JiezouCard::JiezouCard()
{
    will_throw = true;
    handling_method = Card::MethodNone;
    m_skillName = "jiezou";
}

void JiezouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    int id = room->askForCardChosen(source, target, "hejs", "jiezou");
    room->obtainCard(source, id, room->getCardPlace(id) != Player::PlaceHand);
    const Card *spade = room->askForCard(source, ".|spade", "@jiezou_spadecard", QVariant(), Card::MethodDiscard, NULL, false, "jiezou", false);
    if (spade == NULL) {
        room->loseHp(source);
        room->setPlayerFlag(source, "Global_PlayPhaseTerminated");
        room->touhouLogmessage("#jiezou_skip", source, "jiezou_skip", QList<ServerPlayer *>(), "jiezou");
    }
}

bool JiezouCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && !to_select->isAllNude();
}

class Jiezou : public ZeroCardViewAsSkill
{
public:
    Jiezou()
        : ZeroCardViewAsSkill("jiezou")
    {
    }

    virtual const Card *viewAs() const
    {
        return new JiezouCard;
    }
};

ShoucangCard::ShoucangCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
    m_skillName = "shoucang";
}

void ShoucangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    foreach (int id, subcards)
        room->showCard(source, id);
    room->touhouLogmessage("#shoucang_max", source, "shoucang", QList<ServerPlayer *>(), QString::number(subcards.length()));
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

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
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

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0 && cards.length() < 5) {
            ShoucangCard *card = new ShoucangCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && change.player && change.player->isAlive())
                room->setPlayerMark(change.player, "shoucang", 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *marisa = data.value<ServerPlayer *>();
            if (marisa && marisa->hasSkill(this) && marisa->isAlive() && marisa->getPhase() == Player::Discard && !marisa->isKongcheng())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, marisa, marisa);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        return room->askForUseCard(invoke->invoker, "@@shoucang", "@shoucang");
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
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

    virtual int getExtra(const Player *target) const
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

void BaoyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
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

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        bool has_delaytrick = false;
        BaoyiCard *card = new BaoyiCard;
        foreach (int id, Self->getJudgingAreaID()) {
            card->addSubcard(id);
            has_delaytrick = true;
        }
        if (cards.length() == 0 && !has_delaytrick) {
            return NULL;
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *marisa = data.value<ServerPlayer *>();
        if (marisa && marisa->isAlive() && marisa->getPhase() == Player::Start && !marisa->isAllNude() && marisa->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, marisa, marisa);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->setPlayerFlag(invoke->invoker, "Global_InstanceUse_Failed");
        return room->askForUseCard(invoke->invoker, "@@baoyi", "@baoyi");
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *marisa = invoke->invoker;
        int num = marisa->tag["baoyi"].toInt();
        marisa->tag.remove("baoyi");

        while (num--) {
            QList<ServerPlayer *> listt;
            foreach (ServerPlayer *p, room->getOtherPlayers(marisa)) {
                if (marisa->canSlash(p, NULL, false))
                    listt << p;
            }
            if (listt.isEmpty())
                break;
            ServerPlayer *target = room->askForPlayerChosen(marisa, listt, "baoyi", "@@baoyi_chosen:" + QString::number(num + 1), true, true);
            if (target == NULL)
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

class Zhize : public PhaseChangeSkill
{
public:
    Zhize()
        : PhaseChangeSkill("zhize")
    {
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *reimu = data.value<ServerPlayer *>();
        if (reimu->getPhase() == Player::Draw && reimu->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, reimu, reimu, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        QList<ServerPlayer *> players;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (!p->isKongcheng())
                players << p;
        }
        if (players.isEmpty()) {
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            room->loseHp(player);
            player->skip(Player::Play);
            return false;
        }

        ServerPlayer *target = room->askForPlayerChosen(player, players, objectName(), "@@zhize", true, true);
        if (target) {
            QList<int> ids = target->handCards();
            int id = room->doGongxin(player, target, ids, objectName());

            if (id > -1)
                room->obtainCard(player, id, false);

            return true;
        } else {
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            room->loseHp(player);
            player->skip(Player::Play);
        }
        return false;
    }
};

ChunxiCard::ChunxiCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
    m_skillName = "chunxi";
}

void ChunxiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    foreach (int id, subcards)
        room->showCard(source, id);
    source->tag["chunxi"] = QVariant::fromValue(subcards.length());
}

class ChunxiVS : public ViewAsSkill
{
public:
    ChunxiVS()
        : ViewAsSkill("chunxi")
    {
        response_pattern = "@@chunxi";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return to_select->hasFlag("chunxi");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0) {
            ChunxiCard *card = new ChunxiCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
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

    static QList<ServerPlayer *> chunxi_targets(ServerPlayer *player)
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, player->getRoom()->getOtherPlayers(player)) {
            if (!p->isKongcheng())
                targets << p;
        }
        return targets;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        if (room->getTag("FirstRound").toBool())
            return QList<SkillInvokeDetail>();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *reimu = qobject_cast<ServerPlayer *>(move.to);
        if (reimu != NULL && reimu->hasSkill(this) && move.to_place == Player::PlaceHand) {
            foreach (int id, move.card_ids) {
                if (Sanguosha->getCard(id)->getSuit() == Card::Heart && room->getCardPlace(id) == Player::PlaceHand) {
                    ServerPlayer *owner = room->getCardOwner(id);
                    if (owner && owner == reimu)
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, reimu, reimu);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *reimu = invoke->invoker;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        foreach (int id, move.card_ids) {
            if (Sanguosha->getCard(id)->getSuit() == Card::Heart && room->getCardPlace(id) == Player::PlaceHand) {
                ServerPlayer *owner = room->getCardOwner(id);
                if (owner && owner == reimu)
                    room->setCardFlag(id, "chunxi");
            }
        }
        invoke->invoker->tag["chunxi_move"] = data;
        const Card *c = room->askForUseCard(reimu, "@@chunxi", "@chunxi");
        foreach (int id, move.card_ids)
            room->setCardFlag(id, "-chunxi");

        return c != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        int count = invoke->invoker->tag["chunxi"].toInt();
        invoke->invoker->tag.remove("chunxi");

        for (int i = 0; i < count; ++i) {
            QList<ServerPlayer *> targets = chunxi_targets(invoke->invoker);
            if (targets.isEmpty())
                return false;
            QStringList prompt_list;
            prompt_list << "chunxi-target" << QString::number(i + 1) << QString::number(count);
            QString prompt = prompt_list.join(":");
            ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), prompt, true, true);
            if (target == NULL)
                return false;
            int obtainId = room->askForCardChosen(invoke->invoker, target, "hs", objectName());
            room->obtainCard(invoke->invoker, obtainId, false);
        }
        return false;
    }
};

#pragma message WARN("todo_lwtmusou: find a new method for WuyuCost which could help player and AI to operate this skill more esaily")
BllmWuyuCard::BllmWuyuCard()
{
    mute = true;
    target_fixed = true;
}

void BllmWuyuCard::use(Room *room, ServerPlayer *bllm, QList<ServerPlayer *> &) const
{
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

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark("@yu") == 0 && player->isKongcheng())
            return false;
        if (Analeptic::IsAvailable(player))
            return true;
        return true;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (pattern.contains("@@bllmwuyu"))
            return true;
        Analeptic *card = new Analeptic(Card::NoSuit, 0);
        card->deleteLater();
        if (player->isCardLimited(card, Card::MethodUse))
            return false;

        if (matchAvaliablePattern("analeptic", pattern))
            return true;
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@bllmwuyu")
            return selected.isEmpty() && !to_select->isEquipped();
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@bllmwuyu") {
            if (cards.length() != 1)
                return NULL;
            BllmShiyuDummy *shiyu = new BllmShiyuDummy;
            shiyu->addSubcards(cards);
            return shiyu;
        } else if (matchAvaliablePattern("analeptic", pattern))
            return new BllmShiyuCard;
        else
            return new BllmWuyuCard;
    }
};

class BllmWuyu : public PhaseChangeSkill
{
public:
    BllmWuyu()
        : PhaseChangeSkill("bllmwuyu")
    {
        view_as_skill = new BllmWuyuVS;
    }

    static bool BllmWuyuCost(Room *room, ServerPlayer *bllm, QString prompt)
    {
        //tag for AI
        bllm->tag["wuyu_prompt"] = QVariant::fromValue(prompt);
        if (bllm->getMark("@yu") > 0 && bllm->askForSkillInvoke("bllmwuyu", "useyu:" + prompt)) {
            bllm->loseMark("@yu");
            room->touhouLogmessage("#InvokeSkill", bllm, prompt);
            room->notifySkillInvoked(bllm, prompt);
            return true;
        } else {
            const Card *card = room->askForCard(bllm, "..H", "@bllm-discard:" + prompt, QVariant(), "bllmwuyu");
            if (card != NULL) {
                room->touhouLogmessage("#InvokeSkill", bllm, prompt);
                room->notifySkillInvoked(bllm, prompt);
                return true;
            }
        }
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *reimu = data.value<ServerPlayer *>();
        if (reimu && reimu->hasSkill(this) && reimu->isAlive() && reimu->getPhase() == Player::Start) {
            int z = (reimu->getLostHp() + 1) - reimu->getMark("@yu");
            if (z > 0)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, reimu, reimu);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        //tag for AI
        invoke->invoker->tag["wuyu_prompt"] = QVariant::fromValue(objectName());
        return invoke->invoker->askForSkillInvoke(objectName());
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *reimu = data.value<ServerPlayer *>();
        if (reimu && reimu->hasSkill("bllmwuyu") && reimu->isAlive() && reimu->getPhase() == Player::Draw) {
            if (reimu->getMark("@yu") > 0 || reimu->canDiscard(reimu, "hs"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, reimu, reimu);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        return BllmWuyu::BllmWuyuCost(room, invoke->invoker, "bllmcaiyu");
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
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

void BllmSeyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->setPlayerMark(source, "bllmseyu", source->getMark("bllmseyu") + 1);
}

const Card *BllmSeyuCard::validate(CardUseStruct &cardUse) const
{
    ServerPlayer *bllm = cardUse.from;
    Room *room = bllm->getRoom();
    if (BllmWuyu::BllmWuyuCost(room, bllm, "bllmseyu"))
        return cardUse.card;
    return NULL;
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

    virtual int getResidueNum(const Player *from, const Card *) const
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

    void record(TriggerEvent, Room *room, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.player && change.player->getMark("bllmseyu") > 0)
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        ServerPlayer *reimu = change.player;
        if (reimu && reimu->hasSkill("bllmwuyu") && change.to == Player::Judge && !reimu->isSkipped(change.to)) {
            if (reimu->getMark("@yu") > 0 || reimu->canDiscard(reimu, "hs"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, reimu, reimu);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        return BllmWuyu::BllmWuyuCost(room, invoke->invoker, "bllmmingyu");
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
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

void BllmShiyuDummy::use(Room *, ServerPlayer *, QList<ServerPlayer *> &) const
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
        if (dummy) {
            Analeptic *ana = new Analeptic(Card::SuitToBeDecided, -1);
            foreach (int id, dummy->getSubcards())
                ana->addSubcard(id);
            ana->setSkillName("bllmshiyu");
            ana->setFlags("Add_History"); //because the use reason is not equal CardUseStruct::CARD_USE_REASON_PLAY
            return ana;
        }
    }
    return NULL;
}

const Card *BllmShiyuCard::validateInResponse(ServerPlayer *bllm) const
{
    Room *room = bllm->getRoom();
    if (BllmWuyu::BllmWuyuCost(room, bllm, "bllmshiyu")) {
        room->setPlayerFlag(bllm, "Global_expandpileFailed");
        const Card *dummy = room->askForCard(bllm, "@@bllmwuyu", "@bllmshiyu-basics", QVariant(), Card::MethodNone);
        room->setPlayerFlag(bllm, "-Global_expandpileFailed");
        if (dummy) {
            Analeptic *ana = new Analeptic(Card::SuitToBeDecided, -1);
            foreach (int id, dummy->getSubcards())
                ana->addSubcard(id);
            ana->setSkillName("bllmshiyu");
            return ana;
        }
    }
    return NULL;
}

class BllmShuiyu : public PhaseChangeSkill
{
public:
    BllmShuiyu()
        : PhaseChangeSkill("#bllmshuiyu")
    {
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *reimu = data.value<ServerPlayer *>();
        if (reimu && reimu->hasSkill("bllmwuyu") && reimu->isAlive() && reimu->getPhase() == Player::Discard) {
            if (reimu->getMark("@yu") > 0 || reimu->canDiscard(reimu, "hs"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, reimu, reimu);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        return BllmWuyu::BllmWuyuCost(room, invoke->invoker, "bllmshuiyu");
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
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

    virtual int getFixed(const Player *target) const
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

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (to_select->isEquipped() || Self->isJilei(to_select))
            return false;
        if (selected.length() >= 2)
            return false;

        return true;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        bool ok = cards.length() == 2;
        if (cards.length() == 1 && cards.first()->getSuit() == Card::Spade)
            ok = true;

        QList<const Card *> hc = Self->getHandcards();
        foreach (const Card *card, hc) {
            if (cards.contains(card) || Self->isJilei(card))
                hc.removeOne(card);
        }

        if (hc.isEmpty())
            ok = true;

        if (ok) {
            DummyCard *dc = new DummyCard;
            dc->addSubcards(cards);
            return dc;
        }

        return NULL;
    }
};

class Qiangyu : public TriggerSkill
{
public:
    Qiangyu()
        : TriggerSkill("qiangyu")
    {
        events << CardsMoveOneTime << BeforeCardsMove;
        view_as_skill = new QiangyuVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (room->getTag("FirstRound").toBool())
            return QList<SkillInvokeDetail>();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

        if (triggerEvent == BeforeCardsMove) {
            if (move.reason.m_reason == CardMoveReason::S_REASON_DRAW && move.reason.m_extraData.toString() == "drawFromDrawpile") {
                ServerPlayer *marisa = qobject_cast<ServerPlayer *>(move.to);
                if (marisa != NULL && marisa->hasSkill(this))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, marisa, marisa);
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            ServerPlayer *marisa = qobject_cast<ServerPlayer *>(move.to);
            if (marisa != NULL && marisa->hasFlag("qiangyu"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, marisa, marisa, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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
            bool hasSpade = false;
            foreach (const Card *c, hc) {
                if (invoke->invoker->isJilei(c))
                    hc.removeOne(c);
                else if (c->getSuit() == Card::Spade)
                    hasSpade = true;
            }

            if (!hasSpade && hc.length() < 2) {
                // jilei show all cards
                room->doJileiShow(invoke->invoker, invoke->invoker->handCards());
                return false;
            }

            const Card *card = room->askForCard(invoke->invoker, "@@qiangyu!", "qiangyu-discard", data, Card::MethodDiscard, NULL, false, objectName());
            if (!card) {
                // force discard!!!
                DummyCard *dc = new DummyCard;
                dc->deleteLater();

                if (hc.length() > 2) {
                    for (int i = 0; i < 2; ++i) {
                        int x = qrand() % hc.length();
                        const Card *c = hc.value(x);
                        hc.removeAt(x);
                        dc->addSubcard(c);
                    }
                } else
                    dc->addSubcards(hc);
                room->throwCard(dc, invoke->invoker);
            }
        }
        return false;
    }
};

class SlmMolishaDiscardTianyi : public OneCardViewAsSkill
{
public:
    SlmMolishaDiscardTianyi(const QString &name)
        : OneCardViewAsSkill(name)
    {
        expand_pile = "tianyi";
        response_pattern = "@@" + name;
        filter_pattern = ".|.|.|tianyi";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};

/*class Mokai : public TriggerSkill
{
public:
    Mokai()
        : TriggerSkill("mokai")
    {
        events << CardUsed;
        view_as_skill = new SlmMolishaDiscardTianyi("mokai");
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("TrickCard") && use.from->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *marisa = invoke->invoker;
        const Card *pilecard = room->askForCard(marisa, ".Equip", "@mokai", data, Card::MethodNone, NULL, false, objectName());
        if (pilecard) {
            int id = pilecard->getSubcards().first();
            marisa->tag["tianyi"] = id;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *marisa = invoke->invoker;
        bool ok = false;
        int id = marisa->tag.value("tianyi", -1).toInt(&ok);
        if (!ok || id < 0)
            return false;

        marisa->addToPile("tianyi", id);
        if (marisa->getPile("tianyi").length() > marisa->getHp()) {
            const Card *c = room->askForCard(marisa, "@@mokai", "@mokai-dis", data, Card::MethodNone, NULL, false, "mokai");
            if (c != NULL) {
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
                room->throwCard(c, reason, NULL);
            }
        }
        return false;
    }
};*/

//version 2: EquipBroken
/*
class Mokai : public TriggerSkill
{
public:
    Mokai()
        : TriggerSkill("mokai")
    {
        events << CardUsed << CardsMoveOneTime;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *, const QVariant &data) const
    {
        if (event == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("TrickCard") && use.from->hasSkill(this) && use.from->getPhase() == Player::Play
                && use.from->getEquips().length() > use.from->getBrokenEquips().length())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);

        } else if (event == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
            if (player && player->isAlive() && player->hasSkill(this) && !move.broken_ids.isEmpty()) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent event, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (event == CardUsed) {
            if (invoke->invoker->askForSkillInvoke(this, data)) {
                int id = room->askForCardChosen(invoke->invoker, invoke->invoker, "e", objectName(), false, Card::MethodNone, invoke->invoker->getBrokenEquips());
                invoke->tag["mokai_id"] = QVariant::fromValue(id);
                return true;
            }
        } else if (event == CardsMoveOneTime) {
            return invoke->invoker->askForSkillInvoke(this, data);
        }

        return false;
    }

    bool effect(TriggerEvent event, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (event == CardUsed) {
            int id = invoke->tag["mokai_id"].toInt();
            invoke->invoker->addBrokenEquips(QList<int>() << id);
            invoke->invoker->skip(Player::Finish);
        } else if (event == CardsMoveOneTime) {
            invoke->invoker->drawCards(2);
        }
        return false;
    }
};
*/

//vesion 3: Throw Equip
class Mokai : public TriggerSkill
{
public:
    Mokai()
        : TriggerSkill("mokai")
    {
        events << CardUsed << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                room->setPlayerMark(change.player, "mokai", 0);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("TrickCard") && use.card->isBlack() && use.from->hasSkill(this) && use.from->getPhase() == Player::Play
                && use.from->getMark("mokai") < qMax(1, use.from->getHp()))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        //int id = room->askForCardChosen(invoke->invoker, invoke->invoker, "e", objectName(), false, Card::MethodNone, invoke->invoker->getBrokenEquips());
        return room->askForCard(invoke->invoker, "EquipCard", "@mokai", data, Card::MethodDiscard, NULL, false, objectName());
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->setPlayerMark(invoke->invoker, "mokai", invoke->invoker->getMark("mokai") + 1);
        room->touhouLogmessage("#mokai_count", invoke->invoker, objectName(), QList<ServerPlayer *>(), QString::number(invoke->invoker->getMark("mokai")));
        invoke->invoker->drawCards(2);
        return false;
    }
};

class Guangji : public TriggerSkill
{
public:
    Guangji()
        : TriggerSkill("guangji")
    {
        events << TargetConfirmed;
        view_as_skill = new SlmMolishaDiscardTianyi("guangji");
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash"))
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *marisa, use.to)
            if (!marisa->getPile("tianyi").isEmpty() && marisa->hasSkill(this))
                d << SkillInvokeDetail(this, marisa, marisa);
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "@guangji-invoke:" + use.from->objectName();
        const Card *c = room->askForCard(invoke->invoker, "@@guangji", prompt, data, Card::MethodNone, NULL, false, "guangji");
        if (c) {
            room->notifySkillInvoked(invoke->invoker, "guangji");
            invoke->tag["guangji"] = QVariant::fromValue(c);
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *marisa = invoke->invoker;
        CardUseStruct use = data.value<CardUseStruct>();
        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
        const Card *c = invoke->tag.value("guangji").value<const Card *>();
        Q_ASSERT(c != NULL);
        room->throwCard(c, reason, NULL);
        use.nullified_list << marisa->objectName();
        data = QVariant::fromValue(use);

        return false;
    }
};

class Xinghui : public TriggerSkill
{
public:
    Xinghui()
        : TriggerSkill("xinghui")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *marisa = qobject_cast<ServerPlayer *>(move.from);
        if (marisa != NULL && marisa->isAlive() && marisa->hasSkill(this) && move.from_places.contains(Player::PlaceSpecial)) {
            QList<SkillInvokeDetail> d;
            for (int i = 0; i < move.card_ids.size(); i++) {
                if (move.from_pile_names.value(i) == "tianyi")
                    d << SkillInvokeDetail(this, marisa, marisa);
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    //default cost

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1);
        return false;
    }
};

DfgzmSiyuCard::DfgzmSiyuCard()
{
    will_throw = false;
}

void DfgzmSiyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    room->obtainCard(target, subcards.first(), false);
    source->tag["dfgzmsiyu"] = QVariant::fromValue(target);
}

class DfgzmsiyuVS : public OneCardViewAsSkill
{
public:
    DfgzmsiyuVS()
        : OneCardViewAsSkill("dfgzmsiyu")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("DfgzmSiyuCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            DfgzmSiyuCard *card = new DfgzmSiyuCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
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

    void record(TriggerEvent, Room *, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::RoundStart)
            change.player->tag.remove("dfgzmsiyu");
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            ServerPlayer *target = change.player->tag["dfgzmsiyu"].value<ServerPlayer *>();
            if (change.player->isAlive() && target != NULL && target->isAlive() && !target->isKongcheng())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, NULL, true, target);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->touhouLogmessage("#TouhouBuff", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hs", objectName(), true);
        if (id > -1)
            room->obtainCard(invoke->invoker, id, false);
        return false;
    }
};

//for Collateral, but no need now!
ExtraCollateralCard::ExtraCollateralCard()
{
}

bool ExtraCollateralCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *coll = Card::Parse(Self->property("extra_collateral").toString());
    if (!coll)
        return false;
    QStringList tos = Self->property("extra_collateral_current_targets").toString().split("+");

    if (targets.isEmpty())
        return !tos.contains(to_select->objectName()) && !Self->isProhibited(to_select, coll) && coll->targetFilter(targets, to_select, Self);
    else
        return coll->targetFilter(targets, to_select, Self);
}

void ExtraCollateralCard::onUse(Room *, const CardUseStruct &card_use) const
{
    Q_ASSERT(card_use.to.length() == 2);
    ServerPlayer *killer = card_use.to.first();
    ServerPlayer *victim = card_use.to.last();
    killer->setFlags("ExtraCollateralTarget");
    killer->tag["collateralVictim"] = QVariant::fromValue((ServerPlayer *)victim);
}

class QishuVS : public ZeroCardViewAsSkill
{
public:
    QishuVS()
        : ZeroCardViewAsSkill("qishu")
    {
        response_pattern = "@@qishu";
    }

    virtual const Card *viewAs() const
    {
        return new ExtraCollateralCard;
    }
};

class QishuTargetMod : public TargetModSkill
{
public:
    QishuTargetMod()
        : TargetModSkill("#qishu-mod")
    {
        pattern = "Slash,TrickCard+^DelayedTrick";
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

    virtual int getDistanceLimit(const Player *from, const Card *card) const
    {
        if (from->hasSkill("qishu") && isLastHandCard(from, card))
            return 1000;
        else
            return 0;
    }

    virtual int getExtraTargetNum(const Player *player, const Card *card) const
    {
        if (player->hasSkill("qishu") && player->getPhase() == Player::Play && isLastHandCard(player, card))
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
        view_as_skill = new QishuVS;
    }

    static bool isQishu(QList<ServerPlayer *> players, const Card *card)
    {
        if (card->isKindOf("GlobalEffect") || card->isKindOf("AOE"))
            return false;
        if (card->isKindOf("IronChain"))
            return players.length() > 2;
        return true;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const
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

    /*QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.from->hasSkill(this))
            return QList<SkillInvokeDetail>();

        //process Collateral
        if (QishuTargetMod::isLastHandCard(use.from, use.card) && use.card->isKindOf("Collateral"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *player = use.from;
        bool extra_col = false;

        while (true) {
            ServerPlayer *extra = NULL;
            QStringList tos;
            foreach (ServerPlayer *t, use.to)
                tos.append(t->objectName());
            room->setPlayerProperty(player, "extra_collateral", use.card->toString());
            room->setPlayerProperty(player, "extra_collateral_current_targets", tos.join("+"));
            room->askForUseCard(player, "@@qishu", "@qishu-add:::collateral");
            room->setPlayerProperty(player, "extra_collateral", QString());
            room->setPlayerProperty(player, "extra_collateral_current_targets", QString("+"));
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasFlag("ExtraCollateralTarget")) {
                    p->setFlags("-ExtraCollateralTarget");
                    extra = p;
                    break;
                }
            }
            if (extra != NULL) {
                extra_col = true;
                use.to.append(extra);
                room->sortByActionOrder(use.to);
                LogMessage log;
                log.type = "#QishuAdd";
                log.from = player;
                log.to << extra;
                log.arg = use.card->objectName();
                log.arg2 = "qishu";
                room->sendLog(log);
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), extra->objectName());

                ServerPlayer *victim = extra->tag["collateralVictim"].value<ServerPlayer *>();
                if (victim) {
                    LogMessage log;
                    log.type = "#CollateralSlash";
                    log.from = player;
                    log.to << victim;
                    room->sendLog(log);
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, extra->objectName(), victim->objectName());

                    //show hidden general
                    player->showHiddenSkill(objectName());
                }
            } else
                break;
        }
        data = QVariant::fromValue(use);
        if (extra_col)
            room->notifySkillInvoked(player, "qishu");

        return false;
    }*/
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EnterDying) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.who->hasSkill(this) && !dying.who->isCurrent())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dying.who, dying.who, NULL, true);
        } else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->getMark("siyuinvoke") > 0 && change.to == Player::NotActive && change.player->getHp() < change.player->dyingThreshold())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, NULL, true);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == EventPhaseChanging)
            invoke->invoker->removeMark("siyuinvoke");

        return true;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == EnterDying) {
            if (!invoke->invoker->faceUp())
                invoke->invoker->turnOver();

            QList<const Card *> tricks = invoke->invoker->getJudgingArea();
            foreach (const Card *trick, tricks) {
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, invoke->invoker->objectName());
                room->throwCard(trick, reason, NULL);
            }

            // AmazingGrace::clearRestCards();  private function
            //******************************
            room->clearAG();
            QVariantList ag_list = room->getTag("AmazingGrace").toList();
            room->removeTag("AmazingGrace");
            if (!ag_list.isEmpty()) {
                DummyCard *dummy = new DummyCard(VariantList2IntList(ag_list));
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), "amazing_grace", QString());
                room->throwCard(dummy, reason, NULL);
                delete dummy;
            }
            //******************************

            foreach (int id, Sanguosha->getRandomCards()) {
                if (room->getCardPlace(id) == Player::PlaceTable)
                    room->moveCardTo(Sanguosha->getCard(id), NULL, Player::DiscardPile, true);
                if (Sanguosha->getCard(id)->hasFlag("using"))
                    room->setCardFlag(Sanguosha->getCard(id), "-using");
            }

            invoke->invoker->addMark("siyuinvoke");
            room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
            room->notifySkillInvoked(invoke->invoker, objectName());

            room->touhouLogmessage("#touhouExtraTurn", invoke->invoker, objectName());

            invoke->invoker->gainAnExtraTurn();

            //only for UI //While EnterDying, need QuitDying to change '_m_saveMeIcon'
            JsonArray arg;
            arg << QSanProtocol::S_GAME_EVENT_PLAYER_QUITDYING << invoke->invoker->objectName();
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

            //clear currentDying tag
            QStringList currentdying = room->getTag("CurrentDying").toStringList();
            currentdying.removeOne(invoke->invoker->objectName());
            room->setTag("CurrentDying", QVariant::fromValue(currentdying));

            throw TurnBroken;

            Q_UNREACHABLE();
            //return true; // prevent enterdying
        } else if (triggerEvent == EventPhaseChanging) {
            room->notifySkillInvoked(invoke->invoker, "hpymsiyu");
            room->enterDying(invoke->invoker, NULL);
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        DrawNCardsStruct draw = data.value<DrawNCardsStruct>();

        QList<SkillInvokeDetail> d;
        if (triggerEvent == DrawNCards) {
            if (draw.player->hasSkill(this))
                d << SkillInvokeDetail(this, draw.player, draw.player);
        } else if (triggerEvent == AfterDrawNCards) {
            if (draw.player->hasFlag("juheUsed") && draw.player->getHp() > 0)
                d << SkillInvokeDetail(this, draw.player, draw.player, NULL, true);
        }
        return d;
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == DrawNCards)
            return invoke->invoker->askForSkillInvoke(objectName());
        else
            invoke->invoker->setFlags("-juheUsed");
        return true;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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
    const Card *card1 = NULL;
    const Card *card2 = NULL;

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
    if (!card1) {
        // force discard!!!
        int x = qrand() % hc1.length();
        card1 = hc1.value(x);
        room->throwCard(card1, effect.to);
    }

    if (card1) { //for AI
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
    if (!card2) {
        // force discard!!!
        if (hc2.length() > 0) {
            int x = qrand() % hc2.length();
            card2 = hc2.value(x);
            room->throwCard(card2, effect.from);
        }
    }

    effect.from->tag.remove("yinyang_card");
    effect.from->tag.remove("yinyang_target");
    if (card1 && card2) {
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

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("YinyangCard");
    }

    virtual const Card *viewAs() const
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

    void record(TriggerEvent triggerEvent, Room *, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *reimu = qobject_cast<ServerPlayer *>(move.from);
            if (!reimu || !reimu->isCurrent())
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->getPhase() == Player::Finish && player->tag["lingji"].toBool()) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(), "@lingji", true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
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

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return to_select->getSuit() == Card::Spade || to_select->isKindOf("BasicCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard) {
            QString cardname = Self->property("toushi_card").toString();
            Card *card = Sanguosha->cloneCard(cardname, originalCard->getSuit(), originalCard->getNumber());
            card->addSubcard(originalCard);
            card->setSkillName("toushi");
            return card;
        }
        return NULL;
    }
};

class Toushi : public TriggerSkill
{
public:
    Toushi()
        : TriggerSkill("toushi")
    {
        events << EventPhaseEnd << PreCardUsed << CardResponded << EventPhaseChanging;
        view_as_skill = new ToushiVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed || triggerEvent == CardResponded) {
            ServerPlayer *player = NULL;
            const Card *card = NULL;
            if (triggerEvent == PreCardUsed) {
                player = data.value<CardUseStruct>().from;
                card = data.value<CardUseStruct>().card;
            } else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                player = response.m_from;
                if (response.m_isUse)
                    card = response.m_card;
            }
            if (player && player->getPhase() == Player::Play && card && !card->isKindOf("SkillCard") && card->getHandlingMethod() == Card::MethodUse)
                room->setPlayerProperty(player, "toushi_card", card->objectName());
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerProperty(p, "toushi_card", QVariant());
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Play) {
                QString cardname = player->property("toushi_card").toString();
                Card *card = Sanguosha->cloneCard(cardname);
                if (card == NULL)
                    return QList<SkillInvokeDetail>();
                DELETE_OVER_SCOPE(Card, card)

                if (card->isKindOf("Slash") || (card->isNDTrick() && !card->isKindOf("Nullification"))) {
                    foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                        if (p != player && !p->isCardLimited(card, Card::MethodUse))
                            d << SkillInvokeDetail(this, p, p);
                    }
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        QString cardname = current->property("toushi_card").toString();
        Card *card = Sanguosha->cloneCard(cardname);
        QString prompt = "@toushi:" + card->objectName();
        delete card;

        room->setPlayerProperty(invoke->invoker, "toushi_card", cardname);
        room->askForUseCard(invoke->invoker, "@@toushi", prompt);
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive())
                room->setPlayerMark(damage.from, objectName(), damage.from->getMark(objectName()) + damage.damage);
            if (damage.to && damage.to->isAlive())
                room->setPlayerMark(damage.to, objectName(), damage.to->getMark(objectName()) + damage.damage);
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerMark(p, objectName(), 0);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->isWounded())
                targets << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@moli", true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
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
    QMap<const Player *, int> map;

    foreach (const Player *sp, targets)
        map[sp]++;
    foreach (const Player *sp, map.keys()) {
        int num = sp->getEquips().length() - sp->getBrokenEquips().length();
        if (map[sp] > num || num <= 0)
            return false;
    }

    return true;
}

void BodongCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QMap<ServerPlayer *, int> map;
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

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("BodongCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            BodongCard *card = new BodongCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *player = NULL;
        if (triggerEvent == Damage)
            player = damage.from;
        else
            player = damage.to;

        if (player == NULL || player->isDead() || player->getBrokenEquips().isEmpty())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p->canDiscard(player, "hs")) //p != player &&
                d << SkillInvokeDetail(this, p, p, NULL, false, player);
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        QList<int> disable;
        foreach (const Card *c, invoke->targets.first()->getCards("e")) {
            if (!invoke->targets.first()->isBrokenEquip(c->getEffectiveId()))
                disable << c->getEffectiveId();
        }
        int id1 = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "e", objectName(), false, Card::MethodNone, disable);
        invoke->targets.first()->removeBrokenEquips(QList<int>() << id1);

        if (invoke->invoker->canDiscard(invoke->targets.first(), "hs")) {
            int id2 = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hs", objectName(), false, Card::MethodDiscard);
            room->throwCard(id2, invoke->targets.first(), invoke->invoker);
        }
        return false;
    }
};

ProtagonistPackage::ProtagonistPackage()
    : Package("protagonist")
{
    General *reimu = new General(this, "reimu$", "zhu", 4);
    //reimu->addSkill(new Lingqi);
    reimu->addSkill(new Fengmo);
    reimu->addSkill(new Qixiang);
    reimu->addSkill(new Boli);

    General *marisa = new General(this, "marisa$", "zhu", 4);
    marisa->addSkill(new Mofa);
    marisa->addSkill(new Wuyu);

    General *reimu_sp = new General(this, "reimu_sp", "zhu", 4);
    reimu_sp->addSkill(new Saiqian);

    General *marisa_sp = new General(this, "marisa_sp", "zhu", 3);
    marisa_sp->addSkill(new Jiezou);
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
    //marisa_slm->addSkill(new Guangji);
    //marisa_slm->addSkill(new Xinghui);

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
    addMetaObject<JiezouCard>();
    addMetaObject<ShoucangCard>();
    addMetaObject<BaoyiCard>();
    addMetaObject<ChunxiCard>();
    addMetaObject<BllmSeyuCard>();
    addMetaObject<BllmShiyuDummy>();
    addMetaObject<BllmShiyuCard>();
    addMetaObject<BllmWuyuCard>();
    addMetaObject<DfgzmSiyuCard>();
    addMetaObject<ExtraCollateralCard>();
    addMetaObject<YinyangCard>();
    addMetaObject<BodongCard>();

    skills << new WuyuVS << new SaiqianVS;
}

ADD_PACKAGE(Protagonist)
