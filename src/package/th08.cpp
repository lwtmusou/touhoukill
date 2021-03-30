#include "th08.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"
#include "th10.h"

class Yongheng : public TriggerSkill
{
public:
    Yongheng()
        : TriggerSkill("yongheng")
    {
        events << EventPhaseChanging << CardsMoveOneTime;
        frequency = Compulsory;
    }

    static void adjustHandcardNum(ServerPlayer *player, int card_num, QString reason)
    {
        Room *room = player->getRoom();
        int hc_num = player->getHandcardNum();
        if (card_num != hc_num) {
            room->touhouLogmessage("#TriggerSkill", player, reason);
            room->notifySkillInvoked(player, reason);
        }
        if (card_num > hc_num)
            player->drawCards(card_num - hc_num, reason);
        else if (card_num < hc_num)
            room->askForDiscard(player, reason, hc_num - card_num, hc_num - card_num);
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard && change.player->hasSkill(objectName()))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, nullptr, true);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QList<ServerPlayer *> kaguyas;
            ServerPlayer *kaguya1 = qobject_cast<ServerPlayer *>(move.from);
            ServerPlayer *kaguya2 = qobject_cast<ServerPlayer *>(move.to);
            if (kaguya1 && kaguya1->isAlive() && kaguya1->hasSkill(this) && move.from_places.contains(Player::PlaceHand) && kaguya1->getHandcardNum() != 4
                && kaguya1->getPhase() == Player::NotActive)
                kaguyas << kaguya1;
            if (kaguya2 && kaguya2->isAlive() && kaguya2->hasSkill(this) && move.to_place == Player::PlaceHand && kaguya2->getHandcardNum() != 4
                && kaguya2->getPhase() == Player::NotActive)
                kaguyas << kaguya2;
            if (kaguyas.length() > 1)
                std::sort(kaguyas.begin(), kaguyas.end(), ServerPlayer::CompareByActionOrder);
            if (!kaguyas.isEmpty()) {
                QList<SkillInvokeDetail> d;
                foreach (ServerPlayer *p, kaguyas)
                    d << SkillInvokeDetail(this, p, p, nullptr, true);
                return d;
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard) {
                room->touhouLogmessage("#TriggerSkill", invoke->invoker, "yongheng");
                room->notifySkillInvoked(invoke->invoker, objectName());
                invoke->invoker->skip(change.to);
                adjustHandcardNum(invoke->invoker, 4, "yongheng");
            }
        } else
            adjustHandcardNum(invoke->invoker, 4, "yongheng");
        return false;
    }
};

class Zhuqu : public TriggerSkill
{
public:
    Zhuqu()
        : TriggerSkill("zhuqu$")
    {
        events << FinishJudge;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (judge->card->getSuit() != Card::Diamond || judge->ignore_judge)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        if (judge->who->isAlive() && judge->who->getKingdom() == "yyc") {
            foreach (ServerPlayer *p, room->getOtherPlayers(judge->who)) {
                if (p->hasLordSkill(objectName()) && p->isWounded())
                    d << SkillInvokeDetail(this, p, judge->who);
            }
        }
        return d;
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        RecoverStruct recov;
        recov.recover = 1;
        recov.who = invoke->invoker;
        room->recover(invoke->owner, recov);

        return false;
    }
};

class Ruizhi : public TriggerSkill
{
public:
    Ruizhi()
        : TriggerSkill("ruizhi")
    {
        events << PostCardEffected;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.from && effect.to->hasSkill(this) && effect.to->isWounded() && effect.to->isAlive() && effect.card->isNDTrick()) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        QString prompt = "invoke:" + effect.card->objectName();
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        JudgeStruct judge;
        judge.who = invoke->invoker;
        judge.pattern = ".|red";
        judge.good = true;
        judge.reason = objectName();
        room->judge(judge);

        if (judge.isGood() && !judge.ignore_judge) {
            RecoverStruct recover;
            recover.recover = 1;
            room->recover(invoke->invoker, recover);
        }
        return false;
    }
};

MiyaoCard::MiyaoCard()
{
}

bool MiyaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;
    return !to_select->isKongcheng();
}

void MiyaoCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    if (effect.to->canDiscard(effect.to, "hs"))
        room->askForDiscard(effect.to, "miyao", 1, 1, false, false, "miyao_cardchosen");

    if (effect.to->isWounded()) {
        RecoverStruct recover;
        recover.recover = 1;
        recover.who = effect.from;
        room->recover(effect.to, recover);
    }
}

class Miyao : public ZeroCardViewAsSkill
{
public:
    Miyao()
        : ZeroCardViewAsSkill("miyao")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("MiyaoCard");
    }

    const Card *viewAs() const override
    {
        return new MiyaoCard;
    }
};

class Kaifeng : public TriggerSkill
{
public:
    Kaifeng()
        : TriggerSkill("kaifeng")
    {
        events << Damaged << DamageCaused;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Fire || !damage.from || damage.from == damage.to)
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

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QString prompt = "recover:" + QString::number(invoke->preferredTarget->getHp());
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }
    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *mokou = invoke->invoker;
        ServerPlayer *target = invoke->targets.first();
        RecoverStruct recover;
        recover.recover = target->getHp() - mokou->getHp();
        room->recover(mokou, recover);
        return false;
    }
};

