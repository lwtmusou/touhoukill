#include "th01-05.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "washout.h"

class Meiling : public TriggerSkill
{
public:
    Meiling()
        : TriggerSkill("meiling")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if ((damage.card == nullptr) || damage.from == nullptr || damage.to->isDead())
            return QList<SkillInvokeDetail>();

        QList<int> ids;
        if (damage.card->isVirtualCard())
            ids = damage.card->getSubcards();
        else
            ids << damage.card->getEffectiveId();

        if (ids.isEmpty())
            return QList<SkillInvokeDetail>();
        if (damage.card->isKindOf("Jink") || damage.card->isKindOf("Nullification")) { //couple xianshi
            foreach (int id, ids) {
                if (room->getCardPlace(id) != Player::DiscardPile)
                    return QList<SkillInvokeDetail>();
            }
        } else {
            foreach (int id, ids) {
                if (room->getCardPlace(id) != Player::PlaceTable)
                    return QList<SkillInvokeDetail>();
            }
        }

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *mima, room->findPlayersBySkillName(objectName())) {
            if (mima != damage.from)
                d << SkillInvokeDetail(this, mima, mima);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt = "distance:" + damage.to->objectName() + ":" + QString::number(invoke->invoker->distanceTo(damage.to)) + ":" + damage.card->objectName();
        invoke->invoker->tag["meiling"] = data;
        return invoke->invoker->askForSkillInvoke(this, prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        invoke->invoker->obtainCard(damage.card);
        if (damage.to->isAlive() && damage.from->isAlive() && invoke->invoker->distanceTo(damage.to) > invoke->invoker->getLostHp()) {
            room->damage(DamageStruct(objectName(), damage.from, invoke->invoker, 1, DamageStruct::Normal));
        }
        return false;
    }
};

class Fuchou : public TargetModSkill
{
public:
    Fuchou()
        : TargetModSkill("fuchou$")
    {
        pattern = "Slash";
    }

    int getResidueNum(const Player *from, const Card *) const override
    {
        int r = 0;

        if (from->hasLordSkill(this)) {
            QList<const Player *> sib = from->getAliveSiblings();
            sib << from;

            foreach (const Player *s, sib) {
                if (s->getKingdom() == "pc98" && s->getHp() >= from->getHp())
                    ++r;
            }
        }

        return r;
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->getPhase() == Player::RoundStart)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            Player::Phase phase = current->getPhase();
            QList<Player::Phase> phases;
            phases << Player::Start << Player::Judge << Player::Draw << Player::Play << Player::Discard << Player::Finish;

