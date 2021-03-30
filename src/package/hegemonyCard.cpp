#include "hegemonyCard.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "room.h"
#include "standard-equips.h"

HegNullification::HegNullification(Suit suit, int number)
    : Nullification(suit, number)
{
    target_fixed = true;
    setObjectName("heg_nullification");
}

KnownBothHegemony::KnownBothHegemony(Card::Suit suit, int number)
    : TrickCard(suit, number)
{
    setObjectName("known_both_hegemony");
    can_recast = true;
}

QString KnownBothHegemony::getSubtype() const
{
    return "known_both";
}

bool KnownBothHegemony::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num || to_select == Self)
        return false;

    return (!to_select->hasShownGeneral() || (to_select->getGeneral2() && !to_select->hasShownGeneral2()) || !to_select->isKongcheng());
}

bool KnownBothHegemony::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
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

    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (!rec)
        return targets.length() > 0 && targets.length() <= total_num;
    else
        return targets.length() <= total_num;
}

void KnownBothHegemony::onUse(Room *room, const CardUseStruct &card_use) const
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

void KnownBothHegemony::doKnownBoth(const QString &choice, const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    LogMessage log;
    log.type = "#KnownBothView";
    log.from = effect.from;
    log.to << effect.to;
    log.arg = choice;
    foreach (ServerPlayer *p, room->getAllPlayers(true))
        room->doNotify(p, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());

    if (choice == "showhead" || choice == "showdeputy") {
        QStringList list = room->getTag(effect.to->objectName()).toStringList();
        list.removeAt(choice == "showhead" ? 1 : 0);
        foreach (const QString &name, list) {
            LogMessage log;
            log.type = "$KnownBothViewGeneral";
            log.from = effect.from;
            log.to << effect.to;
            log.arg = name;
            log.arg2 = effect.to->getRole();
            room->doNotify(effect.from, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
        }
        JsonArray arg;
        arg << objectName();
        arg << JsonUtils::toJsonArray(list);
        room->doNotify(effect.from, QSanProtocol::S_COMMAND_VIEW_GENERALS, arg);
    } else
        room->showAllCards(effect.to, effect.from);
}

void KnownBothHegemony::onEffect(const CardEffectStruct &effect) const
{
    QStringList select;
    if (!effect.to->hasShownGeneral())
        select << "showhead";
    if (effect.to->getGeneral2() && !effect.to->hasShownGeneral2())
        select << "showdeputy";
    if (!effect.to->isKongcheng() && (effect.to->getShownHandcards().length() < effect.to->getHandcardNum()))
        select << "showcard";

    if (select.isEmpty())
        return;
    int num = qMin(select.length(), 1 + effect.effectValue.first());
    if (effect.from->hasSkill("kuaizhao_hegemony")) {
        if (!effect.from->hasShownSkill("kuaizhao_hegemony") && effect.from->askForSkillInvoke("kuaizhao_hegemony", QVariant::fromValue(effect)))
            effect.from->showHiddenSkill("kuaizhao_hegemony");
        if (effect.from->hasShownSkill("kuaizhao_hegemony"))
            num = select.length();
    }

    Room *room = effect.from->getRoom();
    for (int i = 0; i < num; i += 1) {
        effect.to->setFlags("KnownBothTarget"); //for AI
        QString choice = room->askForChoice(effect.from, objectName(), select.join("+"), QVariant::fromValue(effect.to));
        effect.to->setFlags("-KnownBothTarget");
        select.removeAll(choice);

        doKnownBoth(choice, effect);

        if (select.isEmpty())
            return;
    }

    /*if (effect.from->hasSkill("kuaizhao_hegemony")) {
        while (!select.isEmpty()) {
            effect.to->setFlags("KnownBothTarget"); //for AI
            QString choice = room->askForChoice(effect.from, objectName(), select.join("+") + "+dismiss", QVariant::fromValue(effect.to));
            effect.to->setFlags("-KnownBothTarget");
            if (choice == "dismiss")
                return;
            select.removeAll(choice);
            doKnownBoth(choice, effect);
        }
    }*/
}

bool KnownBothHegemony::isAvailable(const Player *player) const
{
    bool can_use = false;
    foreach (const Player *p, player->getAliveSiblings()) {
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
    return (can_use && !player->isCardLimited(this, Card::MethodUse)) || (can_rec && !player->isCardLimited(this, Card::MethodRecast));
}

BefriendAttacking::BefriendAttacking(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("befriend_attacking");
}

bool BefriendAttacking::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    //hegemony mode is not available for skill Tianqu yet

    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num)
        return false;

    return to_select->hasShownOneGeneral() && !Self->isFriendWith(to_select);
}

void BefriendAttacking::onEffect(const CardEffectStruct &effect) const
{
    if (effect.from->isAlive())
        effect.from->drawCards(3 + effect.effectValue.first());
    if (effect.to->isAlive())
        effect.to->drawCards(1);
}

bool BefriendAttacking::isAvailable(const Player *player) const
{
    return player->hasShownOneGeneral() && TrickCard::isAvailable(player);
}

