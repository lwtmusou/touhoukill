#include "th06.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "settings.h"
#include "skill.h"
#include "standard.h"
#include "th10.h"
#include "util.h"
#include "washout.h"

#include <QApplication>
#include <QCommandLinkButton>
#include <QPointer>

SkltKexueCard::SkltKexueCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodUse;
    m_skillName = "skltkexue_attach";
}

void SkltKexueCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;

    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (who != nullptr && who->hasSkill("skltkexue")) {
        room->notifySkillInvoked(who, "skltkexue");
        room->loseHp(source);
        if (source->isAlive()) {
            if (isHegemonyGameMode(ServerInfo.GameMode))
                source->drawCards(1);
            else
                source->drawCards(2);
        }

        RecoverStruct recover;
        recover.recover = 1;
        recover.who = source;
        room->recover(who, recover);
    }
}

class SkltKexueVS : public ZeroCardViewAsSkill
{
public:
    SkltKexueVS()
        : ZeroCardViewAsSkill("skltkexue_attach")
    {
        attached_lord_skill = true;
    }

    bool isEnabledAtPlay(const Player *) const override
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        if (player->getHp() > player->dyingThreshold() && pattern.contains("peach")) {
            foreach (const Player *p, player->getAliveSiblings()) {
                if (p->hasFlag("Global_Dying") && p->hasSkill("skltkexue", false, false))
                    return true;
            }
        }
        return false;
    }

    const Card *viewAs() const override
    {
        return new SkltKexueCard;
    }
};

class SkltKexue : public TriggerSkill
{
public:
    SkltKexue()
        : TriggerSkill("skltkexue")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Death << Debut << Revive << GeneralShown << Dying;
        show_type = "static";
    }

    void record(TriggerEvent e, Room *room, QVariant &) const override
    {
        if (e == Dying)
            return;

        static QString attachName = "skltkexue_attach";
        QList<ServerPlayer *> sklts;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->hasSkill(this, true) && p->hasShownSkill(this))
                sklts << p;
        }

        if (sklts.length() > 1) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (!p->hasSkill(attachName, true))
                    room->attachSkillToPlayer(p, attachName);
            }
        } else if (sklts.length() == 1) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(this, true) && p->hasSkill(attachName, true))
                    room->detachSkillFromPlayer(p, attachName, true);
                else if (!p->hasSkill(this, true) && !p->hasSkill(attachName, true))
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
        if (triggerEvent == Dying) {
            DyingStruct dying = data.value<DyingStruct>();
            if ((dying.who != nullptr) && dying.who->isAlive() && dying.who->hasSkill(this) && !dying.who->hasShownSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dying.who, dying.who);
            return QList<SkillInvokeDetail>();
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (invoke->invoker->askForSkillInvoke(this, data))
            invoke->invoker->showHiddenSkill(objectName());
        return false;
    }
};

class Mingyun : public TriggerSkill
{
public:
    Mingyun()
        : TriggerSkill("mingyun")
    {
        events << StartJudge << EventPhaseEnd;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;
        if (e == StartJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if ((judge->who == nullptr) || !judge->who->isAlive())
                return QList<SkillInvokeDetail>();

            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(this))
                    r << SkillInvokeDetail(this, p, p);
            }
        } else if (e == EventPhaseEnd) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->getPhase() != Player::Play)
                return QList<SkillInvokeDetail>();

            if (p->isAlive() && p->hasSkill(this))
                r << SkillInvokeDetail(this, p, p);
        }

        return r;
    }

    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        invoke->invoker->tag["mingyun_judge"] = data;
        QString prompt = "playphase";
        JudgeStruct *judge = nullptr;
        if (e == StartJudge) {
            judge = data.value<JudgeStruct *>();
            prompt = "judge:" + judge->who->objectName() + ":" + judge->reason;
        }

        if (invoke->invoker->getHandcardNum() <= invoke->invoker->getMaxHp()) {
            if (invoke->invoker->askForSkillInvoke(this, prompt)) {
                if (judge != nullptr)
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), judge->who->objectName());
                invoke->tag[objectName()] = -1;
                return true;
            }
        } else {
            prompt.prepend("@mingyun_overmaxhp_");
            const Card *c = room->askForCard(invoke->invoker, ".", prompt, data, Card::MethodNone, nullptr, false, objectName(), false, 0);
            if (c != nullptr) {
                invoke->tag[objectName()] = c->getEffectiveId();
                return true;
            }
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (invoke->tag[objectName()].toInt() == -1) {
            QList<int> list = room->getNCards(2);
            room->returnToDrawPile(list);

            room->fillAG(list, invoke->invoker);
            int obtain_id = room->askForAG(invoke->invoker, list, false, objectName());
            room->clearAG(invoke->invoker);
            room->obtainCard(invoke->invoker, obtain_id, false);
        } else {
            room->moveCardTo(Sanguosha->getCard(invoke->tag[objectName()].toInt()), invoke->invoker, nullptr, Player::DrawPile,
                             CardMoveReason(CardMoveReason::S_REASON_PUT, invoke->invoker->objectName()));
        }
        return false;
    }
};

class Xueyi : public TriggerSkill
{
public:
    Xueyi()
        : TriggerSkill("xueyi$")
    {
        events << HpRecover;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        RecoverStruct r = data.value<RecoverStruct>();
        if (r.to->getKingdom() != "hmx")
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> details;
        foreach (ServerPlayer *rem, room->getOtherPlayers(r.to)) {
            if (rem->hasLordSkill(objectName()))
                details << SkillInvokeDetail(this, rem, r.to);
        }
        return details;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->owner))) {
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

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->owner->drawCards(1, objectName());
        return false;
    }
};

class Pohuai : public TriggerSkill
{
public:
    Pohuai()
        : TriggerSkill("pohuai")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    bool canPreshow() const override
    {
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *fldl = data.value<ServerPlayer *>();
        if (!fldl->hasSkill(this) || fldl->isDead() || fldl->getPhase() != Player::Start)
            return QList<SkillInvokeDetail>();

        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, fldl, fldl, nullptr, true); //fldl->hasShownSkill(this)
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        ServerPlayer *fldl = data.value<ServerPlayer *>();
        room->touhouLogmessage("#TriggerSkill", fldl, objectName());
        room->notifySkillInvoked(fldl, objectName());
        room->broadcastSkillInvoke(objectName());

        JudgeStruct judge;
        judge.who = fldl;
        judge.pattern = "Slash";
        judge.good = true;
        judge.reason = objectName();
        room->judge(judge);

        if (judge.isBad() || judge.ignore_judge)
            return false;

        QList<ServerPlayer *> all;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (fldl->distanceTo(p) <= 1)
                all << p;
        }
        if (all.isEmpty())
            return false;

        foreach (ServerPlayer *p, all)
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, fldl->objectName(), p->objectName());

        foreach (ServerPlayer *p, all)
            room->damage(DamageStruct(objectName(), fldl, p));

        return false;
    }
};

