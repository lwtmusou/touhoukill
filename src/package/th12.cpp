#include "th12.h"

#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"

PuduCard::PuduCard()
{
}

bool PuduCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return (targets.isEmpty() && to_select != Self && to_select->isWounded());
}

void PuduCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    RecoverStruct recover;
    room->recover(effect.to, recover);
    room->loseHp(effect.from, 1);
}

class Pudu : public ZeroCardViewAsSkill
{
public:
    Pudu()
        : ZeroCardViewAsSkill("pudu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("PuduCard");
    }

    virtual const Card *viewAs() const
    {
        return new PuduCard;
    }
};

class Jiushu : public TriggerSkill
{
public:
    Jiushu()
        : TriggerSkill("jiushu")
    {
        events << EventPhaseStart;
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->isAlive() && player->getPhase() == Player::Finish && player->isWounded())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(invoke->invoker->getLostHp());
        return false;
    }
};

class Fahua : public TriggerSkill
{
public:
    Fahua()
        : TriggerSkill("fahua$")
    {
        events << TargetConfirming;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!(use.card->isKindOf("TrickCard") && use.from != NULL) || use.to.length() != 1)
            return QList<SkillInvokeDetail>();

        if (use.to.first() == use.from || !use.to.first()->hasLordSkill(this))
            return QList<SkillInvokeDetail>();

        use.card->setFlags("fahua");
        use.card->setFlags("IgnoreFailed");
        bool invoke = false;
        foreach (ServerPlayer *q, room->getLieges("xlc", use.to.first())) {
            //if (q == use.from)
            //    continue;
            if (use.from->isProhibited(q, use.card))
                continue;
            if (use.card->isKindOf("Drowning") && !q->canDiscard(q, "e"))
                continue;
            if (!use.card->targetFilter(QList<const Player *>(), q, use.from))
                continue;
            invoke = true;
            break;
        }
        use.card->setFlags("-fahua");
        use.card->setFlags("-IgnoreFailed");
        if (invoke)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.to.first(), use.to.first());
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.card->setFlags("fahua"); // for distance limit
        use.card->setFlags("IgnoreFailed"); //for factor which named "ignore" and related with Function "isProhibited" and  "targetFilter"
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getLieges("xlc", invoke->invoker)) {
            if (use.to.contains(p) || use.from->isProhibited(p, use.card))
                continue;
            if (use.card->isKindOf("Drowning") && !p->canDiscard(p, "e"))
                continue;

            if (!use.card->targetFilter(QList<const Player *>(), p, use.from))
                continue;
            targets << p;
        }
        use.card->setFlags("-fahua");
        use.card->setFlags("-IgnoreFailed");

        room->setTag("fahua_target", QVariant::fromValue(invoke->invoker));
        room->setTag("fahua_use", data);
        foreach (ServerPlayer *p, targets) {
            QString prompt = "tricktarget:" + use.from->objectName() + ":" + invoke->invoker->objectName() + ":" + use.card->objectName();
            if (p->askForSkillInvoke("fahua_change", prompt)) {
                use.to << p;
                use.to.removeOne(invoke->invoker);
                data = QVariant::fromValue(use);

                QList<ServerPlayer *> logto;
                logto << invoke->invoker;
                room->touhouLogmessage("$CancelTarget", use.from, use.card->objectName(), logto);
                logto << p;
                logto.removeOne(invoke->invoker);
                room->touhouLogmessage("#fahua_change", use.from, use.card->objectName(), logto);

                /*if (use.card->isKindOf("Collateral")) {
                    QList<ServerPlayer *> listt;
                    foreach (ServerPlayer *victim, room->getOtherPlayers(p)) {
                        if (p->canSlash(victim))
                            listt << victim;
                    }
                    //the list will not be empty since we utilizing targetFilter
                    ServerPlayer *newVictim = room->askForPlayerChosen(use.from, listt, objectName(), "@fahuaCollateral:" + p->objectName(), false, false);
                    CardUseStruct new_use;
                    new_use.from = use.from;
                    new_use.to << p;
                    new_use.card = use.card;
                    p->tag["collateralVictim"] = QVariant::fromValue((ServerPlayer *)newVictim);
                    data = QVariant::fromValue(new_use);
                    logto.removeOne(p);
                    logto << newVictim;
                    room->touhouLogmessage("#CollateralSlash", use.from, use.card->objectName(), logto);
                }*/

                break;
            }
        }
        return false;
    }
};

class FahuaDistance : public TargetModSkill
{
public:
    FahuaDistance()
        : TargetModSkill("fahua-dist")
    {
        pattern = "TrickCard";
    }

    int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->hasFlag("fahua"))
            return 1000;

        return 0;
    }
};

WeizhiCard::WeizhiCard()
{
    target_fixed = true;
}

void WeizhiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    if (source->isAlive())
        room->drawCards(source, subcards.length() + 1);
}

class Weizhi : public ViewAsSkill
{
public:
    Weizhi()
        : ViewAsSkill("weizhi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("WeizhiCard") && !player->isNude();
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isKindOf("TrickCard") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0) {
            WeizhiCard *card = new WeizhiCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;
    }
};

class Weizhuang : public TriggerSkill
{
public:
    Weizhuang()
        : TriggerSkill("weizhuang")
    {
        frequency = Compulsory;
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isNDTrick() && use.from != NULL) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, use.to) {
                if (use.from != p && p->hasSkill(this))
                    d << SkillInvokeDetail(this, p, p, NULL, true, use.from);
            }
            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = invoke->invoker;
        room->notifySkillInvoked(player, objectName());
        room->touhouLogmessage("#TriggerSkill", player, "weizhuang");
        CardUseStruct use = data.value<CardUseStruct>();
        use.from->tag["weizhuang_target"] = QVariant::fromValue(player);
        QString prompt = "@weizhuang-discard:" + player->objectName() + ":" + use.card->objectName();
        const Card *card = room->askForCard(use.from, ".Basic", prompt, data, Card::MethodDiscard);
        if (card == NULL) {
            use.nullified_list << player->objectName();
            data = QVariant::fromValue(use);
        }

        return false;
    }
};

class JinghuaVS : public OneCardViewAsSkill
{
public:
    JinghuaVS()
        : OneCardViewAsSkill("jinghua")
    {
        filter_pattern = ".|red|.|.";
        response_pattern = "nullification";
        response_or_use = true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Card *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        return ncard;
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        foreach (const Card *equip, player->getEquips()) {
            if (equip->isRed())
                return true;
        }
        return !player->isKongcheng() || !player->getHandPile().isEmpty();
    }
};

class Jinghua : public TriggerSkill
{
public:
    Jinghua()
        : TriggerSkill("jinghua")
    {
        events << CardUsed << EventPhaseChanging << PostCardEffected;
        view_as_skill = new JinghuaVS;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Nullification"))
                room->setPlayerFlag(use.from, "jinghua");
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerFlag(p, "-jinghua");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e != PostCardEffected)
            return QList<SkillInvokeDetail>();

        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.from && effect.from != effect.to && effect.to->hasSkill(this) && effect.to->isAlive() && effect.to->hasFlag("jinghua") && effect.card->isNDTrick()) {
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

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        QString prompt = "invoke:" + effect.card->objectName();
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        QList<int> ids;
        if (effect.card->isVirtualCard())
            ids = effect.card->getSubcards();
        else
            ids << effect.card->getEffectiveId();

        //CardsMoveStruct move;
        //move.to_place = Player::DrawPile;
        //move.card_ids << ids;
        //room->moveCardsAtomic(move, false);
        room->moveCardsToEndOfDrawpile(ids, true);

        LogMessage l;
        l.type = "$jinghua";
        l.from = invoke->invoker;
        l.card_str = IntList2StringList(ids).join("+");
        room->sendLog(l);

        if (ids.length() > 1) {
            //QList<int> card_ids = room->getNCards(ids.length(), false, true);
            room->askForGuanxing(invoke->invoker, ids, Room::GuanxingDownOnly, objectName());
        }
        return false;
    }
};

class Weiguang : public TriggerSkill
{
public:
    Weiguang()
        : TriggerSkill("weiguang")
    {
        events << CardUsed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if ((use.card->isKindOf("TrickCard") && !use.card->isVirtualCard() && use.from->hasSkill(this) && use.from->isAlive()))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        CardsMoveStruct move;
        move.to = invoke->invoker;
        move.to_place = Player::PlaceHand;
        move.card_ids << room->drawCard(true);
        room->moveCardsAtomic(move, false);
        return false;
    }
};

class Zhengyi : public TriggerSkill
{
public:
    Zhengyi()
        : TriggerSkill("zhengyi")
    {
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isBlack() && (use.card->isNDTrick() || use.card->isKindOf("Slash"))) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, use.to) {
                if (p->hasSkill(this)) {
                    bool invoke = !p->isKongcheng();
                    if (!invoke) {
                        foreach (const Card *card, p->getEquips()) {
                            if (card->isRed()) {
                                invoke = true;
                                break;
                            }
                        }
                    }

                    if (invoke)
                        d << SkillInvokeDetail(this, p, p);
                }
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "@zhengyi:" + use.card->objectName();
        return room->askForCard(invoke->invoker, ".|red", prompt, data, Card::MethodDiscard, NULL, false, objectName());
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.nullified_list << invoke->invoker->objectName();
        data = QVariant::fromValue(use);
        return false;
    }
};

