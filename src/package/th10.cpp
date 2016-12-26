#include "th10.h"

#include "general.h"
#include "settings.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"
#include <QCommandLinkButton>
#include "th13.h"

class ShendeVS : public ViewAsSkill
{
public:
    ShendeVS() : ViewAsSkill("shende")
    {
        expand_pile = "shende";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getPile("shende").length() < 2)
            return false;
        Peach *peach = new Peach(Card::NoSuit, 0);
        peach->deleteLater();
        return peach->isAvailable(player);
    }
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getMark("Global_PreventPeach") > 0)
            return false;
        if (player->getPile("shende").length() >= 2)
            return matchAvaliablePattern("peach", pattern);
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() >= 2)
            return false;
        return Self->getPile("shende").contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;
        Peach *peach = new Peach(Card::SuitToBeDecided, -1);
        peach->addSubcards(cards);
        peach->setSkillName("shende");
        return peach;
    }
};

class Shende : public TriggerSkill
{
public:
    Shende() : TriggerSkill("shende")
    {
        events << CardUsed << CardResponded;
        view_as_skill = new ShendeVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card->isKindOf("Slash") && resp.m_from->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, resp.m_from, resp.m_from);
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.from->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1);
        if (!invoke->invoker->isKongcheng()) {
            const Card *cards = room->askForExchange(invoke->invoker, objectName(), 1, 1, false, "shende-exchange");
            DELETE_OVER_SCOPE(const Card, cards)
                    invoke->invoker->addToPile("shende", cards->getSubcards().first());
        }
        return false;
    }
};

class Qiankun : public MaxCardsSkill
{
public:
    Qiankun() : MaxCardsSkill("qiankun")
    {
    }

    virtual int getExtra(const Player *target) const
    {

        if (target->hasSkill("qiankun"))
            return 2;
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

void GongfengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *kanako = targets.first();
    if (kanako->hasLordSkill("gongfeng")) {
        room->setPlayerFlag(kanako, "gongfengInvoked");

        room->notifySkillInvoked(kanako, "gongfeng");
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), kanako->objectName(), "gongfeng", QString());
        room->obtainCard(kanako, this, reason);
        QList<ServerPlayer *> kanakos;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach (ServerPlayer *p, players) {
            if (p->hasLordSkill("gongfeng") && !p->hasFlag("gongfengInvoked"))
                kanakos << p;
        }
        if (kanakos.isEmpty())
            room->setPlayerFlag(source, "Forbidgongfeng");
    }
}

bool GongfengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasLordSkill("gongfeng")
            && to_select != Self && !to_select->hasFlag("gongfengInvoked");
}

class GongfengVS : public OneCardViewAsSkill
{
public:
    GongfengVS() :OneCardViewAsSkill("gongfeng_attach")
    {
        attached_lord_skill = true;
        filter_pattern = "Slash";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return  !player->hasFlag("Forbidgongfeng") && shouldBeVisible(player);
    }

    virtual bool shouldBeVisible(const Player *Self) const
    {
        return Self && Self->getKingdom() == "fsl";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        GongfengCard *card = new GongfengCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class Gongfeng : public TriggerSkill
{
public:
    Gongfeng() : TriggerSkill("gongfeng$")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging << Revive;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent != EventPhaseChanging) { //the case operating attach skill
            static QString attachName = "gongfeng_attach";
            QList<ServerPlayer *> lords;
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasLordSkill(this, true))
                    lords << p;
            }

            if (lords.length() > 1) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->hasSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            }
            else if (lords.length() == 1) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasLordSkill(this, true) && p->hasSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                    else if (!p->hasLordSkill(this, true) && !p->hasSkill(attachName, true))
                        room->attachSkillToPlayer(p, attachName);
                }
            }
            else {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from == Player::Play) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("gongfengInvoked"))
                        room->setPlayerFlag(p, "-gongfengInvoked");
                    if (p->hasFlag("Forbidgongfeng"))
                        room->setPlayerFlag(p, "-Forbidgongfeng");
                }
            }
        }
    }

};




