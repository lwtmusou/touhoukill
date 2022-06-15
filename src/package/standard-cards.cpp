#include "ai.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "room.h"
#include "settings.h"
#include "standard-equips.h"
#include "standard.h"

#include <functional>

Slash::Slash(Suit suit, int number)
    : BasicCard(suit, number)
{
    setObjectName("slash");
    nature = DamageStruct::Normal;
    drank = 0;
    can_damage = true;
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
        if (slash != nullptr) {
            if (slash->isVirtualCard()) {
                if (slash->subcardsLength() > 0)
                    ids = slash->getSubcards();
            } else {
                ids << slash->getEffectiveId();
            }
        }
        bool has_weapon = (player->hasWeapon("Crossbow", true) || player->hasWeapon("VSCrossbow", true)) && (player->getWeapon() != nullptr)
            && ids.contains(player->getWeapon()->getEffectiveId());
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
    else if (from->getPhase() == Player::Play && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !Slash::IsAvailable(from, slash, false)) {
        QStringList assignee_list = from->property("extra_slash_specific_assignee").toString().split("+");
        if (assignee_list.contains(player->objectName()))
            return true;
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
        foreach (ServerPlayer *target, room->getAlivePlayers())
            if (target->hasFlag("SlashAssignee"))
                room->setPlayerFlag(target, "-SlashAssignee");
    }

    int num1 = use.to.length();
    if (((use.card->isVirtualCard() && use.card->subcardsLength() == 0) || use.card->hasFlag("chosenExtraSlashTarget")) && !player->hasFlag("slashDisableExtraTarget")) {
        QList<ServerPlayer *> targets_ts;
        while (true) {
            QList<const Player *> targets_const;
            foreach (ServerPlayer *p, use.to)
                targets_const << qobject_cast<const Player *>(p);
            foreach (ServerPlayer *p, room->getAlivePlayers())
                if (!use.to.contains(p) && use.card->targetFilter(targets_const, p, use.from))
                    targets_ts << p;
            if (targets_ts.isEmpty())
                break;

            ServerPlayer *extra_target = room->askForPlayerChosen(player, targets_ts, "slash_extra_targets", "@slash_extra_targets", true);
            if (extra_target != nullptr) {
                use.to.append(extra_target);
                room->sortByActionOrder(use.to);
                if (player->hasSkill("shuangren") && !player->hasShownSkill("shuangren"))
                    player->showHiddenSkill("shuangren");
            } else
                break;
            targets_ts.clear();
            targets_const.clear();
        }
    }
    //like dingfeng duanbing
    if (player->canShowHiddenSkill()) {
        int num2 = use.to.length() - num1;
        QStringList skills;
        if (num2 > 1) {
            if (player->isHiddenSkill("kexue") && player->isChained())
                skills << "kexue";
        } else if (num2 == 1) {
            if (player->isHiddenSkill("kexue") && player->isChained())
                skills << "kexue";
            if (player->isHiddenSkill("shuangren"))
                skills << "shuangren";
        }
        if (!skills.isEmpty()) {
            QString to_show = room->askForChoice(player, "tarmod_show", skills.join("+"), QVariant::fromValue(use));
            player->showHiddenSkill(to_show);
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
    } else if (use.to.size() > 1 && player->hasSkill("shuangren")) {
        room->broadcastSkillInvoke("shuangren");
        room->notifySkillInvoked(player, "shuangren");
    }

    int rangefix = 0;
    if (use.card->isVirtualCard()) {
        if ((use.from->getWeapon() != nullptr) && use.card->getSubcards().contains(use.from->getWeapon()->getId())) {
            const Weapon *weapon = qobject_cast<const Weapon *>(use.from->getWeapon()->getRealCard());
            rangefix += weapon->getRange() - use.from->getAttackRange(false);
        }

        if ((use.from->getOffensiveHorse() != nullptr) && use.card->getSubcards().contains(use.from->getOffensiveHorse()->getId()))
            rangefix += 1;
    }
    if (use.card->isVirtualCard() && use.card->getSkillName() == "Spear")
        room->setEmotion(player, "weapon/spear");
    else if (use.to.size() > 1 && player->hasWeapon("Halberd") && player->isLastHandCard(this))
        room->setEmotion(player, "weapon/halberd");
    else if (use.card->isVirtualCard() && use.card->getSkillName() == "Pillar")
        room->setEmotion(player, "weapon/pillar");

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
    effect.effectValue = card_effect.effectValue;

    QVariantList jink_list = effect.from->tag["Jink_" + toString()].toList();
    effect.jink_num = jink_list.takeFirst().toInt();

    if (jink_list.isEmpty())
        effect.from->tag.remove("Jink_" + toString());
    else
        effect.from->tag["Jink_" + toString()] = QVariant::fromValue(jink_list);

    room->slashEffect(effect);
}

bool Slash::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return !targets.isEmpty();
}

bool Slash::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int slash_targets = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    bool distance_limit = ((1 + Sanguosha->correctCardTarget(TargetModSkill::DistanceLimit, Self, this)) < 500);
    if (Self->hasFlag("slashNoDistanceLimit"))
        distance_limit = false;

    int rangefix = 0;
    if ((Self->getWeapon() != nullptr) && subcards.contains(Self->getWeapon()->getId())) {
        const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        rangefix += weapon->getRange() - Self->getAttackRange(false);
    }

    if ((Self->getOffensiveHorse() != nullptr) && subcards.contains(Self->getOffensiveHorse()->getId()))
        rangefix += 1;

    bool has_specific_assignee = false;
    foreach (const Player *p, Self->getAliveSiblings()) {
        if (Slash::IsSpecificAssignee(p, Self, this)) {
            has_specific_assignee = true;
            break;
        }
    }

    if (has_specific_assignee) {
        if (targets.isEmpty())
            return Slash::IsSpecificAssignee(to_select, Self, this) && Self->canSlash(to_select, this, distance_limit, rangefix);
        else {
            if (Self->hasFlag("slashDisableExtraTarget"))
                return false;
            bool canSelect = false;
            foreach (const Player *p, targets) {
                if (Slash::IsSpecificAssignee(p, Self, this)) {
                    canSelect = true;
                    break;
                }
            }
            if (!canSelect)
                return false;
        }
    }
    bool has_shuangren_target = false;
    if (Self->hasSkill("shuangren") && targets.length() >= 1) { //&& distance_limit
        foreach (const Player *p, targets) {
            if (Self->distanceTo(p, rangefix) > Self->getAttackRange(true)) { //double hidden, like shuangren + tianqu. since tianqu has be used, you can not use shuangren.
                has_shuangren_target = true;
                break;
            }
        }
        if (isVirtualCard() && subcardsLength() == 0 && !Self->hasFlag("slashDisableExtraTarget"))
            distance_limit = false;
        else {
            foreach (const Player *p, targets) {
                if (Self->distanceTo(p, rangefix) > Self->getAttackRange(true) && !Slash::IsSpecificAssignee(p, Self, this)) {
                    has_shuangren_target = true;
                    break;
                }
            }
        }
        if (!has_shuangren_target)
            distance_limit = false;
    }

    if (!Self->canSlash(to_select, this, distance_limit, rangefix, targets))
        return false;
    else if (has_shuangren_target) { // slove the conflict of "hidden shuangren" + "hidden tianqu" + "Halberd"
        bool halberd = EquipSkill::equipAvailable(Self, EquipCard::WeaponLocation, "Halberd") && Self->isLastHandCard(this); //  TargetMod from Wepaon Skill
        bool tianqu = Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !hasFlag("IgnoreFailed");

        QList<const Player *> over_distance_limit_targets;
        foreach (const Player *p, targets) {
            if (Self->distanceTo(p, rangefix) > Self->getAttackRange(true)) {
                over_distance_limit_targets << p;
            }
        }

        if (over_distance_limit_targets.length() == 2 && targets.length() < slash_targets) {
            if (halberd && tianqu) {
                slash_targets = slash_targets - 1; //hidden tianqu has been used, so hidden shuangren will be not used.
            }
        } else if (over_distance_limit_targets.length() == 1 && targets.length() < slash_targets) {
            if (!halberd && tianqu) {
                if (Self->distanceTo(to_select, rangefix) > Self->getAttackRange(true))
                    return false;
            }
            if (halberd && targets.length() == 3) {
                if (Self->distanceTo(to_select, rangefix) > Self->getAttackRange(true))
                    return false;
            }
        }
    }

    if (targets.length() >= slash_targets)
        return false;

    return true;
}

