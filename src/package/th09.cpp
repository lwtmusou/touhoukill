#include "th09.h"

#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"

#include <QCommandLinkButton>

class ZuiyueVS : public ZeroCardViewAsSkill
{
public:
    ZuiyueVS()
        : ZeroCardViewAsSkill("zuiyue")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->hasFlag("zuiyue") && Analeptic::IsAvailable(player);
    }

    virtual const Card *viewAs() const
    {
        Analeptic *ana = new Analeptic(Card::NoSuit, 0);
        ana->setSkillName(objectName());
        return ana;
    }
};

class Zuiyue : public TriggerSkill
{
public:
    Zuiyue()
        : TriggerSkill("zuiyue")
    {
        events << PreCardUsed << EventPhaseChanging;
        view_as_skill = new ZuiyueVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("BasicCard") && !use.card->isKindOf("SkillCard") && use.from && use.from->hasSkill(this) && use.from->getPhase() == Player::Play)
                room->setPlayerFlag(use.from, "zuiyue");
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                room->setPlayerFlag(change.player, "-zuiyue");
        }
    }
};

class Doujiu : public TriggerSkill
{
public:
    Doujiu()
        : TriggerSkill("doujiu")
    {
        events << CardUsed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Peach") && !use.card->isKindOf("Analeptic"))
            return QList<SkillInvokeDetail>();
        if (use.from->getPhase() != Player::Play || use.from->isKongcheng() || !use.from->isAlive())
            return QList<SkillInvokeDetail>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *suika, room->findPlayersBySkillName(objectName())) {
            if (use.from != suika)
                d << SkillInvokeDetail(this, suika, suika, NULL, false, use.from);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QVariant _data = QVariant::fromValue(invoke->preferredTarget);
        invoke->invoker->tag["doujiu_target"] = _data;
        return room->askForSkillInvoke(invoke->invoker, objectName(), _data);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        invoke->invoker->drawCards(1);
        CardUseStruct use = data.value<CardUseStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        if (!invoke->invoker->isKongcheng() && invoke->invoker->pindian(invoke->targets.first(), objectName())) {
            if (invoke->invoker->isWounded()) {
                RecoverStruct recover;
                recover.recover = 1;
                room->recover(invoke->invoker, recover);
            }
            use.nullified_list << "_ALL_TARGETS";
            data = QVariant::fromValue(use);
            room->setPlayerFlag(invoke->targets.first(), "Global_PlayPhaseTerminated");
        }
        return false;
    }
};

//related Peach::targetFilter
class Yanhui : public TriggerSkill
{
public:
    Yanhui()
        : TriggerSkill("yanhui$")
    {
        events << PreCardUsed;
    }

    //just for broadcasting skillInvoked
    void record(TriggerEvent, Room *room, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, use.to) {
            if (p->hasLordSkill("yanhui") && p != use.from) {
                if ((use.card->isKindOf("Analeptic") && p->hasFlag("Global_Dying")) || (use.card->isKindOf("Peach") && use.m_reason == CardUseStruct::CARD_USE_REASON_PLAY)) {
                    QList<ServerPlayer *> logto;
                    logto << p;
                    room->touhouLogmessage("#InvokeOthersSkill", use.from, objectName(), logto);
                    room->notifySkillInvoked(p, objectName());
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &) const
    {
        return QList<SkillInvokeDetail>();
    }
};

class Shenpan : public TriggerSkill
{
public:
    Shenpan()
        : TriggerSkill("shenpan")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player && player->isAlive() && player->hasSkill(this) && player->getPhase() == Player::Draw)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(), "@shenpan-select", true, true);
        if (target != NULL)
            invoke->targets << target;
        return target != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->damage(DamageStruct(objectName(), invoke->invoker, invoke->targets.first(), 1, DamageStruct::Thunder));
        if (invoke->targets.first()->isAlive() && invoke->targets.first()->getHandcardNum() > invoke->targets.first()->getHp())
            room->drawCards(invoke->invoker, 1);
        return true;
    }
};

class Huiwu : public TriggerSkill
{
public:
    Huiwu()
        : TriggerSkill("huiwu")
    {
        events << TargetConfirmed;
        show_type = "static";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") || (use.card->isNDTrick())) {
            if (use.from == NULL || use.from->isDead())
                return QList<SkillInvokeDetail>();

            QList<SkillInvokeDetail> details;
            foreach (ServerPlayer *to, use.to) {
                if (to->hasSkill(objectName()) && to != use.from)
                    details << SkillInvokeDetail(this, to, use.from);
            }
            return details;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.from->tag["huiwu"] = QVariant::fromValue(invoke->owner);
        room->setTag("huiwu_use", data);
        QString prompt = "target:" + invoke->owner->objectName() + ":" + use.card->objectName();
        if (use.from->askForSkillInvoke(objectName(), QVariant::fromValue(invoke->owner))) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, use.from->objectName(), invoke->owner->objectName());

            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(invoke->owner, objectName());
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = use.from;
            log.to << invoke->owner;
            log.arg = objectName();
            room->sendLog(log);

            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.nullified_list << invoke->owner->objectName();
        data = QVariant::fromValue(use);
        invoke->owner->drawCards(1);
        return false;
    }
};

