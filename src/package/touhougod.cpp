#include "compiler-specific.h"
#include "touhougod.h"

#include "general.h"
#include "settings.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"
#include "maneuvering.h" // for iceslash
#include "th10.h" //for huaxiang

class Jiexian : public TriggerSkill
{
public:
    Jiexian() : TriggerSkill("jiexian")
    {
        events << DamageInflicted << PreHpRecover;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *room, const QVariant &) const
    {
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *yukari, room->findPlayersBySkillName(objectName())) {
            if (yukari->canDiscard(yukari, "hs"))
                d << SkillInvokeDetail(this, yukari, yukari);
            else { //in order to reduce frequency of client request
                foreach (const Card *c, yukari->getCards("e")) {
                    if (yukari->canDiscard(yukari, c->getEffectiveId()) && event == DamageInflicted &&  c->getSuit() == Card::Heart) {
                        d << SkillInvokeDetail(this, yukari, yukari);
                        break;
                    } else if (yukari->canDiscard(yukari, c->getEffectiveId()) && event == PreHpRecover &&  c->getSuit() == Card::Spade) {
                        d << SkillInvokeDetail(this, yukari, yukari);
                        break;
                    }
                }
            }

        }
        return d;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        const Card *card = NULL;
        ServerPlayer *source = invoke->invoker;
        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            source->tag["jiexian_target"] = QVariant::fromValue(damage.to);
            card = room->askForCard(source, "..H", "@jiexiandamage:" + damage.to->objectName(), data, objectName());
        } else if (triggerEvent == PreHpRecover) {
            RecoverStruct r = data.value<RecoverStruct>();
            source->tag["jiexian_target"] = QVariant::fromValue(r.to);
            card = room->askForCard(source, "..S", "@jiexianrecover:" + r.to->objectName(), QVariant::fromValue(r.to), objectName());
        }
        return card != NULL;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *source = invoke->invoker;
        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), damage.to->objectName());
            room->touhouLogmessage("#jiexiandamage", damage.to, objectName(), QList<ServerPlayer *>(), QString::number(damage.damage));
            if (damage.to->isWounded()) {
                RecoverStruct recover;
                recover.who = source;
                recover.reason = objectName();
                room->recover(damage.to, recover);
            }
            return true;
        } else if (triggerEvent == PreHpRecover) {
            RecoverStruct r = data.value<RecoverStruct>();
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), r.to->objectName());
            room->touhouLogmessage("#jiexianrecover", r.to, objectName(), QList<ServerPlayer *>(), QString::number(data.value<RecoverStruct>().recover));
            room->damage(DamageStruct(objectName(), NULL, r.to));
            return true;
        }
        return false;
    }
};

class Zhouye : public TriggerSkill
{
public:
    Zhouye() : TriggerSkill("zhouye")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseStart << PreMarkChange << EventSkillInvalidityChange;
        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        ServerPlayer *player;
        bool set = false;
        bool remove = false;
        
        if (triggerEvent == EventLoseSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            player = a.player;
            if (a.skill->objectName() == "zhouye")
                remove = true;
        } else if (triggerEvent == EventAcquireSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            player = a.player;
            if (a.skill->objectName() ==  "zhouye" && a.player->getMark("@ye") == 0)
               set = true;
        } else if (triggerEvent == GameStart) {
            player = data.value<ServerPlayer *>();
            if (player && player->hasSkill("zhouye"))
                set = true;
        } else if (triggerEvent == EventSkillInvalidityChange) {
            QList<SkillInvalidStruct>invalids = data.value<QList<SkillInvalidStruct>>();
            foreach(SkillInvalidStruct v, invalids) {
                if (!v.skill || v.skill->objectName() == "zhouye") {
                    player = v.player;
                    if (!v.invalid && v.player->getMark("@ye") == 0 && v.player->hasSkill(this))
                        set = true;
                    else if (v.invalid)
                        remove = true;
                }
            }

        } else if (triggerEvent == PreMarkChange) {
            MarkChangeStruct change = data.value<MarkChangeStruct>();
            if (change.name != "@ye")
                return;
            player = change.player;
            int mark = change.player->getMark("@ye");
            if (mark > 0 && (mark + change.num == 0) && change.player->hasSkill(this))
                set = true;
            else if (mark == 0 && (mark + change.num > 0))
                remove = true;
        }

        if (player != NULL) {
            if (set && player->getMark("zhouye_limit") == 0) {
                room->setPlayerCardLimitation(player, "use", "Slash", false);
                room->setPlayerMark(player, "zhouye_limit", 1);
            } else if (remove && player->getMark("zhouye_limit") > 0) {
                room->removePlayerCardLimitation(player, "use", "Slash$0");
                room->setPlayerMark(player, "zhouye_limit", 0);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Start && player->hasSkill("zhouye"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        if (player->getMark("@ye") > 0)
            player->loseAllMarks("@ye");

        QList<int> idlist = room->getNCards(1);
        int cd_id = idlist.first();
        room->fillAG(idlist, NULL);
        room->getThread()->delay();

        room->clearAG();
        Card *card = Sanguosha->getCard(cd_id);
        if (card->isBlack())
            player->gainMark("@ye", 1);
        room->throwCard(cd_id, NULL);

        return false;
    }
};


HongwuCard::HongwuCard()
{
    will_throw = true;
    target_fixed = true;
}

void HongwuCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->gainMark("@ye", 1);
}

class Hongwu : public ViewAsSkill
{
public:
    Hongwu() : ViewAsSkill("hongwu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return  player->getMark("@ye") == 0;
    }


    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return to_select->isRed() && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            HongwuCard *card = new HongwuCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;

    }
};

ShenqiangCard::ShenqiangCard()
{
    will_throw = true;
}

void ShenqiangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    room->damage(DamageStruct("shenqiang", source, target));
}

class Shenqiang : public OneCardViewAsSkill
{
public:
    Shenqiang() :OneCardViewAsSkill("shenqiang")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@ye") > 0;
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return (to_select->isKindOf("Weapon") || to_select->getSuit() == Card::Heart)
            && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ShenqiangCard *card = new ShenqiangCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class Yewang : public TriggerSkill
{
public:
    Yewang() : TriggerSkill("yewang")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->getMark("@ye") > 0 && damage.to->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, true);
        return QList<SkillInvokeDetail>();
    }



    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        damage.damage = damage.damage - 1;
        room->touhouLogmessage("#YewangTrigger", invoke->invoker, objectName(), QList<ServerPlayer *>(), QString::number(1));
        room->notifySkillInvoked(invoke->invoker, objectName());
        data = QVariant::fromValue(damage);
        if (damage.damage == 0)
            return true;

        return false;
    }
};




class AoyiEffect : public TriggerSkill
{
public:
    AoyiEffect() : TriggerSkill("#aoyi")
    {
        events << GameStart << CardUsed << EventAcquireSkill << EventLoseSkill << EventSkillInvalidityChange;

        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        ServerPlayer *player;
        bool set = false;
        bool remove = false;
        
        
        if (triggerEvent == GameStart) {
            player = data.value<ServerPlayer *>();
            if (player && player->hasSkill("aoyi"))
                set = true;
        } else if (triggerEvent == EventLoseSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            player = a.player;
            if (a.skill->objectName() == "aoyi")
                remove = true;
        } else if (triggerEvent == EventAcquireSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            player = a.player;
            if (a.skill->objectName() == "aoyi")
                set = true;
        } else if (triggerEvent == EventSkillInvalidityChange) {
            QList<SkillInvalidStruct>invalids = data.value<QList<SkillInvalidStruct>>();
            foreach(SkillInvalidStruct v, invalids) {
                if (!v.skill || v.skill->objectName() == "aoyi") {
                    player = v.player;
                    if (!v.invalid  && v.player->hasSkill(this))
                        set = true;
                    else if (v.invalid)
                        remove = true;
                }
            }

        }

        if (player != NULL) {
            if (set && player->getMark("aoyi_limit") == 0) {
                room->setPlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick", false);
                room->setPlayerMark(player, "aoyi_limit", 1);
            }
            else if (remove && player->getMark("aoyi_limit") > 0) {
                room->removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0");
                room->setPlayerMark(player, "aoyi_limit", 0);
            }
        }

    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("IceSlash") && use.from->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        QStringList choices;
        choices << "aoyi1";
        QList<ServerPlayer *> all;
        foreach(ServerPlayer *p, room->getAlivePlayers()) {
            if (player->canDiscard(p, "ej"))
                all << p;
        }
        if (all.length() > 0)
            choices << "aoyi2";
        QString choice = room->askForChoice(player, "aoyi", choices.join("+"));
        if (choice == "aoyi1") {
            room->touhouLogmessage("#InvokeSkill", player, "aoyi");
            room->notifySkillInvoked(player, objectName());
            player->drawCards(player->getLostHp());
        }
        else {

            ServerPlayer *s = room->askForPlayerChosen(player, all, "aoyi", "aoyi_chosenplayer", false, true);
            int to_throw = room->askForCardChosen(player, s, "ej", "aoyi", false, Card::MethodDiscard);
            room->throwCard(to_throw, s, player);
        }

        return false;
    }
};

class Aoyi : public FilterSkill
{
public:
    Aoyi() : FilterSkill("aoyi")
    {
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return  to_select->isNDTrick();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IceSlash *slash = new IceSlash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class AoyiTargetMod : public TargetModSkill
{
public:
    AoyiTargetMod() : TargetModSkill("#aoyi_mod")
    {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *card) const
    {
        if (from->hasSkill("aoyi") && card->isKindOf("IceSlash"))
            return 1000;
        else
            return 0;
    }

};



class Shikong : public TargetModSkill
{
public:
    Shikong() : TargetModSkill("shikong")
    {
        pattern = "Slash";
    }

    static int shikong_modNum(const Player *player, const Card *slash)
    {

        int num = 0;
        int rangefix = 0;
        QList<int> ids = slash->getSubcards();
        if (player->getWeapon() != NULL && ids.contains(player->getWeapon()->getId())) {
            if (player->getAttackRange() > player->getAttackRange(false))
                rangefix = rangefix + player->getAttackRange() - player->getAttackRange(false);
        }
        if (player->getOffensiveHorse() != NULL
            && ids.contains(player->getOffensiveHorse()->getId()))
            rangefix = rangefix + 1;

        foreach (const Player *p, player->getAliveSiblings()) {
            if ((player->inMyAttackRange(p) && player->canSlash(p, slash, true, rangefix))
                || Slash::IsSpecificAssignee(p, player, slash))
                num = num + 1;
        }
        return qMax(1, num);
    }

    virtual int getExtraTargetNum(const Player *player, const Card *card) const
    {
        if (player->hasSkill(objectName()) && player->getPhase() == Player::Play && card->isKindOf("Slash"))
            return shikong_modNum(player, card) - 1;
        else
            return 0;
    }
};

class Ronghui : public TriggerSkill
{
public:
    Ronghui() : TriggerSkill("ronghui")
    {
        events << DamageCaused;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user)
            return QList<SkillInvokeDetail>();

        if (damage.from && damage.from != damage.to && damage.from->getPhase() == Player::Play && damage.from->hasSkill(this)
            && damage.card && damage.card->isKindOf("Slash") && damage.from->canDiscard(damage.to, "e"))
                 return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true);
            return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        DummyCard *dummy = new DummyCard;
        dummy->deleteLater();
        foreach (const Card *c, damage.to->getCards("e")) {
            if (damage.from->canDiscard(damage.to, c->getEffectiveId())) {
                dummy->addSubcard(c);
            }
        }
        if (dummy->subcardsLength() > 0) {
            room->touhouLogmessage("#TriggerSkill", damage.from, objectName());
            room->notifySkillInvoked(damage.from, objectName());
            room->throwCard(dummy, damage.to, damage.from);
        }
        return false;
    }
};

class Jubian : public TriggerSkill
{
public:
    Jubian() : TriggerSkill("jubian")
    {
        events << Damage << CardFinished;
        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *, QVariant &data) const
    {
        if (triggerEvent == Damage) {
             DamageStruct damage = data.value<DamageStruct>();
             if (damage.card != NULL && damage.from && damage.from->getPhase() == Player::Play) {
                 if (damage.card->hasFlag("jubian_card")) {
                     if (!damage.card->hasFlag("jubian_used"))
                         damage.card->setFlags("jubian_used");
                 }
                 else
                     damage.card->setFlags("jubian_card");
             }
        }
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
       if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->hasFlag("jubian_card") && use.card->hasFlag("jubian_used")
                && use.from->hasSkill(this) && use.from->getPhase() == Player::Play)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, true);
        }
       return QList<SkillInvokeDetail>();
    }


    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        const Card *card = data.value<CardUseStruct>().card;
        if (card->hasFlag("jubian_card") && card->hasFlag("jubian_used")) {
            room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
            room->notifySkillInvoked(invoke->invoker, objectName());

            RecoverStruct recov;
            recov.who = invoke->invoker;
            recov.recover = 1;
            room->recover(invoke->invoker, recov);
        }
        return false;
    }
};

