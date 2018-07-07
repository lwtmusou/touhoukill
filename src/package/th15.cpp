#include "th15.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "skill.h"
#include "testCard.h"

class Xiahui : public TriggerSkill
{
public:
    Xiahui()
        : TriggerSkill("xiahui")
    {
        events << Damage << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
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
                    d << SkillInvokeDetail(this, damage.from, damage.from, NULL, false, t);
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
                    d << SkillInvokeDetail(this, damage.to, damage.to, NULL, false, t);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "h", objectName());
        QList<int> ids;
        ids << id;
        invoke->targets.first()->addToShownHandCards(ids);
        return false;
    }
};

/*
class Chunhua : public TriggerSkill
{
public:
    Chunhua()
        : TriggerSkill("chunhua")
    {
        events << TargetConfirmed << TargetSpecified;
    }
    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeBasic || use.card->isNDTrick()) {
            if (e == TargetSpecified && use.from && use.from->isAlive() && use.from->hasSkill(this) && !use.from->hasFlag("chunhua_used"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
            if (e == TargetConfirmed) {
                QList<SkillInvokeDetail> d;
                foreach (ServerPlayer *p, use.to) {
                    if (p->isAlive() && p->hasSkill(this) && !p->hasFlag("chunhua_used"))
                        d << SkillInvokeDetail(this, p, p);
                }
                return d;
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        room->setPlayerFlag(invoke->invoker, "chunhua_used");
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), p->objectName());
            foreach (const Skill *skill, p->getVisibleSkillList()) {
                if (skill->getFrequency() != Skill::Compulsory)
                    continue;
                room->setPlayerSkillInvalidity(p, skill, true);
            }
            p->tag["chunhua"] = QVariant::fromValue(use.card->objectName());
            room->filterCards(p, p->getCards("hes"), true);
        }
        return false;
    }
};
*/
/*
class ChunhuaEffect : public TriggerSkill
{
public:
    ChunhuaEffect()
        : TriggerSkill("#chunhua")
    {
        events << EventPhaseChanging << ShownCardChanged << Dying << GameStart << Debut << EventAcquireSkill;
    }

    static void clearChunhua(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        foreach (const Skill *skill, player->getVisibleSkillList()) {
            if (skill->getFrequency() != Skill::Compulsory)
                continue;
            room->setPlayerSkillInvalidity(player, skill, false);
        }
        player->tag.remove("chunhua");
        room->filterCards(player, player->getCards("hes"), true);
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == GameStart || e == Debut || e == EventAcquireSkill) {
            QList<ServerPlayer *> junkos;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill("chunhua", false))
                    junkos << p;
            }
            if (junkos.isEmpty())
                return;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (!p->hasSkill("#chunhua-filter"))
                    room->acquireSkill(p, "#chunhua-filter", false);
            }
        }

        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers(true)) {
                    room->setPlayerFlag(p, "-chunhua_used");
                    clearChunhua(p);
                }
            }
        }
        if (e == Dying) {
            foreach (ServerPlayer *p, room->getAllPlayers(true))
                clearChunhua(p);
        }
        if (e == ShownCardChanged) {
            ShownCardChangedStruct s = data.value<ShownCardChangedStruct>();
            room->filterCards(s.player, s.player->getCards("hes"), true);
        }
    }
};
*/
/*
class ChunhuaFilter : public FilterSkill

{
public:
    ChunhuaFilter()
        : FilterSkill("#chunhua-filter")
    {
    }

    bool viewFilter(const Card *to_select) const
    {
        Room *room = Sanguosha->currentRoom();
        ServerPlayer *player = room->getCardOwner(to_select->getId());
        if (player != NULL) {
            QString name = player->tag["chunhua"].toString();
            return !name.isEmpty() && player->isShownHandcard(to_select->getId());
        }
        return false;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Room *room = Sanguosha->currentRoom();
        ServerPlayer *player = room->getCardOwner(originalCard->getId());
        if (player != NULL) {
            QString name = player->tag["chunhua"].toString();
            Card *new_card = Sanguosha->cloneCard(name, originalCard->getSuit(), originalCard->getNumber());
            new_card->setSkillName("chunhua");
            WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
            card->takeOver(new_card);
            //new_card->setModified(true);
            return card;
        }
        return originalCard;
    }
};*/

/*
class Shayi : public TriggerSkill
{
public:
    Shayi()
        : TriggerSkill("shayi$")
    {
        events << CardUsed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card == NULL || !use.card->canDamage() || !use.from || use.from->isDead() || use.from->getKingdom() != "gzz")
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->getOtherPlayers(use.from)) {
            if (p->hasLordSkill(this))
                d << SkillInvokeDetail(this, p, use.from);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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
};*/

