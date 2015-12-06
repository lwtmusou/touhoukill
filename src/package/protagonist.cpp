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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") || use.card->isNDTrick())
                return QStringList(objectName());
        } 
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetConfirming) {
            player->tag["lingqicarduse"] = data;
            CardUseStruct use = data.value<CardUseStruct>();
            QString  prompt = "target:" + use.from->objectName() + ":" + use.card->objectName();
            if (player->askForSkillInvoke(objectName(), prompt))
                return true;
        } 
        return false;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetConfirming) {
            JudgeStruct judge;
            judge.reason = "lingqi";
            judge.who = player;
            judge.good = true;
            judge.pattern = ".|heart";
            room->judge(judge);
            CardUseStruct use = data.value<CardUseStruct>();
            if (judge.isGood()){
                use.nullified_list << player->objectName();
                data = QVariant::fromValue(use);
            }
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

    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {   
        JudgeStar judge = data.value<JudgeStar>();
        if (!judge->who || !judge->who->isAlive())
            return TriggerList();

        TriggerList skill_list;
        QList<ServerPlayer *> reimus = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *reimu, reimus) {
            if (judge->who->getHandcardNum() < reimu->getMaxHp())
                skill_list.insert(reimu, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        JudgeStar judge = data.value<JudgeStar>();
        ask_who->tag["qixiang_judge"] = data;
        QString prompt = "target:" + judge->who->objectName() + ":" + judge->reason;
        return room->askForSkillInvoke(ask_who, objectName(), prompt);
    }
    
     virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        JudgeStar judge = data.value<JudgeStar>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, ask_who->objectName(), judge->who->objectName());
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

     virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (!TriggerSkill::triggerable(player) || !player->hasLordSkill(objectName())) return QStringList();
        JudgeStar judge = data.value<JudgeStar>();
        if (judge->card->getSuit() != Card::Heart)
            return QStringList(objectName());
            //everyone is kongcheng?
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        JudgeStar judge = data.value<JudgeStar>();
        player->tag["boli_judge"] = data;
        QString prompt = "judge:" + judge->who->objectName() + ":" + judge->reason;
        return room->askForSkillInvoke(player, "boli", prompt);
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), judge->who->objectName());

        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            QStringList prompts;
            prompts << "@boli-retrial" << judge->who->objectName() << player->objectName() << judge->reason;

            const Card *heartcard = room->askForCard(p, ".H", prompts.join(":"), data, Card::MethodResponse, judge->who, true, objectName());
            if (heartcard) {
                room->retrial(heartcard, p, judge, objectName(), false);
                return true;
            }
        }
        
        
        //JudgeStruct *judge = data.value<JudgeStruct *>();
        //judge->updateResult();
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
    room->setPlayerFlag(source, "mofa_invoked");
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

    virtual bool triggerable(const ServerPlayer *) const
    {
        return true;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (triggerEvent == PreCardUsed) {
            if (player && player->hasFlag("mofa_invoked")) {
                CardUseStruct use = data.value<CardUseStruct>();
                room->setCardFlag(use.card, "mofa_card");
            }
        } else if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->hasFlag("mofa_card")) {
                return QStringList(objectName());
            }   
        }
        return QStringList();
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from) {
                room->touhouLogmessage("#TouhouBuff", damage.from, objectName());
                QList<ServerPlayer *> logto;
                logto << damage.to;
                room->touhouLogmessage("#mofa_damage", damage.from, QString::number(damage.damage + 1), logto, QString::number(damage.damage));
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
        return  !player->hasFlag("Forbidwuyu");
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
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }


    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!player) return QStringList();
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "wuyu")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName(), false, true))
                    lords << p;
            }
            if (lords.isEmpty()) return QStringList();

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("wuyu_attach"))
                    room->attachSkillToPlayer(p, "wuyu_attach");
            }
        }else if (triggerEvent == EventLoseSkill && data.toString() == "wuyu") {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName(), false, true))
                    lords << p;
            }
            if (lords.length() > 2) return QStringList();

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("wuyu_attach"))
                    room->detachSkillFromPlayer(p, "wuyu_attach", true);
            }
        }else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                return QStringList();
            if (player->hasFlag("Forbidwuyu"))
                room->setPlayerFlag(player, "-Forbidwuyu");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("wuyuInvoked"))
                    room->setPlayerFlag(p, "-wuyuInvoked");
            }
        }
        
        return QStringList();
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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!player) return QStringList();
        if (triggerEvent == GameStart || triggerEvent == Debut
            || (triggerEvent == EventAcquireSkill && data.toString() == "saiqian")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill(objectName(), false, true))
                    lords << p;
            }
            if (lords.isEmpty()) return QStringList();

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("saiqian_attach"))
                    room->attachSkillToPlayer(p, "saiqian_attach");
            }
        }else if (triggerEvent == Death || (triggerEvent == EventLoseSkill && data.toString() == "saiqian")) {
            if (triggerEvent == Death) {
                DeathStruct death = data.value<DeathStruct>();
                if (!death.who->hasSkill(objectName(), false, true))//deal the case that death in round of changshi?
                    return QStringList();
            }
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasSkill(objectName(), false, true))
                    lords << p;
            }
            if (lords.length() > 2) return QStringList();

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("saiqian_attach"))
                    room->detachSkillFromPlayer(p, "saiqian_attach", true);
            }
        }else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                return QStringList();
            if (player->hasFlag("Forbidsaiqian"))
                room->setPlayerFlag(player, "-Forbidsaiqian");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("saiqianInvoked"))
                    room->setPlayerFlag(p, "-saiqianInvoked");
            }
        }
        
        return QStringList();
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
    room->setPlayerMark(source, "shoucang", subcards.length());

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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == EventPhaseStart) {    
            if (player->getPhase() == Player::Discard && !player->isKongcheng())
                return QStringList(objectName());
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(player, "shoucang", 0);
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer*) const
    {
        if (triggerEvent == EventPhaseStart) {
            room->askForUseCard(player, "@@shoucang", "@shoucang");
        } 
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
void BaoyiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    int num = subcards.length();
    foreach (int id, subcards) {
        Card *c = Sanguosha->getCard(id);
        if (c->getSuit() == Card::Spade)
            source->setFlags("baoyi");
    }

    while (num > 0) {
        QList<ServerPlayer *> listt;
        Slash *slash = new Slash(Card::NoSuit, 0);
        foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
            if (source->canSlash(p, slash, false))
                listt << p;

        }
        if (!listt.isEmpty()) {
            ServerPlayer * target = room->askForPlayerChosen(source, listt, "baoyi", "@@baoyi_chosen:" + QString::number(num), true, true);
            if (target == NULL)
                break;
            CardUseStruct carduse;
            slash->setSkillName("baoyi");
            carduse.card = slash;
            carduse.from = source;
            carduse.to << target;
            room->useCard(carduse);
        }
        num = num - 1;
    }
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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() != Player::Start) return QStringList();
        if (triggerEvent == EventPhaseStart) {
            if (!player->isAllNude())
                  return QStringList(objectName());
        }else if (triggerEvent == EventPhaseEnd) {
            if (player->hasFlag("baoyi"))
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer*) const
    {
        if (triggerEvent == EventPhaseStart) {
            room->askForUseCard(player, "@@baoyi", "@baoyi");
        } else if (triggerEvent == EventPhaseEnd)
            return true;
        return false;
    }

     virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer*) const
    {
        if (triggerEvent == EventPhaseEnd) {
            player->setFlags("-baoyi");
            room->touhouLogmessage("#TouhouBuff", player, "baoyi");
            player->drawCards(2);
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

    
    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Draw;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        return true;
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
            //case1 ask for ag
            /*room->fillAG(target->handCards(),player);
            int id=room->askForAG(player,target->handCards(),false,objectName());
            room->clearAG(player);
            */
            //case2 gongxin
            QList<int>  ids;
            foreach(const Card *c, target->getCards("h"))
                ids << c->getEffectiveId();

            int id = room->doGongxin(player, target, ids, objectName());

            //case3 cardchosen
            //int id = room->askForCardChosen(player, target, "h", objectName(), true);
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

    static QList<ServerPlayer *> chunxi_targets(ServerPlayer *player){
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, player->getRoom()->getOtherPlayers(player)) {
            if (!p->isKongcheng())
                targets << p;
        }
        return targets;
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to && move.to == player && move.to_place == Player::PlaceHand) {
            if (room->getTag("FirstRound").toBool())
                return QStringList();
            foreach (int id, move.card_ids) { //operate conflict with skill taohuan
                if (Sanguosha->getCard(id)->getSuit() == Card::Heart && room->getCardPlace(id) == Player::PlaceHand) {
				    ServerPlayer *owner = room->getCardOwner(id);
					if (owner && owner == player)
						return QStringList(objectName());
				}    
            }
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer*) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        foreach (int id, move.card_ids) {
            if (Sanguosha->getCard(id)->getSuit() == Card::Heart && room->getCardPlace(id) == Player::PlaceHand ) {
                ServerPlayer *owner = room->getCardOwner(id);
			    if (!owner || owner != player)
					continue;
				QList<ServerPlayer *> targets = chunxi_targets(player);
                if (targets.isEmpty())
                    break;
                
                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@@chunxi", true, true);
                if (target){
                    room->showCard(player, id);
                    int id1 = room->askForCardChosen(player, target, "h", objectName());
                    room->obtainCard(player, id1, false);
                }    
            }
        }
        return false;
    }
};


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
        room->useCard(CardUseStruct(new BllmShiyuCard, bllm, QList<ServerPlayer *>()));
    else if (choice == "bllmseyu")
        room->useCard(CardUseStruct(new BllmSeyuCard, bllm, NULL));
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

    
    virtual bool triggerable(const ServerPlayer *player) const {
        if (TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Start){
            int z = (player->getLostHp() + 1) - player->getMark("@yu");
            return z > 0; 
        }
        return false;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        QString prompt = "bllmwuyu";
        player->tag["wuyu_prompt"] = QVariant::fromValue(prompt);
        return player->askForSkillInvoke(objectName());
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

    
    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Draw;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        return  BllmWuyu::BllmWuyuCost(room, player, "bllmcaiyu");
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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (player->getMark("bllmseyu") > 0);
            room->setPlayerMark(player, "bllmseyu", 0);
        return QStringList();
    }
};

