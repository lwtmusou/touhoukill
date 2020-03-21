#include "touhougod.h"
#include "client.h"
#include "compiler-specific.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "roomthread.h"
#include "settings.h"
#include "skill.h"
#include "standard.h"
#include "testCard.h"
#include "th09.h"
#include "th10.h"
#include "th15.h"
#include <QCommandLinkButton>
#include <QCoreApplication>
#include <QMetaObject>
#include <QPointer>

class Chuanghuan : public TriggerSkill
{
public:
    Chuanghuan()
        : TriggerSkill("chuanghuan")
    {
        events << GameStart;
    }

    virtual int getPriority() const
    {
        return 5;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (data.isNull()) {
            foreach (ServerPlayer *zun, room->findPlayersBySkillName(objectName()))
                d << SkillInvokeDetail(this, zun, zun, NULL, true);
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());

        //get ganerals
        QSet<QString> all = Sanguosha->getLimitedGeneralNames().toSet();
        if (isNormalGameMode(room->getMode()) || room->getMode().contains("_mini_") || room->getMode() == "custom_scenario")
            all.subtract(Config.value("Banlist/Roles", "").toStringList().toSet());
        else if (room->getMode() == "06_XMode") {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["XModeBackup"].toStringList().toSet());
        } else if (room->getMode() == "02_1v1") {
            all.subtract(Config.value("Banlist/1v1", "").toStringList().toSet());
            foreach (ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["1v1Arrange"].toStringList().toSet());
        }

        QSet<QString> room_set;
        foreach (ServerPlayer *player, room->getAllPlayers(true)) {
            QString name = player->getGeneralName();
            if (Sanguosha->isGeneralHidden(name))
                continue;
            room_set << name;

            if (!player->getGeneral2())
                continue;
            name = player->getGeneral2Name();
            if (Sanguosha->isGeneralHidden(name))
                continue;
            room_set << name;
        }

        QStringList names = (all - room_set).toList();
        qShuffle(names);
        if (names.isEmpty())
            return false;

        int n = room->getAlivePlayers().length();
        n = qMin(n, names.length());
        QStringList generals = names.mid(0, n);

        room->getThread()->delay(1000);
        //change hero
        QString general = room->askForGeneral(invoke->invoker, generals);
        room->changeHero(invoke->invoker, general, true, true, false, true);
        QString kingdom = invoke->invoker->getGeneral()->getKingdom();
        room->setPlayerProperty(invoke->invoker, "kingdom", kingdom);

        return false;
    }
};

class Jiexian : public TriggerSkill
{
public:
    Jiexian()
        : TriggerSkill("jiexian")
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
                    if (yukari->canDiscard(yukari, c->getEffectiveId()) && event == DamageInflicted && c->getSuit() == Card::Heart) {
                        d << SkillInvokeDetail(this, yukari, yukari);
                        break;
                    } else if (yukari->canDiscard(yukari, c->getEffectiveId()) && event == PreHpRecover && c->getSuit() == Card::Spade) {
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
    Zhouye()
        : TriggerSkill("zhouye")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseStart << PreMarkChange << EventSkillInvalidityChange;
        frequency = Compulsory;
        related_mark = "@ye";
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        ServerPlayer *player = NULL;
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
            if (a.skill->objectName() == "zhouye" && a.player->getMark("@ye") == 0)
                set = true;
        } else if (triggerEvent == GameStart) {
            player = data.value<ServerPlayer *>();
            if (player && player->hasSkill("zhouye"))
                set = true;
        } else if (triggerEvent == EventSkillInvalidityChange) {
            QList<SkillInvalidStruct> invalids = data.value<QList<SkillInvalidStruct> >();
            foreach (SkillInvalidStruct v, invalids) {
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
            if (set && !player->isCardLimited("use", "zhouye")) {
                room->setPlayerCardLimitation(player, "use", "Slash", objectName(), false);
                room->setPlayerMark(player, "zhouye_limit", 1);
            } else if (remove && player->isCardLimited("use", "zhouye")) {
                room->removePlayerCardLimitation(player, "use", "Slash$0", objectName());
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
    Hongwu()
        : ViewAsSkill("hongwu")
    {
        related_mark = "@ye";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@ye") == 0;
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
    Shenqiang()
        : OneCardViewAsSkill("shenqiang")
    {
        related_mark = "@ye";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@ye") > 0;
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return (to_select->isKindOf("Weapon") || to_select->getSuit() == Card::Heart) && !Self->isJilei(to_select);
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
    Yewang()
        : TriggerSkill("yewang")
    {
        events << DamageInflicted;
        frequency = Compulsory;
        related_mark = "@ye";
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

class Bingfeng : public TriggerSkill
{
public:
    Bingfeng()
        : TriggerSkill("bingfeng")
    {
        events << Dying << DamageCaused << EventPhaseStart;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == Dying) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.damage && dying.damage->from) {
                ServerPlayer *killer = dying.damage->from;
                if (killer->hasSkill(this) && killer != dying.who && killer->isAlive() && dying.who->getMark("@ice") == 0 && dying.who->getHp() < dying.who->dyingThreshold())
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, killer, killer, NULL, true);
            }
        } else if (e == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->hasSkill(this) && damage.from->isAlive() && damage.to->getMark("@ice") > 0)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true);
        } else if (e == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->getPhase() == Player::RoundStart) {
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->getMark("@ice") == 0)
                        return QList<SkillInvokeDetail>();
                }
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        room->notifySkillInvoked(invoke->invoker, objectName());
        if (e == Dying) {
            DyingStruct dying = data.value<DyingStruct>();
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), dying.who->objectName());
            room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
            dying.who->gainMark("@ice");
        } else if (e == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            QList<ServerPlayer *> logto;
            logto << damage.to;
            room->touhouLogmessage("#Bingfeng", invoke->invoker, objectName(), logto, QString::number(1));

            damage.damage = damage.damage - 1;
            data = QVariant::fromValue(damage);
            if (damage.damage == 0)
                return true;
        } else if (e == EventPhaseStart) {
            QString winner;
            QString role = invoke->invoker->getRole();

            if (role == "lord" || role == "loyalist")
                winner = "lord+loyalist";
            else if (role == "rebel")
                winner = "rebel";
            else if (role == "renegade")
                winner = invoke->invoker->objectName();
            if (!winner.isNull()) {
                room->touhouLogmessage("#BingfengWin", invoke->invoker, role);
                room->gameOver(winner);
                return true;
            }
        }
        return false;
    }
};

//change damage.from in gamerule  SlashHit
class Wushen : public TriggerSkill
{
public:
    Wushen()
        : TriggerSkill("wushen")
    {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.from->getWeapon() != NULL) {
                ServerPlayer *source = room->findPlayerBySkillName(objectName());
                if (source && source != use.from)
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, source, source, NULL, true);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.card->setFlags("WushenDamage_" + invoke->invoker->objectName());

            room->notifySkillInvoked(invoke->invoker, objectName());
            LogMessage log;
            log.type = "$WushenChange";
            log.arg = objectName();
            log.from = invoke->invoker;
            log.card_str = use.card->toString();
            room->sendLog(log);
        }
        return false;
    }
};

class Shikong : public TriggerSkill
{
public:
    Shikong()
        : TriggerSkill("shikong")
    {
        events << CardUsed;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.from->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.to.clear();
        room->notifySkillInvoked(invoke->invoker, objectName());
        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (invoke->invoker->canSlash(p, use.card)) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), p->objectName());
                use.to << p;
            }
        }
        room->touhouLogmessage("#Shikong", invoke->invoker, objectName(), use.to);
        data = QVariant::fromValue(use);
        return false;
    }
};

class Ronghui : public TriggerSkill
{
public:
    Ronghui()
        : TriggerSkill("ronghui")
    {
        events << DamageCaused;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user)
            return QList<SkillInvokeDetail>();

        if (damage.from && damage.from != damage.to && damage.from->hasSkill(this) && damage.card && damage.card->isKindOf("Slash") && damage.from->canDiscard(damage.to, "e"))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
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
    Jubian()
        : TriggerSkill("jubian")
    {
        events << Damage << CardFinished;
        frequency = Compulsory;
    }

    void record(TriggerEvent triggerEvent, Room *, QVariant &data) const
    {
        if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != NULL && damage.from) {
                if (damage.card->hasFlag("jubian_card")) {
                    if (!damage.card->hasFlag("jubian_used"))
                        damage.card->setFlags("jubian_used");
                } else
                    damage.card->setFlags("jubian_card");
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->hasFlag("jubian_card") && use.card->hasFlag("jubian_used") && use.from->hasSkill(this) && !use.from->hasFlag("Global_ProcessBroken")
                && use.from->isWounded())
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
    Hengxing()
        : TriggerSkill("hengxing")
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
    Huanmeng()
        : TriggerSkill("huanmeng")
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
    Cuixiang()
        : TriggerSkill("cuixiang")
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
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
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

        if (player->isDead())
            return false;

        //we need id to check cardplace,
        //since skill "jinian",  the last handcard will be return.
        QList<int> able;
        foreach (int id, idlist) {
            if (room->getCardPlace(id) == Player::DiscardPile)
                able << id;
        }

        int x = qMin(able.length(), 2);
        if (x == 0)
            return false;
        room->fillAG(able, NULL);
        for (int i = 0; i < x; i++) {
            int card_id = room->askForAG(player, able, false, "cuixiang");
            //just for displaying chosen card in ag container
            room->takeAG(player, card_id, false);
            room->obtainCard(player, card_id, true);
            able.removeOne(card_id);
            if (able.isEmpty())
                break;
        }
        room->clearAG();
        return false;
    }
};

class Xuying : public TriggerSkill
{
public:
    Xuying()
        : TriggerSkill("xuying")
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
        } else if (triggerEvent == SlashMissed && effect.to->hasSkill(objectName()))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
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
    Kuangyan()
        : TriggerSkill("kuangyan")
    {
        events << Dying;
        related_mark = "@kinki";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *who = data.value<DyingStruct>().who;
        if (who->isCurrent() || !who->hasSkill(this) || who->isDead() || who->getHp() >= who->dyingThreshold())
            return QList<SkillInvokeDetail>();
        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, who, who);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        player->gainMark("@kinki");
        RecoverStruct recover;
        recover.recover = player->dyingThreshold() - player->getHp();
        room->recover(player, recover);

        ServerPlayer *current = room->getCurrent();
        if (current && current->isAlive()) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), current->objectName());
            room->damage(DamageStruct(objectName(), player, current));
        }
        return false;
    }
};

HuimieCard::HuimieCard()
{
}

bool HuimieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return (targets.isEmpty() && to_select != Self);
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
    Huimie()
        : ZeroCardViewAsSkill("huimie")
    {
        related_mark = "@kinki";
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
    Jinguo()
        : TriggerSkill("jinguo")
    {
        events << EventPhaseEnd;
        frequency = Compulsory;
        related_mark = "@kinki";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getPhase() == Player::Play && player->isAlive())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
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
        if (judge.ignore_judge)
            return false;
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
    Shicao()
        : TriggerSkill("shicao")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
        related_mark = "@clock";
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
    Shiting()
        : TriggerSkill("shiting")
    {
        events << EventPhaseChanging;
        related_mark = "@clock";
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
        ServerPlayer *next = qobject_cast<ServerPlayer *>(current->getNextAlive(1, false));
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
    Huanzai()
        : TriggerSkill("huanzai")
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
    Shanghun()
        : TriggerSkill("shanghun")
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
    Banling()
        : TriggerSkill("banling")
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
                int initHp = player->getHp();
                room->setPlayerProperty(player, "renhp", initHp);
                room->setPlayerProperty(player, "linghp", initHp);
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
    Rengui()
        : PhaseChangeSkill("rengui")
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
            int x = qMin(player->getMaxHp() - player->getLingHp(), 2);
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
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (player->canDiscard(p, "hes"))
                    all << p;
            }
            if (all.isEmpty())
                return false;
            ServerPlayer *s = room->askForPlayerChosen(player, all, "renguidiscard", "@rengui-discard:" + QString::number(y), true, true);
            if (s == NULL)
                return false;
            for (int i = 0; i < y; ++i) {
                if (player->isDead() || !player->canDiscard(s, "hes"))
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
    Ningshi()
        : TriggerSkill("ningshi")
    {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from->hasSkill(this) && use.from->getPhase() == Player::Play && use.to.length() == 1 && !use.to.contains(use.from)) {
            if (use.card->isKindOf("Slash") || use.card->isKindOf("TrickCard"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
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
    Gaoao()
        : TriggerSkill("gaoao")
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
        QList<int> ids;
        foreach (int id, move.card_ids) {
            if (room->getCardPlace(id) != Player::DiscardPile)
                ids << id;
        }

        move.card_ids.clear();
        move.from_places.clear();
        move.from_pile_names.clear();

        data = QVariant::fromValue(move);

        if (!ids.isEmpty()) {
            bool isUse = ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE && move.from != NULL);
            DummyCard dummy(ids);
            CardMoveReason reason(isUse ? CardMoveReason::S_REASON_USE : CardMoveReason::S_REASON_NATURAL_ENTER, isUse ? move.from->objectName() : player->objectName(),
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
            QList<ServerPlayer *> listt;
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
            QList<ServerPlayer *> listt;
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
    Shenshou()
        : OneCardViewAsSkill("shenshou")
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
    Yibian()
        : TriggerSkill("yibian")
    {
        events << EventPhaseStart;
        show_type = "static";
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
    return (targets.isEmpty() && to_select->hasShownRole());
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
    Fengyin()
        : OneCardViewAsSkill("fengyin")
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
    Huanxiang()
        : TriggerSkill("huanxiang")
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
        foreach (ServerPlayer *p1, room->getAlivePlayers()) {
            if (p1->getRole() == "renegade") {
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
    RoleShownHandler()
        : TriggerSkill("roleshownhandler")
    {
        events << GameStart;
        global = true;
    }

    void record(TriggerEvent, Room *room, QVariant &) const
    {
        if (isHegemonyGameMode(ServerInfo.GameMode))
            return;

        foreach (ServerPlayer *p, room->getAlivePlayers())
            room->setPlayerProperty(p, "role_shown", p->isLord() ? true : false);
    }
};

class Quanjie : public TriggerSkill
{
public:
    Quanjie()
        : TriggerSkill("quanjie")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Play) {
            foreach (ServerPlayer *src, room->findPlayersBySkillName(objectName())) {
                if (player != src)
                    d << SkillInvokeDetail(this, src, src);
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
        if (card == NULL) {
            player->drawCards(1);
            room->setPlayerCardLimitation(player, "use", "Slash", objectName(), true);
        }
        return false;
    }
};

class Duanzui : public TriggerSkill
{
public:
    Duanzui()
        : TriggerSkill("duanzui")
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
                        foreach (ServerPlayer *ymsnd, room->findPlayersBySkillName(objectName())) {
                            if (!ymsnd->isCurrent())
                                d << SkillInvokeDetail(this, ymsnd, ymsnd);
                        }
                    } else {
                        foreach (ServerPlayer *ymsnd, room->findPlayersBySkillName(objectName())) {
                            if (!ymsnd->isCurrent() && !ymsnd->faceUp())
                                d << SkillInvokeDetail(this, ymsnd, ymsnd);
                        }
                    }
                }
            }
        } else if (e == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player && player->getPhase() == Player::RoundStart && player->getMark("@duanzui-extra") > 0)
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

        ServerPlayer *current = room->getCurrent();
        ServerPlayer *ymsnd = invoke->invoker;
        if (!ymsnd->faceUp())
            ymsnd->turnOver();
        bool extraTurnExist = room->getThread()->hasExtraTurn();
        if (extraTurnExist)
            return false;

        ymsnd->gainMark("@duanzui-extra");
        QList<ServerPlayer *> logto;
        logto << qobject_cast<ServerPlayer *>(current->getNext(false));
        room->touhouLogmessage("#touhouExtraTurn", ymsnd, NULL, logto);
        ymsnd->gainAnExtraTurn();

        return false;
    }
};

HuaxiangCard::HuaxiangCard()
{
    will_throw = false;
    handling_method = Card::MethodNone; //related to UseCardLimit
}

bool HuaxiangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return false;
    if (user_string == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(user_string.split("+").first(), Card::NoSuit, 0);
    DELETE_OVER_SCOPE(Card, card)
    card->setSkillName("huaxiang");
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool HuaxiangCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    if (!user_string.isEmpty())
        return false;
    Card *card = Sanguosha->cloneCard(user_string.split("+").first(), Card::NoSuit, 0);
    DELETE_OVER_SCOPE(Card, card)
    card->setSkillName("huaxiang");
    return card && card->targetFixed();
}

bool HuaxiangCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    if (user_string == NULL)
        return false;
    Card *card = Sanguosha->cloneCard(user_string.split("+").first(), Card::NoSuit, 0);
    DELETE_OVER_SCOPE(Card, card)
    card->setSkillName("huaxiang");
    return card && card->targetsFeasible(targets, Self);
}

const Card *HuaxiangCard::validate(CardUseStruct &card_use) const
{
    card_use.from->showHiddenSkill("huaxiang");
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
    user->showHiddenSkill("huaxiang");
    Room *room = user->getRoom();
    room->touhouLogmessage("#InvokeSkill", user, "huaxiang");
    user->addToPile("rainbow", subcards.first());
    Card *use_card = Sanguosha->cloneCard(user_string, Card::NoSuit, 0);
    use_card->setSkillName("huaxiang");
    use_card->deleteLater();

    return use_card;
}

class Huaxiang : public ViewAsSkill
{
public:
    Huaxiang()
        : ViewAsSkill("huaxiang")
    {
        related_pile = "rainbow";
    }

    static QStringList responsePatterns()
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();

        Card::HandlingMethod method;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            method = Card::MethodResponse;
        else
            method = Card::MethodUse;

        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        const Skill *skill = Sanguosha->getSkill("huaxiang");
        QStringList checkedPatterns;
        QStringList ban_list = Sanguosha->getBanPackages();
        foreach (const Card *card, cards) {
            if ((card->isKindOf("BasicCard") || card->isKindOf("Nullification"))
                && !ban_list.contains(card->getPackage())) { //&& !ServerInfo.Extensions.contains("!" + card->getPackage())
                QString name = card->objectName();
                if (!checkedPatterns.contains(name) && skill->matchAvaliablePattern(name, pattern) && !Self->isCardLimited(card, method)) {
                    if (name.contains("jink") && Self->getMaxHp() > 3)
                        continue;
                    else if (name.contains("peach") && Self->getMaxHp() > 2)
                        continue;
                    else if (name.contains("nullification") && Self->getMaxHp() > 1)
                        continue;
                    checkedPatterns << name;
                }
            }
        }
        return checkedPatterns;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getPile("rainbow").length() > 3)
            return false;
        if (player->isKongcheng())
            return false;
        if (Slash::IsAvailable(player) || Analeptic::IsAvailable(player))
            return true;
        if (player->getMaxHp() <= 2) {
            Card *card = Sanguosha->cloneCard("peach", Card::NoSuit, 0);
            DELETE_OVER_SCOPE(Card, card)
            Card *card1 = Sanguosha->cloneCard("super_peach", Card::NoSuit, 0);
            DELETE_OVER_SCOPE(Card, card1)
            return card->isAvailable(player) || card1->isAvailable(player);
        }
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &) const
    {
        if (player->getPile("rainbow").length() > 3)
            return false;
        if (player->isKongcheng())
            return false;

        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.contains("peach") && checkedPatterns.length() == 1 && player->getMark("Global_PreventPeach") > 0)
            return false;

        return !checkedPatterns.isEmpty();
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (to_select->isEquipped() || !selected.isEmpty())
            return false;

        foreach (int id, Self->getPile("rainbow")) {
            Card *card = Sanguosha->getCard(id);
            if (card->getSuit() == to_select->getSuit())
                return false;
        }

        return selected.isEmpty();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 1)
            return NULL;

        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.length() == 1) {
            HuaxiangCard *card = new HuaxiangCard;
            card->setUserString(checkedPatterns.first());
            card->addSubcards(cards);
            return card;
        }

        QString name = Self->tag.value("huaxiang", QString()).toString();
        if (name != NULL) {
            HuaxiangCard *card = new HuaxiangCard;
            card->setUserString(name);
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
        if (player->getPile("rainbow").length() > 3)
            return false;
        if (player->isKongcheng())
            return false;
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
    Caiyu()
        : TriggerSkill("caiyu")
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
        QList<int> idlist = merin->getPile("rainbow");

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
    Xuanlan()
        : TriggerSkill("xuanlan")
    {
        events << EventPhaseChanging;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Discard && change.player->hasSkill(this) && !change.player->isWounded())
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
    QiannianMax()
        : MaxCardsSkill("#qiannian_max")
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
    Qiannian()
        : TriggerSkill("qiannian")
    {
        events << GameStart << DrawNCards << DrawPileSwaped;
        frequency = Compulsory;
        related_mark = "@qiannian";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == GameStart) {
            ServerPlayer *erin = data.value<ServerPlayer *>();
            if (erin && erin->hasSkill(this))
                d << SkillInvokeDetail(this, erin, erin, NULL, true);
        } else if (triggerEvent == DrawPileSwaped) {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
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
    Qinlue()
        : TriggerSkill("qinlue")
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
            foreach (ServerPlayer *source, room->findPlayersBySkillName(objectName())) {
                if (current != source && source->canDiscard(source, "hes"))
                    d << SkillInvokeDetail(this, source, source);
            }
            return d;
        } else if (change.to == Player::NotActive) {
            if (!change.player)
                return QList<SkillInvokeDetail>();
            foreach (ServerPlayer *player, room->getOtherPlayers(change.player)) {
                ExtraTurnStruct extra = player->tag["ExtraTurnInfo"].value<ExtraTurnStruct>();
                if (extra.reason == "qinlue" && extra.extraTarget == change.player) {
                    if (!player->faceUp())
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true, change.player);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive)
            return true;
        ServerPlayer *source = invoke->invoker;
        ServerPlayer *current = change.player;

        QString prompt = "@qinlue-discard:" + current->objectName();
        const Card *card = room->askForCard(source, "Slash,Weapon", prompt, QVariant::fromValue(current), Card::MethodDiscard, current, false, "qinlue"); //"Slash,EquipCard"
        return card != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *source = invoke->invoker;
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            if (!invoke->invoker->faceUp())
                invoke->invoker->turnOver();
        } else {
            ServerPlayer *current = change.player;
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), current->objectName());

            QString prompt = "@qinlue-discard1:" + source->objectName();
            const Card *card = room->askForCard(current, "Jink,Armor", prompt, QVariant::fromValue(source), Card::MethodDiscard);
            if (!card) {
                room->setPlayerFlag(current, "Global_TurnTerminated");
                if (!room->getThread()->hasExtraTurn()) {
                    ExtraTurnStruct extra;
                    extra.player = invoke->invoker;
                    extra.set_phases << Player::RoundStart << Player::Play << Player::NotActive;
                    extra.reason = "qinlue";
                    extra.extraTarget = current;
                    source->tag["ExtraTurnInfo"] = QVariant::fromValue(extra);
                    invoke->invoker->gainAnExtraTurn();
                }
                return true;
            }
        }
        return false;
    }
};