class Huazhong : public TriggerSkill
{
public:
    Huazhong()
        : TriggerSkill("huazhong$")
    {
        events << Damage << FinishJudge;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (event == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive() && damage.from->getKingdom() == "zhan") {
                foreach (ServerPlayer *p, room->getOtherPlayers(damage.from)) {
                    if (p->hasLordSkill(objectName())) {
                        for (int i = 0; i < damage.damage; i++)
                            d << SkillInvokeDetail(this, p, damage.from);
                    }
                }
            }
        } else if (event == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName() && judge->isGood()) {
                ServerPlayer *lord = judge->relative_player;
                if (lord != NULL && lord->isAlive())
                    d << SkillInvokeDetail(this, lord, lord, NULL, true);
            }
        }
        return d;
    }

    bool cost(TriggerEvent event, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (event == Damage) {
            invoke->invoker->tag["huazhong-target"] = QVariant::fromValue(invoke->owner);
            return invoke->invoker->askForSkillInvoke(objectName(), QVariant::fromValue(invoke->owner));
        } else
            return true;
        return false;
    }

    bool effect(TriggerEvent event, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (event == Damage) {
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(invoke->owner, objectName());
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = invoke->invoker;
            log.to << invoke->owner;
            log.arg = objectName();
            room->sendLog(log);

            JudgeStruct judge;
            judge.pattern = ".|red";
            judge.who = invoke->invoker;
            judge.reason = objectName();
            judge.good = true;
            judge.relative_player = invoke->owner;
            room->judge(judge);

            invoke->invoker->tag.remove("huazhong");
        } else if (event == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            invoke->owner->obtainCard(judge->card);
        }
        return false;
    }
};

class Mingtu : public TriggerSkill
{
public:
    Mingtu()
        : TriggerSkill("mingtu")
    {
        frequency = Frequent;
        events << EnterDying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &) const
    {
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *komachi, room->findPlayersBySkillName(objectName()))
            d << SkillInvokeDetail(this, komachi, komachi);
        return d;
    }

    //defualt cost

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1);
        return false;
    }
};

class Boming : public TriggerSkill
{
public:
    Boming()
        : TriggerSkill("boming")
    {
        frequency = Compulsory;
        events << DamageCaused;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user || !damage.from || damage.from == damage.to || !damage.from->hasSkill(this))
            return QList<SkillInvokeDetail>();
        if (damage.card && damage.card->isKindOf("Slash") && damage.to->getHp() <= damage.to->dyingThreshold())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        QList<ServerPlayer *> logto;
        logto << damage.to;
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName(), logto);
        room->notifySkillInvoked(invoke->invoker, objectName());
        damage.damage = damage.damage + 2;
        data = QVariant::fromValue(damage);
        return false;
    }
};

class Weiya : public TriggerSkill
{
public:
    Weiya()
        : TriggerSkill("weiya")
    {
        frequency = Compulsory;
        events << CardUsed << CardResponded;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *current = room->getCurrent();
        if (!current || !current->hasSkill(objectName()) || !current->isAlive())
            return QList<SkillInvokeDetail>();

        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from != current && (use.card->isKindOf("Nullification") || use.card->isKindOf("BasicCard"))) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current, NULL, true);
            }
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_from == current || !resp.m_card->isKindOf("BasicCard") || resp.m_isRetrial || resp.m_isProvision)
                return QList<SkillInvokeDetail>();
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QString weiya_pattern;
        ServerPlayer *current = room->getCurrent();
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Nullification")) {
                room->notifySkillInvoked(current, objectName());
                room->touhouLogmessage("#weiya_ask", use.from, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                if (room->askForCard(use.from, "nullification", "@weiya:nullification", data, Card::MethodDiscard))
                    return false;
                room->touhouLogmessage("#weiya", use.from, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                room->setPlayerFlag(use.from, "nullifiationNul");
            } else if (use.card->isKindOf("BasicCard")) {
                weiya_pattern = use.card->objectName();
                if (use.card->isKindOf("Slash"))
                    weiya_pattern = "slash";
                room->notifySkillInvoked(current, objectName());
                room->touhouLogmessage("#weiya_ask", use.from, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                if (room->askForCard(use.from, weiya_pattern, "@weiya:" + use.card->objectName(), data, Card::MethodDiscard))
                    return false;
                room->touhouLogmessage("#weiya", use.from, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                use.nullified_list << "_ALL_TARGETS";
                data = QVariant::fromValue(use);
            }
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            const Card *card_star = data.value<CardResponseStruct>().m_card;
            weiya_pattern = card_star->objectName();
            if (card_star->isKindOf("Slash"))
                weiya_pattern = "slash";
            room->notifySkillInvoked(current, objectName());
            room->touhouLogmessage("#weiya_ask", resp.m_from, objectName(), QList<ServerPlayer *>(), card_star->objectName());
            if (room->askForCard(resp.m_from, weiya_pattern, "@weiya:" + card_star->objectName(), data, Card::MethodDiscard))
                return false;
            room->touhouLogmessage("#weiya", resp.m_from, objectName(), QList<ServerPlayer *>(), card_star->objectName());
            resp.m_isNullified = true;
            data = QVariant::fromValue(resp);
        }
        return false;
    }
};