            if (phases.contains(phase)) {
                room->setPlayerMark(current, "shigui", current->getMark("shigui") + 1);
                if (current->hasSkill(objectName())) {
                    LogMessage log;
                    log.type = "#shigui_log";
                    log.from = current;
                    log.to << current;
                    log.arg = QString::number(current->getMark("shigui"));
                    log.arg2 = current->getPhaseString();
                    room->doNotify(current, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
                }
            }
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                room->setPlayerMark(change.player, "shigui", 0);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current->hasSkill(this) && !current->hasFlag("shigui_used") && (current->getPhase() == Player::Draw || current->getPhase() == Player::Play)
                && current->getHandcardNum() != current->getMark("shigui"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (invoke->invoker->getHandcardNum() > invoke->invoker->getMark("shigui")) {
            int n = invoke->invoker->getHandcardNum() - invoke->invoker->getMark("shigui");
            if (room->askForDiscard(invoke->invoker, objectName(), n, n, true, false,
                                    "@shigui-discard:::" + QString::number(n) + ":" + QString::number(invoke->invoker->getMark("shigui")))) {
                invoke->tag["discard"] = true;
                return true;
            }
        } else {
            int n = invoke->invoker->getMark("shigui") - invoke->invoker->getHandcardNum();
            if (room->askForSkillInvoke(invoke->invoker, this, "drawcard:::" + QString::number(n) + ":" + QString::number(invoke->invoker->getMark("shigui")))) {
                invoke->tag["discard"] = false;
                room->drawCards(invoke->invoker, n, objectName());
                return true;
            }
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *current = invoke->invoker;
        bool recover = invoke->tag["discard"].toBool();

        if (recover) {
            room->setPlayerFlag(current, "shigui_used");
            RecoverStruct recover;
            room->recover(invoke->invoker, recover);
        } else {
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
        events << EventPhaseSkipping;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        PhaseSkippingStruct s = data.value<PhaseSkippingStruct>();
        if (s.player == nullptr || s.player->isDead() || s.player->getKingdom() != "pc98")
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->getOtherPlayers(s.player)) {
            if (p->hasLordSkill(objectName())) {
                d << SkillInvokeDetail(this, p, s.player);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        return invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->owner));
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(invoke->owner, objectName());

        LogMessage log;
        log.type = "#InvokeOthersSkill";
        log.from = invoke->invoker;
        log.to << invoke->owner;
        log.arg = objectName();
        room->sendLog(log);
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->owner->objectName());

        invoke->owner->drawCards(1);
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

    const Card *viewAs() const override
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

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
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
            if ((damage.from != nullptr) && damage.from->isAlive() && (damage.card != nullptr) && damage.card->getSkillName() == "zhence")
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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
    Player::Phase phase = Player::PhaseNone;
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

    const Card *viewAs(const Card *originalCard) const override
    {
        if (originalCard != nullptr) {
            ShiquCard *card = new ShiquCard;
            card->addSubcard(originalCard);
            return card;
        }
        return nullptr;
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

    bool canPreshow() const override
    {
        return true;
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
        foreach (const QString &m, marks) {
            if (player->getMark(m) == 0)
                choices << m;
        }
        return choices;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        QStringList choices = shiquChoices(current);
        choices << "cancel";
        QString choice = room->askForChoice(invoke->invoker, objectName(), choices.join("+"));

        if (choice != "cancel") {
            QString prompt = "@shiqu-discard:" + choice + ":" + current->objectName();
            invoke->invoker->tag["shiqu"] = QVariant::fromValue(choice);
            return room->askForUseCard(invoke->invoker, "@@shiqu", prompt) != nullptr;
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
        related_pile = "zhenli";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == AfterDrawInitialCards) {
            DrawNCardsStruct dc = data.value<DrawNCardsStruct>();
            if (dc.player->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dc.player, dc.player, nullptr, true);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.reason.m_reason == CardMoveReason::S_REASON_DRAW && move.reason.m_extraData.toString() != "initialDraw") {
                ServerPlayer *rikako = qobject_cast<ServerPlayer *>(move.to);
                if (rikako != nullptr && rikako->hasSkill(this)) {
                    foreach (int id, move.card_ids) {
                        if (room->getCardPlace(id) == Player::PlaceHand && room->getCardOwner(id) == rikako)
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, rikako, rikako, nullptr, true);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    //default cost

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->sendLog("#TriggerSkill", invoke->invoker, objectName());
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

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        if (Self->getPile("zhenli").contains(to_select->getEffectiveId())) {
            if (selected.isEmpty())
                return true;
            else
                return to_select->getNumber() == selected.first()->getNumber();
        }
        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() > 0) {
            DummyCard *dc = new DummyCard;
            dc->addSubcards(cards);
            return dc;
        }
        return nullptr;
    }
};

class Qiusuo : public TriggerSkill
{
public:
    Qiusuo()
        : TriggerSkill("qiusuo")
    {
        events << EventPhaseStart << DamageDone << TurnStart;
        view_as_skill = new QiusuoVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        switch (triggerEvent) {
        case DamageDone: {
            DamageStruct damage = data.value<DamageStruct>();
            if ((damage.from != nullptr) && damage.from->isCurrent())
                room->setTag("qiusuoDamageOrDeath", true);
            break;
        }
        case TurnStart:
            room->setTag("qiusuoDamageOrDeath", false);
            break;
        default:
            break;
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e != EventPhaseStart)
            return QList<SkillInvokeDetail>();
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->getPhase() == Player::Start && current->hasSkill(this) && !current->getPile("zhenli").isEmpty())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current);
        if (current->getPhase() == Player::Finish && current->hasSkill(this)) {
            QVariant tag = room->getTag("qiusuoDamageOrDeath");
            bool can = tag.canConvert(QVariant::Bool) && tag.toBool();
            if (can)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        const Card *c = room->askForCard(invoke->invoker, "@@qiusuo", "@qiusuo", data, Card::MethodNone, nullptr, false, objectName());
        if (c != nullptr) {
            QVariantList ids;
            foreach (int card_id, c->getSubcards())
                ids << card_id;
            invoke->invoker->tag["qiusuo"] = ids;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QVariantList ids = invoke->invoker->tag["qiusuo"].toList();
        invoke->invoker->tag.remove("qiusuo");
        room->sendLog("#InvokeSkill", invoke->invoker, objectName());
        QList<int> get_ids;
        foreach (const QVariant &card_data, ids) {
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

    static QStringList mengxiaoChoices(ServerPlayer *kana, ServerPlayer *target = nullptr)
    {
        QStringList trick_list;
        if (target != nullptr) {
            foreach (const Card *trick, kana->getJudgingArea()) {
                if (!target->containsTrick(trick->objectName()) && !kana->isProhibited(target, trick))
                    trick_list << trick->objectName();
            }
        } else {
            QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
            QStringList ban_list = Sanguosha->getBanPackages();
            foreach (const Card *card, cards) {
                if (card->getTypeId() == Card::TypeTrick && !card->isNDTrick() && card->getSubtype() == "unmovable_delayed_trick" && !trick_list.contains(card->objectName())
                    && !ban_list.contains(card->getPackage())) { // && !ServerInfo.Extensions.contains("!" + card->getPackage())
                    if (!kana->containsTrick(card->objectName()))
                        trick_list << card->objectName();
                }
            }
        }
        return trick_list;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
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
                    d << SkillInvokeDetail(this, p, p, nullptr, false, current);
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->getPhase() == Player::Finish) {
            QStringList trick_list = mengxiaoChoices(invoke->invoker);
            trick_list << "cancel";
            QString choice = room->askForChoice(invoke->invoker, objectName(), trick_list.join("+"), data);
            if (choice == "cancel")
                return false;

            const Card *card = room->askForCard(invoke->invoker, ".", "@mengxiao:" + choice, data, Card::MethodNone, nullptr, false, objectName());
            if (card == nullptr)
                return false;
            invoke->tag["mengxiao_choice"] = QVariant::fromValue(choice);
            invoke->tag["mengxiao_id"] = QVariant::fromValue(card->getEffectiveId());
            return true;
        } else {
            return invoke->invoker->askForSkillInvoke(this, data);
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *current = room->getCurrent();
        if (current->getPhase() == Player::Finish) {
            room->notifySkillInvoked(invoke->invoker, objectName());

            int card_id = invoke->tag["mengxiao_id"].toInt();
            QString choice = invoke->tag.value("mengxiao_choice", QString()).toString();

            LogMessage log;
            log.type = "$Mengxiao";
            log.from = invoke->invoker;
            log.arg = objectName();
            log.arg2 = choice;
            log.card_str = QString::number(card_id);
            room->sendLog(log);

            Card *delayTrick = Sanguosha->cloneCard(choice);
            WrappedCard *vs_card = Sanguosha->getWrappedCard(card_id);
            vs_card->setSkillName(objectName());
            vs_card->takeOver(delayTrick);
            room->broadcastUpdateCard(room->getAlivePlayers(), vs_card->getId(), vs_card);

            CardsMoveStruct move;
            move.card_ids << vs_card->getId();
            move.to = invoke->invoker;
            move.to_place = Player::PlaceDelayedTrick;
            room->moveCardsAtomic(move, true);
        } else {
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card == nullptr || !damage.to->hasSkill(this))
            return QList<SkillInvokeDetail>();
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            foreach (const Card *c, p->getCards("j")) {
                if (!c->sameColorWith(damage.card))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, nullptr, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        damage.damage = damage.damage - 1;

        room->sendLog("#lubiao", invoke->invoker, objectName(), QList<ServerPlayer *>(), QString::number(1));
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from != nullptr && use.from->hasSkill(this) && use.from->isAlive() && (use.card->isNDTrick() || use.card->isKindOf("Slash"))) {
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();

        QList<ServerPlayer *> logto;
        logto << invoke->targets;
        room->sendLog("#yeyan", invoke->invoker, objectName(), logto, use.card->objectName());
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
            if (card == nullptr) {
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
            if (card1 != nullptr)
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    int getExtra(const Player *target) const override
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

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        return Self->getPile("dream").contains(to_select->getEffectiveId()) && selected.length() < 2;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() == 2) {
            DummyCard *dc = new DummyCard;
            dc->addSubcards(cards);
            return dc;
        }
        return nullptr;
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
        related_pile = "dream";
    }

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.damage != 1)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p->inMyAttackRange(damage.to) || p == damage.to)
                d << SkillInvokeDetail(this, p, p, nullptr, false, damage.to);
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardMoveReason reason(CardMoveReason::S_REASON_UNKNOWN, "", nullptr, "dream", "");
        invoke->invoker->addToPile("dream", room->getNCards(1), false, reason);
        if (invoke->invoker->getPile("dream").length() >= 2) {
            const Card *c
                = room->askForCard(invoke->invoker, "@@huantong", "@huantong:" + invoke->targets.first()->objectName(), data, Card::MethodNone, nullptr, false, objectName());
            if (c != nullptr) {
                QList<int> ids = c->getSubcards();
                //do damage
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
                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", nullptr, objectName(), "");
                    room->throwCard(&dummy, reason, nullptr);
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;
        if (damage.damage > 1) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (!p->getPile("dream").isEmpty())
                    d << SkillInvokeDetail(this, p, p, nullptr, true);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->sendLog("#TriggerSkill", invoke->invoker, objectName());
        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, invoke->invoker->objectName(), nullptr, objectName(), "");
        CardsMoveStruct move(invoke->invoker->getPile("dream"), invoke->invoker, Player::DiscardPile, reason);
        room->moveCardsAtomic(move, true);

        invoke->invoker->drawCards(2);

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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;
        if ((damage.card != nullptr) && damage.card->isKindOf("Slash") && damage.by_user && (damage.from != nullptr) && damage.from->isAlive() && !damage.from->isKongcheng()) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (!p->hasFlag("xuxiang_used") && p != damage.from && !p->isKongcheng())
                    d << SkillInvokeDetail(this, p, p, nullptr, false, damage.from);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt = "target:" + damage.from->objectName() + ":" + damage.to->objectName() + ":" + damage.card->objectName() + ":" + QString::number(damage.damage);
        return room->askForSkillInvoke(invoke->invoker, objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        DamageStruct damage = data.value<DamageStruct>();
        invoke->invoker->tag["xuxiang"] = QVariant::fromValue(damage);
        room->setPlayerFlag(invoke->invoker, "xuxiang_used");
        if (invoke->invoker->pindian(invoke->targets.first(), objectName())) {
            DamageStruct new_damage = invoke->invoker->tag.value("xuxiang").value<DamageStruct>();
            if (new_damage.damage < damage.damage)
                room->sendLog("#Xuxiang_minus", damage.to, objectName(), QList<ServerPlayer *>(), QString::number(1));
            if (new_damage.damage > damage.damage)
                room->sendLog("#Xuxiang_plus", damage.to, objectName(), QList<ServerPlayer *>(), QString::number(1));
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
    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("xuxiang_used"))
                    room->setPlayerFlag(p, "-xuxiang_used");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == Pindian) {
            PindianStruct *pindian = data.value<PindianStruct *>();
            if (pindian->reason == "xuxiang" && pindian->from_number > pindian->to_number) {
                if (pindian->from_card->isKindOf("Jink") || pindian->to_card->isKindOf("Jink") || pindian->from_card->isKindOf("Slash") || pindian->to_card->isKindOf("Slash"))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, pindian->from, pindian->from, nullptr, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        DamageStruct damage = pindian->from->tag.value("xuxiang").value<DamageStruct>();
        int d = damage.damage;
        if (pindian->from_card->isKindOf("Jink") || pindian->to_card->isKindOf("Jink")) {
            if (invoke->invoker->askForSkillInvoke("xuxiang_plus"))
                d++;
        }
        if (pindian->from_card->isKindOf("Slash") || pindian->to_card->isKindOf("Slash")) {
            if (invoke->invoker->askForSkillInvoke("xuxiang_minus"))
                d--;
        }
        damage.damage = qMax(0, d);
        pindian->from->tag["xuxiang"] = QVariant::fromValue(damage);
        return false;
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->askedPlayer->hasSkill(this)) {
            ServerPlayer *target = (pindian->askedPlayer == pindian->from) ? pindian->to : pindian->from;
            if (pindian->askedPlayer->inMyAttackRange(target) && pindian->from_card == nullptr)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, pindian->askedPlayer, pindian->askedPlayer, nullptr, false, target);
        }
        if (pindian->askedPlayer->hasFlag(objectName())) {
            ServerPlayer *owner = (pindian->askedPlayer == pindian->from) ? pindian->to : pindian->from;
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, owner, pindian->askedPlayer, nullptr, false, owner);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QVariant notify_data = QVariant::fromValue(invoke->preferredTarget);
        bool can = invoke->invoker->askForSkillInvoke(this, notify_data);
        if (!can && invoke->invoker->hasFlag(objectName()))
            room->setPlayerFlag(invoke->invoker, "-huanjue");
        return can;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        int id = -1;
        if (invoke->invoker->hasFlag(objectName())) {
            room->setPlayerFlag(invoke->invoker, "-huanjue");
            id = room->drawCard();
        } else {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());

            QList<int> list = room->getNCards(2);
            room->returnToDrawPile(list);

            room->fillAG(list, invoke->invoker);
            id = room->askForAG(invoke->invoker, list, false, objectName());
            room->clearAG(invoke->invoker);
            if (pindian->from_card == nullptr && pindian->to_card == nullptr)
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
    use.from->showHiddenSkill("lianmu");
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

    const Card *viewAs() const override
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

    bool canPreshow() const override
    {
        return true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if ((damage.card != nullptr) && damage.card->isKindOf("Slash"))
                room->setCardFlag(damage.card, "lianmu_damage");
        }

        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->setPlayerFlag(p, "-lianmu_used");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == CardFinished) {
            ServerPlayer *current = room->getCurrent();
            if (current == nullptr || !current->isInMainPhase())
                return QList<SkillInvokeDetail>();

            CardUseStruct use = data.value<CardUseStruct>();
            if ((use.from != nullptr) && use.from->isAlive() && use.from->hasSkill(this) && !use.from->hasFlag("lianmu_used") && use.card->isKindOf("Slash")
                && !use.card->hasFlag("lianmu_damage"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        return room->askForUseCard(invoke->invoker, "@@lianmu", "@lianmu") != nullptr;
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

    int getExtraTargetNum(const Player *, const Card *card) const override
    {
        if (card->getSkillName() == "lianmu")
            return 1;
        else
            return 0;
    }

    int getDistanceLimit(const Player *, const Card *card) const override
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == GameStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if ((player != nullptr) && player->hasSkill("huanwei"))
                room->filterCards(player, player->getCards("hes"), true);
        }
        if (triggerEvent == EventLoseSkill || triggerEvent == EventAcquireSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            if (a.skill->objectName() == "huanwei")
                room->filterCards(a.player, a.player->getCards("hes"), true);
        }

        if (triggerEvent == EventSkillInvalidityChange) {
            QList<SkillInvalidStruct> invalids = data.value<QList<SkillInvalidStruct>>();
            foreach (SkillInvalidStruct v, invalids) {
                if ((v.skill == nullptr) || v.skill->objectName() == "huanwei") {
                    room->filterCards(v.player, v.player->getCards("hes"), true);
                }
            }
        }
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill("huanwei") && (player->getPhase() == Player::RoundStart || player->getPhase() == Player::NotActive))
                room->filterCards(player, player->getCards("hes"), true);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            //use objectname when checking skill ,not "huanwei"( since it is a static skill)
            if (damage.chain || damage.transfer || !damage.by_user || (damage.from == nullptr) || !damage.from->hasSkill("huanwei") || !damage.from->isCurrent())
                return QList<SkillInvokeDetail>();
            if ((damage.card != nullptr) && damage.card->isKindOf("Slash") && damage.card->getSuit() == Card::Spade)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();

        damage.damage = damage.damage - 1;
        QList<ServerPlayer *> logto;
        logto << damage.to;
        room->sendLog("#HuanweiTrigger", invoke->invoker, "huanwei", logto, QString::number(1));
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

    bool viewFilter(const Card *to_select) const override
    {
        Room *room = Sanguosha->currentRoom();
        ServerPlayer *player = room->getCardOwner(to_select->getId());
        return player != nullptr && !player->isCurrent() && to_select->getSuit() == Card::Spade && to_select->isKindOf("Slash");
    }

    const Card *viewAs(const Card *originalCard) const override
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

void SqChuangshiCard::use(Room *room, const CardUseStruct &card_use) const
{
    const QList<ServerPlayer *> &targets = card_use.to;

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

    const Card *viewAs() const override
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

    bool canPreshow() const override
    {
        return true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->setPlayerMark(p, "sqchuangshi", 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        if (e == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->isAlive() && player->getPhase() == Player::Play)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->askForUseCard(invoke->invoker, "@@sqchuangshi", "@sqchuangshi");
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("sqchuangshi") > 0)
                invoke->targets << p;
        }
        return !invoke->targets.isEmpty();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerMark(p, "yuanfa", 0);
            }
        } else if (e == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            room->setPlayerMark(use.from, "yuanfa", 1);
        } else if (e == CardResponded) {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_isUse)
                room->setPlayerMark(response.m_from, "yuanfa", 1);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *current = room->getCurrent();
        if ((current == nullptr) || !current->hasLordSkill(objectName()) || !current->isAlive())
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
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current, nullptr, true, resp.m_from);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardResponseStruct resp = data.value<CardResponseStruct>();
        resp.m_isNullified = true;
        data = QVariant::fromValue(resp);

        room->notifySkillInvoked(invoke->invoker, objectName());
        room->sendLog("#shenwei", resp.m_from, objectName(), QList<ServerPlayer *>(), resp.m_card->objectName());
        return false;
    }
};

ModianCard::ModianCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "modian_attach";
}

bool ModianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasSkill("modian", false, to_select == Self) && !to_select->hasFlag("modianInvoked");
}

void ModianCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *src = card_use.from;
    const QList<ServerPlayer *> &targets = card_use.to;

