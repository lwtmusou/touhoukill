#include "thndj.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"

#include <random>

class Rexue : public TriggerSkill
{
public:
    Rexue()
        : TriggerSkill("rexue")
    {
        events << EventPhaseChanging << TurnStart << Death;
        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const override
    {
        if (triggerEvent == Death)
            room->setTag("rexueDeathInThisRound", true);
        else if (triggerEvent == TurnStart)
            room->setTag("rexueDeathInThisRound", false);
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->hasSkill(this) && change.to == Player::NotActive && change.player->getHp() == 1 && !change.player->tag.value("touhou-extra", false).toBool()
                && !room->getTag("rexueDeathInThisRound").toBool())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, nullptr, true);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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
            int card_id = room->askForCardChosen(mokou, target, "je", objectName(), false, Card::MethodDiscard, disable);
            mokou->showHiddenSkill(objectName()); // ??????????????????????????????????
            room->throwCard(card_id, (target->getJudgingAreaID().contains(card_id)) ? nullptr : target, mokou);
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DeathStruct death = data.value<DeathStruct>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->hasLordSkill(this) && p != death.who)
                d << SkillInvokeDetail(this, p, death.who, nullptr, true);
        }

        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

class HuanyueVS : public OneCardViewAsSkill
{
public:
    HuanyueVS()
        : OneCardViewAsSkill("huanyue")
    {
        response_pattern = "@@huanyue";
        expand_pile = "huanyue_pile";
    }

    bool viewFilter(const Card *to_select, const Player *Self) const override
    {
        if (!Self->getPile("huanyue_pile").contains(to_select->getId()))
            return false;

        QString name = Self->property("huanyue").toString();
        return to_select->getType() != name;
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
    {
        return originalCard;
    }
};

class Huanyue : public TriggerSkill
{
public:
    Huanyue()
        : TriggerSkill("huanyue")
    {
        events << DamageInflicted << Damage << Damaged;
        view_as_skill = new HuanyueVS;
        related_pile = "huanyue_pile";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (e == Damage) {
            if (!damage.card || !damage.from || !damage.from->isAlive() || !damage.from->hasSkill(this))
                return QList<SkillInvokeDetail>();
            QList<int> ids;
            if (damage.card->isVirtualCard())
                ids = damage.card->getSubcards();
            else
                ids << damage.card->getEffectiveId();

            if (ids.isEmpty())
                return QList<SkillInvokeDetail>();
            foreach (int id, ids) {
                if (room->getCardPlace(id) != Player::PlaceTable)
                    return QList<SkillInvokeDetail>();
            }
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);
        }
        if (e == Damaged) {
            if (!damage.card || !damage.to || !damage.to->isAlive() || !damage.to->hasSkill(this))
                return QList<SkillInvokeDetail>();
            QList<int> ids;
            if (damage.card->isVirtualCard())
                ids = damage.card->getSubcards();
            else
                ids << damage.card->getEffectiveId();

            if (ids.isEmpty())
                return QList<SkillInvokeDetail>();
            foreach (int id, ids) {
                if (room->getCardPlace(id) != Player::PlaceTable)
                    return QList<SkillInvokeDetail>();
            }
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        }

