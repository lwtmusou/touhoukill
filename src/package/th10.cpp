#include "th10.h"


#include "general.h"
#include "settings.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"
#include "jsonutils.h"
//#include "touhoucard.h" //for qishu
//#include "yjcm2013-package.h"
#include <QCommandLinkButton>




shendeDummyCard::shendeDummyCard() {
    //mute = true;
    will_throw = false;
    target_fixed = true;
}

shendeFakeMoveCard::shendeFakeMoveCard() {
    //mute = true;
    will_throw = false;
    target_fixed = true;
}
const Card *shendeFakeMoveCard::validate(CardUseStruct &use) const{
    Room *room = use.from->getRoom();
    QList<int> card_ids = use.from->getPile("ShenDePile");
    CardMoveReason reason(CardMoveReason::S_REASON_PREVIEWGIVE, use.from->objectName(), "ShenDePile", "ShenDePile");
    CardsMoveStruct move(card_ids, use.from, use.from, Player::PlaceSpecial, Player::PlaceHand, reason);
    QList<CardsMoveStruct> moves;
    moves << move;
    QList<ServerPlayer *> players;
    players << use.from;
    room->notifyMoveCards(true, moves, true, players);
    room->notifyMoveCards(false, moves, true, players);

    const Card *card = room->askForUseCard(use.from, "@@shende!", "@shende-twoCards");
    CardsMoveStruct move1(card_ids, use.from, NULL, Player::PlaceHand, Player::PlaceTable, reason);
    QList<CardsMoveStruct> moves1;
    moves1 << move1;
    room->notifyMoveCards(true, moves1, true, players);
    room->notifyMoveCards(false, moves1, true, players);

    if (card != NULL){
        Peach *peach = new Peach(Card::SuitToBeDecided, -1);
        foreach(int id, card->getSubcards())
            peach->addSubcard(id);
        peach->setSkillName("shende");
        return peach;
    }
    return NULL;
}



class shendevs : public ViewAsSkill {
public:
    shendevs() : ViewAsSkill("shende") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (player->getPile("ShenDePile").length() < 2)
            return false;
        Peach *peach = new Peach(Card::NoSuit, 0);
        peach->deleteLater();
        return peach->isAvailable(player);
    }
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        if (player->getMark("Global_PreventPeach") > 0)
            return false;
        if (pattern == "@@shende!")
            return true;
        if (player->getPile("ShenDePile").length() >= 2)
            return pattern.contains("peach");
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@shende!"){
            QVariantList s_ids = Self->tag["shende_piles"].toList();
            QList<int> shendes;
            foreach(QVariant card_data, s_ids)
                shendes << card_data.toInt();
            return shendes.contains(to_select->getEffectiveId());
        }
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@shende!"){
            if (cards.length() != 2)
                return NULL;
            shendeDummyCard *card = new shendeDummyCard;
            card->addSubcards(cards);
            return card;
        } else {
            QVariantList ids = Self->tag["GuzhengToGet"].toList();
            foreach(int card_id, Self->getPile("ShenDePile"))
                ids << card_id;
            Self->tag["shende_piles"] = ids;
            return new shendeFakeMoveCard;
        }
    }
};

class shende : public TriggerSkill {
public:
    shende() : TriggerSkill("shende") {
        events << CardUsed << CardResponded;
        view_as_skill = new shendevs;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        bool can = false;
        if (triggerEvent == CardResponded){
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isKindOf("Slash"))
                can = true;
        } else if (triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash"))
                can = true;
        }
        if (can &&  room->askForSkillInvoke(player, objectName(), data)){
            player->drawCards(1);
            if (!player->isKongcheng()){
                const Card *cards = room->askForExchange(player, objectName(), 1, false, "shende-exchange");
                player->addToPile("ShenDePile", cards->getSubcards().first());
            }

        }
        return false;
    }
};

class qiankun : public MaxCardsSkill {
public:
    qiankun() : MaxCardsSkill("qiankun") {
    }

    virtual int getExtra(const Player *target) const{

        if (target->hasSkill("qiankun"))
            return 2;
        else
            return 0;
    }
};


gongfengCard::gongfengCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "gongfengvs";
    mute = true;
}
void gongfengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *kanako = targets.first();
    if (kanako->hasLordSkill("gongfeng")) {
        room->setPlayerFlag(kanako, "gongfengInvoked");

        room->notifySkillInvoked(kanako, "gongfeng");
        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), kanako->objectName(), "gongfeng", QString());
        room->obtainCard(kanako, this, reason);
        QList<ServerPlayer *> kanakos;
        QList<ServerPlayer *> players = room->getOtherPlayers(source);
        foreach(ServerPlayer *p, players) {
            if (p->hasLordSkill("gongfeng") && !p->hasFlag("gongfengInvoked"))
                kanakos << p;
        }
        if (kanakos.isEmpty())
            room->setPlayerFlag(source, "Forbidgongfeng");
    }
}
bool gongfengCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasLordSkill("gongfeng")
        && to_select != Self && !to_select->hasFlag("gongfengInvoked");
}

