#include "th17.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "skill.h"

class Zaoxing : public TriggerSkill
{
public:
    Zaoxing()
        : TriggerSkill("zaoxing")
    {
        events << TurnStart << EventPhaseStart << CardUsed << CardResponded;
    }

    // NOTE: Numbers of @zaoxing and options should match Card::Suit
    static const QStringList options;

    void record(TriggerEvent e, Room *r, QVariant &d) const override
    {
        if (e == TurnStart) {
            ServerPlayer *p = d.value<ServerPlayer *>();
            p->setMark("zaoxingBackup", p->getMark("@zaoxing"));
            if (p->getMark("@zaoxing") > 0)
                r->setPlayerMark(p, "@zaoxing", 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;

        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->getPhase() == Player::RoundStart && p->isAlive() && p->hasSkill(this))
                r << SkillInvokeDetail(this, p, p);
        } else if ((triggerEvent == CardUsed) || (triggerEvent == CardResponded)) {
            const Card *c = nullptr;
            ServerPlayer *from = nullptr;
            if (triggerEvent == CardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                from = use.from;
                c = use.card;
            } else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse) {
                    from = resp.m_from;
                    c = resp.m_card;
                }
            }

            if ((c != nullptr) && (c->getTypeId() != Card::TypeSkill) && (from != nullptr)) {
                foreach (ServerPlayer *zaoxing, room->getAlivePlayers()) {
                    if (zaoxing->getMark("@zaoxing") == 0)
                        continue;
                    if (zaoxing == from)
                        continue;
                    if (zaoxing->getMark("@zaoxing") - 1 == static_cast<int>(c->getSuit())) {
                        SkillInvokeDetail invoke(this, zaoxing, zaoxing, from, true, nullptr, false);
                        invoke.tag["zaoxing"] = QVariant::fromValue<const Card *>(c);
                        r << invoke;
                    }
                }
            }
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
                QStringList optionsThistime = options;
                if (invoke->invoker->getMark("zaoxingBackup") > 0)
                    optionsThistime.removeAt(invoke->invoker->getMark("zaoxingBackup") - 1);
                QString choice = room->askForChoice(invoke->invoker, "zaoxing", optionsThistime.join("+"));
                int index = optionsThistime.indexOf(choice);
                if (index == -1)
                    choice = optionsThistime.first();
                index = options.indexOf(choice);
                invoke->tag["zaoxing"] = index;
                return true;
            }
        } else {
            LogMessage l;
            l.type = "#TouhouBuff";
            l.from = invoke->invoker;
            l.arg = objectName();
            room->sendLog(l);
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart)
            room->setPlayerMark(invoke->invoker, "@zaoxing", invoke->tag.value("zaoxing").toInt() + 1);
        else {
            static const QStringList patterns {"..S", "..C", "..H", "..D"};
            const Card *c = invoke->tag.value("zaoxing").value<const Card *>();
            if (c == nullptr)
                return false;
            const Card *c2 = nullptr;
            if (!invoke->targets.first()->isNude())
                c2 = room->askForCard(
                    invoke->targets.first(), patterns.at(static_cast<int>(c->getSuit())),
                    QString(QStringLiteral("@zaoxing-discard:%1::%2:%3")).arg(invoke->invoker->objectName(), c->objectName(), options.at(static_cast<int>(c->getSuit()))), data,
                    Card::MethodNone);

            if (c2 == nullptr) {
                LogMessage l;
                l.type = "#shenwei";
                l.from = invoke->targets.first();
                l.arg = objectName();
                l.arg2 = c->objectName();

                if (triggerEvent == CardUsed) {
                    CardUseStruct use = data.value<CardUseStruct>();
                    use.nullified_list << "_ALL_TARGETS";
                    data = QVariant::fromValue<CardUseStruct>(use);
                } else {
                    CardResponseStruct resp = data.value<CardResponseStruct>();
                    resp.m_isNullified = true;
                    data = QVariant::fromValue<CardResponseStruct>(resp);
                }
                if (room->getCardPlace(c->getEffectiveId()) == Player::PlaceTable)
                    room->obtainCard(invoke->invoker, c);
            } else
                room->obtainCard(invoke->invoker, c2);
        }

        return false;
    }
};

