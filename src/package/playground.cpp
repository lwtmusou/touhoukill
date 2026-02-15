#include "playground.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "skill.h"
#include "washout.h"

#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QCommandLinkButton>
#include <QPointer>
#include <QVBoxLayout>

#if 0
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

#endif

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

    QList<SkillInvokeDetail> triggerable(TriggerEvent /*triggerEvent*/, const Room *room, const QVariant &data) const override
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

    bool effect(TriggerEvent /*triggerEvent*/, Room *room, QSharedPointer<SkillInvokeDetail> /*invoke*/, QVariant &data) const override
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
        return {};
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent /*triggerEvent*/, const Room *r, const QVariant &data) const override
    {
        RecoverStruct recover = data.value<RecoverStruct>();
        if (recover.to->hasSkill(this) && r->getCurrentDyingPlayer() != recover.to && recover.card != nullptr && recover.card->isKindOf("Peach"))
            return {SkillInvokeDetail(this, recover.to, recover.to, nullptr, true)};

        return {};
    }

    bool effect(TriggerEvent /*triggerEvent*/, Room *r, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    int getEffectIndex(const ServerPlayer *player, const Card * /*card*/) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room * /*room*/, const QVariant &data) const override
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
                    room->sendLog("#bmmaoji-conflictingskill", invoke->targets.first(), conflict);
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

    int getFixed(const Player *target, bool /*include_weapon*/) const override
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

class TailorFuzhongEffect : public TriggerSkill
{
public:
    explicit TailorFuzhongEffect(const QString &base = "tailorfuzhong")
        : TriggerSkill("#" + base + "-effect")
        , b(base)
    {
        events = {BuryVictim};
    }

    int getPriority() const override
    {
        return -6;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent /*triggerEvent*/, const Room *room, const QVariant &data) const override
    {
        if (room->getTagNames().contains(b)) {
            DeathStruct death = data.value<DeathStruct>();
            QStringList l = room->getTag(b).toStringList();
            if (l.contains(death.who->objectName()))
                return {SkillInvokeDetail(this, death.who, death.who, nullptr, true, nullptr, false)};
        }

        return {};
    }

    bool effect(TriggerEvent /*triggerEvent*/, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant & /*data*/) const override
    {
        QStringList roomTag = room->getTag(b).toStringList();

        roomTag.removeAll(invoke->invoker->objectName());
        if (roomTag.isEmpty())
            room->removeTag(b);
        else
            room->setTag(b, roomTag);

        room->revivePlayer(invoke->invoker, true);

        return false;
    }

private:
    QString b;
};

class TailorFuzhong : public TriggerSkill
{
public:
    TailorFuzhong()
        : TriggerSkill("tailorfuzhong")
    {
        frequency = Eternal;
        events = {EnterDying, Death};
    }

    bool playerRevivable(const Player *p) const override
    {
        if (p == nullptr)
            return false;

        Room *room = Sanguosha->currentRoom();
        if (room == nullptr)
            return false;

        return room->getTag(objectName()).toStringList().contains(p->objectName());
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room * /*room*/, const QVariant &data) const override
    {
        if (triggerEvent == EnterDying) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.who->hasSkill(this))
                return {SkillInvokeDetail(this, dying.who, dying.who, nullptr, true)};
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who->hasSkill(this))
                return {SkillInvokeDetail(this, death.who, death.who)};
        }

        return {};
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == Death) {
            QList<const Skill *> l = invoke->invoker->getVisibleSkillList();

            QStringList choices;

            foreach (const Skill *skill, l) {
                if (invoke->invoker->ownSkill(skill))
                    choices << skill->objectName();
            }

            if (choices.isEmpty())
                return false;

            choices << "dismiss";

            QString choice = room->askForChoice(invoke->invoker, objectName(), choices.join("+"));

            if (choice == "dismiss")
                return false;

            invoke->tag[objectName()] = choice;
            return true;
        }

        return TriggerSkill::cost(triggerEvent, room, invoke, data);
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant & /*data*/) const override
    {
        if (triggerEvent == EnterDying) {
            return true;
        } else if (triggerEvent == Death) {
            QString choice = invoke->tag[objectName()].toString();
            invoke->invoker->loseSkill(choice);

            QStringList roomTag;
            if (room->getTagNames().contains(objectName()))
                roomTag = room->getTag(objectName()).toStringList();

            roomTag << invoke->invoker->objectName();
            room->setTag(objectName(), roomTag);
        }
        return false;
    }
};

class TailorChenglu : public TriggerSkill
{
public:
    TailorChenglu()
        : TriggerSkill("tailorchenglu")
    {
        frequency = Eternal;
        events = {Revive};
    }

