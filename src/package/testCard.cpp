#include "testCard.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "room.h"

NatureJink::NatureJink(Suit suit, int number)
    : Jink(suit, number)
{
    //setObjectName("nature_jink");
    //target_fixed = true;
}

bool NatureJink::match(const QString &pattern) const
{
    QStringList patterns = pattern.split("+");
    if (patterns.contains("jink"))
        return true;
    else
        return Jink::match(pattern);
}

AdvancedJink::AdvancedJink(Suit suit, int number)
    : NatureJink(suit, number)
{
    setObjectName("advanced_jink");
}

class CameraSkill : public WeaponSkill
{
public:
    CameraSkill()
        : WeaponSkill("Camera")
    {
        events << Damage;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!equipAvailable(damage.from, EquipCard::WeaponLocation, objectName(), damage.to))
            return QList<SkillInvokeDetail>();

        if (damage.card && damage.card->isKindOf("Slash") && damage.from && damage.from->isAlive() && damage.to->isAlive() && damage.to != damage.from && damage.by_user
            && !damage.chain && !damage.transfer && damage.to->getCards("h").length() > 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, false, damage.to);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = invoke->targets.first();
        if (target->getCards("h").isEmpty())
            return false;

        int id = room->askForCardChosen(invoke->invoker, target, "h", objectName());
        target->addToShownHandCards(QList<int>() << id);

        if (target->getCards("h").isEmpty())
            return false;
        int id2 = room->askForCardChosen(invoke->invoker, target, "h", objectName());
        target->addToShownHandCards(QList<int>() << id2);

        return false;
    }
};

Camera::Camera(Suit suit, int number)
    : Weapon(suit, number, 4)
{
    setObjectName("Camera");
}

class GunSkill : public WeaponSkill
{
public:
    GunSkill()
        : WeaponSkill("Gun")
    {
        events << Damage;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!equipAvailable(damage.from, EquipCard::WeaponLocation, objectName(), damage.to))
            return QList<SkillInvokeDetail>();

        if (damage.card && damage.card->isKindOf("Slash") && damage.from && damage.from->isAlive() && damage.to->isAlive() && damage.to != damage.from) {
            foreach (const Card *c, damage.to->getCards("e")) {
                if (!damage.to->isBrokenEquip(c->getEffectiveId()))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true, damage.to);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = invoke->targets.first();
        QList<int> ids;
        foreach (const Card *c, target->getCards("e")) {
            if (!target->isBrokenEquip(c->getEffectiveId()))
                ids << c->getEffectiveId();
        }
        if (!ids.isEmpty()) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), target->objectName());
            room->touhouLogmessage("#Gun", invoke->invoker, objectName(), QList<ServerPlayer *>() << target);
            target->addBrokenEquips(ids);
        }

        return false;
    }
};

Gun::Gun(Suit suit, int number)
    : Weapon(suit, number, 4)
{
    setObjectName("Gun");
}

class JadeSealSkill : public ZeroCardViewAsSkill
{
public:
    JadeSealSkill()
        : ZeroCardViewAsSkill("JadeSeal")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        KnownBoth *card = new KnownBoth(Card::SuitToBeDecided, -1);
        card->setSkillName(objectName());
        card->deleteLater();
        if (player->isCardLimited(card, Card::MethodUse))
            return false;
        return EquipSkill::equipAvailable(player, EquipCard::TreasureLocation, objectName()) && !player->hasFlag("JadeSeal_used");
        //return true;
    }

    virtual const Card *viewAs() const
    {
        KnownBoth *card = new KnownBoth(Card::SuitToBeDecided, -1);
        card->setSkillName(objectName());
        return card;
    }
};

class JadeSealTriggerSkill : public TreasureSkill
{
public:
    JadeSealTriggerSkill()
        : TreasureSkill("JadeSeal_trigger")
    {
        events << PreCardUsed << EventPhaseChanging;
        frequency = Compulsory;
        global = true;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.card && use.card->getSkillName() == "JadeSeal") {
                room->setPlayerFlag(use.from, "JadeSeal_used");
            }
        } else if (e == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->setPlayerFlag(p, "-JadeSeal_used");
        }
    }
};

JadeSeal::JadeSeal(Suit suit, int number)
    : Treasure(suit, number)
{
    setObjectName("JadeSeal");
}

void JadeSeal::onUninstall(ServerPlayer *player) const
{
    ServerPlayer *current = player->getRoom()->getCurrent();
    if (current)
        player->getRoom()->setPlayerFlag(current, "-JadeSeal_used");
    Treasure::onUninstall(player);
}

