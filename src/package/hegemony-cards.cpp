#include "hegemony-cards.h"

#if 0
#include "maneuvering.h"
#include "standard.h"
#include "engine.h"
#include "client.h"
#include "roomthread.h"


AwaitExhausted::AwaitExhausted(Card::Suit suit, int number) : TrickCard(suit, number)
{
    setObjectName("await_exhausted");
    target_fixed = true;
}

QString AwaitExhausted::getSubtype() const
{
    return "await_exhausted";
}

bool AwaitExhausted::isAvailable(const Player *player) const
{
    bool canUse = false;
    if (!player->isProhibited(player, this))
        canUse = true;
    if (!canUse) {
        QList<const Player *> players = player->getAliveSiblings();
        foreach (const Player *p, players) {
            if (player->isProhibited(p, this))
                continue;
            if (player->isFriendWith(p)) {
                canUse = true;
                break;
            }
        }
    }

    return canUse && TrickCard::isAvailable(player);
}

void AwaitExhausted::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct new_use = card_use;
    if (!card_use.from->isProhibited(card_use.from, this))
        new_use.to << new_use.from;
    foreach (ServerPlayer *p, room->getOtherPlayers(new_use.from)) {
        if (p->isFriendWith(new_use.from)) {
            const ProhibitSkill *skill = room->isProhibited(card_use.from, p, this);
            if (skill) {
                LogMessage log;
                log.type = "#SkillAvoid";
                log.from = p;
                log.arg = skill->objectName();
                log.arg2 = objectName();
                room->sendLog(log);

                room->broadcastSkillInvoke(skill->objectName(), p);
            } else {
                new_use.to << p;
            }
        }
    }

    TrickCard::onUse(room, new_use);
}

void AwaitExhausted::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QStringList nullified_list = room->getTag("CardUseNullifiedList").toStringList();
    bool all_nullified = nullified_list.contains("_ALL_TARGETS");
    foreach (ServerPlayer *target, targets) {
        CardEffectStruct effect;
        effect.card = this;
        effect.from = source;
        effect.to = target;
        effect.multiple = (targets.length() > 1);
        effect.nullified = (all_nullified || nullified_list.contains(target->objectName()));

        QVariantList players;
        for (int i = targets.indexOf(target); i < targets.length(); i++) {
            if (!nullified_list.contains(targets.at(i)->objectName()) && !all_nullified)
                players.append(QVariant::fromValue(targets.at(i)));
        }
        room->setTag("targets" + this->toString(), QVariant::fromValue(players));

        room->cardEffect(effect);
    }

    room->removeTag("targets" + this->toString());

    foreach (ServerPlayer *target, targets) {
        if (target->hasFlag("AwaitExhaustedEffected")) {
            room->setPlayerFlag(target, "-AwaitExhaustedEffected");
            room->askForDiscard(target, objectName(), 2, 2, false, true);
        }
    }

    QList<int> table_cardids = room->getCardIdsOnTable(this);
    if (!table_cardids.isEmpty()) {
        DummyCard dummy(table_cardids);
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), this->getSkillName(), QString());
        if (targets.size() == 1) reason.m_targetId = targets.first()->objectName();
        room->moveCardTo(&dummy, source, NULL, Player::DiscardPile, reason, true);
    }
}

void AwaitExhausted::onEffect(const CardEffectStruct &effect) const
{
    effect.to->drawCards(2);
    effect.to->getRoom()->setPlayerFlag(effect.to, "AwaitExhaustedEffected");
}

KnownBoth::KnownBoth(Card::Suit suit, int number)
    :SingleTargetTrick(suit, number)
{
    setObjectName("known_both");
    can_recast = true;
}

bool KnownBoth::isAvailable(const Player *player) const
{
    bool can_use = false;
    foreach (const Player *p, player->getSiblings()) {
        if (player->isProhibited(p, this))
            continue;
        if (p->isKongcheng() && p->hasShownAllGenerals())
            continue;
        can_use = true;
        break;
    }
    bool can_rec = can_recast;
    QList<int> sub;
    if (isVirtualCard())
        sub = subcards;
    else
        sub << getEffectiveId();
    if (sub.isEmpty() || sub.contains(-1))
        can_rec = false;
    return (can_use && !player->isCardLimited(this, Card::MethodUse))
            || (can_rec && !player->isCardLimited(this, Card::MethodRecast));
}

bool KnownBoth::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->isCardLimited(this, Card::MethodUse))
        return false;

    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num || to_select == Self)
        return false;

    return !to_select->isKongcheng() || !to_select->hasShownAllGenerals();
}

bool KnownBoth::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    bool rec = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY);
    QList<int> sub;
    if (isVirtualCard())
        sub = subcards;
    else
        sub << getEffectiveId();
    foreach (int id, sub) {
        if (Self->getHandPile().contains(id)) {
            rec = false;
            break;
        }
    }

    if (rec && Self->isCardLimited(this, Card::MethodUse))
        return targets.length() == 0;
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() > total_num)
        return false;
    return rec || targets.length() > 0;
}

