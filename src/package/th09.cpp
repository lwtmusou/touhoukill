#include "th09.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"
#include "testCard.h"
#include <QCommandLinkButton>
#include <QCoreApplication>
#include <QPointer>

class ZuiyueVS : public ZeroCardViewAsSkill
{
public:
    ZuiyueVS()
        : ZeroCardViewAsSkill("zuiyue")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return player->hasFlag("zuiyue") && Analeptic::IsAvailable(player);
    }

    const Card *viewAs(const Player * /*Self*/) const override
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
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
        relate_to_place = "head";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Peach") && !use.card->isKindOf("Analeptic"))
            return QList<SkillInvokeDetail>();
        if (use.from->getPhase() != Player::Play || use.from->isKongcheng() || !use.from->isAlive())
            return QList<SkillInvokeDetail>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *suika, room->findPlayersBySkillName(objectName())) {
            if (use.from != suika)
                d << SkillInvokeDetail(this, suika, suika, nullptr, false, use.from);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QVariant _data = QVariant::fromValue(invoke->preferredTarget);
        return room->askForSkillInvoke(invoke->invoker, objectName(), _data);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

YanhuiCard::YanhuiCard()
{
    will_throw = false;
    m_skillName = "yanhui_attach";
}

bool YanhuiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || !to_select->hasLordSkill("yanhui") || to_select == Self)
        return false;

    bool globalDying = false;
    if (Self) {
        QList<const Player *> players = Self->getSiblings();
        players << Self;
        foreach (const Player *p, players) {
            if (p->hasFlag("Global_Dying") && p->isAlive()) {
                globalDying = true;
                break;
            }
        }
    }
    if (globalDying) {
        return to_select->hasFlag("Global_Dying");
    }

    return to_select->isWounded() || to_select->isDebuffStatus();
}

const Card *YanhuiCard::validate(CardUseStruct &) const
{
    SuperPeach *use_card = new SuperPeach(Card::SuitToBeDecided, -1);
    use_card->setSkillName("_yanhui");
    use_card->addSubcard(subcards.first());
    use_card->deleteLater();
    return use_card;
}

const Card *YanhuiCard::validateInResponse(ServerPlayer *) const
{
    SuperPeach *use_card = new SuperPeach(Card::SuitToBeDecided, -1);
    use_card->setSkillName("_yanhui");
    use_card->addSubcard(subcards.first());
    use_card->deleteLater();
    return use_card;
}

class YanhuiVS : public OneCardViewAsSkill
{
public:
    YanhuiVS()
        : OneCardViewAsSkill("yanhui_attach")
    {
        attached_lord_skill = true;
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasLordSkill("yanhui") && (p->isWounded() || p->isDebuffStatus()))
                return true;
        }
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &) const override
    {
        QString pattern = player->getRoomObject()->getCurrentCardUsePattern();
        if (!matchAvaliablePattern("peach", pattern))
            return false;
        bool globalDying = false;
        QList<const Player *> players = player->getSiblings();
        players << player;
        foreach (const Player *p, players) {
            if (p->hasFlag("Global_Dying") && p->isAlive()) {
                globalDying = true;
                break;
            }
        }

        if (globalDying) {
            foreach (const Player *p, player->getAliveSiblings()) {
                if (p->hasLordSkill("yanhui") && p->hasFlag("Global_Dying"))
                    return true;
            }
        } else {
            foreach (const Player *p, player->getAliveSiblings()) {
                if (p->hasLordSkill("yanhui") && (p->isWounded() || p->isDebuffStatus()))
                    return true;
            }
        }

        return false;
    }

    bool shouldBeVisible(const Player *Self) const override
    {
        return Self && Self->getKingdom() == "zhan";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select, const Player * /*Self*/) const override
    {
        return to_select->isKindOf("Peach") || to_select->isKindOf("Analeptic");
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
    {
        if (originalCard != nullptr) {
            YanhuiCard *card = new YanhuiCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return nullptr;
    }
};

class Yanhui : public TriggerSkill
{
public:
    Yanhui()
        : TriggerSkill("yanhui$")
    {
        events << PreCardUsed << GameStart << EventAcquireSkill << EventLoseSkill << Revive;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *p, use.to) {
                if (p->hasLordSkill("yanhui") && p != use.from) {
                    if ((use.card->getSkillName() == objectName())) {
                        QList<ServerPlayer *> logto;
                        logto << p;
                        room->touhouLogmessage("#InvokeOthersSkill", use.from, objectName(), logto);
                        room->notifySkillInvoked(p, objectName());
                    }
                }
            }
        } else {
            static QString attachName = "yanhui_attach";
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasLordSkill(this, true))
                    lords << p;
            }

            if (lords.length() > 1) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->hasLordSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else if (lords.length() == 1) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasLordSkill(this, true) && p->hasLordSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                    else if (!p->hasLordSkill(this, true) && !p->hasLordSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else { // the case that lords is empty
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasLordSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                }
            }
        }
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player && player->isAlive() && player->hasSkill(this) && player->getPhase() == Player::Draw)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(), "@shenpan-select", true, true);
        if (target != nullptr)
            invoke->targets << target;
        return target != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    bool canPreshow() const override
    {
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") || (use.card->isNDTrick())) {
            if (use.from == nullptr || use.from->isDead())
                return QList<SkillInvokeDetail>();

            QList<SkillInvokeDetail> details;
            foreach (ServerPlayer *to, use.to) {
                if (to->hasSkill(objectName()) && to->hasShownSkill(objectName()) && to != use.from)
                    details << SkillInvokeDetail(this, to, use.from);
            }
            return details;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *room, const QVariant &data) const override
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
            if (judge->reason == objectName() && judge->isGood() && !judge->ignore_judge) {
                ServerPlayer *lord = judge->relative_player;
                if (lord != nullptr && lord->isAlive())
                    d << SkillInvokeDetail(this, lord, lord, nullptr, true);
            }
        }
        return d;
    }

    bool cost(TriggerEvent event, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (event == Damage) {
            return invoke->invoker->askForSkillInvoke(objectName(), QVariant::fromValue(invoke->owner));
        } else
            return true;
        return false;
    }

    bool effect(TriggerEvent event, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &) const override
    {
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *komachi, room->findPlayersBySkillName(objectName()))
            d << SkillInvokeDetail(this, komachi, komachi);
        return d;
    }

    //defualt cost

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user || !damage.from || damage.from == damage.to || !damage.from->hasSkill(this))
            return QList<SkillInvokeDetail>();
        if (damage.card && damage.card->isKindOf("Slash") && damage.to->getHp() <= damage.to->dyingThreshold())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *current = room->getCurrent();
        if (!current || !current->hasSkill(objectName()) || !current->isAlive())
            return QList<SkillInvokeDetail>();

        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from != current && (use.card->isKindOf("Nullification") || use.card->isKindOf("BasicCard"))) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current, nullptr, true);
            }
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_from == current || !resp.m_card->isKindOf("BasicCard") || resp.m_isRetrial || resp.m_isProvision)
                return QList<SkillInvokeDetail>();
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current, nullptr, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        QString weiya_pattern;
        ServerPlayer *current = room->getCurrent();
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Nullification")) {
                room->notifySkillInvoked(current, objectName());
                room->touhouLogmessage("#weiya_ask", use.from, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                if (room->askForCard(use.from, "nullification", "@weiya:nullification", data))
                    return false;
                room->touhouLogmessage("#weiya", use.from, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                room->setPlayerFlag(use.from, "nullifiationNul");
            } else if (use.card->isKindOf("BasicCard")) {
                weiya_pattern = use.card->objectName();
                if (use.card->isKindOf("Slash"))
                    weiya_pattern = "slash";
                else if (use.card->isKindOf("Jink"))
                    weiya_pattern = "jink";
                else if (use.card->isKindOf("Analeptic"))
                    weiya_pattern = "analeptic";
                else if (use.card->isKindOf("Peach"))
                    weiya_pattern = "peach";
                room->notifySkillInvoked(current, objectName());
                room->touhouLogmessage("#weiya_ask", use.from, objectName(), QList<ServerPlayer *>(), use.card->objectName());
                if (room->askForCard(use.from, weiya_pattern, "@weiya:" + use.card->objectName(), data))
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
            else if (card_star->isKindOf("Jink"))
                weiya_pattern = "jink";
            else if (card_star->isKindOf("Analeptic"))
                weiya_pattern = "analeptic";
            else if (card_star->isKindOf("Peach"))
                weiya_pattern = "peach";
            room->notifySkillInvoked(current, objectName());
            room->touhouLogmessage("#weiya_ask", resp.m_from, objectName(), QList<ServerPlayer *>(), card_star->objectName());
            if (room->askForCard(resp.m_from, weiya_pattern, "@weiya:" + card_star->objectName(), data))
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->hasSkill(this) && damage.to->isAlive() && damage.to != damage.from)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QVariant _data = QVariant::fromValue(damage.to);
        return invoke->invoker->askForSkillInvoke(objectName(), _data);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), damage.to->objectName());
        JudgeStruct judge;
        judge.reason = objectName();
        judge.who = invoke->invoker;
        judge.good = true;
        judge.pattern = ".|black|2~9";

        room->judge(judge);

        if (judge.isGood() && !judge.ignore_judge) {
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
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
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseEnd)
            return invoke->invoker->askForSkillInvoke(objectName(), data);
        return true;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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
                room->fillAG(ids, source, disable, p->getShownHandcards());
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

    const Card *viewAs(const Player * /*Self*/) const override
    {
        ToupaiCard *card = new ToupaiCard;
        return card;
    }
};