class Hengxing : public TriggerSkill
{
public:
    Hengxing() : TriggerSkill("hengxing")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getPhase() == Player::Finish)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->loseHp(invoke->invoker);
        invoke->invoker->drawCards(3);
        return false;
    }
};




class Huanmeng : public TriggerSkill
{
public:
    Huanmeng() : TriggerSkill("huanmeng")
    {
        events << GameStart << PreHpLost << EventPhaseStart << EventPhaseChanging;
        frequency = Eternal;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == GameStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player && player->hasSkill(this)) {
                room->setPlayerProperty(player, "maxhp", 0);
                room->setPlayerProperty(player, "hp", 0);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == PreHpLost) {
            HpLostStruct hplost = data.value<HpLostStruct>();
            if (hplost.player->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, hplost.player, hplost.player, NULL, true);
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->getPhase() == Player::RoundStart && player->getHandcardNum() == 0)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.player->hasSkill(this) && change.to == Player::Draw)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, NULL, true);
            else if (change.player->hasSkill(this) && change.to == Player::Discard)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }


    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = invoke->invoker;
        if (triggerEvent == PreHpLost) {
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            return true;
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getHandcardNum() == 0) {
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
                room->killPlayer(player);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Draw) {
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
                player->skip(Player::Draw);
            }
            if (change.to == Player::Discard) {
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
                player->skip(Player::Discard);
            }
        }
        return false;
    }
};

class Cuixiang : public TriggerSkill
{
public:
    Cuixiang() : TriggerSkill("cuixiang")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getPhase() == Player::Start)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        QList<int> idlist;
        foreach(ServerPlayer *p, room->getOtherPlayers(player))
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());

        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->canDiscard(p, "hs")) {
                int id = -1;
                //auto throw
                if (p->getHandcardNum() == 1)
                    id = p->getCards("hs").first()->getEffectiveId();
                else {
                    const Card *cards = room->askForExchange(p, objectName(), 1, 1, false, "cuixiang-exchange:" + player->objectName() + ":" + objectName());
                    DELETE_OVER_SCOPE(const Card, cards)
                    id = cards->getSubcards().first();
                }
                room->throwCard(id, p);
                idlist << id;
            } else {
                QList<int> cards = room->getNCards(1);
                room->throwCard(cards.first(), NULL, p);
                idlist << cards.first();
            }
        }

        //we need id to check cardplace,
        //since skill "jinian",  the last handcard will be return.
        QList<int> able;
        foreach(int id, idlist) {
            if (room->getCardPlace(id) == Player::DiscardPile)
                able << id;
        }

        int x = qMin(idlist.length(), 2);
        if (x == 0)
            return false;
        room->fillAG(able, NULL);
        for (int i = 0; i < x; i++) {

            int card_id = room->askForAG(player, able, false, "cuixiang");
            //just for displaying chosen card in ag container
            room->takeAG(player, card_id, false);
            room->obtainCard(player, card_id, true);
            able.removeOne(card_id);
        }
        room->clearAG();
        return false;
    }
};

class Xuying : public TriggerSkill
{
public:
    Xuying() : TriggerSkill("xuying")
    {
        events << SlashHit << SlashMissed;
        frequency = Eternal;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (triggerEvent == SlashHit && effect.to->hasSkill(objectName())) {
            if (effect.to->getHandcardNum() > 0)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, NULL, true);
        }else if (triggerEvent == SlashMissed && effect.to->hasSkill(objectName()))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (triggerEvent == SlashHit) {
                int x = qMax(effect.to->getHandcardNum() / 2, 1);
                room->touhouLogmessage("#TriggerSkill", effect.to, objectName());
                room->notifySkillInvoked(effect.to, objectName());
                room->askForDiscard(effect.to, "xuying", x, x, false, false, "xuying_discard:" + QString::number(x));
        } else if (triggerEvent == SlashMissed) {
            room->touhouLogmessage("#TriggerSkill", effect.to, objectName());
            room->notifySkillInvoked(effect.to, objectName());
            effect.to->drawCards(1);
        }
        return false;
    }
};


class Kuangyan : public TriggerSkill
{
public:
    Kuangyan() : TriggerSkill("kuangyan")
    {
        events << Dying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *who = data.value<DyingStruct>().who;
        ServerPlayer *current = room->getCurrent();
        if (!current || !current->isAlive() || who == current || !who->hasSkill(this))
            return QList<SkillInvokeDetail>();
        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, who, who, NULL, true, current);
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        return invoke->invoker->askForSkillInvoke(objectName(), "recover:" + invoke->preferredTarget->objectName());
    }


    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {

        ServerPlayer *current = invoke->targets.first();
        ServerPlayer *player = invoke->invoker;
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), current->objectName());

        player->gainMark("@kinki");
        RecoverStruct recover;
        recover.recover = player->dyingThreshold() - player->getHp();
        room->recover(player, recover);

        room->damage(DamageStruct(objectName(), player, current));
        return false;
    }
};

HuimieCard::HuimieCard()
{
}

bool HuimieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return(targets.isEmpty() && to_select != Self);
}

void HuimieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    source->gainMark("@kinki");
    if (!target->isChained())
        room->setPlayerProperty(target, "chained", true);

    room->damage(DamageStruct("huimie", source, target, 1, DamageStruct::Fire));
}

class Huimie : public ZeroCardViewAsSkill
{
public:
    Huimie() : ZeroCardViewAsSkill("huimie")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("HuimieCard");
    }

    virtual const Card *viewAs() const
    {
        return new HuimieCard;
    }
};

class Jinguo : public TriggerSkill
{
public:
    Jinguo() : TriggerSkill("jinguo")
    {
        events << EventPhaseEnd;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getPhase() == Player::Play && player->isAlive())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());

        JudgeStruct judge;
        judge.who = player;
        judge.good = true;
        judge.pattern = ".|spade";
        judge.negative = false;
        judge.play_animation = true;
        judge.reason = objectName();
        room->judge(judge);

        if (!judge.isGood()) {
            int x = player->getMark("@kinki");
            if (x == 0)
                return false;
            int y = x / 2;
            if (x > player->getCards("hes").length())
                room->loseHp(player, y);
            else {
                if (!room->askForDiscard(player, objectName(), x, x, true, true, "@jinguo:" + QString::number(x) + ":" + QString::number(y)))
                    room->loseHp(player, y);
            }
        } else
            player->skip(Player::Discard);
        return false;
    }
};



class Shicao : public TriggerSkill
{
public:
    Shicao() : TriggerSkill("shicao")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        QVariant extraTag = room->getTag("touhou-extra");
        bool nowExtraTurn = extraTag.canConvert(QVariant::Bool) && extraTag.toBool();
        if (player->hasSkill(this) && player->getPhase() == Player::Start && !nowExtraTurn)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        invoke->invoker->gainMark("@clock", 1);
        return false;
    }
};

class Shiting : public TriggerSkill
{
public:
    Shiting() : TriggerSkill("shiting")
    {
        events << EventPhaseChanging;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                bool extraTurnExist = room->getThread()->hasExtraTurn();
                if (!extraTurnExist) {
                     foreach (ServerPlayer *sakuya, room->findPlayersBySkillName(objectName())) {
                        if (sakuya->getMark("@clock") > 0)
                            d << SkillInvokeDetail(this, sakuya, sakuya);
                    }
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *current = room->getCurrent();
        ServerPlayer *next = current->getNextAlive();
        QString prompt = "extraturn:" + next->objectName();
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->loseAllMarks("@clock");
        invoke->invoker->gainAnExtraTurn();
        return false;
    }
};

class Huanzai : public TriggerSkill
{
public:
    Huanzai() : TriggerSkill("huanzai")
    {
        events << EventPhaseStart;
        frequency = Limited;
        limit_mark = "@huanzai";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getPhase() == Player::Finish) {
            if (player->getMark("@huanzai") > 0 && player->getMark("@clock") == 0)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->doLightbox("$huanzaiAnimate", 4000);
        invoke->invoker->gainMark("@clock", 1);
        room->removePlayerMark(invoke->invoker, "@huanzai");

        return false;
    }
};

class Shanghun : public TriggerSkill
{
public:
    Shanghun() : TriggerSkill("shanghun")
    {
        events << Damaged;
        frequency = Limited;
        limit_mark = "@shanghun";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->hasSkill(this) && damage.to->isAlive() && damage.to->getMark("@shanghun") > 0 && damage.to->getMark("@clock") == 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->doLightbox("$shanghunAnimate", 4000);
        invoke->invoker->gainMark("@clock", 1);
        room->removePlayerMark(invoke->invoker, "@shanghun");
        return false;
    }
};




class Banling : public TriggerSkill
{
public:
    Banling() : TriggerSkill("banling")
    {
        events << GameStart << PreHpLost << DamageInflicted << PreHpRecover;
        frequency = Eternal;
    }