void KnownBoth::onUse(Room *room, const CardUseStruct &card_use) const
{
    if (card_use.to.isEmpty()) {
        CardMoveReason reason(CardMoveReason::S_REASON_RECAST, card_use.from->objectName());
        reason.m_skillName = getSkillName();
        room->moveCardTo(this, card_use.from, NULL, Player::PlaceTable, reason);
        card_use.from->broadcastSkillInvoke("@recast");

        LogMessage log;
        log.type = "#Card_Recast";
        log.from = card_use.from;
        log.card_str = card_use.card->toString();
        room->sendLog(log);

        QString skill_name = card_use.card->showSkill();
        if (!skill_name.isNull() && card_use.from->ownSkill(skill_name) && !card_use.from->hasShownSkill(skill_name))
            card_use.from->showGeneral(card_use.from->inHeadSkills(skill_name));

        QList<int> table_cardids = room->getCardIdsOnTable(this);
        if (!table_cardids.isEmpty()) {
            DummyCard dummy(table_cardids);
            room->moveCardTo(&dummy, card_use.from, NULL, Player::DiscardPile, reason, true);
        }

        card_use.from->drawCards(1);
    } else
        SingleTargetTrick::onUse(room, card_use);
}

void KnownBoth::onEffect(const CardEffectStruct &effect) const
{
    QStringList choices;
    if (!effect.to->isKongcheng())
        choices << "handcards";
    if (!effect.to->hasShownGeneral1())
        choices << "head_general";
    if (effect.to->getGeneral2() && !effect.to->hasShownGeneral2())
        choices << "deputy_general";

    Room *room = effect.from->getRoom();

    effect.to->setFlags("KnownBothTarget");// For AI
    QString choice = room->askForChoice(effect.from, objectName(),
                                        choices.join("+"), QVariant::fromValue(effect.to));
    effect.to->setFlags("-KnownBothTarget");
    LogMessage log;
    log.type = "#KnownBothView";
    log.from = effect.from;
    log.to << effect.to;
    log.arg = choice;
    foreach (ServerPlayer *p, room->getOtherPlayers(effect.from, true)) {
        room->doNotify(p, QSanProtocol::S_COMMAND_LOG_SKILL, log.toVariant());
    }

    if (choice == "handcards")
        room->showAllCards(effect.to, effect.from);
    else {
        QStringList list = room->getTag(effect.to->objectName()).toStringList();
        list.removeAt(choice == "head_general" ? 1 : 0);
        foreach (const QString &name, list) {
            LogMessage log;
            log.type = "$KnownBothViewGeneral";
            log.from = effect.from;
            log.to << effect.to;
            log.arg = name;
            log.arg2 = choice;
            room->doNotify(effect.from, QSanProtocol::S_COMMAND_LOG_SKILL, log.toVariant());
        }
        JsonArray arg;
        arg << objectName();
        arg << JsonUtils::toJsonArray(list);
        room->doNotify(effect.from, QSanProtocol::S_COMMAND_VIEW_GENERALS, arg);
    }
}

QStringList KnownBoth::checkTargetModSkillShow(const CardUseStruct &use) const
{
    if (use.card == NULL)
        return QStringList();

    if (use.to.length() >= 2) {
        const ServerPlayer *from = use.from;
        QList<const Skill *> skills = from->getSkillList(false, false);
        QList<const TargetModSkill *> tarmods;

        foreach (const Skill *skill, skills) {
            if (from->hasSkill(skill) && skill->inherits("TargetModSkill")) {
                const TargetModSkill *tarmod = qobject_cast<const TargetModSkill *>(skill);
                tarmods << tarmod;
            }
        }

        if (tarmods.isEmpty())
            return QStringList();

        int n = use.to.length() - 1;
        QList<const TargetModSkill *> tarmods_copy = tarmods;

        foreach (const TargetModSkill *tarmod, tarmods_copy) {
            if (tarmod->getExtraTargetNum(from, use.card) == 0) {
                tarmods.removeOne(tarmod);
                continue;
            }

            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            if (from->hasShownSkill(main_skill)) {
                tarmods.removeOne(tarmod);
                n -= tarmod->getExtraTargetNum(from, use.card);
            }
        }

        if (tarmods.isEmpty() || n <= 0)
            return QStringList();

        tarmods_copy = tarmods;

        QStringList shows;
        foreach (const TargetModSkill *tarmod, tarmods_copy) {
            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            shows << main_skill->objectName();
        }
        return shows;
    }
    return QStringList();
}

BefriendAttacking::BefriendAttacking(Card::Suit suit, int number) : SingleTargetTrick(suit, number)
{
    setObjectName("befriend_attacking");
}

