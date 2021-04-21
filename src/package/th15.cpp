#include "th15.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "skill.h"
#include "testCard.h"

#include <QCommandLinkButton>
#include <QCoreApplication>
#include <QPointer>

class Xiahui : public TriggerSkill
{
public:
    Xiahui()
        : TriggerSkill("xiahui")
    {
        events << Damage << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;
        QList<ServerPlayer *> targets;
        if (e == Damage && damage.from && damage.from->isAlive() && damage.from->hasSkill(this)) {
            if (!damage.from->getCards("h").isEmpty())
                targets << damage.from;

            if (damage.to != damage.from && damage.to->isAlive() && !damage.to->getCards("h").isEmpty())
                targets << damage.to;
            if (!targets.isEmpty()) {
                damage.to->getRoom()->sortByActionOrder(targets);
                foreach (ServerPlayer *t, targets)
                    d << SkillInvokeDetail(this, damage.from, damage.from, nullptr, false, t);
            }
        }
        if (e == Damaged && damage.to && damage.to->isAlive() && damage.to->hasSkill(this)) {
            QList<ServerPlayer *> targets;
            if (!damage.to->getCards("h").isEmpty())
                targets << damage.to;
            if (damage.from && damage.to != damage.from && damage.from->isAlive() && !damage.from->getCards("h").isEmpty())
                targets << damage.from;
            if (!targets.isEmpty()) {
                damage.to->getRoom()->sortByActionOrder(targets);
                foreach (ServerPlayer *t, targets)
                    d << SkillInvokeDetail(this, damage.to, damage.to, nullptr, false, t);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "h", objectName());
        QList<int> ids;
        ids << id;
        invoke->targets.first()->addToShownHandCards(ids);
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
                Card *card = use.from->getRoomObject()->cloneCard(cardname);
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
        if (use.card == nullptr || !canShayiDamage(use) || !use.from || use.from->isDead() || use.from->getKingdom() != "gzz")
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

class Chunhua : public TriggerSkill
{
public:
    Chunhua()
        : TriggerSkill("chunhua")
    {
        events << TargetSpecified << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &) const override
    {
        if (e == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->setPlayerFlag(p, "-chunhua");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e != TargetSpecified)
            return QList<SkillInvokeDetail>();

        ServerPlayer *current = room->getCurrent();
        if (current == nullptr || !current->isInMainPhase())
            return QList<SkillInvokeDetail>();

        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.from && use.from->isAlive() && !use.to.isEmpty() && use.card->hasFlag("showncards") && (use.card->isKindOf("BasicCard") || use.card->isNDTrick())
            && ((use.card->isRed() || use.card->isBlack()) || !use.from->getShownHandcards().isEmpty())) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (!p->hasFlag("chunhua"))
                    d << SkillInvokeDetail(this, p, p);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QStringList select;
        if (use.card->isBlack())
            select << "black";
        else {
            QList<int> s = use.from->getShownHandcards();
            foreach (int id, s) {
                if (room->getCard(id)->isBlack()) {
                    select << "black";
                    break;
                }
            }
        }
        if (use.card->isRed())
            select << "red";
        else {
            QList<int> s = use.from->getShownHandcards();
            foreach (int id, s) {
                if (room->getCard(id)->isRed()) {
                    select << "red";
                    break;
                }
            }
        }

        select << "cancel";
        QString choice = room->askForChoice(invoke->invoker, objectName(), select.join("+"), data);
        invoke->tag["chunhua"] = choice;
        return choice != "cancel";
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->setPlayerFlag(invoke->invoker, "chunhua");
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, use.to)
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), p->objectName());

        QString choice = invoke->tag.value("chunhua").toString();
        room->touhouLogmessage("#InvokeSkill", invoke->invoker, objectName());
        if (!use.to.isEmpty()) {
            if (choice == "red")
                room->touhouLogmessage("$ChunhuaRed", use.from, use.card->objectName(), use.to);
            else
                room->touhouLogmessage("$ChunhuaBlack", use.from, use.card->objectName(), use.to);
        }

        room->setCardFlag(use.card, "chunhua");
        room->setCardFlag(use.card, "chunhua_" + choice);

        return false;
    }
};