const QStringList Zaoxing::options {
    "Spade",
    "Club",
    "Heart",
    "Diamond",
};

// TODO: SIMPLIFY Lingshou

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

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
            if (invoke->invoker->hasShownSkill(this)) {
                LogMessage l;
                l.type = "#TriggerSkill";
                l.from = invoke->invoker;
                l.arg = objectName();
                room->sendLog(l);
            }
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    bool isEnabledAtPlay(const Player *) const override
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const override
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
        events << CardsMoveOneTime << EventPhaseChanging << EventPhaseStart << DamageCaused;
        view_as_skill = new BengluoVS;
        global = true;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if ((move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_USE)) {
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
        } else if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != nullptr && damage.card->isKindOf("Slash") && damage.card->getSkillName() == "bengluo" && damage.by_user && !damage.chain && !damage.transfer
                && damage.from != nullptr && damage.from->isAlive() && damage.from->hasSkill(this) && damage.to->getHandcardNum() < damage.from->getHandcardNum()
                && damage.from->getHandcardNum() != damage.from->getMaxCards())
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
                return room->askForCard(invoke->invoker, "@@bengluo-card2", "@bengluo-discard:::" + QString::number(n), data, "bengluo");
            } else {
                if (room->askForSkillInvoke(invoke->invoker, this, data, "@bengluo-draw:::" + QString::number(-n))) {
                    room->drawCards(invoke->invoker, -n, "bengluo");
                    return true;
                }
            }
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *r, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();

            LogMessage l;
            l.type = "#mofa_damage";
            l.from = damage.from;
            l.to << damage.to;
            l.arg = QString::number(damage.damage + 1);
            l.arg2 = QString::number(damage.damage);
            r->sendLog(l);

            damage.damage += 1;
            data = QVariant::fromValue<DamageStruct>(damage);
        }

        return false;
    }
};

LunniCard::LunniCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void LunniCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *current = room->getCurrent();

    Q_ASSERT(current != nullptr);
    Q_ASSERT((current->isAlive() && current->getPhase() == Player::RoundStart));

    use.to << current;
    SkillCard::onUse(room, use);
}

void LunniCard::onEffect(const CardEffectStruct &effect) const
{
    const EquipCard *c = qobject_cast<const EquipCard *>(Sanguosha->getCard(effect.card->getSubcards().first())->getRealCard());
    if (c == nullptr)
        return;

    EquipCard::Location location = c->location();

    int equipped_id = Card::S_UNKNOWN_CARD_ID;
    ServerPlayer *target = effect.to;
    Room *room = target->getRoom();
    if (target->getEquip(location))
        equipped_id = target->getEquip(location)->getEffectiveId();

    QList<CardsMoveStruct> exchangeMove;
    CardsMoveStruct move1(subcards.first(), target, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_PUT, target->objectName()));
    exchangeMove.push_back(move1);
    if (equipped_id != Card::S_UNKNOWN_CARD_ID) {
        CardsMoveStruct move2(equipped_id, nullptr, Player::DiscardPile, CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, target->objectName()));
        exchangeMove.push_back(move2);
    }
    LogMessage log;
    log.from = target;
    log.type = "$Install";
    log.card_str = QString::number(subcards.first());
    room->sendLog(log);

    room->moveCardsAtomic(exchangeMove, true);

    effect.to->setFlags("lunni");

    QVariantList lunniOwners;
    if (effect.to->tag.contains("lunniOwners"))
        lunniOwners = effect.to->tag.value("lunniOwners").toList();
    lunniOwners << QVariant::fromValue<ServerPlayer *>(effect.from);
    effect.to->tag["lunniOwners"] = lunniOwners;
}

class LunniVS : public OneCardViewAsSkill
{
public:
    LunniVS()
        : OneCardViewAsSkill("lunni")
    {
        filter_pattern = "EquipCard";
        response_pattern = "@@lunni";
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        LunniCard *c = new LunniCard;
        c->addSubcard(originalCard);
        c->setShowSkill("lunni");
        return c;
    }
};

