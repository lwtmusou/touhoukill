#include "thxwm.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"


class Kongpiao : public TriggerSkill
{
public:
    Kongpiao() : TriggerSkill("kongpiao")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Play) {
            foreach (ServerPlayer *satori, room->findPlayersBySkillName(objectName())) {
                if (satori->getHandcardNum() < 5 && player->getHandcardNum() > satori->getHandcardNum())
                    d << SkillInvokeDetail(this, satori, satori, NULL, true);
            }
        }
        return d;
    }


    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *satori = invoke->invoker;
        room->touhouLogmessage("#TriggerSkill", satori, objectName());
        room->notifySkillInvoked(satori, objectName());
        satori->drawCards(1);
        return false;
    }
};

ShouhuiCard::ShouhuiCard()
{
    target_fixed = true;
    will_throw = true;
}

void ShouhuiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    if (source->isAlive())
        room->drawCards(source, subcards.length());
}

class Shouhui : public ViewAsSkill
{
public:
    Shouhui() : ViewAsSkill("shouhui")
    {
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return (pattern.contains("peach") && player->hasFlag("Global_Dying"));
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0) {
            ShouhuiCard *card = new ShouhuiCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;

    }
};


WoyuCard::WoyuCard()
{
}

void WoyuCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->doLightbox("$woyuAnimate", 4000);
    SkillCard::onUse(room, card_use);
}

bool WoyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return(targets.isEmpty() && !to_select->hasShownRole() && to_select != Self);
}

void WoyuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->removePlayerMark(source, "@woyu");
    ServerPlayer *target = targets.first();
    QString role = target->getRole();
    room->touhouLogmessage("#WoyuAnnounce", source, role, room->getAllPlayers(), target->getGeneralName());
    room->broadcastProperty(target, "role");
    room->setPlayerProperty(target, "role_shown", true); //important! to notify client

    if (role == "rebel")
        source->drawCards(3);
    else if (role == "loyalist")
        source->throwAllHandCardsAndEquips();
}

class Woyu : public ZeroCardViewAsSkill
{
public:
    Woyu() : ZeroCardViewAsSkill("woyu$")
    {
        frequency = Limited;
        limit_mark = "@woyu";
    }

    virtual const Card *viewAs() const
    {
        return new WoyuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasLordSkill(objectName(), false))
            return player->getMark("@woyu") >= 1;
        return false;
    }
};




class Beisha : public TriggerSkill
{
public:
    Beisha() :TriggerSkill("beisha")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getPhase() == Player::Start && !player->isKongcheng()) {
            int num = player->getHandcardNum();
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getHandcardNum() <= num && player->canSlash(p, NULL, false)) {
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
                }
                if (p->getHandcardNum() <= (num / 2)) {
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<ServerPlayer *> listt;
        int num = invoke->invoker->getHandcardNum();
        foreach(ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (p->getHandcardNum() <= num && invoke->invoker->canSlash(p, NULL, false))
                listt << p;
            else if (p->getHandcardNum() <= (num / 2))
                listt << p;
        }

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, listt, objectName(), "@beisha", true, true);
        if (target != NULL)
        invoke->targets << target;
        return target != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = invoke->targets.first();
        int num = invoke->invoker->getHandcardNum();
        QStringList choices;
        if (target->getHandcardNum() <= num && invoke->invoker->canSlash(target, NULL, false))
            choices << "useslash";
        if (target->getHandcardNum() <= (num / 2))
            choices << "losehp";

        QString choice = room->askForChoice(invoke->invoker, objectName(), choices.join("+"));
        if (choice == "useslash") {
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName("_" + objectName());
            room->useCard(CardUseStruct(slash, invoke->invoker, target));
        } else
            room->loseHp(target);

        return false;
    }
};

class Xisan : public TriggerSkill
{
public:
    Xisan() : TriggerSkill("xisan")
    {
        events << EventPhaseStart;

    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() != Player::Start)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *marisa, room->findPlayersBySkillName(objectName())) {
            d << SkillInvokeDetail(this, marisa, marisa);
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *marisa = invoke->invoker;
        int hd_n = marisa->getHandcardNum();
        QString choice = room->askForChoice(marisa, objectName(), "a+b");
        if (choice == "a")
            marisa->drawCards(3 - hd_n);
        else {
            room->askForDiscard(marisa, objectName(), hd_n, 1, true, false, "@xisan-discard");
            int newhd_n = marisa->getHandcardNum();
            marisa->drawCards(2 - newhd_n);
        }
        return false;
    }
};