class Chunhua : public TriggerSkill
{
public:
    Chunhua()
        : TriggerSkill("chunhua")
    {
        events << TargetSpecified << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &) const
    {
        if (e == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAllPlayers())
                room->setPlayerFlag(p, "-chunhua");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e != TargetSpecified)
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QStringList select;
        if (use.card->isBlack())
            select << "black";
        else {
            QList<int> s = use.from->getShownHandcards();
            foreach (int id, s) {
                if (Sanguosha->getCard(id)->isBlack()) {
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
                if (Sanguosha->getCard(id)->isRed()) {
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (e == SlashEffected) {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("chunhua")) {
                if (effect.slash->hasFlag("chunhua_red") && !effect.to->isWounded())
                    d << SkillInvokeDetail(this, effect.to, effect.to, NULL, true);
            }
        }
        if (e == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->hasFlag("chunhua") && !effect.card->isKindOf("Slash")) {
                if (effect.card->hasFlag("chunhua_red") && !effect.to->isWounded())
                    d << SkillInvokeDetail(this, effect.to, effect.to, NULL, true);
            }
        }
        return d;
    }
    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
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

ShayiCard::ShayiCard()
{
    target_fixed = true;
}

bool ShayiCard::putToPile(Room *room, ServerPlayer *lord)
{
    QList<int> discardpile = room->getDiscardPile();
    QList<int> ids;
    QVariantList shayi_ids = lord->tag["shayi"].toList();

    foreach (QVariant card_data, shayi_ids) {
        if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
            ids << card_data.toInt();
    }

    if (ids.isEmpty())
        return false;

    CardsMoveStruct move;
    move.from_place = Player::DiscardPile;
    move.to = lord;
    move.to_player_name = lord->objectName();
    move.to_pile_name = "#shayi_temp";
    move.card_ids = ids;
    move.to_place = Player::PlaceSpecial;
    move.open = true;

    QList<CardsMoveStruct> _moves = QList<CardsMoveStruct>() << move;
    QList<ServerPlayer *> _lord = QList<ServerPlayer *>() << lord;
    room->setPlayerFlag(lord, "shayi_InTempMoving");
    room->notifyMoveCards(true, _moves, true, _lord);
    room->notifyMoveCards(false, _moves, true, _lord);
    room->setPlayerFlag(lord, "-shayi_InTempMoving");

    QVariantList tag = IntList2VariantList(ids);
    lord->tag["shayi_tempcards"] = tag;
    return true;
}

void ShayiCard::cleanUp(Room *room, ServerPlayer *lord)
{
    QList<int> equips = VariantList2IntList(lord->tag.value("shayi_tempcards", QVariantList()).toList());
    lord->tag.remove("shayi_tempcards");
    if (equips.isEmpty())
        return;

    CardsMoveStruct move;
    move.from = lord;
    move.from_player_name = lord->objectName();
    move.from_place = Player::PlaceSpecial;
    move.from_pile_name = "#shayi_temp";
    move.to_place = Player::DiscardPile;
    move.open = true;
    move.card_ids = equips;

    QList<CardsMoveStruct> _moves = QList<CardsMoveStruct>() << move;
    QList<ServerPlayer *> _lord = QList<ServerPlayer *>() << lord;
    room->setPlayerFlag(lord, "shayi_InTempMoving");
    room->notifyMoveCards(true, _moves, true, _lord);
    room->notifyMoveCards(false, _moves, true, _lord);
    room->setPlayerFlag(lord, "-shayi_InTempMoving");
}

void ShayiCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *lord = card_use.from;
    if (!putToPile(room, lord))
        return;

    room->askForUseCard(lord, "@@shayiuse", "@shayi-use", -1, Card::MethodUse, false, "shayi");
    cleanUp(room, lord);
    return;
}

class ShayiUse : public OneCardViewAsSkill
{
public:
    ShayiUse()
        : OneCardViewAsSkill("shayiuse")
    {
        response_pattern = "@@shayiuse";
        expand_pile = "#shayi_temp";
    }

    bool viewFilter(const Card *to_select) const
    {
        return Self->getPile("#shayi_temp").contains(to_select->getId());
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};

class ShayiVS : public ZeroCardViewAsSkill
{
public:
    ShayiVS()
        : ZeroCardViewAsSkill("shayi")
    {
        response_pattern = "@@shayi";
    }

