#include "playground.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "skill.h"
#include "testCard.h"

#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QCommandLinkButton>
#include <QPointer>
#include <QVBoxLayout>

class Fsu0413Gepi : public TriggerSkill
{
public:
    Fsu0413Gepi()
        : TriggerSkill("fsu0413gepi")
    {
        events << EventPhaseStart;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const override
    {
        ServerPlayer *target = data.value<ServerPlayer *>();
        if (target->getPhase() == Player::NotActive) {
            foreach (ServerPlayer *player, room->getAllPlayers()) {
                QStringList gepi_list = player->tag["fsu0413gepi"].toStringList();
                if (gepi_list.isEmpty())
                    continue;

                player->tag.remove("fsu0413gepi");

                foreach (const QString &skill_name, gepi_list) {
                    room->setPlayerSkillInvalidity(player, skill_name, false);
                    if (player->hasSkill(skill_name)) {
                        LogMessage log;
                        log.type = "$Fsu0413GepiReset";
                        log.from = player;
                        log.arg = skill_name;
                        room->sendLog(log);
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        QList<SkillInvokeDetail> r;

        if (player->getPhase() != Player::Start)
            return r;

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (!p->hasSkill(this) || !player->canDiscard(p, "hes"))
                continue;

            r << SkillInvokeDetail(this, p, p, player, false, player);
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (invoke->targets.first() == invoke->invoker) {
            if (room->askForDiscard(invoke->targets.first(), "fsu0413gepi", 1, 1, true, true, "@fsu0413gepi-discard"))
                return true;
        } else {
            if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
                int id = room->askForCardChosen(invoke->targets.first(), invoke->invoker, "hes", objectName(), false, Card::MethodDiscard);
                room->throwCard(id, invoke->invoker, invoke->invoker == invoke->targets.first() ? NULL : invoke->targets.first());
                return true;
            }
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *player = invoke->targets.first();

        QList<const Skill *> skills = player->getVisibleSkillList();
        QList<const Skill *> skills_canselect;

        foreach (const Skill *s, skills) {
            if (!s->isLordSkill() && s->getFrequency() != Skill::Wake && s->getFrequency() != Skill::Eternal && !s->isAttachedLordSkill())
                skills_canselect << s;
        }

        const Skill *skill_selected = nullptr;

        if (!skills_canselect.isEmpty()) {
            QStringList l;
            foreach (const Skill *s, skills_canselect)
                l << s->objectName();

            QString skill_lose = l.first();
            if (l.length() > 1)
                skill_lose = room->askForChoice(invoke->invoker, objectName(), l.join("+"), data);

            foreach (const Skill *s, skills_canselect) {
                if (s->objectName() == skill_lose) {
                    skill_selected = s;
                    break;
                }
            }

            LogMessage log;
            log.type = "$Fsu0413GepiNullify";
            log.from = invoke->invoker;
            log.to << player;
            log.arg = skill_lose;
            room->sendLog(log);

            QStringList gepi_list = player->tag["fsu0413gepi"].toStringList();
            gepi_list << skill_lose;
            player->tag["fsu0413gepi"] = gepi_list;

            room->setPlayerSkillInvalidity(player, skill_lose, true);
        }

        if (player->isAlive()) {
            bool drawFlag = true;
            if (skill_selected != nullptr) {
                QString trans = Sanguosha->translate(":" + skill_selected->objectName());
                drawFlag = !trans.contains(Sanguosha->translate("fsu0413gepiPlay"));
            }

            if (drawFlag)
                player->drawCards(3, objectName());
        }

        return false;
    }
};

Fsu0413GainianDialog *Fsu0413GainianDialog::getInstance(const QString &object)
{
    static QPointer<Fsu0413GainianDialog> instance;

    if (!instance.isNull() && instance->objectName() != object)
        delete instance;

    if (instance.isNull()) {
        instance = new Fsu0413GainianDialog(object);
        connect(qApp, &QCoreApplication::aboutToQuit, instance.data(), &Fsu0413GainianDialog::deleteLater);
    }

    return instance;
}

bool Fsu0413GainianDialog::isResponseOk(const Player *player, const QString &_pattern) const
{
    const CardPattern *pattern = Sanguosha->getPattern(_pattern);
    if (pattern == nullptr)
        return false;

    foreach (const Card *c, map) {
        QScopedPointer<Card> copy(Sanguosha->cloneCard(c));
        copy->setSkillName("fsu0413gainian");
        foreach (const Card *handCard, player->getHandcards()) {
            if (handCard->isKindOf("DelayedTrick")) {
                copy->clearSubcards();
                copy->addSubcard(handCard);
                if (pattern->match(player, copy.data()) && !player->isCardLimited(copy.data(), Card::MethodUse))
                    return true;
            }
        }

        foreach (int handPileId, player->getHandPile()) {
            const Card *handCard = Sanguosha->getCard(handPileId);
            if (handCard != nullptr && handCard->isKindOf("DelayedTrick")) {
                copy->clearSubcards();
                copy->addSubcard(handCard);
                if (pattern->match(player, copy.data()) && !player->isCardLimited(copy.data(), Card::MethodUse))
                    return true;
            }
        }
    }

    return false;
}

void Fsu0413GainianDialog::popup()
{
    Self->tag.remove("fsu0413gainian");
    QStringList availableCards;

    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const CardPattern *pattern = Sanguosha->getPattern(Sanguosha->getCurrentCardUsePattern());
        if (pattern == nullptr) {
            emit onButtonClick();
            return;
        }

        foreach (const Card *c, map) {
            QScopedPointer<Card> copy(Sanguosha->cloneCard(c));
            copy->setSkillName("fsu0413gainian");
            foreach (const Card *handCard, Self->getHandcards()) {
                if (handCard->isKindOf("DelayedTrick")) {
                    copy->clearSubcards();
                    copy->addSubcard(handCard);
                    if (pattern->match(Self, copy.data()) && !Self->isCardLimited(copy.data(), Card::MethodUse)) {
                        availableCards << copy->objectName();
                        break;
                    }
                }
            }

            foreach (int handPileId, Self->getHandPile()) {
                const Card *handCard = Sanguosha->getCard(handPileId);
                if (handCard != nullptr && handCard->isKindOf("DelayedTrick")) {
                    copy->clearSubcards();
                    copy->addSubcard(handCard);
                    if (pattern->match(Self, copy.data()) && !Self->isCardLimited(copy.data(), Card::MethodUse)) {
                        availableCards << copy->objectName();
                        break;
                    }
                }
            }
        }

        if (availableCards.isEmpty()) {
            emit onButtonClick();
            return;
        } else if (availableCards.length() == 1) {
            Self->tag["fsu0413gainian"] = availableCards.first();
            emit onButtonClick();
            return;
        }
    } else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
        foreach (const Card *c, map) {
            QScopedPointer<Card> copy(Sanguosha->cloneCard(c));
            copy->setSkillName("fsu0413gainian");
            foreach (const Card *handCard, Self->getHandcards()) {
                if (handCard->isKindOf("DelayedTrick")) {
                    copy->clearSubcards();
                    copy->addSubcard(handCard);
                    if (copy->isAvailable(Self) && !Self->isCardLimited(copy.data(), Card::MethodUse)) {
                        availableCards << copy->objectName();
                        break;
                    }
                }
            }

            foreach (int handPileId, Self->getHandPile()) {
                const Card *handCard = Sanguosha->getCard(handPileId);
                if (handCard != nullptr && handCard->isKindOf("DelayedTrick")) {
                    copy->clearSubcards();
                    copy->addSubcard(handCard);
                    if (copy->isAvailable(Self) && !Self->isCardLimited(copy.data(), Card::MethodUse)) {
                        availableCards << copy->objectName();
                        break;
                    }
                }
            }
        }

        if (availableCards.isEmpty()) {
            emit onButtonClick();
            return;
        } else if (availableCards.length() == 1) {
            Self->tag["fsu0413gainian"] = availableCards.first();
            emit onButtonClick();
            return;
        }
    } else {
        emit onButtonClick();
        return;
    }

    foreach (QAbstractButton *but, group->buttons())
        but->setEnabled(availableCards.contains(but->objectName()));

    exec();
}

void Fsu0413GainianDialog::selectCard(QAbstractButton *button)
{
    Self->tag["fsu0413gainian"] = button->objectName();
    emit onButtonClick();
    accept();
}

Fsu0413GainianDialog::Fsu0413GainianDialog(const QString &object)
{
    setObjectName(object);
    setWindowTitle(Sanguosha->translate(object));
    group = new QButtonGroup(this);

    createButtons();

    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectCard(QAbstractButton *)));
}

void Fsu0413GainianDialog::createButtons()
{
    QVBoxLayout *layout = new QVBoxLayout;

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    QStringList ban_list = Sanguosha->getBanPackages();
    foreach (const Card *card, cards) {
        if (card->inherits("DelayedTrick") && !map.contains(card->objectName()) && !ban_list.contains(card->getPackage())) {
            Card *c = Sanguosha->cloneCard(card->objectName());
            c->setParent(this);
            layout->addWidget(createButton(c));
        }
    }

    setLayout(layout);
}

QAbstractButton *Fsu0413GainianDialog::createButton(const Card *card)
{
    QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate(card->objectName()));
    button->setObjectName(card->objectName());
    button->setToolTip(card->getDescription());

