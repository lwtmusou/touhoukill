#include "th10.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "settings.h"
#include "skill.h"
#include "standard.h"
#include "th13.h"
#include "washout.h"
#include <QCommandLinkButton>
#include <QCoreApplication>
#include <QPointer>

class ShendeVS : public ViewAsSkill
{
public:
    ShendeVS()
        : ViewAsSkill("shende")
    {
        expand_pile = "shende";
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        QStringList checkedList;

        if (player->getPile("shende").length() >= 2) {
            QList<int> ids = Sanguosha->getRandomCards();
            foreach (int id, ids) {
                const Card *c = Sanguosha->getCard(id);
                if (!checkedList.contains(c->objectName()) && c->getTypeId() == Card::TypeBasic) {
                    checkedList << c->objectName();
                    Card *clonedCard = Sanguosha->cloneCard(c->objectName(), Card::NoSuit, 0);
                    DELETE_OVER_SCOPE(Card, clonedCard);
                    if (clonedCard->isAvailable(player) && !player->isCardLimited(clonedCard, Card::MethodUse))
                        return true;
                }
            }
        }

        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        Card::HandlingMethod method = Card::MethodUse;
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            method = Card::MethodResponse;

        QStringList checkedList;

        if (player->getPile("shende").length() >= 2) {
            QList<int> ids = Sanguosha->getRandomCards();
            foreach (int id, ids) {
                const Card *c = Sanguosha->getCard(id);
                if (!checkedList.contains(c->objectName()) && c->getTypeId() == Card::TypeBasic) {
                    checkedList << c->objectName();
                    Card *clonedCard = Sanguosha->cloneCard(c->objectName(), Card::NoSuit, 0);
                    DELETE_OVER_SCOPE(Card, clonedCard);
                    const CardPattern *cardPattern = Sanguosha->getPattern(pattern);
                    if (cardPattern != nullptr && cardPattern->match(player, clonedCard) && !player->isCardLimited(clonedCard, method))
                        return true;
                }
            }
        }

        return false;
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        if (!Self->tag.contains("shende"))
            return false;
        if (selected.length() >= 2)
            return false;
        return Self->getPile("shende").contains(to_select->getEffectiveId());
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (!Self->tag.contains("shende"))
            return false;

        if (cards.length() != 2)
            return nullptr;

        QString cardname = Self->tag.value("shende").toString();

        Card *peach = Sanguosha->cloneCard(cardname);
        if (peach != nullptr) {
            peach->addSubcards(cards);
            peach->setSkillName("shende");
            return peach;
        }

        return nullptr;
    }
};

class Shende : public TriggerSkill
{
public:
    Shende()
        : TriggerSkill("shende")
    {
        events = {CardUsed, CardResponded, CardsMoveOneTime};
        view_as_skill = new ShendeVS;
        related_pile = "shende";
    }

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card->isKindOf("Slash") && resp.m_from->hasSkill(this))
                return {SkillInvokeDetail(this, resp.m_from, resp.m_from)};
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.from->hasSkill(this))
                return {SkillInvokeDetail(this, use.from, use.from)};
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
            if (from != nullptr && from->isAlive() && from->hasSkill(this) && from->getPhase() == Player::Discard
                && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)) {
                foreach (int id, move.card_ids) {
                    const Card *c = Sanguosha->getCard(id);
                    if (c->isKindOf("Slash"))
                        return {SkillInvokeDetail(this, from, from)};
                }
            }
        }
        return {};
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->drawCards(1);
        if (!invoke->invoker->isKongcheng()) {
            const Card *cards = room->askForExchange(invoke->invoker, objectName(), 1, 1, false, "shende-exchange");
            DELETE_OVER_SCOPE(const Card, cards)
            invoke->invoker->addToPile("shende", cards->getSubcards().constFirst());
        }
        return false;
    }

    QDialog *getDialog() const override
    {
        return QijiDialog::getInstance("shende", true, false);
    }
};

class Qiankun : public MaxCardsSkill
{
public:
    Qiankun()
        : MaxCardsSkill("qiankun")
    {
        show_type = "static";
    }

    int getExtra(const Player *target) const override
    {
        if (target->hasSkill(objectName()) && target->hasShownSkill(objectName()))
            return 1;
        else
            return 0;
    }
};

class Qiankun2 : public AttackRangeSkill
{
public:
    Qiankun2()
        : AttackRangeSkill("#qiankun2")
    {
    }

    int getExtra(const Player *target, bool) const override
    {
        if (target->hasSkill("qiankun") && target->hasShownSkill("qiankun"))
            return 1;
        else
            return 0;
    }
};

GongfengCard::GongfengCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "gongfeng_attach";
}

void GongfengCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    const QList<ServerPlayer *> &targets = card_use.to;

    ServerPlayer *kanako = targets.first();
    room->setPlayerFlag(kanako, "gongfengInvoked");

    room->notifySkillInvoked(kanako, "gongfeng");
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), kanako->objectName(), "gongfeng", QString());
    room->obtainCard(kanako, this, reason);

    if (Slash::IsAvailable(source) && room->askForSkillInvoke(source, "gongfeng_attach", QVariant::fromValue(card_use), "tooTroublesome:" + kanako->objectName())) {
        room->setPlayerFlag(kanako, "SlashRecorder_gongfeng");
        kanako->tag["gongfeng_use"] = QVariant::fromValue(card_use);
        if (!room->askForUseCard(kanako, "Slash", "@gongfeng-slash:" + source->objectName())) {
            room->setPlayerFlag(kanako, "-SlashRecorder_gongfeng");
            kanako->tag.remove("gongfeng_use");
        }
    }
}

bool GongfengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasLordSkill("gongfeng") && to_select != Self && !to_select->hasFlag("gongfengInvoked");
}