    const Card *viewAs() const
    {
        return new ShayiCard;
    }
};

class Shayi : public TriggerSkill
{
public:
    Shayi()
        : TriggerSkill("shayi$")
    {
        events << PreCardUsed << EventPhaseEnd << CardsMoveOneTime << EventPhaseChanging;
        view_as_skill = new ShayiVS;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == PreCardUsed) { // just for UI and log
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return;
            QList<int> ids;
            QVariantList shayi_ids = use.from->tag["shayi"].toList();

            foreach (QVariant card_data, shayi_ids) {
                if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
                    ids << card_data.toInt();
            }
            int id = use.card->getEffectiveId();
            if (id > -1 && ids.contains(id)) {
                room->touhouLogmessage("#InvokeSkill", use.from, objectName());
                room->notifySkillInvoked(use.from, objectName());
            }
        }

        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Discard) {
                foreach (ServerPlayer *lord, room->getAllPlayers())
                    lord->tag.remove("shayi");
            }
        }

        if (e != CardsMoveOneTime)
            return;

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!move.from || move.from->getPhase() != Player::Discard)
            return;
        if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            foreach (ServerPlayer *lord, room->getAllPlayers()) {
                if (lord->hasLordSkill(objectName()) && lord != move.from) {
                    QVariantList ShayiToGet = lord->tag["shayi"].toList();
                    foreach (int card_id, move.card_ids) {
                        if (!ShayiToGet.contains(card_id) && Sanguosha->getCard(card_id)->isKindOf("Slash"))
                            ShayiToGet << card_id;
                    }
                    lord->tag["shayi"] = ShayiToGet;
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == EventPhaseEnd) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current->getPhase() == Player::Discard && current->getKingdom() == "gzz") {
                QList<SkillInvokeDetail> d;

                foreach (ServerPlayer *lord, room->getAllPlayers()) {
                    if (lord->isCurrent() || !lord->hasLordSkill(objectName()))
                        continue;
                    QVariantList ids = lord->tag["shayi"].toList();
                    QList<int> get_ids;
                    foreach (QVariant card_data, ids) {
                        if (room->getCardPlace(card_data.toInt()) == Player::DiscardPile)
                            get_ids << card_data.toInt();
                    }
                    if (get_ids.length() == 0) {
                        lord->tag.remove("shayi");
                        continue;
                    }
                    d << SkillInvokeDetail(this, lord, lord, NULL, false, current);
                }
                return d;
            }
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->tag["shayi_target"] = QVariant::fromValue(invoke->preferredTarget);
        QString prompt = "target:" + invoke->preferredTarget->objectName();
        room->askForUseCard(invoke->invoker, "@@shayi", "@shayi");
        invoke->invoker->tag.remove("shayi");
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            QVariant extraTag = room->getTag("touhou-extra");
            bool nowExtraTurn = extraTag.canConvert(QVariant::Bool) && extraTag.toBool();

            ServerPlayer *player = data.value<ServerPlayer *>();
            if (!nowExtraTurn && player->hasSkill(this) && player->getPhase() == Player::RoundStart)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
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
        events << EventPhaseStart << EventPhaseChanging << EventAcquireSkill << EventLoseSkill << EventSkillInvalidityChange;
    }

    static void removeSantiLimit(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        if (player->isCardLimited("use", "santi_limit_1"))
            room->removePlayerCardLimitation(player, "use", "TrickCard,EquipCard$1", "santi_limit_1");

        if (player->isCardLimited("use", "santi_limit_2"))
            room->removePlayerCardLimitation(player, "use", "TrickCard,BasicCard$1", "santi_limit_2");

        if (player->isCardLimited("use", "santi_limit_3"))
            room->removePlayerCardLimitation(player, "use", "EquipCard,BasicCard$1", "santi_limit_3");
    }

    static void setSantiLimit(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        removeSantiLimit(player);
        if (player->getMark("santi") == 1)
            room->setPlayerCardLimitation(player, "use", "TrickCard,EquipCard", "santi_limit_1", true);
        else if (player->getMark("santi") == 2)
            room->setPlayerCardLimitation(player, "use", "TrickCard,BasicCard", "santi_limit_2", true);
        else if (player->getMark("santi") == 3)
            room->setPlayerCardLimitation(player, "use", "EquipCard,BasicCard", "santi_limit_3", true);
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        //mark record at first
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *current = room->getCurrent();
            if (current->getPhase() == Player::Play)
                room->setPlayerMark(current, "santi", current->getMark("santi") + 1);
        }
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(change.player, "santi", 0);
        }

        //set or remove limitation
        if (triggerEvent == EventLoseSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            if (a.skill->objectName() == "santi")
                removeSantiLimit(a.player);
        } else if (triggerEvent == EventAcquireSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            if (a.skill->objectName() == "santi")
                setSantiLimit(a.player);
        } else if (triggerEvent == EventSkillInvalidityChange) {
            QList<SkillInvalidStruct> invalids = data.value<QList<SkillInvalidStruct> >();
            foreach (SkillInvalidStruct v, invalids) {
                if (!v.skill || v.skill->objectName() == "santi") {
                    if (!v.invalid && v.player->hasSkill(this))
                        setSantiLimit(v.player);
                    else if (v.invalid)
                        removeSantiLimit(v.player);
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                removeSantiLimit(change.player);
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *current = room->getCurrent();
            if (current->getPhase() == Player::Play && current->hasSkill("santi", false, false))
                setSantiLimit(current);
        }
    }
};

