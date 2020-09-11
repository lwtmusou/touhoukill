#include "th16.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "skill.h"
#include "testCard.h"

// couple: LureTigerProhibit

/*
class MiyiVS : public OneCardViewAsSkill
{
public:
    MiyiVS()
        : OneCardViewAsSkill("miyi")
    {
        response_or_use = true;
    }

    bool viewFilter(const Card *to_select) const
    {
        QList<const Player *> ps = Self->getAliveSiblings();
        ps << Self;

        foreach (const Player *p, ps) {
            QList<const Card *> pc = p->getEquips() + p->getJudgingArea();
            foreach (const Card *c, pc) {
                if (c->getSuit() == to_select->getSuit() || ((c->getSuitString().startsWith("no_suit") && (to_select->getSuitString().startsWith("no_suit")))))
                    return true;
            }
        }

        return false;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        LureTiger *lt = new LureTiger(Card::SuitToBeDecided, -1);
        lt->addSubcard(originalCard);
        lt->setSkillName("miyi");
        return lt;
    }
};

class MiyiTargetMod : public TargetModSkill
{
public:
    MiyiTargetMod()
        : TargetModSkill("#miyi-tiger")
    {
        pattern = "LureTiger";
    }

    int getExtraTargetNum(const Player *from, const Card *card) const
    {
        if (from != NULL && from->hasSkill("miyi") && card->getSkillName() == "miyi")
            return 1;

        return 0;
    }
};

class Miyi : public TriggerSkill
{
public:
    Miyi()
        : TriggerSkill("miyi")
    {
        events << SlashHit << TrickEffect << CardFinished << PreCardUsed;
        view_as_skill = new MiyiVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == "miyi")
                room->setPlayerFlag(use.from, "miyiused");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent != PreCardUsed) {
            if (triggerEvent != CardFinished) {
                ServerPlayer *from = NULL;
                ServerPlayer *to = NULL;
                const Card *card = NULL;
                if (triggerEvent == SlashHit) {
                    SlashEffectStruct effect = data.value<SlashEffectStruct>();
                    from = effect.from;
                    to = effect.to;
                    card = effect.slash;
                } else {
                    CardEffectStruct effect = data.value<CardEffectStruct>();
                    from = effect.from;
                    to = effect.to;
                    card = effect.card;
                }

                if (from != NULL && from->hasSkill(this) && from->isAlive()) {
                    if (to->isRemoved()) {
                        SkillInvokeDetail d(this, from, from, to, true);
                        d.tag["miyi"] = QVariant::fromValue(card);
                        return QList<SkillInvokeDetail>() << d;
                    }
                }
            } else {
                CardUseStruct use = data.value<CardUseStruct>();
                QList<SkillInvokeDetail> r;
                if (use.from != NULL && use.from->tag.contains("miyi_" + use.card->toString())) {
                    QStringList miyitag = use.from->tag["miyi_" + use.card->toString()].toStringList();
                    foreach (const QString &miyivicv, miyitag) {
                        ServerPlayer *miyivic = room->findPlayerByObjectName(miyivicv, true);
                        r << SkillInvokeDetail(this, use.from, use.from, NULL, true, miyivic);
                    }
                }
                return r;
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();

            QStringList miyitag = invoke->invoker->tag["miyi_" + use.card->toString()].toStringList();
            miyitag.removeAll(invoke->preferredTarget->objectName());
            invoke->invoker->tag["miyi_" + use.card->toString()] = miyitag;
        }

        return true;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent != CardFinished) {
            invoke->invoker->tag["miyi-instant"] = QVariant::fromValue(invoke->targets.first());
            bool choice = room->askForSkillInvoke(invoke->invoker, "#miyi-instant", QString("mya:") + invoke->targets.first()->objectName());
            invoke->invoker->tag.remove("miyi-instant");
            if (choice) {
                room->setPlayerProperty(invoke->targets.first(), "removed", false);
                room->removePlayerCardLimitation(invoke->targets.first(), "use", ".$1", "lure_tiger");
            } else {
                const Card *c = invoke->tag["miyi"].value<const Card *>();
                QString miyitagname = "miyi_" + c->toString();
                QStringList l;
                if (invoke->invoker->tag.contains(miyitagname))
                    l = invoke->invoker->tag[miyitagname].toStringList();

                l << invoke->targets.first()->objectName();
                invoke->invoker->tag[miyitagname] = l;
            }
        } else {
            room->setPlayerProperty(invoke->targets.first(), "removed", false);
            room->removePlayerCardLimitation(invoke->targets.first(), "use", ".$1", "lure_tiger");
        }

        return false;
    }
};

class MiyiBasicExceptSlash : public TriggerSkill
{
public:
    MiyiBasicExceptSlash()
        : TriggerSkill("#miyi-basic")
    {
        events << CardEffected;
    }

    int getPriority() const
    {
        return 1;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        QList<SkillInvokeDetail> r;
        if (e == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.from != NULL && effect.from->hasSkill("miyi") && effect.from->isAlive()) {
                if (effect.card->getTypeId() == Card::TypeBasic && !effect.card->isKindOf("Slash") && effect.to->isRemoved()) {
                    SkillInvokeDetail d(Sanguosha->getTriggerSkill("miyi"), effect.from, effect.from, effect.to, true);
                    d.tag["miyi"] = QVariant::fromValue(effect.card);
                    r << d;
                }
            }
        }
        return r;
    }
};


class Zhuzhe : public TriggerSkill
{
public:
    Zhuzhe()
        : TriggerSkill("zhuzhe$")
    {
        events << TargetSpecifying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> r;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card == NULL || use.from == NULL)
            return r;

        if (use.from->getKingdom() != "tkz")
            return r;

        bool flag = false;
        if (use.card->isKindOf("ExNihilo") || use.card->isKindOf("AwaitExhausted"))
            flag = true;
        else if (use.card->getSkillName() == "xianshi") {
            QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
            foreach (const Card *card, cards) {
                if (card->isNDTrick() || card->isKindOf("BasicCard")) {
                    if (use.card->hasFlag("xianshi_" + card->objectName())) {
                        if (card->isKindOf("ExNihilo") || card->isKindOf("AwaitExhausted"))
                            flag = true;

                        break;
                    }
                }
            }
        }

        if (flag) {
            foreach (ServerPlayer *p, room->getOtherPlayers(use.from)) {
                if (p->hasLordSkill(this) && p->isAlive())
                    r << SkillInvokeDetail(this, p, use.from, p);
            }
        }

        return r;
    }

    bool effect(TriggerEvent, Room *r, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.to << invoke->owner;
        r->sortByActionOrder(use.to);
        data = QVariant::fromValue(use);
        return false;
    }
};

*/

