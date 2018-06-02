#include "thndj.h"

#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"

class Rexue : public TriggerSkill
{
public:
    Rexue()
        : TriggerSkill("rexue")
    {
        events << EventPhaseChanging << TurnStart << Death;
        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const
    {
        if (triggerEvent == Death)
            room->setTag("rexueDeathInThisRound", true);
        else if (triggerEvent == TurnStart)
            room->setTag("rexueDeathInThisRound", false);
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->hasSkill(this) && change.to == Player::NotActive && change.player->getHp() == 1 && !change.player->tag.value("touhou-extra", false).toBool()
                && !room->getTag("rexueDeathInThisRound").toBool())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, NULL, true);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->recover(invoke->invoker, RecoverStruct());
        invoke->invoker->drawCards(1);
        if (!room->getThread()->hasExtraTurn())
            invoke->invoker->gainAnExtraTurn();
        else {
            LogMessage log;
            log.type = "#ForbidExtraTurn";
            log.from = invoke->invoker;

            room->sendLog(log);
        }
        return false;
    }
};

class Sidou : public TriggerSkill
{
public:
    Sidou()
        : TriggerSkill("sidou")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *mokou = data.value<ServerPlayer *>();
        if (mokou->hasSkill(this) && mokou->getPhase() == Player::Start) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                QList<const Card *> cards_judge = p->getCards("j");
                if (!cards_judge.isEmpty())
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, mokou, mokou);

                QList<const Card *> cards = p->getCards("e");
                QList<int> ids;
                foreach (const Card *card, cards) {
                    if (p->getWeapon() && p->getWeapon()->getId() == card->getId())
                        continue;

                    ids << card->getId();
                }

                foreach (int id, ids) {
                    if (mokou->canDiscard(p, id, objectName()))
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, mokou, mokou);
                }
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *mokou = invoke->invoker;
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            QList<const Card *> cards_judge = p->getCards("j");
            if (!cards_judge.isEmpty()) {
                targets << p;
                continue;
            }
            QList<const Card *> cards = p->getCards("e");
            QList<int> ids;
            foreach (const Card *card, cards) {
                if (p->getWeapon() && p->getWeapon()->getId() == card->getId())
                    continue;

                ids << card->getId();
            }

            bool can_select = false;
            foreach (int id, ids) {
                if (mokou->canDiscard(p, id, objectName())) {
                    can_select = true;
                    break;
                }
            }
            if (can_select)
                targets << p;
        }

        ServerPlayer *target = room->askForPlayerChosen(mokou, targets, objectName(), "@sidou_target", true, true);
        if (target) {
            QList<int> disable;
            if (target->getWeapon())
                disable << target->getWeapon()->getId();
#pragma message WARN("todo_Fs: split this askforcardchosen. this skill is \"put the cards in judge area to the discard pile\"")
            int card_id = room->askForCardChosen(mokou, target, "je", objectName(), false, Card::MethodDiscard, disable);
            mokou->showHiddenSkill(objectName());
            room->throwCard(card_id, (target->getJudgingAreaID().contains(card_id)) ? NULL : target, mokou);
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1, objectName());
        room->damage(DamageStruct(objectName(), invoke->invoker, invoke->invoker, 1, DamageStruct::Fire));

        return false;
    }
};

class TymhWuyu : public TriggerSkill
{
public:
    TymhWuyu()
        : TriggerSkill("tymhwuyu$")
    {
        events << Death;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->hasLordSkill(this) && p != death.who)
                d << SkillInvokeDetail(this, p, death.who, NULL, true);
        }

        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        death.viewAsKiller = invoke->owner;
        death.useViewAsKiller = true;
        data = QVariant::fromValue(death);

        room->touhouLogmessage("#TriggerSkill", invoke->owner, "tymhwuyu");
        room->notifySkillInvoked(invoke->owner, objectName());
        return false;
    }
};

