#include "th15.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "skill.h"
#include "washout.h"

#include <QCommandLinkButton>
#include <QCoreApplication>
#include <QPointer>

class XiahuiVS : public OneCardViewAsSkill
{
public:
    XiahuiVS()
        : OneCardViewAsSkill("xiahui")
    {
        response_pattern = "@@xiahui!";
        filter_pattern = ".|.|.|handOnly";
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        return new DummyCard({originalCard->getEffectiveId()});
    }
};

class Xiahui : public TriggerSkill
{
public:
    Xiahui()
        : TriggerSkill("xiahui")
    {
        events = {Damage, Damaged, EventPhaseStart};
        view_as_skill = new XiahuiVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        ServerPlayer *fox = nullptr;

        {
            if (triggerEvent == Damage) {
                DamageStruct damage = data.value<DamageStruct>();
                fox = damage.from;
            } else if (triggerEvent == Damaged) {
                DamageStruct damage = data.value<DamageStruct>();
                fox = damage.to;
            } else if (triggerEvent == EventPhaseStart) {
                fox = data.value<ServerPlayer *>();
                if (fox->getPhase() != Player::Play)
                    fox = nullptr;
            }
        }

        if (fox != nullptr && fox->isAlive() && fox->hasSkill(this)) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!p->isKongcheng() && (p->getShownHandcards().length() < p->getHandcardNum()))
                    return {SkillInvokeDetail(this, fox, fox)};
            }
        }

        return {};
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QList<ServerPlayer *> targets;

        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (!p->isKongcheng() && (p->getShownHandcards().length() < p->getHandcardNum()))
                targets << p;
        }

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@xiahui-show" + QString::number(static_cast<int>(triggerEvent)), true, true);
        if (target != nullptr) {
            int id;
            if (invoke->invoker != target) {
                id = room->askForCardChosen(invoke->invoker, target, "h", objectName());
            } else {
                const Card *c = room->askForCard(invoke->invoker, "@@xiahui!", "@xiahui-showself", {}, Card::MethodNone, nullptr, false, QString(), false, 0);
                if (c == nullptr) {
                    foreach (int id, invoke->invoker->handCards()) {
                        if (!invoke->invoker->isShownHandcard(id)) {
                            c = Sanguosha->getCard(id);
                            break;
                        }
                    }
                }
                if (c != nullptr) {
                    id = c->getEffectiveId();
                } else {
                    Q_UNREACHABLE();
                }
            }

            invoke->targets << target;
            invoke->tag[objectName()] = id;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        bool ok = false;
        int id = invoke->tag.value(objectName()).toInt(&ok);
        if (ok)
            invoke->targets.first()->addToShownHandCards({id});
        return false;
    }
};

class Shayi : public TriggerSkill
{
public:
    Shayi()
        : TriggerSkill("shayi$")
    {
        events << CardUsed;
    }

    static bool canShayiDamage(CardUseStruct use)
    {
        if (use.card->canDamage())
            return true;
        if (use.card->getSkillName() == "xianshi") {
            QString cardname = use.from->property("xianshi_card").toString();
            if (cardname != nullptr) {
                Card *card = Sanguosha->cloneCard(cardname);
                if (card != nullptr && card->canDamage())
                    return true;

                DELETE_OVER_SCOPE(Card, card)
            }
        }
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card == nullptr || !canShayiDamage(use) || (use.from == nullptr) || use.from->isDead() || use.from->getKingdom() != "gzz")
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->getOtherPlayers(use.from)) {
            if (p->hasLordSkill(this))
                d << SkillInvokeDetail(this, p, use.from);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (invoke->invoker->askForSkillInvoke("shayi_change", data)) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->owner->objectName());

            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(invoke->owner, objectName());
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = invoke->invoker;
            log.to << invoke->owner;
            log.arg = objectName();
            room->sendLog(log);

            return invoke->owner->askForSkillInvoke(objectName(), data);
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.from = invoke->owner;
        data = QVariant::fromValue(use);

        LogMessage log;
        log.type = "$Shayi";
        log.from = invoke->owner;
        log.card_str = use.card->toString();
        log.arg = objectName();
        room->sendLog(log);
        return false;
    }
};

class ChunhuaVS : public OneCardViewAsSkill
{
public:
    ChunhuaVS()
        : OneCardViewAsSkill("chunhua")
    {
        response_pattern = "@@chunhua";
        expand_pile = "*chunhua";
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        return new DummyCard({originalCard->getEffectiveId()});
    }

    bool viewFilter(const Card *to_select) const override
    {
        QList<int> ids = StringList2IntList(Self->property(objectName().toUtf8().constData()).toString().split("+"));
        if (Self->property("chunhua_isSelfUsing").toString() == "1")
            ids << Self->getShownHandcards();
        return ids.contains(to_select->getEffectiveId());
    }
};