    map.insert(card->objectName(), card);
    group->addButton(button);

    return button;
}

class Fsu0413Gainian : public OneCardViewAsSkill
{
public:
    Fsu0413Gainian()
        : OneCardViewAsSkill("fsu0413gainian")
    {
        filter_pattern = "DelayedTrick|.|.|hand";
        response_or_use = true;
    }

    QDialog *getDialog() const override
    {
        return Fsu0413GainianDialog::getInstance("fsu0413gainian");
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        foreach (const Card *card, player->getHandcards()) {
            if (card->isKindOf("DelayedTrick"))
                return true;
        }

        foreach (int id, player->getHandPile()) {
            const Card *card = Sanguosha->getCard(id);
            if (card != nullptr && card->isKindOf("DelayedTrick"))
                return true;
        }

        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            Fsu0413GainianDialog *d = qobject_cast<Fsu0413GainianDialog *>(getDialog());
            if (d != nullptr)
                return d->isResponseOk(player, pattern);
        }

        return false;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        if (!Self->tag.contains("fsu0413gainian") || Self->tag["fsu0413gainian"].toString().length() == 0)
            return nullptr;

        Card *c = Sanguosha->cloneCard(Self->tag["fsu0413gainian"].toString());
        if (c == nullptr)
            return nullptr;

