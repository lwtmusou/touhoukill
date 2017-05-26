#include "th15.h"
#include "general.h"

#include "engine.h"
#include "skill.h"

class Wuhui : public TriggerSkill
{
public:
    Wuhui()
        : TriggerSkill("wuhui")
    {
        events << Damage << Damaged;
    }

    static QList<ServerPlayer *> wuhuiTargets(const Room *room) {
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getAlivePlayers()) {
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
            QList<ServerPlayer *> targets = wuhuiTargets(room);
            if (targets.isEmpty())
                return d;
            for (int i = 0; i < damage.damage; i++)
                d << SkillInvokeDetail(this, damage.from, damage.from, targets);      
        }
        if (e == Damaged && damage.to && damage.to->isAlive() && damage.to->hasSkill(this)) {
            QList<ServerPlayer *> targets = wuhuiTargets(room);
            if (targets.isEmpty())
                return d;
            for (int i = 0; i < damage.damage; i++)
                d << SkillInvokeDetail(this, damage.to, damage.to, targets);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, invoke->targets, objectName(), "@wuhui", true, true);
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
        new_phases <<  Player::Draw << Player::Play << Player::Discard;
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

    static void removeSantiLimit(ServerPlayer *player) {
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

    static void setSantiLimit(ServerPlayer *player) {
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
        }
        else if (triggerEvent == EventAcquireSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            if (a.skill->objectName() == "santi")
                setSantiLimit(a.player);
        }
        else if (triggerEvent == EventSkillInvalidityChange) {
            QList<SkillInvalidStruct> invalids = data.value<QList<SkillInvalidStruct> >();
            foreach(SkillInvalidStruct v, invalids) {
                if (!v.skill || v.skill->objectName() == "santi") {
                    if (!v.invalid && v.player->hasSkill(this))
                        setSantiLimit(v.player);
                    else if (v.invalid)
                        removeSantiLimit(v.player);
                }
            }
        }
        else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                removeSantiLimit(change.player);
        }
        else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *current = room->getCurrent();
            if (current->getPhase() == Player::Play && current->hasSkill("santi"))
                setSantiLimit(current);
        }
    }
};


class Kuangluan : public TriggerSkill
{
public:
    Kuangluan()
        : TriggerSkill("kuangluan")
    {
        events << TargetSpecified << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        //record times of using card
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.from->getPhase() == Player::Play &&
                (use.card->isNDTrick() || use.card->isKindOf("Slash"))) {
                if (use.from->hasFlag("kuangluan_first"))
                    room->setPlayerFlag(use.from, "kuangluan_second");
                else
                    room->setPlayerFlag(use.from, "kuangluan_first");
            }
        
        }

        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                change.player->setFlags("-kuangluan_first");
                change.player->setFlags("-kuangluan_second");
            }
        }
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.from->getPhase() == Player::Play &&
                (use.card->isNDTrick() || use.card->isKindOf("Slash"))) {
                if (use.from->hasFlag("kuangluan_first") && !use.from->hasFlag("kuangluan_second")) {
                    QList<SkillInvokeDetail> d;
                    
                    foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                        if (!p->getCards("h").isEmpty() || !use.from->getCards("h").isEmpty())
                            d << SkillInvokeDetail(this, p, p, NULL, false, use.from);
                    }
                    return d;
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
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
        invoke->tag["kuangluan"] = choice;
        return true;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QString choice = invoke->tag.value("kuangluan").toString();
        ServerPlayer *target = (choice == "changeProcess") ? invoke->invoker : invoke->targets.first();
        int id = room->askForCardChosen(invoke->invoker, target, "h", objectName());
        QList<int> ids;
        ids << id;
        target->addToShownHandCards(ids);
        if (choice == "changeProcess") {
            CardUseStruct use = data.value<CardUseStruct>();
            room->setCardFlag(use.card, "kuangluanProcess");//gamerule process true effect 
        }
        return false;
    }

};


class KuangluanEffect : public TriggerSkill
{
public:
    KuangluanEffect()
        : TriggerSkill("#kuangluan")
    {
        events << Cancel;
        global = true;//it is game rule. not general skill
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (data.canConvert<CardEffectStruct>()) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (!effect.card->hasFlag("kuangluanProcess"))
                return QList<SkillInvokeDetail>();

            if (effect.to->isAlive() && effect.from->isAlive() && effect.from != effect.to
                && !effect.to->isKongcheng() && !effect.from->isKongcheng())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, NULL, false, effect.from);
        }
        else {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (!effect.slash->hasFlag("kuangluanProcess"))
                return QList<SkillInvokeDetail>();
            if (effect.to->isAlive() && effect.from->isAlive() && effect.from != effect.to
                && !effect.to->isKongcheng() && !effect.from->isKongcheng())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, NULL, false, effect.from);
            
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        const Card *card = NULL;
        if (data.canConvert<CardEffectStruct>()) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            card = effect.card;
        }
        else {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            card = effect.slash;
        }
        QString prompt = "cancel:" + invoke->preferredTarget->objectName() + ":" + card->objectName();
        return invoke->invoker->askForSkillInvoke("kuangluanProcess", prompt);
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        const Card *card = NULL;
        if (data.canConvert<CardEffectStruct>()) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            card = effect.card;
        }
        else {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            card = effect.slash;
        }
        if (invoke->invoker->pindian(invoke->targets.first(), "kuangluanProcess")) {
            LogMessage log;
            log.type = "#KuangluanCancel";
            log.from = invoke->targets.first();
            log.to << invoke->invoker;
            log.arg = card->objectName();
            log.arg2 = "kuangluan";
            room->sendLog(log);

            if (data.canConvert<CardEffectStruct>()) {
                CardEffectStruct effect = data.value<CardEffectStruct>();
                effect.canceled = true;
                data = QVariant::fromValue(effect);
            }
            else {
                SlashEffectStruct effect = data.value<SlashEffectStruct>();
                effect.canceled = true;
                data = QVariant::fromValue(effect);
            }
        }
        return true;
    }

};


class Yuyan : public TriggerSkill
{
public:
    Yuyan()
        : TriggerSkill("yuyan")
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

        }
        else if (e == CardsMoveOneTime) {
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
            foreach(ServerPlayer *piece, room->findPlayersBySkillName(objectName())) {
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
        }
        else if (e == PindianAsked) {
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
    junko->addSkill(new Wuhui);
    junko->addSkill(new Skill("chunhua"));
    junko->addSkill(new Skill("shayi$"));

    General *hecatia = new General(this, "hecatia", "gzz", 4, false);
    hecatia->addSkill(new Santi);
    hecatia->addSkill(new SantiEffect);
    related_skills.insertMulti("santi", "#santi");

    General *clownpiece = new General(this, "clownpiece", "gzz", 3, false);
    clownpiece->addSkill(new Kuangluan);
    clownpiece->addSkill(new Yuyan);

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
    skills << new KuangluanEffect;
}

ADD_PACKAGE(TH15)
