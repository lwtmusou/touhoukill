#include "testCard.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "room.h"

DebuffSlash::DebuffSlash(Suit suit, int number)
    : Slash(suit, number)
{
}

bool DebuffSlash::match(const QString &pattern) const
{
    QStringList patterns = pattern.split("+");
    if (patterns.contains("slash"))
        return true;
    else
        return Slash::match(pattern);
}

IronSlash::IronSlash(Suit suit, int number)
    : DebuffSlash(suit, number)
{
    setObjectName("iron_slash");
}

void IronSlash::debuffEffect(const SlashEffectStruct &effect)
{
    if (effect.to->isAlive() && !effect.to->isChained())
        effect.to->getRoom()->setPlayerProperty(effect.to, "chained", !effect.to->isChained());
}

LightSlash::LightSlash(Suit suit, int number)
    : DebuffSlash(suit, number)
{
    setObjectName("light_slash");
}

void LightSlash::debuffEffect(const SlashEffectStruct &effect)
{
    int num = qMin(1 + effect.effectValue.first(), effect.to->getCards("h").length());
    if (num <= 0)
        return;

    QList<int> ids;
    Room *room = effect.from->getRoom();
    for (int i = 0; i < num; i += 1) {
        int id = room->askForCardChosen(effect.from, effect.to, "h", "light_slash", false, Card::MethodNone, ids);
        ids << id;
        if ((effect.to->getCards("h").length() - ids.length()) <= 0)
            break;
    }

    effect.to->addToShownHandCards(ids);
}

PowerSlash::PowerSlash(Suit suit, int number)
    : DebuffSlash(suit, number)
{
    setObjectName("power_slash");
}

void PowerSlash::debuffEffect(const SlashEffectStruct &effect)
{
    Room *room = effect.from->getRoom();
    QList<int> disable;
    foreach (const Card *c, effect.to->getCards("e")) {
        if (effect.to->isBrokenEquip(c->getEffectiveId()))
            disable << c->getEffectiveId();
    }

    int num = qMin(1 + effect.effectValue.first(), effect.to->getEquips().length() - disable.length());
    if (num <= 0)
        return;

    QList<int> ids;
    for (int i = 0; i < num; i += 1) {
        int id = room->askForCardChosen(effect.from, effect.to, "e", "power_slash", true, Card::MethodNone, disable);
        ids << id;
        disable << id;
        if ((effect.to->getEquips().length() - disable.length()) <= 0)
            break;
    }
    effect.to->addBrokenEquips(ids);
}

NatureJink::NatureJink(Suit suit, int number)
    : Jink(suit, number)
{
}

bool NatureJink::match(const QString &pattern) const
{
    QStringList patterns = pattern.split("+");
    if (patterns.contains("jink"))
        return true;
    else
        return Jink::match(pattern);
}

ChainJink::ChainJink(Suit suit, int number)
    : NatureJink(suit, number)
{
    setObjectName("chain_jink");
}

void ChainJink::onEffect(const CardEffectStruct &effect) const
{
    if (effect.to->isAlive() && !effect.to->isChained())
        effect.to->getRoom()->setPlayerProperty(effect.to, "chained", !effect.to->isChained());
}

LightJink::LightJink(Suit suit, int number)
    : NatureJink(suit, number)
{
    setObjectName("light_jink");
}

void LightJink::onEffect(const CardEffectStruct &effect) const
{
    int num = qMin(1 + effect.effectValue.first(), effect.to->getCards("h").length());
    if (num <= 0)
        return;

    QList<int> ids;
    Room *room = effect.from->getRoom();

    for (int i = 0; i < num; i += 1) {
        int id = room->askForCardChosen(effect.from, effect.to, "h", objectName(), false, Card::MethodNone, ids);
        ids << id;
        if ((effect.to->getCards("h").length() - ids.length()) <= 0)
            break;
    }

    effect.to->addToShownHandCards(ids);
}

MagicAnaleptic::MagicAnaleptic(Card::Suit suit, int number)
    : Analeptic(suit, number)
{
    setObjectName("magic_analeptic");
}

bool MagicAnaleptic::match(const QString &pattern) const
{
    QStringList patterns = pattern.split("+");
    if (patterns.contains("analeptic"))
        return true;
    else
        return Analeptic::match(pattern);
}

void MagicAnaleptic::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->setEmotion(effect.to, "analeptic");

    if (effect.to->hasFlag("Global_Dying") && Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY) {
        // recover hp
        RecoverStruct recover;
        recover.card = this;
        recover.who = effect.from;
        recover.recover = 1 + effect.effectValue.first();
        room->recover(effect.to, recover);
    } else {
        room->addPlayerMark(effect.to, "magic_drank", 1 + effect.effectValue.first());
    }
}