class PagodaSkill : public OneCardViewAsSkill
{
public:
    PagodaSkill()
        : OneCardViewAsSkill("Pagoda")
    {
        filter_pattern = ".|black|.|hand";
        response_pattern = "nullification";
        response_or_use = true;//only skill shenbao can use WoodenOx
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Card *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        return ncard;
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        if (!EquipSkill::equipAvailable(player, EquipCard::TreasureLocation, objectName()))
            return false;
        if (player->hasFlag("Pagoda_used"))
            return false;
        return !player->isKongcheng(); // || !player->getHandPile().isEmpty();
    }
};

Pagoda::Pagoda(Suit suit, int number)
    : Treasure(suit, number)
{
    setObjectName("Pagoda");
}

class PagodaTriggerSkill : public TreasureSkill
{
public:
    PagodaTriggerSkill()
        : TreasureSkill("Pagoda_trigger")
    {
        events << PreCardUsed << EventPhaseChanging << Cancel;
        frequency = Compulsory;
        global = true;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.card && use.card->getSkillName() == "Pagoda") {
                room->setPlayerFlag(use.from, "Pagoda_used");
            }
        } else if (e == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->setPlayerFlag(p, "-Pagoda_used");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e != Cancel)
            return QList<SkillInvokeDetail>();
        if (data.canConvert<CardEffectStruct>()) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (!effect.card->hasFlag("PagodaNullifiation"))
                return QList<SkillInvokeDetail>();

            if (effect.to->isAlive())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        effect.canceled = true;
        data = QVariant::fromValue(effect);

        LogMessage log;
        log.type = "#PagodaNullified";
        log.from = effect.to;
        log.arg = effect.card->objectName();
        log.arg2 = "Pagoda";
        room->sendLog(log);
        room->setEmotion(effect.to, "skill_nullify");
        return true;
    }
};

void Pagoda::onUninstall(ServerPlayer *player) const
{
    ServerPlayer *current = player->getRoom()->getCurrent();
    if (current)
        player->getRoom()->setPlayerFlag(current, "-Pagoda_used");
    Treasure::onUninstall(player);
}

class CamouflageSkill : public ArmorSkill
{
public:
    CamouflageSkill()
        : ArmorSkill("Camouflage")
    {
        events << DamageInflicted << CardsMoveOneTime;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            if (equipAvailable(damage.to, EquipCard::ArmorLocation, objectName())) {
                int count = 0;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getArmor())
                        count++;
                }
                if (count == 1)
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, true);
            }
        } else {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to && move.to_place == Player::PlaceEquip && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE) {
                ServerPlayer *invoker = qobject_cast<ServerPlayer *>(move.to);
                for (int i = 0; i < move.card_ids.size(); i++) {
                    const Card *card = Sanguosha->getEngineCard(move.card_ids[i]);
                    if (card->objectName() == objectName()) {
                        foreach (ServerPlayer *p, room->getAllPlayers()) {
                            if (p->getArmor() && p->getArmor()->objectName() != objectName()) {
                                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, invoker, invoker, NULL, true);
                            }
                        }
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            room->setEmotion(invoke->invoker, "armor/silver_lion");

            LogMessage log;
            log.type = "#Camouflage";
            log.from = invoke->invoker;
            log.arg = QString::number(damage.damage);
            log.arg2 = objectName();
            room->sendLog(log);
            return true;
        } else {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->getArmor() && p->getArmor()->objectName() != objectName()) {
                    targets << p;
                }
            }
            if (!targets.isEmpty()) {
                ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, "camouflage", "@camouflage");
                room->throwCard(target->getArmor()->getId(), target, invoke->invoker);
            }
        }

        return false;
    }
};

Camouflage::Camouflage(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("Camouflage");
}

AwaitExhausted::AwaitExhausted(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("await_exhausted");
}

bool AwaitExhausted::targetFilter(const QList<const Player *> &targets, const Player *, const Player *Self) const
{
    int total_num = 2 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    return targets.length() < total_num && !Self->isCardLimited(this, Card::MethodUse);
}

void AwaitExhausted::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    effect.to->drawCards(2 + effect.effectValue.first());
    int num = qMin(2 + effect.effectValue.last(), effect.to->getCards("ehs").length());
    if (num > 0)
        room->askForDiscard(effect.to, "AwaitExhausted", num, num, false, true);
}

SpellDuel::SpellDuel(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("spell_duel");
}

bool SpellDuel::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QList<int> ids;
    if (isVirtualCard())
        ids = getSubcards();
    else
        ids << getEffectiveId();
    if (ids.length() >= Self->getHandcardNum())
        return false;

    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    return targets.length() < total_num && to_select != Self;
}

