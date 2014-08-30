#include "sp-package.h"
#include "client.h"
#include "general.h"
#include "skill.h"
#include "standard-skillcards.h"
#include "engine.h"
#include "maneuvering.h"
#include "settings.h"
#include "ai.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCommandLinkButton>


class SPMoonSpearSkill: public WeaponSkill {
public:
    SPMoonSpearSkill(): WeaponSkill("SPMoonSpear") {
        events << CardUsed << CardResponded;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::NotActive)
            return false;

        CardStar card = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct card_use = data.value<CardUseStruct>();
            card = card_use.card;
        } else if (triggerEvent == CardResponded) {
            card = data.value<CardResponseStruct>().m_card;
        }

        if (card == NULL || !card->isBlack()
            || (card->getHandlingMethod() != Card::MethodUse && card->getHandlingMethod() != Card::MethodResponse))
            return false;

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *tmp, room->getOtherPlayers(player)) {
            if (player->inMyAttackRange(tmp))
                targets << tmp;
        }
        if (targets.isEmpty()) return false;

        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@sp_moonspear", true, true);
        if (!target) return false;
        room->setEmotion(player, "weapon/moonspear");
        if (!room->askForCard(target, "jink", "@moon-spear-jink", QVariant(), Card::MethodResponse, player))
            room->damage(DamageStruct(objectName(), player, target));
        return false;
    }
};

SPMoonSpear::SPMoonSpear(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("SPMoonSpear");
}

class Jilei: public TriggerSkill {
public:
    Jilei(): TriggerSkill("jilei") {
        events << Damaged;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *yangxiu, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *current = room->getCurrent();
        if (!current || current->getPhase() == Player::NotActive || current->isDead() || !damage.from)
            return false;

        if (room->askForSkillInvoke(yangxiu, objectName(), data)) {
            QString choice = room->askForChoice(yangxiu, objectName(), "BasicCard+EquipCard+TrickCard");
            room->broadcastSkillInvoke(objectName());

            LogMessage log;
            log.type = "#Jilei";
            log.from = damage.from;
            log.arg = choice;
            room->sendLog(log);

            QStringList jilei_list = damage.from->tag[objectName()].toStringList();
            if (jilei_list.contains(choice)) return false;
            jilei_list.append(choice);
            damage.from->tag[objectName()] = QVariant::fromValue(jilei_list);
            QString _type = choice + "|.|.|hand"; // Handcards only
            room->setPlayerCardLimitation(damage.from, "use,response,discard", _type, true);

            QString type_name = choice.replace("Card", "").toLower();
            if (damage.from->getMark("@jilei_" + type_name) == 0)
                room->addPlayerMark(damage.from, "@jilei_" + type_name);
        }

        return false;
    }
};

class JileiClear: public TriggerSkill {
public:
    JileiClear(): TriggerSkill("#jilei-clear") {
        events << EventPhaseChanging << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != target || target != room->getCurrent())
                return false;
        }
        QList<ServerPlayer *> players = room->getAllPlayers();
        foreach (ServerPlayer *player, players) {
            QStringList jilei_list = player->tag["jilei"].toStringList();
            if (!jilei_list.isEmpty()) {
                LogMessage log;
                log.type = "#JileiClear";
                log.from = player;
                room->sendLog(log);

                foreach (QString jilei_type, jilei_list) {
                    room->removePlayerCardLimitation(player, "use,response,discard", jilei_type + "|.|.|hand$1");
                    QString type_name = jilei_type.replace("Card", "").toLower();
                    room->setPlayerMark(player, "@jilei_" + type_name, 0);
                }
                player->tag.remove("jilei");
            }
        }

        return false;
    }
};

class Danlao: public TriggerSkill {
public:
    Danlao(): TriggerSkill("danlao") {
        events << TargetConfirmed << CardEffected;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.to.length() <= 1 || !use.to.contains(player)
                || !use.card->isKindOf("TrickCard")
                || !room->askForSkillInvoke(player, objectName(), data))
                return false;

            player->tag["Danlao"] = use.card->toString();
            room->broadcastSkillInvoke(objectName());

            player->drawCards(1);
        } else {
            if (!player->isAlive() || !player->hasSkill(objectName()))
                return false;

            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (player->tag["Danlao"].isNull() || player->tag["Danlao"].toString() != effect.card->toString())
                return false;

            player->tag["Danlao"] = QVariant(QString());

            LogMessage log;
            log.type = "#DanlaoAvoid";
            log.from = player;
            log.arg = effect.card->objectName();
            log.arg2 = objectName();
            room->sendLog(log);

            return true;
        }

        return false;
    }
};

Yongsi::Yongsi(): TriggerSkill("yongsi") {
    events << DrawNCards << EventPhaseStart;
    frequency = Compulsory;
}

int Yongsi::getKingdoms(ServerPlayer *yuanshu) const{
    QSet<QString> kingdom_set;
    Room *room = yuanshu->getRoom();
    foreach (ServerPlayer *p, room->getAlivePlayers())
        kingdom_set << p->getKingdom();

    return kingdom_set.size();
}

bool Yongsi::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *yuanshu, QVariant &data) const{
    if (triggerEvent == DrawNCards) {
        int x = getKingdoms(yuanshu);
        data = data.toInt() + x;

        Room *room = yuanshu->getRoom();
        LogMessage log;
        log.type = "#YongsiGood";
        log.from = yuanshu;
        log.arg = QString::number(x);
        log.arg2 = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(yuanshu, objectName());

        room->broadcastSkillInvoke("yongsi");
    } else if (triggerEvent == EventPhaseStart && yuanshu->getPhase() == Player::Discard) {
        int x = getKingdoms(yuanshu);
        LogMessage log;
        log.type = yuanshu->getCardCount(true) > x ? "#YongsiBad" : "#YongsiWorst";
        log.from = yuanshu;
        log.arg = QString::number(log.type == "#YongsiBad" ? x : yuanshu->getCardCount(true));
        log.arg2 = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(yuanshu, objectName());
        if (x > 0){
            room->broadcastSkillInvoke("yongsi");
            room->askForDiscard(yuanshu, "yongsi", x, x, false, true);
        }
    }

    return false;
}

class WeidiViewAsSkill: public ViewAsSkill {
public:
    WeidiViewAsSkill(): ViewAsSkill("weidi") {
    }

    static QList<const ViewAsSkill *> getLordViewAsSkills(const Player *player) {
        const Player *lord = NULL;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->isLord()) {
                lord = p;
                break;
            }
        }
        if (!lord) return QList<const ViewAsSkill *>();

        QList<const ViewAsSkill *> vs_skills;
        foreach (const Skill *skill, lord->getVisibleSkillList()) {
            if (skill->isLordSkill() && player->hasLordSkill(skill->objectName())) {
                const ViewAsSkill *vs = ViewAsSkill::parseViewAsSkill(skill);
                if (vs)
                    vs_skills << vs;
            }
        }
        return vs_skills;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        QList<const ViewAsSkill *> vs_skills = getLordViewAsSkills(player);
        foreach (const ViewAsSkill *skill, vs_skills) {
            if (skill->isEnabledAtPlay(player))
                return true;
        }
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        QList<const ViewAsSkill *> vs_skills = getLordViewAsSkills(player);
        foreach (const ViewAsSkill *skill, vs_skills) {
            if (skill->isEnabledAtResponse(player, pattern))
                return true;
        }
        return false;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        QList<const ViewAsSkill *> vs_skills = getLordViewAsSkills(player);
        foreach (const ViewAsSkill *skill, vs_skills) {
            if (skill->isEnabledAtNullification(player))
                return true;
        }
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        QString skill_name = Self->tag["weidi"].toString();
        if (skill_name.isEmpty()) return false;
        const ViewAsSkill *vs_skill = Sanguosha->getViewAsSkill(skill_name);
        if (vs_skill) return vs_skill->viewFilter(selected, to_select);
        return false;	
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        QString skill_name = Self->tag["weidi"].toString();
        if (skill_name.isEmpty()) return NULL;
        const ViewAsSkill *vs_skill = Sanguosha->getViewAsSkill(skill_name);
        if (vs_skill) return vs_skill->viewAs(cards);
        return NULL;
    }
};

WeidiDialog *WeidiDialog::getInstance() {
    static WeidiDialog *instance;
    if (instance == NULL)
        instance = new WeidiDialog();

    return instance;
}

WeidiDialog::WeidiDialog() {
    setObjectName("weidi");
    setWindowTitle(Sanguosha->translate("weidi"));
    group = new QButtonGroup(this);

    button_layout = new QVBoxLayout;
    setLayout(button_layout);
    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectSkill(QAbstractButton *)));
}

void WeidiDialog::popup() {
    Self->tag.remove(objectName());
    foreach (QAbstractButton *button, group->buttons()) {
        button_layout->removeWidget(button);
        group->removeButton(button);
        delete button;
    }

    QList<const ViewAsSkill *> vs_skills = WeidiViewAsSkill::getLordViewAsSkills(Self);
    int count = 0;
    QString name;
    foreach (const ViewAsSkill *skill, vs_skills) {
        QAbstractButton *button = createSkillButton(skill->objectName());
        button->setEnabled(skill->isAvailable(Self, Sanguosha->currentRoomState()->getCurrentCardUseReason(),
            Sanguosha->currentRoomState()->getCurrentCardUsePattern()));
        if (button->isEnabled()) {
            count++;
            name = skill->objectName();
        }
        button_layout->addWidget(button);
    }

    if (count == 0) {
        emit onButtonClick();
        return;
    } else if (count == 1) {
        Self->tag[objectName()] = name;
        emit onButtonClick();
        return;
    }

    exec();
}

void WeidiDialog::selectSkill(QAbstractButton *button) {
    Self->tag[objectName()] = button->objectName();
    emit onButtonClick();
    accept();
}

