#include "th08.h"
#include "th10.h"

#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"

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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard && change.player->hasSkill(objectName()))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, NULL, true);
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
                    d << SkillInvokeDetail(this, p, p, NULL, true);
                return d;
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (judge->card->getSuit() != Card::Diamond)
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->tag["zhuqu-target"] = QVariant::fromValue(invoke->owner);
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.from && effect.to->hasSkill(this) && effect.to->isWounded() && effect.to->isAlive() && effect.card->isNDTrick()) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        QString prompt = "invoke:" + effect.card->objectName();
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        JudgeStruct judge;
        judge.who = invoke->invoker;
        judge.pattern = ".|red";
        judge.good = true;
        judge.reason = objectName();
        room->judge(judge);

        if (judge.isGood()) {
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

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MiyaoCard");
    }

    virtual const Card *viewAs() const
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Fire || !damage.from || damage.from == damage.to)
            return QList<SkillInvokeDetail>();
        if (triggerEvent == DamageCaused) {
            if (damage.from->hasSkill(this) && damage.from->getHp() < damage.to->getHp())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, false, damage.to);
        } else if (triggerEvent == Damaged) {
            if (damage.to->hasSkill(this) && damage.to->getHp() < damage.from->getHp())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, false, damage.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString prompt = "recover:" + QString::number(invoke->preferredTarget->getHp());
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }
    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
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

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return matchAvaliablePattern("fire_attack", pattern) && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard) {
            FireAttack *card = new FireAttack(Card::SuitToBeDecided, -1);
            card->addSubcard(originalCard);
            card->setSkillName(objectName());
            return card;
        }
        return NULL;
    }
};

class Bumie : public TriggerSkill
{
public:
    Bumie()
        : TriggerSkill("bumie")
    {
        events << DamageInflicted << PreHpLost;
        frequency = Compulsory;
    }

    virtual int getPriority() const
    {
        return -100;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *mokou = NULL;
        int changehp = 0;
        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            changehp = damage.damage;
            mokou = damage.to;
        } else {
            HpLostStruct hplost = data.value<HpLostStruct>();
            changehp = hplost.num;
            mokou = hplost.player;
        }
        int threshold = mokou->dyingThreshold();
        if (mokou->hasSkill(this)) {
            if (mokou->getHp() - changehp < threshold)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, mokou, mokou, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *mokou = invoke->invoker;
        int changehp = mokou->getHp() - mokou->dyingThreshold();

        room->touhouLogmessage("#bumie01", mokou, "bumie", QList<ServerPlayer *>(), QString::number(changehp));
        room->notifySkillInvoked(mokou, objectName());
        if (changehp <= 0)
            return true;

        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            damage.damage = changehp;
            data = QVariant::fromValue(damage);
        } else {
            HpLostStruct hplost = data.value<HpLostStruct>();
            hplost.num = changehp;
            data = QVariant::fromValue(hplost);
        }

        return false;
    }
};

class BumieMaxhp : public TriggerSkill
{
public:
    BumieMaxhp()
        : TriggerSkill("#bumie")
    {
        events << HpChanged << CardsMoveOneTime;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *mokou = NULL;
        if (triggerEvent == HpChanged)
            mokou = data.value<ServerPlayer *>();
        else if (triggerEvent == CardsMoveOneTime)
            mokou = qobject_cast<ServerPlayer *>(data.value<CardsMoveOneTimeStruct>().from);

        if (mokou != NULL && mokou->isAlive() && mokou->hasSkill(this) && mokou->getHp() <= mokou->dyingThreshold() && mokou->isKongcheng())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, mokou, mokou, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, "bumie");
        room->notifySkillInvoked(invoke->invoker, "bumie");

        CardsMoveStruct move;
        move.to = invoke->invoker;
        move.to_place = Player::PlaceHand;
        int id = room->drawCard(false);
        move.card_ids << id;
        room->moveCardsAtomic(move, false);
        room->showCard(invoke->invoker, id);

        if (Sanguosha->getCard(id)->getSuit() != Card::Diamond) {
            room->loseMaxHp(invoke->invoker);
            if (invoke->invoker->isWounded()) {
                RecoverStruct recover;
                recover.recover = invoke->invoker->getLostHp();
                room->recover(invoke->invoker, recover);
            }
        }
        return false;
    }
};