class Judu : public TriggerSkill
{
public:
    Judu()
        : TriggerSkill("judu")
    {
        events << Damage;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->hasSkill(this) && damage.to->isAlive() && damage.to != damage.from)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QVariant _data = QVariant::fromValue(damage.to);
        return invoke->invoker->askForSkillInvoke(objectName(), _data);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), damage.to->objectName());
        JudgeStruct judge;
        judge.reason = objectName();
        judge.who = invoke->invoker;
        judge.good = true;
        judge.pattern = ".|black|2~9";

        room->judge(judge);

        if (judge.isGood()) {
            room->loseHp(damage.to);
        }
        return false;
    }
};

class Henyi : public TriggerSkill
{
public:
    Henyi()
        : TriggerSkill("henyi")
    {
        events << EventPhaseEnd << DamageCaused << Damaged << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == Damaged) { //DamageDone?
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.to->hasFlag("henyi"))
                damage.to->setFlags("henyi");
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from == Player::NotActive) { //Player::Play
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("henyi"))
                        p->setFlags("-henyi");
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Play) {
                QList<SkillInvokeDetail> d;
                foreach (ServerPlayer *src, room->getAllPlayers()) {
                    if (src->hasFlag("henyi") && src->hasSkill(this)) {
                        ArcheryAttack *card = new ArcheryAttack(Card::NoSuit, 0);
                        card->deleteLater();
                        if (card->isAvailable(src) && !src->isCardLimited(card, Card::MethodUse))
                            d << SkillInvokeDetail(this, src, src);
                    }
                }
                return d;
            }
        } else if (triggerEvent == DamageCaused) { //need not check weather damage.from has this skill
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->getSkillName() == "henyi" && damage.to->isCurrent() && damage.from && damage.from->hasSkill(this)) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == EventPhaseEnd)
            return invoke->invoker->askForSkillInvoke(objectName(), data);
        return true;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == EventPhaseEnd) {
            ArcheryAttack *card = new ArcheryAttack(Card::NoSuit, 0);
            card->setSkillName("_henyi");
            CardUseStruct carduse;
            carduse.card = card;
            carduse.from = invoke->invoker;
            room->useCard(carduse);

        } else if (triggerEvent == DamageCaused) { //need not check weather damage.from has this skill
            DamageStruct damage = data.value<DamageStruct>();
            room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
            room->notifySkillInvoked(invoke->invoker, objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, damage.from->objectName(), damage.to->objectName());
            RecoverStruct recov;
            room->recover(damage.from, recov);
        }
        return false;
    }
};

ToupaiCard::ToupaiCard()
{
    //will_throw = false;
    //handling_method = Card::MethodNone;
}

bool ToupaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < 2 && to_select != Self && !to_select->isKongcheng();
}

void ToupaiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *p, targets) {
        if (!p->isKongcheng()) {
            room->showAllCards(p, source);
            room->getThread()->delay(1000);
            room->clearAG(source);

            QList<int> ids;
            QList<int> able;
            QList<int> disable;
            foreach (const Card *c, p->getCards("hs")) {
                int id = c->getEffectiveId();
                ids << id;
                if (c->isKindOf("BasicCard") && source->canDiscard(p, id))
                    able << id;
                else
                    disable << id;
            }
            if (!able.isEmpty()) {
                room->fillAG(ids, source, disable);
                int id = room->askForAG(source, able, true, objectName());
                room->clearAG(source);
                if (id > -1)
                    room->throwCard(id, p, source);
            }
        }
    }
}

class ToupaiVS : public ZeroCardViewAsSkill
{
public:
    ToupaiVS()
        : ZeroCardViewAsSkill("toupai")
    {
        response_pattern = "@@toupai";
    }

    virtual const Card *viewAs() const
    {
        ToupaiCard *card = new ToupaiCard;
        return card;
    }
};