QAbstractButton *WeidiDialog::createSkillButton(const QString &skill_name) {
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (!skill) return NULL;

    QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate(skill_name));
    button->setObjectName(skill_name);
    button->setToolTip(skill->getDescription());

    group->addButton(button);
    return button;
}

class Weidi: public GameStartSkill {
public:
    Weidi(): GameStartSkill("weidi") {
        frequency = Compulsory;
        view_as_skill = new WeidiViewAsSkill;
    }

    virtual void onGameStart(ServerPlayer *) const{
        return;
    }

    virtual QDialog *getDialog() const{
        return WeidiDialog::getInstance();
    }
};

class Yicong: public DistanceSkill {
public:
    Yicong(): DistanceSkill("yicong") {
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        int correct = 0;
        if (from->hasSkill(objectName()) && from->getHp() > 2)
            correct--;
        if (to->hasSkill(objectName()) && to->getHp() <= 2)
            correct++;

        return correct;
    }
};

class YicongEffect: public TriggerSkill {
public:
    YicongEffect(): TriggerSkill("#yicong-effect") {
        events << PostHpReduced << HpRecover;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        int hp = player->getHp();
        int index = 0;
        if (triggerEvent == HpRecover) {
            RecoverStruct recover = data.value<RecoverStruct>();
            if (hp > 2 && hp - recover.recover <= 2)
                index = 1;
        } else if (triggerEvent == PostHpReduced) {
            int reduce = 0;
            if (data.canConvert<DamageStruct>()) {
                DamageStruct damage = data.value<DamageStruct>();
                reduce = damage.damage;
            } else
                reduce = data.toInt();
            if (hp <= 2 && hp + reduce > 2)
                index = 2;
        }

        if (index > 0)
            room->broadcastSkillInvoke("yicong", index);
        return false;
    }
};

class Danji: public PhaseChangeSkill {
public:
    Danji(): PhaseChangeSkill("danji") { // What a silly skill!
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->getMark("danji") == 0
               && target->getHandcardNum() > target->getHp();
    }

    virtual bool onPhaseChange(ServerPlayer *guanyu) const{
        Room *room = guanyu->getRoom();
        ServerPlayer *the_lord = room->getLord();
        if (the_lord && (the_lord->getGeneralName() == "caocao" || the_lord->getGeneral2Name() == "caocao")) {
            room->notifySkillInvoked(guanyu, objectName());

            LogMessage log;
            log.type = "#DanjiWake";
            log.from = guanyu;
            log.arg = QString::number(guanyu->getHandcardNum());
            log.arg2 = QString::number(guanyu->getHp());
            room->sendLog(log);
            room->broadcastSkillInvoke(objectName());
            room->doLightbox("$DanjiAnimate", 5000);

            room->addPlayerMark(guanyu, "danji");
            if (room->changeMaxHpForAwakenSkill(guanyu))
                room->acquireSkill(guanyu, "mashu");
        }

        return false;
    }
};

YuanhuCard::YuanhuCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool YuanhuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    if (!targets.isEmpty())
        return false;

    const Card *card = Sanguosha->getCard(subcards.first());
    const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
    int equip_index = static_cast<int>(equip->location());
    return to_select->getEquip(equip_index) == NULL;
}

void YuanhuCard::onUse(Room *room, const CardUseStruct &card_use) const{
    int index = -1;
    if (card_use.to.first() == card_use.from)
        index = 5;
    else if (card_use.to.first()->getGeneralName().contains("caocao"))
        index = 4;
    else {
        const Card *card = Sanguosha->getCard(card_use.card->getSubcards().first());
        if (card->isKindOf("Weapon"))
            index = 1;
        else if (card->isKindOf("Armor"))
            index = 2;
        else if (card->isKindOf("Horse"))
            index = 3;
    }
    room->broadcastSkillInvoke("yuanhu", index);
    SkillCard::onUse(room, card_use);
}

void YuanhuCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *caohong = effect.from;
    Room *room = caohong->getRoom();
    room->moveCardTo(this, caohong, effect.to, Player::PlaceEquip,
                     CardMoveReason(CardMoveReason::S_REASON_PUT, caohong->objectName(), "yuanhu", QString()));

    const Card *card = Sanguosha->getCard(subcards.first());

    LogMessage log;
    log.type = "$ZhijianEquip";
    log.from = effect.to;
    log.card_str = QString::number(card->getEffectiveId());
    room->sendLog(log);

    if (card->isKindOf("Weapon")) {
      QList<ServerPlayer *> targets;
      foreach (ServerPlayer *p, room->getAllPlayers()) {
          if (effect.to->distanceTo(p) == 1 && caohong->canDiscard(p, "hej"))
              targets << p;
      }
      if (!targets.isEmpty()) {
          ServerPlayer *to_dismantle = room->askForPlayerChosen(caohong, targets, "yuanhu", "@yuanhu-discard:" + effect.to->objectName());
          int card_id = room->askForCardChosen(caohong, to_dismantle, "hej", "yuanhu", false, Card::MethodDiscard);
          room->throwCard(Sanguosha->getCard(card_id), to_dismantle, caohong);
      }
    } else if (card->isKindOf("Armor")) {
        effect.to->drawCards(1);
    } else if (card->isKindOf("Horse")) {
        RecoverStruct recover;
        recover.who = effect.from;
        room->recover(effect.to, recover);
    }
}

class YuanhuViewAsSkill: public OneCardViewAsSkill {
public:
    YuanhuViewAsSkill(): OneCardViewAsSkill("yuanhu") {
        filter_pattern = "EquipCard";
        response_pattern = "@@yuanhu";
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        YuanhuCard *first = new YuanhuCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Yuanhu: public PhaseChangeSkill {
public:
    Yuanhu(): PhaseChangeSkill("yuanhu") {
        view_as_skill = new YuanhuViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        if (target->getPhase() == Player::Finish && !target->isNude())
            room->askForUseCard(target, "@@yuanhu", "@yuanhu-equip", -1, Card::MethodNone);
        return false;
    }
};

XuejiCard::XuejiCard() {
}

bool XuejiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (targets.length() >= Self->getLostHp())
        return false;

    if (to_select == Self)
        return false;

    int range_fix = 0;
    if (Self->getWeapon() && Self->getWeapon()->getEffectiveId() == getEffectiveId()) {
        const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        range_fix += weapon->getRange() - Self->getAttackRange(false);
    } else if (Self->getOffensiveHorse() && Self->getOffensiveHorse()->getEffectiveId() == getEffectiveId())
        range_fix += 1;

    return Self->distanceTo(to_select, range_fix) <= Self->getAttackRange();
}

void XuejiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    DamageStruct damage;
    damage.from = source;
    damage.reason = "xueji";

    foreach (ServerPlayer *p, targets) {
        damage.to = p;
        room->damage(damage);
    }
    foreach (ServerPlayer *p, targets) {
        if (p->isAlive())
            p->drawCards(1);
    }
}

class Xueji: public OneCardViewAsSkill {
public:
    Xueji(): OneCardViewAsSkill("xueji") {
        filter_pattern = ".|red!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getLostHp() > 0 && player->canDiscard(player, "he") && !player->hasUsed("XuejiCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        XuejiCard *first = new XuejiCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Huxiao: public TargetModSkill {
public:
    Huxiao(): TargetModSkill("huxiao") {
    }

    virtual int getResidueNum(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()))
            return from->getMark(objectName());
        else
            return 0;
    }
};

class HuxiaoCount: public TriggerSkill {
public:
    HuxiaoCount(): TriggerSkill("#huxiao-count") {
        events << SlashMissed << EventPhaseChanging;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == SlashMissed) {
            if (player->getPhase() == Player::Play)
                room->addPlayerMark(player, "huxiao");
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                if (player->getMark("huxiao") > 0)
                    room->setPlayerMark(player, "huxiao", 0);
        }

        return false;
    }
};

class HuxiaoClear: public DetachEffectSkill {
public:
    HuxiaoClear(): DetachEffectSkill("huxiao") {
    }

    virtual void onSkillDetached(Room *room, ServerPlayer *player) const{
        room->setPlayerMark(player, "huxiao", 0);
    }
};

class WujiCount: public TriggerSkill {
public:
    WujiCount(): TriggerSkill("#wuji-count") {
        events << PreDamageDone << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive() && damage.from == room->getCurrent() && damage.from->getMark("wuji") == 0)
                room->addPlayerMark(damage.from, "wuji_damage", damage.damage);
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                if (player->getMark("wuji_damage") > 0)
                    room->setPlayerMark(player, "wuji_damage", 0);
        }

        return false;
    }
};

class Wuji: public PhaseChangeSkill {
public:
    Wuji(): PhaseChangeSkill("wuji") {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Finish
               && target->getMark("wuji") == 0
               && target->getMark("wuji_damage") >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#WujiWake";
        log.from = player;
        log.arg = QString::number(player->getMark("wuji_damage"));
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());
        room->doLightbox("$WujiAnimate", 4000);

        room->addPlayerMark(player, "wuji");

        if (room->changeMaxHpForAwakenSkill(player, 1)) {
            RecoverStruct recover;
            recover.who = player;
            room->recover(player, recover);

            room->detachSkillFromPlayer(player, "huxiao");
        }

        return false;
    }
};


