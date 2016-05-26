#include "th11.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"
#include "maneuvering.h"




class Xiangqi : public TriggerSkill
{
public:
    Xiangqi() : TriggerSkill("xiangqi")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *satori, room->findPlayersBySkillName(objectName())) {
            if (damage.from && damage.from != satori && damage.card  && !damage.from->isKongcheng()
                && damage.to != damage.from && damage.to->isAlive()
                && (satori->inMyAttackRange(damage.to) || damage.to == satori))
                d << SkillInvokeDetail(this, satori, satori);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt = "show:" + damage.from->objectName() + ":" + damage.to->objectName() + ":" + damage.card->objectName();
        invoke->invoker->tag["xiangqi_from"] = QVariant::fromValue(damage.from);
        invoke->invoker->tag["xiangqi_to"] = QVariant::fromValue(damage.to);
        invoke->invoker->tag["xiangqi_card"] = QVariant::fromValue(damage.card);
        if (invoke->invoker->askForSkillInvoke("xiangqi", prompt)) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), damage.from->objectName());
            int id = room->askForCardChosen(invoke->invoker, damage.from, "h", objectName());
            room->showCard(damage.from, id);
            invoke->invoker->tag["xiangqi_id"] = QVariant::fromValue(id);
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        int id = invoke->invoker->tag["xiangqi_id"].toInt();
        invoke->invoker->tag.remove("xiangqi_id");
        Card *showcard = Sanguosha->getCard(id);
        bool same = false;
        if (showcard->getTypeId() == damage.card->getTypeId())
            same = true;

        if (same && damage.to != invoke->invoker) {
            room->throwCard(id, damage.from, invoke->invoker);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), damage.to->objectName());

            room->damage(DamageStruct("xiangqi", invoke->invoker, damage.to));
        } else
            room->obtainCard(damage.to, showcard);

        return false;
    }
};

class Huzhu : public TriggerSkill
{
public:
    Huzhu() : TriggerSkill("huzhu$")
    {
        events << TargetConfirming;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash"))
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach(ServerPlayer *p, use.to) {
            if (!p->hasLordSkill(objectName()))
                continue;
            foreach (ServerPlayer *liege, room->getLieges("dld", p)) {
                if (liege != use.from && !use.to.contains(liege) && use.from->canSlash(liege, use.card, false) )
                    d << SkillInvokeDetail(this, p, p);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getLieges("dld", invoke->invoker)) {
            if (p == use.from)
                continue;
            if (use.from->canSlash(p, use.card, false) && !use.to.contains(p))
                targets << p;
        }

        foreach (ServerPlayer *p, targets) {
            room->setTag("huzhu_target", QVariant::fromValue(invoke->invoker));
            QString prompt = "slashtarget:" + use.from->objectName() + ":" + invoke->invoker->objectName() + ":" + use.card->objectName();
            if (p->askForSkillInvoke("huzhu_change", prompt)) {
                room->removeTag("huzhu_target");
                use.to << p;
                use.to.removeOne(invoke->invoker);
                data = QVariant::fromValue(use);

                QList<ServerPlayer *> logto;
                logto << invoke->invoker;
                room->touhouLogmessage("$CancelTarget", use.from, use.card->objectName(), logto);
                logto << p;
                logto.removeOne(invoke->invoker);
                room->touhouLogmessage("#huzhu_change", use.from, use.card->objectName(), logto);
                break;
            }
        }
        return false;
    }
};



MaihuoCard::MaihuoCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void MaihuoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->moveCardTo(Sanguosha->getCard(subcards.first()), NULL, Player::DrawPile);
    QList<int> card_to_show = room->getNCards(2, false);
    CardsMoveStruct move(card_to_show, NULL, Player::PlaceTable,
        CardMoveReason(CardMoveReason::S_REASON_TURNOVER, targets.first()->objectName()));
    room->moveCardsAtomic(move, true);
    room->getThread()->delay();
    bool bothred = true;
    DummyCard *dummy = new DummyCard;
    dummy->deleteLater();
    foreach (int id, card_to_show) {
        dummy->addSubcard(id);
        if (!Sanguosha->getCard(id)->isRed())
            bothred = false;
    }