class Lunni : public TriggerSkill
{
public:
    Lunni()
        : TriggerSkill("lunni")
    {
        events << EventPhaseStart << EventPhaseChanging << CardsMoveOneTime;
        view_as_skill = new LunniVS;
    }

    void record(TriggerEvent e, Room *, QVariant &d) const override
    {
        if (e == CardsMoveOneTime) {
            CardsMoveOneTimeStruct s = d.value<CardsMoveOneTimeStruct>();
            if (s.from != nullptr && s.from->getPhase() == Player::Discard && ((s.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                && s.from->hasFlag("lunni")) {
                QList<int> l;
                if (s.from->tag.contains("lunni"))
                    l = VariantList2IntList(s.from->tag.value("lunni").toList());

                l << s.card_ids;
                s.from->tag["lunni"] = IntList2VariantList(l);
            }
        } else if (e == EventPhaseStart) {
            ServerPlayer *p = d.value<ServerPlayer *>();
            if (p->getPhase() == Player::NotActive) {
                p->tag.remove("lunni");
                p->tag.remove("lunniOwners");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;

        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->getPhase() == Player::RoundStart) {
                QList<ServerPlayer *> players = room->getOtherPlayers(p);
                foreach (ServerPlayer *otherp, players) {
                    if (otherp->isAlive() && otherp->hasSkill(this) && !otherp->isAllNude())
                        r << SkillInvokeDetail(this, otherp, otherp, p);
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->hasFlag("lunni")) {
                if (change.from == Player::Play && !change.player->getCards("e").isEmpty()) {
                    ServerPlayer *p = change.player->tag.value("lunniOwners").toList().first().value<ServerPlayer *>();
                    r << SkillInvokeDetail(this, p, change.player, nullptr, true, nullptr, false);
                } else if (change.from == Player::Discard && change.player->tag.contains("lunni")) {
                    QList<int> l = VariantList2IntList(change.player->tag.value("lunni").toList());
                    QList<int> equipsindiscard;
                    foreach (int id, l) {
                        if (Sanguosha->getCard(id)->getTypeId() == Card::TypeEquip && room->getCardPlace(id) == Player::DiscardPile)
                            equipsindiscard << id;
                    }

                    if (equipsindiscard.isEmpty())
                        return r;

                    QVariantList lunniOwners = change.player->tag.value("lunniOwners").toList();
                    SkillInvokeDetail detail(this, nullptr, nullptr, change.player, false, nullptr, false);
                    detail.tag["lunni"] = IntList2VariantList(equipsindiscard);
                    foreach (const QVariant &ownerv, lunniOwners) {
                        detail.owner = detail.invoker = ownerv.value<ServerPlayer *>();
                        if (detail.owner->isAlive())
                            r << SkillInvokeDetail(detail);
                    }
                }
            }
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart)
            return room->askForUseCard(invoke->invoker, "@@lunni", "@lunni-discard:" + invoke->targets.first()->objectName(), -1, Card::MethodNone);
        else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->hasFlag("lunni")) {
                if (change.from == Player::Play && invoke->invoker == change.player)
                    return true;
                else if (change.from == Player::Discard) {
                    QList<int> l = VariantList2IntList(invoke->tag.value("lunni").toList());
                    QList<int> equipsindiscard;
                    foreach (int id, l) {
                        if (Sanguosha->getCard(id)->getTypeId() == Card::TypeEquip && room->getCardPlace(id) == Player::DiscardPile)
                            equipsindiscard << id;
                    }

                    if (equipsindiscard.isEmpty())
                        return false;

                    invoke->tag["lunni"] = IntList2VariantList(equipsindiscard);
                    return true;
                }
            }
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->hasFlag("lunni")) {
                LogMessage l;
                l.type = "#lunni-eff" + QString::number(static_cast<int>(change.from));
                l.from = change.player;
                room->sendLog(l);

                if (change.from == Player::Play) {
                    DummyCard d;
                    d.addSubcards(change.player->getCards("e"));
                    change.player->obtainCard(&d);
                } else {
                    QList<int> l = VariantList2IntList(invoke->tag.value("lunni").toList());
                    QList<int> equipsindiscard;
                    foreach (int id, l) {
                        if (Sanguosha->getCard(id)->getTypeId() == Card::TypeEquip && room->getCardPlace(id) == Player::DiscardPile)
                            equipsindiscard << id;
                    }

                    if (equipsindiscard.isEmpty())
                        return false;

                    room->fillAG(equipsindiscard, invoke->invoker);
                    int id = room->askForAG(invoke->invoker, equipsindiscard, true, "lunni");
                    room->clearAG(invoke->invoker);
                    if (id != -1)
                        room->obtainCard(invoke->invoker, id);
                }
            }
        }

        return false;
    }
};

