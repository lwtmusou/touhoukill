#include "th01-05.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"

#include "clientplayer.h"

class Meiling : public TriggerSkill
{
public:
    Meiling()
        : TriggerSkill("meiling")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.card || damage.from == NULL || damage.to->isDead())
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
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *mima, room->findPlayersBySkillName(objectName())) {
            if (mima != damage.from)
                d << SkillInvokeDetail(this, mima, mima);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt = "distance:" + damage.to->objectName() + ":" + QString::number(invoke->invoker->distanceTo(damage.to)) + ":" + damage.card->objectName();
        invoke->invoker->tag["meiling"] = data;
        return invoke->invoker->askForSkillInvoke(this, prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        invoke->invoker->obtainCard(damage.card);
        if (damage.to->isAlive() && damage.from->isAlive() && invoke->invoker->distanceTo(damage.to) > invoke->invoker->getLostHp()) {
            room->damage(DamageStruct(objectName(), damage.from, invoke->invoker, 1, DamageStruct::Normal));
        }
        return false;
    }
};

class Fuchou : public TriggerSkill
{
public:
    Fuchou()
        : TriggerSkill("fuchou$")
    {
        frequency = Compulsory;
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash") || !use.from->isAlive() || !use.from->hasLordSkill(this))
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        bool can = false;
        foreach (ServerPlayer *t, use.to) {
            foreach (ServerPlayer *p, room->getLieges("pc98", use.from)) {
                if (p->inMyAttackRange(t) && t->getHp() > p->getHp() && p != use.from) {
                    can = true;
                    break;
                }
            }
        }
        if (can)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->touhouLogmessage("#fuchou", invoke->invoker, objectName());
        if (use.m_addHistory) {
            room->addPlayerHistory(use.from, use.card->getClassName(), -1);
            use.m_addHistory = false;
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

class Ciyuan : public TriggerSkill
{
public:
    Ciyuan()
        : TriggerSkill("ciyuan")
    {
        events << EventPhaseStart;
    }

    static Player::Phase ciyuanPhase(QString choice)
    {
        Player::Phase phase = Player::Finish;
        if (choice == "start")
            phase = Player::Start;
        else if (choice == "judge")
            phase = Player::Judge;
        else if (choice == "draw")
            phase = Player::Draw;
        else if (choice == "play")
            phase = Player::Play;
        else if (choice == "discard")
            phase = Player::Discard;
        return phase;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->getPhase() == Player::RoundStart)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QStringList phases;
        phases << "start"
               << "judge"
               << "draw"
               << "play"
               << "discard"
               << "finish"
               << "cancel";
        QString choice = room->askForChoice(invoke->invoker, objectName(), phases.join("+"));
        if (choice != "cancel") {
            phases.removeOne("cancel");
            phases.removeOne(choice);
            QString choice1 = room->askForChoice(invoke->invoker, objectName(), phases.join("+"));
            invoke->invoker->tag["ciyuan"] = QVariant::fromValue(choice);
            invoke->invoker->tag["ciyuan1"] = QVariant::fromValue(choice1);
        }

        return choice != "cancel";
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString choice = invoke->invoker->tag["ciyuan"].toString();
        Player::Phase phase = ciyuanPhase(choice);
        QString choice1 = invoke->invoker->tag["ciyuan1"].toString();
        Player::Phase phase1 = ciyuanPhase(choice1);

        room->notifySkillInvoked(invoke->owner, objectName());
        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = invoke->invoker;
        log.arg = objectName();
        room->sendLog(log);

        log.type = "#ciyuan";
        log.from = invoke->invoker;
        log.arg = "ciyuan:" + choice;
        log.arg2 = "ciyuan:" + choice1;
        room->sendLog(log);

        invoke->invoker->exchangePhases(phase, phase1);
        return false;
    }
};

class Shigui : public TriggerSkill
{
public:
    Shigui()
        : TriggerSkill("shigui")
    {
        events << EventPhaseStart << EventPhaseEnd << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            Player::Phase phase = current->getPhase();
            QList<Player::Phase> phases;
            phases << Player::Start << Player::Judge << Player::Draw << Player::Play << Player::Discard << Player::Finish;
            if (phases.contains(phase))
                room->setPlayerMark(current, "shigui", current->getMark("shigui") + 1);
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                room->setPlayerMark(change.player, "shigui", 0);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current->hasSkill(this) && !current->hasFlag("shigui_used") && (current->getPhase() == Player::Draw || current->getPhase() == Player::Play)
                && current->getHandcardNum() != current->getMark("shigui"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString skillname = invoke->invoker->getHandcardNum() < invoke->invoker->getMark("shigui") ? "draw" : "play";
        QString prompt = skillname + "_notice:" + QString::number(invoke->invoker->getMark("shigui"));
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *current = invoke->invoker;
        room->setPlayerFlag(current, "shigui_used");
        bool recover = current->getHandcardNum() > current->getMark("shigui");
        int num = current->getHandcardNum() - current->getMark("shigui");

        if (recover) {
            room->askForDiscard(invoke->invoker, objectName(), qAbs(num), qAbs(num), false, false, "shigui_discard:" + QString::number(qAbs(num)));
            RecoverStruct recover;
            room->recover(invoke->invoker, recover);
        } else {
            current->drawCards(qAbs(num));
            room->loseHp(current);
        }

        return false;
    }
};

class Chongdong : public TriggerSkill
{
public:
    Chongdong()
        : TriggerSkill("chongdong$")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *current = room->getCurrent();
        if (damage.to->isAlive() && damage.to->hasLordSkill(this) && current && current->isAlive()) {
            foreach (ServerPlayer *liege, room->getLieges("pc98", damage.to)) {
                if (liege->canDiscard(liege, "hs"))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        return invoke->invoker->askForSkillInvoke(this, data);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *current = room->getCurrent();
        foreach (ServerPlayer *liege, room->getLieges("pc98", invoke->invoker)) {
            if (liege->canDiscard(liege, "hs")) {
                const Card *card = room->askForCard(liege, ".|red|.|hand", "@chongdong", data, Card::MethodDiscard);
                if (card != NULL) {
                    room->touhouLogmessage("#chongdong", current, objectName());
                    room->setPlayerFlag(current, "Global_TurnTerminated");
                    break;
                }
            }
        }
        return false;
    }
};

class ZhenceVS : public ZeroCardViewAsSkill
{
public:
    ZhenceVS()
        : ZeroCardViewAsSkill("zhence")
    {
        response_pattern = "@@zhence";
    }

    virtual const Card *viewAs() const
    {
        FireAttack *fire = new FireAttack(Card::NoSuit, 0);
        fire->setSkillName("zhence");
        return fire;
    }
};

class Zhence : public TriggerSkill
{
public:
    Zhence()
        : TriggerSkill("zhence")
    {
        events << EventPhaseChanging << DamageCaused;
        view_as_skill = new ZhenceVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->hasSkill(this) && (change.to == Player::Draw || change.to == Player::Play) && !change.player->isSkipped(change.to)) {
                FireAttack *fire = new FireAttack(Card::NoSuit, 0);
                fire->deleteLater();
                if (!change.player->isCardLimited(fire, Card::MethodUse))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player);
            }
        } else if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive() && damage.card && damage.card->getSkillName() == "zhence")
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            invoke->invoker->tag["zhence"] = data;
            QString prompt;
            if (change.to == Player::Draw)
                prompt = "@zhence:draw";
            if (change.to == Player::Play)
                prompt = "@zhence:play";
            room->askForUseCard(invoke->invoker, "@@zhence", prompt);
            return false;
        }
        return true;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1);
        PhaseChangeStruct change = invoke->invoker->tag["zhence"].value<PhaseChangeStruct>();
        invoke->invoker->skip(change.to);
        return false;
    }
};

ShiquCard::ShiquCard()
{
    will_throw = true;
}

bool ShiquCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && (to_select == Self || to_select->isCurrent());
}

void ShiquCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    QString choice = effect.from->tag["shiqu"].toString();
    Player::Phase phase;
    if (choice == "shiqu_start")
        phase = Player::Start;
    else if (choice == "shiqu_judge")
        phase = Player::Judge;
    else if (choice == "shiqu_draw")
        phase = Player::Draw;
    else if (choice == "shiqu_play")
        phase = Player::Play;
    else if (choice == "shiqu_discard")
        phase = Player::Discard;

    LogMessage log;
    log.type = "#shiqu";
    log.from = effect.from;
    log.to << effect.to;
    log.arg = "shiqu";
    log.arg2 = choice;
    room->sendLog(log);

    ExtraTurnStruct extra;
    extra.player = effect.to;
    extra.set_phases << Player::RoundStart << phase << Player::NotActive;
    effect.to->tag["ExtraTurnInfo"] = QVariant::fromValue(extra);
    effect.to->gainAnExtraTurn();
}

class ShiquVS : public OneCardViewAsSkill
{
public:
    ShiquVS()
        : OneCardViewAsSkill("shiqu")
    {
        response_pattern = "@@shiqu";
        filter_pattern = ".|.|.|hand,equipped!";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            ShiquCard *card = new ShiquCard;
            card->addSubcard(originalCard);
            return card;
        }
        return NULL;
    }
};

class Shiqu : public TriggerSkill
{
public:
    Shiqu()
        : TriggerSkill("shiqu")
    {
        events << EventPhaseStart << EventPhaseChanging;
        view_as_skill = new ShiquVS;
    }

    static QStringList shiquChoices(ServerPlayer *player)
    {
        QStringList marks;
        QStringList choices;
        marks << "shiqu_start"
              << "shiqu_judge"
              << "shiqu_draw"
              << "shiqu_play"
              << "shiqu_discard";
        foreach (QString m, marks) {
            if (player->getMark(m) == 0)
                choices << m;
        }
        return choices;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            switch (current->getPhase()) {
            case Player::Start: {
                room->setPlayerMark(current, "shiqu_start", 1);
                break;
            }
            case Player::Judge: {
                room->setPlayerMark(current, "shiqu_judge", 1);
                break;
            }
            case Player::Draw: {
                room->setPlayerMark(current, "shiqu_draw", 1);
                break;
            }
            case Player::Play: {
                room->setPlayerMark(current, "shiqu_play", 1);
                break;
            }
            case Player::Discard: {
                room->setPlayerMark(current, "shiqu_discard", 1);
                break;
            }
            default:
                break;
            }
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                room->setPlayerMark(change.player, "shiqu_start", 0);
                room->setPlayerMark(change.player, "shiqu_judge", 0);
                room->setPlayerMark(change.player, "shiqu_draw", 0);
                room->setPlayerMark(change.player, "shiqu_play", 0);
                room->setPlayerMark(change.player, "shiqu_discard", 0);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current->getPhase() == Player::Finish && !current->tag.value("touhou-extra", false).toBool() && !room->getThread()->hasExtraTurn()) {
                QStringList choices = shiquChoices(current);
                if (choices.length() > 0) {
                    foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                        if (p->canDiscard(p, "hes"))
                            d << SkillInvokeDetail(this, p, p);
                    }
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        QStringList choices = shiquChoices(current);
        choices << "cancel";
        QString choice = room->askForChoice(invoke->invoker, objectName(), choices.join("+"));

        if (choice != "cancel") {
            QString prompt = "@shiqu-discard:" + choice + ":" + current->objectName();
            invoke->invoker->tag["shiqu"] = QVariant::fromValue(choice);
            return room->askForUseCard(invoke->invoker, "@@shiqu", prompt);
        }
        return false;
    }
};

class Zhenli : public TriggerSkill
{
public:
    Zhenli()
        : TriggerSkill("zhenli")
    {
        frequency = Compulsory;
        events << CardsMoveOneTime << AfterDrawInitialCards;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == AfterDrawInitialCards) {
            DrawNCardsStruct dc = data.value<DrawNCardsStruct>();
            if (dc.player->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dc.player, dc.player, NULL, true);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.reason.m_reason == CardMoveReason::S_REASON_DRAW && move.reason.m_extraData.toString() == "drawFromDrawpile") {
                ServerPlayer *rikako = qobject_cast<ServerPlayer *>(move.to);
                if (rikako != NULL && rikako->hasSkill(this)) {
                    foreach (int id, move.card_ids) {
                        if (room->getCardPlace(id) == Player::PlaceHand && room->getCardOwner(id) == rikako)
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, rikako, rikako, NULL, true);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    //default cost

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        if (triggerEvent == AfterDrawInitialCards)
            invoke->invoker->drawCards(24);
        else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QList<int> ids;
            foreach (int id, move.card_ids) {
                if (room->getCardPlace(id) == Player::PlaceHand && room->getCardOwner(id) == invoke->invoker)
                    ids << id;
            }
            invoke->invoker->addToPile("zhenli", ids, false);
        }
        return false;
    }
};

class QiusuoVS : public ViewAsSkill
{
public:
    QiusuoVS()
        : ViewAsSkill("qiusuo")
    {
        response_pattern = "@@qiusuo";
        expand_pile = "zhenli";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Self->getPile("zhenli").contains(to_select->getEffectiveId())) {
            if (selected.isEmpty())
                return true;
            else
                return to_select->getNumber() == selected.first()->getNumber();
        }
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0) {
            DummyCard *dc = new DummyCard;
            dc->addSubcards(cards);
            return dc;
        }
        return NULL;
    }
};

class Qiusuo : public TriggerSkill
{
public:
    Qiusuo()
        : TriggerSkill("qiusuo")
    {
        events << EventPhaseStart;
        view_as_skill = new QiusuoVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->getPhase() == Player::Play && current->hasSkill(this) && !current->getPile("zhenli").isEmpty())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        const Card *c = room->askForCard(invoke->invoker, "@@qiusuo", "@qiusuo", data, Card::MethodNone, NULL, false, objectName());
        if (c) {
            QVariantList ids;
            foreach (int card_id, c->getSubcards())
                ids << card_id;
            invoke->invoker->tag["qiusuo"] = ids;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QVariantList ids = invoke->invoker->tag["qiusuo"].toList();
        invoke->invoker->tag.remove("qiusuo");
        room->touhouLogmessage("#InvokeSkill", invoke->invoker, objectName());
        QList<int> get_ids;
        foreach (QVariant card_data, ids) {
            int id = card_data.toInt();
            room->showCard(invoke->invoker, id);
            get_ids << id;
        }
        DummyCard dummy(get_ids);
        invoke->invoker->obtainCard(&dummy);
        return false;
    }
};

class Mengxiao : public TriggerSkill
{
public:
    Mengxiao()
        : TriggerSkill("mengxiao")
    {
        events << EventPhaseStart;
    }