class Bushu : public TriggerSkill
{
public:
    Bushu() : TriggerSkill("bushu")
    {
        events << Pindian << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == Pindian) {
            PindianStruct * pindian = data.value<PindianStruct *>();
            if (pindian->reason != "bushu")
                return QList<SkillInvokeDetail>();
            ServerPlayer *target = pindian->from->tag["suwako_bushu"].value<ServerPlayer *>();
            if (pindian->success && target && target->isAlive())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, pindian->from, pindian->from, NULL, true, target);
            else if (!pindian->success && pindian->from->isAlive())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, pindian->from, pindian->from, NULL, true);

        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.from || damage.to->isDead() || !damage.from->isAlive())
                return QList<SkillInvokeDetail>();

            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *suwako, room->findPlayersBySkillName(objectName())) {
                if (damage.from != suwako && !damage.from->isKongcheng() &&
                        (suwako->inMyAttackRange(damage.to) || suwako == damage.to)
                        && !suwako->isKongcheng())
                    d << SkillInvokeDetail(this, suwako, suwako);
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            invoke->invoker->tag["bushu_damage"] = data;
            QString prompt = "damage:" + damage.from->objectName() + ":" + damage.to->objectName();
            return invoke->invoker->askForSkillInvoke(objectName(), prompt);
        }
        invoke->invoker->tag.remove("suwako_bushu");
        return true;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *suwako = invoke->invoker;
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, suwako->objectName(), damage.from->objectName());
            invoke->invoker->tag["suwako_bushu"] = QVariant::fromValue(damage.to);
            suwako->pindian(damage.from, objectName(), NULL);
        } else {
            PindianStruct * pindian = data.value<PindianStruct *>();
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
    Chuancheng() : TriggerSkill("chuancheng")
    {
        events << Death;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who->hasSkill(objectName()))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, death.who, death.who);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(), "@chuancheng", true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
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



class DfgzmJiyi : public TriggerSkill
{
public:
    DfgzmJiyi() : TriggerSkill ("dfgzmjiyi")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Draw && player->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && change.player->hasFlag(objectName())) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, NULL, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (e == EventPhaseStart) {
            invoke->invoker->setFlags(objectName());
            return true;
        } else if (e == EventPhaseChanging) {
            //invoke->invoker->setFlags("-zhunbei");
            room->touhouLogmessage("#TouhouBuff", invoke->invoker, objectName());
            room->notifySkillInvoked(invoke->invoker, "zhunbei");
            invoke->invoker->drawCards(3);
        }
        return false;
    }
};



QijiDialog *QijiDialog::getInstance(const QString &object, bool left, bool right)
{
    static QijiDialog *instance;
    if (instance == NULL || instance->objectName() != object) {
        instance = new QijiDialog(object, left, right);
    }
    return instance;
}

QijiDialog::QijiDialog(const QString &object, bool left, bool right) : object_name(object)
{
    setObjectName(object);
    setWindowTitle(Sanguosha->translate(object)); //need translate title?
    group = new QButtonGroup(this);

    QHBoxLayout *layout = new QHBoxLayout;
    if (left) layout->addWidget(createLeft());
    if (right) layout->addWidget(createRight());
    setLayout(layout);

    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectCard(QAbstractButton *)));
}

