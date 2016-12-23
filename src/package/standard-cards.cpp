#include "standard.h"
#include "standard-equips.h"
#include "maneuvering.h"
#include "general.h"
#include "engine.h"
#include "client.h"
#include "room.h"
#include "ai.h"
#include "settings.h"

#include <functional>

Slash::Slash(Suit suit, int number) : BasicCard(suit, number)
{
    setObjectName("slash");
    nature = DamageStruct::Normal;
    drank = 0;
}

DamageStruct::Nature Slash::getNature() const
{
    return nature;
}

void Slash::setNature(DamageStruct::Nature nature)
{
    this->nature = nature;
}

bool Slash::IsAvailable(const Player *player, const Card *slash, bool considerSpecificAssignee)
{
    Slash *newslash = new Slash(Card::NoSuit, 0);
    newslash->setFlags("Global_SlashAvailabilityChecker");
    newslash->deleteLater();
#define THIS_SLASH (slash == NULL ? newslash : slash)
    if (player->isCardLimited(THIS_SLASH, Card::MethodUse))
        return false;

    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
        QList<int> ids;
        if (slash) {
            if (slash->isVirtualCard()) {
                if (slash->subcardsLength() > 0)
                    ids = slash->getSubcards();
            } else {
                ids << slash->getEffectiveId();
            }
        }
        bool has_weapon = (player->hasWeapon("Crossbow", true) || player->hasWeapon("VSCrossbow", true)) && ids.contains(player->getWeapon()->getEffectiveId());
        if ((!has_weapon && player->hasWeapon("Crossbow")) || player->canSlashWithoutCrossbow(THIS_SLASH))
            return true;
        int used = player->getSlashCount();

        int valid = 1 + Sanguosha->correctCardTarget(TargetModSkill::Residue, player, THIS_SLASH);
        if ((!has_weapon && player->hasWeapon("VSCrossbow")) && used < valid + 3)
            return true;
        
        if (considerSpecificAssignee) {
            QStringList assignee_list = player->property("extra_slash_specific_assignee").toString().split("+");
            if (!assignee_list.isEmpty()) {
                foreach (const Player *p, player->getAliveSiblings()) {
                    if (assignee_list.contains(p->objectName()) && player->canSlash(p, THIS_SLASH))
                        return true;
                }
            }
        }
        return false;
    } else {
        return true;
    }
#undef THIS_SLASH
}

bool Slash::IsSpecificAssignee(const Player *player, const Player *from, const Card *slash)
{
    if (from->hasFlag("slashTargetFix") && player->hasFlag("SlashAssignee"))
        return true;
    else if (from->getPhase() == Player::Play && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY
             && !Slash::IsAvailable(from, slash, false)) {
        QStringList assignee_list = from->property("extra_slash_specific_assignee").toString().split("+");
        if (assignee_list.contains(player->objectName())) return true;
    }
    return false;
}


bool Slash::isAvailable(const Player *player) const
{
    return IsAvailable(player, this) && BasicCard::isAvailable(player);
}

QString Slash::getSubtype() const
{
    return "attack_card";
}

void Slash::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *player = use.from;

    if (player->hasFlag("slashTargetFix")) {
        room->setPlayerFlag(player, "-slashTargetFix");
        room->setPlayerFlag(player, "-slashTargetFixToOne");
        foreach(ServerPlayer *target, room->getAlivePlayers())
            if (target->hasFlag("SlashAssignee"))
                room->setPlayerFlag(target, "-SlashAssignee");
    }


    //since the weapon Fan has changed, this code is no need any more.
    /*if (objectName() == "slash" && use.m_isOwnerUse) {
        bool has_changed = false;
        QString skill_name = getSkillName();
        if (!skill_name.isEmpty()) {
            const Skill *skill = Sanguosha->getSkill(skill_name);
            if (skill && !skill->inherits("FilterSkill"))
                has_changed = true;
        }
        if (!has_changed || subcardsLength() == 0) {
            QVariant data = QVariant::fromValue(use);
            if (use.card->objectName() == "slash" && player->hasWeapon("Fan")) {
                FireSlash *fire_slash = new FireSlash(getSuit(), getNumber());
                if (!isVirtualCard() || subcardsLength() > 0)
                    fire_slash->addSubcard(this);
                fire_slash->setSkillName("Fan");
                QStringList flags = use.card->getFlags();
                foreach(const QString &flag, flags)
                    fire_slash->setFlags(flag);

                bool can_use = true;
                foreach (ServerPlayer *p, use.to) {
                    if (!player->canSlash(p, fire_slash, false)) {
                        can_use = false;
                        break;
                    }
                }
                if (can_use && room->askForSkillInvoke(player, "Fan", data))
                    use.card = fire_slash;
                else
                    delete fire_slash;
            }
        }
    }*/
    if (((use.card->isVirtualCard() && use.card->subcardsLength() == 0) || use.card->hasFlag("pandu")) && !player->hasFlag("slashDisableExtraTarget")) {
        QList<ServerPlayer *> targets_ts;
        while (true) {
            QList<const Player *> targets_const;
            foreach(ServerPlayer *p, use.to)
                targets_const << qobject_cast<const Player *>(p);
            foreach(ServerPlayer *p, room->getAlivePlayers())
                if (!use.to.contains(p) && use.card->targetFilter(targets_const, p, use.from))
                    targets_ts << p;
            if (targets_ts.isEmpty())
                break;

            ServerPlayer *extra_target = room->askForPlayerChosen(player, targets_ts, "slash_extra_targets", "@slash_extra_targets", true);
            if (extra_target) {
                use.to.append(extra_target);
                room->sortByActionOrder(use.to);
            } else
                break;
            targets_ts.clear();
            targets_const.clear();
        }
    }

    if (player->hasFlag("slashNoDistanceLimit"))
        room->setPlayerFlag(player, "-slashNoDistanceLimit");
    if (player->hasFlag("slashDisableExtraTarget"))
        room->setPlayerFlag(player, "-slashDisableExtraTarget");

    foreach (const QString &flag, player->getFlagList()) {
        if (flag.startsWith("SlashRecorder_")) {
            room->setPlayerFlag(player, "-" + flag);
            QString cardFlag = flag.mid(14);
            room->setCardFlag(use.card, cardFlag);
        }
    }

    if (use.to.size() > 1 && player->hasSkill("xuedian")) {
        room->broadcastSkillInvoke("xuedian", 1);
        room->notifySkillInvoked(player, "xuedian");
    } else if (use.to.size() > 1 && player->hasSkill("shikong") && player->getPhase() == Player::Play) {
        room->notifySkillInvoked(player, "shikong");
    } else if (use.to.size() > 1 && player->hasSkill("shuangren")) {
        room->broadcastSkillInvoke("shuangren");
        room->notifySkillInvoked(player, "shuangren");
    }

    int rangefix = 0;
    if (use.card->isVirtualCard()) {
        if (use.from->getWeapon() && use.card->getSubcards().contains(use.from->getWeapon()->getId())) {
            const Weapon *weapon = qobject_cast<const Weapon *>(use.from->getWeapon()->getRealCard());
            rangefix += weapon->getRange() - use.from->getAttackRange(false);
        }

        if (use.from->getOffensiveHorse() && use.card->getSubcards().contains(use.from->getOffensiveHorse()->getId()))
            rangefix += 1;

    }

    if (use.from->hasFlag("BladeUse")) {
        use.from->setFlags("-BladeUse");
        room->setEmotion(player, "weapon/blade");

        LogMessage log;
        log.type = "#BladeUse";
        log.from = use.from;
        log.to << use.to;
        room->sendLog(log);
    } else if (use.from->hasFlag("MoonspearUse")) {
        use.from->setFlags("-MoonspearUse");
        room->setEmotion(player, "weapon/moonspear");

        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = use.from;
        log.arg = "moon_spear";
        room->sendLog(log);
    } else if (use.card->isVirtualCard() && use.card->getSkillName() == "spear")
        room->setEmotion(player, "weapon/spear");
    else if (use.to.size() > 1 && player->hasWeapon("Halberd") && player->isLastHandCard(this))
        room->setEmotion(player, "weapon/halberd");
    //else if (use.card->isVirtualCard() && use.card->getSkillName() == "fan")
    //    room->setEmotion(player, "weapon/fan");

    if (player->getPhase() == Player::Play && player->hasFlag("Global_MoreSlashInOneTurn") && player->hasWeapon("Crossbow")) {
        player->setFlags("-Global_MoreSlashInOneTurn");
        room->setEmotion(player, "weapon/crossbow");
    }

    if (use.card->isKindOf("ThunderSlash"))
        room->setEmotion(player, "thunder_slash");
    else if (use.card->isKindOf("FireSlash"))
        room->setEmotion(player, "fire_slash");
    else if (use.card->isRed())
        room->setEmotion(player, "slash_red");
    else if (use.card->isBlack())
        room->setEmotion(player, "slash_black");
    else
        room->setEmotion(player, "killer");

    BasicCard::onUse(room, use);
}