class Tianming: public TriggerSkill {
public:
    Tianming(): TriggerSkill("tianming") {
        events << TargetConfirming;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && room->askForSkillInvoke(player, objectName())) {
            room->broadcastSkillInvoke(objectName(), 1);
            room->askForDiscard(player, objectName(), 2, 2, false, true);
            player->drawCards(2);

            int max = -1000;
            foreach (ServerPlayer *p, room->getAllPlayers())
                if (p->getHp() > max)
                    max = p->getHp();
            if (player->getHp() == max)
                return false;

            QList<ServerPlayer *> maxs;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->getHp() == max)
                    maxs << p;
                if (maxs.size() > 1)
                    return false;
            }
            ServerPlayer *mosthp = maxs.first();
            if (room->askForSkillInvoke(mosthp, objectName())) {
                room->broadcastSkillInvoke(objectName(), 2);
                room->askForDiscard(mosthp, objectName(), 2, 2, false, true);
                mosthp->drawCards(2);
            }
        }

        return false;
    }
};

MizhaoCard::MizhaoCard() {
    will_throw = false;
    mute = true;
    handling_method = Card::MethodNone;
}

bool MizhaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self;
}

void MizhaoCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->obtainCard(effect.card, false);
    if (effect.to->isKongcheng()) return;

    Room *room = effect.from->getRoom();
    room->broadcastSkillInvoke("mizhao");

    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getOtherPlayers(effect.to))
        if (!p->isKongcheng())
            targets << p;

    if (!targets.isEmpty()) {
        ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "mizhao", "@mizhao-pindian:" + effect.to->objectName());
        target->setFlags("MizhaoPindianTarget");
        effect.to->pindian(target, "mizhao", NULL);
        target->setFlags("-MizhaoPindianTarget");
    }
}

class MizhaoViewAsSkill: public ViewAsSkill {
public:
    MizhaoViewAsSkill(): ViewAsSkill("mizhao") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("MizhaoCard");
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() < Self->getHandcardNum())
            return NULL;

        MizhaoCard *card = new MizhaoCard;
        card->addSubcards(cards);
        return card;
    }
};

class Mizhao: public TriggerSkill {
public:
    Mizhao(): TriggerSkill("mizhao") {
        events << Pindian;
        view_as_skill = new MizhaoViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual int getPriority(TriggerEvent) const{
        return -1;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        PindianStar pindian = data.value<PindianStar>();
        if (pindian->reason != objectName() || pindian->from_number == pindian->to_number)
            return false;

        ServerPlayer *winner = pindian->isSuccess() ? pindian->from : pindian->to;
        ServerPlayer *loser = pindian->isSuccess() ? pindian->to : pindian->from;
        if (winner->canSlash(loser, NULL, false)) {
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_mizhao");
            room->useCard(CardUseStruct(slash, winner, loser), false);
        }

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return -2;
    }
};

class MizhaoSlashNoDistanceLimit: public TargetModSkill {
public:
    MizhaoSlashNoDistanceLimit(): TargetModSkill("#mizhao-slash-ndl") {
    }

    virtual int getDistanceLimit(const Player *, const Card *card) const{
        if (card->isKindOf("Slash") && card->getSkillName() == "mizhao")
            return 1000;
        else
            return 0;
    }
};

class Jieyuan: public TriggerSkill {
public:
    Jieyuan(): TriggerSkill("jieyuan") {
        events << DamageCaused << DamageInflicted;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageCaused) {
            if (damage.to && damage.to->isAlive()
                && damage.to->getHp() >= player->getHp() && damage.to != player && player->canDiscard(player, "h")
                && room->askForCard(player, ".black", "@jieyuan-increase:" + damage.to->objectName(), data, objectName())) {
                    room->broadcastSkillInvoke(objectName(), 1);

                    LogMessage log;
                    log.type = "#JieyuanIncrease";
                    log.from = player;
                    log.arg = QString::number(damage.damage);
                    log.arg2 = QString::number(++damage.damage);
                    room->sendLog(log);

                    data = QVariant::fromValue(damage);
            }
        } else if (triggerEvent == DamageInflicted) {
            if (damage.from && damage.from->isAlive()
                && damage.from->getHp() >= player->getHp() && damage.from != player && player->canDiscard(player, "h")
                && room->askForCard(player, ".red", "@jieyuan-decrease:" + damage.from->objectName(), data, objectName())) {
                    room->broadcastSkillInvoke(objectName(), 2);

                    LogMessage log;
                    log.type = "#JieyuanDecrease";
                    log.from = player;
                    log.arg = QString::number(damage.damage);
                    log.arg2 = QString::number(--damage.damage);
                    room->sendLog(log);

                    data = QVariant::fromValue(damage);
                    if (damage.damage < 1)
                        return true;
            }
        }

        return false;
    }
};

class Fenxin: public TriggerSkill {
public:
    Fenxin(): TriggerSkill("fenxin") {
        events << BeforeGameOverJudge;
        frequency = Limited;
        limit_mark = "@burnheart";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        if (!isNormalGameMode(room->getMode()))
            return false;
        DeathStruct death = data.value<DeathStruct>();
        if (death.damage == NULL)
            return false;
        ServerPlayer *killer = death.damage->from;
        if (killer == NULL || killer->isLord() || player->isLord() || player->getHp() > 0)
            return false;
        if (!TriggerSkill::triggerable(killer) || killer->getMark("@burnheart") == 0)
            return false;
        player->setFlags("FenxinTarget");
        bool invoke = room->askForSkillInvoke(killer, objectName(), QVariant::fromValue(player));
        player->setFlags("-FenxinTarget");
        if (invoke) {
            room->broadcastSkillInvoke(objectName());
            room->doLightbox("$FenxinAnimate");
            room->removePlayerMark(killer, "@burnheart");
            QString role1 = killer->getRole();
            killer->setRole(player->getRole());
            room->notifyProperty(killer, killer, "role", player->getRole());
            room->setPlayerProperty(player, "role", role1);
        }
        return false;
    }
};


class Moukui: public TriggerSkill {
public:
    Moukui(): TriggerSkill("moukui") {
        events << TargetConfirmed << SlashMissed << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (player != use.from || !TriggerSkill::triggerable(player) || !use.card->isKindOf("Slash"))
                return false;
            foreach (ServerPlayer *p, use.to) {
                if (player->askForSkillInvoke(objectName(), QVariant::fromValue(p))) {
                    QString choice;
                    if (!player->canDiscard(p, "he"))
                        choice = "draw";
                    else
                        choice = room->askForChoice(player, objectName(), "draw+discard", QVariant::fromValue(p));
                    if (choice == "draw") {
                        room->broadcastSkillInvoke(objectName(), 1);
                        player->drawCards(1);
                    } else {
                        room->broadcastSkillInvoke(objectName(), 1);
                        int disc = room->askForCardChosen(player, p, "he", objectName(), false, Card::MethodDiscard);
                        room->throwCard(disc, p, player);
                    }
                    room->addPlayerMark(p, objectName() + use.card->toString());
                }
            }
        } else if (triggerEvent == SlashMissed) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.to->isDead() || effect.to->getMark(objectName() + effect.slash->toString()) <= 0)
                return false;
            if (!effect.from->isAlive() || !effect.to->isAlive() || !effect.to->canDiscard(effect.from, "he"))
                return false;
            int disc = room->askForCardChosen(effect.to, effect.from, "he", objectName(), false, Card::MethodDiscard);
            room->broadcastSkillInvoke(objectName(), 2);
            room->throwCard(disc, effect.from, effect.to);
            room->removePlayerMark(effect.to, objectName() + effect.slash->toString());
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return false;
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->setPlayerMark(p, objectName() + use.card->toString(), 0);
        }

        return false;
    }
};

class Baobian: public TriggerSkill {
public:
    Baobian(): TriggerSkill("baobian") {
        events << GameStart << HpChanged << MaxHpChanged << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventLoseSkill) {
            if (data.toString() == objectName()) {
                QStringList baobian_skills = player->tag["BaobianSkills"].toStringList();
                QStringList detachList;
                foreach (QString skill_name, baobian_skills)
                    detachList.append("-" + skill_name);
                room->handleAcquireDetachSkills(player, detachList);
                player->tag["BaobianSkills"] = QVariant();
            }
            return false;
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != objectName()) return false;
        }

        if (!player->isAlive() || !player->hasSkill(objectName(), true)) return false;

        acquired_skills.clear();
        detached_skills.clear();
        BaobianChange(room, player, 1, "shensu");
        BaobianChange(room, player, 2, "paoxiao");
        BaobianChange(room, player, 3, "tiaoxin");
        if (!acquired_skills.isEmpty() || !detached_skills.isEmpty())
            room->handleAcquireDetachSkills(player, acquired_skills + detached_skills);
        return false;
    }


private:
    void BaobianChange(Room *room, ServerPlayer *player, int hp, const QString &skill_name) const{
        QStringList baobian_skills = player->tag["BaobianSkills"].toStringList();
        if (player->getHp() <= hp) {
            if (!baobian_skills.contains(skill_name)) {
                room->notifySkillInvoked(player, "baobian");
                acquired_skills.append(skill_name);
                baobian_skills << skill_name;
            }
        } else {
            if (baobian_skills.contains(skill_name)) {
                detached_skills.append("-" + skill_name);
                baobian_skills.removeOne(skill_name);
            }
        }
        player->tag["BaobianSkills"] = QVariant::fromValue(baobian_skills);
    }

    mutable QStringList acquired_skills, detached_skills;
};

BifaCard::BifaCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool BifaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->getPile("bifa").isEmpty() && to_select != Self;
}

void BifaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    target->tag["BifaSource" + QString::number(getEffectiveId())] = QVariant::fromValue((PlayerStar)source);
    room->broadcastSkillInvoke("bifa", 1);
    target->addToPile("bifa", this, false);
}

class BifaViewAsSkill: public OneCardViewAsSkill {
public:
    BifaViewAsSkill(): OneCardViewAsSkill("bifa") {
        filter_pattern = ".|.|.|hand";
        response_pattern = "@@bifa";
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        Card *card = new BifaCard;
        card->addSubcard(originalcard);
        return card;
    }
};