class Quangui : public TriggerSkill
{
public:
    Quangui()
        : TriggerSkill("quangui")
    {
        events << EnterDying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;

        DyingStruct dying = data.value<DyingStruct>();
        if (dying.damage != nullptr && dying.damage->card != nullptr && dying.damage->damage > 1 && !dying.who->isAllNude()) {
            auto players = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *p, players) {
                if (p->isAlive())
                    r << SkillInvokeDetail(this, p, p, dying.who);
            }
        }

        return r;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (TriggerSkill::cost(EnterDying, room, invoke, data)) {
            int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hesj", "quangui");
            if (id == -1) {
                auto cards = invoke->invoker->getCards("hesj");
                id = cards.at(qrand() % cards.length())->getEffectiveId();
            }

            const Card *c = Sanguosha->getCard(id);
            invoke->tag["quangui"] = static_cast<int>(c->getTypeId());
            room->showCard(invoke->targets.first(), id);
            room->obtainCard(invoke->invoker, id);

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        bool ok = false;
        Card::CardType id = static_cast<Card::CardType>(invoke->tag.value("quangui").toInt());
        if (ok && id == Card::TypeEquip) {
            RecoverStruct recover;
            recover.recover = invoke->targets.first()->dyingThreshold() - invoke->targets.first()->getHp() + 1;
            room->recover(invoke->targets.first(), recover);
            return invoke->targets.first()->getHp() > invoke->targets.first()->dyingThreshold();
        }

        return false;
    }
};

class Yvshou : public TriggerSkill
{
public:
    Yvshou()
        : TriggerSkill("yvshou")
    {
        events << CardUsed << CardResponded << EventPhaseStart << TurnStart;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const override
    {
        if (triggerEvent == TurnStart) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                p->setMark("yvshou", 0);
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;

        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current->isAlive() && current->getPhase() == Player::RoundStart) {
                foreach (ServerPlayer *p, room->getOtherPlayers(current)) {
                    if (p->isAlive() && p->hasSkill(this) && !p->isKongcheng())
                        r << SkillInvokeDetail(this, p, p, current);
                }
            }
        } else if ((triggerEvent == CardUsed) || (triggerEvent == CardResponded)) {
            const Card *card = nullptr;
            ServerPlayer *from = nullptr;
            if (triggerEvent == CardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                card = use.card;
                from = use.from;
            } else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse) {
                    card = resp.m_card;
                    from = resp.m_from;
                }
            }

            if (card != nullptr && card->getNumber() > 0 && card->getTypeId() != Card::TypeSkill && from != nullptr && !from->hasFlag("yvshouFirst")) {
                SkillInvokeDetail d(this, nullptr, nullptr, from, true);
                d.tag["yvshou"] = QVariant::fromValue<const Card *>(card);
                foreach (ServerPlayer *p, room->getOtherPlayers(from)) {
                    if (p->isAlive() && p->getMark("yvshou") > 0 && p->hasSkill(this)) {
                        d.invoker = p;
                        d.owner = p;
                        r << d;
                    }
                }
            }
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            const Card *c = room->askForCard(invoke->invoker, "..", "@yvshou-discard:" + invoke->targets.first()->objectName(), data, "yvshou");
            if (c != nullptr) {
                invoke->tag["yvshou"] = c->getEffectiveId();
                return true;
            }
        } else {
            invoke->targets.first()->setFlags("yvshouFirst");
            return TriggerSkill::cost(triggerEvent, room, invoke, data);
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            invoke->invoker->setMark("yvshou", Sanguosha->getCard(invoke->tag["yvshou"].toInt())->getNumber());
        } else {
            int mark = invoke->invoker->getMark("yvshou");
            const Card *c = invoke->tag.value("yvshou").value<const Card *>();
            int number = c->getNumber();

            LogMessage l;
            l.from = invoke->targets.first();
            l.arg = objectName();
            l.arg2 = c->objectName();

            if (number <= mark) {
                if (triggerEvent == CardUsed) {
                    CardUseStruct use = data.value<CardUseStruct>();
                    if (use.card->isKindOf("EquipCard")) {
                        l.type = "#XushiHegemonySkillAvoid";
                        use.to.clear();
                    } else {
                        l.type = "#shenwei";
                        use.nullified_list << "_ALL_TARGETS";
                    }
                    data = QVariant::fromValue<CardUseStruct>(use);
                } else {
                    CardResponseStruct resp = data.value<CardResponseStruct>();
                    resp.m_isNullified = true;
                    data = QVariant::fromValue<CardResponseStruct>(resp);
                    l.type = "#shenwei";
                }
            } else {
                l.type = "#fuchou";
                if (triggerEvent == CardUsed) {
                    CardUseStruct use = data.value<CardUseStruct>();
                    if (use.m_addHistory) {
                        room->addPlayerHistory(invoke->targets.first(), c->getClassName(), -1);
                        use.m_addHistory = false;
                        data = QVariant::fromValue<CardUseStruct>(use);
                    }
                }
            }

            room->sendLog(l);
        }

        return false;
    }
};

