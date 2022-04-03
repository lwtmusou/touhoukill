#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "exppattern.h"
#include "maneuvering.h"
#include "room.h"
#include "serverplayer.h"
#include "skill.h"

QString BasicCard::getType() const
{
    return "basic";
}

Card::CardType BasicCard::getTypeId() const
{
    return TypeBasic;
}

TrickCard::TrickCard(Suit suit, int number)
    : Card(suit, number)
    , cancelable(true)
{
    handling_method = Card::MethodUse;
}

void TrickCard::setCancelable(bool cancelable)
{
    this->cancelable = cancelable;
}

QString TrickCard::getType() const
{
    return "trick";
}

Card::CardType TrickCard::getTypeId() const
{
    return TypeTrick;
}

bool TrickCard::isCancelable(const CardEffectStruct &effect) const
{
    Q_UNUSED(effect);
    return cancelable;
}

QString EquipCard::getType() const
{
    return "equip";
}

Card::CardType EquipCard::getTypeId() const
{
    return TypeEquip;
}

bool EquipCard::isAvailable(const Player *player) const
{
    bool ignore = (player->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !hasFlag("IgnoreFailed"));
    if (ignore)
        return true;
    return !player->isProhibited(player, this) && Card::isAvailable(player);
}

bool EquipCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    bool ignore = (Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && to_select != Self && !hasFlag("IgnoreFailed"));
    if (ignore)
        return targets.isEmpty();
    if (Self->hasFlag("Global_shehuoInvokerFailed"))
        return targets.isEmpty() && to_select->hasFlag("Global_shehuoFailed");
    return targets.isEmpty() && to_select == Self;
}

void EquipCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *player = use.from;
    if (use.to.isEmpty())
        use.to << player;

    if (!use.to.contains(player)) {
        LogMessage log;
        log.from = player;
        log.to = use.to;
        log.type = "#UseCard";
        log.card_str = use.card->toString();
        room->sendLog(log);
    }

    QVariant data = QVariant::fromValue(use);
    RoomThread *thread = room->getThread();
    thread->trigger(PreCardUsed, room, data);

    CardMoveReason reason(CardMoveReason::S_REASON_USE, player->objectName(), QString(), card_use.card->getSkillName(), QString());
    CardsMoveStruct move(card_use.card->getEffectiveId(), NULL, Player::PlaceTable, reason);
    room->moveCardsAtomic(move, true);

    thread->trigger(CardUsed, room, data);
    thread->trigger(CardFinished, room, data);
}

void EquipCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    if (targets.isEmpty()) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), getSkillName(), QString());
        room->moveCardTo(this, source, NULL, Player::DiscardPile, reason, true);
        return;
    }
    int equipped_id = Card::S_UNKNOWN_CARD_ID;
    ServerPlayer *target = targets.first();
    if (target->getEquip(location()))
        equipped_id = target->getEquip(location())->getEffectiveId();

    QList<CardsMoveStruct> exchangeMove;
    CardsMoveStruct move1(getEffectiveId(), target, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_USE, target->objectName()));
    exchangeMove.push_back(move1);
    if (equipped_id != Card::S_UNKNOWN_CARD_ID) {
        CardsMoveStruct move2(equipped_id, NULL, Player::DiscardPile, CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, target->objectName()));
        exchangeMove.push_back(move2);
    }
    LogMessage log;
    log.from = target;
    log.type = "$Install";
    log.card_str = QString::number(getEffectiveId());
    room->sendLog(log);

    room->moveCardsAtomic(exchangeMove, true);
}

void EquipCard::onInstall(ServerPlayer *player) const
{
    Room *room = player->getRoom();

    const Skill *skill = Sanguosha->getSkill(this);
    if (skill) {
        if (skill->inherits("ViewAsSkill")) {
            room->attachSkillToPlayer(player, objectName());
        } else if (skill->inherits("TriggerSkill")) {
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
            room->getThread()->addTriggerSkill(trigger_skill);

            if (trigger_skill->getViewAsSkill())
                room->attachSkillToPlayer(player, skill->objectName());
        }
    }
}

void EquipCard::onUninstall(ServerPlayer *player) const
{
    Room *room = player->getRoom();
    const Skill *skill = Sanguosha->getSkill(this);

    if (skill && (skill->inherits("ViewAsSkill") || (skill->inherits("TriggerSkill") && qobject_cast<const TriggerSkill *>(skill)->getViewAsSkill())))
        room->detachSkillFromPlayer(player, objectName(), true);
}

QString GlobalEffect::getSubtype() const
{
    return "global_effect";
}

