#include "th10.h"

#include "general.h"
#include "settings.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"

#include <QCommandLinkButton>



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
            return pattern.contains("peach");
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

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        bool can = false;
        if (triggerEvent == CardResponded) {
            const Card * card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isKindOf("Slash"))
                can = true;
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash"))
                can = true;
        }
        if (can)
            return QStringList(objectName());
        return QStringList();
    }



    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return room->askForSkillInvoke(player, objectName(), data);
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        player->drawCards(1);
        if (!player->isKongcheng()) {
            const Card *cards = room->askForExchange(player, objectName(), 1, false, "shende-exchange");
            player->addToPile("shende", cards->getSubcards().first());
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
    mute = true;
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
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!player) return QStringList();
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "gongfeng")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName(), false, true))
                    lords << p;
            }
            if (lords.isEmpty()) return QStringList();

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("gongfeng_attach"))
                    room->attachSkillToPlayer(p, "gongfeng_attach");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "gongfeng") {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName(), false, true))
                    lords << p;
            }
            if (lords.length() > 2) return QStringList();

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("gongfeng_attach"))
                    room->detachSkillFromPlayer(p, "gongfeng_attach", true);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                return QStringList();
            if (player->hasFlag("Forbidgongfeng"))
                room->setPlayerFlag(player, "-Forbidgongfeng");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("gongfengInvoked"))
                    room->setPlayerFlag(p, "-gongfengInvoked");
            }
        }
        return QStringList();
    }


};




class Bushu : public TriggerSkill
{
public:
    Bushu() : TriggerSkill("bushu")
    {
        events << Pindian << Damaged;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        if (triggerEvent == Pindian) {
            PindianStruct * pindian = data.value<PindianStruct *>();
            if (pindian->reason == "bushu" && pindian->from_number <= pindian->to_number
                && pindian->from->isAlive())
                pindian->from->obtainCard(pindian->to_card);
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.from || damage.to->isDead() || !damage.from->isAlive())
                return TriggerList();
            TriggerList skill_list;
            QList<ServerPlayer *> suwakos = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *suwako, suwakos) {
                if (damage.from != suwako && !damage.from->isKongcheng() &&
                    (suwako->inMyAttackRange(damage.to) || suwako == damage.to)
                    && !suwako->isKongcheng())
                    skill_list.insert(suwako, QStringList(objectName()));
            }
            return skill_list;
        }
        return TriggerList();

    }

    virtual bool cost(TriggerEvent triggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *suwako) const
    {
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            suwako->tag["bushu_damage"] = data;
            QString prompt = "damage:" + damage.from->objectName() + ":" + damage.to->objectName();
            return suwako->askForSkillInvoke(objectName(), prompt);
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *suwako) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, suwako->objectName(), damage.from->objectName());
        if (suwako->pindian(damage.from, objectName(), NULL)) {
            RecoverStruct recov;
            recov.who = suwako;
            room->recover(damage.to, recov);
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

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!player->hasSkill(objectName())) return QStringList();
        DeathStruct death = data.value<DeathStruct>();
        if (death.who == player)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@chuancheng", true, true);
        if (target) {
            room->handleAcquireDetachSkills(target, "qiankun");
            room->handleAcquireDetachSkills(target, "chuancheng");
            if (player->getCards("hej").length() > 0) {
                DummyCard *allcard = new DummyCard;
                allcard->deleteLater();
                allcard->addSubcards(player->getCards("hej"));
                room->obtainCard(target, allcard, CardMoveReason(CardMoveReason::S_REASON_RECYCLE, target->objectName()), false);
            }
        }
        return false;
    }
};


class Zhunbei : public PhaseChangeSkill
{
public:
    Zhunbei() :PhaseChangeSkill("zhunbei")
    {

    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Draw)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return room->askForSkillInvoke(player, objectName(), data);
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        player->setFlags("zhunbei");
        return true;
    }
};