class QinlueEffect : public TriggerSkill
{
public:
    QinlueEffect()
        : TriggerSkill("#qinlue_effect")
    {
        events << EventPhaseChanging << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = NULL;
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                player = change.player;
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current->getPhase() == Player::RoundStart)
                player = current;
        }
        if (player != NULL) {
            ExtraTurnStruct extra = player->tag["ExtraTurnInfo"].value<ExtraTurnStruct>();
            if (extra.reason == "qinlue" && extra.extraTarget != NULL) {
                ServerPlayer *target = extra.extraTarget;
                if (target->isAlive() && player->isAlive() && (!player->isKongcheng() || !target->isKongcheng()))
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true, target);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *player = invoke->invoker;
        ServerPlayer *target = player->tag["ExtraTurnInfo"].value<ExtraTurnStruct>().extraTarget;

        QList<CardsMoveStruct> exchangeMove;
        CardsMoveStruct move1(player->handCards(), target, Player::PlaceHand,
                              CardMoveReason(CardMoveReason::S_REASON_SWAP, player->objectName(), target->objectName(), "qinlue", QString()));
        CardsMoveStruct move2(target->handCards(), player, Player::PlaceHand,
                              CardMoveReason(CardMoveReason::S_REASON_SWAP, target->objectName(), player->objectName(), "qinlue", QString()));
        exchangeMove.push_back(move1);
        exchangeMove.push_back(move2);
        room->moveCardsAtomic(exchangeMove, false);

        return false;
    }
};

class Chaoren : public TriggerSkill
{
public:
    Chaoren()
        : TriggerSkill("chaoren")
    {
        events << NumOfEvents;
        show_type = "static";
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
                QList<SkillInvalidStruct> invalids = data.value<QList<SkillInvalidStruct> >();
                foreach (SkillInvalidStruct v, invalids) {
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

class ChaorenLog : public TriggerSkill
{
public:
    ChaorenLog()
        : TriggerSkill("#chaoren")
    {
        events << CardsMoveOneTime;
        global = true;
    }
    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_USE
                || (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE) {
                if (move.card_ids.length() == 1 && !move.from && move.from_places.contains(Player::DrawPile)) {
                    ServerPlayer *byakuren = room->findPlayerBySkillName("chaoren");
                    if (byakuren) {
                        room->notifySkillInvoked(byakuren, "chaoren");
                        LogMessage mes;
                        mes.type = "$chaoren";
                        mes.from = byakuren;
                        mes.arg = "chaoren";
                        mes.card_str = Sanguosha->getCard(move.card_ids.first())->toString();
                        room->sendLog(mes);
                    }
                }
            }
        }
    }
};

class Renge : public TriggerSkill
{
public:
    Renge()
        : TriggerSkill("renge")
    {
        events << GameStart << Revive << EventPhaseStart << Death << EventLoseSkill;
        frequency = Eternal;
    }

    // todo: kill Same General Mode
    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> ret;
        if (triggerEvent == GameStart) {
            if (!data.isNull()) {
                ServerPlayer *p = data.value<ServerPlayer *>();
                if (room->getTag("renge").isNull()) {
                    if (p->hasSkill(this) && p->isAlive())
                        ret << SkillInvokeDetail(this, p, p, NULL, true);
                } else {
                    ServerPlayer *koi = room->getTag("renge").value<ServerPlayer *>();
                    if (p == koi && (!p->hasSkill(this) || p->isDead()))
                        ret << SkillInvokeDetail(this, NULL, koi, NULL, true);
                }
            }
        } else if (triggerEvent == Revive) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (!room->getTag("renge").isNull()) {
                ServerPlayer *koi = room->getTag("renge").value<ServerPlayer *>();
                if (koi->hasSkill(this) && koi->isAlive())
                    ret << SkillInvokeDetail(this, koi, koi, p, true);
            } else if (p->hasSkill(this) && p->isAlive())
                ret << SkillInvokeDetail(this, p, p, NULL, true);
        } else if (triggerEvent == EventPhaseStart) {
            ServerPlayer *p = data.value<ServerPlayer *>();
            if (p->hasSkill(this) && p->isAlive() && p->getPhase() == Player::Start)
                ret << SkillInvokeDetail(this, p, p, NULL, true);
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (!room->getTag("renge").isNull()) {
                ServerPlayer *koi = room->getTag("renge").value<ServerPlayer *>();
                if (death.who == koi && (!death.who->hasSkill(this) || death.who->isDead()))
                    ret << SkillInvokeDetail(this, NULL, koi, NULL, true);
            }
        } else if (triggerEvent == EventLoseSkill) {
            if (!room->getTag("renge").isNull()) {
                SkillAcquireDetachStruct st = data.value<SkillAcquireDetachStruct>();
                ServerPlayer *koi = room->getTag("renge").value<ServerPlayer *>();
                if (koi->hasSkill(this) && koi->isAlive() && st.skill->objectName() != "renge") {
                    if (st.skill->objectName() == "benwo")
                        ret << SkillInvokeDetail(this, koi, koi, st.player, true);
                    else if (st.skill->objectName() == "ziwo" && st.player != koi)
                        ret << SkillInvokeDetail(this, koi, koi, st.player, true);
                } else if (koi == st.player && st.skill->objectName() == "renge") {
                    // assume that koi has lose this skill
                    ret << SkillInvokeDetail(this, NULL, koi, NULL, true);
                }
            }
        }

        return ret;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            QList<ServerPlayer *> ret;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (!p->hasSkill("chaowo", true))
                    ret << p;
            }
            if (!ret.isEmpty()) {
                ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, ret, "renge", "@renge", true, true);
                if (target != NULL)
                    invoke->targets << target;
            }
        }

        return TriggerSkill::cost(triggerEvent, room, invoke, data);
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (!invoke->targets.isEmpty()) {
                ServerPlayer *target = invoke->targets.first();
                room->acquireSkill(target, "chaowo");
            }
        } else if (triggerEvent == EventLoseSkill && data.value<SkillAcquireDetachStruct>().skill->objectName() != "renge") {
            SkillAcquireDetachStruct st = data.value<SkillAcquireDetachStruct>();
            LogMessage l;
            l.type = "#AcquireSkill";
            l.from = invoke->targets.first();
            l.arg = st.skill->objectName();
            room->sendLog(l);

            room->acquireSkill(invoke->targets.first(), st.skill);
        } else {
            if (invoke->owner == NULL) {
                room->removeTag("renge");
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    room->detachSkillFromPlayer(p, "benwo", false, true, false);
                    room->detachSkillFromPlayer(p, "ziwo", false, true, false);
                }
            } else if (invoke->targets.isEmpty()) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    room->acquireSkill(p, "benwo");
                    if (p != invoke->invoker)
                        room->acquireSkill(p, "ziwo");
                }
                room->setTag("renge", QVariant::fromValue(invoke->invoker));
            } else {
                room->acquireSkill(invoke->targets.first(), "benwo");
                if (invoke->targets.first() != invoke->owner)
                    room->acquireSkill(invoke->targets.first(), "ziwo");
            }
        }

        return false;
    }
};

class Benwo : public TriggerSkill
{
    static bool hasLimitedSkill(const Player *player)
    {
        foreach (const Skill *skill, player->getVisibleSkillList()) {
            if (skill != NULL && skill->getFrequency() == Limited && !skill->inherits("EquipSkill"))
                return true;
        }
        return false;
    }

public:
    Benwo()
        : TriggerSkill("benwo")
    {
        events << EventPhaseStart << CardUsed;
        frequency = Compulsory;
    }

    static QString usePattern;
    static QString usePatternWithoutSlash;

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill(this) && player->isAlive() && player->getPhase() == Player::Finish && !player->isKongcheng())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
        } else {
            CardUseStruct use = data.value<CardUseStruct>();

            if (use.from->getMark("benwouse") > 0 && !use.card->isKindOf("SkillCard"))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            QString pattern = usePatternWithoutSlash;
            foreach (const Skill *skill, player->getVisibleSkillList()) {
                if (skill != NULL && skill->getFrequency() == Skill::Eternal && !skill->inherits("EquipSkill")) {
                    pattern = usePattern;
                    break;
                }
            }
            bool canUse = false;
            foreach (int id, player->handCards()) {
                const Card *card = Sanguosha->getCard(id);
                // TODO: CHECK isAvailable FUNCTION OF ALL CARDS!!!!!!
                if (Sanguosha->matchExpPattern(pattern, player, card) && !player->isCardLimited(card, Card::MethodUse)) {
                    foreach (ServerPlayer *t, room->getAlivePlayers()) {
                        if (card->targetFilter(QList<const Player *>(), t, player) && !player->isProhibited(t, card)) {
                            canUse = true;
                            break;
                        }
                    }
                }
                if (canUse)
                    break;
            }

            if (!canUse)
                room->showAllCards(player);
            else {
                if (hasLimitedSkill(player))
                    player->setMark("benwouse", 1);

                if (!room->askForUseCard(player, pattern + "!", "benwo-use")) {
                    const Card *useCard = NULL;
                    ServerPlayer *target = NULL;
                    foreach (int id, player->handCards()) {
                        const Card *card = Sanguosha->getCard(id);
                        // TODO: CHECK isAvailable FUNCTION OF ALL CARDS!!!!!!
                        if (Sanguosha->matchExpPattern(pattern, player, card) && !player->isCardLimited(card, Card::MethodUse)) {
                            foreach (ServerPlayer *t, room->getAlivePlayers()) {
                                if (card->targetFilter(QList<const Player *>(), t, player) && !player->isProhibited(t, card)) {
                                    useCard = card;
                                    target = t;
                                    break;
                                }
                            }
                        }
                    }
                    if (useCard == NULL) { // IMPOSSIBLE!!!!
                        player->setMark("benwouse", 0);
                        room->showAllCards(player);
                    } else {
                        CardUseStruct use;
                        use.from = player;
                        use.card = useCard;
                        // TODO: fill use.to
                        if (!use.card->targetFixed())
                            use.to << target;
                        room->useCard(use);
                    }
                }
            }
        } else {
            CardUseStruct use = data.value<CardUseStruct>();
            use.from->setMark("benwouse", 0);
            if (hasLimitedSkill(use.from)) {
                use.from->drawCards(1, "benwo");
                if (use.card->isKindOf("Slash"))
                    room->recover(use.from, RecoverStruct());
            }
        }

        return false;
    }
};

QString Benwo::usePattern = "BasicCard+^Jink,TrickCard+^Nullification,EquipCard|.|.|hand,benwo";
QString Benwo::usePatternWithoutSlash = "BasicCard+^Jink+^Slash,TrickCard+^Nullification,EquipCard|.|.|hand,benwo";

class Ziwo : public TriggerSkill
{
public:
    Ziwo()
        : TriggerSkill("ziwo")
    {
        events << EventPhaseStart;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->hasSkill(this) && player->getPhase() == Player::RoundStart && player->isAlive())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        else if (player->getMark("ziwoInvalid") > 0 && player->getPhase() == Player::NotActive)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player->hasSkill(this) ? player : NULL, player, NULL, true);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (invoke->invoker->getPhase() == Player::RoundStart) {
            invoke->invoker->setMark("ziwoInvalid", 1);
            room->setPlayerSkillInvalidity(invoke->invoker, NULL, true);
        } else {
            invoke->invoker->setMark("ziwoInvalid", 0);
            room->setPlayerSkillInvalidity(invoke->invoker, NULL, false);
        }

        return false;
    }
};

class Chaowo : public ZeroCardViewAsSkill
{
public:
    static void changeHero(Room *room, ServerPlayer *source, const QString &to_general)
    {
        const General *fromGeneral = source->getGeneral();

        RoomThread *thread = room->getThread();
        JsonArray arg;
        arg << (int)QSanProtocol::S_GAME_EVENT_CHANGE_HERO;
        arg << source->objectName();
        arg << to_general;
        arg << false;
        arg << true;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

        room->changePlayerGeneral(source, to_general);

        if (fromGeneral != NULL) {
            foreach (const Skill *skill, fromGeneral->getSkillList()) {
                if (skill->isLordSkill() && !source->isLord())
                    continue;
                if (skill->getFrequency() == Skill::Limited && !skill->getLimitMark().isEmpty())
                    room->setPlayerMark(source, skill->getLimitMark(), 0);
                SkillAcquireDetachStruct s;
                s.isAcquire = true;
                s.player = source;
                s.skill = skill;
                QVariant _skillobjectName = QVariant::fromValue(s);
                thread->trigger(EventLoseSkill, room, _skillobjectName);
            }
        }

        bool game_start = false;
        const General *toGeneral = Sanguosha->getGeneral(to_general);
        if (toGeneral != NULL) {
            foreach (const Skill *skill, toGeneral->getSkillList()) {
                if (skill->isLordSkill() && !source->isLord())
                    continue;
                if (skill->inherits("TriggerSkill")) {
                    const TriggerSkill *trigger = qobject_cast<const TriggerSkill *>(skill);
                    thread->addTriggerSkill(trigger);
                    if (trigger->getTriggerEvents().contains(GameStart))
                        game_start = true;
                }
                if (skill->getFrequency() == Skill::Limited && !skill->getLimitMark().isEmpty())
                    room->setPlayerMark(source, skill->getLimitMark(), 1);
                SkillAcquireDetachStruct s;
                s.isAcquire = true;
                s.player = source;
                s.skill = skill;
                QVariant _skillobjectName = QVariant::fromValue(s);
                thread->trigger(EventAcquireSkill, room, _skillobjectName);
            }
        }

        room->resetAI(source);
        if (toGeneral != NULL) {
            if (!toGeneral->hasSkill("huanmeng")) {
                int hp = source->getHp();
                int record_hp = source->getHp();
                int maxHp = source->getMaxHp();
                if (fromGeneral->hasSkill("huanmeng")) {
                    maxHp = toGeneral->getMaxHp();
                    hp = toGeneral->getMaxHp();
                } else {
                    int maxHpMinus = toGeneral->getMaxHp() - fromGeneral->getMaxHp();
                    hp += maxHpMinus;
                    maxHp += maxHpMinus;
                }
                hp = qMin(hp, maxHp);

                source->setMaxHp(maxHp);
                room->broadcastProperty(source, "maxhp");

                if (toGeneral->hasSkill("banling")) {
                    source->setRenHp(hp);
                    source->setLingHp(hp);
                    room->broadcastProperty(source, "renhp");
                    room->broadcastProperty(source, "linghp");
                }

                source->setHp(hp);
                room->broadcastProperty(source, "hp");

                //while PostHpReduced, deal "kill player"  or "enterdying".
                if (record_hp > hp) {
                    HpLostStruct l;
                    l.player = source;
                    l.num = record_hp - hp;
                    QVariant lv = QVariant::fromValue(l);
                    room->getThread()->trigger(PostHpReduced, room, lv);
                    if (source->isDead())
                        return;
                }

            } else {
                source->setMaxHp(0);
                source->setHp(0);
                room->broadcastProperty(source, "maxhp");
                room->broadcastProperty(source, "hp");
            }

            QString kingdom = toGeneral->getKingdom();
            if (kingdom == "zhu" || kingdom == "touhougod") {
                kingdom = room->askForKingdom(source);

                LogMessage log;
                log.type = "#ChooseKingdom";
                log.from = source;
                log.arg = kingdom;
                room->sendLog(log);
            }
            room->setPlayerProperty(source, "kingdom", kingdom);
        }

        //remove piles and marks
        QList<int> idlist;
        if (fromGeneral != NULL) {
            foreach (const Skill *skill, fromGeneral->getSkillList()) {
                QString mark = skill->getRelatedMark();
                if (mark != NULL)
                    source->loseAllMarks(mark);

                QString pile = skill->getRelatedPileName();
                if (pile != NULL)
                    idlist.append(source->getPile(pile));
            }
        }
        if (!idlist.isEmpty()) {
            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, source->objectName(), NULL, "chaowo", QString());
            CardsMoveStruct move(idlist, source, Player::DiscardPile, reason);
            room->moveCardsAtomic(move, true);
        }