    static QStringList mengxiaoChoices(ServerPlayer *kana, ServerPlayer *target = NULL)
    {
        QStringList trick_list;
        if (target) {
            foreach (const Card *trick, kana->getJudgingArea()) {
                if (!target->containsTrick(trick->objectName()) && !kana->isProhibited(target, trick))
                    trick_list << trick->objectName();
            }
        } else {
            QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
            foreach (const Card *card, cards) {
                if (card->getTypeId() == Card::TypeTrick && !card->isNDTrick() && card->getSubtype() == "unmovable_delayed_trick" && !trick_list.contains(card->objectName())
                    && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
                    if (!kana->containsTrick(card->objectName()))
                        trick_list << card->objectName();
                }
            }
        }
        return trick_list;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->getPhase() == Player::Finish && current->hasSkill(this) && current->isAlive() && !current->isKongcheng()) {
            QStringList trick_list = mengxiaoChoices(current);
            if (trick_list.isEmpty())
                return QList<SkillInvokeDetail>();
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current);
        } else if (current->getPhase() == Player::Start) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p != current && !mengxiaoChoices(p, current).isEmpty())
                    d << SkillInvokeDetail(this, p, p, NULL, false, current);
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->getPhase() == Player::Finish) {
            QStringList trick_list = mengxiaoChoices(invoke->invoker);
            trick_list << "cancel";
            QString choice = room->askForChoice(invoke->invoker, objectName(), trick_list.join("+"), data);
            if (choice == "cancel")
                return false;

            const Card *card = room->askForCard(invoke->invoker, ".", "@mengxiao:" + choice, data, Card::MethodNone, NULL, false, objectName());
            if (!card)
                return false;

            room->notifySkillInvoked(invoke->invoker, objectName());

            LogMessage log;
            log.type = "$Mengxiao";
            log.from = invoke->invoker;
            log.arg = objectName();
            log.arg2 = choice;
            log.card_str = QString::number(card->getEffectiveId());
            room->sendLog(log);

            Card *delayTrick = Sanguosha->cloneCard(choice);
            WrappedCard *vs_card = Sanguosha->getWrappedCard(card->getEffectiveId());
            vs_card->setSkillName(objectName());
            vs_card->takeOver(delayTrick);
            room->broadcastUpdateCard(room->getAlivePlayers(), vs_card->getId(), vs_card);

            CardsMoveStruct move;
            move.card_ids << vs_card->getId();
            move.to = invoke->invoker;
            move.to_place = Player::PlaceDelayedTrick;
            room->moveCardsAtomic(move, true);

        } else {
#pragma message WARN("todo_lwtmusou:two steps -> one step   using askForUseCard")
            //step 1: invoke
            if (!invoke->invoker->askForSkillInvoke(this, data))
                return false;

            //step 2:choose id
            QStringList trick_list = mengxiaoChoices(invoke->invoker, current);
            QList<int> disable;
            foreach (const Card *trick, invoke->invoker->getJudgingArea()) {
                if (!trick_list.contains(trick->objectName()))
                    disable << trick->getEffectiveId();
            }
            int card_id = room->askForCardChosen(invoke->invoker, invoke->invoker, "j", objectName(), false, Card::MethodDiscard, disable);
            const Card *card = Sanguosha->getCard(card_id);
            room->moveCardTo(card, invoke->invoker, current, Player::PlaceDelayedTrick,
                             CardMoveReason(CardMoveReason::S_REASON_TRANSFER, invoke->invoker->objectName(), objectName(), QString()));
        }

        return false;
    }
};

class Lubiao : public TriggerSkill
{
public:
    Lubiao()
        : TriggerSkill("lubiao")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        bool can = false;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getCards("j").length() > 0) {
                can = true;
                break;
            }
        }
        DamageStruct damage = data.value<DamageStruct>();
        if (can && damage.to->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        damage.damage = damage.damage - 1;

        room->touhouLogmessage("#lubiao", invoke->invoker, objectName(), QList<ServerPlayer *>(), QString::number(1));
        room->notifySkillInvoked(invoke->invoker, objectName());

        if (damage.damage == 0)
            return true;
        data = QVariant::fromValue(damage);

        return false;
    }
};

class Yeyan : public TriggerSkill
{
public:
    Yeyan()
        : TriggerSkill("yeyan")
    {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from && use.from->hasSkill(this) && use.from->isAlive() && (use.card->isNDTrick() || use.card->isKindOf("Slash"))) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *to, use.to) {
                if (to->isAlive() && to->canDiscard(to, "hs") && use.from != to)
                    targets << to;
            }
            if (!targets.isEmpty())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, targets, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();

        QList<ServerPlayer *> logto;
        logto << invoke->targets;
        room->touhouLogmessage("#yeyan", invoke->invoker, objectName(), logto, use.card->objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        foreach (ServerPlayer *to, invoke->targets) {
            if (to->isKongcheng())
                continue;
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), to->objectName());

            QList<const Card *> hc = to->getHandcards();
            foreach (const Card *c, hc) {
                if (to->isJilei(c))
                    hc.removeOne(c);
            }
            if (hc.length() == 0) {
                // jilei show all cards
                room->doJileiShow(to, to->handCards());
                continue;
            }

            QStringList prompt_list;
            prompt_list << "yeyan-discard" << use.card->objectName() << invoke->invoker->objectName();
            QString prompt = prompt_list.join(":");
            const Card *card = room->askForCard(to, ".!", prompt, data, Card::MethodDiscard);
            if (!card) {
                // force discard!!!
                int x = qrand() % hc.length();
                const Card *c = hc.value(x);
                card = c;
                room->throwCard(c, to);
            }

            if (invoke->invoker->isDead())
                continue;
            if (invoke->invoker->isKongcheng()) {
                use.nullified_list << to->objectName();
                continue;
            }

            //show card with the same type
            QString type = card->getType();
            QString pattern = QString("%1Card|.|.|hand").arg(type.left(1).toUpper() + type.right(type.length() - 1));
            QStringList prompt_list1;
            prompt_list1 << "yeyan-show" << use.card->objectName() << to->objectName() << card->getType();
            QString prompt1 = prompt_list1.join(":");
            invoke->invoker->tag["yeyan_target"] = QVariant::fromValue(to);
            invoke->invoker->tag["yeyan_card"] = QVariant::fromValue(card);

            const Card *card1 = room->askForCard(invoke->invoker, pattern, prompt1, data, Card::MethodNone);
            if (card1)
                room->showCard(invoke->invoker, card1->getEffectiveId());
            else
                use.nullified_list << to->objectName();
        }
        data = QVariant::fromValue(use);

        return false;
    }
};

class Youyue : public TriggerSkill
{
public:
    Youyue()
        : TriggerSkill("youyue")
    {
        events << EventPhaseEnd;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *yuka = data.value<ServerPlayer *>();
        if (!yuka->hasSkill(this) || yuka->isDead() || yuka->getPhase() != Player::Play)
            return QList<SkillInvokeDetail>();

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->isKongcheng())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, yuka, yuka);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        int n = 0;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->isKongcheng())
                n++;
        }
        invoke->invoker->drawCards(n);
        return false;
    }
};

class Menghuan : public MaxCardsSkill
{
public:
    Menghuan()
        : MaxCardsSkill("menghuan$")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        int correct = 0;
        if (target->hasLordSkill("menghuan")) {
            foreach (const Player *p, target->getAliveSiblings()) {
                if (p->getKingdom() == "pc98" && p->getHp() > correct)
                    correct = p->getHp();
            }
        }
        return correct;
    }
};

class HuantongVS : public ViewAsSkill
{
public:
    HuantongVS()
        : ViewAsSkill("huantong")
    {
        response_pattern = "@@huantong";
        expand_pile = "dream";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return Self->getPile("dream").contains(to_select->getEffectiveId()) && selected.length() < 2;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            DummyCard *dc = new DummyCard;
            dc->addSubcards(cards);
            return dc;
        }
        return NULL;
    }
};

class Huantong : public TriggerSkill
{
public:
    Huantong()
        : TriggerSkill("huantong")
    {
        events << DamageInflicted;
        view_as_skill = new HuantongVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.damage != 1)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p->inMyAttackRange(damage.to))
                d << SkillInvokeDetail(this, p, p, NULL, false, damage.to);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget))) {
            if (invoke->invoker->getPile("dream").length() >= 2) {
                const Card *c
                    = room->askForCard(invoke->invoker, "@@huantong", "@huantong:" + invoke->preferredTarget->objectName(), data, Card::MethodNone, NULL, false, objectName());
                if (c) {
                    QVariantList ids;
                    foreach (int card_id, c->getSubcards())
                        ids << card_id;
                    invoke->invoker->tag["huantong"] = ids;
                }
            }
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QVariantList ids = invoke->invoker->tag["huantong"].toList();
        invoke->invoker->tag.remove("huantong");
        if (ids.isEmpty()) {
            CardMoveReason reason(CardMoveReason::S_REASON_UNKNOWN, "", NULL, "dream", "");
            invoke->invoker->addToPile("dream", room->getNCards(1), false, reason);
        } else {
            DamageStruct damage = data.value<DamageStruct>();

            QList<int> get_ids;
            QList<int> throw_ids;
            foreach (QVariant card_data, ids) {
                int id = card_data.toInt();
                room->showCard(invoke->invoker, id);
                if (Sanguosha->getCard(id)->isKindOf("BasicCard"))
                    get_ids << id;
                else
                    throw_ids << id;
            }
            if (!get_ids.isEmpty()) {
                DummyCard dummy(get_ids);
                damage.to->obtainCard(&dummy);
            }

            if (!throw_ids.isEmpty()) {
                DummyCard dummy(throw_ids);
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
                room->throwCard(&dummy, reason, NULL);
            }

            LogMessage log;
            log.type = "#HuantongDamage";
            log.from = damage.to;
            log.arg = objectName();
            log.arg2 = QString::number(get_ids.length());
            room->sendLog(log);

            if (get_ids.isEmpty())
                return true;
            damage.damage = get_ids.length();
            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class Mengyan : public TriggerSkill
{
public:
    Mengyan()
        : TriggerSkill("mengyan")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;
        if (damage.damage > 1) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->isWounded())
                    d << SkillInvokeDetail(this, p, p, NULL);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        RecoverStruct recover;
        room->recover(invoke->invoker, recover);

        CardsMoveStruct move;
        move.card_ids = invoke->invoker->getPile("dream");
        move.to_place = Player::PlaceHand;
        move.to = invoke->invoker;
        room->moveCardsAtomic(move, false);

        return false;
    }
};