class GongfengVS : public OneCardViewAsSkill
{
public:
    GongfengVS()
        : OneCardViewAsSkill("gongfeng_attach")
    {
        attached_lord_skill = true;
        filter_pattern = "Slash";
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        if (!shouldBeVisible(player))
            return false;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->hasLordSkill("gongfeng") && !p->hasFlag("gongfengInvoked"))
                return true;
        }
        return false;
    }

    bool shouldBeVisible(const Player *Self) const override
    {
        return (Self != nullptr) && Self->getKingdom() == "fsl";
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        GongfengCard *card = new GongfengCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Gongfeng : public TriggerSkill
{
public:
    Gongfeng()
        : TriggerSkill("gongfeng$")
    {
        events = {GameStart, EventAcquireSkill, EventLoseSkill, Revive, EventPhaseChanging, PreCardUsed};
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from == Player::Play) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("gongfengInvoked"))
                        room->setPlayerFlag(p, "-gongfengInvoked");
                }
            }
        } else if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != nullptr && use.card->isKindOf("Slash") && use.card->hasFlag("gongfeng") && use.from != nullptr) {
                QVariant gongfeng_useV = use.from->tag.value("gongfeng_use");
                if (gongfeng_useV.canConvert<CardUseStruct>()) {
                    CardUseStruct gongfeng_use = gongfeng_useV.value<CardUseStruct>();
                    if (gongfeng_use.from && gongfeng_use.from->isAlive() && gongfeng_use.from->getPhase() == Player::Play) {
                        room->sendLog("#gongfeng-addhistory", gongfeng_use.from, use.card->objectName(), {}, objectName());
                        room->addPlayerHistory(gongfeng_use.from, use.card->getClassName());
                    }
                }
                use.from->tag.remove("gongfeng_use");
            }
        } else { //the case operating attach skill
            static QString attachName = "gongfeng_attach";
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasLordSkill(this, true))
                    lords << p;
            }

            if (lords.length() > 1) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->hasSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else if (lords.length() == 1) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasLordSkill(this, true) && p->hasSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                    else if (!p->hasLordSkill(this, true) && !p->hasSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            } else {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                }
            }
        }
    }
};

class Bushu : public TriggerSkill
{
public:
    Bushu()
        : TriggerSkill("bushu")
    {
        events << Pindian << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == Pindian) {
            PindianStruct *pindian = data.value<PindianStruct *>();
            if (pindian->reason != "bushu")
                return QList<SkillInvokeDetail>();
            ServerPlayer *target = pindian->from->tag["suwako_bushu"].value<ServerPlayer *>();
            if (pindian->success && (target != nullptr) && target->isAlive())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, nullptr, pindian->from, nullptr, true, target);
            else if (!pindian->success && pindian->from->isAlive())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, nullptr, pindian->from, nullptr, true);

        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if ((damage.from == nullptr) || damage.to->isDead() || !damage.from->isAlive())
                return QList<SkillInvokeDetail>();

            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *suwako, room->findPlayersBySkillName(objectName())) {
                if (damage.from != suwako && !damage.from->isKongcheng() && (suwako->inMyAttackRange(damage.to) || suwako == damage.to) && !suwako->isKongcheng())
                    d << SkillInvokeDetail(this, suwako, suwako);
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            QString prompt = "damage:" + damage.from->objectName() + ":" + damage.to->objectName();
            return invoke->invoker->askForSkillInvoke(objectName(), data, prompt);
        }
        invoke->invoker->tag.remove("suwako_bushu");
        return true;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *suwako = invoke->invoker;
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, suwako->objectName(), damage.from->objectName());
            invoke->invoker->tag["suwako_bushu"] = QVariant::fromValue(damage.to);
            suwako->pindian(damage.from, objectName(), nullptr);
        } else {
            PindianStruct *pindian = data.value<PindianStruct *>();
            if (pindian->success) {
                RecoverStruct recov;
                recov.who = suwako;
                room->recover(invoke->targets.first(), recov);
            } else
                suwako->obtainCard(pindian->to_card);
        }
        return false;
    }
};

class Chuancheng : public TriggerSkill
{
public:
    Chuancheng()
        : TriggerSkill("chuancheng")
    {
        events << Death;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who->hasSkill(objectName()))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, death.who, death.who);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(), "@chuancheng", true, true);
        if (target != nullptr) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = invoke->targets.first();
        room->handleAcquireDetachSkills(target, "qiankun");
        room->handleAcquireDetachSkills(target, "chuancheng");
        if (invoke->invoker->getCards("hejs").length() > 0) {
            DummyCard *allcard = new DummyCard;
            allcard->deleteLater();
            allcard->addSubcards(invoke->invoker->getCards("hejs"));
            room->obtainCard(target, allcard, CardMoveReason(CardMoveReason::S_REASON_RECYCLE, target->objectName()), false);
        }
        return false;
    }
};

class DfgzmJiyiVS : public ViewAsSkill
{
public:
    DfgzmJiyiVS()
        : ViewAsSkill("dfgzmjiyi")
    {
        response_pattern = "@@dfgzmjiyi";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        return selected.length() < 2 && !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() == 2) {
            DummyCard *c = new DummyCard;
            c->addSubcards(cards);
            return c;
        }

        return nullptr;
    }
};

class DfgzmJiyi : public TriggerSkill
{
public:
    DfgzmJiyi()
        : TriggerSkill("dfgzmjiyi")
    {
        events << EventPhaseEnd << EventPhaseChanging;
        view_as_skill = new DfgzmJiyiVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        if (e == EventPhaseEnd) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Draw && player->hasSkill(this) && player->getCardCount() >= 2)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && change.player->hasFlag(objectName())) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, nullptr, change.player, nullptr, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseEnd)
            return room->askForCard(invoke->invoker, "@@dfgzmjiyi", "@dfgzmjiyi-discard", QVariant(), Card::MethodDiscard, nullptr, false, objectName());

        return TriggerSkill::cost(triggerEvent, room, invoke, data);
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (e == EventPhaseEnd) {
            invoke->invoker->setFlags(objectName());
            return true;
        } else if (e == EventPhaseChanging) {
            room->sendLog("#TouhouBuff", invoke->invoker, objectName());
            room->notifySkillInvoked(invoke->invoker, objectName());
            invoke->invoker->drawCards(3);
        }
        return false;
    }
};