bool BefriendAttacking::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num)
        return false;

    return to_select->hasShownOneGeneral() && !Self->isFriendWith(to_select);
}

void BefriendAttacking::onEffect(const CardEffectStruct &effect) const
{
    effect.to->drawCards(1);
    effect.from->drawCards(3);
}

bool BefriendAttacking::isAvailable(const Player *player) const
{
    return player->hasShownOneGeneral() && TrickCard::isAvailable(player);
}

QStringList BefriendAttacking::checkTargetModSkillShow(const CardUseStruct &use) const
{
    if (use.card == NULL)
        return QStringList();

    if (use.to.length() >= 2) {
        const ServerPlayer *from = use.from;
        QList<const Skill *> skills = from->getSkillList(false, false);
        QList<const TargetModSkill *> tarmods;

        foreach (const Skill *skill, skills) {
            if (from->hasSkill(skill) && skill->inherits("TargetModSkill")) {
                const TargetModSkill *tarmod = qobject_cast<const TargetModSkill *>(skill);
                tarmods << tarmod;
            }
        }

        if (tarmods.isEmpty())
            return QStringList();

        int n = use.to.length() - 1;
        QList<const TargetModSkill *> tarmods_copy = tarmods;

        foreach (const TargetModSkill *tarmod, tarmods_copy) {
            if (tarmod->getExtraTargetNum(from, use.card) == 0) {
                tarmods.removeOne(tarmod);
                continue;
            }

            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            if (from->hasShownSkill(main_skill)) {
                tarmods.removeOne(tarmod);
                n -= tarmod->getExtraTargetNum(from, use.card);
            }
        }

        if (tarmods.isEmpty() || n <= 0)
            return QStringList();

        tarmods_copy = tarmods;

        QStringList shows;
        foreach (const TargetModSkill *tarmod, tarmods_copy) {
            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            shows << main_skill->objectName();
        }
        return shows;
    }
    return QStringList();
}


Breastplate::Breastplate(Card::Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("Breastplate");
}

class BreastplateSkill : public ArmorSkill
{
public:
    BreastplateSkill() : ArmorSkill("Breastplate")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }
    /*

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (ArmorSkill::triggerable(player) && damage.damage >= player->getHp() && player->getArmor())
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        return player->askForSkillInvoke(this);
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
        room->moveCardTo(player->getArmor(), NULL, Player::DiscardPile, reason, true);
        DamageStruct damage = data.value<DamageStruct>();
        LogMessage log;
        log.type = "#Breastplate";
        log.from = player;
        if (damage.from)
            log.to << damage.from;
        log.arg = QString::number(damage.damage);
        if (damage.nature == DamageStruct::Normal)
            log.arg2 = "normal_nature";
        else if (damage.nature == DamageStruct::Fire)
            log.arg2 = "fire_nature";
        else if (damage.nature == DamageStruct::Thunder)
            log.arg2 = "thunder_nature";
        room->sendLog(log);
        return true;
    }*/
};

IronArmor::IronArmor(Card::Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("IronArmor");
}

class IronArmorSkill : public ArmorSkill
{
public:
    IronArmorSkill() : ArmorSkill("IronArmor")
    {
        events << TargetConfirming;
        frequency = Compulsory;
    }
    /*

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!ArmorSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card) return QStringList();
        if (!use.to.contains(player) || player->getMark("Equips_of_Others_Nullified_to_You") > 0) return QStringList();
        if (use.card->isKindOf("FireAttack") || use.card->isKindOf("FireSlash") || use.card->isKindOf("BurningCamps"))
            return QStringList(objectName());
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        LogMessage log2;
        log2.type = "#IronArmor";
        log2.from = player;
        log2.arg = objectName();
        room->sendLog(log2);

        room->cancelTarget(use, player); // Room::cancelTarget(use, player);

        data = QVariant::fromValue(use);
        return false;
    }*/
};

JadeSeal::JadeSeal(Card::Suit suit, int number)
    : Treasure(suit, number)
{
    setObjectName("JadeSeal");
}

class JadeSealViewAsSkill : public ZeroCardViewAsSkill
{
public:
    JadeSealViewAsSkill() : ZeroCardViewAsSkill("JadeSeal")
    {
        response_pattern = "@@JadeSeal!";
    }

    virtual const Card *viewAs() const
    {/*
        KnownBoth *kb = new KnownBoth(Card::NoSuit, 0);
        kb->setSkillName(objectName());
        return kb;*/
    }
};