class gongfengvs : public OneCardViewAsSkill {
public:
    gongfengvs() :OneCardViewAsSkill("gongfengvs") {
        attached_lord_skill = true;
        filter_pattern = "Slash";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return  !player->hasFlag("Forbidgongfeng") && player->getKingdom() == "fsl";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        gongfengCard *card = new gongfengCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class gongfeng : public TriggerSkill {
public:
    gongfeng() : TriggerSkill("gongfeng$") {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "gongfeng")) {
            QList<ServerPlayer *> lords;
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty()) return false;

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach(ServerPlayer *p, players) {
                if (!p->hasSkill("gongfengvs"))
                    room->attachSkillToPlayer(p, "gongfengvs");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "gongfeng") {
            QList<ServerPlayer *> lords;
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.length() > 2) return false;

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach(ServerPlayer *p, players) {
                if (p->hasSkill("gongfengvs"))
                    room->detachSkillFromPlayer(p, "gongfengvs", true);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                return false;
            if (player->hasFlag("Forbidgongfeng"))
                room->setPlayerFlag(player, "-Forbidgongfeng");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach(ServerPlayer *p, players) {
                if (p->hasFlag("gongfengInvoked"))
                    room->setPlayerFlag(p, "-gongfengInvoked");
            }
        }
        return false;
    }
};




class bushu : public TriggerSkill {
public:
    bushu() : TriggerSkill("bushu") {
        events << Pindian << Damaged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Damaged){
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from == NULL || damage.to->isDead() || !damage.from->isAlive())
                return false;
            ServerPlayer *suwako = room->findPlayerBySkillName(objectName());
            if (suwako == NULL || suwako->isKongcheng())
                return false;
            if (damage.from == suwako || damage.from->isKongcheng() || !suwako->inMyAttackRange(damage.to))
                return false;

            suwako->tag["bushu_damage"] = data;
            QString prompt = "damage:" + damage.from->objectName() + ":" + damage.to->objectName();
            if (suwako->askForSkillInvoke(objectName(), prompt)){
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, suwako->objectName(), damage.from->objectName());
            
				if (suwako->pindian(damage.from, objectName(), NULL)){
                    RecoverStruct recov;
                    recov.who = suwako;
                    room->recover(damage.to, recov);
                }
            }
        }
        if (triggerEvent == Pindian){
            PindianStar pindian = data.value<PindianStar>();
            if (pindian->reason == "bushu" && pindian->from_number <= pindian->to_number
                && pindian->from->isAlive())
                pindian->from->obtainCard(pindian->to_card);
        }
        return false;
    }
};


class chuancheng : public TriggerSkill {
public:
    chuancheng() : TriggerSkill("chuancheng") {
        events << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@chuancheng", true, true);
        if (target) {
            room->handleAcquireDetachSkills(target, "qiankun");
            room->handleAcquireDetachSkills(target, "chuancheng");
            if (player->getCards("hej").length() > 0){
                DummyCard *allcard = new DummyCard;
                allcard->deleteLater();                
                allcard->addSubcards(player->getCards("hej"));
                room->obtainCard(target, allcard, CardMoveReason(CardMoveReason::S_REASON_RECYCLE, target->objectName()), false);
            }
        }
        return false;
    }
};


class zhunbei : public PhaseChangeSkill {
public:
    zhunbei() :PhaseChangeSkill("zhunbei") {

    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if (player->getPhase() == Player::Draw) {
            if (player->askForSkillInvoke(objectName(), QVariant())) {
                player->setFlags("zhunbei");
                return true;
            }
        }
        return false;
    }
};

class zhunbei_effect : public TriggerSkill {
public:
    zhunbei_effect() : TriggerSkill("#zhunbei_effect") {
        events << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return  (target != NULL && target->hasFlag("zhunbei"));

    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            if (player->hasFlag("zhunbei")){
                player->setFlags("-zhunbei");
                room->touhouLogmessage("#TouhouBuff", player, "zhunbei");
                room->notifySkillInvoked(player, "zhunbei");
                player->drawCards(3);
            }
        }
        return false;
    }
};


qijiDialog *qijiDialog::getInstance(const QString &object, bool left, bool right) {
    static qijiDialog *instance;
    if (instance == NULL || instance->objectName() != object)
        instance = new qijiDialog(object, left, right);

    return instance;
}

qijiDialog::qijiDialog(const QString &object, bool left, bool right) : object_name(object) {
    setObjectName(object);
    setWindowTitle(Sanguosha->translate(object)); //need translate title?
    group = new QButtonGroup(this);

    QHBoxLayout *layout = new QHBoxLayout;
    if (left) layout->addWidget(createLeft());
    if (right) layout->addWidget(createRight());
    setLayout(layout);

    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectCard(QAbstractButton *)));
}