class LingduVS : public OneCardViewAsSkill
{
public:
    LingduVS()
        : OneCardViewAsSkill("lingdu")
    {
        expand_pile = "#judging_area";
        response_pattern = "@@lingdu";
    }

    bool viewFilter(const Card *) const override
    {
        return true;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        DummyCard *d = new DummyCard({originalCard->getEffectiveId()});
        return d;
    }
};

class Lingdu : public TriggerSkill
{
public:
    Lingdu()
        : TriggerSkill("lingdu")
    {
        events << CardsMoveOneTime;
        view_as_skill = new LingduVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *current = room->getCurrent();
        if (current == nullptr || current->isDead() || current->getPhase() == Player::NotActive || current->hasFlag("lingdu"))
            return {};

        // move.from is filled with card user if the move is because of Use or Response.
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == nullptr || !move.from->hasSkill(this) || move.from->isDead() || (move.to_place != Player::DiscardPile))
            return {};

        QList<int> card_ids;

        for (int i = 0; i < move.card_ids.length(); ++i) {
            if (room->getCardPlace(move.card_ids.at(i)) != Player::DiscardPile)
                continue;

            Player::Place fromPlace = move.from_places.at(i);
            switch (fromPlace) {
            case Player::PlaceHand:
            case Player::PlaceEquip:
            case Player::PlaceDelayedTrick: {
                card_ids << move.card_ids.at(i);
                break;
            }
            case Player::PlaceTable: {
                if (((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE)
                    || ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE))
                    card_ids << move.card_ids.at(i);
                break;
            }
            default: {
                break;
            }
            }
        }

        if (card_ids.isEmpty())
            return {};

        ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
        if (from == nullptr || from->getPhase() != Player::NotActive)
            return {};