// Todo_Fs: decouple Chunhua from GameRule
// using "room->setTag("SkipGameRule", true);"
class Chunhua : public TriggerSkill
{
public:
    Chunhua()
        : TriggerSkill("chunhua")
    {
        events << TargetSpecified << EventPhaseChanging;
        view_as_skill = new ChunhuaVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                p->tag.remove(objectName());
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e == TargetSpecified) {
            ServerPlayer *current = room->getCurrent();
            if (current != nullptr && current->isInMainPhase()) {
                CardUseStruct use = data.value<CardUseStruct>();
                QList<SkillInvokeDetail> d;
                if (use.from != nullptr && use.from->isAlive() && !use.to.isEmpty() && (use.card->isKindOf("BasicCard") || use.card->isNDTrick())
                    && ((use.card->hasFlag("showncards") && (use.card->isBlack() || use.card->isRed()) && (!use.card->isVirtualCard() || !use.card->getSubcards().isEmpty()))
                        || !use.from->getShownHandcards().isEmpty())) {
                    foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                        if (!p->tag.contains(objectName()))
                            d << SkillInvokeDetail(this, p, p, use.from);
                    }
                }
                return d;
            }
        }

        return {};
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();

        bool useCardIsShownAndHaveColor
            = (use.card->hasFlag("showncards") && (use.card->isBlack() || use.card->isRed()) && (!use.card->isVirtualCard() || !use.card->getSubcards().isEmpty()));
        QList<int> shownCards = use.from->getShownHandcards();
        if (useCardIsShownAndHaveColor)
            shownCards.prepend(use.card->getEffectiveId());

        if (shownCards.isEmpty()) {
            Q_UNREACHABLE();
            return false;
        }

        QStringList prompt = {
            QString(),
            use.from->objectName(),
            QString(),
            use.card->objectName(),
        };

        int noticeIndex = 2;
        if (!useCardIsShownAndHaveColor)
            noticeIndex = 3;
        else if (use.from->getShownHandcards().isEmpty())
            noticeIndex = 1;
        prompt[0] = "@chunhua-prompt" + QString::number(noticeIndex);

        invoke->invoker->tag[objectName() + "_cards"] = IntList2VariantList(shownCards);
        if (use.from != invoke->invoker)
            room->setPlayerProperty(invoke->invoker, objectName().toUtf8().constData(), IntList2StringList(shownCards).join("+"));
        else if (useCardIsShownAndHaveColor)
            room->setPlayerProperty(invoke->invoker, objectName().toUtf8().constData(), IntList2StringList({use.card->getEffectiveId()}).join("+"));
        room->setPlayerProperty(invoke->invoker, (objectName() + "_firstCardIsUsing").toUtf8().constData(), QString(useCardIsShownAndHaveColor ? "1" : "0"));
        room->setPlayerProperty(invoke->invoker, (objectName() + "_isSelfUsing").toUtf8().constData(), QString((use.from == invoke->invoker) ? "1" : "0"));
        const Card *c = room->askForCard(invoke->invoker, "@@chunhua", prompt.join(":"), data, Card::MethodNone, invoke->targets.first(), false, objectName(), false, noticeIndex);
        invoke->invoker->tag.remove(objectName() + "_cards");
        room->setPlayerProperty(invoke->invoker, objectName().toUtf8().constData(), QString());
        room->setPlayerProperty(invoke->invoker, (objectName() + "_firstCardIsUsing").toUtf8().constData(), QString());
        room->setPlayerProperty(invoke->invoker, (objectName() + "_isSelfUsing").toUtf8().constData(), QString());

        if (c != nullptr) {
            int id = c->getEffectiveId();
            const Card *originalCard = Sanguosha->getCard(id);

            LogMessage l;
            l.type = "$chunhua";
            l.from = invoke->invoker;
            l.to = {use.from};
            l.arg = use.card->objectName();
            if (use.from->getShownHandcards().contains(id)) {
                l.type.append("_removeshown");
                l.card_str = QString::number(id);
                use.from->removeShownHandCards({id}, true);
            } else {
                l.type.append("_selectuse");
                l.card_str = use.card->toString();
            }
            if (originalCard->isBlack()) {
                invoke->tag[objectName()] = "black";
                l.type.append("_black");
            } else {
                invoke->tag[objectName()] = "red";
                l.type.append("_red");
            }

            room->sendLog(l);
            invoke->invoker->tag[objectName()] = true;

            return true;
        }

        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, use.to)
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), p->objectName());

        QString choice = invoke->tag.value("chunhua").toString();
        room->setCardFlag(use.card, "chunhua");
        room->setCardFlag(use.card, "chunhua_" + choice);

        return false;
    }
};

//chunhua nullified
class ChunhuaNullified : public TriggerSkill
{
public:
    ChunhuaNullified()
        : TriggerSkill("#chunhua")
    {
        events << SlashEffected << CardEffected;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (e == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("chunhua")) {
                if (effect.slash->hasFlag("chunhua_red") && !effect.to->isWounded())
                    d << SkillInvokeDetail(this, effect.to, effect.to, nullptr, true);
            }
        }
        if (e == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->hasFlag("chunhua") && !effect.card->isKindOf("Slash")) {
                if (effect.card->hasFlag("chunhua_red") && !effect.to->isWounded())
                    d << SkillInvokeDetail(this, effect.to, effect.to, nullptr, true);
            }
        }
        return d;
    }

    bool effect(TriggerEvent e, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        if (e == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            effect.nullified = true;
            data = QVariant::fromValue(effect);
        }
        if (e == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            effect.nullified = true;
            data = QVariant::fromValue(effect);
        }
        return false;
    }
};

class ChunhuaCardEffect : public TriggerSkill
{
public:
    ChunhuaCardEffect()
        : TriggerSkill("#chunhua-cardeffect")
    {
        events = {SlashHit, CardEffected};
    }