void Slash::onEffect(const CardEffectStruct &card_effect) const
{
    Room *room = card_effect.from->getRoom();
    if (card_effect.from->getMark("drank") > 0) {
        room->setCardFlag(this, "drank");
        drank = card_effect.from->getMark("drank");
        room->setPlayerMark(card_effect.from, "drank", 0);
    }

    SlashEffectStruct effect;
    effect.from = card_effect.from;
    effect.nature = nature;
    effect.slash = this;

    effect.to = card_effect.to;
    effect.drank = drank;
    effect.nullified = card_effect.nullified;

    QVariantList jink_list = effect.from->tag["Jink_" + toString()].toList();
    effect.jink_num = jink_list.takeFirst().toInt();

    if (jink_list.isEmpty())
        effect.from->tag.remove("Jink_" + toString());
    else
        effect.from->tag["Jink_" + toString()] = QVariant::fromValue(jink_list);

    room->slashEffect(effect);
}

bool Slash::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    
    //check targets feasible for skill "shikong"
    if (Self->hasSkill("shikong") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
        foreach (const Player *p, Self->getAliveSiblings()) {
            if (Slash::IsSpecificAssignee(p, Self, this)) {
                return !targets.isEmpty();
            }
        }

        int rangefix = 0;
        if (Self->getWeapon() && subcards.contains(Self->getWeapon()->getId())) {
            const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
            rangefix += weapon->getRange() - Self->getAttackRange(false);
        }

        if (Self->getOffensiveHorse() && subcards.contains(Self->getOffensiveHorse()->getId()))
            rangefix += 1;

        foreach (const Player *p, Self->getAliveSiblings()) {
            if (Self->inMyAttackRange(p) && Self->canSlash(p, this, true, rangefix)) {
                if (!targets.contains(p)) {
                    return false;
                }
            }
        }
    }
    return !targets.isEmpty();
}

bool Slash::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int slash_targets = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    bool distance_limit = ((1 + Sanguosha->correctCardTarget(TargetModSkill::DistanceLimit, Self, this)) < 500);
    if (Self->hasFlag("slashNoDistanceLimit"))
        distance_limit = false;

    int rangefix = 0;
    if (Self->getWeapon() && subcards.contains(Self->getWeapon()->getId())) {
        const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        rangefix += weapon->getRange() - Self->getAttackRange(false);
    }

    if (Self->getOffensiveHorse() && subcards.contains(Self->getOffensiveHorse()->getId()))
        rangefix += 1;


    bool has_specific_assignee = false;
    foreach (const Player *p, Self->getAliveSiblings()) {
        if (Slash::IsSpecificAssignee(p, Self, this)) {
            has_specific_assignee = true;
            break;
        }
    }

    if (has_specific_assignee) {
        if (Self->getWeapon() && Self->getWeapon()->isKindOf("Blade") && Self->getWeapon()->hasFlag("using")) { //flag baldeuse didnot work..
            if (subcards.contains(Self->getWeapon()->getId()))
                return false;
        }
        if (targets.isEmpty())
            return Slash::IsSpecificAssignee(to_select, Self, this) && Self->canSlash(to_select, this, distance_limit, rangefix);
        else {
            if (Self->hasFlag("slashDisableExtraTarget")) return false;
            bool canSelect = false;
            foreach (const Player *p, targets) {
                if (Slash::IsSpecificAssignee(p, Self, this)) {
                    canSelect = true;
                    break;
                }
            }
            if (!canSelect) return false;
        }
    }

    if (Self->hasSkill("shuangren") && distance_limit && targets.length() >= 1) {
        if (isVirtualCard() && subcardsLength() == 0 && !Self->hasFlag("slashDisableExtraTarget"))
            distance_limit = false;
        else {
            bool has_shuangren_target = false;
            foreach (const Player *p, targets) {
                if (Self->distanceTo(p, rangefix) > Self->getAttackRange(true) && !Slash::IsSpecificAssignee(p, Self, this)) {
                    has_shuangren_target = true;
                    break;
                }
            }
            if (!has_shuangren_target)
                distance_limit = false;
        }

    }
    if (!Self->canSlash(to_select, this, distance_limit, rangefix, targets)) return false;
    if (targets.length() >= slash_targets)
        return false;
    return true;
}

Jink::Jink(Suit suit, int number) : BasicCard(suit, number)
{
    setObjectName("jink");
    target_fixed = true;
}

QString Jink::getSubtype() const
{
    return "defense_card";
}

bool Jink::isAvailable(const Player *) const
{
    return false;
}

Peach::Peach(Suit suit, int number) : BasicCard(suit, number)
{
    setObjectName("peach");
    target_fixed = true;
}

QString Peach::getSubtype() const
{
    return "recover_card";
}

bool Peach::targetFixed() const
{
    bool ignore = (Self && Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !hasFlag("IgnoreFailed"));
    if (ignore)
        return false;
    if (Self && Self->getKingdom() == "zhan" && Self->getPhase() == Player::Play) {
        foreach(const Player *p, Self->getAliveSiblings()) {
            if (p->hasLordSkill("yanhui") && p->isWounded())
                return false;
        }
    }
    return target_fixed;
}

void Peach::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;

    BasicCard::onUse(room, use);
}

void Peach::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->setEmotion(effect.from, "peach");

    // recover hp
    RecoverStruct recover;
    recover.card = this;
    recover.who = effect.from;
    room->recover(effect.to, recover);
}

