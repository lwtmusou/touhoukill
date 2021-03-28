#include "playground.h"
#include "client.h"
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

#include <random>

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

                foreach (QString skill_name, gepi_list) {
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
            if (p == player || !p->hasSkill(this) || !player->canDiscard(p, "he"))
                continue;

            r << SkillInvokeDetail(this, p, p, player, false, player);
        }

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
            int id = room->askForCardChosen(invoke->targets.first(), invoke->invoker, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(id, invoke->invoker, invoke->invoker == invoke->targets.first() ? NULL : invoke->targets.first());
            return true;
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

bool Fsu0413GainianDialog::isResponseOk(const Player *player, const QString &pattern) const
{
    foreach (const Card *c, map) {
        QScopedPointer<Card> copy(Sanguosha->cloneCard(c));
        copy->setSkillName("fsu0413gainian");
        foreach (const Card *handCard, player->getHandcards()) {
            if (handCard->isKindOf("DelayedTrick")) {
                copy->clearSubcards();
                copy->addSubcard(handCard);
                if ((copy->matchTypeOrName(pattern) || Sanguosha->matchExpPattern(pattern, player, copy.data())) && !player->isCardLimited(copy.data(), Card::MethodUse))
                    return true;
            }
        }

        foreach (int handPileId, player->getHandPile()) {
            const Card *handCard = player->getRoomObject()->getCard(handPileId);
            if (handCard != nullptr && handCard->isKindOf("DelayedTrick")) {
                copy->clearSubcards();
                copy->addSubcard(handCard);
                if ((copy->matchTypeOrName(pattern) || Sanguosha->matchExpPattern(pattern, player, copy.data())) && !player->isCardLimited(copy.data(), Card::MethodUse))
                    return true;
            }
        }
    }

    return false;
}

void Fsu0413GainianDialog::popup(Player *_Self)
{
    Self = _Self;

    Self->tag.remove("fsu0413gainian");
    QStringList availableCards;

    if (Self->getRoomObject()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        foreach (const Card *c, map) {
            QScopedPointer<Card> copy(Sanguosha->cloneCard(c));
            copy->setSkillName("fsu0413gainian");
            foreach (const Card *handCard, Self->getHandcards()) {
                if (handCard->isKindOf("DelayedTrick")) {
                    copy->clearSubcards();
                    copy->addSubcard(handCard);
                    if ((copy->matchTypeOrName(Self->getRoomObject()->getCurrentCardUsePattern())
                         || Sanguosha->matchExpPattern(Self->getRoomObject()->getCurrentCardUsePattern(), Self, copy.data()))
                        && !Self->isCardLimited(copy.data(), Card::MethodUse)) {
                        availableCards << copy->objectName();
                        break;
                    }
                }
            }

            foreach (int handPileId, Self->getHandPile()) {
                const Card *handCard = Self->getRoomObject()->getCard(handPileId);
                if (handCard != nullptr && handCard->isKindOf("DelayedTrick")) {
                    copy->clearSubcards();
                    copy->addSubcard(handCard);
                    if ((copy->matchTypeOrName(Self->getRoomObject()->getCurrentCardUsePattern())
                         || Sanguosha->matchExpPattern(Self->getRoomObject()->getCurrentCardUsePattern(), Self, copy.data()))
                        && !Self->isCardLimited(copy.data(), Card::MethodUse)) {
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
    } else if (Self->getRoomObject()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
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
                const Card *handCard = Self->getRoomObject()->getCard(handPileId);
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
            const Card *card = player->getRoomObject()->getCard(id);
            if (card != nullptr && card->isKindOf("DelayedTrick"))
                return true;
        }

        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        if (player->getRoomObject()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            Fsu0413GainianDialog *d = qobject_cast<Fsu0413GainianDialog *>(getDialog());
            if (d != nullptr)
                return d->isResponseOk(player, pattern);
        }

        return false;
    }

    const Card *viewAs(const Card *originalCard, const Player *Self) const override
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
        frequency = Eternal;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        bool flag = false;
        foreach (int id, const_cast<Room *>(room)->getDiscardPile()) {
            const Card *c = room->getCard(id);
            if (c && c->isKindOf("DelayedTrick")) {
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
            const Card *c = room->getCard(id);
            if (c && c->isKindOf("DelayedTrick"))
                delayedtricks << id;
        }

        int obtainId = delayedtricks.at(QRandomGenerator::global()->generate() % delayedtricks.length());
        st.player->obtainCard(room->getCard(obtainId));

        return false;
    }
};

Fsu0413Fei2ZhaiCard::Fsu0413Fei2ZhaiCard()
{
    target_fixed = true;
    will_throw = true;
}

void Fsu0413Fei2ZhaiCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->doLightbox("$Fsu0413Fei2ZhaiAnimate", 4000);
    room->setPlayerMark(card_use.from, "@fat", 0);
    SkillCard::onUse(room, card_use);
}

void Fsu0413Fei2ZhaiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    while (!source->isNude() && source->isAlive())
        source->throwAllHandCards();

    DummyCard dummy;
    foreach (int id, room->getDiscardPile()) {
        if (room->getCard(id)->isKindOf("Peach"))
            dummy.addSubcard(id);
    }

    source->obtainCard(&dummy);
    room->handleAcquireDetachSkills(source, "fsu0413fei4zhai");
}

class Fsu0413Fei2Zhai : public ViewAsSkill
{
public:
    Fsu0413Fei2Zhai()
        : ViewAsSkill("fsu0413fei2zhai")
    {
        frequency = Limited;
        limit_mark = "@fat";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select, const Player *Self) const override
    {
        return !to_select->isEquipped(Self);
    }

    const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const override
    {
        if (cards.length() == Self->getHandcardNum()) {
            Fsu0413Fei2ZhaiCard *fat = new Fsu0413Fei2ZhaiCard;
            fat->addSubcards(cards);
            return fat;
        }

        return nullptr;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return player->getMark("@fat") > 0;
    }
};

class Fsu0413Fei4Zhai : public TriggerSkill
{
public:
    Fsu0413Fei4Zhai()
        : TriggerSkill("fsu0413fei4zhai")
    {
        events << EventPhaseChanging;
        frequency = Eternal;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        PhaseChangeStruct st = data.value<PhaseChangeStruct>();
        if (st.player->isAlive() && st.player->hasSkill(this) && st.to == Player::Discard)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, st.player, st.player, nullptr, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->skip(Player::Discard);
        return false;
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
                const Card *c = room->getCard(id);
                if (c != nullptr) {
                    s = c->getSuit();
                    foreach (int remainid, cards) {
                        const Card *rc = room->getCard(remainid);
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

    bool viewFilter(const Card *c, const Player *Self) const override
    {
        return Self->canDiscard(Self, c->getId());
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
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
        WrappedCard *wrap = Sanguosha->currentRoom()->getWrappedCard(originalCard->getId());
        wrap->takeOver(slash);
        return wrap;
    }

    int getEffectIndex(const ServerPlayer *player, const Card *) const override
    {
        return (player->getGeneralName() == "benmao" || player->getGeneral2Name() == "benmao") ? 1 : 3;
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
            if (use.from != nullptr && !use.from->hasSkill("bmmaoji") && use.from->isAlive() && use.card != nullptr && use.card->isKindOf("Slash")) {
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
            QStringList conflictingSkills = {"huanwei"};
            foreach (const QString &conflict, conflictingSkills) {
                if (invoke->targets.first()->hasSkill(conflict, true, true)) {
                    room->touhouLogmessage("#bmmaoji-conflictingskill", invoke->targets.first(), conflict);
                    skills << (QStringLiteral("-") + conflict);
                }
            }

            room->handleAcquireDetachSkills(invoke->targets.first(), skills);
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
                QString prompt = QString("@bmmaoji-slash%1:%2::%3").arg(i == eff.jink_num ? "-start" : QString()).arg(eff.from->objectName()).arg(i);
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

PlaygroundPackage::PlaygroundPackage()
    : Package("playground")
{
    General *Fsu0413 = new General(this, "Fsu0413", "touhougod", 5, true);
    Fsu0413->addSkill(new Fsu0413Gepi);
    Fsu0413->addSkill(new Fsu0413Gainian);
    Fsu0413->addSkill(new Fsu0413GainianDis);
    Fsu0413->addSkill(new Fsu0413Lese);
    related_skills.insert("fsu0413gainian", "#fsu0413gainian-dis");

    //    General *jmshtry = new General(this, "jmshtry", "touhougod", 5, true);
    //    jmshtry->addSkill(new JmshtryMdlKudi);

    General *kitsuhattyou = new General(this, "kitsuhattyou", "touhougod", 3, false, true, true);
    kitsuhattyou->addSkill(new Fsu0413JbdNashaT);
    addMetaObject<Fsu0413JbdNashaCard>();

    General *benmao = new General(this, "benmao", "touhougod", 5, true);
    benmao->addSkill(new BmMaoji);
    benmao->addSkill(new BmMaojiTrigger);
    related_skills.insert("bmmaoji", "#bmmaoji");
}

ADD_PACKAGE(Playground)