        if (game_start) {
            QVariant v = QVariant::fromValue(source);
            thread->trigger(GameStart, room, v);
        }
    }

    Chaowo()
        : ZeroCardViewAsSkill("chaowo")
    {
        frequency = Limited;
        limit_mark = "@chaowo";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasFlag("ChaowoNoGeneral"))
            return false;
        if (player->getMark("@chaowo") == 0)
            return false;

        QList<const General *> sameNameGenerals;

        const General *sourceGeneral = player->getGeneral();
        QString sourceGeneralName = sourceGeneral->objectName().split("_").first();
        foreach (const QString &name, Sanguosha->getLimitedGeneralNames()) {
            if (name.split("_").first() == sourceGeneralName)
                sameNameGenerals << Sanguosha->getGeneral(name);
        }

        // remove used general from list
        auto copy = sameNameGenerals;
        auto sib = player->getSiblings();
        sib << player;

        foreach (const General *general, copy) {
            foreach (const Player *p, sib) {
                if (p->getGeneral() == general)
                    sameNameGenerals.removeAll(general);
            }
        }

        return !sameNameGenerals.isEmpty();
    }

    const Card *viewAs() const
    {
        return new ChaowoCard;
    }
};

ChaowoCard::ChaowoCard()
{
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void ChaowoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QList<const General *> sameNameGenerals;

    const General *sourceGeneral = source->getGeneral();
    QString sourceGeneralName = sourceGeneral->objectName().split("_").first();
    foreach (const QString &name, Sanguosha->getLimitedGeneralNames()) {
        if (name.split("_").first() == sourceGeneralName)
            sameNameGenerals << Sanguosha->getGeneral(name);
    }

    // remove used general from list
    auto copy = sameNameGenerals;
    foreach (const General *general, copy) {
        foreach (ServerPlayer *p, room->getAllPlayers(true)) {
            if (p->getGeneral() == general)
                sameNameGenerals.removeAll(general);
        }
    }

    // remove generals in X from list
    copy = sameNameGenerals;
    foreach (ServerPlayer *p, room->getAllPlayers(true)) {
        if (p->hasSkill("anyun", true)) {
            QStringList X = p->getHiddenGenerals();
            foreach (const QString &g, X) {
                foreach (const General *general, copy) {
                    if (Sanguosha->getGeneral(g) == general)
                        sameNameGenerals.removeAll(general);
                }
            }
        }
    }

    // kill nullptr in General list
    sameNameGenerals.removeAll(NULL);

    if (sameNameGenerals.isEmpty()) {
        LogMessage log;
        log.type = "#ChaowoNoAvailableGeneral";
        log.from = source;
        room->doNotify(source, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());

        room->setPlayerFlag(source, "ChaowoNoGeneral");
        return;
    }

    room->setPlayerMark(source, "@chaowo", 0);
    room->doLightbox("$chaowoAnimate", 4000);

    QString toChange = sameNameGenerals.first()->objectName();
    QStringList sameNameGeneralNames;
    foreach (const General *general, sameNameGenerals)
        sameNameGeneralNames << general->objectName();

    if (sameNameGenerals.length() > 1) {
        if (source->isOnline())
            toChange = room->askForGeneral(source, sameNameGeneralNames, toChange);
        else
            toChange = room->askForChoice(source, "chaowo", sameNameGeneralNames.join("+"));
    }

    // Different from Room::changeHero, must be written seprately
    Chaowo::changeHero(room, source, toChange);
}

class Zuosui : public TriggerSkill
{
public:
    Zuosui()
        : TriggerSkill("zuosui")
    {
        events << DamageCaused;
        related_mark = "@xinyang";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.from || damage.from->isDead() || damage.from == damage.to || !damage.from->hasSkill(this))
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
            int discardNum = qMax(damage.to->getCards("hes").length() - x, 0);
            if (discardNum > 0)
                room->askForDiscard(damage.to, objectName(), discardNum, discardNum, false, true);
        }
        return true;
    }
};

class Worao : public TriggerSkill
{
public:
    Worao()
        : TriggerSkill("worao")
    {
        events << TargetConfirmed;
        related_mark = "@xinyang";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from && use.from->isAlive() && (use.card->isKindOf("Slash") || use.card->isNDTrick()) && !use.card->isVirtualCard()) {
            foreach (ServerPlayer *p, use.to) {
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
    Shenhua()
        : TriggerSkill("shenhua")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
        related_mark = "@xinyang";
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
    Hongfo()
        : TriggerSkill("hongfo")
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
        foreach (ServerPlayer *p, all)
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
    Junwei()
        : TriggerSkill("junwei")
    {
        frequency = Compulsory;
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.from->isAlive()) {
            foreach (ServerPlayer *p, use.to) {
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
    Gaizong()
        : TriggerSkill("gaizong")
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
        if (source->isDead())
            break;
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
    WendaoVS()
        : ZeroCardViewAsSkill("wendao")
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
    Wendao()
        : TriggerSkill("wendao")
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

const QStringList ShenbaoDialog::equipViewAsSkills {"Spear", "Pagoda", "JadeSeal"};

ShenbaoDialog *ShenbaoDialog::getInstance(const QString &object)
{
    static QPointer<ShenbaoDialog> instance;

    if (!instance.isNull() && instance->objectName() != object)
        delete instance;

    if (instance.isNull()) {
        instance = new ShenbaoDialog(object);
        connect(qApp, &QCoreApplication::aboutToQuit, instance.data(), &ShenbaoDialog::deleteLater);
    }

    return instance;
}

void ShenbaoDialog::popup()
{
    Self->tag.remove("shenbao_choice");

    QStringList choices = getAvailableChoices(Self, Sanguosha->getCurrentCardUseReason(), Sanguosha->getCurrentCardUsePattern());

    if (choices.isEmpty()) {
        emit onButtonClick();
    } /*else if (choices.length() == 1) {
        Self->tag["shenbao_choice"] = choices.first();
        emit onButtonClick();
    }*/
    else {
        QList<QAbstractButton *> btns = group->buttons();
        foreach (QAbstractButton *btn, btns)
            btn->setEnabled(choices.contains(btn->objectName()));

        exec();
    }
}

QStringList ShenbaoDialog::getAvailableChoices(const Player *player, CardUseStruct::CardUseReason cardUseReason, const QString &cardUsePattern)
{
    QStringList choices;

    foreach (const QString &skillName, equipViewAsSkills) {
        EquipCard *equipCard = qobject_cast<EquipCard *>(Sanguosha->cloneCard(skillName));
        bool available = false;
        if (equipCard != NULL) {
            EquipCard::Location location = equipCard->location();
            switch (location) {
            case EquipCard::WeaponLocation: {
                //if (!(player->hasWeapon(skillName) && !player->hasWeapon(skillName, true)))
                const ViewHasSkill *v = Sanguosha->ViewHas(player, skillName, "weapon", true);
                if (v && v->objectName().contains("shenbao"))
                    available = true;
                break;
            }
            case EquipCard::ArmorLocation: {
                //if (!(player->hasArmorEffect(skillName) && !player->hasArmorEffect(skillName, true)))
                const ViewHasSkill *v = Sanguosha->ViewHas(player, skillName, "armor", true);
                if (v && v->objectName().contains("shenbao"))
                    available = true;
                break;
            }

            case EquipCard::TreasureLocation: {
                //if (!(player->hasTreasure(skillName) && !player->hasTreasure(skillName, true)))
                const ViewHasSkill *v = Sanguosha->ViewHas(player, skillName, "treasure", true);
                if (v && v->objectName().contains("shenbao"))
                    available = true;
                break;
            }

            default:
                available = false;
                break;
            }
        } else
            available = false;

        delete equipCard;
        const ViewAsSkill *skill = Sanguosha->getViewAsSkill(skillName);

        if (available && skill != NULL) {
            if (cardUseReason == CardUseStruct::CARD_USE_REASON_PLAY) {
                if (skill->isEnabledAtPlay(player))
                    choices << skillName;
            } else {
                if (skill->isEnabledAtResponse(player, cardUsePattern))
                    choices << skillName;
            }
        }
    }
    if (isHegemonyGameMode(ServerInfo.GameMode) && cardUseReason == CardUseStruct::CARD_USE_REASON_PLAY && !player->hasShownSkill("shenbao")) {
        choices << "ShowShenbao";
    }

    return choices;
}

QStringList ShenbaoDialog::getAvailableNullificationChoices(const ServerPlayer *player)
{
    QStringList choices;
    //const Skill *shenbao = Sanguosha->getSkill("#shenbao_viewhas");
    foreach (const QString &skillName, equipViewAsSkills) {
        EquipCard *equipCard = qobject_cast<EquipCard *>(Sanguosha->cloneCard(skillName));
        bool available = false;
        if (equipCard != NULL) {
            EquipCard::Location location = equipCard->location();
            switch (location) {
            case EquipCard::WeaponLocation: {
                //if (!(player->hasWeapon(skillName) && !player->hasWeapon(skillName, true)))
                const ViewHasSkill *v = Sanguosha->ViewHas(player, skillName, "weapon", true);
                if (v && v->objectName().contains("shenbao"))
                    available = true;
                break;
            }
            case EquipCard::ArmorLocation: {
                //if (!(player->hasArmorEffect(skillName) && !player->hasArmorEffect(skillName, true)))
                const ViewHasSkill *v = Sanguosha->ViewHas(player, skillName, "armor", true);
                if (v && v->objectName().contains("shenbao"))
                    available = true;
                break;
            }

            case EquipCard::TreasureLocation: {
                //if (!(player->hasTreasure(skillName) && !player->hasTreasure(skillName, true)))
                const ViewHasSkill *v = Sanguosha->ViewHas(player, skillName, "treasure", true);
                if (v && v->objectName().contains("shenbao"))
                    available = true;
                break;
            }
            default:
                available = false;
                break;
            }
        } else
            available = false;

        delete equipCard;
        const ViewAsSkill *skill = Sanguosha->getViewAsSkill(skillName);

        if (available && skill != NULL) {
            if (skill->isEnabledAtNullification(player))
                choices << skillName;
        }
    }

    return choices;
}

void ShenbaoDialog::selectSkill(QAbstractButton *button)
{
    if (button != NULL)
        Self->tag["shenbao_choice"] = button->objectName();
    else
        Self->tag.remove("shenbao_choice");

    emit onButtonClick();
    accept();
}

ShenbaoDialog::ShenbaoDialog(const QString &object)
{
    setObjectName(object);
    group = new QButtonGroup;

    QVBoxLayout *layout = new QVBoxLayout;
    foreach (const QString &skillName, equipViewAsSkills) {
        QCommandLinkButton *btn = new QCommandLinkButton(Sanguosha->translate(skillName));
        btn->setObjectName(skillName);
        const Card *equip = Sanguosha->cloneCard(skillName);
        if (equip)
            btn->setToolTip(equip->getDescription());

        delete equip;

        group->addButton(btn);
        layout->addWidget(btn);
    }

    if (isHegemonyGameMode(ServerInfo.GameMode)) {
        QCommandLinkButton *show_btn = new QCommandLinkButton(Sanguosha->translate("ShowShenbao"));
        show_btn->setObjectName("ShowShenbao");
        group->addButton(show_btn);
        layout->addWidget(show_btn);
    }

    setLayout(layout);
    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectSkill(QAbstractButton *)));
    connect(this, &ShenbaoDialog::rejected, this, &ShenbaoDialog::onButtonClick);
}

class Shenbao : public AttackRangeSkill
{
public:
    Shenbao()
        : AttackRangeSkill("shenbao")
    {
    }

    bool canPreshow() const
    {
        return true;
    }

    virtual int getExtra(const Player *target, bool) const
    {
        if (target->hasSkill(objectName()) && target->hasShownSkill(this)) {
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
    ShenbaoDistance()
        : DistanceSkill("#shenbao_distance")
    {
    }

    virtual int getCorrect(const Player *from, const Player *to) const
    {
        int correct = 0;
        if (from->hasSkill("shenbao") && from->hasShownSkill("shenbao") && from->getMark("Equips_Nullified_to_Yourself") == 0) {
            foreach (const Player *p, from->getAliveSiblings()) {
                if (p->getOffensiveHorse()) {
                    correct = correct - 1;
                }
            }
        }

        if (to->hasSkill("shenbao") && to->hasShownSkill("shenbao") && to->getMark("Equips_Nullified_to_Yourself") == 0) {
            foreach (const Player *p, to->getAliveSiblings()) {
                if (p->getDefensiveHorse()) {
                    correct = correct + 1;
                }
            }
        }

        return correct;
    }
};

ShowShenbaoCard::ShowShenbaoCard()
    : SkillCard()
{
    mute = true;
    handling_method = Card::MethodNone;
}

bool ShowShenbaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return (to_select == Self && targets.isEmpty());
}

const Card *ShowShenbaoCard::validate(CardUseStruct &card_use) const
{
    bool head = card_use.from->inHeadSkills("shenbao");
    card_use.from->showGeneral(head);
    return NULL;
}

class ShenbaoAttach : public ViewAsSkill
{
public:
    ShenbaoAttach()
        : ViewAsSkill("shenbao_attach")
    {
        attached_lord_skill = true;
        response_or_use = true;
    }

    virtual bool shouldBeVisible(const Player *Self) const
    {
        return Self;
    }

    virtual QDialog *getDialog() const
    {
        return ShenbaoDialog::getInstance("shenbao_attach");
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !ShenbaoDialog::getAvailableChoices(player, CardUseStruct::CARD_USE_REASON_PLAY, Sanguosha->getCurrentCardUsePattern()).isEmpty();
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return !ShenbaoDialog::getAvailableChoices(player, Sanguosha->getCurrentCardUseReason(), pattern).isEmpty();
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return !ShenbaoDialog::getAvailableNullificationChoices(player).isEmpty();
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (!Self->tag.contains("shenbao_choice"))
            return false;

        QString name = Self->tag.value("shenbao_choice").toString();
        if (name == "ShowShenbao")
            return false;
        const ViewAsSkill *skill = Sanguosha->getViewAsSkill(name);
        if (skill == NULL)
            return false;

        if (ShenbaoDialog::getAvailableChoices(Self, Sanguosha->getCurrentCardUseReason(), Sanguosha->getCurrentCardUsePattern()).contains(name))
            return skill->viewFilter(selected, to_select);

        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (!Self->tag.contains("shenbao_choice"))
            return NULL;

        QString name = Self->tag.value("shenbao_choice").toString();
        if (name == "ShowShenbao") {
            return new ShowShenbaoCard();
        }
        //if (name == "cancel")
        //    return NULL;

        const ViewAsSkill *skill = Sanguosha->getViewAsSkill(name);
        if (skill == NULL)
            return NULL;

        if (ShenbaoDialog::getAvailableChoices(Self, Sanguosha->getCurrentCardUseReason(), Sanguosha->getCurrentCardUsePattern()).contains(name))
            return skill->viewAs(cards);

        return NULL;
    }
};

class ShenbaoHandler : public TriggerSkill
{
public:
    ShenbaoHandler()
        : TriggerSkill("#shenbao")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Death << Debut << EventSkillInvalidityChange;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == GameStart || triggerEvent == Debut || triggerEvent == EventAcquireSkill) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if ((p->hasSkill("shenbao", true) || p->ownSkill("shenbao")) && !p->hasSkill("shenbao_attach"))
                    room->attachSkillToPlayer(p, "shenbao_attach");
            }
        }
        if (triggerEvent == Death || triggerEvent == EventLoseSkill) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if ((!p->hasSkill("shenbao", true) && !p->ownSkill("shenbao")) && p->hasSkill("shenbao_attach"))
                    room->detachSkillFromPlayer(p, "shenbao_attach", true);
            }
        }
        //deal with broken wood_ox
        if (triggerEvent == EventLoseSkill || triggerEvent == EventAcquireSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            if (a.skill->objectName() == "shenbao") {
                QList<int> broken_equips = a.player->getBrokenEquips();
                if (!broken_equips.isEmpty()) {
                    JsonArray arg;
                    arg << a.player->objectName();
                    arg << JsonUtils::toJsonArray(broken_equips);
                    room->doNotify(a.player, QSanProtocol::S_COMMAND_SET_BROKEN_EQUIP, arg);
                }
            }
        }
        if (triggerEvent == EventSkillInvalidityChange) {
            QList<SkillInvalidStruct> invalids = data.value<QList<SkillInvalidStruct> >();
            foreach (SkillInvalidStruct v, invalids) {
                if (!v.skill || v.skill->objectName() == "shenbao") {
                    QList<int> broken_equips = v.player->getBrokenEquips();
                    if (!broken_equips.isEmpty()) {
                        JsonArray arg;
                        arg << v.player->objectName();
                        arg << JsonUtils::toJsonArray(broken_equips);

                        room->doNotify(v.player, QSanProtocol::S_COMMAND_SET_BROKEN_EQUIP, arg);
                    }
                }
            }
        }
    }
};

class ShenbaoViewHas : public ViewHasSkill

{
public:
    ShenbaoViewHas()
        : ViewHasSkill("#shenbao_viewhas")

    {
    }

    virtual bool ViewHas(const Player *player, const QString &skill_name, const QString &flag, bool ignore_preshow) const
    {
        if (player->isDead()) //do not consider nue??
            return false;

        if (!ignore_preshow && !player->hasSkill("shenbao"))
            return false;
        if (ignore_preshow) {
            if (!player->ownSkill("shenbao"))
                return false;
            if (!player->hasShownSkill("shenbao")) {
                QString flag = (player->inHeadSkills("shenbao")) ? "h" : "d";
                if (!player->canShowGeneral(flag))
                    return false;
            }
        }

        //only consider hegemony mode??  need check items?? no need
        //if (skill_name == "shenbao")
        //    return true;
        if (flag == "weapon") {
            if ((skill_name == "Crossbow" || skill_name == "SixSwords") && !player->hasShownSkill("shenbao"))
                return false;
            //return true;
            QString weapon_name = skill_name;
            foreach (const Player *p, player->getAliveSiblings()) {
                if (p->getWeapon()) {
                    if (weapon_name == "shenbao")
                        return true;
                    WrappedCard *wp = p->getWeapon();
                    if (wp->objectName() == weapon_name || wp->isKindOf(weapon_name.toStdString().c_str()))
                        return true;
                    const Card *real_related_weapon = Sanguosha->getEngineCard(wp->getEffectiveId());
                    if (real_related_weapon->objectName() == weapon_name || real_related_weapon->isKindOf(weapon_name.toStdString().c_str()))
                        return true;
                }
            }
            //self broken equip
            WrappedCard *wp = player->getWeapon();
            if (wp && player->isBrokenEquip(wp->getEffectiveId())) {
                if (wp->objectName() == weapon_name || wp->isKindOf(weapon_name.toStdString().c_str()))
                    return true;
                const Card *real_related_weapon = Sanguosha->getEngineCard(wp->getEffectiveId());
                if (real_related_weapon->objectName() == weapon_name || real_related_weapon->isKindOf(weapon_name.toStdString().c_str()))
                    return true;
            }
            //    return (weapon == NULL || weapon->objectName() != skill_name || player->isBrokenEquip(weapon->getEffectiveId()));
        } else if (flag == "armor") {
            QString armor_name = skill_name;
            foreach (const Player *p, player->getAliveSiblings()) {
                if (p->getArmor()) {
                    if (armor_name == "shenbao")
                        return true;
                    WrappedCard *ar = p->getArmor();
                    if (ar->objectName() == armor_name || ar->isKindOf(armor_name.toStdString().c_str()))
                        return true;
                    const Card *real_related_armor = Sanguosha->getEngineCard(ar->getEffectiveId());
                    if (real_related_armor->objectName() == armor_name || real_related_armor->isKindOf(armor_name.toStdString().c_str()))
                        return true;
                }
            }
            //self broken equip
            WrappedCard *ar = player->getArmor();
            if (ar && player->isBrokenEquip(ar->getEffectiveId())) {
                if (ar->objectName() == armor_name || ar->isKindOf(armor_name.toStdString().c_str()))
                    return true;
                const Card *real_related_armor = Sanguosha->getEngineCard(ar->getEffectiveId());
                if (real_related_armor->objectName() == armor_name || real_related_armor->isKindOf(armor_name.toStdString().c_str()))
                    return true;
            }
            //return (armor == NULL || armor->objectName() != skill_name || player->isBrokenEquip(armor->getEffectiveId()));
        } else if (flag == "treasure") {
            QString treasure_name = skill_name;
            if (skill_name != "wooden_ox") {
                foreach (const Player *p, player->getAliveSiblings()) {
                    if (p->getTreasure()) {
                        if (treasure_name == "shenbao")
                            return true;
                        WrappedCard *ar = p->getTreasure();
                        if (ar->objectName() == treasure_name || ar->isKindOf(treasure_name.toStdString().c_str()))
                            return true;
                        const Card *real_related_treasure = Sanguosha->getEngineCard(ar->getEffectiveId());
                        if (real_related_treasure->objectName() == treasure_name || real_related_treasure->isKindOf(treasure_name.toStdString().c_str()))
                            return true;
                    }
                }
            }

            //self broken equip
            WrappedCard *ar = player->getTreasure();
            if (ar && player->isBrokenEquip(ar->getEffectiveId())) {
                if (ar->objectName() == treasure_name || ar->isKindOf(treasure_name.toStdString().c_str()))
                    return true;
                const Card *real_related_treasure = Sanguosha->getEngineCard(ar->getEffectiveId());
                if (real_related_treasure->objectName() == treasure_name || real_related_treasure->isKindOf(treasure_name.toStdString().c_str()))
                    return true;
            }

            //    return (treasure && treasure->objectName() == skill_name && player->isBrokenEquip(treasure->getEffectiveId()));
            //    return (treasure == NULL || treasure->objectName() != skill_name || player->isBrokenEquip(treasure->getEffectiveId()));
        }

        return false;
    }
};