class Fengxiang : public OneCardViewAsSkill
{
public:
    Fengxiang()
        : OneCardViewAsSkill("fengxiang")
    {
        filter_pattern = ".|red|.|hand";
        response_or_use = true;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const override
    {
        return matchAvaliablePattern("fire_attack", pattern) && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        FireAttack *card = new FireAttack(Card::SuitToBeDecided, -1);
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

KuangzaoCard::KuangzaoCard()
{
}

bool KuangzaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() == 0 && to_select->inMyAttackRange(Self) && to_select != Self;
}

void KuangzaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QString prompt = "@kuangzao-slash:" + source->objectName();
    const Card *card = room->askForUseSlashTo(targets.first(), source, prompt);
    if (card == nullptr)
        room->damage(DamageStruct("kuangzao", nullptr, targets.first()));
}

class Kuangzao : public ZeroCardViewAsSkill
{
public:
    Kuangzao()
        : ZeroCardViewAsSkill("kuangzao")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("KuangzaoCard");
    }

    const Card *viewAs() const override
    {
        return new KuangzaoCard;
    }
};

class Huanshi : public TriggerSkill
{
public:
    Huanshi()
        : TriggerSkill("huanshi")
    {
        events << TargetConfirming;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash"))
            return QList<SkillInvokeDetail>();
        QList<SkillInvokeDetail> d;
        use.card->setFlags("IgnoreFailed");
        foreach (ServerPlayer *to, use.to) {
            if (to->hasSkill(this)) {
                foreach (ServerPlayer *p, room->getOtherPlayers(to)) {
                    if (use.from->canSlash(p, use.card, true) && !use.to.contains(p) && use.from->inMyAttackRange(p)) {
                        d << SkillInvokeDetail(this, to, to);
                        break;
                    }
                }
            }
        }
        use.card->setFlags("-IgnoreFailed");
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *player = invoke->invoker;
        QList<ServerPlayer *> listt;
        use.card->setFlags("IgnoreFailed");
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (use.from->canSlash(p, use.card, true) && !use.to.contains(p) && use.from->inMyAttackRange(p))
                listt << p;
        }
        use.card->setFlags("-IgnoreFailed");
        player->tag["huanshi_source"] = data; //for ai
        ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@huanshi:" + use.from->objectName(), true, true);
        player->tag.remove("huanshi_source");
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.to << invoke->targets.first();
        room->sortByActionOrder(use.to);
        data = QVariant::fromValue(use);
        return false;
    }
};

class Xushi : public TriggerSkill
{
public:
    Xushi()
        : TriggerSkill("xushi")
    {
        events << CardUsed << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        //record times of using card
        if (e == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            ServerPlayer *player = use.from;

            if (player && player->getPhase() == Player::Play && !player->hasFlag("xushi_first") && use.card && use.card->getHandlingMethod() == Card::MethodUse
                && !use.card->isKindOf("SkillCard")) {
                bool ignore = (use.from && use.from->hasSkill("tianqu", false, false) && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY
                               && !use.from->hasFlag("IgnoreFailed"));

                if (!ignore && use.card->targetFixed(use.from))
                    return;

                bool invoke = false;
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (use.from == p)
                        continue;
                    foreach (ServerPlayer *t, room->getAlivePlayers()) {
                        if (use.to.contains(t))
                            continue;

                        if (!use.from->isProhibited(t, use.card) && use.card->targetFilter(QList<const Player *>(), t, use.from)) {
                            invoke = true;
                        }
                    }
                }

                if (invoke) {
                    player->setFlags("xushi_first");
                    room->setCardFlag(use.card, "xushi_first");
                }
            }
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                change.player->setFlags("-xushi_first");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e != CardUsed)
            return QList<SkillInvokeDetail>();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeSkill || use.from == nullptr || use.from->getPhase() != Player::Play || !use.card->hasFlag("xushi_first"))
            return QList<SkillInvokeDetail>();
        //just for skill "tianqu"
        bool ignore = (use.from && use.from->hasSkill("tianqu", false, false) && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY
                       && !use.from->hasFlag("IgnoreFailed"));

        if (!ignore && use.card->targetFixed(use.from))
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (use.from == p)
                continue;
            foreach (ServerPlayer *t, room->getAlivePlayers()) {
                if (use.to.contains(t))
                    continue;

                if (!use.from->isProhibited(t, use.card) && use.card->targetFilter(QList<const Player *>(), t, use.from)) {
                    d << SkillInvokeDetail(this, p, p);
                    break;
                }
            }
        }
        return d;
    }
    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "use:" + use.from->objectName() + ":" + use.to.first()->objectName() + ":" + use.card->objectName();
        invoke->invoker->tag["xushi_use"] = data;
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), use.from->objectName());

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *t, room->getAlivePlayers()) {
            if (use.to.contains(t))
                continue;
            if (!use.from->isProhibited(t, use.card) && use.card->targetFilter(QList<const Player *>(), t, use.from))
                targets << t;
        }

        if (!targets.isEmpty()) {
            use.from->tag["xushi_use"] = data;
            ServerPlayer *newTarget = room->askForPlayerChosen(use.from, targets, objectName(), "@xushi_newTarget:" + use.card->objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, use.from->objectName(), newTarget->objectName());
            room->touhouLogmessage("#xushi_newTarget", use.from, use.card->objectName(), QList<ServerPlayer *>() << newTarget);

            use.to.clear();
            use.to << newTarget;
            data = QVariant::fromValue(use);
        }

        return false;
    }
};