class BllmMingyu : public TriggerSkill
{
public:
    BllmMingyu() : TriggerSkill("#bllmmingyu")
    {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const{
        if (!TriggerSkill::triggerable(player)) return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge){
            if (player->getCards("j").length() == 0) return QStringList(); //just for reducing skill invoke.
            return QStringList(objectName());
        }
        return QStringList();
    }
    
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer*) const{
        return  BllmWuyu::BllmWuyuCost(room, player, "bllmmingyu");
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer*) const{
        player->skip(Player::Judge);
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

    virtual bool triggerable(const ServerPlayer *player) const {
        return TriggerSkill::triggerable(player)
            && player->getPhase() == Player::Discard;
    }
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const {
        return BllmWuyu::BllmWuyuCost(room, player, "bllmshuiyu");
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


class Qiangyu : public TriggerSkill
{
public:
    Qiangyu() : TriggerSkill("qiangyu")
    {
        events << CardsMoveOneTime << DrawCardsFromDrawPile;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == DrawCardsFromDrawPile){
            return QStringList(objectName());
        }
        else if (triggerEvent == CardsMoveOneTime){
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to && move.to == player  && move.to_place == Player::PlaceHand && player->hasFlag("qiangyu")) 
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == DrawCardsFromDrawPile)
            return player->askForSkillInvoke(objectName(), data);
        else if (triggerEvent == CardsMoveOneTime)
            return true;
        return false;
    }
    
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == DrawCardsFromDrawPile){
            data = QVariant::fromValue(data.toInt() + 2);
            room->setPlayerFlag(player, "qiangyu");
        }
        else if (triggerEvent == CardsMoveOneTime){
            room->setPlayerFlag(player, "-qiangyu");
            const Card *card = room->askForCard(player, ".S", "qiangyu_spadecard", data, Card::MethodDiscard, NULL, false, objectName(), false);
             if (!card)
                room->askForDiscard(player, objectName(), 2, 2, false, false);
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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("TrickCard")) 
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        const Card *pilecard = room->askForCard(player, ".Equip", "@mokai", data, Card::MethodNone, NULL, false, objectName(), false);
        if (pilecard) {
            int id = pilecard->getSubcards().first();
            player->addToPile("tianyi", id);
            return true;
        }
        return false;
    }
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QList<int> pile = player->getPile("tianyi");
        if (pile.length() > player->getHp()) {
            if (player->askForSkillInvoke(objectName(), data)) {
                room->fillAG(pile, player);
                int id = room->askForAG(player, pile, false, objectName());
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
                room->clearAG(player);
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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                QList<int> pile = player->getPile("tianyi");
                if (!pile.isEmpty())
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            QString prompt = "invoke:" + use.from->objectName();
            player->tag["guangji_use"] = data;
            return player->askForSkillInvoke(objectName(), prompt);
        } else
            return true;

        return false;
    }
    

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            QList<int> pile = player->getPile("tianyi");
            room->fillAG(pile, player);
            int id = room->askForAG(player, pile, false, objectName());
            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
            room->clearAG(player);
            room->throwCard(Sanguosha->getCard(id), reason, NULL);
            use.nullified_list << player->objectName();
            data = QVariant::fromValue(use);
        }  
        
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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from  && move.from == player && move.from_places.contains(Player::PlaceSpecial)) {
            QStringList trigger_list;
            for (int i = 0; i < move.card_ids.size(); i++) {
                if (move.from_pile_names[i] == "tianyi") trigger_list << objectName();
            }
            return trigger_list;
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName(), data);
    }
    
    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    { 
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        player->drawCards(1);
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

