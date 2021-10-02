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
            // r->setPlayerProperty(p, "zaoxingbackup", p->getMark("@zaoxing"));
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
        } else
            return true;

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

    void record(TriggerEvent e, Room *r, QVariant &d) const override
    {
        if (e == CardsMoveOneTime) {
            CardsMoveOneTimeStruct s = d.value<CardsMoveOneTimeStruct>();
            if (s.from->getPhase() == Player::Discard && ((s.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
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
    Q_UNUSED(kutaka);

    General *yachie = new General(this, "yachie", "gxs");
    Q_UNUSED(yachie);

    General *mayumi = new General(this, "mayumi", "gxs");
    Q_UNUSED(mayumi);

    General *saki = new General(this, "saki", "gxs");
    Q_UNUSED(saki);

    addMetaObject<LunniCard>();
}

ADD_PACKAGE(TH17)