/*
class Yuyi : public TriggerSkill
{
public:
    Yuyi()
        : TriggerSkill("yuyi")
    {
        events << TargetSpecified << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        //record times of using card
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.from->getPhase() == Player::Play && (use.card->isNDTrick() || use.card->isKindOf("Slash"))) {
                if (use.from->hasFlag("yuyi_first"))
                    room->setPlayerFlag(use.from, "yuyi_second");
                else
                    room->setPlayerFlag(use.from, "yuyi_first");
            }
        }

        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                change.player->setFlags("-yuyi_first");
                change.player->setFlags("-yuyi_second");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.from->getPhase() == Player::Play && (use.card->isNDTrick() || use.card->isKindOf("Slash"))) {
                if (use.from->hasFlag("yuyi_first") && !use.from->hasFlag("yuyi_second")) {
                    QList<SkillInvokeDetail> d;

                    foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                        if (!p->getCards("h").isEmpty() || !use.from->getCards("h").isEmpty())
                            d << SkillInvokeDetail(this, p, p, NULL, false, use.from);
                    }
                    return d;
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QStringList select;
        if (!invoke->invoker->getCards("h").isEmpty())
            select << "changeProcess";
        if (!invoke->preferredTarget->getCards("h").isEmpty())
            select << "show";
        select.prepend("cancel");
        QString choice = room->askForChoice(invoke->invoker, objectName(), select.join("+"), data);
        if (choice == "cancel")
            return false;
        invoke->tag["yuyi"] = choice;
        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = invoke->invoker;
        log.arg = objectName();
        room->sendLog(log);
        room->notifySkillInvoked(invoke->invoker, objectName());
        return true;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QString choice = invoke->tag.value("yuyi").toString();
        ServerPlayer *target = (choice == "changeProcess") ? invoke->invoker : invoke->targets.first();
        int id = room->askForCardChosen(invoke->invoker, target, "h", objectName());
        QList<int> ids;
        ids << id;
        target->addToShownHandCards(ids);
        if (choice == "changeProcess") {
            CardUseStruct use = data.value<CardUseStruct>();
            LogMessage log;
            log.type = "#YuyiChanged";
            log.from = invoke->invoker;
            log.to = use.to;
            log.arg = use.card->objectName();
            log.arg2 = objectName();
            room->sendLog(log);

            room->setCardFlag(use.card, "yuyiProcess"); //gamerule process true effect
        }
        return false;
    }
};

class YuyiEffect : public TriggerSkill
{
public:
    YuyiEffect()
        : TriggerSkill("#yuyi")
    {
        events << Cancel;
        global = true; //it is game rule. not general skill
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        if (data.canConvert<CardEffectStruct>()) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (!effect.card->hasFlag("yuyiProcess"))
                return QList<SkillInvokeDetail>();

            if (effect.to->isAlive() && effect.from->isAlive() && effect.from != effect.to && !effect.to->isKongcheng() && !effect.from->isKongcheng())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, NULL, false, effect.from);
        } else {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (!effect.slash->hasFlag("yuyiProcess"))
                return QList<SkillInvokeDetail>();
            if (effect.to->isAlive() && effect.from->isAlive() && effect.from != effect.to && !effect.to->isKongcheng() && !effect.from->isKongcheng())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, NULL, false, effect.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        const Card *card = NULL;
        if (data.canConvert<CardEffectStruct>()) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            card = effect.card;
        } else {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            card = effect.slash;
        }
        QString prompt = "cancel:" + invoke->preferredTarget->objectName() + ":" + card->objectName();
        return invoke->invoker->askForSkillInvoke("yuyiProcess", prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        const Card *card = NULL;
        if (data.canConvert<CardEffectStruct>()) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            card = effect.card;
        } else {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            card = effect.slash;
        }
        if (invoke->invoker->pindian(invoke->targets.first(), "yuyiProcess")) {
            LogMessage log;
            log.type = "#YuyiCancel";
            log.from = invoke->targets.first();
            log.to << invoke->invoker;
            log.arg = card->objectName();
            log.arg2 = "yuyi";
            room->sendLog(log);

            if (data.canConvert<CardEffectStruct>()) {
                CardEffectStruct effect = data.value<CardEffectStruct>();
                effect.canceled = true;
                data = QVariant::fromValue(effect);
            } else {
                SlashEffectStruct effect = data.value<SlashEffectStruct>();
                effect.canceled = true;
                data = QVariant::fromValue(effect);
            }
        }
        return true;
    }
};

class Kuangluan : public TriggerSkill
{
public:
    Kuangluan()
        : TriggerSkill("kuangluan")
    {
        events << PindianAsked << CardsMoveOneTime;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == PindianAsked) {
            PindianStruct *pindian = data.value<PindianStruct *>();
            ServerPlayer *target = pindian->askedPlayer;
            ServerPlayer *piece = (target == pindian->from) ? pindian->to : pindian->from;
            if (!piece->hasSkill(this))
                return QList<SkillInvokeDetail>();
            if (target == pindian->to && pindian->to_card == NULL)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, piece, piece, NULL, false, target);
            if (target == pindian->from && pindian->from_card == NULL)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, piece, piece, NULL, false, target);

        } else if (e == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!move.from_places.contains(Player::PlaceHand))
                return QList<SkillInvokeDetail>();
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_PINDIAN)
                return QList<SkillInvokeDetail>();
            bool can = false;
            for (int i = 0; i < move.card_ids.size(); i++) {
                if (move.from_places[i] == Player::PlaceHand && move.shown_ids.contains(move.card_ids[i])) {
                    can = true;
                    break;
                }
            }
            if (!can)
                return QList<SkillInvokeDetail>();
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *piece, room->findPlayersBySkillName(objectName())) {
                d << SkillInvokeDetail(this, piece, piece);
            }
            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == CardsMoveOneTime) {
            invoke->invoker->drawCards(1);
        } else if (e == PindianAsked) {
            PindianStruct *pindian = data.value<PindianStruct *>();

            int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hs", objectName());
            const Card *c = Sanguosha->getCard(id);
            if (invoke->targets.first() == pindian->to)
                pindian->to_card = c;
            else
                pindian->from_card = c;
            data = QVariant::fromValue(pindian);
        }

        return false;
    }
};

*/