void qijiDialog::popup() {
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY
        &&    object_name != "chuangshi") {
        emit onButtonClick();
        return;
    }

    foreach(QAbstractButton *button, group->buttons()) {
        const Card *card = map[button->objectName()];
        const Player *user;
        if (object_name == "chuangshi"){ //check the card is Available for chuangshi target.
            foreach(const Player *p, Self->getAliveSiblings()){
                if (p->getMark("chuangshi_user") > 0){
                    user = p;
                    break;
                }
            }
        } else
            user = Self;
        if (user == NULL)
            user = Self;
        bool enabled = !user->isCardLimited(card, Card::MethodUse, true) && card->isAvailable(user);
        button->setEnabled(enabled);
    }

    Self->tag.remove(object_name);
    exec();
}

void qijiDialog::selectCard(QAbstractButton *button){
    const Card *card = map.value(button->objectName());
    Self->tag[object_name] = QVariant::fromValue(card);
    //if (button->objectName().contains("slash")) {  //nature slash?
    //    if (objectName() == "guhuo")
    //        Self->tag["GuhuoSlash"] = button->objectName();
    // }
    emit onButtonClick();
    accept();
}

QGroupBox *qijiDialog::createLeft() {
    QGroupBox *box = new QGroupBox;
    box->setTitle(Sanguosha->translate("basic"));

    QVBoxLayout *layout = new QVBoxLayout;

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    QStringList ban_list; //no need to ban
    if (object_name == "chuangshi")
        ban_list << "Analeptic";
    foreach(const Card *card, cards) {
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

QGroupBox *qijiDialog::createRight() {
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
    foreach(const Card *card, cards){
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

QAbstractButton *qijiDialog::createButton(const Card *card){
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

qijiCard::qijiCard() {
    mute = true;
    will_throw = false;
}

bool qijiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const {
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty()) {
            const Card *oc = Sanguosha->getCard(subcards.first());
            card = Sanguosha->cloneCard(user_string.split("+").first(), oc->getSuit(), oc->getNumber());
        }
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
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

bool qijiCard::targetFixed() const {
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty()) {
            const Card *oc = Sanguosha->getCard(subcards.first());
            card = Sanguosha->cloneCard(user_string.split("+").first(), oc->getSuit(), oc->getNumber());
        }
        return card && card->targetFixed();
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

bool qijiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty()) {
            const Card *oc = Sanguosha->getCard(subcards.first());
            card = Sanguosha->cloneCard(user_string.split("+").first(), oc->getSuit(), oc->getNumber());
        }

        return card && card->targetsFeasible(targets, Self);
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

const Card *qijiCard::validate(CardUseStruct &card_use) const {
    ServerPlayer *qiji_general = card_use.from;

    Room *room = qiji_general->getRoom();
    QString to_use = user_string;

    if (user_string == "slash"
        &&
        (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
        || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)) {
        QStringList use_list;
        use_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            use_list << "thunder_slash" << "fire_slash";
        to_use = room->askForChoice(qiji_general, "qiji_skill_slash", use_list.join("+"));
    }

    const Card *card = Sanguosha->getCard(subcards.first());
    Card *use_card = Sanguosha->cloneCard(to_use, card->getSuit(), card->getNumber());
    use_card->setSkillName("qiji");
    use_card->addSubcard(subcards.first());
    use_card->deleteLater();
    qiji_general->gainMark("@qiji");
    return use_card;
}

const Card *qijiCard::validateInResponse(ServerPlayer *user) const{
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
    } else if (user_string == "slash") {
        QStringList use_list;
        use_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            use_list << "thunder_slash" << "fire_slash";
        to_use = room->askForChoice(user, "qiji_skill_slash", use_list.join("+"));
    } else
        to_use = user_string;

    const Card *card = Sanguosha->getCard(subcards.first());
    Card *use_card = Sanguosha->cloneCard(to_use, card->getSuit(), card->getNumber());
    use_card->setSkillName("qiji");
    use_card->addSubcard(subcards.first());
    use_card->deleteLater();
    user->gainMark("@qiji");
    return use_card;
}


class qiji : public OneCardViewAsSkill {
public:
    qiji() : OneCardViewAsSkill("qiji") {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        if (player->getHandcardNum() != 1 || pattern.startsWith(".") || pattern.startsWith("@")) return false;
        if (player->getMark("@qiji") > 0) return false;
        if (pattern == "peach" && player->hasFlag("Global_PreventPeach")) return false;
        for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false;
        }

        if (pattern == "slash" || pattern == "jink"){
            Card *card = Sanguosha->cloneCard(pattern);
            return !player->isCardLimited(card, Card::MethodResponse, true);
        }

        if (pattern.contains("peach") && pattern.contains("analeptic")){
            Card *peach = Sanguosha->cloneCard("peach");
            Card *ana = Sanguosha->cloneCard("analeptic");
            return !player->isCardLimited(peach, Card::MethodResponse, true) ||
                !player->isCardLimited(ana, Card::MethodResponse, true);
        } else if (pattern == "peach"){
            Card *peach = Sanguosha->cloneCard("peach");
            return !player->isCardLimited(peach, Card::MethodResponse, true);
        } else if (pattern == "analeptic"){
            Card *ana = Sanguosha->cloneCard("analeptic");
            return !player->isCardLimited(ana, Card::MethodResponse, true);
        }

        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const {
        if (player->getMark("@qiji") > 0) return false;
        return player->getHandcardNum() == 1;
    }

    virtual const Card *viewAs(const Card *originalCard) const {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            qijiCard *card = new qijiCard;
            card->setUserString(Sanguosha->currentRoomState()->getCurrentCardUsePattern());
            card->addSubcard(originalCard);
            return card;
        }

        const Card *c = Self->tag.value("qiji").value<const Card *>();
        if (c) {
            qijiCard *card = new qijiCard;
            card->setUserString(c->objectName());
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }

    virtual QDialog *getDialog() const {
        return qijiDialog::getInstance("qiji");
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const {
        if (player->getMark("@qiji") > 0) return false;
        if (player->isCardLimited(Sanguosha->cloneCard("nullification"), Card::MethodResponse, true))
            return false;
        return player->getHandcardNum() == 1;
    }
};


class qiji_clear : public TriggerSkill {
public:
    qiji_clear() : TriggerSkill("#qiji_clear") {
        events << EventPhaseChanging;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        foreach(ServerPlayer *p, room->getAlivePlayers()){
            if (p->getMark("@qiji") > 0)
                room->setPlayerMark(p, "@qiji", 0);
        }
        return false;
    }
};



fengshenCard::fengshenCard() {
    will_throw = true;
    mute = true;
}
bool fengshenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (to_select == Self)
        return false;
    else if (targets.isEmpty())
        return Self->inMyAttackRange(to_select);
    else if (targets.length() == 1)
        return Self->distanceTo(targets.first()) <= 1 && Self->distanceTo(to_select) <= 1;
    else
        return Self->distanceTo(to_select) <= 1;
}
void fengshenCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    const Card *card = room->askForCard(effect.to, "Slash", "@fengshen-discard:" + effect.from->objectName());
    if (card == NULL)
        room->damage(DamageStruct("fenshen", effect.from, effect.to));
}

class fengshen : public OneCardViewAsSkill {
public:
    fengshen() : OneCardViewAsSkill("fengshen") {
        filter_pattern = ".|red|.|hand!";
    }


    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("fengshenCard");
    }



    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard != NULL){
            fengshenCard *card = new fengshenCard;
            card->addSubcard(originalCard);
            return card;
        }
        else
            return NULL;
    }
};