    int getPriority() const override
    {
        return 1;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->hasFlag("chunhua") && !effect.card->isKindOf("Slash") && !effect.nullified)
                return {SkillInvokeDetail(this, effect.to, effect.to, nullptr, true, nullptr, false)};
        } else if (triggerEvent == SlashHit) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("chunhua"))
                return {SkillInvokeDetail(this, effect.to, effect.to, nullptr, true, nullptr, false)};
        }

        return {};
    }

    // Note: DO NOT SET room tag SkipGameRule if this function returns true!!!!!
    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        do {
            if (triggerEvent == CardEffected) {
                CardEffectStruct effect = data.value<CardEffectStruct>();

                if (effect.card->getTypeId() == Card::TypeTrick) {
                    if (room->isCanceled(effect)) {
                        effect.to->setFlags("Global_NonSkillNullify");
                        return true;
                    } else {
                        room->getThread()->trigger(TrickEffect, room, data);
                    }
                }
                if (effect.card->hasFlag("chunhua_black")) {
                    DamageStruct d = DamageStruct(effect.card, effect.from, effect.to, 1 + effect.effectValue.first(), DamageStruct::Normal);
                    room->damage(d);
                } else if (effect.card->hasFlag("chunhua_red")) {
                    RecoverStruct recover;
                    recover.card = effect.card;
                    recover.who = effect.from;
                    recover.recover = 1 + effect.effectValue.first();
                    room->recover(effect.to, recover);
                }
            } else if (triggerEvent == SlashHit) {
                SlashEffectStruct effect = data.value<SlashEffectStruct>();
                if (effect.slash->hasFlag("chunhua_red")) {
                    RecoverStruct recover;
                    recover.card = effect.slash;
                    recover.who = effect.from;
                    recover.recover = 1 + effect.effectValue.first();
                    room->recover(effect.to, recover);
                } else {
                    effect.nature = DamageStruct::Normal;
                    if (effect.drank > 0 || effect.magic_drank > 0) {
                        int drank_num = (effect.slash->isKindOf("LightSlash") || effect.slash->isKindOf("PowerSlash")) ? effect.drank : effect.drank + effect.magic_drank;
                        effect.to->setMark("SlashIsDrank", drank_num);
                    }

                    DamageStruct d = DamageStruct(effect.slash, effect.from, effect.to, 1 + effect.effectValue.last(), effect.nature);
                    foreach (ServerPlayer *p, room->getAllPlayers(true)) {
                        if (effect.slash->hasFlag("WushenDamage_" + p->objectName())) {
                            d.from = p->isAlive() ? p : nullptr;
                            d.by_user = false;
                            break;
                        }
                    }

                    room->damage(d);
                }
            }
        } while (0);

        room->setTag("SkipGameRule", true);
        return false;
    }
};

class Santi : public TriggerSkill
{
public:
    Santi()
        : TriggerSkill("santi")
    {
        events << EventPhaseStart;
        frequency = Skill::Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            QVariant extraTag = room->getTag("touhou-extra");
            bool nowExtraTurn = extraTag.canConvert(QVariant::Bool) && extraTag.toBool();

            ServerPlayer *player = data.value<ServerPlayer *>();
            if (!nowExtraTurn && player->hasSkill(this) && player->getPhase() == Player::RoundStart)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, nullptr, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *player = invoke->invoker;
        room->sendLog("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());

        QList<Player::Phase> phases = player->getPhases();
        int index = phases.indexOf(Player::Discard, 0);
        QList<Player::Phase> new_phases;
        new_phases << Player::Draw << Player::Play << Player::Discard;
        player->insertPhases(new_phases, index + 1);
        player->insertPhases(new_phases, index + 1);

        return false;
    }
};

class SantiEffect : public TriggerSkill
{
public:
    SantiEffect()
        : TriggerSkill("#santi")
    {
        frequency = Compulsory;
        events << EventPhaseChanging << CardUsed << CardResponded;
    }

    static void removeSantiLimit(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        if (player->isCardLimited("use", "santi_limit_basic"))
            room->removePlayerCardLimitation(player, "use", "TrickCard,EquipCard$1", "santi_limit_basic");

        if (player->isCardLimited("use", "santi_limit_equip"))
            room->removePlayerCardLimitation(player, "use", "TrickCard,BasicCard$1", "santi_limit_equip");

        if (player->isCardLimited("use", "santi_limit_trick"))
            room->removePlayerCardLimitation(player, "use", "EquipCard,BasicCard$1", "santi_limit_trick");

        if (player->isCardLimited("use", "santi_limit_basic_used"))
            room->removePlayerCardLimitation(player, "use", "BasicCard$1", "santi_limit_basic_used");

        if (player->isCardLimited("use", "santi_limit_equip_used"))
            room->removePlayerCardLimitation(player, "use", "EquipCard$1", "santi_limit_equip_used");

        if (player->isCardLimited("use", "santi_limit_trick_used"))
            room->removePlayerCardLimitation(player, "use", "TrickCard$1", "santi_limit_trick_used");
    }

    static void setSantiLimit(ServerPlayer *player, bool limit_used)
    {
        Room *room = player->getRoom();
        removeSantiLimit(player);
        if (player->getMark("santi_basic") > 0)
            room->setPlayerCardLimitation(player, "use", "TrickCard,EquipCard", "santi_limit_basic", true);
        else if (player->getMark("santi_equip") > 0)
            room->setPlayerCardLimitation(player, "use", "TrickCard,BasicCard", "santi_limit_equip", true);
        else if (player->getMark("santi_trick") > 0)
            room->setPlayerCardLimitation(player, "use", "EquipCard,BasicCard", "santi_limit_trick", true);

        if (limit_used) {
            if (player->getMark("santi_basic_used") > 0)
                room->setPlayerCardLimitation(player, "use", "BasicCard", "santi_limit_basic_used", true);
            if (player->getMark("santi_equip_used") > 0)
                room->setPlayerCardLimitation(player, "use", "EquipCard", "santi_limit_equip_used", true);
            if (player->getMark("santi_trick_used") > 0)
                room->setPlayerCardLimitation(player, "use", "TrickCard", "santi_limit_trick_used", true);
        }
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        //set or remove limitation
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                room->setPlayerMark(change.player, "santi_trick", 0);
                room->setPlayerMark(change.player, "santi_basic", 0);
                room->setPlayerMark(change.player, "santi_equip", 0);

                removeSantiLimit(change.player);
            }
            if (change.to == Player::NotActive) {
                room->setPlayerMark(change.player, "santi_trick_used", 0);
                room->setPlayerMark(change.player, "santi_basic_used", 0);
                room->setPlayerMark(change.player, "santi_equip_used", 0);
            }
            if (change.to == Player::Play) {
                setSantiLimit(change.player, true);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        if (e == CardUsed || e == CardResponded) {
            ServerPlayer *player = nullptr;
            const Card *card = nullptr;
            if (e == CardUsed) {
                player = data.value<CardUseStruct>().from;
                card = data.value<CardUseStruct>().card;
            } else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                player = response.m_from;
                if (response.m_isUse)
                    card = response.m_card;
            }
            if ((player != nullptr) && player->hasSkill(this) && player->getPhase() == Player::Play && (card != nullptr) && card->getHandlingMethod() == Card::MethodUse) {
                if (card->isKindOf("EquipCard") && player->getMark("santi_equip") == 0)
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, nullptr, true);

                if (card->isKindOf("BasicCard") && player->getMark("santi_basic") == 0)
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, nullptr, true);