/*
class Zhuzhe : public TriggerSkill
{
public:
    Zhuzhe()
        : TriggerSkill("zhuzhe$")
    {
        events << TargetSpecifying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> r;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card == NULL || use.from == NULL)
            return r;

        if (use.from->getKingdom() != "tkz")
            return r;

        bool flag = false;
        if (use.card->isKindOf("ExNihilo") || use.card->isKindOf("AwaitExhausted"))
            flag = true;
        else if (use.card->getSkillName() == "xianshi") {
            QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
            foreach (const Card *card, cards) {
                if (card->isNDTrick() || card->isKindOf("BasicCard")) {
                    if (use.card->hasFlag("xianshi_" + card->objectName())) {
                        if (card->isKindOf("ExNihilo") || card->isKindOf("AwaitExhausted"))
                            flag = true;

                        break;
                    }
                }
            }
        }

        if (flag) {
            foreach (ServerPlayer *p, room->getOtherPlayers(use.from)) {
                if (p->hasLordSkill(this) && p->isAlive())
                    r << SkillInvokeDetail(this, p, use.from, p);
            }
        }

        return r;
    }

    bool effect(TriggerEvent, Room *r, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.to << invoke->owner;
        r->sortByActionOrder(use.to);
        data = QVariant::fromValue(use);
        return false;
    }
};
*/

class MiyiVS : public ZeroCardViewAsSkill
{
public:
    MiyiVS()
        : ZeroCardViewAsSkill("miyi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->hasFlag("miyi") && !player->hasFlag("miyi_used");
    }

    virtual const Card *viewAs() const
    {
        LureTiger *c = new LureTiger(Card::NoSuit, 0);
        c->setSkillName(objectName());
        return c;
    }
};

class Miyi : public TriggerSkill
{
public:
    Miyi()
        : TriggerSkill("miyi")
    {
        events << SlashHit << TrickEffect << CardFinished << PreCardUsed;
        view_as_skill = new MiyiVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill && use.from && use.from->isCurrent() && use.from->getPhase() == Player::Play) {
                if (!use.from->hasFlag("miyi"))
                    room->setPlayerFlag(use.from, "miyi");
            }
            if (use.card->getSkillName() == objectName()) {
                if (!use.from->hasFlag("miyi_used"))
                    room->setPlayerFlag(use.from, "miyi_used");
            }

        }
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                foreach(ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("miyi"))
                        room->setPlayerFlag(p, "-miyi");
                    if (p->hasFlag("miyi_used"))
                        room->setPlayerFlag(p, "-miyi_used");
                }
            }
        }
    }
};


ZhaoweiCard::ZhaoweiCard()
{
}

bool ZhaoweiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *self) const
{
    if (self->getMark("zhaowei_mod") == 1) {
        return targets.isEmpty() && to_select != self && !to_select->isNude();
    }
    if (self->getMark("zhaowei_mod") == 2) {
        int num = self->getMark("zhaowei");
        return targets.length() < num && to_select != self && !to_select->isNude();
    }
    return false;
}

void ZhaoweiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    if (source->getMark("zhaowei_mod") == 1) {
        int card_id = room->askForCardChosen(source, targets.first(), "hes", objectName(), false, Card::MethodNone);
        source->obtainCard(Sanguosha->getCard(card_id));
        targets.first()->drawCards(1);
    }
    else if (source->getMark("zhaowei_mod") == 2) {
        QList<ServerPlayer *> targets_other;
        foreach (ServerPlayer *p, targets)
        {
            if (p->isNude())
                continue;
            int card_id = room->askForCardChosen(source, p, "hes", objectName(), false, Card::MethodNone);
            source->obtainCard(Sanguosha->getCard(card_id));
            targets_other << p;
        }
        
        int num = targets_other.length();
        if (num == 0)
            return;
        const Card *cards = room->askForExchange(source, objectName(), num, num, true, "@zhaowei_discard:" + QString::number(num));
        room->throwCard(cards, source, source);

        //obtain card
        QList<int> ids = cards->getSubcards();
        QList<int> disable;
        foreach(ServerPlayer *p, targets_other) {
            QList<int> able;
            foreach(int id, ids) {
                if (room->getCardPlace(id) == Player::DiscardPile && !disable.contains(id))
                    able << id;
            }
            if (able.isEmpty())
                return;
            room->fillAG(ids, p, disable);
            int select_id = room->askForAG(p, able, false, objectName());
            room->clearAG(p);
            
            p->obtainCard(Sanguosha->getCard(select_id));
            disable << select_id;
        }
    }

}


class ZhaoweiVS : public ZeroCardViewAsSkill
{
public:
    ZhaoweiVS()
        : ZeroCardViewAsSkill("zhaowei")
    {
        response_pattern = "@@zhaowei!";
    }


    virtual const Card *viewAs() const
    {
        return new ZhaoweiCard;
    }
};

class Zhaowei : public TriggerSkill
{
public:
    Zhaowei()
        : TriggerSkill("zhaowei")
    {
        events << EventPhaseStart << PreCardUsed << EventPhaseChanging;
        view_as_skill = new ZhaoweiVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeSkill && use.from && use.from->isCurrent()) {
                room->setPlayerMark(use.from, "zhaowei", use.from->getMark("zhaowei") + 1);
            }
        }
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(change.player, "zhaowei", 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent != EventPhaseStart)
            return QList<SkillInvokeDetail>();
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (!player->hasSkill(this) || player->isDead() || player->getPhase() != Player::Finish)
            return QList<SkillInvokeDetail>();
        if (player->getMark("zhaowei") <= player->getHandcardNum())
            return QList<SkillInvokeDetail>();
        foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
            if (!p->isNude())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString choice = room->askForChoice(invoke->invoker, objectName(), "zhaowei1+zhaowei2+cancel", NULL);
        if (choice == "cancel")
            return false;
        invoke->invoker->showHiddenSkill(objectName());
        if (choice == "zhaowei1") {
            room->setPlayerMark(invoke->invoker, "zhaowei_mod" , 1);
            room->askForUseCard(invoke->invoker, "@@zhaowei!", "@zhaowei1");
        }
        if (choice == "zhaowei2") {
            room->setPlayerMark(invoke->invoker, "zhaowei_mod", 2);
            room->askForUseCard(invoke->invoker, "@@zhaowei!", "@zhaowei2");
        }
        return true;
    }
};

ZhuzheCard::ZhuzheCard()
{
    target_fixed = true;
}


void ZhuzheCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    foreach(ServerPlayer *p, room->getOtherPlayers(source)) {
        if (p->getKingdom() != "tkz")
            continue;
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), p->objectName());
        const Card *c = room->askForCard(p, ".|.|.|hand,equipped", "@zhuzhe", NULL, Card::MethodNone, source, false, "zhuzhe");
        if (c != NULL) {
            source->obtainCard(c);
            break;
        }
    }
}

class Zhuzhe : public ZeroCardViewAsSkill
{
public:
    Zhuzhe()
        : ZeroCardViewAsSkill("zhuzhe$")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasUsed("ZhuzheCard"))
            return false;

        QList<const Player *> siblings = player->getSiblings();
        foreach(const Player *p, player->getSiblings()) {
            if (p->getKingdom() == "tkz") {
                return true;
            }
        }
        return false;
    }

    virtual const Card *viewAs() const
    {
        return new ZhuzheCard;
    }
};


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