class Toupai : public TriggerSkill
{
public:
    Toupai()
        : TriggerSkill("toupai")
    {
        events << EventPhaseStart;
        view_as_skill = new ToupaiVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        return room->askForUseCard(invoke->invoker, "@@toupai", "@toupai");
    }

    bool effect(TriggerEvent, Room * /*room*/, QSharedPointer<SkillInvokeDetail>  /*invoke*/, QVariant &data) const override
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *player = room->getCurrent();
        if (!player || player->isDead() || !player->hasSkill(this))
            return QList<SkillInvokeDetail>();
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill && use.card->isRed() && use.from != player)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_from && player != response.m_from && response.m_isUse && response.m_card && response.m_card->getTypeId() != Card::TypeSkill && response.m_card->isRed())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
            if (from && player != from && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                foreach (int id, move.card_ids) {
                    if (room->getCard(id)->isRed()) {
                        Player::Place from_place = move.from_places.at(move.card_ids.indexOf(id));
                        if (from_place == Player::PlaceHand || from_place == Player::PlaceEquip)
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &) const override
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        ServerPlayer *target = invoke->targets.first();
        int card_id = room->askForCardChosen(invoke->invoker, target, "hes", objectName());
        room->showCard(target, card_id);
        Card *card = room->getCard(card_id);
        if (!target->isCardLimited(card, Card::MethodResponse))
            room->retrial(card, target, judge, objectName(), true);
        return false;
    }
};

class Dizhen : public TriggerSkill
{
public:
    Dizhen()
        : TriggerSkill("dizhen")
    {
        events << TargetSpecified;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card != nullptr && use.card->isKindOf("Slash") && use.from->hasSkill(this)) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, use.to)
                d << SkillInvokeDetail(this, use.from, use.from, nullptr, false, p);

            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        JudgeStruct judge;
        judge.reason = objectName();
        judge.who = invoke->invoker;
        judge.good = true;
        judge.pattern = ".|red";

        room->judge(judge);
        ServerPlayer *target = invoke->targets.first();
        if (judge.isGood() && !judge.ignore_judge && invoke->invoker->isAlive() && target->isAlive() && !target->isKongcheng()) {
            QList<int> ids;
            foreach (const Card *card, target->getHandcards())
                ids << card->getEffectiveId();

            int card_id = room->doGongxin(invoke->invoker, target, ids);
            if (card_id == -1)
                return false;
            QStringList select;
            select << "put";
            if (invoke->invoker->canDiscard(target, card_id))
                select << "discard";
            QString result = room->askForChoice(invoke->invoker, objectName(), select.join("+"));
            if (result == "discard")
                room->throwCard(card_id, target, invoke->invoker);
            else {
                invoke->invoker->setFlags("Global_GongxinOperator");
                CardMoveReason reason(CardMoveReason::S_REASON_PUT, invoke->invoker->objectName(), QString(), objectName(), QString());
                room->moveCardTo(room->getCard(card_id), target, nullptr, Player::DrawPile, reason, target->getShownHandcards().contains(card_id));
                invoke->invoker->setFlags("-Global_GongxinOperator");
            }
            invoke->invoker->tag.remove("gongxin");
        }
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

    const Card *slash = nullptr;
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
    return nullptr;
}