class Yuxue : public TriggerSkill
{
public:
    Yuxue()
        : TriggerSkill("yuxue")
    {
        events << Damaged << ConfirmDamage << PreCardUsed;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if ((use.card != nullptr) && use.card->isKindOf("Slash") && use.card->hasFlag("yuxueSlash") && (use.from != nullptr) && use.from->hasSkill(this))
                room->notifySkillInvoked(use.from, objectName());
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == PreCardUsed)
            return QList<SkillInvokeDetail>();

        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == Damaged) {
            if (damage.to->isAlive() && damage.to->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        } else if (triggerEvent == ConfirmDamage) {
            if (damage.chain || damage.transfer || !damage.by_user)
                return QList<SkillInvokeDetail>();
            if (damage.from == damage.to)
                return QList<SkillInvokeDetail>();
            if ((damage.card != nullptr) && damage.card->isKindOf("Slash") && damage.card->hasFlag("yuxueSlash"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, nullptr, damage.from, nullptr, true, nullptr, false);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == Damaged) {
            room->setPlayerFlag(invoke->invoker, "SlashRecorder_yuxueSlash");
            if (invoke->invoker->isHiddenSkill(objectName()))
                room->setPlayerFlag(invoke->invoker, "Global_viewasHidden_Failed"); //only for anyun
            const Card *c = room->askForUseCard(invoke->invoker, "slash", "@yuxue", -1, Card::MethodUse, false, objectName());
            if (c == nullptr)
                room->setPlayerFlag(invoke->invoker, "-SlashRecorder_yuxueSlash");
            return c != nullptr;

        } else if (triggerEvent == ConfirmDamage)
            return true;

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        if (triggerEvent != ConfirmDamage)
            return false;

        DamageStruct damage = data.value<DamageStruct>();
        damage.damage = damage.damage + 1;
        if (damage.from != nullptr) {
            QList<ServerPlayer *> logto;
            logto << damage.to;
            room->touhouLogmessage("#yuxue_damage", damage.from, "yuxue", logto);
        }
        data = QVariant::fromValue(damage);
        return false;
    }
};

class YuxueSlashNdl : public TargetModSkill
{
public:
    YuxueSlashNdl()
        : TargetModSkill("#yuxue-slash-ndl")
    {
        pattern = "Slash";
    }

    int getDistanceLimit(const Player *from, const Card *) const override
    {
        if (from->hasFlag("SlashRecorder_yuxueSlash"))
            return 1000;

        return 0;
    }
};

class Shengyan : public TriggerSkill
{
public:
    Shengyan()
        : TriggerSkill("shengyan")
    {
        events << Damage;
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if ((damage.from != nullptr) && damage.from->isAlive() && damage.from->hasSkill(this)) {
            QList<SkillInvokeDetail> d;
            for (int i = 0; i < damage.damage; ++i)
                d << SkillInvokeDetail(this, damage.from, damage.from);

            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    // the cost is only askForSkillInvoke, omitted

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->drawCards(1, objectName());
        return false;
    }
};

SuodingCard::SuodingCard()
{
}

bool SuodingCard::targetFilter(const QList<const Player *> &, const Player *, const Player *) const
{
    Q_ASSERT(false);
    return false;
}

bool SuodingCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *, int &maxVotes) const
{
    if (to_select->isKongcheng())
        return false;
    int i = 0;

    foreach (const Player *player, targets) {
        if (player == to_select)
            i++;
    }

    maxVotes = qMax(3 - targets.size(), 0) + i;
    return maxVotes > 0;
}

bool SuodingCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    if (targets.toSet().size() > 3 || targets.toSet().size() == 0)
        return false;
    QHash<const Player *, int> map;

    foreach (const Player *sp, targets)
        map[sp]++;
    foreach (const Player *sp, map.keys()) {
        if (map[sp] > sp->getHandcardNum())
            return false;
    }

    return true;
}

void SuodingCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    const QList<ServerPlayer *> &targets = card_use.to;

    QHash<ServerPlayer *, int> map;
    foreach (ServerPlayer *sp, targets)
        map[sp]++;

    QList<ServerPlayer *> newtargets = map.keys();
    room->sortByActionOrder(newtargets);
    foreach (ServerPlayer *sp, newtargets) {
        if (source == sp) {
            const Card *cards = room->askForExchange(source, "suoding", map.value(sp), map.value(sp), false, "suoding_exchange:" + QString::number(map[sp]));
            DELETE_OVER_SCOPE(const Card, cards)
            foreach (int id, cards->getSubcards())
                sp->addToPile("suoding_cards", id, false);
        } else {
            for (int i = 0; i < map[sp]; i++) {
                if (!sp->isKongcheng()) {
                    int card_id = room->askForCardChosen(source, sp, "hs", "suoding"); // fakemove/getrandomhandcard(without 2nd general!!!!)
                    sp->addToPile("suoding_cards", card_id, false);
                }
            }
        }
    }
}

class SuodingVS : public ZeroCardViewAsSkill
{
public:
    SuodingVS()
        : ZeroCardViewAsSkill("suoding")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("SuodingCard");
    }

    const Card *viewAs() const override
    {
        return new SuodingCard;
    }
};

class Suoding : public TriggerSkill
{
public:
    Suoding()
        : TriggerSkill("suoding")
    {
        events << EventPhaseChanging;
        view_as_skill = new SuodingVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QList<SkillInvokeDetail>();

            foreach (ServerPlayer *liege, room->getAllPlayers()) {
                if (!liege->getPile("suoding_cards").isEmpty())
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, nullptr, change.player, nullptr, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    // compulsory effect, cost omitted

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        QList<CardsMoveStruct> moves;

        foreach (ServerPlayer *liege, room->getAllPlayers()) {
            if (!liege->getPile("suoding_cards").isEmpty()) {
                CardsMoveStruct move;
                move.card_ids = liege->getPile("suoding_cards");
                move.to_place = Player::PlaceHand;
                move.to = liege;
                moves << move;
                QList<ServerPlayer *> logto;
                logto << liege;
                room->touhouLogmessage("#suoding_Trigger", invoke->invoker, objectName(), logto, QString::number(move.card_ids.length()));
            }
        }

        room->moveCardsAtomic(moves, false);
        return false;
    }
};

class Huisu : public TriggerSkill
{
public:
    Huisu()
        : TriggerSkill("huisu")
    {
        events << PostHpReduced << EventPhaseChanging << EventPhaseStart;
        global = true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == PostHpReduced) {
            ServerPlayer *player = nullptr;
            if (data.canConvert<DamageStruct>())
                player = data.value<DamageStruct>().to;
            else if (data.canConvert<HpLostStruct>())
                player = data.value<HpLostStruct>().player;
            if (player == nullptr)
                return;
            else
                room->setPlayerFlag(player, "huisu");
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerFlag(p, "-huisu");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent != EventPhaseStart)
            return QList<SkillInvokeDetail>();