        c->setSkillName(objectName());
        c->setShowSkill(objectName());
        c->addSubcard(originalCard);

        return c;
    }
};

class Fsu0413GainianDis : public TargetModSkill
{
public:
    Fsu0413GainianDis()
        : TargetModSkill("#fsu0413gainian-dis")
    {
        pattern = "DelayedTrick";
    }

    int getDistanceLimit(const Player *from, const Card *card) const override
    {
        if (from->hasSkill("fsu0413gainian") && card->getSkillName() == "fsu0413gainian")
            return 1000;

        return 0;
    }
};

class Fsu0413Lese : public TriggerSkill
{
public:
    Fsu0413Lese()
        : TriggerSkill("fsu0413lese")
    {
        events << DrawNCards;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        bool flag = false;
        foreach (int id, const_cast<Room *>(room)->getDiscardPile()) {
            const Card *c = Sanguosha->getCard(id);
            if ((c != nullptr) && c->isKindOf("DelayedTrick")) {
                flag = true;
                break;
            }
        }

        QList<SkillInvokeDetail> r;

        if (!flag)
            return r;

        DrawNCardsStruct st = data.value<DrawNCardsStruct>();

        if (st.player->hasSkill(this) && st.player->isAlive())
            r << SkillInvokeDetail(this, st.player, st.player, nullptr, true);

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

                room->notifySkillInvoked(invoke->invoker, objectName());
            }

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        DrawNCardsStruct st = data.value<DrawNCardsStruct>();
        QList<int> delayedtricks;