        if (e == DamageInflicted) {
            if (damage.card == nullptr)
                return QList<SkillInvokeDetail>();

            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, room->getOtherPlayers(damage.to)) {
                if (p->hasSkill(this)) {
                    foreach (int id, p->getPile("huanyue_pile")) {
                        if (room->getCard(id)->getTypeId() != damage.card->getTypeId()) {
                            d << SkillInvokeDetail(this, p, p, nullptr, false, damage.to);
                            break;
                        }
                    }
                }
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (e == Damage || e == Damaged) {
            QString prompt = "target:" + damage.to->objectName() + ":" + damage.card->objectName();
            invoke->invoker->tag["huanyue_damage"] = data;
            return invoke->invoker->askForSkillInvoke(this, prompt);
        } else {
            QString prompt = QString("@huanyue:%1:%2").arg(damage.to->objectName()).arg(damage.card->objectName());
            invoke->invoker->setProperty("huanyue", damage.card->getType());
            const Card *c = room->askForCard(invoke->invoker, "@@huanyue", prompt, data, Card::MethodNone, nullptr, false, "huanyue");
            if (c) {
                room->notifySkillInvoked(invoke->invoker, objectName());
                room->touhouLogmessage("#InvokeSkill", invoke->invoker, objectName());
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), damage.to->objectName());
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), nullptr, objectName(), QString());
                room->throwCard(c, reason, nullptr);
                return true;
            }
        }
        return false;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (e == Damage || e == Damaged) {
            invoke->invoker->addToPile("huanyue_pile", damage.card);
            QList<int> ids = invoke->invoker->getPile("huanyue_pile");
            if (ids.length() > 1) {
                room->fillAG(ids, invoke->invoker);
                int keep = room->askForAG(invoke->invoker, ids, true, objectName());
                ids.removeOne(keep);
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), nullptr, objectName(), QString());
                DummyCard *dc = new DummyCard;
                dc->deleteLater();
                dc->addSubcards(ids);
                room->throwCard(dc, reason, nullptr);
                room->clearAG(invoke->invoker);
            }
        } else {
            QList<ServerPlayer *> logto;
            logto << damage.to;
            room->touhouLogmessage("#huanyue_log", damage.from, QString::number(damage.damage), logto, QString::number(damage.damage + 1));
            damage.damage = damage.damage + 1;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

class Wanggou : public TriggerSkill
{
public:
    Wanggou()
        : TriggerSkill("wanggou")
    {
        events << EventPhaseStart;
    }

    static void do_wanggou(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        int acquired = 0;
        QList<int> throwIds;
        const Card *card = nullptr;
        while (acquired < 1) {
            int id = room->drawCard();
            CardsMoveStruct move(id, nullptr, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName()));
            move.reason.m_skillName = "wanggou";
            room->moveCardsAtomic(move, true);
            room->getThread()->delay();
            card = room->getCard(id);

            bool get = false;
            if (card->isKindOf("Slash") || card->isKindOf("FireAttack") || card->isKindOf("ArcheryAttack") || card->isKindOf("SavageAssault") || card->isKindOf("Duel")
                || card->isKindOf("Lightning") || card->isKindOf("BoneHealing"))
                get = true;

            if (get) {
                acquired = acquired + 1;
                CardsMoveStruct move2(id, player, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_GOTBACK, player->objectName()));
                room->moveCardsAtomic(move2, true);

                if (!throwIds.isEmpty()) {
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "wanggou", QString());
                    DummyCard dummy(throwIds);
                    room->throwCard(&dummy, reason, nullptr);
                    throwIds.clear();
                }
            } else
                throwIds << id;
        }
        if (card->isKindOf("Slash") && !card->isKindOf("NatureSlash") && !card->isKindOf("DebuffSlash")) {
            if (room->askForSkillInvoke(player, "wanggou", "retry")) {
                room->throwCard(card, player, player);
                do_wanggou(player);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player && player->getPhase() == Player::Play && player->isAlive() && player->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        do_wanggou(invoke->invoker);
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == CardUsed || triggerEvent == CardResponded) {
            ServerPlayer *current = room->getCurrent();
            if (current == nullptr || current->isDead())
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        return invoke->invoker->askForSkillInvoke(this, "draw:" + p->objectName());
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->drawCards(1);
        return false;
    }
};

