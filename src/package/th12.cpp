#include "th12.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"
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

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Finish && player->isWounded())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return room->askForSkillInvoke(player, "jiushu", data);
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->drawCards(player->getLostHp());
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


    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("TrickCard") && use.from != NULL &&  use.from != player && use.to.contains(player)
            && player->hasLordSkill(objectName())) {
            if (use.card->isKindOf("Lightning")) return QStringList();
            room->setCardFlag(use.card, "fahua"); //for rangefix in target filter
            foreach (ServerPlayer *p, room->getLieges("xlc", player)) {
                if (p == use.from)
                    continue;
                if (use.to.contains(p) || use.from->isProhibited(p, use.card)) {
                    continue;
                }
                if (!use.card->targetFilter(QList<const Player *>(), p, use.from))
                    continue;
                return QStringList(objectName());
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName(), data);
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {

        CardUseStruct use = data.value<CardUseStruct>();

        room->setCardFlag(use.card, "fahua"); //for rangefix in target filter
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getLieges("xlc", player)) {
            if (p == use.from)
                continue;
            if (use.to.contains(p) || use.from->isProhibited(p, use.card)) {
                continue;
            }
            if (!use.card->targetFilter(QList<const Player *>(), p, use.from))
                continue;
            targets << p;
        }


        room->setTag("fahua_target", QVariant::fromValue(player));
        room->setTag("fahua_use", data);
        foreach (ServerPlayer *p, targets) {
            QString prompt = "tricktarget:" + use.from->objectName() + ":" + player->objectName() + ":" + use.card->objectName();
            if (p->askForSkillInvoke("fahua_change", prompt)) {
                use.to << p;
                use.to.removeOne(player);
                data = QVariant::fromValue(use);

                QList<ServerPlayer *> logto;
                logto << player;
                room->touhouLogmessage("$CancelTarget", use.from, use.card->objectName(), logto);
                logto << p;
                logto.removeOne(player);
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
                room->getThread()->trigger(TargetConfirming, room, p, data);
                break;
            }
        }


        return false;
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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isNDTrick() && use.to.contains(player) && use.from != player)
                return QStringList(objectName());
        }
        return QStringList();
    }


    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetConfirming) {
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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isBlack() && (use.card->isNDTrick() || use.card->isKindOf("Slash"))
                && use.to.contains(player) && !player->isKongcheng())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            QString prompt = "@zhengyi:" + use.card->objectName();
            const Card *card = room->askForCard(player, ".|red|.|.", prompt, data, Card::MethodDiscard, NULL, false, objectName());
            if (card)
                return true;
            else
                return false;
        }
        return false;
    }
    virtual bool effect(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.nullified_list << player->objectName();
            data = QVariant::fromValue(use);
        }
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

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *toramaru, QVariant &data) const
    {
        if (!toramaru->hasSkill(objectName())) return;
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && move.from == toramaru && move.to_place == Player::DiscardPile) {
                QVariantList ids;
                foreach (int id, move.card_ids) {
                    if (room->getCardPlace(id) == Player::DiscardPile
                        && (move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceSpecial &&
                        move.from_places.at(move.card_ids.indexOf(id)) != Player::PlaceDelayedTrick))
                        ids << id;
                }
                toramaru->tag["baota"] = ids;
            }
        }
    }


    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Finish)
                return QStringList(objectName());
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && move.from == player && move.to_place == Player::DiscardPile) {
                QVariantList ids = player->tag["baota"].toList();
                if (!ids.isEmpty())
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }




    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QVariantList r_ids = player->tag["baota"].toList();
            QList<int> ids;
            foreach(QVariant record_id, r_ids)
                ids << record_id.toInt();

            room->fillAG(ids, player);
            int card_id = room->askForAG(player, ids, true, objectName());
            room->clearAG(player);
            if (card_id > -1) {
                LogMessage mes;
                mes.type = "$baota";
                mes.from = player;
                mes.arg = objectName();
                mes.card_str = Sanguosha->getCard(card_id)->toString();
                room->sendLog(mes);

                QList<int> buttom_ids;
                buttom_ids << card_id;
                room->moveCardsToEndOfDrawpile(buttom_ids, true);
            }


        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@baota", true, true);
            if (target) {
                CardsMoveStruct move;
                move.to = target;
                move.to_place = Player::PlaceHand;
                move.card_ids << room->drawCard(true);
                room->moveCardsAtomic(move, false);
            }
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

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from  && player != use.from  &&  use.to.contains(player) && use.card->isNDTrick()) {
            if (player->canDiscard(use.from, "h"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        player->tag["shuinan_use"] = data;
        return room->askForSkillInvoke(player, objectName(), QVariant::fromValue(use.from));
    }
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.from->objectName());
        int id = room->askForCardChosen(player, use.from, "h", objectName());
        room->throwCard(id, use.from, player);
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

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        player->tag["lizhi_damage"] = data;
        return player->askForSkillInvoke(objectName(), QVariant::fromValue(damage.to));
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->drawCards(2);
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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isNDTrick() && use.to.contains(player) && use.from != player && !use.from->inMyAttackRange(player))
                return QStringList(objectName());
        }
        return QStringList();
    }


    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.nullified_list << player->objectName();
            data = QVariant::fromValue(use);
            room->notifySkillInvoked(player, objectName());
        }
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
            //******************************************************************************************            
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
    }
};



