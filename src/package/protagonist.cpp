#include "protagonist.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"
#include "maneuvering.h"


class Lingqi : public TriggerSkill
{
public:
    Lingqi() : TriggerSkill("lingqi")
    {
        events << TargetConfirming;

    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash") && !use.card->isNDTrick())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach(ServerPlayer *reimu, use.to) {
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
        QString  prompt = "target:" + use.from->objectName() + ":" + use.card->objectName();
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
    Qixiang() : TriggerSkill("qixiang")
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
        JudgeStruct * judge = data.value<JudgeStruct *>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), judge->who->objectName());
        judge->who->drawCards(1);
        return false;
    }

};


class Boli : public TriggerSkill
{
public:
    Boli() : TriggerSkill("boli$")
    {
        events << AskForRetrial;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (!judge->who || !judge->who->isAlive() || judge->card->getSuit() == Card::Heart)
            return QList<SkillInvokeDetail>();


        QList<SkillInvokeDetail> details;
        foreach(ServerPlayer *reimu, room->getAllPlayers()) {
            if (reimu->hasLordSkill(objectName())){
                foreach(ServerPlayer *p, room->getOtherPlayers(reimu)) {
                    if (!p->isKongcheng())
                        details << SkillInvokeDetail(this, reimu, reimu);
                }
            }  
        }
        return details;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *reimu = invoke->invoker;
        JudgeStruct * judge = data.value<JudgeStruct *>();
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
    MofaVS() :OneCardViewAsSkill("mofa")
    {
        filter_pattern = ".|.|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MofaCard");
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
    Mofa() : TriggerSkill("mofa")
    {
        events << PreCardUsed << ConfirmDamage;
        view_as_skill = new MofaVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data)
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.from->hasFlag("mofa_invoked")) 
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
    if (marisa->hasLordSkill("wuyu")) {
        room->setPlayerFlag(marisa, "wuyuInvoked");

        room->notifySkillInvoked(marisa, "wuyu");
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), marisa->objectName(), "wuyu", QString());
        room->obtainCard(marisa, this, reason);
        QList<ServerPlayer *> marisas;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach (ServerPlayer *p, players) {
            if (p->hasLordSkill("wuyu") && !p->hasFlag("wuyuInvoked"))
                marisas << p;
        }
        if (marisas.isEmpty())
            room->setPlayerFlag(source, "Forbidwuyu");
    }
}

bool WuyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasLordSkill("wuyu")
        && to_select != Self && !to_select->hasFlag("wuyuInvoked");
}

