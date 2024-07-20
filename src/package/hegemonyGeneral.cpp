#include "hegemonyGeneral.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "hegemonyCard.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"
#include "th06.h"
#include "th08.h"

class GameRule_AskForGeneralShowHead : public TriggerSkill
{
public:
    GameRule_AskForGeneralShowHead()
        : TriggerSkill("GameRule_AskForGeneralShowHead")
    {
        events << EventPhaseStart;
        global = true;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->showGeneral(true, true);
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (isHegemonyGameMode(ServerInfo.GameMode) && player != nullptr && player->getPhase() == Player::Start && !player->hasShownGeneral()
            && player->disableShow(true).isEmpty())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        return QList<SkillInvokeDetail>();
    }
};

class GameRule_AskForGeneralShowDeputy : public TriggerSkill
{
public:
    GameRule_AskForGeneralShowDeputy()
        : TriggerSkill("GameRule_AskForGeneralShowDeputy")
    {
        events << EventPhaseStart;
        global = true;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->showGeneral(false, true);
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (isHegemonyGameMode(ServerInfo.GameMode) && ServerInfo.Enable2ndGeneral && player != nullptr && player->getPhase() == Player::Start && !player->hasShownGeneral2()
            && player->disableShow(false).isEmpty())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        return QList<SkillInvokeDetail>();
    }
};

class GameRule_AskForArraySummon : public TriggerSkill
{
public:
    GameRule_AskForArraySummon()
        : TriggerSkill("GameRule_AskForArraySummon")
    {
        events << EventPhaseStart;
        global = true;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->gainMark("@nima");
        foreach (const Skill *skill, invoke->invoker->getVisibleSkillList()) {
            if (!skill->inherits("BattleArraySkill"))
                continue;
            const BattleArraySkill *baskill = qobject_cast<const BattleArraySkill *>(skill);
            if (!invoke->invoker->askForSkillInvoke(objectName()))
                return false;
            invoke->invoker->gainMark("@dandan_" + skill->objectName());
            invoke->invoker->showGeneral(invoke->invoker->inHeadSkills(skill->objectName()));
            baskill->summonFriends(invoke->invoker);
            break;
        }
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player == nullptr || player->getPhase() != Player::Start || room->getAlivePlayers().length() < 4)
            return QList<SkillInvokeDetail>();

        foreach (const Skill *skill, player->getVisibleSkillList()) {
            if (!skill->inherits("BattleArraySkill"))
                continue;
            if (qobject_cast<const BattleArraySkill *>(skill)->getViewAsSkill()->isEnabledAtPlay(player)) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            }
        }
        return QList<SkillInvokeDetail>();
    }
};

NiaoxiangSummon::NiaoxiangSummon()
    : ArraySummonCard("niaoxiang")
{
}

class Niaoxiang : public BattleArraySkill
{
public:
    Niaoxiang()
        : BattleArraySkill("niaoxiang", "Siege")
    {
        events << TargetSpecified;
        //array_type = "Siege";
    }

    bool canPreshow() const override
    {
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card == nullptr || !use.card->isKindOf("Slash"))
            return QList<SkillInvokeDetail>();

        QList<ServerPlayer *> skill_owners = room->findPlayersBySkillName(objectName());
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *skill_owner, skill_owners) {
            if (skill_owner->hasShownSkill(this)) { //!BattleArraySkill::triggerable(event, room, data).isEmpty() &&
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *to, use.to) {
                    if (use.from != nullptr && use.from->inSiegeRelation(skill_owner, to))
                        targets << to; //->objectName();
                }

                if (!targets.isEmpty())
                    d << SkillInvokeDetail(this, skill_owner, use.from, targets, true);
                //skill_list.insert(skill_owner, QStringList(objectName() + "->" + targets.join("+")));
            }
        }
        return d;
    }

    //virtual bool cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *ask_who) const
    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (invoke->owner != nullptr && invoke->owner->hasShownSkill(this)) {
            foreach (ServerPlayer *skill_target, invoke->targets)
                room->doBattleArrayAnimate(invoke->owner, skill_target);
            //room->broadcastSkillInvoke(objectName(), invoke->owner);
            return true;
        }
        return false;
    }

    //virtual bool effect(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &data, ServerPlayer *ask_who) const
    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        //room->sendCompulsoryTriggerLog(ask_who, objectName(), true);
        //CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *skill_target, invoke->targets)
            room->loseHp(skill_target);
        /*
        int x = use.to.indexOf(skill_target);
        QVariantList jink_list = use.from->tag["Jink_" + use.card->toString()].toList();
        if (jink_list.at(x).toInt() == 1)
            jink_list[x] = 2;
        use.from->tag["Jink_" + use.card->toString()] = jink_list;
        */
        return false;
    }
};

//********  GameRule   **********

HalfLifeCard::HalfLifeCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "halflife_attach";
    target_fixed = true;
}

void HalfLifeCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;

    source->loseMark("@HalfLife", 1);
    source->drawCards(1);
    room->detachSkillFromPlayer(source, "halflife_attach", true);
}

class HalfLifeVS : public ZeroCardViewAsSkill
{
public:
    HalfLifeVS()
        : ZeroCardViewAsSkill("halflife_attach")
    {
        attached_lord_skill = true;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return player->getMark("@HalfLife") > 0;
    }

    //virtual bool shouldBeVisible(const Player *Self) const
    //{
    //   return Self && Self->getMark("@HalfLife") > 0;
    //}

    const Card *viewAs() const override
    {
        return new HalfLifeCard;
    }
};

class HalfLife : public TriggerSkill
{
public:
    HalfLife()
        : TriggerSkill("HalfLife")
    {
        events << EventPhaseStart << EventPhaseChanging;
        global = true;
    }

    /*void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        static QString attachName = "halflife_attach";
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player && !player->hasSkill(attachName))
            room->attachSkillToPlayer(player, attachName);

    }*/

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && (change.player != nullptr) && change.player->isAlive())
                room->setPlayerMark(change.player, "HalfLife_maxcard", 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if ((current != nullptr) && current->isAlive() && current->getPhase() == Player::Discard && current->getMark("@HalfLife") > 0
                && current->getHandcardNum() > current->getMaxCards())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->loseMark("@HalfLife", 1);
        room->setPlayerMark(invoke->invoker, "HalfLife_maxcard", 2);
        room->detachSkillFromPlayer(invoke->invoker, "halflife_attach", true);
        LogMessage log;
        log.type = "#HalfLife";
        log.from = invoke->invoker;
        room->sendLog(log);
        return false;
    }
};

class HalfLifeMax : public MaxCardsSkill
{
public:
    HalfLifeMax()
        : MaxCardsSkill("#HalfLife_max")
    {
    }

    int getExtra(const Player *target) const override
    {
        return target->getMark("HalfLife_maxcard");
    }
};

CompanionCard::CompanionCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "companion_attach";
    target_fixed = true;
}

void CompanionCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *player = card_use.from;

    player->loseMark("@CompanionEffect", 1);
    room->detachSkillFromPlayer(player, "companion_attach", true);

    QString choice;
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
        QStringList choices;
        if (player->isWounded())
            choices << "recover";
        choices << "draw";

        choice = room->askForChoice(player, "CompanionEffect", choices.join("+"));
        if (choice == "recover") {
            RecoverStruct recover;
            recover.who = player;
            recover.recover = 1;
            room->recover(player, recover);
        } else if (choice == "draw")
            player->drawCards(2);

    } else {
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->hasFlag("Global_Dying")) {
                RecoverStruct recover;
                recover.who = player;
                recover.recover = 1;
                room->recover(p, recover);
                break;
            }
        }
    }

    room->setEmotion(player, "companion");
}

class CompanionVS : public ZeroCardViewAsSkill
{
public:
    CompanionVS()
        : ZeroCardViewAsSkill("companion_attach")
    {
        attached_lord_skill = true;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return player->getMark("@CompanionEffect") > 0;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const override
    {
        return pattern.contains("peach");
    }

    const Card *viewAs() const override
    {
        return new CompanionCard;
    }
};

PioneerCard::PioneerCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "pioneer_attach";
    target_fixed = true;
}

void PioneerCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *player = card_use.from;

    player->loseMark("@Pioneer", 1);
    room->detachSkillFromPlayer(player, "pioneer_attach", true);

    int num = qMax(0, 4 - player->getHandcardNum());
    player->drawCards(num);

    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
        if (!p->hasShownAllGenerals())
            targets << p;
    }

    if (!targets.isEmpty()) {
        ServerPlayer *target = room->askForPlayerChosen(player, targets, "pioneer_attach", "@pioneer_attach");
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
        QStringList select;
        if (!target->hasShownGeneral())
            select << "showhead";
        if ((target->getGeneral2() != nullptr) && !target->hasShownGeneral2())
            select << "showdeputy";

        QString choice = room->askForChoice(player, "pioneer_attach", select.join("+"), QVariant::fromValue(target));

        LogMessage log;
        log.type = "#KnownBothView";
        log.from = player;
        log.to << target;
        log.arg = choice;
        foreach (ServerPlayer *p, room->getAllPlayers(true)) { //room->getOtherPlayers(effect.from, true)
            room->doNotify(p, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
        }

        if (choice == "showhead" || choice == "showdeputy") {
            QStringList list = room->getTag(target->objectName()).toStringList();
            list.removeAt(choice == "showhead" ? 1 : 0);
            foreach (const QString &name, list) {
                LogMessage log;
                log.type = "$KnownBothViewGeneral";
                log.from = player;
                log.to << target;
                log.arg = name;
                log.arg2 = target->getRole();
                room->doNotify(player, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
            }
            JsonArray arg;
            arg << objectName();
            arg << JsonUtils::toJsonArray(list);
            room->doNotify(player, QSanProtocol::S_COMMAND_VIEW_GENERALS, arg);
        }
    }
}

class PioneerVS : public ZeroCardViewAsSkill
{
public:
    PioneerVS()
        : ZeroCardViewAsSkill("pioneer_attach")
    {
        attached_lord_skill = true;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return player->getMark("@Pioneer") > 0;
    }

    const Card *viewAs() const override
    {
        return new PioneerCard;
    }
};

//********  PROTAGONIST   **********

class TuizhiHegemony : public TriggerSkill
{
public:
    TuizhiHegemony()
        : TriggerSkill("tuizhi_hegemony")
    {
        events << CardUsed << CardResponded;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *room, const QVariant &data) const override
    {
        if (event == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if ((use.card != nullptr) && use.card->getSuit() == Card::Heart) {
                if ((use.from != nullptr) && use.from->hasSkill(this)) {
                    foreach (ServerPlayer *p, room->getOtherPlayers(use.from)) {
                        if ((p->hasShownGeneral() && !p->getGeneralName().contains("sujiang")) || (p->hasShownGeneral2() && !p->getGeneral2Name().contains("sujiang")))
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
                    }
                }
            }
        } else if (event == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse && (resp.m_card != nullptr) && resp.m_card->getSuit() == Card::Heart) {
                if ((resp.m_from != nullptr) && resp.m_from->hasSkill(this)) {
                    foreach (ServerPlayer *p, room->getOtherPlayers(resp.m_from)) {
                        if ((p->hasShownGeneral() && !p->getGeneralName().contains("sujiang")) || (p->hasShownGeneral2() && !p->getGeneral2Name().contains("sujiang")))
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, resp.m_from, resp.m_from);
                    }
                }
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if ((p->hasShownGeneral() && !p->getGeneralName().contains("sujiang")) || (p->hasShownGeneral2() && !p->getGeneral2Name().contains("sujiang")))
                targets << p;
        }

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@tuizhi_hegemony", true, true);
        if (target != nullptr)
            invoke->targets << target;
        return target != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = invoke->targets.first();
        QStringList select;
        if (target->hasShownGeneral() && !target->getGeneralName().contains("sujiang"))
            select << "head";
        if ((target->getGeneral2() != nullptr) && target->hasShownGeneral2() && !target->getGeneral2Name().contains("sujiang"))
            select << "deputy";
        if (select.isEmpty())
            return false;

        QString choice = room->askForChoice(invoke->invoker, objectName(), select.join("+"));
        bool ishead = (choice == "head");
        target->hideGeneral(ishead);
        return false;
    }
};

class TongjieHegemony : public TriggerSkill
{
public:
    TongjieHegemony()
        : TriggerSkill("tongjie_hegemony")
    {
        events << GeneralShown << GeneralHidden << GeneralRemoved << EventPhaseStart << Death << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    void doTongjie(Room *room, ServerPlayer *reimu, bool set) const
    {
        if (set && !reimu->tag["tongjie"].toBool()) {
            foreach (ServerPlayer *p, room->getOtherPlayers(reimu))
                room->setPlayerDisableShow(p, "hd", "tongjie");

            reimu->tag["tongjie"] = true;
        } else if (!set && reimu->tag["tongjie"].toBool()) {
            foreach (ServerPlayer *p, room->getOtherPlayers(reimu))
                room->removePlayerDisableShow(p, "tongjie");

            reimu->tag["tongjie"] = false;
        }
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        ServerPlayer *player = nullptr;
        if (triggerEvent != Death) {
            if (triggerEvent == EventAcquireSkill || triggerEvent == EventLoseSkill) {
                SkillAcquireDetachStruct s = data.value<SkillAcquireDetachStruct>();
                player = s.player;
            } else if (triggerEvent == GeneralRemoved || triggerEvent == GeneralHidden || triggerEvent == GeneralShown) {
                ShowGeneralStruct s = data.value<ShowGeneralStruct>();
                player = s.player;
            } else {
                player = data.value<ServerPlayer *>();
            }

            if (player == nullptr || !player->isAlive())
                return;
        } else if (triggerEvent == Death) {
            DeathStruct d = data.value<DeathStruct>();
            player = d.who;
            if (!player->hasShownSkill(this))
                return;
        }

        ServerPlayer *c = room->getCurrent();
        if (c == nullptr || (triggerEvent != EventPhaseStart && c->getPhase() == Player::NotActive) || c != player)
            return;

        if ((triggerEvent == GeneralShown || triggerEvent == EventPhaseStart || triggerEvent == EventAcquireSkill) && !player->hasShownSkill(this))
            return;
        if ((triggerEvent == GeneralShown || triggerEvent == GeneralHidden)) {
            ShowGeneralStruct s = data.value<ShowGeneralStruct>();
            if (!s.player->ownSkill(this))
                return;
            if (s.player->inHeadSkills(this->objectName()) != s.isHead)
                return;
        }

        if (triggerEvent == GeneralRemoved) {
            ShowGeneralStruct s = data.value<ShowGeneralStruct>();

            bool removeReimu = false;
            QStringList generals = room->getTag(s.player->objectName()).toStringList();
            if (s.isHead) {
                if (generals.first() == "reimu_hegemony")
                    removeReimu = true;
            } else {
                if (generals.last() == "reimu_hegemony")
                    removeReimu = true;
            }
            if (!removeReimu)
                return;
        }
        if (triggerEvent == EventPhaseStart) {
            player = data.value<ServerPlayer *>();
            if (!(player->getPhase() == Player::RoundStart || player->getPhase() == Player::NotActive))
                return;
        }
        if (triggerEvent == Death) {
            DeathStruct d = data.value<DeathStruct>();
            player = d.who;
            if (!player->hasShownSkill(this))
                return;
        }

        if ((triggerEvent == EventAcquireSkill || triggerEvent == EventLoseSkill)) {
            SkillAcquireDetachStruct s = data.value<SkillAcquireDetachStruct>();
            if (s.skill->objectName() != objectName())
                return;
        }

        bool set = false;
        if (triggerEvent == GeneralShown || triggerEvent == EventAcquireSkill || (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart))
            set = true;

        if (player != nullptr)
            doTongjie(room, player, set);
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *room, const QVariant &data) const override
    {
        if (event == GeneralShown) {
            ShowGeneralStruct s = data.value<ShowGeneralStruct>();
            ServerPlayer *target = s.player;
            if ((target != nullptr) && target->isAlive()) {
                QList<SkillInvokeDetail> d;
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p != target)
                        d << SkillInvokeDetail(this, p, p, nullptr, true);
                }
                return d;
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->sendLog("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        invoke->invoker->drawCards(1);
        return false;
    }
};

class MofaHegemony : public TriggerSkill
{
public:
    MofaHegemony()
        : TriggerSkill("mofa_hegemony")
    {
        events << DamageCaused << DamageInflicted;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from == nullptr || damage.to == nullptr || damage.from->isDead() || damage.from == damage.to)
            return QList<SkillInvokeDetail>();

        if (event == DamageCaused) {
            if (damage.from->hasSkill(this) && (!damage.to->hasShownGeneral() || !damage.to->hasShownGeneral2())) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, true);
            }
        } else if (event == DamageInflicted) {
            if (damage.to->hasSkill(this) && damage.to->isAlive() && (!damage.from->hasShownGeneral() || !damage.from->hasShownGeneral2())) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, nullptr, true);
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent event, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (invoke->invoker->hasShownSkill(this))
            return true;