Jink::Jink(Suit suit, int number)
    : BasicCard(suit, number)
{
    setObjectName("jink");
    target_fixed = true;
    has_effectvalue = false;
}

QString Jink::getSubtype() const
{
    return "defense_card";
}

bool Jink::isAvailable(const Player *) const
{
    return false;
}

Peach::Peach(Suit suit, int number)
    : BasicCard(suit, number)
{
    setObjectName("peach");
    target_fixed = true;
    can_recover = true;
}

QString Peach::getSubtype() const
{
    return "recover_card";
}

bool Peach::targetFixed(const Player *Self) const
{
    bool ignore = ((Self != nullptr) && Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !hasFlag("IgnoreFailed"));
    if (ignore)
        return false;
    if ((Self != nullptr) && Self->hasFlag("Global_shehuoInvokerFailed"))
        return false;
    bool riyue_ignore = ((Self != nullptr) && Self->hasSkill("riyue") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !hasFlag("IgnoreFailed"));
    if (riyue_ignore && ((canDamage() && isRed()) || (canRecover() && isBlack())))
        return false;
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
    if (effect.to->isDead())
        return;
    Room *room = effect.to->getRoom();
    room->setEmotion(effect.from, "peach");

    if (!effect.to->isWounded())
        room->setCardFlag(this, "-tianxieEffected_" + effect.to->objectName()); //only for skill tianxie

    // recover hp
    RecoverStruct recover;
    recover.card = this;
    recover.who = effect.from;
    recover.recover = 1 + effect.effectValue.first();
    room->recover(effect.to, recover);
}

bool Peach::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !hasFlag("IgnoreFailed")) {
        if (Self != to_select)
            return true;
    }

    if (Self->hasFlag("Global_shehuoInvokerFailed"))
        return (to_select->hasFlag("Global_shehuoFailed") && to_select->isWounded());

    if (Self->hasSkill("riyue") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !hasFlag("IgnoreFailed")) {
        if ((canDamage() && isRed()) || (canRecover() && isBlack()))
            return to_select->isWounded();
    }

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
            if (to_select == Self)
                return true;
            if (Self->getKingdom() == "zhan" && Self->getPhase() == Player::Play && to_select->hasLordSkill("yanhui"))
                return true;
        }
    }
    return false;
}

bool Peach::isAvailable(const Player *player) const
{
    if (!BasicCard::isAvailable(player))
        return false;
    bool isPlay = Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY;
    bool ignore = (player->hasSkill("tianqu") && isPlay && !hasFlag("IgnoreFailed"));
    if (ignore)
        return true;
    if (player->isWounded() && !player->isProhibited(player, this))
        return true;
    foreach (const Player *p, player->getAliveSiblings()) {
        if (!player->isProhibited(p, this)) {
            if (p->hasFlag("Global_Dying") && !isPlay)
                return true;
        }
    }
    return false;
}

Crossbow::Crossbow(Suit suit, int number)
    : Weapon(suit, number, 1)
{
    setObjectName("Crossbow");
}

TribladeCard::TribladeCard()
    : SkillCard()
{
    m_skillName = "Triblade";
}

bool TribladeCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.length() == 0 && to_select->hasFlag("Global_TribladeFailed");
}

void TribladeCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    const ViewHasSkill *v = Sanguosha->ViewHas(card_use.from, "Triblade", "weapon", true);
    if (v != nullptr)
        card_use.from->showHiddenSkill(v->objectName());
    room->setEmotion(card_use.from, "weapon/triblade");
    SkillCard::onUse(room, card_use);
}

void TribladeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->damage(DamageStruct("Triblade", source, targets[0]));
}