                if (card->isKindOf("TrickCard") && player->getMark("santi_trick") == 0)
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, nullptr, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        const Card *card = nullptr;
        if (e == CardUsed)
            card = data.value<CardUseStruct>().card;
        else
            card = data.value<CardResponseStruct>().m_card;

        if (card->isKindOf("EquipCard")) {
            room->setPlayerMark(invoke->invoker, "santi_equip", 1);
            room->setPlayerMark(invoke->invoker, "santi_equip_used", 1);
        } else if (card->isKindOf("BasicCard")) {
            room->setPlayerMark(invoke->invoker, "santi_basic", 1);
            room->setPlayerMark(invoke->invoker, "santi_basic_used", 1);
        }

        else if (card->isKindOf("TrickCard")) {
            room->setPlayerMark(invoke->invoker, "santi_trick", 1);
            room->setPlayerMark(invoke->invoker, "santi_trick_used", 1);
        }
        setSantiLimit(invoke->invoker, false);
        return false;
    }
};

class Kuangluan1 : public TriggerSkill
{
public:
    Kuangluan1()
        : TriggerSkill("#kuangluan1")
    {
        events << CardsMoveOneTime;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        if (room->getTag("FirstRound").toBool())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *playerTo = qobject_cast<ServerPlayer *>(move.to);
        if (playerTo != nullptr && playerTo->isAlive() && move.to_place == Player::PlaceHand && playerTo->getPhase() != Player::Draw && playerTo->getShownHandcards().isEmpty()) {
            bool trigger = false;
            foreach (int id, move.card_ids) {
                if (!playerTo->isShownHandcard(id) && room->getCardPlace(id) == Player::PlaceHand && room->getCardOwner(id) == playerTo) {
                    trigger = true;
                    break;
                }
            }
            if (!trigger)
                return d;
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p != playerTo)
                    d << SkillInvokeDetail(this, p, p, nullptr, false, playerTo);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (invoke->invoker->askForSkillInvoke("kuangluan1", QVariant::fromValue(invoke->preferredTarget))) {
            room->notifySkillInvoked(invoke->invoker, "kuangluan");
            room->sendLog("#InvokeSkill", invoke->owner, objectName());
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> ids;
        foreach (int id, move.card_ids) {
            if (!invoke->targets.first()->isShownHandcard(id) && room->getCardPlace(id) == Player::PlaceHand && room->getCardOwner(id) == invoke->targets.first()) {
                ids << id;
            }
        }
        if (!ids.isEmpty()) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
            invoke->targets.first()->addToShownHandCards(ids);
        }

        return false;
    }
};

class Kuangluan2 : public TriggerSkill
{
public:
    Kuangluan2()
        : TriggerSkill("#kuangluan2")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::NotActive) { //change.to == Player::NotActive ||
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark("kuangluan_invalidity") > 0) {
                        room->setPlayerMark(p, "kuangluan_invalidity", 0);
                        room->setPlayerSkillInvalidity(p, nullptr, false);
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent != CardsMoveOneTime)
            return d;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *playerFrom = qobject_cast<ServerPlayer *>(move.from);
        if (playerFrom != nullptr && playerFrom->isAlive() && !move.shown_ids.isEmpty() && playerFrom->getShownHandcards().isEmpty()
            && playerFrom->getMark("kuangluan_invalidity") == 0) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p != playerFrom)
                    d << SkillInvokeDetail(this, p, p, nullptr, false, playerFrom);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        if (invoke->invoker->askForSkillInvoke("kuangluan2", QVariant::fromValue(invoke->preferredTarget))) {
            room->notifySkillInvoked(invoke->invoker, "kuangluan");
            room->sendLog("#InvokeSkill", invoke->owner, objectName());

            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        if (!invoke->targets.first()->hasFlag("kuangluan_invalidity")) {
            room->setPlayerMark(invoke->targets.first(), "kuangluan_invalidity", 1);
            room->sendLog("#kuangluan_invalidity", invoke->targets.first(), "kuangluan");
            room->setPlayerSkillInvalidity(invoke->targets.first(), nullptr, true);
        }
        return false;
    }
};

