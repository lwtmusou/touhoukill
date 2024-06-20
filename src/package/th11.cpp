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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<SkillInvokeDetail> d;

        foreach (ServerPlayer *satori, room->findPlayersBySkillName(objectName())) {
            if ((damage.from != nullptr) && damage.from != satori && (damage.card != nullptr) && !damage.from->isKongcheng() && damage.to != damage.from && damage.to->isAlive()
                && (satori->inMyAttackRange(damage.to) || damage.to == satori))
                d << SkillInvokeDetail(this, satori, satori);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

MaihuoCard::MaihuoCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void MaihuoCard::use(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *source = card_use.from;
    const QList<ServerPlayer *> &targets = card_use.to;

    room->moveCardTo(Sanguosha->getCard(subcards.first()), nullptr, Player::DrawPile);
    QList<int> card_to_show = room->getNCards(2, false);
    CardsMoveStruct move(card_to_show, nullptr, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, targets.first()->objectName()));
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

    bool isEnabledAtPlay(const Player *player) const override
    {
        return !player->hasUsed("MaihuoCard");
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        MaihuoCard *card = new MaihuoCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Wunian : public TriggerSkill
{
public:
    Wunian()
        : TriggerSkill("wunian")
    {
        events << Predamage << TargetConfirming;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const override
    {
        if (e == Predamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if ((damage.from != nullptr) && damage.from->hasSkill("wunian"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, nullptr, true);
        } else if (e == TargetConfirming) {
            QList<SkillInvokeDetail> d;
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() == Card::TypeTrick) {
                foreach (ServerPlayer *p, use.to) {
                    if (p->hasSkill(this) && p->isWounded() && use.from != nullptr && use.from != p)
                        d << SkillInvokeDetail(this, p, p, nullptr, true);
                }
            }
            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (e == Predamage) {
            DamageStruct damage = data.value<DamageStruct>();
            damage.from = nullptr;
            damage.by_user = false;

            room->touhouLogmessage("#TriggerSkill", invoke->invoker, "wunian");
            room->notifySkillInvoked(invoke->invoker, objectName());
            data = QVariant::fromValue(damage);
        } else if (e == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.to.removeAll(invoke->invoker);
            data = QVariant::fromValue(use);
            LogMessage log;
            log.type = "#SkillAvoid";
            log.from = invoke->invoker;
            log.arg = objectName();
            log.arg2 = use.card->objectName();
            room->sendLog(log);
        }
        return false;
    }
};

YaobanCard::YaobanCard()
{
    will_throw = true;
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

    const Card *viewAs(const Card *originalCard) const override
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

    bool canPreshow() const override
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && !use.card->isKindOf("FireSlash")) {
            QList<SkillInvokeDetail> d;
            if (triggerEvent == TargetSpecifying) {
                if (use.from != nullptr && use.from->hasSkill(this))
                    d << SkillInvokeDetail(this, use.from, use.from, nullptr, true);
            } else {
                foreach (ServerPlayer *p, use.to) {
                    if (p->hasSkill(this))
                        d << SkillInvokeDetail(this, p, p, nullptr, true);
                }
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *player = invoke->invoker;
        room->touhouLogmessage("#TriggerSkill", player, "here");
        room->notifySkillInvoked(player, objectName());
        if (use.from != nullptr)
            room->touhouLogmessage("#HereFilter", use.from, "here");

        FireSlash *new_slash = new FireSlash(use.card->getSuit(), use.card->getNumber());
        if (use.card->getSubcards().length() > 0)
            new_slash->addSubcards(use.card->getSubcards());
        else { //use.from is ai...
            int id = use.card->getEffectiveId();
            if (id > -1)
                new_slash->addSubcard(id);
        }

        //remain the information of original card
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if ((damage.from != nullptr) && damage.from->isAlive() && damage.to->isAlive() && damage.from != damage.to && damage.to->hasSkill(this)) {
            FireSlash *slash = new FireSlash(Card::NoSuit, 0);
            slash->deleteLater();
            if (!damage.to->isCardLimited(slash, Card::MethodUse) && damage.to->canSlash(damage.from, slash, false))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, nullptr, false, damage.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        QString prompt = "target:" + invoke->preferredTarget->objectName();
        invoke->invoker->tag["yuanling"] = QVariant::fromValue(invoke->preferredTarget);
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        FireSlash *slash = new FireSlash(Card::NoSuit, 0);
        slash->setSkillName("_" + objectName());
        room->useCard(CardUseStruct(slash, invoke->invoker, invoke->targets.first()), false);
        return false;
    }
};

SongzangCard::SongzangCard()
{
    target_fixed = true;
}

void SongzangCard::onEffect(const CardEffectStruct &effect) const
{
    DamageStruct d;
    d.from = effect.from;
    d.to = effect.to;
    effect.from->getRoom()->killPlayer(effect.to, &d);
}

class Songzang : public OneCardViewAsSkill
{
public:
    Songzang()
        : OneCardViewAsSkill("songzang")
    {
        filter_pattern = ".|spade!";
    }

    bool isEnabledAtPlay(const Player *) const override
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *p, const QString &pattern) const override
    {
        if (p->property("currentdying").toString().isEmpty())
            return false;

        if (p->property("currentdying").toString() == p->objectName())
            return false;

        return pattern.contains("peach");
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        SongzangCard *card = new SongzangCard;
        card->addSubcard(originalCard);
        return card;
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.to->isAlive() && effect.jink != nullptr && effect.from->hasSkill(this) && effect.from->canDiscard(effect.from, "hs"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.from, effect.from);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        effect.from->tag["guaili_target"] = QVariant::fromValue(effect.to);
        const Card *card = room->askForCard(effect.from, ".|red|.|hand", "@guaili:" + effect.to->objectName(), data, objectName());
        if (card != nullptr)
            return true;
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        effect.from->drawCards(1);
        room->slashResult(effect, nullptr);
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
    return card;
}

class JiuhaoVS : public ZeroCardViewAsSkill
{
public:
    JiuhaoVS()
        : ZeroCardViewAsSkill("jiuhao")
    {
    }

    bool isEnabledAtPlay(const Player *player) const override
    {
        return player->hasFlag("jiuhao") && !player->hasFlag("jiuhaoused");
    }

    const Card *viewAs() const override
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

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            QList<SkillInvokeDetail> d;
            if (use.card->isKindOf("Slash") && use.from != nullptr && use.from->isAlive()) {
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
            if ((damage.card != nullptr) && damage.card->hasFlag("jidu_card"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, nullptr, damage.from, nullptr, true, nullptr, false);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (e == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            QString prompt = "@jidu:" + use.from->objectName() + ":" + use.card->objectName();
            return room->askForCard(invoke->invoker, ".|.|.|.", prompt, data, objectName()) != nullptr;
        }
        return true;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        if (e == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.card->setFlags("jidu_card");
        } else if (e == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from != nullptr) {
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        if ((damage.card != nullptr) && damage.card->isKindOf("Slash") && (damage.from != nullptr) && damage.from->isAlive() && damage.to->hasSkill(this) && damage.to->isAlive()) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, nullptr, false, damage.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, damage.to->objectName(), damage.from->objectName());
        QString choice = room->askForChoice(damage.from, objectName(), "gelong1+gelong2");
        if (choice == "gelong1") {
            damage.from->drawCards(1);
            int max = qMin(damage.from->getCards("hes").length(), 2);
            if (max > 0) {
                //const Card *cards = room->askForExchange(damage.from, objectName(), max, max, true, "@gelong:" + damage.to->objectName() + ":" + QString::number(max));
                //room->obtainCard(damage.to, cards, false);

                QList<int> disable;
                DummyCard *dummy = new DummyCard;
                for (int i = 0; i < max; i += 1) {
                    int card_id = room->askForCardChosen(damage.to, damage.from, "hes", objectName(), false, Card::MethodNone, disable);

                    disable << card_id;
                    dummy->addSubcard(card_id);

                    if (damage.from->getCards("hes").length() - disable.length() <= 0)
                        break;
                }

                CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, damage.to->objectName());
                room->obtainCard(damage.to, dummy, reason, false);
                delete dummy;
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
        events = {EventPhaseStart, Damage};
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const override
    {
        if (e == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            QList<ServerPlayer *> yamames = room->findPlayersBySkillName(objectName());
            if (yamames.isEmpty() || damage.nature != DamageStruct::Normal || (damage.from == nullptr) || damage.from->isDead() || damage.to->isChained())
                return {};

            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *p, yamames) {
                if ((p == damage.from || damage.from->isChained()) && damage.to != p)
                    d << SkillInvokeDetail(this, p, p, nullptr, true, damage.to);
            }
            return d;
        } else if (e == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current->getPhase() == Player::Start && current->isAlive() && current->hasSkill(this)) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->isChained())
                        return {SkillInvokeDetail(this, current, current, nullptr, true)};
                }
            }
        }

        return {};
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        if (triggerEvent == EventPhaseStart) {
            QList<ServerPlayer *> ts;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (!p->isChained())
                    ts << p;
            }
            ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, ts, objectName(), "@chuanran-forceselect");
            if (target == nullptr) {
                qShuffle(ts);
                target = ts.first();
            }
            invoke->targets << target;
            return true;
        }

        return TriggerSkill::cost(triggerEvent, room, invoke, data);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
    {
        room->notifySkillInvoked(invoke->owner, objectName());
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current->getPhase() == Player::Finish && (!current->faceUp() || current->isChained())) {
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(current)) {
            if (current->inMyAttackRange(p))
                targets << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@rebing:" + current->objectName(), true, true);
        if (target != nullptr)
            invoke->targets << target;
        return target != nullptr;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->owner->objectName(), current->objectName());
        QString prompt = "@rebing-slash:" + invoke->invoker->objectName() + ":" + invoke->targets.first()->objectName();
        if (room->askForUseSlashTo(current, invoke->targets.first(), prompt) == nullptr) {
            if (!invoke->invoker->isNude()) {
                int id = room->askForCardChosen(invoke->invoker, current, "hes", objectName());
                room->obtainCard(invoke->invoker, id, false);
            }
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash") || use.from == nullptr || use.from->isDead())
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

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

            if (skillowner->pindian(use.from, "diaoping", nullptr)) {
                good_result = true;
                break;
            }
        }
        return good_result;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.from->turnOver();
        use.nullified_list << "_ALL_TARGETS";
        data = QVariant::fromValue(use);
        return false;
    }
};

class Tongju : public TriggerSkill
{
public:
    Tongju()
        : TriggerSkill("tongju")
    {
        events << TargetConfirming;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("SavageAssault") || use.card->isKindOf("IronChain") || use.card->isKindOf("ArcheryAttack")) {
            foreach (ServerPlayer *p, use.to) {
                if (p->hasSkill(this))
                    d << SkillInvokeDetail(this, p, p, nullptr, true);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
    {
        room->notifySkillInvoked(invoke->invoker, objectName());

        CardUseStruct use = data.value<CardUseStruct>();
        use.to.removeAll(invoke->invoker);
        data = QVariant::fromValue(use);
        LogMessage log;
        log.type = "#SkillAvoid";
        log.from = invoke->invoker;
        log.arg = objectName();
        log.arg2 = use.card->objectName();
        room->sendLog(log);

        return false;
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
            CardsMoveStruct move(id, nullptr, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName()));
            move.reason.m_skillName = "cuiji";
            room->moveCardsAtomic(move, true);
            room->getThread()->delay();
            Card *card = Sanguosha->getCard(id);
            if (card->isRed() == isred) {
                acquired = acquired + 1;
                CardsMoveStruct move2(id, player, Player::PlaceHand, CardMoveReason(CardMoveReason::S_REASON_GOTBACK, player->objectName()));
                room->moveCardsAtomic(move2, true);

                if (!throwIds.isEmpty()) {
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "cuiji", QString());
                    DummyCard dummy(throwIds);
                    room->throwCard(&dummy, reason, nullptr);
                    throwIds.clear();
                }
            } else
                throwIds << id;
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DrawNCardsStruct s = data.value<DrawNCardsStruct>();
        if (s.player->hasSkill(this) && s.n > 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, s.player, s.player);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const override
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

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &data) const override
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

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const override
    {
        DrawNCardsStruct dc = data.value<DrawNCardsStruct>();
        int num = dc.player->tag["cuiji"].toInt();
        if (num > 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dc.player, dc.player, nullptr, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const override
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

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        SavageAssault *card = new SavageAssault(Card::SuitToBeDecided, -1);
        DELETE_OVER_SCOPE(SavageAssault, card)
        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);

        return cardPattern != nullptr && cardPattern->match(player, card);
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        SavageAssault *sa = new SavageAssault(Card::SuitToBeDecided, -1);
        sa->addSubcard(originalCard);
        sa->setSkillName(objectName());
        return sa;
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

    bool isEnabledAtPlay(const Player *player) const override
    {
        return Analeptic::IsAvailable(player);
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const override
    {
        Analeptic *card = new Analeptic(Card::SuitToBeDecided, -1);
        DELETE_OVER_SCOPE(Analeptic, card)
        const CardPattern *cardPattern = Sanguosha->getPattern(pattern);

        return cardPattern != nullptr && cardPattern->match(player, card) && Sanguosha->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE;
    }

    const Card *viewAs(const Card *originalCard) const override
    {
        Analeptic *analeptic = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        analeptic->addSubcard(originalCard);
        analeptic->setSkillName(objectName());
        return analeptic;
    }
};

TH11Package::TH11Package()
    : Package("th11")
{
    General *satori = new General(this, "satori$", "dld", 3);
    satori->addSkill(new Xiangqi);
    //Room::askForCardChosen
    satori->addSkill(new Skill("duxin", Skill::Compulsory, "static"));
    satori->addSkill(new Skill("youtong$"));

    General *koishi = new General(this, "koishi", "dld", 3);
    koishi->addSkill(new Maihuo);
    koishi->addSkill(new Wunian);

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
    addMetaObject<SongzangCard>();
}

ADD_PACKAGE(TH11)