bool Peach::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    //ignore ExtraTarget
    //int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    //if (targets.length() >= total_num)
    //    return false;
    if (Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !hasFlag("IgnoreFailed"))
        return true;
    if (targets.isEmpty() && to_select->isWounded()) {
        bool globalDying = false;
        QList<const Player *> players = Self->getSiblings();
        players << Self;
        foreach (const Player *p, players) {
            if (p->hasFlag("Global_Dying") && p->isAlive()) {
                globalDying = true;
                break;
            }
        }

        if (globalDying && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            return to_select->hasFlag("Global_Dying") && to_select->objectName() == Self->property("currentdying").toString();
        } else {
            if (to_select == Self) return true;
            if (Self->getKingdom() == "zhan" && Self->getPhase() == Player::Play &&  to_select->hasLordSkill("yanhui")) return true;
        }
    }
    return false;
}

bool Peach::isAvailable(const Player *player) const
{
    if (!BasicCard::isAvailable(player))
        return false;
    bool ignore = (player->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !hasFlag("IgnoreFailed"));
    if (ignore)
        return true;
    if (player->isWounded() && !player->isProhibited(player, this)) return true;
    foreach(const Player *p, player->getAliveSiblings()) {
        if (p->hasLordSkill("yanhui") && p->isWounded() && player->getKingdom() == "zhan" && player->getPhase() == Player::Play
                && !player->isProhibited(p, this))
            return true;
    }
    return false;
}

Crossbow::Crossbow(Suit suit, int number)
    : Weapon(suit, number, 1)
{
    setObjectName("Crossbow");
}


TribladeCard::TribladeCard() : SkillCard()
{
    m_skillName = "Triblade";
}

bool TribladeCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.length() == 0 && to_select->hasFlag("Global_TribladeFailed");
}

void TribladeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->damage(DamageStruct("Triblade", source, targets[0]));
}

class TribladeSkillVS : public OneCardViewAsSkill
{
public:
    TribladeSkillVS() : OneCardViewAsSkill("Triblade")
    {
        response_pattern = "@@Triblade";
        filter_pattern = ".|.|.|hand!";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        TribladeCard *c = new TribladeCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class TribladeSkill : public WeaponSkill
{
public:
    TribladeSkill() : WeaponSkill("Triblade")
    {
        events << Damage;
        view_as_skill = new TribladeSkillVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.from || damage.from->isDead()
                || !equipAvailable(damage.from, EquipCard::WeaponLocation, objectName()) || !damage.from->canDiscard(damage.from, "hs"))
            return QList<SkillInvokeDetail>();

        if (damage.to && damage.to->isAlive() && damage.card && damage.card->isKindOf("Slash")
                && damage.by_user && !damage.chain && !damage.transfer) {
            foreach(ServerPlayer *p, room->getOtherPlayers(damage.from)) {
                if (damage.to->distanceTo(p) == 1) {
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        foreach(ServerPlayer *p, room->getOtherPlayers(damage.from)){
            if (damage.to->distanceTo(p) == 1)
                room->setPlayerFlag(p, "Global_TribladeFailed");
        }
        room->askForUseCard(invoke->invoker, "@@Triblade", "@Triblade");
        return false;
    }
};

Triblade::Triblade(Card::Suit suit, int number) : Weapon(suit, number, 3)
{
    setObjectName("Triblade");
}

class DoubleSwordSkill : public WeaponSkill
{
public:
    DoubleSwordSkill() : WeaponSkill("DoubleSword")
    {
        events << TargetSpecified;
    }

    static bool diff(ServerPlayer *player, ServerPlayer *target)
    {
        ServerPlayer *lord = player->getRoom()->getLord();
        QString lordKingdom = "";
        if (lord)
            lordKingdom = lord->getKingdom();
        bool can = false;
        if (lordKingdom == "")
            can = ((player->isMale() && target->isFemale()) || (player->isFemale() && target->isMale()));
        else
            can = (player->getKingdom() != target->getKingdom() && (player->getKingdom() == lordKingdom || target->getKingdom() == lordKingdom));
        return can;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!equipAvailable(use.from, EquipCard::WeaponLocation, objectName()))
            return QList<SkillInvokeDetail>();

        if (use.card != NULL && use.card->isKindOf("Slash")) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, use.to) {
                if (p->isAlive() && diff(use.from, p)) {
                    if (!equipAvailable(use.from, EquipCard::WeaponLocation, objectName(), p))
                        continue;
                    d << SkillInvokeDetail(this, use.from, use.from, NULL, false, p);
                }
            }

            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->tag["DoubleSwordTarget"] = QVariant::fromValue(invoke->preferredTarget);
        if (invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget))) {
            room->setEmotion(invoke->invoker, "weapon/double_sword");
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        bool draw_card = false;

        if (!invoke->targets.first()->canDiscard(invoke->targets.first(), "hs"))
            draw_card = true;
        else {
            QString prompt = "double-sword-card:" + invoke->invoker->objectName();
            const Card *card = room->askForCard(invoke->targets.first(), ".", prompt, data);
            if (!card) draw_card = true;
        }
        if (draw_card)
            invoke->invoker->drawCards(1);

        return false;
    }
};

DoubleSword::DoubleSword(Suit suit, int number)
    : Weapon(suit, number, 2)
{
    setObjectName("DoubleSword");
}

class QinggangSwordSkill : public WeaponSkill
{
public:
    QinggangSwordSkill() : WeaponSkill("QinggangSword")
    {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!equipAvailable(use.from, EquipCard::WeaponLocation, objectName()))
            return QList<SkillInvokeDetail>();

        if (use.card != NULL && use.card->isKindOf("Slash")) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, use.to) {
                if (!equipAvailable(use.from, EquipCard::WeaponLocation, objectName(), p))
                    continue;
                d << SkillInvokeDetail(this, use.from, use.from, NULL, true, p);
            }

            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if ((invoke->preferredTarget->getArmor() && invoke->preferredTarget->hasArmorEffect(invoke->preferredTarget->getArmor()->objectName())) || invoke->preferredTarget->hasArmorEffect("shenbao"))
            room->setEmotion(invoke->invoker, "weapon/qinggang_sword");
        return true;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (invoke->targets.first()->getMark("Equips_of_Others_Nullified_to_You") == 0)
            invoke->targets.first()->addQinggangTag(use.card);
        return false;
    }
};

QinggangSword::QinggangSword(Suit suit, int number)
    : Weapon(suit, number, 2)
{
    setObjectName("QinggangSword");
}

class BladeSkill : public WeaponSkill
{
public:
    BladeSkill() : WeaponSkill("Blade")
    {
        events << TargetSpecified;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash"))
            return QList<SkillInvokeDetail>();
        if (!equipAvailable(use.from, EquipCard::WeaponLocation, objectName()))
            return QList<SkillInvokeDetail>();

        if (use.from->isAlive())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        JudgeStruct judge;
        judge.pattern = ".|red";
        judge.good = true;
        judge.reason = objectName();
        judge.who = invoke->invoker;

        room->judge(judge);
        if (judge.isGood()) {
            CardUseStruct use = data.value<CardUseStruct>();
            QVariantList jink = use.from->tag["Jink_" + use.card->toString()].toList();
            for (int i = 0; i < jink.length(); ++i)
                jink[i] = 0;
            use.from->tag["Jink_" + use.card->toString()] = jink;
        }

        return false;
    }
};