void QijiDialog::popup()
{
    Card::HandlingMethod method;
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        method = Card::MethodResponse;
    else
        method = Card::MethodUse;

    QStringList checkedPatterns;
    QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
    bool play = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY);

    //collect avaliable patterns for specific skill
    QStringList validPatterns;
    if (object_name == "huaxiang") {
        validPatterns << "slash" << "analeptic";
        if (Self->getMaxHp() <= 3)
            validPatterns << "jink";
        if (Self->getMaxHp() <= 2)
            validPatterns << "peach";
    } else if (object_name == "xihua") {
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        foreach(const Card *card, cards) {
            if ((card->isNDTrick() || card->isKindOf("BasicCard"))
                    && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
                QString name = card->objectName();
                if (card->isKindOf("Slash"))
                    name = "slash";
                QString markName = "xihua_record_" + name;
                if (!validPatterns.contains(name) && Self->getMark(markName) == 0)
                    validPatterns << card->objectName();
            }
        }
    }
    //if (object_name == "qiji") {
    else {
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        foreach(const Card *card, cards) {
            if ((card->isNDTrick() || card->isKindOf("BasicCard"))
                    && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
                QString name = card->objectName();
                if (card->isKindOf("Slash"))
                    name = "slash";
                if (!validPatterns.contains(name))
                    validPatterns << card->objectName();
            }
        }
    }
    //then match it and check "CardLimit"
    foreach(QString str, validPatterns) {
        const Skill *skill = Sanguosha->getSkill(object_name);
        if (play || skill->matchAvaliablePattern(str, pattern)) {
            Card *card = Sanguosha->cloneCard(str);
            DELETE_OVER_SCOPE(Card, card)
                    if (!Self->isCardLimited(card, method))
                    checkedPatterns << str;
        }
    }
    //while responsing, if only one pattern were checked, emit click()
    if (object_name != "chuangshi" && !play && checkedPatterns.length() <= 1 && !checkedPatterns.contains("slash")) {
        emit onButtonClick();
        return;
    }

    foreach (QAbstractButton *button, group->buttons()) {
        const Card *card = map[button->objectName()];
        const Player *user = NULL;
        if (object_name == "chuangshi") { //check the card is Available for chuangshi target.
            foreach(const Player *p, Self->getAliveSiblings())
            {
                if (p->getMark("chuangshi_user") > 0) {
                    user = p;
                    break;
                }
            }
        } else
            user = Self;
        if (user == NULL)
            user = Self;

        bool avaliable = card->isAvailable(user);
        if (object_name == "qiji" && user->getMark("xiubu"))
            avaliable = true;

        bool checked = (checkedPatterns.contains(card->objectName()) || (card->isKindOf("Slash") && checkedPatterns.contains("slash")));
        bool enabled = !user->isCardLimited(card, method, true) && avaliable && (checked || object_name == "chuangshi");
        button->setEnabled(enabled);
    }

    Self->tag.remove(object_name);
    exec();
}

void QijiDialog::selectCard(QAbstractButton *button)
{
    const Card *card = map.value(button->objectName());
    Self->tag[object_name] = QVariant::fromValue(card);

    emit onButtonClick();
    accept();
}

QGroupBox *QijiDialog::createLeft()
{
    QGroupBox *box = new QGroupBox;
    box->setTitle(Sanguosha->translate("basic"));

    QVBoxLayout *layout = new QVBoxLayout;

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    QStringList ban_list; //no need to ban
    if (object_name == "chuangshi")
        ban_list << "Analeptic";


    foreach (const Card *card, cards) {
        if (card->getTypeId() == Card::TypeBasic && !map.contains(card->objectName())
                && !ban_list.contains(card->getClassName()) && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
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


    QStringList ban_list; //no need to ban
    if (object_name == "chuangshi")
        ban_list << "GodSalvation" << "ArcheryAttack" << "SavageAssault";
    //    ban_list << "Drowning" << "BurningCamps" << "LureTiger";



    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    foreach (const Card *card, cards) {
        if (card->isNDTrick() && !map.contains(card->objectName()) && !ban_list.contains(card->getClassName())
                && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
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

QijiCard::QijiCard()
{
    will_throw = false;
}

bool QijiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            const Card *oc = Sanguosha->getCard(subcards.first());
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), oc->getSuit(), oc->getNumber());
            DELETE_OVER_SCOPE(Card, card)
                    card->addSubcard(oc);
            return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *card = Self->tag.value("qiji").value<const Card *>();
    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(card->objectName(), oc->getSuit(), oc->getNumber());
    DELETE_OVER_SCOPE(Card, new_card)
            new_card->addSubcard(oc);
    new_card->setSkillName("qiji");
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool QijiCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            const Card *oc = Sanguosha->getCard(subcards.first());
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), oc->getSuit(), oc->getNumber());
            DELETE_OVER_SCOPE(Card, card)
                    card->addSubcard(oc);
            return card && card->targetFixed();
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("qiji").value<const Card *>();
    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(card->objectName(), oc->getSuit(), oc->getNumber());
    DELETE_OVER_SCOPE(Card, new_card)
            new_card->addSubcard(oc);
    new_card->setSkillName("qiji");
    return new_card && new_card->targetFixed();
}