class Jubao : public MasochismSkill
{
public:
    Jubao() : MasochismSkill("jubao")
    {

    }

    QList<SkillInvokeDetail> triggerable(const Room *room, const DamageStruct &damage) const
    {
        if (!damage.to->hasSkill(this) || damage.to->isDead())
            return QList<SkillInvokeDetail>();
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (damage.to->canDiscard(p, "h"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getAllPlayers()) {
            if (damage.to->canDiscard(p, "h"))
                targets << p;
        }

        ServerPlayer *victim = room->askForPlayerChosen(damage.to, targets, objectName(), "@jubao-select", true, true);
        if (victim) {
            int id = room->askForCardChosen(damage.to, victim, "h", objectName(), false, Card::MethodDiscard);
            room->throwCard(id, victim, damage.to);
            invoke->targets << victim;
            damage.to->tag["jubao_id"] = QVariant::fromValue(id);
            return true;
        }
        return false;
    }

    void onDamaged(Room *room, QSharedPointer<SkillInvokeDetail> invoke, const DamageStruct &) const
    {
        ServerPlayer *victim = invoke->targets.first();
        int id = invoke->invoker->tag["jubao_id"].toInt();
        invoke->invoker->tag.remove("jubao_id");
        DummyCard dummy;
        int count = 0;
        foreach(const Card *c, victim->getEquips()) {
            if (invoke->invoker->canDiscard(victim, c->getEffectiveId()) && c->isRed() == Sanguosha->getCard(id)->isRed()) {
                dummy.addSubcard(c);
                count = count + 1;
            }
        }
        if (count > 0)
            room->throwCard(&dummy, victim, invoke->invoker);
    }

};

class Haidi : public TriggerSkill
{
public:
    Haidi() : TriggerSkill("haidi")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
        if (player != NULL && player->isAlive() && player->hasSkill(this) && move.from_places.contains(Player::PlaceHand)
            && player->isWounded() && move.is_last_handcard)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        RecoverStruct recover;
        recover.who = invoke->invoker;
        room->recover(invoke->invoker, recover);
        return false;
    }
};



class Shanji : public TriggerSkill
{
public:
    Shanji() : TriggerSkill("shanji")
    {
        events << DrawInitialCards << AfterDrawInitialCards << CardsMoveOneTime;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
         if (triggerEvent == CardsMoveOneTime) {
             CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
             ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
             if (player != NULL && move.reason.m_skillName != "xijian" && player->hasSkill("shanji")) {
                 int count = 0;
                 for (int i = 0; i < move.card_ids.size(); i++) {
                     if (move.from_pile_names[i] == "piao") count++;
                 }
                 if (count > 0) {
                     room->notifySkillInvoked(player, "shanji");
                     LogMessage log;
                     log.type = "#shanji";
                     log.from = player;
                     log.arg = QString::number(count);
                     log.arg2 = "piao";
                     room->sendLog(log);
                 }
             }
         }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == DrawInitialCards || triggerEvent == AfterDrawInitialCards) {
            DrawNCardsStruct s = data.value<DrawNCardsStruct>();
            if (s.player->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, s.player, s.player, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = invoke->invoker;
        if (triggerEvent == DrawInitialCards) {
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            DrawNCardsStruct s = data.value<DrawNCardsStruct>();
            s.n = s.n + 6;
            data = QVariant::fromValue(s);
        } else if ((triggerEvent == AfterDrawInitialCards)) {
            room->broadcastSkillInvoke("shanji");
            const Card *exchange_card = room->askForExchange(player, "shanji", 6, 6);
            DELETE_OVER_SCOPE(const Card, exchange_card)
            player->addToPile("piao", exchange_card->getSubcards(), false);
        }
        return false;
    }
};

YazhiCard::YazhiCard()
{
    will_throw = false;
    target_fixed = true;
}

void YazhiCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->addToPile("piao", subcards, false);
}