class Baota : public TriggerSkill
{
public:
    Baota()
        : TriggerSkill("baota")
    {
        events << CardsMoveOneTime << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->getPhase() == Player::Finish)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
            if (player != NULL && player->isAlive() && player->hasSkill(this) && move.to_place == Player::DiscardPile) {
                foreach (int id, move.card_ids) {
                    if (room->getCardPlace(id) == Player::DiscardPile
                        && (move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceSpecial && move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceDelayedTrick))
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = invoke->invoker;
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QList<int> ids;
            foreach (int id, move.card_ids) {
                if (room->getCardPlace(id) == Player::DiscardPile
                    && (move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceSpecial && move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceDelayedTrick))
                    ids << id;
            }
            room->fillAG(ids, player);
            int card_id = room->askForAG(player, ids, true, objectName());
            room->clearAG(player);
            if (card_id > -1)
                invoke->tag["baota_id"] = QVariant::fromValue(card_id);
            return card_id > -1;
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@baota", true, true);
            if (target) {
                invoke->targets << target;
                return true;
            }
        }
        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            int card_id = invoke->tag.value("baota_id", -1).toInt();
            if (card_id == -1)
                return false;

            LogMessage mes;
            mes.type = "$baota";
            mes.from = invoke->invoker;
            mes.arg = objectName();
            mes.card_str = Sanguosha->getCard(card_id)->toString();
            room->sendLog(mes);

            QList<int> buttom_ids;
            buttom_ids << card_id;
            room->moveCardsToEndOfDrawpile(buttom_ids, true);
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *target = invoke->targets.first();
            CardsMoveStruct move;
            move.to = target;
            move.to_place = Player::PlaceHand;
            move.card_ids << room->drawCard(true);
            room->moveCardsAtomic(move, false);
        }
        return false;
    }
};

class Shuinan : public TriggerSkill
{
public:
    Shuinan()
        : TriggerSkill("shuinan")
    {
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!(use.from != NULL && use.card->getTypeId() == Card::TypeTrick)) //use.card->isNDTrick()
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, use.to) {
            if (p->hasSkill(this) && use.from != p && p->canDiscard(use.from, "hs"))
                d << SkillInvokeDetail(this, p, p, NULL, false, use.from);
        }

        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        invoke->invoker->tag["shuinan_use"] = data;
        return invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(use.from));
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), use.from->objectName());
        int id = room->askForCardChosen(invoke->invoker, use.from, "hs", objectName(), false, Card::MethodDiscard);
        room->throwCard(id, use.from, invoke->invoker);
        return false;
    }
};

NihuoCard::NihuoCard()
{
}
bool NihuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty())
        return false;

    Duel *duel = new Duel(Card::NoSuit, 0);
    duel->deleteLater();
    return to_select != Self && !to_select->isProhibited(Self, duel) && !to_select->isCardLimited(duel, Card::MethodUse);
}
void NihuoCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    Duel *duel = new Duel(Card::NoSuit, 0);
    duel->setSkillName("_nihuo");
    room->useCard(CardUseStruct(duel, effect.to, effect.from));
}

class Nihuo : public ZeroCardViewAsSkill
{
public:
    Nihuo()
        : ZeroCardViewAsSkill("nihuo")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("NihuoCard");
    }

    virtual const Card *viewAs() const
    {
        return new NihuoCard;
    }
};

class Lizhi : public TriggerSkill
{
public:
    Lizhi()
        : TriggerSkill("lizhi")
    {
        events << DamageCaused;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from != NULL && damage.from->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, false, damage.to);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(2, objectName());
        return true;
    }
};

class Yunshang : public TriggerSkill
{
public:
    Yunshang()
        : TriggerSkill("yunshang")
    {
        frequency = Compulsory;
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!(use.from != NULL && use.card->isNDTrick()))
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, use.to) {
            if (p->hasSkill(this) && !use.from->inMyAttackRange(p) && use.from != p)
                d << SkillInvokeDetail(this, p, p, NULL, true);
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.nullified_list << invoke->invoker->objectName();
        data = QVariant::fromValue(use);
        room->notifySkillInvoked(invoke->invoker, objectName());

        return false;
    }
};

class Souji : public TriggerSkill
{
public:
    Souji()
        : TriggerSkill("souji")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    //case1.1 while provideCard went to discar pile , like skill Tianren
    //if the provider is self,it should not trigger skill
    //case2 while retrial card went to discard pile, we consdier the move.from as the responser, not judge.who!!!
    //if responser(move.from) is self, it could not trigger skill.
    //case 3 if the card went to DiscardPile from PlaceTable,shoud chcek whether it was from private pile, like woodenOx. This case should not trigger skill.