class TribladeSkillVS : public OneCardViewAsSkill
{
public:
    TribladeSkillVS()
        : OneCardViewAsSkill("Triblade")
    {
        response_pattern = "@@Triblade";
        filter_pattern = ".|.|.|hand!";
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        TribladeCard *c = new TribladeCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class TribladeSkill : public WeaponSkill
{
public:
    TribladeSkill()
        : WeaponSkill("Triblade")
    {
        events << Damage;
        view_as_skill = new TribladeSkillVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!equipAvailable(damage.from, EquipCard::WeaponLocation, objectName()) || damage.from->isDead() || !damage.from->canDiscard(damage.from, "hs"))
            return QList<SkillInvokeDetail>();

        if ((damage.to != nullptr) && damage.to->isAlive() && (damage.card != nullptr) && damage.card->isKindOf("Slash") && damage.by_user && !damage.chain && !damage.transfer) {
            foreach (ServerPlayer *p, room->getOtherPlayers(damage.from)) {
                if (damage.to->distanceTo(p) == 1)
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        foreach (ServerPlayer *p, room->getOtherPlayers(damage.from)) {
            if (damage.to->distanceTo(p) == 1)
                room->setPlayerFlag(p, "Global_TribladeFailed");
        }
        room->askForUseCard(invoke->invoker, "@@Triblade", "@Triblade");
        return false;
    }
};

Triblade::Triblade(Card::Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("Triblade");
}

class DoubleSwordSkill : public WeaponSkill
{
public:
    DoubleSwordSkill()
        : WeaponSkill("DoubleSword")
    {
        events << TargetSpecified;
    }

    static bool diff(ServerPlayer *player, ServerPlayer *target)
    {
        ServerPlayer *lord = player->getRoom()->getLord();
        QString lordKingdom = "";
        if (lord != nullptr)
            lordKingdom = lord->getKingdom();
        bool can = false;
        if (lordKingdom == "")
            can = ((player->isMale() && target->isFemale()) || (player->isFemale() && target->isMale()));
        else
            can = (player->getKingdom() != target->getKingdom() && (player->getKingdom() == lordKingdom || target->getKingdom() == lordKingdom));
        return can;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!equipAvailable(use.from, EquipCard::WeaponLocation, objectName()))
            return QList<SkillInvokeDetail>();

        if (isHegemonyGameMode(room->getMode()))
            return QList<SkillInvokeDetail>();

        if (use.card != nullptr && use.card->isKindOf("Slash")) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, use.to) {
                if (p->isAlive() && use.from != nullptr && diff(use.from, p)) {
                    if (!equipAvailable(use.from, EquipCard::WeaponLocation, objectName(), p))
                        continue;
                    d << SkillInvokeDetail(this, use.from, use.from, nullptr, false, p);
                }
            }

            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->tag["DoubleSwordTarget"] = QVariant::fromValue(invoke->preferredTarget);
        if (invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget))) {
            room->setEmotion(invoke->invoker, "weapon/double_sword");
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        bool draw_card = false;

        if (!invoke->targets.first()->canDiscard(invoke->targets.first(), "hs"))
            draw_card = true;
        else {
            QString prompt = "double-sword-card:" + invoke->invoker->objectName();
            const Card *card = room->askForCard(invoke->targets.first(), ".", prompt, data);
            if (card == nullptr)
                draw_card = true;
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
    QinggangSwordSkill()
        : WeaponSkill("QinggangSword")
    {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!equipAvailable(use.from, EquipCard::WeaponLocation, objectName()))
            return QList<SkillInvokeDetail>();

        if (use.from != nullptr && use.card != nullptr && use.card->isKindOf("Slash")) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, use.to) {
                if (!equipAvailable(use.from, EquipCard::WeaponLocation, objectName(), p))
                    continue;
                d << SkillInvokeDetail(this, use.from, use.from, nullptr, true, p);
            }

            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        const ViewHasSkill *v = Sanguosha->ViewHas(invoke->invoker, objectName(), "weapon", true);
        if (v != nullptr) {
            if (!invoke->invoker->hasShownSkill(v) && !invoke->invoker->askForSkillInvoke(this))
                return false;
            invoke->invoker->showHiddenSkill(v->objectName());
        }

        if (((invoke->preferredTarget->getArmor() != nullptr) && invoke->preferredTarget->hasArmorEffect(invoke->preferredTarget->getArmor()->objectName()))
            || invoke->preferredTarget->hasArmorEffect("shenbao"))
            room->setEmotion(invoke->invoker, "weapon/qinggang_sword");
        return true;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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
    BladeSkill()
        : WeaponSkill("Blade")
    {
        events << TargetSpecified;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash"))
            return QList<SkillInvokeDetail>();
        if (!equipAvailable(use.from, EquipCard::WeaponLocation, objectName()))
            return QList<SkillInvokeDetail>();

        if (use.from != nullptr && use.from->isAlive())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->setEmotion(invoke->invoker, "weapon/blade");

        JudgeStruct judge;
        judge.pattern = ".|red";
        judge.good = true;
        judge.reason = objectName();
        judge.who = invoke->invoker;

        room->judge(judge);
        if (judge.isGood() && !judge.ignore_judge) {
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
    SpearSkill()
        : ViewAsSkill("Spear")
    {
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        bool avalilable = Slash::IsAvailable(player);
        return avalilable && player->getMark("Equips_Nullified_to_Yourself") == 0 && player->hasWeapon(objectName(), false, true);
        //&& EquipSkill::equipAvailable(player, EquipCard::WeaponLocation, objectName());
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        Slash *card = new Slash(Card::SuitToBeDecided, -1);
        DELETE_OVER_SCOPE(Slash, card)
        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);
        return cardPattern != nullptr && cardPattern->match(player, card) && player->getMark("Equips_Nullified_to_Yourself") == 0 && player->hasWeapon(objectName(), false, true);
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        return selected.length() < 2 && !to_select->isEquipped();
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() != 2)
            return nullptr;

        Slash *slash = new Slash(Card::SuitToBeDecided, 0);
        slash->setSkillName(objectName());

        const ViewHasSkill *v = Sanguosha->ViewHas(Self, objectName(), "weapon", true);
        if ((v != nullptr) && v->objectName().contains("shenbao"))
            slash->setSkillName("shenbao");
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
    AxeViewAsSkill()
        : ViewAsSkill("Axe")
    {
        response_pattern = "@Axe";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        if (Self->hasWeapon(objectName(), true) && to_select == Self->getWeapon()) //check if cannot throw selfweapon
            return false;
        return selected.length() < 2 && !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        if (cards.length() != 2)
            return nullptr;

        DummyCard *card = new DummyCard;
        card->setSkillName(objectName());
        card->addSubcards(cards);
        return card;
    }
};

class AxeSkill : public WeaponSkill
{
public:
    AxeSkill()
        : WeaponSkill("Axe")
    {
        events << SlashMissed;
        view_as_skill = new AxeViewAsSkill;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (!equipAvailable(effect.from, EquipCard::WeaponLocation, objectName()))
            return QList<SkillInvokeDetail>();

        if (!effect.to->isAlive() || effect.jink == nullptr)
            return QList<SkillInvokeDetail>();

        int cardLimit = 2;
        if (effect.from->hasWeapon(objectName(), true))
            cardLimit = 3;
        if (effect.from->getCardCount(true) >= cardLimit)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.from, effect.from); // this skill don't have a target

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        invoke->invoker->tag["axe_target"] = QVariant::fromValue(effect.to);
        const Card *card = room->askForCard(invoke->invoker, "@Axe", "@Axe:" + effect.to->objectName(), data, objectName());
        if (card != nullptr) {
            room->setEmotion(invoke->invoker, "weapon/axe");
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        room->slashResult(effect, nullptr);
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
    HalberdSkill()
        : TargetModSkill("Halberd")
    {
        frequency = NotCompulsory;
    }

    int getExtraTargetNum(const Player *from, const Card *card) const override
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
    KylinBowSkill()
        : WeaponSkill("KylinBow")
    {
        events << DamageCaused;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!equipAvailable(damage.from, EquipCard::WeaponLocation, objectName(), damage.to))
            return QList<SkillInvokeDetail>();

        if ((damage.card != nullptr) && damage.card->isKindOf("Slash") && damage.by_user && !damage.chain && !damage.transfer) {
            if ((damage.to->getDefensiveHorse() != nullptr) && damage.from->canDiscard(damage.to, damage.to->getDefensiveHorse()->getEffectiveId()))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, false, damage.to);
            if ((damage.to->getOffensiveHorse() != nullptr) && damage.from->canDiscard(damage.to, damage.to->getOffensiveHorse()->getEffectiveId()))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, false, damage.to);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        invoke->invoker->tag[this->objectName()] = data;
        if (invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget))) {
            const ViewHasSkill *v = Sanguosha->ViewHas(invoke->invoker, objectName(), "weapon", true);
            if (v != nullptr)
                invoke->invoker->showHiddenSkill(v->objectName());
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        QStringList horses;
        DamageStruct damage = data.value<DamageStruct>();
        if ((damage.to->getDefensiveHorse() != nullptr) && damage.from->canDiscard(damage.to, damage.to->getDefensiveHorse()->getEffectiveId()))
            horses << "dhorse";
        if ((damage.to->getOffensiveHorse() != nullptr) && damage.from->canDiscard(damage.to, damage.to->getOffensiveHorse()->getEffectiveId()))
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
    EightDiagramSkill()
        : ArmorSkill("EightDiagram")
    {
        events << CardAsked;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardAskedStruct ask = data.value<CardAskedStruct>();
        Card *jink = Sanguosha->cloneCard("jink");
        DELETE_OVER_SCOPE(Card, jink)

        const CardPattern *pattern = Sanguosha->getPattern(ask.pattern);

        if (!(pattern != nullptr && pattern->match(ask.player, jink)))
            return QList<SkillInvokeDetail>();

        ServerPlayer *player = ask.player;
        if (!equipAvailable(player, EquipCard::ArmorLocation, objectName()))
            return QList<SkillInvokeDetail>();

        //since skill yuanfei,we need check
        if (player->isCardLimited(jink, ask.method))
            return QList<SkillInvokeDetail>();
        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (invoke->invoker->askForSkillInvoke(this, data)) {
            const ViewHasSkill *v = Sanguosha->ViewHas(invoke->invoker, objectName(), "armor", true);
            if (v != nullptr)
                invoke->invoker->showHiddenSkill(v->objectName());
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        int armor_id = -1;
        if (invoke->invoker->getArmor() != nullptr) {
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

        if (judge.isGood() && !judge.ignore_judge) {
            Jink *jink = new Jink(Card::NoSuit, 0);
            jink->setSkillName(objectName());
            room->provide(jink);
            return true;
        }

        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *) const override
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
    BreastPlateSkill()
        : ArmorSkill("BreastPlate")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.damage >= damage.to->getHp() && damage.to->isAlive() && (damage.to->getArmor() != nullptr) && damage.to->getArmor()->objectName() == objectName()
            && equipAvailable(damage.to, EquipCard::ArmorLocation, objectName())) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->setEmotion(invoke->invoker, "armor/breast_plate");
        int id = invoke->invoker->getArmor()->getEffectiveId();
        invoke->invoker->addBrokenEquips(QList<int>() << id);
        DamageStruct damage = data.value<DamageStruct>();
        LogMessage log;
        log.type = "#BreastPlate";
        log.from = invoke->invoker;
        if (damage.from != nullptr)
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
    if (ag_list.isEmpty())
        return;
    DummyCard *dummy = new DummyCard(VariantList2IntList(ag_list));
    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), "amazing_grace", QString());
    room->throwCard(dummy, reason, nullptr);
    delete dummy;
}