        //ServerPlayer *marisa = invoke->invoker;
        //JudgeStruct *judge = data.value<JudgeStruct *>();
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt = "";
        if (event == DamageCaused) {
            prompt = "notice1:" + damage.to->objectName() + ":" + QString::number(damage.damage);
        } else if (event == DamageInflicted) {
            prompt = "notice2:" + damage.from->objectName() + ":" + QString::number(damage.damage);
        }

        return room->askForSkillInvoke(invoke->invoker, objectName(), data, prompt);
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        DamageStruct damage = data.value<DamageStruct>();
        if (e == DamageCaused) {
            room->sendLog("#TouhouBuff", damage.from, objectName());
            QList<ServerPlayer *> logto;
            logto << damage.to;
            room->sendLog("#mofa_damage", damage.from, QString::number(damage.damage + 1), logto, QString::number(damage.damage));
            damage.damage = damage.damage + 1;
        } else if (e == DamageInflicted) {
            room->sendLog("#TouhouBuff", damage.from, objectName());
            QList<ServerPlayer *> logto;
            logto << damage.to;
            room->sendLog("#mofa_damage1", damage.from, QString::number(damage.damage - 1), logto, QString::number(damage.damage));
            damage.damage = damage.damage - 1;
        }
        data = QVariant::fromValue(damage);
        if (damage.damage < 1)
            return true;
        return false;
    }
};

class JiezouHegemonyVS : public OneCardViewAsSkill
{
public:
    JiezouHegemonyVS()
        : OneCardViewAsSkill("jiezou_hegemony")
    {
        filter_pattern = ".|spade";
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasFlag("jiezou_hegemony");
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        Snatch *card = new Snatch(originalCard->getSuit(), originalCard->getNumber());
        card->addSubcard(originalCard);
        card->setSkillName("jiezou_hegemony");
        return card;
    }
};

class JiezouHegemony : public TriggerSkill
{
public:
    JiezouHegemony()
        : TriggerSkill("jiezou_hegemony")
    {
        events << PreCardUsed;
        view_as_skill = new JiezouHegemonyVS;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getSkillName() == "jiezou_hegemony") {
            room->setPlayerFlag(use.from, "jiezou_hegemony");
        }
    }
};

class JiezouTargetMod : public TargetModSkill
{
public:
    JiezouTargetMod()
        : TargetModSkill("#jiezoumod")
    {
        pattern = "Snatch";
    }

    int getDistanceLimit(const Player *, const Card *card) const override
    {
        if (card->getSkillName() == "jiezou_hegemony")
            return 1000;
        else
            return 0;
    }
};

//********  SPRING   **********

class LizhiHegemony : public TriggerSkill
{
public:
    LizhiHegemony()
        : TriggerSkill("lizhi_hegemony")
    {
        events << CardFinished << DamageDone << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if ((damage.from != nullptr) && (damage.card != nullptr)) //&& damage.card->isKindOf("Slash")
                room->setCardFlag(damage.card, "lizhiDamage");
        }
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("lizhi_used"))
                    room->setPlayerFlag(p, "-lizhi_used");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *room, const QVariant &data) const override
    {
        if (event == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card == nullptr || !use.card->canDamage() || use.card->hasFlag("lizhiDamage"))
                return QList<SkillInvokeDetail>();

            ServerPlayer *source = nullptr;
            if (!use.to.isEmpty()) {
                foreach (ServerPlayer *p, use.to) {
                    if (p->isAlive() && p->hasSkill(this) && !p->hasFlag("lizhi_used")) {
                        source = p;
                        break;
                    }
                }
            }
            if (source == nullptr && (use.from != nullptr) && use.from->isAlive() && use.from->hasSkill(this) && !use.from->hasFlag("lizhi_used"))
                source = use.from;
            if (source == nullptr)
                return QList<SkillInvokeDetail>();

            QList<int> ids;
            if (use.card->isVirtualCard())
                ids = use.card->getSubcards();
            else
                ids << use.card->getEffectiveId();

            if (ids.isEmpty())
                return QList<SkillInvokeDetail>();
            foreach (int id, ids) {
                if (room->getCardPlace(id) != Player::DiscardPile) //place???
                    return QList<SkillInvokeDetail>();
            }

            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, source, source);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (invoke->invoker->isFriendWith(p, true))
                targets << p;
        }
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@lizhi:" + use.card->objectName(), true, true);
        if (target != nullptr)
            invoke->targets << target;
        return target != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->setPlayerFlag(invoke->invoker, "lizhi_used");
        CardUseStruct use = data.value<CardUseStruct>();
        invoke->targets.first()->obtainCard(use.card);
        return false;
    }
};

class YunshangHegemony : public TriggerSkill
{
public:
    YunshangHegemony()
        : TriggerSkill("yunshang_hegemony")
    {
        frequency = Compulsory;
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;

        if (use.from != nullptr && use.card->isNDTrick()) {
            foreach (ServerPlayer *p, use.to) {
                if (p->hasSkill(this) && !use.from->inMyAttackRange(p) && use.from != p)
                    d << SkillInvokeDetail(this, p, p, nullptr, true);
            }
        }

        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        //for AI
        CardUseStruct use = data.value<CardUseStruct>();
        room->setTag("yunshang_use", data);
        return (invoke->invoker->hasShownSkill(this) || invoke->invoker->askForSkillInvoke(this, data));
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.nullified_list << invoke->invoker->objectName();
        data = QVariant::fromValue(use);
        room->notifySkillInvoked(invoke->invoker, objectName());

        return false;
    }
};

class JingxiaHegemony : public MasochismSkill
{
public:
    JingxiaHegemony()
        : MasochismSkill("jingxia_hegemony")
    {
    }

    QList<SkillInvokeDetail> triggerable(const Room *room, const DamageStruct &damage) const override
    {
        bool invoke = false;
        do {
            if (damage.to->isAlive() && damage.to->hasSkill(this)) {
                if (damage.from != nullptr && damage.to->canDiscard(damage.from, "hes")) {
                    invoke = true;
                    break;
                }
                bool flag = false;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (damage.to->canDiscard(p, "ej")) {
                        flag = true;
                        break;
                    }
                }
                if (flag) {
                    invoke = true;
                    break;
                }
            }
        } while (false);

        if (invoke) {
            QList<SkillInvokeDetail> d;
            for (int i = 0; i < damage.damage; ++i) {
                d << SkillInvokeDetail(this, damage.to, damage.to);
            }
            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QStringList select;
        if (damage.from != nullptr && damage.to->canDiscard(damage.from, "hes"))
            select << "discard";

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (damage.to->canDiscard(p, "ej")) {
                select << "discardfield";
                break;
            }
        }
        select.prepend("dismiss");

        ServerPlayer *player = damage.to;

        player->tag["jingxia"] = QVariant::fromValue(damage);
        QString choice = room->askForChoice(player, objectName(), select.join("+"), QVariant::fromValue(damage));
        player->tag.remove("jingxia");
        if (choice == "dismiss")
            return false;

        invoke->tag["jingxia"] = choice;
        return true;
    }

    void onDamaged(Room *room, QSharedPointer<SkillInvokeDetail> invoke, const DamageStruct &damage) const override
    {
        QList<ServerPlayer *> fieldcard;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (damage.to->canDiscard(p, "ej"))
                fieldcard << p;
        }
        ServerPlayer *player = damage.to;

        QString choice = invoke->tag.value("jingxia").toString();

        room->sendLog("#InvokeSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        if (choice == "discard") {
            for (int i = 0; i < 2; i++) {
                if (player->isDead() || !player->canDiscard(damage.from, "hes"))
                    return;
                if (damage.from == player)
                    room->askForDiscard(player, "jingxia", 1, 1, false, true);
                else {
                    int card_id = room->askForCardChosen(player, damage.from, "hes", "jingxia", false, Card::MethodDiscard);
                    room->throwCard(card_id, damage.from, player);
                }
            }
        } else if (choice == "discardfield") {
            ServerPlayer *player1 = room->askForPlayerChosen(player, fieldcard, "jingxia", "@jingxia-discardfield");
            int card1 = room->askForCardChosen(player, player1, "ej", objectName(), false, Card::MethodDiscard);
            room->throwCard(card1, player1, player);
        }
    }
};

QingtingHegemonyCard::QingtingHegemonyCard()
{
    target_fixed = true;
    m_skillName = "qingting_hegemony";
}

void QingtingHegemonyCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;

    foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
        if (p->isKongcheng())
            continue;
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), p->objectName());
    }
    foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
        if (p->isKongcheng())
            continue;
        const Card *card = nullptr;
        if (source->getMark("shengge") > 0 || p->getHandcardNum() == 1)
            card = new DummyCard(QList<int>() << room->askForCardChosen(source, p, "hs", "qingting"));
        else {
            p->tag["qingting_give"] = QVariant::fromValue(source);
            card = room->askForExchange(p, "qingting_give", 1, 1, false, "qingtingGive:" + source->objectName());
            p->tag.remove("qingting_give");
        }
        DELETE_OVER_SCOPE(const Card, card)

        source->obtainCard(card, false);
        room->setPlayerMark(p, "@qingting", 1);
    }

    //get delay
    if (source->isOnline())
        room->getThread()->delay(2000);

    foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
        if (p->getMark("@qingting") == 0)
            continue;
        room->setPlayerMark(p, "@qingting", 0);
        if (source->isKongcheng())
            continue;

        source->tag["qingting_return"] = QVariant::fromValue(p);
        const Card *card = room->askForExchange(source, "qingting", 1, 1, false, "qingtingReturn:" + p->objectName());
        DELETE_OVER_SCOPE(const Card, card)
        source->tag.remove("qingting_return");
        p->obtainCard(card, false);
    }
}

class QingtingHegemony : public ZeroCardViewAsSkill
{
public:
    QingtingHegemony()
        : ZeroCardViewAsSkill("qingting_hegemony")
    {
    }
    static bool checkQingting(const Player *player)
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (!p->isKongcheng())
                return true;
        }
        return false;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("QingtingHegemonyCard") && checkQingting(player);
    }

    const Card *viewAs() const override
    {
        return new QingtingHegemonyCard;
    }
};

class ShezhengHegemony : public AttackRangeSkill
{
public:
    ShezhengHegemony()
        : AttackRangeSkill("shezheng_hegemony")
    {
        relate_to_place = "deputy";
    }

    bool canPreshow() const override
    {
        return true;
    }

    int getExtra(const Player *player, bool) const override
    {
        if (player->hasSkill(objectName()) && player->hasShownSkill(objectName()) && (player->getWeapon() == nullptr))
            return 1;
        return 0;
    }
};

ShowShezhengCard::ShowShezhengCard()
    : SkillCard()
{
    mute = true;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

const Card *ShowShezhengCard::validate(CardUseStruct &card_use) const
{
    bool head = card_use.from->inHeadSkills("shezheng_hegemony");
    card_use.from->showGeneral(head);
    return nullptr;
}

class ShezhengAttach : public ViewAsSkill
{
public:
    ShezhengAttach()
        : ViewAsSkill("shezheng_attach")
    {
        attached_lord_skill = true;
    }

    bool shouldBeVisible(const Player *Self) const override
    {
        return Self != nullptr;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasShownSkill("shezheng_hegemony");
    }

    bool viewFilter(const QList<const Card *> &, const Card *) const override
    {
        return false;
    }

    const Card *viewAs(const QList<const Card *> &) const override
    {
        return new ShowShezhengCard();
    }
};

class ShezhengHegemonyHandler : public TriggerSkill
{
public:
    ShezhengHegemonyHandler()
        : TriggerSkill("#shezheng_hegemony")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Death << Debut << EventSkillInvalidityChange;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const override
    {
        if (triggerEvent == GameStart || triggerEvent == Debut || triggerEvent == EventAcquireSkill) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if ((p->hasSkill("shezheng_hegemony", true) || p->ownSkill("shezheng_hegemony")) && !p->hasSkill("shezheng_attach"))
                    room->attachSkillToPlayer(p, "shezheng_attach");
            }
        }
        if (triggerEvent == Death || triggerEvent == EventLoseSkill) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if ((!p->hasSkill("shezheng_hegemony", true) && !p->ownSkill("shezheng_hegemony")) && p->hasSkill("shezheng_attach"))
                    room->detachSkillFromPlayer(p, "shezheng_attach", true);
            }
        }
    }
};

class ShezhengViewHas : public ViewHasSkill

{
public:
    ShezhengViewHas()
        : ViewHasSkill("#shezheng_viewhas")

    {
    }

    bool ViewHas(const Player *player, const QString &skill_name, const QString &flag, bool) const override
    {
        if (flag == "weapon" && skill_name == "DoubleSwordHegemony" && player->isAlive() && player->hasSkill("shezheng_hegemony") && (player->getWeapon() == nullptr))
            return true;

        return false;
    }
};

class ChilingHegemony : public TriggerSkill
{
public:
    ChilingHegemony()
        : TriggerSkill("chiling_hegemony")
    {
        events << CardsMoveOneTime;
        relate_to_place = "head";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *miko = qobject_cast<ServerPlayer *>(move.from);
        if (miko != nullptr && miko->isAlive() && miko->hasSkill(objectName()) //&& move.from_places.contains(Player::PlaceHand)
            && (move.to_place == Player::PlaceHand && (move.to != nullptr) && move.to != miko))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, miko, miko);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *target = qobject_cast<ServerPlayer *>(move.to);
        Card *card = Sanguosha->getCard(move.card_ids.first());
        QString prompt = "@chiling_hegemony:" + invoke->invoker->objectName() + ":" + card->objectName();
        room->askForUseCard(target, IntList2StringList(move.card_ids).join("#"), prompt, -1, Card::MethodUse, false);
        return false;
    }
};

class FenleiHegemony : public TriggerSkill
{
public:
    FenleiHegemony()
        : TriggerSkill("fenlei_hegemony")
    {
        events << QuitDying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *who = data.value<DyingStruct>().who;
        if (who->isAlive() && who->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, who, who);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *tokizo = invoke->invoker;
        ServerPlayer *target = room->askForPlayerChosen(tokizo, room->getOtherPlayers(tokizo), objectName(), "@fenlei", true, true);
        if (target != nullptr)
            invoke->targets << target;
        return target != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->damage(DamageStruct(objectName(), nullptr, invoke->targets.first(), 1, DamageStruct::Thunder));
        return false;
    }
};

class BianhuanHegemony : public TriggerSkill
{
public:
    BianhuanHegemony()
        : TriggerSkill("bianhuan_hegemony")
    {
        events << DamageInflicted;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->isAlive() && damage.to->hasSkill(this) && !damage.to->isRemoved())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->loseHp(invoke->invoker, 1);
        return true;
    }
};

class LiandaHegemony : public TargetModSkill
{
public:
    LiandaHegemony()
        : TargetModSkill("lianda_hegemony")
    {
        pattern = "Slash";
    }

    int getResidueNum(const Player *from, const Card *) const override
    {
        if (from->hasSkill(objectName())) // && from->hasShownSkill(objectName())
            return from->getLostHp();

        return 0;
    }
};