class Bifa: public TriggerSkill {
public:
    Bifa(): TriggerSkill("bifa") {
        events << EventPhaseStart;
        view_as_skill = new BifaViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish && !player->isKongcheng()) {
            room->askForUseCard(player, "@@bifa", "@bifa-remove", -1, Card::MethodNone);
        } else if (player->getPhase() == Player::RoundStart && player->getPile("bifa").length() > 0) {
            QList<int> bifa_list = player->getPile("bifa");

            while (!bifa_list.isEmpty()) {
                int card_id = bifa_list.last();
                ServerPlayer *chenlin = player->tag["BifaSource" + QString::number(card_id)].value<PlayerStar>();
                QList<int> ids;
                ids << card_id;

                LogMessage log;
                log.type = "$BifaView";
                log.from = player;
                log.card_str = QString::number(card_id);
                log.arg = "bifa";
                room->doNotify(player, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());

                room->fillAG(ids, player);
                const Card *cd = Sanguosha->getCard(card_id);
                QString pattern;
                if (cd->isKindOf("BasicCard"))
                    pattern = "BasicCard";
                else if (cd->isKindOf("TrickCard"))
                    pattern = "TrickCard";
                else if (cd->isKindOf("EquipCard"))
                    pattern = "EquipCard";
                QVariant data_for_ai = QVariant::fromValue(pattern);
                pattern.append("|.|.|hand");
                const Card *to_give = NULL;
                if (!player->isKongcheng() && chenlin && chenlin->isAlive())
                    to_give = room->askForCard(player, pattern, "@bifa-give", data_for_ai, Card::MethodNone, chenlin);
                if (chenlin && to_give) {
                    room->broadcastSkillInvoke(objectName(), 2);
                    chenlin->obtainCard(to_give, false);
                    player->obtainCard(cd, false);
                } else {
                    room->broadcastSkillInvoke(objectName(), 2);
                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
                    room->throwCard(cd, reason, NULL);
                    room->loseHp(player);
                }
                bifa_list.removeOne(card_id);
                room->clearAG(player);
                player->tag.remove("BifaSource" + QString::number(card_id));
            }
        }
        return false;
    }
};

SongciCard::SongciCard() {
    mute = true;
}

bool SongciCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->getMark("songci" + Self->objectName()) == 0 && to_select->getHandcardNum() != to_select->getHp();
}

void SongciCard::onEffect(const CardEffectStruct &effect) const{
    int handcard_num = effect.to->getHandcardNum();
    int hp = effect.to->getHp();
    effect.to->gainMark("@songci");
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.to, "songci" + effect.from->objectName());
    if (handcard_num > hp) {
        room->broadcastSkillInvoke("songci", 2);
        room->askForDiscard(effect.to, "songci", 2, 2, false, true);
    } else if (handcard_num < hp) {
        room->broadcastSkillInvoke("songci", 1);
        effect.to->drawCards(2, "songci");
    }
}

class SongciViewAsSkill: public ZeroCardViewAsSkill {
public:
    SongciViewAsSkill(): ZeroCardViewAsSkill("songci") {
    }

    virtual const Card *viewAs() const{
        return new SongciCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (player->getMark("songci" + player->objectName()) == 0 && player->getHandcardNum() != player->getHp()) return true;
        foreach (const Player *sib, player->getAliveSiblings())
            if (sib->getMark("songci" + player->objectName()) == 0 && sib->getHandcardNum() != sib->getHp())
                return true;
        return false;
    }
};

class Songci: public TriggerSkill {
public:
    Songci(): TriggerSkill("songci") {
        events << Death;
        view_as_skill = new SongciViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player) return false;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("@songci") > 0)
                room->setPlayerMark(p, "@songci", 0);
            if (p->getMark("songci" + player->objectName()) > 0)
                room->setPlayerMark(p, "songci" + player->objectName(), 0);
        }
        return false;
    }
};

class Xiuluo: public PhaseChangeSkill {
public:
    Xiuluo(): PhaseChangeSkill("xiuluo") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->canDiscard(target, "h")
               && hasDelayedTrick(target);
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        while (hasDelayedTrick(target) && target->canDiscard(target, "h")) {
            QStringList suits;
            foreach (const Card *jcard, target->getJudgingArea()) {
                if (!suits.contains(jcard->getSuitString()))
                    suits << jcard->getSuitString();
            }

            const Card *card = room->askForCard(target, QString(".|%1|.|hand").arg(suits.join(",")),
                                                "@xiuluo", QVariant(), objectName());
            if (!card || !hasDelayedTrick(target)) break;
            room->broadcastSkillInvoke(objectName());

            QList<int> avail_list, other_list;
            foreach (const Card *jcard, target->getJudgingArea()) {
                if (jcard->isKindOf("SkillCard")) continue;
                if (jcard->getSuit() == card->getSuit())
                    avail_list << jcard->getEffectiveId();
                else
                    other_list << jcard->getEffectiveId();
            }
            room->fillAG(avail_list + other_list, NULL, other_list);
            int id = room->askForAG(target, avail_list, false, objectName());
            room->clearAG();
            room->throwCard(id, NULL);
        }

        return false;
    }

private:
    static bool hasDelayedTrick(const ServerPlayer *target) {
        foreach (const Card *card, target->getJudgingArea())
            if (!card->isKindOf("SkillCard")) return true;
        return false;
    }
};

class Shenwei: public DrawCardsSkill {
public:
    Shenwei(): DrawCardsSkill("#shenwei-draw") {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        room->broadcastSkillInvoke("shenwei");
        room->notifySkillInvoked(player, "shenwei");
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = "shenwei";
        room->sendLog(log);

        return n + 2;
    }
};

class ShenweiKeep: public MaxCardsSkill {
public:
    ShenweiKeep(): MaxCardsSkill("shenwei") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasSkill(objectName()))
            return 2;
        else
            return 0;
    }
};

class Shenji: public TargetModSkill {
public:
    Shenji(): TargetModSkill("shenji") {
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const{
        if (from->hasSkill(objectName()) && from->getWeapon() == NULL)
            return 2;
        else
            return 0;
    }
};

class Xingwu: public TriggerSkill {
public:
    Xingwu(): TriggerSkill("xingwu") {
        events << PreCardUsed << CardResponded << EventPhaseStart << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == PreCardUsed || triggerEvent == CardResponded) {
            CardStar card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                if (response.m_isUse)
                   card = response.m_card;
            }
            if (card && card->getTypeId() != Card::TypeSkill && card->getHandlingMethod() == Card::MethodUse) {
                int n = player->getMark(objectName());
                if (card->isBlack())
                    n |= 1;
                else if (card->isRed())
                    n |= 2;
                player->setMark(objectName(), n);
            }
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Discard) {
                int n = player->getMark(objectName());
                bool red_avail = ((n & 2) == 0), black_avail = ((n & 1) == 0);
                if (player->isKongcheng() || (!red_avail && !black_avail))
                    return false;
                QString pattern = ".|.|.|hand";
                if (red_avail != black_avail)
                    pattern = QString(".|%1|.|hand").arg(red_avail ? "red" : "black");
                const Card *card = room->askForCard(player, pattern, "@xingwu", QVariant(), Card::MethodNone);
                if (card) {
                    room->notifySkillInvoked(player, objectName());
                    room->broadcastSkillInvoke(objectName(), 1);

                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);

                    player->addToPile(objectName(), card);
                }
            } else if (player->getPhase() == Player::RoundStart) {
                player->setMark(objectName(), 0);
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.to_place == Player::PlaceSpecial && player->getPile(objectName()).length() >= 3) {
                player->clearOnePrivatePile(objectName());
                QList<ServerPlayer *> males;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->isMale())
                        males << p;
                }
                if (males.isEmpty()) return false;

                ServerPlayer *target = room->askForPlayerChosen(player, males, objectName(), "@xingwu-choose");
                room->broadcastSkillInvoke(objectName(), 2);
                room->damage(DamageStruct(objectName(), player, target, 2));

                if (!player->isAlive()) return false;
                QList<const Card *> equips = target->getEquips();
                if (!equips.isEmpty()) {
                    DummyCard *dummy = new DummyCard;
                    foreach (const Card *equip, equips) {
                        if (player->canDiscard(target, equip->getEffectiveId()))
                            dummy->addSubcard(equip);
                    }
                    if (dummy->subcardsLength() > 0)
                        room->throwCard(dummy, target, player);
                    delete dummy;
                }
            }
        }
        return false;
    }
};

class Luoyan: public TriggerSkill {
public:
    Luoyan(): TriggerSkill("luoyan") {
        events << CardsMoveOneTime << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventLoseSkill && data.toString() == objectName()) {
            room->handleAcquireDetachSkills(player, "-tianxiang|-liuli", true);
        } else if (triggerEvent == EventAcquireSkill && data.toString() == objectName()) {
            if (!player->getPile("xingwu").isEmpty()) {
                room->notifySkillInvoked(player, objectName());
                room->handleAcquireDetachSkills(player, "tianxiang|liuli");
            }
        } else if (triggerEvent == CardsMoveOneTime && player->isAlive() && player->hasSkill(objectName(), true)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.to_place == Player::PlaceSpecial && move.to_pile_name == "xingwu") {
                if (player->getPile("xingwu").length() == 1) {
                    room->notifySkillInvoked(player, objectName());
                    room->handleAcquireDetachSkills(player, "tianxiang|liuli");
                }
            } else if (move.from == player && move.from_places.contains(Player::PlaceSpecial)
                       && move.from_pile_names.contains("xingwu")) {
                if (player->getPile("xingwu").isEmpty())
                    room->handleAcquireDetachSkills(player, "-tianxiang|-liuli", true);
            }
        }
        return false;
    }
};