class Xinyue : public MasochismSkill
{
public:
    Xinyue()
        : MasochismSkill("xinyue")
    {
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive() && damage.to->isAlive() && damage.to->hasSkill(this) && damage.from->getHandcardNum() > damage.to->getHp()) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, nullptr, false, damage.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt = "target:" + damage.from->objectName() + "::" + QString::number(invoke->invoker->getHp());
        return invoke->invoker->askForSkillInvoke(objectName(), data, prompt);
    }

    void onDamaged(Room *room, QSharedPointer<SkillInvokeDetail> invoke, const DamageStruct &) const override
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        int x = invoke->targets.first()->getHandcardNum() - invoke->invoker->getHp();
        room->askForDiscard(invoke->targets.first(), objectName(), x, x, false, false);
    }
};

BuxianCard::BuxianCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool BuxianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < 2 && !to_select->isKongcheng() && to_select != Self;
}

bool BuxianCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void BuxianCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    QVariant data = QVariant::fromValue(use);
    RoomThread *thread = room->getThread();

    thread->trigger(PreCardUsed, room, data);
    use = data.value<CardUseStruct>();
    use.from->showHiddenSkill("buxian");
    LogMessage log;
    log.from = card_use.from;
    log.to << card_use.to;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    thread->trigger(CardUsed, room, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, data);
}

void BuxianCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
    room->moveCardTo(Sanguosha->getCard(subcards.first()), nullptr, Player::DrawPile);
    targets.first()->pindian(targets.last(), "buxian");
}

class BuxianVS : public OneCardViewAsSkill
{
public:
    BuxianVS()
        : OneCardViewAsSkill("buxian")
    {
        filter_pattern = ".|.|.|hand";
        response_pattern = "@@buxian";
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        BuxianCard *card = new BuxianCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Buxian : public TriggerSkill
{
public:
    Buxian()
        : TriggerSkill("buxian")
    {
        events << Damaged << EventPhaseStart;
        view_as_skill = new BuxianVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    static bool hasBuxianTarget(ServerPlayer *player)
    {
        int n = 0;
        foreach (ServerPlayer *p, player->getRoom()->getOtherPlayers(player)) {
            if (!p->isKongcheng())
                n++;
        }
        return n > 1;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Play && player->hasSkill(this) && !player->isKongcheng() && hasBuxianTarget(player))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->hasSkill(this) && damage.to->isAlive() && !damage.to->isKongcheng() && hasBuxianTarget(damage.to))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->askForUseCard(invoke->invoker, "@@buxian", "@buxian");
        return false;
    }
};

class BuxianEffect : public TriggerSkill
{
public:
    BuxianEffect()
        : TriggerSkill("#buxian")
    {
        events << Pindian;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->reason == "buxian")
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, nullptr, pindian->from, nullptr, true, nullptr, false);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        ServerPlayer *bigger = nullptr;
        if (pindian->from_number > pindian->to_number)
            bigger = pindian->from;
        else if (pindian->to_number > pindian->from_number)
            bigger = pindian->to;
        if (bigger != nullptr && (bigger == pindian->to || bigger == pindian->from)) {
            bigger->drawCards(1);
            room->damage(DamageStruct("buxian", nullptr, bigger));
        }
        return false;
    }
};

XingyunCard::XingyunCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
    m_skillName = "xingyun";
}

void XingyunCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    foreach (int id, subcards)
        room->showCard(source, id);
    source->tag["xingyun"] = QVariant::fromValue(subcards.length());
}

class XingyunVS : public ViewAsSkill
{
public:
    XingyunVS()
        : ViewAsSkill("xingyun")
    {
        response_pattern = "@@xingyun";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const override
    {
        return to_select->hasFlag("xingyun");
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() > 0) {
            XingyunCard *card = new XingyunCard;
            card->addSubcards(cards);
            return card;
        } else
            return nullptr;
    }
};

class Xingyun : public TriggerSkill
{
public:
    Xingyun()
        : TriggerSkill("xingyun")
    {
        events << CardsMoveOneTime;
        view_as_skill = new XingyunVS;
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
                    if (owner && owner == tewi)
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
                if (owner && owner == tewi)
                    room->setCardFlag(id, "xingyun");
            }
        }
        invoke->invoker->tag["xingyun_move"] = data;
        const Card *c = room->askForUseCard(tewi, "@@xingyun", "@xingyun");
        foreach (int id, move.card_ids)
            room->setCardFlag(id, "-xingyun");

        return c != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *player = invoke->invoker;
        int count = player->tag["xingyun"].toInt();
        player->tag.remove("xingyun");

        for (int i = 0; i < count; ++i) {
            QString choice = "letdraw";
            if (player->isWounded())
                choice = room->askForChoice(player, objectName(), "letdraw+recover", data);
            if (choice == "letdraw") {
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@xingyun-select");
                target->drawCards(1);
            } else if (choice == "recover") {
                RecoverStruct recover;
                recover.who = player;
                room->recover(player, recover);
            }
        }
        return false;
    }
};

YegeCard::YegeCard()
{
    handling_method = Card::MethodNone;
}

bool YegeCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty())
        return false;
    if (to_select->getPhase() != Player::Play)
        return false;
    Indulgence *indl = new Indulgence(Card::SuitToBeDecided, -1);
    indl->addSubcard(getEffectiveId());
    indl->setSkillName("yege");
    indl->deleteLater();

    bool canUse = !Self->isLocked(indl);
    if (canUse && to_select != Self && !to_select->containsTrick("indulgence") && !Self->isProhibited(to_select, indl))
        return true;
    return false;
}