void AmazingGrace::doPreAction(Room *room, const CardUseStruct &use) const
{
    int count = room->getAllPlayers().length();
    foreach (ServerPlayer *p, room->getAllPlayers()) {
        if (p->hasSkill("shouhuo") && p->hasShownSkill("shouhuo")) {
            room->notifySkillInvoked(p, "shouhuo");
            room->touhouLogmessage("#TriggerSkill", p, "shouhuo");
            count++;
        }
    }
    QList<int> card_ids = room->getNCards(count);
    CardsMoveStruct move(card_ids, nullptr, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, use.from->objectName(), objectName(), QString()));
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
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            clearRestCards(room);
        throw triggerEvent;
    }
}

void AmazingGrace::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    QVariantList ag_list = room->getTag("AmazingGrace").toList();
    if (ag_list.isEmpty() || effect.to->isDead())
        return;
    QList<int> card_ids;
    foreach (QVariant card_id, ag_list)
        card_ids << card_id.toInt();

    int times = 1 + effect.effectValue.first();
    if (effect.to->hasSkill("shouhuo") && effect.to->hasShownSkill("shouhuo"))
        times++;
    for (int i = 0; i < times; i += 1) {
        int card_id = room->askForAG(effect.to, card_ids, false, objectName());
        card_ids.removeOne(card_id);

        room->takeAG(effect.to, card_id, true, QList<ServerPlayer *>(), Player::PlaceTable); // !!!! AG is taken from PlaceTable now!!!!!!!!!
        ag_list.removeOne(card_id);
        if (ag_list.isEmpty())
            break;
    }

    room->setTag("AmazingGrace", ag_list);
}

bool AmazingGrace::isCancelable(const CardEffectStruct &effect) const
{
    QVariantList ag_list = effect.to->getRoom()->getTag("AmazingGrace").toList();
    return !ag_list.isEmpty() && TrickCard::isCancelable(effect);
}

GodSalvation::GodSalvation(Suit suit, int number)
    : GlobalEffect(suit, number)
{
    setObjectName("god_salvation");
    can_recover = true;
}

bool GodSalvation::isCancelable(const CardEffectStruct &effect) const
{
    return effect.to->isWounded() && TrickCard::isCancelable(effect);
}

void GodSalvation::onEffect(const CardEffectStruct &effect) const
{
    if (effect.to->isDead())
        return;
    Room *room = effect.to->getRoom();
    if (!effect.to->isWounded())
        room->setCardFlag(this, "-tianxieEffected_" + effect.to->objectName()); //only for skill tianxie
    else {
        RecoverStruct recover;
        recover.card = this;
        recover.who = effect.from;
        recover.recover = 1 + effect.effectValue.first();
        room->recover(effect.to, recover);
    }
}

SavageAssault::SavageAssault(Suit suit, int number)
    : AOE(suit, number)
{
    setObjectName("savage_assault");
    can_damage = true;
}

void SavageAssault::onEffect(const CardEffectStruct &effect) const
{
    if (effect.to->isDead())
        return;
    Room *room = effect.to->getRoom();
    int times = 1 + effect.effectValue.first();
    bool dodamage = false;
    for (int i = 0; i < times; i += 1) {
        QString prompt = QString("savage-assault-slash:%1:%2:%3").arg(effect.from->objectName()).arg(times).arg(i + 1);
        const Card *slash = room->askForCard(effect.to, "slash", prompt, QVariant::fromValue(effect), Card::MethodResponse, effect.from->isAlive() ? effect.from : nullptr);
        if (slash != nullptr) {
            if (slash->getSkillName() == "Spear")
                room->setEmotion(effect.to, "weapon/spear");
            room->setEmotion(effect.to, "killer");
        } else {
            dodamage = true;
            break;
        }
    }

    if (dodamage) {
        //room->damage(DamageStruct(this, effect.from->isAlive() ? effect.from : NULL, effect.to, 1 + effect.effectValue.last()));
        room->damage(DamageStruct(effect.card, effect.from->isAlive() ? effect.from : nullptr, effect.to, 1 + effect.effectValue.last()));
        room->getThread()->delay();
    }
}

ArcheryAttack::ArcheryAttack(Card::Suit suit, int number)
    : AOE(suit, number)
{
    setObjectName("archery_attack");
    can_damage = true;
}

void ArcheryAttack::onEffect(const CardEffectStruct &effect) const
{
    if (effect.to->isDead())
        return;
    Room *room = effect.to->getRoom();
    int times = 1 + effect.effectValue.first();
    bool dodamage = false;
    for (int i = 0; i < times; i += 1) {
        QString prompt = QString("archery-attack-jink:%1:%2:%3").arg(effect.from->objectName()).arg(times).arg(i + 1);
        const Card *jink = room->askForCard(effect.to, "jink", prompt, QVariant::fromValue(effect), Card::MethodResponse, effect.from->isAlive() ? effect.from : nullptr);
        if ((jink != nullptr) && jink->getSkillName() != "eight_diagram" && jink->getSkillName() != "bazhen")
            room->setEmotion(effect.to, "jink");
        if (jink == nullptr) {
            dodamage = true;
            break;
        }
    }

    if (dodamage) {
        room->damage(DamageStruct(effect.card, effect.from->isAlive() ? effect.from : nullptr, effect.to, 1 + effect.effectValue.last()));
        //room->damage(DamageStruct(this, effect.from->isAlive() ? effect.from : NULL, effect.to, 1 + effect.effectValue.last()));
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
        if ((p->getWeapon() != nullptr) || (p != player && ignore)) {
            canUse = true;
            break;
        }
    }
    return canUse && SingleTargetTrick::isAvailable(player);
}

bool Collateral::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() < total_num) {
        bool ignore = (Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && to_select != Self && !hasFlag("IgnoreFailed"));
        if (to_select == Self)
            return false;
        if ((to_select->getWeapon() == nullptr) && !ignore)
            return false;
        return true;
    }
    return false;
}

bool Collateral::doCollateral(Room *room, ServerPlayer *killer, ServerPlayer *victim, const QString &prompt) const
{
    bool useSlash = false;
    if (killer->canSlash(victim, nullptr, false))
        useSlash = (room->askForUseSlashTo(killer, victim, prompt) != nullptr);
    return useSlash;
}