    ServerPlayer *alice = targets.first();
    if (src == alice)
        src->showHiddenSkill("modian"); // why not use "setShowSkill?"

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

    CardMoveReason reason(CardMoveReason::S_REASON_UNKNOWN, "", nullptr, "modian", "");
    alice->addToPile("modian", subcards, true, reason);
    if (draw)
        src->drawCards(1);
    if (alice->getPile("modian").length() > alice->getHp()) {
        const Card *c = room->askForCard(alice, "@@modian!", "@modian", QVariant(), Card::MethodNone, nullptr, false, "modian");
        // force discard!!!
        if (c == nullptr) {
            QList<int> modians = alice->getPile("modian");

            int x = qrand() % modians.length();
            int id = modians.value(x);
            c = Sanguosha->getCard(id);
        }

        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, "", nullptr, objectName(), "");
        room->throwCard(c, reason, nullptr);
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

    bool isEnabledAtPlay(const Player *player) const override
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasSkill("modian") && !p->hasFlag("modianInvoked"))
                return true;
        }
        return false;
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        return selected.isEmpty() && !to_select->isEquipped() && (to_select->isNDTrick() || to_select->isKindOf("Slash"));
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() == 1) {
            ModianCard *card = new ModianCard;
            card->addSubcards(cards);
            return card;
        } else
            return nullptr;
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

    bool isEnabledAtPlay(const Player *player) const override
    {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasSkill("modian") && !p->hasFlag("modianInvoked"))
                return true;
        }
        if (player->hasSkill("modian") && !player->hasFlag("modianInvoked"))
            return true;
        return false;
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const override
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY)
            return (to_select->isNDTrick() || to_select->isKindOf("Slash")) && !to_select->isEquipped() && !Self->getPile("modian").contains(to_select->getEffectiveId());
        else
            return Self->getPile("modian").contains(to_select->getEffectiveId());
        return false;
    }

    const Card *viewAs(const Card *originalCard) const override
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
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging << Death << Debut << Revive << GeneralShown;
        view_as_skill = new ModianSelfVS;
        //show_type = "static";
        related_pile = "modian";
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent != EventPhaseChanging) { //the case operating attach skill
            QList<ServerPlayer *> alices;
            static QString attachName = "modian_attach";
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(this, true, false))
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

        } else {
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

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->getPile("modian").isEmpty();
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        if (player->getPile("modian").isEmpty())
            return false;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;

        Slash *slash = new Slash(Card::SuitToBeDecided, -1);
        DELETE_OVER_SCOPE(Slash, slash)

        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);
        if (cardPattern == nullptr)
            return false;

        if (cardPattern->match(player, slash)) {
            foreach (int id, player->getPile("modian")) {
                if (Sanguosha->getCard(id)->isKindOf("TrickCard"))
                    return true;
            }
        }

        bool trick = false;
        foreach (int id, player->getPile("modian")) {
            if (Sanguosha->getCard(id)->isKindOf("TrickCard")) {
                Card *trickCard = Sanguosha->cloneCard(Sanguosha->getCard(id)->objectName());
                DELETE_OVER_SCOPE(Card, trickCard)
                if (cardPattern->match(player, trickCard)) {
                    trick = true;
                    break;
                }
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

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
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
            const CardPattern *cardPattern = Sanguosha->getPattern(pattern);
            if (cardPattern == nullptr)
                return false;

            Slash s(Card::SuitToBeDecided, -1);
            Card *trickCard = Sanguosha->cloneCard(to_select->objectName());
            DELETE_OVER_SCOPE(Card, trickCard);

            if (selected.isEmpty()) {
                if (to_select->isKindOf("TrickCard"))
                    return cardPattern->match(Self, &s) || cardPattern->match(Self, trickCard);
            } else if (selected.length() == 1) {
                if (to_select->getTypeId() == selected.first()->getTypeId())
                    return false;
                if (selected.first()->isKindOf("Slash"))
                    return cardPattern->match(Self, trickCard);
                else {
                    Card *trickCard2 = Sanguosha->cloneCard(selected.first()->objectName());
                    DELETE_OVER_SCOPE(Card, trickCard2);

                    return cardPattern->match(Self, trickCard2);
                }
            }
            return false;
        }
        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        Card *f = nullptr;
        const Card *s = nullptr;
        if (cards.length() == 1 && cards.first()->isKindOf("TrickCard")) {
            f = new Slash(Card::SuitToBeDecided, -1);
            s = cards.first();
        } else if (cards.length() == 2) {
            const Card *t = nullptr;

            foreach (const Card *c, cards) {
                if (c->isKindOf("TrickCard"))
                    t = c;
                else if (c->isKindOf("Slash"))
                    s = c;
            }
            if ((t == nullptr) || (s == nullptr))
                return nullptr;

            f = Sanguosha->cloneCard(t->objectName());
        }

        if (f == nullptr)
            return nullptr;

        f->addSubcard(s);
        f->setSkillName(objectName());
        return f;
#if 0
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
                                card->setCanRecast(false);
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
                                card->setCanRecast(false);
                                return card;
                            }
                        }
                    }
                }
            }
        }