Blade::Blade(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("Blade");
}

class SpearSkill : public ViewAsSkill
{
public:
    SpearSkill() : ViewAsSkill("Spear")
    {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        bool avalilable = Slash::IsAvailable(player);
        //consider targetmod skill like "xiubu" need check specific card with subcards
        if (player->getMark("xiubu") && player->getHandcardNum() <= 2) {
            Slash *slash = new Slash(Card::SuitToBeDecided, 0);
            slash->setSkillName(objectName());
            slash->addSubcards(player->getHandcards());
            if (!player->isCardLimited(slash, Card::MethodUse))
                avalilable = true;
        }

        return avalilable && EquipSkill::equipAvailable(player, EquipCard::WeaponLocation, objectName());
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return  matchAvaliablePattern("slash", pattern) && EquipSkill::equipAvailable(player, EquipCard::WeaponLocation, objectName());
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        Slash *slash = new Slash(Card::SuitToBeDecided, 0);
        slash->setSkillName(objectName());
        slash->addSubcards(cards);

        return slash;
    }
};

Spear::Spear(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("Spear");
}

class AxeViewAsSkill : public ViewAsSkill
{
public:
    AxeViewAsSkill() : ViewAsSkill("Axe")
    {
        response_pattern = "@Axe";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {

        if (Self->hasWeapon(objectName(), true) && to_select == Self->getWeapon()) //check if cannot throw selfweapon
            return false;
        return selected.length() < 2 && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        DummyCard *card = new DummyCard;
        card->setSkillName(objectName());
        card->addSubcards(cards);
        return card;
    }
};

class AxeSkill : public WeaponSkill
{
public:
    AxeSkill() : WeaponSkill("Axe")
    {
        events << SlashMissed;
        view_as_skill = new AxeViewAsSkill;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (!equipAvailable(effect.from, EquipCard::WeaponLocation, objectName()))
            return QList<SkillInvokeDetail>();

        if (!effect.to->isAlive())
            return QList<SkillInvokeDetail>();

        int cardLimit = 2;
        if (effect.from->hasWeapon(objectName(), true))
            cardLimit = 3;
        if (effect.from->getCardCount(true) >= cardLimit)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.from, effect.from);  // this skill don't have a target

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        invoke->invoker->tag["axe_target"] = QVariant::fromValue(effect.to);
        const Card *card = room->askForCard(invoke->invoker, "@Axe", "@Axe:" + effect.to->objectName(), data, objectName());
        if (card)
            return true;
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        room->setEmotion(invoke->invoker, "weapon/axe");
        room->slashResult(effect, NULL);
        return true;
    }
};

Axe::Axe(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("Axe");
}

class HalberdSkill : public TargetModSkill
{
public:
    HalberdSkill() : TargetModSkill("Halberd")
    {
        frequency = NotCompulsory;
    }

    virtual int getExtraTargetNum(const Player *from, const Card *card) const
    {
        bool isLastHandCard = false;
        if (from->isLastHandCard(card))
            isLastHandCard = true;
        else if (card->getSkillName() == "xihua" && from->getHandcardNum() == 1)
            isLastHandCard = true;
        if (from->hasWeapon("Halberd") && isLastHandCard)
            return 2;
        else
            return 0;
    }
};

Halberd::Halberd(Suit suit, int number)
    : Weapon(suit, number, 4)
{
    setObjectName("Halberd");
}

class KylinBowSkill : public WeaponSkill
{
public:
    KylinBowSkill() : WeaponSkill("KylinBow")
    {
        events << DamageCaused;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!equipAvailable(damage.from, EquipCard::WeaponLocation, objectName(), damage.to))
            return QList<SkillInvokeDetail>();

        if (damage.card && damage.card->isKindOf("Slash") && damage.by_user && !damage.chain && !damage.transfer) {
            if (damage.to->getDefensiveHorse() && damage.from->canDiscard(damage.to, damage.to->getDefensiveHorse()->getEffectiveId()))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, false, damage.to);
            if (damage.to->getOffensiveHorse() && damage.from->canDiscard(damage.to, damage.to->getOffensiveHorse()->getEffectiveId()))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, false, damage.to);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QStringList horses;
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->getDefensiveHorse() && damage.from->canDiscard(damage.to, damage.to->getDefensiveHorse()->getEffectiveId()))
            horses << "dhorse";
        if (damage.to->getOffensiveHorse() && damage.from->canDiscard(damage.to, damage.to->getOffensiveHorse()->getEffectiveId()))
            horses << "ohorse";

        QString horse_type = room->askForChoice(invoke->invoker, objectName(), horses.join("+"));
        room->setEmotion(invoke->invoker, "weapon/kylin_bow");
        if (horse_type == "dhorse")
            room->throwCard(damage.to->getDefensiveHorse(), damage.to, damage.from);
        else if (horse_type == "ohorse")
            room->throwCard(damage.to->getOffensiveHorse(), damage.to, damage.from);

        return false;
    }
};

KylinBow::KylinBow(Suit suit, int number)
    : Weapon(suit, number, 5)
{
    setObjectName("KylinBow");
}

class EightDiagramSkill : public ArmorSkill
{
public:
    EightDiagramSkill() : ArmorSkill("EightDiagram")
    {
        events << CardAsked;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardAskedStruct ask = data.value<CardAskedStruct>();
        if (!matchAvaliablePattern("jink", ask.pattern))
            return QList<SkillInvokeDetail>();

        ServerPlayer *player = ask.player;
        if (!equipAvailable(player, EquipCard::ArmorLocation, objectName()))
            return QList<SkillInvokeDetail>();

        //since skill yuanfei,we need check
        Card *jink = Sanguosha->cloneCard("jink");
        DELETE_OVER_SCOPE(Card, jink)
                if (player->isCardLimited(jink, ask.method))
                return QList<SkillInvokeDetail>();
        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        int armor_id = -1;
        if (invoke->invoker->getArmor()) {
            armor_id = invoke->invoker->getArmor()->getId();
            room->setCardFlag(armor_id, "using");
        }

        room->setEmotion(invoke->invoker, "armor/eight_diagram");
        JudgeStruct judge;
        judge.pattern = ".|red";
        judge.good = true;
        judge.reason = objectName();
        judge.who = invoke->invoker;

        room->judge(judge);
        if (armor_id != -1)
            room->setCardFlag(armor_id, "-using");

        if (judge.isGood()) {
            Jink *jink = new Jink(Card::NoSuit, 0);
            jink->setSkillName(objectName());
            room->provide(jink);
            return true;
        }

        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const
    {
        return -2;
    }
};

EightDiagram::EightDiagram(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("EightDiagram");
}


class BreastPlateSkill : public ArmorSkill
{
public:
    BreastPlateSkill() : ArmorSkill("BreastPlate")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.damage >= damage.to->getHp() && damage.to->getArmor() && damage.to->getArmor()->objectName() == objectName()
                && equipAvailable(damage.to, EquipCard::ArmorLocation, objectName())) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        room->obtainCard(invoke->invoker, invoke->invoker->getArmor()->getEffectiveId());
        DamageStruct damage = data.value<DamageStruct>();
        LogMessage log;
        log.type = "#BreastPlate";
        log.from = invoke->invoker;
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
    }
};