class Yindu : public TriggerSkill
{
public:
    Yindu()
        : TriggerSkill("yindu")
    {
        events << Death;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
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
    Huanming()
        : TriggerSkill("huanming")
    {
        events << DamageCaused;
        frequency = Limited;
        limit_mark = "@huanming";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.from || damage.from == damage.to || damage.from->getMark("@huanming") == 0 || damage.to->getHp() < damage.from->dyingThreshold()
            || damage.from->getHp() < damage.to->dyingThreshold() || !damage.from->hasSkill(this))
            return QList<SkillInvokeDetail>();
        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from);
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
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
            QVariant lv = QVariant::fromValue(l);
            room->getThread()->trigger(PostHpReduced, room, lv);
        } else if (victim_Hp > victim_newHp) {
            HpLostStruct l;
            l.player = damage.to;
            l.num = victim_Hp - victim_newHp;
            QVariant lv = QVariant::fromValue(l);
            room->getThread()->trigger(PostHpReduced, room, lv);
        }

        return true;
    }
};

//the real distance effect is in  Player::distanceTo()
//this triggerskill is for skilleffect
class Chuanwu : public TriggerSkill
{
public:
    Chuanwu()
        : TriggerSkill("chuanwu")
    {
        events << HpChanged;
        frequency = Compulsory;
        show_type = "static";
    }

    void record(TriggerEvent, Room *room, QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->hasSkill(this) && p->hasShownSkill(this)) {
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

class Fanhun : public TriggerSkill
{
public:
    Fanhun()
        : TriggerSkill("fanhun")
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
            if (who->hasSkill(this) && who->isAlive() && who->getHp() < who->dyingThreshold())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, who, who, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
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
    Yousi()
        : TriggerSkill("yousi")
    {
        events << PostHpReduced;
        frequency = Compulsory;
        show_type = "static";
    }

    void record(TriggerEvent, Room *room, QVariant &data) const
    {
        ServerPlayer *victim = NULL;
        if (data.canConvert<DamageStruct>())
            victim = data.value<DamageStruct>().to;
        else
            victim = data.value<HpLostStruct>().player;

        ServerPlayer *uuz = room->getCurrent();
        if (uuz && uuz->isAlive() && uuz != victim && uuz->hasSkill(this) && uuz->hasShownSkill(this) && victim->getHp() < victim->dyingThreshold()) {
            room->touhouLogmessage("#TriggerSkill", uuz, objectName());
            room->notifySkillInvoked(uuz, objectName());
        }
    }
};

class YousiUI : public TriggerSkill
{
public:
    YousiUI()
        : TriggerSkill("#yousi")
    {
        events << EventAcquireSkill << EventLoseSkill << EventSkillInvalidityChange << EventPhaseStart << Death << HpChanged;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        ServerPlayer *yuyuko = NULL;
        //bool notify = false;
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->hasSkill("yousi")) {
                if (player->getPhase() == Player::RoundStart || player->getPhase() == Player::NotActive)
                    yuyuko = player;
            }
        } else if (triggerEvent == EventAcquireSkill || triggerEvent == EventLoseSkill) {
            SkillAcquireDetachStruct a = data.value<SkillAcquireDetachStruct>();
            if (a.skill->objectName() == "yousi" && a.player->isCurrent())
                yuyuko = a.player;
        } else if (triggerEvent == EventSkillInvalidityChange) {
            QList<SkillInvalidStruct> invalids = data.value<QList<SkillInvalidStruct> >();
            foreach (SkillInvalidStruct v, invalids) {
                if (v.player->isCurrent() && (!v.skill || v.skill->objectName() == "yousi")) {
                    yuyuko = v.player;
                    break;
                }
            }
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who->hasSkill("yousi") && death.who->isCurrent())
                yuyuko = death.who;
        } else if (triggerEvent == HpChanged) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->isCurrent() && player->hasSkill("yousi"))
                yuyuko = player;
        }

        if (yuyuko) {
            foreach (ServerPlayer *p, room->getOtherPlayers(yuyuko)) {
                room->setPlayerProperty(p, "dyingFactor", p->getDyingFactor());
            }
        }
    }
};

class Kuixin : public TriggerSkill
{
public:
    Kuixin()
        : TriggerSkill("kuixin")
    {
        events << CardFinished; // << TargetConfirmed << TargetSpecified;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("SkillCard") || use.card->isKindOf("Jink") || use.from == NULL || use.to.length() != 1 || use.from == use.to.first()
            || use.from->hasFlag("Global_ProcessBroken"))
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        QList<ServerPlayer *> satoris;
        int fromHP = use.from->getLostHp() + 1;
        int toHP = use.to.first()->getLostHp() + 1;
        if (use.from->isAlive() && use.from->hasSkill(this) && use.to.first()->getCards("h").length() > 0 && (use.to.first()->getShownHandcards().length() < fromHP))
            satoris << use.from;
        else if (use.to.first()->isAlive() && use.to.first()->hasSkill(this) && use.from->getCards("h").length() > 0 && use.from->getShownHandcards().length() < toHP)
            satoris << use.to.first();

        use.from->getRoom()->sortByActionOrder(satoris);
        foreach (ServerPlayer *p, satoris)
            if (p == use.from)
                d << SkillInvokeDetail(this, use.from, use.from, NULL, true, use.to.first());
            else
                d << SkillInvokeDetail(this, use.to.first(), use.to.first(), NULL, true, use.from);

        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (invoke->targets.first()->getCards("h").isEmpty())
            return false;

        LogMessage log;
        log.type = "#ChoosePlayerWithSkill";
        log.from = invoke->invoker;
        log.to << invoke->targets.first();
        log.arg = objectName();
        room->sendLog(log);
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());

        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "h", objectName());
        invoke->targets.first()->addToShownHandCards(QList<int>() << id);
        return false;
    }
};

XinhuaCard::XinhuaCard()
{
    will_throw = false;
}

bool XinhuaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *oc = Sanguosha->getCard(subcards.first());
    return oc->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, oc, targets);
    ;
}

bool XinhuaCard::targetFixed() const
{
    const Card *oc = Sanguosha->getCard(subcards.first());
    return oc->targetFixed();
}

bool XinhuaCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Card *oc = Sanguosha->getCard(subcards.first());
    if (oc->canRecast() && targets.length() == 0)
        return false;
    return oc->targetsFeasible(targets, Self);
}

const Card *XinhuaCard::validate(CardUseStruct &use) const
{
    Room *room = use.from->getRoom();
    const Card *card = Sanguosha->getCard(subcards.first());
    use.from->showHiddenSkill("xinhua");

    room->notifySkillInvoked(use.from, "xinhua");
    LogMessage mes;
    mes.type = "$xinhua";
    mes.from = use.from;
    mes.to << room->getCardOwner(subcards.first());
    mes.arg = "xinhua";
    mes.card_str = card->toString();
    room->sendLog(mes);

    return card;
}

const Card *XinhuaCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();
    const Card *card = Sanguosha->getCard(subcards.first());
    user->showHiddenSkill("xinhua");

    room->notifySkillInvoked(user, "xinhua");
    LogMessage mes;
    mes.type = "$xinhua";
    mes.from = user;
    mes.to << room->getCardOwner(subcards.first());
    mes.arg = "xinhua";
    mes.card_str = card->toString();
    room->sendLog(mes);

    return card;
}

class XinhuaVS : public OneCardViewAsSkill
{
public:
    XinhuaVS()
        : OneCardViewAsSkill("xinhua")
    {
        expand_pile = "%shown_card";
    }

    static QStringList responsePatterns()
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();

        QStringList validPatterns;
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        QStringList ban_list = Sanguosha->getBanPackages();
        foreach (const Card *card, cards) {
            if ((card->isNDTrick() || card->isKindOf("BasicCard")) && !ban_list.contains(card->getPackage())) { //&& !ServerInfo.Extensions.contains("!" + card->getPackage())
                QString p = card->objectName();
                if (card->isKindOf("Slash"))
                    p = "slash";
                if (!validPatterns.contains(p))
                    validPatterns << card->objectName();
            }
        }

        QStringList checkedPatterns;
        foreach (QString str, validPatterns) {
            const Skill *skill = Sanguosha->getSkill("xinhua");
            if (skill->matchAvaliablePattern(str, pattern))
                checkedPatterns << str;
        }
        return checkedPatterns;
    }

    static bool hasShown(const Player *player)
    {
        bool hasShown = false;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (!p->getShownHandcards().isEmpty()) {
                hasShown = true;
                break;
            }
        }
        return hasShown;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return hasShown(player);
    }

    bool isEnabledAtResponse(const Player *player, const QString &) const
    {
        if (!hasShown(player))
            return false;
        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.contains("peach") && checkedPatterns.length() == 1 && player->getMark("Global_PreventPeach") > 0)
            return false;
        return !checkedPatterns.isEmpty();
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() >= 1)
            return false;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
            || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
            QStringList patterns = responsePatterns();
            foreach (const Player *p, Self->getAliveSiblings()) {
                if (p->getShownHandcards().contains(to_select->getId())) {
                    foreach (QString pattern, patterns) {
                        ExpPattern exp(pattern);
                        if (exp.match(Self, to_select))
                            return true;
                    }
                }
            }
        } else if ((Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY)) {
            foreach (const Player *p, Self->getAliveSiblings()) {
                if (p->getShownHandcards().contains(to_select->getId()) && to_select->isAvailable(Self))
                    return true;
            }
        }
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        XinhuaCard *card = new XinhuaCard;
        card->addSubcard(originalCard);
        return card;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        if (player->hasFlag("xinhua_used"))
            return false;
        foreach (const Player *p, player->getAliveSiblings()) {
            foreach (int id, p->getShownHandcards()) {
                if (Sanguosha->getCard(id)->isKindOf("Nullification") && !player->isCardLimited(Sanguosha->getCard(id), Card::MethodUse, true))
                    return true;
            }
        }
        return false;
    }
};

class Xinhua : public TriggerSkill
{
public:
    Xinhua()
        : TriggerSkill("xinhua")
    {
        events << EventPhaseChanging;
        view_as_skill = new XinhuaVS;
    }

    void record(TriggerEvent e, Room *room, QVariant &) const
    {
        if (e == EventPhaseChanging) {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                room->setPlayerFlag(p, "-xinhua_used");
        }
    }
};

class Dongcha : public TriggerSkill
{
public:
    Dongcha()
        : TriggerSkill("dongcha")
    {
        events << CardUsed << CardResponded;
    }