class Diexing : public TriggerSkill
{
public:
    Diexing()
        : TriggerSkill("diexing")
    {
        events << HpChanged << CardsMoveOneTime;
        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *, QVariant &data) const
    {
        if (triggerEvent == HpChanged) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p != NULL) {
                if (!p->tag.contains("diexingHp"))
                    p->tag["diexingHp"] = p->getGeneral()->getMaxHp();

                p->tag["diexingHp2"] = p->tag.value("diexingHp");
                p->tag["diexingHp"] = p->getHp();
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        QList<SkillInvokeDetail> r;
        if (triggerEvent == HpChanged) {
            ServerPlayer *player = data.value<ServerPlayer *>();

            if (player != NULL && player->isAlive() && player->hasSkill(this)) {
                // judge if hp is deduced
                bool ok = false;
                if (player->tag.value("diexingHp2", -1).toInt(&ok) > player->getHp() && ok)
                    r << SkillInvokeDetail(this, player, player, NULL, true);
            }
        } else {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != NULL && move.from->isAlive() && move.from->hasSkill(this)
                && ((move.to != move.from) || (move.to_place == Player::PlaceTable) || (move.to_place == Player::DiscardPile) || (move.to_place == Player::DrawPile))) {
                int n = 0;
                for (int i = 0; i < move.card_ids.length(); ++i) {
                    if (((move.from_places.at(i) == Player::PlaceHand) || (move.from_places.at(i) == Player::PlaceEquip)))
                        ++n;
                }
                if (n >= 2) {
                    ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
                    if (player != NULL)
                        r << SkillInvokeDetail(this, player, player, NULL, true);
                }
            }
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
            if (invoke->invoker->hasShownSkill(this)) {
                LogMessage l;
                l.type = "#TriggerSkill";
                l.from = invoke->invoker;
                l.arg = objectName();
                room->sendLog(l);

                room->notifySkillInvoked(invoke->invoker, objectName());
            }

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == HpChanged)
            invoke->invoker->drawCards(2, objectName());
        else
            room->recover(invoke->invoker, RecoverStruct());

        return false;
    }
};

LinsaCard::LinsaCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void LinsaCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();

    // get the card detail before give
    Card::Suit suit = Sanguosha->getCard(getSubcards().first())->getSuit();
    room->showCard(effect.from, getEffectiveId());

    CardMoveReason r(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), "linsa", QString());
    room->obtainCard(effect.to, this, r);

    if (suit == Card::Heart)
        room->loseHp(effect.to);
    else if (suit == Card::Spade) {
        LogMessage l;
        l.type = "#LinsaNullifyPre";
        l.from = effect.from;
        l.to << effect.to;
        room->sendLog(l);

        effect.to->tag["linsaNullifyFrom"] = QVariant::fromValue<ServerPlayer *>(effect.from);
    }
}

class LinsaVS : public OneCardViewAsSkill
{
public:
    LinsaVS()
        : OneCardViewAsSkill("linsa")
    {
        filter_pattern = ".|.|.|hand";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("LinsaCard");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        LinsaCard *c = new LinsaCard;
        c->addSubcard(originalCard);
        c->setShowSkill(objectName());

        return c;
    }
};

class Linsa : public TriggerSkill
{
public:
    Linsa()
        : TriggerSkill("linsa")
    {
        events << CardUsed << CardResponded << EventPhaseChanging;
        view_as_skill = new LinsaVS;
    }

    void record(TriggerEvent e, Room *room, QVariant &data)
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->tag.contains("linsaNullifyFrom")) {
                        ServerPlayer *from = p->tag.value("linsaNullifyFrom").value<ServerPlayer *>();
                        if (from != NULL) {
                            LogMessage l;
                            l.type = "#LinsaNullifyPreRoundOver";
                            l.from = from;
                            l.to << p;
                            room->sendLog(l);
                        }

                        p->tag.remove("linsaNullifyFrom");
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *p = NULL;
        CardUseStruct use;
        if (triggerEvent == CardUsed) {
            use = data.value<CardUseStruct>();
            p = use.from;
        } else if (triggerEvent = CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse) {
                use.from = resp.m_from;
                use.card = resp.m_card;
                use.m_isHandcard = resp.m_isHandcard;
                p = use.from;
            }
        }

        if (p != NULL && use.m_isHandcard) {
            ServerPlayer *e = p->tag.value("linsaNullifyFrom", NULL).value<ServerPlayer *>();
            if (e != NULL)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, e, p, NULL, true, NULL, false);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        const Card *nullifiedCard = NULL;

        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.nullified_list << "_ALL_TARGETS";
            data = QVariant::fromValue<CardUseStruct>(use);
            nullifiedCard = use.card;
        } else if (triggerEvent = CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            resp.m_isNullified = true;
            data = QVariant::fromValue<CardResponseStruct>(resp);
            nullifiedCard = resp.m_card;
        }

        invoke->invoker->tag.remove("linsaNullifyFrom");

        if (nullifiedCard != NULL) {
            LogMessage l;
            l.type = "#LinsaNullify";
            l.from = invoke->owner;
            l.to << invoke->invoker;
            l.arg = nullifiedCard->objectName();
            room->sendLog(l);
        }

        return false;
    }
};