class Huanyue : public TriggerSkill
{
public:
    Huanyue()
        : TriggerSkill("huanyue")
    {
        events << DamageInflicted;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card == NULL || !damage.card->isNDTrick())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->getOtherPlayers(damage.to)) {
            if (p->hasSkill(this) && damage.to->canDiscard(p, "hs"))
                d << SkillInvokeDetail(this, p, p, NULL, false, damage.to);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt = "target:" + damage.to->objectName() + ":" + damage.card->objectName();
        invoke->invoker->tag["huanyue_damage"] = data;
        return invoke->invoker->askForSkillInvoke(this, prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), damage.to->objectName());
        int card_id = room->askForCardChosen(damage.to, invoke->invoker, "hs", objectName(), false, Card::MethodDiscard);
        room->throwCard(card_id, invoke->invoker, damage.to);
        if (Sanguosha->getCard(card_id)->isBlack()) {
            QList<ServerPlayer *> logto;
            logto << damage.to;
            room->touhouLogmessage("#huanyue_log", damage.from, QString::number(damage.damage), logto, QString::number(damage.damage + 1));
            damage.damage = damage.damage + 1;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

class Sizhai : public TriggerSkill
{
public:
    Sizhai()
        : TriggerSkill("sizhai")
    {
        events << EventPhaseStart << CardUsed << CardResponded;
        frequency = Frequent;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == CardUsed || triggerEvent == CardResponded) {
            ServerPlayer *current = room->getCurrent();
            if (current == NULL || current->isDead())
                return;

            if (triggerEvent == CardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.from == current && (use.card->isKindOf("BasicCard") || use.card->isKindOf("TrickCard")))
                    current->setFlags("sizhai");
            } else if (triggerEvent == CardResponded) {
                CardResponseStruct card_star = data.value<CardResponseStruct>();
                if (card_star.m_from == current && (card_star.m_card->isKindOf("BasicCard") || card_star.m_card->isKindOf("TrickCard")))
                    current->setFlags("sizhai");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Finish && !player->hasFlag("sizhai")) {
                QList<SkillInvokeDetail> d;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasSkill(this))
                        d << SkillInvokeDetail(this, p, p);
                }
                return d;
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        return invoke->invoker->askForSkillInvoke(this, "draw:" + p->objectName());
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1);
        return false;
    }
};

/*
class Yuanhu : public TriggerSkill
{
public:
    Yuanhu()
        : TriggerSkill("yuanhu")
    {
        events << DrawNCards << AfterDrawNCards;
        show_type = "static";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        DrawNCardsStruct dc = data.value<DrawNCardsStruct>();
        if (triggerEvent == DrawNCards) {
            if (dc.n <= 0)
                return QList<SkillInvokeDetail>();

            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, room->getOtherPlayers(dc.player)) {
                if (p->hasSkill(this))
                    d << SkillInvokeDetail(this, p, dc.player);
            }
            return d;
        } else {
            if (dc.player->hasFlag("yuanhu_draw")) {
                QStringList drawer = dc.player->tag.value("yuanhu_drawers", QStringList()).toStringList();
                if (drawer.isEmpty())
                    return QList<SkillInvokeDetail>();

                QList<SkillInvokeDetail> d;
                foreach (const QString &s, drawer.toSet()) {
                    ServerPlayer *p = room->findPlayerByObjectName(s);
                    if (p == NULL)
                        continue;

                    d << SkillInvokeDetail(this, p, dc.player, NULL, true);
                }
                return d;
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == DrawNCards) {
            QString prompt = "invoke:" + invoke->owner->objectName();
            invoke->invoker->tag["yuanhu"] = QVariant::fromValue(invoke->owner);
            if (invoke->invoker->askForSkillInvoke(this, prompt)) {
                DrawNCardsStruct dc = data.value<DrawNCardsStruct>();
                --dc.n;
                data = QVariant::fromValue(dc);
                return true;
            }
        } else {
            QStringList drawer = invoke->invoker->tag.value("yuanhu_drawers", QStringList()).toStringList();
            drawer.removeOne(invoke->owner->objectName());
            invoke->invoker->tag["yuanhu_drawers"] = QVariant::fromValue(drawer);
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == DrawNCards) {
            invoke->invoker->setFlags("yuanhu_draw");
            QStringList drawer = invoke->invoker->tag.value("yuanhu_drawers", QStringList()).toStringList();
            drawer << invoke->owner->objectName();
            invoke->invoker->tag["yuanhu_drawers"] = QVariant::fromValue(drawer);
        } else {
            invoke->owner->drawCards(1, objectName());
            invoke->owner->tag["yuanhu_target"] = QVariant::fromValue(invoke->invoker);
            const Card *c = room->askForExchange(invoke->owner, objectName(), 2, 1, false, "@yuanhu-exchange:" + invoke->invoker->objectName(), true);
            if (c != NULL) {
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, invoke->owner->objectName(), objectName(), QString());
                room->obtainCard(invoke->invoker, c, reason, false);
            }
        }

        return false;
    }
};
*/