class Yazhi : public OneCardViewAsSkill
{
public:
    Yazhi() :OneCardViewAsSkill("yazhi")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("YazhiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        YazhiCard *card = new YazhiCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class Tianxiang : public TriggerSkill
{
public:
    Tianxiang() : TriggerSkill("tianxiang$")
    {
        events << GameStart;
        frequency = Compulsory;
    }

    void record(TriggerEvent, Room *room, QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (!player) return;
        if (player->hasLordSkill(objectName()) && player->isLord()) {
            room->notifySkillInvoked(player, objectName());
            foreach(ServerPlayer *p, room->getAllPlayers())
                room->setPlayerMark(p, "tianxiang", 1);
        }
    }
};



class Qingcang : public TriggerSkill
{
public:
    Qingcang() : TriggerSkill("qingcang")
    {
        events << DrawNCards << AfterDrawNCards;

    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        DrawNCardsStruct s = data.value<DrawNCardsStruct>();
        if (triggerEvent == DrawNCards) {
            if (s.player->hasSkill(this) && s.player->getCards("j").isEmpty())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, s.player, s.player);
        }
        else if (triggerEvent == AfterDrawNCards && s.player->hasFlag("qingcangUsed") && !s.player->isKongcheng())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, s.player, s.player, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent triggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == DrawNCards)
            return invoke->invoker->askForSkillInvoke("qingcang", data);
        return true;
    }


    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == DrawNCards) {
            DrawNCardsStruct s = data.value<DrawNCardsStruct>();
            s.n = s.n + 4;
            data = QVariant::fromValue(s);
            room->setPlayerFlag(invoke->invoker, "qingcangUsed");
        } else if (triggerEvent == AfterDrawNCards) {
            ServerPlayer *player = invoke->invoker;
            const Card *card = room->askForExchange(player, objectName(), 1, 1, false, "@qingcang-card");
            DELETE_OVER_SCOPE(const Card, card)

            Card *supplyshortage = Sanguosha->cloneCard("supply_shortage", card->getSuit(), card->getNumber());
            WrappedCard *vs_card = Sanguosha->getWrappedCard(card->getSubcards().first());
            vs_card->setSkillName("_qingcang");
            vs_card->takeOver(supplyshortage);
            room->broadcastUpdateCard(room->getAlivePlayers(), vs_card->getId(), vs_card);
            CardsMoveStruct  move;
            move.card_ids << vs_card->getId();
            move.to = player;
            move.to_place = Player::PlaceDelayedTrick;
            room->moveCardsAtomic(move, true);
        }
        return false;
    }
};


class Changqing : public TriggerSkill
{
public:
    Changqing() : TriggerSkill("changqing")
    {
        events << Dying;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *who = data.value<DyingStruct>().who;
        if (who->hasSkill(this) && room->getAlivePlayers().length() >= 5)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, who, who, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->recover(invoke->invoker, RecoverStruct());
        return false;
    }
};


THXWMPackage::THXWMPackage()
    : Package("thxwm")
{
    General *satori_xwm = new General(this, "satori_xwm$", "dld", 3, false);
    satori_xwm->addSkill(new Kongpiao);
    satori_xwm->addSkill(new Shouhui);
    satori_xwm->addSkill(new Woyu);


    General *yuyuko_xwm = new General(this, "yuyuko_xwm", "yym", 4, false);
    yuyuko_xwm->addSkill(new Beisha);

    General *marisa_xwm = new General(this, "marisa_xwm", "zhu", 4, false);
    marisa_xwm->addSkill(new Xisan);

    General *aya_xwm = new General(this, "aya_xwm", "zhan", 3, false);
    aya_xwm->addSkill(new Jubao);
    aya_xwm->addSkill(new Haidi);

    General *mokou_xwm = new General(this, "mokou_xwm$", "yyc", 3, false);
    mokou_xwm->addSkill(new Shanji);
    mokou_xwm->addSkill(new Yazhi);
    mokou_xwm->addSkill(new Tianxiang);


    General *remilia_xwm = new General(this, "remilia_xwm", "hmx", 3, false);
    remilia_xwm->addSkill(new Qingcang);
    remilia_xwm->addSkill(new Changqing);

    addMetaObject<ShouhuiCard>();
    addMetaObject<YazhiCard>();
    addMetaObject<WoyuCard>();
}

ADD_PACKAGE(THXWM)