class Toupai : public PhaseChangeSkill
{
public:
    Toupai()
        : PhaseChangeSkill("toupai")
    {
        view_as_skill = new ToupaiVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *aya = data.value<ServerPlayer *>();
        if (aya->getPhase() == Player::Draw && aya->hasSkill(this)) {
            foreach (ServerPlayer *p, room->getOtherPlayers(aya)) {
                if (!p->isKongcheng())
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, aya, aya);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        return room->askForUseCard(invoke->invoker, "@@toupai", "@toupai");
    }

    virtual bool onPhaseChange(ServerPlayer *) const
    {
        return true;
    }
};

class Qucai : public TriggerSkill
{
public:
    Qucai()
        : TriggerSkill("qucai")
    {
        events << CardUsed << CardResponded << CardsMoveOneTime;
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *player = room->getCurrent();
        if (!player || player->isDead() || !player->hasSkill(this))
            return QList<SkillInvokeDetail>();
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isRed() && use.from != player)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_from && player != response.m_from && response.m_isUse && response.m_card && response.m_card->isRed())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
            if (from && player != from && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                foreach (int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->isRed()) {
                        Player::Place from_place = move.from_places.at(move.card_ids.indexOf(id));
                        if (from_place == Player::PlaceHand || from_place == Player::PlaceEquip)
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1);
        return false;
    }
};

class Feixiang : public TriggerSkill
{
public:
    Feixiang()
        : TriggerSkill("feixiang")
    {
        events << AskForRetrial;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &) const
    {
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *tenshi, room->findPlayersBySkillName(objectName())) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (!p->isNude()) {
                    d << SkillInvokeDetail(this, tenshi, tenshi);
                    break;
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (!p->isNude())
                targets << p;
        }

        QString prompt = "@feixiang-playerchosen:" + judge->who->objectName() + ":" + judge->reason;
        invoke->invoker->tag["feixiang_judge"] = data;
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), prompt, true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        ServerPlayer *target = invoke->targets.first();
        int card_id = room->askForCardChosen(invoke->invoker, target, "hes", objectName());
        room->showCard(target, card_id);
        Card *card = Sanguosha->getCard(card_id);
        if (!target->isCardLimited(card, Card::MethodResponse))
            room->retrial(card, target, judge, objectName());
        return false;
    }
};

class Dizhen : public TriggerSkill
{
public:
    Dizhen()
        : TriggerSkill("dizhen")
    {
        events << FinishJudge;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (!judge->who || !judge->who->isAlive())
            return QList<SkillInvokeDetail>();
        ServerPlayer *dying = room->getCurrentDyingPlayer();
        if (dying != NULL && dying->isAlive())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *tenshi, room->findPlayersBySkillName(objectName())) {
            if (judge->card->isRed() && judge->who->getHp() > 0)
                d << SkillInvokeDetail(this, tenshi, tenshi, NULL, false, judge->who);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        invoke->invoker->tag["dizhen_judge"] = data;
        QString prompt = "target:" + judge->who->objectName() + ":" + judge->reason;
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        room->damage(DamageStruct(objectName(), invoke->invoker, invoke->targets.first(), 1, DamageStruct::Normal));
        return false;
    }
};

TianrenCard::TianrenCard()
{
}

bool TianrenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

const Card *TianrenCard::validate(CardUseStruct &cardUse) const
{
    cardUse.m_isOwnerUse = false;
    ServerPlayer *lord = cardUse.from;
    QList<ServerPlayer *> targets = cardUse.to;
    Room *room = lord->getRoom();

    room->notifySkillInvoked(lord, "tianren");
    LogMessage log;
    log.from = lord;
    log.to = targets;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    const Card *slash = NULL;
    QList<ServerPlayer *> lieges = room->getLieges("zhan", lord);
    foreach (ServerPlayer *target, targets)
        target->setFlags("tianrenTarget");
    foreach (ServerPlayer *liege, lieges) {
        try {
            slash = room->askForCard(liege, "slash", "@tianren-slash:" + lord->objectName(), QVariant(), Card::MethodResponse, lord, false, QString(), true);
        } catch (TriggerEvent triggerEvent) {
            if (triggerEvent == TurnBroken) {
                foreach (ServerPlayer *target, targets)
                    target->setFlags("-tianrenTarget");
            }
            throw triggerEvent;
        }

        if (slash) {
            room->setCardFlag(slash, "CardProvider_" + liege->objectName());
            foreach (ServerPlayer *target, targets)
                target->setFlags("-tianrenTarget");

            return slash;
        }
    }
    foreach (ServerPlayer *target, targets)
        target->setFlags("-tianrenTarget");
    room->setPlayerFlag(lord, "Global_tianrenFailed");
    return NULL;
}

class TianrenVS : public ZeroCardViewAsSkill
{
public:
    TianrenVS()
        : ZeroCardViewAsSkill("tianren$")
    {
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    static bool hasZhanGenerals(const Player *player)
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->getKingdom() == "zhan")
                return true;
        }
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return hasZhanGenerals(player) && (matchAvaliablePattern("slash", pattern)) && (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            && (!player->hasFlag("Global_tianrenFailed")) && !player->isCurrent();
    }

    virtual const Card *viewAs() const
    {
        return new TianrenCard;
    }
};