class Yuyi : public TriggerSkill
{
public:
    Yuyi()
        : TriggerSkill("yuyi")
    {
        events << DamageInflicted;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if ((damage.card != nullptr) && damage.card->isKindOf("Slash") && damage.to->isAlive() && damage.to->hasSkill(this) && damage.to->canDiscard(damage.to, "hs"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        const Card *card = room->askForCard(invoke->invoker, ".|.|.|hand", "@yuyi_discard", data, Card::MethodDiscard, nullptr, false, objectName());
        invoke->tag["yuyi"] = QVariant::fromValue(card);
        return card != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if ((damage.from == nullptr) || damage.from->isDead() || !invoke->invoker->canDiscard(damage.from, "hs"))
            return false;

        int id = room->askForCardChosen(invoke->invoker, damage.from, "hs", objectName());
        room->throwCard(id, damage.from, invoke->invoker);
        const Card *c = invoke->tag.value("yuyi").value<const Card *>();
        if (c->getColor() != Sanguosha->getCard(id)->getColor()) {
            room->sendLog("#YuyiTrigger", invoke->invoker, objectName(), QList<ServerPlayer *>(), QString::number(1));
            damage.damage = damage.damage - 1;
            data = QVariant::fromValue(damage);
            if (damage.damage <= 0)
                return true;
        }

        return false;
    }
};

class Shehuo : public TriggerSkill
{
public:
    Shehuo()
        : TriggerSkill("shehuo")
    {
        events << TargetSpecifying << TargetConfirming;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.length() != 1 || use.from == nullptr || use.from == use.to.first())
            return QList<SkillInvokeDetail>();
        if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
            if (e == TargetSpecifying && use.from->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
            if (e == TargetConfirming && use.to.first()->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.to.first(), use.to.first());
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "target:" + use.from->objectName() + ":" + use.to.first()->objectName() + ":" + use.card->objectName();
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *target = use.from;
        ServerPlayer *player = use.to.first();

        room->setPlayerFlag(target, "Global_shehuoFailed"); //for targetfilter
        room->setPlayerFlag(player, "Global_shehuoInvokerFailed"); //for targetfilter

        QString prompt = "@shehuo_use:" + target->objectName() + ":" + use.card->objectName();
        QString pattern;

        if (use.card->isNDTrick())
            pattern = "BasicCard+^Jink,EquipCard|.|.|shehuo";
        else
            pattern = "TrickCard+^Nullification,EquipCard|.|.|shehuo";

        //for ai
        player->tag["shehuo_target"] = QVariant::fromValue(target);
        const Card *c = room->askForUseCard(player, pattern, prompt, -1, Card::MethodUse, false, objectName());

        if (c != nullptr) {
            room->sendLog("$CancelTarget", use.from, use.card->objectName(), use.to);
            foreach (ServerPlayer *p, use.to)
                room->setEmotion(p, "skill_nullify");
            use.to.clear();
            data = QVariant::fromValue(use);
        } else {
            if (use.card->isKindOf("Slash"))
                room->setCardFlag(use.card, "ZeroJink");
            else
                room->setCardFlag(use.card, "shehuoTrick");
        }
        return false;
    }
};

class ShehuoEffect : public TriggerSkill
{
public:
    ShehuoEffect()
        : TriggerSkill("#shehuo")
    {
        events << TrickCardCanceling;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if ((effect.to != nullptr) && (effect.card != nullptr) && effect.card->hasFlag("shehuoTrick"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, nullptr, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &) const override
    {
        return true;
    }
};

class ShehuoProhibit : public ProhibitSkill
{
public:
    ShehuoProhibit()
        : ProhibitSkill("#shehuoProhibit")
    {
    }

    bool isProhibited(const Player *from, const Player *to, const Card *, const QList<const Player *> &targets, bool) const override
    {
        if (from != nullptr && from->hasFlag("Global_shehuoInvokerFailed")) {
            bool check = false;
            foreach (const Player *p, targets) {
                if (p->hasFlag("Global_shehuoFailed")) {
                    check = true;
                    break;
                }
            }
            if (!check)
                return !to->hasFlag("Global_shehuoFailed");
            return false;
        }
        return false;
    }
};

class ShehuoTargetMod : public TargetModSkill
{
public:
    ShehuoTargetMod()
        : TargetModSkill("#shehuoTargetMod")
    {
        pattern = ".";
    }

    int getDistanceLimit(const Player *from, const Card *) const override
    {
        if (from->hasFlag("Global_shehuoInvokerFailed"))
            return 1000;
        else
            return 0;
    }
};

class Shenyan : public TriggerSkill
{
public:
    Shenyan()
        : TriggerSkill("shenyan")
    {
        events << EventPhaseStart << EventPhaseChanging << DamageDone;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if ((damage.from != nullptr) && damage.from->isCurrent())
                room->setPlayerFlag(damage.from, "shenyan_used");
        }

        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerFlag(p, "-shenyan_used");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e != EventPhaseStart)
            return QList<SkillInvokeDetail>();
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->isDead() || current->getPhase() != Player::Discard || current->hasFlag("shenyan_used"))
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (p != current) {
                AwaitExhausted *card = new AwaitExhausted(Card::NoSuit, 0);
                card->setSkillName(objectName());
                card->deleteLater();
                if (p->isCardLimited(card, Card::MethodUse) || p->isProhibited(p, card, {current}) || p->isProhibited(current, card, {p}))
                    continue;
                d << SkillInvokeDetail(this, p, p);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        AwaitExhausted *card = new AwaitExhausted(Card::NoSuit, 0);
        card->setSkillName(objectName());
        CardUseStruct use;
        use.card = card;
        use.from = invoke->invoker;
        ServerPlayer *current = room->getCurrent();
        use.to << current;
        if (current != invoke->invoker)
            use.to << invoke->invoker;
        room->useCard(use, false);
        return false;
    }
};

class Bumeng : public TriggerSkill
{
public:
    Bumeng()
        : TriggerSkill("bumeng")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerFlag(p, "-bumeng");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e != CardsMoveOneTime)
            return QList<SkillInvokeDetail>();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
            ServerPlayer *thrower = room->findPlayerByObjectName(move.reason.m_playerId);
            if (player != nullptr && player->isAlive() && move.to_place == Player::DiscardPile && thrower != nullptr && player == thrower) {
                QList<int> ids;
                foreach (int id, move.card_ids) {
                    if (room->getCardPlace(id) == Player::DiscardPile)
                        ids << id;
                }
                if (ids.isEmpty())
                    return QList<SkillInvokeDetail>();
                QList<SkillInvokeDetail> d;
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p != player && !p->hasFlag("bumeng"))
                        d << SkillInvokeDetail(this, p, p, nullptr, false, player);
                }
                return d;
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->setPlayerFlag(invoke->invoker, "bumeng");
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *target = invoke->targets.first();
        QList<int> ids;
        foreach (int id, move.card_ids) {
            if (room->getCardPlace(id) == Player::DiscardPile)
                ids << id;
        }
        if (ids.isEmpty())
            return false;