/*QStringList BefriendAttacking::checkTargetModSkillShow(const CardUseStruct &use) const
{
    if (use.card == NULL)
        return QStringList();

    if (use.to.length() >= 2) {
        const ServerPlayer *from = use.from;
        QList<const Skill *> skills = from->getSkillList(false, false);
        QList<const TargetModSkill *> tarmods;

        foreach(const Skill *skill, skills) {
            if (from->hasSkill(skill) && skill->inherits("TargetModSkill")) {
                const TargetModSkill *tarmod = qobject_cast<const TargetModSkill *>(skill);
                tarmods << tarmod;
            }
        }

        if (tarmods.isEmpty())
            return QStringList();

        int n = use.to.length() - 1;
        QList<const TargetModSkill *> tarmods_copy = tarmods;

        foreach(const TargetModSkill *tarmod, tarmods_copy) {
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
        foreach(const TargetModSkill *tarmod, tarmods_copy) {
            const Skill *main_skill = Sanguosha->getMainSkill(tarmod->objectName());
            shows << main_skill->objectName();
        }
        return shows;
    }
    return QStringList();
}*/

AwaitExhaustedHegemony::AwaitExhaustedHegemony(Card::Suit suit, int number)
    : TrickCard(suit, number)
{
    setObjectName("await_exhausted_hegemony");
    target_fixed = true;
}

QString AwaitExhaustedHegemony::getSubtype() const
{
    return "await_exhausted";
}

bool AwaitExhaustedHegemony::isAvailable(const Player *player) const
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

void AwaitExhaustedHegemony::onUse(Room *room, const CardUseStruct &card_use) const
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

                //room->broadcastSkillInvoke(skill->objectName(), p);
            } else {
                new_use.to << p;
            }
        }
    }

    TrickCard::onUse(room, new_use);
}

void AwaitExhaustedHegemony::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
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
        if (hasFlag("mopao"))
            effect.effectValue.first() = effect.effectValue.first() + 1;
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
        if (targets.size() == 1)
            reason.m_targetId = targets.first()->objectName();
        room->moveCardTo(&dummy, source, nullptr, Player::DiscardPile, reason, true);
    }
}

void AwaitExhaustedHegemony::onEffect(const CardEffectStruct &effect) const
{
    effect.to->drawCards(2 + effect.effectValue.first());
    effect.to->getRoom()->setPlayerFlag(effect.to, "AwaitExhaustedEffected");
}

class DoubleSwordHegemonySkill : public WeaponSkill
{
public:
    DoubleSwordHegemonySkill()
        : WeaponSkill("DoubleSwordHegemony")
    {
        events << TargetSpecified;
        global = true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!equipAvailable(use.from, EquipCard::WeaponLocation, objectName()))
            return QList<SkillInvokeDetail>();

        if (!isHegemonyGameMode(room->getMode()))
            return QList<SkillInvokeDetail>();

        if (use.card != nullptr && use.card->isKindOf("Slash")) {
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, use.to) {
                if (p->isAlive() && !p->hasShownAllGenerals()) {
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
            const ViewHasSkill *v = Sanguosha->ViewHas(invoke->invoker, objectName(), "weapon", true);
            if (v)
                invoke->invoker->showHiddenSkill(v->objectName());

            room->setEmotion(invoke->invoker, "weapon/double_sword");
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        //canshow?
        QStringList select;
        ServerPlayer *target = invoke->targets.first();
        if (target->canDiscard(invoke->targets.first(), "hs"))
            select << "discard";
        if (!target->hasShownGeneral() && target->canShowGeneral("h"))
            select << "showhead";
        if (target->getGeneral2() && !target->hasShownGeneral2() && target->canShowGeneral("d"))
            select << "showdeputy";

        if (select.isEmpty())
            return false;
        QString choice = room->askForChoice(target, objectName(), select.join("+"), data);
        if (choice == "discard") {
            room->askForDiscard(target, objectName(), 1, 1, false, false);

        } else {
            bool ishead = (choice == "showhead");
            target->showGeneral(ishead, true);
            //target->drawCards(1);
        }
        return false;
    }
};

DoubleSwordHegemony::DoubleSwordHegemony(Suit suit, int number)
    : Weapon(suit, number, 2)
{
    setObjectName("DoubleSwordHegemony");
}

SixSwords::SixSwords(Suit suit, int number)
    : Weapon(suit, number, 2)
{
    setObjectName("SixSwords");
}

class SixSwordsSkill : public AttackRangeSkill
{
public:
    SixSwordsSkill()
        : AttackRangeSkill("SixSwords")
    {
    }