class Kuangluan : public TriggerSkill
{
public:
    Kuangluan()
        : TriggerSkill("kuangluan")
    {
        events << EventPhaseChanging;
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        if (room->getTag("FirstRound").toBool())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *playerTo = qobject_cast<ServerPlayer *>(move.to);
        if (playerTo != NULL && playerTo->isAlive() && move.to_place == Player::PlaceHand && playerTo->getPhase() != Player::Draw && playerTo->getShownHandcards().isEmpty()) {
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
                    d << SkillInvokeDetail(this, p, p, NULL, false, playerTo);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (invoke->invoker->askForSkillInvoke("kuangluan1", QVariant::fromValue(invoke->preferredTarget))) {
            room->notifySkillInvoked(invoke->invoker, objectName());
            room->touhouLogmessage("#InvokeSkill", invoke->owner, objectName());
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive || change.from == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark("kuangluan_invalidity") > 0) {
                        room->setPlayerMark(p, "kuangluan_invalidity", 0);
                        room->setPlayerSkillInvalidity(p, NULL, false);
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent != CardsMoveOneTime)
            return d;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *playerFrom = qobject_cast<ServerPlayer *>(move.from);
        if (playerFrom != NULL && playerFrom->isAlive() && !move.shown_ids.isEmpty() && playerFrom->getShownHandcards().isEmpty()
            && playerFrom->getMark("kuangluan_invalidity") == 0) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p != playerFrom)
                    d << SkillInvokeDetail(this, p, p, NULL, false, playerFrom);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (invoke->invoker->askForSkillInvoke("kuangluan2", QVariant::fromValue(invoke->preferredTarget))) {
            room->notifySkillInvoked(invoke->invoker, objectName());
            room->touhouLogmessage("#InvokeSkill", invoke->owner, objectName());

            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        if (!invoke->targets.first()->hasFlag("kuangluan_invalidity")) {
            //room->setPlayerFlag(invoke->targets.first(), "kuangluan_invalidity");
            room->setPlayerMark(invoke->targets.first(), "kuangluan_invalidity", 1);
            room->touhouLogmessage("#kuangluan_invalidity", invoke->targets.first(), "kuangluan");
            room->setPlayerSkillInvalidity(invoke->targets.first(), NULL, true);
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && damage.to->isAlive() && damage.to->hasSkill(this) && damage.to->canDiscard(damage.to, "hs"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        const Card *card = room->askForCard(invoke->invoker, ".|.|.|hand", "@yuyi_discard", data, Card::MethodDiscard, NULL, false, objectName());
        invoke->tag["yuyi"] = QVariant::fromValue(card);
        return card != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.from || damage.from->isDead() || !invoke->invoker->canDiscard(damage.from, "hs"))
            return false;

        int id = room->askForCardChosen(invoke->invoker, damage.from, "hs", objectName());
        room->throwCard(id, damage.from, invoke->invoker);
        const Card *c = invoke->tag.value("yuyi").value<const Card *>();
        if (c->getColor() != Sanguosha->getCard(id)->getColor()) {
            room->touhouLogmessage("#YuyiTrigger", invoke->invoker, objectName(), QList<ServerPlayer *>(), QString::number(1));
            damage.damage = damage.damage - 1;
            data = QVariant::fromValue(damage);
            if (damage.damage <= 0)
                return true;
        }

        return false;
    }
};

/*
class Yuyi : public TriggerSkill
{
public:
    Yuyi()
        : TriggerSkill("yuyi")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("yuyi_invalidity")) {
                        room->setPlayerFlag(p, "-yuyi_invalidity");
                        room->setPlayerSkillInvalidity(p, NULL, false);
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e != EventPhaseStart)
            return QList<SkillInvokeDetail>();
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->isDead() || current->getPhase() != Player::Play)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        if (current->hasSkill(this)) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!p->getShownHandcards().isEmpty()) {
                    d << SkillInvokeDetail(this, current, current);
                    break;
                }
            }
        }
        if (current->getShownHandcards().isEmpty()) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                if (p != current)
                    d << SkillInvokeDetail(this, p, p);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        return invoke->invoker->askForSkillInvoke(this);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (invoke->invoker->isCurrent()) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!p->getShownHandcards().isEmpty())
                    targets << p;
            }
            room->sortByActionOrder(targets);
            foreach (ServerPlayer *t, targets)
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), t->objectName());
            foreach (ServerPlayer *t, targets)
                t->drawCards(1);

            foreach (ServerPlayer *t, targets) {
                if (!t->hasFlag("yuyi_invalidity")) {
                    room->setPlayerFlag(t, "yuyi_invalidity");
                    room->setPlayerSkillInvalidity(t, NULL, true);
                }
            }
        } else {
            ServerPlayer *current = data.value<ServerPlayer *>();
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), current->objectName());
            current->drawCards(1);
            if (!current->hasFlag("yuyi_invalidity")) {
                room->setPlayerFlag(current, "yuyi_invalidity");
                room->setPlayerSkillInvalidity(current, NULL, true);
            }
        }
        return false;
    }
};*/

class Shehuo : public TriggerSkill
{
public:
    Shehuo()
        : TriggerSkill("shehuo")
    {
        events << TargetSpecifying << TargetConfirming;
    }

    //for  SkillInvalidity
    /*void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.from || !use.from->hasFlag("shehuoInvoker") || use.card->isKindOf("SkillCard"))
                return;
            room->setPlayerFlag(use.from, "-shehuoInvoker");

            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasFlag("shehuoOwner")) {
                    room->setPlayerFlag(p, "-shehuoOwner");
                    room->setPlayerFlag(p, "shehuo_invalidity");
                    room->setPlayerSkillInvalidity(p, "shehuo", true);
                }
            }
        }
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("shehuo_invalidity")) {
                        room->setPlayerFlag(p, "-shehuo_invalidity");
                        room->setPlayerSkillInvalidity(p, "shehuo", false);
                    }
                }
            }
        }
    }*/

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.to.length() != 1 || use.from == NULL || use.from == use.to.first())
            return QList<SkillInvokeDetail>();
        if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
            if (e == TargetSpecifying && use.from->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
            if (e == TargetConfirming && use.to.first()->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.to.first(), use.to.first());
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "target:" + use.from->objectName() + ":" + use.to.first()->objectName() + ":" + use.card->objectName();
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
    {
        //room->touhouLogmessage("#TriggerSkill", invoke->owner, objectName());
        //room->notifySkillInvoked(invoke->owner, objectName());
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *target = use.from;
        ServerPlayer *player = use.to.first();

        room->setPlayerFlag(target, "Global_shehuoFailed"); //for targetfilter
        room->setPlayerFlag(player, "Global_shehuoInvokerFailed"); //for targetfilter
        //room->setPlayerFlag(invoke->owner, "shehuoOwner"); // for record
        //room->setPlayerFlag(invoke->invoker, "shehuoInvoker"); // for record
        //const Card *c = room->askForUseCard(invoke->invoker, "@@shehuo", "@shehuo");

        QString prompt = "@shehuo_use:" + target->objectName() + ":" + use.card->objectName();
        QString pattern;

        if (use.card->isNDTrick())
            pattern = "BasicCard+^Jink,EquipCard|.|.|shehuo";
        else
            pattern = "TrickCard+^Nullification,EquipCard|.|.|shehuo";

        //for ai
        player->tag["shehuo_target"] = QVariant::fromValue(target);
        const Card *c = room->askForUseCard(player, pattern, prompt, -1, Card::MethodUse, false, objectName());
        //room->setPlayerFlag(invoke->owner, "-shehuoOwner"); // for record
        //room->setPlayerFlag(invoke->invoker, "-shehuoInvoker"); // for record

        if (c != NULL) {
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.to && effect.card && effect.card->hasFlag("shehuoTrick"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &) const
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

    virtual bool isProhibited(const Player *from, const Player *to, const Card *, const QList<const Player *> &targets, bool) const
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

    virtual int getDistanceLimit(const Player *from, const Card *) const
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
        events << EventPhaseStart << EventPhaseChanging << TargetSpecified;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();

            if (use.from && use.from->isCurrent() && use.card && !use.card->isKindOf("SkillCard")) {
                foreach (ServerPlayer *p, use.to) {
                    if (p != use.from) {
                        room->setPlayerFlag(use.from, "shenyan_used");
                        break;
                    }
                }
            }
        }

        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerFlag(p, "-shenyan_used");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
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

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->setPlayerFlag(p, "-bumeng");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e != CardsMoveOneTime)
            return QList<SkillInvokeDetail>();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
            ServerPlayer *thrower = room->findPlayerByObjectName(move.reason.m_playerId);
            if (player != NULL && player->isAlive() && move.to_place == Player::DiscardPile && thrower != NULL && player == thrower) {
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
                        d << SkillInvokeDetail(this, p, p, NULL, false, player);
                }
                return d;
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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

    void record(TriggerEvent e, Room *room, QVariant &data) const
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
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
                        d << SkillInvokeDetail(this, p, p, NULL, false, from);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        bool can = invoke->invoker->askForSkillInvoke(this, data);
        //clear flag
        room->setPlayerFlag(invoke->preferredTarget, "-rumeng");
        return can;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &) const
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->hasFlag("showncards") && use.from->hasSkill(this) && use.to.length() == 1) {
            foreach (ServerPlayer *p, use.to) {
                if (!p->getShownHandcards().isEmpty())
                    d << SkillInvokeDetail(this, use.from, use.from, NULL, true, p);
            }
        }
        return d;
    }
    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
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