void Collateral::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    Room *room = source->getRoom();
    ServerPlayer *killer = effect.to;
    if (!source->isAlive() || !killer->isAlive())
        return;
    int times = 1 + effect.effectValue.first();

    QList<ServerPlayer *> targets;
    QList<ServerPlayer *> victims;
    foreach (ServerPlayer *p, room->getOtherPlayers(killer)) {
        if (killer->canSlash(p))
            targets << p;
    }

    if (targets.isEmpty())
        return;
    QString prompt = QString("collateral-chooseVictim:%1").arg(killer->objectName());
    for (int i = 0; i < times; i += 1) {
        source->tag["collateral-killer"] = QVariant::fromValue(killer);
        ServerPlayer *victim = room->askForPlayerChosen(source, targets, "collateral", prompt);
        LogMessage log;
        log.type = "#CollateralSlash";
        log.from = killer;
        log.to << victim;
        room->sendLog(log);
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, killer->objectName(), victim->objectName());
        victims << victim;
        targets.removeOne(victim);
        if (targets.isEmpty())
            break;
    }
    room->sortByActionOrder(victims);
    foreach (ServerPlayer *v, victims) {
        WrappedCard *weapon = killer->getWeapon();
        prompt = QString("collateral-slash:%1:%2").arg(v->objectName()).arg(source->objectName());
        bool doSlash = doCollateral(room, killer, v, prompt);
        if (!doSlash && source->isAlive() && (killer->getWeapon() != nullptr))
            source->obtainCard(weapon);
        if (!doSlash)
            break;
    }
}

Nullification::Nullification(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    target_fixed = true;
    setObjectName("nullification");
    has_effectvalue = false;
}

void Nullification::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    // does nothing, just throw it
    if (room->getCardPlace(getEffectiveId()) == Player::PlaceTable) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName());
        room->moveCardTo(this, source, nullptr, Player::DiscardPile, reason);
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
    if (effect.to->isDead())
        return;
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
        if (friend_num < enemy_num)
            extra = 1;
    }
    effect.to->drawCards(2 + extra + effect.effectValue.first());
}

Duel::Duel(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("duel");
    can_damage = true;
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
    if (first->isDead() || second->isDead())
        return;
    Room *room = first->getRoom();

    room->setEmotion(first, "duel");
    room->setEmotion(second, "duel");

    int times = 1 + effect.effectValue.first();
    bool dodamage = false;
    forever {
        if (!first->isAlive())
            break;

        for (int i = 0; i < times; i += 1) {
            QString prompt = QString("duel-slash:%1:%2:%3").arg(second->objectName()).arg(times).arg(i + 1);
            const Card *slash = room->askForCard(first, "slash", prompt, QVariant::fromValue(effect), Card::MethodResponse, second);
            if (slash == nullptr) {
                dodamage = true;
                break;
            } else if (slash->getSkillName() == "Spear")
                room->setEmotion(first, "weapon/spear");
        }
        if (dodamage)
            break;

        qSwap(first, second);
    }
    //DamageStruct damage(this, second->isAlive() ? second : NULL, first, 1 + effect.effectValue.last());
    DamageStruct damage(effect.card, second->isAlive() ? second : nullptr, first, 1 + effect.effectValue.last());
    if (second != effect.from)
        damage.by_user = false;
    room->damage(damage);
}

Snatch::Snatch(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("snatch");
}

bool Snatch::isAvailable(const Player *player) const
{
    auto siblings = player->getAliveSiblings();

    foreach (const Player *p, siblings) {
        if (targetFilter(QList<const Player *>(), p, player))
            return SingleTargetTrick::isAvailable(player);
    }

    return false;
}

bool Snatch::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num || to_select == Self)
        return false;
    bool ignore = (Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && to_select != Self && !hasFlag("IgnoreFailed"));
    if (to_select->isAllNude() && !ignore)
        return false;
    int distance_limit = 1 + Sanguosha->correctCardTarget(TargetModSkill::DistanceLimit, Self, this);
    int rangefix = 0;
    if ((Self->getOffensiveHorse() != nullptr) && subcards.contains(Self->getOffensiveHorse()->getId()))
        rangefix += 1;
    if (Self->distanceTo(to_select, rangefix) > distance_limit)
        return false;

    return true;
}

void Snatch::onEffect(const CardEffectStruct &effect) const
{
    if (effect.from->isDead() || effect.to->isDead())
        return;
    if (effect.to->isAllNude()) {
        effect.to->getRoom()->setCardFlag(this, "-tianxieEffected_" + effect.to->objectName()); //only for skill tianxie
        return;
    }

    Room *room = effect.to->getRoom();
    bool using_2013 = (room->getMode() == "02_1v1" && Config.value("1v1/Rule", "2013").toString() != "Classical");
    QString flag = using_2013 ? "hes" : "hejs";
    effect.from->tag["SnatchCard"] = QVariant::fromValue(effect.card);

    QList<int> disable;
    DummyCard *dummy = new DummyCard;
    for (int i = 0; i < (1 + effect.effectValue.first()); i += 1) {
        int card_id = room->askForCardChosen(effect.from, effect.to, flag, objectName(), false, Card::MethodNone, disable);

        disable << card_id;
        dummy->addSubcard(card_id);

        if (effect.to->getCards(flag).length() - disable.length() <= 0)
            break;
    }

    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
    room->obtainCard(effect.from, dummy, reason, false);
    delete dummy;
}

Dismantlement::Dismantlement(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("dismantlement");
}

bool Dismantlement::isAvailable(const Player *player) const
{
    auto siblings = player->getAliveSiblings();

    foreach (const Player *p, siblings) {
        if (targetFilter(QList<const Player *>(), p, player))
            return SingleTargetTrick::isAvailable(player);
    }

    return false;
}

bool Dismantlement::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    bool ignore = (Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && to_select != Self && !hasFlag("IgnoreFailed"));
    return targets.length() < total_num && to_select != Self && (!to_select->isAllNude() || ignore);
}

void Dismantlement::onEffect(const CardEffectStruct &effect) const
{
    if (effect.from->isDead() || effect.to->isDead())
        return;

    Room *room = effect.to->getRoom();
    bool using_2013 = (room->getMode() == "02_1v1" && Config.value("1v1/Rule", "2013").toString() != "Classical");
    QString flag = using_2013 ? "hes" : "hejs";
    if (!effect.from->canDiscard(effect.to, flag)) {
        room->setCardFlag(this, "-tianxieEffected_" + effect.to->objectName()); //only for skill tianxie
        return;
    }

    int card_id = -1;

    QList<int> ids, disable;
    foreach (const Card *c, effect.to->getCards(flag)) {
        if (!effect.from->canDiscard(effect.to, c->getEffectiveId()))
            disable << c->getEffectiveId();
    }

    DummyCard *dummy = new DummyCard;
    AI *ai = effect.from->getAI();
    //for AI: sgs.ai_choicemade_filter.cardChosen.snatch
    //like Xunshi
    effect.from->tag["DismantlementCard"] = QVariant::fromValue(effect.card);

    for (int i = 0; i < (1 + effect.effectValue.first()); i += 1) {
        bool visible = false;
        if (using_2013 && (ai == nullptr)) {
            if ((effect.to->getEquips().isEmpty() || !effect.from->canDiscard(effect.to, "e")) && effect.from->canDiscard(effect.to, "hs")) {
                visible = true;
                LogMessage log;
                log.type = "$ViewAllCards";
                log.from = effect.from;
                log.to << effect.to;
                log.card_str = IntList2StringList(effect.to->handCards()).join("+");
                room->doNotify(effect.from, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
            } else
                visible = false;
        }
        card_id = room->askForCardChosen(effect.from, effect.to, flag, objectName(), visible, Card::MethodDiscard, disable);
        disable << card_id;
        dummy->addSubcard(card_id);

        if (effect.to->getCards(flag).length() - disable.length() <= 0)
            break;
    }

    room->throwCard(dummy, effect.to, effect.from);
    delete dummy;
}

Indulgence::Indulgence(Suit suit, int number)
    : DelayedTrick(suit, number)
{
    setObjectName("indulgence");

    judge.pattern = ".|heart";
    judge.good = true;
    judge.reason = objectName();
    has_effectvalue = false;
}

QString Indulgence::getSubtype() const
{
    return "unmovable_delayed_trick";
}

bool Indulgence::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    bool ignore = ((Self != nullptr) && Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && to_select != Self
                   && !hasFlag("IgnoreFailed"));
    return targets.isEmpty() && (!to_select->containsTrick(objectName()) || ignore) && to_select != Self;
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
    has_effectvalue = false;
}