class WuyuVS : public OneCardViewAsSkill
{
public:
    WuyuVS() :OneCardViewAsSkill("wuyu_attach")
    {
        attached_lord_skill = true;
        filter_pattern = ".|spade|.|hand";

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("Forbidwuyu");
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
    Wuyu() : TriggerSkill("wuyu$")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Death << EventPhaseChanging;
    }


    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent != EventPhaseChanging) { //the case operating attach skill
            static QString attachName = "wuyu_attach";
            QList<ServerPlayer *> lords;
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasLordSkill(this, false, true))
                    lords << p;
            }

            if (lords.length() > 1) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->hasSkill(attachName, true, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else if (lords.length() == 1) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasLordSkill(this, false, true) && p->hasSkill(attachName, true, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                    else if (!p->hasLordSkill(this, false, true) && !p->hasSkill(attachName, true, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasSkill(attachName, true, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from == Player::Play) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("wuyuInvoked"))
                        room->setPlayerFlag(p, "-wuyuInvoked");
                    if (p->hasFlag("Forbidwuyu"))
                        room->setPlayerFlag(p, "-Forbidwuyu");
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
    return targets.isEmpty() && to_select->hasSkill("saiqian")
        && to_select != Self && !to_select->hasFlag("saiqianInvoked");
}

void SaiqianCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *reimu = targets.first();
    if (reimu->hasSkill("saiqian")) {

        reimu->tag["saiqian_source"] = QVariant::fromValue(source);

        room->setPlayerFlag(reimu, "saiqianInvoked");
        room->notifySkillInvoked(reimu, "saiqian");
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), reimu->objectName(), "saiqian", QString());
        room->obtainCard(reimu, this, reason, false);
        QList<ServerPlayer *> reimus;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach (ServerPlayer *p, players) {
            if (p->hasSkill("saiqian") && !p->hasFlag("saiqianInvoked"))
                reimus << p;
        }
        if (reimus.isEmpty())
            room->setPlayerFlag(source, "Forbidsaiqian");


        //start effect
        QStringList saiqian_str;
        saiqian_str << "losehp_saiqian" << "discard_saiqian" << "cancel_saiqian";
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
                const Card *heartcard = room->askForCard(reimu, ".H", "@saiqian-discard:" + source->objectName(), QVariant::fromValue(source), Card::MethodDiscard, NULL, true, "saiqian");
                if (heartcard != NULL)
                    room->recover(source, recov);
            }
            saiqian_str.removeOne(choice);
        }
    }
}
class SaiqianVS : public ViewAsSkill
{
public:
    SaiqianVS() : ViewAsSkill("saiqian_attach")
    {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("Forbidsaiqian");
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
    Saiqian() : TriggerSkill("saiqian")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging << Death << Debut;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent != EventPhaseChanging) { //the case operating attach skill
            QList<ServerPlayer *> reimus;
            static QString attachName = "saiqian_attach";
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(this, false, true))
                    reimus << p;
            }

            if (reimus.length() > 1) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->hasSkill(attachName, true, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else if (reimus.length() == 1) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasSkill(this, false, true) && p->hasSkill(attachName, true, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                    else if (!p->hasSkill(this, false, true) && !p->hasSkill(attachName, true, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasSkill(attachName, true, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                }
            }
            
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from == Player::Play) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("saiqianInvoked"))
                        room->setPlayerFlag(p, "-saiqianInvoked");
                    if (p->hasFlag("Forbidsaiqian"))
                        room->setPlayerFlag(p, "-Forbidsaiqian");
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
    int id = room->askForCardChosen(source, target, "hej", "jiezou");
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
    Jiezou() : ZeroCardViewAsSkill("jiezou")
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
    handling_method = Card::MethodUse;
    m_skillName = "shoucang";
}

void ShoucangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    foreach(int id, subcards)
        room->showCard(source, id);
    room->touhouLogmessage("#shoucang_max", source, "shoucang", QList<ServerPlayer *>(), QString::number(subcards.length()));
    source->tag["shoucang"] = QVariant::fromValue(subcards.length());
}

class ShoucangVS : public ViewAsSkill
{
public:
    ShoucangVS() : ViewAsSkill("shoucang")
    {
        response_pattern = "@@shoucang";
    }


    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (to_select->isEquipped())
            return false;
        if (selected.isEmpty())
            return  !to_select->isEquipped();
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
    Shoucang() : TriggerSkill("shoucang")
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
            if (marisa  && marisa->hasSkill(this) && marisa->isAlive() && marisa->getPhase() == Player::Discard && !marisa->isKongcheng())
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
    ShoucangMax() : MaxCardsSkill("#shoucang")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasSkill(objectName()))
            return target->getMark("shoucang");
        else
            return 0;
    }
};


BaoyiCard::BaoyiCard()
{
    will_throw = true;
    target_fixed = true;
    handling_method = Card::MethodDiscard;
}

void BaoyiCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    foreach (int id, subcards) {
        Card *c = Sanguosha->getCard(id);
        if (c->getSuit() == Card::Spade) {
            source->setFlags("baoyi");
            break;
        }
    }
    source->tag["baoyi"] = QVariant::fromValue(subcards.length());
    
}
class BaoyiVS : public ViewAsSkill
{
public:
    BaoyiVS() : ViewAsSkill("baoyi")
    {
        response_pattern = "@@baoyi";
    }


    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return  to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
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
        if (cards.length() > 0) {
            card->addSubcards(cards);
        }
        return card;
    }
};
class Baoyi : public TriggerSkill
{
public:
    Baoyi() : TriggerSkill("baoyi")
    {
        events << EventPhaseStart << EventPhaseEnd;
        view_as_skill = new BaoyiVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *marisa = data.value<ServerPlayer *>();
        if (!marisa || marisa->isDead() || marisa->getPhase() != Player::Start) 
            return QList<SkillInvokeDetail>();
        if (triggerEvent == EventPhaseStart) {
            if (!marisa->isAllNude() && marisa->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, marisa, marisa);
        } else if (triggerEvent == EventPhaseEnd) {
            if (marisa->hasFlag("baoyi"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, marisa, marisa, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart) {
            room->askForUseCard(invoke->invoker, "@@baoyi", "@baoyi");
        } else if (triggerEvent == EventPhaseEnd) {
            invoke->invoker->setFlags("-baoyi");
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *marisa = invoke->invoker;
        if (triggerEvent == EventPhaseStart) {
            int num = marisa->tag["baoyi"].toInt();
            marisa->tag.remove("baoyi");
            if (num <= 0)
                return false;
            while (num--) {
                QList<ServerPlayer *> listt;
                foreach(ServerPlayer *p, room->getOtherPlayers(marisa)) {
                    if (marisa->canSlash(p, NULL, false))
                        listt << p;
                }
                if (listt.isEmpty())
                    break;
                ServerPlayer * target = room->askForPlayerChosen(marisa, listt, "baoyi", "@@baoyi_chosen:" + QString::number(num), true, true);
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
        } else if (triggerEvent == EventPhaseEnd) {
            room->touhouLogmessage("#TouhouBuff", marisa, "baoyi");
            marisa->drawCards(2);
        }
        return false;
    }

};

class Zhize : public PhaseChangeSkill
{
public:
    Zhize() :PhaseChangeSkill("zhize")
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


class Chunxi : public TriggerSkill
{
public:
    Chunxi() : TriggerSkill("chunxi")
    {
        events << CardsMoveOneTime;
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

    void record(TriggerEvent, Room *room, QVariant &data)
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *reimu = qobject_cast<ServerPlayer *>(move.to);
        if (reimu != NULL && move.to_place == Player::PlaceHand)
            reimu->tag.remove("chunxi_currentIndex");
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        if (room->getTag("FirstRound").toBool())
            return QList<SkillInvokeDetail>();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *reimu = qobject_cast<ServerPlayer *>(move.to);
        if (reimu != NULL && reimu->hasSkill(this) && move.to_place == Player::PlaceHand) {
            reimu->tag.remove("chunxi_ids");
            QList<SkillInvokeDetail> d;
            QVariantList v;
            foreach (int id, move.card_ids) {
                if (Sanguosha->getCard(id)->getSuit() == Card::Heart && room->getCardPlace(id) == Player::PlaceHand) {
                    ServerPlayer *owner = room->getCardOwner(id);
                    if (owner && owner == reimu) {
                        d << SkillInvokeDetail(this, reimu, reimu);
                        v << id;
                    }
                }
            }
            reimu->tag["chunxi_ids"] = v;
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *reimu = invoke->invoker;
        QList<ServerPlayer *> targets = chunxi_targets(reimu);
        if (targets.isEmpty())
            return false;
        QVariantList chunxi_ids = reimu->tag.value("chunxi_ids").toList();
        if (chunxi_ids.isEmpty())
            return false;

        bool ok = false;
        int index = reimu->tag.value("chunxi_currentIndex", 0).toInt(&ok);
        int id = -1;
        if (ok)
            id = chunxi_ids.value(index++).toInt(&ok);

        if (ok && id > -1) {
            reimu->tag["chunxi_currentIndex"] = index;
            reimu->tag["chunxi_currentId"] = id;
            ServerPlayer *target = room->askForPlayerChosen(reimu, targets, objectName(), "@@chunxi", true, true);
            if (target == NULL)
                return false;

            reimu->tag["chunxi_currentPlayer"] = QVariant::fromValue(target);
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *reimu = invoke->invoker;
        ServerPlayer *target = reimu->tag.value("chunxi_currentPlayer", NULL).value<ServerPlayer *>();
        bool ok = false;
        int id = reimu->tag.value("chunxi_currentId", -1).toInt(&ok);
        if (target != NULL && ok && id > -1) {
            room->showCard(reimu, id);
            int obtainId = room->askForCardChosen(reimu, target, "h", objectName());
            room->obtainCard(reimu, obtainId, false);
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
    BllmWuyuVS() : ViewAsSkill("bllmwuyu")
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
        if (pattern.contains("analeptic"))
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
        } else if (pattern.contains("analeptic"))
            return new BllmShiyuCard;
        else
            return new BllmWuyuCard;
    }
};

class BllmWuyu : public PhaseChangeSkill
{
public:
    BllmWuyu() :PhaseChangeSkill("bllmwuyu")
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
    BllmCaiyu() :PhaseChangeSkill("#bllmcaiyu")
    {
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *reimu = data.value<ServerPlayer *>();
        if (reimu && reimu->hasSkill("bllmwuyu") && reimu->isAlive() && reimu->getPhase() == Player::Draw) {
            if (reimu->getMark("@yu") > 0 || reimu->canDiscard(reimu, "h"))
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
    BllmSeyu() : TargetModSkill("#bllmseyu")
    {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *) const
    {
        return from->getMark("bllmseyu");
    }
};

class BllmSeyuClear : public TriggerSkill
{
public:
    BllmSeyuClear() : TriggerSkill("#bllmseyu_clear")
    {
        events << EventPhaseChanging;
    }

    void record(TriggerEvent, Room *room, QVariant &data)
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.player && change.player->getMark("bllmseyu") > 0)
            room->setPlayerMark(change.player, "bllmseyu", 0);
    }
};

class BllmMingyu : public TriggerSkill
{
public:
    BllmMingyu() : TriggerSkill("#bllmmingyu")
    {
        events << EventPhaseChanging;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        ServerPlayer *reimu = change.player;
        if (reimu && reimu->hasSkill("bllmwuyu") && change.to == Player::Judge  && !reimu->isSkipped(change.to)) {
            if (reimu->getMark("@yu") > 0 || reimu->canDiscard(reimu, "h"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, reimu, reimu);
        }
        return QList<SkillInvokeDetail > ();
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
            foreach(int id, dummy->getSubcards())
                ana->addSubcard(id);
            ana->setSkillName("bllmshiyu");
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
            foreach(int id, dummy->getSubcards())
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
    BllmShuiyu() :PhaseChangeSkill("#bllmshuiyu")
    {

    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *reimu = data.value<ServerPlayer *>();
        if (reimu && reimu->hasSkill("bllmwuyu") && reimu->isAlive() && reimu->getPhase() == Player::Discard) {
            if (reimu->getMark("@yu") > 0 || reimu->canDiscard(reimu, "h"))
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
    BllmShuiyuMax() : MaxCardsSkill("#bllmshuiyu2")
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
    QiangyuVS() : ViewAsSkill("qiangyu")
    {
        response_pattern = "@@qiangyu!";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (to_select->isEquipped())
            return false;
        if (selected.length() == 2)
            return false;

        return true;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        bool ok = cards.length() == 2;
        if (cards.length() == 1 && (cards.first()->getSuit() == Card::Spade || Self->getHandcardNum() == 1))
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
    Qiangyu() : TriggerSkill("qiangyu")
    {
        events << CardsMoveOneTime << BeforeCardsMove;
        // frequency = Frequent;
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
            move.card_ids << room->getNCards(2);
            data = QVariant::fromValue(move);
            invoke->invoker->setFlags("qiangyu");
        } else {
            invoke->invoker->setFlags("-qiangyu");
            if (!room->askForCard(invoke->invoker, "@@qiangyu!", "qiangyu-discard", data, Card::MethodDiscard, NULL, false, objectName())) {
                // force discard!!!
                DummyCard dc;
                if (invoke->invoker->getHandcardNum() <= 2)
                    dc.addSubcards(invoke->invoker->handCards());
                else {
                    QList<int> handcards = invoke->invoker->handCards();
                    for (int i = 0; i < 2; ++i) {
                        int x = qrand() % handcards.length();
                        int id = handcards.value(x);
                        handcards.removeAt(x);
                        dc.addSubcard(id);
                    }
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
    Mokai() : TriggerSkill("mokai")
    {
        events << CardUsed;
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
        const Card *pilecard = room->askForCard(marisa, ".Equip", "@mokai", data, Card::MethodNone, NULL, false, objectName(), false);
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
        QList<int> pile = marisa->getPile("tianyi");
        if (pile.length() > marisa->getHp()) {
            if (marisa->askForSkillInvoke(objectName(), data)) {
                room->fillAG(pile, marisa);
                int id = room->askForAG(marisa, pile, false, objectName());
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
                room->clearAG(marisa);
                room->throwCard(Sanguosha->getCard(id), reason, NULL);
            }
        }
        return false;
    }
};


class Guangji : public TriggerSkill
{
public:
    Guangji() : TriggerSkill("guangji")
    {
        events << TargetConfirming;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash"))
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach(ServerPlayer *marisa, use.to)         
            if (!marisa->getPile("tianyi").isEmpty() && marisa->hasSkill(this))
                d << SkillInvokeDetail(this, marisa, marisa);
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "invoke:" + use.from->objectName();
        invoke->invoker->tag["guangji_use"] = data;
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }


    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *marisa = invoke->invoker;
        CardUseStruct use = data.value<CardUseStruct>();
        QList<int> pile = marisa->getPile("tianyi");
        room->fillAG(pile, marisa);
        int id = room->askForAG(marisa, pile, false, objectName());
        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
        room->clearAG(marisa);
        room->throwCard(Sanguosha->getCard(id), reason, NULL);
        use.nullified_list << marisa->objectName();
        data = QVariant::fromValue(use);

        return false;
    }
};


class Xinghui : public TriggerSkill
{
public:
    Xinghui() : TriggerSkill("xinghui")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *marisa = qobject_cast<ServerPlayer *>(move.from);
        if (marisa != NULL && marisa->hasSkill(this) &&  move.from_places.contains(Player::PlaceSpecial)) {
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



ProtagonistPackage::ProtagonistPackage()
    : Package("protagonist")
{

    General *reimu = new General(this, "reimu$", "zhu", 4, false);
    reimu->addSkill(new Lingqi);
    reimu->addSkill(new Qixiang);
    reimu->addSkill(new Boli);

    General *marisa = new General(this, "marisa$", "zhu", 4, false);
    marisa->addSkill(new Mofa);
    marisa->addSkill(new Wuyu);

    General *reimu_sp = new General(this, "reimu_sp", "zhu", 4, false);
    reimu_sp->addSkill(new Saiqian);

    General *marisa_sp = new General(this, "marisa_sp", "zhu", 3, false);
    marisa_sp->addSkill(new Jiezou);
    marisa_sp->addSkill(new Shoucang);
    marisa_sp->addSkill(new ShoucangMax);
    related_skills.insertMulti("shoucang", "#shoucang");

    General *marisa_sp2 = new General(this, "marisa_sp2", "zhu", 4, false);
    marisa_sp2->addSkill(new Baoyi);

    General *reimu_yym = new General(this, "reimu_yym", "zhu", 4, false);
    reimu_yym->addSkill(new Zhize);
    reimu_yym->addSkill(new Chunxi);

    General *reimu_slm = new General(this, "reimu_slm", "zhu", 4, false);
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


    General *marisa_slm = new General(this, "marisa_slm", "zhu", 3, false);
    marisa_slm->addSkill(new Qiangyu);
    marisa_slm->addSkill(new Mokai);
    marisa_slm->addSkill(new Guangji);
    marisa_slm->addSkill(new Xinghui);

    General *reimu_old = new General(this, "reimu_old", "zhu", 4, false);
    Q_UNUSED(reimu_old);
    General *marisa_old = new General(this, "marisa_old", "zhu", 4, false);
    Q_UNUSED(marisa_old);


    addMetaObject<MofaCard>();
    addMetaObject<WuyuCard>();
    addMetaObject<SaiqianCard>();
    addMetaObject<JiezouCard>();
    addMetaObject<ShoucangCard>();
    addMetaObject<BaoyiCard>();
    addMetaObject<BllmSeyuCard>();
    addMetaObject<BllmShiyuDummy>();
    addMetaObject<BllmShiyuCard>();
    addMetaObject<BllmWuyuCard>();


    skills << new WuyuVS << new SaiqianVS;

}

ADD_PACKAGE(Protagonist)