bool QijiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            const Card *oc = Sanguosha->getCard(subcards.first());
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), oc->getSuit(), oc->getNumber());
            DELETE_OVER_SCOPE(Card, card)
                    card->addSubcard(oc);
            return card && card->targetsFeasible(targets, Self);
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("qiji").value<const Card *>();
    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *new_card = Sanguosha->cloneCard(card->objectName(), oc->getSuit(), oc->getNumber());
    DELETE_OVER_SCOPE(Card, new_card)
            new_card->addSubcard(oc);
    new_card->setSkillName("qiji");
    if (card->isKindOf("IronChain") && targets.length() == 0)
        return false;
    return new_card && new_card->targetsFeasible(targets, Self);
}

const Card *QijiCard::validate(CardUseStruct &use) const
{
    QString to_use = user_string;

    const Card *card = Sanguosha->getCard(subcards.first());
    Card *use_card = Sanguosha->cloneCard(to_use, card->getSuit(), card->getNumber());
    use_card->setSkillName("qiji");
    use_card->addSubcard(subcards.first());
    use_card->deleteLater();
    use.from->getRoom()->setPlayerMark(use.from, "qiji", 1);
    return use_card;
}

const Card *QijiCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();

    QString to_use;
    // use_list is useless?
    if (user_string == "peach+analeptic") {
        QStringList use_list;
        Card *peach = Sanguosha->cloneCard("peach");
        DELETE_OVER_SCOPE(Card, peach)
                if (!user->isCardLimited(peach, Card::MethodResponse, true))
                use_list << "peach";
        Card *ana = Sanguosha->cloneCard("analeptic");
        DELETE_OVER_SCOPE(Card, ana)
                if (!Config.BanPackages.contains("maneuvering") && !user->isCardLimited(ana, Card::MethodResponse, true))
                use_list << "analeptic";
        to_use = room->askForChoice(user, "qiji_skill_saveself", use_list.join("+"));
    } else
        to_use = user_string;

    const Card *card = Sanguosha->getCard(subcards.first());
    Card *use_card = Sanguosha->cloneCard(to_use, card->getSuit(), card->getNumber());
    use_card->setSkillName("qiji");
    use_card->addSubcard(subcards.first());
    use_card->deleteLater();
    user->getRoom()->setPlayerMark(user, "qiji", 1);
    return use_card;
}


class QijiVS : public OneCardViewAsSkill
{
public:
    QijiVS() : OneCardViewAsSkill("qiji")
    {
        filter_pattern = ".|.|.|hand";
    }

    static QStringList responsePatterns() {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        Card::HandlingMethod method;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            method = Card::MethodResponse;
        else
            method = Card::MethodUse;

        QStringList validPatterns;
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        foreach(const Card *card, cards) {
            if ((card->isNDTrick() || card->isKindOf("BasicCard"))
                    && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
                QString pattern = card->objectName();
                if (card->isKindOf("Slash"))
                    pattern = "slash";
                if (!validPatterns.contains(pattern))
                    validPatterns << card->objectName();
            }
        }

        QStringList checkedPatterns;
        foreach(QString str, validPatterns) {
            const Skill *skill = Sanguosha->getSkill("qiji");
            if (skill->matchAvaliablePattern(str, pattern))
                checkedPatterns << str;
        }
        return checkedPatterns;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &) const
    {
        if (player->getHandcardNum() != 1) return false;
        //if (player->getHandcardNum() != 1 || pattern.startsWith(".") || pattern.startsWith("@")) return false;
        if (player->getMark("qiji") > 0) return false;
        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.contains("peach") && checkedPatterns.length() == 1 && player->getMark("Global_PreventPeach") > 0) return false;
        return !checkedPatterns.isEmpty();
        /*for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false;
        }*/

        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark("qiji") > 0) return false;
        return player->getHandcardNum() == 1;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
                || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            QStringList checkedPatterns = responsePatterns();

            if (checkedPatterns.length() > 1 || checkedPatterns.contains("slash")) {
                const Card *c = Self->tag.value("qiji").value<const Card *>();
                if (c)
                    pattern = c->objectName();
                else
                    return NULL;
            } else
                pattern = checkedPatterns.first();
            QijiCard *card = new QijiCard;
            card->setUserString(pattern);
            card->addSubcard(originalCard);
            return card;

        }

        const Card *c = Self->tag.value("qiji").value<const Card *>();
        if (c) {
            QijiCard *card = new QijiCard;
            card->setUserString(c->objectName());
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }



    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        if (player->getMark("qiji") > 0) return false;
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
    Qiji() : TriggerSkill("qiji")
    {
        events << EventPhaseChanging << PreCardUsed;
        view_as_skill = new QijiVS;
    }

    virtual QDialog *getDialog() const
    {
        return QijiDialog::getInstance("qiji");
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseChanging) {
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getMark("qiji") > 0)
                    room->setPlayerMark(p, "qiji", 0);
            }
        }
        //record for ai, since AI prefer use a specific card,  but not the SkillCard QijiCard.
        if (e == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == objectName())
                room->setPlayerMark(use.from, "qiji", 1);
        }

    }
};