class Yanyu: public TriggerSkill {
public:
    Yanyu(): TriggerSkill("yanyu") {
        events << EventPhaseStart << BeforeCardsMove << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *xiahou = room->findPlayerBySkillName(objectName());
            if (xiahou && player->getPhase() == Player::Play) {
                if (!xiahou->canDiscard(xiahou, "he")) return false;
                const Card *card = room->askForCard(xiahou, "..", "@yanyu-discard", QVariant(), objectName());
                if (card)
                    xiahou->addMark("YanyuDiscard" + QString::number(card->getTypeId()), 3);
            }
        } else if (triggerEvent == BeforeCardsMove && TriggerSkill::triggerable(player)) {
            ServerPlayer *current = room->getCurrent();
            if (!current || current->getPhase() != Player::Play) return false;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile) {
                QList<int> ids, disabled;
                QList<int> all_ids = move.card_ids;
                foreach (int id, move.card_ids) {
                    const Card *card = Sanguosha->getCard(id);
                    if (player->getMark("YanyuDiscard" + QString::number(card->getTypeId())) > 0)
                        ids << id;
                    else
                        disabled << id;
                }
                if (ids.isEmpty()) return false;
                while (!ids.isEmpty()) {
                    room->fillAG(all_ids, player, disabled);
                    bool only = (all_ids.length() == 1);
                    int card_id = -1;
                    if (only)
                        card_id = ids.first();
                    else
                        card_id = room->askForAG(player, ids, true, objectName());
                    if (card_id == -1) {
                        room->clearAG(player); //fix ag
                        break;
                    }
                    if (only)
                        player->setMark("YanyuOnlyId", card_id + 1); // For AI
                    const Card *card = Sanguosha->getCard(card_id);
                    ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(),
                                                                    QString("@yanyu-give:::%1:%2\\%3").arg(card->objectName())
                                                                                                      .arg(card->getSuitString() + "_char")
                                                                                                      .arg(card->getNumberString()),
                                                                    only, true);
                    room->clearAG(player);
                    player->setMark("YanyuOnlyId", 0);
                    if (target) {
                        player->removeMark("YanyuDiscard" + QString::number(card->getTypeId()));
                        int index = move.card_ids.indexOf(card_id);
                        Player::Place place = move.from_places.at(index);
                        move.from_places.removeAt(index);
                        move.card_ids.removeOne(card_id);
                        data = QVariant::fromValue(move);
                        ids.removeOne(card_id);
                        disabled << card_id;
                        foreach (int id, ids) {
                            const Card *card = Sanguosha->getCard(id);
                            if (player->getMark("YanyuDiscard" + QString::number(card->getTypeId())) == 0) {
                                ids.removeOne(id);
                                disabled << id;
                            }
                        }
                        if (move.from && move.from->objectName() == target->objectName() && place != Player::PlaceTable) {
                            // just indicate which card she chose...
                            LogMessage log;
                            log.type = "$MoveCard";
                            log.from = target;
                            log.to << target;
                            log.card_str = QString::number(card_id);
                            room->sendLog(log);
                        }
                        target->obtainCard(card);
                    } else
                        break;
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    p->setMark("YanyuDiscard1", 0);
                    p->setMark("YanyuDiscard2", 0);
                    p->setMark("YanyuDiscard3", 0);
                }
            }
        }
        return false;
    }
};

class Xiaode: public TriggerSkill {
public:
    Xiaode(): TriggerSkill("xiaode") {
        events << BuryVictim;
    }

    virtual int getPriority(TriggerEvent) const{
        return -2;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &) const{
        ServerPlayer *xiahoushi = room->findPlayerBySkillName(objectName());
        if (!xiahoushi || !xiahoushi->tag["XiaodeSkill"].toString().isEmpty()) return false;
        QStringList skill_list = xiahoushi->tag["XiaodeVictimSkills"].toStringList();
        if (skill_list.isEmpty()) return false;
        if (!room->askForSkillInvoke(xiahoushi, objectName(), QVariant::fromValue(skill_list))) return false;
        QString skill_name = room->askForChoice(xiahoushi, objectName(), skill_list.join("+"));
        xiahoushi->tag["XiaodeSkill"] = skill_name;
        room->acquireSkill(xiahoushi, skill_name);
        return false;
    }
};

class XiaodeEx: public TriggerSkill {
public:
    XiaodeEx(): TriggerSkill("#xiaode") {
        events << EventPhaseChanging << EventLoseSkill << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                QString skill_name = player->tag["XiaodeSkill"].toString();
                if (!skill_name.isEmpty()) {
                    room->detachSkillFromPlayer(player, skill_name, false, true);
                    player->tag.remove("XiaodeSkill");
                }
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "xiaode") {
            QString skill_name = player->tag["XiaodeSkill"].toString();
            if (!skill_name.isEmpty()) {
                room->detachSkillFromPlayer(player, skill_name, false, true);
                player->tag.remove("XiaodeSkill");
            }
        } else if (triggerEvent == Death && TriggerSkill::triggerable(player)) {
            DeathStruct death = data.value<DeathStruct>();
            QStringList skill_list;
            skill_list.append(addSkillList(death.who->getGeneral()));
            skill_list.append(addSkillList(death.who->getGeneral2()));
            player->tag["XiaodeVictimSkills"] = QVariant::fromValue(skill_list);
        }
        return false;
    }

private:
    QStringList addSkillList(const General *general) const{
        if (!general) return QStringList();
        QStringList skill_list;
        foreach (const Skill *skill, general->getSkillList()) {
            if (skill->isVisible() && !skill->isLordSkill() && skill->getFrequency() != Skill::Wake)
                skill_list.append(skill->objectName());
        }
        return skill_list;
    }
};

class Xiaoguo: public TriggerSkill {
public:
    Xiaoguo(): TriggerSkill("xiaoguo") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::Finish)
            return false;
        ServerPlayer *yuejin = room->findPlayerBySkillName(objectName());
        if (!yuejin || yuejin == player)
            return false;
        if (yuejin->canDiscard(yuejin, "h") && room->askForCard(yuejin, ".Basic", "@xiaoguo", QVariant(), objectName())) {
            room->broadcastSkillInvoke(objectName(), 1);
            if (!room->askForCard(player, ".Equip", "@xiaoguo-discard", QVariant())) {
                room->broadcastSkillInvoke(objectName(), 2);
                room->damage(DamageStruct("xiaoguo", yuejin, player));
            } else {
                room->broadcastSkillInvoke(objectName(), 3);
                if (yuejin->isAlive())
                    yuejin->drawCards(1);
            }
        }
        return false;
    }
};

ZhoufuCard::ZhoufuCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool ZhoufuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    return targets.isEmpty() && to_select->getPile("incantation").isEmpty();
}

void ZhoufuCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    target->tag["ZhoufuSource" + QString::number(getEffectiveId())] = QVariant::fromValue((PlayerStar)source);
    target->addToPile("incantation", this);
}

class ZhoufuViewAsSkill: public OneCardViewAsSkill {
public:
    ZhoufuViewAsSkill(): OneCardViewAsSkill("zhoufu") {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("ZhoufuCard");
    }

    virtual const Card *viewAs(const Card *originalcard) const{
        Card *card = new ZhoufuCard;
        card->addSubcard(originalcard);
        return card;
    }
};

class Zhoufu: public TriggerSkill {
public:
    Zhoufu(): TriggerSkill("zhoufu") {
        events << StartJudge << EventPhaseChanging;
        view_as_skill = new ZhoufuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getPile("incantation").length() > 0;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == StartJudge) {
            int card_id = player->getPile("incantation").first();

            JudgeStar judge = data.value<JudgeStar>();
            judge->card = Sanguosha->getCard(card_id);

            LogMessage log;
            log.type = "$ZhoufuJudge";
            log.from = player;
            log.arg = objectName();
            log.card_str = QString::number(judge->card->getEffectiveId());
            room->sendLog(log);

            room->moveCardTo(judge->card, NULL, judge->who, Player::PlaceJudge,
                             CardMoveReason(CardMoveReason::S_REASON_JUDGE,
                             judge->who->objectName(),
                             QString(), QString(), judge->reason), true);
            judge->updateResult();
            room->setTag("SkipGameRule", true);
        } else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                int id = player->getPile("incantation").first();
                PlayerStar zhangbao = player->tag["ZhoufuSource" + QString::number(id)].value<PlayerStar>();
                if (zhangbao && zhangbao->isAlive())
                    zhangbao->obtainCard(Sanguosha->getCard(id));
            }
        }
        return false;
    }
};

class Yingbing: public TriggerSkill {
public:
    Yingbing(): TriggerSkill("yingbing") {
        events << StartJudge;
        frequency = Frequent;
    }

    virtual int getPriority(TriggerEvent) const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        int id = judge->card->getEffectiveId();
        PlayerStar zhangbao = player->tag["ZhoufuSource" + QString::number(id)].value<PlayerStar>();
        if (zhangbao && TriggerSkill::triggerable(zhangbao)
            && zhangbao->askForSkillInvoke(objectName(), data))
            zhangbao->drawCards(2);
        return false;
    }
};