class fengsu : public DistanceSkill {
public:
    fengsu() : DistanceSkill("fengsu") {
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        int correct = 0;
        if (from->hasSkill("fengsu"))
            correct = correct - (from->getLostHp());

        if (to->hasSkill("fengsu"))
            correct = correct + to->getLostHp();
        return correct;
    }
};
class fengsuEffect : public TriggerSkill {
public:
    fengsuEffect() : TriggerSkill("#fengsu-effect") {
        events << HpChanged; //<< PostHpReduced << HpRecover
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        room->notifySkillInvoked(player, "fengsu");
        return false;
    }
};

xinshangCard::xinshangCard() {
    mute = true;
}
void xinshangCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    effect.to->drawCards(1);

    const Card *card = room->askForCard(effect.to, ".S", "@xinshang-spadecard:" + effect.from->objectName(), QVariant::fromValue(effect.from), Card::MethodNone, NULL, false, "xinshang", false);
    if (card != NULL){
        room->obtainCard(effect.from, card);
        room->setPlayerFlag(effect.from, "xinshang_effect");
    } else {
        if (effect.from->canDiscard(effect.to, "he")){
            room->throwCard(room->askForCardChosen(effect.from, effect.to, "he", "xinshang", false, Card::MethodDiscard), effect.to, effect.from);
            if (effect.from->canDiscard(effect.to, "he"))
                room->throwCard(room->askForCardChosen(effect.from, effect.to, "he", "xinshang", false, Card::MethodDiscard), effect.to, effect.from);
        }
    }
}



class xinshang : public ZeroCardViewAsSkill {
public:
    xinshang() : ZeroCardViewAsSkill("xinshang") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("xinshangCard");
    }

    virtual const Card *viewAs() const{
        return new xinshangCard;
    }
};

class xinshang_effect : public TargetModSkill {
public:
    xinshang_effect() : TargetModSkill("#xinshang_effect") {
        pattern = "BasicCard,TrickCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (from->hasFlag("xinshang_effect"))
            return 1000;
        else
            return 0;
    }

    virtual int getResidueNum(const Player *from, const Card *card) const{
        if (from->hasFlag("xinshang_effect"))
            return 1000;
        else
            return 0;
    }
};

class micai : public TriggerSkill {
public:
    micai() : TriggerSkill("micai") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        DamageStruct damage = data.value<DamageStruct>();
        int num = player->getHandcardNum();

        if (num == 0){
            room->touhouLogmessage("#micai01", player, "micai", QList<ServerPlayer *>(), QString::number(damage.damage - num));
            room->notifySkillInvoked(player, objectName());

            return true;
        }
        else if (damage.damage > num) {
            room->touhouLogmessage("#micai01", player, "micai", QList<ServerPlayer *>(), QString::number(damage.damage - num));
            damage.damage = num;
            room->notifySkillInvoked(player, objectName());
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};


zaihuoCard::zaihuoCard() {
    mute = true;
    will_throw = false;
    //handling_method = Card::MethodNone;
}
void zaihuoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    room->showCard(source, subcards.first());
    room->obtainCard(target, subcards.first(), true);