class Lizhan : public TriggerSkill
{
public:
    Lizhan()
        : TriggerSkill("lizhan")
    {
        events << EventPhaseEnd << CardsMoveOneTime << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        //need this for turn broken? or @pingyi or multiple moukou?
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Discard) {
                foreach (ServerPlayer *mokou, room->getAllPlayers())
                    mokou->tag.remove("lizhan");
            }
        }

        if (e != CardsMoveOneTime)
            return;

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!move.from || move.from->getPhase() != Player::Discard)
            return;
        if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            foreach (ServerPlayer *mokou, room->findPlayersBySkillName(objectName())) {
                if (mokou != move.from) {
                    QVariantList LizhanToGet = mokou->tag["lizhan"].toList();
                    foreach (int card_id, move.card_ids) {
                        if (!LizhanToGet.contains(card_id) && Sanguosha->getCard(card_id)->isKindOf("Slash"))
                            LizhanToGet << card_id;
                    }
                    mokou->tag["lizhan"] = LizhanToGet;
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == EventPhaseEnd) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current->getPhase() == Player::Discard) {
                QList<SkillInvokeDetail> d;

                QList<ServerPlayer *> mokous = room->findPlayersBySkillName(objectName());
                foreach (ServerPlayer *mokou, mokous) {
                    if (mokou->isCurrent())
                        continue;
                    QVariantList ids = mokou->tag["lizhan"].toList();
                    QList<int> get_ids;
                    foreach (QVariant card_data, ids) {
                        if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
                            get_ids << card_data.toInt();
                    }
                    if (get_ids.length() == 0) {
                        mokou->tag.remove("lizhan");
                        continue;
                    }
                    d << SkillInvokeDetail(this, mokou, mokou, NULL, false, current);
                }
                return d;
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->tag["lizhan_target"] = QVariant::fromValue(invoke->preferredTarget);
        QString prompt = "target:" + invoke->preferredTarget->objectName();

        if (invoke->invoker->askForSkillInvoke(objectName(), prompt))
            return true;
        invoke->invoker->tag.remove("lizhan");
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *current = invoke->targets.first();

        QString prompt = "@lizhan_slash:" + current->objectName();
        QVariantList ids = invoke->invoker->tag["lizhan"].toList();
        invoke->invoker->tag.remove("lizhan");

        QList<int> all;
        foreach (QVariant card_data, ids) {
            if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
                all << card_data.toInt();
        }

        room->fillAG(all, invoke->invoker);
        int id = room->askForAG(invoke->invoker, all, false, objectName());
        room->clearAG(invoke->invoker);
        Card *lizhan_slash = Sanguosha->getCard(id);
        invoke->invoker->obtainCard(lizhan_slash, true);
        Slash *tmpslash = new Slash(Card::NoSuit, 0);
        tmpslash->deleteLater();
        if (invoke->invoker->isCardLimited(tmpslash, Card::MethodUse, true))
            return false;
        if (!invoke->invoker->canSlash(current, tmpslash, false))
            return false;
        QList<ServerPlayer *> victims;
        victims << current;
        room->askForUseSlashTo(invoke->invoker, victims, prompt, false, false, false);

        return false;
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
    if (card == NULL)
        room->damage(DamageStruct("kuangzao", NULL, targets.first()));
}

class Kuangzao : public ZeroCardViewAsSkill
{
public:
    Kuangzao()
        : ZeroCardViewAsSkill("kuangzao")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("KuangzaoCard");
    }

    virtual const Card *viewAs() const
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.to << invoke->targets.first();
        room->sortByActionOrder(use.to);
        data = QVariant::fromValue(use);
        return false;
    }
};

