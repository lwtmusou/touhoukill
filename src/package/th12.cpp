#include "th12.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"
#include "maneuvering.h"

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
    Pudu() : ZeroCardViewAsSkill("pudu")
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
    Jiushu() : TriggerSkill("jiushu")
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
    Fahua() : TriggerSkill("fahua$")
    {
        events << TargetConfirming;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!(use.card->isKindOf("TrickCard") && use.from != NULL))
            return QList<SkillInvokeDetail>();
        QList<ServerPlayer *> byakurens;
        foreach (ServerPlayer *p, use.to) {
            if (p->hasLordSkill(this) && p != use.from) {
                byakurens << p;
            }
        }

        use.card->setFlags("fahua");
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, byakurens) {
            bool flag = false;
            foreach (ServerPlayer *q, room->getLieges("xlc", p)) {
                if (q == use.from)
                    continue;
                if (use.to.contains(q) || use.from->isProhibited(q, use.card))
                    continue;
                if (!use.card->targetFilter(QList<const Player *>(), q, use.from))
                    continue;
                d << SkillInvokeDetail(this, p, p);
                flag = true;
                break;
            }
            if (flag)
                continue;
        }
        use.card->setFlags("-fahua");
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();

        use.card->setFlags("fahua");
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getLieges("xlc", invoke->invoker)) {
            if (p == use.from)
                continue;
            if (use.to.contains(p) || use.from->isProhibited(p, use.card)) {
                continue;
            }
            if (!use.card->targetFilter(QList<const Player *>(), p, use.from))
                continue;
            targets << p;
        }
        use.card->setFlags("-fahua");

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

                if (use.card->isKindOf("DelayedTrick")) {
                    CardsMoveStruct move;
                    move.card_ids << use.card->getId();
                    move.to_place = Player::PlaceDelayedTrick;
                    move.to = p;
                    room->moveCardsAtomic(move, true);
                } else if (use.card->isKindOf("Collateral")) {
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
                }
                break;
            }
        }
        return false;
    }
};