    void record(TriggerEvent, Room *room, QVariant &data) const
    {
        ServerPlayer *nazurin = room->getCurrent();
        if (nazurin == NULL || !nazurin->hasSkill(objectName()) || nazurin->isDead())
            return;

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to_place == Player::PlaceTable) {
            //record "soujiExcept"  for case 3
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE
                || (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE) {
                QVariantList record_ids = nazurin->tag["soujiExcept"].toList();
                foreach (int id, move.card_ids) {
                    if (move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceHand && move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceEquip) {
                        if (!record_ids.contains(id))
                            record_ids << id;
                    }
                }
                nazurin->tag["soujiExcept"] = record_ids;
                return;
            }
        } else if (move.to_place == Player::PlaceJudge) {
            QVariantList record_ids = nazurin->tag["soujiExcept"].toList();
            foreach (int id, move.card_ids) {
                if (move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceHand && move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceEquip) {
                    if (!record_ids.contains(id))
                        record_ids << id;
                }
            }
            nazurin->tag["soujiExcept"] = record_ids;
            return;
        }

        if (move.to_place == Player::DiscardPile) {
            ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
            //find the real fromer of some cases, such as retrial or provide
            if (move.reason.m_extraData.value<ServerPlayer *>() != NULL)
                from = move.reason.m_extraData.value<ServerPlayer *>();

            if (from != NULL && nazurin != from) {
                QVariantList obtain_ids;
                foreach (int id, move.card_ids) {
                    if (room->getCardPlace(id) != Player::DiscardPile)
                        continue;

                    switch (move.from_places.at(move.card_ids.indexOf(id))) {
                    case Player::PlaceHand:
                        obtain_ids << id;
                        break;
                    case Player::PlaceEquip:
                        obtain_ids << id;
                        break;
                    case Player::PlaceJudge: { //case of retrial, need check rebyre
                        ServerPlayer *rebyre = move.reason.m_extraData.value<ServerPlayer *>();
                        if (rebyre && rebyre != nazurin) {
                            QVariantList record_ids = nazurin->tag["soujiExcept"].toList();
                            if (!record_ids.contains(id))
                                obtain_ids << id;
                        }
                        break;
                    }
                    case Player::PlaceTable: {
                        QVariantList record_ids = nazurin->tag["soujiExcept"].toList();
                        if (!record_ids.contains(id))
                            obtain_ids << id;
                        break;
                    }
                    default:
                        break;
                    }
                }
                nazurin->tag["souji"] = obtain_ids;
            }
        }

        // delete record: soujiExcept
        QVariantList record_ids = nazurin->tag["soujiExcept"].toList();
        foreach (int id, move.card_ids) {
            if (record_ids.contains(id))
                record_ids.removeOne(id);
        }
        nazurin->tag["soujiExcept"] = record_ids;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *nazurin = room->getCurrent();
        if (nazurin && nazurin->isAlive() && nazurin->hasSkill(objectName()) && move.to_place == Player::DiscardPile) {
            QVariantList obtain_ids = nazurin->tag["souji"].toList();
            if (obtain_ids.length() > 0)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, nazurin, nazurin);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        bool doEffect = invoke->invoker->askForSkillInvoke(objectName(), data);
        if (!doEffect)
            invoke->invoker->tag.remove("souji");
        return doEffect;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QVariantList ids = invoke->invoker->tag["souji"].toList();
        invoke->invoker->tag.remove("souji");

        QList<int> obtain_ids;
        foreach (QVariant record_id, ids)
            obtain_ids << record_id.toInt();

        CardsMoveStruct mo;
        mo.card_ids = obtain_ids;
        mo.to = invoke->invoker;
        mo.to_place = Player::PlaceHand;
        room->moveCardsAtomic(mo, true);
        return false;
    }
};

class Tansuo : public TriggerSkill
{
public:
    Tansuo()
        : TriggerSkill("tansuo")
    {
        events << CardsMoveOneTime << EventPhaseEnd << EventPhaseChanging;
        frequency = Frequent;
    }

    void record(TriggerEvent triggerEvent, Room *, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != NULL && move.from->getPhase() == Player::Discard && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD))
                move.from->addMark("tansuo", move.card_ids.length());
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Discard)
                change.player->setMark("tansuo", 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseEnd) {
            ServerPlayer *naz = data.value<ServerPlayer *>();
            if (naz->getPhase() != Player::Discard || !naz->hasSkill(this))
                return QList<SkillInvokeDetail>();

            if (naz->getMark("tansuo") >= naz->getHp())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, naz, naz);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<int> card_ids = room->getNCards(2, true, true);
        DummyCard dc;
        dc.addSubcards(card_ids);
        invoke->invoker->obtainCard(&dc, false);

        return false;
    }
};