class Tianren : public TriggerSkill
{
public:
    Tianren()
        : TriggerSkill("tianren$")
    {
        events << CardAsked;
        view_as_skill = new TianrenVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardAskedStruct s = data.value<CardAskedStruct>();
        ServerPlayer *player = s.player;
        if (!player->hasLordSkill(objectName()) || player->isCurrent())
            return QList<SkillInvokeDetail>();

        QString pattern = s.pattern;
        QString prompt = s.prompt;

        bool can = (matchAvaliablePattern("slash", pattern) || matchAvaliablePattern("jink", pattern));
        if ((!can) || prompt.startsWith("@tianren"))
            return QList<SkillInvokeDetail>();

        if (matchAvaliablePattern("slash", pattern))
            pattern = "slash";
        else
            pattern = "jink";
        Card *dummy = Sanguosha->cloneCard(pattern);
        DELETE_OVER_SCOPE(Card, dummy)
        if (player->isCardLimited(dummy, s.method))
            return QList<SkillInvokeDetail>();

        if (!room->getLieges("zhan", player).isEmpty())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardAskedStruct s = data.value<CardAskedStruct>();
        QString pattern = s.pattern;
        //for ai  to add global flag
        //slashsource or jinksource
        if (matchAvaliablePattern("slash", pattern))
            room->setTag("tianren_slash", true);
        else
            room->setTag("tianren_slash", false);
        return invoke->invoker->askForSkillInvoke(objectName(), data);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QString pattern = data.value<CardAskedStruct>().pattern;
        if (matchAvaliablePattern("slash", pattern))
            pattern = "slash";
        else
            pattern = "jink";
        QVariant tohelp = QVariant::fromValue((ServerPlayer *)invoke->invoker);
        foreach (ServerPlayer *liege, room->getLieges("zhan", invoke->invoker)) {
            const Card *resp = room->askForCard(liege, pattern, "@tianren-" + pattern + ":" + invoke->invoker->objectName(), tohelp, Card::MethodResponse, invoke->invoker, false,
                                                QString(), true);
            if (resp) {
                room->provide(resp, liege);
                return true;
            }
        }
        return false;
    }
};

class Jingdian : public TriggerSkill
{
public:
    Jingdian()
        : TriggerSkill("jingdian")
    {
        frequency = Compulsory;
        events << DamageInflicted;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature == DamageStruct::Thunder && damage.to->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->touhouLogmessage("#jingdian", invoke->invoker, objectName(), QList<ServerPlayer *>(), QString::number(damage.damage));
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->drawCards(invoke->invoker, 3 * damage.damage, objectName());
        return true;
    }
};

class Leiyun : public OneCardViewAsSkill
{
public:
    Leiyun()
        : OneCardViewAsSkill("leiyun")
    {
        response_or_use = true;
        filter_pattern = ".|spade,heart|.|hand";
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return matchAvaliablePattern("lightning", pattern);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            Lightning *card = new Lightning(originalCard->getSuit(), originalCard->getNumber());
            card->addSubcard(originalCard);
            card->setSkillName("leiyun");
            return card;
        } else
            return NULL;
    }
};

class Kuaizhao : public DrawCardsSkill
{
public:
    Kuaizhao()
        : DrawCardsSkill("kuaizhao")
    {
    }

    int getDrawNum(const DrawNCardsStruct &draw) const
    {
        return draw.n;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DrawNCardsStruct dc = data.value<DrawNCardsStruct>();
        if (dc.n <= 0 || !dc.player->hasSkill(this))
            return QList<SkillInvokeDetail>();

        foreach (ServerPlayer *p, room->getOtherPlayers(dc.player)) {
            if (!p->isKongcheng() && dc.player->inMyAttackRange(p))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dc.player, dc.player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        QList<ServerPlayer *> others = room->getOtherPlayers(player);
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->isKongcheng() || !player->inMyAttackRange(p))
                others.removeOne(p);
        }

        ServerPlayer *target = room->askForPlayerChosen(player, others, objectName(), "@kuaizhao-select_one", true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        invoke->invoker->tag["kuaizhao_target"] = QVariant::fromValue(invoke->targets.first());

        DrawNCardsStruct s = data.value<DrawNCardsStruct>();
        s.n = s.n - 1;
        data = QVariant::fromValue(s);
        return false;
    }
};

class KuaizhaoEffect : public TriggerSkill
{
public:
    KuaizhaoEffect()
        : TriggerSkill("#kuaizhao")
    {
        events << AfterDrawNCards;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DrawNCardsStruct dc = data.value<DrawNCardsStruct>();
        ServerPlayer *target = dc.player->tag["kuaizhao_target"].value<ServerPlayer *>();
        if (target)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dc.player, dc.player, NULL, true, target);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->tag.remove("kuaizhao_target");

        ServerPlayer *target = invoke->targets.first();
        room->showAllCards(target);
        room->getThread()->delay(1000);
        room->clearAG();