    room->getThread()->delay();
    if (target->isKongcheng())
        return;
    int card_id = room->askForCardChosen(source, target, "h", "zaihuo");
    room->showCard(target, card_id);
    room->getThread()->delay();

    if (Sanguosha->getCard(subcards.first())->getSuit() != Sanguosha->getCard(card_id)->getSuit())
        room->damage(DamageStruct("zaihuo", NULL, target));
}

class zaihuo : public OneCardViewAsSkill {
public:
    zaihuo() : OneCardViewAsSkill("zaihuo") {
        filter_pattern = ".|.|.|hand";
    }


    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("zaihuoCard");
    }


    virtual const Card *viewAs(const Card *originalCard) const{
        zaihuoCard *card = new zaihuoCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class hina_jie : public TriggerSkill {
public:
    hina_jie() : TriggerSkill("hina_jie") {
        events << Damaged;
        frequency = Frequent;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *hina = room->findPlayerBySkillName(objectName());
        if (damage.to == NULL || damage.to->isDead())
            return false;
        if (hina == NULL || hina->getPhase() != Player::NotActive
            || !hina->inMyAttackRange(damage.to))
            return false;
        QString prompt = "target:" + damage.to->objectName();
        if (room->askForSkillInvoke(hina, objectName(), prompt))
            hina->drawCards(1);
        return false;
    }
};


class changshi : public TriggerSkill {
public:
    changshi() : TriggerSkill("changshi") {
        events << EventPhaseStart << EventLoseSkill << Death << EventPhaseChanging;
        frequency = Eternal;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Start){
            foreach(ServerPlayer *p, room->getOtherPlayers(player))
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
		
			room->touhouLogmessage("#changshi01", player, "changshi");
            room->notifySkillInvoked(player, objectName());
            //for zhengti huashen UI

            //remove card limit,if the source skill can not clear it correctly.
            foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                if (p->hasSkill("zhengti")){
                    Json::Value arg(Json::arrayValue);
                    arg[0] = (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
                    arg[1] = QSanProtocol::Utils::toJsonString(p->objectName());
                    arg[2] = QSanProtocol::Utils::toJsonString(p->getGeneral()->objectName());
                    arg[3] = QSanProtocol::Utils::toJsonString("clear");//QString()
                    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
                }
                room->setPlayerMark(p, "changshi", 1);
                room->filterCards(p, p->getCards("he"), true);

                if (p->getMark("zhouye_limit") > 0){
                    room->setPlayerMark(p, "zhouye_limit", 0);
                    room->removePlayerCardLimitation(p, "use", "Slash$0");
                }
                if (p->getMark("yexing_limit") > 0){
                    room->setPlayerMark(p, "yexing_limit", 0);
                    room->removePlayerCardLimitation(p, "use", "TrickCard+^DelayedTrick$0");
                }

                if (p->getMark("aoyi_limit") > 0){
                    room->removePlayerCardLimitation(p, "use", "TrickCard+^DelayedTrick$0");
                    room->setPlayerMark(p, "aoyi_limit", 0);
                }

            }

            QStringList marks;
            marks << "@an" << "@bian" << "@clock" << "@kinki" << "@qiannian" << "@shi" << "@ye" << "@yu" << "@zhengti"
                << "@huanyue" << "@kuangqi" << "@in_jiejie";
            foreach(ServerPlayer *p, room->getAlivePlayers()){
                //throw cards in special place
                QList<int>  idlist;
                foreach(QString pile, p->getPileNames()) {
                    if (pile == "wooden_ox") continue;
                    foreach(int card, p->getPile(pile))
                        idlist << card;
                }
                if (idlist.length() > 0){
                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, p->objectName(), NULL, objectName(), "");
                    CardsMoveStruct move(idlist, p, Player::DiscardPile,
                        reason);
                    room->moveCardsAtomic(move, true);
                }
                // may be we can deal this before adding changshi mark?
                foreach(QString m, marks){
                    if (p->getMark(m) > 0)
                        p->loseAllMarks(m);
                }

            }
        }
        else if (triggerEvent == Death || triggerEvent == EventPhaseChanging) {

            if (triggerEvent == Death){
                DeathStruct death = data.value<DeathStruct>();
                if (death.who != player)
                    return false;
            }
            if (triggerEvent == EventPhaseChanging){
                PhaseChangeStruct change = data.value<PhaseChangeStruct>();
                if (change.to != Player::NotActive)
                    return false;
            }
            foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                room->setPlayerMark(p, "changshi", 0);
                room->filterCards(p, p->getCards("he"), true);
                if (p->hasSkill("zhouye") && p->getMark("zhouye_limit") == 0){
                    room->setPlayerMark(p, "zhouye_limit", 1);
                    room->setPlayerCardLimitation(p, "use", "Slash", false);
                }
                if (p->hasSkill("yexing") && p->getMark("yexing_limit") == 0){
                    room->setPlayerMark(p, "yexing_limit", 1);
                    room->setPlayerCardLimitation(p, "use", "TrickCard+^DelayedTrick", false);
                }
                if (p->hasSkill("aoyi") && p->getMark("yexing_limit") == 0){
                    room->setPlayerMark(p, "aoyi_limit", 1);
                    room->setPlayerCardLimitation(p, "use", "TrickCard+^DelayedTrick", false);
                }
            }
        }
        return false;
    }
};