class Xunbao : public TriggerSkill
{
public:
    Xunbao()
        : TriggerSkill("xunbao")
    {
        events << CardUsed << CardResponded;
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from->hasSkill(this) && use.from->isAlive() && !use.card->isKindOf("SkillCard"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
        }
        if (e == CardResponded) {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_isUse && response.m_from && response.m_from->isAlive() && response.m_from->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, response.m_from, response.m_from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        const Card *card = NULL;
        if (e == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
        }
        if (e == CardResponded) {
            CardResponseStruct response = data.value<CardResponseStruct>();
            card = response.m_card;
        }

        QList<int> ids;

        if (room->getDrawPile().length() < 2)
            room->swapPile();

        const QList<int> &drawpile = room->getDrawPile();
        ids << drawpile.last();
        if (drawpile.length() >= 2)
            ids << drawpile.at(drawpile.length() - 2);
        //= room->getNCards(2, true, true);
        //ids << room->drawCard(true);
        LogMessage l;
        l.type = "$xunbaoDrawpile";
        l.card_str = IntList2StringList(ids).join("+");
        room->doNotify(invoke->invoker, QSanProtocol::S_COMMAND_LOG_SKILL, l.toJsonValue());

        QList<int> able;
        QList<int> disable;
        foreach (int id, ids) {
            if (Sanguosha->getCard(id)->getSuit() == card->getSuit())
                able << id;
            else
                disable << id;
        }

        room->fillAG(ids, invoke->invoker, disable);
        int choose_id = room->askForAG(invoke->invoker, able, true, objectName());
        room->clearAG(invoke->invoker);

        if (choose_id > -1) {
            QList<int> ids1;
            ids1 << choose_id;
            CardsMoveStruct move(ids1, NULL, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, invoke->invoker->objectName(), objectName(), QString()));
            room->moveCardsAtomic(move, true);
            ids.removeOne(choose_id);
            DummyCard dummy(ids1);
            invoke->invoker->obtainCard(&dummy);
        }

        return false;
    }
};

class LingbaiVS : public OneCardViewAsSkill
{
public:
    LingbaiVS()
        : OneCardViewAsSkill("lingbai")
    {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("lingbai") > 0 && Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getMark("lingbai") > 0 && (matchAvaliablePattern("slash", pattern) || matchAvaliablePattern("jink", pattern)))
            return true;
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isEquipped();
    }

    const Card *viewAs(const Card *originalCard) const
    {
        bool play = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY);
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (play || matchAvaliablePattern("slash", pattern)) {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName(objectName());
            return slash;
        } else if (matchAvaliablePattern("jink", pattern)) {
            Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
            jink->addSubcard(originalCard);
            jink->setSkillName(objectName());
            return jink;
        }
        return NULL;
    }
};

class Lingbai : public TriggerSkill
{
public:
    Lingbai()
        : TriggerSkill("lingbai")
    {
        events << TargetConfirmed << EventPhaseChanging;
        view_as_skill = new LingbaiVS;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isNDTrick()) {
                foreach (ServerPlayer *p, use.to) {
                    if (p->hasSkill(this) && !p->isCurrent())
                        room->setPlayerMark(p, "lingbai", 1);
                }
            }
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerMark(p, "lingbai", 0);
            }
        }
    }
};

class Yiwang : public TriggerSkill
{
public:
    Yiwang()
        : TriggerSkill("yiwang")
    {
        events << CardsMoveOneTime;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *ko = qobject_cast<ServerPlayer *>(move.from);
        if (ko != NULL && ko->isAlive() && ko->hasSkill(this) && move.from_places.contains(Player::PlaceEquip)) {
            bool wound = false;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->isWounded()) {
                    wound = true;
                    break;
                }
            }
            if (!wound)
                return QList<SkillInvokeDetail>();

            foreach (Player::Place place, move.from_places) {
                if (place == Player::PlaceEquip)
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, ko, ko);
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->isWounded())
                targets << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@yiwang-recover", true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        RecoverStruct recover;
        recover.who = invoke->invoker;
        room->recover(invoke->targets.first(), recover);

        if (invoke->targets.first() != invoke->invoker)
            invoke->invoker->drawCards(1, objectName());

        return false;
    }
};

class Jingxia : public MasochismSkill
{
public:
    Jingxia()
        : MasochismSkill("jingxia")
    {
    }