        foreach (int id, room->getDiscardPile()) {
            const Card *c = Sanguosha->getCard(id);
            if ((c != nullptr) && c->isKindOf("DelayedTrick"))
                delayedtricks << id;
        }

        int obtainId = delayedtricks.at(qrand() % delayedtricks.length());
        st.player->obtainCard(Sanguosha->getCard(obtainId));

        return false;
    }
};

class Fsu0413Fei2Zhai : public TriggerSkill
{
public:
    Fsu0413Fei2Zhai()
        : TriggerSkill("fsu0413fei2zhai")
    {
        events << EventPhaseChanging << CardsMoveOneTime;
        frequency = Eternal;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct st = data.value<PhaseChangeStruct>();
            if (st.player->isAlive() && st.player->hasSkill(this) && st.to == Player::Discard)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, st.player, st.player, nullptr, true);
        } else {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_USE)) {
                foreach (int id, move.card_ids) {
                    const Card *c = Sanguosha->getCard(id);
                    if (c->isKindOf("Peach") && room->getCardPlace(id) == Player::DiscardPile) {
                        QList<SkillInvokeDetail> d;
                        foreach (ServerPlayer *p, room->getAllPlayers()) {
                            if (p->hasSkill(this))
                                d << SkillInvokeDetail(this, p, p, nullptr, true);
                        }
                        return d;
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        LogMessage l;
        l.type = "#TriggerSkill";
        l.from = invoke->invoker;
        l.arg = objectName();
        room->sendLog(l);
        room->notifySkillInvoked(invoke->invoker, objectName());

        if (e == EventPhaseChanging)
            invoke->invoker->skip(Player::Discard);
        else {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            DummyCard d;
            foreach (int id, move.card_ids) {
                const Card *c = Sanguosha->getCard(id);
                if (c->isKindOf("Peach") && room->getCardPlace(id) == Player::DiscardPile)
                    d.addSubcard(id);
            }

            invoke->invoker->obtainCard(&d);
        }
        return false;
    }
};

// negative skill version 1
#if 0
class Fsu0413Fei4Zhai : public TriggerSkill
{
public:
    Fsu0413Fei4Zhai()
        : TriggerSkill("fsu0413fei4zhai")
    {
        events << DamageCaused << EventPhaseChanging << DamageDone;
        frequency = Eternal;
        global = true;
    }

    void record(TriggerEvent triggerEvent, Room *, QVariant &data) const override
    {
        if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from != nullptr)
                damage.from->setFlags("fsu0413fei4zhaidamaged");
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            change.player->setFlags("-fsu0413fei4zhaidamaged");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        if (e == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from != nullptr && (damage.from->hasSkill(this) && (damage.damage > 1 || damage.from->hasFlag("fsu0413fei4zhaidamaged"))))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, true);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *r, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();

        LogMessage l;
        l.type = "#micai01";
        l.from = invoke->invoker;
        l.arg = objectName();
        l.arg2 = QString::number(damage.damage);
        r->sendLog(l);
        r->notifySkillInvoked(invoke->invoker, objectName());

        return true;
    }
};
#endif

class Fsu0413Fei4Zhai : public TriggerSkill
{
public:
    Fsu0413Fei4Zhai()
        : TriggerSkill("fsu0413fei4zhai")
    {
        events << PreHpRecover;
        frequency = Eternal;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *r, const QVariant &data) const override
    {
        RecoverStruct recover = data.value<RecoverStruct>();
        if (recover.to->hasSkill(this) && r->getCurrentDyingPlayer() != recover.to && recover.card != nullptr && recover.card->isKindOf("Peach"))
            return {SkillInvokeDetail(this, recover.to, recover.to, nullptr, true)};

        return {};
    }