#endif
        return nullptr;
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const override
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

    const Card *viewAs() const override
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

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from != nullptr && use.from->hasSkill(this)) {
            if (use.card->isKindOf("Slash") || (use.card->isBlack() && use.card->isNDTrick() && !use.card->isKindOf("Nullification"))) {
                use.card->setFlags("xunshi");
                QList<const Player *> plist;
                foreach (ServerPlayer *p, use.to)
                    plist << p;
                foreach (ServerPlayer *p, room->getOtherPlayers(use.from)) {
                    if (!use.to.contains(p) && p->getHp() <= p->dyingThreshold() && use.card->targetFilter(QList<const Player *>(), p, use.from)
                        && !use.from->isProhibited(p, use.card, plist)) {
                        use.card->setFlags("-xunshi");
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
                    }
                }
                use.card->setFlags("-xunshi");
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.card->setFlags("xunshi");
        QList<const Player *> plist;
        foreach (ServerPlayer *p, use.to)
            plist << p;
        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (!use.to.contains(p) && p->getHp() <= p->dyingThreshold() && use.card->targetFilter(QList<const Player *>(), p, use.from)
                && !use.from->isProhibited(p, use.card, plist))
                room->setPlayerFlag(p, "Global_baosiFailed");
        }
        use.card->setFlags("-xunshi");
        return room->askForUseCard(invoke->invoker, "@@baosi", "@baosi:" + use.card->objectName()) != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

MoyanCard::MoyanCard()
{
}

bool MoyanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return to_select != Self && Self->getLostHp() > targets.length();
}

void MoyanCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->doLightbox("$moyanAnimate", 4000);
    SkillCard::onUse(room, card_use);
}

void MoyanCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    const QList<ServerPlayer *> &targets = card_use.to;

    room->removePlayerMark(source, "@moyan");
    foreach (ServerPlayer *p, targets)
        room->setPlayerProperty(p, "dyingFactor", p->getDyingFactor() + 1);
}

class Moyan : public ZeroCardViewAsSkill
{
public:
    Moyan()
        : ZeroCardViewAsSkill("moyan")
    {
        frequency = Limited;
        limit_mark = "@moyan";
    }

    const Card *viewAs() const override
    {
        return new MoyanCard;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return player->getMark("@moyan") >= 1 && player->isWounded();
    }
};

class ZongjiuVS : public ViewAsSkill
{
public:
    ZongjiuVS()
        : ViewAsSkill("zongjiu")
    {
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *) const override
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const override
    {
        return pattern.startsWith("@@zongjiu-card");
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        if (Sanguosha->getCurrentCardUsePattern() == "@@zongjiu-card1") {
            if (selected.isEmpty())
                return !to_select->isEquipped();
        }

        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (Sanguosha->getCurrentCardUsePattern() == "@@zongjiu-card1") {
            if (cards.length() == 1) {
                Analeptic *analeptic = new Analeptic(Card::SuitToBeDecided, -1);
                analeptic->addSubcards(cards);
                analeptic->setSkillName(objectName());
                analeptic->setShowSkill(objectName());
                return analeptic;
            }
        } else if (Sanguosha->getCurrentCardUsePattern() == "@@zongjiu-card2") {
            if (cards.length() == 0) {
                Slash *slash = new Slash(Card::NoSuit, 0);
                slash->setSkillName(objectName());
                slash->setShowSkill(objectName());
                return slash;
            }
        }

        return nullptr;
    }
};

class Zongjiu : public TriggerSkill
{
public:
    Zongjiu()
        : TriggerSkill("zongjiu")
    {
        events = {TargetConfirmed, EventPhaseStart};
        view_as_skill = new ZongjiuVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *p, use.to) {
                if (p->isAlive() && p->hasSkill(this) && p->getMark("drank") == 0 && use.card->getTypeId() == Card::TypeTrick)
                    d << SkillInvokeDetail(this, p, p);
            }
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->getPhase() == Player::Finish && p->isAlive()) {
                foreach (ServerPlayer *kon, room->getAllPlayers()) {
                    if (kon->hasSkill(this) && kon->isAlive() && kon->getMark("drank") > 0)
                        d << SkillInvokeDetail(this, kon, kon);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent != EventPhaseStart)
            return room->askForUseCard(invoke->invoker, "@@zongjiu-card1", "@zongjiu-card1", 1, Card::MethodUse, true, objectName());
        else
            return room->askForUseCard(invoke->invoker, "@@zongjiu-card2", "@zongjiu-card2", 2, Card::MethodUse, true, objectName());
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &) const override
    {
        return false;
    }
};

class Xingyou : public TriggerSkill
{
public:
    Xingyou()
        : TriggerSkill("xingyou")
    {
        events = {Damage, Damaged};
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *p = nullptr;
        if (triggerEvent == Damage)
            p = data.value<DamageStruct>().from;
        else if (triggerEvent == Damaged)
            p = data.value<DamageStruct>().to;

        if (p != nullptr && p->isAlive() && p->hasSkill(this) && p->isWounded() && p->getMark("drank") > 0)
            return {SkillInvokeDetail(this, p, p, nullptr, true)};

        return {};
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        LogMessage l;
        l.type = "#TriggerSkill";
        l.from = invoke->invoker;
        l.arg = objectName();
        room->sendLog(l);
        room->notifySkillInvoked(invoke->invoker, objectName());

        room->setPlayerMark(invoke->invoker, "drank", 0);
        RecoverStruct r;
        r.reason = objectName();
        room->recover(invoke->invoker, r);
        return false;
    }
};

class Huanshu : public TriggerSkill
{
public:
    Huanshu()
        : TriggerSkill("huanshu")
    {
        events << PostCardEffected << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerFlag(p, "-huanshu_disable");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e != PostCardEffected)
            return QList<SkillInvokeDetail>();

        CardEffectStruct effect = data.value<CardEffectStruct>();
        if ((effect.from != nullptr) && effect.from != effect.to && effect.to->hasSkill(this) && effect.to->isAlive() && !effect.to->hasFlag("huanshu_disable")
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
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        QString prompt = "invoke:" + effect.card->objectName();
        invoke->invoker->tag["huanshu"] = data; //for ai
        return invoke->invoker->askForSkillInvoke(this, prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        room->setPlayerFlag(invoke->invoker, "huanshu_disable");
        invoke->invoker->obtainCard(effect.card);
        invoke->invoker->addToShownHandCards(effect.card->getSubcards());
        return false;
    }
};

QirenCard::QirenCard()
{
    will_throw = false;
    m_skillName = "qiren";
}

bool QirenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int shownId = (Self->getShownHandcards().contains(subcards.first())) ? subcards.first() : subcards.last();
    Card *oc = Sanguosha->getCard(shownId);
    int id = (Self->getShownHandcards().contains(subcards.first())) ? subcards.last() : subcards.first();

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

bool QirenCard::targetFixed(const Player *Self) const
{
    int shownId = (Self->getShownHandcards().contains(subcards.first())) ? subcards.first() : subcards.last();
    return Sanguosha->getCard(shownId)->targetFixed(Self);
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
    if (oc->targetFixed(player)) {
        if (oc->isKindOf("AOE") || oc->isKindOf("GlobalEffect")) {
            QList<const Player *> players = player->getAliveSiblings();
            if (oc->isKindOf("GlobalEffect"))
                players << Self;
            int count = 0;
            foreach (const Player *p, players) {
                auto useTosExceptp = players;
                useTosExceptp.removeAll(p);
                if (!player->isProhibited(p, oc, useTosExceptp))
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

    room->notifySkillInvoked(use.from, "qiren");
    //show hidden
    card_use.from->showHiddenSkill("qiren");
    //target_fixed
    if (use.to.isEmpty()) {
        if (oc->isKindOf("AOE") || oc->isKindOf("GlobalEffect")) {
            ServerPlayer *source = card_use.from;
            QList<ServerPlayer *> players = (oc->isKindOf("GlobalEffect")) ? room->getAllPlayers() : room->getOtherPlayers(source);
            QList<const Player *> useTos;
            foreach (ServerPlayer *p, players)
                useTos << p;
            foreach (ServerPlayer *player, players) {
                auto useTosExceptp = useTos;
                useTosExceptp.removeAll(player);
                const ProhibitSkill *skill = room->isProhibited(source, player, oc, useTosExceptp);
                if (skill != nullptr) {
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

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->getShownHandcards().isEmpty() && !player->hasFlag("qirenUsed");
    }

    bool isEnabledAtResponse(const Player *player, const QString &) const override
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
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

        return !player->getShownHandcards().isEmpty() && !player->hasFlag("qirenUsed");
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
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

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() != 2)
            return nullptr;

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
        events << EventPhaseChanging;
        view_as_skill = new QirenVS;
    }

    void record(TriggerEvent e, Room *room, QVariant &) const override
    {
        if (e == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("qirenUsed"))
                    room->setPlayerFlag(p, "-qirenUsed");
            }
        }
    }
};

LuliCard::LuliCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void LuliCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;

    LogMessage log;
    log.type = "#Card_Recast";
    log.from = source;
    log.card_str = IntList2StringList(subcards).join("+");
    room->sendLog(log);

    CardMoveReason reason(CardMoveReason::S_REASON_RECAST, source->objectName());
    reason.m_skillName = getSkillName();
    room->moveCardTo(this, source, nullptr, Player::DiscardPile, reason);
    source->broadcastSkillInvoke("@recast");

    source->drawCards(subcards.length());
}

class LuliVS : public ViewAsSkill
{
public:
    LuliVS()
        : ViewAsSkill("luli")
    {
        response_pattern = "@@luli";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *) const override
    {
        int num = Self->getMark("luli");
        return selected.length() < num;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.isEmpty())
            return nullptr;
        LuliCard *card = new LuliCard();
        card->addSubcards(cards);
        return card;
    }
};

class Luli : public TriggerSkill
{
public:
    Luli()
        : TriggerSkill("luli")
    {
        events << PreCardUsed << TargetConfirmed << EventPhaseChanging;
        view_as_skill = new LuliVS;
    }