class jinian : public TriggerSkill {
public:
    jinian() : TriggerSkill("jinian") {
        events << CardsMoveOneTime << BeforeCardsMove;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == BeforeCardsMove){
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != NULL && move.from_places.contains(Player::PlaceHand)){
                //remain bug : move to DelayedTrick or special  place.
                //&& move.to != Player::PlaceDelayedTrick
                if (!move.from->hasSkill("jinian") || !player->hasSkill("jinian")
                    || move.from->hasFlag("jinian_used"))
                    return false;
                QVariantList h_ids = player->tag["jinian"].toList();

                foreach(int id, move.card_ids){
                    if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceHand)
                        h_ids << id;
                }
                if (move.from->getHandcardNum() <= h_ids.length())
                    player->tag["jinian"] = h_ids;
            }
        }
        if (triggerEvent == CardsMoveOneTime){
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

            if (!player->hasSkill("jinian") || player->hasFlag("jinian_used"))
                return false;
            if (move.from != NULL  && move.origin_to_place == Player::DiscardPile  && move.from == player){

                CardsMoveStruct mo;
                QVariantList ids = player->tag["jinian"].toList();
                player->tag.remove("jinian");

                if (player->isDead())
                    return false;

                QList<int> h_ids;
                foreach(QVariant card_data, ids) {
                    int card_id = card_data.toInt();
                    if (room->getCardPlace(card_id) == Player::DiscardPile)
                        h_ids << card_id;
                }
                if (h_ids.length() > 0 && room->askForSkillInvoke(player, objectName(), data)){
                    player->setFlags("jinian_used");
                    mo.card_ids = h_ids;
                    mo.to = player;
                    mo.to_place = Player::PlaceHand;
                    room->moveCardsAtomic(mo, true);
                }
            }
        }
        return false;
    }
};

class jinian_clear : public TriggerSkill {
public:
    jinian_clear() : TriggerSkill("#jinian_clear") {
        events << EventPhaseChanging;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        foreach(ServerPlayer *p, room->getAlivePlayers()){
            p->tag.remove("jinian");
            if (p->hasFlag("jinian_used"))
                p->setFlags("-jinian_used");
        }
        return false;
    }
};


tianyanCard::tianyanCard() {
    target_fixed = true;
    mute = true;
}
void tianyanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
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


class tianyan : public ZeroCardViewAsSkill {
public:
    tianyan() : ZeroCardViewAsSkill("tianyan") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("tianyanCard");
    }

    virtual const Card *viewAs() const{
        return new tianyanCard;
    }
};


fengrangCard::fengrangCard() {
    target_fixed = true;
    handling_method = Card::MethodUse;
    m_skillName = "fengrang";
    mute = true;
}
const Card *fengrangCard::validate(CardUseStruct &card_use) const{
    AmazingGrace *card = new AmazingGrace(Card::NoSuit, 0);
    card->setSkillName("fengrang");
    return card;
}

class fengrang : public ZeroCardViewAsSkill {
public:
    fengrang() : ZeroCardViewAsSkill("fengrang") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        AmazingGrace *card = new AmazingGrace(Card::NoSuit, 0);
        card->deleteLater();
        return !player->hasUsed("fengrangCard")
            && card->isAvailable(player);
    }

    virtual const Card *viewAs() const{
        return new fengrangCard;
    }
};


class shouhuo : public TriggerSkill {
public:
    shouhuo() : TriggerSkill("shouhuo") {
        events << TrickCardCanceling;
        frequency = Compulsory;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();

        if (effect.to != NULL && effect.to->hasSkill(objectName())
            && effect.card != NULL && effect.card->isKindOf("AmazingGrace")){
            room->notifySkillInvoked(effect.to, objectName());
            return true;
        }
        return false;
    }
};


jiliaoCard::jiliaoCard() {
    mute = true;
}
bool jiliaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty();
}
void jiliaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
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
    if (room->askForSkillInvoke(source, "jiliao", "throwcard:" + target->objectName())){
        int to_throw = room->askForCardChosen(source, target, "h", "jiliao", false, Card::MethodDiscard);
        room->throwCard(to_throw, target, source);
    }
}

class jiliao : public ZeroCardViewAsSkill {
public:
    jiliao() : ZeroCardViewAsSkill("jiliao") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("jiliaoCard");
    }

    virtual const Card *viewAs() const{
        return new jiliaoCard;
    }
};
class zhongyan : public TriggerSkill {
public:
    zhongyan() : TriggerSkill("zhongyan") {
        events << DamageInflicted;
        frequency = Limited;
        limit_mark = "@zhongyan";
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from == NULL || damage.from->isDead() || damage.from == player
            || player->getMark("@zhongyan") == 0)
            return false;