class Yuanhu : public TriggerSkill
{
public:
    Yuanhu()
        : TriggerSkill("yuanhu")
    {
        events << EventPhaseEnd;
        show_type = "static";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->isDead() || current->getPhase() != Player::Draw || current->isKongcheng())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p != current && p->hasShownSkill(this))
                d << SkillInvokeDetail(this, p, current);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        invoke->invoker->tag["yuanhu_drawers"] = QVariant::fromValue(invoke->owner);
        const Card *c = room->askForCard(invoke->invoker, ".", "@yuanhu:" + invoke->owner->objectName(), data, Card::MethodNone);
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        int card_id = invoke->invoker->tag["yuanhu_id"].toInt();
        bool visible = invoke->invoker->getShownHandcards().contains(card_id);
        invoke->owner->obtainCard(room->getCard(card_id), visible);

        invoke->owner->tag["yuanhu_target"] = QVariant::fromValue(invoke->invoker);
        const Card *c = room->askForExchange(invoke->owner, objectName(), 2, 1, true, "@yuanhu-exchange:" + invoke->invoker->objectName(), true);
        if (c != nullptr) {
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard && change.player->hasSkill(this) && change.player->getHandcardNum() <= 7)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, nullptr, true);
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->getPhase() == Player::Finish && p->hasSkill(this) && p->getHandcardNum() < p->getMaxCards())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, p, p, nullptr, true);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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
        filter_pattern = ".!";
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->isNude() && player->getMaxHp() < 4;
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
    {
        HunpoCard *card = new HunpoCard;
        card->addSubcard(originalCard);
        return card;
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;
        if (damage.to->isDead() || !damage.from || damage.from->isDead() || damage.from == damage.to)
            return d;

        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p != damage.from && (p->inMyAttackRange(damage.to) || p == damage.to))
                d << SkillInvokeDetail(this, p, p, nullptr, false, damage.from);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (invoke->invoker->isAlive())
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), damage.from->objectName());
        room->damage(DamageStruct("fanji", (invoke->invoker->isAlive()) ? invoke->invoker : nullptr, damage.from));

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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DrawNCardsStruct qnum = data.value<DrawNCardsStruct>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p == qnum.player)
                continue;
            if (p->getHp() > qnum.player->getHp())
                d << SkillInvokeDetail(this, p, p, nullptr, false, qnum.player);
            else if (p->getHp() == 1 && qnum.n > 0)
                d << SkillInvokeDetail(this, p, p, nullptr, false, qnum.player);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QString prompt = (invoke->invoker->getHp() > invoke->preferredTarget->getHp()) ? "plus:" : "minus:";
        prompt = prompt + invoke->preferredTarget->objectName();
        invoke->invoker->tag["zaiwu"] = QVariant::fromValue(invoke->preferredTarget);
        return invoke->invoker->askForSkillInvoke(this, prompt);
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    const Card *viewAs(const Player * /*Self*/) const override
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

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
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
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, nullptr, true);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }
    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (e == EventPhaseEnd) {
            room->askForUseCard(invoke->invoker, "@@liexi", "@liexi");
            return false;
        }
        return true;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    int getDistanceLimit(const Player *, const Card *card) const override
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        switch (triggerEvent) {
        case DamageDone: {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != nullptr && damage.card->isKindOf("Slash") && damage.card->hasFlag("mengwei_extra"))
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
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
                d << SkillInvokeDetail(this, effect.from, effect.from, nullptr, true);
            }
        }
        return d;
    }

    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *player = nullptr;
        const Card *card = nullptr;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            player = use.from;
            card = use.card;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            player = resp.m_from;
            card = resp.m_card;
        }

        if (player != nullptr && player->hasSkill(this) && card != nullptr && card->getTypeId() == Card::TypeBasic)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, nullptr, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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
        pattern = "Slash,TrickCard+^DelayedTrick";
    }

    int getDistanceLimit(const Player *from, const Card *) const override
    {
        if (from->getRoomObject()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && from->hasSkill(objectName()) && from->isChained())
            return 1000;
        else
            return 0;
    }

    int getExtraTargetNum(const Player *player, const Card *) const override
    {
        if (player->getRoomObject()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && player->hasSkill(objectName()) && player->isChained())
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

    void record(TriggerEvent, Room *room, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card != nullptr && (use.card->isKindOf("Slash") || use.card->isNDTrick()) && use.to.length() > 1 && use.from != nullptr && use.from->isChained()
            && use.from->hasSkill("kexue"))
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
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
            if (player && player->getPhase() == Player::Play && card && card->getHandlingMethod() == Card::MethodUse && !card->isKindOf("SkillCard")) {
                if (player->hasFlag("xiubu_first") && !player->hasFlag("xiubu_second")) {
                    QList<SkillInvokeDetail> d;
                    foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                        if ((p->inMyAttackRange(player) || p == player) && p->canDiscard(player, "hs"))
                            d << SkillInvokeDetail(this, p, p, nullptr, false, player);
                    }

                    return d;
                }
            }
        } else if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Play && player->getMark("xiubu") > 0 && player->hasFlag("xiubu_second"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, nullptr, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    int getDistanceLimit(const Player *from, const Card *card) const override
    {
        if (from->getMark("xiubu") > 0 && isLastHandCard(from, card))
            return 1000;
        else
            return 0;
    }

    int getResidueNum(const Player *from, const Card *card) const override
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
        expand_pile = "jinengPile";
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        if (player->getPile("jinengPile").isEmpty())
            return false;

        Slash s(Card::SuitToBeDecided, -1);
        Jink j(Card::SuitToBeDecided, -1);
        Analeptic a(Card::SuitToBeDecided, -1);
        KnownBoth k(Card::SuitToBeDecided, -1);

        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);

        return cardPattern != nullptr && (cardPattern->match(player, &s) || cardPattern->match(player, &j) || cardPattern->match(player, &a) || cardPattern->match(player, &k));
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->getPile("jinengPile").isEmpty();
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select, const Player *Self) const override
    {
        if (!Self->getPile("jinengPile").contains(to_select->getEffectiveId()))
            return false;

        switch (Self->getRoomObject()->getCurrentCardUseReason()) {
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
            QString pattern = Self->getRoomObject()->getCurrentCardUsePattern();
            Slash s(Card::SuitToBeDecided, -1);
            Jink j(Card::SuitToBeDecided, -1);
            Analeptic a(Card::SuitToBeDecided, -1);
            KnownBoth k(Card::SuitToBeDecided, -1);

            const CardPattern *cardPattern = Sanguosha->getPattern(pattern);

            return (cardPattern != nullptr && (to_select->getSuit() == Card::Heart && cardPattern->match(Self, &j))
                    || (to_select->getSuit() == Card::Spade && cardPattern->match(Self, &s)) || (to_select->getSuit() == Card::Diamond && cardPattern->match(Self, &a))
                    || (to_select->getSuit() == Card::Club && cardPattern->match(Self, &k)));
        }
        default:
            break;
        }

        return false;
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
    {
        if (originalCard == nullptr)
            return nullptr;

        Card *new_card = nullptr;

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
            new_card->setCanRecast(false);
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
        related_pile = "jinengPile";
    }

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
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
                if (aya && aya->isAlive() && room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge && !judge->ignore_judge) {
                    bool can = true;
                    foreach (int id, aya->getPile("jinengPile")) {
                        if (judge->card->getSuit() == room->getCard(id)->getSuit()) {
                            can = false;
                            break;
                        }
                    }
                    if (can)
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, aya, aya, nullptr, true);
                }
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    int getDistanceLimit(const Player *, const Card *card) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