class JadeSealSkill : public TreasureSkill
{
public:
    JadeSealSkill() : TreasureSkill("JadeSeal")
    {
        events << DrawNCards << EventPhaseStart;
        view_as_skill = new JadeSealViewAsSkill;
    }
    /*

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TreasureSkill::triggerable(player) || !player->hasShownOneGeneral())
            return QStringList();
        if (triggerEvent == DrawNCards) {
            return QStringList(objectName());
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            KnownBoth *kb = new KnownBoth(Card::NoSuit, 0);
            kb->setSkillName(objectName());
            kb->deleteLater();
            if (kb->isAvailable(player))
                return QStringList(objectName());
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (triggerEvent == DrawNCards)
            return true;
        if (!room->askForUseCard(player, "@@JadeSeal!", "@JadeSeal")) {
            KnownBoth *kb = new KnownBoth(Card::NoSuit, 0);
            kb->setSkillName(objectName());
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!player->isProhibited(p, kb) && (!p->isKongcheng() || !p->hasShownAllGenerals()))
                    targets << p;
            }
            if (targets.isEmpty()) {
                delete kb;
            } else {
                ServerPlayer *target = targets.at(qrand() % targets.length());
                room->useCard(CardUseStruct(kb, player, target), false);
            }
        }
        return false;
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *) const
    {
        data = data.toInt() + 1;
        return false;
    }*/
};

Drowning::Drowning(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("drowning");
}

bool Drowning::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num)
        return false;

    return to_select->hasEquip() && to_select != Self;
}

void Drowning::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    if (!effect.to->getEquips().isEmpty()
            && room->askForChoice(effect.to, objectName(), "throw+damage", QVariant::fromValue(effect)) == "throw")
        effect.to->throwAllEquips();
    else
        room->damage(DamageStruct(this, effect.from->isAlive() ? effect.from : NULL, effect.to, 1, DamageStruct::Thunder));
}

bool Drowning::isAvailable(const Player *player) const
{
    bool canUse = false;
    QList<const Player *> players = player->getAliveSiblings();
    foreach (const Player *p, players) {
        if (player->isProhibited(p, this))
            continue;
        if (!p->hasEquip())
            continue;
        canUse = true;
        break;
    }

    return canUse && TrickCard::isAvailable(player);
}

BurningCamps::BurningCamps(Card::Suit suit, int number)
    : AOE(suit, number)
{
    setObjectName("burning_camps");
}

bool BurningCamps::isAvailable(const Player *player) const
{/*
    bool canUse = false;
    QList<const Player *> players = player->getNextAlive()->getFormation();
    foreach (const Player *p, players) {
        if (player->isProhibited(p, this))
            continue;

        canUse = true;
        break;
    }

    return canUse && TrickCard::isAvailable(player);*/
}

void BurningCamps::onUse(Room *room, const CardUseStruct &card_use) const
{/*
    CardUseStruct new_use = card_use;
    QList<const Player *> targets = card_use.from->getNextAlive()->getFormation();
    foreach (const Player *player, targets) {
        const Skill *skill = room->isProhibited(card_use.from, player, this);
        ServerPlayer *splayer = room->findPlayer(player->objectName());
        if (skill) {
            if (!skill->isVisible())
                skill = Sanguosha->getMainSkill(skill->objectName());
            if (skill->isVisible()) {
                LogMessage log;
                log.type = "#SkillAvoid";
                log.from = splayer;
                log.arg = skill->objectName();
                log.arg2 = objectName();
                room->sendLog(log);

                room->broadcastSkillInvoke(skill->objectName());
            }
        } else
            new_use.to << splayer;
    }

    TrickCard::onUse(room, new_use);*/
}

void BurningCamps::onEffect(const CardEffectStruct &effect) const
{
    effect.to->getRoom()->damage(DamageStruct(this, effect.from, effect.to, 1, DamageStruct::Fire));
}

LureTiger::LureTiger(Card::Suit suit, int number)
    : TrickCard(suit, number)
{
    setObjectName("lure_tiger");
}

QString LureTiger::getSubtype() const
{
    return "lure_tiger";
}

bool LureTiger::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 2 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num)
        return false;
    if (Self->isCardLimited(this, Card::MethodUse))
        return false;

    return to_select != Self;
}

void LureTiger::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QStringList nullified_list = room->getTag("CardUseNullifiedList").toStringList();
    bool all_nullified = nullified_list.contains("_ALL_TARGETS");
    foreach (ServerPlayer *target, targets) {
        CardEffectStruct effect;
        effect.card = this;
        effect.from = source;
        effect.to = target;
        effect.multiple = (targets.length() > 1);
        effect.nullified = (all_nullified || nullified_list.contains(target->objectName()));

        QVariantList players;
        for (int i = targets.indexOf(target); i < targets.length(); i++) {
            if (!nullified_list.contains(targets.at(i)->objectName()) && !all_nullified)
                players.append(QVariant::fromValue(targets.at(i)));
        }
        room->setTag("targets" + this->toString(), QVariant::fromValue(players));

        room->cardEffect(effect);
    }

    room->removeTag("targets" + this->toString());

    source->drawCards(1, objectName());
    /*

    QList<int> table_cardids = room->getCardIdsOnTable(this);
    if (!table_cardids.isEmpty()) {
        DummyCard dummy(table_cardids);
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), this->getSkillName(), QString());
        if (targets.size() == 1) reason.m_targetId = targets.first()->objectName();
        room->moveCardTo(&dummy, source, NULL, Player::DiscardPile, reason, true);
    }*/
}