SuperPeach::SuperPeach(Suit suit, int number)
    : Peach(suit, number)
{
    setObjectName("super_peach");
    target_fixed = true;
}

bool SuperPeach::match(const QString &pattern) const
{
    QStringList patterns = pattern.split("+");
    if (patterns.contains("peach"))
        return true;
    else
        return Peach::match(pattern);
}

bool SuperPeach::targetFixed() const
{
    bool globalDying = false;
    if (Self) {
        QList<const Player *> players = Self->getSiblings();
        players << Self;
        foreach (const Player *p, players) {
            if (p->hasFlag("Global_Dying") && p->isAlive()) {
                globalDying = true;
                break;
            }
        }
    }

    if (globalDying && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
        return true;
    return false;
}

void SuperPeach::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->setEmotion(effect.from, "peach");

    effect.to->removeShownHandCards(effect.to->getShownHandcards(), true);
    effect.to->removeBrokenEquips(effect.to->getBrokenEquips(), true);
    if (effect.to->isChained())
        effect.to->getRoom()->setPlayerProperty(effect.to, "chained", !effect.to->isChained());

    RecoverStruct recover;
    recover.card = this;
    recover.who = effect.from;
    recover.recover = 1 + effect.effectValue.first();
    room->recover(effect.to, recover);
}

bool SuperPeach::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !hasFlag("IgnoreFailed"))
        return true;
    if (Self->hasFlag("Global_shehuoInvokerFailed"))
        return (to_select->hasFlag("Global_shehuoFailed") && to_select->isDebuffStatus());

    if (targets.isEmpty()) {
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
            if (to_select->isDebuffStatus())
                return true;
        }
    }
    return false;
}

bool SuperPeach::isAvailable(const Player *player) const
{
    if (!BasicCard::isAvailable(player))
        return false;
    bool isPlay = Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY;
    bool ignore = (player->hasSkill("tianqu") && isPlay && !hasFlag("IgnoreFailed"));
    if (ignore)
        return true;

    if ((player->isDebuffStatus() || player->hasFlag("Global_Dying")) && !player->isProhibited(player, this))
        return true;
    foreach (const Player *p, player->getAliveSiblings()) {
        if (p->isDebuffStatus() && !player->isProhibited(p, this))
            return true;
    }

    foreach (const Player *p, player->getAliveSiblings()) {
        if (!player->isProhibited(p, this)) {
            if (p->hasFlag("Global_Dying") && !isPlay)
                return true;
            if (p->hasLordSkill("yanhui") && (p->isWounded() || p->isDebuffStatus()) && player->getKingdom() == "zhan" && player->getPhase() == Player::Play)
                return true;
        }
    }
    return false;
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
            room->setEmotion(invoke->invoker, "weapon/gun");
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

class PillarSkillVS : public OneCardViewAsSkill
{
public:
    PillarSkillVS()
        : OneCardViewAsSkill("Pillar")
    {
        response_pattern = "@@Pillar";
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() == 0 && to_select->getTypeId() == Card::TypeBasic)
            return true;
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcard(originalCard);
            slash->setSkillName("_Pillar");
            return slash;
        }
        return NULL;
    }
};

class PillarSkill : public WeaponSkill
{
public:
    PillarSkill()
        : WeaponSkill("Pillar")
    {
        events << EventPhaseEnd;
        frequency = Compulsory;
        view_as_skill = new PillarSkillVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->isDead() || player->getPhase() != Player::Finish)
            return QList<SkillInvokeDetail>();
        if (!equipAvailable(player, EquipCard::WeaponLocation, objectName()))
            return QList<SkillInvokeDetail>();

        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        const Card *c = room->askForUseCard(invoke->invoker, "@@Pillar", "@Pillar", -1, Card::MethodUse, false, objectName());
        if (c == NULL && invoke->invoker->getWeapon() && invoke->invoker->getWeapon()->objectName() == objectName()) {
            int id = invoke->invoker->getWeapon()->getId();
            invoke->invoker->addBrokenEquips(QList<int>() << id);
        }
        return false;
    }
};

Pillar::Pillar(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("Pillar");
}