class TianrenVS : public ZeroCardViewAsSkill
{
public:
    TianrenVS()
        : ZeroCardViewAsSkill("tianren$")
    {
    }

    bool isEnabledAtPlay(const Player *) const override
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

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        return hasZhanGenerals(player) && (matchAvaliablePattern("slash", pattern))
            && (player->getRoomObject()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) && (!player->hasFlag("Global_tianrenFailed"))
            && !player->isCurrent();
    }

    const Card *viewAs(const Player * /*Self*/) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature == DamageStruct::Thunder && damage.to->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, nullptr, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    bool isEnabledAtResponse(const Player *p, const QString &pattern) const override
    {
        Lightning l(Card::SuitToBeDecided, -1);
        return l.isAvailable(p) && matchAvaliablePattern("lightning", pattern);
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        Lightning l(Card::SuitToBeDecided, -1);
        return l.isAvailable(player);
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
    {
        Lightning *card = new Lightning(originalCard->getSuit(), originalCard->getNumber());
        card->addSubcard(originalCard);
        card->setSkillName("leiyun");
        return card;
    }
};

class Kuaizhao : public TriggerSkill
{
public:
    Kuaizhao()
        : TriggerSkill("kuaizhao")
    {
        events << DrawNCards;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DrawNCardsStruct dc = data.value<DrawNCardsStruct>();
        ServerPlayer *target = dc.player->tag["kuaizhao_target"].value<ServerPlayer *>();
        if (target)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dc.player, dc.player, nullptr, true, target);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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
        //view_as_skill = new ShowDistanceSkill(objectName());
    }

    int getFixed(const Player *target, bool) const override
    {
        if (isHegemonyGameMode(ServerInfo.GameMode)) {
            if (target->hasSkill("duanjiao") && target->hasShownSkill("duanjiao"))
                return 3;
        } else if (target->hasSkill("duanjiao"))
            return 3;

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

    static QList<ServerPlayer *> nengwuTargets(ServerPlayer *player, bool draw, TriggerEvent e)
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, player->getRoom()->getOtherPlayers(player)) {
            if (e == CardsMoveOneTime && draw && player->getHandcardNum() > p->getHandcardNum())
                targets << p;
            else if (e == HpRecover && draw && player->getHp() > p->getHp())
                targets << p;
            else if (e == CardsMoveOneTime && !draw && player->getHandcardNum() < p->getHandcardNum() && player->canDiscard(p, "hs"))
                targets << p;
            else if (e == Damaged && !draw && player->getHp() < p->getHp() && player->canDiscard(p, "hs"))
                targets << p;
        }
        return targets;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == HpRecover) {
            RecoverStruct r = data.value<RecoverStruct>();
            if (r.to->hasSkill(this) && !r.to->isCurrent() && !nengwuTargets(r.to, true, triggerEvent).isEmpty())
                d << SkillInvokeDetail(this, r.to, r.to);
        } else if (triggerEvent == CardsMoveOneTime) {
            if (room->getTag("FirstRound").toBool())
                return QList<SkillInvokeDetail>();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *playerTo = qobject_cast<ServerPlayer *>(move.to);
            if (playerTo != nullptr && playerTo->isAlive() && playerTo->hasSkill(this) && move.to_place == Player::PlaceHand && !playerTo->isCurrent()
                && !nengwuTargets(playerTo, true, triggerEvent).isEmpty()) {
                d << SkillInvokeDetail(this, playerTo, playerTo);
            }
        }
        return d;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<ServerPlayer *> targets = nengwuTargets(invoke->invoker, true, triggerEvent);

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, "nengwudraw", "@nengwu-draw", true, true);
        if (target) {
            room->notifySkillInvoked(invoke->invoker, objectName());
            invoke->targets << target;
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

class Nengwu2 : public TriggerSkill
{
public:
    Nengwu2()
        : TriggerSkill("#nengwu2")
    {
        events << CardsMoveOneTime << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == Damaged) {
            ServerPlayer *player = data.value<DamageStruct>().to;
            if (player->hasSkill("nengwu") && player->isAlive() && !player->isCurrent() && !Nengwu::nengwuTargets(player, false, triggerEvent).isEmpty())
                d << SkillInvokeDetail(this, player, player);
        } else if (triggerEvent == CardsMoveOneTime) {
            if (room->getTag("FirstRound").toBool())
                return QList<SkillInvokeDetail>();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *playerFrom = qobject_cast<ServerPlayer *>(move.from);
            if (playerFrom != nullptr && playerFrom->isAlive() && playerFrom->hasSkill("nengwu") && move.from_places.contains(Player::PlaceHand) && !playerFrom->isCurrent()
                && !Nengwu::nengwuTargets(playerFrom, false, triggerEvent).isEmpty()) {
                d << SkillInvokeDetail(this, playerFrom, playerFrom);
            }
        }
        return d;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<ServerPlayer *> targets = Nengwu::nengwuTargets(invoke->invoker, false, triggerEvent);

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, "nengwudiscard", "@nengwu-discard", true, true);
        if (target) {
            room->notifySkillInvoked(invoke->invoker, "nengwu");
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *current = room->getCurrent();
        if (!current || current->getKingdom() != "zhan" || current->isDead())
            return QList<SkillInvokeDetail>();
        if (current->isKongcheng())
            return QList<SkillInvokeDetail>();

        ServerPlayer *player = nullptr;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getHandlingMethod() == Card::MethodUse && use.card->getSuit() == Card::Heart) //caution the case using a skillcard
                player = use.from;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct s = data.value<CardResponseStruct>();
            if (!s.m_isProvision && s.m_card->getSuit() == Card::Heart)
                player = s.m_from;
        }

        if (player == nullptr || player == current)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->getOtherPlayers(current)) {
            if (p->hasLordSkill(objectName()))
                d << SkillInvokeDetail(this, p, current);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *lord = invoke->owner;
        int card_id = invoke->invoker->tag["xiwang_id"].toInt();
        invoke->invoker->tag.remove("xiwang_id");
        room->obtainCard(lord, card_id, false); //room->getCardPlace(card_id) != Player::PlaceHand
        return false;
    }
};