        int num = 0;
        foreach (const Card *c, target->getCards("hs")) {
            if (c->isKindOf("BasicCard"))
                num++;
        }
        room->drawCards(invoke->invoker, qMin(2, num));
        return false;
    }
};

class Duanjiao : public AttackRangeSkill
{
public:
    Duanjiao()
        : AttackRangeSkill("duanjiao")
    {
    }

    virtual int getFixed(const Player *target, bool) const
    {
        if (target->hasSkill("duanjiao"))
            return 3;
        else
            return -1;
    }
};

class Nengwu : public TriggerSkill
{
public:
    Nengwu()
        : TriggerSkill("nengwu")
    {
        events << HpRecover << CardsMoveOneTime;
    }

    static QList<ServerPlayer *> nengwuTargets(ServerPlayer *player, bool draw)
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, player->getRoom()->getOtherPlayers(player)) {
            if (player->inMyAttackRange(p) && draw) {
                targets << p;
            } else if (player->inMyAttackRange(p) && !draw && player->canDiscard(p, "hs")) {
                targets << p;
            }
        }
        return targets;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == HpRecover) {
            RecoverStruct r = data.value<RecoverStruct>();
            if (r.to->hasSkill(this) && r.to->getPhase() != Player::Draw && !nengwuTargets(r.to, true).isEmpty())
                d << SkillInvokeDetail(this, r.to, r.to);
        } else if (triggerEvent == CardsMoveOneTime) {
            if (room->getTag("FirstRound").toBool())
                return QList<SkillInvokeDetail>();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *playerTo = qobject_cast<ServerPlayer *>(move.to);
            if (playerTo != NULL && playerTo->isAlive() && playerTo->hasSkill(this) && move.to_place == Player::PlaceHand && playerTo->getPhase() != Player::Draw
                && !nengwuTargets(playerTo, true).isEmpty()) {
                d << SkillInvokeDetail(this, playerTo, playerTo);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<ServerPlayer *> targets = nengwuTargets(invoke->invoker, true);

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, "nengwudraw", "@nengwu-draw", true, true);
        if (target) {
            room->notifySkillInvoked(invoke->invoker, objectName());
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->targets.first()->drawCards(1);
        return false;
    }
};

class Nengwu2 : public TriggerSkill
{
public:
    Nengwu2()
        : TriggerSkill("#nengwu2")
    {
        events << CardsMoveOneTime << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == Damaged) {
            ServerPlayer *player = data.value<DamageStruct>().to;
            if (player->hasSkill("nengwu") && player->isAlive() && player->getPhase() != Player::Play && !Nengwu::nengwuTargets(player, false).isEmpty())
                d << SkillInvokeDetail(this, player, player);
        } else if (triggerEvent == CardsMoveOneTime) {
            if (room->getTag("FirstRound").toBool())
                return QList<SkillInvokeDetail>();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *playerFrom = qobject_cast<ServerPlayer *>(move.from);
            if (playerFrom != NULL && playerFrom->isAlive() && playerFrom->hasSkill("nengwu") && move.from_places.contains(Player::PlaceHand)
                && playerFrom->getPhase() != Player::Play && !Nengwu::nengwuTargets(playerFrom, false).isEmpty()) {
                d << SkillInvokeDetail(this, playerFrom, playerFrom);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<ServerPlayer *> targets = Nengwu::nengwuTargets(invoke->invoker, false);

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, "nengwudiscard", "@nengwu-discard", true, true);
        if (target) {
            room->notifySkillInvoked(invoke->invoker, "nengwu");
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->throwCard(room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hs", objectName(), false, Card::MethodDiscard), invoke->targets.first(), invoke->invoker);
        return false;
    }
};

class Xiwang : public TriggerSkill
{
public:
    Xiwang()
        : TriggerSkill("xiwang$")
    {
        events << CardUsed << CardResponded;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *current = room->getCurrent();
        if (!current || current->getKingdom() != "zhan" || current->isDead())
            return QList<SkillInvokeDetail>();
        if (current->isKongcheng())
            return QList<SkillInvokeDetail>();

        ServerPlayer *player = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getHandlingMethod() == Card::MethodUse && use.card->getSuit() == Card::Heart) //caution the case using a skillcard
                player = use.from;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct s = data.value<CardResponseStruct>();
            if (!s.m_isProvision && s.m_card->getSuit() == Card::Heart)
                player = s.m_from;
        }

        if (player == NULL || player == current)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->getOtherPlayers(current)) {
            if (p->hasLordSkill(objectName()))
                d << SkillInvokeDetail(this, p, current);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *current = invoke->invoker;
        if (current->isKongcheng())
            return false;
        ServerPlayer *lord = invoke->owner;
        current->tag["xiwang_target"] = QVariant::fromValue(lord); // for ai
        const Card *card = room->askForCard(current, ".|.|.|hand", "@xiwang:" + lord->objectName(), data, Card::MethodNone);
        if (card) {
            room->notifySkillInvoked(lord, objectName());
            QList<ServerPlayer *> logto;
            logto << lord;
            room->touhouLogmessage("#InvokeOthersSkill", current, objectName(), logto);

            current->tag["xiwang_id"] = QVariant::fromValue(card->getEffectiveId());
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *lord = invoke->owner;
        int card_id = invoke->invoker->tag["xiwang_id"].toInt();
        invoke->invoker->tag.remove("xiwang_id");
        room->obtainCard(lord, card_id, room->getCardPlace(card_id) != Player::PlaceHand);
        return false;
    }
};

NianliDialog *NianliDialog::getInstance(const QString &object)
{
    static NianliDialog *instance;
    if (instance == NULL || instance->objectName() != object) {
        instance = new NianliDialog(object);
    }
    return instance;
}

NianliDialog::NianliDialog(const QString &object)
    : object_name(object)
{
    setObjectName(object);
    setWindowTitle(Sanguosha->translate(object));
    group = new QButtonGroup(this);

    layout = new QVBoxLayout;
    setLayout(layout);

    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectCard(QAbstractButton *)));
}