    virtual int getPriority() const
    {
        return -1;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == GameStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player && player->hasSkill(this)) {
                room->setPlayerProperty(player, "renhp", player->getMaxHp());
                room->setPlayerProperty(player, "linghp", player->getMaxHp());
            }
        }
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == PreHpLost) {
            HpLostStruct hplost = data.value<HpLostStruct>();
            if (hplost.player->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, hplost.player, hplost.player, NULL, true);
        } else if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, true);
        } else if (triggerEvent == PreHpRecover) {
            RecoverStruct recov = data.value<RecoverStruct>();
            if (recov.to->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, recov.to, recov.to, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = invoke->invoker;
        if (triggerEvent == PreHpLost) {
            HpLostStruct hplost = data.value<HpLostStruct>();
            player->tag["banling_minus"] = QVariant::fromValue(hplost.num);
            for (int i = 0; i < hplost.num; i++) {
                QString choice = room->askForChoice(player, "banling_minus", "lingtili+rentili");
                if (choice == "lingtili") {
                    room->touhouLogmessage("#lingtilidamage", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                    room->setPlayerProperty(player, "linghp", player->getLingHp() - 1);

                } else if (choice == "rentili") {
                    room->touhouLogmessage("#rentilidamage", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                    room->setPlayerProperty(player, "renhp", player->getRenHp() - 1);
                }
            }

            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#LoseHp";
            log.from = player;
            log.arg = QString::number(hplost.num);
            room->sendLog(log);
            log.arg = QString::number(player->getHp());
            log.arg2 = QString::number(player->getMaxHp());
            log.type = "#GetHp";
            room->sendLog(log);

            room->getThread()->trigger(PostHpReduced, room, data);
            room->getThread()->trigger(PostHpLost, room, data);
            return true;
        } else if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            player->tag["banling_minus"] = QVariant::fromValue(damage.damage);
            for (int i = 0; i < damage.damage; i++) {
                QString choice = room->askForChoice(player, "banling_minus", "lingtili+rentili");
                if (choice == "lingtili") {
                    room->touhouLogmessage("#lingtilidamage", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                    room->setPlayerProperty(player, "linghp", player->getLingHp() - 1);
                }
                if (choice == "rentili") {
                    room->touhouLogmessage("#rentilidamage", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                    room->setPlayerProperty(player, "renhp", player->getRenHp() - 1);
                }
            }


            QVariant qdata = QVariant::fromValue(damage);
            room->getThread()->trigger(PreDamageDone, room, qdata);

            room->getThread()->trigger(DamageDone, room, qdata);

            room->notifySkillInvoked(player, objectName());

            room->getThread()->trigger(Damage, room, qdata);

            room->getThread()->trigger(Damaged, room, qdata);

            room->getThread()->trigger(DamageComplete, room, qdata);

            return true;
        } else if (triggerEvent == PreHpRecover) {
            RecoverStruct recov = data.value<RecoverStruct>();
            for (int i = 0; i < recov.recover; i++) {
                QString choice = "rentili";

                if (player->getLingHp() < player->getMaxHp() && player->getRenHp() < player->getMaxHp())
                    choice = room->askForChoice(player, "banling_plus", "lingtili+rentili");
                else {
                    if (player->getLingHp() == player->getMaxHp())
                        choice = "rentili";
                    else
                        choice = "lingtili";
                }
                if (choice == "rentili") {
                    room->setPlayerProperty(player, "renhp", player->getRenHp() + 1);
                    room->touhouLogmessage("#rentilirecover", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                    continue;
                }
                if (choice == "lingtili") {
                    room->setPlayerProperty(player, "linghp", player->getLingHp() + 1);
                    room->touhouLogmessage("#lingtilirecover", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                    continue;
                }
            }
            room->notifySkillInvoked(player, objectName());
        }
        return false;
    }
};

class Rengui : public PhaseChangeSkill
{
public:
    Rengui() :PhaseChangeSkill("rengui")
    {
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Start && player->hasSkill("banling")) {
            if (player->getLingHp() < player->getMaxHp())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            if (player->getRenHp() < player->getMaxHp())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        }
        return QList<SkillInvokeDetail>();
    }


    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        //effect 1
        if (player->getLingHp() < player->getMaxHp()) {
            int  x = qMin(player->getMaxHp() - player->getLingHp(), 2);
            ServerPlayer *s = room->askForPlayerChosen(player, room->getAlivePlayers(), "renguidraw", "@rengui-draw:" + QString::number(x), true, true);
            if (s) {
                room->notifySkillInvoked(player, objectName());
                s->drawCards(x);
            }
        }
        //effect 2
        if (player->getRenHp() < player->getMaxHp()) {
            int y = qMin(player->getMaxHp() - player->getRenHp(), 2);
            QList<ServerPlayer *> all;
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (player->canDiscard(p, "hes"))
                    all << p;
            }
            if (all.isEmpty())
                return false;
            ServerPlayer *s = room->askForPlayerChosen(player, all, "renguidiscard", "@rengui-discard:" + QString::number(y), true, true);
            if (s == NULL)
                return false;
            for (int i = 0; i < y; ++i) {
                if (!player->canDiscard(s, "hes"))
                    break;
                if (s == player)
                    room->askForDiscard(s, "rengui-discard", 1, 1, false, true);
                else {
                    int id = room->askForCardChosen(player, s, "hes", objectName(), false, Card::MethodDiscard);
                    room->throwCard(id, s, player);
                }
            }
        }

        return false;
    }
};



class Ningshi : public TriggerSkill
{
public:
    Ningshi() : TriggerSkill("ningshi")
    {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from->hasSkill(this)  && use.from->getPhase() == Player::Play
            && use.to.length() == 1 && !use.to.contains(use.from)) {
            if (use.card->isKindOf("Slash") || use.card->isKindOf("TrickCard"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        room->touhouLogmessage("#TriggerSkill", use.from, objectName());
        room->notifySkillInvoked(use.from, objectName());
        room->loseHp(use.to.first());
        return false;
    }
};

class Gaoao : public TriggerSkill
{
public:
    Gaoao() : TriggerSkill("gaoao")
    {
        events << BeforeCardsMove;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        if (room->getTag("FirstRound").toBool())
            return QList<SkillInvokeDetail>();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.to);
        if (player != NULL && player->hasSkill(this) && !player->isCurrent()
            && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip || move.to_place == Player::PlaceDelayedTrick))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *player = invoke->invoker;
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        QList <int> ids;
        foreach(int id, move.card_ids) {
            if (room->getCardPlace(id) != Player::DiscardPile)
                ids << id;
        }
        

        move.card_ids.clear();
        move.from_places.clear();
        move.from_pile_names.clear();

        data = QVariant::fromValue(move);

        if (!ids.isEmpty()) {
            bool isUse = ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE   && move.from != NULL);
                DummyCard dummy(ids);
                CardMoveReason reason(isUse ? CardMoveReason::S_REASON_USE : CardMoveReason::S_REASON_NATURAL_ENTER, 
                    isUse ? move.from->objectName() : player->objectName(), 
                    objectName(), QString());
                if (isUse) {
                    const Card *card = move.reason.m_extraData.value<const Card *>();
                    if (card)
                        reason.m_extraData = QVariant::fromValue(card);
                }
                room->throwCard(&dummy, reason, NULL);
        }
        return false;
    }
};



ShenshouCard::ShenshouCard()
{
    will_throw = false;
}

void ShenshouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();

    Card *card = Sanguosha->getCard(subcards.first());

    int x = 0;
    int y = 0;
    int z = 0;
    if (card->isKindOf("Slash"))
        x = 1;
    if (card->getSuit() == Card::Spade)
        y = 1;
    if (card->getNumber() > 4 && card->getNumber() < 10)
        z = 1;
    if (source->hasSkill("shenshou") && source->getHandcardNum() == 1) {
        x = 1;
        y = 1;
        z = 1;
    }
    QStringList choices;

    if (x == 1) {
        choices << "shenshou_slash";
    }
    if (y == 1)
        choices << "shenshou_obtain";
    if (z == 1)
        choices << "shenshou_draw";
    choices << "cancel";

    room->showCard(source, subcards.first());

    //    tempcard=sgs.Sanguosha:cloneCard("slash",sgs.Card_Spade,5)

    //    local mes=sgs.LogMessage()
    //    mes.type="$ShenshouTurnOver"
    //    mes.from=source
    //    mes.arg="shenshou"
    //    mes.card_str=tempcard:toString()
    //    room:sendLog(mes)

    room->moveCardTo(card, target, Player::PlaceHand, true);

    while (!choices.isEmpty()) {
        source->tag["shenshou_x"] = QVariant::fromValue(x);
        source->tag["shenshou_y"] = QVariant::fromValue(y);
        source->tag["shenshou_z"] = QVariant::fromValue(z);
        source->tag["shenshou_target"] = QVariant::fromValue(target);
        QString choice = room->askForChoice(source, "shenshou", choices.join("+"));
        choices.removeOne(choice);
        if (choice == "cancel")
            break;
        if (choice == "shenshou_slash") {
            x = 0;
            QList<ServerPlayer *>listt;
            foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
                if (target->inMyAttackRange(p) && target->canSlash(p, NULL, false))
                    listt << p;
            }
            if (listt.length() > 0) {
                ServerPlayer *slashtarget = room->askForPlayerChosen(source, listt, "shenshou", "@shenshou-slash:" + target->objectName());
                if (slashtarget != NULL) {
                    Slash *slash = new Slash(Card::NoSuit, 0);
                    slash->setSkillName("shenshou");
                    room->useCard(CardUseStruct(slash, target, slashtarget));
                }
            }
        }
        if (choice == "shenshou_obtain") {
            y = 0;
            QList<ServerPlayer *>listt;
            foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
                if (target->inMyAttackRange(p) && !p->isKongcheng())
                    listt << p;
            }
            if (listt.length() > 0) {
                ServerPlayer *cardtarget = room->askForPlayerChosen(source, listt, "shenshou", "@shenshou-obtain:" + target->objectName());
                if (cardtarget != NULL) {
                    int card1 = room->askForCardChosen(target, cardtarget, "hs", "shenshou");
                    room->obtainCard(target, card1, false);
                }
            }
        }
        if (choice == "shenshou_draw") {
            z = 0;
            source->drawCards(1);
        }
        source->tag.remove("shenshou_x");
        source->tag.remove("shenshou_y");
        source->tag.remove("shenshou_z");

        source->tag.remove("shenshou_target");
    }
}

class Shenshou : public OneCardViewAsSkill
{
public:
    Shenshou() :OneCardViewAsSkill("shenshou")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ShenshouCard");
    }


    virtual const Card *viewAs(const Card *originalCard) const
    {
        ShenshouCard *card = new ShenshouCard;
        card->addSubcard(originalCard);

        return card;
    }
};



class Yibian : public TriggerSkill
{
public:
    Yibian() : TriggerSkill("yibian")
    {
        events << EventPhaseStart;
    }

    static bool sameRole(ServerPlayer *player1, ServerPlayer *player2)
    {
        QString role1 = (player1->isLord()) ? "loyalist" : player1->getRole();
        QString role2 = (player2->isLord()) ? "loyalist" : player2->getRole();
        if (role1 == role2 && role1 == "renegade")
            return false;
        return role1 == role2;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Start) {
            ServerPlayer *reimu = room->findPlayerBySkillName(objectName());
            if (!reimu)
                return QList<SkillInvokeDetail>();
            if (!player->hasShownRole())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            else if (!player->isNude()) {
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->hasShownRole() && sameRole(player, p)) {
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (!invoke->invoker->hasShownRole()) {
            QString prompt = "yibian_notice";
            return invoke->invoker->askForSkillInvoke(objectName(), prompt);
        } else {
            return true;
        }
        return false;
    }


    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        if (!player->hasShownRole()) {
            QString role = player->getRole();
            room->touhouLogmessage("#YibianShow", invoke->invoker, role, room->getAllPlayers());
            room->broadcastProperty(invoke->invoker, "role");
            room->setPlayerProperty(invoke->invoker, "role_shown", true); //important! to notify client

            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasShownRole() && !sameRole(player, p))
                    targets << p;
            }
            if (targets.length() > 0) {
                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@yibian", false, true);
                target->drawCards(1);
            }
        } else {
            QList<ServerPlayer *> targets;
            QList<int> ids;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasShownRole() && sameRole(player, p)) {
                    targets << p;
                }
            }
            foreach (const Card *c, player->getCards("hes")) {
                ids << c->getEffectiveId();
            }
            QString prompt = "yibian_give";
            room->askForYiji(player, ids, objectName(), false, false, true, 1, targets, CardMoveReason(), prompt);
        }

        return false;
    }

};

FengyinCard::FengyinCard()
{
    will_throw = true;
}

bool FengyinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return(targets.isEmpty() && to_select->hasShownRole());
}

void FengyinCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const
{

    ServerPlayer *target = targets.first();
    QString role = target->getRole();
    room->broadcastProperty(target, "role");
    room->setPlayerProperty(target, "role_shown", false); //important! to notify client

    room->touhouLogmessage("#FengyinHide", target, role, room->getAllPlayers());
    target->turnOver();
}

class Fengyin : public OneCardViewAsSkill
{
public:
    Fengyin() : OneCardViewAsSkill("fengyin")
    {
        filter_pattern = ".|heart|.|.!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("FengyinCard");
    }


    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            FengyinCard *card = new FengyinCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }
};

class Huanxiang : public TriggerSkill
{
public:
    Huanxiang() : TriggerSkill("huanxiang")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    static bool hasSameNumberRoles(Room *room)
    {
        //step1: loyalists vs. rebels
        int loyal_num = 0;
        int loyal_shown_num = 0;
        int rebel_num = 0;
        int rebel_shown_num = 0;

        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            QString role = (p->isLord()) ? "loyalist" : p->getRole();
            if (role == "loyalist") {
                loyal_num++;
                if (p->hasShownRole())
                    loyal_shown_num++;
            } else if (role == "rebel") {
                rebel_num++;
                if (p->hasShownRole())
                    rebel_shown_num++;
            }
        }
        if (loyal_num > 0 && rebel_num > 0 && loyal_shown_num == rebel_shown_num)
            return true;

        //step2: compare renegades
        bool find_shown_renegde = false;
        bool find_hide_renegde = false;
        int renegde_num = 0;
        QStringList roles;
        foreach (ServerPlayer *p1, room->getAlivePlayers()) {
            if (p1->getRole() == "renegade") {
                renegde_num++;
                if (p1->hasShownRole())
                    find_shown_renegde = true;
                else
                    find_hide_renegde = true;

                foreach (ServerPlayer *p2, room->getOtherPlayers(p1)) {
                    if (p2->getRole() == "renegade") {
                        if (p1->hasShownRole() == p2->hasShownRole())
                            return true;
                    }
                }
            }
        }

        //step3: renegades vs. loyal or rebel
        if (find_shown_renegde && (loyal_shown_num == 1 || rebel_shown_num == 1)) {
            return true;
        } else if (find_hide_renegde && ((loyal_shown_num == 0 && loyal_num > 0) || (rebel_shown_num == 0 && rebel_num > 0))) {
            return true;
        }


        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getPhase() == Player::Finish)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        if (!hasSameNumberRoles(room))
            room->askForDiscard(invoke->invoker, objectName(), 1, 1, false, true, "huanxiang_discard");
        else
            invoke->invoker->drawCards(1);
        return false;
    }

};

class RoleShownHandler : public TriggerSkill
{
public:
    RoleShownHandler() : TriggerSkill("roleshownhandler")
    {
        events << GameStart;
        global = true;
    }

    void record(TriggerEvent, Room *room, QVariant &) const
    {
        foreach(ServerPlayer *p, room->getAlivePlayers())
            room->setPlayerProperty(p, "role_shown", p->isLord() ? true : false);
    }
};



class Quanjie : public TriggerSkill
{
public:
    Quanjie() :TriggerSkill("quanjie")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark("quanjie") > 0) {
                        room->setPlayerMark(p, objectName(), 0);
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::Play) {
                foreach (ServerPlayer *src, room->findPlayersBySkillName(objectName())) {
                    if (player != src)
                    d << SkillInvokeDetail(this, src, src);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        return invoke->invoker->askForSkillInvoke(objectName(), QVariant::fromValue(player));
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), player->objectName());

        const Card *card = room->askForCard(player, "%slash,%thunder_slash,%fire_slash", "@quanjie-discard");
        if (card == NULL && player->getMark(objectName()) == 0) {
            player->drawCards(1);
            room->setPlayerMark(player, objectName(), 1);
            room->setPlayerCardLimitation(player, "use", "Slash", true);
        }
        return false;
    }
};