        ServerPlayer *current = data.value<ServerPlayer *>();
        if ((current == nullptr) || current->getPhase() != Player::Finish)
            return QList<SkillInvokeDetail>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->hasSkill(this) && p->hasFlag("huisu"))
                d << SkillInvokeDetail(this, p, p);
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        JudgeStruct judge;
        judge.pattern = ".|red";
        judge.good = true;
        judge.reason = objectName();
        judge.who = invoke->invoker;
        room->judge(judge);

        if (judge.isGood() && !judge.ignore_judge) {
            RecoverStruct recov;
            recov.recover = 1;
            room->recover(invoke->invoker, recov);
        }
        return false;
    }
};

class Bolan : public TriggerSkill
{
public:
    Bolan()
        : TriggerSkill("bolan")
    {
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("TrickCard"))
            return d;
        foreach (ServerPlayer *p, use.to) {
            if (p->hasSkill(this) && p != use.from)
                d << SkillInvokeDetail(this, p, p);
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *player = invoke->invoker;
        QList<int> list = room->getNCards(3);

        room->askForGuanxing(player, list, Room::GuanxingBothSides, objectName());

        return false;
    }
};

HezhouCard::HezhouCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "hezhou";
}

bool HezhouCard::do_hezhou(ServerPlayer *player) const
{
    Room *room = player->getRoom();
    Card *hezhoucard = Sanguosha->cloneCard(player->tag["hezhou_choice"].toString());
    DELETE_OVER_SCOPE(Card, hezhoucard)

    QList<int> ids;
    if (room->getDrawPile().length() < 2)
        room->swapPile();

    const QList<int> &drawpile = room->getDrawPile();
    ids << drawpile.last();
    if (drawpile.length() >= 2)
        ids << drawpile.at(drawpile.length() - 2);

    CardsMoveStruct move(ids, nullptr, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString()));
    room->moveCardsAtomic(move, true);

    room->getThread()->delay();
    bool success = false;
    Card *card1 = Sanguosha->getCard(ids.first());
    Card *card2 = Sanguosha->getCard(ids.last());
    if (card1->getSuit() != card2->getSuit() && card1->getTypeId() != card2->getTypeId())
        success = true;

    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
    DummyCard dummy(ids);
    room->throwCard(&dummy, reason, nullptr);

    return success;
}

bool HezhouCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return false;

    if (user_string == nullptr)
        return false;
    Card *card = Sanguosha->cloneCard(user_string.split("+").first(), Card::NoSuit, 0);
    if (card == nullptr)
        return false;
    DELETE_OVER_SCOPE(Card, card)
    card->setSkillName("hezhou");
    if (card->targetFixed(Self))
        return false;
    return card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool HezhouCard::targetFixed(const Player *) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;
    if (user_string == nullptr)
        return false;

    // return false by default
    // we need a confirming chance to pull back, since this is a zero cards viewas Skill.
    return false;
}

bool HezhouCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;
    if (user_string == nullptr)
        return false;
    Card *card = Sanguosha->cloneCard(user_string.split("+").first(), Card::NoSuit, 0);
    if (card == nullptr)
        return false;
    DELETE_OVER_SCOPE(Card, card)
    card->setSkillName("hezhou");
    if (card->canRecast() && targets.length() == 0)
        return false;
    if (card->targetFixed(Self))
        return true;
    return card->targetsFeasible(targets, Self);
}

const Card *HezhouCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *player = card_use.from;
    player->showHiddenSkill("hezhou");
    Room *room = player->getRoom();
    QString to_use = user_string;
    room->notifySkillInvoked(player, "hezhou");

    LogMessage log;
    log.type = card_use.to.isEmpty() ? "#HezhouNoTarget" : "#Hezhou";
    log.from = player;
    log.to = card_use.to;
    log.arg = to_use;
    log.arg2 = "hezhou";
    room->sendLog(log);

    player->tag["hezhou_choice"] = QVariant::fromValue(to_use);
    bool success = do_hezhou(player);
    room->setPlayerFlag(player, "hezhou_used");
    if (success) {
        Card *use_card = Sanguosha->cloneCard(to_use);
        use_card->setSkillName("hezhou");
        use_card->deleteLater();

        return use_card;
    } else
        return nullptr;
}

const Card *HezhouCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();
    room->notifySkillInvoked(user, "hezhou");
    LogMessage log;
    log.type = "#HezhouNoTarget";
    log.from = user;
    log.arg = user_string;
    log.arg2 = "hezhou";
    room->sendLog(log);

    user->tag["hezhou_choice"] = QVariant::fromValue(user_string);
    user->showHiddenSkill("hezhou");
    bool success = do_hezhou(user);
    room->setPlayerFlag(user, "hezhou_used");
    if (success) {
        Card *use_card = Sanguosha->cloneCard(user_string);
        use_card->setSkillName("hezhou");
        use_card->deleteLater();
        return use_card;
    } else
        return nullptr;
}

class HezhouVS : public ZeroCardViewAsSkill
{
public:
    HezhouVS()
        : ZeroCardViewAsSkill("hezhou")
    {
    }

    static QStringList responsePatterns()
    {
        const CardPattern *pattern = Sanguosha->getPattern(Sanguosha->currentRoomState()->getCurrentCardUsePattern());
        Card::HandlingMethod method = Card::MethodUse;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            method = Card::MethodResponse;

        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();

        QStringList checkedPatterns;
        QStringList ban_list = Sanguosha->getBanPackages();
        foreach (const Card *card, cards) {
            if ((!Self->isCurrent() && card->isKindOf("Peach"))
                || (Self->isCurrent() && card->isNDTrick() && !card->isKindOf("AOE") && !card->isKindOf("GlobalEffect")) && !ban_list.contains(card->getPackage())) {
                QString name = card->objectName();
                if (!checkedPatterns.contains(name) && (pattern != nullptr && pattern->match(Self, card)) && !Self->isCardLimited(card, method))
                    checkedPatterns << name;
            }
        }
        return checkedPatterns;
    }

    bool isEnabledAtResponse(const Player *player, const QString &) const override
    {
        if (player->hasFlag("hezhou_used"))
            return false;

        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.contains("peach") && checkedPatterns.length() == 1 && player->getMark("Global_PreventPeach") > 0)
            return false;

        return !checkedPatterns.isEmpty();
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasFlag("hezhou_used");
    }

    const Card *viewAs() const override
    {
        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.length() == 1) {
            HezhouCard *card = new HezhouCard;
            card->setUserString(checkedPatterns.first());
            return card;
        }

        QString name = Self->tag.value("hezhou", QString()).toString();
        if (name != nullptr) {
            HezhouCard *card = new HezhouCard;
            card->setUserString(name);
            return card;
        } else
            return nullptr;
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const override
    {
        return !player->hasFlag("hezhou_used") && player->isCurrent();
    }
};