class ShanshiHegemony : public TriggerSkill
{
public:
    ShanshiHegemony()
        : TriggerSkill("shanshi_hegemony")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                p->setMark("shanshi_invoke", 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent != CardsMoveOneTime)
            return QList<SkillInvokeDetail>();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == nullptr || move.from->isDead())
            return QList<SkillInvokeDetail>();
        // one lose a card
        if ((move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
            && (move.to != move.from || (move.to_place != Player::PlaceHand && move.to_place != Player::PlaceEquip))) {
            if (move.from->hasSkill(this) && !move.from->isCurrent()) {
                // you lose a card in other's round
                ServerPlayer *current = room->getCurrent();
                if (current == nullptr || !current->isCurrent() || current->isDead() || !current->isInMainPhase())
                    return QList<SkillInvokeDetail>();

                ServerPlayer *myo = qobject_cast<ServerPlayer *>(move.from);
                int num = qMax(1, myo->getHp());
                if (myo == nullptr || myo == current || myo->getMark("shanshi_invoke") > 0 || myo->getHandcardNum() >= num)
                    return QList<SkillInvokeDetail>();
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, myo, myo, nullptr, false, current);
            } else {
                // others lose a card in your round
                ServerPlayer *myo = room->getCurrent();

                if (myo == nullptr || !myo->hasSkill(this) || !myo->isCurrent() || myo->isDead() || myo->getMark("shanshi_invoke") > 0)
                    return QList<SkillInvokeDetail>();

                ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
                int num = qMax(1, from->getHp());
                if (from == nullptr || from == myo || from->getHandcardNum() >= num)
                    return QList<SkillInvokeDetail>();

                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, myo, myo, nullptr, false, from);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget))) {
            invoke->invoker->setMark("shanshi_invoke", 1);
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<ServerPlayer *> l = QList<ServerPlayer *>() << invoke->invoker << invoke->targets.first();
        room->sortByActionOrder(l);
        room->drawCards(l, 1, objectName());
        return false;
    }
};

//********  SUMMER   **********
class BolanHgemony : public TriggerSkill
{
public:
    BolanHgemony()
        : TriggerSkill("bolan_hegemony")
    {
        events << CardUsed << EventPhaseChanging << TargetConfirmed;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                p->setFlags("-" + objectName());
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging)
            return QList<SkillInvokeDetail>();

        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (!use.card->isNDTrick())
            return d;

        QList<ServerPlayer *> owners = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *p, owners) {
            if (!p->hasFlag(objectName())) {
                if (triggerEvent == CardUsed && (use.from != nullptr) && p->isFriendWith(use.from, true)) { //use.from->hasShownOneGeneral() &&
                    d << SkillInvokeDetail(this, p, p);
                    continue;
                }
                if (triggerEvent == TargetConfirmed) {
                    foreach (ServerPlayer *to, use.to) {
                        if (p->isFriendWith(to, true)) {
                            d << SkillInvokeDetail(this, p, p);
                            break;
                        }
                    }
                }
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<int> list = room->getNCards(2);
        ServerPlayer *player = invoke->invoker;
        player->setFlags(objectName());

        CardsMoveStruct move(list, nullptr, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, invoke->invoker->objectName(), objectName(), QString()));
        room->moveCardsAtomic(move, true);

        QList<int> able;
        QList<int> disabled;
        foreach (int id, list) {
            Card *tmp_card = Sanguosha->getCard(id);
            if (tmp_card->isKindOf("TrickCard") || use.card->getSuit() == tmp_card->getSuit())
                able << id;
            else
                disabled << id;
        }

        if (!able.isEmpty()) {
            DummyCard dummy(able);
            room->obtainCard(player, &dummy);
        }
        if (!disabled.isEmpty()) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, invoke->invoker->objectName(), objectName(), QString());
            DummyCard dummy(disabled);
            room->throwCard(&dummy, reason, nullptr);
        }

        return false;
    }
};

class BeishuiHegemonyVS : public ViewAsSkill
{
public:
    BeishuiHegemonyVS()
        : ViewAsSkill("beishui_hegemony")
    {
        response_or_use = true;
    }

    static QStringList responsePatterns()
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);

        Card::HandlingMethod method = Card::MethodUse;
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();

        QStringList checkedPatterns;
        QStringList ban_list = Sanguosha->getBanPackages();
        foreach (const Card *card, cards) {
            if ((card->isKindOf("BasicCard")) && !ban_list.contains(card->getPackage())) { //&& !ServerInfo.Extensions.contains("!" + card->getPackage())
                QString name = card->objectName();
                if (!checkedPatterns.contains(name) && (cardPattern != nullptr && cardPattern->match(Self, card)) && !Self->isCardLimited(card, method))
                    checkedPatterns << name;
            }
        }

        return checkedPatterns;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        if (player->getMark("beishui") > 0)
            return false;
        if (Slash::IsAvailable(player) || Analeptic::IsAvailable(player))
            return true;
        Card *card = Sanguosha->cloneCard("peach", Card::NoSuit, 0);
        DELETE_OVER_SCOPE(Card, card)
        Card *card1 = Sanguosha->cloneCard("super_peach", Card::NoSuit, 0);
        DELETE_OVER_SCOPE(Card, card1)
        return card->isAvailable(player) || card1->isAvailable(player);
    }

    bool isEnabledAtResponse(const Player *player, const QString &) const override
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            return false;
        if (player->getMark("beishui") > 0)
            return false;

        //check main phase
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

        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.contains("peach") && checkedPatterns.length() == 1 && player->getMark("Global_PreventPeach") > 0)
            return false;

        return !checkedPatterns.isEmpty();
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *) const override
    {
        //int num = qMax(1, Self->getHp());
        int roles = 1;
        if (Self->getRole() != "careerist") {
            foreach (const Player *p, Self->getAliveSiblings()) {
                if (Self->isFriendWith(p)) //p->getRole() == Self->getRole()
                    roles++;
            }
        }
        int num = qMax(roles, Self->getHp());
        return selected.length() < num;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        //int num = qMax(1, Self->getHp());
        int roles = 1;
        if (Self->getRole() != "careerist") {
            foreach (const Player *p, Self->getAliveSiblings()) {
                if (Self->isFriendWith(p)) //p->getRole() == Self->getRole()
                    roles++;
            }
        }

        int num = qMax(roles, Self->getHp());
        if (cards.length() != num)
            return nullptr;

        QString name = Self->tag.value("beishui_hegemony", QString()).toString();
        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.length() == 1)
            name = checkedPatterns.first();
        if (name != nullptr) {
            Card *card = Sanguosha->cloneCard(name);
            card->setSkillName(objectName());
            card->addSubcards(cards);
            return card;
        } else
            return nullptr;
    }
};

class BeishuiHegemony : public TriggerSkill
{
public:
    BeishuiHegemony()
        : TriggerSkill("beishui_hegemony")
    {
        events << EventPhaseChanging << PreCardUsed << CardResponded;
        view_as_skill = new BeishuiHegemonyVS;
    }

    QDialog *getDialog() const override
    {
        return BeishuiDialog::getInstance("beishui_hegemony", true, false);
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getMark("beishui") > 0)
                    room->setPlayerMark(p, "beishui", 0);
            }
        }
        //record for ai, since AI prefer use a specific card,  but not the SkillCard QijiCard.
        if (e == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == objectName())
                room->setPlayerMark(use.from, "beishui", 1);
        }
        if (e == CardResponded) {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if ((response.m_from != nullptr) && response.m_isUse && !response.m_isProvision && (response.m_card != nullptr) && response.m_card->getSkillName() == objectName())
                room->setPlayerMark(response.m_from, "beishui", 1);
        }
    }
};

class HezhouHegemonyVS : public ViewAsSkill
{
public:
    HezhouHegemonyVS()
        : ViewAsSkill("hezhou_hegemony")
    {
        response_or_use = true;
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        if (selected.length() == 0)
            return true;
        else if (selected.length() == 1) {
            if (to_select->getTypeId() == selected.first()->getTypeId())
                return false;
            else {
                QList<int> ids = Self->getPile("wooden_ox");
                if (to_select->isKindOf("WoodenOx") && ids.contains(selected.first()->getId()))
                    return false;
                else if (selected.first()->isKindOf("WoodenOx") && ids.contains(to_select->getId()))
                    return false;
                else
                    return true;
            }
        } else
            return false;
    }

    bool isEnabledAtPlay(const Player *) const override
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        Peach *card = new Peach(Card::SuitToBeDecided, -1);
        DELETE_OVER_SCOPE(Peach, card)
        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);

        return cardPattern != nullptr && cardPattern->match(player, card) && !player->isCurrent() && player->getMark("Global_PreventPeach") == 0
            && (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE);
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() != 2)
            return nullptr;
        Peach *peach = new Peach(Card::SuitToBeDecided, -1);
        peach->addSubcards(cards);
        peach->setSkillName(objectName());
        return peach;
    }
};

class HezhouHegemony : public TriggerSkill
{
public:
    HezhouHegemony()
        : TriggerSkill("hezhou_hegemony")
    {
        events << CardsMoveOneTime;
        view_as_skill = new HezhouHegemonyVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
        if (player != nullptr && player->isAlive() && player->hasSkill(this) && move.to_place == Player::DiscardPile
            && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE) {
            const Card *card = move.reason.m_extraData.value<const Card *>();
            if ((card != nullptr) && card->getSkillName() == objectName()) {
                foreach (int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->isKindOf("TrickCard") && room->getCardPlace(id) == Player::DiscardPile)
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *player = invoke->invoker;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QString name = "";
        foreach (int id, move.card_ids) {
            if (Sanguosha->getCard(id)->isKindOf("TrickCard"))
                name = Sanguosha->getCard(id)->objectName();
        }
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@hezhou_hegemony:" + name, true, true);
        if (target != nullptr)
            invoke->targets << target;
        return target != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *target = invoke->targets.first();

        QList<int> ids;
        foreach (int id, move.card_ids) {
            if (Sanguosha->getCard(id)->isKindOf("TrickCard") && room->getCardPlace(id) == Player::DiscardPile)
                ids << id;
        }

        move.removeCardIds(ids);
        data = QVariant::fromValue(move);

        CardsMoveStruct mo;
        mo.card_ids = ids;
        mo.to = target;
        mo.to_place = Player::PlaceHand;
        room->moveCardsAtomic(mo, true);

        return false;
    }
};

class MoqiHgemony : public TriggerSkill
{
public:
    MoqiHgemony()
        : TriggerSkill("moqi_hegemony")
    {
        events << CardUsed << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                p->setFlags("-" + objectName());
        }
    }

    static bool can_add(CardUseStruct use)
    {
        return use.card->isNDTrick() && !(use.card->isKindOf("IronChain") || use.card->isKindOf("LureTiger") || use.card->isKindOf("Nullification"));
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging)
            return QList<SkillInvokeDetail>();

        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (!can_add(use))
            return d;

        QList<ServerPlayer *> owners = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *p, owners) {
            if (!p->hasFlag(objectName())) {
                if ((use.from != nullptr) && p->isFriendWith(use.from, true)) {
                    d << SkillInvokeDetail(this, p, p);
                }
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        //add log?
        CardUseStruct use = data.value<CardUseStruct>();
        use.m_effectValue.first()++;
        data = QVariant::fromValue(use);
        room->setPlayerFlag(invoke->invoker, objectName());
        return false;
    }
};

class XuyuHegemony : public TriggerSkill
{
public:
    XuyuHegemony()
        : TriggerSkill("xuyu_hegemony")
    {
        events << CardsMoveOneTime;
        frequency = Compulsory;
        relate_to_place = "head";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *kaguya = qobject_cast<ServerPlayer *>(move.from);

            if ((kaguya != nullptr) && kaguya->isAlive() && kaguya->hasSkill(this) && kaguya->getMark("xuyu_invoked") < 1 && move.from_places.contains(Player::PlaceHand)
                && kaguya->isKongcheng())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, kaguya, kaguya, nullptr, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->setPlayerMark(invoke->invoker, "xuyu_invoked", 1);
        room->doLightbox("$xuyuHegemonyAnimate", 4000);
        invoke->invoker->removeGeneral(!invoke->invoker->inHeadSkills(objectName()));

        // add this to Player::skills, not Player::acquired_skills.
        bool head = invoke->invoker->inHeadSkills(objectName());
        invoke->invoker->addSkill("yongheng", head);
        invoke->invoker->sendSkillsToOthers(head);

        const Skill *skill = Sanguosha->getSkill("yongheng");
        const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
        room->getThread()->addTriggerSkill(trigger_skill);

        QString flag = (head) ? "h" : "d";
        invoke->invoker->setSkillsPreshowed(flag, true);
        invoke->invoker->notifyPreshow();
        //room->setPlayerProperty(invoke->invoker, "general_showed", true);

        //QString skillname = invoke->invoker->inHeadSkills(objectName()) ? "yongheng" : "yongheng!";
        //room->handleAcquireDetachSkills(invoke->invoker, skillname);
        //room->acquireSkill(invoke->invoker, skillname);

        return false;
    }
};

class YaoshiHegemony : public TriggerSkill
{
public:
    YaoshiHegemony()
        : TriggerSkill("yaoshi_hegemony")
    {
        events << DamageInflicted;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if ((damage.from != nullptr) && (damage.card != nullptr) && damage.from->isAlive() && damage.from->hasSkill(this)) //&& damage.card->isKindOf("Slash")
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, false, damage.to);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->sendLog("#yaoshi_log", invoke->targets.first(), objectName(), QList<ServerPlayer *>(), QString::number(damage.damage));

        RecoverStruct recover;
        room->recover(invoke->targets.first(), recover);

        return true;
    }
};

class FengxiangHegemony : public OneCardViewAsSkill
{
public:
    FengxiangHegemony()
        : OneCardViewAsSkill("fengxiang_hegemony")
    {
        filter_pattern = ".|red|.|hand";
        response_or_use = true;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        FireAttack *card = new FireAttack(Card::SuitToBeDecided, -1);
        DELETE_OVER_SCOPE(FireAttack, card)
        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);
        return cardPattern != nullptr && cardPattern->match(player, card)
            && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        if (originalCard != nullptr) {
            FireAttack *card = new FireAttack(Card::SuitToBeDecided, -1);
            card->addSubcard(originalCard);
            card->setSkillName(objectName());
            return card;
        }
        return nullptr;
    }
};

class KaifengHegemony : public TriggerSkill
{
public:
    KaifengHegemony()
        : TriggerSkill("kaifeng_hegemony")
    {
        events << Damaged << DamageCaused;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Fire || (damage.from == nullptr) || damage.from == damage.to)
            return QList<SkillInvokeDetail>();
        if (triggerEvent == DamageCaused) {
            if (damage.from->hasSkill(this) && damage.from->getHp() < damage.to->getHp() && !damage.from->isDead())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, false, damage.to);
        } else if (triggerEvent == Damaged) {
            if (damage.to->hasSkill(this) && damage.to->getHp() < damage.from->getHp() && !damage.to->isDead())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, nullptr, false, damage.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *mokou = invoke->invoker;
        RecoverStruct recover;
        recover.recover = 1;
        room->recover(mokou, recover);
        return false;
    }
};

XushiHegemonyCard::XushiHegemonyCard()
{
    will_throw = true;
    //handling_method = Card::MethodNone;
    m_skillName = "xushi_hegemony";
}

bool XushiHegemonyCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->hasFlag("Global_xushiFailed");
}

void XushiHegemonyCard::use(Room *room, const CardUseStruct &card_use) const
{
    const QList<ServerPlayer *> &targets = card_use.to;

    foreach (ServerPlayer *p, targets)
        room->setPlayerFlag(p, "xushi_cancel");
}