class Kangkai: public TriggerSkill {
public:
    Kangkai(): TriggerSkill("kangkai") {
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            foreach (ServerPlayer *to, use.to) {
                if (!player->isAlive()) break;
                if (player->distanceTo(to) <= 1 && TriggerSkill::triggerable(player)
                    && room->askForSkillInvoke(player, objectName(), QVariant::fromValue((PlayerStar)to))) {
                        player->drawCards(1);
                        if (!player->isNude() && player != to) {
                            const Card *card = NULL;
                            if (player->getCardCount() > 1) {
                                card = room->askForCard(player, "..!", "@kangkai-give:" + to->objectName(), data, Card::MethodNone);
                                if (!card)
                                    card = player->getCards("he").at(qrand() % player->getCardCount());
                            } else {
                                Q_ASSERT(player->getCardCount() == 1);
                                card = player->getCards("he").first();
                            }
                            to->obtainCard(card);
                            if (card->getTypeId() == Card::TypeEquip && room->getCardOwner(card->getEffectiveId()) == to
                                && !to->isLocked(card)) {
                                    to->tag["KangkaiSlash"] = data;
                                    bool will_use = room->askForSkillInvoke(to, "kangkai_use", "use");
                                    to->tag.remove("KangkaiSlash");
                                    if (will_use)
                                        room->useCard(CardUseStruct(card, to, to));
                            }
                        }
                }
            }
        }
        return false;
    }
};


HongyuanCard::HongyuanCard() {
    mute = true;
}

bool HongyuanCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() <= 2 && !targets.contains(Self);
}

bool HongyuanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return to_select != Self && targets.length() < 2;
}

void HongyuanCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->setFlags("HongyuanTarget");
}

class HongyuanViewAsSkill: public ZeroCardViewAsSkill {
public:
    HongyuanViewAsSkill(): ZeroCardViewAsSkill("hongyuan") {
        response_pattern = "@@hongyuan";
    }

    virtual const Card *viewAs() const{
        return new HongyuanCard;
    }
};

class Hongyuan: public DrawCardsSkill {
public:
    Hongyuan(): DrawCardsSkill("hongyuan") {
        frequency = NotFrequent;
        view_as_skill = new HongyuanViewAsSkill;
    }

    virtual int getDrawNum(ServerPlayer *zhugejin, int n) const{
        Room *room = zhugejin->getRoom();
        bool invoke = false;
        if (room->getMode().startsWith("06_"))
            invoke = room->askForSkillInvoke(zhugejin, objectName());
        else
            invoke = room->askForUseCard(zhugejin, "@@hongyuan", "@hongyuan");
        if (invoke) {
            room->broadcastSkillInvoke(objectName());
            zhugejin->setFlags("hongyuan");
            return n - 1;
        } else
            return n;
    }
};

class HongyuanDraw: public TriggerSkill {
public:
    HongyuanDraw(): TriggerSkill("#hongyuan") {
        events << AfterDrawNCards;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (!player->hasFlag("hongyuan"))
            return false;
        player->setFlags("-hongyuan");

        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (room->getMode().startsWith("06_")) {
                if (AI::GetRelation3v3(player, p) == AI::Friend)
                    targets << p;
            } else if (p->hasFlag("HongyuanTarget")) {
                p->setFlags("-HongyuanTarget");
                targets << p;
            }
        }

        if (targets.isEmpty()) return false;
        room->drawCards(targets, 1, "hongyuan");
        return false;
    }
};

class Huanshi: public TriggerSkill {
public:
    Huanshi(): TriggerSkill("huanshi") {
        events << AskForRetrial;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && !target->isNude();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        const Card *card = NULL;
        if (room->getMode().startsWith("06_")) {
            if (AI::GetRelation3v3(player, judge->who) != AI::Friend) return false;
            QStringList prompt_list;
            prompt_list << "@huanshi-card" << judge->who->objectName()
                << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
            QString prompt = prompt_list.join(":");

            card = room->askForCard(player, "..", prompt, data, Card::MethodResponse, judge->who, true);
        } else if (!player->isKongcheng()) {
            QList<int> ids, disabled_ids;
            foreach (const Card *card, player->getHandcards()) {
                if (player->isCardLimited(card, Card::MethodResponse))
                    disabled_ids << card->getEffectiveId();
                else
                    ids << card->getEffectiveId();
            }
            if (!ids.isEmpty() && room->askForSkillInvoke(player, objectName(), data)) {
                if (judge->who != player) {
                    LogMessage log;
                    log.type = "$ViewAllCards";
                    log.from = judge->who;
                    log.to << player;
                    log.card_str = IntList2StringList(player->handCards()).join("+");
                    room->doNotify(judge->who, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
                }
                judge->who->tag["HuanshiJudge"] = data;
                room->fillAG(player->handCards(), judge->who, disabled_ids);
                int card_id = room->askForAG(judge->who, ids, false, objectName());
                room->clearAG(judge->who);
                judge->who->tag.remove("HuanshiJudge");
                card = Sanguosha->getCard(card_id);
            }
        }
        if (card != NULL) {
            room->broadcastSkillInvoke(objectName());
            room->retrial(card, player, judge, objectName());
        }

        return false;
    }
};

class Mingzhe: public TriggerSkill {
public:
    Mingzhe(): TriggerSkill("mingzhe") {
        events << BeforeCardsMove << CardsMoveOneTime << CardUsed << CardResponded;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::NotActive) return false;
        if (triggerEvent == BeforeCardsMove || triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != player) return false;

            if (triggerEvent == BeforeCardsMove) {
                CardMoveReason reason = move.reason;
                if ((reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                    const Card *card;
                    int i = 0;
                    foreach (int card_id, move.card_ids) {
                        card = Sanguosha->getCard(card_id);
                        if (room->getCardOwner(card_id) == player && card->isRed()
                            && (move.from_places[i] == Player::PlaceHand
                            || move.from_places[i] == Player::PlaceEquip)) {
                                player->addMark(objectName());
                        }
                        i++;
                    }
                }
            } else {
                int n = player->getMark(objectName());
                try {
                    for (int i = 0; i < n; i++) {
                        player->removeMark(objectName());
                        if (player->isAlive() && player->askForSkillInvoke(objectName(), data)) {
                            room->broadcastSkillInvoke(objectName());
                            player->drawCards(1);
                        } else {
                            break;
                        }
                    }
                    player->setMark(objectName(), 0);
                }
                catch (TriggerEvent triggerEvent) {
                    if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                        player->setMark(objectName(), 0);
                    throw triggerEvent;
                }
            }
        } else {
            CardStar card = NULL;
            if (triggerEvent == CardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                card = use.card;
            } else if (triggerEvent == CardResponded) {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                card = resp.m_card;
            }
            if (card && card->isRed() && player->askForSkillInvoke(objectName(), data)) {
                room->broadcastSkillInvoke(objectName());
                player->drawCards(1);
            }
        }
        return false;
    }
};


#include "jsonutils.h"
class AocaiViewAsSkill: public ZeroCardViewAsSkill {
public:
    AocaiViewAsSkill(): ZeroCardViewAsSkill("aocai") {
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        if (player->getPhase() != Player::NotActive || player->hasFlag("Global_AocaiFailed")) return false;
        if (pattern == "slash")
            return Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
        else if (pattern == "peach")
            return player->getMark("Global_PreventPeach") == 0;
        else if (pattern.contains("analeptic"))
            return true;
        return false;
    }

    virtual const Card *viewAs() const{
        AocaiCard *aocai_card = new AocaiCard;
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
            pattern = "analeptic";
        aocai_card->setUserString(pattern);
        return aocai_card;
    }
};

class Aocai: public TriggerSkill {
public:
    Aocai(): TriggerSkill("aocai") {
        events << CardAsked;
        view_as_skill = new AocaiViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QString pattern = data.toStringList().first();
        if (player->getPhase() == Player::NotActive
            && (pattern == "slash" || pattern == "jink")
            && room->askForSkillInvoke(player, objectName(), data)) {
            QList<int> ids = room->getNCards(2, false);
            QList<int> enabled, disabled;
            foreach (int id, ids) {
                if (Sanguosha->getCard(id)->objectName().contains(pattern))
                    enabled << id;
                else
                    disabled << id;
            }
            int id = view(room, player, ids, enabled, disabled);
            if (id != -1) {
                const Card *card = Sanguosha->getCard(id);
                room->provide(card);
                return true;
            }
        }
        return false;
    }

    static int view(Room *room, ServerPlayer *player, QList<int> &ids, QList<int> &enabled, QList<int> &disabled) {
        int result = -1;
        LogMessage log;
        log.type = "$ViewDrawPile";
        log.from = player;
        log.card_str = IntList2StringList(ids).join("+");
        room->doNotify(player, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
        room->broadcastSkillInvoke("aocai");
        room->notifySkillInvoked(player, "aocai");
        if (enabled.isEmpty()) {
            Json::Value arg(Json::arrayValue);
            arg[0] = QSanProtocol::Utils::toJsonString(".");
            arg[1] = false;
            arg[2] = QSanProtocol::Utils::toJsonArray(ids);
            room->doNotify(player, QSanProtocol::S_COMMAND_SHOW_ALL_CARDS, arg);
        } else {
            room->fillAG(ids, player, disabled);
            int id = room->askForAG(player, enabled, true, "aocai");
            if (id != -1) {
                ids.removeOne(id);
                result = id;
            }
            room->clearAG(player);
        }

        QList<int> &drawPile = room->getDrawPile();
        for (int i = ids.length() - 1; i >= 0; i--)
            drawPile.prepend(ids.at(i));
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_UPDATE_PILE, Json::Value(drawPile.length()));
        if (result == -1)
            room->setPlayerFlag(player, "Global_AocaiFailed");
        return result;
    }
};

AocaiCard::AocaiCard() {
}

bool AocaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    const Card *card = NULL;
    if (!user_string.isEmpty()) {
        card = Sanguosha->cloneCard(user_string.split("+").first());
    }
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool AocaiCard::targetFixed() const{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFixed();
}

bool AocaiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    const Card *card = NULL;
    if (!user_string.isEmpty()) {
        card = Sanguosha->cloneCard(user_string.split("+").first());
    }
    return card && card->targetsFeasible(targets, Self);
}