class Hezhou : public TriggerSkill
{
public:
    Hezhou()
        : TriggerSkill("hezhou")
    {
        events << EventPhaseChanging;
        view_as_skill = new HezhouVS;
    }

    QDialog *getDialog() const override
    {
        return QijiDialog::getInstance("hezhou");
    }

    void record(TriggerEvent, Room *room, QVariant &data) const override
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasFlag("hezhou_used"))
                    room->setPlayerFlag(p, "-hezhou_used");
            }
        }
    }
};

class Taiji : public TriggerSkill
{
public:
    Taiji()
        : TriggerSkill("taiji")
    {
        events << TargetSpecified << TargetConfirmed << SlashMissed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (e == TargetSpecified || e == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash") || use.to.length() != 1)
                return d;
            QList<int> ids;
            if (use.card->isVirtualCard())
                ids = use.card->getSubcards();
            else
                ids << use.card->getEffectiveId();

            if (ids.isEmpty())
                return QList<SkillInvokeDetail>();
            foreach (int id, ids) {
                if (room->getCardPlace(id) != Player::PlaceTable)
                    return QList<SkillInvokeDetail>();
            }
            if (e == TargetSpecified && use.from != nullptr && use.from->isAlive() && use.from->hasSkill(this) && use.to.first()->isAlive())
                d << SkillInvokeDetail(this, use.from, use.from, nullptr, false, use.to.first());
            else if (e == TargetConfirmed && use.to.first()->isAlive() && use.to.first()->hasSkill(this))
                d << SkillInvokeDetail(this, use.to.first(), use.to.first(), nullptr, false, use.from);
        } else if (e == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if ((effect.from == nullptr) || effect.from->isDead())
                return QList<SkillInvokeDetail>();
            if ((effect.slash == nullptr) || effect.jink == nullptr || !effect.slash->hasFlag("taiji_" + effect.from->objectName()))
                return QList<SkillInvokeDetail>();
            QList<int> ids;
            if (effect.jink->isVirtualCard())
                ids = effect.jink->getSubcards();
            else
                ids << effect.jink->getEffectiveId();
            if (ids.isEmpty())
                return QList<SkillInvokeDetail>();
            foreach (int id, ids) {
                if (room->getCardPlace(id) != Player::DiscardPile)
                    return QList<SkillInvokeDetail>();
            }
            d << SkillInvokeDetail(this, nullptr, effect.from, nullptr, true, nullptr, false);
        }
        return d;
    }

    bool cost(TriggerEvent e, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (e == SlashMissed)
            return true;
        invoke->invoker->tag["taiji"] = data;
        return invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget));
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (e == TargetSpecified || e == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            //room->setCardFlag(use.card, "taiji_" + liege->objectName());
            ServerPlayer *user = (e == TargetSpecified) ? invoke->invoker : invoke->targets.first();
            ServerPlayer *target = (e == TargetSpecified) ? invoke->targets.first() : invoke->invoker;
            room->setCardFlag(use.card, "taiji_" + user->objectName());
            if (use.m_addHistory) {
                room->addPlayerHistory(use.from, use.card->getClassName(), -1);
                use.m_addHistory = false;
                data = QVariant::fromValue(use);
            }
            target->obtainCard(use.card);
        } else if (e == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            effect.from->obtainCard(effect.jink);
        }
        return false;
    }
};

BeishuiDialog *BeishuiDialog::getInstance(const QString &object, bool left, bool right)
{
    static QPointer<BeishuiDialog> instance;

    if (!instance.isNull() && instance->objectName() != object)
        delete instance;

    if (instance.isNull()) {
        instance = new BeishuiDialog(object, left, right);
        connect(qApp, &QCoreApplication::aboutToQuit, instance.data(), &BeishuiDialog::deleteLater);
    }

    return instance;
}

BeishuiDialog::BeishuiDialog(const QString &object, bool left, bool)
    : object_name(object)
{
    setObjectName(object);
    setWindowTitle(Sanguosha->translate(object)); //need translate title?
    group = new QButtonGroup(this);

    QHBoxLayout *layout = new QHBoxLayout;
    if (left)
        layout->addWidget(createLeft());
    //if (right)
    //    layout->addWidget(createRight());
    setLayout(layout);

    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectCard(QAbstractButton *)));
}

void BeishuiDialog::popup()
{
    Self->tag.remove(object_name);

    Card::HandlingMethod method = Card::MethodUse;
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        method = Card::MethodResponse;

    QStringList checkedPatterns;
    QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
    const CardPattern *cardPattern = Sanguosha->getPattern(pattern);

    bool play = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY);

    //collect available patterns for specific skill
    QStringList validPatterns;
    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    QStringList ban_list = Sanguosha->getBanPackages();
    foreach (const Card *card, cards) {
        if ((card->isNDTrick() || card->isKindOf("BasicCard")) && !ban_list.contains(card->getPackage())) { //!ServerInfo.Extensions.contains("!" + card->getPackage())
            QString name = card->objectName();
            if (!validPatterns.contains(name))
                validPatterns << card->objectName();
        }
    }

    //then match it and check "CardLimit"
    foreach (const QString &str, validPatterns) {
        Card *card = Sanguosha->cloneCard(str);
        DELETE_OVER_SCOPE(Card, card)
        if (play || (cardPattern != nullptr && cardPattern->match(Self, card)) && !Self->isCardLimited(card, method))
            checkedPatterns << str;
    }
    //while responding, if only one pattern were checked, emit click()

    if (!play && checkedPatterns.length() <= 1) {
        // @ todo: basic card
        emit onButtonClick();
        return;
    }

    foreach (QAbstractButton *button, group->buttons()) {
        const Card *card = map[button->objectName()];
        const Player *user = Self;

        bool available = (!play) || card->isAvailable(user);
        if (card->isKindOf("Peach"))
            available = card->isAvailable(user);

        bool checked = checkedPatterns.contains(card->objectName());
        //check isCardLimited
        bool enabled = available && checked;
        button->setEnabled(enabled);
    }

    exec();
}

void BeishuiDialog::selectCard(QAbstractButton *button)
{
    const Card *card = map.value(button->objectName());
    Self->tag[object_name] = QVariant::fromValue(card->objectName());

    emit onButtonClick();
    accept();
}

QGroupBox *BeishuiDialog::createLeft()
{
    QGroupBox *box = new QGroupBox;
    box->setTitle(Sanguosha->translate("basic"));

    QVBoxLayout *layout = new QVBoxLayout;

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    QStringList ban_list = Sanguosha->getBanPackages();
    foreach (const Card *card, cards) {
        if (card->getTypeId() == Card::TypeBasic && !map.contains(card->objectName())
            && !ban_list.contains(card->getPackage())) { //!ServerInfo.Extensions.contains("!" + card->getPackage())
            Card *c = Sanguosha->cloneCard(card->objectName());
            c->setParent(this);
            layout->addWidget(createButton(c));
        }
    }

    layout->addStretch();
    box->setLayout(layout);
    return box;
}