    room->obtainCard(targets.first(), dummy);
    if (bothred) {
        QString choice = "draw";
        if (source->isWounded())
            choice = room->askForChoice(source, "maihuo", "draw+recover");
        if (choice == "draw")
            source->drawCards(2);
        else {
            RecoverStruct recover;
            recover.who = source;
            room->recover(source, recover);
        }
    }
}

class Maihuo : public OneCardViewAsSkill
{
public:
    Maihuo() :OneCardViewAsSkill("maihuo")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MaihuoCard");
    }


    virtual const Card *viewAs(const Card *originalCard) const
    {
        MaihuoCard *card = new MaihuoCard;
        card->addSubcard(originalCard);

        return card;
    }
};


class Wunian : public ProhibitSkill
{
public:
    Wunian() : ProhibitSkill("wunian")
    {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return from != to &&  to->hasSkill(objectName()) && to->isWounded() && card->isKindOf("TrickCard");
    }
};

class WunianEffect : public TriggerSkill
{
public:
    WunianEffect() : TriggerSkill("#wuniantr")
    {
        events << Predamage;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->hasSkill("wunian"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        damage.from = NULL;
        damage.by_user = false;

        room->touhouLogmessage("#TriggerSkill", invoke->invoker, "wunian");
        room->notifySkillInvoked(invoke->invoker, objectName());
        data = QVariant::fromValue(damage);
        return false;
    }
};



YaobanCard::YaobanCard()
{
    will_throw = true;
    handling_method = Card::MethodUse;
    m_skillName = "yaoban";
}

void YaobanCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->getRoom()->damage(DamageStruct("yaoban", effect.from, effect.to));
}

bool YaobanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QString str = Self->property("yaoban").toString();
    QStringList yaoban_targets = str.split("+");
    return  targets.isEmpty() && yaoban_targets.contains(to_select->objectName());
}

class YaobanVS : public OneCardViewAsSkill
{
public:
    YaobanVS() :OneCardViewAsSkill("yaoban")
    {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@yaoban";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        YaobanCard *card = new YaobanCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class Yaoban : public TriggerSkill
{
public:
    Yaoban() : TriggerSkill("yaoban")
    {
        view_as_skill = new YaobanVS;
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Fire)
            return QList<SkillInvokeDetail>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *utsuho, room->findPlayersBySkillName(objectName())) {
            if (utsuho->canDiscard(utsuho, "h"))
                d << SkillInvokeDetail(this, utsuho, utsuho);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *ldlk = invoke->invoker;
        DamageStruct damage = data.value<DamageStruct>();
        QStringList    yaobanTargets;
        foreach(ServerPlayer *p, room->getOtherPlayers(damage.to))
            yaobanTargets << p->objectName();

        ldlk->tag["yaoban_damage"] = QVariant::fromValue(damage);
        room->setPlayerProperty(ldlk, "yaoban", yaobanTargets.join("+"));
        room->askForUseCard(ldlk, "@@yaoban", "@yaoban:" + damage.to->objectName());
        room->setPlayerProperty(ldlk, "yaoban", QVariant());
        ldlk->tag.remove("yaoban_damage");
        return false;
    }
};

class Here : public TriggerSkill
{
public:
    Here() : TriggerSkill("here")
    {
        events << TargetConfirming << TargetSpecifying;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && !use.card->isKindOf("FireSlash")) {
            QList<SkillInvokeDetail> d;
            if (triggerEvent == TargetSpecifying) {
                if (use.from->hasSkill(this))
                d << SkillInvokeDetail(this, use.from, use.from, NULL, true);
            } else {
                foreach(ServerPlayer *p, use.to) {
                    if (p->hasSkill(this))
                        d << SkillInvokeDetail(this, p, p, NULL, true);
                }
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *player = invoke->invoker;
        room->touhouLogmessage("#TriggerSkill", player, "here");
        room->notifySkillInvoked(player, objectName());
        if (use.from)
            room->touhouLogmessage("#HereFilter", use.from, "here");

        FireSlash *new_slash = new FireSlash(use.card->getSuit(), use.card->getNumber());
        if (use.card->getSubcards().length() > 0)
            new_slash->addSubcards(use.card->getSubcards());
        else { //use.from is ai...
            int id = use.card->getEffectiveId();
            if (id > -1)
                new_slash->addSubcard(id);
        }

        //remain the information of origianl card
        new_slash->setSkillName(use.card->getSkillName());
        QStringList flags = use.card->getFlags();
        foreach(const QString &flag, flags)
            new_slash->setFlags(flag);
        use.card = new_slash;
        data = QVariant::fromValue(use);
        return false;
    }
};



class Yuanling : public TriggerSkill
{
public:
    Yuanling() : TriggerSkill("yuanling")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive() && damage.to->isAlive()
            && damage.from != damage.to && damage.to->hasSkill(this)) {
            FireSlash *slash = new FireSlash(Card::NoSuit, 0);
            slash->deleteLater();
            if (!damage.to->isCardLimited(slash, Card::MethodUse) && damage.to->canSlash(damage.from, slash, false))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, false, damage.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString prompt = "target:" + invoke->preferredTarget->objectName();
        invoke->invoker->tag["yuanling"] = QVariant::fromValue(invoke->preferredTarget);
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        FireSlash *slash = new FireSlash(Card::NoSuit, 0);
        slash->setSkillName("_" + objectName());
        room->useCard(CardUseStruct(slash, invoke->invoker, invoke->targets.first()), false);
        return false;
    }
};

class Songzang : public TriggerSkill
{
public:
    Songzang() : TriggerSkill("songzang")
    {
        events << AskForPeaches;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 10;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.nowAskingForPeaches != dying.who && dying.nowAskingForPeaches->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dying.nowAskingForPeaches, dying.nowAskingForPeaches, NULL, false, dying.who);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        //just for ai
        invoke->invoker->tag["songzang_dying"] = data;
        return room->askForCard(invoke->invoker, ".|spade", "@songzang:" + invoke->preferredTarget->objectName(), data, objectName());
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        room->setPlayerFlag(invoke->targets.first(), "-Global_Dying");
        DamageStruct damage;
        damage.from = invoke->invoker;
        room->killPlayer(invoke->targets.first(), &damage);