class ZhunbeiEffect : public TriggerSkill
{
public:
    ZhunbeiEffect() : TriggerSkill("#zhunbei_effect")
    {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            if (player->hasFlag("zhunbei")) {
                player->setFlags("-zhunbei");
                room->touhouLogmessage("#TouhouBuff", player, "zhunbei");
                room->notifySkillInvoked(player, "zhunbei");
                player->drawCards(3);
            }
        }
        return QStringList();
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
    bool doNotShow = false;
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY
        && object_name != "chuangshi" &&  !Sanguosha->currentRoomState()->getCurrentCardUsePattern().contains("slash"))
        doNotShow = true;
    if (doNotShow) {
        emit onButtonClick();
        return;
    }

    foreach (QAbstractButton *button, group->buttons()) {
        const Card *card = map[button->objectName()];
        const Player *user;
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


        bool enabled = !user->isCardLimited(card, Card::MethodUse, true) && card->isAvailable(user);
        if (object_name == "huaxiang" && user->getMaxHp() > 2 && card->isKindOf("Peach"))
            enabled = false;

        QStringList response_use_salsh;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY
            &&  Sanguosha->currentRoomState()->getCurrentCardUsePattern().contains("slash")) {
            response_use_salsh << "Slash" << "ThunderSlash" << "FireSlash";
        }

        if (object_name == "nianli" && card->isKindOf("Slash"))
            enabled = true;
        if (!response_use_salsh.isEmpty() && !response_use_salsh.contains(card->getClassName())) {
            button->setEnabled(false);
        } else
            button->setEnabled(enabled);
    }

    //if only one choice


    Self->tag.remove(object_name);
    exec();
}

void QijiDialog::selectCard(QAbstractButton *button)
{
    const Card *card = map.value(button->objectName());
    Self->tag[object_name] = QVariant::fromValue(card);
    //if (button->objectName().contains("slash")) {  //nature slash?
    //    if (objectName() == "guhuo")
    //        Self->tag["GuhuoSlash"] = button->objectName();
    // }

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

    if (object_name == "nianli") {
        foreach (const Card *card, cards) {
            if (card->getClassName() != "Slash")
                ban_list << card->getClassName();
        }
    }


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

    if (object_name == "nianli") {
        foreach (const Card *card, cards) {
            if (card->getClassName() != "Snatch")
                ban_list << card->getClassName();
        }
    }


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
    mute = true;
    will_throw = false;
}

bool QijiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            const Card *oc = Sanguosha->getCard(subcards.first());
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), oc->getSuit(), oc->getNumber());
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
    new_card->addSubcard(oc);
    new_card->setSkillName("qiji");
    if (card->isKindOf("IronChain") && targets.length() == 0)
        return false;
    return new_card && new_card->targetsFeasible(targets, Self);
}

const Card *QijiCard::validate(CardUseStruct &) const
{
    QString to_use = user_string;

    const Card *card = Sanguosha->getCard(subcards.first());
    Card *use_card = Sanguosha->cloneCard(to_use, card->getSuit(), card->getNumber());
    use_card->setSkillName("qiji");
    use_card->addSubcard(subcards.first());
    use_card->deleteLater();
    return use_card;
}

const Card *QijiCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();

    QString to_use;
    if (user_string == "peach+analeptic") {
        QStringList use_list;
        Card *peach = Sanguosha->cloneCard("peach");
        if (!user->isCardLimited(peach, Card::MethodResponse, true))
            use_list << "peach";
        Card *ana = Sanguosha->cloneCard("analeptic");
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

    return use_card;
}