        room->setTag("zhongyan_damage", data);
        int n = qMax(1, damage.from->getLostHp());
        QString prompt = "target:" + damage.from->objectName() + ":" + QString::number(n);
        if (room->askForSkillInvoke(player, objectName(), prompt)) {
            room->removePlayerMark(player, "@zhongyan");
            room->doLightbox("$zhongyanAnimate", 4000);
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());
		
			int x = damage.from->getLostHp();
            room->loseHp(damage.from, qMax(1, x));
            return true;
        }
        return false;
    }
};

dfgzmsiyuCard::dfgzmsiyuCard() {
    mute = true;
    will_throw = false;
}
void dfgzmsiyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    room->obtainCard(target, subcards.first(), false);
    source->setFlags("dfgzmsiyu");
    source->tag["dfgzmsiyu"] = QVariant::fromValue(target);
}
class dfgzmsiyuvs : public OneCardViewAsSkill {
public:
    dfgzmsiyuvs() : OneCardViewAsSkill("dfgzmsiyu") {
        filter_pattern = ".|.|.|hand";
    }


    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("dfgzmsiyuCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard != NULL){
            dfgzmsiyuCard *card = new dfgzmsiyuCard;
            card->addSubcard(originalCard);
            return card;
        }
        else
            return NULL;
    }
};

class dfgzmsiyu : public TriggerSkill {
public:
    dfgzmsiyu() : TriggerSkill("dfgzmsiyu") {
        events << EventPhaseChanging;
        view_as_skill = new dfgzmsiyuvs;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();

        if (change.to == Player::NotActive && player->hasFlag("dfgzmsiyu")){
            player->setFlags("-dfgzmsiyu");
            ServerPlayer *target = player->tag["dfgzmsiyu"].value<ServerPlayer *>();
            player->tag.remove("dfgzmsiyu");
            if (target != NULL && target->isAlive() && !target->isKongcheng()){

                room->touhouLogmessage("#TouhouBuff", player, "dfgzmsiyu");
                room->notifySkillInvoked(player, "dfgzmsiyu");
                //room->fillAG(target->handCards(),player);
                //int id=room->askForAG(player,target->handCards(),false,"dfgzmsiyu");
                //room->clearAG(player);
                int id = room->askForCardChosen(player, target, "h", objectName(), true);
                if (id > -1)
                    room->obtainCard(player, id, false);
            }
        }
        return false;
    }
};

//for Collateral
ExtraCollateralCard::ExtraCollateralCard() {
}

bool ExtraCollateralCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    const Card *coll = Card::Parse(Self->property("extra_collateral").toString());
    if (!coll) return false;
    QStringList tos = Self->property("extra_collateral_current_targets").toString().split("+");

    if (targets.isEmpty())
        return !tos.contains(to_select->objectName())
        && !Self->isProhibited(to_select, coll) && coll->targetFilter(targets, to_select, Self);
    else
        return coll->targetFilter(targets, to_select, Self);
}

void ExtraCollateralCard::onUse(Room *, const CardUseStruct &card_use) const{
    Q_ASSERT(card_use.to.length() == 2);
    ServerPlayer *killer = card_use.to.first();
    ServerPlayer *victim = card_use.to.last();
    killer->setFlags("ExtraCollateralTarget");
    killer->tag["collateralVictim"] = QVariant::fromValue((PlayerStar)victim);
}


class qishuvs : public ZeroCardViewAsSkill {
public:
    qishuvs() : ZeroCardViewAsSkill("qishu") {
        response_pattern = "@@qishu";
    }


    virtual const Card *viewAs() const{
        return new ExtraCollateralCard;
    }
};
class qishuMod : public TargetModSkill {
public:
    qishuMod() : TargetModSkill("#qishu-mod") {
        pattern = "Slash,TrickCard+^DelayedTrick";
    }
    static bool isLastHandCard(const Player *player, const Card *card){
        QList<int> subcards = card->getSubcards();
        if (subcards.length() == 0 || player->isKongcheng())
            return false;
        QList<int> wood_ox = player->getPile("wooden_ox");
        int wood_num = 0;
        foreach(int id, subcards){
            if (wood_ox.contains(id))
                wood_num++;
        }
        if (subcards.length() - wood_num == player->getHandcardNum())
            return true;
        else
            return false;
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (from->hasSkill("qishu") && isLastHandCard(from, card))
            return 1000;
        else
            return 0;
    }