void LureTiger::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    room->setPlayerCardLimitation(effect.to, "use", ".", false);
    room->setPlayerProperty(effect.to, "removed", true);
    effect.from->setFlags("LureTigerUser");
}

class LureTigerSkill : public TriggerSkill
{
public:
    LureTigerSkill() : TriggerSkill("lure_tiger_effect")
    {
        events << Death << EventPhaseChanging;
        global = true;
    }
    /*

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!player->hasFlag("LureTigerUser"))
            return QStringList();
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return QStringList();
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return QStringList();
        }

        foreach(ServerPlayer *p, room->getOtherPlayers(player))
            if (p->isRemoved()) {
                room->setPlayerProperty(p, "removed", false);
                room->removePlayerCardLimitation(p, "use", ".$0");
            }

        return QStringList();
    }*/
};

class LureTigerProhibit : public ProhibitSkill
{
public:
    LureTigerProhibit() : ProhibitSkill("#lure_tiger-prohibit")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {/*
        return to->isRemoved() && card->getTypeId() != Card::TypeSkill;*/
    }
};

FightTogether::FightTogether(Card::Suit suit, int number)
    : GlobalEffect(suit, number)
{
    setObjectName("fight_together");
    can_recast = true;
}

bool FightTogether::isAvailable(const Player *player) const
{
    if (player->hasFlag("Global_FightTogetherFailed"))
        return false;
    bool rec = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) && can_recast;
    QList<int> sub;
    if (isVirtualCard())
        sub = subcards;
    else
        sub << getEffectiveId();
    foreach (int id, sub) {
        if (player->getPile("wooden_ox").contains(id)) {
            rec = false;
            break;
        }
    }

    if (rec && !player->isCardLimited(this, Card::MethodRecast))
        return true;/*
    QStringList big_kingdoms = player->getBigKingdoms(objectName());
    return (!big_kingdoms.isEmpty() || (player->hasLordSkill("hongfa") && !player->getPile("heavenly_army").isEmpty())) // HongfaTianbing
        && GlobalEffect::isAvailable(player);*/
}

void FightTogether::onUse(Room *room, const CardUseStruct &card_use) const
{/*
    ServerPlayer *source = card_use.from;
    QStringList big_kingdoms = source->getBigKingdoms(objectName(), MaxCardsType::Normal);
    if (big_kingdoms.isEmpty()) {
        if (!source->isCardLimited(this, Card::MethodRecast)) {
            CardMoveReason reason(CardMoveReason::S_REASON_RECAST, card_use.from->objectName());
            reason.m_skillName = getSkillName();
            room->moveCardTo(this, card_use.from, NULL, Player::PlaceTable, reason, true);
            card_use.from->broadcastSkillInvoke("@recast");

            LogMessage log;
            log.type = "#Card_Recast";
            log.from = card_use.from;
            log.card_str = card_use.card->toString();
            room->sendLog(log);

            QString skill_name = card_use.card->showSkill();
            if (!skill_name.isNull() && card_use.from->ownSkill(skill_name) && !card_use.from->hasShownSkill(skill_name))
                card_use.from->showGeneral(card_use.from->inHeadSkills(skill_name));

            QList<int> table_cardids = room->getCardIdsOnTable(this);
            if (!table_cardids.isEmpty()) {
                DummyCard dummy(table_cardids);
                room->moveCardTo(&dummy, card_use.from, NULL, Player::DiscardPile, reason, true);
            }

            card_use.from->drawCards(1);
            return;
        } else
            room->setPlayerFlag(source, "Global_FightTogetherFailed");
        return;
    }
    QList<ServerPlayer *> bigs, smalls;
    foreach (ServerPlayer *p, room->getAllPlayers()) {
        const Skill *skill = room->isProhibited(source, p, this);
        if (skill) {
            if (!skill->isVisible())
                skill = Sanguosha->getMainSkill(skill->objectName());
            if (skill->isVisible()) {
                LogMessage log;
                log.type = "#SkillAvoid";
                log.from = p;
                log.arg = skill->objectName();
                log.arg2 = objectName();
                room->sendLog(log);

                room->broadcastSkillInvoke(skill->objectName());
            }
            continue;
        }
        QString kingdom = p->objectName();
        if (big_kingdoms.length() == 1 && big_kingdoms.first().startsWith("sgs")) { // for JadeSeal
            if (big_kingdoms.contains(kingdom))
                bigs << p;
            else
                smalls << p;
        } else {
            if (!p->hasShownOneGeneral()) {
                smalls << p;
                continue;
            }
            if (p->getRole() == "careerist")
                kingdom = "careerist";
            else
                kingdom = p->getKingdom();
            if (big_kingdoms.contains(kingdom))
                bigs << p;
            else
                smalls << p;
        }
    }
    QStringList choices;
    if (!bigs.isEmpty())
        choices << "big";
    if (!smalls.isEmpty())
        choices << "small";
    if (!source->isCardLimited(this, Card::MethodRecast))
        choices << "recast";

    Q_ASSERT(!choices.isEmpty());

    QString choice = room->askForChoice(source, objectName(), choices.join("+"));
    if (choice == "recast") {
        CardMoveReason reason(CardMoveReason::S_REASON_RECAST, card_use.from->objectName());
        reason.m_skillName = getSkillName();
        room->moveCardTo(this, card_use.from, NULL, Player::PlaceTable, reason, true);
        card_use.from->broadcastSkillInvoke("@recast");

        LogMessage log;
        log.type = "#Card_Recast";
        log.from = card_use.from;
        log.card_str = card_use.card->toString();
        room->sendLog(log);

        QString skill_name = card_use.card->showSkill();
        if (!skill_name.isNull() && card_use.from->ownSkill(skill_name) && !card_use.from->hasShownSkill(skill_name))
            card_use.from->showGeneral(card_use.from->inHeadSkills(skill_name));

        QList<int> table_cardids = room->getCardIdsOnTable(this);
        if (!table_cardids.isEmpty()) {
            DummyCard dummy(table_cardids);
            room->moveCardTo(&dummy, card_use.from, NULL, Player::DiscardPile, reason, true);
        }

        card_use.from->drawCards(1);
        return;
    }

    CardUseStruct use = card_use;
    if (choice == "big")
        use.to = bigs;
    else if (choice == "small")
        use.to = smalls;
    Q_ASSERT(!use.to.isEmpty());
    TrickCard::onUse(room, use);*/
}