FengshenCard::FengshenCard()
{
    will_throw = true;
}

bool FengshenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (to_select == Self)
        return false;
    else if (targets.isEmpty())
        return Self->inMyAttackRange(to_select);
    else if (targets.length() == 1)
        return Self->distanceTo(targets.first()) <= 1 && Self->distanceTo(to_select) <= 1;
    else
        return Self->distanceTo(to_select) <= 1;
}

void FengshenCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    const Card *card = room->askForCard(effect.to, "Slash", "@fengshen-discard:" + effect.from->objectName());
    if (card == NULL)
        room->damage(DamageStruct("fenshen", effect.from, effect.to));
}

class Fengshen : public OneCardViewAsSkill
{
public:
    Fengshen() : OneCardViewAsSkill("fengshen")
    {
        filter_pattern = ".|red|.|hand!";
    }


    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("FengshenCard") && !player->isKongcheng();
    }



    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            FengshenCard *card = new FengshenCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }
};

class Fengsu : public DistanceSkill
{
public:
    Fengsu() : DistanceSkill("fengsu")
    {
    }

    virtual int getCorrect(const Player *from, const Player *to) const
    {
        int correct = 0;
        if (from->hasSkill("fengsu"))
            correct = correct - (from->getLostHp());

        if (to->hasSkill("fengsu"))
            correct = correct + to->getLostHp();
        return correct;
    }
};

class FengsuEffect : public TriggerSkill
{
public:
    FengsuEffect() : TriggerSkill("#fengsu-effect")
    {
        events << HpChanged;
    }


    void record(TriggerEvent, Room *room, QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill("fengsu"))
            room->notifySkillInvoked(player, "fengsu");
    }
};



XinshangCard::XinshangCard()
{
}

void XinshangCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    effect.to->drawCards(1);

    const Card *card = room->askForCard(effect.to, ".S", "@xinshang-spadecard:" + effect.from->objectName(), QVariant::fromValue(effect.from), Card::MethodNone, NULL, false, "xinshang", false);
    if (card != NULL) {
        room->obtainCard(effect.from, card);
        room->setPlayerFlag(effect.from, "xinshang_effect");
    } else {
        if (effect.from->canDiscard(effect.to, "hes")) {
            room->throwCard(room->askForCardChosen(effect.from, effect.to, "hes", "xinshang", false, Card::MethodDiscard), effect.to, effect.from);
            if (effect.from->canDiscard(effect.to, "hes"))
                room->throwCard(room->askForCardChosen(effect.from, effect.to, "hes", "xinshang", false, Card::MethodDiscard), effect.to, effect.from);
        }
    }
}

class Xinshang : public ZeroCardViewAsSkill
{
public:
    Xinshang() : ZeroCardViewAsSkill("xinshang")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("XinshangCard");
    }

    virtual const Card *viewAs() const
    {
        return new XinshangCard;
    }
};

class XinshangTargetMod : public TargetModSkill
{
public:
    XinshangTargetMod() : TargetModSkill("#xinshang_effect")
    {
        pattern = "BasicCard,TrickCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->hasFlag("xinshang_effect"))
            return 1000;
        else
            return 0;
    }