class QijiVS : public OneCardViewAsSkill
{
public:
    QijiVS() : OneCardViewAsSkill("qiji")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getHandcardNum() != 1 || pattern.startsWith(".") || pattern.startsWith("@")) return false;
        if (player->getMark("qiji") > 0) return false;
        if (pattern == "peach" && player->hasFlag("Global_PreventPeach")) return false;
        for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false;
        }

        if (pattern == "slash" || pattern == "jink") {
            Card *card = Sanguosha->cloneCard(pattern);
            return !player->isCardLimited(card, Card::MethodResponse, true);
        }

        if (pattern.contains("peach") && pattern.contains("analeptic")) {
            Card *peach = Sanguosha->cloneCard("peach");
            Card *ana = Sanguosha->cloneCard("analeptic");
            return !player->isCardLimited(peach, Card::MethodResponse, true) ||
                !player->isCardLimited(ana, Card::MethodResponse, true);
        } else if (pattern == "peach") {
            Card *peach = Sanguosha->cloneCard("peach");
            return !player->isCardLimited(peach, Card::MethodResponse, true);
        } else if (pattern == "analeptic") {
            Card *ana = Sanguosha->cloneCard("analeptic");
            return !player->isCardLimited(ana, Card::MethodResponse, true);
        }

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
            if (pattern.contains("slash")) {
                const Card *c = Self->tag.value("qiji").value<const Card *>();
                if (c)
                    pattern = c->objectName();
                else
                    return NULL;
            }
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
        if (player->isCardLimited(Sanguosha->cloneCard("nullification"), Card::MethodResponse, true))
            return false;
        return player->getHandcardNum() == 1;
    }
};

class Qiji : public TriggerSkill
{
public:
    Qiji() : TriggerSkill("qiji")
    {
        events << EventPhaseChanging << PreCardUsed << CardResponded;
        view_as_skill = new QijiVS;
    }

    virtual QDialog *getDialog() const
    {
        return QijiDialog::getInstance("qiji");
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {

        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getMark("qiji") > 0)
                    room->setPlayerMark(p, "qiji", 0);
            }
        } else {
            ServerPlayer *current = room->getCurrent();
            if (!current || !current->isAlive() || current->getPhase() == Player::NotActive)
                return QStringList();
            if (triggerEvent == PreCardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.card->getSkillName() == "qiji") {
                    room->setPlayerMark(use.from, "qiji", 1);
                }
            }
            if (triggerEvent == CardResponded) {
                const Card * card_star = data.value<CardResponseStruct>().m_card;
                if (card_star->getSkillName() == "qiji")
                    room->setPlayerMark(player, "qiji", 1);
            }
        }

        return QStringList();
    }
};



FengshenCard::FengshenCard()
{
    will_throw = true;
    mute = true;
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

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        room->notifySkillInvoked(player, "fengsu");
        return QStringList();
    }
};

XinshangCard::XinshangCard()
{
    mute = true;
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
        if (effect.from->canDiscard(effect.to, "he")) {
            room->throwCard(room->askForCardChosen(effect.from, effect.to, "he", "xinshang", false, Card::MethodDiscard), effect.to, effect.from);
            if (effect.from->canDiscard(effect.to, "he"))
                room->throwCard(room->askForCardChosen(effect.from, effect.to, "he", "xinshang", false, Card::MethodDiscard), effect.to, effect.from);
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

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        int num = player->getHandcardNum();
        if (num == 0 || damage.damage > num)
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        int num = player->getHandcardNum();
        if (num == 0) {
            room->touhouLogmessage("#micai01", player, "micai", QList<ServerPlayer *>(), QString::number(damage.damage - num));
            room->notifySkillInvoked(player, objectName());
            return true;
        } else if (damage.damage > num) {
            room->touhouLogmessage("#micai01", player, "micai", QList<ServerPlayer *>(), QString::number(damage.damage - num));
            damage.damage = num;
            room->notifySkillInvoked(player, objectName());
            data = QVariant::fromValue(damage);
        }
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


    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer* &) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *hina = room->findPlayerBySkillName(objectName());
        if (hina && damage.to != hina)
            return QStringList(objectName());

        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const
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
        ServerPlayer *hina = room->findPlayerBySkillName(objectName());
        hina->tag["jie_damage"] = data;
        return room->askForSkillInvoke(hina, objectName(), prompt);
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {

        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *hina = room->findPlayerBySkillName(objectName());

        hina->drawCards(1);
        damage.to = hina;
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

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (player->getPhase() == Player::Finish) {
            ServerPlayer *hina = room->findPlayerBySkillName(objectName());
            if (hina && hina != player && hina->isWounded() && !hina->isKongcheng())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {

        ServerPlayer *hina = room->findPlayerBySkillName(objectName());
        const Card *card = room->askForCard(hina, ".|black|.|hand", "@liuxing:" + player->objectName(), data, Card::MethodNone, player, false, objectName());
        if (card) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, hina->objectName(), player->objectName());
            room->notifySkillInvoked(hina, objectName());
            room->touhouLogmessage("#InvokeSkill", hina, objectName());

            room->obtainCard(player, card, true);
            //need add player information to help ai  make decision
            player->tag["liuxing_source"] = QVariant::fromValue(hina);
            QString choice = room->askForChoice(player, objectName(), "losehp+recover", QVariant::fromValue(hina));
            if (choice == "losehp")
                room->loseHp(player, 1);
            else
                room->recover(hina, RecoverStruct());
        }

        return false;
    }
};