    bool canPreshow() const override
    {
        return true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from != nullptr && use.from->isCurrent() && (use.card != nullptr))
                room->setPlayerMark(use.from, "luli", use.from->getMark("luli") + 1);
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers(true)) {
                    room->setPlayerMark(p, "luli", 0);
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        if (e == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from == nullptr || !use.from->isCurrent() || use.from->isDead())
                return QList<SkillInvokeDetail>();
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, use.to) {
                if (p->hasSkill(this) && use.from != p && !p->isAllNude())
                    d << SkillInvokeDetail(this, p, p, nullptr, false, use.from);
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        int num = invoke->preferredTarget->getMark("luli");
        room->setPlayerMark(invoke->invoker, "luli", num);
        return room->askForUseCard(invoke->invoker, "@@luli", "luliuse:" + QString::number(num), -1, Card::MethodRecast) != nullptr;
    }
};

class Xunlun : public TriggerSkill
{
public:
    Xunlun()
        : TriggerSkill("xunlun")
    {
        events << CardUsed << CardResponded;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *player = nullptr;
        const Card *card = nullptr;
        if (e == CardUsed) {
            player = data.value<CardUseStruct>().from;
            card = data.value<CardUseStruct>().card;
        } else {
            CardResponseStruct response = data.value<CardResponseStruct>();
            player = response.m_from;
            if (response.m_isUse)
                card = response.m_card;
        }
        if (card == nullptr || card->getTypeId() != Card::TypeBasic)
            return QList<SkillInvokeDetail>();
        if (player == nullptr || player->isDead())
            return QList<SkillInvokeDetail>();

        if (player->getPhase() == Player::Play && player->getCards("s").isEmpty())
            return QList<SkillInvokeDetail>();
        if (player->getPhase() != Player::Play && player->getCards("h").isEmpty())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
            d << SkillInvokeDetail(this, p, p, nullptr, false, player);
        return d;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *target = invoke->targets.first();

        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), target->objectName());
        const Card *card = nullptr;
        if (e == CardUsed) {
            card = data.value<CardUseStruct>().card;
        } else {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_isUse)
                card = response.m_card;
        }

        QString flag = target->getPhase() == Player::Play ? "s" : "h";
        if (target->getCards(flag).isEmpty())
            return false;

        int id = room->askForCardChosen(invoke->invoker, target, flag, objectName());
        if (flag == "h")
            target->addToShownHandCards(QList<int>() << id);
        else
            target->removeShownHandCards(QList<int>() << id);

        if (target->getPhase() == Player::Play && Sanguosha->getCard(id)->getColor() != card->getColor())
            target->drawCards(1);
        else if (target->getPhase() != Player::Play && Sanguosha->getCard(id)->getColor() == card->getColor())
            target->drawCards(1);
        return false;
    }
};

class Chenjue : public TriggerSkill
{
public:
    Chenjue()
        : TriggerSkill("chenjue")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->isDead() || current->getPhase() != Player::Discard)
            return QList<SkillInvokeDetail>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p->hasSkill(this) && p != current && p->getHandcardNum() < current->getHandcardNum() && !p->isKongcheng())
                d << SkillInvokeDetail(this, p, p);
        }

        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), current->objectName());

        const Card *card1 = room->askForCard(current, ".|.|.|hand,equipped!", "@chenjue:" + invoke->owner->objectName(), data, Card::MethodNone);
        if (card1 == nullptr) {
            // force !!!
            QList<const Card *> hc1 = invoke->invoker->getHandcards();
            int x = qrand() % hc1.length();
            card1 = hc1.value(x);
        }
        const Card *card2 = room->askForCard(invoke->owner, ".|.|.|hand,equipped!", "@chenjue:" + current->objectName(), data, Card::MethodNone);
        if (card2 == nullptr) {
            // force !!!
            QList<const Card *> hc2 = invoke->owner->getHandcards();
            int y = qrand() % hc2.length();
            card2 = hc2.value(y);
        }

        QList<int> ids1;
        ids1 << card1->getId();
        QList<int> ids2;
        ids2 << card2->getId();
        QList<CardsMoveStruct> exchangeMove;
        CardsMoveStruct move1(ids1, invoke->owner, Player::PlaceHand,
                              CardMoveReason(CardMoveReason::S_REASON_SWAP, current->objectName(), invoke->owner->objectName(), objectName(), QString()));
        CardsMoveStruct move2(ids2, current, Player::PlaceHand,
                              CardMoveReason(CardMoveReason::S_REASON_SWAP, invoke->owner->objectName(), current->objectName(), objectName(), QString()));

        exchangeMove.push_back(move1);
        exchangeMove.push_back(move2);
        room->moveCardsAtomic(exchangeMove, false);

        if (invoke->owner->isAlive())
            room->askForUseCard(invoke->owner, "BasicCard+^Jink,EquipCard|.|.|hand", "@chenjue_use", -1, Card::MethodUse, false, objectName());
        if (current->isAlive())
            room->askForUseCard(current, "BasicCard+^Jink,EquipCard|.|.|hand", "@chenjue_use", -1, Card::MethodUse, false, objectName());

        return false;
    }
};

class Xiewu : public TriggerSkill
{
public:
    Xiewu()
        : TriggerSkill("xiewu")
    {
        events << CardUsed << CardResponded << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerFlag(p, "-xiewu");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
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
            if ((player != nullptr) && player->isAlive() && (card != nullptr) && card->getTypeId() == Card::TypeBasic) {
                foreach (ServerPlayer *yuki, room->findPlayersBySkillName(objectName())) {
                    if (!yuki->hasFlag("xiewu") && yuki->getHandcardNum() == player->getHandcardNum())
                        d << SkillInvokeDetail(this, yuki, yuki, player);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *target = invoke->targets.first();
        QStringList select;
        select << "draw";
        if (target->canDiscard(target, "hes"))
            select << "discard";
        select << "cancel";
        QString choice = room->askForChoice(invoke->invoker, objectName(), select.join("+"), data);
        invoke->tag[objectName()] = choice;
        return choice != "cancel";
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->setPlayerFlag(invoke->invoker, "xiewu");
        room->sendLog("#InvokeSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        QString choice = invoke->tag.value(objectName()).toString();

        if (choice == "draw")
            invoke->targets.first()->drawCards(1);
        else
            room->askForDiscard(invoke->targets.first(), objectName(), 1, 1, false, true, "xiewu_discard:" + invoke->invoker->objectName());
        return false;
    }
};

class Anliu : public TriggerSkill
{
public:
    Anliu()
        : TriggerSkill("anliu")
    {
        events << EventPhaseChanging << Damage << Damaged;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->setPlayerFlag(p, "-anliu_used");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e == EventPhaseChanging)
            return QList<SkillInvokeDetail>();

        ServerPlayer *current = room->getCurrent();
        if (current == nullptr || !current->isInMainPhase())
            return QList<SkillInvokeDetail>();

        DamageStruct damage = data.value<DamageStruct>();
        if ((damage.card == nullptr) || !damage.card->isKindOf("Slash") || damage.to->isDead())
            return QList<SkillInvokeDetail>();

        if ((e == Damage && damage.from != nullptr && damage.from->hasSkill(this) && !damage.from->hasFlag("anliu_used"))
            || (e == Damaged && damage.to->hasSkill(this) && !damage.to->hasFlag("anliu_used"))) {
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
            ServerPlayer *meira = (e == Damage) ? damage.from : damage.to;

            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, meira, meira);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        QList<ServerPlayer *> targets;
        DamageStruct damage = data.value<DamageStruct>();
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (damage.to->distanceTo(p) <= 1 && !p->isRemoved())
                targets << p;
        }
        if (targets.isEmpty())
            return false;

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@anliu:" + damage.to->objectName(), true, true);
        if (target != nullptr)
            invoke->targets << target;
        return target != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->setPlayerFlag(invoke->invoker, "anliu_used");
        DamageStruct damage = data.value<DamageStruct>();
        invoke->targets.first()->obtainCard(damage.card);
        invoke->invoker->tag["anliu-target"] = QVariant::fromValue(invoke->targets.first());
        if (!invoke->targets.first()->isKongcheng() && invoke->invoker->askForSkillInvoke(this, data)) {
            int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hs", objectName());
            room->showCard(invoke->targets.first(), id);
            if (Sanguosha->getCard(id)->getTypeId() == Card::TypeBasic)
                room->loseHp(invoke->targets.first(), 1);
        }
        return false;
    }
};

ZhancheCard::ZhancheCard()
{
    will_throw = true;
    handling_method = Card::MethodDiscard;
}

bool ZhancheCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && !to_select->getEquips().isEmpty();
}

void ZhancheCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    const Card *c = room->askForCard(effect.to, ".Equip!", "@zhanche-robbed:" + effect.from->objectName(), QVariant::fromValue<CardEffectStruct>(effect), Card::MethodNone);
    if (c == nullptr) {
        if (!effect.to->getEquips().isEmpty())
            c = effect.to->getEquips().constFirst();
        else {
            // ?????
            Q_UNREACHABLE();
            foreach (const Card *hc, effect.to->getHandcards()) {
                if (hc->isKindOf("EquipCard")) {
                    c = hc;
                    break;
                }
            }
        }
    }

    if (c == nullptr) {
        Q_UNREACHABLE();
        return;
    }

    room->obtainCard(effect.from, c);

    int num = effect.from->getEquips().length();
    foreach (ServerPlayer *p, room->getOtherPlayers(effect.from)) {
        if (p->getEquips().length() > num)
            return;
    }
    LogMessage l;
    l.type = "#zhanche-engine";
    l.from = effect.from;
    l.arg = "zhanche";
    room->sendLog(l);

    //Player.cpp: delete the relation between  this flag and  Player::hasArmorEffect, Player::hasTreasure
    room->setPlayerFlag(effect.from, "zhanche");
}

class ZhancheVS : public OneCardViewAsSkill
{
public:
    ZhancheVS()
        : OneCardViewAsSkill("zhanche")
    {
        filter_pattern = ".!";
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("ZhancheCard");
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        ZhancheCard *c = new ZhancheCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class Zhanche : public TriggerSkill
{
public:
    Zhanche()
        : TriggerSkill("zhanche")
    {
        events << EventPhaseChanging;
        view_as_skill = new ZhancheVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play && change.player->hasFlag("zhanche"))
                room->setPlayerFlag(change.player, "-zhanche");
        }
    }
};

class ZhancheD : public TargetModSkill
{
public:
    ZhancheD()
        : TargetModSkill("#zhanche-distance")
    {
        pattern = ".";
    }

    int getDistanceLimit(const Player *from, const Card *) const override
    {
        if (from->hasFlag("zhanche"))
            return 1000;

        return 0;
    }
};

class HuosuiVS : public ZeroCardViewAsSkill
{
public:
    HuosuiVS()
        : ZeroCardViewAsSkill("huosui")
    {
        response_pattern = "@@huosui";
    }

    const Card *viewAs() const override
    {
        PowerSlash *slash = new PowerSlash(Card::NoSuit, 0);
        slash->setSkillName(objectName());
        return slash;
    }
};

class Huosui : public TriggerSkill
{
public:
    Huosui()
        : TriggerSkill("huosui")
    {
        events << EventPhaseStart << CardsMoveOneTime << TurnStart;
        global = true;
        view_as_skill = new HuosuiVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            bool flag = (move.from != nullptr) && move.from_places.contains(Player::PlaceEquip);
            if (flag) {
                ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
                if (from != nullptr)
                    move.from->setFlags("huosui");
            }

        } else if (triggerEvent == TurnStart) {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                p->setFlags("-huosui");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;

        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if ((current != nullptr) && current->isAlive() && current->getPhase() == Player::Finish) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->isAlive() && p->hasSkill(this) && p->hasFlag("huosui")) {
                        bool flag = false;
                        foreach (ServerPlayer *v, room->getOtherPlayers(p)) {
                            if (p->canSlash(v, false)) {
                                flag = true;
                                break;
                            }
                        }

                        if (flag)
                            r << SkillInvokeDetail(this, p, p);
                    }
                }
            }
        }

        return r;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->askForUseCard(invoke->invoker, "@@huosui", "@huosui-victim");
        return false;
    }
};

class HuosuiTM : public TargetModSkill
{
public:
    HuosuiTM()
        : TargetModSkill("#huosui-dist")
    {
        pattern = "Slash";
    }

    int getDistanceLimit(const Player *, const Card *c) const override
    {
        if (c->getSkillName() == "huosui")
            return 1000;

        return 0;
    }
};

YihuanCard::YihuanCard()
{
    will_throw = true;
}

void YihuanCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->getRoom()->damage(DamageStruct("yihuan", effect.from, effect.to));
}

class Yihuan : public OneCardViewAsSkill
{
public:
    Yihuan()
        : OneCardViewAsSkill("yihuan")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("YihuanCard");
    }

    bool viewFilter(const Card *to_select) const override
    {
        if (Self->isJilei(to_select))
            return false;

        if (to_select->getTypeId() == Card::TypeEquip)
            return false;

        if (to_select->canDamage())
            return false;

        return true;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        YihuanCard *c = new YihuanCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class WuzuiVS : public OneCardViewAsSkill
{
public:
    WuzuiVS()
        : OneCardViewAsSkill("wuzui")
    {
        expand_pile = "guilt";
    }

    bool isEnabledAtPlay(const Player *) const override
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const override
    {
        return pattern.startsWith("@@wuzui-card");
    }

    bool viewFilter(const Card *to_select) const override
    {
        if (Sanguosha->getCurrentCardUsePattern() == "@@wuzui-card1") {
            if (Self->getPile("guilt").contains(to_select->getId()))
                return false;
            bool ok = false;
            int type = Self->property("wuzui").toString().toInt(&ok);
            if (ok && type != 0) {
                return to_select->getTypeId() == static_cast<Card::CardType>(type) && to_select->canDamage();
            }
        } else if (Sanguosha->getCurrentCardUsePattern() == "@@wuzui-card2")
            return Self->getPile("guilt").contains(to_select->getId());

        return false;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        return new DummyCard({originalCard->getId()});
    }
};

class Wuzui : public TriggerSkill
{
public:
    Wuzui()
        : TriggerSkill("wuzui")
    {
        events << EventPhaseChanging << DamageInflicted << DamageCaused;
        view_as_skill = new WuzuiVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers(true)) {
                    if (p->tag.contains("wuzui")) {
                        QStringList l = p->tag.value("wuzui").toStringList();
                        foreach (const QString &ts, l) {
                            ServerPlayer *p = room->findPlayerByObjectName(ts);
                            if (p != nullptr)
                                room->setPlayerProperty(p, "dyingFactor", p->getDyingFactor() - 1);
                        }
                        p->tag.remove("wuzui");
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != nullptr) {
                if (damage.to->isAlive() && damage.to->hasSkill(this))
                    return {SkillInvokeDetail(this, damage.to, damage.to)};
            }
        } else if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from != nullptr && damage.from->isAlive() && damage.from->hasSkill(this) && !damage.from->getPile("guilt").isEmpty())
                return {SkillInvokeDetail(this, damage.from, damage.from, damage.to)};
        }

        return {};
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageInflicted) {
            if (room->askForSkillInvoke(invoke->invoker, this, data)) {
                room->setPlayerProperty(invoke->invoker, "wuzui", QString::number(static_cast<int>(damage.card->getTypeId())));
                const Card *c
                    = room->askForCard(invoke->invoker, "@@wuzui-card1", "@wuzui-put:::" + damage.card->getType(), data, Card::MethodNone, nullptr, false, QString(), false, 1);
                invoke->tag["guilt"] = (c != nullptr);

                if (c != nullptr)
                    invoke->invoker->addToPile("guilt", c->getEffectiveId());

                return true;
            }
        } else if (triggerEvent == DamageCaused) {
            const Card *c
                = room->askForCard(invoke->invoker, "@@wuzui-card2", "@wuzui-discard:" + damage.to->objectName(), data, Card::MethodNone, nullptr, false, QString(), false, 2);
            if (c != nullptr) {
                room->throwCard(c, nullptr);
                return true;
            }
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == DamageInflicted) {
            bool guilt = invoke->tag["guilt"].toBool();
            if (guilt) {
                DamageStruct damage = data.value<DamageStruct>();
                damage.damage -= 1;
                data = QVariant::fromValue<DamageStruct>(damage);

                LogMessage l;
                l.type = "#yaoshi_log";
                l.from = damage.from;
                l.arg = objectName();
                l.arg2 = QString::number(1);

                room->sendLog(l);

                return damage.damage <= 0;
            } else
                invoke->invoker->drawCards(1, "wuzui");
        } else if (triggerEvent == DamageCaused) {
            LogMessage l;
            l.type = "#DeathSickle";
            l.from = invoke->targets.first();
            l.arg = QString::number(invoke->targets.first()->dyingThreshold() + 1);
            l.arg2 = QString::number(1);
            room->sendLog(l);
            QStringList t = invoke->invoker->tag.value("wuzui", QStringList()).toStringList();
            t << invoke->targets.first()->objectName();
            invoke->invoker->tag["wuzui"] = t;
            room->setPlayerProperty(invoke->targets.first(), "dyingFactor", invoke->targets.first()->getDyingFactor() + 1);
        }

        return false;
    }
};