    static bool dongcha(ServerPlayer *satori, const Card *card)
    {
        foreach (const Card *c, satori->getCards("s")) {
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
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (use.from != p && dongcha(p, use.card))
                    d << SkillInvokeDetail(this, p, p, NULL, false, use.from);
            }
        } else {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_isUse) {
                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
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
        prompt_list << "@dongcha" << card->objectName() << invoke->preferredTarget->objectName() << card->getType();
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
    Zhuiyi()
        : TriggerSkill("zhuiyi")
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
                foreach (int id, move.card_ids) {
                    if (move.from->isShownHandcard(id) && !record_ids.contains(id))
                        record_ids << id;
                }
                room->setTag("zhuiyi", record_ids);
            }
            return;
        } else {
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_DISCARD || !move.from_places.contains(Player::PlaceHand)
                || move.to_place != Player::DiscardPile) {
                QVariantList record_ids = room->getTag("zhuiyi").toList();
                foreach (int id, move.card_ids)
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
            if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD && move.from_places.contains(Player::PlaceHand)
                && move.to_place == Player::DiscardPile && from != NULL) {
                foreach (int id, move.card_ids) {
                    if (record_ids.contains(id)) {
                        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
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

        foreach (int id, move.card_ids) {
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
        foreach (int id, mo.card_ids) {
            if (room->getCardOwner(id) == invoke->invoker && room->getCardPlace(id) == Player::PlaceHand)
                show_ids << id;
        }

        if (!show_ids.isEmpty())
            invoke->invoker->addToShownHandCards(show_ids);
        return false;
    }
};

class Tianqu : public TargetModSkill
{
public:
    Tianqu()
        : TargetModSkill("tianqu")
    {
        pattern = ".";
    }

    virtual int getResidueNum(const Player *from, const Card *) const
    {
        if (from->hasSkill("tianqu"))
            return 1000;
        return 0;
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const
    {
        if (from->hasSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !card->hasFlag("IgnoreFailed"))
            return 1000;
        else
            return 0;
    }
};

class Fengmi : public TriggerSkill
{
public:
    Fengmi()
        : TriggerSkill("fengmi")
    {
        events << TargetSpecified;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.from && use.from->isAlive() && use.from->hasSkill(this) && use.card->isKindOf("Slash")) {
            foreach (ServerPlayer *p, use.to) {
                if (p != use.from && p->canDiscard(p, "hejs"))
                    d << SkillInvokeDetail(this, use.from, use.from, NULL, false, p);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        DummyCard dummy;
        ServerPlayer *target = invoke->targets.first();
        foreach (const Card *c, target->getCards("hesj")) {
            if (target->canDiscard(target, c->getEffectiveId()))
                dummy.addSubcard(c);
        }
        int count = dummy.getSubcards().length();
        if (count > 0) {
            room->throwCard(&dummy, target, target);
            target->drawCards(count);
        }
        return false;
    }
};

class Huanhun : public TriggerSkill
{
public:
    Huanhun()
        : TriggerSkill("huanhun")
    {
        events << BuryVictim;
        frequency = Eternal;
    }

    virtual int getPriority() const
    {
        return -1;
    }

    static bool canHuanhun(ServerPlayer *player)
    {
        QString role = player->getRole();
        if (role == "renegade")
            return true;
        int loyalist = 0;
        int rebel = 0;
        int renegade = 0;
        foreach (ServerPlayer *p, player->getRoom()->getAllPlayers()) {
            if (p->getRole() == "rebel")
                rebel++;
            else if (p->getRole() == "renegde")
                renegade++;
            else if (p->getRole() == "loyalist" || p->getRole() == "lord")
                loyalist++;
        }
        renegade = qMin(1, renegade);
        if (role == "rebel")
            return rebel < loyalist || rebel < renegade;
        else
            return loyalist < rebel || loyalist < renegade;
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        QList<SkillInvokeDetail> d;
        foreach (ServerPlayer *p, room->getOtherPlayers(death.who, true)) {
            if (p->isDead() && p->hasSkill(this) && canHuanhun(p))
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
    Tongling()
        : TriggerSkill("tongling")
    {
        events << EventPhaseStart;
        frequency = Limited;
        limit_mark = "@tongling";
    }

    static bool findSameSkill(const Skill *source_skill, ServerPlayer *player)
    {
        foreach (ServerPlayer *p, player->getRoom()->getAlivePlayers()) {
            foreach (const Skill *skill, p->getVisibleSkillList()) {
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
            foreach (ServerPlayer *p, room->getAllPlayers(true)) {
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

            foreach (ServerPlayer *p, room->getAllPlayers(true)) {
                if (p->isDead() && p->getGeneral() != NULL && p->getGeneralName() != "sujiang" && p->getGeneralName() != "sujiangf") {
                    foreach (const Skill *skill, p->getGeneral()->getVisibleSkillList()) {
                        if (skill->isLordSkill() || skill->getFrequency() == Skill::Eternal || skill->getFrequency() == Skill::Wake || findSameSkill(skill, invoke->invoker))
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
                foreach (ServerPlayer *p, room->getAllPlayers(true)) {
                    if (p->isDead() && p->getGeneral() != NULL && p->getGeneralName() != "sujiang" && p->getGeneralName() != "sujiangf")
                        general_list << p->getGeneralName();
                }

                QString general = room->askForGeneral(invoke->invoker, general_list);
                //forbid cheat
                if (!general_list.contains(general))
                    general = general_list.at(qrand() % general_list.length());
                foreach (ServerPlayer *p, room->getAllPlayers(true)) {
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
        room->doLightbox("$tonglingAnimate", 4000);
        room->removePlayerMark(invoke->invoker, "@tongling");
        const General *general = invoke->targets.first()->getGeneral();
        QString new_general = invoke->targets.first()->isMale() ? "sujiang" : "sujiangf";
        room->changeHero(invoke->targets.first(), new_general, false, false, false, false);

        QStringList skill_names;

        foreach (const Skill *skill, general->getVisibleSkillList()) {
            if (skill->isLordSkill() || skill->getFrequency() == Skill::Eternal || skill->getFrequency() == Skill::Wake || findSameSkill(skill, invoke->invoker))
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
            if (invoke->invoker->inHeadSkills(objectName())) {
                arg << general->objectName();
                arg << skill_name;
            }
            arg << QString() << QString();
            if (!invoke->invoker->inHeadSkills(objectName())) {
                arg << general->objectName();
                arg << skill_name;
            }
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
            room->handleAcquireDetachSkills(invoke->invoker, skill_name, true);
        }
        return false;
    }
};

RumoCard::RumoCard()
{
}

static int rumoNum(const Player *Self)
{
    QString roles = Sanguosha->getRoles(ServerInfo.GameMode);
    QMap<QString, int> roles_map;
    roles_map.insert("loyal", roles.count("Z") + roles.count("C"));
    roles_map.insert("rebel", roles.count("F"));
    foreach (const Player *p, Self->getSiblings()) {
        if (p->isDead()) {
            if (p->getRole() == "rebel")
                --roles_map["rebel"];
            else if (p->getRole() != "renegade")
                --roles_map["loyal"];
        }
    }

    return qMax(roles_map["loyal"], roles_map["rebel"]);
}

bool RumoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    int num = rumoNum(Self);
    if (!targets.contains(Self))
        return to_select == Self && targets.length() < num;
    return !targets.contains(to_select) && targets.length() < num;
}

bool RumoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    int num = rumoNum(Self);
    return targets.length() > 0 && targets.length() <= num && targets.contains(Self);
}
void RumoCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->doLightbox("$rumoAnimate", 4000);
    SkillCard::onUse(room, card_use);
}

void RumoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->removePlayerMark(source, "@rumo");
    QStringList roles = room->aliveRoles();
    int loyalist = 0;
    int rebel = 0;
    int renegade = 0;
    foreach (QString role, roles) {
        if (role == "rebel")
            rebel++;
        else if (role == "renegade")
            renegade++;
        else if (role == "loyalist" || role == "lord")
            loyalist++;
    }
    renegade = qMin(renegade, 1);
    int num = qMax(renegade, loyalist);
    num = qMax(num, rebel);

    room->sortByActionOrder(targets);
    foreach (ServerPlayer *target, targets) {
        if (!target->isChained())
            room->setPlayerProperty(target, "chained", true);
    }
    source->drawCards(num);
}

class Rumo : public ZeroCardViewAsSkill
{
public:
    Rumo()
        : ZeroCardViewAsSkill("rumo")
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

AnyunDialog *AnyunDialog::getInstance(const QString &object)
{
    static QPointer<AnyunDialog> instance;

    if (!instance.isNull() && instance->objectName() != object)
        delete instance;

    if (instance.isNull()) {
        instance = new AnyunDialog(object);
        connect(qApp, &QCoreApplication::aboutToQuit, instance.data(), &AnyunDialog::deleteLater);
    }

    return instance;
}

AnyunDialog::AnyunDialog(const QString &object)
    : object_name(object)
{
    setObjectName(object);
    setWindowTitle(Sanguosha->translate(object));
    group = new QButtonGroup(this);

    layout = new QVBoxLayout;
    setLayout(layout);

    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectSkill(QAbstractButton *)));
}

void AnyunDialog::popup()
{
    bool play = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY);

    foreach (QAbstractButton *button, group->buttons()) {
        layout->removeWidget(button);
        group->removeButton(button);
        delete button;
    }

    foreach (QString hidden, Self->getHiddenGenerals()) {
        const General *g = Sanguosha->getGeneral(hidden);
        foreach (const Skill *skill, g->getSkillList()) {
            const ViewAsSkill *vs = Sanguosha->getViewAsSkill(skill->objectName());
            if (vs && !vs->inherits("FilterSkill")) {
                bool add = false;
                if (play && vs->isEnabledAtPlay(Self))
                    add = true;
                if (!play) {
                    QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
                    if (vs->isEnabledAtResponse(Self, pattern))
                        add = true;
                }
                if (add) {
                    QCommandLinkButton *button = new QCommandLinkButton;
                    button->setText(Sanguosha->translate(vs->objectName()));
                    button->setObjectName(vs->objectName());
                    group->addButton(button);

                    button->setEnabled(true);
                    button->setToolTip(Sanguosha->getSkill(vs->objectName())->getDescription());
                    layout->addWidget(button);
                }
            }
        }
    }

    Self->tag.remove(object_name);
    exec();
}

void AnyunDialog::selectSkill(QAbstractButton *button)
{
    const QString skillName = button->objectName();
    const Skill *skill = Sanguosha->getSkill(skillName);
    if (skill != NULL) {
        QDialog *dialog = skill->getDialog();
        if (dialog != NULL)
            QMetaObject::invokeMethod(dialog, "popup", Qt::DirectConnection);
    }

    Self->tag[object_name] = QVariant::fromValue(skillName);
    emit onButtonClick();
    accept();
}

class AnyunVS : public ViewAsSkill
{
public:
    AnyunVS()
        : ViewAsSkill("anyun")
    {
        response_or_use = true;
    }

    static bool hasHiddenViewas(const Player *player)
    {
        bool play = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY);
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        foreach (QString hidden, Self->getHiddenGenerals()) {
            const General *g = Sanguosha->getGeneral(hidden);
            foreach (const Skill *skill, g->getSkillList()) {
                const ViewAsSkill *vs = Sanguosha->getViewAsSkill(skill->objectName());
                if (vs) {
                    if (play && vs->isEnabledAtPlay(player))
                        return true;
                    if (!play) {
                        if (vs->isEnabledAtResponse(player, pattern))
                            return true;
                    }
                }
            }
        }
        return false;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (!player->canShowHiddenSkill())
            return false;
        return hasHiddenViewas(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &) const
    {
        if (!player->canShowHiddenSkill())
            return false;
        if (player->hasFlag("Global_viewasHidden_Failed"))
            return false;
        return hasHiddenViewas(player);
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        QString name = Self->tag.value("anyun", QString()).toString();
        if (name != NULL) {
            const ViewAsSkill *s = Sanguosha->getViewAsSkill(name);
            return s->viewFilter(selected, to_select);
        }
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        QString name = Self->tag.value("anyun", QString()).toString();
        if (name != NULL) {
            const ViewAsSkill *s = Sanguosha->getViewAsSkill(name);
            return s->viewAs(cards);
        }
        return NULL;
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        if (!player->canShowHiddenSkill())
            return false;
        foreach (QString hidden, player->getHiddenGenerals()) {
            const General *g = Sanguosha->getGeneral(hidden);
            foreach (const Skill *skill, g->getSkillList()) {
                const ViewAsSkill *vs = Sanguosha->getViewAsSkill(skill->objectName());
                if (vs) {
                    if (vs->isEnabledAtNullification(player))
                        return true;
                }
            }
        }
        return false;
    }
};

class Anyun : public TriggerSkill
{
public:
    Anyun()
        : TriggerSkill("anyun")
    {
        events << GameStart;
        view_as_skill = new AnyunVS;
    }

    virtual QDialog *getDialog() const
    {
        return AnyunDialog::getInstance("anyun");
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == GameStart) {
            ServerPlayer *nue = data.value<ServerPlayer *>();
            if (nue && nue->hasSkill(this))
                d << SkillInvokeDetail(this, nue, nue, NULL, true);
        }
        return d;
    }

    static void AcquireGenerals(ServerPlayer *zuoci, int n, bool init, QStringList deleteName)
    {
        Room *room = zuoci->getRoom();
        QStringList huashens = zuoci->getHiddenGenerals();
        QStringList list = GetAvailableGenerals(zuoci, init); //do not remove parameter "init", which will be used for testing.
        qShuffle(list);
        if (list.isEmpty())
            return;
        n = qMin(n, list.length());

        QStringList acquired = list.mid(0, n);
        if (n > 1 && !init) {
            QString limit_general = room->askForGeneral(zuoci, acquired);
            acquired.clear();
            acquired << limit_general;
        }
        //add triggerSkill
        foreach (QString name, acquired) {
            huashens << name;
            const General *general = Sanguosha->getGeneral(name);
            if (general) {
                foreach (const TriggerSkill *skill, general->getTriggerSkills()) {
                    room->getThread()->addTriggerSkill(skill);
                }
            }
        }

        LogMessage log;
        log.type = "#GetHuashen";
        log.from = zuoci;
        log.arg = QString::number(n);
        log.arg2 = QString::number(huashens.length());
        room->sendLog(log);

        LogMessage l;
        l.type = "#GetHuashenDetail";
        l.from = zuoci;
        l.arg = acquired.join("\\, \\");

        room->doNotify(zuoci, QSanProtocol::S_COMMAND_LOG_SKILL, l.toJsonValue());

        zuoci->addHiddenGenerals(acquired);
        if (!deleteName.isEmpty())
            zuoci->removeHiddenGenerals(deleteName);
    }

    static QStringList GetAvailableGenerals(ServerPlayer *zuoci, bool init)
    {
        QSet<QString> all = Sanguosha->getLimitedGeneralNames().toSet();
        Room *room = zuoci->getRoom();
        if (isNormalGameMode(room->getMode()) || room->getMode().contains("_mini_") || room->getMode() == "custom_scenario")
            all.subtract(Config.value("Banlist/Roles", "").toStringList().toSet());
        else if (room->getMode() == "06_XMode") {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["XModeBackup"].toStringList().toSet());
        } else if (room->getMode() == "02_1v1") {
            all.subtract(Config.value("Banlist/1v1", "").toStringList().toSet());
            foreach (ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["1v1Arrange"].toStringList().toSet());
        }
        QSet<QString> huashen_set, room_set;
        huashen_set = zuoci->getHiddenGenerals().toSet();

        foreach (ServerPlayer *player, room->getAllPlayers(true)) {
            QString name = player->getGeneralName();
            //deal conflict with seiga_god
            if (name == "sujiang" || name == "sujiangf")
                name = player->tag.value("init_general", QString()).toString();
            room_set << name;
            //do not consider general2
        }

        static QSet<QString> banned;
        // ban nue and zun
        if (banned.isEmpty())
            banned << "nue_god"
                   << "zun"
                   << "koishi_god"
                   << "seiga_god"
                   << "youmu_god";
        return (all - banned - huashen_set - room_set).toList();

        Q_UNUSED(init);
        //for test!!! do not remove
        /*QSet<QString> test;
        if (init)
           test << "remilia_god" << "flandre" << "kokoro";
        else
           test << "renko" << "renko" << "renko";//test hidden general changing
        return test.toList();*/
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        AcquireGenerals(invoke->invoker, 3, true, QStringList());
        return false;
    }
};

class AnyunShowStaticSkill : public TriggerSkill
{
public:
    AnyunShowStaticSkill()
        : TriggerSkill("#anyunShowStatic")
    {
        events << EventPhaseStart;
    }

    static QList<const Skill *> getStaticSkills(ServerPlayer *nue)
    {
        QList<const Skill *> show;
        QString shown = nue->getShownHiddenGeneral();
        if (shown != NULL)
            return show;
        QStringList generals = nue->getHiddenGenerals();
        foreach (QString name, generals) {
            const General *p = Sanguosha->getGeneral(name);
            foreach (const Skill *skill, p->getSkillList()) {
                if (skill->getShowType() == "static" && !skill->isLordSkill() && !skill->isAttachedLordSkill() && skill->getFrequency() != Skill::Limited
                    && skill->getFrequency() != Skill::Wake && skill->getFrequency() != Skill::Eternal)
                    show << skill;
            }
        }
        return show;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *nue = data.value<ServerPlayer *>();
            if (nue && nue->hasSkill("anyun") && nue->getPhase() == Player::Start) {
                QList<const Skill *> show = getStaticSkills(nue);
                if (!show.isEmpty())
                    d << SkillInvokeDetail(this, nue, nue);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        QList<const Skill *> show = getStaticSkills(invoke->invoker);
        QStringList skills;
        foreach (const Skill *s, show)
            skills << s->objectName();
        skills << "cancel";
        QString choice = room->askForChoice(invoke->invoker, objectName(), skills.join("+"), data);
        if (choice == "cancel")
            return false;

        invoke->tag["anyun_state"] = choice;
        return true;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString name = invoke->tag.value("anyun_state").toString();
        // this flag is only for checking hasSkill during showHidden
        room->setPlayerFlag(invoke->invoker, "has_anyu_state");
        invoke->invoker->showHiddenSkill(name);
        room->setPlayerFlag(invoke->invoker, "-has_anyu_state");
        return false;
    }
};

class AnyunProhibit : public TriggerSkill
{
public:
    AnyunProhibit()
        : TriggerSkill("#anyun_prohibit")
    {
        events << TargetConfirming;
    }

    static QStringList getProhibitSkills(ServerPlayer *nue, CardUseStruct use, bool once = false)
    {
        QStringList show;
        QString shown = nue->getShownHiddenGeneral();
        if (shown != NULL)
            return show;
        QStringList generals = nue->getHiddenGenerals();
        foreach (QString name, generals) {
            const General *p = Sanguosha->getGeneral(name);
            foreach (const Skill *skill, p->getSkillList()) {
                if (skill->inherits("ProhibitSkill")) {
                    const ProhibitSkill *s = qobject_cast<const ProhibitSkill *>(skill);
                    if (s->isProhibited(use.from, nue, use.card, QList<const Player *>(), true))
                        show << s->objectName();
                }
                if (!show.isEmpty() && once)
                    return show;
            }
        }
        return show;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *nue, use.to) {
            if (nue->hasSkill("anyun")) {
                QStringList show = getProhibitSkills(nue, use, true);
                if (!show.isEmpty())
                    d << SkillInvokeDetail(this, nue, nue);
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *nue = invoke->invoker;
        QStringList skills = getProhibitSkills(nue, use);
        skills << "cancel";
        QString choice = room->askForChoice(invoke->invoker, objectName(), skills.join("+"), data);
        if (choice == "cancel")
            return false;

        invoke->tag["anyun_prohibit"] = choice;
        return true;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.to.removeAll(invoke->invoker);
        data = QVariant::fromValue(use);
        QString name = invoke->tag.value("anyun_prohibit").toString();
        invoke->invoker->showHiddenSkill(name);

        LogMessage log;
        log.type = "#SkillAvoid";
        log.from = invoke->invoker;
        log.arg = name;
        log.arg2 = use.card->objectName();
        room->sendLog(log);
        return false;
    }
};

class Benzun : public TriggerSkill
{
public:
    Benzun()
        : TriggerSkill("benzun")
    {
        events << EventPhaseChanging << GameStart;
        frequency = Compulsory;
    }

    void record(TriggerEvent e, Room *, QVariant &data) const
    {
        if (e == GameStart) {
            ServerPlayer *nue = data.value<ServerPlayer *>();
            if (nue)
                nue->tag["init_MaxHp"] = QVariant::fromValue(nue->getMaxHp());
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent != EventPhaseChanging)
            return QList<SkillInvokeDetail>();
        QList<SkillInvokeDetail> d;
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            foreach (ServerPlayer *nue, room->findPlayersBySkillName(objectName())) {
                QString name = nue->getShownHiddenGeneral();
                QStringList generals = nue->getHiddenGenerals();
                if (name != NULL && generals.contains(name))
                    d << SkillInvokeDetail(this, nue, nue, NULL, true);
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QString name = invoke->invoker->getShownHiddenGeneral();
        QStringList names;
        names << name;
        Anyun::AcquireGenerals(invoke->invoker, 1, false, names);

        int maxhp = invoke->invoker->tag["init_MaxHp"].toInt();
        room->setPlayerProperty(invoke->invoker, "maxhp", maxhp);

        //remove piles and marks
        const General *fromGeneral = Sanguosha->getGeneral(name);
        QList<int> idlist;
        if (fromGeneral != NULL) {
            foreach (const Skill *skill, fromGeneral->getSkillList()) {
                QString mark = skill->getRelatedMark();
                if (mark != NULL)
                    invoke->invoker->loseAllMarks(mark);

                QString pile = skill->getRelatedPileName();
                if (pile != NULL)
                    idlist.append(invoke->invoker->getPile(pile));
            }
        }
        if (!idlist.isEmpty()) {
            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, invoke->invoker->objectName(), NULL, "benzun", QString());
            CardsMoveStruct move(idlist, invoke->invoker, Player::DiscardPile, reason);
            room->moveCardsAtomic(move, true);
        }

        JsonArray arg;
        arg << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
        arg << invoke->invoker->objectName();
        arg << QString() << QString() << QString() << QString();
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

        return false;
    }
};

class Chongneng : public TriggerSkill
{
public:
    Chongneng()
        : TriggerSkill("chongneng")
    {
        events << TargetSpecifying;
        related_mark = "@star";
    }

    static bool can_add(CardUseStruct use)
    {
        return use.from->getMark("@star") > 0 && use.card->getTypeId() != Card::TypeEquip && !use.card->isKindOf("DelayedTrick")
            && !(use.card->isKindOf("IronChain") || use.card->isKindOf("LureTiger"));
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QStringList ban_list;
        ban_list << "Jink"
                 << "Nullification";
        if (ban_list.contains(use.card->getClassName()))
            return QList<SkillInvokeDetail>();

        if (use.from && use.from->isAlive() && use.from->hasSkill(this)) {
            if (use.m_isHandcard && use.card->getTypeId() != Card::TypeSkill && !use.to.isEmpty())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (invoke->invoker->askForSkillInvoke(this, data)) {
            CardUseStruct use = data.value<CardUseStruct>();
            invoke->invoker->showHiddenSkill(objectName());
            bool optional = can_add(use);
            ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, use.to, objectName(), "@chongneng", optional, true);
            int chongneng_choice = (target == NULL) ? 2 : 1;
            invoke->tag["chongneng"] = QVariant::fromValue(chongneng_choice);
            if (target)
                invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        int chongneng_choice = invoke->tag["chongneng"].toInt();
        CardUseStruct use = data.value<CardUseStruct>();
        if (chongneng_choice == 1) {
            invoke->invoker->gainMark("@star");
            use.to.removeAll(invoke->targets.first());
            data = QVariant::fromValue(use);
            room->touhouLogmessage("$CancelTarget", use.from, use.card->objectName(), invoke->targets);
        } else {
            invoke->invoker->loseMark("@star");
            QString choice = "first";
            if (use.card->isKindOf("FireAttack") || use.card->isKindOf("Duel") || use.card->isKindOf("SavageAssault") || use.card->isKindOf("ArcheryAttack")
                || use.card->isKindOf("AwaitExhausted")) {
                QString choices = QString("%1_first+%2_second").arg(use.card->objectName()).arg(use.card->objectName());
                choice = room->askForChoice(invoke->invoker, objectName(), choices);
                if (choice.endsWith("second"))
                    room->setCardFlag(use.card, "mopao2");
                else
                    room->setCardFlag(use.card, "mopao");
            } else if (use.card->isKindOf("Slash")) {
                QString choice = "second";
                if (use.card->isKindOf("LightSlash") || use.card->isKindOf("PowerSlash")) {
                    QString choices = QString("%1_first+%2_second").arg(use.card->objectName()).arg(use.card->objectName());
                    choice = room->askForChoice(invoke->invoker, objectName(), choices);
                }

                if (choice.endsWith("second"))
                    room->setCardFlag(use.card, "mopao2");
                else
                    room->setCardFlag(use.card, "mopao");
            } else
                room->setCardFlag(use.card, "mopao");

            LogMessage log;
            log.type = "#Chongneng";
            log.from = invoke->invoker;
            log.arg = use.card->objectName();
            if (choice.endsWith("second"))
                log.arg2 = "2";
            else
                log.arg2 = "1";
            room->sendLog(log);
        }

        if (use.card->getSuit() == Card::Spade)
            invoke->invoker->drawCards(1);
        return false;
    }
};

class Huixing : public TriggerSkill
{
public:
    Huixing()
        : TriggerSkill("huixing")
    {
        events << CardFinished;
        related_mark = "@star";
    }

    static bool hasCardValue(CardUseStruct use)
    {
        return !(use.card->isKindOf("IronChain") || use.card->isKindOf("LureTiger"));
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        //QStringList ban_list;
        //ban_list << "Jink"
        //         << "Nullification";
        //if (ban_list.contains(use.card->getClassName()))
        //    return QList<SkillInvokeDetail>();
        if (use.card->isKindOf("Jink") || use.card->isKindOf("Nullification"))
            return QList<SkillInvokeDetail>();
        if (use.from && use.from->isAlive() && use.from->hasSkill(this) && use.from->getMark("@star") > 0 && use.to.length() == 1 && !use.from->hasFlag("Global_ProcessBroken")) {
            if ((use.card->isKindOf("BasicCard") || use.card->isNDTrick())) {
                ServerPlayer *target = use.to.first();
                if (target->isAlive() && !target->isRemoved()) {
                    ServerPlayer *next = qobject_cast<ServerPlayer *>(target->getNextAlive(1));
                    Card *card = Sanguosha->cloneCard(use.card->objectName());
                    card->setSkillName("huixing");
                    card->deleteLater();
                    if (next && next != target && !use.from->isCardLimited(card, Card::MethodUse) && !use.from->isProhibited(next, card)) {
                        if (card->isKindOf("Peach") && next->isWounded())
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, false, next);
                        else if (card->targetFilter(QList<const Player *>(), next, use.from))
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, false, next);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "target:" + objectName() + ":" + invoke->preferredTarget->objectName() + ":" + use.card->objectName();
        return invoke->invoker->askForSkillInvoke(this, prompt);
    }
    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        invoke->invoker->loseMark("@star");
        CardUseStruct use = data.value<CardUseStruct>();
        Card *card = Sanguosha->cloneCard(use.card->objectName());
        //copy flag
        QStringList flags;
        flags << "mopao"
              << "mopao2"
              << "jidu_card";
        foreach (QString flag, flags) {
            if (use.card->hasFlag(flag))
                card->setFlags(flag);
        }

        card->setSkillName("huixing");
        room->useCard(CardUseStruct(card, invoke->invoker, invoke->targets.first()), false);
        return false;
    }
};

class HuixingTargetMod : public TargetModSkill
{
public:
    HuixingTargetMod()
        : TargetModSkill("#huixing_effect")
    {
        pattern = "BasicCard,TrickCard";
    }

    virtual int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->getSkillName() == "huixing")
            return 1000;
        else
            return 0;
    }
};

XianshiDialog *XianshiDialog::getInstance(const QString &object, bool left, bool right)
{
    static QPointer<XianshiDialog> instance;

    if (!instance.isNull() && instance->objectName() != object)
        delete instance;

    if (instance.isNull()) {
        instance = new XianshiDialog(object, left, right);
        connect(qApp, &QCoreApplication::aboutToQuit, instance.data(), &XianshiDialog::deleteLater);
    }

    return instance;
}

XianshiDialog::XianshiDialog(const QString &object, bool left, bool right)
    : object_name(object)
{
    setObjectName(object);
    setWindowTitle(Sanguosha->translate(object)); //need translate title?
    group = new QButtonGroup(this);

    QHBoxLayout *layout = new QHBoxLayout;
    if (left)
        layout->addWidget(createLeft());
    if (right)
        layout->addWidget(createRight());
    setLayout(layout);

    connect(group, SIGNAL(buttonClicked(QAbstractButton *)), this, SLOT(selectCard(QAbstractButton *)));
}

void XianshiDialog::popup()
{
    /*Card::HandlingMethod method;
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        method = Card::MethodResponse;
    else
        method = Card::MethodUse;
*/
    QStringList checkedPatterns;
    QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
    bool play = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY);
    QString xianshi_record = Self->property("xianshi_record").toString();

    if (play)
        checkedPatterns = xianshi_record.split("+");
    else {
        const Skill *skill = Sanguosha->getSkill(object_name);
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        bool basic_pattern = false;
        bool trick_pattern = false;
        foreach (const Card *card, cards) {
            if (card->isKindOf("BasicCard") && !basic_pattern && skill->matchAvaliablePattern(card->objectName(), pattern))
                basic_pattern = true;
            else if (card->isKindOf("TrickCard") && !trick_pattern && skill->matchAvaliablePattern(card->objectName(), pattern))
                trick_pattern = true;
            if (trick_pattern && basic_pattern)
                break;
        }
        foreach (QString name, xianshi_record.split("+")) {
            if (skill->matchAvaliablePattern(name, "BasicCard") && trick_pattern)
                checkedPatterns << name;
            else if (skill->matchAvaliablePattern(name, "TrickCard") && basic_pattern)
                checkedPatterns << name;
        }
    }

    foreach (QAbstractButton *button, group->buttons()) {
        const Card *card = map[button->objectName()];
        bool enabled = checkedPatterns.contains(card->objectName());

        button->setEnabled(enabled);
    }

    Self->tag.remove(object_name);
    exec();
}

void XianshiDialog::selectCard(QAbstractButton *button)
{
    const Card *card = map.value(button->objectName());
    Self->tag[object_name] = QVariant::fromValue(card->objectName());

    emit onButtonClick();
    accept();
}

QGroupBox *XianshiDialog::createLeft()
{
    QGroupBox *box = new QGroupBox;
    box->setTitle(Sanguosha->translate("basic"));

    QVBoxLayout *layout = new QVBoxLayout;

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    QStringList ban_list = Sanguosha->getBanPackages();

    foreach (const Card *card, cards) {
        if (card->getTypeId() == Card::TypeBasic && !map.contains(card->objectName())
            && !ban_list.contains(card->getPackage())) { //&& !ServerInfo.Extensions.contains("!" + card->getPackage())
            Card *c = Sanguosha->cloneCard(card->objectName());
            c->setParent(this);
            layout->addWidget(createButton(c));
        }
    }

    layout->addStretch();
    box->setLayout(layout);
    return box;
}

QGroupBox *XianshiDialog::createRight()
{
    QGroupBox *box = new QGroupBox(Sanguosha->translate("ndtrick"));
    QHBoxLayout *layout = new QHBoxLayout;

    QGroupBox *box1 = new QGroupBox(Sanguosha->translate("single_target_trick"));
    QVBoxLayout *layout1 = new QVBoxLayout;

    QGroupBox *box2 = new QGroupBox(Sanguosha->translate("multiple_target_trick"));
    QVBoxLayout *layout2 = new QVBoxLayout;

    QStringList ban_list = Sanguosha->getBanPackages();

    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
    foreach (const Card *card, cards) {
        if (card->isNDTrick() && !map.contains(card->objectName()) && !ban_list.contains(card->getPackage())) { //&& !ServerInfo.Extensions.contains("!" + card->getPackage())
            Card *c = Sanguosha->cloneCard(card->objectName());
            c->setSkillName(object_name);
            c->setParent(this);

            QVBoxLayout *layout = c->isKindOf("SingleTargetTrick") ? layout1 : layout2;
            layout->addWidget(createButton(c));
        }
    }

    box->setLayout(layout);
    box1->setLayout(layout1);
    box2->setLayout(layout2);

    layout1->addStretch();
    layout2->addStretch();

    layout->addWidget(box1);
    layout->addWidget(box2);
    return box;
}

QAbstractButton *XianshiDialog::createButton(const Card *card)
{
    if (card->objectName() == "slash" && map.contains(card->objectName()) && !map.contains("normal_slash")) {
        QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate("normal_slash"));
        button->setObjectName("normal_slash");
        button->setToolTip(card->getDescription());

        map.insert("normal_slash", card);
        group->addButton(button);

        return button;
    } else {
        QCommandLinkButton *button = new QCommandLinkButton(Sanguosha->translate(card->objectName()));
        button->setObjectName(card->objectName());
        button->setToolTip(card->getDescription());

        map.insert(card->objectName(), card);
        group->addButton(button);

        return button;
    }
}

XianshiCard::XianshiCard()
{
    will_throw = false;
}

bool XianshiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *card = Sanguosha->cloneCard(oc->objectName());
    DELETE_OVER_SCOPE(Card, card)
    card->addSubcard(oc);
    card->setSkillName("xianshi");
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool XianshiCard::targetFixed() const
{
    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *card = Sanguosha->cloneCard(oc->objectName());
    DELETE_OVER_SCOPE(Card, card)
    card->addSubcard(oc);
    card->setSkillName("xianshi");
    return card && card->targetFixed();
}

bool XianshiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Card *oc = Sanguosha->getCard(subcards.first());
    Card *card = Sanguosha->cloneCard(oc->objectName());
    DELETE_OVER_SCOPE(Card, card)
    card->addSubcard(oc);
    card->setSkillName("xianshi");
    if (card->canRecast() && targets.length() == 0)
        return false;
    return card && card->targetsFeasible(targets, Self);
}

const Card *XianshiCard::validate(CardUseStruct &use) const
{
    QString xianshi_name = user_string;
    use.from->showHiddenSkill("xianshi");
    const Card *card = Sanguosha->getCard(subcards.first());
    Card *use_card = Sanguosha->cloneCard(card->objectName(), card->getSuit(), card->getNumber());
    use_card->setSkillName("xianshi");

    use_card->addSubcard(subcards.first());
    use_card->deleteLater();
    //log
    use.from->getRoom()->touhouLogmessage("#Xianshi", use.from, use_card->objectName(), QList<ServerPlayer *>(), xianshi_name);
    //record selected effect
    use.from->getRoom()->setPlayerProperty(use.from, "xianshi_card", xianshi_name);
    return use_card;
}

const Card *XianshiCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();

    QString xianshi_name = user_string;
    user->showHiddenSkill("xianshi");

    const Card *card = Sanguosha->getCard(subcards.first());
    Card *use_card = Sanguosha->cloneCard(card->objectName(), card->getSuit(), card->getNumber());
    use_card->setSkillName("xianshi");
    use_card->addSubcard(subcards.first());
    use_card->deleteLater();
    //log
    room->touhouLogmessage("#Xianshi", user, use_card->objectName(), QList<ServerPlayer *>(), xianshi_name);
    //record selected effect
    room->setPlayerProperty(user, "xianshi_card", xianshi_name);
    return use_card;
}

class XianshiVS : public OneCardViewAsSkill
{
public:
    XianshiVS()
        : OneCardViewAsSkill("xianshi")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const Card *c) const
    {
        QString selected_effect = Self->tag.value("xianshi", QString()).toString();
        bool play = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY);
        if (play) {
            if (matchAvaliablePattern(selected_effect, "BasicCard"))
                return c->isNDTrick() && c->isAvailable(Self);
            else
                return c->isKindOf("BasicCard") && c->isAvailable(Self);
        } else {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (matchAvaliablePattern(selected_effect, "BasicCard"))
                return c->isNDTrick() && matchAvaliablePattern(c->objectName(), pattern);
            else
                return c->isKindOf("BasicCard") && matchAvaliablePattern(c->objectName(), pattern);
        }

        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            return false;
        if (player->hasFlag("xianshi_used"))
            return false;
        QString xianshi_record = player->property("xianshi_record").toString();
        if (xianshi_record == NULL)
            return false;

        QStringList checkedPatterns;
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        bool basic_pattern = false;
        bool trick_pattern = false;
        foreach (const Card *card, cards) {
            if (card->isKindOf("BasicCard") && !basic_pattern && matchAvaliablePattern(card->objectName(), pattern))
                basic_pattern = true;
            else if (card->isKindOf("TrickCard") && !trick_pattern && matchAvaliablePattern(card->objectName(), pattern))
                trick_pattern = true;
            if (trick_pattern && basic_pattern)
                break;
        }

        foreach (QString name, xianshi_record.split("+")) {
            if (matchAvaliablePattern(name, "BasicCard") && trick_pattern)
                return true;
            else if (matchAvaliablePattern(name, "TrickCard") && basic_pattern)
                return true;
        }

        return !checkedPatterns.isEmpty();
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasFlag("xianshi_used"))
            return false;

        QString xianshi_record = player->property("xianshi_record").toString();
        if (xianshi_record == NULL)
            return false;

        return true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            QString name = Self->tag.value("xianshi", QString()).toString();
            if (name != NULL) {
                XianshiCard *card = new XianshiCard;
                card->setUserString(name);
                card->addSubcard(originalCard);
                return card;
            } else
                return NULL;
        } else
            return NULL;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        if (player->hasFlag("xianshi_used"))
            return false;
        bool record = false;
        QString xianshi_record = player->property("xianshi_record").toString();
        if (xianshi_record == NULL)
            return false;
        foreach (QString name, xianshi_record.split("+")) {
            if (matchAvaliablePattern(name, "BasicCard")) {
                record = true;
                break;
            }
        }
        if (!record)
            return false;

        foreach (const Card *card, player->getHandcards()) {
            if (card->objectName() == "nullification")
                return true;
        }

        if (player->hasTreasure("wooden_ox")) {
            foreach (int id, player->getPile("wooden_ox")) {
                if (Sanguosha->getCard(id)->objectName() == "nullification")
                    return true;
            }
        }
        return false;
    }
};