class Xuxiang : public TriggerSkill
{
public:
    Xuxiang()
        : TriggerSkill("xuxiang")
    {
        events << DamageCaused;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;
        if (damage.card && damage.card->isKindOf("Slash") 
            && damage.by_user && damage.from && damage.from->isAlive() 
            && !damage.from->isKongcheng()) {
            foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (!p->hasFlag("xuxiang_used") && p != damage.from && !p->isKongcheng())
                    d << SkillInvokeDetail(this, p, p, NULL, false, damage.from);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt = "target:" + damage.from->objectName() + ":" + damage.to->objectName() 
            + ":" + damage.card->objectName() + ":" + QString::number(damage.damage);
        return room->askForSkillInvoke(invoke->invoker, objectName(), prompt);

    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        DamageStruct damage = data.value<DamageStruct>();
        invoke->invoker->tag["xuxiang"] = QVariant::fromValue(damage);
        room->setPlayerFlag(invoke->invoker, "xuxiang_used");
        if (invoke->invoker->pindian(invoke->targets.first(), objectName())) {
            DamageStruct new_damage = invoke->invoker->tag.value("xuxiang").value<DamageStruct>();
            if (new_damage.damage == 0)
                return true;
            data = QVariant::fromValue(new_damage);
        }
        return false;
    }
};


class XuxiangRecord : public TriggerSkill
{
public:
    XuxiangRecord()
        : TriggerSkill("#xuxiang")
    {
        events << Pindian << EventPhaseChanging;
    }
    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == Pindian) {
            PindianStruct *pindian = data.value<PindianStruct *>();
            if (pindian->reason == "xuxiang" && pindian->from_number > pindian->to_number) {
                DamageStruct damage = pindian->from->tag.value("xuxiang").value<DamageStruct>();
                int d = 0;
                if (pindian->from_card->isBlack())
                    d++;
                if (pindian->to_card->isBlack())
                    d++;
                damage.damage = d;
                pindian->from->tag["xuxiang"] = QVariant::fromValue(damage);
            }
        }
        else if (triggerEvent == EventPhaseChanging) {
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("xuxiang_used"))
                    room->setPlayerFlag(p, "-xuxiang_used");
            }
        }
    }
};

class Huanjue : public TriggerSkill
{
public:
    Huanjue()
        : TriggerSkill("huanjue")
    {
        events << PindianAsked;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->askedPlayer->hasSkill(this)) {
            ServerPlayer *target = (pindian->askedPlayer == pindian->from) ? pindian->to : pindian->from;
            if (pindian->askedPlayer->inMyAttackRange(target) && pindian->from_card == NULL)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, pindian->askedPlayer, pindian->askedPlayer, NULL, false, target);
        }
        if (pindian->askedPlayer->hasFlag(objectName())) {
            ServerPlayer *owner = (pindian->askedPlayer == pindian->from) ? pindian->to : pindian->from;
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, owner, pindian->askedPlayer, NULL, false, owner);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QVariant notify_data = QVariant::fromValue(invoke->preferredTarget);
        bool can =  invoke->invoker->askForSkillInvoke(this, notify_data);
        if (!can && invoke->invoker->hasFlag(objectName()))
            room->setPlayerFlag(invoke->invoker, "-huanjue");
        return can;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        int id = -1;
        if (invoke->invoker->hasFlag(objectName())) {
            room->setPlayerFlag(invoke->invoker, "-huanjue");
            id = room->drawCard();
        }
        else {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());

            QList<int> list = room->getNCards(2);
            room->returnToTopDrawPile(list);

            room->fillAG(list, invoke->invoker);
            id = room->askForAG(invoke->invoker, list, false, objectName());
            room->clearAG(invoke->invoker);
            if (pindian->from_card == NULL && pindian->to_card == NULL)
                room->setPlayerFlag(invoke->targets.first(), objectName());
        }
        if (id > -1) {
            Card *c = Sanguosha->getCard(id);
            if (invoke->invoker == pindian->from)
                pindian->from_card = c;
            else
                pindian->to_card = c;
            room->showCard(invoke->invoker, id);
            data = QVariant::fromValue(pindian);
            CardMoveReason reason1(CardMoveReason::S_REASON_PINDIAN, invoke->invoker->objectName(),invoke->targets.first()->objectName(), pindian->reason, QString());
            //room->moveCardTo(c, invoke->invoker, NULL, Player::PlaceTable, reason1, false);
            //room->moveCardTo(c, invoke->invoker, NULL, Player::PlaceWuGu, reason1, false);
        }
        return false;
    }
};


LianmuCard::LianmuCard()
{
    will_throw = false;
}

bool LianmuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *card = new Slash(Card::NoSuit, 0);
    card->deleteLater();
    card->setSkillName("lianmu");
    return card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool LianmuCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Slash *card = new Slash(Card::NoSuit, 0);
    card->deleteLater();
    card->setSkillName("lianmu");
    return card->targetsFeasible(targets, Self);
}

const Card *LianmuCard::validate(CardUseStruct &use) const
{
    Slash *card = new Slash(Card::NoSuit, 0);
    card->setSkillName("lianmu");
    use.from->getRoom()->setPlayerFlag(use.from, "lianmu_used");
    return card;
}

class LianmuVS : public ZeroCardViewAsSkill
{
public:
    LianmuVS()
        : ZeroCardViewAsSkill("lianmu")
    {
        response_pattern = "@@lianmu";
    }

    virtual const Card *viewAs() const
    {
        LianmuCard *slash = new LianmuCard;
        return slash;
    }
};

class Lianmu : public TriggerSkill
{
public:
    Lianmu()
        : TriggerSkill("lianmu")
    {
        events << DamageDone << CardFinished << EventPhaseChanging;
        view_as_skill = new LianmuVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash"))
                room->setCardFlag(damage.card, "lianmu_damage");
        }

        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->setPlayerFlag(p, "-lianmu_used");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.from->hasSkill(this) && !use.from->hasFlag("lianmu_used") && use.card->isKindOf("Slash") && !use.card->hasFlag("lianmu_damage"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->askForUseCard(invoke->invoker, "@@lianmu", "@lianmu");
        return false;
    }
};

class LianmuTargetMod : public TargetModSkill
{
public:
    LianmuTargetMod()
        : TargetModSkill("#lianmu_mod")
    {
        frequency = NotFrequent;
        pattern = "Slash";
    }

    virtual int getExtraTargetNum(const Player *, const Card *card) const
    {
        if (card->getSkillName() == "lianmu")
            return 1;
        else
            return 0;
    }

    int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->getSkillName() == "lianmu")
            return 1000;

        return 0;
    }
};