class HakkeroSkill : public WeaponSkill
{
public:
    HakkeroSkill()
        : WeaponSkill("Hakkero")
    {
        events << SlashMissed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (!equipAvailable(effect.from, EquipCard::WeaponLocation, objectName()))
            return QList<SkillInvokeDetail>();
        if (effect.from->isAlive() && effect.to->isAlive() && effect.jink != NULL && effect.from->canDiscard(effect.to, "hes"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.from, effect.from);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        int damage_value = 1 + effect.drank + effect.effectValue.last();

        QStringList damage_flags;
        damage_flags << "jidu_card"
                     << "mofa_card"
                     << "yuxueSlash";
        foreach (const QString &flag, effect.slash->getFlags())
            if (damage_flags.contains(flag))
                damage_value++;

        DummyCard *dummy = new DummyCard;
        int card_id = -1;
        QList<int> disable;
        foreach (const Card *c, effect.to->getCards("hes")) {
            if (!effect.from->canDiscard(effect.to, c->getEffectiveId()))
                disable << c->getEffectiveId();
        }
        for (int i = 0; i < damage_value; i += 1) {
            card_id = room->askForCardChosen(effect.from, effect.to, "hes", objectName(), false, Card::MethodDiscard, disable);
            disable << card_id;
            dummy->addSubcard(card_id);

            if (effect.to->getCards("hes").length() - disable.length() <= 0)
                break;
        }

        if (dummy->subcardsLength() > 0) {
            room->setEmotion(effect.to, "weapon/hakkero");
            room->throwCard(dummy, effect.to, effect.from);
        }
        delete dummy;
        return false;
    }
};

Hakkero::Hakkero(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("Hakkero");
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
    }

    virtual const Card *viewAs() const
    {
        KnownBoth *card = new KnownBoth(Card::SuitToBeDecided, -1);
        card->setSkillName(objectName());
        card->setCanRecast(false);
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
        response_or_use = true; //only skill shenbao can use WoodenOx
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (!EquipSkill::equipAvailable(player, EquipCard::TreasureLocation, objectName()))
            return false;
        if (player->hasFlag("Pagoda_used"))
            return false;

        return pattern == response_pattern;
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

        return !player->isKongcheng();
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
                room->setEmotion(use.from, "treasure/pagoda");
                room->setPlayerFlag(use.from, "Pagoda_used");
            }
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerFlag(p, "-Pagoda_used");
            }
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
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
            room->setEmotion(invoke->invoker, "armor/camouflage");

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

class HagoromoSkill : public ArmorSkill
{
public:
    HagoromoSkill()
        : ArmorSkill("Hagoromo")
    {
        events << CardAsked;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *current = room->getCurrent();
        if (!current || current->isDead() || current->isChained())
            return QList<SkillInvokeDetail>();

        CardAskedStruct ask = data.value<CardAskedStruct>();
        if (!matchAvaliablePattern("jink", ask.pattern))
            return QList<SkillInvokeDetail>();

        ServerPlayer *player = ask.player;
        if (equipAvailable(player, EquipCard::ArmorLocation, objectName()) && player->getArmor() && player->getArmor()->objectName() == objectName()) {
            Card *jink = Sanguosha->cloneCard("chain_jink");
            DELETE_OVER_SCOPE(Card, jink)
            if (player->isCardLimited(jink, ask.method))
                return QList<SkillInvokeDetail>();
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->setEmotion(invoke->invoker, "armor/hagoromo");
        int id = invoke->invoker->getArmor()->getEffectiveId();
        invoke->invoker->addBrokenEquips(QList<int>() << id);

        ChainJink *jink = new ChainJink(Card::NoSuit, 0);
        jink->setSkillName(objectName());
        room->provide(jink);
        return true;
    }
};

Hagoromo::Hagoromo(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("Hagoromo");
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

AllianceFeast::AllianceFeast(Suit suit, int number)
    : GlobalEffect(suit, number)
{
    setObjectName("alliance_feast");
}

bool AllianceFeast::isCancelable(const CardEffectStruct &effect) const
{
    return (effect.to->isChained() || !effect.to->getShownHandcards().isEmpty() || !effect.to->getBrokenEquips().isEmpty()) && TrickCard::isCancelable(effect);
}

void AllianceFeast::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    if (!(effect.to->isChained() || !effect.to->getShownHandcards().isEmpty() || !effect.to->getBrokenEquips().isEmpty())) {
        room->setCardFlag(this, "-tianxieEffected_" + effect.to->objectName()); //only for skill tianxie
    } else {
        effect.to->removeShownHandCards(effect.to->getShownHandcards(), true);
        effect.to->removeBrokenEquips(effect.to->getBrokenEquips(), true);
        if (effect.to->isChained())
            effect.to->getRoom()->setPlayerProperty(effect.to, "chained", !effect.to->isChained());

        int drawnum = 1 + effect.effectValue.first();
        effect.to->drawCards(drawnum);
    }
}

BoneHealing::BoneHealing(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("bone_healing");
    can_damage = true;
}

bool BoneHealing::isAvailable(const Player *player) const
{
    auto siblings = player->getAliveSiblings();

    foreach (const Player *p, siblings) {
        if (targetFilter(QList<const Player *>(), p, player))
            return SingleTargetTrick::isAvailable(player);
    }

    return false;
}

bool BoneHealing::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num || to_select == Self)
        return false;
    bool ignore = (Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && to_select != Self && !hasFlag("IgnoreFailed"));
    if (ignore)
        return true;

    return to_select->isDebuffStatus();
}