    int getExtra(const Player *target, bool) const override
    {
        foreach (const Player *p, target->getAliveSiblings()) {
            if (p->hasWeapon("SixSwords") && p->isFriendWith(target) && p->getMark("Equips_Nullified_to_Yourself") == 0)
                return 1;
        }

        return 0;
    }
};

HegemonyCardPackage::HegemonyCardPackage()
    : Package("hegemony_card", Package::CardPack)
{
    QList<Card *> cards;

    // clang-format off

    cards
 //Basic
        << new Slash(Card::Spade, 5)
        << new Slash(Card::Spade, 7)
        << new Slash(Card::Spade, 8)
        << new Slash(Card::Spade, 8)
        << new Slash(Card::Spade, 9)
        << new Slash(Card::Spade, 10)
        << new Slash(Card::Spade, 11)

        << new Slash(Card::Club, 2)
        << new Slash(Card::Club, 3)
        << new Slash(Card::Club, 4)
        << new Slash(Card::Club, 5)
        << new Slash(Card::Club, 8)
        << new Slash(Card::Club, 9)
        << new Slash(Card::Club, 10)
        << new Slash(Card::Club, 11)
        << new Slash(Card::Club, 11)

        << new Slash(Card::Heart, 10)
        << new Slash(Card::Heart, 12)

        << new Slash(Card::Diamond, 10)
        << new Slash(Card::Diamond, 11)
        << new Slash(Card::Diamond, 12)

        << new FireSlash(Card::Heart, 4)

        << new FireSlash(Card::Diamond, 4)
        << new FireSlash(Card::Diamond, 5)

        << new ThunderSlash(Card::Spade, 6)
        << new ThunderSlash(Card::Spade, 7)

        << new ThunderSlash(Card::Club, 6)
        << new ThunderSlash(Card::Club, 7)
        << new ThunderSlash(Card::Club, 8)

        << new Jink(Card::Heart, 2)
        << new Jink(Card::Heart, 11)
        << new Jink(Card::Heart, 13)

        << new Jink(Card::Diamond, 2)
        << new Jink(Card::Diamond, 3)
        << new Jink(Card::Diamond, 6)
        << new Jink(Card::Diamond, 7)
        << new Jink(Card::Diamond, 7)
        << new Jink(Card::Diamond, 8)
        << new Jink(Card::Diamond, 8)
        << new Jink(Card::Diamond, 9)
        << new Jink(Card::Diamond, 10)
        << new Jink(Card::Diamond, 11)
        << new Jink(Card::Diamond, 13)

        << new Peach(Card::Heart, 4)
        << new Peach(Card::Heart, 6)
        << new Peach(Card::Heart, 7)
        << new Peach(Card::Heart, 8)
        << new Peach(Card::Heart, 9)
        << new Peach(Card::Heart, 10)
        << new Peach(Card::Heart, 12)

        << new Peach(Card::Diamond, 2)

        << new Analeptic(Card::Spade, 9)
        << new Analeptic(Card::Club, 9)
        << new Analeptic(Card::Diamond, 9)


//Trick
        << new AmazingGrace(Card::Heart, 3)
        << new GodSalvation
        << new SavageAssault(Card::Spade, 13)
        << new SavageAssault(Card::Club, 7)
        << new ArcheryAttack
        << new Duel(Card::Spade, 1)
        << new Duel(Card::Club, 1)
        << new ExNihilo(Card::Heart, 7)
        << new ExNihilo(Card::Heart, 8)
        << new Snatch(Card::Spade, 3)
        << new Snatch(Card::Spade, 4)
        << new Snatch(Card::Diamond, 3)
        << new Dismantlement(Card::Spade, 3)
        << new Dismantlement(Card::Spade, 4)
        << new Dismantlement(Card::Heart, 12)
        << new IronChain(Card::Spade, 12)
        << new IronChain(Card::Club, 12)
        << new IronChain(Card::Club, 13)
        << new FireAttack(Card::Heart, 2)
        << new FireAttack(Card::Heart, 3)
        << new Collateral(Card::Club, 12)
        << new Nullification(Card::Spade, 11)
        << new HegNullification(Card::Club, 13)
        << new HegNullification(Card::Diamond, 12)
        << new AwaitExhaustedHegemony(Card::Heart, 11)
        << new AwaitExhaustedHegemony(Card::Diamond, 4)
        << new KnownBothHegemony(Card::Club, 3)
        << new KnownBothHegemony(Card::Club, 4)
        << new BefriendAttacking
        << new Indulgence(Card::Club, 6)
        << new Indulgence(Card::Heart, 6)
        << new SupplyShortage(Card::Spade, 10)
        << new SupplyShortage(Card::Club, 10)
        << new Lightning(Card::Spade, 1)



// Equip
        << new Crossbow(Card::Diamond)
        << new DoubleSwordHegemony(Card::Spade, 2)
        << new QinggangSword
        << new IceSword(Card::Spade, 2)
        << new Spear
        << new Fan(Card::Diamond, 1)
        << new Axe
        << new KylinBow
        << new SixSwords
        << new Triblade(Card::Diamond, 12)

        << new EightDiagram(Card::Spade, 2)
        << new RenwangShield(Card::Club, 2)
        << new Vine(Card::Club, 2)
        << new SilverLion(Card::Club, 1);

    QList<Card *> horses;

    horses
        << new DefensiveHorse(Card::Spade, 5)
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
    // clang-format on

    foreach (Card *card, cards)
        card->setParent(this);

    skills << new SixSwordsSkill << new DoubleSwordHegemonySkill;
}

ADD_PACKAGE(HegemonyCard)