NianliDialog *NianliDialog::getInstance(const QString &object)
{
    static QPointer<NianliDialog> instance;

    if (!instance.isNull() && instance->objectName() != object)
        delete instance;

    if (instance.isNull()) {
        instance = new NianliDialog(object);
        connect(qApp, &QCoreApplication::aboutToQuit, instance.data(), &NianliDialog::deleteLater);
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

void NianliDialog::popup(Player *_Self)
{
    Self = _Self;

    if (Self->getRoomObject()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
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
    }

    Self->tag.remove(object_name);
    exec();
}

void NianliDialog::selectCard(QAbstractButton *button)
{
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
    if (new_card->targetFixed(Self))
        return false;
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool NianliCard::targetFixed(const Player *Self) const
{
    Card *new_card = Sanguosha->cloneCard(user_string, Card::SuitToBeDecided, 0);
    DELETE_OVER_SCOPE(Card, new_card)
    new_card->setSkillName("nianli");
    return new_card && new_card->targetFixed(Self);
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

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("NianliCard");
    }

    const Card *viewAs(const Player *Self) const override
    {
        QString name = Self->tag.value("nianli", QString()).toString();
        if (name != nullptr) {
            NianliCard *card = new NianliCard;
            card->setUserString(name);
            return card;
        } else
            return nullptr;
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

    QDialog *getDialog() const override
    {
        return NianliDialog::getInstance("nianli");
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
        if (triggerEvent != TargetSpecified)
            return QList<SkillInvokeDetail>();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getSkillName() == objectName() && use.from && use.from->isAlive())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, nullptr, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
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

    int getResidueNum(const Player *from, const Card *card) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DrawNCardsStruct dc = data.value<DrawNCardsStruct>();
        if (dc.player->hasSkill(this) && dc.player->getHandcardNum() > dc.player->getMaxCards())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dc.player, dc.player);
        return QList<SkillInvokeDetail>();
    }

    //defualt cost

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    int getCorrect(const Player *from, const Player *to) const override
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

class Jianshe : public TriggerSkill
{
public:
    Jianshe()
        : TriggerSkill("jianshe")
    {
        events << EventPhaseStart << CardsMoveOneTime << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
            if (player && player->isAlive() && move.from_places.contains(Player::PlaceHand))
                room->setPlayerFlag(player, "jianshe_losed");
        }
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("jianshe_losed"))
                        room->setPlayerFlag(p, "-jianshe_losed");
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e != EventPhaseStart)
            return QList<SkillInvokeDetail>();

        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player == nullptr || !player->isAlive() || player->getPhase() != Player::Finish)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        for (ServerPlayer *skiller : room->findPlayersBySkillName(objectName())) {
            if (skiller != player && skiller->canDiscard(skiller, "hs"))
                d << SkillInvokeDetail(this, skiller, skiller);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *current = room->getCurrent();
        if (!invoke->invoker->hasFlag("jianshe_losed"))
            return room->askForCard(invoke->invoker, ".|.|.|hand", "@jianshe-discard:" + current->objectName(), QVariant::fromValue(current), Card::MethodDiscard, nullptr, false,
                                    objectName());
        else
            return invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(current));
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = room->getCurrent();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), target->objectName());
        if (target->getHandcardNum() > 1) {
            QList<QString> uselist;
            uselist << "jianshe_jian";
            uselist << "jianshe_she";
            QString use = room->askForChoice(target, objectName(), uselist.join("+"));
            if (use == "jianshe_jian") {
                room->drawCards(target, 1, objectName());
                room->loseHp(target, 1);
            } else {
                int hands = target->getHandcardNum();
                room->askForDiscard(target, "@jianshe-hint", hands - 1, hands - 1);
                room->recover(target, RecoverStruct());
            }
        } else {
            room->drawCards(target, 1, objectName());
            room->loseHp(target, 1);
        }
        return false;
    }
};

class YsJie : public TriggerSkill
{
public:
    YsJie()
        : TriggerSkill("ysjie")
    {
        events << EventPhaseStart << EventAcquireSkill << EventLoseSkill << Death << EventSkillInvalidityChange << HpChanged << CardsMoveOneTime;
        show_type = "static";
        frequency = Compulsory;
    }

    static void removeYsJieLimit(ServerPlayer *player, QList<ServerPlayer *> targets)
    {
        Room *room = player->getRoom();
        foreach (ServerPlayer *p, targets) {
            if (p->isCardLimited("use,response", "ysjie"))
                room->removePlayerCardLimitation(p, "use,response", ".|.|.|.", "ysjie");
        }
    }