class Changshi : public TriggerSkill
{
public:
    Changshi() : TriggerSkill("changshi")
    {
        events << EventPhaseStart << Death << EventPhaseChanging;//<< EventLoseSkill
        frequency = Eternal;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {

        if (!player->hasSkill(objectName()))
            return QStringList();
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart)
            return QStringList(objectName());
        else if (triggerEvent == Death || triggerEvent == EventPhaseChanging) {
            if (triggerEvent == Death) {
                DeathStruct death = data.value<DeathStruct>();
                if (death.who != player)
                    return QStringList();
            }
            if (triggerEvent == EventPhaseChanging) {
                PhaseChangeStruct change = data.value<PhaseChangeStruct>();
                if (change.to != Player::NotActive)
                    return QStringList();
            }
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                room->setPlayerMark(p, "changshi", 0);
                room->filterCards(p, p->getCards("he"), true);
                QVariant args;
                args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

            }
            foreach(ServerPlayer *p, room->getOtherPlayers(player))
                p->loseMark("@changshi", 1);
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) {
            foreach(ServerPlayer *p, room->getOtherPlayers(player))
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());

            room->touhouLogmessage("#changshi01", player, "changshi");
            room->notifySkillInvoked(player, objectName());
            //for zhengti huashen UI

            //remove card limit,if the source skill can not clear it correctly.
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasSkill("zhengti")) {
                    QVariant arg(Json::arrayValue);
                    arg[0] = (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
                    arg[1] = JsonUtils::toJsonString(p->objectName());
                    arg[2] = JsonUtils::toJsonString(p->getGeneral()->objectName());
                    arg[3] = JsonUtils::toJsonString(QString());//"clear"
                    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
                }
                room->setPlayerMark(p, "changshi", 1); // real mark for   Player::hasSkill()
                room->filterCards(p, p->getCards("he"), true);
                QVariant args;
                args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

            }
            foreach (ServerPlayer *p, room->getOtherPlayers(player))
                p->gainMark("@changshi");

            QStringList marks;
            marks << "@clock" << "@kinki" << "@qiannian" << "@shi" << "@ye" << "@yu" << "@zhengti"
                << "@xinyang";
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                //throw cards in special place
                QList<int>  idlist;
                foreach (QString pile, p->getPileNames()) {
                    if (pile == "wooden_ox") continue;
                    foreach(int card, p->getPile(pile))
                        idlist << card;
                }
                if (idlist.length() > 0) {
                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, p->objectName(), NULL, objectName(), "");
                    CardsMoveStruct move(idlist, p, Player::DiscardPile,
                        reason);
                    room->moveCardsAtomic(move, true);
                }
                // may be we can deal this before adding changshi mark?
                foreach (QString m, marks) {
                    if (p->getMark(m) > 0)
                        p->loseAllMarks(m);
                }

            }
        }

        return false;
    }
};

class Jinian : public TriggerSkill
{
public:
    Jinian() : TriggerSkill("jinian")
    {
        events << CardsMoveOneTime << BeforeCardsMove;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (!player || !player->hasSkill(objectName()) || player->hasFlag("jinian_used")) return;
        if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place != Player::PlaceJudge && move.to_place != Player::PlaceTable
                && move.to_place != Player::DiscardPile)  //just for temp?
                return;
            if (move.from && move.from_places.contains(Player::PlaceHand) && move.from == player) {

                QVariantList record_ids = player->tag["jinian"].toList();
                QList<int> h_ids;
                foreach (int id, move.card_ids) {
                    if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand)
                        h_ids << id;
                }