class XushiHegemonyVS : public OneCardViewAsSkill
{
public:
    XushiHegemonyVS()
        : OneCardViewAsSkill("xushi_hegemony")
    {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@xushi_hegemony";
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        XushiHegemonyCard *c = new XushiHegemonyCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class XushiHegemony : public TriggerSkill
{
public:
    XushiHegemony()
        : TriggerSkill("xushi_hegemony")
    {
        events << TargetConfirming;
        view_as_skill = new XushiHegemonyVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.length() < 2)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p->canDiscard(p, "hs"))
                d << SkillInvokeDetail(this, p, p);
        }

        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "@xushi_hegemony_targetchosen:" + use.card->objectName();
        invoke->invoker->tag["xushi_hegemony_use"] = data;
        foreach (ServerPlayer *p, use.to)
            room->setPlayerFlag(p, "Global_xushiFailed");
        const Card *c = room->askForUseCard(invoke->invoker, "@@xushi_hegemony", prompt); //, -1, Card::MethodUse, false
        //ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, use.to, objectName(), prompt, true, true);
        if (c != nullptr) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("xushi_cancel")) {
                    room->setPlayerFlag(p, "-xushi_cancel");
                    invoke->targets << p;
                }
            }
        }
        return c != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();

        ServerPlayer *target = invoke->targets.first();
        use.to.removeAll(target);
        data = QVariant::fromValue(use);

        LogMessage log;
        log.type = "#XushiHegemonySkillAvoid";
        log.from = target;
        log.arg = objectName();
        log.arg2 = use.card->objectName();
        room->sendLog(log);
        return false;
    }
};

class XinyueHegemony : public TriggerSkill
{
public:
    XinyueHegemony()
        : TriggerSkill("xinyue_hegemony")
    {
        events << Damaged << EventPhaseChanging;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if ((damage.from != nullptr) && damage.from->isAlive() && damage.to->isAlive() && damage.to->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, nullptr, false, damage.from);
        } else if (e == EventPhaseChanging) {
            QList<SkillInvokeDetail> d;
            //ServerPlayer *player = data.value<ServerPlayer *>();
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) { //check current is dead
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("xinyue_transform")) {
                        d << SkillInvokeDetail(this, nullptr, p, nullptr, true);
                    }
                }
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent e, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (e == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            QString prompt = "target:" + damage.from->objectName() + "::" + QString::number(invoke->invoker->getHp());
            return invoke->invoker->askForSkillInvoke(objectName(), data, prompt);
        }
        return true;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (e == Damaged) {
            if (invoke->invoker->hasShownGeneral() && invoke->invoker->getGeneralName() == "keine_hegemony")
                room->setPlayerFlag(invoke->invoker, "xinyue_transform");
            else if ((invoke->invoker->getGeneral2() != nullptr) && invoke->invoker->hasShownGeneral2() && invoke->invoker->getGeneral2Name() == "keine_hegemony")
                room->setPlayerFlag(invoke->invoker, "xinyue_transform");
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
            int x = invoke->targets.first()->getHandcardNum() - invoke->invoker->getHp();
            if (x <= 0)
                return false;
            room->askForDiscard(invoke->targets.first(), objectName(), x, x, false, false);
        } else {
            room->setPlayerFlag(invoke->invoker, "-xinyue_transform");
            QStringList generals = room->getTag(invoke->invoker->objectName()).toStringList();
            QString old_general = "keine_hegemony";
            QString new_general = "keine_sp_hegemony";
            bool head = (generals.first() == old_general);
            room->transformGeneral(invoke->invoker, new_general, static_cast<int>(head));
        }
        return false;
    }
};

class WangyueHegemony : public TriggerSkill
{
public:
    WangyueHegemony()
        : TriggerSkill("wangyue_hegemony")
    {
        events << Damaged << EventPhaseChanging;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if ((damage.from != nullptr) && damage.from->isAlive() && damage.to->isAlive() && damage.to->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, nullptr, false, damage.from);
        } else if (e == EventPhaseChanging) {
            QList<SkillInvokeDetail> d;
            //ServerPlayer *player = data.value<ServerPlayer *>();
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) { //check current is dead
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("wangyue_transform"))
                        d << SkillInvokeDetail(this, nullptr, p, nullptr, true);
                }
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent e, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (e == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            int num = qMin(5, damage.from->getHandcardNum());
            QString prompt = "target:" + damage.from->objectName() + "::" + QString::number(num);
            return invoke->invoker->askForSkillInvoke(objectName(), data, prompt);
        }
        return true;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (e == Damaged) {
            if (invoke->invoker->hasShownGeneral() && invoke->invoker->getGeneralName() == "keine_sp_hegemony")
                room->setPlayerFlag(invoke->invoker, "wangyue_transform");
            else if ((invoke->invoker->getGeneral2() != nullptr) && invoke->invoker->hasShownGeneral2() && invoke->invoker->getGeneral2Name() == "keine_sp_hegemony")
                room->setPlayerFlag(invoke->invoker, "wangyue_transform");

            int num = qMin(5, invoke->targets.first()->getHandcardNum());
            if (num <= 0)
                return false;
            int x = num - invoke->invoker->getHandcardNum();
            invoke->invoker->drawCards(x);
        } else {
            room->setPlayerFlag(invoke->invoker, "-wangyue_transform");
            QStringList generals = room->getTag(invoke->invoker->objectName()).toStringList();
            QString old_general = "keine_sp_hegemony";
            QString new_general = "keine_hegemony";
            bool head = (generals.first() == old_general);
            room->transformGeneral(invoke->invoker, new_general, static_cast<int>(head));
        }
        return false;
    }
};

XingyunHegemonyCard::XingyunHegemonyCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
    m_skillName = "xingyun_hegemony";
}

void XingyunHegemonyCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;

    foreach (int id, subcards)
        room->showCard(source, id);
}

class XingyunHegemonyVS : public OneCardViewAsSkill
{
public:
    XingyunHegemonyVS()
        : OneCardViewAsSkill("xingyun_hegemony")
    {
        response_pattern = "@@xingyun_hegemony";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        return selected.isEmpty() && to_select->hasFlag("xingyun");
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        if (originalCard != nullptr) {
            XingyunHegemonyCard *card = new XingyunHegemonyCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return nullptr;
    }
};

class XingyungHegemony : public TriggerSkill
{
public:
    XingyungHegemony()
        : TriggerSkill("xingyun_hegemony")
    {
        events << CardsMoveOneTime;
        view_as_skill = new XingyunHegemonyVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        if (room->getTag("FirstRound").toBool())
            return QList<SkillInvokeDetail>();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *tewi = qobject_cast<ServerPlayer *>(move.to);
        if (tewi != nullptr && tewi->hasSkill(this) && move.to_place == Player::PlaceHand) {
            foreach (int id, move.card_ids) {
                if (Sanguosha->getCard(id)->getSuit() == Card::Heart && room->getCardPlace(id) == Player::PlaceHand) {
                    ServerPlayer *owner = room->getCardOwner(id);
                    if ((owner != nullptr) && owner == tewi)
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, tewi, tewi);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *tewi = invoke->invoker;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        foreach (int id, move.card_ids) {
            if (Sanguosha->getCard(id)->getSuit() == Card::Heart && room->getCardPlace(id) == Player::PlaceHand) {
                ServerPlayer *owner = room->getCardOwner(id);
                if ((owner != nullptr) && owner == tewi)
                    room->setCardFlag(id, "xingyun");
            }
        }
        invoke->invoker->tag["xingyun_move"] = data;
        const Card *c = room->askForUseCard(tewi, "@@xingyun_hegemony", "@xingyun_hegemony");
        foreach (int id, move.card_ids)
            room->setCardFlag(id, "-xingyun");

        return c != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *player = invoke->invoker;
        QString choice = "letdraw";
        //AI:askForChoice or askForPlayerChosen use the "xingyun" AI
        if (player->isWounded())
            choice = room->askForChoice(player, "xingyun", "letdraw+recover", data);
        if (choice == "letdraw") {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), "xingyun", "@xingyun-select");
            target->drawCards(1);
        } else if (choice == "recover") {
            RecoverStruct recover;
            recover.who = player;
            room->recover(player, recover);
        }

        return false;
    }
};

class YueshiHegemony : public Ruizhi
{
public:
    YueshiHegemony()
        : Ruizhi("yueshi_hegemony")
    {
        relate_to_place = "head";
    }
};

class PingyiHegemony : public TriggerSkill
{
public:
    PingyiHegemony()
        : TriggerSkill("pingyi_hegemony")
    {
        events << Damage << Damaged << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("pingyi_used"))
                        room->setPlayerFlag(p, "-pingyi_used");
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        if (e == EventPhaseChanging)
            return QList<SkillInvokeDetail>();

        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *yori = (e == Damage) ? damage.from : damage.to;
        if (yori == nullptr || yori->hasFlag("pingyi_used") || yori->isDead() || !yori->hasSkill(this) || !yori->canDiscard(yori, "hes"))
            return QList<SkillInvokeDetail>();

        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, yori, yori);
    }

    static QStringList GetAvailableGenerals(Room *room, ServerPlayer *yori)
    {
        QList<const General *> all_generals = Sanguosha->findChildren<const General *>();
        //QStringList all = Sanguosha->getPackages  //Sanguosha->getLimitedGeneralNames(); //.toSet()
        QStringList all_shu;
        foreach (const General *general, all_generals) {
            if (general->getKingdom() == "shu")
                all_shu << general->objectName();
        }
        /*foreach(QString g_name, all) {
            const General *general = Sanguosha->getGeneral(g_name);
            if (general->getKingdom() == "shu")
                all_shu << g_name;
        }*/
        //all_shu << "keine_sp_hegemony";

        //all_shu.removeAll("yorihime_hegemony");
        QStringList names = room->getTag(yori->objectName()).toStringList();
        all_shu.removeAll(names.first());
        all_shu.removeAll(names.last());

        foreach (ServerPlayer *player, room->getAllPlayers()) {
            if (player->hasShownGeneral()) {
                QString name1 = player->getGeneralName();
                const General *general = Sanguosha->getGeneral(name1);
                if (general->getKingdom() == "shu")
                    all_shu.removeAll(name1);
            }
            if (player->hasShownGeneral2()) {
                QString name2 = player->getGeneral2Name();
                const General *general = Sanguosha->getGeneral(name2);
                if (general->getKingdom() == "shu")
                    all_shu.removeAll(name2);
            }
        }
        return all_shu;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        return room->askForCard(invoke->invoker, ".|.|.|hand,equipped", "@pingyi_hegemony", data, Card::MethodDiscard, nullptr, false, objectName()) != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        //bool other_general_place = !invoke->invoker->inHeadSkills(objectName());
        //invoke->invoker->showGeneral(other_general_place);
        if (!invoke->invoker->hasSkill("pingyi_hegemony")) //check whether the general yorihime is removed during skill_cost (throw one card)
            return false;
        QStringList choices = GetAvailableGenerals(room, invoke->invoker);
        if (choices.isEmpty())
            return false;

        room->setPlayerFlag(invoke->invoker, "pingyi_used");

        //AI *ai = invoke->invoker->getAI();
        if (!invoke->invoker->isOnline()) { //  ai: Just make a random choice
            //QString general_name = room->askForChoice(invoke->invoker, objectName(), choices.join("+"));
            int idx = qrand() % choices.length();
            QString general_name = choices.at(idx);
            const General *general = Sanguosha->getGeneral(general_name);

            QStringList skill_names;

            foreach (const Skill *skill, general->getVisibleSkillList()) {
                if (skill->isAttachedLordSkill() || skill->getFrequency() == Skill::Limited || skill->relateToPlace(true) || skill->relateToPlace(false))
                    continue;

                skill_names << skill->objectName();
            }
            if (skill_names.isEmpty())
                return false;

            int skill_idx = qrand() % skill_names.length();
            QString skill_name = skill_names.at(skill_idx);
            const Skill *skill = Sanguosha->getSkill(skill_name);
            skillProcess(room, invoke->invoker, general_name, skill);
        } else {
            room->setPlayerFlag(invoke->invoker, "Pingyi_Choose");
            QString general_name = room->askForGeneral(invoke->invoker, choices);
            room->setPlayerFlag(invoke->invoker, "-Pingyi_Choose");
            const General *general = Sanguosha->getGeneral(general_name);

            QStringList skill_names;

            foreach (const Skill *skill, general->getVisibleSkillList()) {
                if (skill->isAttachedLordSkill() || skill->getFrequency() == Skill::Limited || skill->relateToPlace(true) || skill->relateToPlace(false))
                    continue;

                skill_names << skill->objectName();
            }
            if (skill_names.isEmpty())
                return false;

            QString skill_name = room->askForChoice(invoke->invoker, objectName(), skill_names.join("+"));
            const Skill *skill = Sanguosha->getSkill(skill_name);
            skillProcess(room, invoke->invoker, general_name, skill);
        }
        return false;
    }

    static void skillProcess(Room *room, ServerPlayer *yori, QString pingyi_general = QString(), const Skill *skill = nullptr)
    {
        JsonArray arg;
        arg << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
        arg << yori->objectName();
        if (pingyi_general.isEmpty() || skill == nullptr) {
            arg << QString() << QString() << QString() << QString();
        } else {
            if (yori->inHeadSkills("pingyi_hegemony")) {
                arg << pingyi_general;
                arg << skill->objectName();
            }
            arg << QString() << QString();
            if (!yori->inHeadSkills("pingyi_hegemony")) {
                arg << pingyi_general;
                arg << skill->objectName();
            }
        }
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

        if (yori->tag.contains("pingyi_skill") && yori->tag.contains("pingyi_General")) {
            QString originalSkillName = yori->tag.value("pingyi_skill", QString()).toString();

            //ServerPlayer *originalOwner = yori->tag["pingyi_originalOwner"].value<ServerPlayer *>();

            // 1. yorihime lost the acquired skill
            if (!originalSkillName.isEmpty()) {
                if (!yori->inHeadSkills("pingyi_hegemony"))
                    originalSkillName = originalSkillName + "!";
                room->handleAcquireDetachSkills(yori, "-" + originalSkillName, false); //
            }
            yori->tag.remove("pingyi_skill");
            yori->tag.remove("pingyi_General");
            yori->tag.remove("Huashen_skill");
            yori->tag.remove("Huashen_target");
            yori->tag.remove("Huashen_place");
        }

        if (skill != nullptr && !pingyi_general.isEmpty()) {
            yori->tag["pingyi_skill"] = skill->objectName();
            yori->tag["pingyi_General"] = pingyi_general;

            // 2. acquire the skill
            yori->tag["Huashen_skill"] = skill->objectName(); //for marshal
            yori->tag["Huashen_target"] = pingyi_general;

            //QString skillname = yori->inHeadSkills("pingyi_hegemony") ? skill->objectName() : skill->objectName() + "!";
            //room->handleAcquireDetachSkills(yori, skillname, true); //need rewrite handleAcquireDetachSkills

            bool head = yori->inHeadSkills("pingyi_hegemony");
            QString place = (head) ? "head" : "deputy";
            yori->tag["Huashen_place"] = place;

            bool game_start = false;
            // process main skill
            yori->addSkill(skill->objectName(), head);
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
            if (trigger_skill != nullptr) {
                room->getThread()->addTriggerSkill(trigger_skill);
                if (trigger_skill->getTriggerEvents().contains(GameStart)) {
                    game_start = true;
                }
            }
            SkillAcquireDetachStruct s;
            s.isAcquire = true;
            s.player = yori;
            s.skill = skill;
            QVariant data = QVariant::fromValue(s);
            room->getThread()->trigger(EventAcquireSkill, room, data);

            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_ACQUIRE_SKILL << yori->objectName() << skill->objectName() << head;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

            //process related skill
            foreach (const Skill *related_skill, Sanguosha->getRelatedSkills(skill->objectName())) {
                if (!related_skill->isVisible()) { //??
                    //acquireSkill(player, related_skill);
                    yori->addSkill(related_skill->objectName(), head);
                    const TriggerSkill *tr = qobject_cast<const TriggerSkill *>(related_skill);
                    if (tr != nullptr) {
                        room->getThread()->addTriggerSkill(tr);
                        if (tr->getTriggerEvents().contains(GameStart))
                            game_start = true;
                    }
                    SkillAcquireDetachStruct s;
                    s.isAcquire = true;
                    s.player = yori;
                    s.skill = related_skill;
                    QVariant data = QVariant::fromValue(s);
                    room->getThread()->trigger(EventAcquireSkill, room, data);
                }
            }

            yori->sendSkillsToOthers(head);
            QString flag = (head) ? "h" : "d";
            yori->setSkillsPreshowed(flag, true);
            yori->notifyPreshow();

            //trigger game start
            if (game_start) {
                QVariant v = QVariant::fromValue(yori);
                room->getThread()->trigger(GameStart, room, v);
            }
        }
    }
};