    static void setYsJieLimit(ServerPlayer *player, QList<ServerPlayer *> targets)
    {
        Room *room = player->getRoom();
        foreach (ServerPlayer *p, targets) {
            if (!p->isCardLimited("use,response", "ysjie"))
                room->setPlayerCardLimitation(p, "use,response", ".|.|.|.", "ysjie", true);
        }
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->getPhase() == Player::RoundStart) {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->getHandcardNum() < p->getLostHp())
                        targets << p;
                }
                if (!targets.isEmpty())
                    setYsJieLimit(player, targets);
            }
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who && death.who->isCurrent() && death.who->hasSkill(this)) {
                removeYsJieLimit(death.who, room->getOtherPlayers(death.who));
            }
        } else if (triggerEvent == EventLoseSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            if (a.player && a.player->isCurrent() && a.skill->objectName() == objectName()) {
                removeYsJieLimit(a.player, room->getOtherPlayers(a.player));
            }
        } else if (triggerEvent == EventAcquireSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            if (a.player && a.player->isCurrent() && a.skill->objectName() == objectName()) {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getOtherPlayers(a.player)) {
                    if (p->getHandcardNum() < p->getLostHp())
                        targets << p;
                }
                setYsJieLimit(a.player, targets);
            }
        } else if (triggerEvent == EventSkillInvalidityChange) {
            QList<SkillInvalidStruct> invalids = data.value<QList<SkillInvalidStruct> >();
            foreach (SkillInvalidStruct v, invalids) {
                if (v.player == nullptr || !v.player->isCurrent())
                    continue;

                if (!v.skill || v.skill->objectName() == objectName()) {
                    if (!v.invalid && v.player->hasSkill(this)) {
                        QList<ServerPlayer *> targets;
                        foreach (ServerPlayer *p, room->getOtherPlayers(v.player)) {
                            if (p->getHandcardNum() < p->getLostHp())
                                targets << p;
                        }
                        setYsJieLimit(v.player, targets);
                    } else if (v.invalid)
                        removeYsJieLimit(v.player, room->getOtherPlayers(v.player));
                }
            }
        } else if (triggerEvent == HpChanged || triggerEvent == CardsMoveOneTime) {
            if (triggerEvent == CardsMoveOneTime) {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (!move.from_places.contains(Player::PlaceHand) && move.to_place != Player::PlaceHand)
                    return;
            }

            ServerPlayer *current = room->getCurrent();
            if (current && current->hasSkill(this)) {
                QList<ServerPlayer *> targets1;
                QList<ServerPlayer *> targets2;
                foreach (ServerPlayer *p, room->getOtherPlayers(current)) {
                    if (p->getHandcardNum() < p->getLostHp())
                        targets1 << p;
                    else
                        targets2 << p;
                }
                if (!targets1.isEmpty())
                    setYsJieLimit(current, targets1);
                if (!targets2.isEmpty())
                    removeYsJieLimit(current, targets2);
            }
        }
    }
};

class Yishen : public DistanceSkill
{
public:
    Yishen()
        : DistanceSkill("yishen$")
    {
    }

    int getCorrect(const Player *from, const Player *to) const override
    {
        int correct = 0;
        if (from->hasLordSkill(objectName())) {
            if (from != to) {
                QList<const Player *> players = from->getAliveSiblings();
                foreach (const Player *player, players) {
                    if (player->getKingdom() == "zhan" && player != to && player->inMyAttackRange(to)) {
                        correct = -1;
                        break;
                    }
                }
            }
        }
        return correct;
    }
};

MengxiangTargetCard::MengxiangTargetCard()
{
}

bool MengxiangTargetCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < Self->getMark("mengxiang") && !to_select->isKongcheng(); //
}

void MengxiangTargetCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->notifySkillInvoked(source, "mengxiang");
    foreach (ServerPlayer *p, targets)
        room->setPlayerMark(p, "mengxiangtarget", 1);
}

MengxiangCard::MengxiangCard()
{
    will_throw = false;
}

bool MengxiangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *oc = Self->getRoomObject()->getCard(subcards.first());
    return oc->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, oc, targets);
}

bool MengxiangCard::targetFixed(const Player *Self) const
{
    const Card *oc = Self->getRoomObject()->getCard(subcards.first());
    return oc->targetFixed(Self);
}

bool MengxiangCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Card *oc = Self->getRoomObject()->getCard(subcards.first());
    if (oc->canRecast() && targets.length() == 0)
        return false;
    return oc->targetsFeasible(targets, Self);
}

const Card *MengxiangCard::validate(CardUseStruct &use) const
{
    Room *room = use.from->getRoom();
    const Card *card = room->getCard(subcards.first());
    use.from->showHiddenSkill("mengxiang");

    room->notifySkillInvoked(use.from, "mengxiang");
    LogMessage mes;
    mes.type = "$mengxiang";
    mes.from = use.from;
    mes.to << room->getCardOwner(subcards.first());
    mes.arg = "mengxiang";
    mes.card_str = card->toString();
    room->sendLog(mes);

    return card;
}