class HuanweiEffect : public TriggerSkill
{
public:
    HuanweiEffect()
        : TriggerSkill("#huanwei")
    {
        events << EventAcquireSkill << EventLoseSkill << GameStart << EventSkillInvalidityChange << EventPhaseStart << DamageCaused;

        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == GameStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player && player->hasSkill("huanwei"))
                room->filterCards(player, player->getCards("hes"), true);
        }
        if (triggerEvent == EventLoseSkill || triggerEvent == EventAcquireSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            if (a.skill->objectName() == "huanwei")
                room->filterCards(a.player, a.player->getCards("hes"), true);
        }

        if (triggerEvent == EventSkillInvalidityChange) {
            QList<SkillInvalidStruct> invalids = data.value<QList<SkillInvalidStruct> >();
            foreach (SkillInvalidStruct v, invalids) {
                if (!v.skill || v.skill->objectName() == "huanwei") {
                    room->filterCards(v.player, v.player->getCards("hes"), true);
                    //if (!v.invalid)
                }
            }
        }
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && (player->getPhase() == Player::RoundStart || player->getPhase() == Player::NotActive))
                room->filterCards(player, player->getCards("hes"), true);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.chain || damage.transfer || !damage.by_user || !damage.from || !damage.from->hasSkill("huanwei") || !damage.from->isCurrent())
                return QList<SkillInvokeDetail>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->getSuit() == Card::Spade)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        damage.damage = damage.damage - 1;
        QList<ServerPlayer *> logto;
        logto << damage.to;
        room->touhouLogmessage("#HuanweiTrigger", invoke->invoker, "huanwei", logto, QString::number(1));
        room->notifySkillInvoked(invoke->invoker, "huanwei");
        data = QVariant::fromValue(damage);
        if (damage.damage == 0)
            return true;

        return false;
    }
};

class Huanwei : public FilterSkill
{
public:
    Huanwei()
        : FilterSkill("huanwei")
    {
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        Room *room = Sanguosha->currentRoom();
        ServerPlayer *player = room->getCardOwner(to_select->getId());
        return player != NULL && !player->isCurrent() && to_select->getSuit() == Card::Spade && to_select->isKindOf("Slash");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
        jink->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(jink);
        return card;
    }
};

SqChuangshiCard::SqChuangshiCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool SqChuangshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return !targets.contains(to_select);
}

void SqChuangshiCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *p, targets)
        room->setPlayerMark(p, "sqchuangshi", 1);
}

class SqChuangshiVS : public ZeroCardViewAsSkill
{
public:
    SqChuangshiVS()
        : ZeroCardViewAsSkill("sqchuangshi")
    {
        response_pattern = "@@sqchuangshi";
    }

    virtual const Card *viewAs() const
    {
        return new SqChuangshiCard;
    }
};

class SqChuangshi : public TriggerSkill
{
public:
    SqChuangshi()
        : TriggerSkill("sqchuangshi")
    {
        events << EventPhaseStart << EventPhaseChanging;
        view_as_skill = new SqChuangshiVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->setPlayerMark(p, "sqchuangshi", 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->isAlive() && player->getPhase() == Player::Play)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->askForUseCard(invoke->invoker, "@@sqchuangshi", "@sqchuangshi");
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("sqchuangshi") > 0)
                invoke->targets << p;
        }
        return !invoke->targets.isEmpty();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        foreach (ServerPlayer *p, invoke->targets)
            room->askForUseCard(p, "BasicCard+^Jink,TrickCard+^Nullification,EquipCard|.|.|sqchuangshi", "@sqchuangshi_use", -1, Card::MethodUse, false, objectName());
        return false;
    }
};

class Yuanfa : public TriggerSkill
{
public:
    Yuanfa()
        : TriggerSkill("yuanfa")
    {
        events << EventPhaseStart << EventPhaseChanging << CardUsed << CardResponded;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerMark(p, "yuanfa", 0);
            }
        } else if (e == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("SkillCard"))
                room->setPlayerMark(use.from, "yuanfa", 1);
        } else if (e == CardResponded) {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_isUse)
                room->setPlayerMark(response.m_from, "yuanfa", 1);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e != EventPhaseStart)
            return QList<SkillInvokeDetail>();
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (!player->hasSkill(this) || player->isDead() || player->getPhase() != Player::Finish)
            return QList<SkillInvokeDetail>();

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("yuanfa") > 0)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("yuanfa") > 0) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), p->objectName());
                p->drawCards(1);
            }
        }
        return false;
    }
};

class Shenwei : public TriggerSkill
{
public:
    Shenwei()
        : TriggerSkill("shenwei$")
    {
        frequency = Compulsory;
        events << CardResponded;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *current = room->getCurrent();
        if (!current || !current->hasLordSkill(objectName()) || !current->isAlive())
            return QList<SkillInvokeDetail>();

        CardResponseStruct resp = data.value<CardResponseStruct>();
        int count = 0;
        if (resp.m_isUse && resp.m_from != current && resp.m_card->isKindOf("Jink") && resp.m_card->getSuit() == Card::Diamond) {
            foreach (ServerPlayer *p, room->getOtherPlayers(resp.m_from)) {
                if (p->getKingdom() == "pc98" && p->inMyAttackRange(resp.m_from))
                    count++;
            }
        }
        if (count >= 2)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current, NULL, true, resp.m_from);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardResponseStruct resp = data.value<CardResponseStruct>();
        resp.m_isNullified = true;
        data = QVariant::fromValue(resp);

        room->notifySkillInvoked(invoke->invoker, objectName());
        room->touhouLogmessage("#shenwei", resp.m_from, objectName(), QList<ServerPlayer *>(), resp.m_card->objectName());
        return false;
    }
};

ModianCard::ModianCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "modian_attach";
}

bool ModianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->hasSkill("modian") && !to_select->hasFlag("modianInvoked");
}

void ModianCard::use(Room *room, ServerPlayer *src, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *alice = targets.first();
    room->setPlayerFlag(alice, "modianInvoked");
    room->notifySkillInvoked(alice, "modian");

    bool draw = true;
    foreach (int id, alice->getPile("modian")) {
        if (Sanguosha->getCard(id)->objectName() == Sanguosha->getCard(subcards.first())->objectName()) {
            draw = false;
            break;
        } else if (Sanguosha->getCard(id)->isKindOf("Slash") && Sanguosha->getCard(subcards.first())->isKindOf("Slash")) {
            draw = false;
            break;
        }
    }
    if (draw)
        src->drawCards(1);

    CardMoveReason reason(CardMoveReason::S_REASON_UNKNOWN, "", NULL, "modian", "");
    alice->addToPile("modian", subcards, true, reason);

    if (alice->getPile("modian").length() > alice->getHp()) {
        const Card *c = room->askForCard(alice, "@@modian!", "@modian", QVariant(), Card::MethodNone, NULL, false, "modian");
        // force discard!!!
        if (c == NULL) {
            QList<int> modians = alice->getPile("modian");

            int x = qrand() % modians.length();
            int id = modians.value(x);
            c = Sanguosha->getCard(id);
        }

        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", NULL, objectName(), "");
        room->throwCard(c, reason, NULL);
        room->recover(alice, RecoverStruct());
    }
}

class ModianVS : public ViewAsSkill
{
public:
    ModianVS()
        : ViewAsSkill("modian_attach")
    {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasSkill("modian") && !p->hasFlag("modianInvoked"))
                return true;
        }
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.isEmpty() && !to_select->isEquipped() && (to_select->isNDTrick() || to_select->isKindOf("Slash"));
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 1) {
            ModianCard *card = new ModianCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }
};

class ModianSelfVS : public OneCardViewAsSkill
{
public:
    ModianSelfVS()
        : OneCardViewAsSkill("modian")
    {
        expand_pile = "modian";
        response_pattern = "@@modian!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasSkill("modian") && !p->hasFlag("modianInvoked"))
                return true;
        }
        if (player->hasSkill("modian") && !player->hasFlag("modianInvoked"))
            return true;
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY)
            return (to_select->isNDTrick() || to_select->isKindOf("Slash")) && !to_select->isEquipped() && !Self->getPile("modian").contains(to_select->getEffectiveId());
        else
            return Self->getPile("modian").contains(to_select->getEffectiveId());
        return false;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            ModianCard *card = new ModianCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return originalCard;
    }
};

class Modian : public TriggerSkill
{
public:
    Modian()
        : TriggerSkill("modian")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging << Death << Debut << Revive;
        view_as_skill = new ModianSelfVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent != EventPhaseChanging) { //the case operating attach skill
            QList<ServerPlayer *> alices;
            static QString attachName = "modian_attach";
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(this, true))
                    alices << p;
            }

            if (alices.length() >= 1) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->hasSkill(attachName, true) && !p->hasSkill("modian"))
                        room->attachSkillToPlayer(p, attachName);
                    else if (p->hasSkill(attachName, true) && p->hasSkill("modian"))
                        room->detachSkillFromPlayer(p, attachName, true);
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
                    if (p->hasFlag("modianInvoked"))
                        room->setPlayerFlag(p, "-modianInvoked");
                }
            }
        }
    }
};