class Tansuo : public TriggerSkill
{
public:
    Tansuo() : TriggerSkill("tansuo")
    {
        events << CardsMoveOneTime << EventPhaseEnd;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() != Player::Discard)
            return QStringList();
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && move.from == player) {
                if (move.to_place == Player::DiscardPile) {
                    int count = player->getMark("tansuo");
                    count = count + move.card_ids.length();
                    player->setMark("tansuo", count);
                }
            }
        }
        if (triggerEvent == EventPhaseEnd) {
            int count = player->getMark("tansuo");
            player->setMark("tansuo", 0); //need clear at EventPhaseChanging?
            if (count >= player->getHp())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName());
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (triggerEvent == EventPhaseEnd) {

            CardsMoveStruct move;
            move.to = player;
            move.to_place = Player::PlaceHand;
            for (int i = 1; i <= 2; i++)
                move.card_ids << room->drawCard(true);
            room->moveCardsAtomic(move, false);

        }
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

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from != NULL && move.from == player && move.from_places.contains(Player::PlaceEquip)) {
            foreach (Player::Place place, move.from_places) {
                if (place == Player::PlaceEquip) {
                    foreach (ServerPlayer *p, room->getAllPlayers()) {
                        if (p->isWounded())
                            return QStringList(objectName());
                    }
                }
            }
        }
        return QStringList();
    }


    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> wounded;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->isWounded())
                wounded << p;
        }

        ServerPlayer * recovertarget = room->askForPlayerChosen(player, wounded, objectName(), "@yiwang-recover", true, true);
        if (!recovertarget)
            return false;

        RecoverStruct recover2;
        recover2.who = player;
        room->recover(recovertarget, recover2);
        if (recovertarget != player)
            player->drawCards(1);
        return false;
    }
};


class Jingxia : public MasochismSkill
{
public:
    Jingxia() : MasochismSkill("jingxia")
    {

    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && player->canDiscard(damage.from, "he"))
            return QStringList(objectName());
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (player->canDiscard(p, "ej"))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const
    {
        Room *room = player->getRoom();
        for (int i = 0; i < damage.damage; i++) {
            QStringList choices;
            if (damage.from != NULL && player->canDiscard(damage.from, "he"))
                choices << "discard";
            QList<ServerPlayer *> fieldcard;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (player->canDiscard(p, "ej"))
                    fieldcard << p;
            }
            if (!fieldcard.isEmpty())
                choices << "discardfield";

            choices << "dismiss";

            player->tag["jingxia"] = QVariant::fromValue(damage);
            QString choice = room->askForChoice(player, objectName(), choices.join("+"), QVariant::fromValue(damage));
            player->tag.remove("jingxia");
            if (choice == "dismiss")
                return;

            room->touhouLogmessage("#InvokeSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            if (choice == "discard") {
                for (int i = 0; i < 2; i++) {
                    if (!player->canDiscard(damage.from, "he"))
                        return;
                    int card_id = room->askForCardChosen(player, damage.from, "he", objectName(), false, Card::MethodDiscard);
                    //objectName() + "-discard"
                    room->throwCard(card_id, damage.from, player);
                }
            } else {
                //local aidelay = sgs.GetConfig("AIDelay", 0)
                // sgs.SetConfig("AIDelay", 0)
                //        ---------------AIDELAY == 0-------------------
                ServerPlayer *player1 = room->askForPlayerChosen(player, fieldcard, objectName(), "@jingxia-discardfield");
                int card1 = room->askForCardChosen(player, player1, "ej", objectName(), false, Card::MethodDiscard);
                //objectName()+"-discardfield"
                //sgs.SetConfig("AIDelay", aidelay)
                //        ----------------------------------------------
                room->throwCard(card1, player1, player);
                //       ---------------AIDELAY == 0-------------------
                //sgs.SetConfig("AIDelay", 0)
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
                    //sgs.SetConfig("AIDelay", aidelay)
                    room->throwCard(card2, player2, player);
                }
                //       ----------------------------------------------
                //sgs.SetConfig("AIDelay", aidelay)
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

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return room->askForSkillInvoke(player, objectName(), data);
    }
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->loseMaxHp(player, 1);
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

    addMetaObject<PuduCard>();
    addMetaObject<WeizhiCard>();
    addMetaObject<NihuoCard>();
    addMetaObject<NuhuoCard>();
}

ADD_PACKAGE(TH12)