QijiDialog *QijiDialog::getInstance(const QString &object, bool left, bool right)
{
    static QPointer<QijiDialog> instance;

    if (!instance.isNull() && instance->objectName() != object)
        delete instance;

    if (instance.isNull()) {
        instance = new QijiDialog(object, left, right);
        connect(qApp, &QCoreApplication::aboutToQuit, instance.data(), &QijiDialog::deleteLater);
    }

    return instance;
}

QijiDialog::QijiDialog(const QString &object, bool left, bool right)
    : object_name(object)
{
    setObjectName(object);
    setWindowTitle(Sanguosha->translate(object)); //need translate title?
    group = new QButtonGroup(this);

    QHBoxLayout *layout = new QHBoxLayout;
    if (left)
        layout->addWidget(createLeft());
    if (right)
        layout->addWidget(createRight());
    setLayout(layout);

    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectCard(QAbstractButton *)));
}

void QijiDialog::popup()
{
    Self->tag.remove(object_name);

    Card::HandlingMethod method = Card::MethodUse;
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        method = Card::MethodResponse;

    QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
    const CardPattern *cardPattern = Sanguosha->getPattern(pattern);
    bool play = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY);

    //collect available patterns for specific skill
    QStringList validPatterns;
    QStringList ban_list = Sanguosha->getBanPackages();
    if (object_name == "huaxiang") {
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        foreach (const Card *card, cards) {
            if (card->isKindOf("BasicCard") && !ban_list.contains(card->getPackage())) {
                QString name = card->objectName();
                if (!validPatterns.contains(name)) {
                    if (name.contains("jink") && Self->getMaxHp() > 3)
                        continue;
                    else if (name.contains("peach") && Self->getMaxHp() > 2)
                        continue;
                    validPatterns << name;
                }
            }
        }
    } else if (object_name == "hezhou") {
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        foreach (const Card *card, cards) {
            if ((!Self->isCurrent() && card->isKindOf("Peach"))
                || (Self->isCurrent() && card->isNDTrick() && !card->isKindOf("AOE") && !card->isKindOf("GlobalEffect")) && !ban_list.contains(card->getPackage())) {
                QString name = card->objectName();
                if (!validPatterns.contains(name))
                    validPatterns << card->objectName();
            }
        }
    } else if (object_name == "shende") {
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        foreach (const Card *card, cards) {
            if (card->isKindOf("BasicCard") && !ban_list.contains(card->getPackage())) {
                QString name = card->objectName();
                if (!validPatterns.contains(name))
                    validPatterns << name;
            }
        }
    } else {
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        foreach (const Card *card, cards) {
            if ((card->isNDTrick() || card->isKindOf("BasicCard")) && !ban_list.contains(card->getPackage())) {
                QString name = card->objectName();
                if (!validPatterns.contains(name))
                    validPatterns << card->objectName();
            }
        }
    }

    //then match it and check "CardLimit"
    QStringList checkedPatterns;
    foreach (const QString &str, validPatterns) {
        Card *card = Sanguosha->cloneCard(str);
        DELETE_OVER_SCOPE(Card, card)
        if (play || (cardPattern != nullptr && cardPattern->match(Self, card)) && !Self->isCardLimited(card, method))
            checkedPatterns << str;
    }

    if (checkedPatterns.isEmpty()) { // not available
        emit onButtonClick();
        return;
    }

    QStringList availablePatterns;
    foreach (QAbstractButton *button, group->buttons()) {
        const Card *card = map[button->objectName()];
        const Player *user = nullptr;
        if (object_name == "chuangshi") { //check the card is Available for chuangshi target.
            foreach (const Player *p, Self->getAliveSiblings()) {
                if (p->getMark("chuangshi_user") > 0) {
                    user = p;
                    break;
                }
            }
        }

        if (user == nullptr)
            user = Self;

        bool available = (!play) || card->isAvailable(user);
        if (card->isKindOf("Peach"))
            available = card->isAvailable(user);
        if (object_name == "chuangshi" && (card->isKindOf("Jink") || card->isKindOf("Nullification")))
            available = false;
        if (object_name == "qiji" && (user->getMark("xiubu") != 0))
            available = true;

        bool checked = checkedPatterns.contains(card->objectName());
        //check isCardLimited
        bool enabled = available && (checked || object_name == "chuangshi");
        button->setEnabled(enabled);
        if (enabled)
            availablePatterns << card->objectName();
    }

    if (availablePatterns.length() == 1) {
        Self->tag[object_name] = availablePatterns.first();
        emit onButtonClick();
        return;
    }

    exec();
}

void QijiDialog::selectCard(QAbstractButton *button)
{
    const Card *card = map.value(button->objectName());
    Self->tag[object_name] = QVariant::fromValue(card->objectName());

    emit onButtonClick();
    accept();
}

QGroupBox *QijiDialog::createLeft()
{
    QGroupBox *box = new QGroupBox;
    box->setTitle(Sanguosha->translate("basic"));

    QVBoxLayout *layout = new QVBoxLayout;

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    QStringList ban_list = Sanguosha->getBanPackages();
    foreach (const Card *card, cards) {
        if (card->getTypeId() == Card::TypeBasic && !map.contains(card->objectName())
            && !ban_list.contains(card->getPackage())) { // && !ServerInfo.Extensions.contains("!" + card->getPackage())
            Card *c = Sanguosha->cloneCard(card->objectName());
            c->setParent(this);
            layout->addWidget(createButton(c));
        }
    }

    layout->addStretch();
    box->setLayout(layout);
    return box;
}