class Yuanhu : public TriggerSkill
{
public:
    Yuanhu()
        : TriggerSkill("yuanhu")
    {
        events << EventPhaseEnd;
        show_type = "static";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->isDead() || current->getPhase() != Player::Draw || current->isKongcheng())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p != current)
                d << SkillInvokeDetail(this, p, current);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        invoke->invoker->tag["yuanhu_drawers"] = QVariant::fromValue(invoke->owner);
        const Card *c = room->askForCard(invoke->invoker, ".", "@yuanhu:" + invoke->owner->objectName(), data, Card::MethodNone);
        //(invoke->owner, objectName(), 2, 1, false, "@yuanhu-exchange:" + invoke->invoker->objectName(), true);
        if (c) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->owner->objectName());
            invoke->invoker->tag["yuanhu_id"] = QVariant::fromValue(c->getEffectiveId());
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(invoke->owner, objectName());
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = invoke->invoker;
            log.to << invoke->owner;
            log.arg = objectName();
            room->sendLog(log);

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        int card_id = invoke->invoker->tag["yuanhu_id"].toInt();
        bool visible = invoke->invoker->getShownHandcards().contains(card_id);
        invoke->owner->obtainCard(Sanguosha->getCard(card_id), visible);

        invoke->owner->tag["yuanhu_target"] = QVariant::fromValue(invoke->invoker);
        const Card *c = room->askForExchange(invoke->owner, objectName(), 2, 1, false, "@yuanhu-exchange:" + invoke->invoker->objectName(), true);
        if (c != NULL) {
            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, invoke->owner->objectName(), objectName(), QString());
            room->obtainCard(invoke->invoker, c, reason, false);
        }
        return false;
    }
};

class Shouxie : public TriggerSkill
{
public:
    Shouxie()
        : TriggerSkill("shouxie")
    {
        events << EventPhaseChanging << EventPhaseStart;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard && change.player->hasSkill(this) && change.player->getHandcardNum() <= 7)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, NULL, true);
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->getPhase() == Player::Finish && p->hasSkill(this) && p->getHandcardNum() < p->getMaxCards())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, p, p, NULL, true);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == EventPhaseChanging)
            invoke->invoker->skip(Player::Discard);
        else if (triggerEvent == EventPhaseStart)
            invoke->invoker->drawCards(invoke->invoker->getMaxCards() - invoke->invoker->getHandcardNum(), objectName());

        return false;
    }
};

HunpoCard::HunpoCard()
{
    will_throw = true;
    target_fixed = true;
    mute = true;
}

void HunpoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->setPlayerProperty(source, "maxhp", source->getMaxHp() + 1);
    room->touhouLogmessage("#GainMaxHp", source, QString::number(1));
    room->touhouLogmessage("#GetHp", source, QString::number(source->getHp()), QList<ServerPlayer *>(), QString::number(source->getMaxHp()));
}

class Hunpo : public OneCardViewAsSkill
{
public:
    Hunpo()
        : OneCardViewAsSkill("hunpo")
    {
        filter_pattern = ".|.|.|.!";
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return !Self->isJilei(to_select);
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isNude() && player->getMaxHp() < 4;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            HunpoCard *card = new HunpoCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }
};

