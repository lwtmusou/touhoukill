#include "th11.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"

class Xiangqi : public TriggerSkill
{
public:
    Xiangqi()
        : TriggerSkill("xiangqi")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;
        if (!damage.by_user)
            return d;
        foreach (ServerPlayer *satori, room->findPlayersBySkillName(objectName())) {
            if (damage.from && damage.from != satori && damage.card && !damage.from->isKongcheng() && damage.to != damage.from && damage.to->isAlive()
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
            invoke->invoker->showHiddenSkill(objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), damage.from->objectName());
            int id = room->askForCardChosen(invoke->invoker, damage.from, "hs", objectName());
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
    Huzhu()
        : TriggerSkill("huzhu$")
    {
        events << TargetConfirming;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash") || use.to.length() != 1)
            return QList<SkillInvokeDetail>();
        if (!use.to.first()->hasLordSkill(objectName()))
            return QList<SkillInvokeDetail>();

        use.card->setFlags("IgnoreFailed");
        foreach (ServerPlayer *liege, room->getLieges("dld", use.to.first())) {
            if (use.from->canSlash(liege, use.card, false)) {
                use.card->setFlags("-IgnoreFailed");
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.to.first(), use.to.first());
            }
        }
        use.card->setFlags("-IgnoreFailed");
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<ServerPlayer *> targets;
        use.card->setFlags("IgnoreFailed");
        foreach (ServerPlayer *p, room->getLieges("dld", invoke->invoker)) {
            if (use.from->canSlash(p, use.card, false))
                targets << p;
        }
        use.card->setFlags("-IgnoreFailed");

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
    CardsMoveStruct move(card_to_show, NULL, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, targets.first()->objectName()));
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
    Maihuo()
        : OneCardViewAsSkill("maihuo")
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
    Wunian()
        : ProhibitSkill("wunian")
    {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &, bool include_hidden) const
    {
        return from != to && to->hasSkill(objectName(), false, include_hidden) && to->isWounded() && card->isKindOf("TrickCard");
    }
};

class WunianEffect : public TriggerSkill
{
public:
    WunianEffect()
        : TriggerSkill("#wuniantr")
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
    //handling_method = Card::MethodUse;
    m_skillName = "yaoban";
}

void YaobanCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->getRoom()->damage(DamageStruct("yaoban", effect.from, effect.to));
}

bool YaobanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->hasFlag("Global_yaobanFailed");
}

class YaobanVS : public OneCardViewAsSkill
{
public:
    YaobanVS()
        : OneCardViewAsSkill("yaoban")
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
    Yaoban()
        : TriggerSkill("yaoban")
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
            if (utsuho->canDiscard(utsuho, "hs"))
                d << SkillInvokeDetail(this, utsuho, utsuho);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *ldlk = invoke->invoker;
        DamageStruct damage = data.value<DamageStruct>();
        foreach (ServerPlayer *p, room->getOtherPlayers(damage.to))
            room->setPlayerFlag(p, "Global_yaobanFailed");
        room->askForUseCard(ldlk, "@@yaoban", "@yaoban:" + damage.to->objectName());
        return false;
    }
};

class Here : public TriggerSkill
{
public:
    Here()
        : TriggerSkill("here")
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
                foreach (ServerPlayer *p, use.to) {
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
        foreach (const QString &flag, flags)
            new_slash->setFlags(flag);
        use.card = new_slash;
        data = QVariant::fromValue(use);
        return false;
    }
};