QAbstractButton *BeishuiDialog::createButton(const Card *card)
{
    if (card->objectName() == "slash" && map.contains(card->objectName()) && !map.contains("normal_slash")) {
        QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate("normal_slash"));
        button->setObjectName("normal_slash");
        button->setToolTip(card->getDescription());

        map.insert("normal_slash", card);
        group->addButton(button);

        return button;
    } else {
        QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate(card->objectName()));
        button->setObjectName(card->objectName());
        button->setToolTip(card->getDescription());

        map.insert(card->objectName(), card);
        group->addButton(button);

        return button;
    }
}

class BeishuiVS : public ViewAsSkill
{
public:
    BeishuiVS()
        : ViewAsSkill("beishui")
    {
        response_or_use = true;
    }

    static QStringList responsePatterns()
    {
        const CardPattern *pattern = Sanguosha->getPattern(Sanguosha->currentRoomState()->getCurrentCardUsePattern());

        Card::HandlingMethod method = Card::MethodUse;
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();

        QStringList checkedPatterns;
        QStringList ban_list = Sanguosha->getBanPackages();
        foreach (const Card *card, cards) {
            if ((card->isKindOf("BasicCard")) && !ban_list.contains(card->getPackage())) { //!ServerInfo.Extensions.contains("!" + card->getPackage())
                QString name = card->objectName();
                if (!checkedPatterns.contains(name) && (pattern != nullptr && pattern->match(Self, card)) && !Self->isCardLimited(card, method))
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
        int num = qMax(1, Self->getHp());
        return selected.length() < num;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        int num = qMax(1, Self->getHp());
        if (cards.length() != num)
            return nullptr;

        QString name = Self->tag.value("beishui", QString()).toString();
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

class Beishui : public TriggerSkill
{
public:
    Beishui()
        : TriggerSkill("beishui")
    {
        events << EventPhaseChanging << PreCardUsed << CardResponded;
        view_as_skill = new BeishuiVS;
    }

    QDialog *getDialog() const override
    {
        return BeishuiDialog::getInstance("beishui", true, false);
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getMark("beishui") > 0)
                    room->setPlayerMark(p, "beishui", 0);
            }
        }
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

class Dongjie : public TriggerSkill
{
public:
    Dongjie()
        : TriggerSkill("dongjie")
    {
        events = {TargetSpecified, EventPhaseChanging, Cancel};
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                p->setFlags("-" + objectName());
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            QList<SkillInvokeDetail> d;
            if (use.from != nullptr && use.from->hasSkill(this) && use.from->isAlive() && !use.to.isEmpty() && !use.from->hasFlag(objectName())) {
                foreach (ServerPlayer *to, use.to) {
                    if (to != use.from)
                        d << SkillInvokeDetail(this, use.from, use.from, nullptr, false, to);
                }
            }
            return d;
        } else if (triggerEvent == Cancel) {
            const Card *c = nullptr;
            ServerPlayer *from = nullptr;
            ServerPlayer *to = nullptr;
            if (data.canConvert<SlashEffectStruct>()) {
                SlashEffectStruct e = data.value<SlashEffectStruct>();
                c = e.slash;
                from = e.from;
                to = e.to;
            } else if (data.canConvert<CardEffectStruct>()) {
                CardEffectStruct e = data.value<CardEffectStruct>();
                c = e.card;
                from = e.from;
                to = e.to;
            }

            if (c != nullptr && c->hasFlag(objectName()))
                return {SkillInvokeDetail(this, from, from, to, true, nullptr, false)};
        }

        return {};
    }

    // default cost

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == TargetSpecified) {
            ServerPlayer *target = invoke->targets.first();
            CardUseStruct use = data.value<CardUseStruct>();

            invoke->invoker->setFlags(objectName());

            bool given = false;
            if (!target->isKongcheng()) {
                const Card *c = room->askForCard(target, ".red", "@dongjie-give:" + invoke->invoker->objectName() + "::" + use.card->objectName(), data, Card::MethodNone, nullptr,
                                                 false, {}, false, 0);
                if (c != nullptr) {
                    given = true;
                    room->moveCardTo(c, invoke->invoker, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_GIVE, invoke->invoker->objectName(), objectName(), {}), true);
                }
            }

            if (!given) {
                LogMessage l;
                l.type = "#dongjie-notgiven";
                l.from = invoke->invoker;
                l.arg = use.card->objectName();
                l.arg2 = objectName();
                room->sendLog(l);

                use.card->setFlags(objectName());
                target->drawCards(1);
                target->turnOver();
            }
        } else if (triggerEvent == Cancel) {
            return true;
        }

        return false;
    }
};

class Bingpo : public TriggerSkill
{
public:
    Bingpo()
        : TriggerSkill("bingpo")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->hasSkill(this) && damage.nature != DamageStruct::Fire && damage.damage >= damage.to->getHp())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, nullptr, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->touhouLogmessage("#bingpolog", invoke->invoker, "bingpo", QList<ServerPlayer *>(), QString::number(damage.damage));
        room->notifySkillInvoked(invoke->invoker, objectName());
        return true;
    }
};

int ZhenyeCard::X(const Player *p)
{
    return qMin(qMax(p->getLostHp(), 1), 3);
}

ZhenyeCard::ZhenyeCard()
{
    m_skillName = "zhenye";
}

void ZhenyeCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *nokia = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = nokia->getRoom();

    // should be in onUse!!
    nokia->turnOver();

    int x = X(target);

    bool discarded = false;
    if (target->getCardCount() >= x) {
        const Card *c = room->askForExchange(target, getSkillName(), x, x, true, "@zhenye-exchange:" + nokia->objectName() + "::" + QString::number(x), true);
        if (c != nullptr) {
            discarded = true;
            DummyCard toGet;
            DummyCard toDiscard;
            foreach (int id, c->getSubcards()) {
                room->showCard(target, id);

                const Card *originalCard = Sanguosha->getCard(id);
                if (originalCard->isBlack())
                    toGet.addSubcard(id);
                else if (!target->isJilei(originalCard))
                    toDiscard.addSubcard(id);
            }

            if (!toGet.getSubcards().isEmpty())
                room->obtainCard(nokia, &toGet, CardMoveReason(CardMoveReason::S_REASON_GIVE, nokia->objectName(), getSkillName(), QString()));

            if (!toDiscard.getSubcards().isEmpty())
                room->throwCard(&toDiscard, target);

            delete c;
        }
    }

    if (!discarded) {
        room->drawCards(target, x, getSkillName());
        target->turnOver();
    }
}

class Zhenye : public ZeroCardViewAsSkill
{
public:
    Zhenye()
        : ZeroCardViewAsSkill("zhenye")
    {
    }

    const Card *viewAs() const override
    {
        return new ZhenyeCard;
    }