class Fanji : public TriggerSkill
{
public:
    Fanji()
        : TriggerSkill("fanji")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;
        if (damage.to->isDead() || !damage.from || damage.from->isDead() || damage.from == damage.to)
            return d;

        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p != damage.from && (p->inMyAttackRange(damage.to) || p == damage.to))
                d << SkillInvokeDetail(this, p, p, NULL, false, damage.from);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        invoke->invoker->tag["fanji_damage"] = data;
        QString prompt = "target:" + damage.from->objectName() + ":" + damage.to->objectName();
        if (invoke->invoker->askForSkillInvoke(this, prompt)) {
            invoke->invoker->showHiddenSkill(objectName());
            QString choice = room->askForChoice(invoke->invoker, objectName(), "hp+maxhp");
            if (choice == "maxhp")
                room->loseMaxHp(invoke->invoker);
            else
                room->loseHp(invoke->invoker);

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (invoke->invoker->isAlive())
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), damage.from->objectName());
        room->damage(DamageStruct("fanji", (invoke->invoker->isAlive()) ? invoke->invoker : NULL, damage.from));

        return false;
    }
};

class Zaiwu : public TriggerSkill
{
public:
    Zaiwu()
        : TriggerSkill("zaiwu")
    {
        events << DrawNCards;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DrawNCardsStruct qnum = data.value<DrawNCardsStruct>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p == qnum.player)
                continue;
            if (p->getHp() > qnum.player->getHp())
                d << SkillInvokeDetail(this, p, p, NULL, false, qnum.player);
            else if (p->getHp() == 1 && qnum.n > 0)
                d << SkillInvokeDetail(this, p, p, NULL, false, qnum.player);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString prompt = (invoke->invoker->getHp() > invoke->preferredTarget->getHp()) ? "plus:" : "minus:";
        prompt = prompt + invoke->preferredTarget->objectName();
        invoke->invoker->tag["zaiwu"] = QVariant::fromValue(invoke->preferredTarget);
        return invoke->invoker->askForSkillInvoke(this, prompt);
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        bool plus = invoke->invoker->getHp() > invoke->targets.first()->getHp();
        DrawNCardsStruct draw = data.value<DrawNCardsStruct>();
        if (!plus)
            draw.n = draw.n - 1;
        else
            draw.n = draw.n + 1;

        data = QVariant::fromValue(draw);
        return false;
    }
};

class LiexiVS : public ZeroCardViewAsSkill
{
public:
    LiexiVS()
        : ZeroCardViewAsSkill("liexi")
    {
        response_pattern = "@@liexi";
    }

    virtual const Card *viewAs() const
    {
        Slash *s = new Slash(Card::NoSuit, 0);
        s->setSkillName(objectName());
        return s;
    }
};
class Liexi : public TriggerSkill
{
public:
    Liexi()
        : TriggerSkill("liexi")
    {
        events << EventPhaseEnd << TargetSpecifying;
        view_as_skill = new LiexiVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == EventPhaseEnd) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->getPhase() == Player::Play && player->isAlive()) {
                Slash *s = new Slash(Card::NoSuit, 0);
                s->setSkillName(objectName());
                s->deleteLater();
                if (!player->isCardLimited(s, Card::MethodUse)) {
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
                }
            }
        } else if (e == TargetSpecifying) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.card->getSkillName() == objectName()) {
                foreach (ServerPlayer *t, room->getOtherPlayers(use.from)) {
                    if (!use.to.contains(t) && use.from->canSlash(t, use.card, false))
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, true);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }
    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (e == EventPhaseEnd) {
            room->askForUseCard(invoke->invoker, "@@liexi", "@liexi");
            return false;
        }
        return true;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *t, room->getOtherPlayers(use.from)) {
            if (!use.to.contains(t) && use.from->canSlash(t, use.card, false))
                targets << t;
        }
        room->setTag("liexi_extra", data);
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@liexi_extra");
        QString prompt = "target:" + use.from->objectName() + ":" + invoke->invoker->objectName() + ":" + use.card->objectName();
        if (target->askForSkillInvoke("liexi_extra", prompt)) {
            use.to << target;
            room->sortByActionOrder(use.to);
            data = QVariant::fromValue(use);
            target->drawCards(1);

            room->touhouLogmessage("#liexi_extra", use.from, use.card->objectName(), QList<ServerPlayer *>() << target);
        } else {
            use.nullified_list << "_ALL_TARGETS";
            data = QVariant::fromValue(use);
        }

        return false;
    }
};
class LiexiTargetMod : public TargetModSkill
{
public:
    LiexiTargetMod()
        : TargetModSkill("#liexi")
    {
        pattern = "Slash";
    }