const Card *AocaiCard::validateInResponse(ServerPlayer *user) const{
    Room *room = user->getRoom();
    QList<int> ids = room->getNCards(2, false);
    QStringList names = user_string.split("+");
    if (names.contains("slash")) names << "fire_slash" << "thunder_slash";

    QList<int> enabled, disabled;
    foreach (int id, ids) {
        if (names.contains(Sanguosha->getCard(id)->objectName()))
            enabled << id;
        else
            disabled << id;
    }

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = user;
    log.arg = "aocai";
    room->sendLog(log);

    int id = Aocai::view(room, user, ids, enabled, disabled);
    return Sanguosha->getCard(id);
}

const Card *AocaiCard::validate(CardUseStruct &cardUse) const{
    cardUse.m_isOwnerUse = false;
    ServerPlayer *user = cardUse.from;
    Room *room = user->getRoom();
    QList<int> ids = room->getNCards(2, false);
    QStringList names = user_string.split("+");
    if (names.contains("slash")) names << "fire_slash" << "thunder_slash";

    QList<int> enabled, disabled;
    foreach (int id, ids) {
        if (names.contains(Sanguosha->getCard(id)->objectName()))
            enabled << id;
        else
            disabled << id;
    }

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = user;
    log.arg = "aocai";
    room->sendLog(log);

    int id = Aocai::view(room, user, ids, enabled, disabled);
    return Sanguosha->getCard(id);
}

DuwuCard::DuwuCard() {
    mute = true;
}

bool DuwuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (!targets.isEmpty() || qMax(0, to_select->getHp()) != subcardsLength())
        return false;
    if (!Self->inMyAttackRange(to_select) || Self == to_select)
        return false;

    if (Self->getWeapon() && subcards.contains(Self->getWeapon()->getId())) {
        const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        int distance_fix = weapon->getRange() - Self->getAttackRange(false);
        if (Self->getOffensiveHorse() && subcards.contains(Self->getOffensiveHorse()->getId()))
            distance_fix += 1;
        return Self->distanceTo(to_select, distance_fix) <= Self->getAttackRange();
    } else if (Self->getOffensiveHorse() && subcards.contains(Self->getOffensiveHorse()->getId())) {
        return Self->distanceTo(to_select, 1) <= Self->getAttackRange();
    } else
        return true;
}

void DuwuCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    if (getSubcards().length() == 1)
        room->broadcastSkillInvoke("duwu", 2);
    else
        room->broadcastSkillInvoke("duwu", 1);

    room->damage(DamageStruct("duwu", effect.from, effect.to));
}

class DuwuViewAsSkill: public ViewAsSkill {
public:
    DuwuViewAsSkill(): ViewAsSkill("duwu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasFlag("DuwuEnterDying");
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *card) const{
        return !Self->isJilei(card);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        DuwuCard *duwu = new DuwuCard;
        if (!cards.isEmpty())
            duwu->addSubcards(cards);
        return duwu;
    }
};

class Duwu: public TriggerSkill {
public:
    Duwu(): TriggerSkill("duwu") {
        events << QuitDying;
        view_as_skill = new DuwuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.damage && dying.damage->getReason() == "duwu" && !dying.damage->chain && !dying.damage->transfer) {
            ServerPlayer *from = dying.damage->from;
            if (from && from->isAlive()) {
                room->setPlayerFlag(from, "DuwuEnterDying");
                room->loseHp(from, 1);
            }
        }
        return false;
    }
};

class Shuijian: public DrawCardsSkill {
public:
    Shuijian(): DrawCardsSkill("shuijian") {
        frequency = Frequent;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        if (player->askForSkillInvoke(objectName())) {
            player->getRoom()->broadcastSkillInvoke(objectName());
            return n + player->getEquips().length() / 2 + 1;
        } else
            return n;
    }
};

JisuCard::JisuCard() {
    mute = true;
}

bool JisuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("jisu");
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

void JisuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    foreach (ServerPlayer *target, targets) {
        if (!source->canSlash(target, NULL, false))
            targets.removeOne(target);
    }

    if (targets.length() > 0) {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_jisu");
        room->useCard(CardUseStruct(slash, source, targets));
    }
}

class JisuViewAsSkill: public ZeroCardViewAsSkill {
public:
    JisuViewAsSkill(): ZeroCardViewAsSkill("jisu") {
        response_pattern = "@@jisu";
    }

    virtual const Card *viewAs() const{
        return new JisuCard;
    }
};

class Jisu: public TriggerSkill {
public:
    Jisu(): TriggerSkill("jisu") {
        events << EventPhaseChanging;
        view_as_skill = new JisuViewAsSkill;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge && !player->isSkipped(Player::Judge)
            && !player->isSkipped(Player::Draw)) {
            if (Slash::IsAvailable(player) && room->askForUseCard(player, "@@jisu", "@jisu-slash")) {
                player->skip(Player::Judge, true);
                player->skip(Player::Draw, true);
            }
        }
        return false;
    }
};

class Shuiyong: public TriggerSkill {
public:
    Shuiyong(): TriggerSkill("shuiyong") {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature == DamageStruct::Fire) {
            room->notifySkillInvoked(player, objectName());
            room->broadcastSkillInvoke(objectName());

            LogMessage log;
            log.type = "#ShuiyongProtect";
            log.from = player;
            log.arg = QString::number(damage.damage);
            log.arg2 = "fire_nature";
            room->sendLog(log);
            return true;
        }
        return false;
    }
};

Fentian::Fentian(): PhaseChangeSkill("fentian"){
    frequency = Compulsory;
}

bool Fentian::onPhaseChange(ServerPlayer *hanba) const{
    if (hanba->getPhase() != Player::Finish)
        return false;

    if (hanba->getHandcardNum() >= hanba->getHp())
        return false;

    QList<ServerPlayer*> targets;
    Room* room = hanba->getRoom();
    foreach(ServerPlayer* p, room->getOtherPlayers(hanba)){
        if (hanba->inMyAttackRange(p) && !p->isNude())
            targets << p;
    };

    if (targets.isEmpty())
        return false;

    room->broadcastSkillInvoke(objectName());
    ServerPlayer *target = room->askForPlayerChosen(hanba, targets, objectName());
    int id = room->askForCardChosen(hanba, target, "he", objectName());
    hanba->addToPile("burn", id);
    return false;
}

class FentianRange: public AttackRangeSkill{
public:
    FentianRange(): AttackRangeSkill("#fentian"){

    }

    virtual int getExtra(const Player *target, bool ) const{
        if (target->hasSkill(objectName()))
            return target->getPile("burn").length();

        return 0;
    }
};

Zhiri::Zhiri(): PhaseChangeSkill("zhiri") {
    frequency = Wake;
}

bool Zhiri::onPhaseChange(ServerPlayer *hanba) const {
    if (hanba->getMark(objectName()) > 0 || hanba->getPhase() != Player::Start)
        return false;

    if (hanba->getPile("burn").length() < 3)
        return false;

    Room *room = hanba->getRoom();
    room->broadcastSkillInvoke(objectName());
    room->doLightbox("$ZhiriAnimate", 4000);

    if (room->changeMaxHpForAwakenSkill(hanba)) {
        room->acquireSkill(hanba, "xintan");
        room->addPlayerMark(hanba, objectName());
    }

    return false;
};

XintanCard::XintanCard() {
    will_throw = true;
    handling_method = Card::MethodNone;
}

bool XintanCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const {
    return targets.isEmpty();
}

void XintanCard::onEffect(const CardEffectStruct &effect) const {
    ServerPlayer *hanba = effect.from;
    QList<int> burn = hanba->getPile("burn");
    if (burn.length() < 2)
        return;

    Room *room = hanba->getRoom();
    QList<int> subs;

    if (burn.length() == 2)
        subs = burn;
    else {
        int aidelay = Config.AIDelay;
        Config.AIDelay = 0;
        while (subs.length() < 2) {
            room->fillAG(burn, hanba);
            int id = room->askForAG(hanba, burn, false, objectName());
            subs << id;
            burn.removeOne(id);
            room->clearAG(hanba);
        }
        Config.AIDelay = aidelay;
    };
    CardsMoveStruct move;
    move.from = hanba;
    move.to_place = Player::DiscardPile;
    move.reason = CardMoveReason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, hanba->objectName(), objectName(), QString());
    move.card_ids = subs;
    room->moveCardsAtomic(move, true);

    room->loseHp(effect.to);
}

class Xintan: public ZeroCardViewAsSkill {
public:
    Xintan(): ZeroCardViewAsSkill("xintan") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getPile("burn").length() >= 2 && !player->hasUsed("XintanCard");
    }

    virtual const Card *viewAs() const{
        return new XintanCard;
    }
};

class Nianrui: public DrawCardsSkill{
public:
    Nianrui(): DrawCardsSkill("nianrui"){
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *, int n) const{
        return n + 2;
    }
};

class Qixiang: public FilterSkill{
public:
    Qixiang(): FilterSkill("qixiang"){

    }

    virtual bool viewFilter(const Card *to_select) const{
        Room *room = Sanguosha->currentRoom();
        if (room->getCardPlace(to_select->getEffectiveId()) == Player::PlaceJudge){
            ServerPlayer *nianshou = room->getCardOwner(to_select->getEffectiveId());
            if (nianshou != NULL && nianshou->hasSkill(objectName())){
                QString reason = nianshou->property("qixiang_currentjudge").toString();
                if (reason == "indulgence")
                    return to_select->getSuit() == Card::Diamond;
                else if (reason == "supply_shortage")
                    return to_select->getSuit() == Card::Spade;
            }
        }
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        WrappedCard *wrap = Sanguosha->getWrappedCard(originalCard->getEffectiveId());
        if (originalCard->getSuit() == Card::Diamond)
            wrap->setSuit(Card::Heart);
        else if (originalCard->getSuit() == Card::Spade)
            wrap->setSuit(Card::Club);
        else
            return originalCard;

        wrap->setSkillName(objectName());
        wrap->setModified(true);
        return wrap;
    }
};