QString Disaster::getSubtype() const
{
    return "movable_delayed_trick";
}

bool Disaster::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    bool ignore = (Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && to_select != Self && !hasFlag("IgnoreFailed"));
    if (ignore)
        return targets.isEmpty();

    return targets.isEmpty() && to_select == Self && !to_select->containsTrick(objectName());
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

Lightning::Lightning(Suit suit, int number)
    : Disaster(suit, number)
{
    setObjectName("lightning");
    can_damage = true;
    judge.pattern = ".|spade|2~9";
    judge.good = false;
    judge.reason = objectName();
}

void Lightning::takeEffect(ServerPlayer *target) const
{
    target->getRoom()->damage(DamageStruct(this, nullptr, target, 3, DamageStruct::Thunder));
}

// EX cards

class IceSwordSkill : public WeaponSkill
{
public:
    IceSwordSkill()
        : WeaponSkill("IceSword")
    {
        events << DamageCaused;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!equipAvailable(damage.from, EquipCard::WeaponLocation, objectName(), damage.to))
            return QList<SkillInvokeDetail>();

        if ((damage.card != nullptr) && damage.by_user && damage.card->isKindOf("Slash") && damage.from->canDiscard(damage.to, "hes") && !damage.chain && !damage.transfer
            && damage.from != damage.to)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, false, damage.to);

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        //bool ret = WeaponSkill::cost(triggerEvent, room, invoke, data);
        //if (ret)
        //    room->setEmotion(invoke->invoker, "weapon/ice_sword");
        //return ret;
        invoke->invoker->tag[this->objectName()] = data;
        if (invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget))) {
            const ViewHasSkill *v = Sanguosha->ViewHas(invoke->invoker, objectName(), "weapon", true);
            if (v != nullptr)
                invoke->invoker->showHiddenSkill(v->objectName());

            room->setEmotion(invoke->invoker, "weapon/ice_sword");
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *from = invoke->invoker;
        ServerPlayer *to = invoke->targets.first();

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
    RenwangShieldSkill()
        : ArmorSkill("RenwangShield")
    {
        events << SlashEffected;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (!equipAvailable(effect.to, EquipCard::ArmorLocation, objectName()))
            return QList<SkillInvokeDetail>();

        if (effect.slash->isBlack())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, nullptr, true);

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        const ViewHasSkill *v = Sanguosha->ViewHas(invoke->invoker, objectName(), "armor", true);
        if (v != nullptr) {
            if (!invoke->invoker->hasShownSkill(v) && !invoke->invoker->askForSkillInvoke(this))
                return false;
            invoke->invoker->showHiddenSkill(v->objectName());
        }
        return true;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
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
    HorseSkill()
        : DistanceSkill("Horse")
    {
    }

    int getCorrect(const Player *from, const Player *to) const override
    {
        int correct = 0;
        const Horse *horse = nullptr;
        if ((from->getOffensiveHorse() != nullptr) && from->getMark("Equips_Nullified_to_Yourself") == 0) {
            horse = qobject_cast<const Horse *>(from->getOffensiveHorse()->getRealCard());
            if ((horse != nullptr) && !from->isBrokenEquip(horse->getEffectiveId(), true))
                correct += horse->getCorrect();
        }
        if ((to->getDefensiveHorse() != nullptr) && to->getMark("Equips_Nullified_to_Yourself") == 0) {
            horse = qobject_cast<const Horse *>(to->getDefensiveHorse()->getRealCard());
            if ((horse != nullptr) && !to->isBrokenEquip(horse->getEffectiveId(), true))
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
        if (p->getTreasure() == nullptr)
            targets << p;
    }
    if (targets.isEmpty())
        return;
    ServerPlayer *target = room->askForPlayerChosen(source, targets, "wooden_ox", "@wooden_ox-move", true);
    if (target != nullptr) {
        const Card *treasure = source->getTreasure();
        if (treasure != nullptr)
            room->moveCardTo(treasure, source, target, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_TRANSFER, source->objectName(), "wooden_ox", QString()));
    }
}

class WoodenOxSkill : public OneCardViewAsSkill
{
public:
    WoodenOxSkill()
        : OneCardViewAsSkill("wooden_ox")
    {
        filter_pattern = ".|.|.|hand";
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("WoodenOxCard") && player->hasTreasure(objectName());
    }

    const Card *viewAs(const Card *originalCard) const override
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
    WoodenOxTriggerSkill()
        : TreasureSkill("wooden_ox_trigger")
    {
        events << CardsMoveOneTime;
        frequency = Compulsory;
        global = true;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == nullptr)
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == nullptr || move.from->getPile("wooden_ox").isEmpty())
            return QList<SkillInvokeDetail>();

        for (int i = 0; i < move.card_ids.size(); i++) {
            if (move.from_places[i] != Player::PlaceEquip)
                continue;
            const Card *card = Sanguosha->getEngineCard(move.card_ids[i]);
            if (card->objectName() == "wooden_ox") {
                ServerPlayer *invoker = qobject_cast<ServerPlayer *>(move.from);
                if (!move.reason.m_playerId.isEmpty())
                    invoker = room->findPlayerByObjectName(move.reason.m_playerId);

                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, qobject_cast<ServerPlayer *>(move.from), invoker, nullptr, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *to = qobject_cast<ServerPlayer *>(move.to);
        if ((to != nullptr) && (to->getTreasure() != nullptr) && to->getTreasure()->objectName() == "wooden_ox" && move.to_place == Player::PlaceEquip) {
            CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, "", nullptr, "wooden_ox", "");
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
        //for HegNullification???
        room->setTag("targets" + this->toString(), QVariant::fromValue(players));

        room->cardEffect(effect);
    }

    room->removeTag("targets" + this->toString());

    source->drawCards(1, objectName());

    if (room->getCardPlace(getEffectiveId()) == Player::PlaceTable) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), this->getSkillName(), QString());
        if (targets.size() == 1)
            reason.m_targetId = targets.first()->objectName();
        reason.m_extraData = QVariant::fromValue((const Card *)this);
        room->moveCardTo(this, source, nullptr, Player::DiscardPile, reason, true);
    }
}

void LureTiger::onEffect(const CardEffectStruct &effect) const
{
    if (effect.to->isDead())
        return;
    Room *room = effect.to->getRoom();
    room->touhouLogmessage("#Shenyin1", effect.to, objectName(), QList<ServerPlayer *>());

    room->setPlayerCardLimitation(effect.to, "use", ".", "lure_tiger", true);
    room->setPlayerProperty(effect.to, "removed", true);
    effect.from->setFlags("LureTigerUser");
}