QGroupBox *QijiDialog::createRight()
{
    QGroupBox *box = new QGroupBox(Sanguosha->translate("ndtrick"));
    QHBoxLayout *layout = new QHBoxLayout;

    QGroupBox *box1 = new QGroupBox(Sanguosha->translate("single_target_trick"));
    QVBoxLayout *layout1 = new QVBoxLayout;

    QGroupBox *box2 = new QGroupBox(Sanguosha->translate("multiple_target_trick"));
    QVBoxLayout *layout2 = new QVBoxLayout;

    QStringList ban_list = Sanguosha->getBanPackages();

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    foreach (const Card *card, cards) {
        if (card->isNDTrick() && !map.contains(card->objectName()) && !ban_list.contains(card->getPackage())) {
            if (object_name == "chuangshi" || object_name == "hezhou") {
                if (!card->isNDTrick() || card->isKindOf("AOE") || card->isKindOf("GlobalEffect"))
                    continue;
            }

            Card *c = Sanguosha->cloneCard(card->objectName());
            c->setSkillName(object_name);
            c->setParent(this);

            QVBoxLayout *layout = c->isKindOf("SingleTargetTrick") ? layout1 : layout2;
            layout->addWidget(createButton(c));
        }
    }

    box->setLayout(layout);
    box1->setLayout(layout1);
    box2->setLayout(layout2);

    layout1->addStretch();
    layout2->addStretch();

    layout->addWidget(box1);
    layout->addWidget(box2);
    return box;
}

QAbstractButton *QijiDialog::createButton(const Card *card)
{
    if (card->objectName() == "slash" && map.contains(card->objectName()) && !map.contains("normal_slash")) {
        QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate("normal_slash"));
        button->setObjectName("normal_slash");
        button->setToolTip(card->getDescription());

        map.insert("normal_slash", card);
        group->addButton(button);

        return button;
    } else {
        QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate(card->objectName()));
        button->setObjectName(card->objectName());
        button->setToolTip(card->getDescription());

        map.insert(card->objectName(), card);
        group->addButton(button);

        return button;
    }
}

class QijiVS : public OneCardViewAsSkill
{
public:
    QijiVS()
        : OneCardViewAsSkill("qiji")
    {
        filter_pattern = ".|.|.|hand";
    }

    static QStringList responsePatterns()
    {
        const CardPattern *pattern = Sanguosha->getPattern(Sanguosha->currentRoomState()->getCurrentCardUsePattern());

        Card::HandlingMethod method = Card::MethodUse;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            method = Card::MethodResponse;

        QStringList checkedPatterns;
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        QStringList ban_list = Sanguosha->getBanPackages();
        foreach (const Card *card, cards) {
            if ((card->isNDTrick() || card->isKindOf("BasicCard")) && !ban_list.contains(card->getPackage())) { //&& !ServerInfo.Extensions.contains("!" + card->getPackage())
                QString p = card->objectName();
                if (!checkedPatterns.contains(p) && (pattern != nullptr && pattern->match(Self, card)) && !Self->isCardLimited(card, method))
                    checkedPatterns << p;
            }
        }
        return checkedPatterns;
    }

    bool isEnabledAtResponse(const Player *player, const QString &) const override
    {
        if (player->getHandcardNum() != 1)
            return false;
        if (player->getMark("qiji") > 0)
            return false;
        //check main phase
        if (player->isCurrent()) {
            if (!player->isInMainPhase())
                return false;
        } else {
            foreach (const Player *p, player->getSiblings()) {
                if (p->isCurrent()) {
                    if (!p->isInMainPhase())
                        return false;
                    break;
                }
            }
        }

        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.contains("peach") && checkedPatterns.length() == 1 && player->getMark("Global_PreventPeach") > 0)
            return false;
        return !checkedPatterns.isEmpty();
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        if (player->getMark("qiji") > 0)
            return false;
        return player->getHandcardNum() == 1;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.length() == 1) {
            Card *card = Sanguosha->cloneCard(checkedPatterns.first());
            card->setSkillName(objectName());
            card->addSubcard(originalCard);
            card->setCanRecast(false);
            return card;
        }

        QString name = Self->tag.value("qiji", QString()).toString();
        if (name != nullptr) {
            Card *card = Sanguosha->cloneCard(name);
            card->setSkillName(objectName());
            card->addSubcard(originalCard);
            card->setCanRecast(false);
            return card;
        } else
            return nullptr;
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const override
    {
        if (player->getMark("qiji") > 0)
            return false;
        //check main phase
        if (player->isCurrent()) {
            if (!player->isInMainPhase())
                return false;
        } else {
            foreach (const Player *p, player->getSiblings()) {
                if (p->isCurrent()) {
                    if (!p->isInMainPhase())
                        return false;
                    break;
                }
            }
        }

        Nullification *nul = new Nullification(Card::NoSuit, 0);
        DELETE_OVER_SCOPE(Nullification, nul)
        if (player->isCardLimited(nul, Card::MethodUse, true))
            return false;
        return player->getHandcardNum() == 1;
    }
};

class Qiji : public TriggerSkill
{
public:
    Qiji()
        : TriggerSkill("qiji")
    {
        events << EventPhaseChanging << PreCardUsed << CardResponded;
        view_as_skill = new QijiVS;
    }

    QDialog *getDialog() const override
    {
        return QijiDialog::getInstance("qiji");
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getMark("qiji") > 0)
                    room->setPlayerMark(p, "qiji", 0);
            }
        }

        if (e == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == objectName())
                room->setPlayerMark(use.from, "qiji", 1);
        }
        if (e == CardResponded) {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if ((response.m_from != nullptr) && (response.m_card != nullptr) && response.m_card->getSkillName() == objectName())
                room->setPlayerMark(response.m_from, "qiji", 1);
        }
    }
};

FengshenCard::FengshenCard()
{
    will_throw = true;
    m_skillName = "fengshen";
}

bool FengshenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (to_select == Self)
        return false;
    else if (targets.isEmpty())
        return Self->inMyAttackRange(to_select);
    else if (targets.length() == 1)
        return Self->distanceTo(targets.first()) == 1 && Self->distanceTo(to_select) == 1;
    else
        return Self->distanceTo(to_select) == 1;
}

void FengshenCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    const Card *card = room->askForCard(effect.to, "Slash", "@fengshen-discard:" + effect.from->objectName());
    if (card == nullptr)
        room->damage(DamageStruct("fenshen", effect.from, effect.to));
}

class Fengshen : public OneCardViewAsSkill
{
public:
    Fengshen()
        : OneCardViewAsSkill("fengshen")
    {
        filter_pattern = ".|red|.|hand!";
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("FengshenCard") && !player->isKongcheng();
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        if (originalCard != nullptr) {
            FengshenCard *card = new FengshenCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return nullptr;
    }
};

ShowFengsu::ShowFengsu()
    : ShowDistanceCard()
{
}

class FengsuDistance : public DistanceSkill
{
public:
    FengsuDistance()
        : DistanceSkill("#fengsu-distance")
    {
        show_type = "static";
    }

    int getCorrect(const Player *from, const Player *to) const override
    {
        int correct = 0;
        if (isHegemonyGameMode(ServerInfo.GameMode)) {
            if (from->hasSkill("fengsu") && from->hasShownSkill("fengsu"))
                correct = correct - (from->getLostHp());

            if (to->hasSkill("fengsu") && to->hasShownSkill("fengsu"))
                correct = correct + to->getLostHp();
        } else {
            if (from->hasSkill("fengsu"))
                correct = correct - (from->getLostHp());

            if (to->hasSkill("fengsu"))
                correct = correct + to->getLostHp();
        }

        return correct;
    }
};

class Fengsu : public TriggerSkill
{
public:
    Fengsu()
        : TriggerSkill("fengsu")
    {
        events << HpChanged;
        frequency = Compulsory;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const override
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if ((player != nullptr) && player->hasSkill("fengsu") && player->hasShownSkill("fengsu"))
            room->notifySkillInvoked(player, "fengsu");
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (isHegemonyGameMode(ServerInfo.GameMode) && (player != nullptr) && player->hasSkill(objectName()) && !player->hasShownSkill(objectName())) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }

        return QList<SkillInvokeDetail>();
    }
};

XinshangCard::XinshangCard()
{
}

void XinshangCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    effect.to->drawCards(1);

    const Card *card = room->askForCard(effect.to, ".S", "@xinshang-spadecard:" + effect.from->objectName(), QVariant::fromValue(effect.from), Card::MethodNone, nullptr, false,
                                        "xinshang", false);
    if (card != nullptr) {
        room->obtainCard(effect.from, card);
        room->setPlayerFlag(effect.from, "xinshang_effect");
    } else {
        if (effect.from->isAlive() && effect.from->canDiscard(effect.to, "hes")) {
            room->throwCard(room->askForCardChosen(effect.from, effect.to, "hes", "xinshang", false, Card::MethodDiscard), effect.to, effect.from);
            if (effect.from->isAlive() && effect.from->canDiscard(effect.to, "hes"))
                room->throwCard(room->askForCardChosen(effect.from, effect.to, "hes", "xinshang", false, Card::MethodDiscard), effect.to, effect.from);
        }
    }
}

class Xinshang : public ZeroCardViewAsSkill
{
public:
    Xinshang()
        : ZeroCardViewAsSkill("xinshang")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("XinshangCard");
    }

    const Card *viewAs() const override
    {
        return new XinshangCard;
    }
};

class XinshangTargetMod : public TargetModSkill
{
public:
    XinshangTargetMod()
        : TargetModSkill("#xinshang_effect")
    {
        pattern = "BasicCard,TrickCard";
    }

    int getDistanceLimit(const Player *from, const Card *) const override
    {
        if (from->hasFlag("xinshang_effect"))
            return 1000;
        else
            return 0;
    }

    int getResidueNum(const Player *from, const Card *) const override
    {
        if (from->hasFlag("xinshang_effect"))
            return 1000;
        else
            return 0;
    }
};

class Micai : public TriggerSkill
{
public:
    Micai()
        : TriggerSkill("micai")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->hasSkill(this) && damage.damage > damage.to->getHandcardNum())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, nullptr, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *player = invoke->invoker;
        room->sendLog("#micai01", player, "micai", QList<ServerPlayer *>(), QString::number(1));
        damage.damage = damage.damage - 1;
        room->notifySkillInvoked(player, objectName());
        if (damage.damage == 0)
            return true;
        data = QVariant::fromValue(damage);

        return false;
    }
};

class Jie : public TriggerSkill
{
public:
    Jie()
        : TriggerSkill("jie")
    {
        events << DamageInflicted;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *hina, room->findPlayersBySkillName(objectName())) {
            if ((hina != nullptr) && damage.to != hina)
                d << SkillInvokeDetail(this, hina, hina);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt;
        QString nature;
        switch (damage.nature) {
        case DamageStruct::Normal:
            nature = "normal_nature";
            break;
        case DamageStruct::Fire:
            nature = "fire_nature";
            break;
        case DamageStruct::Thunder:
            nature = "thunder_nature";
            break;
        case DamageStruct::Ice:
            nature = "ice_nature";
            break;
        }
        if (damage.from != nullptr)
            prompt = "transfer1:" + damage.to->objectName() + ":" + damage.from->objectName() + ":" + QString::number(damage.damage) + ":" + nature;
        else
            prompt = "transfer2:" + damage.to->objectName() + "::" + QString::number(damage.damage) + ":" + nature;
        return room->askForSkillInvoke(invoke->invoker, objectName(), data, prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        invoke->invoker->drawCards(1);
        damage.to = invoke->invoker;
        damage.transfer = true;
        room->damage(damage);
        return true;
    }
};

class Liuxing : public TriggerSkill
{
public:
    Liuxing()
        : TriggerSkill("liuxing")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Finish) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *hina, room->findPlayersBySkillName(objectName())) {
                if (hina->hasSkill(this) && hina != player && hina->isWounded() && !hina->isKongcheng())
                    d << SkillInvokeDetail(this, hina, hina, nullptr, false, player);
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *hina = invoke->invoker;
        ServerPlayer *player = invoke->preferredTarget;
        const Card *card = room->askForCard(hina, ".|black|.|hand", "@liuxing:" + player->objectName(), data, Card::MethodNone, player, false, objectName());
        if (card != nullptr) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, hina->objectName(), player->objectName());
            room->notifySkillInvoked(hina, objectName());
            room->sendLog("#InvokeSkill", hina, objectName());

            room->obtainCard(player, card, true);
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *hina = invoke->invoker;
        ServerPlayer *player = invoke->targets.first();
        QString choice = room->askForChoice(player, objectName(), "losehp+recover", QVariant::fromValue(hina));
        if (choice == "losehp")
            room->loseHp(player, 1);
        else
            room->recover(hina, RecoverStruct());
        return false;
    }
};