    void record(TriggerEvent /*triggerEvent*/, Room *room, QVariant &data) const override
    {
        QStringList roomTag;
        if (room->getTagNames().contains(objectName()))
            roomTag = room->getTag(objectName()).toStringList();

        ServerPlayer *p = data.value<ServerPlayer *>();
        if (roomTag.contains(p->objectName()))
            room->setPlayerMark(p, objectName(), 1);
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent /*triggerEvent*/, const Room * /*room*/, const QVariant &data) const override
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        if (p->hasSkill(this))
            return {SkillInvokeDetail(this, p, p)};

        return {};
    }

    // This is Eternal + Wake skill
    // It should be considered that Eternal should not be a frequency, as well as Compulsory
    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (invoke->invoker->getMark(objectName()) == 0)
            return TriggerSkill::cost(triggerEvent, room, invoke, data);

        room->setPlayerMark(invoke->invoker, "@waked", invoke->invoker->getMark("@waked") + 1);
        return false;
    }

    bool effect(TriggerEvent /*triggerEvent*/, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant & /*data*/) const override
    {
        room->doLightbox("$tailorchengluAnimate");

        QStringList roomTag;
        if (room->getTagNames().contains(objectName()))
            roomTag = room->getTag(objectName()).toStringList();

        roomTag << invoke->invoker->objectName();
        room->setTag(objectName(), roomTag);

        room->setPlayerMark(invoke->invoker, objectName(), 1);

        QStringList triggerList;

        // Currently no GameStart is considered. Both "huwei / kaifeng" invokes no GameStart

        if (room->changeMaxHpForAwakenSkill(invoke->invoker, 0)) {
            if (!invoke->invoker->ownSkill("tailorminxin")) {
                invoke->invoker->loseSkill("tailorfuzhong");
                invoke->invoker->loseSkill("#tailorfuzhong-effect");
                invoke->invoker->addSkill("huwei");

                triggerList = QStringList {"-tailorfuzhong", "huwei"};
            } else {
                invoke->invoker->addSkill("kaifeng");
                triggerList = QStringList {"kaifeng"};
            }
        }

        foreach (const QString &trigger, triggerList) {
            if (!trigger.startsWith("-")) {
                const Skill *skill = Sanguosha->getSkill(trigger);
                QList<const Skill *> related = Sanguosha->getRelatedSkills(trigger);
                related.prepend(skill);
                foreach (const Skill *skillx, related) {
                    const TriggerSkill *ts = dynamic_cast<const TriggerSkill *>(skillx);
                    if (ts != nullptr)
                        room->getThread()->addTriggerSkill(ts);
                    if (skillx != skill)
                        invoke->invoker->addSkill(skillx->objectName());
                }
            }
        }

        // do not trigger EventAcquireSkill / EventLoseSkill since it is not acquire / detach

        // foreach (const QString trigger, triggerList) {
        //     SkillAcquireDetachStruct s;
        //     s.isAcquire = trigger.startsWith("-");
        //     s.player = invoke->invoker;
        //     QString skillName = trigger;
        //     if (s.isAcquire)
        //         skillName = skillName.mid(1);
        //     s.skill = Sanguosha->getSkill(skillName);
        //     QVariant data = QVariant::fromValue(s);
        //     room->getThread()->trigger(s.isAcquire ? EventAcquireSkill : EventLoseSkill, room, data);
        // }

        return false;
    }
};

class TailorMinxin : public TriggerSkill
{
public:
    TailorMinxin()
        : TriggerSkill("tailorminxin")
    {
        events = {PostHpReduced};
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent /*triggerEvent*/, const Room * /*room*/, const QVariant &data) const override
    {
        int point = -1;
        ServerPlayer *p = nullptr;
        if (data.canConvert<DamageStruct>()) {
            DamageStruct damage = data.value<DamageStruct>();
            point = damage.damage;
            p = damage.to;
        } else if (data.canConvert<HpLostStruct>()) {
            HpLostStruct hplost = data.value<HpLostStruct>();
            point = hplost.num;
            p = hplost.player;
        }

        if (p != nullptr && point > 0 && p->getHp() + point > 0 && p->getHp() <= 0 && p->hasSkill(this))
            return {SkillInvokeDetail(this, p, p, nullptr, true)};

        return {};
    }

    bool cost(TriggerEvent /*triggerEvent*/, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant & /*data*/) const override
    {
        ServerPlayer *t = room->askForPlayerChosen(invoke->invoker, room->getAlivePlayers(), objectName(), "tailorminxin-select", false, true);
        if (t == nullptr && invoke->invoker->hasShownSkill(this))
            t = room->getAllPlayers().first();

        if (t == nullptr)
            return false;

        room->setPlayerProperty(invoke->invoker, "dyingFactor", invoke->invoker->getDyingFactor() + 1);
        invoke->targets << t;
        return true;
    }