class Yuejie : public TriggerSkill
{
public:
    Yuejie()
        : TriggerSkill("yuejie")
    {
        events << CardsMoveOneTime << EventPhaseStart;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current->getPhase() == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    p->setFlags("-yuejie");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e != CardsMoveOneTime)
            return {};

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.card_ids.length() == 1 && move.to != nullptr && move.to->hasSkill(this) && move.to->isAlive() && !move.to->hasFlag("yuejie")
            && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceDelayedTrick || move.to_place == Player::PlaceEquip)) {
            ServerPlayer *invoker = qobject_cast<ServerPlayer *>(move.to);
            if (invoker != nullptr) {
                bool flag = false;
                int x = invoker->getLostHp() + 1;
                int fromCardNum = 0;
                switch (move.to_place) {
                case Player::PlaceHand:
                    fromCardNum = invoker->getHandcardNum();
                    break;
                case Player::PlaceDelayedTrick:
                    fromCardNum = invoker->getJudgingAreaID().length();
                    break;
                case Player::PlaceEquip:
                    fromCardNum = invoker->getEquips().length();
                    break;
                default:
                    break;
                }

                foreach (ServerPlayer *p, room->getOtherPlayers(invoker)) {
                    int toCardNum = 0;
                    switch (move.to_place) {
                    case Player::PlaceHand:
                        toCardNum = p->getHandcardNum();
                        break;
                    case Player::PlaceDelayedTrick:
                        toCardNum = p->getJudgingAreaID().length();
                        break;
                    case Player::PlaceEquip:
                        toCardNum = p->getEquips().length();
                        break;
                    default:
                        break;
                    }

                    if (abs(fromCardNum - toCardNum) <= x) {
                        flag = true;
                        break;
                    }
                }

                if (flag)
                    return {SkillInvokeDetail(this, invoker, invoker)};
            }
        }

        return {};
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<ServerPlayer *> targets;
        int x = invoke->invoker->getLostHp() + 1;
        int fromCardNum = 0;
        switch (move.to_place) {
        case Player::PlaceHand:
            fromCardNum = invoke->invoker->getHandcardNum();
            break;
        case Player::PlaceDelayedTrick:
            fromCardNum = invoke->invoker->getJudgingAreaID().length();
            break;
        case Player::PlaceEquip:
            fromCardNum = invoke->invoker->getEquips().length();
            break;
        default:
            break;
        }

        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            int toCardNum = 0;
            switch (move.to_place) {
            case Player::PlaceHand:
                fromCardNum = invoke->invoker->getHandcardNum();
                toCardNum = p->getHandcardNum();
                break;
            case Player::PlaceDelayedTrick:
                fromCardNum = invoke->invoker->getJudgingAreaID().length();
                toCardNum = p->getJudgingAreaID().length();
                break;
            case Player::PlaceEquip:
                fromCardNum = invoke->invoker->getEquips().length();
                toCardNum = p->getEquips().length();
                break;
            default:
                break;
            }

            if (abs(fromCardNum - toCardNum) <= x)
                targets << p;
        }

        static const QMap<Player::Place, QString> promptMap {
            std::make_pair(Player::PlaceHand, "PlaceHand"),
            std::make_pair(Player::PlaceDelayedTrick, "PlaceDelayedTrick"),
            std::make_pair(Player::PlaceEquip, "PlaceEquip"),
        };

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@yuejie-select:::" + promptMap.value(move.to_place), true, true);
        if (target != nullptr)
            invoke->targets << target;

        return target != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *from = invoke->invoker;
        ServerPlayer *to = invoke->targets.first();
        from->setFlags("yuejie");

        CardsMoveStruct move1;
        CardsMoveStruct move2;

        switch (move.to_place) {
        case Player::PlaceHand:
            move1.card_ids = from->handCards();
            move2.card_ids = to->handCards();
            break;
        case Player::PlaceDelayedTrick:
            move1.card_ids = from->getJudgingAreaID();
            move2.card_ids = to->getJudgingAreaID();
            break;
        case Player::PlaceEquip:
            foreach (const Card *equip, from->getEquips())
                move1.card_ids << equip->getId();
            foreach (const Card *equip, to->getEquips())
                move2.card_ids << equip->getId();
            break;
        default:
            break;
        }

        move1.to = to;
        move2.to = from;
        move1.to_place = move2.to_place = move.to_place;
        move1.reason = CardMoveReason(CardMoveReason::S_REASON_SWAP, from->objectName(), to->objectName(), objectName(), QString());
        move2.reason = CardMoveReason(CardMoveReason::S_REASON_SWAP, from->objectName(), from->objectName(), objectName(), QString());
        room->moveCardsAtomic({move1, move2}, false);
        return false;
    }
};

class Kongbai : public TriggerSkill
{
public:
    Kongbai()
        : TriggerSkill("kongbai")
    {
        events << EventPhaseChanging << TurnStart << CardsMoveOneTime;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);

            if ((from != nullptr) && from->isAlive() && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))) {
                foreach (int id, move.card_ids) {
                    if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand || move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceEquip)
                        from->addMark(objectName(), 1);
                }
            }
        }
        if (triggerEvent == TurnStart) {
            foreach (ServerPlayer *p, room->getAllPlayers(true))
                p->setMark(objectName(), 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && change.player != nullptr && !change.player->tag.value("touhou-extra", false).toBool()) {
                int lose_num = 0;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getMark(objectName()) > lose_num)
                        lose_num = p->getMark(objectName());
                }
                ServerPlayer *target = nullptr;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getMark(objectName()) == lose_num) {
                        if (target == nullptr)
                            target = p;
                        else
                            return {};
                    }
                }

                if (target != nullptr && lose_num > 0) {
                    QList<SkillInvokeDetail> d;
                    foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                        if (p->getMark(objectName()) > 0)
                            d << SkillInvokeDetail(this, p, p, nullptr, false, target);
                    }
                    return d;
                }
            }
        }

        return {};
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (!room->getThread()->hasExtraTurn()) {
            QList<ServerPlayer *> logto;
            logto << invoke->targets.first();
            room->sendLog("#touhouExtraTurn", invoke->targets.first(), nullptr, logto);
            invoke->targets.first()->gainAnExtraTurn();
            if (invoke->targets.first() != invoke->invoker)
                room->loseHp(invoke->invoker, 1);
        } else {
            LogMessage log;
            log.type = "#ForbidExtraTurn";
            log.from = invoke->invoker;

            room->sendLog(log);
        }
        return false;
    }
};

class Moou : public MasochismSkill
{
public:
    Moou()
        : MasochismSkill("moou")
    {
    }

    QList<SkillInvokeDetail> triggerable(const Room *room, const DamageStruct &damage) const override
    {
        bool invoke = false;
        do {
            if (damage.to->isAlive() && damage.to->hasSkill(this)) {
                bool flag = false;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (damage.to->canDiscard(p, "hes")) {
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
            for (int i = 0; i < damage.damage; ++i)
                d << SkillInvokeDetail(this, damage.to, damage.to);

            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<ServerPlayer *> fieldcard;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (damage.to->canDiscard(p, "hes")) {
                fieldcard << p;
            }
        }
        if (fieldcard.isEmpty())
            return false;

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, fieldcard, objectName(), "@moou_target", true, true);
        if (target != nullptr) {
            invoke->targets << target;
            return true;
        }

        return false;
    }

    void onDamaged(Room *room, QSharedPointer<SkillInvokeDetail> invoke, const DamageStruct &) const override
    {
        ServerPlayer *target = invoke->targets.first();
        QList<int> disable;
        int card_id = -1;
        DummyCard *dummy = new DummyCard;

        foreach (const Card *c, target->getCards("hes")) {
            if (!invoke->invoker->canDiscard(target, c->getEffectiveId()))
                disable << c->getEffectiveId();
        }

        for (int i = 0; i < 2; i++) {
            if (invoke->invoker->isDead() || !invoke->invoker->canDiscard(target, "hes"))
                break;

            if (i != 0 && !invoke->invoker->askForSkillInvoke(this, "continue:" + target->objectName() + "::" + QString::number(i + 1)))
                break;

            bool visible = (target == invoke->invoker);
            card_id = room->askForCardChosen(invoke->invoker, target, "hes", objectName(), visible, Card::MethodDiscard, disable);

            disable << card_id;
            dummy->addSubcard(card_id);

            if (target->getCards("hes").length() - disable.length() <= 0)
                break;
        }
        int dummy_num = dummy->getSubcards().length();
        if (dummy_num > 0) {
            room->throwCard(dummy, target, invoke->invoker);
            target->drawCards(dummy_num);
        }

        delete dummy;
    }
};

class Mowei : public TriggerSkill
{
public:
    Mowei()
        : TriggerSkill("mowei")
    {
        events << TargetConfirming;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from == nullptr || !use.card->canDamage() || use.to.length() != 1)
            return QList<SkillInvokeDetail>();