    int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->getSkillName() == "liexi")
            return 1000;

        return 0;
    }
};

class Mengwei : public TriggerSkill
{
public:
    Mengwei()
        : TriggerSkill("mengwei")
    {
        events << TargetConfirming << DamageDone << PostCardEffected << SlashEffected;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        switch (triggerEvent) {
        case DamageDone: {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != NULL && damage.card->isKindOf("Slash") && damage.card->hasFlag("mengwei_extra"))
                room->setCardFlag(damage.card, "mengwei_damage");
            break;
        }
        case PostCardEffected: {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->hasFlag("mengwei_extra") && !effect.card->hasFlag("mengwei_damage"))
                room->setCardFlag(effect.card, "mengwei_cancel");
            break;
        }
        default:
            break;
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (e == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                foreach (ServerPlayer *p, use.to) {
                    if (p->hasSkill(this)) {
                        foreach (ServerPlayer *t, room->getOtherPlayers(p)) {
                            if (!use.to.contains(t) && use.from->canSlash(t, use.card, false)) {
                                d << SkillInvokeDetail(this, p, p);
                                break;
                            }
                        }
                    }
                }
            }
        } else if (e == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("mengwei_cancel")) {
                d << SkillInvokeDetail(this, effect.from, effect.from, NULL, true);
            }
        }
        return d;
    }

    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = invoke->invoker;
        if (e == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *t, room->getOtherPlayers(player)) {
                if (!use.to.contains(t) && use.from->canSlash(t, use.card, false))
                    targets << t;
            }
            room->setTag("mengwei_extra", data);
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@mengwei:" + use.from->objectName(), true, true);
            if (target) {
                invoke->targets << target;
                return true;
            }
        } else if (e == SlashEffected)
            return true;
        return false;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            QString prompt = "target:" + use.from->objectName() + ":" + invoke->invoker->objectName() + ":" + use.card->objectName();
            if (invoke->targets.first()->askForSkillInvoke("mengwei_extra", prompt)) {
                use.to << invoke->targets.first();
                room->sortByActionOrder(use.to);
                room->setCardFlag(use.card, "mengwei_extra");
                data = QVariant::fromValue(use);
                invoke->targets.first()->drawCards(1);

                room->touhouLogmessage("#mengwei_extra", use.from, use.card->objectName(), QList<ServerPlayer *>() << invoke->targets.first());
            }
        } else if (e == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            LogMessage log;
            log.type = "#MengweiNullified";
            log.from = effect.from;
            log.to << effect.to;
            log.arg = objectName();
            log.arg2 = effect.slash->objectName();
            room->sendLog(log);
            room->setEmotion(effect.to, "skill_nullify");
            return true;
        }
        return false;
    }
};

class Liangzi : public TriggerSkill
{
public:
    Liangzi()
        : TriggerSkill("liangzi")
    {
        events << CardUsed << CardResponded;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = NULL;
        const Card *card = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            player = use.from;
            card = use.card;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            player = resp.m_from;
            card = resp.m_card;
        }

        if (player != NULL && player->hasSkill(this) && card != NULL && card->getTypeId() == Card::TypeBasic)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        LogMessage log;
        log.from = invoke->invoker;
        log.arg = "liangzi";
        log.type = "#TriggerSkill";
        room->sendLog(log);
        room->notifySkillInvoked(invoke->invoker, "liangzi");

        room->setPlayerProperty(invoke->invoker, "chained", !invoke->invoker->isChained());
        return false;
    }
};