    bool isEnabledAtPlay(const Player *nokia) const override
    {
        return !nokia->hasUsed("ZhenyeCard");
    }
};

class Anyu : public TriggerSkill
{
public:
    Anyu()
        : TriggerSkill("anyu")
    {
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card != nullptr && use.card->isBlack()) {
            foreach (ServerPlayer *to, use.to) {
                if (to->hasSkill(this) && to->isAlive())
                    d << SkillInvokeDetail(this, to, to);
            }
        }

        return d;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->turnOver();
        invoke->invoker->drawCards(1);

        return false;
    }
};

class Moqi : public TriggerSkill
{
public:
    Moqi()
        : TriggerSkill("moqi")
    {
        events << CardUsed << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play && (change.player != nullptr) && change.player->isAlive()) {
                change.player->setFlags("-moqi_first");
                change.player->setFlags("-moqi_second");
            }
        }
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isNDTrick() && (use.from != nullptr) && use.from->isAlive() && use.from->getPhase() == Player::Play) {
                if (!use.from->hasFlag("moqi_first"))
                    use.from->setFlags("moqi_first");
                else
                    use.from->setFlags("moqi_second");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging)
            return QList<SkillInvokeDetail>();

        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->isNDTrick() && (use.from != nullptr) && use.from->isAlive() && use.from->getPhase() == Player::Play && !use.from->hasFlag("moqi_second")) {
            QList<ServerPlayer *> owners = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *p, owners)
                d << SkillInvokeDetail(this, p, p);
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

SishuCard::SishuCard()
{
    target_fixed = true;
}

static void do_sishu(ServerPlayer *player)
{
    Room *room = player->getRoom();
    int acquired = 0;
    QList<int> throwIds;
    while (acquired < 1) {
        int id = room->drawCard();
        CardsMoveStruct move(id, nullptr, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName()));
        move.reason.m_skillName = "sishu";
        room->moveCardsAtomic(move, true);
        room->getThread()->delay();
        Card *card = Sanguosha->getCard(id);
        if (card->isNDTrick()) {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getAllPlayers(), "sishu", QString(), false, true);
            acquired = acquired + 1;
            CardsMoveStruct move2(id, target, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_GOTBACK, target->objectName()));
            room->moveCardsAtomic(move2, true);
            if (target != player)
                room->recover(player, RecoverStruct());
            if (!throwIds.isEmpty()) {
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "sishu", QString());
                DummyCard dummy(throwIds);
                room->throwCard(&dummy, reason, nullptr);
                throwIds.clear();
            }
        } else
            throwIds << id;
    }
}

void SishuCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->doLightbox("$sishuAnimate", 4000);
    SkillCard::onUse(room, card_use);
}

void SishuCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;

    room->removePlayerMark(source, "@sishu");
    int num = 1 + source->getLostHp();
    for (int i = 0; i < num; i += 1)
        do_sishu(source);
}

class Sishu : public ZeroCardViewAsSkill
{
public:
    Sishu()
        : ZeroCardViewAsSkill("sishu")
    {
        frequency = Limited;
        limit_mark = "@sishu";
    }

    const Card *viewAs() const override
    {
        return new SishuCard;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return player->getMark("@sishu") >= 1;
    }
};

class Juxian : public TriggerSkill
{
public:
    Juxian()
        : TriggerSkill("juxian")
    {
        events << EnterDying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DyingStruct dying = data.value<DyingStruct>();

        if (dying.who->hasSkill(this) && dying.who->isAlive() && dying.who->getHp() < dying.who->dyingThreshold())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dying.who, dying.who);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->turnOver();

        if (!invoke->invoker->faceUp()) {
            QList<int> list = room->getNCards((room->alivePlayerCount() < 4) ? 1 : 3);
            CardsMoveStruct move(list, nullptr, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, invoke->invoker->objectName(), objectName(), QString()));
            room->moveCardsAtomic(move, true);

            QVariantList listc = IntList2VariantList(list);
            invoke->invoker->tag["juxian_cards"] = listc;
            Card::Suit suit = room->askForSuit(invoke->invoker, objectName());
            invoke->invoker->tag.remove("juxian_cards");

            room->touhouLogmessage("#ChooseSuit", invoke->invoker, Card::Suit2String(suit));

            QList<int> get;
            QList<int> thro;
            foreach (int id, list) {
                if (Sanguosha->getCard(id)->getSuit() != suit)
                    get << id;
                else
                    thro << id;
            }

            if (!get.isEmpty()) {
                DummyCard dummy(get);
                invoke->invoker->obtainCard(&dummy);
            }
            if (!thro.isEmpty()) {
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, invoke->invoker->objectName(), objectName(), QString());
                DummyCard dummy(thro);
                room->throwCard(&dummy, reason, nullptr);
                RecoverStruct recover;
                recover.recover = thro.length();
                room->recover(invoke->invoker, recover);
            }
        }

        return false;
    }
};

BanyueCard::BanyueCard()
{
}

bool BanyueCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return (targets.length() < 3);
}

void BanyueCard::use(Room *room, const CardUseStruct &card_use) const // onEffect is better?
{
    ServerPlayer *source = card_use.from;
    const QList<ServerPlayer *> &targets = card_use.to;

    foreach (ServerPlayer *p, targets) {
        if (p->isAlive())
            p->drawCards(1);
    }
    if (targets.length() >= 2)
        room->loseHp(source);
}

class Banyue : public ZeroCardViewAsSkill
{
public:
    Banyue()
        : ZeroCardViewAsSkill("banyue")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("BanyueCard");
    }

    const Card *viewAs() const override
    {
        return new BanyueCard;
    }
};

class Mizong : public TriggerSkill
{
public:
    Mizong()
        : TriggerSkill("mizong")
    {
        events << EventPhaseStart << FinishJudge << EventPhaseChanging;
        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && change.player->hasFlag("mizong")) {
                foreach (ServerPlayer *p, room->getOtherPlayers(change.player)) {
                    if (p->getMark("@mizong") > 0) {
                        room->setPlayerMark(p, "@mizong", 0);
                        room->setFixedDistance(change.player, p, -1);

                        QStringList assignee_list = change.player->property("extra_slash_specific_assignee").toString().split("+");
                        assignee_list.removeOne(p->objectName());
                        room->setPlayerProperty(change.player, "extra_slash_specific_assignee", assignee_list.join("+"));
                    }
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName() && judge->card->isRed())
                judge->pattern = "red";
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *, const QVariant &data) const override
    {
        if (event == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (!player->hasSkill(this) || player->isDead() || player->getPhase() != Player::Play)
                return QList<SkillInvokeDetail>();
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, nullptr, true);
        } else if (event == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName() && judge->card->isBlack() && !judge->ignore_judge) {
                if (judge->who->isAlive())
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, judge->who, judge->who, nullptr, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == EventPhaseStart) {
            bool optional = !invoke->invoker->hasShownSkill(this);

            ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(), "@mizong-ask", optional, true);
            if (target != nullptr) {
                room->notifySkillInvoked(invoke->owner, objectName());
                invoke->targets << target;
            }
            return target != nullptr;
        } else
            return true;
        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            JudgeStruct judge;
            judge.who = invoke->invoker;
            judge.reason = objectName();
            judge.good = true;
            judge.play_animation = false;
            room->judge(judge);

            if (judge.pattern == "red" && !judge.ignore_judge) {
                room->setPlayerFlag(invoke->invoker, "mizong");
                room->setFixedDistance(invoke->invoker, invoke->targets.first(), 1);
                invoke->targets.first()->gainMark("@mizong"); //gainMark could trigger skill Ganying

                QStringList assignee_list = invoke->invoker->property("extra_slash_specific_assignee").toString().split("+");
                assignee_list << invoke->targets.first()->objectName();
                room->setPlayerProperty(invoke->invoker, "extra_slash_specific_assignee", assignee_list.join("+"));
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            invoke->owner->obtainCard(judge->card);
        }
        return false;
    }
};