class Xianshi : public TriggerSkill
{
public:
    Xianshi()
        : TriggerSkill("xianshi")
    {
        events << TargetSpecified << EventPhaseChanging << PreCardUsed << CardResponded;
        view_as_skill = new XianshiVS;
    }

    virtual QDialog *getDialog() const
    {
        return XianshiDialog::getInstance("xianshi");
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            ServerPlayer *current = room->getCurrent();
            if (use.from && current && current->getPhase() == Player::Play && use.card && !use.to.isEmpty()) {
                if (use.card->isNDTrick() || use.card->isKindOf("BasicCard")) {
                    foreach (ServerPlayer *p, room->getAlivePlayers()) {
                        QString xianshi_record = p->property("xianshi_record").toString();
                        QStringList cards;
                        if (xianshi_record != NULL)
                            cards = xianshi_record.split("+");

                        if (!cards.contains(use.card->objectName())) {
                            cards.append(use.card->objectName());
                            xianshi_record = cards.join("+");
                            room->setPlayerProperty(p, "xianshi_record", xianshi_record);
                        }
                    }
                }
            }
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    room->setPlayerFlag(p, "-xianshi_used");
                    room->setPlayerProperty(p, "xianshi_record", QString());
                    room->setPlayerProperty(p, "xianshi_card", QString());
                }
            }
        } else if (e == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->getSkillName() == objectName()) {
                room->setPlayerFlag(use.from, "xianshi_used");
                QString cardname = use.from->property("xianshi_card").toString();
                room->setCardFlag(use.card, "xianshi_" + cardname);
            }
        } else if (e == CardResponded) { //only for jink with extra effect
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_from && response.m_isUse && response.m_card && response.m_card->isKindOf("Jink") && response.m_card->getSkillName() == objectName()) {
                room->setPlayerFlag(response.m_from, "xianshi_used");
            }
        }
    }
};

class Riyue : public TargetModSkill
{
public:
    Riyue()
        : TargetModSkill("riyue")
    {
        pattern = "Slash,TrickCard";
    }

    virtual int getExtraTargetNum(const Player *player, const Card *card) const
    {
        if (!player->hasSkill(this) || !Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY)
            return 0;

        if (card->isRed()) {
            if (card->canDamage())
                return 1000;
            if (card->getSkillName() == "xianshi") {
                QString selected_effect = Self->tag.value("xianshi", QString()).toString();
                if (selected_effect != NULL) {
                    Card *extracard = Sanguosha->cloneCard(selected_effect);
                    extracard->deleteLater();
                    if (extracard->canDamage())
                        return 1000;
                }
            }

        } else if (card->isBlack()) {
            if (card->isKindOf("Peach"))
                return 1000;
            if (card->getSkillName() == "xianshi") {
                QString selected_effect = Self->tag.value("xianshi", QString()).toString();
                if (selected_effect != NULL && (selected_effect.contains("peach") || selected_effect.contains("analeptic")))
                    return 1000;
            }
        }

        return 0;
    }
};

WenyueCard::WenyueCard()
{
    will_throw = false;
}

bool WenyueCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (to_select->hasFlag("Global_wenyueFailed"))
        return false;
    if (targets.length() == 0) {
        foreach (int id, subcards) {
            if (Sanguosha->getCard(id)->getTypeId() == Card::TypeEquip) {
                const EquipCard *equip = qobject_cast<const EquipCard *>(Sanguosha->getCard(id)->getRealCard());
                if (!to_select->getEquip(equip->location()))
                    return true;
            }
        }
    }
    return false;
}

void WenyueCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    int throw_id = -1, card_id = -1;
    foreach (int id, subcards) {
        if (Sanguosha->getCard(id)->getTypeId() == Card::TypeEquip)
            card_id = id;
        else
            throw_id = id;
    }
    if (card_id != -1) {
        DummyCard dummy;
        dummy.addSubcard(throw_id);
        room->throwCard(&dummy, source, source);
    }
    if (throw_id != -1) {
        const Card *card = Sanguosha->getCard(card_id);
        LogMessage zhijian;
        zhijian.type = "$ZhijianEquip";
        zhijian.from = targets.first();
        zhijian.card_str = QString::number(card_id);
        room->sendLog(zhijian);

        room->moveCardTo(card, source, targets.first(), Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_TRANSFER, source->objectName(), "qianqiang", QString()));
    }
    //deal skill qianqiang or xianji;
    if (source->getGeneralName().startsWith("alice")) {
        QList<ServerPlayer *> skill_targets;
        ServerPlayer *owner = source->tag["wenyueOwner"].value<ServerPlayer *>();
        if (owner->isAlive() && (!owner->hasSkill("qianqiang") || !owner->hasSkill("xianji")))
            skill_targets << owner;

        if (targets.first()->isAlive() && (!targets.first()->hasSkill("qianqiang") || !targets.first()->hasSkill("xianji")))
            skill_targets << targets.first();
        if (skill_targets.isEmpty())
            return;

        ServerPlayer *obtainer = room->askForPlayerChosen(source, skill_targets, "wenyue", "@wenyue_handleAcquireDetachSkill", true, true);
        if (obtainer) {
            QStringList select;
            if (!obtainer->hasSkill("qianqiang"))
                select << "qianqiang";
            if (!obtainer->hasSkill("xianji"))
                select << "xianji";

            source->tag["wenyue_obtainer"] = QVariant::fromValue(obtainer);
            QString skillname = room->askForChoice(source, "wenyue", select.join("+"), QVariant::fromValue(obtainer));
            foreach (ServerPlayer *p, room->getOtherPlayers(obtainer)) {
                if (p->hasSkill(skillname, false, false)) {
                    room->setPlayerMark(p, "@" + skillname, 0);
                    room->handleAcquireDetachSkills(p, "-" + skillname);
                    break;
                }
            }
            room->setPlayerMark(obtainer, "@" + skillname, 1);
            room->handleAcquireDetachSkills(obtainer, skillname);
        }
    }
}

class WenyueVS : public ViewAsSkill
{
public:
    WenyueVS()
        : ViewAsSkill("wenyue")
    {
        response_pattern = "@@wenyue";
        expand_pile = "#wenyue_temp";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        QList<int> temp = Self->getPile("#wenyue_temp");
        if (selected.isEmpty()) {
            return temp.contains(to_select->getId()) || (to_select->getTypeId() != Card::TypeEquip && !Self->isJilei(to_select));
        } else if (selected.length() == 1) {
            if (temp.contains(selected.first()->getId()))
                return to_select->getTypeId() != Card::TypeEquip && !Self->isJilei(to_select);
            else if (selected.first()->getTypeId() != Card::TypeEquip)
                return temp.contains(to_select->getId());
        }
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            WenyueCard *card = new WenyueCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }
};

class Wenyue : public TriggerSkill
{
public:
    Wenyue()
        : TriggerSkill("wenyue")
    {
        events << CardsMoveOneTime << GameStart;
        view_as_skill = new WenyueVS;
    }

    static bool putToPile(Room *room, ServerPlayer *alice, QList<int> ids)
    {
        CardsMoveStruct move;
        move.from_place = Player::DiscardPile;
        move.to = alice;
        move.to_player_name = alice->objectName();
        move.to_pile_name = "#wenyue_temp";
        move.card_ids = ids;
        move.to_place = Player::PlaceSpecial;
        move.open = true;

        QList<CardsMoveStruct> _moves = QList<CardsMoveStruct>() << move;
        QList<ServerPlayer *> _alice = QList<ServerPlayer *>() << alice;
        room->setPlayerFlag(alice, "wenyue_InTempMoving");
        room->notifyMoveCards(true, _moves, true, _alice);
        room->notifyMoveCards(false, _moves, true, _alice);
        room->setPlayerFlag(alice, "-wenyue_InTempMoving");

        QVariantList tag = IntList2VariantList(ids);
        alice->tag["wenyue_tempcards"] = tag;
        return true;
    }