void FightTogether::onEffect(const CardEffectStruct &effect) const
{/*
    Room *room = effect.from->getRoom();
    if (!effect.to->isChained()) {
        if (!effect.to->canBeChainedBy(effect.from))
            return;
        effect.to->setChained(true);
        room->setEmotion(effect.to, "chain");
        room->broadcastProperty(effect.to, "chained");
        room->getThread()->trigger(ChainStateChanged, room, effect.to);
    } else
        effect.to->drawCards(1);*/
}

AllianceFeast::AllianceFeast(Card::Suit suit, int number)
    : AOE(suit, number)
{
    setObjectName("alliance_feast");
    target_fixed = false;
}

bool AllianceFeast::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty())
        return false;/*
    return to_select->hasShownOneGeneral() && !Self->isFriendWith(to_select);*/
}

void AllianceFeast::onUse(Room *room, const CardUseStruct &card_use) const
{/*
    ServerPlayer *source = card_use.from;
    QList<ServerPlayer *> targets;
    if (!source->isProhibited(source, this))
        targets << source;
    if (card_use.to.length() == 1) {
        ServerPlayer *target = card_use.to.first();
        QList<ServerPlayer *> other_players = room->getOtherPlayers(source);
        foreach (ServerPlayer *player, other_players) {
            if (!target->isFriendWith(player))
                continue;
            const Skill *skill = room->isProhibited(source, player, this);
            if (skill) {
                if (!skill->isVisible())
                    skill = Sanguosha->getMainSkill(skill->objectName());
                if (skill->isVisible()) {
                    LogMessage log;
                    log.type = "#SkillAvoid";
                    log.from = player;
                    log.arg = skill->objectName();
                    log.arg2 = objectName();
                    room->sendLog(log);

                    room->broadcastSkillInvoke(skill->objectName());
                }
            } else
                targets << player;
        }
    } else
        targets = card_use.to;

    CardUseStruct use = card_use;
    use.to = targets;
    TrickCard::onUse(room, use);*/
}

void AllianceFeast::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{/*
    QStringList nullified_list = room->getTag("CardUseNullifiedList").toStringList();
    bool all_nullified = nullified_list.contains("_ALL_TARGETS");
    foreach (ServerPlayer *target, targets) {
        CardEffectStruct effect;
        effect.card = this;
        effect.from = source;
        effect.to = target;
        effect.multiple = (targets.length() > 1);
        effect.nullified = (all_nullified || nullified_list.contains(target->objectName()));

        QVariantList players;
        for (int i = targets.indexOf(target); i < targets.length(); i++) {
            if (!nullified_list.contains(targets.at(i)->objectName()) && !all_nullified)
                players.append(QVariant::fromValue(targets.at(i)));
        }
        room->setTag("targets" + this->toString(), QVariant::fromValue(players));

        if (target == source) {
            int n = 0;
            ServerPlayer *enemy = targets.last();
            foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
                if (enemy->isFriendWith(p))
                    ++n;
            }
            target->setMark(objectName(), n);
        }
        room->cardEffect(effect);
        target->setMark(objectName(), 0);
    }

    room->removeTag("targets" + this->toString());

    QList<int> table_cardids = room->getCardIdsOnTable(this);
    if (!table_cardids.isEmpty()) {
        DummyCard dummy(table_cardids);
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), this->getSkillName(), QString());
        room->moveCardTo(&dummy, source, NULL, Player::DiscardPile, reason, true);
    }*/
}