class Shishi : public TriggerSkill
{
public:
    Shishi()
        : TriggerSkill("shishi")
    {
        events << CardUsed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
            foreach (ServerPlayer *keine, room->findPlayersBySkillName(objectName())) {
                if (use.from != keine && keine->getPile("lishi").length() == 0)
                    d << SkillInvokeDetail(this, keine, keine);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "use:" + use.from->objectName() + ":" + use.card->objectName();
        invoke->invoker->tag["shishi_use"] = data;
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *src = invoke->invoker;
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, src->objectName(), use.from->objectName());

        if (use.card && room->getCardPlace(use.card->getEffectiveId()) == Player::PlaceTable)
            src->addToPile("lishi", use.card, true);
        //addtopile will clear cardflag(especially  use.from is robot ai )

        if (use.card->isKindOf("Nullification")) {
            room->touhouLogmessage("#weiya", use.from, objectName(), QList<ServerPlayer *>(), use.card->objectName());
            room->setPlayerFlag(use.from, "nullifiationNul");
        } else {
            use.nullified_list << "_ALL_TARGETS";
            data = QVariant::fromValue(use);
        }
        if (src->canDiscard(src, "hs"))
            room->askForDiscard(src, objectName(), 1, 1, false, false, "shishi_discard");

        return false;
    }
};

class Shouye : public TriggerSkill
{
public:
    Shouye()
        : TriggerSkill("shouye")
    {
        events << EventPhaseStart << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Start && player->hasSkill(this) && player->getPile("lishi").length() > 0)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->hasSkill(this) && damage.to->isAlive() && damage.to->getPile("lishi").length() > 0)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(), "@" + objectName(), true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<int> pile = invoke->invoker->getPile("lishi");
        DummyCard dummy(pile);
        room->obtainCard(invoke->targets.first(), &dummy);
        return false;
    }
};

class Xushi : public TriggerSkill
{
public:
    Xushi()
        : TriggerSkill("xushi")
    {
        events << TargetSpecifying << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &) const
    {
        if (e == EventPhaseChanging) {
            foreach (ServerPlayer *t, room->getAlivePlayers()) {
                room->setPlayerFlag(t, "-xushi_used");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e != TargetSpecifying)
            return QList<SkillInvokeDetail>();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeEquip || use.card->getTypeId() == Card::TypeSkill || use.to.length() != 1 || use.from == NULL)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (use.to.contains(p) || use.from == p || p->hasFlag("xushi_used"))
                continue;
            foreach (ServerPlayer *t, room->getOtherPlayers(use.to.first())) {
                if (!use.from->isProhibited(t, use.card) && use.card->targetFilter(QList<const Player *>(), t, use.from)
                    && (!p->getCards("h").isEmpty() || !t->getCards("s").isEmpty())) {
                    d << SkillInvokeDetail(this, p, p);
                    break;
                }
            }
        }
        return d;
    }
    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "use:" + use.from->objectName() + ":" + use.to.first()->objectName() + ":" + use.card->objectName();
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        invoke->invoker->setFlags("xushi_used");
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), use.from->objectName());
        QList<ServerPlayer *> targets;
        if (!invoke->invoker->getCards("h").isEmpty())
            targets << invoke->invoker;

        if (!use.to.first()->getCards("s").isEmpty())
            targets << use.to.first();

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@xushi");
        QString flag = (target == invoke->invoker) ? "h" : "s";
        int card_id = room->askForCardChosen(invoke->invoker, target, flag, objectName());
        if (flag == "h")
            target->addToShownHandCards(QList<int>() << card_id);
        else
            target->removeShownHandCards(QList<int>() << card_id, true);

        QList<ServerPlayer *> victims;
        foreach (ServerPlayer *t, room->getOtherPlayers(use.to.first())) {
            if (!use.from->isProhibited(t, use.card) && use.card->targetFilter(QList<const Player *>(), t, use.from))
                victims << t;
        }
        if (!victims.isEmpty()) {
            ServerPlayer *newTarget = room->askForPlayerChosen(use.from, victims, objectName(), "@xushi_newTarget:" + use.card->objectName());
            use.to.clear();
            use.to << newTarget;
            room->touhouLogmessage("#xushi_newTarget", use.from, use.card->objectName(), QList<ServerPlayer *>() << newTarget);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, use.from->objectName(), newTarget->objectName());
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive() && damage.to->isAlive() && damage.to->hasSkill(this) && damage.from->getHandcardNum() > damage.to->getHp()) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, false, damage.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        invoke->invoker->tag["xinyue_target"] = QVariant::fromValue(damage.from);
        QString prompt = "target:" + damage.from->objectName() + "::" + QString::number(invoke->invoker->getHp());
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    void onDamaged(Room *room, QSharedPointer<SkillInvokeDetail> invoke, const DamageStruct &) const
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
    room->moveCardTo(Sanguosha->getCard(subcards.first()), NULL, Player::DrawPile);
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

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard) {
            BuxianCard *card = new BuxianCard;
            card->addSubcard(originalCard);
            return card;
        }
        return NULL;
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

    static bool hasBuxianTarget(ServerPlayer *player)
    {
        int n = 0;
        foreach (ServerPlayer *p, player->getRoom()->getOtherPlayers(player)) {
            if (!p->isKongcheng())
                n++;
        }
        return n > 1;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->reason == "buxian")
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, pindian->from, pindian->from, NULL, true, NULL, false);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        ServerPlayer *bigger = NULL;
        if (pindian->from_number > pindian->to_number)
            bigger = pindian->from;
        else if (pindian->to_number > pindian->from_number)
            bigger = pindian->to;
        if (bigger != NULL && (bigger == pindian->to || bigger == pindian->from)) {
            bigger->drawCards(1);
            room->damage(DamageStruct("buxian", NULL, bigger));
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

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return to_select->hasFlag("xingyun");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0) {
            XingyunCard *card = new XingyunCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        if (room->getTag("FirstRound").toBool())
            return QList<SkillInvokeDetail>();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *tewi = qobject_cast<ServerPlayer *>(move.to);
        if (tewi != NULL && tewi->hasSkill(this) && move.to_place == Player::PlaceHand) {
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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

        return c != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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
    Indulgence *indl = new Indulgence(getSuit(), getNumber());
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
        Indulgence *indulgence = new Indulgence(getSuit(), getNumber());
        indulgence->addSubcard(getEffectiveId());
        indulgence->setSkillName("yege");
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

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard) {
            YegeCard *indl = new YegeCard;
            indl->addSubcard(originalCard);
            indl->setSkillName(objectName());
            return indl;
        }
        return NULL;
    }
};