const Card *YegeCard::validate(CardUseStruct &cardUse) const
{
    ServerPlayer *to = cardUse.to.first();
    if (!to->containsTrick("indulgence")) {
        Indulgence *indulgence = new Indulgence(Card::SuitToBeDecided, -1);
        indulgence->addSubcard(getEffectiveId());
        indulgence->setSkillName("yege");
        indulgence->setShowSkill("yege");
        return indulgence;
    }
    return this;
}

class YegeVS : public OneCardViewAsSkill
{
public:
    YegeVS()
        : OneCardViewAsSkill("yege")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
        response_pattern = "@@yege";
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        YegeCard *indl = new YegeCard;
        indl->addSubcard(originalCard);
        indl->setShowSkill("yege");
        indl->setSkillName(objectName());
        return indl;
    }
};

class Yege : public TriggerSkill
{
public:
    Yege()
        : TriggerSkill("yege")
    {
        events << EventPhaseStart << CardUsed;
        view_as_skill = new YegeVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (e == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() != Player::Play)
                return d;

            foreach (ServerPlayer *p, room->getAllPlayers()) {
                foreach (const Card *c, p->getCards("j")) {
                    if (c->isKindOf("Indulgence"))
                        return d;
                }
            }

            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p != player)
                    d << SkillInvokeDetail(this, p, p, nullptr, false, player);
            }
        } else if (isHegemonyGameMode(ServerInfo.GameMode)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != nullptr && use.card->isKindOf("Indulgence") && use.card->getSkillName() == "yege" && use.from != nullptr && use.from->hasSkill(this) && use.from->isAlive()
                && use.card->getSuit() == Card::Diamond)
                d << SkillInvokeDetail(this, use.from, use.from, nullptr, true);
        }
        return d;
    }

    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &v) const override
    {
        if (e == EventPhaseStart) {
            QString prompt = "@yege:" + invoke->preferredTarget->objectName();
            const Card *card = room->askForUseCard(invoke->invoker, "@@yege", prompt);
            return card != nullptr;
        }

        return TriggerSkill::cost(e, room, invoke, v);
    }

    bool effect(TriggerEvent e, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (e == EventPhaseStart) {
            if (!isHegemonyGameMode(ServerInfo.GameMode))
                invoke->invoker->drawCards(1, "yege");
        } else
            invoke->invoker->drawCards(1, "yege");
        return false;
    }
};

class Laolong : public TriggerSkill
{
public:
    Laolong()
        : TriggerSkill("laolong")
    {
        events << DamageCaused << DamageInflicted;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (e == DamageCaused) {
            if (damage.from && damage.to && damage.from->hasSkill(this) && damage.from->isAlive()) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    foreach (const Card *c, p->getCards("j")) {
                        if (c->isBlack() && damage.from->canDiscard(p, c->getEffectiveId()))
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);
                    }
                }
            }
        }
        if (e == DamageInflicted) {
            if (damage.to && damage.to->hasSkill(this) && damage.to->isAlive()) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    foreach (const Card *c, p->getCards("j")) {
                        if (c->isRed() && damage.to->canDiscard(p, c->getEffectiveId()))
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            foreach (const Card *c, p->getCards("j")) {
                if (((e == DamageCaused && c->isBlack()) || (e == DamageInflicted && c->isRed())) && invoke->invoker->canDiscard(p, c->getEffectiveId())) {
                    targets << p;
                    break;
                }
            }
        }
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt = (e == DamageCaused) ? ("@laolong1:" + damage.to->objectName() + ":" + QString::number(damage.damage)) : ("@laolong2:" + QString::number(damage.damage));

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), prompt, true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *target = invoke->targets.first();
        QList<int> disable;
        foreach (const Card *c, target->getCards("j")) {
            if ((e == DamageCaused && c->isRed()) || (e == DamageInflicted && c->isBlack()) || !invoke->invoker->canDiscard(target, c->getEffectiveId())) {
                disable << c->getEffectiveId();
            }
        }
        int card_id = room->askForCardChosen(invoke->invoker, target, "j", objectName(), false, Card::MethodDiscard, disable);
        room->throwCard(card_id, (target->getJudgingAreaID().contains(card_id)) ? nullptr : target, invoke->invoker);

        DamageStruct damage = data.value<DamageStruct>();
        int record = damage.damage;

        if (e == DamageInflicted)
            damage.damage = damage.damage - 1;

        if (e == DamageCaused)
            damage.damage = damage.damage + 1;

        room->touhouLogmessage("#laolong_damage", damage.to, QString::number(damage.damage), QList<ServerPlayer *>(), QString::number(record));
        data = QVariant::fromValue(damage);
        if (damage.damage == 0)
            return true;
        return false;
    }
};

class YinghuoClear : public TriggerSkill
{
public:
    YinghuoClear()
        : TriggerSkill("#yinghuo")
    {
        events << EventPhaseChanging;
    }

    static void yinghuo_record(Room *room, ServerPlayer *player, QString pattern)
    {
        if (pattern.contains("slash"))
            pattern = "slash";
        else if (pattern.contains("jink"))
            pattern = "jink";
        else if (pattern.contains("analeptic"))
            pattern = "analeptic";
        else if (pattern.contains("peach"))
            pattern = "peach";

        QString markName = "yinghuo_record_" + pattern;
        room->setPlayerMark(player, markName, 1);
    }