//chunhua nullified
class ChunhuaEffect : public TriggerSkill
{
public:
    ChunhuaEffect()
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
    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        if (e == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            room->touhouLogmessage("#CancelChunhua", effect.to, effect.slash->objectName());
            room->setEmotion(effect.to, "skill_nullify");
            return true;
        }
        if (e == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            room->setEmotion(effect.to, "skill_nullify");
            effect.nullified = true;
            data = QVariant::fromValue(effect);
            room->touhouLogmessage("#CancelChunhua", effect.to, effect.card->objectName());
            return false;
        }
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
        room->touhouLogmessage("#TriggerSkill", player, objectName());
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
            if (player && player->hasSkill(this) && player->getPhase() == Player::Play && card && !card->isKindOf("SkillCard") && card->getHandlingMethod() == Card::MethodUse) {
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
            room->notifySkillInvoked(invoke->invoker, objectName());
            room->touhouLogmessage("#InvokeSkill", invoke->owner, objectName());
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
            if (change.from == Player::NotActive) {
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
            room->notifySkillInvoked(invoke->invoker, objectName());
            room->touhouLogmessage("#InvokeSkill", invoke->owner, objectName());

            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        if (!invoke->targets.first()->hasFlag("kuangluan_invalidity")) {
            room->setPlayerMark(invoke->targets.first(), "kuangluan_invalidity", 1);
            room->touhouLogmessage("#kuangluan_invalidity", invoke->targets.first(), "kuangluan");
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
        if (damage.card && damage.card->isKindOf("Slash") && damage.to->isAlive() && damage.to->hasSkill(this) && damage.to->canDiscard(damage.to, "hs"))
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
        if (!damage.from || damage.from->isDead() || !invoke->invoker->canDiscard(damage.from, "hs"))
            return false;

        int id = room->askForCardChosen(invoke->invoker, damage.from, "hs", objectName());
        room->throwCard(id, damage.from, invoke->invoker);
        const Card *c = invoke->tag.value("yuyi").value<const Card *>();
        if (c->getColor() != room->getCard(id)->getColor()) {
            room->touhouLogmessage("#YuyiTrigger", invoke->invoker, objectName(), QList<ServerPlayer *>(), QString::number(1));
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
            room->touhouLogmessage("$CancelTarget", use.from, use.card->objectName(), use.to);
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
        if (effect.to && effect.card && effect.card->hasFlag("shehuoTrick"))
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
        if (from->hasFlag("Global_shehuoInvokerFailed")) {
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
            if (damage.from && damage.from->isCurrent())
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
                if (p->isCardLimited(card, Card::MethodUse) || p->isProhibited(p, card) || p->isProhibited(current, card))
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
            if (!current || !current->isAlive() || current->getPhase() != Player::Play)
                return;

            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
            if (from && move.from_places.contains(Player::PlaceHand)
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
        if (!current || !current->isAlive() || current->getPhase() != Player::Play)
            return d;

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
        if (from && move.from_places.contains(Player::PlaceHand)
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
        room->touhouLogmessage("#rumeng_skip", current, "rumeng_skip", QList<ServerPlayer *>(), objectName());
        return false;
    }
};

class Emeng : public TriggerSkill
{
public:
    Emeng()
        : TriggerSkill("emeng")
    {
        events << TargetSpecified;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->hasFlag("showncards") && use.from->hasSkill(this) && use.to.length() == 1) {
            foreach (ServerPlayer *p, use.to) {
                if (!p->getShownHandcards().isEmpty())
                    d << SkillInvokeDetail(this, use.from, use.from, nullptr, true, p);
            }
        }
        return d;
    }
    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        ServerPlayer *target = invoke->targets.first();
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        int num = qMin(3, target->getShownHandcards().length());
        target->drawCards(num);
        target->turnOver();
        return false;
    }
};

YuejianCard::YuejianCard()
{
    will_throw = false;
}

bool YuejianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void YuejianCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    source->pindian(target, "yuejian");
}

class YuejianVS : public ZeroCardViewAsSkill
{
public:
    YuejianVS()
        : ZeroCardViewAsSkill("yuejian")
    {
        response_pattern = "@@yuejian!";
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        if (player->isKongcheng() || player->hasUsed("YuejianCard"))
            return false;
        return true;
    }

    const Card *viewAs(const Player *Self) const override
    {
        if (Self->getRoomObject()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            YuejianCard *c = new YuejianCard;
            return c;
        } else {
            AwaitExhausted *card = new AwaitExhausted(Card::SuitToBeDecided, -1);
            card->setSkillName(objectName());
            return card;
        }
        return nullptr;
    }
};

class Yuejian : public TriggerSkill
{
public:
    Yuejian()
        : TriggerSkill("yuejian")
    {
        events << Pindian;
        view_as_skill = new YuejianVS;
        related_pile = "dango";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == Pindian) {
            PindianStruct *pindian = data.value<PindianStruct *>();
            if (pindian->reason == objectName() && pindian->from->isAlive()) {
                if (pindian->success) {
                    AwaitExhausted *card = new AwaitExhausted(Card::SuitToBeDecided, -1);
                    card->setSkillName(objectName());
                    card->deleteLater();
                    if (pindian->from->isCardLimited(card, Card::MethodUse) || pindian->from->isProhibited(pindian->from, card))
                        return QList<SkillInvokeDetail>();
                }

                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, pindian->from, pindian->from, nullptr, true, nullptr, false);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->success) {
            room->setPlayerFlag(invoke->invoker, "yuejianSuccess");
            room->askForUseCard(invoke->invoker, "@@yuejian!", "@yuejian1");
        } else {
            invoke->invoker->addToPile("dango", pindian->from_card);
        }
        return false;
    }
};

class Jiangguo : public OneCardViewAsSkill
{
public:
    Jiangguo()
        : OneCardViewAsSkill("jiangguo")
    {
        expand_pile = "dango";
        filter_pattern = ".|.|.|dango";
        related_pile = "dango";
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return player->getPile("dango").length() > 0 && MagicAnaleptic::IsAvailable(player);
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        MagicAnaleptic *card = new MagicAnaleptic(Card::SuitToBeDecided, -1);
        DELETE_OVER_SCOPE(MagicAnaleptic, card)
        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);

        return cardPattern != nullptr && cardPattern->match(player, card) && player->getPile("dango").length() > 0;
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
    {
        MagicAnaleptic *ana = new MagicAnaleptic(Card::SuitToBeDecided, -1);
        ana->addSubcard(originalCard);
        ana->setSkillName(objectName());
        return ana;
    }
};

YidanCard::YidanCard()
{
    will_throw = false;
}

bool YidanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Card *card = Self->getRoomObject()->cloneCard("light_slash");
    DELETE_OVER_SCOPE(Card, card)
    card->addSubcards(subcards);
    card->setSkillName("yidan");
    return card && !to_select->isKongcheng() && to_select->getShownHandcards().isEmpty() && card->targetFilter(targets, to_select, Self)
        && !Self->isProhibited(to_select, card, targets);
}

const Card *YidanCard::validate(CardUseStruct &card_use) const
{
    card_use.from->showHiddenSkill("yidan");
    Card *card = card_use.from->getRoomObject()->cloneCard("light_slash");
    card->setSkillName("yidan");
    card->addSubcards(subcards);
    return card;
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
        return !player->hasUsed("YidanCard");
    }