        return true; //avoid triggering askforpeach
    }
};



class Guaili : public TriggerSkill
{
public:
    Guaili() : TriggerSkill("guaili")
    {
        events << SlashMissed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.to->isAlive() && effect.from->hasSkill(this) && effect.from->canDiscard(effect.from, "h"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.from, effect.from);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        effect.from->tag["guaili_target"] = QVariant::fromValue(effect.to);
        const Card *card = room->askForCard(effect.from, ".|red|.|hand", "@guaili:" + effect.to->objectName(), data, objectName());
        if (card)
            return true;
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        effect.from->drawCards(1);
        room->slashResult(effect, NULL);
        return true;
    }
};

JiuhaoCard::JiuhaoCard()
{
    will_throw = false;
}

bool JiuhaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *card = new Slash(Card::NoSuit, 0);
    card->deleteLater();
    card->setFlags("jiuhao");
    return card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool JiuhaoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Slash *card = new Slash(Card::NoSuit, 0);
    card->deleteLater();
    card->setFlags("jiuhao");
    return card->targetsFeasible(targets, Self);
}

const Card *JiuhaoCard::validate(CardUseStruct &use) const
{
    Slash *card = new Slash(Card::NoSuit, 0);
    use.from->getRoom()->setCardFlag(card, "jiuhao");
    card->setSkillName("jiuhao");
    //use.from->getRoom()->setPlayerFlag(use.from, "jiuhaoused");
    return card;
}

class JiuhaoVS : public ZeroCardViewAsSkill
{
public:
    JiuhaoVS() : ZeroCardViewAsSkill("jiuhao")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->hasFlag("jiuhao") && !player->hasFlag("jiuhaoused");
    }

    virtual const Card *viewAs() const
    {
        JiuhaoCard *slash = new JiuhaoCard;
        return slash;
    }
};