class FahuaDistance : public TargetModSkill
{
public:
    FahuaDistance() : TargetModSkill("fahua-dist")
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
    Weizhi() : ViewAsSkill("weizhi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return  !player->hasUsed("WeizhiCard") && !player->isNude();
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
    Weizhuang() : TriggerSkill("weizhuang")
    {
        frequency = Compulsory;
        events << TargetConfirming;
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



class Zhengyi : public TriggerSkill
{
public:
    Zhengyi() : TriggerSkill("zhengyi")
    {
        events << TargetConfirming;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isBlack() && (use.card->isNDTrick() || use.card->isKindOf("Slash"))) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, use.to) {
                if (p->hasSkill(this)) {
                    bool invoke = p->isKongcheng();
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
    Baota() : TriggerSkill("baota")
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
            if (player != NULL && player->hasSkill(this) && move.to_place == Player::DiscardPile) {
                foreach (int id, move.card_ids) {
                    if (room->getCardPlace(id) == Player::DiscardPile
                        && (move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceSpecial &&
                            move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceDelayedTrick))
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
                    && (move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceSpecial &&
                    move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceDelayedTrick))
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
    Shuinan() : TriggerSkill("shuinan")
    {
        events << TargetConfirming;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!(use.from != NULL && use.card->isNDTrick()))
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, use.to) {
            if (p->hasSkill(this) && use.from != p && p->canDiscard(use.from, "h"))
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
        int id = room->askForCardChosen(invoke->invoker, use.from, "h", objectName(), false, Card::MethodDiscard);
        room->throwCard(id, use.from, invoke->invoker);
        return false;
    }
};

NihuoCard::NihuoCard()
{

}
bool NihuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty()) return false;

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
    Nihuo() : ZeroCardViewAsSkill("nihuo")
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
    Lizhi() : TriggerSkill("lizhi")
    {
        events << DamageCaused;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);

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
    Yunshang() : TriggerSkill("yunshang")
    {
        frequency = Compulsory;
        events << TargetConfirming;
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
    Souji() : TriggerSkill("souji")
    {
        events << CardsMoveOneTime << BeforeCardsMove;
        frequency = Frequent;
    }

#pragma message WARN("todo_lwtmusou:while rewrite, need consider how to determine move.from like these cases")
//case1.1 while provideCard went to discar pile , like skill Tianren
    //if the provider is self,it should not trigger skill
//case2 while retrial card went to discard pile, we consdier the move.from as the responser, not judge.who!!!   
    //if responser(move.from) is self, it could not trigger skill.
//case 3 if the card which went to discardpile was from private pile, like woodenOx,it should not trigger skill.
/*

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *nazurin, QVariant &data) const
    {
        if (!nazurin->hasSkill(objectName())) return;
        if (!nazurin->isCurrent()) return;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        //record room Tag("UseOrResponseFromPile") 
        if (triggerEvent == BeforeCardsMove) { //record origin_from_places?
            if (move.to_place != Player::PlaceTable)
                return;
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE
                || (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE) {
                QVariantList record_ids = room->getTag("UseOrResponseFromPile").toList();
                foreach (int id, move.card_ids) {
                    if (move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceHand && move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceEquip) {
                        if (!record_ids.contains(id))
                            record_ids << id;
                    }
                }
                room->setTag("UseOrResponseFromPile", record_ids);
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            if (nazurin->isCurrent() && move.from  && move.from != nazurin
                && move.to_place == Player::DiscardPile) {
                QVariantList obtain_ids;
                foreach (int id, move.card_ids) {
                    if (room->getCardPlace(id) != Player::DiscardPile)
                        continue;

                    switch (move.from_places.at(move.card_ids.indexOf(id))) {
                        case Player::PlaceHand: obtain_ids << id; break;
                        case Player::PlaceEquip: obtain_ids << id; break;
                        case Player::PlaceJudge: obtain_ids << id; break;
                        case Player::PlaceTable:
                        {
                            QVariantList record_ids = room->getTag("UseOrResponseFromPile").toList();
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
            // ******************************************************************************************            
                        // delete record: UseOrResponseFromPile
            QVariantList record_ids = room->getTag("UseOrResponseFromPile").toList();
            foreach (int id, move.card_ids) {
                if (record_ids.contains(id))
                    record_ids.removeOne(id);
            }
            room->setTag("UseOrResponseFromPile", record_ids);
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == CardsMoveOneTime) { // record?
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (player->isCurrent() && move.from  && move.from != player
                && move.to_place == Player::DiscardPile) {
                QVariantList obtain_ids = player->tag["souji"].toList();
                if (obtain_ids.length() > 0)
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (room->askForSkillInvoke(player, objectName(), data))
            return true;
        player->tag.remove("souji");
        return false;
    }


    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QVariantList ids = player->tag["souji"].toList();
        player->tag.remove("souji");

        QList<int> obtain_ids;
        foreach (QVariant record_id, ids)
            obtain_ids << record_id.toInt();

        CardsMoveStruct mo;
        mo.card_ids = obtain_ids;
        mo.to = player;
        mo.to_place = Player::PlaceHand;
        room->moveCardsAtomic(mo, true);
        return false;
    }*/
};



class Tansuo : public TriggerSkill
{
public:
    Tansuo() : TriggerSkill("tansuo")
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



class Yiwang : public TriggerSkill
{
public:
    Yiwang() : TriggerSkill("yiwang")
    {
        events << CardsMoveOneTime;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *ko = qobject_cast<ServerPlayer *>(move.from);
        if (ko != NULL && ko->hasSkill(this) && move.from_places.contains(Player::PlaceEquip)) {
            bool wound = false;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->isWounded()) {
                    wound = true;
                    break;
                }
            }
            if (!wound)
                return QList<SkillInvokeDetail>();

            QList<SkillInvokeDetail> d;
            foreach (Player::Place place, move.from_places) {
                if (place == Player::PlaceEquip)
                    d << SkillInvokeDetail(this, ko, ko);
            }
            return d;
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
    Jingxia() : MasochismSkill("jingxia")
    {

    }

    QList<SkillInvokeDetail> triggerable(const Room *room, const DamageStruct &damage) const
    {
        bool invoke = false;
        do {
            if (damage.to->isAlive() && damage.to->hasSkill(this)) {
                if (damage.from != NULL && damage.to->canDiscard(damage.from, "he")) {
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
        if (damage.from != NULL && damage.to->canDiscard(damage.from, "he"))
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
                if (!player->canDiscard(damage.from, "he"))
                    return;
                int card_id = room->askForCardChosen(player, damage.from, "he", objectName(), false, Card::MethodDiscard);
                room->throwCard(card_id, damage.from, player);
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
            if (fieldcard2.length() == 0)
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
    Bianhuan() : TriggerSkill("bianhuan")
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
    Nuhuo() : ZeroCardViewAsSkill("nuhuo")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->deleteLater();
        if (!player->isCardLimited(slash, Card::MethodUse)
            && !player->hasUsed("NuhuoCard")) {
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
    Shanshi() : TriggerSkill("shanshi")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    p->setMark("shanshi_invoke", 0);
            }
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
        if ((move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip)) && (move.to != move.from || (move.to_place != Player::PlaceHand && move.to_place != Player::PlaceEquip))) {
            if (move.from->hasSkill(this) && move.from->getPhase() == Player::NotActive) {
                // you lose a card in other's round
                ServerPlayer *current = room->getCurrent();
                if (current == NULL || current->getPhase() == Player::NotActive || current->isDead())
                    return QList<SkillInvokeDetail>();

                ServerPlayer *myo = qobject_cast<ServerPlayer *>(move.from);
                if (myo == NULL || myo == current || myo->getMark("shanshi_invoke") > 0)
                    return QList<SkillInvokeDetail>();
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, myo, myo, NULL, false, current);
            } else if (move.from->getPhase() != Player::NotActive) {
                // others lose a card in your round
                ServerPlayer *myo = room->getCurrent();
                if (myo == NULL || myo->getPhase() == Player::NotActive || myo->isDead() || myo->getMark("shanshi_invoke") > 0)
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

bool ShuxinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (!targets.isEmpty())
        return false;

    if (to_select->isKongcheng())
        return false;

    return true;
}

void ShuxinCard::onEffect(const CardEffectStruct &effect) const
{
    QList<int> card_ids = effect.to->handCards();
    QList<int> selected;
    QList<int> disabled;
    Room *room = effect.to->getRoom();
    for (int i = 0; i < 2; ++i) {
        card_ids << disabled;
        disabled.clear();
        foreach (int id, card_ids) {
            const Card *c = Sanguosha->getCard(id);
            if (!effect.from->canDiscard(effect.to, id) || c == NULL || !c->isBlack()) {
                card_ids.removeOne(id);
                disabled << id;
            }
        }

        if (card_ids.length() == 0) {
            if (i == 0) // showcards
                room->showAllCards(effect.to, effect.from);
            break;
        }

        if (i == 0) {
            LogMessage log;
            log.type = "$ViewAllCards";
            log.from = effect.from;
            log.to << effect.to;
            log.card_str = IntList2StringList(effect.to->handCards()).join("+");
            room->doNotify(effect.from, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
        }

        room->fillAG(card_ids + disabled + selected, effect.from, disabled + selected);
        int id = room->askForAG(effect.from, card_ids, true, "shuxin");
        if (id == -1)
            break;
        room->clearAG(effect.from);
        card_ids.removeOne(id);
        selected << id;
    }
    if (selected.isEmpty())
        return;

    DummyCard dummy;
    dummy.addSubcards(selected);
    room->throwCard(&dummy, effect.to, effect.from);

    if (selected.length() == 1)
        return;

    const Card *card1 = Sanguosha->getCard(selected.first());
    const Card *card2 = Sanguosha->getCard(selected.last());
    if (card1 != NULL && card2 != NULL && card1->getSuit() != card2->getSuit())
        room->recover(effect.to, RecoverStruct());
}

class Shuxin : public ZeroCardViewAsSkill
{
public:
    Shuxin() : ZeroCardViewAsSkill("shuxin")
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


TH12Package::TH12Package()
    : Package("th12")
{
    General *byakuren = new General(this, "byakuren$", "xlc", 4, false);
    byakuren->addSkill(new Pudu);
    byakuren->addSkill(new Jiushu);
    byakuren->addSkill(new Fahua);

    General *nue = new General(this, "nue", "xlc", 3, false);
    nue->addSkill(new Weizhi);
    nue->addSkill(new Weizhuang);

    General *toramaru = new General(this, "toramaru", "xlc", 4, false);
    toramaru->addSkill(new Zhengyi);
    toramaru->addSkill(new Baota);

    General *murasa = new General(this, "murasa", "xlc", 4, false);
    murasa->addSkill(new Shuinan);
    murasa->addSkill(new Nihuo);

    General *ichirin = new General(this, "ichirin", "xlc", 4, false);
    ichirin->addSkill(new Lizhi);
    ichirin->addSkill(new Yunshang);

    General *nazrin = new General(this, "nazrin", "xlc", 3, false);
    nazrin->addSkill(new Souji);
    nazrin->addSkill(new Tansuo);

    General *kogasa = new General(this, "kogasa", "xlc", 3, false);
    kogasa->addSkill(new Yiwang);
    kogasa->addSkill(new Jingxia);

    General *unzan = new General(this, "unzan", "xlc", 4, true);
    unzan->addSkill(new Bianhuan);
    unzan->addSkill(new Nuhuo);

    General *myouren = new General(this, "myouren", "xlc", 4, false);
    myouren->addSkill(new Shanshi);
    myouren->addSkill(new Shuxin);

    addMetaObject<PuduCard>();
    addMetaObject<WeizhiCard>();
    addMetaObject<NihuoCard>();
    addMetaObject<NuhuoCard>();
    addMetaObject<ShuxinCard>();

    skills << new FahuaDistance;
}

ADD_PACKAGE(TH12)