    virtual int getResidueNum(const Player *from, const Card *) const
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
    Micai() : TriggerSkill("micai")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->hasSkill(this) && damage.damage > damage.to->getHandcardNum())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *player = invoke->invoker;
        room->touhouLogmessage("#micai01", player, "micai", QList<ServerPlayer *>(), QString::number(1));
        damage.damage = damage.damage -1;
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
    Jie() : TriggerSkill("jie")
    {
        events << DamageInflicted;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;
        foreach(ServerPlayer *hina, room->findPlayersBySkillName(objectName())) {
            if (hina && damage.to != hina)
                d << SkillInvokeDetail(this, hina, hina);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt;
        QString nature;
        switch (damage.nature) {
        case DamageStruct::Normal: nature = "normal_nature"; break;
        case DamageStruct::Fire: nature = "fire_nature"; break;
        case DamageStruct::Thunder: nature = "thunder_nature"; break;
        case DamageStruct::Ice: nature = "ice_nature"; break;
        }
        if (damage.from)
            prompt = "transfer1:" + damage.to->objectName() + ":" + damage.from->objectName() + ":" + QString::number(damage.damage) + ":" + nature;
        else
            prompt = "transfer2:" + damage.to->objectName() + ":" + QString::number(damage.damage) + ":" + nature;
        invoke->invoker->tag["jie_damage"] = data;
        return room->askForSkillInvoke(invoke->invoker, objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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
    Liuxing() : TriggerSkill("liuxing")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Finish) {
            QList<SkillInvokeDetail> d;
            foreach(ServerPlayer *hina, room->findPlayersBySkillName(objectName())) {
                if (hina->hasSkill(this) && hina != player && hina->isWounded() && !hina->isKongcheng())
                    d << SkillInvokeDetail(this, hina, hina, NULL, false, player);
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *hina = invoke->invoker;
        ServerPlayer *player = invoke->preferredTarget;
        const Card *card = room->askForCard(hina, ".|black|.|hand", "@liuxing:" + player->objectName(), data, Card::MethodNone, player, false, objectName());
        if (card) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, hina->objectName(), player->objectName());
            room->notifySkillInvoked(hina, objectName());
            room->touhouLogmessage("#InvokeSkill", hina, objectName());

            room->obtainCard(player, card, true);
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *hina = invoke->invoker;
        ServerPlayer *player = invoke->targets.first();
        //need add player information to help ai  make decision
        player->tag["liuxing_source"] = QVariant::fromValue(hina);
        QString choice = room->askForChoice(player, objectName(), "losehp+recover", QVariant::fromValue(hina));
        player->tag.remove("liuxing_source");
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
    Changshi() : TriggerSkill("changshi")
    {
        events << EventPhaseStart << EventPhaseChanging;
        frequency = Eternal;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("changshiInvoked")) {
                        room->setPlayerSkillInvalidity(p, NULL, false);
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart){
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::RoundStart && player->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->touhouLogmessage("#changshi01", invoke->invoker, "changshi");
        room->notifySkillInvoked(invoke->invoker, objectName());

        foreach(ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), p->objectName());
            room->setPlayerSkillInvalidity(p, NULL, true);
            p->setFlags("changshiInvoked");
        }

        //deal with mark and private pile
        QStringList marks;
        marks << "@clock" << "@kinki" << "@qiannian" << "@shi" << "@ye" << "@yu" << "@zhengti"
              << "@xinyang" << "@ice";
        QStringList disablePiles;
        disablePiles << "wooden_ox";
        foreach(ServerPlayer *p, room->getAlivePlayers()) {
            //throw cards in special place
            QList<int>  idlist;
            foreach(QString pile, p->getPileNames()) {
                if (disablePiles.contains(pile)) continue;
                foreach(int card, p->getPile(pile))
                    idlist << card;
            }
            if (idlist.length() > 0) {
                CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, p->objectName(), NULL, "changshi", "");
                CardsMoveStruct move(idlist, p, Player::DiscardPile,
                                     reason);
                room->moveCardsAtomic(move, true);
            }
            foreach(QString m, marks) {
                if (p->getMark(m) > 0)
                    p->loseAllMarks(m);
            }

        }
        return false;
    }
};