class PingyiHegemonyHandler : public TriggerSkill
{
public:
    PingyiHegemonyHandler()
        : TriggerSkill("#pingyi_hegemony")
    {
        events << GeneralShown << GeneralHidden << GeneralRemoved;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == GeneralShown) {
            ShowGeneralStruct s = data.value<ShowGeneralStruct>();
            ServerPlayer *target = s.player;

            if ((target != nullptr) && target->isAlive()) {
                QString name = s.isHead ? s.player->getGeneralName() : s.player->getGeneral2Name();
                QList<SkillInvokeDetail> d;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    QString who = p->tag.value("pingyi_General", QString()).toString();
                    if (who == name)
                        d << SkillInvokeDetail(this, nullptr, p, nullptr, true);
                }
                return d;
            }
        }
        if (triggerEvent == GeneralHidden || triggerEvent == GeneralRemoved) {
            ShowGeneralStruct s = data.value<ShowGeneralStruct>();
            ServerPlayer *target = s.player;
            if ((target != nullptr) && target->isAlive()) {
                //QStringList names = room->getTag(target->objectName()).toStringList();
                //QString name = s.isHead ? names.first() : names.last();
                bool invoke = true;
                if (target->hasShownGeneral() && target->getGeneralName() == "yorihime_hegemony")
                    invoke = false;
                if ((target->getGeneral2() != nullptr) && target->hasShownGeneral2() && target->getGeneral2Name() == "yorihime_hegemony")
                    invoke = false;
                if (invoke) {
                    QString who = target->tag.value("pingyi_General", QString()).toString();
                    if (!who.isEmpty())
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, nullptr, target, nullptr, true);
                }
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        PingyiHegemony::skillProcess(room, invoke->invoker);
        return false;
    }
};

class YinghuoHegemony : public TriggerSkill
{
public:
    YinghuoHegemony()
        : TriggerSkill("yinghuo_hegemony")
    {
        events << CardUsed << CardResponded;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if ((use.from != nullptr) && use.from->isAlive() && use.from->hasSkill(this) && (use.card != nullptr) && use.card->getTypeId() == Card::TypeBasic) {
                if (use.to.length() == 1 && use.to.first() == use.from)
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
            }
        } else {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if ((response.m_from != nullptr) && response.m_from->isAlive() && response.m_from->hasSkill(this) && response.m_isUse && (response.m_card != nullptr)
                && response.m_card->getTypeId() == Card::TypeBasic)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, response.m_from, response.m_from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->drawCards(1);
        return false;
    }
};

class ChongqunHegemony : public TriggerSkill
{
public:
    ChongqunHegemony()
        : TriggerSkill("chongqun_hegemony")
    {
        events << CardsMoveOneTime;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
        if (player != nullptr && player->isAlive() && player->hasSkill(this) && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            bool can = false;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (player->canDiscard(p, "hs")) {
                    can = true;
                    break;
                }
            }
            if (!can)
                return d;

            foreach (int id, move.card_ids) {
                if (Sanguosha->getCard(id)->getTypeId() == Card::TypeBasic) {
                    d << SkillInvokeDetail(this, player, player);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        // how to notice player the remain times?
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (invoke->invoker->canDiscard(p, "hs"))
                targets << p;
        }
        if (targets.isEmpty())
            return false;
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@chongqun_target", true, true);
        if (target != nullptr) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hs", objectName());
        room->throwCard(id, invoke->targets.first(), invoke->invoker);
        return false;
    }
};

class ZhenyeHegemony : public TriggerSkill
{
public:
    ZhenyeHegemony()
        : TriggerSkill("zhenye_hegemony")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *nokia = data.value<ServerPlayer *>();
        if (nokia->hasSkill(this) && nokia->getPhase() == Player::Finish)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, nokia, nokia);

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(), "@zhenye-select-heg", true, true);
        if (target != nullptr) {
            invoke->targets << target;
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->targets.first()->turnOver();
        int num = 1;
        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (invoke->invoker->isFriendWith(p))
                num++;
        }
        invoke->targets.first()->drawCards(num);
        invoke->invoker->turnOver();

        return false;
    }
};

//********  AUTUMN   **********

QiankunHegemony::QiankunHegemony(const QString &owner)
    : MaxCardsSkill("qiankun_" + owner)
{
}

int QiankunHegemony::getExtra(const Player *target) const
{
    if (target->hasSkill(objectName()) && target->hasShownSkill(objectName()))
        return 2;
    else
        return 0;
}

class ChuanchengHegemony : public TriggerSkill
{
public:
    ChuanchengHegemony()
        : TriggerSkill("chuancheng_hegemony")
    {
        events << Death;
    }

    bool canPreshow() const override
    {
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who->hasSkill(objectName())) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(death.who)) {
                if (death.who->isFriendWith(p))
                    targets << p;
            }
            if (!targets.isEmpty())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, death.who, death.who, targets);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, invoke->targets, objectName(), "@chuancheng_hegemony", true, true);
        if (target != nullptr) {
            invoke->targets.clear();
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = invoke->targets.first();
        room->handleAcquireDetachSkills(target, "qiankun_suwako");
        room->handleAcquireDetachSkills(target, "chuancheng_hegemony");
        if (invoke->invoker->getCards("hejs").length() > 0) {
            DummyCard *allcard = new DummyCard;
            allcard->deleteLater();
            allcard->addSubcards(invoke->invoker->getCards("hejs"));
            room->obtainCard(target, allcard, CardMoveReason(CardMoveReason::S_REASON_RECYCLE, target->objectName()), false);
        }
        return false;
    }
};

ShowFengsuCard::ShowFengsuCard()
    : SkillCard()
{
    mute = true;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

const Card *ShowFengsuCard::validate(CardUseStruct &card_use) const
{
    bool head = card_use.from->inHeadSkills("fengsu");
    card_use.from->showGeneral(head);
    return nullptr;
}

class FengsuAttach : public ViewAsSkill
{
public:
    FengsuAttach()
        : ViewAsSkill("fengsu_attach")
    {
        attached_lord_skill = true;
    }

    bool shouldBeVisible(const Player *Self) const override
    {
        return Self != nullptr;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasShownSkill("fengsu");
    }

    bool viewFilter(const QList<const Card *> &, const Card *) const override
    {
        return false;
    }

    const Card *viewAs(const QList<const Card *> &) const override
    {
        return new ShowFengsuCard();
    }
};

class FengsuHegemonyHandler : public TriggerSkill
{
public:
    FengsuHegemonyHandler()
        : TriggerSkill("#fengsu_hegemony")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Death << Debut << EventSkillInvalidityChange;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const override
    {
        if (!isHegemonyGameMode(ServerInfo.GameMode))
            return;

        if (triggerEvent == GameStart || triggerEvent == Debut || triggerEvent == EventAcquireSkill) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if ((p->hasSkill("fengsu", true) || p->ownSkill("fengsu")) && !p->hasSkill("fengsu_attach")) {
                    room->attachSkillToPlayer(p, "fengsu_attach");
                }
            }
        }
        if (triggerEvent == Death || triggerEvent == EventLoseSkill) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if ((!p->hasSkill("fengsu", true) && !p->ownSkill("fengsu")) && p->hasSkill("fengsu_attach"))
                    room->detachSkillFromPlayer(p, "fengsu_attach", true);
            }
        }
    }
};

class DuxinHegemony : public TriggerSkill
{
public:
    DuxinHegemony()
        : TriggerSkill("duxin_hegemony")
    {
        events << TargetConfirmed;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("SkillCard") || use.from == nullptr || use.to.length() != 1 || use.from == use.to.first())
            return QList<SkillInvokeDetail>();

        ServerPlayer *satori = use.to.first();
        if (satori->hasSkill(objectName()) && (use.from->getGeneral2() != nullptr) && !use.from->hasShownGeneral2()) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, satori, satori, nullptr, true, use.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QStringList list = room->getTag(invoke->targets.first()->objectName()).toStringList();
        //list.removeAt(choice == "showhead" ? 1 : 0);
        list.removeAt(0); //remove head
        foreach (const QString &name, list) {
            LogMessage log;
            log.type = "$KnownBothViewGeneral";
            log.from = invoke->invoker;
            log.to << invoke->targets.first();
            log.arg = name;
            log.arg2 = invoke->targets.first()->getRole();
            room->doNotify(invoke->invoker, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
        }
        JsonArray arg;
        arg << objectName();
        arg << JsonUtils::toJsonArray(list);
        room->doNotify(invoke->invoker, QSanProtocol::S_COMMAND_VIEW_GENERALS, arg);
        return false;
    }
};

class WunianHgemony : public TriggerSkill
{
public:
    WunianHgemony()
        : TriggerSkill("wunian_hegemony")
    {
        events << Predamage << TargetConfirming;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        if (e == Predamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if ((damage.from != nullptr) && damage.from->hasSkill(objectName()))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, true);
        } else if (e == TargetConfirming) {
            QList<SkillInvokeDetail> d;
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() == Card::TypeTrick) {
                foreach (ServerPlayer *p, use.to) {
                    if (p->hasSkill(this) && p->isWounded() && use.from != nullptr && use.from != p)
                        d << SkillInvokeDetail(this, p, p, nullptr, true);
                }
            }
            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        //for AI
        if (e == TargetConfirming)
            room->setTag("wunian_hegemony_use", data);

        return (invoke->invoker->hasShownSkill(this) || invoke->invoker->askForSkillInvoke(this, data));
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (e == Predamage) {
            DamageStruct damage = data.value<DamageStruct>();

            ServerPlayer *target
                = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(), "@wunian_transfer:" + damage.to->objectName(), false, true);
            damage.from = target;
            damage.transfer = true;

            data = QVariant::fromValue(damage);
        } else if (e == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.to.removeAll(invoke->invoker);
            data = QVariant::fromValue(use);
            LogMessage log;
            log.type = "#SkillAvoid";
            log.from = invoke->invoker;
            log.arg = objectName();
            log.arg2 = use.card->objectName();
            room->notifySkillInvoked(invoke->invoker, objectName());
            room->sendLog(log);
        }
        return false;
    }
};

class ChuanranHgemony : public TriggerSkill
{
public:
    ChuanranHgemony()
        : TriggerSkill("chuanran_hegemony")
    {
        events << Damaged;
        //frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<ServerPlayer *> yamames = room->findPlayersBySkillName(objectName());
        if (yamames.isEmpty() || damage.nature != DamageStruct::Normal || (damage.from == nullptr) || damage.from->isDead() || (damage.card == nullptr)
            || !damage.card->isKindOf("Slash") || damage.to->isChained() || damage.to->isDead())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, yamames) {
            if ((p == damage.from || p->isFriendWith(damage.from, true) || damage.from->isChained()) && damage.to != p) {
                d << SkillInvokeDetail(this, p, p, nullptr, false, damage.to);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->notifySkillInvoked(invoke->owner, objectName());
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());

        room->setPlayerProperty(invoke->targets.first(), "chained", true);
        return false;
    }
};

class DiaopingHegemony : public TriggerSkill
{
public:
    DiaopingHegemony()
        : TriggerSkill("diaoping_hegemony")
    {
        events << TargetSpecified;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash") || use.from == nullptr || use.from->isDead())
            return QList<SkillInvokeDetail>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *kisume, room->findPlayersBySkillName(objectName())) {
            if (kisume == use.from)
                continue;
            foreach (ServerPlayer *p, use.to) {
                if (kisume->isFriendWith(p) || kisume == p) {
                    if (kisume->getHandcardNum() > 0 && use.from->getHandcardNum() > 0 && kisume != use.from)
                        d << SkillInvokeDetail(this, kisume, kisume);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *skillowner = invoke->invoker;
        QString prompt = "slashtarget:" + use.from->objectName() + ":" + use.card->objectName();
        skillowner->tag["diaoping_slash"] = data;

        return room->askForSkillInvoke(skillowner, objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), use.from->objectName());

        if (invoke->invoker->pindian(use.from, objectName(), nullptr)) {
            if (!use.from->isChained())
                room->setPlayerProperty(use.from, "chained", true);
            use.nullified_list << "_ALL_TARGETS";
            data = QVariant::fromValue(use);
        } else {
            QList<ServerPlayer *> l = QList<ServerPlayer *>() << invoke->invoker << use.from;
            room->sortByActionOrder(l);
            room->drawCards(l, 1, objectName());
        }
        return false;
    }
};

class TongjuHegemony : public TriggerSkill
{
public:
    TongjuHegemony()
        : TriggerSkill("tongju_hegemony")
    {
        events << TargetConfirming;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("SavageAssault") || use.card->isKindOf("IronChain") || use.card->isKindOf("ArcheryAttack")) {
            foreach (ServerPlayer *p, use.to) {
                if (p->hasSkill(this))
                    d << SkillInvokeDetail(this, p, p, nullptr, true);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->notifySkillInvoked(invoke->invoker, objectName());

        CardUseStruct use = data.value<CardUseStruct>();
        use.to.removeAll(invoke->invoker);
        data = QVariant::fromValue(use);
        LogMessage log;
        log.type = "#SkillAvoid";
        log.from = invoke->invoker;
        log.arg = objectName();
        log.arg2 = use.card->objectName();
        room->sendLog(log);

        return false;
    }
};

class CuijiHegemony : public TriggerSkill
{
public:
    CuijiHegemony()
        : TriggerSkill("cuiji_hegemony")
    {
        events << DrawNCards;
        relate_to_place = "deputy";
    }

    static void do_cuiji(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        QString choice = room->askForChoice(player, "cuiji_hegemony_suit", "club+diamond+heart+spade+basic+nonbasic");
        //QString pattern = ".|" + choice;
        //if (choice.contains("basic"))
        //    pattern = QString(choice.startsWith("non") ? "^" : "") + "BasicCard";
        room->sendLog("#cuiji_choice", player, "cuiji_hegemony", QList<ServerPlayer *>(), choice); //"cuiji_hegemony:" + choice
        //room->notifySkillInvoked(player, "cuiji_hegemony");
        int acquired = 0;
        QList<int> throwIds;
        while (acquired < 1) {
            int id = room->drawCard();
            CardsMoveStruct move(id, nullptr, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName()));
            move.reason.m_skillName = "cuiji_hegemony";
            room->moveCardsAtomic(move, true);
            room->getThread()->delay();
            Card *card = Sanguosha->getCard(id);
            bool matched = false;
            if (card->getSuitString() == choice)
                matched = true;
            else if (card->getType() == choice)
                matched = true;
            else if (choice == "nonbasic" && card->getType() != "basic")
                matched = true;
            // if (card->match(pattern))   //only match objectname or basiccard
            if (matched) {
                acquired = acquired + 1;
                CardsMoveStruct move2(id, player, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_GOTBACK, player->objectName()));
                room->moveCardsAtomic(move2, true);

                if (!throwIds.isEmpty()) {
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "cuiji_hegemony", QString());
                    DummyCard dummy(throwIds);
                    room->throwCard(&dummy, reason, nullptr);
                    throwIds.clear();
                }
            } else
                throwIds << id;
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DrawNCardsStruct s = data.value<DrawNCardsStruct>();
        if (s.player->hasSkill(this) && s.n > 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, s.player, s.player);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        DrawNCardsStruct draw = data.value<DrawNCardsStruct>();
        invoke->invoker->tag["cuiji_hegemony"] = 1;
        draw.n = draw.n - 1;
        data = QVariant::fromValue(draw);
        return false;
    }
};