class YuejianVS : public ZeroCardViewAsSkill
{
public:
    YuejianVS()
        : ZeroCardViewAsSkill("yuejian")
    {
        response_pattern = "@@yuejian";
    }

    virtual const Card *viewAs() const
    {
        if (!Self->hasFlag("Global_yuejianFailed")) {
            KnownBoth *card = new KnownBoth(Card::SuitToBeDecided, -1);
            card->setSkillName(objectName());
            return card;
        } else {
            AwaitExhausted *card = new AwaitExhausted(Card::SuitToBeDecided, -1);
            card->setSkillName(objectName());
            return card;
        }

        return NULL;
    }
};

class Yuejian : public TriggerSkill
{
public:
    Yuejian()
        : TriggerSkill("yuejian")
    {
        events << CardFinished << Pindian;
        view_as_skill = new YuejianVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("SkillCard") && !use.card->isVirtualCard() && use.from && use.from->hasSkill(this) && !use.to.isEmpty() && use.to.contains(use.from)
                && !use.from->isKongcheng()) {
                foreach (ServerPlayer *t, room->getOtherPlayers(use.from)) {
                    if (!t->isKongcheng())
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
                }
            }
        } else if (triggerEvent == Pindian) {
            PindianStruct *pindian = data.value<PindianStruct *>();
            if (pindian->reason == objectName())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, pindian->from, pindian->from, NULL, true, NULL, false);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent event, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (event == CardFinished) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *t, room->getOtherPlayers(invoke->invoker)) {
                if (!t->isKongcheng())
                    targets << t;
            }
            ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@yuejian-pindian", true, true);
            if (target)
                invoke->targets << target;
            return target != NULL;
        } else if (event == Pindian) {
            PindianStruct *pindian = data.value<PindianStruct *>();
            if (pindian->success)
                room->askForUseCard(invoke->invoker, "@@yuejian", "@yuejian1");
            else {
                room->setPlayerFlag(invoke->invoker, "Global_yuejianFailed");
                room->askForUseCard(invoke->invoker, "@@yuejian", "@yuejian2");
            }
        }

        return false;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->pindian(invoke->targets.first(), objectName());
        return false;
    }
};