    bool effect(TriggerEvent /*triggerEvent*/, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant & /*data*/) const override
    {
        room->damage({objectName(), invoke->invoker, invoke->targets.first(), 1, DamageStruct::Fire});

        return false;
    }
};

class TailorMiezui : public TriggerSkill
{
public:
    TailorMiezui()
        : TriggerSkill("tailormiezui")
    {
        events = {Dying, Damage, EventLoseSkill, Death};
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventLoseSkill || triggerEvent == Death) {
            QVariantMap l;
            if (room->getTagNames().contains(objectName()))
                l = room->getTag(objectName()).toMap();

            ServerPlayer *p = nullptr;
            if (triggerEvent == EventLoseSkill)
                p = data.value<SkillAcquireDetachStruct>().player;
            else if (triggerEvent == Death)
                p = data.value<DeathStruct>().who;

            if (p != nullptr) {
                if (l.contains(p->objectName())) {
                    QStringList s = l.value(p->objectName()).toStringList();
                    foreach (const QString &sp, s) {
                        ServerPlayer *t = room->findPlayerByObjectName(sp, true);
                        if (t != nullptr)
                            room->removePlayerMark(t, "@" + objectName(), 1);
                    }

                    l.remove(p->objectName());
                    room->setTag(objectName(), l);
                }
            }
        } else if (triggerEvent == Dying) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.damage != nullptr) {
                dying.damage->trigger_info << objectName();
                if (dying.damage->from != nullptr)
                    dying.damage->from->tag[objectName()] = true;
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.trigger_info.contains(objectName()) && damage.from != nullptr && damage.from->hasSkill(this) && damage.from->isAlive()
                && damage.to->tag.contains(objectName()) && damage.to->tag.value(objectName()).toBool()) {
                QVariantMap l;
                if (room->getTagNames().contains(objectName()))
                    l = room->getTag(objectName()).toMap();

                QStringList s;
                if (l.contains(damage.from->objectName()))
                    s = l.value(damage.from->objectName()).toStringList();

                if (!s.contains(damage.to->objectName()))
                    return {SkillInvokeDetail(this, damage.from, damage.from, damage.to)};
            }
        }

        return {};
    }

    bool cost(TriggerEvent /*triggerEvent*/, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        QString choice = room->askForChoice(invoke->invoker, objectName(), "dismiss+0+1+2", data);
        if (choice == "dismiss")
            return false;

        int n = choice.toInt();
        invoke->tag[objectName()] = n;

        QVariantMap l;
        if (room->getTagNames().contains(objectName()))
            l = room->getTag(objectName()).toMap();

        QStringList s;
        if (l.contains(invoke->invoker->objectName()))
            s = l.value(invoke->invoker->objectName()).toStringList();

        s << invoke->targets.first()->objectName();

        l[invoke->invoker->objectName()] = s;
        room->setTag(objectName(), l);

        return true;
    }

    bool effect(TriggerEvent /*triggerEvent*/, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant & /*data*/) const override
    {
        int n = invoke->tag[objectName()].toInt();

        if (n != 0)
            room->setPlayerProperty(invoke->invoker, "dyingFactor", invoke->invoker->getDyingFactor() - n);

        if (invoke->invoker->dyingThreshold() == 1 && room->askForSkillInvoke(invoke->invoker, objectName(), "death"))
            room->killPlayer(invoke->invoker);

        return false;
    }
};

PlaygroundPackage::PlaygroundPackage()
    : Package("playground")
{
    General *dovefs = new General(this, "fsu0413", "touhougod", 4, true);
    dovefs->addSkill(new Fsu0413Gainian);
    dovefs->addSkill(new Fsu0413GainianDis);
    dovefs->addSkill(new Fsu0413Lese);
    related_skills.insertMulti("fsu0413gainian", "#fsu0413gainian-dis");

    General *otaku = new General(this, "otaku", "touhougod", 5, true);
    otaku->addSkill(new Fsu0413Fei2Zhai);
    otaku->addSkill(new Fsu0413Fei4Zhai);

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

    General *tailormokou = new General(this, "tailormokou", "touhougod", 2);
    tailormokou->addSkill(new TailorFuzhong);
    tailormokou->addSkill(new TailorFuzhongEffect);
    tailormokou->addSkill(new TailorChenglu);
    tailormokou->addSkill(new TailorMinxin);
    tailormokou->addSkill(new TailorMiezui);
    related_skills.insertMulti("tailorfuzhong", "#tailorfuzhong-effect");
}

ADD_PACKAGE(Playground)