#pragma message WARN("todo_fs: check this skill whether it needs record, since changshi return a skill which has already invalided by Skill pingyi")
class Changshi : public TriggerSkill
{
public:
    Changshi()
        : TriggerSkill("changshi")
    {
        events << EventPhaseStart;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if ((player != nullptr) && player->getPhase() == Player::NotActive)
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark("changshiInvoked") > 0) {
                        room->setPlayerMark(p, "changshiInvoked", 0);
                        room->setPlayerSkillInvalidity(p, nullptr, false);
                    }
                }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if ((player != nullptr) && player->getPhase() == Player::RoundStart && player->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, nullptr);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        QStringList select;
        select << "skillInvalid";

        AllianceFeast *card = new AllianceFeast(Card::NoSuit, 0);
        card->setSkillName(objectName());
        card->deleteLater();

        if (!invoke->invoker->isCardLimited(card, Card::MethodUse) && card->isAvailable(invoke->invoker))
            select << "debuff";

        select << "cancel";
        QString choice = room->askForChoice(invoke->invoker, objectName(), select.join("+"), data);
        invoke->tag["changshi"] = choice;
        return choice != "cancel";
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        QString choice = invoke->tag.value("changshi").toString();
        AllianceFeast *card = new AllianceFeast(Card::NoSuit, 0);
        card->setSkillName(objectName());
        card->deleteLater();
        if (choice == "skillInvalid") {
            room->sendLog("#changshi01", invoke->invoker, "changshi");
            room->notifySkillInvoked(invoke->invoker, objectName());

            foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), p->objectName());
                room->setPlayerSkillInvalidity(p, nullptr, true);
                room->setPlayerMark(p, "changshiInvoked", 1);
            }
        } else if (choice == "debuff") {
            CardUseStruct carduse;
            carduse.card = card;
            carduse.from = invoke->invoker;

            room->useCard(carduse, true);
        }

        QStringList select;
        if (choice == "skillInvalid") {
            if (!invoke->invoker->isCardLimited(card, Card::MethodUse) && card->isAvailable(invoke->invoker))
                select << "debuff";
        } else {
            select << "skillInvalid";
        }
        select << "cancel";

        if (select.length() > 1) {
            QString choice = room->askForChoice(invoke->invoker, objectName(), select.join("+"), data);
            if (choice == "skillInvalid") {
                room->sendLog("#changshi01", invoke->invoker, "changshi");
                room->notifySkillInvoked(invoke->invoker, objectName());

                foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), p->objectName());
                    room->setPlayerSkillInvalidity(p, nullptr, true);
                    room->setPlayerMark(p, "changshiInvoked", 1);
                }
            } else if (choice == "debuff") {
                CardUseStruct carduse;
                carduse.card = card;
                carduse.from = invoke->invoker;

                room->useCard(carduse, true);
            }
        }
        return false;
    }
};

#pragma message WARN("todo_lwtmusou: develop a method to change move.is_last_handcard,which move to discardpile")
class Jinian : public TriggerSkill
{
public:
    Jinian()
        : TriggerSkill("jinian")
    {
        events << CardsMoveOneTime;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *sanae = qobject_cast<ServerPlayer *>(move.from);
        if (move.reason.m_extraData.value<ServerPlayer *>() != NULL)
            sanae = move.reason.m_extraData.value<ServerPlayer *>();
        if (sanae == nullptr || !sanae->hasSkill(this) || sanae->hasFlag("jinian_used"))
            return; // no need to update record.
        //record some temp ids, went to discardpile undirectly (through other places).
        if (move.to_place == Player::PlaceTable && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            //1 temp ids: record lasthand ids for using or responding
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE
                || (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE) {
                QVariantList record_ids = sanae->tag["jinianTemp"].toList();
                foreach (int id, move.card_ids) {
                    if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand && !record_ids.contains(id))
                        record_ids << id;
                }
                sanae->tag["jinianTemp"] = record_ids;
                return;
            }
        } else if (move.to_place == Player::PlaceJudge && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            //2 temp ids: record lasthand ids for retrial
            QVariantList record_ids = sanae->tag["jinianTemp"].toList();
            foreach (int id, move.card_ids) {
                if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand && !record_ids.contains(id))
                    record_ids << id;
            }
            sanae->tag["jinianTemp"] = record_ids;
            return;
        }

        //obtain jinian id while arriving discardpile.
        if (move.to_place == Player::DiscardPile) {
            QVariantList ids = sanae->tag["jinian"].toList();
            QVariantList record_ids = sanae->tag["jinianTemp"].toList();
            if (move.is_last_handcard) {
                //3.1 directly went to discardpile
                foreach (int id, move.card_ids) {
                    if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand && !ids.contains(id) && room->getCardPlace(id) == Player::DiscardPile)
                        ids << id;
                }
            } else {
                //3.2 check whether went to discard pile through palceTable or placeJudge
                foreach (int id, move.card_ids) {
                    if (!ids.contains(id) && record_ids.contains(id) && room->getCardPlace(id) == Player::DiscardPile)
                        ids << id;
                }
            }
            foreach (int id, move.card_ids)
                record_ids.removeOne(id);
            sanae->tag["jinian"] = ids;
            sanae->tag["jinianTemp"] = record_ids;
            return;
        }

        //4 delete temp ids
        QVariantList record_ids = sanae->tag["jinianTemp"].toList();
        foreach (int id, move.card_ids)
            record_ids.removeOne(id);
        sanae->tag["jinianTemp"] = record_ids;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *current = room->getCurrent();
        if (current == nullptr || !current->isInMainPhase())
            return QList<SkillInvokeDetail>();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
        if (move.reason.m_extraData.value<ServerPlayer *>() != NULL)
            player = move.reason.m_extraData.value<ServerPlayer *>();

        if (player != nullptr && player->isAlive() && player->hasSkill(this) && move.to_place == Player::DiscardPile && !player->hasFlag("jinian_used") && player->isAlive()) {
            QVariantList ids = player->tag["jinian"].toList();
            foreach (int id, move.card_ids) {
                if (ids.contains(id))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (invoke->invoker->askForSkillInvoke(objectName(), data))
            return true;
        else {
            QVariantList ids = invoke->invoker->tag["jinian"].toList();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            foreach (int id, move.card_ids) {
                if (ids.contains(id))
                    ids.removeOne(id);
            }
            invoke->invoker->tag["jinian"] = ids;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *player = invoke->invoker;
        player->setFlags("jinian_used");
        QList<int> get_ids;
        QVariantList ids = player->tag["jinian"].toList();
        player->tag.remove("jinian");
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        foreach (int id, move.card_ids) {
            if (ids.contains(id))
                get_ids << id;
        }

        CardsMoveStruct mo;
        mo.card_ids = get_ids;
        mo.to = player;
        mo.to_place = Player::PlaceHand;
        room->moveCardsAtomic(mo, true);

        return false;
    }
};

class JinianClear : public TriggerSkill
{
public:
    JinianClear()
        : TriggerSkill("#jinian_clear")
    {
        events << EventPhaseChanging;
    }

    void record(TriggerEvent, Room *room, QVariant &) const override
    {
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            p->tag.remove("jinian");
            p->tag.remove("jinianTemp");
            if (p->hasFlag("jinian_used"))
                p->setFlags("-jinian_used");
        }
    }
};