void NianliDialog::popup()
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
        emit onButtonClick();
        return;
    }

    foreach (QAbstractButton *button, group->buttons()) {
        layout->removeWidget(button);
        group->removeButton(button);
        delete button;
    }

    QStringList card_names;
    card_names << "slash"
               << "snatch";

    foreach (QString card_name, card_names) {
        QCommandLinkButton *button = new QCommandLinkButton;
        button->setText(Sanguosha->translate(card_name));
        button->setObjectName(card_name);
        group->addButton(button);

        bool can = true;
        Card *c = Sanguosha->cloneCard(card_name);
        c->setSkillName("nianli");
        if (Self->isCardLimited(c, Card::MethodUse) || !c->isAvailable(Self))
            can = false;

        button->setEnabled(can);
        button->setToolTip(c->getDescription());
        layout->addWidget(button);
        delete c;

        /*if (!map.contains(card_name)) {
            Card *c = Sanguosha->cloneCard(card_name);
            c->setParent(this);
            map.insert(card_name, c);
        }*/
    }

    Self->tag.remove(object_name);
    exec();
}

void NianliDialog::selectCard(QAbstractButton *button)
{
    //const Card *card = map.value(button->objectName());
    Self->tag[object_name] = QVariant::fromValue(button->objectName());

    emit onButtonClick();
    accept();
}

NianliCard::NianliCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool NianliCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Card *new_card = Sanguosha->cloneCard(user_string, Card::SuitToBeDecided, 0);
    DELETE_OVER_SCOPE(Card, new_card)
    new_card->setSkillName("nianli");
    if (new_card->targetFixed())
        return false;
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool NianliCard::targetFixed() const
{
    Card *new_card = Sanguosha->cloneCard(user_string, Card::SuitToBeDecided, 0);
    DELETE_OVER_SCOPE(Card, new_card)
    new_card->setSkillName("nianli");
    //return false;
    return new_card && new_card->targetFixed();
}

bool NianliCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Card *new_card = Sanguosha->cloneCard(user_string, Card::SuitToBeDecided, 0);
    DELETE_OVER_SCOPE(Card, new_card)
    new_card->setSkillName("nianli");
    return new_card && new_card->targetsFeasible(targets, Self);
}

const Card *NianliCard::validate(CardUseStruct &card_use) const
{
    QString to_use = user_string;
    QList<int> ids = card_use.from->getRoom()->getNCards(2);

    Card *use_card = Sanguosha->cloneCard(to_use, Card::SuitToBeDecided, 0);
    use_card->setSkillName("nianli");
    use_card->addSubcards(ids);
    use_card->deleteLater();
    return use_card;
}

class NianliVS : public ZeroCardViewAsSkill
{
public:
    NianliVS()
        : ZeroCardViewAsSkill("nianli")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("NianliCard");
    }

    /*virtual QStringList getDialogCardOptions() const {
        QStringList options;
        options << "slash" << "snatch";
        return options;
    }*/

    virtual const Card *viewAs() const
    {
        QString name = Self->tag.value("nianli", QString()).toString();
        if (name != NULL) {
            NianliCard *card = new NianliCard;
            card->setUserString(name);
            return card;
        } else
            return NULL;
    }
};

class Nianli : public TriggerSkill
{
public:
    Nianli()
        : TriggerSkill("nianli")
    {
        events << TargetSpecified << PreCardUsed;
        view_as_skill = new NianliVS;
    }

    virtual QDialog *getDialog() const
    {
        return NianliDialog::getInstance("nianli");
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
        if (triggerEvent != TargetSpecified)
            return QList<SkillInvokeDetail>();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getSkillName() == objectName() && use.from && use.from->isAlive())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        bool loseHp = false;
        CardUseStruct use = data.value<CardUseStruct>();
        QString pattern = "";
        if (use.card->isRed())
            pattern = ".|red|.|hand";
        else if (use.card->isBlack())
            pattern = ".|black|.|hand";
        else
            loseHp = true;