class CuijiHEffect : public TriggerSkill
{
public:
    CuijiHEffect()
        : TriggerSkill("#cuiji_hegemony")
    {
        events << AfterDrawNCards;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DrawNCardsStruct dc = data.value<DrawNCardsStruct>();
        int num = dc.player->tag["cuiji_hegemony"].toInt();
        if (num > 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dc.player, dc.player, nullptr, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->tag.remove("cuiji_hegemony");
        CuijiHegemony::do_cuiji(invoke->invoker);
        return false;
    }
};

// taken from LijianCard from original QSanguosha -- Target 2 is the target of KnownBoth, Target 1 is the user
KuaizhaoHegemonyCard::KuaizhaoHegemonyCard()
{
    m_skillName = "kuaizhao_hegemony";
    sort_targets = false;
}

bool KuaizhaoHegemonyCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    KnownBothHegemony *trick = new KnownBothHegemony(Card::NoSuit, 0);
    trick->setCanRecast(false);
    trick->deleteLater();

    if (targets.length() == 0)
        return !to_select->isCardLimited(trick, Card::MethodUse);
    else if (targets.length() == 1)
        return trick->targetFilter(QList<const Player *>(), to_select, targets.first()) && !targets.first()->isProhibited(to_select, trick);

    return false;
}

bool KuaizhaoHegemonyCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void KuaizhaoHegemonyCard::use(Room *room, const CardUseStruct &card_use) const
{
    const QList<ServerPlayer *> &targets = card_use.to;

    ServerPlayer *to = targets.at(1);
    ServerPlayer *from = targets.at(0);

    KnownBothHegemony *trick = new KnownBothHegemony(Card::NoSuit, 0);
    trick->setSkillName("_kuaizhao_hegemony");
    if (!from->isCardLimited(trick, Card::MethodUse) && !from->isProhibited(to, trick))
        room->useCard(CardUseStruct(trick, from, to));
    else
        delete trick;
}

class KuaizhaoHegemony : public ZeroCardViewAsSkill
{
public:
    KuaizhaoHegemony()
        : ZeroCardViewAsSkill("kuaizhao_hegemony")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("KuaizhaoHegemonyCard");
    }

    const Card *viewAs() const override
    {
        return new KuaizhaoHegemonyCard;
    }
};

//********  WINTER   **********

class ShihuiHegemony : public TriggerSkill
{
public:
    ShihuiHegemony()
        : TriggerSkill("shihui_hegemony")
    {
        events << Damage << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        if (e == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if ((damage.from == nullptr) || damage.from->isDead() || damage.card == nullptr || !damage.card->isKindOf("Slash"))
                return QList<SkillInvokeDetail>();

            AwaitExhaustedHegemony *card = new AwaitExhaustedHegemony(Card::SuitToBeDecided, -1);
            card->setSkillName(objectName());
            card->deleteLater();
            if (damage.from->hasSkill(this) && !damage.from->isCardLimited(card, Card::HandlingMethod::MethodUse))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);
        } else if (e == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->isDead() || damage.card == nullptr || !damage.card->isKindOf("Slash"))
                return QList<SkillInvokeDetail>();

            AwaitExhaustedHegemony *card = new AwaitExhaustedHegemony(Card::SuitToBeDecided, -1);
            card->setSkillName(objectName());
            card->deleteLater();

            if (damage.to->hasSkill(this) && !damage.to->isCardLimited(card, Card::HandlingMethod::MethodUse))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        AwaitExhaustedHegemony *card = new AwaitExhaustedHegemony(Card::SuitToBeDecided, -1);
        card->setSkillName(objectName());
        room->useCard(CardUseStruct(card, invoke->invoker, nullptr), false);

        return false;
    }
};

class HezouHegemonyVS : public OneCardViewAsSkill
{
public:
    HezouHegemonyVS()
        : OneCardViewAsSkill("hezou_hegemony")
    {
        filter_pattern = "TrickCard,EquipCard";
        response_or_use = true;
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

    const Card *viewAs(const Card *originalCard) const override
    {
        if (originalCard != nullptr) {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName(objectName());
            return slash;
        }
        return nullptr;
    }
};

class HezouHegemony : public TriggerSkill
{
public:
    HezouHegemony()
        : TriggerSkill("hezou_hegemony")
    {
        events << CardUsed << CardResponded;
        view_as_skill = new HezouHegemonyVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *player = nullptr;
        const Card *card = nullptr;
        if (triggerEvent == CardUsed) {
            player = data.value<CardUseStruct>().from;
            card = data.value<CardUseStruct>().card;
        } else {
            CardResponseStruct response = data.value<CardResponseStruct>();
            player = response.m_from;
            //if (response.m_isUse)
            card = response.m_card;
        }
        if ((player != nullptr) && (card != nullptr) && card->getSkillName() == objectName()) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                QList<const Card *> cards = p->getCards("ej");
                foreach (const Card *c, cards) {
                    if (c->getSuit() == card->getSuit())
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, nullptr, true);
                }
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->sendLog("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        invoke->invoker->drawCards(1);
        return false;
    }
};

class XianlingHegemony : public MaxCardsSkill
{
public:
    XianlingHegemony()
        : MaxCardsSkill("xianling_hegemony")
    {
    }

    int getExtra(const Player *target) const override
    {
        int num = 0;
        if (target->hasSkill(objectName()) && target->hasShownSkill(objectName())) {
            ++num;
        }

        foreach (const Player *p, target->getAliveSiblings()) {
            if (target->isFriendWith(p) && p->hasSkill(objectName()) && p->hasShownSkill(objectName()))
                ++num;
        }
        return num;
    }
};

class JizouHegemony : public TriggerSkill
{
public:
    JizouHegemony()
        : TriggerSkill("jizou_hegemony")
    {
        events << Damage;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from == nullptr || damage.to->isDead() || damage.from->isDead())
            return QList<SkillInvokeDetail>();
        if (damage.from == damage.to || !damage.from->hasSkill(this))
            return QList<SkillInvokeDetail>();

        Dismantlement *card = new Dismantlement(Card::NoSuit, 0);
        card->setSkillName(objectName());
        card->deleteLater();

        if (damage.from->isCardLimited(card, Card::MethodUse) || damage.from->isProhibited(damage.to, card))
            return QList<SkillInvokeDetail>();
        if (card->targetFilter(QList<const Player *>(), damage.to, damage.from))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, false, damage.to);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        Dismantlement *card = new Dismantlement(Card::NoSuit, 0);
        card->setSkillName(objectName());
        card->deleteLater();

        room->useCard(CardUseStruct(card, invoke->invoker, invoke->targets.first()), false);
        return false;
    }
};

class GuanlingHegemony : public TargetModSkill
{
public:
    GuanlingHegemony()
        : TargetModSkill("guanling_hegemony")
    {
        pattern = "Slash";
    }

    int getResidueNum(const Player *from, const Card *) const override
    {
        int num = 0;
        if (from->hasSkill(objectName())) // && from->hasShownSkill(objectName())
            ++num;

        foreach (const Player *p, from->getAliveSiblings()) {
            if (from->isFriendWith(p) && p->hasSkill(objectName()) && p->hasShownSkill(objectName()))
                ++num;
        }
        return num;
    }
};

class JianlingHegemony : public TriggerSkill
{
public:
    JianlingHegemony()
        : TriggerSkill("jianling_hegemony")
    {
        events << DrawNCards;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        DrawNCardsStruct qnum = data.value<DrawNCardsStruct>();
        if (qnum.player->hasSkill(objectName()))
            d << SkillInvokeDetail(this, qnum.player, qnum.player, nullptr, qnum.player->hasShownSkill(this));

        foreach (ServerPlayer *p, room->getOtherPlayers(qnum.player)) {
            if (qnum.player->isFriendWith(p) && p->hasSkill(objectName()) && p->hasShownSkill(this))
                d << SkillInvokeDetail(this, p, qnum.player, nullptr, true);
        }

        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DrawNCardsStruct qnum = data.value<DrawNCardsStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->owner->objectName(), qnum.player->objectName());
        qnum.n = qnum.n + 1;
        data = QVariant::fromValue(qnum);
        room->notifySkillInvoked(invoke->owner, objectName());
        room->sendLog("#TriggerSkill", invoke->owner, objectName());
        return false;
    }
};

class BaochunHegemony : public TriggerSkill
{
public:
    BaochunHegemony()
        : TriggerSkill("baochun_hegemony")
    {
        events << Damaged << HpRecover;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e == Damaged) {
            QList<SkillInvokeDetail> d;
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->isAlive()) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (damage.to == p) {
                        if (damage.to->hasSkill(this))
                            d << SkillInvokeDetail(this, damage.to, damage.to, nullptr, true);
                    } else {
                        if (p->hasSkill(this) && p->isFriendWith(damage.to, true))
                            d << SkillInvokeDetail(this, p, p, nullptr, true);
                    }
                }
            }
            return d;
        } else if (e == HpRecover) {
            RecoverStruct r = data.value<RecoverStruct>();
            if (r.to->isAlive() && r.to->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, r.to, r.to, nullptr, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (e == HpRecover)
            room->setPlayerFlag(invoke->invoker, "Global_baochunAIFailed");
        return (invoke->invoker->hasShownSkill(this) || invoke->invoker->askForSkillInvoke(this, data));
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        bool draw = true;
        if (e == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (invoke->invoker != damage.to)
                draw = false;
        }
        room->sendLog("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        if (!draw) {
            if (invoke->invoker->canDiscard(invoke->invoker, "hes"))
                room->askForDiscard(invoke->invoker, objectName(), 1, 1, false, true);
        } else {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (invoke->invoker->isFriendWith(p, true))
                    targets << p;
            }
            foreach (ServerPlayer *p, targets) {
                if (p->isAlive())
                    p->drawCards(1);
            }
        }

        return false;
    }
};

ChunhenHegemonyCard::ChunhenHegemonyCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "chunhen_hegemony";
}

void ChunhenHegemonyCard::use(Room *room, const CardUseStruct &card_use) const
{
    const QList<ServerPlayer *> &targets = card_use.to;

    room->obtainCard(targets.first(), this);
}

class ChunhenHegemonyVS : public ViewAsSkill
{
public:
    ChunhenHegemonyVS()
        : ViewAsSkill("chunhen_hegemony")
    {
        expand_pile = "*chunhen_temp";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const override
    {
        return StringList2IntList(Self->property("chunhen_temp").toString().split("+")).contains(to_select->getId());
    }

    bool isEnabledAtPlay(const Player *) const override
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const override
    {
        return pattern.startsWith("@@chunhen");
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.isEmpty())
            return nullptr;
        ChunhenHegemonyCard *card = new ChunhenHegemonyCard;
        card->addSubcards(cards);
        return card;
    }
};

class ChunhenHegemony : public TriggerSkill
{
public:
    ChunhenHegemony()
        : TriggerSkill("chunhen_hegemony")
    {
        events << CardsMoveOneTime;
        view_as_skill = new ChunhenHegemonyVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
        if (player == nullptr || !player->hasSkill(this) || player->isDead())
            return QList<SkillInvokeDetail>();

        if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            if ((move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip)) && move.to_place == Player::DiscardPile) {
                QList<int> ids;
                foreach (int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->isRed() && room->getCardPlace(id) == Player::DiscardPile)
                        ids << id;
                }
                if (ids.isEmpty())
                    return QList<SkillInvokeDetail>();

                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> disable; //record cards into discarplie while this process. like yongheng
        while (true) {
            QList<int> ids;
            foreach (int id, move.card_ids) {
                if (Sanguosha->getCard(id)->isRed() && room->getCardPlace(id) == Player::DiscardPile && !disable.contains(id))
                    ids << id;
            }
            if (ids.isEmpty())
                return false;

            invoke->invoker->tag["chunhen_cards"] = IntList2VariantList(ids);
            room->setPlayerProperty(invoke->invoker, "chunhen_temp", IntList2StringList(ids).join("+"));
            const Card *usecard = room->askForUseCard(invoke->invoker, "@@chunhen_hegemony", "@chunhen_give", -1, Card::MethodNone);
            room->setPlayerProperty(invoke->invoker, "chunhen_temp", QString());

            if (usecard != nullptr)
                disable += usecard->getSubcards();
            else
                return false;
        }

        return false;
    }
};

class HanboHegemony : public TriggerSkill
{
public:
    HanboHegemony()
        : TriggerSkill("hanbo_hegemony")
    {
        events << DamageCaused;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if ((damage.from != nullptr) && damage.from->hasSkill(this) && damage.nature == DamageStruct::Normal && damage.from != damage.to && damage.to->isAlive()
            && damage.to->isKongcheng())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, true, damage.to);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *target = invoke->targets.first();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), target->objectName());
        DamageStruct damage = data.value<DamageStruct>();

        LogMessage log;
        log.type = "#HanboEffect";
        log.from = invoke->invoker;
        log.to << target;
        log.arg = QString::number(damage.damage);
        log.arg2 = QString::number(++damage.damage);
        room->sendLog(log);

        data = QVariant::fromValue(damage);
        return false;
    }
};

DongzhiHegemonyCard::DongzhiHegemonyCard()
{
    m_skillName = "dongzhi_hegemony";
}

bool DongzhiHegemonyCard::targetFilter(const QList<const Player *> &selected, const Player *to_select, const Player *) const
{
    if (to_select->isNude())
        return false;
    if (selected.isEmpty())
        return to_select->hasShownOneGeneral();
    else
        return to_select->hasShownOneGeneral() && selected.first()->isFriendWith(to_select);
}

void DongzhiHegemonyCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->doLightbox("$dongzhiAnimate", 4000);
    SkillCard::onUse(room, card_use);
}

void DongzhiHegemonyCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    const QList<ServerPlayer *> &targets = card_use.to;

    room->removePlayerMark(source, "@dongzhi");

    QString flag = "hes";
    foreach (ServerPlayer *p, targets) {
        int num = qMin(p->getEquips().length() + 1, p->getCards(flag).length());
        QList<int> disable;
        DummyCard *dummy = new DummyCard;
        for (int i = 0; i < num; i += 1) {
            int card_id = room->askForCardChosen(source, p, flag, objectName(), false, Card::MethodDiscard, disable);
            //"dongzhi_hegemony"
            disable << card_id;
            dummy->addSubcard(card_id);

            if (p->getCards(flag).length() - disable.length() <= 0)
                break;
        }
        room->throwCard(dummy, p, source);
        delete dummy;
    }
}

class DongzhiHegemony : public ZeroCardViewAsSkill
{
public:
    DongzhiHegemony()
        : ZeroCardViewAsSkill("dongzhi_hegemony")
    {
        limit_mark = "@dongzhi";
        frequency = Limited;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return player->getMark("@dongzhi") > 0;
    }

    const Card *viewAs() const override
    {
        return new DongzhiHegemonyCard;
    }
};

class DongjieHegemony : public TriggerSkill
{
public:
    DongjieHegemony()
        : TriggerSkill("dongjie_hegemony")
    {
        events << DamageCaused; //<< EventPhaseChanging
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if ((damage.from != nullptr) && damage.from->hasSkill(this) && (damage.card != nullptr) && damage.card->isKindOf("Slash"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, false, damage.to);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), damage.to->objectName());

        //invoke->invoker->setFlags(objectName());

        if (room->askForCard(damage.to, ".|.|.|hand", "@dongjie_discard:" + damage.from->objectName(), data, Card::MethodDiscard) == nullptr) {
            damage.to->drawCards(1);
            damage.to->turnOver();
            return true;
        }
        return false;
    }
};

class BingpoHgemony : public TriggerSkill
{
public:
    BingpoHgemony()
        : TriggerSkill("bingpo_hegemony")
    {
        events << Dying;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.who->hasSkill(this) && dying.who->getHp() < dying.who->dyingThreshold() && (dying.damage == nullptr || dying.damage->nature != DamageStruct::Fire))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dying.who, dying.who, nullptr, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->sendLog("#bingpo_hegemony_log", invoke->invoker, objectName(), QList<ServerPlayer *>(), QString::number(1));
        room->notifySkillInvoked(invoke->invoker, objectName());
        RecoverStruct recover;
        room->recover(invoke->invoker, recover);
        return false;
    }
};