void GlobalEffect::onUse(Room *room, const CardUseStruct &card_use) const
{
    if (!card_use.to.isEmpty()) {
        TrickCard::onUse(room, card_use);
        return;
    }

    ServerPlayer *source = card_use.from;
    QList<ServerPlayer *> targets, all_players = room->getAllPlayers();
    foreach (ServerPlayer *player, all_players) {
        const ProhibitSkill *skill = room->isProhibited(source, player, this);
        if (skill) {
            LogMessage log;
            log.type = "#SkillAvoid";
            log.from = player;
            log.arg = skill->objectName();
            log.arg2 = objectName();
            room->sendLog(log);

            room->broadcastSkillInvoke(skill->objectName());
        } else
            targets << player;
    }

    CardUseStruct use = card_use;
    use.to = targets;
    TrickCard::onUse(room, use);
}

bool GlobalEffect::isAvailable(const Player *player) const
{
    bool canUse = false;
    QList<const Player *> players = player->getAliveSiblings();
    players << player;
    foreach (const Player *p, players) {
        if (player->isProhibited(p, this))
            continue;

        canUse = true;
        break;
    }

    return canUse && TrickCard::isAvailable(player);
}

QString AOE::getSubtype() const
{
    return "aoe";
}

bool AOE::isAvailable(const Player *player) const
{
    bool canUse = false;
    QList<const Player *> players = player->getAliveSiblings();
    foreach (const Player *p, players) {
        if (player->isProhibited(p, this))
            continue;

        canUse = true;
        break;
    }

    return canUse && TrickCard::isAvailable(player);
}

void AOE::onUse(Room *room, const CardUseStruct &card_use) const
{
    if (!card_use.to.isEmpty()) {
        TrickCard::onUse(room, card_use);
        return;
    }

    ServerPlayer *source = card_use.from;
    QList<ServerPlayer *> targets, other_players = room->getOtherPlayers(source);
    foreach (ServerPlayer *player, other_players) {
        const ProhibitSkill *skill = room->isProhibited(source, player, this);
        if (skill) {
            LogMessage log;
            log.type = "#SkillAvoid";
            log.from = player;
            log.arg = skill->objectName();
            log.arg2 = objectName();
            room->sendLog(log);

            if (player->hasSkill(skill))
                room->notifySkillInvoked(player, skill->objectName());
            room->broadcastSkillInvoke(skill->objectName());
        } else
            targets << player;
    }

    CardUseStruct use = card_use;
    use.to = targets;
    TrickCard::onUse(room, use);
}

QString SingleTargetTrick::getSubtype() const
{
    return "single_target_trick";
}

bool SingleTargetTrick::targetFilter(const QList<const Player *> &targets, const Player *, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num)
        return false;
    return true;
}

DelayedTrick::DelayedTrick(Suit suit, int number, bool movable, bool returnable)
    : TrickCard(suit, number)
    , movable(movable)
    , returnable(returnable)
{
    judge.negative = true;
}

void DelayedTrick::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    WrappedCard *wrapped = Sanguosha->getWrappedCard(getEffectiveId());
    use.card = wrapped;

    LogMessage log;
    log.from = use.from;
    log.to = use.to;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    QVariant data = QVariant::fromValue(use);
    RoomThread *thread = room->getThread();
    thread->trigger(PreCardUsed, room, data);

    //CardMoveReason reason(CardMoveReason::S_REASON_USE, use.from->objectName(), use.to.first()->objectName(), getSkillName(), QString());
    //room->moveCardTo(this, use.from, use.to.first(), Player::PlaceDelayedTrick, reason, true);

    CardMoveReason reason(CardMoveReason::S_REASON_USE, use.from->objectName(), QString(), card_use.card->getSkillName(), QString());
    CardsMoveStruct move(card_use.card->getEffectiveId(), NULL, Player::PlaceTable, reason);
    room->moveCardsAtomic(move, true);
    //show hidden after move Event to avoid filter card
    use.from->showHiddenSkill(getSkillName());

    thread->trigger(CardUsed, room, data);
    thread->trigger(CardFinished, room, data);
}

void DelayedTrick::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QStringList nullified_list = room->getTag("CardUseNullifiedList").toStringList();
    bool all_nullified = nullified_list.contains("_ALL_TARGETS");
    if (all_nullified || targets.isEmpty() || targets.first()->isDead()) {
        if (movable) {
            onNullified(source);
            if (room->getCardOwner(getEffectiveId()) != source)
                return;
        }
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), getSkillName(), QString());
        //room->moveCardTo(this, room->getCardOwner(getEffectiveId()), NULL, Player::DiscardPile, reason, true);
        room->moveCardTo(this, source, NULL, Player::DiscardPile, reason, true);
    } else {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), targets.first()->objectName(), getSkillName(), QString());
        room->moveCardTo(this, source, targets.first(), Player::PlaceDelayedTrick, reason, true);
    }
}