class Yinren : public TriggerSkill
{
public:
    Yinren()
        : TriggerSkill("yinren")
    {
        events << CardFinished << TargetSpecified; //EventPhaseChanging
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return;
            foreach (ServerPlayer *p, use.to) {
                QStringList yinren = p->property("YinrenInvalid").toStringList();
                if (!yinren.contains(use.card->toString()))
                    continue;

                yinren.removeOne(use.card->toString());
                room->setPlayerProperty(p, "YinrenInvalid", yinren);

                if (yinren.isEmpty() && p->hasFlag("yinren")) {
                    p->setFlags("-yinren");
                    room->setPlayerSkillInvalidity(p, nullptr, false);
                    room->removePlayerCardLimitation(p, "use,response", ".|red$1", objectName());
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *, const QVariant &data) const override
    {
        if (event != TargetSpecified)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from != nullptr && use.from->hasSkill(this) && use.card->isKindOf("Slash") && use.card->isBlack()) {
            foreach (ServerPlayer *p, use.to)
                d << SkillInvokeDetail(this, use.from, use.from, nullptr, false, p);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        return invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget));
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *target = invoke->targets.first();
        QStringList yinren = target->property("YinrenInvalid").toStringList();
        if (!yinren.contains(use.card->toString())) {
            yinren << use.card->toString();
            room->setPlayerProperty(target, "YinrenInvalid", yinren);
        }
        if (!target->hasFlag(objectName())) {
            target->setFlags(objectName());
            room->setPlayerSkillInvalidity(target, nullptr, true);
            room->setPlayerCardLimitation(target, "use,response", ".|red", objectName(), true);
        }
        return false;
    }
};

class XiaoyinVS : public OneCardViewAsSkill
{
public:
    XiaoyinVS()
        : OneCardViewAsSkill("xiaoyinVS")
    {
        response_pattern = "@@xiaoyinVS!";
        filter_pattern = ".";
        response_or_use = true;
    }

    const Card *viewAs(const Card *c) const override
    {
        LureTiger *lure = new LureTiger(Card::SuitToBeDecided, 0);
        lure->addSubcard(c);
        lure->setSkillName("_xiaoyin");
        return lure;
    }
};

class Xiaoyin : public TriggerSkill
{
public:
    Xiaoyin()
        : TriggerSkill("xiaoyin")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() != Player::Play || player->isDead() || player->isKongcheng())
            return QList<SkillInvokeDetail>();

        LureTiger *card = new LureTiger(Card::SuitToBeDecided, 0);
        card->deleteLater();
        if (player->isCardLimited(card, Card::MethodUse, true))
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (!player->isProhibited(p, card) && player->inMyAttackRange(p))
                d << SkillInvokeDetail(this, p, p, nullptr, false, player);
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = invoke->targets.first();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), target->objectName());
        room->setPlayerFlag(invoke->invoker, "Global_xiaoyinFailed");
        target->drawCards(1, objectName());
        const Card *card = room->askForUseCard(target, "@@xiaoyinVS!", "xiaoyinuse:" + invoke->invoker->objectName());
        if (card == nullptr) {
            //force use!
            foreach (const Card *c, target->getCards("hes")) {
                LureTiger *lure = new LureTiger(Card::SuitToBeDecided, 0);
                lure->addSubcard(c->getEffectiveId());
                lure->setSkillName("_xiaoyin");
                room->setCardFlag(lure, "lure_" + invoke->invoker->objectName());
                if (!target->isCardLimited(lure, Card::MethodUse, true) && !target->isProhibited(invoke->invoker, lure)) {
                    room->useCard(CardUseStruct(lure, target, invoke->invoker), false);
                    return false;
                } else
                    delete lure;
            }
            room->showAllCards(invoke->targets.first());
            room->getThread()->delay(1000);
            room->clearAG();
        }
        return false;
    }
};

class XiaoyinProhibit : public ProhibitSkill
{
public:
    XiaoyinProhibit()
        : ProhibitSkill("#xiaoyin")
    {
    }

    bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &, bool) const override
    {
        return card->getSkillName() == "xiaoyin" && !to->hasFlag("Global_xiaoyinFailed") && !card->hasFlag("lure_" + to->objectName());
    }
};

class Fenghua : public TriggerSkill
{
public:
    Fenghua()
        : TriggerSkill("fenghua")
    {
        events << GameStart << CardsMoveOneTime << TargetConfirmed << Debut;
        frequency = Compulsory;
        related_pile = "fenghua";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == GameStart || triggerEvent == Debut) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if ((player != nullptr) && player->hasSkill(this) && player->getPile(objectName()).isEmpty())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, nullptr, true);
        } else if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            QList<SkillInvokeDetail> d;
            if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
                foreach (ServerPlayer *p, use.to) {
                    if (!p->hasSkill(this) || p == use.from)
                        continue;
                    foreach (int id, p->getPile(objectName())) {
                        if (Sanguosha->getCard(id)->getSuit() == use.card->getSuit()) {
                            d << SkillInvokeDetail(this, p, p, nullptr, true);
                            break;
                        }
                    }
                }
            }
            return d;
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *satsuki = qobject_cast<ServerPlayer *>(move.from);
            if (satsuki != nullptr && satsuki->isAlive() && satsuki->hasSkill(this) && move.from_places.contains(Player::PlaceSpecial)
                && satsuki->getPile(objectName()).isEmpty()) {
                for (int i = 0; i < move.card_ids.size(); i++) {
                    if (move.from_pile_names.value(i) == objectName())
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, satsuki, satsuki, nullptr, true);
                }
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == GameStart || triggerEvent == Debut || triggerEvent == CardsMoveOneTime)
            invoke->invoker->addToPile(objectName(), room->getNCards(1));
        else if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.nullified_list << invoke->invoker->objectName();
            data = QVariant::fromValue(use);

            QList<int> ids = invoke->invoker->getPile(objectName());
            room->notifySkillInvoked(invoke->invoker, objectName());

            //room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
            LogMessage mes;
            mes.type = "$Fenghua";
            mes.from = invoke->invoker;
            mes.arg = objectName();
            mes.card_str = IntList2StringList(ids).join("+");
            room->sendLog(mes);

            DummyCard dummy(ids);
            room->obtainCard(invoke->invoker, &dummy);
        }
        return false;
    }
};

