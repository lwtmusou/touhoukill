#include "playground.h"
#include "clientplayer.h"
#include "engine.h"
#include "general.h"
#include "skill.h"

class Fsu0413Gepi : public TriggerSkill
{
public:
    Fsu0413Gepi()
        : TriggerSkill("fsu0413gepi")
    {
        events << EventPhaseStart;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
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

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
            int id = room->askForCardChosen(invoke->targets.first(), invoke->invoker, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(id, invoke->invoker, invoke->invoker == invoke->targets.first() ? NULL : invoke->targets.first());
            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = invoke->targets.first();

        QList<const Skill *> skills = player->getVisibleSkillList();
        QList<const Skill *> skills_canselect;

        foreach (const Skill *s, skills) {
            if (!s->isLordSkill() && s->getFrequency() != Skill::Wake && s->getFrequency() != Skill::Eternal && !s->isAttachedLordSkill())
                skills_canselect << s;
        }

        if (!skills_canselect.isEmpty()) {
            QStringList l;
            foreach (const Skill *s, skills_canselect)
                l << s->objectName();

            QString skill_lose = l.first();
            if (l.length() > 1)
                skill_lose = room->askForChoice(invoke->invoker, objectName(), l.join("+"), data);

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

        if (player->isAlive())
            player->drawCards(3, objectName());

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
                if ((copy->match(pattern) || Sanguosha->matchExpPattern(pattern, player, copy.data())) && !player->isCardLimited(copy.data(), Card::MethodUse))
                    return true;
            }
        }

        foreach (int handPileId, player->getHandPile()) {
            const Card *handCard = Sanguosha->getCard(handPileId);
            if (handCard != NULL && handCard->isKindOf("DelayedTrick")) {
                copy->clearSubcards();
                copy->addSubcard(handCard);
                if ((copy->match(pattern) || Sanguosha->matchExpPattern(pattern, player, copy.data())) && !player->isCardLimited(copy.data(), Card::MethodUse))
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
        foreach (const Card *c, map) {
            QScopedPointer<Card> copy(Sanguosha->cloneCard(c));
            copy->setSkillName("fsu0413gainian");
            foreach (const Card *handCard, Self->getHandcards()) {
                if (handCard->isKindOf("DelayedTrick")) {
                    copy->clearSubcards();
                    copy->addSubcard(handCard);
                    if ((copy->match(Sanguosha->getCurrentCardUsePattern()) || Sanguosha->matchExpPattern(Sanguosha->getCurrentCardUsePattern(), Self, copy.data()))
                        && !Self->isCardLimited(copy.data(), Card::MethodUse)) {
                        availableCards << copy->objectName();
                        break;
                    }
                }
            }

            foreach (int handPileId, Self->getHandPile()) {
                const Card *handCard = Sanguosha->getCard(handPileId);
                if (handCard != NULL && handCard->isKindOf("DelayedTrick")) {
                    copy->clearSubcards();
                    copy->addSubcard(handCard);
                    if ((copy->match(Sanguosha->getCurrentCardUsePattern()) || Sanguosha->matchExpPattern(Sanguosha->getCurrentCardUsePattern(), Self, copy.data()))
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
                if (handCard != NULL && handCard->isKindOf("DelayedTrick")) {
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

    QDialog *getDialog() const
    {
        return Fsu0413GainianDialog::getInstance("fsu0413gainian");
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        foreach (const Card *card, player->getHandcards()) {
            if (card->isKindOf("DelayedTrick"))
                return true;
        }

        foreach (int id, player->getHandPile()) {
            const Card *card = Sanguosha->getCard(id);
            if (card != NULL && card->isKindOf("DelayedTrick"))
                return true;
        }

        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            Fsu0413GainianDialog *d = qobject_cast<Fsu0413GainianDialog *>(getDialog());
            if (d != NULL)
                return d->isResponseOk(player, pattern);
        }

        return false;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        if (!Self->tag.contains("fsu0413gainian") || Self->tag["fsu0413gainian"].toString().length() == 0)
            return NULL;

        Card *c = Sanguosha->cloneCard(Self->tag["fsu0413gainian"].toString());
        if (c == NULL)
            return NULL;

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

    int getDistanceLimit(const Player *from, const Card *card) const
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        bool flag = false;
        foreach (int id, const_cast<Room *>(room)->getDiscardPile()) {
            const Card *c = Sanguosha->getCard(id);
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
            r << SkillInvokeDetail(this, st.player, st.player, NULL, true);

        return r;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (TriggerSkill::cost(triggerEvent, room, invoke, data)) {
            LogMessage l;
            l.type = "#TriggerSkill";
            l.from = invoke->invoker;
            l.arg = objectName();
            room->sendLog(l);

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
    {
        DrawNCardsStruct st = data.value<DrawNCardsStruct>();
        QList<int> delayedtricks;

        foreach (int id, room->getDiscardPile()) {
            const Card *c = Sanguosha->getCard(id);
            if (c && c->isKindOf("DelayedTrick"))
                delayedtricks << id;
        }

        int obtainId = delayedtricks.at(qrand() % delayedtricks.length());
        st.player->obtainCard(Sanguosha->getCard(obtainId));

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
    related_skills.insertMulti("fsu0413gainian", "#fsu0413gainian-dis");
}

ADD_PACKAGE(Playground)