        room->fillAG(ids, invoke->invoker);
        int id1 = room->askForAG(invoke->invoker, ids, false, objectName());
        room->clearAG(invoke->invoker);
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), target->objectName());
        room->obtainCard(target, id1);

        target->addToShownHandCards(QList<int>() << id1);

        ids.clear();
        foreach (int id, move.card_ids) {
            if (room->getCardPlace(id) == Player::DiscardPile)
                ids << id;
        }
        if (ids.isEmpty())
            return false;
        room->fillAG(ids, invoke->invoker);
        int id2 = room->askForAG(invoke->invoker, ids, true, objectName());
        room->clearAG(invoke->invoker);
        if (id2 > -1) {
            room->obtainCard(invoke->invoker, id2);

            invoke->invoker->addToShownHandCards(QList<int>() << id2);
        }
        return false;
    }
};

class Rumeng : public TriggerSkill
{
public:
    Rumeng()
        : TriggerSkill("rumeng")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const override
    {
        //prevent insert (like moveEvent)
        if (e == CardsMoveOneTime) {
            ServerPlayer *current = room->getCurrent();
            if ((current == nullptr) || !current->isAlive() || current->getPhase() != Player::Play)
                return;

            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
            if ((from != nullptr) && move.from_places.contains(Player::PlaceHand)
                && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE
                    || (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE)) {
                if (!move.shown_ids.isEmpty() && from->getShownHandcards().isEmpty())
                    room->setPlayerFlag(from, "rumeng"); //keep this flag till askForSkillInvoke, since some insert could generate new ShownHandcards
            }
        }
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerFlag(p, "-rumeng");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        if (e != CardsMoveOneTime)
            return d;
        ServerPlayer *current = room->getCurrent();
        if ((current == nullptr) || !current->isAlive() || current->getPhase() != Player::Play)
            return d;

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
        if ((from != nullptr) && move.from_places.contains(Player::PlaceHand)
            && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE
                || (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE)) {
            if (!move.shown_ids.isEmpty() && from->hasFlag("rumeng")) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p != current)
                        d << SkillInvokeDetail(this, p, p, nullptr, false, from);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        bool can = invoke->invoker->askForSkillInvoke(this, data);
        //clear flag
        room->setPlayerFlag(invoke->preferredTarget, "-rumeng");
        return can;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &) const override
    {
        ServerPlayer *current = room->getCurrent();
        room->setPlayerFlag(current, "Global_PlayPhaseTerminated");
        room->sendLog("#rumeng_skip", current, "rumeng_skip", QList<ServerPlayer *>(), objectName());
        return false;
    }
};

class ChihouVS : public ViewAsSkill
{
public:
    ChihouVS()
        : ViewAsSkill("chihou")
    {
        response_or_use = true;
    }

    const Player *getChihouOther(const Player *apple) const
    {
        QString name = apple->property((objectName() + "_target").toUtf8().constData()).toString();
        return apple->parent()->findChild<const Player *>(name);
    }

    bool isMagicAnalepticAvailable(const Player *apple, const QString &pattern = QString()) const
    {
        if (apple->getMark("ViewAsSkill_" + objectName() + "Effect") > 0 && apple->getPhase() == Player::Play) {
            if (apple->hasUsed(objectName() + "MagicAnaleptic"))
                return false;
            const Player *other = getChihouOther(apple);
            if (other == nullptr)
                return false;
            QList<int> shownHandcards = apple->getShownHandcards() + other->getShownHandcards();
            bool flag = false;
            foreach (int id, shownHandcards) {
                const Card *c = Sanguosha->getCard(id);
                if (c->isBlack()) {
                    flag = true;
                    break;
                }
            }
            if (!flag)
                return false;
            if (pattern.isEmpty())
                return true;

            const CardPattern *cardPattern = Sanguosha->getPattern(pattern);
            if (cardPattern != nullptr) {
                MagicAnaleptic *ma = new MagicAnaleptic(Card::NoSuit, 0);
                DELETE_OVER_SCOPE(MagicAnaleptic, ma)
                if (cardPattern->match(apple, ma))
                    return true;
            }
        }
        return false;
    }