class JuxianHegemony : public TriggerSkill
{
public:
    JuxianHegemony()
        : TriggerSkill("juxian_hegemony")
    {
        events << Dying;
        frequency = Limited;
        limit_mark = "@juxian";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DyingStruct dying = data.value<DyingStruct>();

        if (dying.who->hasSkill(this) && dying.who->isAlive() && dying.who->getHp() < dying.who->dyingThreshold() && dying.who->getMark("@juxian") > 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dying.who, dying.who);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->removePlayerMark(invoke->invoker, "@juxian");
        room->doLightbox("$juxianAnimate", 4000);

        QList<int> list = room->getNCards(3);
        CardsMoveStruct move(list, nullptr, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, invoke->invoker->objectName(), objectName(), QString()));
        room->moveCardsAtomic(move, true);

        int s = 0;
        int h = 0;
        int c = 0;
        int d = 0;
        foreach (int id, list) {
            Card *card = Sanguosha->getCard(id);
            if (card->getSuit() == Card::Spade)
                s = 1;
            else if (card->getSuit() == Card::Heart)
                h = 1;
            else if (card->getSuit() == Card::Club)
                c = 1;
            else if (card->getSuit() == Card::Diamond)
                d = 1;
        }

        DummyCard dummy(list);
        invoke->invoker->obtainCard(&dummy);

        RecoverStruct recover;
        recover.recover = (s + h + c + d);
        room->recover(invoke->invoker, recover);

        return false;
    }
};

BanyueHegemonyCard::BanyueHegemonyCard()
{
    m_skillName = "banyue_hegemony";
    sort_targets = false;
}

bool BanyueHegemonyCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Card *card = Sanguosha->cloneCard("befriend_attacking");
    DELETE_OVER_SCOPE(Card, card)
    if (targets.isEmpty()) {
        return (to_select == Self || to_select->hasShownOneGeneral()) && (!to_select->isCardLimited(card, Card::HandlingMethod::MethodUse));
    } else if (targets.length() == 1) {
        const Player *user = targets.first();
        return to_select->hasShownOneGeneral() && !user->isFriendWith(to_select, (user == Self)) && !user->isProhibited(to_select, card, QList<const Player *>());
    }
    return false;
}

bool BanyueHegemonyCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void BanyueHegemonyCard::use(Room *room, const CardUseStruct &card_use) const // onEffect is better?
{
    ServerPlayer *source = card_use.from;
    const QList<ServerPlayer *> &targets = card_use.to;

    room->loseHp(source);
    ServerPlayer *to1 = targets.first();
    ServerPlayer *to2 = targets.last();
    Card *card = Sanguosha->cloneCard("befriend_attacking");
    card->setSkillName("_banyue_hegemony");

    CardUseStruct use;
    use.from = to1;
    use.to << to2;
    use.card = card;
    room->useCard(use);
}

class BanyueHegemony : public ZeroCardViewAsSkill
{
public:
    BanyueHegemony()
        : ZeroCardViewAsSkill("banyue_hegemony")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("BanyueHegemonyCard") && !player->isRemoved();
    }

    const Card *viewAs() const override
    {
        return new BanyueHegemonyCard;
    }
};

class LuanyingVS : public OneCardViewAsSkill
{
public:
    LuanyingVS()
        : OneCardViewAsSkill("luanying_hegemony")
    {
        response_pattern = "@@luanying";
        expand_pile = "jingjie";
    }

    bool viewFilter(const Card *to_select) const override
    {
        if (!Self->getPile("jingjie").contains(to_select->getId()))
            return false;

        QString property = Self->property("luanying").toString();
        if (property == "black")
            return to_select->isBlack();
        else
            return to_select->isRed();
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        return new DummyCard({originalCard->getEffectiveId()});
    }
};

class LuanyingHegemony : public TriggerSkill
{
public:
    LuanyingHegemony()
        : TriggerSkill("luanying_hegemony")
    {
        events << CardUsed << CardResponded << EventPhaseChanging;
        view_as_skill = new LuanyingVS;
        relate_to_place = "head";
    }

    //bool canPreshow() const
    //{
    //    return true;
    //}

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                room->setPlayerFlag(p, "-luanying_used");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging)
            return QList<SkillInvokeDetail>();

        ServerPlayer *user = nullptr;
        const Card *card = nullptr;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
            user = use.from;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse) { //!resp.m_isProvision && !resp.m_isRetrial
                card = resp.m_card;
                user = resp.m_from;
            }
        }

        if (user == nullptr || card == nullptr)
            return QList<SkillInvokeDetail>();

        if (card->isKindOf("BasicCard")) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, room->getOtherPlayers(user)) {
                if (p->hasSkill(this) && p->isAlive() && !p->hasFlag("luanying_used")) {
                    QList<int> jingjie = p->getPile("jingjie");
                    bool flag = false;
                    foreach (int id, jingjie) {
                        if (Sanguosha->getCard(id)->getColor() == card->getColor()) {
                            flag = true;
                            break;
                        }
                    }
                    if (flag)
                        d << SkillInvokeDetail(this, p, p);
                }
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *user = nullptr;
        const Card *card = nullptr;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
            user = use.from;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            card = resp.m_card;
            user = resp.m_from;
        }

        if (user == nullptr || card == nullptr)
            return false;

        if (card->isRed()) {
            invoke->invoker->setProperty("luanying", "red");
            room->notifyProperty(invoke->invoker, invoke->invoker, "luanying");
        } else {
            invoke->invoker->setProperty("luanying", "black");
            room->notifyProperty(invoke->invoker, invoke->invoker, "luanying");
        }

        QString prompt = "@luanying-invoke:" + user->objectName() + ":" + card->objectName();
        const Card *c = room->askForCard(invoke->invoker, "@@luanying", prompt, data, Card::MethodNone, nullptr, false, "luanying");

        if (c != nullptr) {
            room->obtainCard(user, c, true);
            room->sendLog("#weiya", user, objectName(), QList<ServerPlayer *>(), card->objectName());
            room->setPlayerFlag(invoke->invoker, "luanying_used");
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.nullified_list << "_ALL_TARGETS";
            data = QVariant::fromValue(use);
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct s = data.value<CardResponseStruct>();
            s.m_isNullified = true;
            data = QVariant::fromValue(s);
        }

        return false;
    }
};

class MengxianVS : public OneCardViewAsSkill
{
public:
    MengxianVS()
        : OneCardViewAsSkill("mengxian_hegemony")
    {
        expand_pile = "jingjie";
        response_pattern = "@@mengxian_hegemony";
        filter_pattern = ".|.|.|jingjie";
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        return new DummyCard({originalCard->getEffectiveId()});
    }
};

class MengxianHegemony : public TriggerSkill
{
public:
    MengxianHegemony()
        : TriggerSkill("mengxian_hegemony")
    {
        events << DamageCaused;
        view_as_skill = new MengxianVS;
        relate_to_place = "deputy";
    }

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;
        if ((damage.card != nullptr) && damage.by_user && damage.card->isKindOf("Slash") && !damage.chain && !damage.transfer && damage.from != damage.to
            && damage.from->isAlive()) {
            QList<ServerPlayer *> sources = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *s, sources) {
                if (!s->getPile("jingjie").isEmpty() && s->isFriendWith(damage.from, true))
                    d << SkillInvokeDetail(this, s, s, nullptr, false, damage.to);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *from = damage.from;
        ServerPlayer *to = damage.to;
        QString prompt = "@mengxian_hegemony:" + from->objectName() + ":" + to->objectName();
        const Card *c = room->askForCard(invoke->invoker, "@@mengxian_hegemony", prompt, data, Card::MethodNone, nullptr, false, objectName());
        if (c != nullptr) {
            room->notifySkillInvoked(invoke->invoker, objectName());
            room->sendLog("#InvokeSkill", invoke->invoker, objectName());
            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", nullptr, objectName(), "");
            room->throwCard(c, reason, nullptr);

            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), to->objectName());
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *from = invoke->invoker;
        ServerPlayer *to = invoke->targets.first();

        if (from->canDiscard(to, "hes")) {
            int card_id = room->askForCardChosen(from, to, "hes", "mengxian_hegemony", false, Card::MethodDiscard);
            room->throwCard(Sanguosha->getCard(card_id), to, from);

            if (from->isAlive() && to->isAlive() && from->canDiscard(to, "hes")) {
                card_id = room->askForCardChosen(from, to, "hes", "mengxian_hegemony", false, Card::MethodDiscard);
                room->throwCard(Sanguosha->getCard(card_id), to, from);
            }
        }

        return true;
    }
};