QString DelayedTrick::getSubtype() const
{
    return "delayed_trick";
}

void DelayedTrick::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    CardMoveReason reason(CardMoveReason::S_REASON_USE, effect.to->objectName(), getSkillName(), QString());
    room->moveCardTo(this, NULL, Player::PlaceTable, reason, true);

    LogMessage log;
    log.from = effect.to;
    log.type = "#DelayedTrick";
    log.arg = effect.card->objectName();
    room->sendLog(log);

    JudgeStruct judge_struct = judge;
    judge_struct.who = effect.to;
    room->judge(judge_struct);

    if (judge_struct.negative == judge_struct.isBad()) {
        if (effect.to->isAlive() && !judge_struct.ignore_judge)
            takeEffect(effect.to);
        if (room->getCardOwner(getEffectiveId()) == NULL) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString());
            room->throwCard(this, reason, NULL);
        }
    } else if (movable) {
        onNullified(effect.to);
    } else if (returnable && effect.to->isAlive()) {
        if (room->getCardOwner(getEffectiveId()) == NULL) {
            if (isVirtualCard()) {
                Card *delayTrick = Sanguosha->cloneCard(objectName());
                WrappedCard *vs_card = Sanguosha->getWrappedCard(getEffectiveId());
                vs_card->setSkillName(getSkillName());
                vs_card->takeOver(delayTrick);
                room->broadcastUpdateCard(room->getAlivePlayers(), vs_card->getId(), vs_card);
            }

            CardsMoveStruct move;
            move.card_ids << getEffectiveId();
            move.to = effect.to;
            move.to_place = Player::PlaceDelayedTrick;
            room->moveCardsAtomic(move, true);
        }
    } else {
        if (room->getCardOwner(getEffectiveId()) == NULL) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString());
            room->throwCard(this, reason, NULL);
        }
    }
}

void DelayedTrick::onNullified(ServerPlayer *target) const
{
    Room *room = target->getRoom();
    RoomThread *thread = room->getThread();

    if (movable) {
        QList<ServerPlayer *> players = room->getOtherPlayers(target);
        players << target;
        ServerPlayer *next = NULL; //next meaning this next one
        bool next2next = false; //it's meaning another next(a second next) is necessary
        foreach (ServerPlayer *player, players) {
            if (player->containsTrick(objectName()))
                continue;

            const ProhibitSkill *skill = room->isProhibited(target, player, this);
            if (skill) {
                LogMessage log;
                log.type = "#SkillAvoid";
                log.from = player;
                log.arg = skill->objectName();
                log.arg2 = objectName();
                room->sendLog(log);

                room->broadcastSkillInvoke(skill->objectName());
                continue;
            }

            next = player;
            CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, target->objectName(), QString(), getSkillName(), QString());
            room->moveCardTo(this, target, player, Player::PlaceDelayedTrick, reason, true);
            if (target == player)
                break;

            CardUseStruct use;
            use.from = NULL;
            use.to << player;
            use.card = this;
            QVariant data = QVariant::fromValue(use);
            thread->trigger(TargetConfirming, room, data);
            CardUseStruct new_use = data.value<CardUseStruct>();
            if (new_use.to.isEmpty()) {
                next2next = true;
                break;
            }

            thread->trigger(TargetConfirmed, room, data);
            break;
        }
        //case:stop.
        if (!next) {
            CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, target->objectName(), QString(), getSkillName(), QString());
            room->moveCardTo(this, target, target, Player::PlaceDelayedTrick, reason, true);
        }
        //case: next2next
        if (next && next2next)
            onNullified(next);
    } else {
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, target->objectName());
        room->throwCard(this, reason, NULL);
    }
}

JudgeStruct DelayedTrick::getJudge()
{
    return this->judge;
}

Weapon::Weapon(Suit suit, int number, int range)
    : EquipCard(suit, number)
    , range(range)
{
    can_recast = true;
}

bool Weapon::isAvailable(const Player *player) const
{
    QString mode = player->getGameMode();
    if (mode == "04_1v3" && !player->isCardLimited(this, Card::MethodRecast))
        return true;
    return !player->isCardLimited(this, Card::MethodUse) && EquipCard::isAvailable(player);
}

int Weapon::getRange() const
{
    return range;
}

QString Weapon::getSubtype() const
{
    return "weapon";
}