    static void cleanUp(Room *room, ServerPlayer *alice)
    {
        QList<int> equips = VariantList2IntList(alice->tag.value("wenyue_tempcards", QVariantList()).toList());
        alice->tag.remove("wenyue_tempcards");
        if (equips.isEmpty())
            return;

        CardsMoveStruct move;
        move.from = alice;
        move.from_player_name = alice->objectName();
        move.from_place = Player::PlaceSpecial;
        move.from_pile_name = "#wenyue_temp";
        move.to_place = Player::DiscardPile;
        move.open = true;
        move.card_ids = equips;

        QList<CardsMoveStruct> _moves = QList<CardsMoveStruct>() << move;
        QList<ServerPlayer *> _alice = QList<ServerPlayer *>() << alice;
        room->setPlayerFlag(alice, "wenyue_InTempMoving");
        room->notifyMoveCards(true, _moves, true, _alice);
        room->notifyMoveCards(false, _moves, true, _alice);
        room->setPlayerFlag(alice, "-wenyue_InTempMoving");
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == GameStart) {
            ServerPlayer *alice = data.value<ServerPlayer *>();
            if (alice && alice->hasSkill(this)) {
                room->setPlayerMark(alice, "@qianqiang", 1);
                room->setPlayerMark(alice, "@xianji", 1);
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (e == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();

            ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
            if (move.reason.m_extraData.value<ServerPlayer *>() != NULL)
                player = move.reason.m_extraData.value<ServerPlayer *>();
            if (player != NULL && move.to_place == Player::DiscardPile) {
                QList<int> equip_ids;
                foreach (int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->getTypeId() == Card::TypeEquip && room->getCardPlace(id) == Player::DiscardPile)
                        equip_ids << id;
                }
                if (equip_ids.isEmpty())
                    return QList<SkillInvokeDetail>();

                bool canMove = false;
                foreach (int id, equip_ids) {
                    const EquipCard *equip = qobject_cast<const EquipCard *>(Sanguosha->getCard(id)->getRealCard());
                    foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                        if (!p->getEquip(equip->location())) {
                            canMove = true;
                            break;
                        }
                    }
                    if (canMove)
                        break;
                }

                foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p->canDiscard(p, "hs"))
                        d << SkillInvokeDetail(this, p, p);
                }
            }
        }

        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
        if (move.reason.m_extraData.value<ServerPlayer *>() != NULL)
            player = move.reason.m_extraData.value<ServerPlayer *>();

        while (true) {
            QList<int> equip_ids;
            foreach (int id, move.card_ids) {
                if (Sanguosha->getCard(id)->getTypeId() == Card::TypeEquip && room->getCardPlace(id) == Player::DiscardPile)
                    equip_ids << id;
            }
            if (equip_ids.isEmpty())
                return false;

            ServerPlayer *alice = invoke->invoker;
            if (!alice->canDiscard(alice, "hs"))
                return false;
            if (!putToPile(room, alice, equip_ids))
                return false;

            room->setPlayerFlag(player, "Global_wenyueFailed");
            alice->tag["wenyueOwner"] = QVariant::fromValue(player);
            QVariantList listc = IntList2VariantList(equip_ids);
            invoke->invoker->tag["wenyue_cards"] = listc;
            const Card *usecard = room->askForUseCard(alice, "@@wenyue", "@wenyue");
            cleanUp(room, alice);
            if (usecard == NULL)
                return false;
        }
        return false;
    }
};

QianqiangCard::QianqiangCard()
{
    will_throw = false;
}

bool QianqiangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (targets.length() == 0) {
        const EquipCard *equip = qobject_cast<const EquipCard *>(Sanguosha->getCard(subcards.first())->getRealCard());
        if (!to_select->getEquip(equip->location()))
            return true;
    }
    return false;
}

void QianqiangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    int card_id = subcards.first();
    const Card *card = Sanguosha->getCard(card_id);
    if (room->getCardPlace(card_id) != Player::PlaceEquip) {
        LogMessage zhijian;
        zhijian.type = "$ZhijianEquip";
        zhijian.from = targets.first();
        zhijian.card_str = QString::number(card_id);
        room->sendLog(zhijian);
    }
    room->moveCardTo(card, source, targets.first(), Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_TRANSFER, source->objectName(), "qianqiang", QString()));

    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->setSkillName("_qianqiang");
    room->useCard(CardUseStruct(slash, source, targets.first()), true);
}

class Qianqiang : public OneCardViewAsSkill
{
public:
    Qianqiang()
        : OneCardViewAsSkill("qianqiang")
    {
        response_or_use = true;
        filter_pattern = "EquipCard|.|.|hand,equipped";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("QianqiangCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            QianqiangCard *card = new QianqiangCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }
};

class Xianji : public TriggerSkill
{
public:
    Xianji()
        : TriggerSkill("xianji")
    {
        events << Dying;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *who = data.value<DyingStruct>().who;
        if (!who->hasSkill(this) || who->getHp() >= who->dyingThreshold() || who->isDead())
            return QList<SkillInvokeDetail>();

        foreach (ServerPlayer *p, room->getOtherPlayers(who)) {
            if (!p->getEquips().isEmpty())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, who, who);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (!p->getEquips().isEmpty())
                targets << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, "xianji", "@xianji_target", true, true);
        if (target)
            invoke->targets << target;
        return target != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->killPlayer(invoke->invoker);
        int num = invoke->targets.first()->getEquips().length();

        invoke->invoker->tag["xianji_target"] = QVariant::fromValue(invoke->targets.first());
        QString choice = room->askForChoice(invoke->invoker, objectName(), "loseHP+recoverHP");
        if (choice == "loseHP") {
            room->loseHp(invoke->targets.first(), num);
        } else {
            RecoverStruct recover;
            recover.who = invoke->invoker;
            recover.recover = num;
            room->recover(invoke->targets.first(), recover);
        }
        return false;
    }
};

class XiuyeVS : public OneCardViewAsSkill
{
public:
    XiuyeVS()
        : OneCardViewAsSkill("xiuye")
    {
        expand_pile = "#xiuye_temp";
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            return false;
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        foreach (const Card *c, ClientInstance->discarded_list) {
            if (c->getSuit() == Card::Club && (c->getTypeId() == Card::TypeBasic || c->isNDTrick())) {
                if (this->matchAvaliablePattern(c->objectName(), pattern))
                    return true;
            }
        }
        return false;
    }

    bool isEnabledAtPlay(const Player *) const
    {
        foreach (const Card *c, ClientInstance->discarded_list) {
            if (c->getSuit() == Card::Club && (c->getTypeId() == Card::TypeBasic || c->isNDTrick()))
                return true;
        }
        return false;
    }

    bool viewFilter(const Card *to_select) const
    {
        if (to_select->getSuit() != Card::Club || !ClientInstance->discarded_list.contains(to_select))
            return false;
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) // in play!!
            return to_select->isAvailable(Self);
        else {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            return this->matchAvaliablePattern(to_select->objectName(), pattern);
        }
        return false;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard) {
            Card *card = Sanguosha->cloneCard(originalCard->objectName());
            card->setSkillName("xiuye");
            card->addSubcard(originalCard);
            card->setCanRecast(false);
            return card;
        } else
            return NULL;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        QList<int> discardpile = player->getRoom()->getDiscardPile();
        foreach (int id, discardpile) {
            Card *c = Sanguosha->getCard(id);
            if (c->getSuit() == Card::Club && c->isKindOf("Nullification"))
                return true;
        }
        return false;
    }
};

class Xiuye : public TriggerSkill
{
public:
    Xiuye()
        : TriggerSkill("xiuye")
    {
        events << CardFinished << EventPhaseChanging; //<< CardsMoveOneTime
        view_as_skill = new XiuyeVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSuit() == Card::Club && use.card->getSkillName() == objectName() && use.from && use.from->isAlive()) {
                QList<int> ids;
                if (use.card->isVirtualCard())
                    ids = use.card->getSubcards();
                else
                    ids << use.card->getEffectiveId();

                if (ids.isEmpty())
                    return QList<SkillInvokeDetail>();
                foreach (int id, ids) {
                    if (room->getCardPlace(id) != Player::DiscardPile)
                        return QList<SkillInvokeDetail>();
                }
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from, NULL, true);
            }
        } else if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && change.player && !change.player->getPile("xiuye").isEmpty() && change.player->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, change.player, change.player, NULL, true);
        }

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            invoke->invoker->addToPile("xiuye", use.card->getSubcards());
        } else {
            QList<int> ids = invoke->invoker->getPile("xiuye");

            /*CardsMoveStruct move;
            move.card_ids = ids;
            move.from = invoke->invoker;
            move.to_place = Player::DrawPile;
            room->moveCardsAtomic(move, true);//move visible?
            */
            //room->askForGuanxing(invoke->invoker, room->getNCards(num), Room::GuanxingDownOnly, objectName());

            room->moveCardsToEndOfDrawpile(ids, true);

            LogMessage l;
            l.type = "$jinghua";
            l.from = invoke->invoker;
            l.card_str = IntList2StringList(ids).join("+");
            room->sendLog(l);

            if (ids.length() > 1)
                room->askForGuanxing(invoke->invoker, ids, Room::GuanxingDownOnly, objectName());

            /*
            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), NULL, objectName(), QString());
            DummyCard *dc = new DummyCard;
            dc->deleteLater();
            dc->addSubcards(invoke->invoker->getPile("xiuye"));
            room->throwCard(dc, reason, NULL);*/
        }
        return false;
    }
};

KuangjiCard::KuangjiCard()
{
    will_throw = true;
    handling_method = Card::MethodNone;
    m_skillName = "kuangji";
}

static bool hasCardValue(QString name)
{
    if (name == "iron_chain" || name == "lure_tiger")
        return false;
    return true;
}

bool KuangjiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return to_select->hasFlag("Global_kuangjiFailed") && targets.length() < subcards.length();
}

bool KuangjiCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    QString kuangji_name = Self->property("kuangji_name").toString();
    return (targets.isEmpty() && hasCardValue(kuangji_name)) || targets.length() == subcards.length();
}

void KuangjiCard::use(Room *room, ServerPlayer *user, QList<ServerPlayer *> &targets) const
{
    room->setPlayerMark(user, "kuangji_cardlength", subcards.length());
    if (targets.isEmpty())
        room->setPlayerMark(user, "kuangji_value", subcards.length());
    else {
        foreach (ServerPlayer *t, targets)
            room->setPlayerMark(t, "kuangji_select", 1);
    }
}

class KuangjiVS : public ViewAsSkill
{
public:
    KuangjiVS()
        : ViewAsSkill("kuangji")
    {
        response_pattern = "@@kuangji";
    }

    static QString uniqueName(QString pattern)
    {
        if (pattern.contains("slash"))
            pattern = "slash";
        else if (pattern.contains("jink"))
            pattern = "jink";
        else if (pattern.contains("analeptic"))
            pattern = "analeptic";
        else if (pattern.contains("peach"))
            pattern = "peach";
        return pattern;
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        //const Card *card = Self->tag.value("kuangji").value<const Card *>();
        QString kuangji_name = Self->property("kuangji_name").toString();
        return uniqueName(to_select->objectName()) == uniqueName(kuangji_name);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0) {
            KuangjiCard *card = new KuangjiCard;
            card->addSubcards(cards);
            return card;
        } else
            return NULL;
    }
};

class Kuangji : public TriggerSkill
{
public:
    Kuangji()
        : TriggerSkill("kuangji")
    {
        events << TargetSpecifying;
        view_as_skill = new KuangjiVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        //QStringList ban_list;
        //ban_list << "Jink"
        //    << "Nullification";
        //if (ban_list.contains(use.card->getClassName()))
        //    return QList<SkillInvokeDetail>();
        if (use.from && use.from->isAlive() && use.from->hasSkill(this) && use.to.length() == 1) {
            if ((use.card->isKindOf("BasicCard") || use.card->isNDTrick())) {
                if (!use.from->canDiscard(use.from, "hs"))
                    return QList<SkillInvokeDetail>();
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        //invoke->invoker->tag["kuangji"] = QVariant::fromValue(use.card);
        room->setPlayerProperty(invoke->invoker, "kuangji_name", use.card->objectName());
        ServerPlayer *target = use.to.first();
        room->setPlayerFlag(invoke->invoker, "Global_kuangjiTargetmodFailed");
        foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
            if (!use.from->isProhibited(p, use.card)) {
                if (use.card->isKindOf("Peach") && p->isWounded())
                    room->setPlayerFlag(p, "Global_kuangjiFailed");
                else if (use.card->targetFilter(QList<const Player *>(), p, use.from))
                    room->setPlayerFlag(p, "Global_kuangjiFailed");
            }
        }

        return room->askForUseCard(invoke->invoker, "@@kuangji", "@kuangji:" + KuangjiVS::uniqueName(use.card->objectName()));
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *t, room->getAlivePlayers()) {
            if (t->getMark("kuangji_select") > 0) {
                room->setPlayerMark(t, "kuangji_select", 0);
                use.to << t;
            }
        }
        room->sortByActionOrder(use.to);
        data = QVariant::fromValue(use);

        int num = invoke->invoker->getMark("kuangji_cardlength");
        if (num >= invoke->invoker->getHp())
            invoke->invoker->drawCards(num);
        room->setPlayerMark(invoke->invoker, "kuangji_cardlength", 0);

        return false;
    }
};

class KuangjiTargetMod : public TargetModSkill
{
public:
    KuangjiTargetMod()
        : TargetModSkill("#kuangji_effect")
    {
        pattern = "BasicCard,TrickCard";
    }

    virtual int getDistanceLimit(const Player *player, const Card *) const
    {
        if (player->hasFlag("Global_kuangjiTargetmodFailed"))
            return 1000;
        else
            return 0;
    }
};

class Qizhi : public TriggerSkill
{
public:
    Qizhi()
        : TriggerSkill("qizhi")
    {
        events << HpChanged << EventPhaseStart;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == HpChanged) {
            ServerPlayer *tenshi = data.value<ServerPlayer *>();
            if (tenshi == NULL || tenshi->isDead() || !tenshi->hasSkill(this))
                return QList<SkillInvokeDetail>();

            ServerPlayer *current = room->getCurrent();
            if (current == NULL || current->isDead())
                return QList<SkillInvokeDetail>();

            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, tenshi, tenshi, NULL, true);
        }
        if (e == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current == NULL || current->isDead())
                return QList<SkillInvokeDetail>();

            if (current->getPhase() == Player::Start && current->hasSkill(this)) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current, NULL, true);
            }
            if (current->getPhase() == Player::Judge && current->getJudgingAreaID().isEmpty()) {
                ServerPlayer *tenshi = room->findPlayerBySkillName(objectName());
                if (tenshi) {
                    QVariant tag = room->getTag("qizhi_card");
                    if (!tag.isNull() && tag.canConvert(QVariant::String)) {
                        QString exclude = tag.toString();
                        if (exclude != "")
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, tenshi, tenshi, NULL, true);
                    }
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    static void choiceQizhi(Room *room)
    {
        QStringList names;
        names << "lightning"
              << "indulgence"
              << "supply_shortage"
              << "saving_energy"
              << "spring_breath";
        QVariant tag = room->getTag("qizhi_card");
        if (!tag.isNull() && tag.canConvert(QVariant::String)) {
            QString exclude = tag.toString();
            names.removeAll(exclude);
        }

        ServerPlayer *current = room->getCurrent();
        QString choice = room->askForChoice(current, "qizhi", names.join("+"), QVariant());
        room->setTag("qizhi_card", choice);
        room->touhouLogmessage("#qizhi", current, choice);
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
    {
        if (e == HpChanged)
            choiceQizhi(room);
        else if (e == EventPhaseStart) {
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (current->getPhase() == Player::Start)
                choiceQizhi(room);
            else {
                QVariant tag = room->getTag("qizhi_card");
                QString exclude = tag.toString();
                room->touhouLogmessage("#qizhi_judge", current, objectName(), QList<ServerPlayer *>(), exclude);
                if (exclude == "lightning") {
                    Lightning *c = new Lightning(Card::NoSuit, 0);
                    JudgeStruct judge_struct = c->getJudge();
                    judge_struct.who = current;
                    room->judge(judge_struct);

                    if (judge_struct.negative == judge_struct.isBad()) {
                        if (judge_struct.who->isAlive() && !judge_struct.ignore_judge)
                            c->takeEffect(judge_struct.who);
                    }
                    delete c;
                }

                if (exclude == "indulgence") {
                    Indulgence *c = new Indulgence(Card::NoSuit, 0);
                    JudgeStruct judge_struct = c->getJudge();
                    judge_struct.who = current;
                    room->judge(judge_struct);

                    if (judge_struct.negative == judge_struct.isBad()) {
                        if (judge_struct.who->isAlive() && !judge_struct.ignore_judge)
                            c->takeEffect(judge_struct.who);
                    }
                    delete c;
                }
                if (exclude == "supply_shortage") {
                    SupplyShortage *c = new SupplyShortage(Card::NoSuit, 0);
                    JudgeStruct judge_struct = c->getJudge();
                    judge_struct.who = current;
                    room->judge(judge_struct);

                    if (judge_struct.negative == judge_struct.isBad()) {
                        if (judge_struct.who->isAlive() && !judge_struct.ignore_judge)
                            c->takeEffect(judge_struct.who);
                    }
                    delete c;
                }
                if (exclude == "saving_energy") {
                    SavingEnergy *c = new SavingEnergy(Card::NoSuit, 0);
                    JudgeStruct judge_struct = c->getJudge();
                    judge_struct.who = current;
                    room->judge(judge_struct);

                    if (judge_struct.negative == judge_struct.isBad()) {
                        if (judge_struct.who->isAlive() && !judge_struct.ignore_judge)
                            c->takeEffect(judge_struct.who);
                    }
                    delete c;
                }
                if (exclude == "spring_breath") {
                    SpringBreath *c = new SpringBreath(Card::NoSuit, 0);
                    JudgeStruct judge_struct = c->getJudge();
                    judge_struct.who = current;
                    room->judge(judge_struct);

                    if (judge_struct.negative == judge_struct.isBad()) {
                        if (judge_struct.who->isAlive() && !judge_struct.ignore_judge)
                            c->takeEffect(judge_struct.who);
                    }
                    delete c;
                }
            }
        }

        return false;
    }
};

class Tiandao : public TriggerSkill
{
public:
    Tiandao()
        : TriggerSkill("tiandao")
    {
        events << FinishJudge << EventPhaseStart; // << EventPhaseChanging
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player->getPhase() == Player::RoundStart) {
                if (player->isLord() && !player->tag.value("touhou-extra", false).toBool()) {
                    foreach (ServerPlayer *p, room->getAlivePlayers()) {
                        room->setPlayerMark(p, "@tiandao", 0);
                    }
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        if (event == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (room->getCardPlace(judge->card->getEffectiveId()) != Player::PlaceJudge || judge->who->isDead())
                return QList<SkillInvokeDetail>();
            if (judge->card->isKindOf("Nullification") || judge->card->isKindOf("Jink"))
                return QList<SkillInvokeDetail>();

            QList<ServerPlayer *> invokers = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *p, invokers) {
                if (p->isAlive() && p->getMark("@tiandao") < p->getHp() && !p->isCardLimited(judge->card, Card::MethodUse) && !p->isProhibited(judge->who, judge->card)) {
                    judge->card->setFlags("IgnoreFailed");
                    judge->card->setFlags("tiandao");
                    bool can = judge->card->targetFilter(QList<const Player *>(), judge->who, p);
                    judge->card->setFlags("-IgnoreFailed");
                    judge->card->setFlags("-tiandao");
                    if (judge->card->getTypeId() == Card::TypeEquip)
                        can = true;
                    if (can)
                        d << SkillInvokeDetail(this, p, p, NULL, false, judge->who);
                }
            }
        }
        /*else if (event == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach(ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->getPile("tiandao").length() > 0)
                        d << SkillInvokeDetail(this, p, p, NULL, true);
                }
            }
        }*/
        return d;
    }

    /*bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging)
            return true;
        return invoke->invoker->askForSkillInvoke(this, data);
    }*/

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (triggerEvent == FinishJudge) {
            invoke->invoker->gainMark("@tiandao");
            JudgeStruct *judge = data.value<JudgeStruct *>();
            room->useCard(CardUseStruct(judge->card, invoke->invoker, invoke->targets.first()));

            //judge->who->addToPile("tiandao", judge->card->getEffectiveId());

            judge->ignore_judge = true;
            data = QVariant::fromValue(judge);
        }
        /*else if (triggerEvent == EventPhaseChanging) {
            QList<int> pile = invoke->invoker->getPile("tiandao");
            foreach(int id, pile) {
                Card *c = Sanguosha->getCard(id);
                room->useCard(CardUseStruct(c, invoke->invoker, invoke->invoker));
            }
        }*/
        return false;
    }
};