class Kexue : public TargetModSkill
{
public:
    Kexue()
        : TargetModSkill("kexue")
    {
        pattern = "Slash";
    }
    virtual int getDistanceLimit(const Player *from, const Card *) const
    {
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && from->hasSkill(objectName()) && from->isChained())
            return 1000;
        else
            return 0;
    }

    virtual int getExtraTargetNum(const Player *player, const Card *) const
    {
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && player->hasSkill(objectName()) && player->isChained())
            return 1000;
        else
            return 0;
    }
};

class KexueEffect : public TriggerSkill
{
public:
    KexueEffect()
        : TriggerSkill("#kexue-effect")
    {
        events << PreCardUsed;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.to.length() > 1 && use.from->isChained() && use.from->hasSkill("kexue"))
            room->notifySkillInvoked(use.from, "kexue");
    }
};

class Xiubu : public TriggerSkill
{
public:
    Xiubu()
        : TriggerSkill("xiubu")
    {
        events << CardResponded << CardUsed << EventPhaseChanging << EventPhaseEnd << PreCardUsed;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        //clear histroy
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.m_isLastHandcard && use.m_addHistory && !use.card->isKindOf("SkillCard") && use.from->getMark("xiubu") > 0) {
                room->addPlayerHistory(use.from, use.card->getClassName(), -1);
                use.m_addHistory = false;
                data = QVariant::fromValue(use);
            }
        }

        //record times of using card
        if (triggerEvent == CardUsed || triggerEvent == CardResponded) {
            ServerPlayer *player = NULL;
            const Card *card = NULL;
            if (triggerEvent == CardUsed) {
                player = data.value<CardUseStruct>().from;
                card = data.value<CardUseStruct>().card;
            } else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                player = response.m_from;
                if (response.m_isUse)
                    card = response.m_card;
            }
            if (player && player->getPhase() == Player::Play && card && card->getHandlingMethod() == Card::MethodUse && !card->isKindOf("SkillCard")) {
                if (player->hasFlag("xiubu_first"))
                    player->setFlags("xiubu_second");
                else
                    player->setFlags("xiubu_first");
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                change.player->setFlags("-xiubu_first");
                change.player->setFlags("-xiubu_second");
                room->setPlayerMark(change.player, "xiubu", 0);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == CardUsed || triggerEvent == CardResponded) {
            ServerPlayer *player = NULL;
            const Card *card = NULL;
            if (triggerEvent == CardUsed) {
                player = data.value<CardUseStruct>().from;
                card = data.value<CardUseStruct>().card;
            } else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                player = response.m_from;
                if (response.m_isUse)
                    card = response.m_card;
            }
            if (player && player->getPhase() == Player::Play && card && card->getHandlingMethod() == Card::MethodUse && !card->isKindOf("SkillCard")) {
                if (player->hasFlag("xiubu_first") && !player->hasFlag("xiubu_second")) {
                    QList<SkillInvokeDetail> d;
                    foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                        if ((p->inMyAttackRange(player) || p == player) && p->canDiscard(player, "hs"))
                            d << SkillInvokeDetail(this, p, p, NULL, false, player);
                    }

                    return d;
                }
            }
        } else if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Play && player->getMark("xiubu") > 0 && player->hasFlag("xiubu_second"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == EventPhaseEnd)
            return true;
        else {
            if (invoke->invoker == invoke->preferredTarget) {
                if (room->askForCard(invoke->invoker, ".", "@xiubu-self", QVariant::fromValue(invoke->preferredTarget), objectName()))
                    return true;
            } else {
                if (invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget))) {
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->preferredTarget->objectName());
                    int id = room->askForCardChosen(invoke->invoker, invoke->preferredTarget, "hs", objectName());
                    room->throwCard(id, invoke->preferredTarget, invoke->invoker);
                    return true;
                }
            }
        }
        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == EventPhaseEnd)
            invoke->invoker->drawCards(2);
        else
            room->setPlayerMark(invoke->targets.first(), "xiubu", 1);

        return false;
    }
};