class Youle : public TriggerSkill
{
public:
    Youle()
        : TriggerSkill("youle")
    {
        events << EventPhaseChanging << Damage << Damaged;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive()) {
                if (!damage.from->hasFlag("youle"))
                    damage.from->setFlags("youle");
            }
        }

        else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to && damage.to->isAlive()) {
                if (!damage.to->hasFlag("youle"))
                    damage.to->setFlags("youle");
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("youle"))
                        p->setFlags("-youle");
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && !change.player->tag.value("touhou-extra", false).toBool()) {
                QList<SkillInvokeDetail> d;
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p->hasFlag("youle"))
                        d << SkillInvokeDetail(this, p, p);
                }
                return d;
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (!p->isAllNude())
                targets << p;
        }
        if (targets.isEmpty())
            return false;

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, "youle", "@youle", true, true);
        if (target) {
            QStringList flags;
            if (!target->getCards("j").isEmpty())
                flags.append("j");
            if (!target->getCards("e").isEmpty())
                flags.append("e");
            if (!target->getCards("hs").isEmpty())
                flags.append("h");

            invoke->invoker->tag[objectName()] = QVariant::fromValue(target);
            QString choice = room->askForChoice(invoke->invoker, objectName(), flags.join("+"), data);
            room->notifySkillInvoked(invoke->invoker, objectName());
            invoke->targets << target;
            invoke->tag["youle"] = choice;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = invoke->targets.first();

        QString flag = invoke->tag.value("youle").toString();

        QList<const Card *> cards;
        //throw card
        if (flag == "e" || flag == "j") {
            foreach (const Card *c, target->getCards(flag)) {
                if (target->canDiscard(target, c->getId()))
                    cards << c;
            }

            if (!cards.isEmpty()) {
                DummyCard dummy;
                dummy.addSubcards(cards);
                room->throwCard(&dummy, target, target);
            }
        } else {
            int x = qMin(5, target->getHandcardNum()); // has not considered "jilei" yet.
            room->askForDiscard(target, objectName(), x, x, false, false, "@youle-discard");
        }

        //gain Extra Turn
        if (!room->getThread()->hasExtraTurn())
            target->gainAnExtraTurn();
        else {
            LogMessage log;
            log.type = "#ForbidExtraTurn";
            log.from = target;

            room->sendLog(log);
        }
        return false;
    }
};