void Weapon::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *player = card_use.from;
    if (room->getMode() == "04_1v3" && use.card->isKindOf("Weapon")
        && (player->isCardLimited(use.card, Card::MethodUse)
            || (!player->getHandPile().contains(getEffectiveId()) //!player->getPile("wooden_ox").contains(getEffectiveId())
                || player->askForSkillInvoke("weapon_recast", QVariant::fromValue(use))))) {
        CardMoveReason reason(CardMoveReason::S_REASON_RECAST, player->objectName());
        reason.m_eventName = "weapon_recast";
        room->moveCardTo(use.card, player, NULL, Player::DiscardPile, reason);
        player->broadcastSkillInvoke("@recast");

        LogMessage log;
        log.type = "#Card_Recast";
        log.from = player;
        log.card_str = use.card->toString();
        room->sendLog(log);

        player->drawCards(1);
        return;
    }
    EquipCard::onUse(room, use);
}

EquipCard::Location Weapon::location() const
{
    return WeaponLocation;
}

QString Weapon::getCommonEffectName() const
{
    return "weapon";
}

QString Armor::getSubtype() const
{
    return "armor";
}

EquipCard::Location Armor::location() const
{
    return ArmorLocation;
}

QString Armor::getCommonEffectName() const
{
    return "armor";
}

Horse::Horse(Suit suit, int number, int correct)
    : EquipCard(suit, number)
    , correct(correct)
{
}

int Horse::getCorrect() const
{
    return correct;
}

void Horse::onInstall(ServerPlayer *) const
{
}

void Horse::onUninstall(ServerPlayer *) const
{
}

QString Horse::getCommonEffectName() const
{
    return "horse";
}

OffensiveHorse::OffensiveHorse(Card::Suit suit, int number, int correct)
    : Horse(suit, number, correct)
{
}

QString OffensiveHorse::getSubtype() const
{
    return "offensive_horse";
}

DefensiveHorse::DefensiveHorse(Card::Suit suit, int number, int correct)
    : Horse(suit, number, correct)
{
}

QString DefensiveHorse::getSubtype() const
{
    return "defensive_horse";
}

EquipCard::Location Horse::location() const
{
    if (correct > 0)
        return DefensiveHorseLocation;
    else
        return OffensiveHorseLocation;
}

QString Treasure::getSubtype() const
{
    return "treasure";
}

EquipCard::Location Treasure::location() const
{
    return TreasureLocation;
}

QString Treasure::getCommonEffectName() const
{
    return "treasure";
}

StandardPackage::StandardPackage()
    : Package("standard")
{
    //addGenerals();

    patterns["."] = new ExpPattern(".|.|.|hand");
    patterns[".S"] = new ExpPattern(".|spade|.|hand");
    patterns[".C"] = new ExpPattern(".|club|.|hand");
    patterns[".H"] = new ExpPattern(".|heart|.|hand");
    patterns[".D"] = new ExpPattern(".|diamond|.|hand");
    patterns[".black"] = new ExpPattern(".|black|.|hand");
    patterns[".red"] = new ExpPattern(".|red|.|hand");

    patterns[".."] = new ExpPattern(".");
    patterns["..S"] = new ExpPattern(".|spade");
    patterns["..C"] = new ExpPattern(".|club");
    patterns["..H"] = new ExpPattern(".|heart");
    patterns["..D"] = new ExpPattern(".|diamond");
    patterns["..black"] = new ExpPattern(".|black");
    patterns["..red"] = new ExpPattern(".|red");

    patterns[".Basic"] = new ExpPattern("BasicCard");
    patterns[".Trick"] = new ExpPattern("TrickCard");
    patterns[".Equip"] = new ExpPattern("EquipCard");

    patterns[".Weapon"] = new ExpPattern("Weapon");
    patterns[".Armor"] = new ExpPattern("Armor");
    patterns[".Horse"] = new ExpPattern("Horse");
    patterns[".OffHorse"] = new ExpPattern("OffensiveHorse");
    patterns[".DefHorse"] = new ExpPattern("DefensiveHorse");

    patterns["slash"] = new ExpPattern("Slash");
    patterns["jink"] = new ExpPattern("Jink");
    patterns["peach"] = new ExpPattern("Peach");
    patterns["nullification"] = new ExpPattern("Nullification");
    patterns["peach+analeptic"] = new ExpPattern("Peach,Analeptic");
}

ADD_PACKAGE(Standard)

TestPackage::TestPackage()
    : Package("test")
{
    // for test only
    new General(this, "sujiang", "touhougod", 5, true, true);
    new General(this, "sujiangf", "touhougod", 5, false, true);

    new General(this, "anjiang", "touhougod", 4, true, true, true);
}

ADD_PACKAGE(Test)