class Yege : public TriggerSkill
{
public:
    Yege()
        : TriggerSkill("yege")
    {
        events << EventPhaseStart;
        view_as_skill = new YegeVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() != Player::Play)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            foreach (const Card *c, p->getCards("j")) {
                if (c->isKindOf("Indulgence"))
                    return QList<SkillInvokeDetail>();
            }
        }

        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p != player)
                d << SkillInvokeDetail(this, p, p, NULL, false, player);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString prompt = "@yege:" + invoke->preferredTarget->objectName();
        const Card *card = room->askForUseCard(invoke->invoker, "@@yege", prompt);
        return card != NULL;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1);
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (e == DamageCaused) {
            if (damage.from && damage.to && damage.to != damage.from && damage.from->hasSkill(this) && damage.from->isAlive()) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    foreach (const Card *c, p->getCards("j")) {
                        if (c->isBlack() && damage.from->canDiscard(p, c->getEffectiveId()))
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);
                    }
                }
            }
        }
        if (e == DamageInflicted) {
            if (damage.from && damage.to && damage.to != damage.from && damage.to->hasSkill(this) && damage.to->isAlive()) {
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

    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            foreach (const Card *c, p->getCards("j")) {
                if (((e == DamageCaused && c->isBlack()) || e == DamageInflicted && c->isRed()) && invoke->invoker->canDiscard(p, c->getEffectiveId())) {
                    targets << p;
                    break;
                }
            }
        }
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt = (e == DamageCaused) ? ("@laolong1:" + damage.to->objectName() + ":" + QString::number(damage.damage))
                                             : ("@laolong2:" + damage.from->objectName() + ":" + QString::number(damage.damage));

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), prompt, true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *target = invoke->targets.first();
        QList<int> disable;
        foreach (const Card *c, target->getCards("j")) {
            if ((e == DamageCaused && c->isRed()) || (e == DamageInflicted && c->isBlack()) || !invoke->invoker->canDiscard(target, c->getEffectiveId())) {
                disable << c->getEffectiveId();
            }
        }
        int card_id = room->askForCardChosen(invoke->invoker, target, "j", objectName(), false, Card::MethodDiscard, disable);
        room->throwCard(card_id, (target->getJudgingAreaID().contains(card_id)) ? NULL : target, invoke->invoker);

        DamageStruct damage = data.value<DamageStruct>();
        int record = damage.damage;
        QList<ServerPlayer *> logto;
        logto << damage.to;
        if (e == DamageInflicted)
            damage.damage = damage.damage - 1;

        if (e == DamageCaused)
            damage.damage = damage.damage + 1;
        room->touhouLogmessage("#laolong_damage", damage.from, QString::number(damage.damage), logto, QString::number(record));
        data = QVariant::fromValue(damage);
        if (damage.damage == 0)
            return true;
        return false;
    }
};