class QixiangTrigger: public TriggerSkill{
public:
    QixiangTrigger(): TriggerSkill("#qixiang-trigger"){
        events << StartJudge << FinishJudge << TurnBroken;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == StartJudge && TriggerSkill::triggerable(player)){
            if (!player->property("qixiang_stack").canConvert(QVariant::StringList))
                room->setPlayerProperty(player, "qixiang_stack", QStringList());
            
            if (player->property("qixiang_currentjudge").canConvert(QVariant::String)){
                QString s = player->property("qixiang_currentjudge").toString();
                QStringList stack = player->property("qixiang_stack").toStringList();
                stack << s;
                room->setPlayerProperty(player, "qixiang_stack", stack);
            }

            JudgeStruct *judge = data.value<JudgeStruct *>();
            room->setPlayerProperty(player, "qixiang_currentjudge", judge->reason);
        }
        else if (triggerEvent == FinishJudge && TriggerSkill::triggerable(player)){
            QStringList stack = player->property("qixiang_stack").toStringList();
            if (!stack.isEmpty()){
                QString s = stack.takeLast();
                room->setPlayerProperty(player, "qixiang_currentjudge", s);
                room->setPlayerProperty(player, "qixiang_stack", stack);
            }
            else {
                room->setPlayerProperty(player, "qixiang_currentjudge", QVariant());
                room->setPlayerProperty(player, "qixiang_stack", QVariant());
            }
        }
        else if (triggerEvent == TurnBroken){
            ServerPlayer *nianshou = room->findPlayerBySkillName("qixiang");
            if (nianshou != NULL){
                room->setPlayerProperty(nianshou, "qixiang_currentjudge", QVariant());
                room->setPlayerProperty(nianshou, "qixiang_stack", QVariant());
            }
        }
        return false;
    }
};


SPCardPackage::SPCardPackage()
    : Package("sp_cards")
{
    (new SPMoonSpear)->setParent(this);
    skills << new SPMoonSpearSkill;

    type = CardPack;
}

ADD_PACKAGE(SPCard)


ChaosPackage::ChaosPackage()
    : Package("Chaos")
{
    General *sunyang = new General(this, "sunyang", "wu", 4);
    sunyang->addSkill(new Shuijian);

    General *yeshiwen = new General(this, "yeshiwen", "wu", 3, false);
    yeshiwen->addSkill(new Jisu);
    yeshiwen->addSkill(new SlashNoDistanceLimitSkill("jisu"));
    yeshiwen->addSkill(new Shuiyong);
    related_skills.insertMulti("jisu", "#jisu-slash-ndl");

    General *hanba = new General(this, "hanba", "qun", 4, false);
    hanba->addSkill(new Fentian);
    hanba->addSkill(new FentianRange);
    hanba->addSkill(new Zhiri);
    hanba->addRelateSkill("xintan");
    related_skills.insertMulti("fentian", "#fentian");

    addMetaObject<JisuCard>();
    addMetaObject<XintanCard>();

    skills << new Xintan;
}

ADD_PACKAGE(Chaos)


SPPackage::SPPackage()
    : Package("sp")
{
    General *yangxiu = new General(this, "yangxiu", "wei", 3); // SP 001
    yangxiu->addSkill(new Jilei);
    yangxiu->addSkill(new JileiClear);
    yangxiu->addSkill(new Danlao);
    related_skills.insertMulti("jilei", "#jilei-clear");

    General *gongsunzan = new General(this, "gongsunzan", "qun"); // SP 003
    gongsunzan->addSkill(new Yicong);
    gongsunzan->addSkill(new YicongEffect);
    related_skills.insertMulti("yicong", "#yicong-effect");

    General *yuanshu = new General(this, "yuanshu", "qun"); // SP 004
    yuanshu->addSkill(new Yongsi);
    yuanshu->addSkill(new Weidi);

    General *sp_sunshangxiang = new General(this, "sp_sunshangxiang", "shu", 3, false, true); // SP 005
    sp_sunshangxiang->addSkill("jieyin");
    sp_sunshangxiang->addSkill("xiaoji");

    General *sp_pangde = new General(this, "sp_pangde", "wei", 4, true, true); // SP 006
    sp_pangde->addSkill("mashu");
    sp_pangde->addSkill("mengjin");

    General *sp_guanyu = new General(this, "sp_guanyu", "wei", 4); // SP 007
    sp_guanyu->addSkill("wusheng");
    sp_guanyu->addSkill(new Danji);

    General *shenlvbu1 = new General(this, "shenlvbu1", "god", 8, true, true); // SP 008 (2-1)
    shenlvbu1->addSkill("mashu");
    shenlvbu1->addSkill("wushuang");

    General *shenlvbu2 = new General(this, "shenlvbu2", "god", 4, true, true); // SP 008 (2-2)
    shenlvbu2->addSkill("mashu");
    shenlvbu2->addSkill("wushuang");
    shenlvbu2->addSkill(new Xiuluo);
    shenlvbu2->addSkill(new ShenweiKeep);
    shenlvbu2->addSkill(new Shenwei);
    shenlvbu2->addSkill(new Shenji);
    related_skills.insertMulti("shenwei", "#shenwei-draw");

    General *sp_caiwenji = new General(this, "sp_caiwenji", "wei", 3, false, true); // SP 009
    sp_caiwenji->addSkill("beige");
    sp_caiwenji->addSkill("duanchang");

    General *sp_machao = new General(this, "sp_machao", "qun", 4, true, true); // SP 011
    sp_machao->addSkill("mashu");
    sp_machao->addSkill("tieji");

    General *sp_jiaxu = new General(this, "sp_jiaxu", "wei", 3, true, true); // SP 012
    sp_jiaxu->addSkill("wansha");
    sp_jiaxu->addSkill("luanwu");
    sp_jiaxu->addSkill("weimu");

    General *caohong = new General(this, "caohong", "wei"); // SP 013
    caohong->addSkill(new Yuanhu);

    General *guanyinping = new General(this, "guanyinping", "shu", 3, false); // SP 014
    guanyinping->addSkill(new Xueji);
    guanyinping->addSkill(new Huxiao);
    guanyinping->addSkill(new HuxiaoCount);
    guanyinping->addSkill(new HuxiaoClear);
    guanyinping->addSkill(new Wuji);
    guanyinping->addSkill(new WujiCount);
    related_skills.insertMulti("wuji", "#wuji-count");
    related_skills.insertMulti("huxiao", "#huxiao-count");
    related_skills.insertMulti("huxiao", "#huxiao-clear");

    General *liuxie = new General(this, "liuxie", "qun", 3);
    liuxie->addSkill(new Tianming);
    liuxie->addSkill(new Mizhao);
    liuxie->addSkill(new MizhaoSlashNoDistanceLimit);
    related_skills.insertMulti("mizhao", "#mizhao-slash-ndl");

    General *lingju = new General(this, "lingju", "qun", 3, false);
    lingju->addSkill(new Jieyuan);
    lingju->addSkill(new Fenxin);

    General *fuwan = new General(this, "fuwan", "qun", 4);
    fuwan->addSkill(new Moukui);

    General *xiahouba = new General(this, "xiahouba", "shu"); // SP 019
    xiahouba->addSkill(new Baobian);

    General *chenlin = new General(this, "chenlin", "wei", 3); // SP 020
    chenlin->addSkill(new Bifa);
    chenlin->addSkill(new Songci);

    General *erqiao = new General(this, "erqiao", "wu", 3, false); // SP 021
    erqiao->addSkill(new Xingwu);
    erqiao->addSkill(new Luoyan);

    General *xiahoushi = new General(this, "xiahoushi", "shu", 3, false); // SP 023
    xiahoushi->addSkill(new Yanyu);
    xiahoushi->addSkill(new Xiaode);
    xiahoushi->addSkill(new XiaodeEx);
    related_skills.insertMulti("xiaode", "#xiaode");

    General *sp_yuejin = new General(this, "yuejin", "wei"); // SP 024
    sp_yuejin->addSkill(new Xiaoguo);

    General *zhangbao = new General(this, "zhangbao", "qun", 3); // SP 025
    zhangbao->addSkill(new Zhoufu);
    zhangbao->addSkill(new Yingbing);

    General *caoang = new General(this, "caoang", "wei", 4);
    caoang->addSkill(new Kangkai);

    General *zhugejin = new General(this, "zhugejin", "wu", 3); // SP 027
    zhugejin->addSkill(new Hongyuan);
    zhugejin->addSkill(new HongyuanDraw);
    zhugejin->addSkill(new Huanshi);
    zhugejin->addSkill(new Mingzhe);
    related_skills.insertMulti("hongyuan", "#hongyuan");

    addMetaObject<HongyuanCard>();
    addMetaObject<YuanhuCard>();
    addMetaObject<XuejiCard>();
    addMetaObject<MizhaoCard>();
    addMetaObject<BifaCard>();
    addMetaObject<SongciCard>();
    addMetaObject<ZhoufuCard>();

}

ADD_PACKAGE(SP)

OLPackage::OLPackage()
    : Package("OL")
{

    General *zhugeke = new General(this, "zhugeke", "wu", 3); // OL 002
    zhugeke->addSkill(new Aocai);
    zhugeke->addSkill(new Duwu);

    General *nianshou = new General(this, "nianshou", "qun", 10000, true, true, true);
    nianshou->addSkill(new Nianrui);
    nianshou->addSkill(new Qixiang);
    nianshou->addSkill(new QixiangTrigger);
    related_skills.insertMulti("qixiang", "#qixiang-trigger");

    addMetaObject<AocaiCard>();
    addMetaObject<DuwuCard>();
}

ADD_PACKAGE(OL)