    bool isAwaitExhaustedAvailable(const Player *apple, const QString &pattern = QString()) const
    {
        if (apple->getMark("ViewAsSkill_" + objectName() + "Effect") > 0 && apple->getPhase() == Player::Play) {
            if (apple->hasUsed(objectName() + "AwaitExhausted"))
                return false;
            const Player *other = getChihouOther(apple);
            if (other == nullptr)
                return false;
            QList<int> shownHandcards = apple->getShownHandcards() + other->getShownHandcards();
            bool flag = false;
            foreach (int id, shownHandcards) {
                const Card *c = Sanguosha->getCard(id);
                if (c->isRed()) {
                    flag = true;
                    break;
                }
            }
            if (!flag)
                return false;
            if (pattern.isEmpty())
                return true;

            const CardPattern *cardPattern = Sanguosha->getPattern(pattern);
            if (cardPattern != nullptr) {
                AwaitExhausted *ae = new AwaitExhausted(Card::NoSuit, 0);
                DELETE_OVER_SCOPE(AwaitExhausted, ae)

                if (cardPattern->match(apple, ae))
                    return true;
            }
        }
        return false;
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override
    {
        QString pattern;
        if (Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY)
            pattern = Sanguosha->getCurrentCardUsePattern();

        if (selected.isEmpty() && isMagicAnalepticAvailable(Self, pattern))
            return !Self->hasEquip(to_select);

        return false;
    }

    const Card *viewAs(const QList<const Card *> &cards) const override
    {
        QString pattern;
        if (Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_PLAY)
            pattern = Sanguosha->getCurrentCardUsePattern();

        if (isAwaitExhaustedAvailable(Self, pattern) && cards.isEmpty()) {
            AwaitExhausted *ae = new AwaitExhausted(Card::NoSuit, 0);
            ae->setSkillName("_" + objectName());
            return ae;
        }
        if (isMagicAnalepticAvailable(Self, pattern) && cards.length() == 1) {
            MagicAnaleptic *ma = new MagicAnaleptic(Card::SuitToBeDecided, -1);
            ma->addSubcards(cards);
            ma->setSkillName("_" + objectName());
            return ma;
        }

        return nullptr;
    }

    bool isEnabledAtPlay(const Player *apple) const override
    {
        return isMagicAnalepticAvailable(apple) || isAwaitExhaustedAvailable(apple);
    }

    bool isEnabledAtResponse(const Player *apple, const QString &pattern) const override
    {
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return isMagicAnalepticAvailable(apple, pattern) || isAwaitExhaustedAvailable(apple, pattern);

        return false;
    }
};

class Chihou : public TriggerSkill
{
public:
    Chihou()
        : TriggerSkill("chihou")
    {
        events = {EventPhaseStart, PreCardUsed, EventPhaseChanging};
        view_as_skill = new ChihouVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == objectName() && use.from != nullptr && use.from->getPhase() == Player::Play)
                room->addPlayerHistory(use.from, objectName() + use.card->getClassName());
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play && change.player->getMark("ViewAsSkill_" + objectName() + "Effect") > 0) {
                room->setPlayerMark(change.player, "ViewAsSkill_" + objectName() + "Effect", 0);
                room->setPlayerProperty(change.player, (objectName() + "_target").toUtf8().constData(), QString());
                change.player->tag.remove(objectName() + "_target");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *apple = data.value<ServerPlayer *>();
            if (apple->isAlive() && apple->hasSkill(this) && apple->getPhase() == Player::Play && !apple->isKongcheng()) {
                foreach (ServerPlayer *p, room->getOtherPlayers(apple)) {
                    if (!p->isKongcheng())
                        return {SkillInvokeDetail(this, apple, apple)};
                }
            }
        }

        return {};
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *apple = invoke->invoker;
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(apple)) {
            if (!p->isKongcheng())
                targets << p;
        }

        ServerPlayer *target = room->askForPlayerChosen(apple, targets, objectName(), "@chihou-apple", true, true);
        if (target != nullptr) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *apple = invoke->invoker;
        ServerPlayer *target = invoke->targets.first();

        room->setPlayerMark(apple, "ViewAsSkill_" + objectName() + "Effect", 1);
        room->setPlayerProperty(apple, (objectName() + "_target").toUtf8().constData(), target->objectName());
        apple->tag[objectName() + "_target"] = QVariant::fromValue(target);

        const Card *d = room->askForExchange(target, objectName(), 1, 1, false, "@chihou-show1:" + apple->objectName());
        target->addToShownHandCards({d->getEffectiveId()});
        const Card *c = room->askForExchange(apple, objectName(), 1, 1, false, "@chihou-show2:" + target->objectName());
        apple->addToShownHandCards({c->getEffectiveId()});

        return false;
    }
};

YidanDialog *YidanDialog::getInstance()
{
    static QPointer<YidanDialog> instance;

    if (instance.isNull()) {
        instance = new YidanDialog;
        connect(qApp, &QCoreApplication::aboutToQuit, instance.data(), &YidanDialog::deleteLater);
    }

    return instance;
}

YidanDialog::YidanDialog()
{
    setObjectName("yidan");

    group = new QButtonGroup(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(createButton("light_slash"));
    layout->addWidget(createButton("iron_slash"));
    setLayout(layout);

    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectCard(QAbstractButton *)));
}

void YidanDialog::popup()
{
    Self->tag.remove(objectName());

    QStringList availablePatterns;
    foreach (QAbstractButton *button, group->buttons()) {
        const Card *card = map[button->objectName()];
        if (card->isAvailable(Self) && !Self->hasUsed(objectName() + card->getClassName()))
            availablePatterns << button->objectName();
    }

    if (availablePatterns.length() == 1) {
        Self->tag[objectName()] = availablePatterns.first();
        emit onButtonClick();
        return;
    }

    exec();
}

void YidanDialog::selectCard(QAbstractButton *button)
{
    const Card *card = map.value(button->objectName());
    Self->tag[objectName()] = QVariant::fromValue(card->objectName());

    emit onButtonClick();
    accept();
}

QAbstractButton *YidanDialog::createButton(const QString &name)
{
    QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate(name));
    button->setObjectName(name);

    Card *c = Sanguosha->cloneCard(name);
    c->setSkillName(objectName());
    c->setParent(this);

    button->setToolTip(c->getDescription());
    map.insert(c->objectName(), c);
    group->addButton(button);

    return button;
}