class XiubuTargetMod : public TargetModSkill
{
public:
    XiubuTargetMod()
        : TargetModSkill("#xiubu-mod")
    {
        pattern = "BasicCard,TrickCard";
    }
    static bool isLastHandCard(const Player *player, const Card *card)
    {
        QList<int> subcards = card->getSubcards();
        if (subcards.length() != 1 || player->getHandcardNum() != 1)
            return false;

        return subcards.contains(player->getHandcards().first()->getEffectiveId());
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const
    {
        if (from->getMark("xiubu") > 0 && isLastHandCard(from, card))
            return 1000;
        else
            return 0;
    }

    virtual int getResidueNum(const Player *from, const Card *card) const
    {
        if (from->getMark("xiubu") > 0 && isLastHandCard(from, card))
            return 1000;
        else
            return 0;
    }
};

class JinengVS : public OneCardViewAsSkill
{
public:
    JinengVS()
        : OneCardViewAsSkill("jineng")
    {
        //response_or_use = true;
        expand_pile = "jinengPile";
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        //QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (player->getPile("jinengPile").isEmpty())
            return false;
        return matchAvaliablePattern("slash", pattern) || matchAvaliablePattern("jink", pattern) || matchAvaliablePattern("analeptic", pattern)
            || matchAvaliablePattern("known_both", pattern);
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("jinengPile").isEmpty();
        //return Slash::IsAvailable(player) || Analeptic::IsAvailable(player);
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        if (!Self->getPile("jinengPile").contains(to_select->getEffectiveId()))
            return false;

        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
            if (to_select->getSuit() == Card::Club)
                return true;
            else if (to_select->getSuit() == Card::Spade) {
                Slash *slash = new Slash(Card::SuitToBeDecided, -1);
                slash->addSubcard(to_select);
                slash->deleteLater();
                return slash->isAvailable(Self);
            } else if (to_select->getSuit() == Card::Diamond) {
                Analeptic *ana = new Analeptic(Card::SuitToBeDecided, -1);
                ana->addSubcard(to_select);
                ana->deleteLater();
                return ana->isAvailable(Self);
            }
            return false;
        }
        case CardUseStruct::CARD_USE_REASON_RESPONSE:
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            return (to_select->getSuit() == Card::Heart && matchAvaliablePattern("jink", pattern))
                || (to_select->getSuit() == Card::Spade && matchAvaliablePattern("slash", pattern))
                || (to_select->getSuit() == Card::Diamond && matchAvaliablePattern("analeptic", pattern))
                || (to_select->getSuit() == Card::Club && matchAvaliablePattern("known_both", pattern));
        }
        default:
            break;
        }

        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard == NULL)
            return NULL;

        Card *new_card = NULL;

        switch (originalCard->getSuit()) {
        case Card::Spade: {
            new_card = new Slash(Card::SuitToBeDecided, -1);
            break;
        }
        case Card::Heart: {
            new_card = new Jink(Card::SuitToBeDecided, -1);
            break;
        }
        case Card::Club: {
            new_card = new KnownBoth(Card::SuitToBeDecided, -1);
            break;
        }
        case Card::Diamond: {
            new_card = new Analeptic(Card::SuitToBeDecided, -1);
            break;
        }
        default:
            break;
        }

        if (new_card) {
            new_card->setSkillName(objectName());
            new_card->addSubcard(originalCard);
        }
        return new_card;
    }
};

class Jineng : public TriggerSkill
{
public:
    Jineng()
        : TriggerSkill("jineng")
    {
        events << EventPhaseStart << Damage << FinishJudge;
        view_as_skill = new JinengVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == EventPhaseStart) {
            ServerPlayer *aya = data.value<ServerPlayer *>();
            if (aya->hasSkill(this) && aya->isAlive() && aya->getPhase() == Player::Start)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, aya, aya);
        }
        if (e == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->hasSkill(this) && damage.from->isAlive())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);
        }
        if (e == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName()) {
                ServerPlayer *aya = judge->who;
                if (aya && aya->isAlive() && room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
                    bool can = true;
                    foreach (int id, aya->getPile("jinengPile")) {
                        if (judge->card->getSuit() == Sanguosha->getCard(id)->getSuit()) {
                            can = false;
                            break;
                        }
                    }
                    if (can)
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, aya, aya, NULL, true);
                }
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            invoke->invoker->addToPile("jinengPile", judge->card->getEffectiveId());
        } else {
            ServerPlayer *aya = invoke->invoker;

            JudgeStruct judge;
            judge.who = aya;
            judge.pattern = ".";
            judge.good = false;
            judge.reason = objectName();
            room->judge(judge);
        }
        return false;
    }
};