class Guaiqi : public ViewAsSkill
{
public:
    Guaiqi()
        : ViewAsSkill("guaiqi")
    {
        expand_pile = "modian";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("modian").isEmpty();
    }
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getPile("modian").isEmpty())
            return false;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;
        if (matchAvaliablePattern("slash", pattern)) {
            foreach (int id, player->getPile("modian")) {
                if (Sanguosha->getCard(id)->isKindOf("TrickCard"))
                    return true;
            }
        }

        bool trick = false;
        foreach (int id, player->getPile("modian")) {
            if (Sanguosha->getCard(id)->isKindOf("TrickCard") && matchAvaliablePattern(Sanguosha->getCard(id)->objectName(), pattern)) {
                trick = true;
                break;
            }
        }
        if (trick) {
            foreach (int id, player->getPile("modian")) {
                if (Sanguosha->getCard(id)->isKindOf("Slash"))
                    return true;
            }
        }
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (!Self->getPile("modian").contains(to_select->getEffectiveId()))
            return false;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            if (selected.isEmpty())
                return true;
            else if (selected.length() == 1)
                return to_select->getTypeId() != selected.first()->getTypeId();
        } else {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (selected.isEmpty()) {
                if (to_select->isKindOf("TrickCard"))
                    return matchAvaliablePattern("slash", pattern) || matchAvaliablePattern(to_select->objectName(), pattern);
            } else if (selected.length() == 1) {
                if (to_select->getTypeId() == selected.first()->getTypeId())
                    return false;
                if (selected.first()->isKindOf("Slash"))
                    return matchAvaliablePattern(to_select->objectName(), pattern);
                else
                    return (matchAvaliablePattern(selected.first()->objectName(), pattern));
            }
            return false;
        }
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        //Play
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            if (cards.length() == 1 && cards.first()->isKindOf("TrickCard")) {
                Slash *slash = new Slash(cards.first()->getSuit(), cards.first()->getNumber());
                slash->addSubcard(cards.first());
                slash->setSkillName(objectName());
                return slash;

            } else if (cards.length() == 2) {
                foreach (const Card *c, cards) {
                    if (c->isKindOf("TrickCard")) {
                        Card *card = Sanguosha->cloneCard(c->objectName());
                        foreach (const Card *c, cards) {
                            if (c->isKindOf("Slash")) {
                                card->addSubcard(c);
                                card->setSkillName(objectName());
                                return card;
                            }
                        }
                    }
                }
            }
        } else { // RESPONSE_USE
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (cards.length() == 1 && cards.first()->isKindOf("TrickCard") && matchAvaliablePattern("slash", pattern)) {
                Slash *slash = new Slash(cards.first()->getSuit(), cards.first()->getNumber());
                slash->addSubcard(cards.first());
                slash->setSkillName(objectName());
                return slash;
            } else if (cards.length() == 2) {
                foreach (const Card *c, cards) {
                    if (c->isKindOf("TrickCard") && matchAvaliablePattern(c->objectName(), pattern)) {
                        Card *card = Sanguosha->cloneCard(c->objectName());
                        foreach (const Card *c, cards) {
                            if (c->isKindOf("Slash")) {
                                card->addSubcard(c);
                                card->setSkillName(objectName());
                                return card;
                            }
                        }
                    }
                }
            }
        }
        return NULL;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        bool hasNul = false;
        foreach (int id, player->getPile("modian")) {
            if (Sanguosha->getCard(id)->isKindOf("Nullification")) {
                hasNul = true;
                break;
            }
        }
        if (hasNul) {
            foreach (int id, player->getPile("modian")) {
                if (Sanguosha->getCard(id)->isKindOf("Slash")) {
                    Nullification *nul = new Nullification(Card::SuitToBeDecided, -1);
                    nul->addSubcard(id);
                    DELETE_OVER_SCOPE(Nullification, nul)

                    if (!player->isCardLimited(nul, Card::MethodUse, true))
                        return true;
                }
            }
        }
        return false;
    }
};

BaosiCard::BaosiCard()
{
    handling_method = Card::MethodNone;
    m_skillName = "baosi";
}

void BaosiCard::onEffect(const CardEffectStruct &effect) const
{
    QVariantMap baosi_list = effect.from->tag.value("baosi", QVariantMap()).toMap();
    baosi_list[effect.to->objectName()] = true;
    effect.from->tag["baosi"] = baosi_list;
}

bool BaosiCard::targetFilter(const QList<const Player *> &, const Player *to_select, const Player *) const
{
    return to_select->hasFlag("Global_baosiFailed");
}

class BaosiVS : public ZeroCardViewAsSkill
{
public:
    BaosiVS()
        : ZeroCardViewAsSkill("baosi")
    {
        response_pattern = "@@baosi";
    }

    virtual const Card *viewAs() const
    {
        BaosiCard *card = new BaosiCard;
        return card;
    }
};

class Baosi : public TriggerSkill
{
public:
    Baosi()
        : TriggerSkill("baosi")
    {
        events << TargetSpecifying;
        view_as_skill = new BaosiVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from && use.from->hasSkill(this)) {
            if (use.card->isKindOf("Slash") || (use.card->isBlack() && use.card->isNDTrick() && !use.card->isKindOf("Nullification"))) {
                foreach (ServerPlayer *p, room->getOtherPlayers(use.from)) {
                    if (!use.to.contains(p) && p->getHp() <= p->dyingThreshold() && !use.from->isProhibited(p, use.card))
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (!use.to.contains(p) && p->getHp() <= p->dyingThreshold() && !use.from->isProhibited(p, use.card))
                room->setPlayerFlag(p, "Global_baosiFailed");
        }
        return room->askForUseCard(invoke->invoker, "@@baosi", "@baosi:" + use.card->objectName());
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QVariantMap baosi_list = invoke->invoker->tag.value("baosi", QVariantMap()).toMap();
        invoke->invoker->tag.remove("baosi");
        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            bool add = baosi_list.value(p->objectName(), false).toBool();
            if (add)
                use.to << p;
        }
        room->sortByActionOrder(use.to);
        data = QVariant::fromValue(use);
        return false;
    }
};

EzhaoCard::EzhaoCard()
{
}

bool EzhaoCard::targetFilter(const QList<const Player *> &, const Player *to_select, const Player *Self) const
{
    return to_select != Self && to_select->isWounded();
}

void EzhaoCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->doLightbox("$ezhaoAnimate", 4000);
    SkillCard::onUse(room, card_use);
}

void EzhaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->removePlayerMark(source, "@ezhao");
    foreach (ServerPlayer *p, targets) {
        room->recover(p, RecoverStruct());
        room->setPlayerProperty(p, "dyingFactor", p->getDyingFactor() + 1);
    }
}

class Ezhao : public ZeroCardViewAsSkill
{
public:
    Ezhao()
        : ZeroCardViewAsSkill("ezhao")
    {
        frequency = Limited;
        limit_mark = "@ezhao";
    }

    virtual const Card *viewAs() const
    {
        return new EzhaoCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@ezhao") >= 1;
    }
};

ZongjiuCard::ZongjiuCard()
{
    will_throw = false;
    handling_method = Card::MethodNone; //related to UseCardLimit
}

bool ZongjiuCard::targetFixed() const
{
    Analeptic *ana = new Analeptic(Card::NoSuit, 0);
    ana->setSkillName("zongjiu");
    ana->deleteLater();
    return ana->targetFixed();
}

const Card *ZongjiuCard::validate(CardUseStruct &use) const
{
    use.from->removeShownHandCards(subcards, true);
    Analeptic *ana = new Analeptic(Card::NoSuit, 0);
    ana->setSkillName("zongjiu");
    return ana;
}