    bool effect(TriggerEvent, Room *r, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();

        LogMessage l;
        l.type = "#jiexianrecover";
        l.from = invoke->invoker;
        l.arg = objectName();
        l.arg2 = QString::number(damage.damage);
        r->sendLog(l);
        r->notifySkillInvoked(invoke->invoker, objectName());

        return true;
    }
};

class JmshtryMdlKudi : public TriggerSkill
{
public:
    JmshtryMdlKudi()
        : TriggerSkill("jmshtrymdlkudi")
    {
        events << EventPhaseStart << DamageInflicted;
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->isAlive() && p->getPhase() == Player::Start && p->hasSkill(this))
                r << SkillInvokeDetail(this, p, p);

        } else {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->isAlive() && damage.to->hasSkill(this))
                r << SkillInvokeDetail(this, damage.to, damage.to);
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
            QList<int> cards = room->getNCards(3);
            room->fillAG(cards, invoke->invoker);
            invoke->tag["cards"] = IntList2VariantList(cards);
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<int> cards = VariantList2IntList(invoke->tag.value("cards", QVariantList()).toList());
        if (cards.isEmpty())
            return false;
        QList<int> getcards;
        QList<int> guanxingcards;
        Card::Suit s = Card::NoSuit;

        do {
            int id = room->askForAG(invoke->invoker, cards, s != Card::NoSuit, objectName());
            if (id == -1)
                break;

            room->takeAG(invoke->invoker, id, false, QList<ServerPlayer *>() << invoke->invoker);
            room->showCard(invoke->invoker, id);

            getcards << id;

            cards.removeOne(id);

            if (s == Card::NoSuit) {
                const Card *c = Sanguosha->getCard(id);
                if (c != nullptr) {
                    s = c->getSuit();
                    foreach (int remainid, cards) {
                        const Card *rc = Sanguosha->getCard(remainid);
                        if (rc != nullptr && rc->getSuit() != s) {
                            guanxingcards << remainid;
                            cards.removeOne(remainid);
                            room->takeAG(nullptr, remainid, false, QList<ServerPlayer *>() << invoke->invoker);
                        }
                    }
                }
            }
        } while (!cards.isEmpty());

        room->clearAG();

        DummyCard dc;
        dc.addSubcards(getcards);
        room->obtainCard(invoke->invoker, &dc);

        guanxingcards.append(cards);
        room->askForGuanxing(invoke->invoker, guanxingcards, Room::GuanxingDownOnly, objectName());

        return false;
    }
};

Fsu0413JbdNashaCard::Fsu0413JbdNashaCard()
{
}

bool Fsu0413JbdNashaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && Self->inMyAttackRange(to_select);
}

void Fsu0413JbdNashaCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->gainMark("@Brid");
}

class Fsu0413JbdNasha : public OneCardViewAsSkill
{
public:
    Fsu0413JbdNasha()
        : OneCardViewAsSkill("fsu0413jbdnasha")
    {
    }

    bool viewFilter(const Card *c) const override
    {
        return Self->canDiscard(Self, c->getId());
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        Fsu0413JbdNashaCard *c = new Fsu0413JbdNashaCard;
        c->addSubcard(originalCard);
        return c;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return player->usedTimes("Fsu0413JbdNaShaCard") < 1000;
    }
};

class Fsu0413JbdNashaT : public TriggerSkill
{
public:
    Fsu0413JbdNashaT()
        : TriggerSkill("fsu0413jbdnasha")
    {
        view_as_skill = new Fsu0413JbdNasha;
        events << TurnStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        ServerPlayer *jbd = nullptr;

        foreach (ServerPlayer *ps, room->getAllPlayers()) {
            if (ps->hasSkill(this)) {
                jbd = ps;
                break;
            }
        }

        if (jbd != nullptr && p != nullptr && p->isAlive() && p->getMark("@Brid") > 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, jbd, jbd, p, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->targets.first()->loseAllMarks("@Brid");

        QString choice = room->askForChoice(invoke->targets.first(), objectName(), "turn+disc");
        if (choice == "turn")
            invoke->targets.first()->turnOver();
        else {
            DummyCard c;
            c.addSubcards(invoke->targets.first()->getCards("hsej"));
            room->throwCard(&c, invoke->targets.first());
        }

        return false;
    }
};

class BmMaoji : public FilterSkill
{
public:
    BmMaoji()
        : FilterSkill("bmmaoji")
    {
    }