    QList<SkillInvokeDetail> triggerable(const Room *room, const DamageStruct &damage) const
    {
        bool invoke = false;
        do {
            if (damage.to->isAlive() && damage.to->hasSkill(this)) {
                if (damage.from != NULL && damage.to->canDiscard(damage.from, "hes")) {
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QStringList select;
        if (damage.from != NULL && damage.to->canDiscard(damage.from, "hes"))
            select << "discard";

        QList<ServerPlayer *> fieldcard;
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

    void onDamaged(Room *room, QSharedPointer<SkillInvokeDetail> invoke, const DamageStruct &damage) const
    {
        QList<ServerPlayer *> fieldcard;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (damage.to->canDiscard(p, "ej"))
                fieldcard << p;
        }
        ServerPlayer *player = damage.to;

        QString choice = invoke->tag.value("jingxia").toString();

        room->touhouLogmessage("#InvokeSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        if (choice == "discard") {
            for (int i = 0; i < 2; i++) {
                if (player->isDead() || !player->canDiscard(damage.from, "hes"))
                    return;
                if (damage.from == player)
                    room->askForDiscard(player, "jingxia", 1, 1, false, true);
                else {
                    int card_id = room->askForCardChosen(player, damage.from, "hes", objectName(), false, Card::MethodDiscard);
                    room->throwCard(card_id, damage.from, player);
                }
            }
        } else if (choice == "discardfield") {
            ServerPlayer *player1 = room->askForPlayerChosen(player, fieldcard, objectName(), "@jingxia-discardfield");
            int card1 = room->askForCardChosen(player, player1, "ej", objectName(), false, Card::MethodDiscard);
            room->throwCard(card1, player1, player);
            QList<ServerPlayer *> fieldcard2;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (player->canDiscard(p, "ej"))
                    fieldcard2 << p;
            }
            if (fieldcard2.length() == 0 || !player->isAlive())
                return;
            ServerPlayer *player2 = room->askForPlayerChosen(player, fieldcard2, objectName(), "@jingxia-discardfield2", true);
            if (player2 != NULL) {
                int card2 = room->askForCardChosen(player, player2, "ej", objectName(), false, Card::MethodDiscard);
                room->throwCard(card2, player2, player);
            }
        }
    }
};

class Bianhuan : public TriggerSkill
{
public:
    Bianhuan()
        : TriggerSkill("bianhuan")
    {
        events << DamageInflicted;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->isAlive() && damage.to->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->loseMaxHp(invoke->invoker);
        return true;
    }
};

NuhuoCard::NuhuoCard()
{
}
bool NuhuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return (targets.isEmpty() && to_select != Self);
}
void NuhuoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    room->damage(DamageStruct("nuhuo", target, source));

    QList<ServerPlayer *> all;
    foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
        if (source->canSlash(p, NULL, true))
            all << p;
    }
    if (!all.isEmpty()) {
        ServerPlayer *victim = room->askForPlayerChosen(target, all, "nuhuo", "@nuhuo:" + source->objectName(), false);
        QList<ServerPlayer *> logto;
        logto << victim;
        room->touhouLogmessage("#nuhuoChoose", target, "nuhuo", logto);

        Slash *slash = new Slash(Card::NoSuit, 0);
        CardUseStruct carduse;
        slash->setSkillName("_nuhuo");
        carduse.card = slash;
        carduse.from = source;
        carduse.to << victim;
        room->useCard(carduse, false);
    }
}
class Nuhuo : public ZeroCardViewAsSkill
{
public:
    Nuhuo()
        : ZeroCardViewAsSkill("nuhuo")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->deleteLater();
        if (!player->isCardLimited(slash, Card::MethodUse) && !player->hasUsed("NuhuoCard")) {
            foreach (const Player *p, player->getAliveSiblings()) {
                if (player->canSlash(p, slash, true))
                    return true;
            }
        }
        return false;
    }

    virtual const Card *viewAs() const
    {
        return new NuhuoCard;
    }
};

class Shanshi : public TriggerSkill
{
public:
    Shanshi()
        : TriggerSkill("shanshi")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                p->setMark("shanshi_invoke", 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent != CardsMoveOneTime)
            return QList<SkillInvokeDetail>();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == NULL || move.from->isDead())
            return QList<SkillInvokeDetail>();
        // one lose a card
        if ((move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
            && (move.to != move.from || (move.to_place != Player::PlaceHand && move.to_place != Player::PlaceEquip))) {
            if (move.from->hasSkill(this) && !move.from->isCurrent()) {
                // you lose a card in other's round
                ServerPlayer *current = room->getCurrent();
                if (current == NULL || !current->isCurrent() || current->isDead())
                    return QList<SkillInvokeDetail>();

                ServerPlayer *myo = qobject_cast<ServerPlayer *>(move.from);
                if (myo == NULL || myo == current || myo->getMark("shanshi_invoke") > 0)
                    return QList<SkillInvokeDetail>();
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, myo, myo, NULL, false, current);
            } else {
                // others lose a card in your round
                ServerPlayer *myo = room->getCurrent();
                if (myo == NULL || !myo->hasSkill(this) || !myo->isCurrent() || myo->isDead() || myo->getMark("shanshi_invoke") > 0)
                    return QList<SkillInvokeDetail>();

                ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
                if (from == NULL || from == myo)
                    return QList<SkillInvokeDetail>();

                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, myo, myo, NULL, false, from);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->tag["shanshi"] = QVariant::fromValue(invoke->preferredTarget);
        if (invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget))) {
            invoke->invoker->setMark("shanshi_invoke", 1);
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<ServerPlayer *> l = QList<ServerPlayer *>() << invoke->invoker << invoke->targets.first();
        room->sortByActionOrder(l);
        room->drawCards(l, 1, objectName());
        return false;
    }
};