BreastPlate::BreastPlate(Card::Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("BreastPlate");
}

AmazingGrace::AmazingGrace(Suit suit, int number)
    : GlobalEffect(suit, number)
{
    setObjectName("amazing_grace");
    has_preact = true;
}

void AmazingGrace::clearRestCards(Room *room) const
{
    room->clearAG();

    QVariantList ag_list = room->getTag("AmazingGrace").toList();
    room->removeTag("AmazingGrace");
    if (ag_list.isEmpty()) return;
    DummyCard *dummy = new DummyCard(VariantList2IntList(ag_list));
    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), "amazing_grace", QString());
    room->throwCard(dummy, reason, NULL);
    delete dummy;

}

void AmazingGrace::doPreAction(Room *room, const CardUseStruct &use) const
{
    int count = room->getAllPlayers().length();
    foreach (ServerPlayer *p, room->getAllPlayers()) {
        if (p->hasSkill("shouhuo")) {
            room->notifySkillInvoked(p, "shouhuo");
            room->touhouLogmessage("#TriggerSkill", p, "shouhuo");
            count++;
        }
    }
    QList<int> card_ids = room->getNCards(count);
    CardsMoveStruct move(card_ids, NULL, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, use.from->objectName(), objectName(), QString()));
    room->moveCardsAtomic(move, true);
    room->fillAG(card_ids);
    room->setTag("AmazingGrace", IntList2VariantList(card_ids));
}

void AmazingGrace::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    //shemi count
    if (getSkillName() == "shemi")
        room->addPlayerHistory(source, "ShemiAG");

    try {
        GlobalEffect::use(room, source, targets);
        clearRestCards(room);
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            clearRestCards(room);
        throw triggerEvent;
    }
}

void AmazingGrace::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    QVariantList ag_list = room->getTag("AmazingGrace").toList();
    QList<int> card_ids;
    foreach(QVariant card_id, ag_list)
        card_ids << card_id.toInt();

    int card_id = room->askForAG(effect.to, card_ids, false, objectName());
    card_ids.removeOne(card_id);

    room->takeAG(effect.to, card_id);
    ag_list.removeOne(card_id);

    if (effect.to->hasSkill("shouhuo")) {
        int card_id1 = room->askForAG(effect.to, card_ids, false, objectName());
        card_ids.removeOne(card_id1);

        room->takeAG(effect.to, card_id1);
        ag_list.removeOne(card_id1);
    }

    room->setTag("AmazingGrace", ag_list);
}

GodSalvation::GodSalvation(Suit suit, int number)
    : GlobalEffect(suit, number)
{
    setObjectName("god_salvation");
}

bool GodSalvation::isCancelable(const CardEffectStruct &effect) const
{
    return effect.to->isWounded() && TrickCard::isCancelable(effect);
}

void GodSalvation::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    if (!effect.to->isWounded())
        ;//room->setEmotion(effect.to, "skill_nullify");
    else {
        RecoverStruct recover;
        recover.card = this;
        recover.who = effect.from;
        room->recover(effect.to, recover);
    }
}

SavageAssault::SavageAssault(Suit suit, int number)
    : AOE(suit, number)
{
    setObjectName("savage_assault");
}

void SavageAssault::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    const Card *slash = room->askForCard(effect.to,
                                         "slash",
                                         "savage-assault-slash:" + effect.from->objectName(),
                                         QVariant::fromValue(effect),
                                         Card::MethodResponse,
                                         effect.from->isAlive() ? effect.from : NULL);
    if (slash) {
        if (slash->getSkillName() == "spear") room->setEmotion(effect.to, "weapon/spear");
        room->setEmotion(effect.to, "killer");
    } else {
        room->damage(DamageStruct(this, effect.from->isAlive() ? effect.from : NULL, effect.to));
        room->getThread()->delay();
    }
}

ArcheryAttack::ArcheryAttack(Card::Suit suit, int number)
    : AOE(suit, number)
{
    setObjectName("archery_attack");
}

void ArcheryAttack::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    const Card *jink = room->askForCard(effect.to,
                                        "jink",
                                        "archery-attack-jink:" + effect.from->objectName(),
                                        QVariant::fromValue(effect),
                                        Card::MethodResponse,
                                        effect.from->isAlive() ? effect.from : NULL);
    if (jink && jink->getSkillName() != "eight_diagram" && jink->getSkillName() != "bazhen")
        room->setEmotion(effect.to, "jink");

    if (!jink) {
        room->damage(DamageStruct(this, effect.from->isAlive() ? effect.from : NULL, effect.to));
        room->getThread()->delay();
    }
}

Collateral::Collateral(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("collateral");
}

bool Collateral::isAvailable(const Player *player) const
{
    bool canUse = false;
    bool ignore = (player->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !hasFlag("IgnoreFailed"));
    foreach (const Player *p, player->getAliveSiblings()) {
        if (p->getWeapon() || (p != player && ignore)) {
            canUse = true;
            break;
        }
    }
    return canUse && SingleTargetTrick::isAvailable(player);
}

bool Collateral::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

bool Collateral::targetFilter(const QList<const Player *> &targets,
                              const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty()) {
        // @todo: fix this. We should probably keep the codes here, but change the code in
        // roomscene such that if it is collateral, then targetFilter's result is overrode
        Q_ASSERT(targets.length() <= 2);
        if (targets.length() == 2) return false;
        const Player *slashFrom = targets[0];
        /* @todo: develop a new mechanism of filtering targets
                    to remove the coupling here and to fix the similar bugs caused by TongJi */
        if (to_select == Self && to_select->hasSkill("kongcheng") && Self->isLastHandCard(this, true))
            return false;
        return slashFrom->canSlash(to_select);
    } else {
        bool ignore = (Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && to_select != Self && !hasFlag("IgnoreFailed"));
        if (to_select == Self)
            return false;

        if (!to_select->getWeapon() && !ignore)
            return false;

        foreach (const Player *p, to_select->getAliveSiblings()) {
            if (to_select->canSlash(p)
                    && (!(p == Self && p->hasSkill("kongcheng") && Self->isLastHandCard(this, true))))
                return true;
        }
    }
    return false;
}

void Collateral::onUse(Room *room, const CardUseStruct &card_use) const
{
    Q_ASSERT(card_use.to.length() == 2);
    ServerPlayer *killer = card_use.to.at(0);
    ServerPlayer *victim = card_use.to.at(1);

    CardUseStruct new_use = card_use;
    new_use.to.removeAt(1);
    killer->tag["collateralVictim"] = QVariant::fromValue((ServerPlayer *)victim);

    SingleTargetTrick::onUse(room, new_use);
}

bool Collateral::doCollateral(Room *room, ServerPlayer *killer, ServerPlayer *victim, const QString &prompt) const
{
    bool useSlash = false;
    if (killer->canSlash(victim, NULL, false))
        useSlash = room->askForUseSlashTo(killer, victim, prompt);
    return useSlash;
}