class Shouhu : public TriggerSkill
{
public:
    Shouhu()
        : TriggerSkill("shouhu")
    {
        events << EventPhaseStart;
    }
    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Play && player->hasSkill(this)) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->isWounded() && p->getHp() < player->getHp())
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (p->isWounded() && p->getHp() < invoke->invoker->getHp())
                targets << p;
        }
        ServerPlayer *t = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@shouhu", true, true);
        if (t != nullptr) {
            invoke->targets << t;
            return true;
        }

        return false;
    }
    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->recover(invoke->targets.first(), RecoverStruct());
        return false;
    }
};

class Shaojie : public TriggerSkill
{
public:
    Shaojie()
        : TriggerSkill("shaojie")
    {
        events << EventPhaseStart << DamageInflicted;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() != Player::Play)
                return QList<SkillInvokeDetail>();
            if (player->getShownHandcards().length() >= player->getHandcardNum())
                return QList<SkillInvokeDetail>();

            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (player->inMyAttackRange(p))
                    d << SkillInvokeDetail(this, p, p, nullptr, false, player);
            }
            return d;
        } else {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->hasSkill(this) && (damage.from != nullptr) && damage.from != damage.to && (damage.card != nullptr)) {
                if (damage.card->hasFlag("showncards"))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
                foreach (int id, damage.from->getShownHandcards()) {
                    if (damage.card->getColor() == Sanguosha->getCard(id)->getColor())
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            return invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget));
        } else {
            DamageStruct damage = data.value<DamageStruct>();
            QString prompt = "invoke:" + damage.from->objectName() + ":" + damage.card->objectName() + ":" + QString::number(damage.damage);
            return invoke->invoker->askForSkillInvoke(this, data, prompt);
        }
        return false;
    }
    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
            int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "h", objectName());
            invoke->targets.first()->addToShownHandCards(QList<int>() << id);
        } else {
            DamageStruct damage = data.value<DamageStruct>();
            room->sendLog("#shaojie", invoke->invoker, objectName(), QList<ServerPlayer *>(), QString::number(damage.damage));
            room->notifySkillInvoked(invoke->invoker, objectName());
            return true;
        }
        return false;
    }
};

FengrangCard::FengrangCard()
{
    target_fixed = true;
    handling_method = Card::MethodUse;
    m_skillName = "fengrang";
}

const Card *FengrangCard::validate(CardUseStruct &) const
{
    AmazingGrace *card = new AmazingGrace(Card::NoSuit, 0);
    card->setSkillName("fengrang");
    return card;
}

class Fengrang : public ZeroCardViewAsSkill
{
public:
    Fengrang()
        : ZeroCardViewAsSkill("fengrang")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        AmazingGrace *card = new AmazingGrace(Card::NoSuit, 0);
        card->deleteLater();
        return !player->hasUsed("FengrangCard") && card->isAvailable(player);
    }

    const Card *viewAs() const override
    {
        return new FengrangCard;
    }
};

class Shouhuo : public TriggerSkill
{
public:
    Shouhuo()
        : TriggerSkill("shouhuo")
    {
        events << TrickCardCanceling;
        frequency = Compulsory;
        show_type = "static";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if ((effect.to != nullptr) && effect.to->hasSkill(objectName()) && (effect.card != nullptr) && effect.card->isKindOf("AmazingGrace"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, nullptr, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        room->notifySkillInvoked(effect.to, objectName());
        return true;
    }
};

JiliaoCard::JiliaoCard()
{
}

bool JiliaoCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void JiliaoCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    const QList<ServerPlayer *> &targets = card_use.to;

    ServerPlayer *target = targets.first();
    QList<int> equips;
    foreach (const Card *c, target->getEquips())
        equips << c->getId();
    CardsMoveStruct move;
    move.card_ids = equips;
    move.to_place = Player::PlaceHand;
    move.to = target;
    room->moveCardsAtomic(move, true);
    if (target->getHandcardNum() <= target->getMaxCards() || !source->canDiscard(target, "hs"))
        return;
    if (room->askForSkillInvoke(source, "jiliao", QVariant::fromValue(target), "throwcard:" + target->objectName())) {
        if (target == source)
            room->askForDiscard(source, "jiliao", 1, 1, false, false);
        else {
            int to_throw = room->askForCardChosen(source, target, "hs", "jiliao", false, Card::MethodDiscard);
            room->throwCard(to_throw, target, source);
        }
    }
}

class Jiliao : public ZeroCardViewAsSkill
{
public:
    Jiliao()
        : ZeroCardViewAsSkill("jiliao")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("JiliaoCard");
    }