ShuxinCard::ShuxinCard()
{
}

bool ShuxinCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void ShuxinCard::onEffect(const CardEffectStruct &effect) const
{
    bool hasBlack = false;
    Room *room = effect.to->getRoom();
    foreach (const Card *c, effect.to->getCards("hs")) {
        if (c->isBlack() && !effect.to->isJilei(c)) {
            hasBlack = true;
            break;
        }
    }
    QString pattern = hasBlack ? "@@shuxinVS!" : "@@shuxinVS";
    const Card *card = room->askForCard(effect.to, pattern, "@shuxin");
    if (hasBlack && card == NULL) {
        // force discard!!!
        QList<const Card *> hc = effect.to->getCards("hes");
        foreach (const Card *c, hc) {
            if (effect.to->isJilei(c) || !c->isBlack())
                hc.removeOne(c);
        }

        if (hc.length() > 0) {
            int x = qrand() % hc.length();
            const Card *c = hc.value(x);
            card = c;
            room->throwCard(c, effect.to);
        }
    }

    if (card) {
        if (card->subcardsLength() >= effect.to->getHp())
            room->recover(effect.to, RecoverStruct());
    } else if (!effect.to->isKongcheng()) {
        room->showAllCards(effect.to);
        room->getThread()->delay(1000);
        room->clearAG();
    }
}

class ShuxinVS : public ViewAsSkill
{
public:
    ShuxinVS()
        : ViewAsSkill("shuxinVS")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !Self->isJilei(to_select) && to_select->isBlack();
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@shuxinVS");
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

class Shuxin : public ZeroCardViewAsSkill
{
public:
    Shuxin()
        : ZeroCardViewAsSkill("shuxin")
    {
    }

    const Card *viewAs() const
    {
        return new ShuxinCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ShuxinCard");
    }
};

HuishengCard::HuishengCard()
{
    will_throw = false;
}

bool HuishengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QString cardname = Self->property("huisheng_card").toString();
    QString str = Self->property("huisheng_target").toString();
    Card *new_card = Sanguosha->cloneCard(cardname);
    DELETE_OVER_SCOPE(Card, new_card)
    new_card->setSkillName("huisheng");
    if (new_card->isKindOf("Peach"))
        return to_select->objectName() == str && new_card->isAvailable(to_select);
    if (new_card->targetFixed() && !targets.isEmpty())
        return false;
    if (targets.isEmpty() && to_select->objectName() != str)
        return false;
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool HuishengCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    QString cardname = Self->property("huisheng_card").toString();
    Card *new_card = Sanguosha->cloneCard(cardname);
    DELETE_OVER_SCOPE(Card, new_card)
    new_card->setSkillName("huisheng");

    if (targets.length() < 1)
        return false;
    return new_card && new_card->targetsFeasible(targets, Self);
}

const Card *HuishengCard::validate(CardUseStruct &card_use) const
{
    QString cardname = card_use.from->property("huisheng_card").toString();
    Card *card = Sanguosha->cloneCard(cardname);
    card->setSkillName("huisheng");
    return card;
}

class HuishengVS : public ZeroCardViewAsSkill
{
public:
    HuishengVS()
        : ZeroCardViewAsSkill("huisheng")
    {
        response_pattern = "@@huisheng";
    }

    virtual const Card *viewAs() const
    {
        return new HuishengCard;
    }
};