class MengxiangVS : public ViewAsSkill
{
public:
    MengxiangVS()
        : ViewAsSkill("mengxiang")
    {
        expand_pile = "#mengxiang_temp";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const override
    {
        //if (Self->hasFlag("Global_mengxiangFailed")) {
        if (Self->getRoomObject()->getCurrentCardUsePattern() == "@@mengxiang-card2") {
            if ((to_select->isKindOf("Jink") || to_select->isKindOf("Nullification")))
                return false;
            if (to_select->isKindOf("Peach") && !to_select->isAvailable(Self))
                return false;
            return selected.isEmpty() && Self->getPile("#mengxiang_temp").contains(to_select->getId());
        }
        return false;
    }

    bool isEnabledAtPlay(const Player *) const override
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const override
    {
        return pattern.startsWith("@@mengxiang");
    }

    const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const override
    {
        if (cards.length() > 1)
            return nullptr;

        if (Self->getRoomObject()->getCurrentCardUsePattern() == "@@mengxiang-card2") {
            MengxiangCard *card = new MengxiangCard;
            card->addSubcards(cards);
            return card;
        } else {
            MengxiangTargetCard *card = new MengxiangTargetCard;
            return card;
        }
    }
};

class Mengxiang : public TriggerSkill
{
public:
    Mengxiang()
        : TriggerSkill("mengxiang")
    {
        events << EventPhaseChanging << EventPhaseEnd << CardsMoveOneTime << CardUsed;
        view_as_skill = new MengxiangVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != nullptr) {
                ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
                if (player->getPhase() == Player::Discard && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                    && move.to_place == Player::DiscardPile) {
                    room->setPlayerMark(player, objectName(), player->getMark(objectName()) + move.card_ids.length());
                }
            }
        }
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    room->setPlayerMark(p, objectName(), 0);
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->hasSkill(this) && change.to == Player::Play && !change.player->isSkipped(change.to)) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player);
            }
        } else if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->isAlive() && player->hasFlag(objectName()) && player->getMark(objectName()) > 0 && player->getPhase() == Player::Discard) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            return invoke->invoker->askForSkillInvoke(this);
        } else if (triggerEvent == EventPhaseEnd) {
            room->askForUseCard(invoke->invoker, "@@mengxiang-card1", "@mengxiang");

            foreach (ServerPlayer *player, room->getAllPlayers(false)) {
                if (player->getMark("mengxiangtarget") == 1) {
                    invoke->targets << player;
                    player->removeMark("mengxiangtarget");
                }
            }
            return !invoke->targets.isEmpty();
        }
        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            invoke->invoker->skip(Player::Play);
            invoke->invoker->setFlags(objectName());
        } else if (triggerEvent == EventPhaseEnd) {
            foreach (ServerPlayer *p, invoke->targets) {
                if (p->isDead())
                    continue;
                QList<int> ids;
                foreach (const Card *c, p->getHandcards())
                    ids << c->getEffectiveId();

                if (!ids.isEmpty()) {
                    putToPile(room, invoke->invoker, ids);

                    //Flag  mengxiangtarget is only for DashBoard UI
                    foreach (ServerPlayer *t, room->getAllPlayers()) {
                        if (t == p)
                            room->setPlayerFlag(t, "mengxiangtarget");
                        else
                            room->setPlayerFlag(t, "-mengxiangtarget");
                    }

                    if (room->askForUseCard(invoke->invoker, "@@mengxiang-card2", "@mengxiang_use:" + p->objectName()) && p->isAlive())
                        p->drawCards(1);
                    cleanUp(room, invoke->invoker);
                }
            }
        }
        return false;
    }

    static bool putToPile(Room *room, ServerPlayer *player, QList<int> ids)
    {
        CardsMoveStruct move;
        move.from_place = Player::DiscardPile;
        move.to = player;
        move.to_player_name = player->objectName();
        move.to_pile_name = "#mengxiang_temp";
        move.card_ids = ids;
        move.to_place = Player::PlaceSpecial;
        move.open = true;

        QList<CardsMoveStruct> _moves = QList<CardsMoveStruct>() << move;
        QList<ServerPlayer *> _player = QList<ServerPlayer *>() << player;
        room->setPlayerFlag(player, "mengxiang_InTempMoving");
        room->notifyMoveCards(true, _moves, true, _player);
        room->notifyMoveCards(false, _moves, true, _player);
        room->setPlayerFlag(player, "-mengxiang_InTempMoving");

        QVariantList tag = IntList2VariantList(ids);
        player->tag["mengxiang_tempcards"] = tag;
        return true;
    }

    static void cleanUp(Room *room, ServerPlayer *player)
    {
        QList<int> hands = VariantList2IntList(player->tag.value("mengxiang_tempcards", QVariantList()).toList());
        player->tag.remove("mengxiang_tempcards");
        if (hands.isEmpty())
            return;

        CardsMoveStruct move;
        move.from = player;
        move.from_player_name = player->objectName();
        move.from_place = Player::PlaceSpecial;
        move.from_pile_name = "#mengxiang_temp";
        move.to_place = Player::DiscardPile;
        move.open = true;
        move.card_ids = hands;

        QList<CardsMoveStruct> _moves = QList<CardsMoveStruct>() << move;
        QList<ServerPlayer *> _player = QList<ServerPlayer *>() << player;
        room->setPlayerFlag(player, "mengxiang_InTempMoving");
        room->notifyMoveCards(true, _moves, true, _player);
        room->notifyMoveCards(false, _moves, true, _player);
        room->setPlayerFlag(player, "-mengxiang_InTempMoving");
    }
};

JishiCard::JishiCard()
{
    target_fixed = true;
    will_throw = false;
}

void JishiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    int x = subcards.length();
    CardsMoveStruct move;
    move.card_ids = subcards;
    //move.from = source;
    move.to_place = Player::DrawPile;
    room->moveCardsAtomic(move, false);
    if (x > 1)
        room->askForGuanxing(source, room->getNCards(x), Room::GuanxingUpOnly, "jishi");
}

class JishiVS : public ViewAsSkill
{
public:
    JishiVS()
        : ViewAsSkill("jishi")
    {
        response_pattern = "@@jishi";
        expand_pile = "#jishi_temp";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select, const Player *Self) const override
    {
        return Self->getPile("#jishi_temp").contains(to_select->getId());
    }

    const Card *viewAs(const QList<const Card *> &cards, const Player * /*Self*/) const override //
    {
        if (cards.isEmpty())
            return nullptr;

        JishiCard *card = new JishiCard;
        card->addSubcards(cards);
        return card;
    }
};

class Jishi : public TriggerSkill
{
public:
    Jishi()
        : TriggerSkill("jishi")
    {
        events << CardsMoveOneTime;
        view_as_skill = new JishiVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD && move.to_place == Player::DiscardPile) {
                foreach (int id, move.card_ids) {
                    if (room->getCardPlace(move.card_ids.first()) != Player::DiscardPile)
                        continue;

                    if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand) {
                        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
                        if (player != nullptr && player->isAlive() && player->hasSkill(this))
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
                    } else if (move.origin_from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand) {
                        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.origin_from);
                        if (player != nullptr && player->isAlive() && player->hasSkill(this))
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> ids;
        foreach (int id, move.card_ids) {
            if (room->getCardPlace(move.card_ids.first()) != Player::DiscardPile)
                continue;

            if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand)
                ids << id;
            else if (move.origin_from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand)
                ids << id;
        }
        if (ids.isEmpty())
            return false;

        putToPile(room, invoke->invoker, ids);
        room->askForUseCard(invoke->invoker, "@@jishi", "@jishi");
        cleanUp(room, invoke->invoker);