                //if is last_hand_cards,record its.
                if (move.from->getHandcardNum() <= h_ids.length()) {
                    foreach (int id, h_ids) {
                        //check ocurrence
                        if (!record_ids.contains(id))
                            record_ids << id;
                    }
                    player->tag["jinian"] = record_ids;
                }
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            QVariantList ids = player->tag["jinian"].toList();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place != Player::PlaceJudge && move.to_place != Player::PlaceTable
                && move.to_place != Player::DiscardPile) {
                // (place == Player::PlaceHand || place == Player::PlaceEquip || place == Player::PlaceDelayedTrick
                //    || place == Player::PlaceSpecial || place == Player::DrawPile) 
                QVariantList new_ids;
                foreach (QVariant card_data, ids) {
                    int card_id = card_data.toInt();
                    if (!move.card_ids.contains(card_id))
                        new_ids << card_id;
                }
                player->tag["jinian"] = new_ids;
            }

            else if (move.to_place == Player::DiscardPile) {
                QVariantList get_ids;
                QVariantList backup_ids;
                foreach (QVariant card_data, ids) {
                    int card_id = card_data.toInt();
                    if (move.card_ids.contains(card_id) &&
                        room->getCardPlace(card_id) == Player::DiscardPile) {
                        get_ids << card_id;
                    } else
                        backup_ids << card_id;
                }
                //need check from and delete get_ids
                if (!move.from || move.from != player) {

                    QVariantList new_get_ids;
                    get_ids = new_get_ids;
                }
                player->tag["jinian"] = backup_ids;
                player->tag["jinianGet"] = get_ids;
            }
        }
    }


    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player))return QStringList();
        //if (!player->hasSkill(objectName())) 
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (player->hasFlag("jinian_used"))
                return QStringList();
            if (player->isDead())
                return QStringList();

            //record tmp: get it or not
            QVariantList record = player->tag["jinianGet"].toList();

            if (!record.isEmpty())
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (room->askForSkillInvoke(player, objectName(), data))
            return true;
        else {
            player->tag.remove("jinianGet");
        }
        return false;
    }


    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *current = room->getCurrent();
        if (current && current->isAlive() && current->getPhase() != Player::NotActive)
            player->setFlags("jinian_used");
        QList<int> get_ids;
        QVariantList delete_ids = player->tag["jinianGet"].toList();
        foreach (QVariant card_data, delete_ids) {
            int card_id = card_data.toInt();
            get_ids << card_id;
        }

        CardsMoveStruct mo;
        mo.card_ids = get_ids;
        mo.to = player;
        mo.to_place = Player::PlaceHand;
        room->moveCardsAtomic(mo, true);

        player->tag.remove("jinian");
        player->tag.remove("jinianGet");
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
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer* &) const
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                p->tag.remove("jinian");
                p->tag.remove("jinianGet");
                if (p->hasFlag("jinian_used"))
                    p->setFlags("-jinian_used");
            }
        }
        return QStringList();
    }
};


TianyanCard::TianyanCard()
{
    target_fixed = true;
    mute = true;
}
void TianyanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    int x = qMin(room->alivePlayerCount(), 4);

    source->drawCards(x);
    const Card *cards = room->askForExchange(source, "tianyan", x, false, "tianyan_exchange:" + QString::number(x));
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
    mute = true;
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

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer* &) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.to  && effect.to->hasSkill(objectName())
            && effect.card && effect.card->isKindOf("AmazingGrace"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        room->notifySkillInvoked(effect.to, objectName());
        return true;
    }
};


JiliaoCard::JiliaoCard()
{
    mute = true;
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
    if (target->getHandcardNum() <= target->getMaxCards() || !source->canDiscard(target, "h"))
        return;
    if (room->askForSkillInvoke(source, "jiliao", "throwcard:" + target->objectName())) {
        int to_throw = room->askForCardChosen(source, target, "h", "jiliao", false, Card::MethodDiscard);
        room->throwCard(to_throw, target, source);
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

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from == NULL || damage.from->isDead() || damage.from == player
            || player->getMark("@zhongyan") == 0)
            return QStringList();
        return QStringList(objectName());
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->setTag("zhongyan_damage", data);
        int n = qMax(1, damage.from->getLostHp());
        QString prompt = "target:" + damage.from->objectName() + ":" + QString::number(n);
        return room->askForSkillInvoke(player, objectName(), prompt);
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {

        DamageStruct damage = data.value<DamageStruct>();

        room->removePlayerMark(player, "@zhongyan");
        room->doLightbox("$zhongyanAnimate", 4000);
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());

        int x = damage.from->getLostHp();
        room->loseHp(damage.from, qMax(1, x));
        return true;

    }
};