#pragma message WARN("todo_lwtmusou: develop a method to change move.is_last_handcard,which move to discardpile")
class Jinian : public TriggerSkill
{
public:
    Jinian() : TriggerSkill("jinian")
    {
        events << CardsMoveOneTime;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *sanae = qobject_cast<ServerPlayer *>(move.from);
        if (move.reason.m_extraData.value<ServerPlayer *>() != NULL)
            sanae = move.reason.m_extraData.value<ServerPlayer *>();
        if (sanae == NULL || !sanae->hasSkill(this) || sanae->hasFlag("jinian_used"))
            return;// no need to update record.
        //record some temp ids, went to discardpile undirectly (through other places).
        if (move.to_place == Player::PlaceTable && move.from_places.contains(Player::PlaceHand)  && move.is_last_handcard) {
            //1 temp ids: record lasthand ids for using or responsing
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE
                    || (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE) {
                QVariantList record_ids = sanae->tag["jinianTemp"].toList();
                foreach(int id, move.card_ids) {
                    if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand
                            && !record_ids.contains(id))
                        record_ids << id;
                }
                sanae->tag["jinianTemp"] = record_ids;
                return;
            }
        } else if (move.to_place == Player::PlaceJudge && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard) {
            //2 temp ids: record lasthand ids for retrial
            QVariantList record_ids = sanae->tag["jinianTemp"].toList();
            foreach(int id, move.card_ids) {
                if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand
                        && !record_ids.contains(id))
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
                foreach(int id, move.card_ids) {
                    if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand
                            && !ids.contains(id) && room->getCardPlace(id) == Player::DiscardPile)
                        ids << id;
                }
            } else {
                //3.2 check whether went to discard pile through palceTable or placeJudge
                foreach(int id, move.card_ids) {
                    if (!ids.contains(id) && record_ids.contains(id) && room->getCardPlace(id) == Player::DiscardPile)
                        ids << id;
                }
            }
            foreach(int id, move.card_ids)
                record_ids.removeOne(id);
            sanae->tag["jinian"] = ids;
            sanae->tag["jinianTemp"] = record_ids;
            return;
        }

        //4 delete temp ids
        QVariantList record_ids = sanae->tag["jinianTemp"].toList();
        foreach(int id, move.card_ids)
            record_ids.removeOne(id);
        sanae->tag["jinianTemp"] = record_ids;
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
        if (move.reason.m_extraData.value<ServerPlayer *>() != NULL)
            player = move.reason.m_extraData.value<ServerPlayer *>();

        if (player != NULL && player->isAlive() && player->hasSkill(this) && move.to_place == Player::DiscardPile
                && !player->hasFlag("jinian_used") && player->isAlive()) {
            QVariantList ids = player->tag["jinian"].toList();
            foreach(int id, move.card_ids) {
                if (ids.contains(id))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (invoke->invoker->askForSkillInvoke(objectName(), data))
            return true;
        else {
            QVariantList ids = invoke->invoker->tag["jinian"].toList();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            foreach(int id, move.card_ids) {
                if (ids.contains(id))
                    ids.removeOne(id);
            }
            invoke->invoker->tag["jinian"] = ids;
        }
        return false;
    }


    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = invoke->invoker;
        player->setFlags("jinian_used");
        QList<int> get_ids;
        QVariantList ids = player->tag["jinian"].toList();
        player->tag.remove("jinian");
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        foreach(int id, move.card_ids) {
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
    JinianClear() : TriggerSkill("#jinian_clear")
    {
        events << EventPhaseChanging;
    }

    void record(TriggerEvent, Room *room, QVariant &) const
    {
        foreach(ServerPlayer *p, room->getAllPlayers()) {
            p->tag.remove("jinian");
            p->tag.remove("jinianTemp");
            if (p->hasFlag("jinian_used"))
                p->setFlags("-jinian_used");
        }
    }
};




TianyanCard::TianyanCard()
{
    target_fixed = true;
}

void TianyanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    int x = qMin(room->alivePlayerCount(), 4);

    source->drawCards(x);
    const Card *cards = room->askForExchange(source, "tianyan", x, x, true, "tianyan_exchange:" + QString::number(x));
    DELETE_OVER_SCOPE(const Card, cards)
            CardsMoveStruct move;
    move.card_ids = cards->getSubcards();
    move.from = source;
    move.to_place = Player::DrawPile;
    room->moveCardsAtomic(move, false);
    room->askForGuanxing(source, room->getNCards(x), Room::GuanxingUpOnly, "tianyan");
}

class Tianyan : public ZeroCardViewAsSkill
{
public:
    Tianyan() : ZeroCardViewAsSkill("tianyan")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("TianyanCard");
    }

    virtual const Card *viewAs() const
    {
        return new TianyanCard;
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
    Fengrang() : ZeroCardViewAsSkill("fengrang")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        AmazingGrace *card = new AmazingGrace(Card::NoSuit, 0);
        card->deleteLater();
        return !player->hasUsed("FengrangCard")
                && card->isAvailable(player);
    }

    virtual const Card *viewAs() const
    {
        return new FengrangCard;
    }
};