class Duanzui : public TriggerSkill
{
public:
    Duanzui() : TriggerSkill("duanzui")
    {
        events << EventPhaseChanging << EventPhaseStart << Death << TurnStart;
        frequency = Frequent;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        switch (triggerEvent) {
        case Death:
            room->setTag("duanzui", true);
            break;
        case TurnStart:
            room->setTag("duanzui", false);
            break;
        case EventPhaseChanging: {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                if (change.player->getMark("@duanzui-extra") > 0) {
                    change.player->loseMark("@duanzui-extra");
                    room->handleAcquireDetachSkills(change.player, "-shenpan");
                }
            }
            break;
        }
        default:
            break;
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                QVariant deathTag = room->getTag("duanzui");
                bool death = deathTag.canConvert(QVariant::Bool) && deathTag.toBool();
                bool extraTurnExist = room->getThread()->hasExtraTurn();
                if (death) {
                    if (!extraTurnExist) {
                        foreach(ServerPlayer *ymsnd, room->findPlayersBySkillName(objectName())) {
                            if (!ymsnd->isCurrent())
                                d << SkillInvokeDetail(this, ymsnd, ymsnd);
                        }
                    } else {
                        foreach(ServerPlayer *ymsnd, room->findPlayersBySkillName(objectName())) {
                            if (!ymsnd->isCurrent() && !ymsnd->faceUp())
                                d << SkillInvokeDetail(this, ymsnd, ymsnd);
                        }

                    }
                }
            }
        }
        else if (e == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getMark("@duanzui-extra") > 0)
                d << SkillInvokeDetail(this, player, player, NULL, true);
        }
        return d;
    }

    bool cost(TriggerEvent e, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == EventPhaseChanging)
            return invoke->invoker->askForSkillInvoke(objectName(), data);
        return true;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (e == EventPhaseStart) {
            room->handleAcquireDetachSkills(invoke->invoker, "shenpan");
            return false;
        }

        ServerPlayer * current = room->getCurrent();
        ServerPlayer *ymsnd = invoke->invoker;
        if (!ymsnd->faceUp())
            ymsnd->turnOver();
        bool extraTurnExist = room->getThread()->hasExtraTurn();
        if (extraTurnExist)
            return false;

        ymsnd->gainMark("@duanzui-extra");
        QList<ServerPlayer *> logto;
        logto << current->getNext();
        room->touhouLogmessage("#touhouExtraTurn", ymsnd, NULL, logto);
        ymsnd->gainAnExtraTurn();


        return false;
    }
};



HuaxiangCard::HuaxiangCard()
{
    will_throw = false;
}

bool HuaxiangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), Card::NoSuit, 0);
            DELETE_OVER_SCOPE(Card, card)
            return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *card = Self->tag.value("huaxiang").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName(), Card::NoSuit, 0);
    DELETE_OVER_SCOPE(Card, new_card)
    new_card->setSkillName("huaxiang");
    return new_card && new_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, new_card, targets);
}

bool HuaxiangCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), Card::NoSuit, 0);
            DELETE_OVER_SCOPE(Card, card)
            return card && card->targetFixed();
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("huaxiang").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName(), Card::NoSuit, 0);
    DELETE_OVER_SCOPE(Card, new_card)
    new_card->setSkillName("huaxiang");
    return new_card && new_card->targetFixed();
}

bool HuaxiangCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        if (!user_string.isEmpty()) {
            Card *card = Sanguosha->cloneCard(user_string.split("+").first(), Card::NoSuit, 0);
            DELETE_OVER_SCOPE(Card, card)
            return card && card->targetsFeasible(targets, Self);
        }
        return false;
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *card = Self->tag.value("huaxiang").value<const Card *>();
    Card *new_card = Sanguosha->cloneCard(card->objectName(), Card::NoSuit, 0);
    DELETE_OVER_SCOPE(Card, new_card)
    new_card->setSkillName("huaxiang");
    return new_card && new_card->targetsFeasible(targets, Self);
}

const Card *HuaxiangCard::validate(CardUseStruct &card_use) const
{
    QString to_use = user_string;
    card_use.from->getRoom()->touhouLogmessage("#InvokeSkill", card_use.from, "huaxiang");
    card_use.from->addToPile("rainbow", subcards.first());

    Card *use_card = Sanguosha->cloneCard(to_use, Card::NoSuit, 0);
    use_card->setSkillName("huaxiang");
    use_card->deleteLater();
    return use_card;
}

const Card *HuaxiangCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();

    QString to_use;
    if (user_string == "peach+analeptic") {
        QStringList use_list;
        Card *peach = Sanguosha->cloneCard("peach");
        DELETE_OVER_SCOPE(Card, peach)
        if (!user->isCardLimited(peach, Card::MethodResponse, true) && user->getMaxHp() <= 2)
            use_list << "peach";
        Card *ana = Sanguosha->cloneCard("analeptic");
        DELETE_OVER_SCOPE(Card, ana)
        if (!Config.BanPackages.contains("maneuvering") && !user->isCardLimited(ana, Card::MethodResponse, true))
            use_list << "analeptic";
        to_use = room->askForChoice(user, "huaxiang_skill_saveself", use_list.join("+"));
    } else
        to_use = user_string;

    room->touhouLogmessage("#InvokeSkill", user, "huaxiang");
    user->addToPile("rainbow", subcards.first());
    Card *use_card = Sanguosha->cloneCard(to_use, Card::NoSuit, 0);
    use_card->setSkillName("huaxiang");
    use_card->deleteLater();

    return use_card;
}



class Huaxiang : public ViewAsSkill
{
public:
    Huaxiang() : ViewAsSkill("huaxiang")
    {

    }

    static QStringList responsePatterns() {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();

        Card::HandlingMethod method;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            method = Card::MethodResponse;
        else
            method = Card::MethodUse;

        QStringList validPatterns;
        validPatterns << "slash" << "analeptic";
        if (Self->getMaxHp() <= 3)
            validPatterns << "jink";
        if (Self->getMaxHp() <= 2)
            validPatterns << "peach";
        if (Self->getMaxHp() <= 1)
            validPatterns << "nullification";
        QStringList checkedPatterns;
        foreach(QString str, validPatterns) {
            const Skill *skill = Sanguosha->getSkill("huaxiang");
            if (skill->matchAvaliablePattern(str, pattern)) {
                Card *card = Sanguosha->cloneCard(str);
                DELETE_OVER_SCOPE(Card, card)
                if (!Self->isCardLimited(card, method))
                    checkedPatterns << str;
            }
        }
        return checkedPatterns;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getPile("rainbow").length() > 3) return false;
        if (player->isKongcheng()) return false;
        if (Slash::IsAvailable(player) || Analeptic::IsAvailable(player))
            return true;
        if (player->getMaxHp() <= 2) {
            Card *card = Sanguosha->cloneCard("peach", Card::NoSuit, 0);
            DELETE_OVER_SCOPE(Card, card)
            return card->isAvailable(player);
        }
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &) const
    {
        if (player->getPile("rainbow").length() > 3) return false;
        if (player->isKongcheng()) return false;

        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.contains("peach")  && checkedPatterns.length() == 1
            && player->getMark("Global_PreventPeach") > 0) return false;

        return !checkedPatterns.isEmpty();
        /*for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false;
        }*/

    }


    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (to_select->isEquipped() || !selected.isEmpty()) return false;

        foreach (int id, Self->getPile("rainbow")) {
            Card *card = Sanguosha->getCard(id);
            if (card->getSuit() == to_select->getSuit())
                return false;
        }

        return selected.isEmpty();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 1) return NULL;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
            || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();

            QStringList checkedPatterns = responsePatterns();
            if (checkedPatterns.length() >1 || checkedPatterns.contains("slash")) {
                const Card *c = Self->tag.value("huaxiang").value<const Card *>();
                if (c)
                    pattern = c->objectName();
                else
                    return NULL;
            } else
                pattern = checkedPatterns.first();
            HuaxiangCard *card = new HuaxiangCard;
            card->setUserString(pattern);
            card->addSubcards(cards);
            return card;
        }

        const Card *c = Self->tag.value("huaxiang").value<const Card *>();
        if (c) {
            HuaxiangCard *card = new HuaxiangCard;
            card->setUserString(c->objectName());
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }

    virtual QDialog *getDialog() const
    {
        return QijiDialog::getInstance("huaxiang", true, false);
    }


    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        if (player->getPile("rainbow").length() > 3) return false;
        if (player->isKongcheng()) return false;
        Nullification *nul = new Nullification(Card::NoSuit, 0);
        DELETE_OVER_SCOPE(Nullification, nul)
        if (player->isCardLimited(nul, Card::MethodResponse, true))
            return false;
        return player->getMaxHp() <= 1;
    }
};

class Caiyu : public TriggerSkill
{
public:
    Caiyu() : TriggerSkill("caiyu")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Finish) {
            foreach (ServerPlayer *merin, room->findPlayersBySkillName(objectName())) {
                if (merin->getPile("rainbow").length() > 3)
                    d << SkillInvokeDetail(this, merin, merin);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        return invoke->invoker->askForSkillInvoke(objectName(), "discard");
    }


    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *merin = invoke->invoker;
        QList<int>  idlist = merin->getPile("rainbow");

        CardsMoveStruct move;
        move.card_ids = idlist;
        move.to_place = Player::PlaceHand;
        move.to = merin;
        room->moveCardsAtomic(move, true);
        int x = qMin(merin->getHandcardNum(), 2);
        if (x > 0)
            room->askForDiscard(merin, objectName(), x, x, false, false, "caiyu_discard:" + QString::number(x));

        if (room->askForSkillInvoke(merin, objectName(), "loseMaxHp")) {
            room->loseMaxHp(merin, 1);
        }
        return false;
    }
};

class Xuanlan : public TriggerSkill
{
public:
    Xuanlan() : TriggerSkill("xuanlan")
    {
        events << EventPhaseChanging;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {

        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Discard && change.player->hasSkill(this)
            && !change.player->isWounded())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->skip(Player::Discard);
        return false;
    }
};




class QiannianMax : public MaxCardsSkill
{
public:
    QiannianMax() : MaxCardsSkill("#qiannian_max")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        int n = target->getMark("@qiannian");
        if (n > 0 && target->hasSkill("qiannian"))
            return 2 * n;
        else
            return 0;
    }
};

class Qiannian : public TriggerSkill
{
public:
    Qiannian() : TriggerSkill("qiannian")
    {
        events << GameStart << DrawNCards << DrawPileSwaped;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == GameStart) {
            ServerPlayer *erin = data.value<ServerPlayer *>();
            if (erin && erin->hasSkill(this))
                d << SkillInvokeDetail(this, erin, erin, NULL, true);
        } else if (triggerEvent == DrawPileSwaped) {
            foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                d << SkillInvokeDetail(this, p, p, NULL, true);
        } else if (triggerEvent == DrawNCards) {
            DrawNCardsStruct dc = data.value<DrawNCardsStruct>();
            if (dc.player->getMark("@qiannian") > 0 && dc.player->hasSkill(this))
                d << SkillInvokeDetail(this, dc.player, dc.player, NULL, true);
        }
        return d;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = invoke->invoker;
        log.arg = objectName();
        if (triggerEvent == GameStart || triggerEvent == DrawPileSwaped) {
            room->sendLog(log);
            room->notifySkillInvoked(invoke->invoker, objectName());

            invoke->invoker->gainMark("@qiannian", 1);
        } else if (triggerEvent == DrawNCards) {
            DrawNCardsStruct dc = data.value<DrawNCardsStruct>();
            dc.n = dc.n + invoke->invoker->getMark("@qiannian");
             data = QVariant::fromValue(dc);
            room->sendLog(log);
            room->notifySkillInvoked(invoke->invoker, objectName());
        }
        return false;
    }
};




class Qinlue : public TriggerSkill
{
public:
    Qinlue() : TriggerSkill("qinlue")
    {
        events << EventPhaseChanging;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Play) {
            ServerPlayer *current = change.player;
            if (!current || !current->isCurrent() || current->isSkipped(Player::Play))
                return QList<SkillInvokeDetail>();

            QList<SkillInvokeDetail> d;
            foreach(ServerPlayer *source, room->findPlayersBySkillName(objectName())) {
                if (current != source && source->canDiscard(source, "hes"))
                    d << SkillInvokeDetail(this, source, source);
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        ServerPlayer *source = invoke->invoker;
        ServerPlayer *current = change.player;

        QString  prompt = "@qinlue-discard:" + current->objectName();
        const Card *card = room->askForCard(source, "Slash,EquipCard", prompt, QVariant::fromValue(current), Card::MethodDiscard, current, false, "qinlue");
        return card != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *source = invoke->invoker;
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        ServerPlayer *current = change.player;
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), current->objectName());

        QString  prompt = "@qinlue-discard1:" + source->objectName();
        const Card *card = room->askForCard(current, "Jink", prompt, QVariant::fromValue(source), Card::MethodDiscard);
        if (!card) {
            current->skip(Player::Play);
            if (!room->getThread()->hasExtraTurn()) {
                ExtraTurnStruct extra;
                extra.player = invoke->invoker;
                extra.set_phases << Player::RoundStart << Player::Play << Player::NotActive;
                extra.reason = "qinlue";
                extra.extraTarget = current;
                room->setTag("ExtraTurnStruct", QVariant::fromValue(extra));
                source->tag["ExtraTurnInfo"] = QVariant::fromValue(extra);
                invoke->invoker->gainAnExtraTurn();
            }
        }
        return false;
    }

};