void AllianceFeast::onEffect(const CardEffectStruct &effect) const
{/*
    Room *room = effect.to->getRoom();
    if (effect.to->getMark(objectName()) > 0) {
        effect.to->drawCards(effect.to->getMark(objectName()), objectName());
    } else {
        QStringList choices;
        if (effect.to->isWounded())
            choices << "recover";
        choices << "draw";
        QString choice = room->askForChoice(effect.to, objectName(), choices.join("+"));
        if (choice == "recover") {
            RecoverStruct recover;
            recover.who = effect.from;
            room->recover(effect.to, recover);
        } else {
            effect.to->drawCards(1, objectName());
            if (effect.to->isChained()) {
                effect.to->setChained(false);
                room->setEmotion(effect.to, "chain");
                room->broadcastProperty(effect.to, "chained");
                room->getThread()->trigger(ChainStateChanged, room, effect.to);
            }
        }
    }*/
}

bool AllianceFeast::isAvailable(const Player *player) const
{/*
    return player->hasShownOneGeneral() && !player->isProhibited(player, this);*/
}

ThreatenEmperor::ThreatenEmperor(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("threaten_emperor");
    target_fixed = true;
}

void ThreatenEmperor::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;
    SingleTargetTrick::onUse(room, use);
}

bool ThreatenEmperor::isAvailable(const Player *player) const
{/*
    if (!player->hasShownOneGeneral())
        return false;
    QStringList big_kingdoms = player->getBigKingdoms(objectName(), MaxCardsType::Max);
    bool invoke = !big_kingdoms.isEmpty();
    if (invoke) {
        if (big_kingdoms.length() == 1 && big_kingdoms.first().startsWith("sgs")) // for JadeSeal
            invoke = big_kingdoms.contains(player->objectName());
        else if (player->getRole() == "careerist")
            invoke = false;
        else
            invoke = big_kingdoms.contains(player->getKingdom());
    }
    return invoke && !player->isProhibited(player, this) && TrickCard::isAvailable(player);*/
}

void ThreatenEmperor::onEffect(const CardEffectStruct &effect) const
{
    if (effect.from->getPhase() == Player::Play)
        effect.from->setFlags("Global_PlayPhaseTerminated");
    effect.to->setMark("ThreatenEmperorExtraTurn", 1);
}

class ThreatenEmperorSkill : public TriggerSkill
{
public:
    ThreatenEmperorSkill() : TriggerSkill("threaten_emperor")
    {
        events << EventPhaseChanging;
        global = true;
    }
    /*

    virtual int getPriority() const
    {
        return 1;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        TriggerList list;
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return list;
        foreach(ServerPlayer *p, room->getAllPlayers())
            if (p->getMark("ThreatenEmperorExtraTurn") > 0)
                list.insert(p, QStringList(objectName()));

        return list;
    }

    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *ask_who) const
    {
        ask_who->removeMark("ThreatenEmperorExtraTurn");
        return room->askForCard(ask_who, "..", "@threaten_emperor", data, objectName());
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *ask_who) const
    {

        LogMessage l;
        l.type = "#Fangquan";
        l.to << ask_who;
        room->sendLog(l);

        ask_who->gainAnExtraTurn();
        return false;
    }*/
};

ImperialOrder::ImperialOrder(Suit suit, int number)
    : GlobalEffect(suit, number)
{
    setObjectName("imperial_order");
}

bool ImperialOrder::isAvailable(const Player *player) const
{/*
    bool invoke = !player->hasShownOneGeneral();
    if (!invoke) {
        foreach (const Player *p, player->getAliveSiblings()) {
            if (!p->hasShownOneGeneral() && !player->isProhibited(p, this)) {
                invoke = true;
                break;
            }
        }
    }
    return invoke && TrickCard::isAvailable(player);*/
}