HegemonyGeneralPackage::HegemonyGeneralPackage()
    : Package("hegemonyGeneral")
{
    General *reimu_hegemony = new General(this, "reimu_hegemony", "zhu", 4);
    reimu_hegemony->addSkill(new TuizhiHegemony);
    reimu_hegemony->addSkill(new TongjieHegemony);
    reimu_hegemony->addCompanion("marisa_hegemony");
    reimu_hegemony->addCompanion("yukari_hegemony");
    reimu_hegemony->addCompanion("aya_hegemony");

    General *marisa_hegemony = new General(this, "marisa_hegemony", "zhu", 4);
    marisa_hegemony->addSkill(new MofaHegemony);
    marisa_hegemony->addSkill(new JiezouHegemony);
    marisa_hegemony->addSkill(new JiezouTargetMod);
    related_skills.insertMulti("mofa_hegemony", "#jiezoumod");
    marisa_hegemony->addCompanion("patchouli_hegemony");
    marisa_hegemony->addCompanion("alice_hegemony");
    marisa_hegemony->addCompanion("nitori_hegemony");

    //Spring
    General *byakuren_hegemony = new General(this, "byakuren_hegemony", "wu", 4);
    byakuren_hegemony->addSkill("pudu");
    byakuren_hegemony->addSkill("jiushu");
    byakuren_hegemony->addCompanion("toramaru_hegemony");
    byakuren_hegemony->addCompanion("murasa_hegemony");
    byakuren_hegemony->addCompanion("ichirin_hegemony");

    General *nue_hegemony = new General(this, "nue_hegemony", "wu", 3);
    nue_hegemony->addSkill("weizhi");
    nue_hegemony->addSkill("weizhuang");
    nue_hegemony->addCompanion("mamizou_hegemony");

    General *toramaru_hegemony = new General(this, "toramaru_hegemony", "wu", 4);
    toramaru_hegemony->addSkill("jinghua");
    toramaru_hegemony->addSkill("weiguang");
    toramaru_hegemony->addCompanion("nazrin_hegemony");

    General *murasa_hegemony = new General(this, "murasa_hegemony", "wu", 4);
    murasa_hegemony->addSkill("shuinan");
    murasa_hegemony->addSkill("nihuo");

    General *ichirin_hegemony = new General(this, "ichirin_hegemony", "wu", 4);
    ichirin_hegemony->addSkill(new LizhiHegemony);
    ichirin_hegemony->addSkill(new YunshangHegemony);

    General *nazrin_hegemony = new General(this, "nazrin_hegemony", "wu", 3);
    nazrin_hegemony->addSkill("xunbao");
    nazrin_hegemony->addSkill("lingbai");

    General *miko_hegemony = new General(this, "miko_hegemony", "wu", 4);
    miko_hegemony->addSkill(new QingtingHegemony);
    miko_hegemony->addSkill(new ChilingHegemony);
    miko_hegemony->addSkill(new ShezhengHegemony);
    miko_hegemony->addSkill(new ShezhengViewHas);
    miko_hegemony->addSkill(new ShezhengHegemonyHandler);
    miko_hegemony->addCompanion("futo_hegemony");
    miko_hegemony->addCompanion("toziko_hegemony");
    miko_hegemony->addCompanion("seiga_hegemony");
    miko_hegemony->setHeadMaxHpAdjustedValue(-1);
    related_skills.insertMulti("shezheng_hegemony", "#shezheng_hegemony");
    related_skills.insertMulti("shezheng_hegemony", "#shezheng_viewhas");

    General *mamizou_hegemony = new General(this, "mamizou_hegemony", "wu", 4);
    mamizou_hegemony->addSkill("xihua");

    General *futo_hegemony = new General(this, "futo_hegemony", "wu", 3);
    futo_hegemony->addSkill("shijie");
    futo_hegemony->addSkill("fengshui");
    futo_hegemony->addCompanion("toziko_hegemony");

    General *toziko_hegemony = new General(this, "toziko_hegemony", "wu", 4);
    toziko_hegemony->addSkill("leishi");
    toziko_hegemony->addSkill(new FenleiHegemony);

    General *seiga_hegemony = new General(this, "seiga_hegemony", "wu", 3);
    seiga_hegemony->addSkill("xiefa");
    seiga_hegemony->addSkill("chuanbi");
    seiga_hegemony->addCompanion("yoshika_hegemony");

    General *yoshika_hegemony = new General(this, "yoshika_hegemony", "wu", 4);
    yoshika_hegemony->addSkill("duzhua");
    yoshika_hegemony->addSkill("taotie");

    General *kyouko_hegemony = new General(this, "kyouko_hegemony", "wu", 3);
    kyouko_hegemony->addSkill("songjing");
    kyouko_hegemony->addSkill("gongzhen");

    General *kogasa_hegemony = new General(this, "kogasa_hegemony", "wu", 3);
    kogasa_hegemony->addSkill("yiwang");
    kogasa_hegemony->addSkill(new JingxiaHegemony);

    General *kokoro_hegemony = new General(this, "kokoro_hegemony", "wu", 4);
    kokoro_hegemony->addSkill("nengwu");
    kokoro_hegemony->addSkill("#nengwu2");
    kokoro_hegemony->addCompanion("miko_hegemony");

    General *unzan_hegemony = new General(this, "unzan_hegemony", "wu", 5, true);
    unzan_hegemony->addSkill(new BianhuanHegemony);
    unzan_hegemony->addSkill(new LiandaHegemony);
    unzan_hegemony->addCompanion("ichirin_hegemony");

    General *myouren_hegemony = new General(this, "myouren_hegemony", "wu", 4, true);
    myouren_hegemony->addSkill(new ShanshiHegemony);
    myouren_hegemony->addSkill("shuxin");
    myouren_hegemony->addCompanion("byakuren_hegemony");

    General *sunny_hegemony = new General(this, "sunny_hegemony", "wu", 3);
    sunny_hegemony->addSkill("zheshe");
    sunny_hegemony->addSkill("zhuxi");
    sunny_hegemony->addCompanion("lunar_hegemony");
    sunny_hegemony->addCompanion("star_hegemony");

    General *lunar_hegemony = new General(this, "lunar_hegemony", "wu", 3);
    lunar_hegemony->addSkill("zhuonong");
    lunar_hegemony->addSkill("jijing");
    lunar_hegemony->addCompanion("star_hegemony");

    General *star_hegemony = new General(this, "star_hegemony", "wu", 3);
    star_hegemony->addSkill("ganying");
    star_hegemony->addSkill("dubi");

    //Summer
    General *remilia_hegemony = new General(this, "remilia_hegemony", "shu", 3);
    remilia_hegemony->addSkill("skltkexue");
    remilia_hegemony->addSkill("mingyun");
    remilia_hegemony->addCompanion("flandre_hegemony");
    remilia_hegemony->addCompanion("sakuya_hegemony");
    remilia_hegemony->addCompanion("patchouli_hegemony");

    General *flandre_hegemony = new General(this, "flandre_hegemony", "shu", 3);
    flandre_hegemony->addSkill("pohuai");
    flandre_hegemony->addSkill("yuxue");
    flandre_hegemony->addSkill("shengyan");
    flandre_hegemony->addCompanion("meirin_hegemony");

    General *sakuya_hegemony = new General(this, "sakuya_hegemony", "shu", 4);
    sakuya_hegemony->addSkill("suoding");
    sakuya_hegemony->addSkill("huisu");
    sakuya_hegemony->addCompanion("meirin_hegemony");

    General *patchouli_hegemony = new General(this, "patchouli_hegemony", "shu", 3);
    patchouli_hegemony->addSkill(new BolanHgemony);
    patchouli_hegemony->addSkill(new HezhouHegemony);
    patchouli_hegemony->addCompanion("koakuma_hegemony");

    General *meirin_hegemony = new General(this, "meirin_hegemony", "shu", 4);
    meirin_hegemony->addSkill("taiji");
    meirin_hegemony->addSkill(new BeishuiHegemony);

    General *koakuma_hegemony = new General(this, "koakuma_hegemony", "shu", 3);
    koakuma_hegemony->addSkill(new MoqiHgemony);
    koakuma_hegemony->addSkill("sishu");

    General *kaguya_hegemony = new General(this, "kaguya_hegemony", "shu", 4);
    kaguya_hegemony->addSkill(new XuyuHegemony);
    kaguya_hegemony->addSkill("shenbao");
    kaguya_hegemony->addCompanion("eirin_hegemony");
    kaguya_hegemony->addCompanion("mokou_hegemony");

    General *eirin_hegemony = new General(this, "eirin_hegemony", "shu", 4);
    eirin_hegemony->addSkill("ruizhi");
    eirin_hegemony->addSkill(new YaoshiHegemony);
    eirin_hegemony->addCompanion("reisen_hegemony");

    General *mokou_hegemony = new General(this, "mokou_hegemony", "shu", 4);
    mokou_hegemony->addSkill(new KaifengHegemony);
    mokou_hegemony->addSkill(new FengxiangHegemony);
    mokou_hegemony->addCompanion("keine_hegemony");
    mokou_hegemony->addCompanion("keine_sp_hegemony");

    General *reisen_hegemony = new General(this, "reisen_hegemony", "shu", 4);
    reisen_hegemony->addSkill("kuangzao");
    reisen_hegemony->addSkill("huanshi");
    reisen_hegemony->addCompanion("tewi_hegemony");

    General *keine_hegemony = new General(this, "keine_hegemony", "shu", 3);
    keine_hegemony->addSkill(new XushiHegemony);
    keine_hegemony->addSkill(new XinyueHegemony);

    General *keine_sp_hegemony = new General(this, "keine_sp_hegemony", "shu", 3, false, true);
    keine_sp_hegemony->addSkill("chuangshi");
    keine_sp_hegemony->addSkill(new WangyueHegemony);

    General *tewi_hegemony = new General(this, "tewi_hegemony", "shu", 3);
    tewi_hegemony->addSkill("buxian");
    tewi_hegemony->addSkill(new XingyungHegemony);

    General *toyohime_hegemony = new General(this, "toyohime_hegemony", "shu", 4);
    toyohime_hegemony->addSkill("lianxi");
    toyohime_hegemony->addSkill(new YueshiHegemony);
    toyohime_hegemony->setHeadMaxHpAdjustedValue(-1);
    toyohime_hegemony->addCompanion("yorihime_hegemony");

    General *yorihime_hegemony = new General(this, "yorihime_hegemony", "shu", 4);
    yorihime_hegemony->addSkill(new PingyiHegemony);
    yorihime_hegemony->addSkill(new PingyiHegemonyHandler);
    related_skills.insertMulti("pingyi_hegemony", "#pingyi_hegemony");

    General *wriggle_hegemony = new General(this, "wriggle_hegemony", "shu", 3);
    wriggle_hegemony->addSkill(new YinghuoHegemony);
    wriggle_hegemony->addSkill(new ChongqunHegemony);

    General *rumia_hegemony = new General(this, "rumia_hegemony", "shu", 3);
    rumia_hegemony->addSkill(new ZhenyeHegemony);
    rumia_hegemony->addSkill("anyu");

    General *yuka_hegemony = new General(this, "yuka_hegemony", "shu", 4);
    yuka_hegemony->addSkill("weiya");

    General *mystia_hegemony = new General(this, "mystia_hegemony", "shu", 3);
    mystia_hegemony->addSkill("yege");
    mystia_hegemony->addSkill("laolong");

    General *shinmyoumaru_hegemony = new General(this, "shinmyoumaru_hegemony", "shu", 3);
    shinmyoumaru_hegemony->addSkill("baochui");
    shinmyoumaru_hegemony->addSkill("yicun");
    shinmyoumaru_hegemony->addCompanion("seija_hegemony");

    General *seija_hegemony = new General(this, "seija_hegemony", "shu", 3);
    seija_hegemony->addSkill("nizhuan");
    seija_hegemony->addSkill("guizha");

    //Autumn
    General *kanako_hegemony = new General(this, "kanako_hegemony", "qun", 4);
    kanako_hegemony->addSkill("shende");
    kanako_hegemony->addSkill(new QiankunHegemony("kanako"));
    kanako_hegemony->addCompanion("suwako_hegemony");
    kanako_hegemony->addCompanion("sanae_hegemony");

    General *suwako_hegemony = new General(this, "suwako_hegemony", "qun", 3);
    suwako_hegemony->addSkill("bushu");
    suwako_hegemony->addSkill(new QiankunHegemony("suwako"));
    suwako_hegemony->addSkill(new ChuanchengHegemony);
    suwako_hegemony->addCompanion("sanae_hegemony");

    General *sanae_hegemony = new General(this, "sanae_hegemony", "qun", 4);
    sanae_hegemony->addSkill("dfgzmjiyi");
    sanae_hegemony->addSkill("qiji");

    General *aya_hegemony = new General(this, "aya_hegemony", "qun", 3);
    aya_hegemony->addSkill("fengshen");
    aya_hegemony->addSkill("fengsu");
    aya_hegemony->addSkill(new FengsuHegemonyHandler);
    related_skills.insertMulti("fengsu", "#fengsu_hegemony");
    aya_hegemony->addCompanion("momizi_hegemony");

    General *nitori_hegemony = new General(this, "nitori_hegemony", "qun", 3);
    nitori_hegemony->addSkill("xinshang");
    nitori_hegemony->addSkill("#xinshang_effect");
    nitori_hegemony->addSkill("micai");

    General *hina_hegemony = new General(this, "hina_hegemony", "qun", 3);
    hina_hegemony->addSkill("jie");
    hina_hegemony->addSkill("liuxing");

    General *momizi_hegemony = new General(this, "momizi_hegemony", "qun", 4);
    momizi_hegemony->addSkill("buju");

    General *minoriko_hegemony = new General(this, "minoriko_hegemony", "qun", 4);
    minoriko_hegemony->addSkill("fengrang");
    minoriko_hegemony->addSkill("shouhuo");
    minoriko_hegemony->addCompanion("shizuha_hegemony");

    General *shizuha_hegemony = new General(this, "shizuha_hegemony", "qun", 4);
    shizuha_hegemony->addSkill("jiliao");
    shizuha_hegemony->addSkill("zhongyan");

    General *satori_hegemony = new General(this, "satori_hegemony", "qun", 3);
    satori_hegemony->addSkill("xiangqi");
    satori_hegemony->addSkill(new DuxinHegemony);
    satori_hegemony->addCompanion("koishi_hegemony");
    satori_hegemony->addCompanion("utsuho_hegemony");
    satori_hegemony->addCompanion("rin_hegemony");

    General *koishi_hegemony = new General(this, "koishi_hegemony", "qun", 3);
    koishi_hegemony->addSkill("maihuo");
    koishi_hegemony->addSkill(new WunianHgemony);

    General *utsuho_hegemony = new General(this, "utsuho_hegemony", "qun", 4);
    utsuho_hegemony->addSkill("yaoban");
    utsuho_hegemony->addSkill("here");
    utsuho_hegemony->addCompanion("rin_hegemony");

    General *rin_hegemony = new General(this, "rin_hegemony", "qun", 4);
    rin_hegemony->addSkill("yuanling");
    rin_hegemony->addSkill("songzang");

    General *yugi_hegemony = new General(this, "yugi_hegemony", "qun", 4);
    yugi_hegemony->addSkill("guaili");
    yugi_hegemony->addSkill("jiuhao");
    yugi_hegemony->addCompanion("parsee_hegemony");

    General *parsee_hegemony = new General(this, "parsee_hegemony", "qun", 3);
    parsee_hegemony->addSkill("jidu");
    parsee_hegemony->addSkill("gelong");

    General *yamame_hegemony = new General(this, "yamame_hegemony", "qun", 4);
    yamame_hegemony->addSkill(new ChuanranHgemony);
    yamame_hegemony->addSkill("rebing");
    yamame_hegemony->addCompanion("kisume_hegemony");

    General *kisume_hegemony = new General(this, "kisume_hegemony", "qun", 3);
    kisume_hegemony->addSkill(new DiaopingHegemony);
    kisume_hegemony->addSkill(new TongjuHegemony);

    General *suika_hegemony = new General(this, "suika_hegemony", "qun", 4);
    suika_hegemony->addSkill("zuiyue");
    suika_hegemony->addSkill("doujiu");
    suika_hegemony->addSkill(new CuijiHegemony);
    suika_hegemony->addSkill(new CuijiHEffect);
    suika_hegemony->setHeadMaxHpAdjustedValue(-1);
    related_skills.insertMulti("cuiji_hegemony", "#cuiji_hegemony");
    suika_hegemony->addCompanion("yugi_hegemony");
    suika_hegemony->addCompanion("reimu_hegemony");

    General *kasen_hegemony = new General(this, "kasen_hegemony", "qun", 4);
    kasen_hegemony->addSkill("zhujiu");
    kasen_hegemony->addSkill("yushou");
    kasen_hegemony->addCompanion("suika_hegemony");
    kasen_hegemony->addCompanion("yugi_hegemony");

    General *hatate_hegemony = new General(this, "hatate_hegemony", "qun", 4);
    hatate_hegemony->addSkill(new KuaizhaoHegemony);
    hatate_hegemony->addSkill("duanjiao");
    hatate_hegemony->addCompanion("aya_hegemony");

    //Winter
    General *yuyuko_hegemony = new General(this, "yuyuko_hegemony", "wei", 4, false);
    yuyuko_hegemony->addSkill("sidie");
    yuyuko_hegemony->addSkill("yiling");
    yuyuko_hegemony->addCompanion("yukari_hegemony");
    yuyuko_hegemony->addCompanion("youmu_hegemony");

    General *yukari_hegemony = new General(this, "yukari_hegemony", "wei", 4, false);
    yukari_hegemony->addSkill("shenyin");
    yukari_hegemony->addSkill("xijian");
    yukari_hegemony->addCompanion("ran_hegemony");

    General *ran_hegemony = new General(this, "ran_hegemony", "wei", 3, false);
    ran_hegemony->addSkill(new ShihuiHegemony);
    ran_hegemony->addSkill("huanzang");
    ran_hegemony->addSkill("#huanzang");
    ran_hegemony->addCompanion("chen_hegemony");

    General *youmu_hegemony = new General(this, "youmu_hegemony", "wei", 4, false);
    youmu_hegemony->addSkill("shuangren");
    youmu_hegemony->addSkill("zhanwang");

    General *lunasa_hegemony = new General(this, "lunasa_hegemony", "wei", 4, false);
    lunasa_hegemony->addSkill(new HezouHegemony);
    lunasa_hegemony->addSkill(new XianlingHegemony);
    lunasa_hegemony->addCompanion("merlin_hegemony");
    lunasa_hegemony->addCompanion("lyrica_hegemony");

    General *merlin_hegemony = new General(this, "merlin_hegemony", "wei", 3, false);
    merlin_hegemony->addSkill(new JizouHegemony);
    merlin_hegemony->addSkill(new GuanlingHegemony);
    merlin_hegemony->addCompanion("lyrica_hegemony");

    General *lyrica_hegemony = new General(this, "lyrica_hegemony", "wei", 3, false);
    lyrica_hegemony->addSkill("xiezou");
    lyrica_hegemony->addSkill(new JianlingHegemony);

    General *alice_hegemony = new General(this, "alice_hegemony", "wei", 4, false);
    alice_hegemony->addSkill("zhanzhen");
    alice_hegemony->addSkill("renou");
    alice_hegemony->addCompanion("shanghai_hegemony");

    General *chen_hegemony = new General(this, "chen_hegemony", "wei", 3, false);
    chen_hegemony->addSkill("qimen");
    chen_hegemony->addSkill("dunjia");
    General *letty_hegemony = new General(this, "letty_hegemony", "wei", 4);
    letty_hegemony->addSkill(new HanboHegemony);
    letty_hegemony->addSkill(new DongzhiHegemony);
    letty_hegemony->addCompanion("cirno_hegemony");

    General *lilywhite_hegemony = new General(this, "lilywhite_hegemony", "wei", 3);
    lilywhite_hegemony->addSkill(new BaochunHegemony);
    lilywhite_hegemony->addSkill(new ChunhenHegemony);

    General *shanghai_hegemony = new General(this, "shanghai_hegemony", "wei", 3);
    shanghai_hegemony->addSkill("zhancao");
    shanghai_hegemony->addSkill("mocao");

    General *youki_hegemony = new General(this, "youki_hegemony", "wei", 4, true);
    youki_hegemony->addSkill("shoushu");
    youki_hegemony->addSkill("yujian");
    youki_hegemony->addCompanion("youmu_hegemony");

    General *cirno_hegemony = new General(this, "cirno_hegemony", "wei", 3);
    cirno_hegemony->addSkill(new DongjieHegemony);
    cirno_hegemony->addSkill(new BingpoHgemony);
    cirno_hegemony->addCompanion("daiyousei_hegemony");

    General *daiyousei_hegemony = new General(this, "daiyousei_hegemony", "wei", 3);
    daiyousei_hegemony->addSkill(new JuxianHegemony);
    daiyousei_hegemony->addSkill(new BanyueHegemony);

    General *renko_hegemony = new General(this, "renko_hegemony", "wei", 4);
    renko_hegemony->addSkill("shitu");
    renko_hegemony->addCompanion("merry_hegemony");

    General *merry_hegemony = new General(this, "merry_hegemony", "wei", 4);
    merry_hegemony->addSkill("jingjie");
    merry_hegemony->addSkill(new LuanyingHegemony);
    merry_hegemony->setHeadMaxHpAdjustedValue(-1);
    merry_hegemony->addSkill(new MengxianHegemony);
    related_skills.insertMulti("mengxian_hegemony", "#mengxian_hegemony");

    General *rinnosuke_hegemony = new General(this, "rinnosuke_hegemony", "wei", 4, true);
    rinnosuke_hegemony->addSkill("xiufu");
    rinnosuke_hegemony->addCompanion("marisa_hegemony");
    rinnosuke_hegemony->addCompanion("tokiko_hegemony");

    General *tokiko_hegemony = new General(this, "tokiko_hegemony", "wei", 3);
    tokiko_hegemony->addSkill("fandu");
    tokiko_hegemony->addSkill("taohuan");

    General *mima_hegemony = new General(this, "mima_hegemony", "wei", 4);
    mima_hegemony->addSkill("meiling");
    mima_hegemony->addCompanion("marisa_hegemony");

    addMetaObject<NiaoxiangSummon>();
    addMetaObject<HalfLifeCard>();
    addMetaObject<CompanionCard>();
    addMetaObject<PioneerCard>();
    addMetaObject<QingtingHegemonyCard>();
    addMetaObject<ShowShezhengCard>();
    addMetaObject<XushiHegemonyCard>();
    addMetaObject<XingyunHegemonyCard>();
    addMetaObject<ShowFengsuCard>();
    addMetaObject<ChunhenHegemonyCard>();
    addMetaObject<DongzhiHegemonyCard>();
    addMetaObject<BanyueHegemonyCard>();
    addMetaObject<KuaizhaoHegemonyCard>();

    //GameRule
    skills << new GameRule_AskForGeneralShowHead << new GameRule_AskForGeneralShowDeputy << new GameRule_AskForArraySummon << new HalfLife << new HalfLifeVS << new HalfLifeMax
           << new CompanionVS << new PioneerVS;
    //General skill
    skills << new ShezhengAttach << new FengsuAttach;
}

ADD_PACKAGE(HegemonyGeneral)