class Shixue : public TriggerSkill
{
public:
    Shixue()
        : TriggerSkill("shixue")
    {
        events << PreHpRecover << Damage;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == PreHpRecover) {
            RecoverStruct r = data.value<RecoverStruct>();
            if (r.to->hasSkill(this) && r.reason != objectName() && !r.to->hasFlag("Global_Dying"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, r.to, r.to, nullptr, true);
        }
        if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if ((damage.from != nullptr) && damage.from->isAlive() && damage.from->isWounded() && damage.from->hasSkill(this)) {
                QList<SkillInvokeDetail> d;
                for (int i = 0; i < damage.damage; ++i)
                    d << SkillInvokeDetail(this, damage.from, damage.from, nullptr, true);

                return d;
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        if (triggerEvent == PreHpRecover) {
            RecoverStruct r = data.value<RecoverStruct>();
            room->touhouLogmessage("#shixue1", invoke->invoker, objectName(), QList<ServerPlayer *>(), QString::number(r.recover));
            invoke->invoker->drawCards(2);
            return true;
        }
        if (invoke->invoker->isWounded()) {
            RecoverStruct recover;
            recover.who = invoke->invoker;
            recover.reason = objectName();
            room->recover(invoke->invoker, recover);
        }
        return false;
    }
};

class Ziye : public TriggerSkill
{
public:
    Ziye()
        : TriggerSkill("ziye")
    {
        events << Dying; // << Death;
        frequency = Wake;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        //DeathStruct death = data.value<DeathStruct>();
        DyingStruct dying = data.value<DyingStruct>();
        if ((dying.damage != nullptr) && (dying.damage->from != nullptr)) {
            ServerPlayer *player = dying.damage->from;
            if (player->hasSkill(this) && player->getMark(objectName()) == 0 && player != dying.who)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, nullptr, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->addPlayerMark(invoke->invoker, objectName());
        room->doLightbox("$ziyeAnimate", 4000);
        room->touhouLogmessage("#ZiyeWake", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        if (room->changeMaxHpForAwakenSkill(invoke->invoker))
            room->handleAcquireDetachSkills(invoke->invoker, "anyue");
        return false;
    }
};

class Anyue : public TriggerSkill
{
public:
    Anyue()
        : TriggerSkill("anyue")
    {
        events << HpRecover << TargetSpecified;
    }

    void record(TriggerEvent event, Room *, QVariant &data) const override
    {
        if (event == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != nullptr && use.card->isKindOf("Slash") && use.card->getSkillName() == objectName()) {
                foreach (ServerPlayer *p, use.to) {
                    p->addQinggangTag(use.card);
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *room, const QVariant &data) const override
    {
        if (event != HpRecover)
            return QList<SkillInvokeDetail>();
        RecoverStruct r = data.value<RecoverStruct>();
        if (r.to->hasFlag("Global_Dying"))
            return QList<SkillInvokeDetail>();

        bool can = false;
        foreach (ServerPlayer *p, room->getOtherPlayers(r.to)) {
            if (p->getHp() <= r.to->getHp()) {
                can = true;
                break;
            }
        }
        if (!can)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            Slash *newslash = new Slash(Card::NoSuit, 0);
            newslash->deleteLater();
            if (p->isCardLimited(newslash, Card::MethodUse))
                continue;
            if (r.to->isAlive() && r.to != p && p->canSlash(r.to, false))
                d << SkillInvokeDetail(this, p, p, nullptr, false, r.to);
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->turnOver();
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_anyue");
        room->useCard(CardUseStruct(slash, invoke->invoker, invoke->targets.first()), false);
        return false;
    }
};

TH06Package::TH06Package()
    : Package("th06")
{
    General *remilia = new General(this, "remilia$", "hmx", 3);
    remilia->addSkill(new SkltKexue);
    remilia->addSkill(new Mingyun);
    remilia->addSkill(new Xueyi);

    General *flandre = new General(this, "flandre", "hmx", 3);
    flandre->addSkill(new Pohuai);
    flandre->addSkill(new Yuxue);
    flandre->addSkill(new YuxueSlashNdl);
    flandre->addSkill(new Shengyan);
    related_skills.insertMulti("yuxue", "#yuxue-slash-ndl");

    General *sakuya = new General(this, "sakuya", "hmx", 4);
    sakuya->addSkill(new Suoding);
    sakuya->addSkill(new Huisu);

    General *patchouli = new General(this, "patchouli", "hmx", 3);
    patchouli->addSkill(new Bolan);
    patchouli->addSkill(new Hezhou);

    General *meirin = new General(this, "meirin", "hmx", 4);
    meirin->addSkill(new Taiji);
    meirin->addSkill(new Beishui);

    General *cirno = new General(this, "cirno", "hmx", 3);
    cirno->addSkill(new Dongjie);
    cirno->addSkill(new Bingpo);

    General *rumia = new General(this, "rumia", "hmx", 3);
    rumia->addSkill(new Zhenye);
    rumia->addSkill(new Anyu);

    General *koakuma = new General(this, "koakuma", "hmx", 3);
    koakuma->addSkill(new Moqi);
    koakuma->addSkill(new Sishu);

    General *daiyousei = new General(this, "daiyousei", "hmx", 3);
    daiyousei->addSkill(new Juxian);
    daiyousei->addSkill(new Banyue);

    General *sakuya_sp = new General(this, "sakuya_sp", "hmx", 4);
    sakuya_sp->addSkill(new Mizong);
    sakuya_sp->addSkill(new Yinren);

    General *satsuki = new General(this, "satsuki", "hmx", 3);
    satsuki->addSkill(new Xiaoyin);
    satsuki->addSkill(new XiaoyinProhibit);
    satsuki->addSkill(new Fenghua);
    related_skills.insertMulti("xiaoyin", "#xiaoyin");

    General *rumia_sp = new General(this, "rumia_sp", "hmx", 4);
    rumia_sp->addSkill(new Shixue);
    rumia_sp->addSkill(new Ziye);
    rumia_sp->addRelateSkill("anyue");

    addMetaObject<SkltKexueCard>();
    addMetaObject<SuodingCard>();
    addMetaObject<HezhouCard>();
    addMetaObject<SishuCard>();
    addMetaObject<ZhenyeCard>();
    addMetaObject<BanyueCard>();

    skills << new SkltKexueVS << new XiaoyinVS << new Anyue;
}

ADD_PACKAGE(TH06)