    bool viewFilter(const Card *to_select) const override
    {
        const Room *room = Sanguosha->currentRoom();
        Q_ASSERT(room != nullptr);
        return room->getCardPlace(to_select->getId()) == Player::PlaceHand;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        PowerSlash *slash = new PowerSlash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName("bmmaoji");
        WrappedCard *wrap = Sanguosha->getWrappedCard(originalCard->getId());
        wrap->takeOver(slash);
        return wrap;
    }

    int getEffectIndex(const ServerPlayer *player, const Card *) const override
    {
        return player->hasSkill("bmbenti", true) ? 1 : 3;
    }
};

class BmMaojiTrigger : public TriggerSkill
{
public:
    BmMaojiTrigger()
        : TriggerSkill("#bmmaoji")
    {
        events << TargetConfirmed << SlashProceed << Cancel;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> r;
        if (e == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from != nullptr && !use.from->hasSkill("bmmaoji", true) && use.from->isAlive() && use.card != nullptr && use.card->isKindOf("Slash")) {
                foreach (ServerPlayer *p, use.to) {
                    if (p->hasSkill("bmmaoji"))
                        r << SkillInvokeDetail(this, p, p, use.from, true);
                }
            }
        } else if (e == SlashProceed) {
            SlashEffectStruct eff = data.value<SlashEffectStruct>();
            if (eff.from->hasSkill("bmmaoji"))
                r << SkillInvokeDetail(this, eff.from, eff.from, eff.to, true);
        } else if (e == Cancel && data.canConvert<SlashEffectStruct>()) {
            SlashEffectStruct eff = data.value<SlashEffectStruct>();
            if (eff.slash->hasFlag("bmmaoji"))
                r << SkillInvokeDetail(this, eff.from, eff.from, eff.to, true, nullptr, false);
        }
        return r;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (e == TargetConfirmed) {
            room->broadcastSkillInvoke("bmmaoji", 2);

            if (!invoke->targets.first()->getCards("e").isEmpty()) {
                DummyCard c;
                c.addSubcards(invoke->targets.first()->getCards("e"));
                invoke->targets.first()->obtainCard(&c);
            }

            QStringList skills = {"bmmaoji"};
            QStringList conflictingSkills = {"huanwei", "ftmsuanshu"};
            foreach (const QString &conflict, conflictingSkills) {
                if (invoke->targets.first()->hasSkill(conflict, true, true)) {
                    room->touhouLogmessage("#bmmaoji-conflictingskill", invoke->targets.first(), conflict);
                    skills << (QStringLiteral("-") + conflict);
                }
            }

            room->handleAcquireDetachSkills(invoke->targets.first(), skills);

            JsonArray arg;
            arg << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
            arg << invoke->targets.first()->objectName();
            arg << "benmao";
            arg << "bmmaoji";
            arg << QString();
            arg << QString();
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
        } else if (e == SlashProceed) {
            SlashEffectStruct eff = data.value<SlashEffectStruct>();
            eff.slash->setFlags("bmmaoji"); // for triggering the effect afterwards...
        } else if (e == Cancel) {
            // In this case, BmMaoji Flag is set to the slash itself.
            // We should always use the BmMaoji slash procedure in this case and ignore the game rule.
            // So this effect will always return true.

            SlashEffectStruct eff = data.value<SlashEffectStruct>();
            // Ignoring the force hit case at this time...

            for (int i = 2; i > 0; i--) {
                QString prompt = QString("@bmmaoji-slash%1:%2::%3").arg((i == eff.jink_num ? "-start" : QString()), eff.from->objectName(), QString::number(i));
                const Card *slash = room->askForCard(eff.to, "slash", prompt, data, Card::MethodResponse, eff.from);
                if (slash == nullptr)
                    return true;
            }

            eff.canceled = true;
            data = QVariant::fromValue<SlashEffectStruct>(eff);
            return true;
        }
        return false;
    }
};