DfgzmSiyuCard::DfgzmSiyuCard()
{
    mute = true;
    will_throw = false;
}
void DfgzmSiyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    room->obtainCard(target, subcards.first(), false);
    source->setFlags("dfgzmsiyu");
    source->tag["dfgzmsiyu"] = QVariant::fromValue(target);
}
class DfgzmsiyuVS : public OneCardViewAsSkill
{
public:
    DfgzmsiyuVS() : OneCardViewAsSkill("dfgzmsiyu")
    {
        filter_pattern = ".|.|.|hand";
    }


    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("DfgzmSiyuCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            DfgzmSiyuCard *card = new DfgzmSiyuCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }
};

class DfgzmSiyu : public TriggerSkill
{
public:
    DfgzmSiyu() : TriggerSkill("dfgzmsiyu")
    {
        events << EventPhaseChanging;
        view_as_skill = new DfgzmsiyuVS;
    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {

        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive && player->hasFlag("dfgzmsiyu")) {
            player->setFlags("-dfgzmsiyu");
            ServerPlayer *target = player->tag["dfgzmsiyu"].value<ServerPlayer *>();
            player->tag.remove("dfgzmsiyu");
            if (target != NULL && target->isAlive() && !target->isKongcheng()) {

                room->touhouLogmessage("#TouhouBuff", player, "dfgzmsiyu");
                room->notifySkillInvoked(player, "dfgzmsiyu");
                int id = room->askForCardChosen(player, target, "h", objectName(), true);
                if (id > -1)
                    room->obtainCard(player, id, false);
            }
        }
        return QStringList();
    }
};

//for Collateral
ExtraCollateralCard::ExtraCollateralCard()
{
}

bool ExtraCollateralCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *coll = Card::Parse(Self->property("extra_collateral").toString());
    if (!coll) return false;
    QStringList tos = Self->property("extra_collateral_current_targets").toString().split("+");

    if (targets.isEmpty())
        return !tos.contains(to_select->objectName())
        && !Self->isProhibited(to_select, coll) && coll->targetFilter(targets, to_select, Self);
    else
        return coll->targetFilter(targets, to_select, Self);
}

void ExtraCollateralCard::onUse(Room *, const CardUseStruct &card_use) const
{
    Q_ASSERT(card_use.to.length() == 2);
    ServerPlayer *killer = card_use.to.first();
    ServerPlayer *victim = card_use.to.last();
    killer->setFlags("ExtraCollateralTarget");
    killer->tag["collateralVictim"] = QVariant::fromValue((ServerPlayer *)victim);
}


class QishuVS : public ZeroCardViewAsSkill
{
public:
    QishuVS() : ZeroCardViewAsSkill("qishu")
    {
        response_pattern = "@@qishu";
    }