    static bool yinghuo_choice_limit(const Player *player, QString pattern)
    {
        Card *c = Sanguosha->cloneCard(pattern);
        DELETE_OVER_SCOPE(Card, c)
        if (pattern.contains("slash"))
            pattern = "slash";
        else if (pattern.contains("jink"))
            pattern = "jink";
        else if (pattern.contains("analeptic"))
            pattern = "analeptic";
        else if (pattern.contains("peach"))
            pattern = "peach";
        QString markName = "yinghuo_record_" + pattern;

        if (player->getMark(markName) > 0)
            return true;
        else
            return false;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return;
            QStringList patterns;
            QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
            foreach (const Card *card, cards) {
                if (!patterns.contains(card->objectName()))
                    patterns << card->objectName();
            }

            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                foreach (QString pattern, patterns) {
                    QString markName = "yinghuo_record_" + pattern;
                    if (p->getMark(markName) > 0)
                        room->setPlayerMark(p, markName, 0);
                }
            }
        }
    }
};

YinghuoCard::YinghuoCard()
{
    will_throw = false;
    handling_method = Card::MethodNone; //related to UseCardLimit
}

bool YinghuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return false;

    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(oc->objectName());
    DELETE_OVER_SCOPE(Card, new_card)

    new_card->setSkillName("yinghuo");
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool YinghuoCard::targetFixed(const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(oc->objectName());
    DELETE_OVER_SCOPE(Card, new_card)

    new_card->setSkillName("yinghuo");
    return new_card && new_card->targetFixed(Self);
}

bool YinghuoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(oc->objectName());
    DELETE_OVER_SCOPE(Card, new_card)

    new_card->setSkillName("yinghuo");
    return new_card && new_card->targetsFeasible(targets, Self);
}

const Card *YinghuoCard::validate(CardUseStruct &use) const
{
    use.from->showHiddenSkill("yinghuo");
    Room *room = use.from->getRoom();

    const Card *card = Sanguosha->getCard(subcards.first());
    YinghuoClear::yinghuo_record(room, use.from, card->objectName());
    Card *use_card = Sanguosha->cloneCard(card->objectName());
    use_card->setSkillName("yinghuo");
    use_card->deleteLater();
    use.from->addToShownHandCards(subcards);
    return use_card;
}

const Card *YinghuoCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();
    const Card *card = Sanguosha->getCard(subcards.first());
    YinghuoClear::yinghuo_record(room, user, card->objectName());
    Card *use_card = Sanguosha->cloneCard(card->objectName());
    use_card->setSkillName("yinghuo");
    use_card->deleteLater();
    user->addToShownHandCards(subcards);
    return use_card;
}

class Yinghuo : public OneCardViewAsSkill
{
public:
    Yinghuo()
        : OneCardViewAsSkill("yinghuo")
    {
    }

    static QStringList responsePatterns()
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();

        QStringList validPatterns;
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        QStringList ban_list = Sanguosha->getBanPackages();
        foreach (const Card *card, cards) {
            if ((card->isKindOf("BasicCard")) && !ban_list.contains(card->getPackage())) { //&& !ServerInfo.Extensions.contains("!" + card->getPackage())
                QString p = card->objectName();
                if (card->isKindOf("Slash"))
                    p = "slash";
                if (!validPatterns.contains(p))
                    validPatterns << card->objectName();
            }
        }

        QStringList checkedPatterns;
        foreach (QString str, validPatterns) {
            const Skill *skill = Sanguosha->getSkill("yinghuo");
            if (skill->matchAvaliablePattern(str, pattern) && !YinghuoClear::yinghuo_choice_limit(Self, str))
                checkedPatterns << str;
        }
        return checkedPatterns;
    }

    bool viewFilter(const QList<const Card *> &, const Card *card) const override
    {
        if (Self->isShownHandcard(card->getId()) || !card->isKindOf("BasicCard"))
            return false;

        if (YinghuoClear::yinghuo_choice_limit(Self, card->objectName()))
            return false;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "peach+analeptic")
                pattern = "peach,analeptic";
            ExpPattern p(pattern);
            return p.match(Self, card);
        } else {
            if (card->isKindOf("Jink"))
                return false;
            Card *zero_subcard = Sanguosha->cloneCard(card->objectName());
            DELETE_OVER_SCOPE(Card, zero_subcard)
            return zero_subcard->isAvailable(Self);
        }
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &) const override
    {
        if (player->getShownHandcards().length() >= player->getHandcardNum())
            return false;

        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.contains("peach") && checkedPatterns.length() == 1 && player->getMark("Global_PreventPeach") > 0)
            return false;
        return !checkedPatterns.isEmpty();
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        if (player->getShownHandcards().length() >= player->getHandcardNum())
            return false;

        if (Analeptic::IsAvailable(player) && !YinghuoClear::yinghuo_choice_limit(player, "analeptic"))
            return true;

        if (Slash::IsAvailable(player) && !YinghuoClear::yinghuo_choice_limit(player, "slash"))
            return true;
        Card *card = Sanguosha->cloneCard("peach", Card::NoSuit, 0);
        DELETE_OVER_SCOPE(Card, card)
        return card->isAvailable(player) && !YinghuoClear::yinghuo_choice_limit(player, "peach");
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        if (originalCard) {
            YinghuoCard *card = new YinghuoCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return nullptr;
    }
};