        ServerPlayer *target = use.to.first();
        if (target->isKongcheng() || target == use.from)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p == use.from || p == target)
                continue;
            if (p->distanceTo(target) == 1 || p->getKingdom() == target->getKingdom())
                d << SkillInvokeDetail(this, p, p);
        }

        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "invoke:" + use.from->objectName() + ":" + use.to.first()->objectName() + ":" + use.card->objectName();
        if (invoke->invoker->askForSkillInvoke(this, prompt)) {
            int id = room->askForCardChosen(invoke->invoker, use.to.first(), "hs", objectName());
            room->obtainCard(invoke->invoker, id, false);
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();

        ServerPlayer *target = use.to.first();
        use.to.removeAll(target);

        use.to << invoke->invoker;
        room->sortByActionOrder(use.to);
        data = QVariant::fromValue(use);

        QList<ServerPlayer *> logto;
        logto << target;
        room->sendLog("$CancelTarget", use.from, use.card->objectName(), logto);
        logto << invoke->invoker;
        logto.removeOne(target);
        room->sendLog("#mowei_change", use.from, use.card->objectName(), logto);

        const Card *card = room->askForExchange(invoke->invoker, "mowei", 1, 1, false, "moweiReturn:" + target->objectName(), true);
        if (card != nullptr) {
            DELETE_OVER_SCOPE(const Card, card)
            target->obtainCard(card, false);
        }
        return false;
    }
};

class Menshao : public TriggerSkill
{
public:
    Menshao()
        : TriggerSkill("menshao")
    {
        events << PostCardEffected << CardEffected << TurnStart;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            //igonre insertion like muplty carduse events
            if (effect.from != nullptr && !effect.to->hasFlag("menshao_used") && effect.card->isBlack() && (effect.card->isKindOf("Slash") || effect.card->isNDTrick())) {
                effect.to->tag["menshao_hp"] = QVariant::fromValue(effect.to->getHp());
                effect.to->tag["menshao_handcard"] = QVariant::fromValue(effect.to->getHandcardNum());
            }
        }
        if (triggerEvent == TurnStart) {
            foreach (ServerPlayer *p, room->getAllPlayers(true)) {
                room->setPlayerFlag(p, "-menshao_used");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent != PostCardEffected)
            return QList<SkillInvokeDetail>();

        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.card->isBlack() && (effect.card->isKindOf("Slash") || effect.card->isNDTrick())) {
            if ((effect.from != nullptr) && effect.to->hasSkill(this) && !effect.to->hasFlag("menshao_used")) {
                int handacardnum = effect.to->tag["menshao_handcard"].toInt();
                int hp = effect.to->tag["menshao_hp"].toInt();
                if ((effect.to->getHandcardNum() == handacardnum && effect.to->isWounded()) || effect.to->getHp() == hp)
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        int handacardnum = effect.to->tag["menshao_handcard"].toInt();
        int hp = effect.to->tag["menshao_hp"].toInt();

        QStringList select;
        if (effect.to->getHandcardNum() == handacardnum && effect.to->isWounded())
            select << "recover";
        if (effect.to->getHp() == hp)
            select << "draw";
        select << "cancel";

        invoke->tag["menshao_choices"] = QVariant::fromValue(select);
        QString choice = room->askForChoice(invoke->invoker, objectName(), select.join("+"), data);
        invoke->tag["menshao_choice"] = QVariant::fromValue(choice);
        return (choice != "cancel");
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->setPlayerFlag(invoke->invoker, "menshao_used");
        room->sendLog("#InvokeSkill", invoke->invoker, objectName());
        QStringList select = invoke->tag["menshao_choices"].toStringList();
        QString choice = invoke->tag["menshao_choice"].toString();
        //effect1
        if (choice == "recover") {
            RecoverStruct recover;
            recover.who = invoke->invoker;
            room->recover(invoke->invoker, recover);
        } else
            invoke->invoker->drawCards(1);

        select.removeOne(choice);
        if (!invoke->invoker->isWounded())
            select.removeOne("recover");

        //effect2
        if (select.length() > 1) {
            QString choice = room->askForChoice(invoke->invoker, objectName(), select.join("+"), data);
            if (choice == "recover") {
                RecoverStruct recover;
                recover.who = invoke->invoker;
                room->recover(invoke->invoker, recover);
            } else if (choice == "draw")
                invoke->invoker->drawCards(1);
            else
                return false;

            //extra effect
            QList<ServerPlayer *> fieldcard;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (invoke->invoker->canDiscard(p, "ej"))
                    fieldcard << p;
            }
            if (fieldcard.isEmpty())
                return false;
            ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, fieldcard, objectName(), "@menshao-discardfield");
            int cardid = room->askForCardChosen(invoke->invoker, target, "ej", objectName(), false, Card::MethodDiscard);
            room->throwCard(cardid, target, invoke->invoker);
        }

        return false;
    }
};

TH0105Package::TH0105Package()
    : Package("th0105")
{
    General *mima = new General(this, "mima$", "pc98", 4);
    mima->addSkill(new Meiling);
    mima->addSkill(new Fuchou);

    General *yumemi = new General(this, "yumemi$", "pc98", 4);
    yumemi->addSkill(new Ciyuan);
    yumemi->addSkill(new Shigui);
    yumemi->addSkill(new Chongdong);

    General *chiyuri = new General(this, "chiyuri", "pc98", 4);
    chiyuri->addSkill(new Zhence);
    chiyuri->addSkill(new Shiqu);

    General *rikako = new General(this, "rikako", "pc98", 4);
    rikako->addSkill(new Zhenli);
    rikako->addSkill(new Qiusuo);

    General *kana = new General(this, "kana", "pc98", 3);
    kana->addSkill(new Mengxiao);
    kana->addSkill(new Lubiao);

    General *yuka_old = new General(this, "yuka_old$", "pc98", 4);
    yuka_old->addSkill(new Yeyan);
    yuka_old->addSkill(new Youyue);
    yuka_old->addSkill(new Menghuan);

    General *gengetsumugetsu = new General(this, "gengetsumugetsu", "pc98", 3);
    gengetsumugetsu->addSkill(new Huantong);
    gengetsumugetsu->addSkill(new Mengyan);

    General *elly = new General(this, "elly", "pc98", 4);
    elly->addSkill(new Lianmu);
    elly->addSkill(new LianmuTargetMod);
    elly->addSkill(new HuanweiEffect);
    elly->addSkill(new Huanwei);
    related_skills.insertMulti("lianmu", "#lianmu_mod");
    related_skills.insertMulti("huanwei", "#huanwei");

    General *shinki = new General(this, "shinki$", "pc98", 4);
    shinki->addSkill(new SqChuangshi);
    shinki->addSkill(new Yuanfa);
    shinki->addSkill(new Shenwei);

    General *alice_old = new General(this, "alice_old", "pc98", 3);
    alice_old->addSkill(new Modian);
    alice_old->addSkill(new Guaiqi);

    General *sariel = new General(this, "sariel", "pc98", 4);
    sariel->addSkill(new Baosi);
    sariel->addSkill(new Moyan);

    General *konngara = new General(this, "konngara", "pc98");
    konngara->addSkill(new Zongjiu);
    konngara->addSkill(new Xingyou);

    General *yumeko = new General(this, "yumeko", "pc98", 4);
    yumeko->addSkill(new Huanshu);
    yumeko->addSkill(new Qiren);

    General *yukimai = new General(this, "yukimai", "pc98", 3);
    yukimai->addSkill(new Xiewu);
    yukimai->addSkill(new Luli);

    General *meira = new General(this, "meira", "pc98", 4);
    meira->addSkill(new Anliu);

    General *rika = new General(this, "rika", "pc98");
    rika->addSkill(new Zhanche);
    rika->addSkill(new ZhancheD);
    rika->addSkill(new Huosui);
    rika->addSkill(new HuosuiTM);
    related_skills.insertMulti("zhanche", "#zhanche-distance");
    related_skills.insertMulti("huosui", "#huosui-dist");

    General *elis = new General(this, "elis", "pc98", 3);
    elis->addSkill(new Yihuan);
    elis->addSkill(new Wuzui);

    General *luize = new General(this, "luize", "pc98");
    luize->addSkill(new Yuejie);

    General *ellen = new General(this, "ellen", "pc98", 3);
    ellen->addSkill(new Kongbai);
    ellen->addSkill(new Moou);

    General *sara = new General(this, "sara", "pc98", 3);
    sara->addSkill(new Mowei);
    sara->addSkill(new Menshao);

    addMetaObject<ShiquCard>();
    addMetaObject<LianmuCard>();
    addMetaObject<SqChuangshiCard>();
    addMetaObject<ModianCard>();
    addMetaObject<BaosiCard>();
    addMetaObject<MoyanCard>();
    addMetaObject<QirenCard>();
    addMetaObject<LuliCard>();
    addMetaObject<ZhancheCard>();
    addMetaObject<YihuanCard>();

    skills << new ModianVS;
}

ADD_PACKAGE(TH0105)