class LureTigerSkill : public TriggerSkill
{
public:
    LureTigerSkill()
        : TriggerSkill("lure_tiger_effect")
    {
        events << EventPhaseChanging;
        global = true;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const override
    {
        //the current player can also to be removed
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAllPlayers(true)) {
                if (p->isRemoved()) {
                    room->touhouLogmessage("#Shenyin2", p, objectName(), QList<ServerPlayer *>());
                    room->setPlayerProperty(p, "removed", false);
                    room->removePlayerCardLimitation(p, "use", ".$1", "lure_tiger");
                }
            }
        }
    }
};

class LureTigerProhibit : public ProhibitSkill
{
public:
    LureTigerProhibit()
        : ProhibitSkill("#lure_tiger-prohibit")
    {
    }

    bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &, bool) const override
    {
        return card->getTypeId() != Card::TypeSkill && to->isRemoved();
    }
};

Drowning::Drowning(Suit suit, int number)
    : AOE(suit, number)
{
    setObjectName("drowning");
}

bool Drowning::isCancelable(const CardEffectStruct &effect) const
{
    return !effect.to->getEquips().isEmpty() && TrickCard::isCancelable(effect);
}

void Drowning::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    if (!effect.to->canDiscard(effect.to, "e")) {
        room->setCardFlag(this, "-tianxieEffected_" + effect.to->objectName()); //only for skill tianxie
        return;
    }

    DummyCard *dummy = new DummyCard;
    QList<int> ids;
    QStringList prohibit;
    QString subpattern = ".";
    int times = qMin(1 + effect.effectValue.first(), effect.to->getEquips().length());
    for (int i = 0; i < times; i += 1) {
        bool candiscard = false;
        foreach (const Card *c, effect.to->getEquips()) {
            if (!ids.contains(c->getEffectiveId()) && effect.to->canDiscard(effect.to, c->getId())) {
                candiscard = true;
                break;
            }
        }

        if (!candiscard)
            break;
        QString prompt = QString("@drowning:%1:%2:%3").arg(effect.from->objectName()).arg(times).arg(i + 1);
        QString pattern = QString("%1|.|.|equipped!").arg(subpattern);
        const Card *card = room->askForCard(effect.to, pattern, prompt, QVariant::fromValue(effect), Card::MethodNone);
        // force discard!!!
        if (card == nullptr) {
            QList<const Card *> equips = effect.to->getCards("e");
            foreach (const Card *c, equips) {
                if (effect.to->isJilei(c) || ids.contains(c->getEffectiveId()))
                    equips.removeOne(c);
            }
            if (!equips.isEmpty()) {
                int x = qrand() % equips.length();
                card = equips.value(x);
            }
        }

        ids << card->getEffectiveId();
        prohibit << ('^' + QString::number(card->getEffectiveId()));
        subpattern = prohibit.join("+");

        dummy->addSubcard(card);
    }

    if (!dummy->getSubcards().isEmpty())
        room->throwCard(dummy, effect.to, effect.to);
    delete dummy;
}

KnownBoth::KnownBoth(Card::Suit suit, int number)
    : TrickCard(suit, number)
{
    setObjectName("known_both");
    can_recast = true;
}

QString KnownBoth::getSubtype() const
{
    return "known_both";
}

bool KnownBoth::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 2 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num || to_select == Self)
        return false;

    return !to_select->isKongcheng() && (to_select->getShownHandcards().length() < to_select->getHandcardNum());
}

bool KnownBoth::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    bool rec = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) && can_recast;
    if (rec) {
        QList<int> sub;
        if (isVirtualCard())
            sub = subcards;
        else
            sub << getEffectiveId();

        foreach (int id, sub) {
            if (Self->getHandPile().contains(id)) {
                rec = false;
                break;
            } else { // for skill chaoren
                if (id == Self->property("chaoren").toInt()) {
                    rec = false;
                    break;
                }
            }
        }
    }

    if (rec && Self->isCardLimited(this, Card::MethodUse))
        return targets.length() == 0;

    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
        return targets.length() != 0;

    int total_num = 2 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (!rec)
        return targets.length() > 0 && targets.length() <= total_num;
    else
        return targets.length() <= total_num;
}

void KnownBoth::onUse(Room *room, const CardUseStruct &card_use) const
{
    if (card_use.to.isEmpty()) {
        LogMessage log;
        log.type = "#Card_Recast";
        log.from = card_use.from;
        log.card_str = card_use.card->toString();
        room->sendLog(log);

        CardMoveReason reason(CardMoveReason::S_REASON_RECAST, card_use.from->objectName());
        reason.m_skillName = getSkillName();
        room->moveCardTo(this, card_use.from, nullptr, Player::DiscardPile, reason);
        card_use.from->broadcastSkillInvoke("@recast");

        card_use.from->drawCards(1);
    } else {
        if (getSkillName() == "JadeSeal")
            room->setEmotion(card_use.from, "treasure/jade_seal");

        TrickCard::onUse(room, card_use);
    }
}

void KnownBoth::onEffect(const CardEffectStruct &effect) const
{
    if (effect.to->isDead())
        return;
    if (effect.to->getCards("h").isEmpty()) {
        effect.to->getRoom()->setCardFlag(this, "-tianxieEffected_" + effect.to->objectName()); //only for skill tianxie
        return;
    }

    Room *room = effect.from->getRoom();
    QList<int> ids;

    for (int i = 0; i < (1 + effect.effectValue.first()); i += 1) {
        int id = room->askForCardChosen(effect.from, effect.to, "h", objectName(), false, Card::MethodNone, ids);
        ids << id;
        if ((effect.to->getCards("h").length() - ids.length()) <= 0)
            break;
    }

    effect.to->addToShownHandCards(ids);
}

void KnownBoth::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QStringList nullified_list = room->getTag("CardUseNullifiedList").toStringList();
    bool all_nullified = nullified_list.contains("_ALL_TARGETS");
    int magic_drank = 0;
    if ((source != nullptr) && source->getMark("magic_drank") > 0)
        magic_drank = source->getMark("magic_drank");

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
        //for HegNullification???
        room->setTag("targets" + this->toString(), QVariant::fromValue(players));
        if (hasFlag("mopao"))
            effect.effectValue.first() = effect.effectValue.first() + 1;
        if (source->getMark("kuangji_value") > 0) {
            effect.effectValue.first() = effect.effectValue.first() + source->getMark("kuangji_value");
            room->setPlayerMark(source, "kuangji_value", 0);
        }

        effect.effectValue.first() = effect.effectValue.first() + magic_drank;
        room->cardEffect(effect);
    }
    if (magic_drank > 0)
        room->setPlayerMark(source, "magic_drank", 0);
    room->removeTag("targets" + this->toString());

    if (source->isAlive() && source->isCurrent()) {
        room->touhouLogmessage("#KnownBothLimit", source);
        room->setTag("KnownBothUsed", true);
        foreach (ServerPlayer *p, room->getOtherPlayers(source)) {
            if (p->getMark("KnownBoth_Limit") == 0) {
                room->setPlayerCardLimitation(p, "use,response", ".|.|.|show", "known_both", true);
                room->setPlayerMark(p, "KnownBoth_Limit", 1);
            }
        }
    }

    if (room->getCardPlace(getEffectiveId()) == Player::PlaceTable) {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), this->getSkillName(), QString());
        if (targets.size() == 1)
            reason.m_targetId = targets.first()->objectName();
        reason.m_extraData = QVariant::fromValue((const Card *)this);
        room->moveCardTo(this, source, nullptr, Player::DiscardPile, reason, true);
    }
}