class ZongjiuVS : public OneCardViewAsSkill
{
public:
    ZongjiuVS()
        : OneCardViewAsSkill("zongjiu")
    {
        filter_pattern = ".|.|.|show";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Analeptic::IsAvailable(player) && !player->getShownHandcards().isEmpty();
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return !player->getShownHandcards().isEmpty() && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
            && matchAvaliablePattern("analeptic", pattern);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            ZongjiuCard *card = new ZongjiuCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }
};

class Zongjiu : public TriggerSkill
{
public:
    Zongjiu()
        : TriggerSkill("zongjiu")
    {
        events << CardUsed;
        view_as_skill = new ZongjiuVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Peach")) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->getCards("h").length() > 0)
                    d << SkillInvokeDetail(this, p, p);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        const Card *card = room->askForCard(invoke->invoker, ".|.|.|handOnly", "@zongjiu", data, Card::MethodNone, NULL, false, objectName());
        if (card) {
            invoke->invoker->tag["zongjiu1"] = QVariant::fromValue(card->getEffectiveId());
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        int id = invoke->invoker->tag["zongjiu1"].toInt();
        invoke->invoker->addToShownHandCards(QList<int>() << id);
        return false;
    }
};

class Xingyou : public TriggerSkill
{
public:
    Xingyou()
        : TriggerSkill("xingyou")
    {
        events << CardUsed << SlashMissed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.from->isAlive() && use.from->hasSkill(this)) {
                foreach (int id, use.from->getShownHandcards()) {
                    if (Sanguosha->getCard(id)->getSuit() == use.card->getSuit())
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
                }
            }
        } else {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.from->isAlive() && effect.from->hasSkill(this) && effect.jink != NULL
                && effect.to->isAlive() && effect.from->canDiscard(effect.to, "hes")) {
                foreach (int id, effect.from->getShownHandcards()) {
                    if (Sanguosha->getCard(id)->getSuit() == effect.slash->getSuit())
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.from, effect.from, NULL, false, effect.to);
                }
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (e == CardUsed) {
            invoke->invoker->drawCards(1);
        } else {
            int card_id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hes", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, invoke->targets.first(), invoke->invoker);
        }
        return false;
    }
};

class Huanshu : public TriggerSkill
{
public:
    Huanshu()
        : TriggerSkill("huanshu")
    {
        events << PostCardEffected;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.from && effect.from != effect.to && effect.to->hasSkill(this) && effect.to->isAlive() && effect.to->getShownHandcards().length() < effect.to->getHp()
            && (effect.card->isNDTrick() || effect.card->isKindOf("BasicCard"))) {
            QList<int> ids;
            if (effect.card->isVirtualCard())
                ids = effect.card->getSubcards();
            else
                ids << effect.card->getEffectiveId();

            if (ids.isEmpty())
                return QList<SkillInvokeDetail>();
            foreach (int id, ids) {
                if (room->getCardPlace(id) != Player::PlaceTable)
                    return QList<SkillInvokeDetail>();
            }
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        invoke->invoker->obtainCard(effect.card);
        invoke->invoker->addToShownHandCards(effect.card->getSubcards());
        return false;
    }
};

QirenCard::QirenCard()
{
    will_throw = false;
}

bool QirenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int shownId = (Self->getShownHandcards().contains(subcards.first())) ? subcards.first() : subcards.last();
    Card *oc = Sanguosha->getCard(shownId);
    int id = (Self->getShownHandcards().contains(subcards.first())) ? subcards.last() : subcards.first();

    //do not consider extraTarget
    if (oc->isKindOf("Collateral") && !targets.isEmpty()) {
        return false;
    }
    //only consider slashTargetFix
    if (Sanguosha->getCard(id)->isKindOf("Slash")) {
        if (Self->hasFlag("slashTargetFix")) {
            if (Self->hasFlag("slashDisableExtraTarget")) {
                return to_select->hasFlag("SlashAssignee") && oc->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, oc, targets);
            } else {
                bool need_specific_assignee = true;
                foreach (const Player *p, Self->getAliveSiblings()) {
                    if (p->hasFlag("SlashAssignee") && targets.contains(p)) {
                        need_specific_assignee = false;
                        break;
                    }
                }
                if (need_specific_assignee)
                    return to_select->hasFlag("SlashAssignee") && oc->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, oc, targets);
            }
        }
    }

    return oc->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, oc, targets);
}

bool QirenCard::targetFixed() const
{
    int shownId = (Self->getShownHandcards().contains(subcards.first())) ? subcards.first() : subcards.last();
    return Sanguosha->getCard(shownId)->targetFixed();
}

bool QirenCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    int shownId = (Self->getShownHandcards().contains(subcards.first())) ? subcards.first() : subcards.last();

    if (Sanguosha->getCard(shownId)->canRecast() && targets.length() == 0)
        return false;
    //do not consider extraTarget
    if (Sanguosha->getCard(shownId)->isKindOf("Collateral"))
        return targets.length() == 1;
    return Sanguosha->getCard(shownId)->targetsFeasible(targets, Self);
}

bool QirenCard::isAvailable(const Player *player) const
{
    int shownId = (Self->getShownHandcards().contains(subcards.first())) ? subcards.first() : subcards.last();
    int id = (Self->getShownHandcards().contains(subcards.first())) ? subcards.last() : subcards.first();

    Card *oc = Sanguosha->getCard(shownId);

    //Do not check isAvaliable() in ViewFilter in ViewAsSkill
    //check prohibit
    if (oc->targetFixed()) {
        if (oc->isKindOf("AOE") || oc->isKindOf("GlobalEffect")) {
            QList<const Player *> players = player->getAliveSiblings();
            if (oc->isKindOf("GlobalEffect"))
                players << Self;
            int count = 0;
            foreach (const Player *p, players) {
                if (!player->isProhibited(p, oc))
                    count++;
            }
            if (count == 0)
                return false;
        } else if (oc->isKindOf("EquipCard") || oc->isKindOf("Peach") || oc->isKindOf("Analeptic") || oc->isKindOf("DelayedTrick")) {
            if (player->isProhibited(player, oc))
                return false;
        }
    }
    //check times
    Card *card = Sanguosha->getCard(id);
    bool play = player->getPhase() == Player::Play && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY;
    if (card->isKindOf("Slash") && play) {
        if (!Slash::IsAvailable(player, card, true))
            return false;
    }
    if (card->isKindOf("Analeptic") && play) {
        if (player->usedTimes("Analeptic") > Sanguosha->correctCardTarget(TargetModSkill::Residue, Self, card))
            return false;
    }
    return !player->isCardLimited(card, Card::MethodUse);
}