void Collateral::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    Room *room = source->getRoom();
    ServerPlayer *killer = effect.to;
    ServerPlayer *victim = effect.to->tag["collateralVictim"].value<ServerPlayer *>();
    effect.to->tag.remove("collateralVictim");
    if (!victim) return;
    WrappedCard *weapon = killer->getWeapon();

    QString prompt = QString("collateral-slash:%1:%2").arg(victim->objectName()).arg(source->objectName());

    if (victim->isDead()) {
        if (source->isAlive() && killer->isAlive() && killer->getWeapon())
            source->obtainCard(weapon);
    } else if (source->isDead()) {
        if (killer->isAlive())
            doCollateral(room, killer, victim, prompt);
    } else {
        if (killer->isDead()) {
            ; // do nothing
        } else if (!killer->getWeapon()) {
            doCollateral(room, killer, victim, prompt);
        } else {
            if (!doCollateral(room, killer, victim, prompt)) {
                if (killer->getWeapon())
                    source->obtainCard(weapon);
            }
        }
    }
}

Nullification::Nullification(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    target_fixed = true;
    setObjectName("nullification");
}

void Nullification::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    // does nothing, just throw it
    if (room->getCardPlace(getEffectiveId()) == Player::PlaceTable) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName());
        room->moveCardTo(this, source, NULL, Player::DiscardPile, reason);
    }
}

bool Nullification::isAvailable(const Player *) const
{
    return false;
}

ExNihilo::ExNihilo(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("ex_nihilo");
    target_fixed = true;
}

void ExNihilo::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;
    SingleTargetTrick::onUse(room, use);
}

bool ExNihilo::isAvailable(const Player *player) const
{
    return !player->isProhibited(player, this) && TrickCard::isAvailable(player);
}

void ExNihilo::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    int extra = 0;
    if (room->getMode() == "06_3v3" && Config.value("3v3/OfficialRule", "2013").toString() == "2013") {
        int friend_num = 0, enemy_num = 0;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (AI::GetRelation3v3(effect.to, p) == AI::Friend)
                friend_num++;
            else
                enemy_num++;
        }
        if (friend_num < enemy_num) extra = 1;
    }
    effect.to->drawCards(2 + extra);
}

Duel::Duel(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("duel");
}

bool Duel::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    return targets.length() < total_num && to_select != Self;
}

void Duel::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *first = effect.to;
    ServerPlayer *second = effect.from;
    Room *room = first->getRoom();

    room->setEmotion(first, "duel");
    room->setEmotion(second, "duel");

    forever{
        if (!first->isAlive())
            break;
        if (second->tag["Wushuang_" + toString()].toStringList().contains(first->objectName())) {
            const Card *slash = room->askForCard(first,
                                                 "slash",
                                                 "@wushuang-slash-1:" + second->objectName(),
                                                 QVariant::fromValue(effect),
                                                 Card::MethodResponse,
                                                 second);
            if (slash == NULL)
                break;

            slash = room->askForCard(first, "slash",
                                     "@wushuang-slash-2:" + second->objectName(),
                                     QVariant::fromValue(effect),
                                     Card::MethodResponse,
                                     second);
            if (slash == NULL)
                break;
        } else {
            const Card *slash = room->askForCard(first,
                                                 "slash",
                                                 "duel-slash:" + second->objectName(),
                                                 QVariant::fromValue(effect),
                                                 Card::MethodResponse,
                                                 second);
            if (slash == NULL)
                break;
        }

        qSwap(first, second);
    }

    DamageStruct damage(this, second->isAlive() ? second : NULL, first);
    if (second != effect.from)
        damage.by_user = false;
    room->damage(damage);
}

Snatch::Snatch(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("snatch");
}

bool Snatch::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{

    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num || to_select == Self)
        return false;
    bool ignore = (Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY
                   && to_select != Self && !hasFlag("IgnoreFailed"));
    if (to_select->isAllNude() && !ignore)
        return false;
    int distance_limit = 1 + Sanguosha->correctCardTarget(TargetModSkill::DistanceLimit, Self, this);
    int rangefix = 0;
    if (Self->getOffensiveHorse() && subcards.contains(Self->getOffensiveHorse()->getId()))
        rangefix += 1;
    if (Self->distanceTo(to_select, rangefix) > distance_limit)
        return false;

    return true;
}

void Snatch::onEffect(const CardEffectStruct &effect) const
{
    if (effect.from->isDead())
        return;
    if (effect.to->isAllNude())
        return;

    Room *room = effect.to->getRoom();
    bool using_2013 = (room->getMode() == "02_1v1" && Config.value("1v1/Rule", "2013").toString() != "Classical");
    QString flag = using_2013 ? "hes" : "hejs";
    //for AI: sgs.ai_choicemade_filter.cardChosen.snatch
    //like Liyou  Xunshi
    effect.from->tag["SnatchCard"] = QVariant::fromValue(effect.card);
    int card_id = room->askForCardChosen(effect.from, effect.to, flag, objectName());
    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
    room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
}

Dismantlement::Dismantlement(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("dismantlement");
}

bool Dismantlement::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    bool ignore = (Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY
                   && to_select != Self && !hasFlag("IgnoreFailed"));
    return targets.length() < total_num  && to_select != Self
            && (!to_select->isAllNude() || ignore);
}

void Dismantlement::onEffect(const CardEffectStruct &effect) const
{
    if (effect.from->isDead())
        return;

    Room *room = effect.to->getRoom();
    bool using_2013 = (room->getMode() == "02_1v1" && Config.value("1v1/Rule", "2013").toString() != "Classical");
    QString flag = using_2013 ? "hes" : "hejs";
    if (!effect.from->canDiscard(effect.to, flag))
        return;

    bool isNeoqixi = (getSkillName() == "neo2013qixi");

    int card_id = -1;
    AI *ai = effect.from->getAI();
    //for AI: sgs.ai_choicemade_filter.cardChosen.snatch
    //like Xunshi
    effect.from->tag["DismantlementCard"] = QVariant::fromValue(effect.card);
    if ((!isNeoqixi && !using_2013) || ai)
        card_id = room->askForCardChosen(effect.from, effect.to, flag, objectName(), false, Card::MethodDiscard);
    else {
        if (!effect.to->getEquips().isEmpty())
            card_id = room->askForCardChosen(effect.from, effect.to, flag, objectName(), false, Card::MethodDiscard);
        if (card_id == -1 || (!effect.to->isKongcheng() && effect.to->handCards().contains(card_id))) {
            LogMessage log;
            log.type = "$ViewAllCards";
            log.from = effect.from;
            log.to << effect.to;
            log.card_str = IntList2StringList(effect.to->handCards()).join("+");
            room->doNotify(effect.from, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());

            card_id = room->askForCardChosen(effect.from, effect.to, "hs", objectName(), true, Card::MethodDiscard);
            //Fs: I want to use room->doGongxin here
        }
    }
    room->throwCard(card_id, room->getCardPlace(card_id) == Player::PlaceDelayedTrick ? NULL : effect.to, effect.from);
}