GeshengCard::GeshengCard()
{
    handling_method = Card::MethodNone;
}

bool GeshengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty())
        return false;
    if (to_select->getPhase() != Player::Judge)
        return false;
    Indulgence *indl = new Indulgence(getSuit(), getNumber());
    indl->addSubcard(getEffectiveId());
    indl->setSkillName("gesheng");
    indl->deleteLater();

    bool canUse = !Self->isLocked(indl);
    if (canUse && to_select != Self && !to_select->containsTrick("indulgence") && !Self->isProhibited(to_select, indl))
        return true;
    return false;
}

const Card *GeshengCard::validate(CardUseStruct &cardUse) const
{
    ServerPlayer *to = cardUse.to.first();
    if (!to->containsTrick("indulgence")) {
        Indulgence *indulgence = new Indulgence(getSuit(), getNumber());
        indulgence->addSubcard(getEffectiveId());
        indulgence->setSkillName("gesheng");
        return indulgence;
    }
    return this;
}

class GeshengVS : public OneCardViewAsSkill
{
public:
    GeshengVS()
        : OneCardViewAsSkill("gesheng")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
        response_pattern = "@@gesheng";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard) {
            GeshengCard *indl = new GeshengCard;
            indl->addSubcard(originalCard);
            indl->setSkillName(objectName());
            return indl;
        }
        return NULL;
    }
};

class Gesheng : public TriggerSkill
{
public:
    Gesheng()
        : TriggerSkill("gesheng")
    {
        events << EventPhaseChanging << EventPhaseStart << EventPhaseEnd;
        view_as_skill = new GeshengVS;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Judge) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("gesheng"))
                        room->setPlayerFlag(p, "-gesheng");
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging)
            return QList<SkillInvokeDetail>();

        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() != Player::Judge)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseStart) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                foreach (const Card *c, p->getCards("j")) {
                    if (c->isKindOf("Indulgence") && c->getSuit() != Card::Diamond)
                        return QList<SkillInvokeDetail>();
                }
            }

            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p != player)
                    d << SkillInvokeDetail(this, p, p, NULL, false, player);
            }
        } else if (triggerEvent == EventPhaseEnd) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasFlag("gesheng")) {
                    d << SkillInvokeDetail(this, p, p, NULL, true, player);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart) {
            return invoke->invoker->askForSkillInvoke(objectName(), QVariant::fromValue(invoke->preferredTarget));
        }
        return true;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *src = invoke->invoker;
        ServerPlayer *player = invoke->targets.first();
        if (triggerEvent == EventPhaseStart) {
            room->setPlayerFlag(src, "gesheng");
            src->drawCards(1);
        } else if (triggerEvent == EventPhaseEnd) {
            bool canuse = true;
            if (player->isDead())
                canuse = false;
            Indulgence *indl = new Indulgence(Card::NoSuit, 0);
            indl->deleteLater();
            if (src->isCardLimited(indl, Card::MethodUse, true))
                canuse = false;
            if (player->containsTrick("indulgence") || src->isProhibited(player, indl))
                canuse = false;
            if (canuse) {
                QString prompt = "@gesheng:" + player->objectName();
                const Card *card = room->askForUseCard(src, "@@gesheng", prompt);
                if (!card)
                    room->loseHp(src, 2);
            } else
                room->loseHp(src, 2);
        }
        return false;
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

bool YinghuoCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(oc->objectName());
    DELETE_OVER_SCOPE(Card, new_card)

    new_card->setSkillName("yinghuo");
    return new_card && new_card->targetFixed();
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
    const Card *card = Sanguosha->getCard(subcards.first());
    Card *use_card = Sanguosha->cloneCard(card->objectName());
    use_card->setSkillName("yinghuo");
    use_card->deleteLater();
    use.from->addToShownHandCards(subcards);
    return use_card;
}

