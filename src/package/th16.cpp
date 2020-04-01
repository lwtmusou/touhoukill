#include "th16.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "skill.h"

MenfeiCard::MenfeiCard()
{
}

bool MenfeiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void MenfeiCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->gainMark("@door");
}

class MenfeiVS : public ZeroCardViewAsSkill
{
public:
    MenfeiVS()
        : ZeroCardViewAsSkill("menfei")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark("@door") > 0)
            return false;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->getMark("@door") > 0)
                return false;
        }
        return true;
    }

    virtual const Card *viewAs() const
    {
        return new MenfeiCard;
    }
};

class Menfei : public TriggerSkill
{
public:
    Menfei()
        : TriggerSkill("menfei")
    {
        events << CardFinished;
        view_as_skill = new MenfeiVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeSkill)
            return QList<SkillInvokeDetail>();

        ServerPlayer *player = use.from;

        if (player && player->isAlive() && player->hasSkill(this)) {
            ServerPlayer *target = NULL;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getMark("@door") > 0) {
                    target = p;
                    break;
                }
            }
            if (target) {
                ServerPlayer *next = qobject_cast<ServerPlayer *>(target->getNextAlive(1));
                if (next && next != target) {
                    SkillInvokeDetail r(this, player, player, NULL, true);
                    r.tag["door"] = QVariant::fromValue(next);
                    return QList<SkillInvokeDetail>() << r;
                }
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->getMark("@door") > 0) {
                room->setPlayerMark(p, "@door", 0);
                break;
            }
        }
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        ServerPlayer *next = invoke->tag["door"].value<ServerPlayer *>();
        if (next)
            next->gainMark("@door");
        return false;
    }
};

class Houhu : public TriggerSkill
{
public:
    Houhu()
        : TriggerSkill("houhu")
    {
        events << TargetSpecifying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeBasic || use.card->isNDTrick()) {
            if (use.card->isKindOf("Jink") || use.card->isKindOf("Nullification"))
                return QList<SkillInvokeDetail>();
            ServerPlayer *player = use.from;
            if (player && player->isAlive() && player->hasSkill(this)) {
                ServerPlayer *target = NULL;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getMark("@door") > 0) {
                        target = p;
                        break;
                    }
                }
                if (target) {
                    if (use.to.contains(target)) {
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, false, target);
                    } else if (!player->isProhibited(target, use.card)) {
                        use.card->setFlags("IgnoreFailed");
                        use.card->setFlags("houhu");
                        bool can = use.card->targetFilter(QList<const Player *>(), target, player);
                        if (use.card->isKindOf("Peach") && target->isWounded())
                            can = true;
                        use.card->setFlags("-houhu");
                        use.card->setFlags("-IgnoreFailed");
                        if (can)
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, false, target);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.contains(invoke->targets.first())) {
            invoke->invoker->drawCards(1);
        } else {
            CardUseStruct use = data.value<CardUseStruct>();
            QList<ServerPlayer *> logto;
            logto << invoke->targets.first();
            room->touhouLogmessage("#houhu", invoke->invoker, use.card->objectName(), logto, objectName());

            use.to << invoke->targets.first();
            room->sortByActionOrder(use.to);
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

class HouhuDistance : public TargetModSkill
{
public:
    HouhuDistance()
        : TargetModSkill("houhu-dist")
    {
        pattern = "BasicCard,TrickCard";
    }

    int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->hasFlag("houhu"))
            return 1000;

        return 0;
    }
};

class Xunfo : public TriggerSkill
{
public:
    Xunfo()
        : TriggerSkill("xunfo")
    {
        events << CardsMoveOneTime << HpRecover;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        bool triggerFlag = false;

        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DRAW) && move.reason.m_extraData.toString() != "xunfo"
                && move.reason.m_extraData.toString() != "initialDraw" && move.to->isLord())
                triggerFlag = true;
        } else {
            RecoverStruct recover = data.value<RecoverStruct>();
            if (recover.to->isLord())
                triggerFlag = true;
        }

        QList<SkillInvokeDetail> r;

        if (!triggerFlag)
            return r;

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->isAlive() && p->hasSkill(this))
                r << SkillInvokeDetail(this, p, p, NULL, true);
        }

        return r;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1, "xunfo");
        return false;
    }
};