class QinlueEffect : public TriggerSkill
{
public:
    QinlueEffect() : TriggerSkill("#qinlue_effect")
    {
        events << EventPhaseChanging << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::RoundStart) {
                ExtraTurnStruct extra = player->tag["ExtraTurnInfo"].value<ExtraTurnStruct>();
                if (extra.reason == "qinlue" && extra.extraTarget != NULL) {
                    ServerPlayer *target = extra.extraTarget;
                    if (target->isAlive() && !target->isKongcheng() || !player->isKongcheng())
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true, target);
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            ServerPlayer *player = change.player;
            if (change.to == Player::NotActive) {
                ExtraTurnStruct extra = player->tag["ExtraTurnInfo"].value<ExtraTurnStruct>();
                if (extra.reason == "qinlue" && extra.extraTarget != NULL) {
                    ServerPlayer *target = extra.extraTarget;
                    if ((target != NULL && target->isAlive() && !player->isKongcheng()) || !player->getPile("zhanbei").isEmpty())
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true, target);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = invoke->invoker;
            ServerPlayer *target = player->tag["ExtraTurnInfo"].value<ExtraTurnStruct>().extraTarget;
            if (!player->isKongcheng()) {
                DummyCard *dummy = new DummyCard;
                dummy->deleteLater();
                dummy->addSubcards(player->getHandcards());
                player->addToPile("zhanbei", dummy, false);
            }

            if (target != NULL && target->isAlive() && !target->isKongcheng()) {
                DummyCard *dummy1 = new DummyCard;
                dummy1->deleteLater();
                dummy1->addSubcards(target->getHandcards());
                room->obtainCard(player, dummy1, false);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            ServerPlayer *player = invoke->invoker;
            ServerPlayer *target = player->tag["ExtraTurnInfo"].value<ExtraTurnStruct>().extraTarget;
            if (target != NULL && target->isAlive()) {
                DummyCard *dummy = new DummyCard;
                dummy->deleteLater();
                dummy->addSubcards(player->getHandcards());
                room->obtainCard(target, dummy, false);
            }
            if (!player->getPile("zhanbei").isEmpty()) {
                DummyCard *dummy1 = new DummyCard;
                dummy1->deleteLater();
                dummy1->addSubcards(player->getPile("zhanbei"));
                room->obtainCard(player, dummy1, false);
            }
        }
        return false;
    }

};


class Chaoren : public TriggerSkill
{
public:
    Chaoren() : TriggerSkill("chaoren")
    {
        events << NumOfEvents;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {

        if (room->getTag("FirstRound").toBool())
            return;
        if (triggerEvent == FetchDrawPileCard)
            return;
        //need trigger after drawing intiial cards?
        const QList<int> &drawpile = room->getDrawPile();
        int new_firstcard = -1;
        if (!drawpile.isEmpty())
            new_firstcard = drawpile.first();

        foreach (ServerPlayer *p, room->getAllPlayers()) {
            int new_firstcardProperty = new_firstcard;
            bool ok = false;
            int old_firstcard = p->property("chaoren").toInt(&ok);
            if (!ok)
                old_firstcard = -1;

            if (!p->hasSkill(objectName()))
                new_firstcardProperty = -1;

            bool notifyProperty = old_firstcard != new_firstcardProperty;
            if (notifyProperty) {
                p->setProperty("chaoren", new_firstcardProperty);
                room->notifyProperty(p, p, "chaoren");
            }

            if (!p->hasSkill(objectName()))
                continue;

            // for client log
            bool notifyLog = (new_firstcard != old_firstcard);
            if (triggerEvent == EventAcquireSkill) {
                SkillAcquireDetachStruct s = data.value<SkillAcquireDetachStruct>();
                if (s.skill->objectName() == objectName() && s.player == p)
                    notifyLog = true;
            }
            if (triggerEvent == EventSkillInvalidityChange) {
                QList<SkillInvalidStruct> invalids = data.value<QList<SkillInvalidStruct>>();
                foreach(SkillInvalidStruct v, invalids) {
                    if ((!v.skill || v.skill->objectName() == objectName()) && v.player == p && !v.invalid) {
                        notifyLog = true;
                        break;
                    }
                }
            }

            if (new_firstcard > -1 && notifyLog) {
                QList<int> watchlist;
                watchlist << new_firstcard;
                LogMessage l;
                l.type = "$chaorendrawpile";
                l.card_str = IntList2StringList(watchlist).join("+");

                room->doNotify(p, QSanProtocol::S_COMMAND_LOG_SKILL, l.toJsonValue());
            }
        }
    }
};


class Biaoxiang : public TriggerSkill
{
public:
    Biaoxiang() : TriggerSkill("biaoxiang")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getMark("biaoxiang") == 0 && player->getPhase() == Player::Start && player->getHandcardNum() < 2)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        room->doLightbox("$biaoxiangAnimate", 4000);
        room->touhouLogmessage("#BiaoxiangWake", player, objectName());
        room->notifySkillInvoked(player, objectName());
        room->addPlayerMark(player, objectName());
        int x = player->getMaxHp();
        int y;
        if (x > 3)
            y = 3 - x;
        else
            y = 4 - x;

        if (room->changeMaxHpForAwakenSkill(player, y))
            room->handleAcquireDetachSkills(player, "ziwo");
        return false;
    }
};

ZiwoCard::ZiwoCard()
{
    will_throw = true;
    target_fixed = true;
}

void ZiwoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    RecoverStruct recov;
    recov.recover = 1;
    recov.who = source;
    room->recover(source, recov);
}

class Ziwo : public ViewAsSkill
{
public:
    Ziwo() : ViewAsSkill("ziwo")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return  player->isWounded();
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isEquipped() && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            ZiwoCard *card = new ZiwoCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;

    }
};

class Shifang : public TriggerSkill
{
public:
    Shifang() : TriggerSkill("shifang")
    {
        events << CardsMoveOneTime << BeforeCardsMove;
        frequency = Wake;
    }
    static void koishi_removeskill(Room *room, ServerPlayer *player)
    {
        foreach (const Skill *skill, player->getVisibleSkillList()) {
            if (skill->getFrequency() == Skill::Wake || skill->isAttachedLordSkill()
                || skill->getFrequency() == Skill::Eternal)
                continue;
            else {
                QString skill_name = skill->objectName();

                room->handleAcquireDetachSkills(player, "-" + skill_name);

            }
        }
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
            if (player != NULL && player->hasSkill(this) && player->getCards("e").length() == 1 && player->getMark("shifang") == 0) {
                foreach(int id, move.card_ids) {
                    if (room->getCardPlace(id) == Player::PlaceEquip) {
                        room->setCardFlag(Sanguosha->getCard(id), "shifang");
                        room->setPlayerFlag(player, "shifangInvoked");
                        break;
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
         if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
            if (player != NULL && player->isAlive() && player->hasSkill(this) && player->hasFlag("shifangInvoked") && player->getMark("shifang") == 0) {
                foreach(int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->hasFlag("shifang")) {
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
                    }
                }
            }
        }
         return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        room->doLightbox("$shifangAnimate", 4000);
        room->touhouLogmessage("#ShifangWake", player, objectName());
        koishi_removeskill(room, player);

        room->notifySkillInvoked(player, objectName());
        room->addPlayerMark(player, objectName());
        int x = player->getMaxHp();
        if (room->changeMaxHpForAwakenSkill(player, 4 - x)) {
            room->handleAcquireDetachSkills(player, "benwo");
            room->setPlayerFlag(player, "-shifangInvoked");
        }
        return false;
    }
};

class Benwo : public TriggerSkill
{
public:
    Benwo() : TriggerSkill("benwo")
    {
        events << DamageInflicted;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->hasSkill(this) && damage.to->isWounded() && damage.from != NULL && damage.from->isAlive())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, false, damage.from);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->tag["benwo_target"] = QVariant::fromValue(invoke->preferredTarget);
        int x = invoke->invoker->getLostHp();
        QString prompt = "invoke:" + invoke->preferredTarget->objectName() + ":" + QString::number(x);
        return invoke->invoker->askForSkillInvoke(objectName(), QVariant::fromValue(invoke->preferredTarget));
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        int x = invoke->invoker->getLostHp();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());

        invoke->invoker->drawCards(x);
        room->askForDiscard(invoke->targets.first(), objectName(), x, x, false, true);
        return false;
    }
};

class Yizhi : public TriggerSkill
{
public:
    Yizhi() : TriggerSkill("yizhi")
    {
        events << Dying;
        frequency = Wake;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *who = data.value<DyingStruct>().who;
        if (who->hasSkill(this) && who->getMark("yizhi") == 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, who, who, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        room->doLightbox("$yizhiAnimate", 4000);
        room->touhouLogmessage("#YizhiWake", player, objectName());
        room->notifySkillInvoked(player, objectName());
        room->addPlayerMark(player, objectName());

        int x = player->dyingThreshold() - player->getHp();
        RecoverStruct recov;
        recov.recover = x;
        recov.who = player;
        room->recover(player, recov);

        x = player->getMaxHp();
        if (room->changeMaxHpForAwakenSkill(player, 3 - x)) {
            Shifang::koishi_removeskill(room, player);
            room->handleAcquireDetachSkills(player, "chaowo");
        }

        return false;
    }
};

ChaowoCard::ChaowoCard()
{
    will_throw = true;
}

bool ChaowoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return(targets.isEmpty() && to_select->getMaxHp() >= Self->getMaxHp());
}

void ChaowoCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{

    targets.first()->drawCards(2);
    if (targets.first()->getMaxHp() == 3)
        source->drawCards(2);
}

class ChaowoVS : public OneCardViewAsSkill
{
public:
    ChaowoVS() : OneCardViewAsSkill("chaowo")
    {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@chaowo";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            ChaowoCard *card = new ChaowoCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }
};

class Chaowo : public TriggerSkill
{
public:
    Chaowo() : TriggerSkill("chaowo")
    {
        events << EventPhaseStart;
        view_as_skill = new ChaowoVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getPhase() == Player::Finish)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->askForUseCard(invoke->invoker, "@@chaowo", "@chaowo");
        return false;
    }
};




class Zuosui : public TriggerSkill
{
public:
    Zuosui() : TriggerSkill("zuosui")
    {
        events << DamageCaused;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.from || damage.from == damage.to || !damage.from->hasSkill(this))
            return QList<SkillInvokeDetail>();
        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        invoke->invoker->tag["zuosui_damage"] = data;
        return invoke->invoker->askForSkillInvoke(objectName(), QVariant::fromValue(damage.to));
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = invoke->invoker;
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());

        player->gainMark("@xinyang");

        damage.to->tag["zuosui_source"] = QVariant::fromValue(player);
        QString choice = room->askForChoice(damage.to, objectName(), "1+2+3+4");
        int x;
        if (choice == "1")
            x = 1;
        else if (choice == "2")
            x = 2;
        else if (choice == "3")
            x = 3;
        else
            x = 4;
        room->touhouLogmessage("#zuosuichoice", damage.to, objectName(), QList<ServerPlayer *>(), QString::number(x));
        player->tag["zuosui_number"] = QVariant::fromValue(x);
        choice = room->askForChoice(player, objectName(), "losehp+discard");
        if (choice == "losehp") {
            damage.to->drawCards(x);
            room->loseHp(damage.to, x);
        } else {
            int discardNum = damage.to->getCards("hes").length() > x ? damage.to->getCards("he").length() - x : 0;
            if (discardNum > 0)
                room->askForDiscard(damage.to, objectName(), discardNum, discardNum, false, true);
        }
        return true;
    }
};

class Worao : public TriggerSkill
{
public:
    Worao() : TriggerSkill("worao")
    {
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from && use.from->isAlive() && (use.card->isKindOf("Slash") || use.card->isNDTrick())) {
            foreach(ServerPlayer *p, use.to) {
                if (p->hasSkill(this) && p != use.from)
                    d << SkillInvokeDetail(this, p, p, NULL, false, use.from);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "invoke:" + use.from->objectName() + ":" + use.card->objectName();
        invoke->invoker->tag["worao_use"] = data;
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = invoke->invoker;
        CardUseStruct use = data.value<CardUseStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.from->objectName());
        player->gainMark("@xinyang");
        player->drawCards(2);
        if (!player->isKongcheng()) {
            const Card *card = room->askForExchange(player, objectName(), 1, 1, false, "woraoGive:" + use.from->objectName());
            DELETE_OVER_SCOPE(const Card, card)
            use.from->obtainCard(card, false);
        }
        return false;
    }
};