class BmBenti : public AttackRangeSkill
{
public:
    BmBenti()
        : AttackRangeSkill("bmbenti")
    {
    }

    int getFixed(const Player *target, bool) const override
    {
        int n = 0;
        if (target->hasSkill(this)) {
            QList<const Player *> ps = target->getAliveSiblings();
            ps << target;
            foreach (const Player *p, ps) {
                if (p->hasSkill("bmmaoji", true))
                    ++n;
            }
        }

        return n;
    }
};

class FtmSuanshu : public FilterSkill
{
public:
    FtmSuanshu()
        : FilterSkill("ftmsuanshu")
    {
    }

    bool viewFilter(const Card *to_select) const override
    {
        const Room *room = Sanguosha->currentRoom();
        Q_ASSERT(room != nullptr);
        ServerPlayer *p = room->getCardOwner(to_select->getId());
        Q_ASSERT(p != nullptr);
        return room->getCardPlace(to_select->getId()) == Player::PlaceHand && p->getHandcardNum() == 9;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        ExNihilo *exNihilo = new ExNihilo(originalCard->getSuit(), originalCard->getNumber());
        exNihilo->setSkillName("ftmsuanshu");
        WrappedCard *wrap = Sanguosha->getWrappedCard(originalCard->getId());
        wrap->takeOver(exNihilo);
        return wrap;
    }
};

class FtmSuanshuTrigger : public TriggerSkill
{
public:
    FtmSuanshuTrigger()
        : TriggerSkill("#ftmsuanshu")
    {
        events = {EventSkillInvalidityChange, EventAcquireSkill, EventLoseSkill, GameStart, CardsMoveOneTime};
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == GameStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if ((player != nullptr) && player->hasSkill("ftmsuanshu"))
                room->filterCards(player, player->getCards("hes"), true);
        }
        if (triggerEvent == EventLoseSkill || triggerEvent == EventAcquireSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            if (a.skill->objectName() == "ftmsuanshu")
                room->filterCards(a.player, a.player->getCards("hes"), true);
        }
        if (triggerEvent == EventSkillInvalidityChange) {
            QList<SkillInvalidStruct> invalids = data.value<QList<SkillInvalidStruct>>();
            foreach (SkillInvalidStruct v, invalids) {
                if ((v.skill == nullptr) || v.skill->objectName() == "ftmsuanshu")
                    room->filterCards(v.player, v.player->getCards("hes"), true);
            }
        }
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *p = nullptr;
            if (move.from != nullptr && move.from->hasSkill("ftmsuanshu"))
                p = qobject_cast<ServerPlayer *>(move.from);
            else if (move.to != nullptr && move.to->hasSkill("ftmsuanshu"))
                p = qobject_cast<ServerPlayer *>(move.to);
            if (p != nullptr && p->hasSkill("ftmsuanshu"))
                room->filterCards(p, p->getCards("hes"), true);
        }
    }
};