class Jiuhao : public TriggerSkill
{
public:
    Jiuhao() : TriggerSkill("jiuhao")
    {
        events << PreCardUsed << EventPhaseChanging;
        view_as_skill = new JiuhaoVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            //since pingyi, we need to record "peach used" for everyone.
            if (use.from->getPhase() == Player::Play
                && (use.card->isKindOf("Peach") || use.card->isKindOf("Analeptic")))
                room->setPlayerFlag(use.from, "jiuhao");
            if (use.card->hasFlag(objectName())) {
                room->addPlayerHistory(use.from, use.card->getClassName(), -1);
                room->setPlayerFlag(use.from, "jiuhaoused");
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                room->setPlayerFlag(change.player, "-jiuhao");
                room->setPlayerFlag(change.player, "-jiuhaoused");
            }
        }
    }
};



class JiduVS : public OneCardViewAsSkill
{
public:
    JiduVS() : OneCardViewAsSkill("jidu")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }


    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            Duel *duel = new Duel(Card::SuitToBeDecided, -1);
            duel->addSubcard(originalCard);
            duel->setSkillName(objectName());
            return duel;
        } else
            return NULL;
    }
};

class Jidu : public MasochismSkill
{
public:
    Jidu() : MasochismSkill("jidu")
    {
        view_as_skill = new JiduVS;
    }

    QList<SkillInvokeDetail> triggerable(const Room *, const DamageStruct &damage) const
    {
        if (damage.card && damage.card->isKindOf("Duel") && damage.to->hasSkill(this) && damage.to->isAlive())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        return QList<SkillInvokeDetail>();
    }

    void onDamaged(Room *, QSharedPointer<SkillInvokeDetail> invoke, const DamageStruct &) const
    {
        invoke->invoker->drawCards(1);
    }
};

class JiduProhibit : public ProhibitSkill
{
public:
    JiduProhibit() : ProhibitSkill("#jiduprevent")
    {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return card->getSkillName() == "jidu" && from->getHp() > to->getHp();
    }
};

class Gelong : public TriggerSkill
{
public:
    Gelong() : TriggerSkill("gelong")
    {
        events << TargetConfirmed;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->isKindOf("Slash") && use.from  && use.from->isAlive()) {
            foreach(ServerPlayer *p, use.to) {
                if (p->hasSkill(this))
                    d << SkillInvokeDetail(this, p, p, NULL, true, use.from);
            }
        }
        return d;
    }


    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *player = invoke->invoker;

        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.from->objectName());

        SupplyShortage *supply = new SupplyShortage(Card::NoSuit, 0);
        supply->deleteLater();
        QString choice;
        bool canchoice = true;

        if (player->isProhibited(use.from, supply) || use.from->containsTrick("supply_shortage"))
            canchoice = false;
        if (canchoice)
            choice = room->askForChoice(use.from, objectName(), "gelong1+gelong2");
        else
            choice = "gelong1";

        if (choice == "gelong1") {
            room->loseHp(use.from);
            if (use.from->isAlive())
                use.from->drawCards(1);
        } else {
            Card *first = Sanguosha->getCard(room->drawCard());
            SupplyShortage *supplyshortage = new SupplyShortage(first->getSuit(), first->getNumber());
            WrappedCard *vs_card = Sanguosha->getWrappedCard(first->getId());
            vs_card->setSkillName("_gelong");
            vs_card->takeOver(supplyshortage);
            room->broadcastUpdateCard(room->getAlivePlayers(), vs_card->getId(), vs_card);
            CardsMoveStruct move;
            move.card_ids << vs_card->getId();
            move.to = use.from;
            move.to_place = Player::PlaceDelayedTrick;
            room->moveCardsAtomic(move, true);

            LogMessage mes;
            mes.type = "$PasteCard";
            mes.from = use.from;
            mes.to << use.from;
            mes.arg = objectName();
            mes.card_str = vs_card->toString();
            room->sendLog(mes);
        }

        return false;
    }
};



ChuanranCard::ChuanranCard()
{
    will_throw = true;
    handling_method = Card::MethodNone;
}

bool ChuanranCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QString str = Self->property("chuanran").toString();
    QStringList chuanran_targets = str.split("+");
    return  targets.isEmpty() && chuanran_targets.contains(to_select->objectName());
}

void ChuanranCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    int id = source->tag["chuanran_id"].toInt();
    Card *card = Sanguosha->getCard(id);
    CardsMoveStruct  move;
    move.to = target;
    move.to_place = Player::PlaceDelayedTrick;
    QString trick_name = card->objectName();
    if (!card->isKindOf("DelayedTrick")) {
        SupplyShortage *supplyshortage = new SupplyShortage(card->getSuit(), card->getNumber());
        trick_name = supplyshortage->objectName();
        WrappedCard *vs_card = Sanguosha->getWrappedCard(card->getId());
        vs_card->setSkillName("_chuanran");
        vs_card->takeOver(supplyshortage);
        room->broadcastUpdateCard(room->getAlivePlayers(), vs_card->getId(), vs_card);
    }

    move.card_ids << id;
    room->moveCardsAtomic(move, true);
    room->touhouLogmessage("#chuanran_move", target, trick_name);
}

class ChuanranVS : public OneCardViewAsSkill
{
public:
    ChuanranVS() : OneCardViewAsSkill("chuanran")
    {
        filter_pattern = ".|black|.|.!";
        response_pattern = "@@chuanran";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            ChuanranCard *indl = new ChuanranCard;
            indl->addSubcard(originalCard);
            return indl;
        } else
            return NULL;
    }
};

class Chuanran : public TriggerSkill
{
public:
    Chuanran() : TriggerSkill("chuanran")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
        view_as_skill = new ChuanranVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *current = room->getCurrent();
            if (!move.from || !current || current->getPhase() != Player::Judge || move.from != current)
                return;
            if ((move.from_places.contains(Player::PlaceDelayedTrick) && move.to_place == Player::PlaceTable)) {
                QVariantList record_ids = room->getTag("chuanranTemp").toList();
                foreach (int id, move.card_ids) {
                    if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceDelayedTrick && !record_ids.contains(id)) {
                        record_ids << id;
                    }
                }
                room->setTag("chuanranTemp", record_ids);
            } else if (move.to_place == Player::DiscardPile) {
                QVariantList record_ids = room->getTag("chuanranTemp").toList();
                QVariantList ids = room->getTag("chuanran").toList();
                foreach(int id, move.card_ids) {
                    if (room->getCardPlace(id) == Player::DiscardPile) {
                        if (move.from_places.at(move.card_ids.indexOf(id)) == Player::PlaceDelayedTrick && !ids.contains(id))
                            ids << id;
                        else if (record_ids.contains(id) && !ids.contains(id))
                            ids << id;
                    }
                }
                foreach(int id, move.card_ids)
                    record_ids.removeOne(id);
                room->setTag("chuanranTemp", record_ids);
                room->setTag("chuanran", ids);
            } else {
                QVariantList record_ids = room->getTag("chuanranTemp").toList();
                foreach(int id, move.card_ids)
                    record_ids.removeOne(id);
                room->setTag("chuanranTemp", record_ids);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Judge) {
                room->removeTag("chuanranTemp");
                room->removeTag("chuanran");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *current = room->getCurrent();
            if (!current || current->getPhase() != Player::Judge)
                return QList<SkillInvokeDetail>();

            QList<SkillInvokeDetail> d;
            if (move.to_place == Player::DiscardPile) {
                QVariantList ids = room->getTag("chuanran").toList();
                foreach(int id, move.card_ids) {
                    if (room->getCardPlace(id) == Player::DiscardPile && ids.contains(id)) {
                        foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                            if (current != current)
                                d << SkillInvokeDetail(this, p, p);
                        }
                        break;
                    }
                }
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QVariantList chuanran_ids = room->getTag("chuanran").toList();
        QList<int> all;
        foreach(int id, move.card_ids) {
            if (room->getCardPlace(id) == Player::DiscardPile && chuanran_ids.contains(id)) {
                all << id;
                chuanran_ids.removeOne(id);
            }
        }
        room->setTag("chuanran", chuanran_ids);


        foreach(int id, all) {
            QList<ServerPlayer *>others;
            QStringList chuanranTargets;
            QString trickname;
            if (Sanguosha->getCard(id)->isKindOf("DelayedTrick"))
                trickname = Sanguosha->getCard(id)->objectName();
            else
                trickname = "supply_shortage";
            foreach(ServerPlayer *p, room->getOtherPlayers(room->getCurrent())) {
                if (!p->containsTrick(trickname)) {
                    others << p;
                    chuanranTargets << p->objectName();
                }
            }
            if (!chuanranTargets.isEmpty()) {
                room->setPlayerProperty(invoke->invoker, "chuanran", chuanranTargets.join("+"));
                invoke->invoker->tag["chuanran_cardname"] == QVariant::fromValue(trickname);
                invoke->invoker->tag["chuanran_id"] = QVariant::fromValue(id);
                room->askForUseCard(invoke->invoker, "@@chuanran", "@chuanran:" + trickname);
                room->setPlayerProperty(invoke->invoker, "chuanran", QVariant());
            }
        }
        return false;
    }
};