class Shengyu : public TriggerSkill
{
public:
    Shengyu()
        : TriggerSkill("shengyu")
    {
        events << TargetSpecified << CardFinished;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        QList<SkillInvokeDetail> r;
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetSpecified) {
            if (use.card != NULL && (use.card->isKindOf("Slash") || use.card->isNDTrick()) && (use.card->isRed() || use.card->isBlack()) && use.from != NULL
                && use.from->hasSkill(this) && use.from->isAlive()) {
                foreach (ServerPlayer *p, use.to)
                    r << SkillInvokeDetail(this, use.from, use.from, NULL, true, p);
            }
        } else {
            if (use.card != NULL) {
                foreach (ServerPlayer *p, use.to) {
                    if (p->hasFlag("shengyu_" + use.card->toString()))
                        r << SkillInvokeDetail(this, use.from, use.from, NULL, true, p, false);
                }
            }
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
            if (invoke->invoker->hasShownSkill(this) && triggerEvent == TargetSpecified) {
                LogMessage l;
                l.type = "#TriggerSkill";
                l.arg = objectName();
                l.from = invoke->invoker;
                room->sendLog(l);

                room->notifySkillInvoked(invoke->invoker, objectName());
            }
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString colorstring = "red";
        if (use.card->isBlack())
            colorstring = "black";
        else if (!use.card->isRed())
            return false;

        if (triggerEvent == TargetSpecified) {
            QList<int> ids;
            foreach (const Card *eq, invoke->targets.first()->getEquips()) {
                if (eq->sameColorWith(use.card) && !invoke->targets.first()->getBrokenEquips().contains(eq->getId()))
                    ids << eq->getId();
            }

            if (ids.isEmpty() || invoke->targets.first()->askForSkillInvoke("#shengyu-select", QString("mow:%1::%2").arg(invoke->invoker->objectName()).arg(colorstring))) {
                room->setPlayerCardLimitation(invoke->targets.first(), "use,response", ".|" + colorstring + "|.|.", "shengyu", false);
                invoke->targets.first()->setFlags("shengyu_" + use.card->toString());
            } else
                invoke->targets.first()->addBrokenEquips(ids);
        } else
            room->removePlayerCardLimitation(invoke->targets.first(), "use,response", ".|" + colorstring + "|.|.$0", "shengyu");

        return false;
    }
};

class Modao : public OneCardViewAsSkill
{
public:
    Modao()
        : OneCardViewAsSkill("modao")
    {
        filter_pattern = ".|club|.|.";
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        BoneHealing *bh = new BoneHealing(Card::SuitToBeDecided, -1);
        DELETE_OVER_SCOPE(BoneHealing, bh)
        return bh->isAvailable(player);
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        BoneHealing *bh = new BoneHealing(Card::SuitToBeDecided, -1);
        DELETE_OVER_SCOPE(BoneHealing, bh)
        return bh->match(pattern);
    }

    const Card *viewAs(const Card *originalCard) const
    {
        BoneHealing *bh = new BoneHealing(Card::SuitToBeDecided, -1);
        bh->addSubcard(originalCard);
        bh->setSkillName(objectName());
        bh->setShowSkill(objectName());
        return bh;
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

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
            if (invoke->invoker->hasShownSkill(this)) {
                LogMessage log;
                log.type = "#TriggerSkill";
                log.from = invoke->invoker;
                log.arg = objectName();
                room->sendLog(log);

                room->notifySkillInvoked(invoke->invoker, objectName());
            }
            return true;
        }

        return false;
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

class Minghe : public TriggerSkill
{
public:
    Minghe()
        : TriggerSkill("minghe")
    {
        events << TargetSpecified;
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> r;
        if (use.from != NULL && use.card != NULL && !use.card->isKindOf("SkillCard") && use.to.length() >= 2) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(this))
                    r << SkillInvokeDetail(this, p, p);
            }
        }

        return r;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1, objectName());
        return false;
    }
};

GuwuCard::GuwuCard()
    : SkillCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void GuwuCard::onEffect(const CardEffectStruct &effect) const
{
    CardMoveReason r(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), objectName(), QString());
    Room *room = effect.from->getRoom();
    room->obtainCard(effect.to, this, r, false);

    try {
        room->setPlayerMark(effect.to, "guwugive", 1);
        room->askForUseCard(effect.to, "@@guwugiveuse", "@guwu-giveuse");
        if (effect.to->getMark("guwugive") != 0)
            room->setPlayerMark(effect.to, "guwugive", 0);
    } catch (TriggerEvent ev) {
        if (ev == TurnBroken) {
            if (effect.to->getMark("guwugive") != 0)
                room->setPlayerMark(effect.to, "guwugive", 0);
        }
        throw;
    }
}

class Guwu : public ViewAsSkill
{
public:
    Guwu()
        : ViewAsSkill("guwu")
    {
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() >= 2)
            return false;
        return !to_select->isEquipped();
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        if (cards.length() > 2)
            return NULL;

