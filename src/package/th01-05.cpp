#include "th01-05.h"
#include "general.h"
#include "skill.h"
#include "engine.h"

class Qianyi : public TriggerSkill
{
public:
    Qianyi() : TriggerSkill("qianyi")
    {
        events << EventPhaseStart;
    }

    static Player::Phase qianyiPhase(QString choice) {
        if (choice == "start")
            return Player::Start;
        else if (choice == "judge")
            return Player::Judge;
        else if (choice == "draw")
            return Player::Draw;
        else if (choice == "play")
            return Player::Play;
        else if (choice == "discard")
            return Player::Discard;
        else if (choice == "finish")
            return Player::Finish;
        return Player::Finish;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->getPhase() == Player::RoundStart) {
            QList<SkillInvokeDetail> d;
            foreach(ServerPlayer *p, room->getOtherPlayers(current)) {
                if (p->hasSkill(this) && !p->getCards("e").isEmpty())
                    d << SkillInvokeDetail(this, p, p, NULL, false, current);
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *target = invoke->preferredTarget;
        QString prompt = "@qianyi:" + target->objectName();
        const Card *card = room->askForCard(invoke->invoker, ".|.|.|equipped", prompt, data, Card::MethodNone);
        if (card) {
            const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
            int equipped_id = -1;
            if (target->getEquip(equip->location()) != NULL)
                equipped_id = target->getEquip(equip->location())->getEffectiveId();
            QList<CardsMoveStruct> exchangeMove;
            CardsMoveStruct move1(equip->getId(), target, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_TRANSFER, target->objectName()));

            exchangeMove.push_back(move1);
            if (equipped_id != -1) {
                CardsMoveStruct move2(equipped_id, NULL, Player::DiscardPile, CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, target->objectName()));
                exchangeMove.push_back(move2);
            }

            LogMessage zhijian;
            zhijian.type = "$ZhijianEquip";
            zhijian.from = target;
            zhijian.card_str = QString::number(equip->getId());
            room->sendLog(zhijian);

            room->moveCardsAtomic(exchangeMove, true);
        }
        return card != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        RecoverStruct recover;
        room->recover(invoke->invoker, recover);
        QString choice = room->askForChoice(invoke->invoker, objectName(), "start+judge+draw+play+discard+finish", data);
        Player::Phase phase = qianyiPhase(choice);
        if (!invoke->targets.first()->isSkipped(phase))
            invoke->targets.first()->skip(phase);
        return false;
    }
};

class Mengxiao : public TriggerSkill
{
public:
    Mengxiao() : TriggerSkill("mengxiao")
    {
        events << CardsMoveOneTime;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *kana = qobject_cast<ServerPlayer *>(move.from);
        if (kana != NULL && kana->hasSkill(this) && kana->isAlive() && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                foreach(const Card *c, p->getCards("ej")) {
                    foreach(int card_id, move.card_ids) {
                        if (Sanguosha->getCard(card_id)->getSuit() == c->getSuit())
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, kana, kana);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QList<ServerPlayer *> targets;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        foreach(ServerPlayer *p, room->getAllPlayers()) {
            foreach(const Card *c, p->getCards("ej")) {
                if (targets.contains(p))
                    continue;
                foreach(int card_id, move.card_ids) {
                    if (Sanguosha->getCard(card_id)->getSuit() == c->getSuit()) {
                        targets << p;
                        break;
                    }
                }
            }
        }
        invoke->invoker->tag["mengxiao"] = data;
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@" + objectName(), true, true);
        if (target)
            invoke->targets << target;
        return target != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *target = invoke->targets.first();
        QList<int> disable;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        foreach(const Card *c, target->getCards("ej")) {
            bool find = false;
            foreach(int card_id, move.card_ids) {
                if (Sanguosha->getCard(card_id)->getSuit() == c->getSuit()) {
                    find = true;
                    break;
                }
            }
            if (!find)
                disable << c->getId();
        }

        int card_id = room->askForCardChosen(invoke->invoker, target, "je", objectName(), false, Card::MethodNone, disable);
        room->obtainCard(invoke->invoker, card_id);

        return false;
    }

};



TH0105Package::TH0105Package()
    : Package("th0105")
{
    General *shinki = new General(this, "shinki$", "pc98", 4, false);
    Q_UNUSED(shinki);
    General *alice = new General(this, "alice_old", "pc98", 4, false);
    Q_UNUSED(alice);
    General *yuka = new General(this, "yuka_old$", "pc98", 4, false);
    Q_UNUSED(yuka)
    General *gengetsumugetsu = new General(this, "gengetsumugetsu", "pc98", 4, false);
    Q_UNUSED(gengetsumugetsu);
    General *elly = new General(this, "elly", "pc98", 4, false);
    Q_UNUSED(elly);
    General *yumemi = new General(this, "yumemi$", "pc98", 4, false);
    Q_UNUSED(yumemi);
    General *chiyuri = new General(this, "chiyuri", "pc98", 4, false);
    Q_UNUSED(chiyuri);
    General *rikako = new General(this, "rikako", "pc98", 4, false);
    Q_UNUSED(rikako);
    General *kana = new General(this, "kana", "pc98", 3, false);
    kana->addSkill(new Qianyi);
    kana->addSkill(new Mengxiao);


    General *mima = new General(this, "mima$", "pc98", 4, false);
    Q_UNUSED(mima);
}

ADD_PACKAGE(TH0105)