class Rebing : public MasochismSkill
{
public:
    Rebing() : MasochismSkill("rebing")
    {
    }

    QList<SkillInvokeDetail> triggerable(const Room *room, const DamageStruct &damage) const
    {
        if (damage.to->hasSkill(this) && damage.to->isAlive()) {
            foreach (ServerPlayer *p, room->getOtherPlayers(damage.to)) {
                if (!p->isKongcheng() && p->getCards("j").length() > 0)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<ServerPlayer *> listt;
        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (!p->isKongcheng() && p->getCards("j").length() > 0)
                listt << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, listt, objectName(), "@rebing", true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    void onDamaged(Room *room, QSharedPointer<SkillInvokeDetail> invoke, const DamageStruct &) const
    {
        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "h", objectName());
        room->obtainCard(invoke->invoker, id, false);
    }
};



class Diaoping : public TriggerSkill
{
public:
    Diaoping() : TriggerSkill("diaoping")
    {
        events << TargetSpecified;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash") || !use.from || use.from->isDead())
            return QList<SkillInvokeDetail>();
        QList<SkillInvokeDetail> d;
        foreach(ServerPlayer *kisume, room->findPlayersBySkillName(objectName())) {
            foreach(ServerPlayer *p, use.to) {
                if (kisume->inMyAttackRange(p) || kisume == p) {
                    if (kisume->getHandcardNum() > 0 && use.from->getHandcardNum() > 0 && kisume != use.from)
                        d << SkillInvokeDetail(this, kisume, kisume);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *skillowner = invoke->invoker;
        bool good_result = false;
        QString prompt = "slashtarget:" + use.from->objectName() + ":" + use.card->objectName();
        skillowner->tag["diaoping_slash"] = data;
        while (use.from->isAlive() && good_result == false && skillowner->getHandcardNum() > 0 && use.from->getHandcardNum() > 0) {
            if (!room->askForSkillInvoke(skillowner, "diaoping", prompt))
                return false;
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, skillowner->objectName(), use.from->objectName());

            if (skillowner->pindian(use.from, "diaoping", NULL)) {
                good_result = true;
                break;
            }
        }
        return good_result;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.from->turnOver();
        use.nullified_list << "_ALL_TARGETS";
        data = QVariant::fromValue(use);
        return false;
    }

};

class Tongju : public ProhibitSkill
{
public:
    Tongju() : ProhibitSkill("tongju")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->hasSkill(objectName()) && ((card->isKindOf("SavageAssault") || card->isKindOf("IronChain")) || card->isKindOf("ArcheryAttack"));
    }
};



class Cuiji : public TriggerSkill
{
public:
    Cuiji() : TriggerSkill("cuiji")
    {
        events << DrawNCards;
    }

    static bool do_cuiji(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        QString choice = room->askForChoice(player, "cuiji", "red+black+cancel");
        if (choice == "cancel")
            return false;
        bool isred = (choice == "red");
        room->touhouLogmessage("#cuiji_choice", player, "cuiji", QList<ServerPlayer *>(), choice);
        room->notifySkillInvoked(player, "cuiji");
        int acquired = 0;
        while (acquired < 1) {
            int id = room->drawCard();
            CardsMoveStruct move(id, NULL, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName()));
            move.reason.m_skillName = "cuiji";
            room->moveCardsAtomic(move, true);
            room->getThread()->delay();
            Card *card = Sanguosha->getCard(id);
            if (card->isRed() == isred) {
                acquired = acquired + 1;
                CardsMoveStruct move2(id, player, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_GOTBACK, player->objectName()));
                room->moveCardsAtomic(move2, false);
            } else {
                CardsMoveStruct move3(id, NULL, Player::DiscardPile, CardMoveReason(CardMoveReason::S_REASON_NATURAL_ENTER, ""));
                room->moveCardsAtomic(move3, true);
            }
        }
        return true;

    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DrawNCardsStruct s = data.value<DrawNCardsStruct>();
        if (s.player->hasSkill(this) && s.n > 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, s.player, s.player);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DrawNCardsStruct s = data.value<DrawNCardsStruct>();
        int num = 0;
        for (int i = 0; i < s.n; ++i) {
            if (do_cuiji(invoke->invoker))
                num++;
            else
                break;
        }
        invoke->invoker->tag["cuiji"] = QVariant::fromValue(num);
        return num > 0;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DrawNCardsStruct draw = data.value<DrawNCardsStruct>();
        int minus = draw.player->tag["cuiji"].toInt();
        draw.player->tag.remove("cuiji");
        draw.n = draw.n - minus;
        data = QVariant::fromValue(draw);
        return false;
    }


};

class Baigui : public OneCardViewAsSkill
{
public:
    Baigui() : OneCardViewAsSkill("baigui")
    {
        filter_pattern = ".|spade|.|hand";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            SavageAssault *sa = new SavageAssault(Card::SuitToBeDecided, -1);
            sa->addSubcard(originalCard);
            sa->setSkillName(objectName());
            return sa;
        } else
            return NULL;
    }
};