        SkillInvokeDetail d(this, from, from);
        d.tag["lingdu"] = IntList2VariantList(card_ids);
        return {d};
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        const Card *l = room->askForCard(invoke->invoker, "@@lingdu", "@lingdu-discard", invoke->tag.value("lingdu"), Card::MethodNone, nullptr, false, "lingdu");
        if (l != nullptr) {
            // need #InvokeSkill log? waiting for test result

            room->getCurrent()->setFlags("lingdu");
            QList<int> card_ids = VariantList2IntList(invoke->tag.value("lingdu").toList());
            int id = -1;
            if (card_ids.length() == 1)
                id = card_ids.first();
            else {
                room->fillAG(card_ids, invoke->invoker);
                id = room->askForAG(invoke->invoker, card_ids, false, "lingdu");
                room->clearAG(invoke->invoker);
                if (id == -1)
                    id = card_ids.first();
            }

            bool isLastCard = false;
            switch (room->getCardPlace(l->getEffectiveId())) {
            case Player::PlaceHand: {
                isLastCard = (invoke->invoker->getHandcardNum() == 1 && invoke->invoker->getHandcards().first()->getEffectiveId() == l->getEffectiveId());
                break;
            }
            case Player::PlaceEquip: {
                isLastCard = (invoke->invoker->getEquips().length() == 1 && invoke->invoker->getEquips().first()->getEffectiveId() == l->getEffectiveId());
                break;
            }
            case Player::PlaceDelayedTrick: {
                isLastCard = (invoke->invoker->getJudgingAreaID().length() == 1 && invoke->invoker->getJudgingAreaID().first() == l->getEffectiveId());
                break;
            }
            default: {
                break;
            }
            }
            invoke->tag["card1"] = l->getEffectiveId();
            invoke->tag["card2"] = id;
            invoke->tag["isLast"] = isLastCard;

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        bool ok1 = false;
        bool ok2 = false;
        bool isLastCard = invoke->tag.value("isLast").toBool();
        int card1 = invoke->tag.value("card1").toInt(&ok1);
        int card2 = invoke->tag.value("card2").toInt(&ok2);

        if (ok1 && ok2) {
            CardsMoveStruct move1;
            move1.card_ids << card1;
            move1.to = nullptr;
            move1.to_place = Player::DiscardPile;
            move1.reason.m_reason = CardMoveReason::S_REASON_PUT;
            move1.reason.m_playerId = invoke->invoker->objectName();
            move1.reason.m_skillName = "lingdu";
            CardsMoveStruct move2;
            move2.card_ids << card2;
            move2.to = invoke->invoker;
            move2.to_place = Player::PlaceHand;
            move2.reason.m_reason = CardMoveReason::S_REASON_RECYCLE;
            move2.reason.m_playerId = invoke->invoker->objectName();
            move2.reason.m_skillName = "lingdu";

            room->moveCardsAtomic({move1, move2}, true);

            if (isLastCard)
                invoke->invoker->drawCards(1, "lingdu");
        }
        return false;
    }
};

class Duozhi : public TriggerSkill
{
public:
    Duozhi()
        : TriggerSkill("duozhi")
    {
        events << CardFinished << EventPhaseStart;
        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *c = data.value<ServerPlayer *>();
            if (c->getPhase() == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark("duozhi") > 0) {
                        p->setMark("duozhi", 0);
                        room->removePlayerCardLimitation(p, "use,response", ".$1", "duozhi", true);
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from != nullptr && use.from->isAlive() && use.from->hasSkill(this)) {
                ServerPlayer *current = room->getCurrent();
                if (current != nullptr && current->getPhase() != Player::NotActive && current->isAlive())
                    return {SkillInvokeDetail(this, use.from, use.from, nullptr, true)};
            }
        }

        return {};
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
            if (invoke->invoker->hasShownSkill(this)) {
                LogMessage l;
                l.type = "#TriggerSkill";
                l.from = invoke->invoker;
                l.arg = objectName();
                room->sendLog(l);
            }

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        LogMessage l;

        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (p->getMark("duozhi") == 0) {
                p->setMark("duozhi", 1);
                room->setPlayerCardLimitation(p, "use,response", ".", "duozhi", true);
                l.to << p;
            }
        }

        if (!l.to.isEmpty()) {
            l.type = "#duozhi";
            room->sendLog(l);
        }

        return false;
    }
};

class Jinji : public DistanceSkill
{
public:
    Jinji()
        : DistanceSkill("jinji")
    {
    }

    int getCorrect(const Player *from, const Player *to) const override
    {
        if (from == to)
            return 0;
        int i = 0;
        if (from->hasSkill(this))
            i -= from->getEquips().length();
        if (to->hasSkill(this))
            i += to->getBrokenEquips().length();

        return i;
    }
};

class TianxingVS : public OneCardViewAsSkill
{
public:
    TianxingVS()
        : OneCardViewAsSkill("tianxing")
    {
        response_pattern == "@@tianxing";
    }

    bool viewFilter(const Card *to_select) const override
    {
        return Self->hasEquip(to_select) && !Self->isBrokenEquip(to_select->getEffectiveId());
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        return new DummyCard({originalCard->getEffectiveId()});
    }
};