YidanCard::YidanCard()
{
    will_throw = false;
}

bool YidanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *card = new Slash(Card::SuitToBeDecided, 0);
    card->addSubcards(subcards);
    card->setSkillName("yidan");
    card->deleteLater();

    int slash_targets = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, card);
    if (targets.length() >= slash_targets)
        return false;

    /*bool has_specific_assignee = false;
    foreach(const Player *p, Self->getAliveSiblings()) {
        if (Slash::IsSpecificAssignee(p, Self, this)) {
            has_specific_assignee = true;
            break;
        }
    }*/
    bool can = false;
    if (!Self->isProhibited(to_select, card, targets)) {
        foreach (const Card *c, to_select->getEquips()) {
            if (card->getSuit() == c->getSuit()) {
                can = true;
                break;
            }
        }
        if (!can) {
            foreach (int id, to_select->getShownHandcards()) {
                if (card->getSuit() == Sanguosha->getCard(id)->getSuit()) {
                    can = true;
                    break;
                }
            }
        }
        return can && card->targetFilter(targets, to_select, Self); //&& !targets.isEmpty()
    }
    return false;
}

bool YidanCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Slash *card = new Slash(Card::SuitToBeDecided, 0);
    card->addSubcards(subcards);
    card->setSkillName("yidan");
    card->deleteLater();

    bool yidan = false;
    foreach (const Player *p, targets) {
        foreach (const Card *c, p->getEquips()) {
            if (card->getSuit() == c->getSuit()) {
                yidan = true;
                break;
            }
        }
        if (yidan)
            break;
        foreach (int id, p->getShownHandcards()) {
            if (card->getSuit() == Sanguosha->getCard(id)->getSuit()) {
                yidan = true;
                break;
            }
        }
        if (yidan)
            break;
    }
    return yidan && card->targetsFeasible(targets, Self);
}