    virtual int getExtraTargetNum(const Player *player, const Card *card) const{
        if (player->hasSkill("qishu") && player->getPhase() == Player::Play && isLastHandCard(player, card))
            return 1000;
        else
            return 0;
    }
};
class qishu : public TriggerSkill {
public:
    qishu() : TriggerSkill("qishu") {
        events << PreCardUsed;
        view_as_skill = new  qishuvs;
    }
    static bool isQishu(QList<ServerPlayer *>players, const Card *card){
        if (card->isKindOf("GlobalEffect") || card->isKindOf("AOE"))
            return false;
        if (card->isKindOf("IronChain"))
            return players.length() > 2;
        return true;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        if (triggerEvent == PreCardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            bool  extra_col = false;
            //process Collateral
            if (qishuMod::isLastHandCard(player, use.card) && use.card->isKindOf("Collateral")){
                while (true){
                    ServerPlayer *extra = NULL;
                    QStringList tos;
                    foreach(ServerPlayer *t, use.to)
                        tos.append(t->objectName());
                    room->setPlayerProperty(player, "extra_collateral", use.card->toString());
                    room->setPlayerProperty(player, "extra_collateral_current_targets", tos.join("+"));
                    room->askForUseCard(player, "@@qishu", "@qishu-add:::collateral");
                    room->setPlayerProperty(player, "extra_collateral", QString());
                    room->setPlayerProperty(player, "extra_collateral_current_targets", QString("+"));
                    foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
                        if (p->hasFlag("ExtraCollateralTarget")) {
                            p->setFlags("-ExtraCollateralTarget");
                            extra = p;
                            break;
                        }
                    }
                    if (extra != NULL){
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

                        ServerPlayer *victim = extra->tag["collateralVictim"].value<PlayerStar>();
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
            if (use.card->isKindOf("Collateral")){
                if (extra_col)
                    room->notifySkillInvoked(player, "qishu");
            }
            else if (use.to.length() > 1 && qishuMod::isLastHandCard(player, use.card)) {
                if (use.card->isKindOf("Slash") || use.card->isNDTrick()){

                    //sanguosha:correctCardTarget
                    if (isQishu(use.to, use.card))
                        room->notifySkillInvoked(player, "qishu");

                }
            }
        }
        return false;
    }
};

th10Package::th10Package()
    : Package("th10")
{

    General *fsl001 = new General(this, "fsl001$", "fsl", 4, false);
    fsl001->addSkill(new shende);
    fsl001->addSkill(new qiankun);
    fsl001->addSkill(new gongfeng);

    General *fsl002 = new General(this, "fsl002", "fsl", 3, false);
    fsl002->addSkill(new bushu);
    fsl002->addSkill("qiankun");
    fsl002->addSkill(new chuancheng);

    General *fsl003 = new General(this, "fsl003", "fsl", 3, false);
    fsl003->addSkill(new zhunbei);
    fsl003->addSkill(new zhunbei_effect);
    fsl003->addSkill(new qiji);
    fsl003->addSkill(new qiji_clear);
    related_skills.insertMulti("zhunbei", "#zhunbei_effect");
    related_skills.insertMulti("qiji", "#qiji_clear");


    General *fsl004 = new General(this, "fsl004", "fsl", 3, false);
    fsl004->addSkill(new fengshen);
    fsl004->addSkill(new fengsu);
    fsl004->addSkill(new fengsuEffect);
    related_skills.insertMulti("fengsu", "#fengsu-effect");

    General *fsl005 = new General(this, "fsl005", "fsl", 3, false);
    fsl005->addSkill(new xinshang);
    fsl005->addSkill(new xinshang_effect);
    fsl005->addSkill(new micai);
    related_skills.insertMulti("xinshang", "#xinshang_effect");

    General *fsl006 = new General(this, "fsl006", "fsl", 3, false);
    fsl006->addSkill(new zaihuo);
    fsl006->addSkill(new hina_jie);

    General *fsl007 = new General(this, "fsl007", "fsl", 3, false);
    fsl007->addSkill(new changshi);
    fsl007->addSkill(new jinian);
    fsl007->addSkill(new jinian_clear);
    related_skills.insertMulti("jinian", "#jinian_clear");


    General *fsl008 = new General(this, "fsl008", "fsl", 4, false);
    fsl008->addSkill(new tianyan);

    General *fsl009 = new General(this, "fsl009", "fsl", 4, false);
    fsl009->addSkill(new fengrang);
    fsl009->addSkill(new shouhuo);


    General *fsl010 = new General(this, "fsl010", "fsl", 4, false);
    fsl010->addSkill(new jiliao);
    fsl010->addSkill(new zhongyan);

    General *fsl011 = new General(this, "fsl011", "fsl", 4, false);
    fsl011->addSkill(new dfgzmsiyu);
    fsl011->addSkill(new qishu);
    fsl011->addSkill(new qishuMod);
    related_skills.insertMulti("qishu", "#qishu-mod");

    addMetaObject<shendeDummyCard>();
    addMetaObject<shendeFakeMoveCard>();
    addMetaObject<gongfengCard>();
    addMetaObject<qijiCard>();
    addMetaObject<fengshenCard>();
    addMetaObject<xinshangCard>();
    addMetaObject<zaihuoCard>();
    addMetaObject<tianyanCard>();
    addMetaObject<fengrangCard>();
    addMetaObject<jiliaoCard>();
    addMetaObject<dfgzmsiyuCard>();
    addMetaObject<ExtraCollateralCard>();
    skills << new gongfengvs;
}

ADD_PACKAGE(th10)