class YaoliVS : public ViewAsSkill
{
public:
    YaoliVS(QString name, bool attach)
        : ViewAsSkill(name)
    {
        attached_lord_skill = attach;
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const override
    {
        if (Self->getRoomObject()->getCurrentCardUsePattern() == "@@" + objectName())
            return false;

        return selected.isEmpty() && Self->canDiscard(Self, to_select->getEffectiveId());
    }

    const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const override
    {
        if (Self->getRoomObject()->getCurrentCardUsePattern() == "@@" + objectName()) {
            QString cardName = Self->property("yaolitrick").toString();
            Card *c = Self->getRoomObject()->cloneCard(cardName);
            if (c != nullptr) {
                c->setSkillName("_yaolitrick");
                c->setCanRecast(false);
                if (c->isAvailable(Self))
                    return c;
                else
                    delete c;
            }
        } else {
            if (cards.length() == 1) {
                YaoliCard *card = new YaoliCard;
                card->addSubcards(cards);
                return card;
            }
        }
        return nullptr;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        QList<const Player *> sib = player->getAliveSiblings();
        sib << player;
        foreach (const Player *p, sib) {
            if (p->hasSkill("yaoli") && !p->hasFlag("yaoliselected"))
                return true;
        }

        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const override
    {
        return pattern == "@@" + objectName();
    }
};

// intentionally put this triggerskill before to provide Yaoli::el variable

class Yaoli : public TriggerSkill
{
public:
    static QStringList el;

    Yaoli()
        : TriggerSkill("yaoli")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Death << Debut << Revive << GeneralShown << EventPhaseEnd;
        view_as_skill = new YaoliVS("yaoli", false);
    }

    void record(TriggerEvent e, Room *room, QVariant &) const override
    {
        if (e == EventPhaseEnd)
            return;

        static QString attachName = "yaoliattach";
        QList<ServerPlayer *> sklts;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->hasSkill(this, true) && p->hasShownSkill(this))
                sklts << p;
        }

        if (sklts.length() >= 0) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(this)) {
                    if (p->hasSkill(attachName))
                        room->detachSkillFromPlayer(p, attachName, true);
                } else if (!p->hasSkill(attachName))
                    room->attachSkillToPlayer(p, attachName);
            }
        } else { // the case that sklts is empty
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(attachName, true))
                    room->detachSkillFromPlayer(p, attachName, true);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;

        if (triggerEvent != EventPhaseEnd)
            return r;

        ServerPlayer *p = data.value<ServerPlayer *>();
        if (p->getPhase() == Player::Play && p->hasFlag("yaolieffected")) {
            // Beware!! This should match Card::TypeBasic to Card::TypeEquip
            // Tag "yaolieffect0" is for no actual effect
            for (int i = 0; i <= 3; ++i) {
                QString tagName = "yaolieffect" + QString::number(i);
                if (p->tag.contains(tagName)) {
                    QVariantList owners = p->tag[tagName].toList();
                    foreach (const QVariant &_owner, owners) {
                        ServerPlayer *owner = _owner.value<ServerPlayer *>();
                        SkillInvokeDetail d(this, owner, p, nullptr, true, nullptr, false);
                        d.tag["yaolieffect"] = i;
                        r << d;
                    }
                }
            }
        }

        return r;
    }

    bool effect(TriggerEvent, Room *r, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        int i = invoke->tag["yaolieffect"].toInt();

        if (i != 0) {
            LogMessage l;
            l.type = "#yaolifinish";
            l.from = p;
            l.arg = el.at(i);
            r->sendLog(l);
        }

        QString tagName = "yaolieffect" + QString::number(i);
        QVariantList owners = p->tag[tagName].toList();
        QVariantList ownersNew;
        foreach (const QVariant &_owner, owners) {
            if (_owner.value<ServerPlayer *>() != invoke->owner)
                ownersNew << _owner;
        }
        if (ownersNew.isEmpty())
            p->tag.remove(tagName);
        else
            p->tag[tagName] = ownersNew;

        r->setPlayerFlag(invoke->owner, "-yaoliselected");

        return false;
    }
};