class Chongqun : public TriggerSkill
{
public:
    Chongqun()
        : TriggerSkill("chongqun")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
    }

    static bool hasChongqunTarget(ServerPlayer *player)
    {
        foreach (ServerPlayer *p, player->getRoom()->getOtherPlayers(player)) {
            if (!p->isNude() && player->canDiscard(p, "hs"))
                return true;
        }
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        if (e != CardsMoveOneTime)
            return QList<SkillInvokeDetail>();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!move.from_places.contains(Player::PlaceHand))
            return QList<SkillInvokeDetail>();

        if (!move.shown_ids.isEmpty()) {
            ServerPlayer *invoker = qobject_cast<ServerPlayer *>(move.from);
            if (invoker == nullptr || !invoker->hasSkill(this) || invoker->isDead() || !hasChongqunTarget(invoker))
                return QList<SkillInvokeDetail>();
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, invoker, invoker);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (!p->isNude() && invoke->invoker->canDiscard(p, "hs"))
                targets << p;
        }

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@chongqun_target", true, true);
        if (target) {
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

#pragma message WARN("todo_lwtmusou:check this skill after rewriting QijiDialog.")
class ChuangshiVS : public ZeroCardViewAsSkill
{
public:
    ChuangshiVS()
        : ZeroCardViewAsSkill("chuangshi")
    {
        response_pattern = "@@chuangshi";
    }

    const Card *viewAs() const override
    {
        QString name = Self->tag.value("chuangshi", QString()).toString();
        if (name != nullptr) {
            ChuangshiCard *card = new ChuangshiCard;
            card->setUserString(name);
            return card;
        }
        return nullptr;
    }
};

class Chuangshi : public TriggerSkill
{
public:
    Chuangshi()
        : TriggerSkill("chuangshi")
    {
        events << DrawNCards;
        view_as_skill = new ChuangshiVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    QDialog *getDialog() const override
    {
        return QijiDialog::getInstance("chuangshi");
    }

    static bool use_chuangshi(Room *room, ServerPlayer *player)
    {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), "chuangshi", "@chuangshi_target", true, true);
        if (target != nullptr) {
            player->showHiddenSkill("chuangshi");
            room->setPlayerMark(target, "chuangshi_user", 1);
            room->setPlayerProperty(player, "chuangshi_user", target->objectName());
            const Card *card = room->askForUseCard(player, "@@chuangshi", "@chuangshi_prompt:" + target->objectName());
            room->setPlayerMark(target, "chuangshi_user", 0);
            return card != nullptr;
        }
        return false;
    }
    static const Player *getChuangshiUser(const Player *player)
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->getMark("chuangshi_user") > 0)
                return p;
        }
        return nullptr;
    }

    static ServerPlayer *getChuangshiUser1(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getMark("chuangshi_user") > 0)
                return p;
        }
        return nullptr;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == DrawNCards) {
            DrawNCardsStruct qnum = data.value<DrawNCardsStruct>();
            if (qnum.player->hasSkill(this) && qnum.n > 0)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, qnum.player, qnum.player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        return use_chuangshi(room, invoke->invoker);
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        DrawNCardsStruct qnum = data.value<DrawNCardsStruct>();
        qnum.n = qnum.n - 1;
        data = QVariant::fromValue(qnum);
        return false;
    }
};

ChuangshiCard::ChuangshiCard()
{
    will_throw = false;
}

bool ChuangshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Player *user = Chuangshi::getChuangshiUser(Self);
    QString name = Self->tag.value("chuangshi", QString()).toString();
    Card *new_card = Sanguosha->cloneCard(name);
    DELETE_OVER_SCOPE(Card, new_card)
    new_card->setSkillName("chuangshi");

    if (new_card->targetFixed(Self))
        return false;
    if (new_card->isKindOf("FireAttack"))
        return new_card && (new_card->targetFilter(targets, to_select, user) || (to_select == user && !user->isKongcheng())) && !user->isProhibited(to_select, new_card, targets);
    else
        return new_card && new_card->targetFilter(targets, to_select, user) && !user->isProhibited(to_select, new_card, targets);
}

bool ChuangshiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Player *user = Chuangshi::getChuangshiUser(Self);
    QString name = Self->tag.value("chuangshi", QString()).toString();
    Card *new_card = Sanguosha->cloneCard(name);
    DELETE_OVER_SCOPE(Card, new_card)
    new_card->setSkillName("chuangshi");
    if (new_card->canRecast() && targets.length() == 0)
        return false;
    return new_card && new_card->targetsFeasible(targets, user);
}

void ChuangshiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *user = Chuangshi::getChuangshiUser1(source);
    Card *use_card = Sanguosha->cloneCard(user_string);

    room->setPlayerMark(user, "chuangshi_user", 0);
    CardUseStruct carduse;
    use_card->setSkillName("_chuangshi");
    carduse.card = use_card;
    carduse.from = user;
    carduse.to = targets;

    room->sortByActionOrder(carduse.to);
    room->useCard(carduse, true);
}

class Wangyue : public MasochismSkill
{
public:
    Wangyue()
        : MasochismSkill("wangyue")
    {
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive() && damage.to->isAlive() && damage.to->hasSkill(this) && damage.from->getHandcardNum() > damage.to->getHandcardNum()
            && damage.to->getHandcardNum() < 5) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, nullptr, false, damage.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        int num = qMin(5, damage.from->getHandcardNum());
        QString prompt = "target:" + damage.from->objectName() + "::" + QString::number(num);
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    void onDamaged(Room *, QSharedPointer<SkillInvokeDetail> invoke, const DamageStruct &) const override
    {
        int num = qMin(5, invoke->targets.first()->getHandcardNum());
        int x = num - invoke->invoker->getHandcardNum();
        invoke->invoker->drawCards(x);
    }
};