class Tianxing : public TriggerSkill
{
public:
    Tianxing()
        : TriggerSkill("tianxing")
    {
        events << Damage << EventPhaseStart;
        view_as_skill = new TianxingVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *c = data.value<ServerPlayer *>();
            if (c->getPhase() == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark("tianxing") > 0) {
                        p->setMark("tianxing", 0);
                        room->setPlayerProperty(p, "tianxing", QString());
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;

        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *c = data.value<ServerPlayer *>();
            if (c->getPhase() == Player::RoundStart) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p->isAlive() && p->canSlash(c, false)) {
                        bool flag = false;
                        foreach (const Card *c, p->getEquips()) {
                            if (!p->getBrokenEquips().contains(c->getEffectiveId())) {
                                flag = true;
                                break;
                            }
                        }
                        if (flag)
                            r << SkillInvokeDetail(this, p, p, c);
                    }
                }
            }
        } else {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from != nullptr && damage.from->isAlive() && damage.from->hasSkill(this) && damage.card != nullptr && damage.card->isKindOf("Slash")
                && !damage.to->property("tianxing").toString().split("+").contains(damage.from->objectName()))
                r << SkillInvokeDetail(this, damage.from, damage.from, damage.to, true);
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            const Card *c = room->askForCard(invoke->invoker, "@@tianxing", "@tianxing-discard", data, Card::MethodNone, nullptr, false, "tianxing");
            if (c != nullptr) {
                invoke->invoker->addBrokenEquips({c->getEffectiveId()});
                return true;
            }
        } else {
            if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
                if (invoke->invoker->hasShownSkill(this)) {
                    LogMessage l;
                    l.type = "#TriggerSkill";
                    l.from = invoke->invoker;
                    l.arg = objectName();
                    room->sendLog(l);
                }

                return true;
            }
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (triggerEvent == EventPhaseStart) {
            Slash *s = new Slash(Card::NoSuit, 0);
            s->setSkillName("_tianxing");
            room->useCard(CardUseStruct(s, invoke->invoker, {invoke->targets.first()}));
        } else {
            invoke->targets.first()->setMark("tianxing", 1);

            LogMessage l;
            l.type = "#tianxing";
            l.from = invoke->targets.first();
            l.to << invoke->invoker;
            room->sendLog(l);

            QSet<QString> prohibited = invoke->targets.first()->property("tianxing").toString().split("+").toSet();
            prohibited << invoke->invoker->objectName();
            room->setPlayerProperty(invoke->targets.first(), "tianxing", QStringList(prohibited.toList()).join("+"));
        }

        return false;
    }
};

class TianxingP : public ProhibitSkill
{
public:
    TianxingP()
        : ProhibitSkill("#tianxing-prohibit")
    {
    }

    bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others, bool) const override
    {
        return from->property("tianxing").toString().split("+").contains(to->objectName()) && card->getTypeId() != Card::TypeSkill && others.isEmpty();
    }
};

TH17Package::TH17Package()
    : Package("th17")
{
    General *keiki = new General(this, "keiki$", "gxs");
    keiki->addSkill(new Zaoxing);

    General *eika = new General(this, "eika", "gxs", 3);
    eika->addSkill(new Shanlei);
    eika->addSkill(new Bengluo);

    General *urumi = new General(this, "urumi", "gxs");
    urumi->addSkill(new Lunni);
    urumi->addSkill(new Quangui);

    General *kutaka = new General(this, "kutaka", "gxs");
    kutaka->addSkill(new Yvshou);
    kutaka->addSkill(new Lingdu);

    General *yachie = new General(this, "yachie", "gxs");
    yachie->addSkill(new Duozhi);

    General *mayumi = new General(this, "mayumi", "gxs");
    Q_UNUSED(mayumi);

    General *saki = new General(this, "saki", "gxs");
    saki->addSkill(new Jinji);
    saki->addSkill(new Tianxing);
    saki->addSkill(new TianxingP);
    related_skills.insertMulti("tianxing", "#tianxing-probibit");

    addMetaObject<LunniCard>();
}

ADD_PACKAGE(TH17)