class KnownBothSkill : public TriggerSkill
{
public:
    KnownBothSkill()
        : TriggerSkill("known_both_effect")
    {
        events << EventPhaseChanging << Revive;
        global = true;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark("KnownBoth_Limit") > 0) {
                        room->removePlayerCardLimitation(p, "use,response", ".|.|.|show$1", "known_both");
                        room->setPlayerMark(p, "KnownBoth_Limit", 0);
                    }
                }
                room->setTag("KnownBothUsed", false);
            }
        } else {
            QVariant knownBothTag = room->getTag("KnownBothUsed");
            if (knownBothTag.canConvert(QVariant::Bool) && knownBothTag.toBool() && (room->getCurrent() != nullptr)) {
                foreach (ServerPlayer *p, room->getOtherPlayers(room->getCurrent())) {
                    if (p->getMark("KnownBoth_Limit") == 0) {
                        room->setPlayerCardLimitation(p, "use", ".|.|.|show", "known_both", true);
                        room->setPlayerMark(p, "KnownBoth_Limit", 1);
                    }
                }
            }
        }
    }
};

SavingEnergy::SavingEnergy(Suit suit, int number)
    : DelayedTrick(suit, number)
{
    setObjectName("saving_energy");

    judge.pattern = ".|spade";
    judge.good = false;
    judge.negative = false;
    judge.reason = objectName();
    has_effectvalue = false;
}

QString SavingEnergy::getSubtype() const
{
    return "unmovable_delayed_trick";
}

bool SavingEnergy::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    bool ignore = ((Self != nullptr) && Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && to_select != Self
                   && !hasFlag("IgnoreFailed"));
    return targets.isEmpty() && (!to_select->containsTrick(objectName()) || ignore);
}

void SavingEnergy::takeEffect(ServerPlayer *target) const
{
    target->skip(Player::Discard);
    target->setFlags("savingEnergy");
}

class SavingEnergySkill : public TriggerSkill
{
public:
    SavingEnergySkill()
        : TriggerSkill("saving_energy_effect")
    {
        events << EventPhaseStart;
        global = true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        ServerPlayer *p = data.value<ServerPlayer *>();
        if (p->getPhase() == Player::Finish && p->hasFlag("savingEnergy"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, nullptr, p, nullptr, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        invoke->invoker->drawCards(2, "saving_energy");
        return false;
    }
};

class DeathSickleSkill : public WeaponSkill
{
public:
    DeathSickleSkill()
        : WeaponSkill("DeathSickle")
    {
        events << TargetSpecified << CardFinished; //EventPhaseChanging
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return;

            foreach (ServerPlayer *p, use.to) {
                QStringList death = p->property("DeathSickle").toStringList();
                if (!death.contains(use.card->toString()))
                    continue;

                death.removeOne(use.card->toString());
                room->setPlayerProperty(p, "DeathSickle", death);

                if (death.isEmpty() && p->getDyingFactor() > 0)
                    room->setPlayerProperty(p, "dyingFactor", p->getDyingFactor() - 1);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        if (e != TargetSpecified)
            return QList<SkillInvokeDetail>();
        CardUseStruct use = data.value<CardUseStruct>();
        if (!equipAvailable(use.from, EquipCard::WeaponLocation, objectName()))
            return QList<SkillInvokeDetail>();

        if (use.from != nullptr && use.card != nullptr && use.card->isKindOf("Slash")) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, use.to) {
                if (p->isAlive() && p->dyingThreshold() == 1) {
                    if (!equipAvailable(use.from, EquipCard::WeaponLocation, objectName(), p))
                        continue;
                    d << SkillInvokeDetail(this, use.from, use.from, nullptr, false, p);
                }
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (invoke->invoker->askForSkillInvoke(this, QVariant::fromValue(invoke->preferredTarget))) {
            room->setEmotion(invoke->invoker, "weapon/death_sickle");
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *target = invoke->targets.first();
        room->touhouLogmessage("#DeathSickle", target, QString::number(target->dyingThreshold() + 1), QList<ServerPlayer *>(), QString::number(1));
        QStringList death = target->property("DeathSickle").toStringList();
        if (!death.contains(use.card->toString())) {
            death << use.card->toString();
            room->setPlayerProperty(target, "DeathSickle", death);
            room->setPlayerProperty(target, "dyingFactor", target->getDyingFactor() + 1);
        }

        return false;
    }
};

DeathSickle::DeathSickle(Card::Suit suit, int number)
    : Weapon(suit, number, 2)
{
    setObjectName("DeathSickle");
}

StandardCardPackage::StandardCardPackage()
    : Package("standard_cards", Package::CardPack)
{
    QList<Card *> cards;

    // clang-format off

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

    // clang-format on

    skills << new DoubleSwordSkill << new QinggangSwordSkill << new BladeSkill << new SpearSkill << new AxeSkill << new KylinBowSkill << new EightDiagramSkill << new HalberdSkill
           << new BreastPlateSkill << new TribladeSkill;

    QList<Card *> horses;

    // clang-format off

    horses << new DefensiveHorse(Card::Spade, 5)
           << new DefensiveHorse(Card::Club, 5)
           << new DefensiveHorse(Card::Heart, 13)
           << new OffensiveHorse(Card::Heart, 5)
           << new OffensiveHorse(Card::Spade, 13)
           << new OffensiveHorse(Card::Diamond, 13);

    // clang-format on

    horses.at(0)->setObjectName("JueYing");
    horses.at(1)->setObjectName("DiLu");
    horses.at(2)->setObjectName("ZhuaHuangFeiDian");
    horses.at(3)->setObjectName("ChiTu");
    horses.at(4)->setObjectName("DaYuan");
    horses.at(5)->setObjectName("ZiXing");

    cards << horses;

    addMetaObject<TribladeCard>();
    skills << new HorseSkill;

    // clang-format off

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

    // clang-format on

    foreach (Card *card, cards)
        card->setParent(this);
}

StandardExCardPackage::StandardExCardPackage()
    : Package("standard_ex_cards", Package::CardPack)
{
    QList<Card *> cards;

    // clang-format off

    cards << new IceSword(Card::Spade, 2)
        << new RenwangShield(Card::Club, 2)
        << new Lightning(Card::Heart, 12)
        << new Nullification(Card::Diamond, 12)
        << new WoodenOx(Card::Diamond, 5)
        << new LureTiger(Card::Heart, 2)
        << new LureTiger(Card::Diamond, 10)
        << new Drowning(Card::Spade, 13)
        << new KnownBoth(Card::Club, 3)
        << new KnownBoth(Card::Spade, 4)
        << new SavingEnergy(Card::Diamond, 9)
        << new SavingEnergy(Card::Heart, 11)
        << new DeathSickle(Card::Club, 13);
    // clang-format on

    skills << new RenwangShieldSkill << new IceSwordSkill << new WoodenOxSkill << new WoodenOxTriggerSkill << new LureTigerSkill << new LureTigerProhibit << new KnownBothSkill
           << new SavingEnergySkill << new DeathSickleSkill << new FakeMoveSkill("dismantle");
    insertRelatedSkills("lure_tiger_effect", "#lure_tiger-prohibit");

    foreach (Card *card, cards)
        card->setParent(this);

    addMetaObject<WoodenOxCard>();
}

ADD_PACKAGE(StandardCard)
ADD_PACKAGE(StandardExCard)