        GuwuCard *c = new GuwuCard;
        c->addSubcards(cards);
        c->setShowSkill(objectName());
        return c;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("GuwuCard");
    }
};

class GuwuGiveUseVS : public OneCardViewAsSkill
{
public:
    GuwuGiveUseVS()
        : OneCardViewAsSkill("guwugiveuse")
    {
        response_pattern = "@@guwugiveuse";
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        // how to prevent recasting?
        // "'A' proceed the effect of 'guwu' and used 'C' as 'C'" will be output in the log box, which is not what I need
        // I don't want to deal with the logic in SkillCard::validate

        Card *c = Sanguosha->cloneCard(originalCard);
        c->setSkillName("_guwu");
        c->setCanRecast(false);

        if (c->isAvailable(Self))
            return c;
        else
            delete c;

        return NULL;
    }
};

class GuwuGiveUse : public TriggerSkill
{
public:
    GuwuGiveUse()
        : TriggerSkill("guwugiveuse")
    {
        events << PreCardUsed;
        view_as_skill = new GuwuGiveUseVS;
        global = true;
    }

    void record(TriggerEvent, Room *room, QVariant &data)
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from->getMark("guwugive") != 0)
            room->setPlayerMark(use.from, "guwugive", 0);
    }
};

class GuwuGiveUseTM : public TargetModSkill
{
public:
    GuwuGiveUseTM()
        : TargetModSkill("#guwu")
    {
        pattern = ".|.|.|hand";
    }

    int getExtraTargetNum(const Player *from, const Card *) const
    {
        if (from->getMark("guwugive") > 0)
            return 1;

        return 0;
    }
};

class Kuangwu : public TriggerSkill
{
public:
    Kuangwu()
        : TriggerSkill("kuangwu")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> r;
        ServerPlayer *target = data.value<ServerPlayer *>();
        if (target != NULL && target->getPhase() == Player::Play) {
            foreach (ServerPlayer *sp, room->getAllPlayers()) {
                if (sp->hasSkill(this) && sp != target && !sp->isNude())
                    r << SkillInvokeDetail(this, sp, sp, target, false, target);
            }
        }

        return r;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        return room->askForCard(invoke->invoker, ".", "@kuangwu", data, Card::MethodDiscard, invoke->targets.first(), false, objectName());
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<int> cardslist = room->getNCards(3);
        room->returnToTopDrawPile(cardslist);
        QList<int> takelist;
        QList<int> asklist = cardslist;

        room->fillAG(cardslist);
        while (!asklist.isEmpty()) {
            int id = room->askForAG(invoke->targets.first(), asklist, false, objectName());
            room->takeAG(invoke->targets.first(), id, false, QList<ServerPlayer *>() << invoke->targets.first());

            const Card *c = Sanguosha->getCard(id);
            Card::CardType t = Card::TypeSkill;
            if (c != NULL)
                t = c->getTypeId();

            takelist << id;
            asklist.removeAll(id);
            foreach (int remainedId, asklist) {
                const Card *cr = Sanguosha->getCard(remainedId);
                if (cr != NULL && cr->getTypeId() == t) {
                    asklist.removeAll(remainedId);
                    room->takeAG(NULL, id, false, QList<ServerPlayer *>() << invoke->targets.first());
                }
            }
        }
        room->clearAG();

        if (!takelist.isEmpty()) {
            DummyCard dc;
            dc.addSubcards(takelist);
            invoke->targets.first()->obtainCard(&dc);
        }

        return false;
    }
};

class Zhumao : public TriggerSkill
{
public:
    Zhumao()
        : TriggerSkill("zhumao")
    {
        events << TargetSpecifying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> r;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card != NULL && (use.card->isKindOf("Slash") || use.card->isNDTrick()) && use.from != NULL) {
            foreach (ServerPlayer *m, use.to) {
                if (m->isAlive() && m->hasSkill(this)) {
                    // m is mai here
                    if (m->getHandcardNum() < use.from->getHandcardNum()) {
                        use.card->setFlags("xunshi");
                        use.card->setFlags("IgnoreFailed");
                        QList<ServerPlayer *> noselectPlayers = use.to;
                        noselectPlayers << use.from << m;
                        foreach (ServerPlayer *p, room->getAllPlayers()) {
                            if (!noselectPlayers.contains(p) && use.card->targetFilter(QList<const Player *>(), p, use.from)) {
                                r << SkillInvokeDetail(this, m, m);
                                break;
                            }
                        }
                        use.card->setFlags("-xunshi");
                        use.card->setFlags("-IgnoreFailed");
                    }
                }
            }
        }