// Beware!! This should match Card::TypeBasic to Card::TypeEquip
QStringList Yaoli::el {"", "BasicCard", "TrickCard", "EquipCard"};

YaoliCard::YaoliCard()
{
    will_throw = true;
    m_skillName = "yaoli";
}

bool YaoliCard::targetFixed(const Player *Self) const
{
    bool flag = false;

    QList<const Player *> sib = Self->getAliveSiblings();
    sib << Self;

    foreach (const Player *p, sib) {
        if (p->hasSkill("yaoli") && !p->hasFlag("yaoliselected")) {
            if (flag)
                return false;
            flag = true;
        }
    }

    return true;
}

bool YaoliCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->hasSkill("yaoli") && !to_select->hasFlag("yaoliselected");
}

void YaoliCard::onUse(Room *room, const CardUseStruct &_use) const
{
    CardUseStruct use = _use;
    if (use.to.isEmpty()) {
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->hasSkill("yaoli") && !p->hasFlag("yaoliselected")) {
                use.to << p;
                break;
            }
        }
    }

    SkillCard::onUse(room, use);
}

void YaoliCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    room->setPlayerFlag(effect.to, "yaoliselected");
    effect.from->setFlags("yaolieffected");

    int yaoliTagTypeId = 0;

    if (effect.to->askForSkillInvoke("yaoli-draw", QVariant::fromValue(effect), "hahahahahaha:" + effect.from->objectName())) {
        effect.to->drawCards(1, "yaoli");
        if (effect.to->canDiscard(effect.to, "hes")) {
            const Card *discard = room->askForCard(effect.to, "..!", "@yaoli-discard", QVariant::fromValue<const Card *>(this));
            if (discard == nullptr) {
                QList<const Card *> allcards = effect.to->getCards("hes");
                QList<const Card *> cards;
                foreach (const Card *c, allcards) {
                    if (effect.to->canDiscard(effect.to, c->getEffectiveId()))
                        cards << c;
                }

                discard = cards.at(QRandomGenerator::global()->generate() % cards.length());
                room->throwCard(discard, effect.to);
            }

            const Card *thiscard = room->getCard(subcards.first());
            discard = room->getCard(discard->getEffectiveId());
            if (thiscard->getTypeId() == discard->getTypeId()) {
                LogMessage l;
                l.type = "#yaolistart";
                l.from = effect.from;
                l.to << effect.to;
                l.arg = Yaoli::el[static_cast<int>(thiscard->getTypeId())];
                room->sendLog(l);

                yaoliTagTypeId = static_cast<int>(thiscard->getTypeId());
            }
        }
    }

    QString tagName = "yaolieffect" + QString::number(yaoliTagTypeId);

    QVariantList yaoliTagList;
    if (effect.from->tag.contains(tagName))
        yaoliTagList = effect.from->tag.value(tagName).toList();

    effect.from->tag[tagName] = (yaoliTagList << QVariant::fromValue(effect.to));
}

class YaoliBasic : public TriggerSkill
{
public:
    YaoliBasic()
        : TriggerSkill("yaolibasic")
    {
        events << CardUsed;
        global = true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeBasic && use.from != nullptr && use.from->isAlive() && use.from->tag.contains("yaolieffect1"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, nullptr, true, nullptr, false);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();

        LogMessage l;
        l.type = "#yaolibasic";
        l.from = use.from;
        l.arg = use.card->objectName();

        room->sendLog(l);

        if (!use.card->isKindOf("Slash") || use.card->isKindOf("LightSlash") || use.card->isKindOf("PowerSlash"))
            use.card->setFlags("mopao");
        else
            use.card->setFlags("mopao2");

        return false;
    }
};

class YaoliTrick : public TriggerSkill
{
public:
    YaoliTrick()
        : TriggerSkill("yaolitrick")
    {
        events << CardFinished;
        global = true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isNDTrick() && use.from != nullptr && use.from->isAlive() && use.from->tag.contains("yaolieffect2") && use.card->getSkillName() != "yaolitrick") {
            QString cardName = use.card->getClassName();

            Card *c = use.from->getRoomObject()->cloneCard(cardName);
            if (c != nullptr) {
                DELETE_OVER_SCOPE(Card, c)
                c->setSkillName("_yaolitrick");
                c->setCanRecast(false);
                if (c->isAvailable(use.from))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, nullptr, true, nullptr, false);
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();

        room->setPlayerProperty(use.from, "yaolitrick", use.card->getClassName());
        room->setPlayerFlag(use.from, "Global_InstanceUse_Failed");

        QString pattern = "@@yaoliattach";
        if (use.from->hasSkill("yaoli"))
            pattern = "@@yaoli";

        room->askForUseCard(use.from, pattern, "@yaolitrick:::" + use.card->objectName());

        return false;
    }
};