    const Card *viewAs() const override
    {
        return new JiliaoCard;
    }
};

class Zhongyan : public TriggerSkill
{
public:
    Zhongyan()
        : TriggerSkill("zhongyan")
    {
        events << DamageInflicted;
        frequency = Limited;
        limit_mark = "@zhongyan";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from == nullptr || damage.from->isDead() || damage.from == damage.to || !damage.to->hasSkill(this) || damage.to->getMark("@zhongyan") == 0)
            return QList<SkillInvokeDetail>();
        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->setTag("zhongyan_damage", data);
        int n = qMax(1, damage.from->getLostHp());
        QString prompt = "target:" + damage.from->objectName() + ":" + QString::number(n);
        return room->askForSkillInvoke(invoke->invoker, objectName(), data, prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();

        room->removePlayerMark(invoke->invoker, "@zhongyan");
        room->doLightbox("$zhongyanAnimate", 4000);
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), damage.from->objectName());

        int x = damage.from->getLostHp();
        room->loseHp(damage.from, qMax(1, x));
        return true;
    }
};

// put this on top for BujuCard references Buju
class BujuVS : public ZeroCardViewAsSkill
{
public:
    BujuVS()
        : ZeroCardViewAsSkill("buju")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("BujuCard");
    }

    const Card *viewAs() const override
    {
        return new BujuCard;
    }
};

class Buju : public TriggerSkill
{
public:
    static void doEffect(Room *room, ServerPlayer *source)
    {
        int x = qMin(room->alivePlayerCount(), 4);

        source->drawCards(x);
        const Card *cards = room->askForExchange(source, "buju", x, x, true, "buju_exchange:" + QString::number(x));
        DELETE_OVER_SCOPE(const Card, cards)
        CardsMoveStruct move;
        move.card_ids = cards->getSubcards();
        move.from = source;
        move.to_place = Player::DrawPile;
        room->moveCardsAtomic(move, false);
        room->askForGuanxing(source, room->getNCards(x), Room::GuanxingUpOnly, "buju");
    }

    Buju()
        : TriggerSkill("buju")
    {
        events = {Damaged};
        view_as_skill = new BujuVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to != nullptr && damage.to->isAlive() && damage.to->hasSkill(this)) {
            return {SkillInvokeDetail(this, damage.to, damage.to)};
        }

        return {};
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        doEffect(room, invoke->invoker);
        return false;
    }
};

BujuCard::BujuCard()
{
    target_fixed = true;
}

void BujuCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    Buju::doEffect(room, source);
}

TH10Package::TH10Package()
    : Package("th10")
{
    General *kanako = new General(this, "kanako$", "fsl", 4);
    kanako->addSkill(new Shende);
    kanako->addSkill(new Qiankun);
    kanako->addSkill(new Qiankun2);
    kanako->addSkill(new Gongfeng);
    related_skills.insertMulti("qiankun", "#qiankun2");

    General *suwako = new General(this, "suwako", "fsl", 3);
    suwako->addSkill(new Bushu);
    suwako->addSkill("qiankun");
    suwako->addSkill(new Chuancheng);

    General *sanae = new General(this, "sanae", "fsl", 3);
    sanae->addSkill(new DfgzmJiyi);
    sanae->addSkill(new Qiji);

    General *aya = new General(this, "aya", "fsl", 3);
    aya->addSkill(new Fengshen);
    aya->addSkill(new Fengsu);
    aya->addSkill(new FengsuDistance);
    related_skills.insertMulti("fengsu", "#fengsu-distance");

    General *nitori = new General(this, "nitori", "fsl", 3);
    nitori->addSkill(new Xinshang);
    nitori->addSkill(new XinshangTargetMod);
    nitori->addSkill(new Micai);
    related_skills.insertMulti("xinshang", "#xinshang_effect");

    General *hina = new General(this, "hina", "fsl", 3);
    hina->addSkill(new Jie);
    hina->addSkill(new Liuxing);

    General *sanae_sp = new General(this, "sanae_sp", "fsl", 3);
    sanae_sp->addSkill(new Changshi);
    sanae_sp->addSkill(new Jinian);
    sanae_sp->addSkill(new JinianClear);
    related_skills.insertMulti("jinian", "#jinian_clear");

    General *momizi = new General(this, "momizi", "fsl", 4);
    momizi->addSkill(new Shouhu);
    momizi->addSkill(new Shaojie);

    General *minoriko = new General(this, "minoriko", "fsl", 4);
    minoriko->addSkill(new Fengrang);
    minoriko->addSkill(new Shouhuo);

    General *shizuha = new General(this, "shizuha", "fsl", 4);
    shizuha->addSkill(new Jiliao);
    shizuha->addSkill(new Zhongyan);

    General *momizi_sp = new General(this, "momizi_sp", "fsl", 4);
    momizi_sp->addSkill(new Buju);

    addMetaObject<GongfengCard>();
    addMetaObject<FengshenCard>();
    addMetaObject<ShowFengsu>(); // for hegemony
    addMetaObject<XinshangCard>();
    addMetaObject<FengrangCard>();
    addMetaObject<JiliaoCard>();
    addMetaObject<BujuCard>();

    skills << new GongfengVS;
}

ADD_PACKAGE(TH10)