class JinengTargetMod : public TargetModSkill
{
public:
    JinengTargetMod()
        : TargetModSkill("#jinengmod")
    {
        pattern = "Slash";
    }

    virtual int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->getSkillName() == "jineng")
            return 1000;
        else
            return 0;
    }
};

class Kuaibao : public TriggerSkill
{
public:
    Kuaibao()
        : TriggerSkill("kuaibao")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current && current->getPhase() == Player::Start && current->isAlive()) {
            foreach (ServerPlayer *aya, room->findPlayersBySkillName(objectName())) {
                if (aya != current && !aya->getPile("jinengPile").isEmpty() && aya->getPile("jinengPile").length() > aya->getHp())
                    d << SkillInvokeDetail(this, aya, aya);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QList<int> ids = invoke->invoker->getPile("jinengPile");
        CardsMoveStruct move(ids, invoke->invoker, invoke->invoker, Player::PlaceSpecial, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_UNKNOWN, QString()));
        room->moveCardsAtomic(move, true);

        LogMessage mes;
        mes.type = "$Kuaibao";
        mes.from = invoke->invoker;
        mes.arg = objectName();
        mes.card_str = IntList2StringList(ids).join("+");
        room->sendLog(mes);

        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->isAlive())
            room->damage(DamageStruct(objectName(), invoke->invoker, current));

        return false;
    }
};

THNDJPackage::THNDJPackage()
    : Package("thndj")
{
    General *mokou_ndj = new General(this, "mokou_ndj$", "zhu", 4);
    mokou_ndj->addSkill(new Rexue);
    mokou_ndj->addSkill(new Sidou);
    mokou_ndj->addSkill(new TymhWuyu);

    General *kaguya_ndj = new General(this, "kaguya_ndj", "zhu", 3);
    kaguya_ndj->addSkill(new Huanyue);
    kaguya_ndj->addSkill(new Sizhai);

    General *yukari_ndj = new General(this, "yukari_ndj", "yym", 3);
    yukari_ndj->addSkill(new Yuanhu);
    yukari_ndj->addSkill(new Shouxie);

    General *youmu_ndj = new General(this, "youmu_ndj", "yym", 4);
    youmu_ndj->addSkill(new Hunpo);
    youmu_ndj->addSkill(new Fanji);

    General *merry_ndj = new General(this, "merry_ndj", "wai", 3);
    merry_ndj->addSkill(new Liexi);
    merry_ndj->addSkill(new LiexiTargetMod);
    merry_ndj->addSkill(new Mengwei);
    related_skills.insertMulti("liexi", "#liexi");

    General *renko_ndj = new General(this, "renko_ndj", "wai", 4);
    renko_ndj->addSkill(new Liangzi);
    renko_ndj->addSkill(new Kexue);
    renko_ndj->addSkill(new KexueEffect);
    related_skills.insertMulti("kexue", "#kexue-effect");

    General *sanae_ndj = new General(this, "sanae_ndj", "fsl", 4);
    sanae_ndj->addSkill(new Xiubu);
    sanae_ndj->addSkill(new XiubuTargetMod);
    related_skills.insertMulti("xiubu", "#xiubu-mod");

    General *aya_ndj = new General(this, "aya_ndj", "fsl", 3);
    aya_ndj->addSkill(new Jineng);
    aya_ndj->addSkill(new JinengTargetMod);
    aya_ndj->addSkill(new Kuaibao);
    related_skills.insertMulti("jineng", "#jinengmod");

    addMetaObject<HunpoCard>();
}

ADD_PACKAGE(THNDJ)