        return r;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<ServerPlayer *> noselectPlayers = use.to;
        noselectPlayers << use.from << invoke->invoker;
        QList<ServerPlayer *> selectPlayer;
        use.card->setFlags("xunshi");
        use.card->setFlags("IgnoreFailed");
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (!noselectPlayers.contains(p) && use.card->targetFilter(QList<const Player *>(), p, use.from))
                selectPlayer << p;
        }
        use.card->setFlags("-xunshi");
        use.card->setFlags("-IgnoreFailed");

        if (!selectPlayer.isEmpty()) {
            ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, selectPlayer, objectName(), "@zhumao-select:::" + use.card->objectName(), true, true);
            if (target != NULL) {
                invoke->targets << target;
                return true;
            }
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        invoke->targets.first()->drawCards(1, objectName());
        if (!invoke->targets.first()->isAlive())
            return false;

        use.to << invoke->targets.first();
        room->sortByActionOrder(use.to);
        data = QVariant::fromValue<CardUseStruct>(use);

        LogMessage l;
        l.type = "#liexi_extra";
        l.arg = use.card->objectName();
        l.from = use.from;
        l.to << invoke->targets.first();
        room->sendLog(l);

        bool flag = true;
        foreach (ServerPlayer *p, use.to) {
            if (p != invoke->invoker && p->getHandcardNum() <= invoke->invoker->getHandcardNum()) {
                flag = false;
                break;
            }
        }

        if (flag && room->askForSkillInvoke(invoke->invoker, "#zhumao-cancelself", "yes")) {
            LogMessage l;
            l.type = "$CancelTarget";
            l.from = use.from;
            l.to << invoke->invoker;
            l.arg = use.card->objectName();
            room->sendLog(l);

            use.to.removeAll(invoke->invoker);
            data = QVariant::fromValue<CardUseStruct>(use);
        }

        return false;
    }
};

ChuntengCard::ChuntengCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    target_fixed = true;
}

void ChuntengCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    // source->obtainCard(this);
    source->tag["chuntengid"] = getEffectiveId();
}

class ChuntengVS : public OneCardViewAsSkill
{
public:
    ChuntengVS()
        : OneCardViewAsSkill("chunteng")
    {
        response_pattern = "@@chunteng";
        filter_pattern = ".|.|.|spring";
        expand_pile = "spring";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        ChuntengCard *c = new ChuntengCard;
        c->addSubcard(originalCard);

        return c;
    }
};

class Chunteng : public TriggerSkill
{
public:
    Chunteng()
        : TriggerSkill("chunteng")
    {
        events << AfterDrawInitialCards << EventPhaseStart;
        view_as_skill = new ChuntengVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        QList<SkillInvokeDetail> r;

        ServerPlayer *target = NULL;
        if (triggerEvent == AfterDrawInitialCards) {
            DrawNCardsStruct st = data.value<DrawNCardsStruct>();
            if (st.isInitial)
                target = st.player;
        } else {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Start)
                target = player;
        }

        if (target != NULL && target->isAlive() && target->hasSkill(this))
            r << SkillInvokeDetail(this, target, target);

        return r;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (invoke->invoker->getPile("spring").isEmpty()) {
            if (invoke->invoker->askForSkillInvoke(this))
                return true;
        } else {
            if (room->askForUseCard(invoke->invoker, "@@chunteng", "@chunteng", -1, Card::MethodNone, true, objectName()))
                return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (invoke->invoker->getPile("spring").isEmpty()) {
            invoke->invoker->addToPile("spring", room->getNCards(4), false);
        } else {
            bool ok = false;
            int id = invoke->invoker->tag["chuntengid"].toInt(&ok);
            const Card *card = Sanguosha->getCard(id);
            if (ok && card != NULL)
                invoke->invoker->obtainCard(card, false);

            invoke->invoker->tag.remove("chuntengid");
        }

        return false;
    }
};

HuazhaoCard::HuazhaoCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void HuazhaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->showCard(source, getEffectiveId());
}

Huazhao2Card::Huazhao2Card()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "_huazhao";
}

void Huazhao2Card::onEffect(const CardEffectStruct &effect) const
{
    CardMoveReason r(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "huazhao", QString());
    Room *room = effect.from->getRoom();
    room->obtainCard(effect.to, this, r, false);

    if (effect.to->getHandcardNum() > effect.from->getPile("spring").length()) {
        if (!room->askForDiscard(effect.to, "huazhao", 1, 1, true, true, "@huazhao-discard")) {
            DummyCard d;
            d.addSubcards(effect.from->getPile("spring"));
            CardMoveReason r2(CardMoveReason::S_REASON_PUT, effect.to->objectName(), QString(), "huazhao", QString());
            room->throwCard(&d, r2, NULL);
        }
    }
}

class HuazhaoVS : public OneCardViewAsSkill
{
public:
    HuazhaoVS()
        : OneCardViewAsSkill("huazhao")
    {
        expand_pile = "spring";
    }