//operate the targets
void QirenCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    int shownId = (card_use.from->isShownHandcard(subcards.first())) ? subcards.first() : subcards.last();
    int id = (card_use.from->isShownHandcard(subcards.first())) ? subcards.last() : subcards.first();
    Card *oc = Sanguosha->getCard(shownId);
    Card *card = Sanguosha->getCard(id);
    QList<ServerPlayer *> targets;
    CardUseStruct use = card_use;
    use.card = card;
    //target_fixed
    if (use.to.isEmpty()) {
        if (oc->isKindOf("AOE") || oc->isKindOf("GlobalEffect")) {
            ServerPlayer *source = card_use.from;
            QList<ServerPlayer *> players = (oc->isKindOf("GlobalEffect")) ? room->getAllPlayers() : room->getOtherPlayers(source);
            foreach (ServerPlayer *player, players) {
                const ProhibitSkill *skill = room->isProhibited(source, player, oc);
                if (skill) {
                    LogMessage log;
                    log.type = "#SkillAvoid";
                    log.from = player;
                    log.arg = skill->objectName();
                    log.arg2 = objectName();
                    room->sendLog(log);

                    if (player->hasSkill(skill))
                        room->notifySkillInvoked(player, skill->objectName());
                    room->broadcastSkillInvoke(skill->objectName());
                } else
                    targets << player;
            }
            use.to = targets;
        } else if (oc->isKindOf("EquipCard") || oc->isKindOf("Peach") || oc->isKindOf("Analeptic") || oc->isKindOf("DelayedTrick")) {
            use.to << use.from;
        }
    }

    room->setPlayerFlag(use.from, "qirenUsed");
    use.from->removeShownHandCards(QList<int>() << shownId, true);
    LogMessage log;
    log.type = "#Qiren";
    log.from = use.from;
    log.arg = oc->objectName();
    log.arg2 = "qiren";
    room->sendLog(log);

    if (card->isKindOf("Collateral")) {
        QList<ServerPlayer *> killers = use.to;
        ServerPlayer *killer = NULL;
        use.to.clear();
        foreach (ServerPlayer *k, killers) {
            QList<ServerPlayer *> victims;
            foreach (ServerPlayer *p, room->getOtherPlayers(k)) {
                if (k->canSlash(p)) {
                    victims << p;
                }
            }
            if (!victims.isEmpty() && killer == NULL) {
                killer = k;
                use.to << killer;
                ServerPlayer *victim = room->askForPlayerChosen(use.from, victims, "qiren", "@qiren:" + killer->objectName());
                use.to << victim;
            } else {
                room->setCardFlag(card, "qirenCollater_" + k->objectName());
            }
        }
        //if Collateral has no effect, do use directly for moving this card.
        if (use.to.length() != 2) {
            use.to = killers;
            QList<CardsMoveStruct> moves;

            QVariant data = QVariant::fromValue(use);
            RoomThread *thread = room->getThread();

            thread->trigger(PreCardUsed, room, data);
            use = data.value<CardUseStruct>();

            LogMessage log;
            log.from = use.from;
            log.to << use.to;
            log.type = "#UseCard";
            log.card_str = use.card->toString();
            room->sendLog(log);

            CardMoveReason reason(CardMoveReason::S_REASON_USE, use.from->objectName(), QString(), use.card->getSkillName(), QString());
            if (use.to.size() == 1)
                reason.m_targetId = use.to.first()->objectName();

            reason.m_extraData = QVariant::fromValue(use.card);
            CardsMoveStruct move(use.card->getEffectiveId(), NULL, Player::PlaceTable, reason);
            moves.append(move);
            room->moveCardsAtomic(moves, true);

            thread->trigger(CardUsed, room, data);
            use = data.value<CardUseStruct>();
            thread->trigger(CardFinished, room, data);
            return;
        }
    }

    //do new use
    room->useCard(use);
}

class QirenVS : public ViewAsSkill
{
public:
    QirenVS()
        : ViewAsSkill("qiren")
    {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getShownHandcards().isEmpty() && !player->hasFlag("qirenUsed");
    }
    virtual bool isEnabledAtResponse(const Player *player, const QString &) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            return false;
        return !player->getShownHandcards().isEmpty() && !player->hasFlag("qirenUsed");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (to_select->isEquipped())
            return false;
        if (to_select->isKindOf("Jink") || to_select->isKindOf("Nullification"))
            return false;

        if (selected.length() >= 2)
            return false;

        if (!Self->isShownHandcard(to_select->getEffectiveId())) {
            if (to_select->isKindOf("EquipCard") || to_select->isKindOf("DelayedTrick"))
                return false;
            if (Self->isCardLimited(to_select, Card::MethodUse))
                return false;
            bool matchPattern = true;
            if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
                QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
                ExpPattern p(pattern);
                matchPattern = p.match(Self, to_select);
            }
            return (selected.isEmpty() || Self->isShownHandcard(selected.first()->getEffectiveId())) && matchPattern;
        } else {
            return selected.isEmpty() || !Self->isShownHandcard(selected.first()->getEffectiveId());
        }
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        QirenCard *card = new QirenCard;
        card->addSubcards(cards);
        return card;
    }
};

class Qiren : public TriggerSkill
{
public:
    Qiren()
        : TriggerSkill("qiren")
    {
        events << EventPhaseChanging << PreCardUsed;
        view_as_skill = new QirenVS;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("qirenUsed"))
                    room->setPlayerFlag(p, "-qirenUsed");
            }
        } else if (e == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Collateral")) {
                foreach (ServerPlayer *k, room->getAlivePlayers()) {
                    if (use.card->hasFlag("qirenCollater_" + k->objectName())) {
                        QList<ServerPlayer *> victims;
                        foreach (ServerPlayer *p, room->getOtherPlayers(k)) {
                            if (k->canSlash(p))
                                victims << p;
                        }
                        if (!victims.isEmpty()) {
                            use.to << k;
                            ServerPlayer *victim = room->askForPlayerChosen(use.from, victims, "qiren", "@qiren:" + k->objectName());
                            k->tag["collateralVictim"] = QVariant::fromValue((ServerPlayer *)victim);
                            LogMessage log;
                            log.type = "#QishuAdd";
                            log.from = use.from;
                            log.to << k;
                            log.arg = use.card->objectName();
                            log.arg2 = "qiren";
                            room->sendLog(log);

                            LogMessage log1;
                            log1.type = "#CollateralSlash";
                            log1.from = use.from;
                            log1.to << victim;
                            room->sendLog(log1);
                        }
                    }
                }
                data = QVariant::fromValue(use);
            }
        }
    }
};

TH0105Package::TH0105Package()
    : Package("th0105")
{
    General *mima = new General(this, "mima$", "pc98", 4, false);
    mima->addSkill(new Meiling);
    mima->addSkill(new Fuchou);

    General *yumemi = new General(this, "yumemi$", "pc98", 4, false);
    yumemi->addSkill(new Ciyuan);
    yumemi->addSkill(new Shigui);
    yumemi->addSkill(new Chongdong);

    General *chiyuri = new General(this, "chiyuri", "pc98", 4, false);
    chiyuri->addSkill(new Zhence);
    chiyuri->addSkill(new Shiqu);

    General *rikako = new General(this, "rikako", "pc98", 4, false);
    rikako->addSkill(new Zhenli);
    rikako->addSkill(new Qiusuo);

    General *kana = new General(this, "kana", "pc98", 3, false);
    kana->addSkill(new Mengxiao);
    kana->addSkill(new Lubiao);

    General *yuka_old = new General(this, "yuka_old$", "pc98", 4, false);
    yuka_old->addSkill(new Yeyan);
    yuka_old->addSkill(new Youyue);
    yuka_old->addSkill(new Menghuan);

    General *gengetsumugetsu = new General(this, "gengetsumugetsu", "pc98", 3, false);
    //gengetsumugetsu->addSkill(new Huantong);
    //gengetsumugetsu->addSkill(new Mengyan);
    gengetsumugetsu->addSkill(new Xuxiang);
    gengetsumugetsu->addSkill(new XuxiangRecord);
    gengetsumugetsu->addSkill(new Huanjue);
    related_skills.insertMulti("xuxiang", "#xuxiang");

    General *elly = new General(this, "elly", "pc98", 4, false);
    elly->addSkill(new Lianmu);
    elly->addSkill(new LianmuTargetMod);
    elly->addSkill(new HuanweiEffect);
    elly->addSkill(new Huanwei);
    related_skills.insertMulti("lianmu", "#lianmu_mod");
    related_skills.insertMulti("huanwei", "#huanwei");

    General *shinki = new General(this, "shinki$", "pc98", 4, false);
    shinki->addSkill(new SqChuangshi);
    shinki->addSkill(new Yuanfa);
    shinki->addSkill(new Shenwei);

    General *alice_old = new General(this, "alice_old", "pc98", 3, false);
    alice_old->addSkill(new Modian);
    alice_old->addSkill(new Guaiqi);

    General *sariel = new General(this, "sariel", "pc98", 4, false);
    sariel->addSkill(new Baosi);
    sariel->addSkill(new Ezhao);

    General *konngara = new General(this, "konngara", "pc98", 4, false);
    konngara->addSkill(new Zongjiu);
    konngara->addSkill(new Xingyou);

    General *yumeko = new General(this, "yumeko", "pc98", 4, false);
    yumeko->addSkill(new Huanshu);
    yumeko->addSkill(new Qiren);

    addMetaObject<ShiquCard>();
    addMetaObject<LianmuCard>();
    addMetaObject<SqChuangshiCard>();
    addMetaObject<ModianCard>();
    addMetaObject<BaosiCard>();
    addMetaObject<EzhaoCard>();
    addMetaObject<ZongjiuCard>();
    addMetaObject<QirenCard>();

    skills << new ModianVS;
}

ADD_PACKAGE(TH0105)