namespace HuyuanNs {
bool cardAvailable(const Card *c)
{
    switch (c->getSuit()) {
    case Card::Spade:
    case Card::Heart:
    case Card::Club:
    case Card::Diamond:
        return true;
        break;
    default:
        return false;
    }

    return false;
}
}

class HuyuanDis : public ViewAsSkill
{
public:
    HuyuanDis()
        : ViewAsSkill("huyuandis")
    {
        response_pattern = "@@huyuandis";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        QStringList ls = Self->property("huyuansuits").toString().split(",");
        foreach (const Card *c, selected) {
            if (HuyuanNs::cardAvailable(c))
                ls.removeAll(c->getSuitString());
            else
                return false;
        }

        if (ls.length() == 0)
            return false;

        return HuyuanNs::cardAvailable(to_select) && ls.contains(to_select->getSuitString());
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        QStringList ls = Self->property("huyuansuits").toString().split(",");
        foreach (const Card *c, cards) {
            if (HuyuanNs::cardAvailable(c))
                ls.removeAll(c->getSuitString());
            else
                return NULL;
        }

        if (ls.length() == 0) {
            DummyCard *card = new DummyCard;
            card->addSubcards(cards);
            return card;
        }

        return NULL;
    }
};

HuyuanCard::HuyuanCard()
{
    handling_method = MethodNone;
    will_throw = false;
}

void HuyuanCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    QStringList suits;

    foreach (int id, getSubcards()) {
        room->showCard(effect.from, id);
        const Card *c = Sanguosha->getCard(id);
        if (HuyuanNs::cardAvailable(c))
            suits << c->getSuitString();
    }

    room->setPlayerProperty(effect.to, "huyuansuits", suits.join(","));
    bool discarded = false;
    try {
        QString prompt;
        if (suits.length() == 1)
            prompt = QString("@huyuandis1:") + effect.from->objectName() + QString("::") + suits.first();
        else if (suits.length() == 2)
            prompt = QString("@huyuandis2:") + effect.from->objectName() + QString("::") + suits.first() + QString(":") + suits.last();
        else if (suits.length() == 3)
            prompt = (QString("@huyuandis3%1:") + effect.from->objectName() + QString("::%2:%3")).arg(suits.first()).arg(suits.last()).arg(suits.at(1));

        discarded = room->askForCard(effect.to, "@@huyuandis", prompt, suits);
    } catch (TriggerEvent event) {
        if (event == TurnBroken)
            room->setPlayerProperty(effect.to, "huyuansuits", QVariant());

        throw event;
    }

    room->setPlayerProperty(effect.to, "huyuansuits", QVariant());

    if (discarded)
        room->recover(effect.to, RecoverStruct());
    else {
        room->loseHp(effect.to);
        if (effect.to->isAlive())
            effect.to->drawCards(getSubcards().length(), "huyuan");
    }
}

class Huyuan : public ViewAsSkill
{
public:
    Huyuan()
        : ViewAsSkill("huyuan")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("HuyuanCard");
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() >= 3)
            return false;
        if (!HuyuanNs::cardAvailable(to_select))
            return false;
        foreach (const Card *card, selected) {
            if (card->getSuit() == to_select->getSuit())
                return false;
        }

        return true;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 0)
            return NULL;

        QSet<Card::Suit> suits;
        foreach (const Card *card, cards) {
            if (!HuyuanNs::cardAvailable(card))
                return NULL;

            suits.insert(card->getSuit());
        }

        if (suits.size() == cards.length()) {
            HuyuanCard *hy = new HuyuanCard;
            hy->addSubcards(cards);
            return hy;
        }

        return NULL;
    }
};