class YaoliEquip : public TriggerSkill
{
public:
    YaoliEquip()
        : TriggerSkill("yaoliequip")
    {
        events << CardUsed;
        global = true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeEquip && use.from != nullptr && use.from->isAlive() && use.from->tag.contains("yaolieffect3"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, nullptr, true, nullptr, false);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        const EquipCard *equip = qobject_cast<const EquipCard *>(use.card->getRealCard());
        if (equip == nullptr) // unicorn
            return false;

        int x = 1;
        if (equip->location() == EquipCard::WeaponLocation) {
            const Weapon *weapon = qobject_cast<const Weapon *>(equip);
            if (weapon == nullptr) {
                // unicorn
            } else
                x = weapon->getRange();
        }

        if (use.from->askForSkillInvoke("yaoli-equip", data, "mowmowmowmow:::" + QString::number(x))) {
            bool discard = false;
            do {
                QList<ServerPlayer *> ts;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (use.from->canDiscard(p, "ej"))
                        ts << p;
                }

                if (ts.isEmpty())
                    break;

                ServerPlayer *t = room->askForPlayerChosen(
                    use.from, ts, "yaoli", QStringLiteral("@yaoliequip") + (discard ? QStringLiteral("") : QStringLiteral("i")) + ":::" + QString::number(x), true, false);

                if (t != nullptr) {
                    int id = room->askForCardChosen(use.from, t, "ej", "yaoliequip", false, Card::MethodDiscard);
                    if (id != -1) {
                        if (!discard) {
                            LogMessage l;
                            l.type = "#yaoliequipi";
                            l.from = use.from;

                            room->sendLog(l);
                            room->notifySkillInvoked(use.from, "yaoli");
                        }
                        discard = true;
                        ServerPlayer *thrower = use.from;
                        if (room->getCardOwner(id) == use.from && room->getCardPlace(id) != Player::PlaceJudge)
                            thrower = nullptr;
                        room->throwCard(id, t, thrower);
                    } else
                        break;
                } else
                    break;
            } while (--x);

            if (!discard) {
                LogMessage l;
                l.type = "#yaoliequip0";
                l.from = use.from;

                room->notifySkillInvoked(use.from, "yaoli");
                room->drawCards(use.from, x, "yaoli");
            }
        }
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
    kaguya_ndj->addSkill(new Wanggou);

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
    related_skills.insert("liexi", "#liexi");

    General *renko_ndj = new General(this, "renko_ndj", "wai", 4);
    renko_ndj->addSkill(new Liangzi);
    renko_ndj->addSkill(new Kexue);
    renko_ndj->addSkill(new KexueEffect);
    related_skills.insert("kexue", "#kexue-effect");

    General *sanae_ndj = new General(this, "sanae_ndj", "fsl", 4);
    sanae_ndj->addSkill(new Xiubu);
    sanae_ndj->addSkill(new XiubuTargetMod);
    related_skills.insert("xiubu", "#xiubu-mod");

    General *aya_ndj = new General(this, "aya_ndj", "fsl", 3);
    aya_ndj->addSkill(new Jineng);
    aya_ndj->addSkill(new JinengTargetMod);
    aya_ndj->addSkill(new Kuaibao);
    related_skills.insert("jineng", "#jinengmod");

    General *tenshi_ndj = new General(this, "tenshi_ndj", "zhan", 4);
    tenshi_ndj->addSkill(new Youle);
    addMetaObject<HunpoCard>();

    General *eirin = new General(this, "eirin_ndj", "yyc");
    eirin->addSkill(new Yaoli);
    addMetaObject<YaoliCard>();

    skills << new YaoliVS("yaoliattach", true) << new YaoliBasic << new YaoliEquip << new YaoliTrick;
}

ADD_PACKAGE(THNDJ)