void BoneHealing::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    DamageStruct damage(effect.card, effect.from, effect.to, 1 + effect.effectValue.first());
    room->damage(damage);

    effect.to->removeShownHandCards(effect.to->getShownHandcards(), true);
    effect.to->removeBrokenEquips(effect.to->getBrokenEquips(), true);
    if (effect.to->isChained())
        effect.to->getRoom()->setPlayerProperty(effect.to, "chained", !effect.to->isChained());
}

SpringBreath::SpringBreath(Suit suit, int number)
    : DelayedTrick(suit, number, false, true)
{
    setObjectName("spring_breath");

    judge.pattern = ".|heart|2~9";
    judge.good = true;
    judge.negative = false;
    judge.reason = objectName();
}

QString SpringBreath::getSubtype() const
{
    return "unmovable_delayed_trick";
}

bool SpringBreath::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    bool ignore
        = (Self && Self->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && to_select != Self && !hasFlag("IgnoreFailed"));
    return targets.isEmpty() && (!to_select->containsTrick(objectName()) || ignore);
}

void SpringBreath::takeEffect(ServerPlayer *target) const
{
    target->drawCards(6);
}

TestCardPackage::TestCardPackage()
    : Package("test_card", Package::CardPack)
{
    QList<Card *> cards;

    // clang-format off

    cards
        // Equip
        << new Gun(Card::Diamond, 1)//Card::Club, 13
        //<< new Pillar(Card::Diamond, 1)
        << new Hakkero(Card::Spade, 11)
        << new JadeSeal(Card::Heart, 13)
        << new Pagoda(Card::Spade, 12)
        << new Camouflage(Card::Spade, 1)
        << new Hagoromo(Card::Club, 1)
        //Trick
        << new AwaitExhausted(Card::Diamond, 4)
        << new AwaitExhausted(Card::Heart, 10)
        << new AllianceFeast(Card::Heart, 1)
        << new BoneHealing(Card::Spade, 7)
        << new BoneHealing(Card::Spade, 3)
        << new BoneHealing(Card::Club, 5)

        << new Nullification(Card::Club, 12)
        << new Nullification(Card::Club, 11)

        << new SpringBreath(Card::Heart, 8)

        //Basic
        << new IronSlash(Card::Club, 7)
        << new IronSlash(Card::Spade, 9)
        << new IronSlash(Card::Spade, 5)
        << new LightSlash(Card::Club, 6)
        << new LightSlash(Card::Club, 9)
        << new LightSlash(Card::Club, 4)
        << new PowerSlash(Card::Club, 10)
        << new PowerSlash(Card::Diamond, 8)
        << new PowerSlash(Card::Spade, 6)
        << new ThunderSlash(Card::Spade, 8)
        << new ThunderSlash(Card::Club, 8)
        << new FireSlash(Card::Heart, 9)

        << new MagicAnaleptic(Card::Spade, 10)
        << new MagicAnaleptic(Card::Diamond, 6)
        << new SuperPeach(Card::Heart, 4)
        << new SuperPeach(Card::Diamond, 13)
        << new SuperPeach(Card::Heart, 7)
        << new SuperPeach(Card::Diamond, 3)

        << new ChainJink(Card::Heart, 5)
        << new ChainJink(Card::Heart, 6)
        << new ChainJink(Card::Heart, 3)
        << new LightJink(Card::Diamond, 2)
        << new LightJink(Card::Diamond, 11)
        << new LightJink(Card::Diamond, 7);

    // clang-format on

    foreach (Card *card, cards)
        card->setParent(this);

    skills << new GunSkill << new JadeSealSkill << new JadeSealTriggerSkill << new PagodaSkill << new PagodaTriggerSkill << new CamouflageSkill << new PillarSkill
           << new HakkeroSkill << new HagoromoSkill;
}

ADD_PACKAGE(TestCard)
