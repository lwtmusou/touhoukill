#include "th17.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "skill.h"


class Shanlei : public TriggerSkill
{
public:
    Shanlei()
        : TriggerSkill("shanlei")
    {
        events << EventPhaseStart << EventPhaseChanging;
        frequency = NotCompulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *p = nullptr;

        if (e == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->isAlive() && player->hasSkill(this) && player->getPhase() == Player::RoundStart && player->getHandcardNum() > player->getMaxCards())
                p = player;
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct c = data.value<PhaseChangeStruct>();
            if (c.to == Player::NotActive && c.player->isAlive() && c.player->hasSkill(this)) {
                bool flag = false;
                foreach (ServerPlayer *p, room->getOtherPlayers(c.player)) {
                    if (p->getHandcardNum() >= c.player->getHandcardNum()) {
                        flag = true;
                        break;
                    }
                }
                if (flag)
                    p = c.player;
            }
        }

        QList<SkillInvokeDetail> r;
        if (p != nullptr)
            r << SkillInvokeDetail(this, p, p, nullptr, true);

        return r;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            // it is certain that p->getHandcardNum() > p->getMaxCards() equals true
            int n = invoke->invoker->getHandcardNum() - invoke->invoker->getMaxCards();
            room->askForDiscard(invoke->invoker, objectName(), n, n, false, false, "@shanlei-discard");
        } else {
            int cardnumMost = 0;
            foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
                if (p->getHandcardNum() >= cardnumMost)
                    cardnumMost = p->getHandcardNum();
            }
            int n = cardnumMost - invoke->invoker->getHandcardNum() + 1;
            room->drawCards(invoke->invoker, n, "shanlei");
        }

        return false;
    }
};

class BengluoVS : public ViewAsSkill
{
public:
    BengluoVS()
        : ViewAsSkill("bengluo")
    {
    }

    bool isResponseOrUse() const override
    {
        return Sanguosha->getCurrentCardUsePattern() == "@@bengluo-card1";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        if (Sanguosha->getCurrentCardUsePattern() == "@@bengluo-card1") {
            if (!selected.isEmpty())
                return false;
        } else {
            bool ok = false;
            if ((selected.length() >= Self->property("bengluoDiscardnum").toString().toInt(&ok)) || !ok)
                return false;

            if (Self->isJilei(to_select))
                return false;
        }
        return !to_select->isEquipped();
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (Sanguosha->getCurrentCardUsePattern() == "@@bengluo-card1") {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcards(cards);
            slash->setShowSkill("bengluo");
            slash->setSkillName("bengluo");
            return slash;
        } else {
            bool ok = false;
            if ((cards.length() == Self->property("bengluoDiscardnum").toString().toInt(&ok)) && ok) {
                DummyCard *card = new DummyCard;
                card->addSubcards(cards);
                return card;
            }
        }

        return nullptr;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        return pattern.startsWith("@@bengluo-card");
    }
};

class Bengluo : public TriggerSkill
{
public:
    Bengluo()
        : TriggerSkill("bengluo")
    {
        events << CardsMoveOneTime << EventPhaseChanging << EventPhaseStart << Damage;
        view_as_skill = new BengluoVS;
        global = true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from_places.contains(Player::PlaceHand) && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_USE)) {
                ServerPlayer *current = room->getCurrent();
                if (current != nullptr && current->isInMainPhase() && current->isAlive()) {
                    if (move.from->getHandcardNum() > move.from->getMaxCards())
                        move.from->setFlags("bengluo");
                }
            }
        } else if (triggerEvent == EventPhaseStart) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                p->setFlags("-bengluo");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Start || change.from == Player::Judge || change.from == Player::Draw || change.from == Player::Play || change.from == Player::Discard
                || change.from == Player::Finish) {
                QList<SkillInvokeDetail> r;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasSkill(this) && p->hasFlag("bengluo"))
                        r << SkillInvokeDetail(this, p, p);
                }
                return r;
            }
        } else if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != nullptr && damage.card->isKindOf("Slash") && damage.card->getSkillName() == "bengluo" && damage.by_user && !damage.chain && !damage.transfer
                && damage.from != nullptr && damage.from->isAlive() && damage.from->hasSkill(this))
                return {SkillInvokeDetail(this, damage.from, damage.from)};
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging)
            return room->askForUseCard(invoke->invoker, "@@bengluo-card1", "@bengluo-kill");
        else {
            int n = invoke->invoker->getHandcardNum() - invoke->invoker->getMaxCards();
            if (n > 0) {
                room->setPlayerProperty(invoke->invoker, "bengluoDiscardnum", QString::number(n));
                return room->askForCard(invoke->invoker, "@@bengluo-card2", "@bengluo-discard:::" + QString::number(n), data);
            } else {
                if (room->askForSkillInvoke(invoke->invoker, this, data, "@bengluo-draw:::" + QString::number(-n))) {
                    if (n != 0)
                        room->drawCards(invoke->invoker, -n, "bengluo");
                    return true;
                }
            }
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            damage.damage += 1;
            data = QVariant::fromValue<DamageStruct>(damage);
        }

        return false;
    }
};

TH17Package::TH17Package()
    : Package("th17")
{
    General *keiki = new General(this, "keiki$", "gxs");
    Q_UNUSED(keiki);

    General *eika = new General(this, "eika", "gxs", 3);
    eika->addSkill(new Shanlei);
    eika->addSkill(new Bengluo);

    General *urumi = new General(this, "urumi", "gxs");
    Q_UNUSED(urumi);

    General *kutaka = new General(this, "kutaka", "gxs");
    Q_UNUSED(kutaka);

    General *yachie = new General(this, "yachie", "gxs");
    Q_UNUSED(yachie);

    General *mayumi = new General(this, "mayumi", "gxs");
    Q_UNUSED(mayumi);

    General *saki = new General(this, "saki", "gxs");
    Q_UNUSED(saki);
}

ADD_PACKAGE(TH17)