class Shenhua : public TriggerSkill
{
public:
    Shenhua() : TriggerSkill("shenhua")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getPhase() == Player::Finish)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        QString choice = "loseHp"; 
        if (invoke->invoker->getMark("@xinyang") > 0)
            choice = room->askForChoice(invoke->invoker, objectName(), "loseHp+discardMark", data);

        if (choice == "loseHp")
            room->loseMaxHp(invoke->invoker, 1);
        else
            invoke->invoker->loseMark("@xinyang", invoke->invoker->getMark("@xinyang"));
        return false;
    }
};




class Hongfo : public TriggerSkill
{
public:
    Hongfo() : TriggerSkill("hongfo")
    {
        events << AfterDrawNCards;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &) const
    {
        ServerPlayer *player = room->getCurrent();
        if (player && player->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<ServerPlayer *> all = room->getLieges(invoke->invoker->getKingdom(), NULL);
        room->sortByActionOrder(all);
        foreach(ServerPlayer *p, all)
            p->drawCards(1);
        QList<ServerPlayer *> others;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getKingdom() != invoke->invoker->getKingdom())
                others << p;
        }
        if (others.length() <= 0)
            return false;
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, others, objectName(), "@hongfo", false, true);

        LogMessage log;
        log.type = "#hongfoChangeKingdom";
        log.from = target;
        log.arg = invoke->invoker->getKingdom();
        room->sendLog(log);
        room->setPlayerProperty(target, "kingdom", invoke->invoker->getKingdom());
        return false;
    }
};

class Junwei : public TriggerSkill
{
public:
    Junwei() : TriggerSkill("junwei")
    {
        frequency = Compulsory;
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.from->isAlive()) {
            foreach(ServerPlayer *p, use.to) {
                if (use.from != p && p->hasSkill(this) && use.from->getKingdom() != p->getKingdom())
                    d << SkillInvokeDetail(this, p, p, NULL, true, use.from);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = invoke->invoker;
        room->notifySkillInvoked(player, objectName());
        room->touhouLogmessage("#TriggerSkill", player, "junwei");
        CardUseStruct use = data.value<CardUseStruct>();
        use.from->tag["junwei_target"] = QVariant::fromValue(player);
        QString prompt = "@junwei-discard:" + player->objectName() + ":" + use.card->objectName();
        const Card *card = room->askForCard(use.from, ".|black|.|hand,equipped", prompt, data, Card::MethodDiscard);
        if (card == NULL) {
            use.nullified_list << player->objectName();
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

class Gaizong : public TriggerSkill
{
public:
    Gaizong() : TriggerSkill("gaizong")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getPhase() == Player::Finish && player->getMark("gaizong") == 0) {
            QStringList kingdoms;
            int num = 0;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (!kingdoms.contains(p->getKingdom())) {
                    kingdoms << p->getKingdom();
                    num += 1;
                }
            }
            if (num <= 2)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        room->addPlayerMark(player, objectName());
        room->doLightbox("$gaizongAnimate", 4000);
        room->touhouLogmessage("#GaizongWake", player, objectName());
        room->notifySkillInvoked(player, objectName());
        if (room->changeMaxHpForAwakenSkill(player)) {
            QString kingdom = room->askForKingdom(player);

            LogMessage log;
            log.type = "#ChooseKingdom";
            log.from = player;
            log.arg = kingdom;
            room->sendLog(log);

            room->setPlayerProperty(player, "kingdom", kingdom);
            room->handleAcquireDetachSkills(player, "-hongfo");
            room->handleAcquireDetachSkills(player, "wendao");
        }

        return false;
    }
};

WendaoCard::WendaoCard()
{
}

bool WendaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!Self->canDiscard(to_select, "hs"))
        return false;
    if (targets.length() == 0)
        return true;
    else {
        foreach (const Player *p, targets) {
            if (p->getKingdom() == to_select->getKingdom())
                return false;
        }
        return true;
    }
}

void WendaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->sortByActionOrder(targets);
    foreach (ServerPlayer *p, targets) {
        const Card *wendaoCard = NULL;
        if (p == source) {
            wendaoCard = room->askForCard(p, ".!", "@wendao-dis", QVariant(), QString());
            if (wendaoCard == NULL) {
                wendaoCard = p->getRandomHandCard();
                room->throwCard(wendaoCard, p);
            }
        } else {
            int card_id = room->askForCardChosen(source, p, "hs", objectName(), false, Card::MethodDiscard);
            room->throwCard(card_id, p, source);
            wendaoCard = Sanguosha->getCard(card_id);
        }
        if (wendaoCard->isRed()) {
            RecoverStruct recover;
            room->recover(source, recover);
        }
    }
}

class WendaoVS : public ZeroCardViewAsSkill
{
public:
    WendaoVS() : ZeroCardViewAsSkill("wendao")
    {
        response_pattern = "@@wendao";
    }

    virtual const Card *viewAs() const
    {
        return new WendaoCard;
    }
};

class Wendao : public TriggerSkill
{
public:
    Wendao() : TriggerSkill("wendao")
    {
        events << EventPhaseStart;
        view_as_skill = new WendaoVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getPhase() == Player::Play)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->askForUseCard(invoke->invoker, "@@wendao", "@wendao");
        return false;
    }
};




class Shenbao : public AttackRangeSkill
{
public:
    Shenbao() : AttackRangeSkill("shenbao")
    {

    }

    virtual int getExtra(const Player *target, bool) const
    {
        if (target->hasSkill(objectName())) {
            int self_weapon_range = 1;
            int extra = 0;
            if (target->getWeapon()) {
                WrappedCard *weapon = target->getWeapon();
                const Weapon *card = qobject_cast<const Weapon *>(weapon->getRealCard());
                self_weapon_range = card->getRange();
            }
            foreach (const Player *p, target->getAliveSiblings()) {
                if (p->getWeapon()) {
                    WrappedCard *weapon = p->getWeapon();
                    const Weapon *card = qobject_cast<const Weapon *>(weapon->getRealCard());
                    int other_weapon_range = card->getRange();
                    int new_extra = other_weapon_range - self_weapon_range;
                    if (new_extra > extra)
                        extra = new_extra;
                }
            }
            return extra;
        }
        return 0;
    }
};

class ShenbaoDistance : public DistanceSkill
{
public:
    ShenbaoDistance() : DistanceSkill("#shenbao_distance")
    {
    }

    virtual int getCorrect(const Player *from, const Player *to) const
    {
        int correct = 0;
        if (from->hasSkill("shenbao") && from->getMark("Equips_Nullified_to_Yourself") == 0) {
            foreach (const Player *p, from->getAliveSiblings()) {
                if (p->getOffensiveHorse()) {
                    correct = correct - 1;
                }
            }
        }


        if (to->hasSkill("shenbao") && to->getMark("Equips_Nullified_to_Yourself") == 0) {
            foreach (const Player *p, to->getAliveSiblings()) {
                if (p->getDefensiveHorse()) {
                    correct = correct + 1;
                }
            }
        }

        return correct;
    }
};

class ShenbaoSpear : public ViewAsSkill
{
public:
    ShenbaoSpear() : ViewAsSkill("shenbao_spear")
    {
        attached_lord_skill = true;
        response_or_use = true;
    }

    virtual bool shouldBeVisible(const Player *Self) const
    {
        return Self;
        //return Self->hasWeapon("Spear") && !Self->hasWeapon("Spear", true);
        // need update dashboard when real weapon moved
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasWeapon("Spear") && !player->hasWeapon("Spear", true)) {
            const ViewAsSkill *spear_skill = Sanguosha->getViewAsSkill("Spear");
            return spear_skill->isEnabledAtPlay(player);
        }
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->hasWeapon("Spear") && !player->hasWeapon("Spear", true)) {

            const ViewAsSkill *spear_skill = Sanguosha->getViewAsSkill("Spear");
            return spear_skill->isEnabledAtResponse(player, pattern);
        }
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Self->hasWeapon("Spear") && !Self->hasWeapon("Spear", true)) {
            const ViewAsSkill *spear_skill = Sanguosha->getViewAsSkill("Spear");
            return spear_skill->viewFilter(selected, to_select);
        }
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (Self->hasWeapon("Spear") && !Self->hasWeapon("Spear", true)) {
            const ViewAsSkill *spear_skill = Sanguosha->getViewAsSkill("Spear");
            return spear_skill->viewAs(cards);
        }
        return NULL;
    }
};

class ShenbaoHandler : public TriggerSkill
{
public:
    ShenbaoHandler() : TriggerSkill("#shenbao")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Death << Debut;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const
    {
        if (triggerEvent == GameStart || triggerEvent == Debut || triggerEvent == EventAcquireSkill) {
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill("shenbao", true) && !p->hasSkill("shenbao_spear"))
                    room->attachSkillToPlayer(p, "shenbao_spear");
            }
        }
        if (triggerEvent == Death || triggerEvent == EventLoseSkill) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (!p->hasSkill("shenbao", true) && p->hasSkill("shenbao_spear"))
                    room->detachSkillFromPlayer(p, "shenbao_spear", true);
            }
        }
    }
};




class Yindu : public TriggerSkill
{
public:
    Yindu() : TriggerSkill("yindu")
    {
        events << Death;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        QList<SkillInvokeDetail> d;
        foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (death.who && death.who != p)
                d << SkillInvokeDetail(this, p, p);
        }
        return d;
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        QString prompt = "invoke:" + death.who->objectName();
        invoke->invoker->tag["yindu_death"] = data;
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), death.who->objectName());
        invoke->invoker->drawCards(3);
        room->setPlayerFlag(death.who, "skipRewardAndPunish");

        return false;
    }
};

class Huanming : public TriggerSkill
{
public:
    Huanming() : TriggerSkill("huanming")
    {
        events << DamageCaused;
        frequency = Limited;
        limit_mark = "@huanming";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.from || damage.from == damage.to
            || damage.from->getMark("@huanming") == 0
            || damage.to->getHp() < damage.from->dyingThreshold() || damage.from->getHp() < damage.to->dyingThreshold() || !damage.from->hasSkill(this))
            return QList<SkillInvokeDetail>();
        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        invoke->invoker->tag["huanming_damage"] = data;
        return invoke->invoker->askForSkillInvoke(objectName(), QVariant::fromValue(damage.to));
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *player = invoke->invoker;
        room->removePlayerMark(player, "@huanming");
        room->doLightbox("$huanmingAnimate", 4000);
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());

        int source_Hp = player->getHp();
        int victim_Hp = damage.to->getHp();
        int source_newHp = qMin(damage.to->getHp(), player->getMaxHp());
        int victim_newHp = qMin(player->getHp(), damage.to->getMaxHp());
        room->setPlayerProperty(player, "hp", source_newHp);
        room->setPlayerProperty(damage.to, "hp", victim_newHp);

        room->touhouLogmessage("#GetHp", player, QString::number(player->getHp()), QList<ServerPlayer *>(), QString::number(player->getMaxHp()));
        room->touhouLogmessage("#GetHp", damage.to, QString::number(damage.to->getHp()), QList<ServerPlayer *>(), QString::number(damage.to->getMaxHp()));
        if (source_Hp > source_newHp) {
            HpLostStruct l;
            l.player = player;
            l.num = source_Hp - source_newHp;
            room->getThread()->trigger(PostHpReduced, room, QVariant::fromValue(l));
        } else if (victim_Hp > victim_newHp) {
            HpLostStruct l;
            l.player = damage.to;
            l.num = victim_Hp - victim_newHp;
            room->getThread()->trigger(PostHpReduced, room, QVariant::fromValue(l));
        }
        
        return true;
    }
};

//the real distance effect is in  Player::distanceTo()
//this triggerskill is for skilleffect
class Chuanwu : public TriggerSkill
{
public:
    Chuanwu() : TriggerSkill("chuanwu")
    {
        events << HpChanged;
        frequency = Compulsory;
    }


    void record(TriggerEvent, Room *room, QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->hasSkill(this)) {
                int right = qAbs(p->getSeat() - player->getSeat());
                int left = room->alivePlayerCount() - right;
                int distance = qMin(left, right);
                distance += Sanguosha->correctDistance(p, player);
                if (distance > player->getHp())
                    room->notifySkillInvoked(p, objectName());
            }
        }
    }
};



class Huanhun : public TriggerSkill
{
public:
    Huanhun() : TriggerSkill("huanhun")
    {
        events << BuryVictim;
        frequency = Eternal;
    }

    virtual int getPriority() const
    {
        return -1;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->getOtherPlayers(death.who, true)) {
            if (p->isDead() && p->hasSkill(this))
                d << SkillInvokeDetail(this, p, p, NULL, true);
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->revivePlayer(invoke->invoker);

        return false;
    }
};

class Tongling : public TriggerSkill
{
public:
    Tongling() : TriggerSkill("tongling")
    {
        events << EventPhaseStart;
        frequency = Limited;
        limit_mark = "@tongling";
    }