Indulgence::Indulgence(Suit suit, int number)
    : DelayedTrick(suit, number)
{
    setObjectName("indulgence");

    judge.pattern = ".|heart";
    judge.good = true;
    judge.reason = objectName();
}

bool Indulgence::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    bool ignore = (Self && Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && to_select != Self && !hasFlag("IgnoreFailed"));
    return targets.isEmpty() && (!to_select->containsTrick(objectName()) || ignore)
            && to_select != Self;
}

void Indulgence::takeEffect(ServerPlayer *target) const
{
    target->clearHistory();
    target->skip(Player::Play);
}

Disaster::Disaster(Card::Suit suit, int number)
    : DelayedTrick(suit, number, true)
{
    target_fixed = true;
}


bool Disaster::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    bool ignore = (Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && to_select != Self && !hasFlag("IgnoreFailed"));
    if (ignore)
        return targets.isEmpty();
    return targets.isEmpty() && to_select == Self;
}

void Disaster::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;
    DelayedTrick::onUse(room, use);
}

bool Disaster::isAvailable(const Player *player) const
{
    bool ignore = (player->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !hasFlag("IgnoreFailed"));
    if (ignore)
        return true;
    if (player->containsTrick(objectName()))
        return false;

    return !player->isProhibited(player, this) && DelayedTrick::isAvailable(player);
}

Lightning::Lightning(Suit suit, int number) :Disaster(suit, number)
{
    setObjectName("lightning");

    judge.pattern = ".|spade|2~9";
    judge.good = false;
    judge.reason = objectName();
}

void Lightning::takeEffect(ServerPlayer *target) const
{
    target->getRoom()->damage(DamageStruct(this, NULL, target, 3, DamageStruct::Thunder));
}

// EX cards

class IceSwordSkill : public WeaponSkill
{
public:
    IceSwordSkill() : WeaponSkill("IceSword")
    {
        events << DamageCaused;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!equipAvailable(damage.from, EquipCard::WeaponLocation, objectName(), damage.to))
            return QList<SkillInvokeDetail>();

        if (damage.card && damage.by_user && damage.card->isKindOf("Slash") && damage.from->canDiscard(damage.to, "hes") && !damage.chain && !damage.transfer && damage.from != damage.to)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, false, damage.to);

        return QList<SkillInvokeDetail>();
    }


    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *from = invoke->invoker;
        ServerPlayer *to = invoke->targets.first();
        room->setEmotion(from, "weapon/ice_sword");


        if (from->canDiscard(to, "hes")) {
            int card_id = room->askForCardChosen(from, to, "hes", "IceSword", false, Card::MethodDiscard);
            room->throwCard(Sanguosha->getCard(card_id), to, from);

            if (from->isAlive() && to->isAlive() && from->canDiscard(to, "hes")) {
                card_id = room->askForCardChosen(from, to, "hes", "IceSword", false, Card::MethodDiscard);
                room->throwCard(Sanguosha->getCard(card_id), to, from);
            }
        }

        return true;
    }
};

IceSword::IceSword(Suit suit, int number)
    : Weapon(suit, number, 2)
{
    setObjectName("IceSword");
}

class RenwangShieldSkill : public ArmorSkill
{
public:
    RenwangShieldSkill() : ArmorSkill("RenwangShield")
    {
        events << SlashEffected;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (!equipAvailable(effect.to, EquipCard::ArmorLocation, objectName()))
            return QList<SkillInvokeDetail>();

        if (effect.slash->isBlack())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, NULL, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        LogMessage log;
        log.type = "#ArmorNullify";
        log.from = effect.to;
        log.arg = objectName();
        log.arg2 = effect.slash->objectName();
        room->sendLog(log);

        room->setEmotion(effect.to, "armor/renwang_shield");
        effect.to->setFlags("Global_NonSkillNullify");
        return true;
    }
};

RenwangShield::RenwangShield(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("RenwangShield");
}

class HorseSkill : public DistanceSkill
{
public:
    HorseSkill() :DistanceSkill("Horse")
    {
    }

    virtual int getCorrect(const Player *from, const Player *to) const
    {
        int correct = 0;
        const Horse *horse = NULL;
        if (from->getOffensiveHorse() && from->getMark("Equips_Nullified_to_Yourself") == 0) {
            horse = qobject_cast<const Horse *>(from->getOffensiveHorse()->getRealCard());
            if (horse)
                correct += horse->getCorrect();
        }
        if (to->getDefensiveHorse() && to->getMark("Equips_Nullified_to_Yourself") == 0) {
            horse = qobject_cast<const Horse *>(to->getDefensiveHorse()->getRealCard());
            if (horse)
                correct += horse->getCorrect();
        }
        return correct;
    }
};

WoodenOxCard::WoodenOxCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "wooden_ox";
}

void WoodenOxCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->addToPile("wooden_ox", subcards, false);

    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
        if (!p->getTreasure())
            targets << p;
    }
    if (targets.isEmpty())
        return;
    ServerPlayer *target = room->askForPlayerChosen(source, targets, "wooden_ox", "@wooden_ox-move", true);
    if (target) {
        const Card *treasure = source->getTreasure();
        if (treasure)
            room->moveCardTo(treasure, source, target, Player::PlaceEquip,
                             CardMoveReason(CardMoveReason::S_REASON_TRANSFER,
                                            source->objectName(), "wooden_ox", QString()));
    }
}

class WoodenOxSkill : public OneCardViewAsSkill
{
public:
    WoodenOxSkill() : OneCardViewAsSkill("wooden_ox")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("WoodenOxCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        WoodenOxCard *card = new WoodenOxCard;
        card->addSubcard(originalCard);
        card->setSkillName("wooden_ox");
        return card;
    }
};

class WoodenOxTriggerSkill : public TreasureSkill
{
public:
    WoodenOxTriggerSkill() : TreasureSkill("wooden_ox_trigger")
    {
        events << CardsMoveOneTime;
        frequency = Compulsory;
        global = true;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == NULL)
            return;