class Shouhuo : public TriggerSkill
{
public:
    Shouhuo() : TriggerSkill("shouhuo")
    {
        events << TrickCardCanceling;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.to  && effect.to->hasSkill(objectName())
                && effect.card && effect.card->isKindOf("AmazingGrace"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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

void JiliaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    QList<int> equips;
    foreach(const Card *c, target->getEquips())
        equips << c->getId();
    CardsMoveStruct move;
    move.card_ids = equips;
    move.to_place = Player::PlaceHand;
    move.to = target;
    room->moveCardsAtomic(move, true);
    if (target->getHandcardNum() <= target->getMaxCards() || !source->canDiscard(target, "hs"))
        return;
    if (room->askForSkillInvoke(source, "jiliao", "throwcard:" + target->objectName())) {
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
    Jiliao() : ZeroCardViewAsSkill("jiliao")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("JiliaoCard");
    }

    virtual const Card *viewAs() const
    {
        return new JiliaoCard;
    }
};

class Zhongyan : public TriggerSkill
{
public:
    Zhongyan() : TriggerSkill("zhongyan")
    {
        events << DamageInflicted;
        frequency = Limited;
        limit_mark = "@zhongyan";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from == NULL || damage.from->isDead() || damage.from == damage.to ||
                !damage.to->hasSkill(this) || damage.to->getMark("@zhongyan") == 0)
            return QList<SkillInvokeDetail>();
        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->setTag("zhongyan_damage", data);
        int n = qMax(1, damage.from->getLostHp());
        QString prompt = "target:" + damage.from->objectName() + ":" + QString::number(n);
        return room->askForSkillInvoke(invoke->invoker, objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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




TH10Package::TH10Package()
    : Package("th10")
{

    General *kanako = new General(this, "kanako$", "fsl", 4, false);
    kanako->addSkill(new Shende);
    kanako->addSkill(new Qiankun);
    kanako->addSkill(new Gongfeng);

    General *suwako = new General(this, "suwako", "fsl", 3, false);
    suwako->addSkill(new Bushu);
    suwako->addSkill("qiankun");
    suwako->addSkill(new Chuancheng);

    General *sanae = new General(this, "sanae", "fsl", 3, false);
    sanae->addSkill(new DfgzmJiyi);
    sanae->addSkill(new Qiji);

    General *aya = new General(this, "aya", "fsl", 3, false);
    aya->addSkill(new Fengshen);
    aya->addSkill(new Fengsu);
    aya->addSkill(new FengsuEffect);
    related_skills.insertMulti("fengsu", "#fengsu-effect");

    General *nitori = new General(this, "nitori", "fsl", 3, false);
    nitori->addSkill(new Xinshang);
    nitori->addSkill(new XinshangTargetMod);
    nitori->addSkill(new Micai);
    related_skills.insertMulti("xinshang", "#xinshang_effect");

    General *hina = new General(this, "hina", "fsl", 3, false);
    hina->addSkill(new Jie);
    hina->addSkill(new Liuxing);

    General *sanae_sp = new General(this, "sanae_sp", "fsl", 3, false);
    sanae_sp->addSkill(new Changshi);
    sanae_sp->addSkill(new Jinian);
    sanae_sp->addSkill(new JinianClear);
    related_skills.insertMulti("jinian", "#jinian_clear");


    General *momizi = new General(this, "momizi", "fsl", 4, false);
    momizi->addSkill(new Tianyan);

    General *minoriko = new General(this, "minoriko", "fsl", 4, false);
    minoriko->addSkill(new Fengrang);
    minoriko->addSkill(new Shouhuo);


    General *shizuha = new General(this, "shizuha", "fsl", 4, false);
    shizuha->addSkill(new Jiliao);
    shizuha->addSkill(new Zhongyan);

    

    addMetaObject<GongfengCard>();
    addMetaObject<QijiCard>();
    addMetaObject<FengshenCard>();
    addMetaObject<XinshangCard>();
    addMetaObject<TianyanCard>();
    addMetaObject<FengrangCard>();
    addMetaObject<JiliaoCard>();
    skills << new GongfengVS;
}

ADD_PACKAGE(TH10)