    static bool  findSameSkill(const Skill *source_skill, ServerPlayer *player)
    {
        foreach(ServerPlayer *p, player->getRoom()->getAlivePlayers()) {
            foreach(const Skill *skill, p->getVisibleSkillList()) {
                if (skill->objectName() == source_skill->objectName())
                    return true;
            }
        }
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getPhase() == Player::RoundStart && player->getMark("@tongling") > 0) {
            foreach(ServerPlayer *p, room->getAllPlayers(true)) {
                if (p->isDead() && p->getGeneral() != NULL && p->getGeneralName() != "sujiang" && p->getGeneralName() != "sujiangf")
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (!invoke->invoker->isOnline()) {
            QStringList skill_names;
            QList<ServerPlayer *> targets;

            foreach(ServerPlayer *p, room->getAllPlayers(true)) {
                if (p->isDead() && p->getGeneral() != NULL && p->getGeneralName() != "sujiang" && p->getGeneralName() != "sujiangf") {
                    foreach(const Skill *skill, p->getGeneral()->getVisibleSkillList()) {
                        if (skill->isLordSkill()
                            || skill->getFrequency() == Skill::Eternal
                            || skill->getFrequency() == Skill::Wake
                            || findSameSkill(skill, invoke->invoker))
                            continue;
                        if (!skill_names.contains(skill->objectName())) {
                            skill_names << skill->objectName();
                            targets << p;
                        }
                    }
                }
            }
            skill_names << "cancel";
            QString skill_name = room->askForChoice(invoke->invoker, objectName(), skill_names.join("+"));
            if (skill_name != "cancel") {
                invoke->targets << targets.at(skill_names.indexOf(skill_name));
                room->notifySkillInvoked(invoke->invoker, objectName());
                room->touhouLogmessage("#InvokeSkill", invoke->invoker, objectName());
            }
            return skill_name != "cancel";

        } else {
            if (invoke->invoker->askForSkillInvoke(this, data)) {
                QStringList general_list;
                foreach(ServerPlayer *p, room->getAllPlayers(true)) {
                    if (p->isDead() && p->getGeneral() != NULL && p->getGeneralName() != "sujiang" && p->getGeneralName() != "sujiangf")
                            general_list << p->getGeneralName();
                }

                QString general = room->askForGeneral(invoke->invoker, general_list);
                //forbid cheat
                if (!general_list.contains(general))
                    general = general_list.at(qrand() % general_list.length());
                foreach(ServerPlayer *p, room->getAllPlayers(true)) {
                    if (p->isDead() && p->getGeneralName() == general) {
                        invoke->targets << p;
                        break;
                    }
                }
                return true;
            }

        }


        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->removePlayerMark(invoke->invoker, "@tongling");
        const General *general = invoke->targets.first()->getGeneral();
        QString new_general = invoke->targets.first()->isMale() ? "sujiang" : "sujiangf";
        room->changeHero(invoke->targets.first(), new_general, false, false, false, false);

        QStringList skill_names;

        foreach(const Skill *skill, general->getVisibleSkillList()) {
            if (skill->isLordSkill()
                || skill->getFrequency() == Skill::Eternal
                || skill->getFrequency() == Skill::Wake 
                || findSameSkill(skill, invoke->invoker))
                continue;
            if (!skill_names.contains(skill->objectName()))
                skill_names << skill->objectName();
        }
        if (skill_names.isEmpty())
            skill_names << "cancel";
        QString skill_name = room->askForChoice(invoke->invoker, objectName(), skill_names.join("+"));

        if (skill_name != "cancel") {
            invoke->invoker->tag["Huashen_target"] = general->objectName();
            invoke->invoker->tag["Huashen_skill"] = skill_name;

            JsonArray arg;
            arg << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
            arg << invoke->invoker->objectName();
            arg << general->objectName();
            arg << skill_name;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
            room->handleAcquireDetachSkills(invoke->invoker, skill_name, true);
        }
        return false;
    }

};

RumoCard::RumoCard()
{
}

bool RumoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int num = qMax(Self->getHp(), 1);
    if (!targets.contains(Self))
        return to_select == Self && targets.length() < num;
    return !targets.contains(to_select) && targets.length() < num;
}

bool RumoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    int num = qMax(Self->getHp(), 1);
    return targets.length() >0 && targets.length() <= num && targets.contains(Self);
}

void RumoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->removePlayerMark(source, "@rumo");
    int num = qMax(source->getHp(), 1);
    room->sortByActionOrder(targets);
    foreach(ServerPlayer *target, targets) {
        if (!target->isChained())
            room->setPlayerProperty(target, "chained", true);
    }
    source->drawCards(num);
}

class Rumo : public ZeroCardViewAsSkill
{
public:
    Rumo() : ZeroCardViewAsSkill("rumo")
    {
        frequency = Limited;
        limit_mark = "@rumo";
    }

    virtual const Card *viewAs() const
    {
        return new RumoCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@rumo") >= 1;
    }
};





class Xinhua : public TriggerSkill
{
public:
    Xinhua() : TriggerSkill("xinhua")
    {
        events << TargetConfirmed << TargetSpecified;
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->isKindOf("SkillCard") || use.from ==NULL )
            return d;

        if (e == TargetSpecified) {
            if (use.from->hasSkill(this) && !use.from->isKongcheng()) {
                foreach(ServerPlayer *p, use.to) {
                    if (use.from != p && !p->isKongcheng())
                        d << SkillInvokeDetail(this, use.from, use.from, NULL, false, p);
                }
            }
        } else {
            if (!use.from->isKongcheng()) {
                foreach(ServerPlayer *p, use.to) {
                    if (use.from != p && p->hasSkill(this) && !p->isKongcheng())
                        d << SkillInvokeDetail(this, p, p, NULL, false, use.from);
                }
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {

        LogMessage log;
        log.type = "#ChoosePlayerWithSkill";
        log.from = invoke->invoker;
        log.to << invoke->targets.first();
        log.arg = objectName();
        room->sendLog(log);
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());

        QList<ServerPlayer *> targets;
        targets << invoke->invoker << invoke->targets.first();
        room->sortByActionOrder(targets);
        ServerPlayer *a = targets.at(0);
        ServerPlayer *b = targets.at(1);

        int id1 = room->askForCardChosen(invoke->invoker, a, "hs", objectName());
        int id2 = room->askForCardChosen(invoke->invoker, b, "hs", objectName());

        QList<int> ids1, ids2;
        ids1 << id1;
        ids2 << id2;

        // need process known cards?
        /*foreach(ServerPlayer *p, room->getAlivePlayers()) {
        if (p != a && p != b) {
        JsonArray arr;
        arr << a->objectName() << b->objectName();
        room->doNotify(p, QSanProtocol::S_COMMAND_EXCHANGE_KNOWN_CARDS, arr);
        }
        }*/

        QList<CardsMoveStruct> exchangeMove;
        CardsMoveStruct move1(ids1, b, Player::PlaceHand,
            CardMoveReason(CardMoveReason::S_REASON_SWAP, a->objectName(), b->objectName(), objectName(), QString()));
        CardsMoveStruct move2(ids2, a, Player::PlaceHand,
            CardMoveReason(CardMoveReason::S_REASON_SWAP, b->objectName(), a->objectName(), objectName(), QString()));
        exchangeMove.push_back(move1);
        exchangeMove.push_back(move2);
        room->moveCardsAtomic(exchangeMove, false);

        if (room->getCardOwner(id1) == b && room->getCardPlace(id1) == Player::PlaceHand)
            b->addToShownHandCards(ids1);
        if (room->getCardOwner(id2) == a && room->getCardPlace(id2) == Player::PlaceHand)
            a->addToShownHandCards(ids2);

        return false;
    }
};

class Dongcha : public TriggerSkill
{
public:
    Dongcha() : TriggerSkill("dongcha")
    {
        events << CardUsed << CardResponded;
    }

    static bool dongcha(ServerPlayer *satori, const Card *card)
    {
        foreach(const Card *c, satori->getCards("s")) {
            if (c->getTypeId() == card->getTypeId()) {
                return true;
            }
        }
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (e == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
                foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (use.from != p && dongcha(p, use.card))
                        d << SkillInvokeDetail(this, p, p, NULL, false, use.from);
                }
        } else {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_isUse) {
                foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (response.m_from != p && dongcha(p, response.m_card))
                        d << SkillInvokeDetail(this, p, p, NULL, false, response.m_from);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        const Card *card = (e == CardUsed) ? data.value<CardUseStruct>().card : data.value<CardResponseStruct>().m_card;
        
        QStringList prompt_list;
        prompt_list << "@dongcha" << card->objectName()
            << invoke->preferredTarget->objectName() << card->getType();
        QString prompt = prompt_list.join(":");

        QString type = card->getType();
        QString pattern = QString("%1Card|.|.|show").arg(type.left(1).toUpper() + type.right(type.length() - 1));
        const Card *card1 = room->askForCard(invoke->invoker, pattern, prompt, data, Card::MethodNone, NULL, false, objectName());
        if (card1 != NULL) {
            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = invoke->invoker;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(invoke->invoker, objectName());

            QList<int> ids;
            ids << card1->getEffectiveId();
            invoke->invoker->removeShownHandCards(ids, true);
        }

        return card1 != NULL;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            // SkillCard has already excluded
            room->touhouLogmessage(use.card->isKindOf("Nullification") ? "$CancelWithoutTarget" : "$CancelTarget", use.from, use.card->objectName(), use.to);
            if (use.card->isKindOf("Nullification"))
                room->setPlayerFlag(use.from, "nullifiationNul");
            else
                use.to.clear();
            data = QVariant::fromValue(use);
            if (!use.m_showncards.isEmpty() && !use.from->getCards("h").isEmpty()) {
                int id = room->askForCardChosen(invoke->invoker, use.from, "h", objectName());
                QList<int> show_ids;
                show_ids << id;
                use.from->addToShownHandCards(show_ids);
            }
        } else {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            resp.m_isNullified = true;
            data = QVariant::fromValue(resp);
            room->touhouLogmessage("$CancelWithoutTarget", resp.m_from, resp.m_card->objectName(), QList<ServerPlayer *>());
            if (resp.m_isShowncard && !resp.m_from->getCards("h").isEmpty()) {
                int id = room->askForCardChosen(invoke->invoker, resp.m_from, "h", objectName());
                QList<int> show_ids;
                show_ids << id;
                resp.m_from->addToShownHandCards(show_ids);
            }
        }

        return false;
    }
};

class Zhuiyi : public TriggerSkill
{
public:
    Zhuiyi() : TriggerSkill("zhuiyi")
    {
        events << BeforeCardsMove << CardsMoveOneTime;
        frequency = Compulsory;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (e == BeforeCardsMove) {
            if (move.from != NULL && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD
                && move.from_places.contains(Player::PlaceHand) && move.to_place == Player::DiscardPile) {
                QVariantList record_ids = room->getTag("zhuiyi").toList();
                foreach(int id, move.card_ids) {
                    if (move.from->isShownHandcard(id) && !record_ids.contains(id))
                        record_ids << id;
                }
                room->setTag("zhuiyi", record_ids);
            }
            return;
        } else {
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_DISCARD ||
                !move.from_places.contains(Player::PlaceHand)  || move.to_place != Player::DiscardPile) {
                QVariantList record_ids = room->getTag("zhuiyi").toList();
                foreach(int id, move.card_ids)
                    record_ids.removeOne(id);
                room->setTag("zhuiyi", record_ids);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (e == CardsMoveOneTime) {
            QVariantList record_ids = room->getTag("zhuiyi").toList();
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *from = qobject_cast<ServerPlayer *>(move.from);
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD
                && move.from_places.contains(Player::PlaceHand)  && move.to_place == Player::DiscardPile && from != NULL) {
                foreach(int id, move.card_ids) {
                    if (record_ids.contains(id)) {
                        foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                            if (p != from)
                                d << SkillInvokeDetail(this, p, p, NULL, true);
                        }    
                        break;
                    }
                }
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QVariantList record_ids = room->getTag("zhuiyi").toList();
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        QList<int> get_ids;

        foreach(int id, move.card_ids) {
            if (record_ids.contains(id))
                get_ids << id;
        }

        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());

        CardsMoveStruct mo;
        mo.card_ids = get_ids;
        mo.to = invoke->invoker;
        mo.to_place = Player::PlaceHand;
        room->moveCardsAtomic(mo, true);

        QList<int> show_ids;
        foreach(int id, mo.card_ids) {
            if (room->getCardOwner(id) == invoke->invoker && room->getCardPlace(id) == Player::PlaceHand)
                show_ids << id;
        }

        if (!show_ids.isEmpty())
            invoke->invoker->addToShownHandCards(show_ids);
        return false;
    }
};