void ImperialOrder::onUse(Room *room, const CardUseStruct &card_use) const
{/*
    ServerPlayer *source = card_use.from;
    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getAllPlayers()) {
        if (p->hasShownOneGeneral())
            continue;
        const Skill *skill = room->isProhibited(source, p, this);
        if (skill) {
            if (!skill->isVisible())
                skill = Sanguosha->getMainSkill(skill->objectName());
            if (skill && skill->isVisible()) {
                LogMessage log;
                log.type = "#SkillAvoid";
                log.from = p;
                log.arg = skill->objectName();
                log.arg2 = objectName();
                room->sendLog(log);

                room->broadcastSkillInvoke(skill->objectName());
            }
            continue;
        }
        targets << p;
    }

    CardUseStruct use = card_use;
    use.to = targets;
    Q_ASSERT(!use.to.isEmpty());
    TrickCard::onUse(room, use);*/
}

void ImperialOrder::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{/*
    room->setCardFlag(this, "imperial_order_normal_use");
    GlobalEffect::use(room, source, targets);*/
}

void ImperialOrder::onEffect(const CardEffectStruct &effect) const
{/*
    Room *room = effect.to->getRoom();
    if (room->askForCard(effect.to, "EquipCard", "@imperial_order-equip"))
        return;
    QStringList choices;
    if (!effect.to->hasShownAllGenerals()
        && ((!effect.to->hasShownGeneral1() && effect.to->disableShow(true).isEmpty())
        || (effect.to->getGeneral2() && !effect.to->hasShownGeneral2() && effect.to->disableShow(false).isEmpty())))
        choices << "show";
    choices << "losehp";
    QString choice = room->askForChoice(effect.to, objectName(), choices.join("+"));
    if (choice == "show") {
        effect.to->askForGeneralShow();
        effect.to->drawCards(1, objectName());
    } else {
        room->loseHp(effect.to);
    }*/
}
#endif
HegemonyCardsPackage::HegemonyCardsPackage()
    : Package("strategic_advantage", Package::CardPack)
{
#if 0
    QList<Card *> cards;

    cards
            // basics
            // -- spade
            << new Slash(Card::Spade, 4)
            << new Analeptic(Card::Spade, 6) // transfer
            << new Slash(Card::Spade, 7)
            << new Slash(Card::Spade, 8)
            << new ThunderSlash(Card::Spade, 9)
            << new ThunderSlash(Card::Spade, 10)
            << new ThunderSlash(Card::Spade, 11) // transfer
               // -- heart
            << new Jink(Card::Heart, 4)
            << new Jink(Card::Heart, 5)
            << new Jink(Card::Heart, 6)
            << new Jink(Card::Heart, 7)
            << new Peach(Card::Heart, 8)
            << new Peach(Card::Heart, 9)
            << new Slash(Card::Heart, 10)
            << new Slash(Card::Heart, 11)
               // -- club
            << new Slash(Card::Club, 4)
            << new ThunderSlash(Card::Club, 5) // transfer
            << new Slash(Card::Club, 6)
            << new Slash(Card::Club, 7)
            << new Slash(Card::Club, 8)
            << new Analeptic(Card::Club, 9)
               // -- diamond
            << new Peach(Card::Diamond, 2)
            << new Peach(Card::Diamond, 3) // transfer
            << new Jink(Card::Diamond, 6)
            << new Jink(Card::Diamond, 7)
            << new FireSlash(Card::Diamond, 8)
            << new FireSlash(Card::Diamond, 9)
            << new Jink(Card::Diamond, 13)

               // tricks
               // -- spade
            << new ThreatenEmperor(Card::Spade, 1) // transfer
            << new BurningCamps(Card::Spade, 3) // transfer
            << new FightTogether(Card::Spade, 12)
            << new Nullification(Card::Spade, 13)
               // -- heart
            << new AllianceFeast()
            << new LureTiger(Card::Heart, 2)
            << new BurningCamps(Card::Heart, 12) //transfer
            << new Drowning(Card::Heart, 13)
               // -- club
            << new ImperialOrder(Card::Club, 3)
            << new FightTogether(Card::Club, 10)
            << new BurningCamps(Card::Club, 11) //transfer
            << new Drowning(Card::Club, 12)
               // -- diamond
            << new ThreatenEmperor(Card::Diamond, 1) // transfer
            << new ThreatenEmperor(Card::Diamond, 4) // transfer
            << new LureTiger(Card::Diamond, 10) // transfer

               // equips
            << new IronArmor();
    Horse *horse = new OffensiveHorse(Card::Heart, 3, -1); // transfer
    horse->setObjectName("JingFan");
    cards
            << horse
            << new JadeSeal(Card::Club, 1)
            << new Breastplate(); // transfer

    skills << new IronArmorSkill
           << new JadeSealSkill
           << new BreastplateSkill
           << new LureTigerSkill << new LureTigerProhibit
           << new ThreatenEmperorSkill;
    insertRelatedSkills("lure_tiger_effect", "#lure_tiger-prohibit");

    foreach(Card *card, cards)
        card->setParent(this);
#endif
}

ADD_PACKAGE(HegemonyCards)