    const Card *viewAs(const Card *originalCard, const Player * /*Self*/) const override
    {
        YidanCard *slash = new YidanCard;
        slash->addSubcard(originalCard);
        return slash;
    }
};

class Yidan : public TriggerSkill
{
public:
    Yidan()
        : TriggerSkill("yidan")
    {
        events << PreCardUsed;
        view_as_skill = new YidanVS;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getSkillName() == objectName()) {
            //for AI
            //room->setPlayerFlag(use.from, "yidan_" + use.card->objectName());

            if (use.m_addHistory) {
                room->addPlayerHistory(use.from, use.card->getClassName(), -1);
                use.m_addHistory = false;
                data = QVariant::fromValue(use);
            }
        }
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

class Xuechu : public TriggerSkill
{
public:
    Xuechu()
        : TriggerSkill("xuechu")
    {
        events << TargetSpecified << Damage;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.from->hasSkill(this) && use.card != nullptr && use.card->isKindOf("Slash") && !use.to.isEmpty())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, nullptr, true);

        } else if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive() && damage.from->hasSkill(this) && damage.to->isAlive() && damage.to != damage.from)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, true);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (e == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            QVariantList jink_list = invoke->invoker->tag["Jink_" + use.card->toString()].toList();

            foreach (ServerPlayer *target, use.to) {
                int index = use.to.indexOf(target);
                LogMessage log;
                log.type = "#NoJink";
                log.from = target;
                room->sendLog(log);
                jink_list[index] = 0;
            }

            invoke->invoker->tag["Jink_" + use.card->toString()] = jink_list;
        } else if (e == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            const Card *card = room->askForCard(damage.to, "Jink", "@xuechu", data, objectName());
            if (card != nullptr)
                room->recover(damage.to, RecoverStruct());
        }

        return false;
    }
};

TH15Package::TH15Package()
    : Package("th15")
{
    General *junko = new General(this, "junko$", "gzz", 4);
    junko->addSkill(new Xiahui);
    junko->addSkill(new Chunhua);
    junko->addSkill(new ChunhuaEffect);
    junko->addSkill(new Shayi);
    related_skills.insert("chunhua", "#chunhua");

    General *hecatia = new General(this, "hecatia", "gzz", 4);
    hecatia->addSkill(new Santi);
    hecatia->addSkill(new SantiEffect);
    related_skills.insert("santi", "#santi");

    General *clownpiece = new General(this, "clownpiece", "gzz", 3);
    clownpiece->addSkill(new Yuyi);
    clownpiece->addSkill(new Skill("kuangluan"));
    clownpiece->addSkill(new Kuangluan1);
    clownpiece->addSkill(new Kuangluan2);
    related_skills.insert("kuangluan", "#kuangluan1");
    related_skills.insert("kuangluan", "#kuangluan2");

    General *sagume = new General(this, "sagume", "gzz", 4);
    sagume->addSkill(new Shehuo);
    sagume->addSkill(new ShehuoEffect);
    sagume->addSkill(new Shenyan);
    related_skills.insert("shehuo", "#shehuo");

    General *doremy = new General(this, "doremy", "gzz", 3);
    doremy->addSkill(new Bumeng);
    doremy->addSkill(new Rumeng);

    General *ringo = new General(this, "ringo", "gzz", 3);
    ringo->addSkill(new Yuejian);
    ringo->addSkill(new Jiangguo);

    General *seiran = new General(this, "seiran", "gzz", 4);
    seiran->addSkill(new Yidan);
    seiran->addSkill(new YidanTargetMod);
    related_skills.insert("yidan", "#yidanmod");
    seiran->addSkill(new Xuechu);

    addMetaObject<YuejianCard>();
    addMetaObject<YidanCard>();
    skills << new ShehuoProhibit << new ShehuoTargetMod;
}

ADD_PACKAGE(TH15)