class Fanhun : public TriggerSkill
{
public:
    Fanhun() : TriggerSkill("fanhun")
    {
        events << EventPhaseStart << Dying;
        frequency = Eternal;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {

        if (e == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->getPhase() == Player::Finish && player->getMaxHp() > 4)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        } else if (e == Dying) {
            ServerPlayer *who = data.value<DyingStruct>().who;
            if (who->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, who, who, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }


    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (e == Dying) {
            room->setPlayerProperty(invoke->invoker, "maxhp", invoke->invoker->getMaxHp() + 1);
            RecoverStruct recov;
            recov.recover = invoke->invoker->getMaxHp() - invoke->invoker->getHp();
            room->recover(invoke->invoker, recov);
            invoke->invoker->drawCards(invoke->invoker->getMaxHp());
        } else if (e == EventPhaseStart)
            room->killPlayer(invoke->invoker);
        return false;
    }
};


//the effect is in  Player::dyingThreshold()
//this triggerskill is for skilleffect
class Yousi : public TriggerSkill
{
public:
    Yousi() : TriggerSkill("yousi")
    {
        events << PostHpReduced;
        frequency = Compulsory;
    }


    void record(TriggerEvent, Room *room, QVariant &data) const
    {
        ServerPlayer *victim = NULL;
        if (data.canConvert<DamageStruct>())
            victim = data.value<DamageStruct>().to;
        else
            victim = data.value<HpLostStruct>().player;

        ServerPlayer *uuz = room->getCurrent();
        if (uuz && uuz->isAlive() && uuz != victim && uuz->hasSkill(this) && victim->getHp() < victim->dyingThreshold()) {
            room->notifySkillInvoked(uuz, objectName());
        }
    }
};


class Tianqu : public TriggerSkill
{
public:
    Tianqu() : TriggerSkill("tianqu")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() != Player::Start)
            return QList<SkillInvokeDetail>();
        foreach(ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("@road") > 0) {
                return QList<SkillInvokeDetail>();
            }
        }
        foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName()))
            d << SkillInvokeDetail(this, p, p, NULL, true);
        return d;
    }


    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->gainMark("@road");
        return false;
    }
};

class Duyu : public TriggerSkill
{
public:
    Duyu() : TriggerSkill("duyu")
    {
        events << CardUsed << CardFinished << EventPhaseChanging;
        frequency = Compulsory;
    }

    static ServerPlayer *adjanceny(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        int matchCount = (player->getSeat() == room->getAllPlayers().length()) ? 1 : player->getSeat() + 1;

        foreach(ServerPlayer *p, room->getAllPlayers()) {
            if (p->getSeat() == matchCount)
                return p;
        }
        return player;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        //record times of using card
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from->isCurrent() && use.from->getPhase() == Player::Play && use.card->isKindOf("Slash")) {
                if (!use.from->hasFlag("duyu")) {
                    use.from->setFlags("duyu");
                    room->setCardFlag(use.card, "duyu");
                }
            }
        }
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                change.player->setFlags("-duyu");
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.from->isCurrent() && use.from->getPhase() == Player::Play && use.card->hasFlag("duyu")) {
                ServerPlayer *starter = NULL;
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->getMark("@road") > 0) {
                        starter = p;
                        break;
                    }
                }
                if (starter != NULL) {
                    foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                        if (p != use.from)
                            d << SkillInvokeDetail(this, p, p, NULL, true, starter);
                    }
                }
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        bool move = true;
        if (use.from && use.from->isAlive() && use.from != invoke->invoker)
            move = !room->askForUseSlashTo(invoke->invoker, use.from, "@duyu:" + use.from->objectName() + ":" + invoke->targets.first()->objectName());
        if (move) {
            invoke->targets.first()->loseMark("@road");
            ServerPlayer *next = adjanceny(invoke->targets.first());
            next->gainMark("@road");
        }

        return false;
    }
};

FengmiCard::FengmiCard()
{
    will_throw = false;
}

bool FengmiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(Card::SuitToBeDecided, -1);
    slash->deleteLater();
    slash->setSkillName("fengmi");

    bool can = false;
    foreach(const Player *p, targets) {
        if (Self->distanceTo(p) == subcards.length()) {
            can = true;
            break;
        }
    }
    if (can)
        return slash->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, slash, targets);
    else
        return  (Self->distanceTo(to_select) == subcards.length()) && slash->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, slash, targets);
}

bool FengmiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    bool can = false;
    foreach(const Player *p, targets) {
        if (Self->distanceTo(p) == subcards.length()) {
            can = true;
            break;
        }
    }
    Slash *slash = new Slash(Card::SuitToBeDecided, -1);
    slash->deleteLater();
    slash->setSkillName("fengmi");
    return can && slash->targetsFeasible(targets, Self);
}

const Card *FengmiCard::validate(CardUseStruct &) const
{
    Slash *slash = new Slash(Card::SuitToBeDecided, -1);
    slash->addSubcards(subcards);
    slash->setSkillName("fengmi");
    return slash;
}

const Card *FengmiCard::validateInResponse(ServerPlayer *) const
{
    Slash *slash = new Slash(Card::SuitToBeDecided, -1);
    slash->addSubcards(subcards);
    slash->setSkillName("fengmi");
    return slash;
}


class FengmiVS : public ViewAsSkill
{
public:
    FengmiVS() : ViewAsSkill("fengmi")
    {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player);
    }

    virtual bool viewFilter(const QList<const Card*> &, const Card*to_select) const
    {
        return !to_select->isEquipped();
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return matchAvaliablePattern("slash", pattern) &&
            Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 0)
            return NULL;
        //Slash *slash = new Slash(Card::SuitToBeDecided, -1);
        //slash->addSubcards(cards);
        //slash->setSkillName(objectName());
        //return slash;
        FengmiCard *card = new FengmiCard;
        card->addSubcards(cards);
        return card;
    }
};

class Fengmi : public TriggerSkill
{
public:
    Fengmi() : TriggerSkill("fengmi")
    {
        events << CardUsed;
        view_as_skill = new FengmiVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getSkillName() == objectName() && use.card->isRed() && use.from->isAlive())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, true);

        return QList<SkillInvokeDetail>();
    }

    // compulsory effect, cost omitted

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->drawCards(1);
        return false;
    }
};

class FengmiTargetMod : public TargetModSkill
{
public:
    FengmiTargetMod() : TargetModSkill("#fengmi")
    {
        pattern = "Slash";
    }
    virtual int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->getSkillName() == "fengmi")
            return 1000;
        return 0;
    }
};

TouhouGodPackage::TouhouGodPackage()
    : Package("touhougod")
{
    General *yukari_god = new General(this, "yukari_god", "touhougod", 4, false);
    yukari_god->addSkill(new Jiexian);

    General *remilia_god = new General(this, "remilia_god", "touhougod", 3, false);
    remilia_god->addSkill(new Zhouye);
    remilia_god->addSkill(new Hongwu);
    remilia_god->addSkill(new Shenqiang);
    remilia_god->addSkill(new Yewang);

    General *cirno_god = new General(this, "cirno_god", "touhougod", 4, false);
    cirno_god->addSkill(new Aoyi);
    cirno_god->addSkill(new AoyiEffect);
    cirno_god->addSkill(new AoyiTargetMod);
    related_skills.insertMulti("aoyi", "#aoyi_mod");
    related_skills.insertMulti("aoyi", "#aoyi");

    General *utsuho_god = new General(this, "utsuho_god", "touhougod", 4, false);
    utsuho_god->addSkill(new Shikong);
    utsuho_god->addSkill(new Ronghui);
    utsuho_god->addSkill(new Jubian);
    utsuho_god->addSkill(new Hengxing);

    General *suika_god = new General(this, "suika_god", "touhougod", 0, false);
    suika_god->addSkill(new Huanmeng);
    suika_god->addSkill(new Cuixiang);
    suika_god->addSkill(new Xuying);

    General *flandre_god = new General(this, "flandre_god", "touhougod", 3, false);
    flandre_god->addSkill(new Kuangyan);
    flandre_god->addSkill(new Huimie);
    flandre_god->addSkill(new Jinguo);

    General *sakuya_god = new General(this, "sakuya_god", "touhougod", 3, false);
    sakuya_god->addSkill(new Shicao);
    sakuya_god->addSkill(new Shiting);
    sakuya_god->addSkill(new Huanzai);
    sakuya_god->addSkill(new Shanghun);

    General *youmu_god = new General(this, "youmu_god", "touhougod", 3, false);
    youmu_god->addSkill(new Banling);
    youmu_god->addSkill(new Rengui);

    General *reisen_god = new General(this, "reisen_god", "touhougod", 4, false);
    reisen_god->addSkill(new Ningshi);
    reisen_god->addSkill(new Gaoao);

    General *sanae_god = new General(this, "sanae_god", "touhougod", 4, false);
    sanae_god->addSkill(new Shenshou);

    General *reimu_god = new General(this, "reimu_god", "touhougod", 4, false);
    reimu_god->addSkill(new Yibian);
    reimu_god->addSkill(new Fengyin);
    reimu_god->addSkill(new Huanxiang);

    General *shikieiki_god = new General(this, "shikieiki_god", "touhougod", 4, false);
    shikieiki_god->addSkill(new Quanjie);
    shikieiki_god->addSkill(new Duanzui);

    General *meirin_god = new General(this, "meirin_god", "touhougod", 4, false);
    meirin_god->addSkill(new Huaxiang);
    meirin_god->addSkill(new Caiyu);
    meirin_god->addSkill(new Xuanlan);

    General *eirin_god = new General(this, "eirin_god", "touhougod", 4, false);
    eirin_god->addSkill(new Qiannian);
    eirin_god->addSkill(new QiannianMax);
    related_skills.insertMulti("qiannian", "#qiannian_max");

    General *kanako_god = new General(this, "kanako_god", "touhougod", 4, false);
    kanako_god->addSkill(new Qinlue);
    kanako_god->addSkill(new QinlueEffect);
    related_skills.insertMulti("qinlue", "#qinlue_effect");

    General *byakuren_god = new General(this, "byakuren_god", "touhougod", 4, false);
    byakuren_god->addSkill(new Chaoren);

    General *koishi_god = new General(this, "koishi_god", "touhougod", 3, false);
    koishi_god->addSkill(new Biaoxiang);
    koishi_god->addSkill(new Shifang);
    koishi_god->addSkill(new Yizhi);
    koishi_god->addRelateSkill("ziwo");
    koishi_god->addRelateSkill("benwo");
    koishi_god->addRelateSkill("chaowo");

    General *suwako_god = new General(this, "suwako_god", "touhougod", 5, false);
    suwako_god->addSkill(new Zuosui);
    suwako_god->addSkill(new Worao);
    suwako_god->addSkill(new Shenhua);

    General *miko_god = new General(this, "miko_god", "touhougod", 4, false);
    miko_god->addSkill(new Hongfo);
    miko_god->addSkill(new Junwei);
    miko_god->addSkill(new Gaizong);
    miko_god->addRelateSkill("wendao");

    General *kaguya_god = new General(this, "kaguya_god", "touhougod", 4, false);
    kaguya_god->addSkill(new Shenbao);
    kaguya_god->addSkill(new ShenbaoDistance);
    kaguya_god->addSkill(new ShenbaoHandler);
    related_skills.insertMulti("shenbao", "#shenbao_distance");
    related_skills.insertMulti("shenbao", "#shenbao");

    General *komachi_god = new General(this, "komachi_god", "touhougod", 4, false);
    komachi_god->addSkill(new Yindu);
    komachi_god->addSkill(new Huanming);
    komachi_god->addSkill(new Chuanwu);

    General *seiga_god = new General(this, "seiga_god", "touhougod", 3, false);
    seiga_god->addSkill(new Huanhun);
    seiga_god->addSkill(new Tongling);
    seiga_god->addSkill(new Rumo);

    General *satori_god = new General(this, "satori_god", "touhougod", 3, false);
    satori_god->addSkill(new Xinhua);
    satori_god->addSkill(new Dongcha);
    satori_god->addSkill(new Zhuiyi);

    General *yuyuko_god = new General(this, "yuyuko_god", "touhougod", 1, false);
    yuyuko_god->addSkill(new Fanhun);
    yuyuko_god->addSkill(new Yousi);

    General *aya_god = new General(this, "aya_god", "touhougod", 4, false);
    aya_god->addSkill(new Tianqu);
    aya_god->addSkill(new Duyu);
    aya_god->addSkill(new Fengmi);
    aya_god->addSkill(new FengmiTargetMod);
    related_skills.insertMulti("fengmi", "#fengmi");

    General *uuz13 = new General(this, "yuyuko_1v3", "touhougod", 1, false, true);
    Q_UNUSED(uuz13);

    addMetaObject<HongwuCard>();
    addMetaObject<ShenqiangCard>();
    addMetaObject<HuimieCard>();
    addMetaObject<ShenshouCard>();
    addMetaObject<FengyinCard>();
    addMetaObject<HuaxiangCard>();
    addMetaObject<ZiwoCard>();
    addMetaObject<ChaowoCard>();
    addMetaObject<WendaoCard>();
    addMetaObject<RumoCard>();
    addMetaObject<FengmiCard>();

    skills << new Ziwo << new Benwo << new Chaowo << new Wendao << new ShenbaoSpear << new RoleShownHandler;
}

ADD_PACKAGE(TouhouGod)