class FtmFeitian : public TriggerSkill
{
public:
    FtmFeitian()
        : TriggerSkill("ftmfeitian")
    {
        frequency = Compulsory;
        events = {CardUsed, CardResponded, EventPhaseStart};
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room * /*room*/, const QVariant &data) const override
    {
        switch (e) {
        case EventPhaseStart: {
            ServerPlayer *from = data.value<ServerPlayer *>();
            if (from != nullptr && from->isAlive() && from->hasSkill(this)
                && (from->getPhase() == Player::Finish || (from->getPhase() == Player::NotActive && from->getMark("@flying") > 0)))
                return {SkillInvokeDetail(this, from, from, nullptr, true)};

            break;
        }
        default: {
            ServerPlayer *from = nullptr;
            const Card *c = nullptr;
            if (e == CardUsed) {
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

            if (from != nullptr && from->isAlive() && from->hasSkill(this) && c != nullptr && (c->isBlack() || (c->isRed() && from->getMark("@flying") > 0))) {
                SkillInvokeDetail d(this, from, from, nullptr, true);
                d.tag["card"] = QVariant::fromValue(c);
                return {d};
            }
            break;
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

                room->notifySkillInvoked(invoke->invoker, objectName());
            }

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant & /*data*/) const override
    {
        int x = invoke->invoker->getMark("@flying");

        if (triggerEvent == EventPhaseStart) {
            if (invoke->invoker->getPhase() == Player::Finish) {
                if (x == 0) {
                    invoke->invoker->drawCards(2, "ftmfeitian");
                } else {
                    ServerPlayer *feitian = nullptr;
                    foreach (ServerPlayer *p, room->getAlivePlayers()) {
                        if (p->getSeat() == x) {
                            feitian = p;
                            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), p->objectName());
                            if (invoke->invoker->askForSkillInvoke("ftmfeitian_x", QVariant::fromValue(p), "recover-or-losehp:" + p->objectName()))
                                room->loseHp(p);
                            else
                                room->recover(p, RecoverStruct());

                            break;
                        }
                    }
                    if (feitian == nullptr && x == room->getDrawPile().length())
                        invoke->invoker->drawCards(room->getDrawPile().length(), "ftmfeitian");
                }
            } else if (invoke->invoker->getPhase() == Player::NotActive) {
                invoke->invoker->loseAllMarks("@flying");
            }
        } else {
            const Card *c = invoke->tag.value("card").value<const Card *>();
            int getNum = x;
            if (c->isBlack())
                getNum += 1;

            invoke->invoker->gainMark("@flying", getNum);
        }

        return false;
    }
};

PlaygroundPackage::PlaygroundPackage()
    : Package("playground")
{
    General *dyingfs = new General(this, "dyingfsu0413", "touhougod", 4, true);
    dyingfs->addSkill(new Fsu0413Gepi);
    dyingfs->addSkill(new Skill("fsu0413sile", Skill::NotCompulsory));

    General *dovefs = new General(this, "dovefsu0413", "touhougod", 4, true);
    dovefs->addSkill(new Fsu0413Gainian);
    dovefs->addSkill(new Fsu0413GainianDis);
    dovefs->addSkill(new Fsu0413Lese);
    related_skills.insertMulti("fsu0413gainian", "#fsu0413gainian-dis");

    //    General *jmshtry = new General(this, "jmshtry", "touhougod", 5, true);
    //    jmshtry->addSkill(new JmshtryMdlKudi);

    General *otaku = new General(this, "otaku", "touhougod", 5, true);
    otaku->addSkill(new Fsu0413Fei2Zhai);
    otaku->addSkill(new Fsu0413Fei4Zhai);

    General *kitsuhattyou = new General(this, "kitsuhattyou", "touhougod", 3, false, true, true);
    kitsuhattyou->addSkill(new Fsu0413JbdNashaT);
    addMetaObject<Fsu0413JbdNashaCard>();

    General *benmao = new General(this, "benmao", "touhougod", 5, true);
    benmao->addSkill(new BmMaoji);
    benmao->addSkill(new BmMaojiTrigger);
    benmao->addSkill(new BmBenti);
    related_skills.insertMulti("bmmaoji", "#bmmaoji");

    General *god9 = new General(this, "god9", "touhougod", 9);
    god9->addSkill(new FtmSuanshu);
    god9->addSkill(new FtmSuanshuTrigger);
    related_skills.insertMulti("ftmsuanshu", "#ftmsuanshu");

    General *fsb = new General(this, "flyingskybright", "touhougod", 4, true);
    fsb->addSkill(new FtmFeitian);
}

ADD_PACKAGE(Playground)