const Card *YinghuoCard::validateInResponse(ServerPlayer *user) const
{
    const Card *card = Sanguosha->getCard(subcards.first());
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
        Card::HandlingMethod method;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            method = Card::MethodResponse;
        else
            method = Card::MethodUse;

        QStringList validPatterns;
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        foreach (const Card *card, cards) {
            if ((card->isKindOf("BasicCard")) && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
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
            if (skill->matchAvaliablePattern(str, pattern))
                checkedPatterns << str;
        }
        return checkedPatterns;
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *card) const
    {
        if (Self->isShownHandcard(card->getId()) || !card->isKindOf("BasicCard"))
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

    virtual bool isEnabledAtResponse(const Player *player, const QString &) const
    {
        if (player->getShownHandcards().length() >= player->getHandcardNum())
            return false;

        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.contains("peach") && checkedPatterns.length() == 1 && player->getMark("Global_PreventPeach") > 0)
            return false;
        return !checkedPatterns.isEmpty();
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getShownHandcards().length() >= player->getHandcardNum())
            return false;

        if (Analeptic::IsAvailable(player) || Slash::IsAvailable(player))
            return true;
        Card *card = Sanguosha->cloneCard("peach", Card::NoSuit, 0);
        DELETE_OVER_SCOPE(Card, card)
        return card->isAvailable(player);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard) {
            YinghuoCard *card = new YinghuoCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e != CardsMoveOneTime)
            return QList<SkillInvokeDetail>();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!move.from_places.contains(Player::PlaceHand))
            return QList<SkillInvokeDetail>();

        if (!move.shown_ids.isEmpty()) {
            ServerPlayer *invoker = qobject_cast<ServerPlayer *>(move.from);
            if (invoker == NULL || !invoker->hasSkill(this) || invoker->isDead() || !hasChongqunTarget(invoker))
                return QList<SkillInvokeDetail>();
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, invoker, invoker);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        //room->askForDiscard(invoke->targets.first(), objectName(), 1, 1, false, true, "chongqun_discard:" + invoke->invoker->objectName());
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

    virtual const Card *viewAs() const
    {
        QString name = Self->tag.value("chuangshi", QString()).toString();
        if (name != NULL) {
            ChuangshiCard *card = new ChuangshiCard;
            card->setUserString(name);
            return card;
        }
        return NULL;
    }
};

class Chuangshi : public TriggerSkill
{
public:
    Chuangshi()
        : TriggerSkill("chuangshi")
    {
        events << EventPhaseStart << DrawNCards << EventPhaseChanging;
        view_as_skill = new ChuangshiVS;
    }

    virtual QDialog *getDialog() const
    {
        return QijiDialog::getInstance("chuangshi");
    }

    static bool use_chuangshi(Room *room, ServerPlayer *player)
    {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), "chuangshi", "@chuangshi_target", true, true);
        if (target != NULL) {
            player->showHiddenSkill("chuangshi");
            room->setPlayerMark(target, "chuangshi_user", 1);
            room->setPlayerProperty(player, "chuangshi_user", target->objectName());
            const Card *card = room->askForUseCard(player, "@@chuangshi", "@chuangshi_prompt:" + target->objectName());
            room->setPlayerMark(target, "chuangshi_user", 0);
            return card != NULL;
        }
        return false;
    }
    static const Player *getChuangshiUser(const Player *player)
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->getMark("chuangshi_user") > 0)
                return p;
        }
        return NULL;
    }

    static ServerPlayer *getChuangshiUser1(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getMark("chuangshi_user") > 0)
                return p;
        }
        return NULL;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *player, room->getAllPlayers()) {
                if (player->hasFlag("chuangshi"))
                    player->setFlags("-chuangshi");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->getPhase() == Player::Draw) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            }
        } else if (triggerEvent == DrawNCards) {
            DrawNCardsStruct qnum = data.value<DrawNCardsStruct>();
            if (qnum.player->hasFlag("chuangshi")) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, qnum.player, qnum.player, NULL, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart) {
            use_chuangshi(room, invoke->invoker);
        } else if (triggerEvent == DrawNCards)
            return true;
        return false;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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

    if (new_card->targetFixed())
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
    //if (card->isKindOf("IronChain") && targets.length() == 0)
    if (new_card->canRecast() && targets.length() == 0)
        return false;
    return new_card && new_card->targetsFeasible(targets, user);
}

void ChuangshiCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    card_use.from->setFlags("chuangshi");
    Card *card = Sanguosha->cloneCard(user_string);
    DELETE_OVER_SCOPE(Card, card)
    if (card->isKindOf("Collateral")) {
        ServerPlayer *from = card_use.from; //ensure that the length of use.to should be 2.
        ServerPlayer *to1 = card_use.to.at(0);
        ServerPlayer *to2 = card_use.to.at(1);
        QList<ServerPlayer *> logto;
        logto << to1 << to2;

        ServerPlayer *chuangshi_user = Chuangshi::getChuangshiUser1(from);

        room->setPlayerMark(chuangshi_user, "chuangshi_user", 0);
        room->touhouLogmessage("#ChoosePlayerWithSkill", from, "chuangshi", logto, "");
        Card *use_card = Sanguosha->cloneCard(card->objectName());
        use_card->setSkillName("_chuangshi");
        CardUseStruct use;
        use.from = chuangshi_user;
        use.to = card_use.to;
        use.card = use_card;
        room->useCard(use);
    } else
        SkillCard::onUse(room, card_use);
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive() && damage.to->isAlive() && damage.to->hasSkill(this) && damage.from->getHandcardNum() > damage.to->getHandcardNum()
            && damage.to->getHandcardNum() < 5) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, false, damage.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        //invoke->invoker->tag["wangyue_target"] = QVariant::fromValue(damage.from);
        int num = qMin(5, damage.from->getHandcardNum());
        QString prompt = "target:" + damage.from->objectName() + "::" + QString::number(num);
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    void onDamaged(Room *, QSharedPointer<SkillInvokeDetail> invoke, const DamageStruct &) const
    {
        //room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        int num = qMin(5, invoke->targets.first()->getHandcardNum());
        int x = num - invoke->invoker->getHandcardNum();
        invoke->invoker->drawCards(x);
        //room->askForDiscard(invoke->targets.first(), "wangyue", x, x, false, false);
    }
};

HuweiCard::HuweiCard()
{
    target_fixed = true;
}

const Card *HuweiCard::validate(CardUseStruct &cardUse) const
{
    Room *room = cardUse.from->getRoom();
    room->touhouLogmessage("#InvokeSkill", cardUse.from, "huwei");
    room->notifySkillInvoked(cardUse.from, "huwei");
    cardUse.from->drawCards(2);
    room->setPlayerFlag(cardUse.from, "Global_huweiFailed");
    return NULL;
}

class HuweiVS : public ZeroCardViewAsSkill
{
public:
    HuweiVS()
        : ZeroCardViewAsSkill("huwei")
    {
    }

    virtual const Card *viewAs() const
    {
        return new HuweiCard;
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
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
            if (player != NULL && player->isAlive() && player->hasSkill(this) && player->getPhase() != Player::Play
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                foreach (int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->isKindOf("Slash"))
                        d << SkillInvokeDetail(this, player, player);
                }
            }
        }

        return d;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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

    bool effect(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
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

    virtual const Card *viewAs() const
    {
        return new JinxiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@jinxi") >= 1 && player->isWounded();
    }
};

