#include "th15.h"
#include "general.h"

#include "engine.h"
#include "skill.h"

class Xiahui : public TriggerSkill
{
public:
    Xiahui()
        : TriggerSkill("xiahui")
    {
        events << Damage << Damaged;
    }

    static QList<ServerPlayer *> xiahuiTargets(const Room *room)
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (!p->getCards("h").isEmpty())
                targets << p;
        }
        return targets;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;
        if (e == Damage && damage.from && damage.from->isAlive() && damage.from->hasSkill(this)) {
            QList<ServerPlayer *> targets = xiahuiTargets(room);
            if (targets.isEmpty())
                return d;
            for (int i = 0; i < damage.damage; i++)
                d << SkillInvokeDetail(this, damage.from, damage.from, targets);
        }
        if (e == Damaged && damage.to && damage.to->isAlive() && damage.to->hasSkill(this)) {
            QList<ServerPlayer *> targets = xiahuiTargets(room);
            if (targets.isEmpty())
                return d;
            for (int i = 0; i < damage.damage; i++)
                d << SkillInvokeDetail(this, damage.to, damage.to, targets);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, invoke->targets, objectName(), "@xiahui", true, true);
        if (target) {
            invoke->targets.clear();
            invoke->targets << target;
        }
        return target != NULL;
    }
    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "h", objectName());
        QList<int> ids;
        ids << id;
        invoke->targets.first()->addToShownHandCards(ids);
        return false;
    }
};

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
            ServerPlayer *player = data.value<ServerPlayer *>();
            room->filterCards(player, player->getCards("hes"), true);
        }
    }
};

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
};

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
        if (player->getMark("santi_limit_1") > 0) {
            room->removePlayerCardLimitation(player, "use", "TrickCard,EquipCard$1");
            room->setPlayerMark(player, "santi_limit_1", 0);
        }
        if (player->getMark("santi_limit_2") > 0) {
            room->removePlayerCardLimitation(player, "use", "TrickCard,BasicCard$1");
            room->setPlayerMark(player, "santi_limit_2", 0);
        }
        if (player->getMark("santi_limit_3") > 0) {
            room->removePlayerCardLimitation(player, "use", "EquipCard,BasicCard$1");
            room->setPlayerMark(player, "santi_limit_3", 0);
        }
    }

    static void setSantiLimit(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        removeSantiLimit(player);
        if (player->getMark("santi") == 1) {
            room->setPlayerCardLimitation(player, "use", "TrickCard,EquipCard", true);
            room->setPlayerMark(player, "santi_limit_1", 1);
        }
        if (player->getMark("santi") == 2) {
            room->setPlayerCardLimitation(player, "use", "TrickCard,BasicCard", true);
            room->setPlayerMark(player, "santi_limit_2", 1);
        }
        if (player->getMark("santi") == 3) {
            room->setPlayerCardLimitation(player, "use", "EquipCard,BasicCard", true);
            room->setPlayerMark(player, "santi_limit_3", 1);
        }
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
            if (current->getPhase() == Player::Play && current->hasSkill("santi"))
                setSantiLimit(current);
        }
    }
};

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
    if (!Self->isProhibited(to_select, card, targets)) {
        foreach (const Card *c, to_select->getEquips()) {
            if (card->getSuit() == c->getSuit())
                return true;
        }
        foreach (int id, to_select->getShownHandcards()) {
            if (card->getSuit() == Sanguosha->getCard(id)->getSuit())
                return true;
        }
        return !targets.isEmpty() && card->targetFilter(targets, to_select, Self);
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

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        if (to->hasSkill("yidan") && card->isKindOf("Slash")) {
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

TH15Package::TH15Package()
    : Package("th15")
{
    General *junko = new General(this, "junko$", "gzz", 4, false);
    junko->addSkill(new Xiahui);
    junko->addSkill(new Chunhua);
    junko->addSkill(new ChunhuaEffect);
    junko->addSkill(new Shayi);
    related_skills.insertMulti("chunhua", "#chunhua");

    General *hecatia = new General(this, "hecatia", "gzz", 4, false);
    hecatia->addSkill(new Santi);
    hecatia->addSkill(new SantiEffect);
    related_skills.insertMulti("santi", "#santi");

    General *clownpiece = new General(this, "clownpiece", "gzz", 3, false);
    clownpiece->addSkill(new Yuyi);
    clownpiece->addSkill(new Kuangluan);

    General *seiran = new General(this, "seiran", "gzz", 4, false);
    seiran->addSkill(new Yidan);
    seiran->addSkill(new YidanProhibit);
    related_skills.insertMulti("yidan", "#yidan");

    General *ringo = new General(this, "ringo", "gzz", 4, false, true, true);
    Q_UNUSED(ringo)

    General *doremy = new General(this, "doremy", "gzz", 4, false, true, true);
    Q_UNUSED(doremy)

    General *sagume = new General(this, "sagume", "gzz", 4, false, true, true);
    Q_UNUSED(sagume)

    addMetaObject<YidanCard>();
    skills << new YuyiEffect << new ChunhuaFilter;
}

ADD_PACKAGE(TH15)