        return false;
    }

    static bool putToPile(Room *room, ServerPlayer *player, QList<int> ids)
    {
        CardsMoveStruct move;
        move.from_place = Player::DiscardPile;
        move.to = player;
        move.to_player_name = player->objectName();
        move.to_pile_name = "#jishi_temp";
        move.card_ids = ids;
        move.to_place = Player::PlaceSpecial;
        move.open = true;

        QList<CardsMoveStruct> _moves = QList<CardsMoveStruct>() << move;
        QList<ServerPlayer *> _player = QList<ServerPlayer *>() << player;
        room->setPlayerFlag(player, "jishi_InTempMoving");
        room->notifyMoveCards(true, _moves, true, _player);
        room->notifyMoveCards(false, _moves, true, _player);
        room->setPlayerFlag(player, "-jishi_InTempMoving");

        QVariantList tag = IntList2VariantList(ids);
        player->tag["jishi_tempcards"] = tag;
        return true;
    }

    static void cleanUp(Room *room, ServerPlayer *player)
    {
        QList<int> cards = VariantList2IntList(player->tag.value("jishi_tempcards", QVariantList()).toList());
        player->tag.remove("jishi_tempcards");
        if (cards.isEmpty())
            return;

        CardsMoveStruct move;
        move.from = player;
        move.from_player_name = player->objectName();
        move.from_place = Player::PlaceSpecial;
        move.from_pile_name = "#jishi_temp";
        move.to_place = Player::DiscardPile;
        move.open = true;
        move.card_ids = cards;

        QList<CardsMoveStruct> _moves = QList<CardsMoveStruct>() << move;
        QList<ServerPlayer *> _player = QList<ServerPlayer *>() << player;
        room->setPlayerFlag(player, "jishi_InTempMoving");
        room->notifyMoveCards(true, _moves, true, _player);
        room->notifyMoveCards(false, _moves, true, _player);
        room->setPlayerFlag(player, "-jishi_InTempMoving");
    }
};

MianLingCard::MianLingCard()
{
    will_throw = false;
}

bool MianLingCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *oc = Self->getRoomObject()->getCard(subcards.first());
    return oc->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, oc, targets);
}

bool MianLingCard::targetFixed(const Player *Self) const
{
    const Card *oc = Self->getRoomObject()->getCard(subcards.first());
    return oc->targetFixed(Self);
}

bool MianLingCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Card *oc = Self->getRoomObject()->getCard(subcards.first());
    if (oc->canRecast() && targets.length() == 0)
        return false;
    return oc->targetsFeasible(targets, Self);
}

const Card *MianLingCard::validate(CardUseStruct &use) const
{
    Room *room = use.from->getRoom();
    const Card *c = room->getCard(subcards.first());
    c->setFlags("mianling");
    return c;
}

const Card *MianLingCard::validateInResponse(ServerPlayer *p) const
{
    Room *room = p->getRoom();
    const Card *c = room->getCard(subcards.first());
    c->setFlags("mianling");
    return c;
}

class MianlingVS : public ViewAsSkill
{
public:
    MianlingVS()
        : ViewAsSkill("mianling")
    {
        expand_pile = "qsmian";
        // response_or_use = true;
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const override
    {
        if (!Self->getPile("qsmian").contains(to_select->getId()))
            return false;

        if (Self->getRoomObject()->getCurrentCardUsePattern() == "@@mianling!")
            return Self->getPile("qsmian").length() - selected.length() > 1 + Self->getAliveSiblings().length();
        else
            return selected.length() == 0
                && ((Self->getRoomObject()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY)
                        ? to_select->isAvailable(Self)
                        : Sanguosha->matchExpPattern(Self->getRoomObject()->getCurrentCardUsePattern(), Self, to_select));
    }

    const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const override
    {
        if (Self->getRoomObject()->getCurrentCardUsePattern() == "@@mianling!") {
            if (Self->getPile("qsmian").length() - cards.length() == 1 + Self->getAliveSiblings().length()) {
                DummyCard *dc = new DummyCard;
                dc->addSubcards(cards);
                return dc;
            }
        } else if (cards.length() == 1) {
            MianLingCard *ml = new MianLingCard;
            ml->addSubcards(cards);
            return ml;
        }

        return nullptr;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        foreach (int id, player->getPile("qsmian")) {
            if (player->getRoomObject()->getCard(id)->isAvailable(player))
                return true;
        }

        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        if (pattern == "@@mianling!")
            return true;

        foreach (int id, player->getPile("qsmian")) {
            if (Sanguosha->matchExpPattern(pattern, player, player->getRoomObject()->getCard(id)))
                return true;
        }

        return false;
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const override
    {
        return !player->getPile("qsmian").isEmpty();
    }
};

class Mianling : public TriggerSkill
{
public:
    Mianling()
        : TriggerSkill("mianling")
    {
        view_as_skill = new MianlingVS;
        events << BeforeCardsMove << CardFinished; // << ??
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.reason.m_reason == CardMoveReason::S_REASON_DRAW && move.reason.m_extraData.toString() != "initialDraw") {
                ServerPlayer *marisa = qobject_cast<ServerPlayer *>(move.to);
                if (marisa != nullptr && marisa->hasSkill(this))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, marisa, marisa, nullptr, true);
            }
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != nullptr && use.card->hasFlag("mianling") && use.from != nullptr && use.from->isAlive())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, nullptr, true);
        } // else if ???

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QList<int> c = room->getNCards(move.card_ids.length());
            invoke->invoker->addToPile("qsmian", c);

            if (invoke->invoker->getPile("qsmian").length() > room->getAlivePlayers().length()) {
                int n = invoke->invoker->getPile("qsmian").length() - room->getAlivePlayers().length();

                DummyCard dc;
                const Card *e = room->askForCard(invoke->invoker, "@@mianling!", "@mianling-exchange:::" + QString::number(n), QVariant(), Card::MethodNone);
                if (e == nullptr) {
                    QList<int> m = invoke->invoker->getPile("qsmian");
                    qShuffle(m);
                    m = m.mid(1, n);
                    dc.addSubcards(m);
                    e = &dc;
                }

                room->throwCard(e, nullptr);
            }
        } else {
            if (!room->askForDiscard(invoke->invoker, "mianling", 1, 1, true, true, "@mianling-discard"))
                room->loseHp(invoke->invoker);
        }

        return false;
    }
};

class Ximshang : public TriggerSkill
{
public:
    Ximshang()
        : TriggerSkill("ximshang")
    {
        events << CardsMoveOneTime << EventPhaseStart;
        global = true; // BE WARE! this skill should record even if no one actually has this skill
    }