HuweiCard::HuweiCard()
{
    target_fixed = true;
}

const Card *HuweiCard::validate(CardUseStruct &cardUse) const
{
    cardUse.from->showHiddenSkill("huwei");
    Room *room = cardUse.from->getRoom();
    room->touhouLogmessage("#InvokeSkill", cardUse.from, "huwei");
    room->notifySkillInvoked(cardUse.from, "huwei");
    cardUse.from->drawCards(2);
    room->setPlayerFlag(cardUse.from, "Global_huweiFailed");
    return nullptr;
}

class HuweiVS : public ZeroCardViewAsSkill
{
public:
    HuweiVS()
        : ZeroCardViewAsSkill("huwei")
    {
    }

    const Card *viewAs() const override
    {
        return new HuweiCard;
    }

    bool isEnabledAtPlay(const Player *) const override
    {
        return false;
    }
    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        if ((!player->hasFlag("Global_huweiFailed") && matchAvaliablePattern("slash", pattern)
             && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)) {
            Slash *tmpslash = new Slash(Card::NoSuit, 0);
            tmpslash->deleteLater();
            if (player->isCardLimited(tmpslash, Card::MethodUse))
                return false;
            //check avaliable target
            foreach (const Player *p, player->getAliveSiblings()) {
                if (tmpslash->targetFilter(QList<const Player *>(), p, player))
                    return true;
            }
        }
        return false;
    }
};

class Huwei : public TriggerSkill
{
public:
    Huwei()
        : TriggerSkill("huwei")
    {
        events << CardAsked << CardUsed << CardsMoveOneTime << CardResponded;
        view_as_skill = new HuweiVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == CardAsked) {
            CardAskedStruct s = data.value<CardAskedStruct>();
            if (matchAvaliablePattern("slash", s.pattern) && s.player->hasSkill(this) && s.player->getPhase() != Player::Play) {
                Slash *tmpslash = new Slash(Card::NoSuit, 0);
                tmpslash->deleteLater();
                if (!s.player->isCardLimited(tmpslash, s.method))
                    d << SkillInvokeDetail(this, s.player, s.player);
            }
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.from->hasSkill(this) && use.from->getPhase() != Player::Play)
                d << SkillInvokeDetail(this, use.from, use.from);
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card->isKindOf("Slash") && resp.m_from->hasSkill(this) && resp.m_from->getPhase() != Player::Play)
                d << SkillInvokeDetail(this, resp.m_from, resp.m_from);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
            if (player != nullptr && player->isAlive() && player->hasSkill(this) && player->getPhase() != Player::Play
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                foreach (int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->isKindOf("Slash"))
                        d << SkillInvokeDetail(this, player, player);
                }
            }
        }

        return d;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == CardAsked) {
            return room->askForSkillInvoke(invoke->invoker, objectName(), data);
        } else {
            // how to notice player the remain times?
            ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(), "@huwei_targetdraw", true, true);
            if (target) {
                invoke->targets << target;
                return true;
            }
        }
        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == CardAsked)
            invoke->invoker->drawCards(2);
        else
            invoke->targets.first()->drawCards(2);
        return false;
    }
};

JinxiCard::JinxiCard()
{
    target_fixed = true;
}

void JinxiCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->doLightbox("$jinxiAnimate", 4000);
    SkillCard::onUse(room, card_use);
}

void JinxiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->removePlayerMark(source, "@jinxi");
    RecoverStruct recov;
    recov.recover = source->getMaxHp() - source->getHp();
    recov.who = source;
    room->recover(source, recov);
    if (source->getHandcardNum() < 4)
        source->drawCards(4 - source->getHandcardNum());
}

class Jinxi : public ZeroCardViewAsSkill
{
public:
    Jinxi()
        : ZeroCardViewAsSkill("jinxi")
    {
        frequency = Limited;
        limit_mark = "@jinxi";
    }

    const Card *viewAs() const override
    {
        return new JinxiCard;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return player->getMark("@jinxi") >= 1 && player->isWounded();
    }
};

class Yemang : public TriggerSkill
{
public:
    Yemang()
        : TriggerSkill("yemang")
    {
        events << EventPhaseChanging << EventPhaseStart;
        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from == Player::Play)
                room->setPlayerMark(phase_change.player, "yemang", 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current && current->isAlive() && current->getPhase() == Player::Play) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p->inMyAttackRange(current))
                        d << SkillInvokeDetail(this, p, p, nullptr, true, current);
                }
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *player = invoke->targets.first();
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), player->objectName());

        room->touhouLogmessage("#yemangRange", player, "yemang");
        room->setPlayerMark(player, "yemang", player->getMark("yemang") + 1);
        return false;
    }
};

class YemangRange : public AttackRangeSkill
{
public:
    YemangRange()
        : AttackRangeSkill("yemang_range")
    {
    }

    int getExtra(const Player *target, bool) const override
    {
        return -target->getMark("yemang");
    }
};

class MingmuRange : public AttackRangeSkill
{
public:
    MingmuRange()
        : AttackRangeSkill("mingmu_range")
    {
    }

    int getExtra(const Player *target, bool) const override
    {
        return target->getMark("mingmu");
    }
};

MingmuCard::MingmuCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "mingmu_attach";
}