class Yuanling : public TriggerSkill
{
public:
    Yuanling()
        : TriggerSkill("yuanling")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive() && damage.to->isAlive() && damage.from != damage.to && damage.to->hasSkill(this)) {
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
    Songzang()
        : TriggerSkill("songzang")
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
    Guaili()
        : TriggerSkill("guaili")
    {
        events << SlashMissed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.to->isAlive() && effect.jink != NULL && effect.from->hasSkill(this) && effect.from->canDiscard(effect.from, "hs"))
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
    JiuhaoVS()
        : ZeroCardViewAsSkill("jiuhao")
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
    Jiuhao()
        : TriggerSkill("jiuhao")
    {
        events << PreCardUsed << EventPhaseChanging;
        view_as_skill = new JiuhaoVS;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            //since pingyi, we need to record "peach used" for everyone.
            if (use.from->getPhase() == Player::Play && (use.card->isKindOf("Peach") || use.card->isKindOf("Analeptic")))
                room->setPlayerFlag(use.from, "jiuhao");
            if (use.card->hasFlag(objectName())) {
                if (use.m_addHistory) {
                    room->addPlayerHistory(use.from, use.card->getClassName(), -1);
                    use.m_addHistory = false;
                    data = QVariant::fromValue(use);
                }
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

class Jidu : public TriggerSkill
{
public:
    Jidu()
        : TriggerSkill("jidu")
    {
        events << TargetSpecified << ConfirmDamage;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            QList<SkillInvokeDetail> d;
            if (use.card->isKindOf("Slash") && use.from && use.from->isAlive()) {
                bool can = false;
                foreach (ServerPlayer *p, use.to) {
                    if (p->getHp() >= use.from->getHp()) {
                        can = true;
                        break;
                    }
                }
                if (can) {
                    QList<ServerPlayer *> owners = room->findPlayersBySkillName(objectName());
                    foreach (ServerPlayer *p, owners) {
                        if (p->canDiscard(p, "hes"))
                            d << SkillInvokeDetail(this, p, p);
                    }
                }
            }
            return d;
        } else if (e == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->hasFlag("jidu_card"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true, NULL, false);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            QString prompt = "@jidu:" + use.from->objectName() + ":" + use.card->objectName();
            return room->askForCard(invoke->invoker, ".|.|.|.", prompt, data, objectName());
        }
        return true;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.card->setFlags("jidu_card");
            //room->setCardFlag(use.card, "jidu_card");//why can not clear flag while skill"taiji" triggered firstly?
        } else if (e == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from) {
                room->touhouLogmessage("#TouhouBuff", damage.from, objectName());
                QList<ServerPlayer *> logto;
                logto << damage.to;
                room->touhouLogmessage("#jidu_damage", damage.from, QString::number(damage.damage + 1), logto, QString::number(damage.damage));
            }
            damage.damage = damage.damage + 1;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

class Gelong : public TriggerSkill
{
public:
    Gelong()
        : TriggerSkill("gelong")
    {
        events << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && damage.from && damage.from->isAlive() && damage.to->hasSkill(this) && damage.to->isAlive()) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, false, damage.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, damage.to->objectName(), damage.from->objectName());
        QString choice = room->askForChoice(damage.from, objectName(), "gelong1+gelong2");
        if (choice == "gelong1") {
            damage.from->drawCards(1);
            int max = qMin(damage.from->getCards("hes").length(), 2);
            if (max > 0) {
                const Card *cards = room->askForExchange(damage.from, objectName(), max, max, true, "@gelong:" + damage.to->objectName() + ":" + QString::number(max));
                room->obtainCard(damage.to, cards, false);
            }
        } else {
            damage.from->turnOver();
            room->recover(damage.from, RecoverStruct());
        }
        return false;
    }
};

class Chuanran : public TriggerSkill
{
public:
    Chuanran()
        : TriggerSkill("chuanran")
    {
        events << Damage;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<ServerPlayer *> yamames = room->findPlayersBySkillName(objectName());
        if (yamames.isEmpty() || damage.nature != DamageStruct::Normal || !damage.from || damage.from->isDead() || damage.to->isChained())
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, yamames) {
            if ((p == damage.from || damage.from->isChained()) && damage.to != p) {
                d << SkillInvokeDetail(this, p, p, NULL, true, damage.to);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->notifySkillInvoked(invoke->owner, objectName());
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = invoke->owner;
        log.arg = objectName();
        room->sendLog(log);

        room->setPlayerProperty(invoke->targets.first(), "chained", true);
        return false;
    }
};

class Rebing : public TriggerSkill
{
public:
    Rebing()
        : TriggerSkill("rebing")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->getPhase() == Player::Finish && (!current->faceUp() || current->isChained()) && !current->isNude()) {
            bool invoke = false;
            foreach (ServerPlayer *p, room->getOtherPlayers(current)) {
                if (current->inMyAttackRange(p)) {
                    invoke = true;
                    break;
                }
            }
            if (!invoke)
                return d;
            QList<ServerPlayer *> yamames = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *p, yamames) {
                if (p != current)
                    d << SkillInvokeDetail(this, p, p);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(current)) {
            if (current->inMyAttackRange(p))
                targets << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@rebing:" + current->objectName(), true, true);
        if (target)
            invoke->targets << target;
        return target != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->owner->objectName(), current->objectName());
        QString prompt = "@rebing-slash:" + invoke->invoker->objectName() + ":" + invoke->targets.first()->objectName();
        if (!room->askForUseSlashTo(current, invoke->targets.first(), prompt)) {
            int id = room->askForCardChosen(invoke->invoker, current, "hes", objectName());
            room->obtainCard(invoke->invoker, id, room->getCardPlace(id) != Player::PlaceHand);
        }
        return false;
    }
};

class Diaoping : public TriggerSkill
{
public:
    Diaoping()
        : TriggerSkill("diaoping")
    {
        events << TargetSpecified;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash") || !use.from || use.from->isDead())
            return QList<SkillInvokeDetail>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *kisume, room->findPlayersBySkillName(objectName())) {
            foreach (ServerPlayer *p, use.to) {
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
            skillowner->showHiddenSkill(objectName());
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
    Tongju()
        : ProhibitSkill("tongju")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &, bool include_hidden) const
    {
        return to->hasSkill(objectName(), false, include_hidden) && ((card->isKindOf("SavageAssault") || card->isKindOf("IronChain")) || card->isKindOf("ArcheryAttack"));
    }
};

class Cuiji : public TriggerSkill
{
public:
    Cuiji()
        : TriggerSkill("cuiji")
    {
        events << DrawNCards;
    }