void SpellDuel::onEffect(const CardEffectStruct &effect) const
{
    if (effect.from->isKongcheng() || effect.to->isKongcheng())
        return;
    Room *room = effect.from->getRoom();

    if (effect.from->pindian(effect.to, "spell_duel", NULL)) {
        DamageStruct damage(this, effect.from, effect.to);
        room->damage(damage);
    } else {
        DamageStruct damage(this, effect.to, effect.from);
        damage.by_user = false;
        room->damage(damage);
    }
}

Kusuri::Kusuri(Suit suit, int number)
    : BasicCard(suit, number)
{
    setObjectName("kusuri");
}

QString Kusuri::getSubtype() const
{
    return "recover_card";
}

bool Kusuri::isAvailable(const Player *player) const
{
    if (!BasicCard::isAvailable(player))
        return false;
    bool isPlay = Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY;
    bool ignore = (player->hasSkill("tianqu") && isPlay && !hasFlag("IgnoreFailed"));
    if (ignore)
        return true;
    if (player->isWounded() && !player->isProhibited(player, this))
        return true;

    QList<const Player *> targets = player->getAliveSiblings();
    targets << player;
    foreach (const Player *p, targets) {
        if (!player->isProhibited(p, this)) {
            if (p->hasFlag("Global_Dying") && !isPlay)
                return true;
            if (p->isChained() || !p->getShownHandcards().isEmpty() || !p->faceUp() || !p->getBrokenEquips().isEmpty())
                return true;
        }
    }
    return false;
}

bool Kusuri::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
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
        int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
        return targets.length() < total_num
            && (to_select->isChained() || !to_select->getShownHandcards().isEmpty() || !to_select->faceUp() || !to_select->getBrokenEquips().isEmpty());
    }
    return false;
}

void Kusuri::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    if (effect.to->hasFlag("Global_Dying") && Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
        // recover hp
        RecoverStruct recover;
        recover.card = this;
        recover.who = effect.from;
        room->recover(effect.to, recover);
    } else {
        if (!effect.to->faceUp())
            effect.to->turnOver();
        if (effect.to->isChained())
            room->setPlayerProperty(effect.to, "chained", false);

        if (!effect.to->getBrokenEquips().isEmpty())
            effect.to->removeBrokenEquips(effect.to->getBrokenEquips(), true);

        if (!effect.to->getShownHandcards().isEmpty())
            effect.to->removeShownHandCards(effect.to->getShownHandcards(), true);
    }
}

TestCardPackage::TestCardPackage()
    : Package("test_card", Package::CardPack)
{
    QList<Card *> cards;

    // clang-format off

    cards
         // Equip
        //<< new Camera(Card::Diamond, 11)
        << new Gun(Card::Club, 13)
        << new JadeSeal(Card::Heart, 13)
        << new Pagoda(Card::Spade, 12)
        << new Camouflage(Card::Spade, 1)

        //Trick
        << new AwaitExhausted(Card::Diamond, 4)
        << new AwaitExhausted(Card::Heart, 10)
        //<< new SpellDuel(Card::Heart, 1) << new SpellDuel(Card::Diamond, 1)

        << new Nullification(Card::Club, 12)
        //Basic
        //<< new Kusuri(Card::Diamond, 3) << new Kusuri(Card::Heart, 8) << new Kusuri(Card::Heart, 9)
        << new Slash(Card::Spade, 7)
        << new Slash(Card::Spade, 9)
        << new Slash(Card::Club, 6)
        << new Slash(Card::Club, 9)
        << new Slash(Card::Heart, 8)
        << new Slash(Card::Diamond, 8)
        << new ThunderSlash(Card::Spade, 8)
        << new ThunderSlash(Card::Club, 8)
        << new FireSlash(Card::Heart, 9)

        << new Analeptic(Card::Spade, 10)
        << new Peach(Card::Heart, 4)
        << new Peach(Card::Diamond, 13)
        
        << new AdvancedJink(Card::Heart, 5)
        << new AdvancedJink(Card::Heart, 6)
        << new AdvancedJink(Card::Diamond, 2)
        << new AdvancedJink(Card::Diamond, 11);

    // clang-format on

    foreach (Card *card, cards)
        card->setParent(this);

    skills << new CameraSkill << new GunSkill << new JadeSealSkill << new JadeSealTriggerSkill << new PagodaSkill << new PagodaTriggerSkill << new CamouflageSkill;
}

ADD_PACKAGE(TestCard)