        if (!loseHp) {
            QString prompt = "@nianli-discard:";
            prompt = prompt + use.card->getSuitString();
            prompt = prompt + ":" + use.card->objectName();
            const Card *card = room->askForCard(use.from, pattern, prompt, data, Card::MethodNone);
            if (card) {
                CardsMoveStruct move;
                move.card_ids = card->getSubcards();
                move.from = use.from;
                move.to_place = Player::DrawPile;
                room->moveCardsAtomic(move, true);

            } else
                loseHp = true;
        }
        if (loseHp) {
            room->loseHp(use.from, 1);
        }
        return false;
    }
};

class NianliTargetMod : public TargetModSkill
{
public:
    NianliTargetMod()
        : TargetModSkill("#nianlimod")
    {
        pattern = "Slash";
    }

    virtual int getResidueNum(const Player *from, const Card *card) const
    {
        if (from->hasSkill("nianli") && ((card->getSkillName() == "nianli") || card->hasFlag("Global_SlashAvailabilityChecker")))
            return 1000;
        else
            return 0;
    }
};

class Shenmi : public TriggerSkill
{
public:
    Shenmi()
        : TriggerSkill("shenmi")
    {
        frequency = Frequent;
        events << AfterDrawNCards;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DrawNCardsStruct dc = data.value<DrawNCardsStruct>();
        if (dc.player->hasSkill(this) && dc.player->getHandcardNum() > dc.player->getMaxCards())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dc.player, dc.player);
        return QList<SkillInvokeDetail>();
    }

    //defualt cost

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->askForGuanxing(invoke->invoker, room->getNCards(3), Room::GuanxingUpOnly, objectName());
        return false;
    }
};

class Liqun : public DistanceSkill
{
public:
    Liqun()
        : DistanceSkill("liqun$")
    {
    }

    virtual int getCorrect(const Player *from, const Player *to) const
    {
        int correct = 0;
        if (to->hasLordSkill(objectName())) {
            if (from != to) {
                QList<const Player *> players = from->getAliveSiblings();
                foreach (const Player *player, players) {
                    if (player->getKingdom() == "zhan" && player != to && from->inMyAttackRange(player)) {
                        correct = 1;
                        break;
                    }
                }
            }
        }
        return correct;
    }
};

TH09Package::TH09Package()
    : Package("th09")
{
    General *suika = new General(this, "suika$", "zhan", 3, false);
    suika->addSkill(new Zuiyue);
    suika->addSkill(new Doujiu);
    suika->addSkill(new Yanhui);

    General *shikieiki = new General(this, "shikieiki$", "zhan", 3, false);
    shikieiki->addSkill(new Shenpan);
    shikieiki->addSkill(new Huiwu);
    shikieiki->addSkill(new Huazhong);

    General *komachi = new General(this, "komachi", "zhan", 4, false);
    komachi->addSkill(new Boming);
    komachi->addSkill(new Mingtu);

    General *yuka = new General(this, "yuka", "zhan", 4, false);
    yuka->addSkill(new Weiya);

    General *medicine = new General(this, "medicine", "zhan", 3, false);
    medicine->addSkill(new Judu);
    medicine->addSkill(new Henyi);

    General *aya_sp = new General(this, "aya_sp", "zhan", 4, false);
    aya_sp->addSkill(new Toupai);
    aya_sp->addSkill(new Qucai);

    General *tenshi = new General(this, "tenshi$", "zhan", 4, false);
    tenshi->addSkill(new Feixiang);
    tenshi->addSkill(new Dizhen);
    tenshi->addSkill(new Tianren);

    General *iku = new General(this, "iku", "zhan", 4, false);
    iku->addSkill(new Jingdian);
    iku->addSkill(new Leiyun);

    General *hatate = new General(this, "hatate", "zhan", 4, false);
    hatate->addSkill(new Kuaizhao);
    hatate->addSkill(new KuaizhaoEffect);
    hatate->addSkill(new Duanjiao);
    related_skills.insertMulti("kuaizhao", "#kuaizhao");

    General *kokoro = new General(this, "kokoro$", "zhan", 4, false);
    kokoro->addSkill(new Nengwu);
    kokoro->addSkill(new Nengwu2);
    related_skills.insertMulti("nengwu", "#nengwu2");
    kokoro->addSkill(new Xiwang);

    General *sumireko = new General(this, "sumireko$", "zhan", 4, false);
    sumireko->addSkill(new Nianli);
    sumireko->addSkill(new NianliTargetMod);
    sumireko->addSkill(new Shenmi);
    sumireko->addSkill(new Liqun);
    related_skills.insertMulti("nianli", "#nianlimod");

    addMetaObject<ToupaiCard>();
    addMetaObject<TianrenCard>();
    addMetaObject<NianliCard>();
}

ADD_PACKAGE(TH09)