class YidanVS : public OneCardViewAsSkill
{
public:
    YidanVS()
        : OneCardViewAsSkill("yidan")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !(player->hasUsed("yidanIronSlash") && player->hasUsed("yidanLightSlash"));
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        QString name = Self->tag[objectName()].toString();
        if (!name.isEmpty()) {
            Card *slash = Sanguosha->cloneCard(name);
            if (slash != nullptr) {
                slash->addSubcard(originalCard);
                slash->setSkillName(objectName());
                return slash;
            }
        }

        return nullptr;
    }
};

class Yidan : public TriggerSkill
{
public:
    Yidan()
        : TriggerSkill("yidan")
    {
        events = {PreCardUsed, CardAsked, ChoiceMade};
        view_as_skill = new YidanVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == objectName()) {
                room->addPlayerHistory(use.from, objectName() + use.card->getClassName());
                if (use.m_addHistory) {
                    room->addPlayerHistory(use.from, use.card->getClassName(), -1);
                    use.m_addHistory = false;
                    data = QVariant::fromValue(use);
                }
            }
        } else if (triggerEvent == CardAsked) {
            CardAskedStruct ask = data.value<CardAskedStruct>();
            if (ask.method == Card::MethodUse && ask.pattern == "jink" && ask.originalData.canConvert<SlashEffectStruct>()) {
                SlashEffectStruct effect = ask.originalData.value<SlashEffectStruct>();
                if (effect.slash->getSkillName() == objectName() && !effect.from->inMyAttackRange(effect.to)) {
                    room->setPlayerMark(effect.to, "yidan_jink", 1);
                    room->attachSkillToPlayer(effect.to, "yidan_othervs", true);
                }
            }
        } else if (triggerEvent == ChoiceMade) {
            ChoiceMadeStruct made = data.value<ChoiceMadeStruct>();
            if (made.type == ChoiceMadeStruct::CardResponded && made.args.first() == "jink" && made.m_extraData.canConvert<SlashEffectStruct>()) {
                SlashEffectStruct effect = made.m_extraData.value<SlashEffectStruct>();
                if (effect.slash->getSkillName() == objectName() && effect.to->getMark("yidan_jink") > 0) {
                    room->setPlayerMark(effect.to, "yidan_jink", 0);
                    room->detachSkillFromPlayer(effect.to, "yidan_othervs", true, true, false);
                }
            }
        }
    }

    QDialog *getDialog() const override
    {
        return YidanDialog::getInstance();
    }
};

class YidanTargetMod : public TargetModSkill
{
public:
    YidanTargetMod()
        : TargetModSkill("#yidanmod")
    {
        pattern = "Slash";
    }

    int getDistanceLimit(const Player *, const Card *card) const override
    {
        if (card->getSkillName() == "yidan")
            return 1000;
        else
            return 0;
    }

    int getResidueNum(const Player *, const Card *card) const override
    {
        if (card->getSkillName() == "yidan")
            return 1000;
        else
            return 0;
    }
};

class YidanProhibit : public ProhibitSkill
{
public:
    YidanProhibit()
        : ProhibitSkill("#yidanprohibit")
    {
    }

    bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &, bool) const override
    {
        if (card->getSkillName() == "yidan")
            return to->isDebuffStatus();

        return false;
    }
};

class YidanOtherVS : public OneCardViewAsSkill
{
public:
    YidanOtherVS()
        : OneCardViewAsSkill("yidan_othervs")
    {
        attached_lord_skill = true;
        filter_pattern = "BasicCard";
    }

    bool isEnabledAtPlay(const Player *) const override
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        return pattern == "jink" && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE && player->getMark("yidan_jink") > 0;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        Jink *jink = new Jink(Card::SuitToBeDecided, -1);
        jink->setSkillName("_yidan");
        jink->addSubcard(originalCard);
        return jink;
    }
};

TH15Package::TH15Package()
    : Package("th15")
{
    General *junko = new General(this, "junko$", "gzz", 4);
    junko->addSkill(new Xiahui);
    junko->addSkill(new Chunhua);
    junko->addSkill(new ChunhuaNullified);
    junko->addSkill(new ChunhuaCardEffect);
    junko->addSkill(new Shayi);
    related_skills.insertMulti("chunhua", "#chunhua");
    related_skills.insertMulti("chunhua", "#chunhua-cardeffect");

    General *hecatia = new General(this, "hecatia", "gzz", 4);
    hecatia->addSkill(new Santi);
    hecatia->addSkill(new SantiEffect);
    related_skills.insertMulti("santi", "#santi");

    General *clownpiece = new General(this, "clownpiece", "gzz", 3);
    clownpiece->addSkill(new Yuyi);
    clownpiece->addSkill(new Skill("kuangluan"));
    clownpiece->addSkill(new Kuangluan1);
    clownpiece->addSkill(new Kuangluan2);
    related_skills.insertMulti("kuangluan", "#kuangluan1");
    related_skills.insertMulti("kuangluan", "#kuangluan2");

    General *sagume = new General(this, "sagume", "gzz", 4);
    sagume->addSkill(new Shehuo);
    sagume->addSkill(new ShehuoEffect);
    sagume->addSkill(new Shenyan);
    related_skills.insertMulti("shehuo", "#shehuo");

    General *doremy = new General(this, "doremy", "gzz", 3);
    doremy->addSkill(new Bumeng);
    doremy->addSkill(new Rumeng);

    General *ringo = new General(this, "ringo", "gzz", 3);
    ringo->addSkill(new Chihou);

    General *seiran = new General(this, "seiran", "gzz", 4);
    seiran->addSkill(new Yidan);
    seiran->addSkill(new YidanTargetMod);
    seiran->addSkill(new YidanProhibit);
    related_skills.insertMulti("yidan", "#yidanmod");
    related_skills.insertMulti("yidan", "#yidanprohibit");

    skills << new ShehuoProhibit << new ShehuoTargetMod << new YidanOtherVS;
}

ADD_PACKAGE(TH15)