class Jiuchong : public OneCardViewAsSkill
{
public:
    Jiuchong() : OneCardViewAsSkill("jiuchong")
    {
        filter_pattern = ".|heart|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Analeptic::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.contains("analeptic") && Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            Analeptic *ana = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
            ana->addSubcard(originalCard);
            ana->setSkillName(objectName());
            return ana;
        } else
            return NULL;
    }
};




TH11Package::TH11Package()
    : Package("th11")
{
    General *satori = new General(this, "satori$", "dld", 3, false);
    satori->addSkill(new Xiangqi);
    //Room::askForCardChosen
    satori->addSkill(new Skill("duxin", Skill::Compulsory));
    satori->addSkill(new Huzhu);

    General *koishi = new General(this, "koishi", "dld", 3, false);
    koishi->addSkill(new Maihuo);
    koishi->addSkill(new Wunian);
    koishi->addSkill(new WunianEffect);
    related_skills.insertMulti("wunian", "#wuniantr");

    General *utsuho = new General(this, "utsuho", "dld", 4, false);
    utsuho->addSkill(new Yaoban);
    utsuho->addSkill(new Here);


    General *rin = new General(this, "rin", "dld", 4, false);
    rin->addSkill(new Yuanling);
    rin->addSkill(new Songzang);


    General *yugi = new General(this, "yugi", "dld", 4, false);
    yugi->addSkill(new Guaili);
    yugi->addSkill(new Jiuhao);
    related_skills.insertMulti("jiuhao", "#jiuhaoTargetMod");


    General *parsee = new General(this, "parsee", "dld", 3, false);
    parsee->addSkill(new Jidu);
    parsee->addSkill(new JiduProhibit);
    parsee->addSkill(new Gelong);
    related_skills.insertMulti("jidu", "#jiduprevent");

    General *yamame = new General(this, "yamame", "dld", 4, false);
    yamame->addSkill(new Chuanran);
    yamame->addSkill(new Rebing);

    General *kisume = new General(this, "kisume", "dld", 3, false);
    kisume->addSkill(new Diaoping);
    kisume->addSkill(new Tongju);

    General *suika_sp = new General(this, "suika_sp", "dld", 3, false);
    suika_sp->addSkill(new Cuiji);
    suika_sp->addSkill(new Baigui);
    suika_sp->addSkill(new Jiuchong);

    addMetaObject<MaihuoCard>();
    addMetaObject<YaobanCard>();
    addMetaObject<JiuhaoCard>();
    addMetaObject<ChuanranCard>();
}

ADD_PACKAGE(TH11)