    virtual const Card *viewAs() const
    {
        return new ExtraCollateralCard;
    }
};
class QishuTargetMod : public TargetModSkill
{
public:
    QishuTargetMod() : TargetModSkill("#qishu-mod")
    {
        pattern = "Slash,TrickCard+^DelayedTrick";
    }
    static bool isLastHandCard(const Player *player, const Card *card)
    {
        QList<int> subcards = card->getSubcards();
        if (subcards.length() == 0 || player->isKongcheng())
            return false;
        QList<int> wood_ox = player->getPile("wooden_ox");
        int wood_num = 0;
        foreach (int id, subcards) {
            if (wood_ox.contains(id))
                wood_num++;
        }
        if (subcards.length() - wood_num == player->getHandcardNum())
            return true;
        else
            return false;
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const
    {
        if (from->hasSkill("qishu") && isLastHandCard(from, card))
            return 1000;
        else
            return 0;
    }


    virtual int getExtraTargetNum(const Player *player, const Card *card) const
    {
        if (player->hasSkill("qishu") && player->getPhase() == Player::Play && isLastHandCard(player, card))
            return 1000;
        else
            return 0;
    }
};
class Qishu : public TriggerSkill
{
public:
    Qishu() : TriggerSkill("qishu")
    {
        events << PreCardUsed;
        view_as_skill = new  QishuVS;
    }
    static bool isQishu(QList<ServerPlayer *>players, const Card *card)
    {
        if (card->isKindOf("GlobalEffect") || card->isKindOf("AOE"))
            return false;
        if (card->isKindOf("IronChain"))
            return players.length() > 2;
        return true;
    }




    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!player->hasSkill(objectName())) return QStringList();
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            bool  extra_col = false;
            //process Collateral
            if (QishuTargetMod::isLastHandCard(player, use.card) && use.card->isKindOf("Collateral")) {
                while (true) {
                    ServerPlayer *extra = NULL;
                    QStringList tos;
                    foreach(ServerPlayer *t, use.to)
                        tos.append(t->objectName());
                    room->setPlayerProperty(player, "extra_collateral", use.card->toString());
                    room->setPlayerProperty(player, "extra_collateral_current_targets", tos.join("+"));
                    room->askForUseCard(player, "@@qishu", "@qishu-add:::collateral");
                    room->setPlayerProperty(player, "extra_collateral", QString());
                    room->setPlayerProperty(player, "extra_collateral_current_targets", QString("+"));
                    foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                        if (p->hasFlag("ExtraCollateralTarget")) {
                            p->setFlags("-ExtraCollateralTarget");
                            extra = p;
                            break;
                        }
                    }
                    if (extra != NULL) {
                        extra_col = true;
                        use.to.append(extra);
                        room->sortByActionOrder(use.to);
                        LogMessage log;
                        log.type = "#QishuAdd";
                        log.from = player;
                        log.to << extra;
                        log.arg = use.card->objectName();
                        log.arg2 = "qishu";
                        room->sendLog(log);
                        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), extra->objectName());

                        ServerPlayer *victim = extra->tag["collateralVictim"].value<ServerPlayer *>();
                        if (victim) {
                            LogMessage log;
                            log.type = "#CollateralSlash";
                            log.from = player;
                            log.to << victim;
                            room->sendLog(log);
                            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, extra->objectName(), victim->objectName());
                        }
                    } else
                        break;
                }
                data = QVariant::fromValue(use);

            }

            //for log
            if (use.card->isKindOf("Collateral")) {
                if (extra_col)
                    room->notifySkillInvoked(player, "qishu");
            } else if (use.to.length() > 1 && QishuTargetMod::isLastHandCard(player, use.card)) {
                if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {

                    //sanguosha:correctCardTarget
                    if (isQishu(use.to, use.card))
                        room->notifySkillInvoked(player, "qishu");

                }
            }
        }
        return QStringList();
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
    sanae->addSkill(new Zhunbei);
    sanae->addSkill(new ZhunbeiEffect);
    sanae->addSkill(new Qiji);
    related_skills.insertMulti("zhunbei", "#zhunbei_effect");


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

    General *sanae_slm = new General(this, "sanae_slm", "fsl", 4, false);
    sanae_slm->addSkill(new DfgzmSiyu);
    sanae_slm->addSkill(new Qishu);
    sanae_slm->addSkill(new QishuTargetMod);
    related_skills.insertMulti("qishu", "#qishu-mod");


    addMetaObject<GongfengCard>();
    addMetaObject<QijiCard>();
    addMetaObject<FengshenCard>();
    addMetaObject<XinshangCard>();
    addMetaObject<TianyanCard>();
    addMetaObject<FengrangCard>();
    addMetaObject<JiliaoCard>();
    addMetaObject<DfgzmSiyuCard>();
    addMetaObject<ExtraCollateralCard>();
    skills << new GongfengVS;
}

ADD_PACKAGE(TH10)