bool MingmuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasSkill("mingmu") && to_select != Self && !to_select->hasFlag("mingmuInvoked");
}

void MingmuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *mystia = targets.first();
    room->setPlayerFlag(mystia, "mingmuInvoked");
    room->notifySkillInvoked(mystia, "mingmu");
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), mystia->objectName(), "mingmu", QString());
    room->obtainCard(mystia, this, reason, false);

    room->askForUseCard(mystia, "EquipCard|.|.|hand,wooden_ox", "@mingmu_equip");

    QStringList option;
    option << "mingmu_give"
           << "mingmu_disable";
    while (!option.isEmpty() && mystia->isAlive()) {
        QString choice = room->askForChoice(mystia, "mingmu", option.join("+"));
        if (choice == "mingmu_give") {
            const Card *card = room->askForCard(mystia, "Slash", "@mingmu:" + source->objectName(), QVariant(), Card::MethodNone, nullptr, false, objectName());
            if (card) {
                CardMoveReason reason1(CardMoveReason::S_REASON_GIVE, mystia->objectName(), source->objectName(), "mingmu", QString());
                room->obtainCard(source, card, reason1);
                if (!option.contains("cancel"))
                    option << "cancel";
            }
        } else if (choice == "mingmu_disable") {
            if (!option.contains("cancel"))
                option << "cancel";
            room->setPlayerMark(source, "mingmu", source->getMark("mingmu") + 1);
            room->touhouLogmessage("#mingmuRange", source, "mingmu");
        } else if (choice == "cancel")
            break;

        option.removeOne(choice);
        if (option.length() == 1 && option.first() == "cancel")
            break;
    }
}

class MingmuVS : public ViewAsSkill
{
public:
    MingmuVS()
        : ViewAsSkill("mingmu_attach")
    {
        attached_lord_skill = true;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasSkill("mingmu") && !p->hasFlag("mingmuInvoked"))
                return true;
        }
        return false;
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *) const override
    {
        return selected.length() < 2;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() > 0 && cards.length() <= 2) {
            MingmuCard *card = new MingmuCard;
            card->addSubcards(cards);
            return card;
        } else
            return nullptr;
    }
};

class Mingmu : public TriggerSkill
{
public:
    Mingmu()
        : TriggerSkill("mingmu")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging << Death << Debut << Revive << GeneralShown;
        show_type = "static";
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent != EventPhaseChanging) { //the case operating attach skill
            QList<ServerPlayer *> sources;
            static QString attachName = "mingmu_attach";
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(this, true))
                    sources << p;
            }

            if (sources.length() > 1) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->hasSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else if (sources.length() == 1) {
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
                room->setPlayerMark(phase_change.player, "mingmu", 0);
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("mingmuInvoked"))
                        room->setPlayerFlag(p, "-mingmuInvoked");
                }
            }
        }
    }
};

TH08Package::TH08Package()
    : Package("th08")
{
    General *kaguya = new General(this, "kaguya$", "yyc", 4);
    kaguya->addSkill(new Yongheng);
    kaguya->addSkill(new Zhuqu);

    General *eirin = new General(this, "eirin", "yyc", 4);
    eirin->addSkill(new Ruizhi);
    eirin->addSkill(new Miyao);

    General *mokou = new General(this, "mokou", "yyc", 4);
    mokou->addSkill(new Kaifeng);
    mokou->addSkill(new Fengxiang);

    General *reisen = new General(this, "reisen", "yyc", 4);
    reisen->addSkill(new Kuangzao);
    reisen->addSkill(new Huanshi);

    General *keine = new General(this, "keine", "yyc", 3);
    keine->addSkill(new Xushi);
    keine->addSkill(new Xinyue);

    General *tewi = new General(this, "tewi", "yyc", 3);
    tewi->addSkill(new Buxian);
    tewi->addSkill(new BuxianEffect);
    tewi->addSkill(new Xingyun);
    related_skills.insertMulti("buxian", "#buxian");

    General *mystia = new General(this, "mystia", "yyc", 3);
    mystia->addSkill(new Yege);
    mystia->addSkill(new Laolong);

    General *wriggle = new General(this, "wriggle", "yyc", 3);
    wriggle->addSkill(new Yinghuo);
    wriggle->addSkill(new YinghuoClear);
    wriggle->addSkill(new Chongqun);
    related_skills.insertMulti("yinghuo", "#yinghuo");

    General *keine_sp = new General(this, "keine_sp", "yyc", 3);
    keine_sp->addSkill(new Chuangshi);
    keine_sp->addSkill(new Wangyue);

    General *mokou_sp = new General(this, "mokou_sp", "yyc", 4);
    mokou_sp->addSkill(new Huwei);
    mokou_sp->addSkill(new Jinxi);

    General *mystia_sp = new General(this, "mystia_sp", "yyc", 3);
    mystia_sp->addSkill(new Yemang);
    mystia_sp->addSkill(new Mingmu);

    addMetaObject<MiyaoCard>();
    addMetaObject<KuangzaoCard>();
    addMetaObject<BuxianCard>();
    addMetaObject<XingyunCard>();
    addMetaObject<YegeCard>();
    addMetaObject<YinghuoCard>();
    addMetaObject<ChuangshiCard>();
    addMetaObject<HuweiCard>();
    addMetaObject<JinxiCard>();
    addMetaObject<MingmuCard>();

    skills << new MingmuVS << new YemangRange << new MingmuRange;
}

ADD_PACKAGE(TH08)