    void record(TriggerEvent e, Room *room, QVariant &d) const override
    {
        if (e == CardsMoveOneTime) {
            if (room->getCurrent() != nullptr && room->getCurrent()->isAlive() && room->getCurrent()->getPhase() != Player::NotActive) {
                CardsMoveOneTimeStruct m = d.value<CardsMoveOneTimeStruct>();
                if (m.from != nullptr && (m.from_places.contains(Player::PlaceHand) || m.from_places.contains(Player::PlaceEquip)))
                    m.from->setFlags("ximshanglost");

                if (m.to_place == Player::DiscardPile) {
                    foreach (int id, m.card_ids) {
                        const Card *c = room->getCard(id);
                        bool flag = false;
                        switch (c->getSuit()) {
                        case Card::Spade: // 0
                        case Card::Club: // 1
                        case Card::Heart: // 2
                        case Card::Diamond: // 3
                            flag = true;
                            break;
                        default:
                            break;
                        }
                        if (flag) {
                            // BE WARE! check this when changing enum Card::Suit
                            int m = room->getCurrent()->getMark("ximshangsuits");
                            m = m | (1 << static_cast<int>(c->getSuit()));
                            room->getCurrent()->setMark("ximshangsuits", m);
                        }
                    }
                }
            }
        } else {
            ServerPlayer *p = d.value<ServerPlayer *>();
            if (p->getPhase() == Player::NotActive) {
                p->setMark("ximshangsuits", 0);
                foreach (ServerPlayer *sp, room->getAlivePlayers())
                    sp->setFlags("-ximshanglost");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *r, const QVariant &d) const override
    {
        QList<SkillInvokeDetail> ret;
        if (e == EventPhaseStart) {
            ServerPlayer *p = d.value<ServerPlayer *>();

            // magic number: 1 << 0 | 1 << 1 | 1 << 2 | 1 << 3 == 15
            // BE WARE! Since this matches Card::Suit, be sure to check this when changing Card::Suit
            if (p != nullptr && p->isAlive() && p->getPhase() == Player::Finish && p->getMark("ximshangsuits") == 15) {
                foreach (ServerPlayer *sp, r->getAlivePlayers()) {
                    if (sp->hasSkill(this) && sp->hasFlag("ximshanglost"))
                        ret << SkillInvokeDetail(this, sp, sp);
                }
            }
        }

        return ret;
    }

    bool cost(TriggerEvent, Room *r, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *t = r->askForPlayerChosen(invoke->invoker, r->getAlivePlayers(), "ximshang", "@ximshang-select", true, true);
        if (t != nullptr) {
            invoke->targets << t;
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->damage(DamageStruct("ximshang", invoke->invoker, invoke->targets.first()));
        return false;
    }
};

TH09Package::TH09Package()
    : Package("th09")
{
    General *suika = new General(this, "suika$", "zhan", 3);
    suika->addSkill(new Zuiyue);
    suika->addSkill(new Doujiu);
    suika->addSkill(new Yanhui);

    General *shikieiki = new General(this, "shikieiki$", "zhan", 3);
    shikieiki->addSkill(new Shenpan);
    shikieiki->addSkill(new Huiwu);
    shikieiki->addSkill(new Huazhong);

    General *komachi = new General(this, "komachi", "zhan", 4);
    komachi->addSkill(new Boming);
    komachi->addSkill(new Mingtu);

    General *yuka = new General(this, "yuka", "zhan", 4);
    yuka->addSkill(new Weiya);

    General *medicine = new General(this, "medicine", "zhan", 3);
    medicine->addSkill(new Judu);
    medicine->addSkill(new Henyi);

    General *aya_sp = new General(this, "aya_sp", "zhan", 4);
    aya_sp->addSkill(new Toupai);
    aya_sp->addSkill(new Qucai);

    General *tenshi = new General(this, "tenshi$", "zhan", 4);
    tenshi->addSkill(new Feixiang);
    tenshi->addSkill(new Dizhen);
    tenshi->addSkill(new Tianren);

    General *iku = new General(this, "iku", "zhan", 4);
    iku->addSkill(new Jingdian);
    iku->addSkill(new Leiyun);

    General *hatate = new General(this, "hatate", "zhan", 4);
    hatate->addSkill(new Kuaizhao);
    hatate->addSkill(new KuaizhaoEffect);
    hatate->addSkill(new Duanjiao);
    related_skills.insert("kuaizhao", "#kuaizhao");

    General *kokoro = new General(this, "kokoro$", "zhan", 4);
    kokoro->addSkill(new Nengwu);
    kokoro->addSkill(new Nengwu2);
    related_skills.insert("nengwu", "#nengwu2");
    kokoro->addSkill(new Xiwang);

    General *sumireko = new General(this, "sumireko$", "zhan", 4);
    sumireko->addSkill(new Nianli);
    sumireko->addSkill(new NianliTargetMod);
    sumireko->addSkill(new Shenmi);
    sumireko->addSkill(new Liqun);
    related_skills.insert("nianli", "#nianlimod");

    General *sumireko_sp = new General(this, "sumireko_sp", "zhan", 4);
    sumireko_sp->addSkill(new Mengxiang);
    sumireko_sp->addSkill(new Jishi);

    General *yorigamis = new General(this, "yorigamis$", "zhan", 4);
    yorigamis->addSkill(new Jianshe);
    yorigamis->addSkill(new YsJie);
    yorigamis->addSkill(new Yishen);

    General *kokorosp = new General(this, "kokoro_sp", "zhan", 3);
    kokorosp->addSkill(new Mianling);
    kokorosp->addSkill(new Ximshang);

    addMetaObject<YanhuiCard>();
    addMetaObject<ToupaiCard>();
    addMetaObject<TianrenCard>();
    addMetaObject<NianliCard>();
    addMetaObject<MengxiangCard>();
    addMetaObject<MengxiangTargetCard>();
    addMetaObject<JishiCard>();
    addMetaObject<MianLingCard>();

    skills << new YanhuiVS;
}

ADD_PACKAGE(TH09)