    bool viewFilter(const Card *to_select) const
    {
        if (Self->getPile("spring").contains(to_select->getId())) {
            if (Sanguosha->getCurrentCardUsePattern() == "@@huazhao-card2!")
                return true;

            bool ok = false;
            Card::Suit s = static_cast<Card::Suit>(Self->property("huazhao1").toString().toInt(&ok));
            if (ok)
                return to_select->getSuit() == s;
        }

        return false;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Card *c = NULL;
        if (Sanguosha->getCurrentCardUsePattern() == "@@huazhao-card1") {
            c = new HuazhaoCard;
            c->setSkillName(objectName());
        } else {
            c = new Huazhao2Card;
            c->setSkillName("_huazhao");
        }

        c->addSubcard(originalCard);

        return c;
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@huazhao-card");
    }
};

class Huazhao : public TriggerSkill
{
public:
    Huazhao()
        : TriggerSkill("huazhao")
    {
        events << CardUsed << CardResponded;
        view_as_skill = new HuazhaoVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct u;

        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            u.card = use.card;
            u.from = use.from;
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse) {
                u.card = resp.m_card;
                u.from = resp.m_from;
            }
        }

        if (u.card != NULL && !u.card->isKindOf("SkillCard") && u.from != NULL && u.from->isAlive() && u.from->hasSkill(this) && !u.from->getPile("spring").isEmpty()) {
            SkillInvokeDetail d(this, u.from, u.from);
            d.tag["u"] = QVariant::fromValue<CardUseStruct>(u);

            return QList<SkillInvokeDetail>() << d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        CardUseStruct u = invoke->tag["u"].value<CardUseStruct>();
        room->setPlayerProperty(u.from, "huazhao1", QString::number(static_cast<int>(u.card->getSuit())));
        return room->askForUseCard(u.from, "@@huazhao-card1", "@huazhao1:::" + u.card->getSuitString(), -1, Card::MethodNone, true, objectName());
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        CardUseStruct u = invoke->tag["u"].value<CardUseStruct>();
        if (!room->askForUseCard(u.from, "@@huazhao-card2!", "@huazhao2", -1, Card::MethodNone, true, "_huazhao")) {
            QList<ServerPlayer *> p = room->getAllPlayers();
            p.removeAll(u.from);
            ServerPlayer *target = p[qrand() % p.length()];

            Card *c = new Huazhao2Card;
            c->setSkillName("_huazhao");
            c->addSubcard(u.from->getPile("spring")[qrand() % u.from->getPile("spring").length()]);

            CardUseStruct newUse(c, u.from, target);
            room->useCard(newUse);
        }

        return false;
    }
};

TH16Package::TH16Package()
    : Package("th16")
{
    General *okina = new General(this, "okina$", "tkz", 4);
    okina->addSkill(new Miyi);
    //okina->addSkill(new MiyiBasicExceptSlash);
    //okina->addSkill(new MiyiTargetMod);
    okina->addSkill(new Zhaowei);
    okina->addSkill(new Zhuzhe);
    //related_skills.insertMulti("miyi", "#miyi-tiger");
    //related_skills.insertMulti("miyi", "#miyi-basic");

    General *eternity = new General(this, "eternity", "tkz", 3);
    eternity->addSkill(new Diexing);
    eternity->addSkill(new Linsa);

    General *nemuno = new General(this, "nemuno", "tkz", 4);
    nemuno->addSkill(new Shengyu);
    nemuno->addSkill(new Modao);

    General *aun = new General(this, "aun", "tkz", 3);
    aun->addSkill(new Xunfo);
    aun->addSkill(new Huyuan);

    General *narumi = new General(this, "narumi", "tkz");
    narumi->addSkill(new Puti);
    narumi->addSkill(new Zangfa);

    General *satono = new General(this, "satono", "tkz", 3);
    satono->addSkill(new Minghe);
    satono->addSkill(new Guwu);
    satono->addSkill(new GuwuGiveUseTM);
    related_skills.insertMulti("guwu", "#guwu");

    General *mai = new General(this, "mai", "tkz", 3);
    mai->addSkill(new Kuangwu);
    mai->addSkill(new Zhumao);

    General *okinasp = new General(this, "okina_sp", "tkz");
    okinasp->addSkill(new Menfei);
    okinasp->addSkill(new Houhu);

    General *lili = new General(this, "lilywhite_sp", "tkz");
    lili->addSkill(new Chunteng);
    lili->addSkill(new Huazhao);

    addMetaObject<ZhaoweiCard>();
    addMetaObject<ZhuzheCard>();
    addMetaObject<MenfeiCard>();
    addMetaObject<HuyuanCard>();
    addMetaObject<ChuntengCard>();
    addMetaObject<HuazhaoCard>();
    addMetaObject<Huazhao2Card>();
    addMetaObject<LinsaCard>();
    skills << new HouhuDistance << new ZangfaDistance << new HuyuanDis << new GuwuGiveUse;
}

ADD_PACKAGE(TH16)