class Yemang : public ProhibitSkill
{
public:
    Yemang()
        : ProhibitSkill("yemang")
    {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &, bool include_hidden) const
    {
        if (card->isKindOf("Slash") && from && from->distanceTo(to) > 1) {
            foreach (const Player *p, from->getAliveSiblings()) {
                if (p->hasSkill(objectName(), false, include_hidden) && p->inMyAttackRange(from))
                    return true;
            }
        }
        return false;
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

    QStringList option;
    option << "mingmu_give"
           << "mingmu_disable";
    while (!option.isEmpty() && mystia->isAlive()) {
        QString choice = room->askForChoice(mystia, "mingmu", option.join("+"));
        if (choice == "mingmu_give") {
            const Card *card = room->askForCard(mystia, "Slash", "@mingmu:" + source->objectName(), QVariant(), Card::MethodNone, NULL, false, objectName());
            if (card) {
                CardMoveReason reason1(CardMoveReason::S_REASON_GIVE, mystia->objectName(), source->objectName(), "mingmu", QString());
                room->obtainCard(source, card, reason1);
                option << "cancel";
            }
        } else if (choice == "mingmu_disable") {
            option << "cancel";
            room->setPlayerFlag(mystia, "mingmuInvalid");
            room->touhouLogmessage("#mingmuInvalid", mystia, "yemang");
            const Skill *yemang = Sanguosha->getSkill("yemang");
            room->setPlayerSkillInvalidity(mystia, yemang, true);
        } else if (choice == "cancel")
            break;

        option.removeOne(choice);
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

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasSkill("mingmu") && !p->hasFlag("mingmuInvoked"))
                return true;
        }
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *) const
    {
        return selected.length() < 2;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0 && cards.length() <= 2) {
            MingmuCard *card = new MingmuCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }
};

class Mingmu : public TriggerSkill
{
public:
    Mingmu()
        : TriggerSkill("mingmu")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging << Death << Debut << Revive;
        show_type = "static";
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
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
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("mingmuInvoked"))
                        room->setPlayerFlag(p, "-mingmuInvoked");
                    if (p->hasFlag("mingmuInvalid")) {
                        const Skill *yemang = Sanguosha->getSkill("yemang");
                        room->setPlayerSkillInvalidity(p, yemang, false);
                    }
                }
            }
        }
    }
};

TH08Package::TH08Package()
    : Package("th08")
{
    General *kaguya = new General(this, "kaguya$", "yyc", 4, false);
    kaguya->addSkill(new Yongheng);
    kaguya->addSkill(new Zhuqu);

    General *eirin = new General(this, "eirin", "yyc", 4, false);
    eirin->addSkill(new Ruizhi);
    eirin->addSkill(new Miyao);

    General *mokou = new General(this, "mokou", "yyc", 4, false);
    mokou->addSkill(new Kaifeng);
    mokou->addSkill(new Fengxiang);

    General *reisen = new General(this, "reisen", "yyc", 4, false);
    reisen->addSkill(new Kuangzao);
    reisen->addSkill(new Huanshi);

    General *keine = new General(this, "keine", "yyc", 3, false);
    keine->addSkill(new Xushi);
    keine->addSkill(new Xinyue);

    General *tewi = new General(this, "tewi", "yyc", 3, false);
    tewi->addSkill(new Buxian);
    tewi->addSkill(new BuxianEffect);
    tewi->addSkill(new Xingyun);
    related_skills.insertMulti("buxian", "#buxian");

    General *mystia = new General(this, "mystia", "yyc", 3, false);
    mystia->addSkill(new Yege);
    mystia->addSkill(new Laolong);
    //mystia->addSkill(new Gesheng);
    //mystia->addSkill(new Yemang);

    General *wriggle = new General(this, "wriggle", "yyc", 3, false);
    wriggle->addSkill(new Yinghuo);
    wriggle->addSkill(new Chongqun);

    General *shirasawa = new General(this, "shirasawa", "yyc", 3, false);
    shirasawa->addSkill(new Chuangshi);
    shirasawa->addSkill(new Wangyue);

    General *mokou_sp = new General(this, "mokou_sp", "yyc", 4, false);
    mokou_sp->addSkill(new Huwei);
    mokou_sp->addSkill(new Jinxi);

    General *mystia_sp = new General(this, "mystia_sp", "yyc", 3, false);
    mystia_sp->addSkill(new Yemang);
    mystia_sp->addSkill(new Mingmu);

    addMetaObject<MiyaoCard>();
    addMetaObject<KuangzaoCard>();
    addMetaObject<BuxianCard>();
    addMetaObject<XingyunCard>();
    addMetaObject<YegeCard>();
    addMetaObject<GeshengCard>();
    addMetaObject<YinghuoCard>();
    addMetaObject<ChuangshiCard>();
    addMetaObject<HuweiCard>();
    addMetaObject<JinxiCard>();
    addMetaObject<MingmuCard>();

    skills << new MingmuVS;
}

ADD_PACKAGE(TH08)