class Puti : public TriggerSkill
{
public:
    Puti()
        : TriggerSkill("puti")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                if (change.player->isCardLimited("use", "puti"))
                    room->removePlayerCardLimitation(change.player, "use", "Slash$1", "puti");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (!player->hasSkill(this) || player->isDead() || player->getPhase() != Player::Play)
                return QList<SkillInvokeDetail>();
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        int acquired = 0;
        QList<int> throwIds;
        while (acquired < 1) {
            int id = room->drawCard();
            CardsMoveStruct move(id, NULL, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName()));
            move.reason.m_skillName = "puti";
            room->moveCardsAtomic(move, true);
            room->getThread()->delay();
            Card *card = Sanguosha->getCard(id);
            if (card->getTypeId() == Card::TypeTrick) {
                acquired = acquired + 1;
                CardsMoveStruct move2(id, player, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_GOTBACK, player->objectName()));
                room->moveCardsAtomic(move2, true);

                room->setPlayerCardLimitation(player, "use", "Slash", "puti", true);
                if (!throwIds.isEmpty()) {
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "sishu", QString());
                    DummyCard dummy(throwIds);
                    room->throwCard(&dummy, reason, NULL);
                    throwIds.clear();
                }
            } else
                throwIds << id;
        }

        return false;
    }
};

class Zangfa : public TriggerSkill
{
public:
    Zangfa()
        : TriggerSkill("zangfa")
    {
        events << TargetSpecifying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isNDTrick() || use.card->isKindOf("Nullification"))
            return QList<SkillInvokeDetail>();

        bool invoke = false;
        use.card->setFlags("IgnoreFailed");
        use.card->setFlags("zangfa");
        foreach (ServerPlayer *q, room->getAlivePlayers()) {
            if (!use.to.contains(q) && !use.from->isProhibited(q, use.card) && use.card->targetFilter(QList<const Player *>(), q, use.from)) {
                invoke = true;
                break;
            }
        }
        use.card->setFlags("-zangfa");
        use.card->setFlags("-IgnoreFailed");
        if (!invoke)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        QList<ServerPlayer *> invokers = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *p, invokers) {
            if (p->isAlive() && (p == use.from || use.to.contains(p)))
                d << SkillInvokeDetail(this, p, p);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QList<ServerPlayer *> listt;
        CardUseStruct use = data.value<CardUseStruct>();
        use.card->setFlags("IgnoreFailed");
        use.card->setFlags("zangfa");
        foreach (ServerPlayer *q, room->getAlivePlayers()) {
            if (!use.to.contains(q) && !use.from->isProhibited(q, use.card) && use.card->targetFilter(QList<const Player *>(), q, use.from))
                listt << q;
        }
        use.card->setFlags("-zangfa");
        use.card->setFlags("-IgnoreFailed");
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, listt, objectName(), "@zangfa:" + use.card->objectName(), true, true);
        //player->tag.remove("huanshi_source");
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

class ZangfaDistance : public TargetModSkill
{
public:
    ZangfaDistance()
        : TargetModSkill("zangfa-dist")
    {
        pattern = "TrickCard";
    }

    int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->hasFlag("zangfa"))
            return 1000;

        return 0;
    }
};

TH16Package::TH16Package()
    : Package("th16")
{
    General *okina = new General(this, "okina$", "tkz");
    okina->addSkill(new Menfei);
    okina->addSkill(new Houhu);

    General *eternity = new General(this, "eternity", "tkz", 4, false, true);
    Q_UNUSED(eternity);

    General *nemuno = new General(this, "nemuno", "tkz", 4, false, true);
    Q_UNUSED(nemuno);

    General *aun = new General(this, "aun", "tkz", 3);
    aun->addSkill(new Xunfo);
    aun->addSkill(new Huyuan);

    General *narumi = new General(this, "narumi", "tkz");
    narumi->addSkill(new Puti);
    narumi->addSkill(new Zangfa);

    General *satono = new General(this, "satono", "tkz", 4, false, true);
    Q_UNUSED(satono);

    General *mai = new General(this, "mai", "tkz", 4, false, true);
    Q_UNUSED(mai);

    addMetaObject<MenfeiCard>();
    addMetaObject<HuyuanCard>();
    skills << new HouhuDistance << new ZangfaDistance << new HuyuanDis;
}

ADD_PACKAGE(TH16)