const Card *YidanCard::validate(CardUseStruct &) const
{
    Slash *card = new Slash(Card::SuitToBeDecided, 0);
    card->addSubcards(subcards);
    card->setSkillName("yidan");
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

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return true;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            return false;
        return matchAvaliablePattern("slash", pattern);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            YidanCard *slash = new YidanCard;
            slash->addSubcard(originalCard);
            return slash;
        }
        return NULL;
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == objectName()) {
                if (use.m_addHistory) {
                    room->addPlayerHistory(use.from, use.card->getClassName(), -1);
                    use.m_addHistory = false;
                    data = QVariant::fromValue(use);
                }
            }
        }
    }
};

class YidanProhibit : public ProhibitSkill
{
public:
    YidanProhibit()
        : ProhibitSkill("#yidan")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &, bool include_hidden) const
    {
        if (to->hasSkill("yidan", false, include_hidden) && card->isKindOf("Slash")) {
            foreach (const Card *c, to->getEquips()) {
                if (card->getSuit() == c->getSuit())
                    return true;
            }
            foreach (int id, to->getShownHandcards()) {
                if (card->getSuit() == Sanguosha->getCard(id)->getSuit())
                    return true;
            }
        }
        return false;
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

    virtual int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->getSkillName() == "yidan")
            return 1000;
        else
            return 0;
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
    related_skills.insertMulti("chunhua", "#chunhua");

    General *hecatia = new General(this, "hecatia", "gzz", 4);
    hecatia->addSkill(new Santi);
    hecatia->addSkill(new SantiEffect);
    related_skills.insertMulti("santi", "#santi");

    General *clownpiece = new General(this, "clownpiece", "gzz", 3);
    clownpiece->addSkill(new Yuyi);
    clownpiece->addSkill(new Kuangluan);
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

    General *ringo = new General(this, "ringo", "gzz", 4);
    ringo->addSkill(new Yuejian);

    General *seiran = new General(this, "seiran", "gzz", 4);
    seiran->addSkill(new Yidan);
    seiran->addSkill(new YidanProhibit);
    seiran->addSkill(new YidanTargetMod);
    related_skills.insertMulti("yidan", "#yidan");
    related_skills.insertMulti("yidan", "#yidanmod");

    addMetaObject<ShayiCard>();
    //addMetaObject<ShayiMoveCard>();
    addMetaObject<YidanCard>();
    skills << new ShayiUse << new ShehuoProhibit << new ShehuoTargetMod; //<< new ChunhuaFilter << new YuyiEffect
}

ADD_PACKAGE(TH15)