class Huisheng : public TriggerSkill
{
public:
    Huisheng()
        : TriggerSkill("huisheng")
    {
        events << CardFinished; // << PreCardUsed
        view_as_skill = new HuishengVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Jink") || use.from->hasFlag("Global_ProcessBroken") || !use.from->isAlive())
            return QList<SkillInvokeDetail>();
        if (use.from && use.to.length() == 1 && (use.card->isKindOf("BasicCard") || use.card->isNDTrick())) {
            ServerPlayer *source = use.to.first();
            if (use.from != source && source->hasSkill(this) && source->isAlive() && use.from->isAlive()) {
                Card *card = Sanguosha->cloneCard(use.card->objectName());
                DELETE_OVER_SCOPE(Card, card)
                if (!source->isCardLimited(card, Card::MethodUse) && !source->isProhibited(use.from, card))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, source, source);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        room->setTag("huisheng_use", data);
        CardUseStruct use = data.value<CardUseStruct>();
        Card *card = Sanguosha->cloneCard(use.card->objectName());

        QString prompt = "@huisheng-use:" + use.from->objectName() + ":" + card->objectName();
        room->setPlayerProperty(invoke->invoker, "huisheng_card", card->objectName());
        delete card;
        room->setPlayerProperty(invoke->invoker, "huisheng_target", use.from->objectName());
        invoke->invoker->tag["huisheng_target"] = QVariant::fromValue(use.from);
        room->askForUseCard(invoke->invoker, "@@huisheng", prompt);
        room->setPlayerProperty(invoke->invoker, "huisheng_target", QVariant());
        return false;
    }
};

class HuishengTargetMod : public TargetModSkill
{
public:
    HuishengTargetMod()
        : TargetModSkill("#huisheng_effect")
    {
        pattern = "BasicCard,TrickCard";
    }

    virtual int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->getSkillName() == "huisheng")
            return 1000;
        else
            return 0;
    }
};

class Yexiang : public TriggerSkill
{
public:
    Yexiang()
        : TriggerSkill("yexiang")
    {
        events << CardsMoveOneTime;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *victim = qobject_cast<ServerPlayer *>(move.from);
        QList<SkillInvokeDetail> d;
        if (victim != NULL && victim->isAlive() && move.from_places.contains(Player::PlaceDelayedTrick)) {
            bool can = false;
            foreach (int id, move.card_ids) {
                if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceDelayedTrick) {
                    WrappedCard *vs_card = Sanguosha->getWrappedCard(id);
                    if (vs_card->getSubtype() == "unmovable_delayed_trick") {
                        can = true;
                        break;
                    }
                }
            }
            if (!can)
                return d;
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p != victim && p->canDiscard(victim, "hs")) {
                    d << SkillInvokeDetail(this, p, p, NULL, false, victim);
                }
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        ServerPlayer *target = invoke->targets.first();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
        if (invoke->invoker->canDiscard(target, "hs")) {
            int card_id = room->askForCardChosen(player, target, "hs", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, target, player);
            if (invoke->invoker->canDiscard(target, "hs")) {
                int card_id = room->askForCardChosen(player, target, "hs", objectName(), false, Card::MethodDiscard);
                room->throwCard(card_id, target, player);
            }
        }
        return false;
    }
};

TH12Package::TH12Package()
    : Package("th12")
{
    General *byakuren = new General(this, "byakuren$", "xlc", 4);
    byakuren->addSkill(new Pudu);
    byakuren->addSkill(new Jiushu);
    byakuren->addSkill(new Fahua);

    General *nue = new General(this, "nue", "xlc", 3);
    nue->addSkill(new Weizhi);
    nue->addSkill(new Weizhuang);

    General *toramaru = new General(this, "toramaru", "xlc", 4);
    toramaru->addSkill(new Jinghua);
    toramaru->addSkill(new Weiguang);

    General *murasa = new General(this, "murasa", "xlc", 4);
    murasa->addSkill(new Shuinan);
    murasa->addSkill(new Nihuo);

    General *ichirin = new General(this, "ichirin", "xlc", 4);
    ichirin->addSkill(new Lizhi);
    ichirin->addSkill(new Yunshang);

    General *nazrin = new General(this, "nazrin", "xlc", 3);
    nazrin->addSkill(new Xunbao);
    nazrin->addSkill(new Lingbai);

    General *kogasa = new General(this, "kogasa", "xlc", 3);
    kogasa->addSkill(new Yiwang);
    kogasa->addSkill(new Jingxia);

    General *unzan = new General(this, "unzan", "xlc", 4, true);
    unzan->addSkill(new Bianhuan);
    unzan->addSkill(new Nuhuo);

    General *myouren = new General(this, "myouren", "xlc", 4, true);
    myouren->addSkill(new Shanshi);
    myouren->addSkill(new Shuxin);

    General *kyouko_sp = new General(this, "kyouko_sp", "xlc", 3);
    kyouko_sp->addSkill(new Huisheng);
    kyouko_sp->addSkill(new HuishengTargetMod);
    kyouko_sp->addSkill(new Yexiang);
    related_skills.insertMulti("huisheng", "#huisheng_effect");

    addMetaObject<PuduCard>();
    addMetaObject<WeizhiCard>();
    addMetaObject<NihuoCard>();
    addMetaObject<NuhuoCard>();
    addMetaObject<ShuxinCard>();
    addMetaObject<HuishengCard>();
    skills << new FahuaDistance << new ShuxinVS;
}

ADD_PACKAGE(TH12)