        if (move.from->hasTreasure("wooden_ox")) {
            int count = 0;
            for (int i = 0; i < move.card_ids.size(); i++) {
                if (move.from_pile_names[i] == "wooden_ox")
                    count++;
            }

            if (count > 0) {
                LogMessage log;
                log.type = "#WoodenOx";
                log.from = qobject_cast<ServerPlayer *>(move.from);
                log.arg = QString::number(count);
                log.arg2 = "wooden_ox";
                room->sendLog(log);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == NULL || move.from->getPile("wooden_ox").isEmpty())
            return QList<SkillInvokeDetail>();

        for (int i = 0; i < move.card_ids.size(); i++) {
            if (move.from_places[i] != Player::PlaceEquip)
                continue;
            const Card *card = Sanguosha->getEngineCard(move.card_ids[i]);
            if (card->objectName() == "wooden_ox") {
                ServerPlayer *invoker = qobject_cast<ServerPlayer *>(move.from);
                if (!move.reason.m_playerId.isEmpty())
                    invoker = room->findPlayerByObjectName(move.reason.m_playerId);

                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, qobject_cast<ServerPlayer *>(move.from), invoker, NULL, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *to = qobject_cast<ServerPlayer *>(move.to);
        if (to && to->getTreasure() && to->getTreasure()->objectName() == "wooden_ox" && move.to_place == Player::PlaceEquip) {
            CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, "", NULL, "wooden_ox", "");
            to->addToPile("wooden_ox", invoke->owner->getPile("wooden_ox"), false, reason);
        } else
            invoke->owner->clearOnePrivatePile("wooden_ox");
        room->addPlayerHistory(invoke->owner, "WoodenOxCard", -1);
        return false;
    }
};

WoodenOx::WoodenOx(Suit suit, int number)
    : Treasure(suit, number)
{
    setObjectName("wooden_ox");
}

void WoodenOx::onUninstall(ServerPlayer *player) const
{
    player->getRoom()->addPlayerHistory(player, "WoodenOxCard", 0);
    Treasure::onUninstall(player);
}

StandardCardPackage::StandardCardPackage()
    : Package("standard_cards", Package::CardPack)
{
    QList<Card *> cards;

    cards << new Slash(Card::Spade, 7)
          << new Slash(Card::Spade, 8)
          << new Slash(Card::Spade, 8)
          << new Slash(Card::Spade, 9)
          << new Slash(Card::Spade, 9)
          << new Slash(Card::Spade, 10)
          << new Slash(Card::Spade, 10)

          << new Slash(Card::Club, 2)
          << new Slash(Card::Club, 3)
          << new Slash(Card::Club, 4)
          << new Slash(Card::Club, 5)
          << new Slash(Card::Club, 6)
          << new Slash(Card::Club, 7)
          << new Slash(Card::Club, 8)
          << new Slash(Card::Club, 8)
          << new Slash(Card::Club, 9)
          << new Slash(Card::Club, 9)
          << new Slash(Card::Club, 10)
          << new Slash(Card::Club, 10)
          << new Slash(Card::Club, 11)
          << new Slash(Card::Club, 11)

          << new Slash(Card::Heart, 10)
          << new Slash(Card::Heart, 10)
          << new Slash(Card::Heart, 11)

          << new Slash(Card::Diamond, 6)
          << new Slash(Card::Diamond, 7)
          << new Slash(Card::Diamond, 8)
          << new Slash(Card::Diamond, 9)
          << new Slash(Card::Diamond, 10)
          << new Slash(Card::Diamond, 13)

          << new Jink(Card::Heart, 2)
          << new Jink(Card::Heart, 2)
          << new Jink(Card::Heart, 13)

          << new Jink(Card::Diamond, 2)
          << new Jink(Card::Diamond, 2)
          << new Jink(Card::Diamond, 3)
          << new Jink(Card::Diamond, 4)
          << new Jink(Card::Diamond, 5)
          << new Jink(Card::Diamond, 6)
          << new Jink(Card::Diamond, 7)
          << new Jink(Card::Diamond, 8)
          << new Jink(Card::Diamond, 9)
          << new Jink(Card::Diamond, 10)
          << new Jink(Card::Diamond, 11)
          << new Jink(Card::Diamond, 11)

          << new Peach(Card::Heart, 3)
          << new Peach(Card::Heart, 4)
          << new Peach(Card::Heart, 6)
          << new Peach(Card::Heart, 7)
          << new Peach(Card::Heart, 8)
          << new Peach(Card::Heart, 9)
          << new Peach(Card::Heart, 12)

          << new Peach(Card::Diamond, 12)

          << new Triblade(Card::Club)
             //<< new Crossbow(Card::Club)
          << new Crossbow(Card::Diamond)
          << new DoubleSword
          << new QinggangSword
          << new Blade
          << new Spear
          << new Axe
          << new Halberd
          << new KylinBow

          << new EightDiagram(Card::Spade)
          << new BreastPlate(Card::Club);
    //<< new EightDiagram(Card::Club);

    skills << new DoubleSwordSkill << new QinggangSwordSkill
           << new BladeSkill << new SpearSkill << new AxeSkill
           << new KylinBowSkill << new EightDiagramSkill
           << new HalberdSkill << new BreastPlateSkill << new TribladeSkill;

    QList<Card *> horses;
    horses << new DefensiveHorse(Card::Spade, 5)
           << new DefensiveHorse(Card::Club, 5)
           << new DefensiveHorse(Card::Heart, 13)
           << new OffensiveHorse(Card::Heart, 5)
           << new OffensiveHorse(Card::Spade, 13)
           << new OffensiveHorse(Card::Diamond, 13);

    horses.at(0)->setObjectName("JueYing");
    horses.at(1)->setObjectName("DiLu");
    horses.at(2)->setObjectName("ZhuaHuangFeiDian");
    horses.at(3)->setObjectName("ChiTu");
    horses.at(4)->setObjectName("DaYuan");
    horses.at(5)->setObjectName("ZiXing");

    cards << horses;

    addMetaObject<TribladeCard>();
    skills << new HorseSkill;

    cards << new AmazingGrace(Card::Heart, 3)
          << new AmazingGrace(Card::Heart, 4)
          << new GodSalvation
          << new SavageAssault(Card::Spade, 7)
          << new SavageAssault(Card::Spade, 13)
          << new SavageAssault(Card::Club, 7)
          << new ArcheryAttack
          << new Duel(Card::Spade, 1)
          << new Duel(Card::Club, 1)
          << new Duel(Card::Diamond, 1)
          << new ExNihilo(Card::Heart, 7)
          << new ExNihilo(Card::Heart, 8)
          << new ExNihilo(Card::Heart, 9)
          << new ExNihilo(Card::Heart, 11)
          << new Snatch(Card::Spade, 3)
          << new Snatch(Card::Spade, 4)
          << new Snatch(Card::Spade, 11)
          << new Snatch(Card::Diamond, 3)
          << new Snatch(Card::Diamond, 4)
          << new Dismantlement(Card::Spade, 3)
          << new Dismantlement(Card::Spade, 4)
          << new Dismantlement(Card::Spade, 12)
          << new Dismantlement(Card::Club, 3)
          << new Dismantlement(Card::Club, 4)
          << new Dismantlement(Card::Heart, 12)
          << new Collateral(Card::Club, 12)
          << new Collateral(Card::Club, 13)
          << new Nullification(Card::Spade, 11)
          << new Nullification(Card::Club, 12)
          << new Nullification(Card::Club, 13)
          << new Indulgence(Card::Spade, 6)
          << new Indulgence(Card::Club, 6)
          << new Indulgence(Card::Heart, 6)
          << new Lightning(Card::Spade, 1);

    foreach(Card *card, cards)
        card->setParent(this);
}

StandardExCardPackage::StandardExCardPackage()
    : Package("standard_ex_cards", Package::CardPack)
{
    QList<Card *> cards;
    cards << new IceSword(Card::Spade, 2)
          << new RenwangShield(Card::Club, 2)
          << new Lightning(Card::Heart, 12)
          << new Nullification(Card::Diamond, 12)
          << new WoodenOx(Card::Diamond, 5);
    skills << new RenwangShieldSkill << new IceSwordSkill << new WoodenOxSkill << new WoodenOxTriggerSkill;

    foreach(Card *card, cards)
        card->setParent(this);

    addMetaObject<WoodenOxCard>();
}

ADD_PACKAGE(StandardCard)
ADD_PACKAGE(StandardExCard)