class TiandaoDistance : public TargetModSkill
{
public:
    TiandaoDistance()
        : TargetModSkill("tiandao-dist")
    {
        pattern = "BasicCard,TrickCard";
    }

    int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->hasFlag("tiandao"))
            return 1000;

        return 0;
    }
};

QizhiCard::QizhiCard(Suit suit, int number)
    : DelayedTrick(suit, number)
{
    mute = true;
    handling_method = Card::MethodNone;
    setObjectName("QizhiCard");
}

bool QizhiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    //if (!targets.isEmpty())
    //    return false;

    //if (to_select->containsTrick(objectName()))
    //    return false;

    //return true;
    return targets.isEmpty();
}

/*void QizhiCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    DelayedTrick::onUse(room, card_use);
}*/
void QizhiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    DelayedTrick::use(room, source, targets);
    if (getSuit() == Card::Spade || getSuit() == Card::Heart)
        source->drawCards(1);
}
void QizhiCard::takeEffect(ServerPlayer *) const
{
}

class QizhiVS : public OneCardViewAsSkill
{
public:
    QizhiVS()
        : OneCardViewAsSkill("qizhi_attach")
    {
        filter_pattern = ".";
        attached_lord_skill = true;
        //response_or_use = false;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("QizhiCard"); // && !player->isKongcheng()
    }

    const Card *viewAs(const Card *originalCard) const
    {
        QizhiCard *card = new QizhiCard(originalCard->getSuit(), originalCard->getNumber());
        card->addSubcard(originalCard->getId());
        //card->setSkillName(objectName());
        return card;
    }
};

class QizhiSelfVS : public OneCardViewAsSkill
{
public:
    QizhiSelfVS()
        : OneCardViewAsSkill("qizhialernative")
    {
        filter_pattern = ".";
        //response_or_use = false;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("QizhiCard");
    }

    //virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    //{
    //}

    const Card *viewAs(const Card *originalCard) const
    {
        QizhiCard *card = new QizhiCard(originalCard->getSuit(), originalCard->getNumber());
        card->addSubcard(originalCard->getId());
        //card->setSkillName(objectName());
        return card;
    }
};

class QizhiAlernative : public TriggerSkill
{
public:
    QizhiAlernative()
        : TriggerSkill("qizhialernative")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Death << Debut << Revive << GeneralShown << EventPhaseStart; //<< EventPhaseChanging
        view_as_skill = new QizhiSelfVS;
        show_type = "static";
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const
    {
        if (triggerEvent != EventPhaseStart) { //the case operating attach skill
            QList<ServerPlayer *> owners;
            static QString attachName = "qizhi_attach";
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(this, true, false))
                    owners << p;
            }

            if (owners.length() >= 1) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (!p->hasSkill(attachName, true) && !p->hasSkill("qizhialernative"))
                        room->attachSkillToPlayer(p, attachName);
                    else if (p->hasSkill(attachName, true) && p->hasSkill("qizhialernative"))
                        room->detachSkillFromPlayer(p, attachName, true);
                }
            } else {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasSkill(attachName, true))
                        room->detachSkillFromPlayer(p, attachName, true);
                }
            }
        }
        /*else { //tianbian 
            ServerPlayer *current = data.value<ServerPlayer *>();
            if (!current->hasSkill(this) || current->isDead() || current->getPhase() != Player::Judge || current->getJudgingArea().isEmpty())
                return QList<SkillInvokeDetail>();

        }*/
    }
};

class Tianbian : public TriggerSkill
{
public:
    Tianbian()
        : TriggerSkill("tianbian")
    {
        events << EventPhaseProceeding;
        frequency = Eternal;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current == NULL || current->isDead())
            return QList<SkillInvokeDetail>();
        if (current->getPhase() == Player::Judge) {
            QList<const Card *> tricks = current->getJudgingArea();
            if (tricks.isEmpty())
                return QList<SkillInvokeDetail>();
            QList<ServerPlayer *> tenshis = room->findPlayersBySkillName(objectName());
            QList<SkillInvokeDetail> d;
            foreach (ServerPlayer *tenshi, tenshis)
                d << SkillInvokeDetail(this, tenshi, tenshi, NULL, true);
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *current = data.value<ServerPlayer *>();
        int maxchoice = qMax(current->getJudgingAreaID().length(), 2);
        //step 1 choose card which will not be used
        QList<int> disable;
        for (int i = 0; i < maxchoice; i += 1) {
            if (!invoke->invoker->askForSkillInvoke("tianbian_choosecard"))
                break;

            int card_id = room->askForCardChosen(invoke->invoker, current, "j", objectName(), false, Card::MethodDiscard, disable); //refuasable??
            if (card_id <= -1)
                break;
            disable << card_id;
        }

        //step 2 use card which is avaliable
        QList<int> tricks = current->getJudgingAreaID();
        QList<int> effected;
        while (!tricks.isEmpty() && current->isAlive()) {
            tricks = current->getJudgingAreaID();
            //foreach(int id, effected) {
            //	if (tricks.contains(id))
            //		tricks.removeOne(id);
            //}

            QList<int> use_ids, disable_use_ids;
            foreach (int id, tricks) {
                const Card *c = Sanguosha->getEngineCard(id); //deletelater?
                c->setFlags("tianbian");
                c->setFlags("IgnoreFailed");
                bool can = !disable.contains(id) && usecheck(invoke->invoker, current, c);
                c->setFlags("-IgnoreFailed");
                c->setFlags("-tianbian");
                if (can)
                    use_ids << id;
                else
                    disable_use_ids << id;
            }
            if (use_ids.length() == 0)
                break;
            int card_id = room->askForCardChosen(invoke->invoker, current, "j", objectName(), false, Card::MethodNone, disable_use_ids);
            const Card *c = Sanguosha->getEngineCard(card_id);
            //real delaycard??
            room->useCard(CardUseStruct(c, invoke->invoker, current));
            effected << card_id;
        }

        //step 3 throw all card, and effect delay
        if (current->isAlive() && !current->getJudgingAreaID().isEmpty()) {
            int num = current->getJudgingAreaID().length() / 2;
            DummyCard *dummy = new DummyCard;
            dummy->addSubcards(current->getJudgingAreaID());
            room->throwCard(dummy, current, invoke->invoker);
            QStringList names;
            names << "lightning"
                  << "indulgence"
                  << "supply_shortage"
                  << "saving_energy"
                  << "spring_breath";

            for (int i = 0; i < num; i += 1) {
                QString choice = room->askForChoice(invoke->invoker, objectName(), names.join("+"), QVariant());
                tianbianEffect(choice, current);
            }
        }

        return false;
    }
    static void tianbianEffect(QString choice, ServerPlayer *current)
    {
        DelayedTrick *delay_trick;
        Room *room = current->getRoom();
        if (choice == "lightning") {
            Lightning *c = new Lightning(Card::NoSuit, 0);
            c->deleteLater();
            delay_trick = qobject_cast<DelayedTrick *>(c);
        } else if (choice == "indulgence") {
            Indulgence *c = new Indulgence(Card::NoSuit, 0);
            c->deleteLater();
            delay_trick = qobject_cast<DelayedTrick *>(c);
        } else if (choice == "supply_shortage") {
            SupplyShortage *c = new SupplyShortage(Card::NoSuit, 0);
            c->deleteLater();
            delay_trick = qobject_cast<DelayedTrick *>(c);
        } else if (choice == "saving_energy") {
            SavingEnergy *c = new SavingEnergy(Card::NoSuit, 0);
            c->deleteLater();
            delay_trick = qobject_cast<DelayedTrick *>(c);
        } else if (choice == "spring_breath") {
            SpringBreath *c = new SpringBreath(Card::NoSuit, 0);
            c->deleteLater();
            delay_trick = qobject_cast<DelayedTrick *>(c);
        }
        if (delay_trick != NULL) {
            //bool on_effect = room->cardEffect(delay_trick, NULL, current);  // iscanceled  ->  DelayedTrick::onEffect
            //effected << trick;
            //if (!on_effect)
            //	trick->onNullified(player); just for move delaytrick card

            JudgeStruct judge_struct = delay_trick->getJudge();
            judge_struct.who = current;
            room->judge(judge_struct);

            if (judge_struct.negative == judge_struct.isBad()) {
                if (judge_struct.who->isAlive() && !judge_struct.ignore_judge)
                    delay_trick->takeEffect(judge_struct.who);
            }
            delete delay_trick;
        }
    }

    static bool usecheck(ServerPlayer *from, ServerPlayer *to, const Card *c)
    {
        if (c->isKindOf("Jink") || c->isKindOf("Nullification")) //using matchpattern is better?
            return false;
        if (from->isCardLimited(c, Card::MethodUse) || from->isProhibited(to, c))
            return false;
        if (c->isKindOf("Peach")) {
            if (to->isWounded())
                return true;
        } else if (c->targetFilter(QList<const Player *>(), to, from))
            return true;
        return false;
    }
};

class TianbianDistance : public TargetModSkill
{
public:
    TianbianDistance()
        : TargetModSkill("#tianbian-dist")
    {
        pattern = "Slash,TrickCard+^DelayedTrick";
    }

    int getDistanceLimit(const Player *, const Card *card) const
    {
        if (card->hasFlag("tianbian"))
            return 1000;

        return 0;
    }
};

//skills  << new TianbianDistance

TouhouGodPackage::TouhouGodPackage()
    : Package("touhougod")
{
    General *zun = new General(this, "zun", "touhougod", 1);
    zun->addSkill(new Chuanghuan);

    General *yukari_god = new General(this, "yukari_god", "touhougod", 4);
    yukari_god->addSkill(new Jiexian);

    General *remilia_god = new General(this, "remilia_god", "touhougod", 3);
    remilia_god->addSkill(new Zhouye);
    remilia_god->addSkill(new Hongwu);
    remilia_god->addSkill(new Shenqiang);
    remilia_god->addSkill(new Yewang);

    General *cirno_god = new General(this, "cirno_god", "touhougod", 9);
    cirno_god->addSkill(new Bingfeng);
    cirno_god->addSkill(new Wushen);

    General *utsuho_god = new General(this, "utsuho_god", "touhougod", 4);
    utsuho_god->addSkill(new Shikong);
    utsuho_god->addSkill(new Ronghui);
    utsuho_god->addSkill(new Jubian);
    utsuho_god->addSkill(new Hengxing);

    General *suika_god = new General(this, "suika_god", "touhougod", 0);
    suika_god->addSkill(new Huanmeng);
    suika_god->addSkill(new Cuixiang);
    suika_god->addSkill(new Xuying);

    General *flandre_god = new General(this, "flandre_god", "touhougod", 3);
    flandre_god->addSkill(new Kuangyan);
    flandre_god->addSkill(new Huimie);
    flandre_god->addSkill(new Jinguo);

    General *sakuya_god = new General(this, "sakuya_god", "touhougod", 3);
    sakuya_god->addSkill(new Shicao);
    sakuya_god->addSkill(new Shiting);
    sakuya_god->addSkill(new Huanzai);
    sakuya_god->addSkill(new Shanghun);

    General *youmu_god = new General(this, "youmu_god", "touhougod", 3);
    youmu_god->addSkill(new Banling);
    youmu_god->addSkill(new Rengui);

    General *reisen_god = new General(this, "reisen_god", "touhougod", 4);
    reisen_god->addSkill(new Ningshi);
    reisen_god->addSkill(new Gaoao);

    General *sanae_god = new General(this, "sanae_god", "touhougod", 4);
    sanae_god->addSkill(new Shenshou);

    General *reimu_god = new General(this, "reimu_god", "touhougod", 4);
    reimu_god->addSkill(new Yibian);
    reimu_god->addSkill(new Fengyin);
    reimu_god->addSkill(new Huanxiang);

    General *shikieiki_god = new General(this, "shikieiki_god", "touhougod", 4);
    shikieiki_god->addSkill(new Quanjie);
    shikieiki_god->addSkill(new Duanzui);

    General *meirin_god = new General(this, "meirin_god", "touhougod", 4);
    meirin_god->addSkill(new Huaxiang);
    meirin_god->addSkill(new Caiyu);
    meirin_god->addSkill(new Xuanlan);

    General *eirin_god = new General(this, "eirin_god", "touhougod", 4);
    eirin_god->addSkill(new Qiannian);
    eirin_god->addSkill(new QiannianMax);
    related_skills.insertMulti("qiannian", "#qiannian_max");

    General *kanako_god = new General(this, "kanako_god", "touhougod", 4);
    kanako_god->addSkill(new Qinlue);
    kanako_god->addSkill(new QinlueEffect);
    related_skills.insertMulti("qinlue", "#qinlue_effect");

    General *byakuren_god = new General(this, "byakuren_god", "touhougod", 4);
    byakuren_god->addSkill(new Chaoren);

    General *koishi_god = new General(this, "koishi_god", "touhougod", 3);
    koishi_god->addSkill(new Renge);
    koishi_god->addRelateSkill("ziwo");
    koishi_god->addRelateSkill("benwo");
    koishi_god->addRelateSkill("chaowo");

    General *suwako_god = new General(this, "suwako_god", "touhougod", 5);
    suwako_god->addSkill(new Zuosui);
    suwako_god->addSkill(new Worao);
    suwako_god->addSkill(new Shenhua);

    General *miko_god = new General(this, "miko_god", "touhougod", 4);
    miko_god->addSkill(new Hongfo);
    miko_god->addSkill(new Junwei);
    miko_god->addSkill(new Gaizong);
    miko_god->addRelateSkill("wendao");

    General *kaguya_god = new General(this, "kaguya_god", "touhougod", 4);
    kaguya_god->addSkill(new Shenbao);
    kaguya_god->addSkill(new ShenbaoDistance);
    kaguya_god->addSkill(new ShenbaoHandler);
    kaguya_god->addSkill(new ShenbaoViewHas);
    related_skills.insertMulti("shenbao", "#shenbao_distance");
    related_skills.insertMulti("shenbao", "#shenbao");
    related_skills.insertMulti("shenbao", "#shenbao_viewhas");

    General *komachi_god = new General(this, "komachi_god", "touhougod", 4);
    komachi_god->addSkill(new Yindu);
    komachi_god->addSkill(new Huanming);
    komachi_god->addSkill(new Chuanwu);

    General *yuyuko_god = new General(this, "yuyuko_god", "touhougod", 1);
    yuyuko_god->addSkill(new Fanhun);
    yuyuko_god->addSkill(new Yousi);
    yuyuko_god->addSkill(new YousiUI);
    related_skills.insertMulti("yousi", "#yousi");

    General *satori_god = new General(this, "satori_god", "touhougod", 3);
    satori_god->addSkill(new Kuixin);
    satori_god->addSkill(new Xinhua);

    General *aya_god = new General(this, "aya_god", "touhougod", 4);
    aya_god->addSkill(new Tianqu);
    aya_god->addSkill(new Fengmi);

    General *seiga_god = new General(this, "seiga_god", "touhougod", 3);
    seiga_god->addSkill(new Huanhun);
    seiga_god->addSkill(new Tongling);
    seiga_god->addSkill(new Rumo);

    General *nue_god = new General(this, "nue_god", "touhougod", 3);
    nue_god->addSkill(new Anyun);
    nue_god->addSkill(new AnyunShowStaticSkill);
    nue_god->addSkill(new AnyunProhibit);
    nue_god->addSkill(new Benzun);
    related_skills.insertMulti("anyun", "#anyunShowStatic");
    related_skills.insertMulti("anyun", "#anyun_prohibit");

    General *marisa_god = new General(this, "marisa_god", "touhougod", 4);
    marisa_god->addSkill(new Chongneng);
    marisa_god->addSkill(new Huixing);
    marisa_god->addSkill(new HuixingTargetMod);
    related_skills.insertMulti("huixing", "#huixing_effect");

    General *patchouli_god = new General(this, "patchouli_god", "touhougod", 3);
    patchouli_god->addSkill(new Xianshi);
    patchouli_god->addSkill(new Riyue);

    General *alice_god = new General(this, "alice_god", "touhougod", 4);
    alice_god->addSkill(new Wenyue);
    alice_god->addSkill(new Qianqiang);
    alice_god->addSkill(new Xianji);

    General *yuka_god = new General(this, "yuka_god", "touhougod", 4);
    yuka_god->addSkill(new Xiuye);
    yuka_god->addSkill(new Kuangji);
    yuka_god->addSkill(new KuangjiTargetMod);
    related_skills.insertMulti("kuangji", "#kuangji_effect");

    General *tenshi_god = new General(this, "tenshi_god", "touhougod", 4);
    tenshi_god->addSkill(new Qizhi);
    tenshi_god->addSkill(new Tiandao);

    General *tenshi_god_sp = new General(this, "tenshi_god_sp", "touhougod", 4);
    tenshi_god_sp->addSkill(new QizhiAlernative);
    tenshi_god_sp->addSkill(new Tianbian);
    tenshi_god_sp->addSkill(new TianbianDistance);
    related_skills.insertMulti("tianbian", "#tianbian-dist");

    //    General *shinmyoumaru_god = new General(this, "shinmyoumaru_god", "touhougod", 4, false, true, true);
    //    Q_UNUSED(shinmyoumaru_god);
    //    General *tenshi_god = new General(this, "tenshi_god", "touhougod", 3, false, true, true);
    //    Q_UNUSED(tenshi_god);
    //    General *uuz13 = new General(this, "yuyuko_1v3", "touhougod", 1, false, true, true);
    //    Q_UNUSED(uuz13);
    //    General *kaguya13 = new General(this, "kaguya_1v3", "touhougod", 1, false, true, true);
    //    Q_UNUSED(kaguya13);

    addMetaObject<HongwuCard>();
    addMetaObject<ShenqiangCard>();
    addMetaObject<HuimieCard>();
    addMetaObject<ShenshouCard>();
    addMetaObject<FengyinCard>();
    addMetaObject<HuaxiangCard>();
    addMetaObject<ChaowoCard>();
    addMetaObject<ShowShenbaoCard>();
    addMetaObject<WendaoCard>();
    addMetaObject<XinhuaCard>();
    addMetaObject<RumoCard>();
    addMetaObject<XianshiCard>();
    addMetaObject<WenyueCard>();
    addMetaObject<QianqiangCard>();
    addMetaObject<KuangjiCard>();
    addMetaObject<QizhiCard>();
    skills << new ChaorenLog << new Wendao << new RoleShownHandler << new ShenbaoAttach << new Ziwo << new Benwo << new Chaowo << new TiandaoDistance << new QizhiVS;
}

ADD_PACKAGE(TouhouGod)