    static void do_cuiji(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        QString choice = room->askForChoice(player, "cuiji_suit", "red+black");
        bool isred = (choice == "red");
        room->touhouLogmessage("#cuiji_choice", player, "cuiji", QList<ServerPlayer *>(), choice);
        room->notifySkillInvoked(player, "cuiji");
        int acquired = 0;
        QList<int> throwIds;
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

                if (!throwIds.isEmpty()) {
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "cuiji", QString());
                    DummyCard dummy(throwIds);
                    room->throwCard(&dummy, reason, NULL);
                    throwIds.clear();
                }
            } else
                throwIds << id;
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DrawNCardsStruct s = data.value<DrawNCardsStruct>();
        if (s.player->hasSkill(this) && s.n > 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, s.player, s.player);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DrawNCardsStruct s = data.value<DrawNCardsStruct>();
        QStringList choices;
        int max = qMin(s.n, 2);
        for (int i = 0; i <= max; ++i)
            choices << QString::number(i);
        QString choice = room->askForChoice(invoke->invoker, objectName(), choices.join("+"), data);
        int num = choices.indexOf(choice);

        invoke->invoker->tag["cuiji"] = QVariant::fromValue(num);
        return num > 0;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DrawNCardsStruct draw = data.value<DrawNCardsStruct>();
        int minus = draw.player->tag["cuiji"].toInt();

        draw.n = draw.n - minus;
        data = QVariant::fromValue(draw);
        return false;
    }
};

class CuijiEffect : public TriggerSkill
{
public:
    CuijiEffect()
        : TriggerSkill("#cuiji")
    {
        events << AfterDrawNCards;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DrawNCardsStruct dc = data.value<DrawNCardsStruct>();
        int num = dc.player->tag["cuiji"].toInt();
        if (num > 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dc.player, dc.player, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        int num = invoke->invoker->tag["cuiji"].toInt();
        invoke->invoker->tag.remove("cuiji");

        for (int i = 0; i < num; ++i)
            Cuiji::do_cuiji(invoke->invoker);

        return false;
    }
};

class Baigui : public OneCardViewAsSkill
{
public:
    Baigui()
        : OneCardViewAsSkill("baigui")
    {
        filter_pattern = ".|spade|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return matchAvaliablePattern("savage_assault", pattern);
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
    Jiuchong()
        : OneCardViewAsSkill("jiuchong")
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
        return matchAvaliablePattern("analeptic", pattern) && Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE;
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
    General *satori = new General(this, "satori$", "dld", 3);
    satori->addSkill(new Xiangqi);
    //Room::askForCardChosen
    satori->addSkill(new Skill("duxin", Skill::Compulsory, "static"));
    //satori->addSkill(new Huzhu);
    satori->addSkill(new Skill("youtong$"));

    General *koishi = new General(this, "koishi", "dld", 3);
    koishi->addSkill(new Maihuo);
    koishi->addSkill(new Wunian);
    koishi->addSkill(new WunianEffect);
    related_skills.insertMulti("wunian", "#wuniantr");

    General *utsuho = new General(this, "utsuho", "dld", 4);
    utsuho->addSkill(new Yaoban);
    utsuho->addSkill(new Here);

    General *rin = new General(this, "rin", "dld", 4);
    rin->addSkill(new Yuanling);
    rin->addSkill(new Songzang);

    General *yugi = new General(this, "yugi", "dld", 4);
    yugi->addSkill(new Guaili);
    yugi->addSkill(new Jiuhao);

    General *parsee = new General(this, "parsee", "dld", 3);
    parsee->addSkill(new Jidu);
    parsee->addSkill(new Gelong);

    General *yamame = new General(this, "yamame", "dld", 4);
    yamame->addSkill(new Chuanran);
    yamame->addSkill(new Rebing);

    General *kisume = new General(this, "kisume", "dld", 3);
    kisume->addSkill(new Diaoping);
    kisume->addSkill(new Tongju);

    General *suika_sp = new General(this, "suika_sp", "dld", 3);
    suika_sp->addSkill(new Cuiji);
    suika_sp->addSkill(new CuijiEffect);
    suika_sp->addSkill(new Baigui);
    suika_sp->addSkill(new Jiuchong);
    related_skills.insertMulti("cuiji", "#cuiji");

    addMetaObject<MaihuoCard>();
    addMetaObject<YaobanCard>();
    addMetaObject<JiuhaoCard>();
}

ADD_PACKAGE(TH11)
