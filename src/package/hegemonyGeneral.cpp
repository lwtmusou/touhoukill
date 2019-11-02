#include "hegemonyGeneral.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "maneuvering.h"
#include "skill.h"
#include "standard.h"
#include "th06.h"
#include "hegemonyCard.h"
//#include "th08.h"


class GameRule_AskForGeneralShowHead : public TriggerSkill
{
public:
    GameRule_AskForGeneralShowHead()
        : TriggerSkill("GameRule_AskForGeneralShowHead")
    {
        events << EventPhaseStart;
        global = true;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->showGeneral(true, true);
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player != NULL && player->getPhase() == Player::Start && !player->hasShownGeneral() && player->disableShow(true).isEmpty())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        return QList<SkillInvokeDetail>();
    }
};

class GameRule_AskForGeneralShowDeputy : public TriggerSkill
{
public:
    GameRule_AskForGeneralShowDeputy()
        : TriggerSkill("GameRule_AskForGeneralShowDeputy")
    {
        events << EventPhaseStart;
        global = true;
    }

    bool effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->showGeneral(false, true);
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (ServerInfo.Enable2ndGeneral && player != NULL && player->getPhase() == Player::Start && !player->hasShownGeneral2() && player->disableShow(false).isEmpty())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
        return QList<SkillInvokeDetail>();
    }
};

class GameRule_AskForArraySummon : public TriggerSkill
{
public:
    GameRule_AskForArraySummon() : TriggerSkill("GameRule_AskForArraySummon")
    {
        events << EventPhaseStart;
        global = true;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        invoke->invoker->gainMark("@nima");
        foreach(const Skill *skill, invoke->invoker->getVisibleSkillList()) {
            if (!skill->inherits("BattleArraySkill")) continue;
            const BattleArraySkill *baskill = qobject_cast<const BattleArraySkill *>(skill);
            if (!invoke->invoker->askForSkillInvoke(objectName())) return false;
            invoke->invoker->gainMark("@dandan_" + skill->objectName());
            invoke->invoker->showGeneral(invoke->invoker->inHeadSkills(skill->objectName()));
            baskill->summonFriends(invoke->invoker);
            break;
        }
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *room, const QVariant &data) const
    {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player == NULL || player->getPhase() != Player::Start || room->getAlivePlayers().length() < 4)
            return QList<SkillInvokeDetail>();

        foreach(const Skill *skill, player->getVisibleSkillList()) {
            if (!skill->inherits("BattleArraySkill")) continue;
            if (qobject_cast<const BattleArraySkill *>(skill)->getViewAsSkill()->isEnabledAtPlay(player)) {
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            }
                
            
        }
        return QList<SkillInvokeDetail>();
    }
};


NiaoxiangSummon::NiaoxiangSummon()
    : ArraySummonCard("niaoxiang")
{
}

class Niaoxiang : public BattleArraySkill
{
public:
    Niaoxiang() : BattleArraySkill("niaoxiang", "Siege")
    {
        events << TargetSpecified;
        //array_type = "Seige";
    }

    virtual bool canPreshow() const
    {
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *room, const QVariant &data) const
    {

        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card == NULL || !use.card->isKindOf("Slash"))
            return QList<SkillInvokeDetail>();

        QList<ServerPlayer *> skill_owners = room->findPlayersBySkillName(objectName());
        QList<SkillInvokeDetail> d;
        foreach(ServerPlayer *skill_owner, skill_owners) {
            if ( skill_owner->hasShownSkill(this)) {//!BattleArraySkill::triggerable(event, room, data).isEmpty() &&
                QList<ServerPlayer *> targets;
                foreach(ServerPlayer *to, use.to) {
                    if (use.from->inSiegeRelation(skill_owner, to))
                        targets << to;  //->objectName();
                }

                if (!targets.isEmpty())
                    d << SkillInvokeDetail(this, skill_owner, use.from, targets, true);
                //skill_list.insert(skill_owner, QStringList(objectName() + "->" + targets.join("+")));
            }
        }
        return d;
    }

    //virtual bool cost(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &, ServerPlayer *ask_who) const  
    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (invoke->owner != NULL && invoke->owner->hasShownSkill(this)) {
            foreach (ServerPlayer *skill_target, invoke->targets)
                room->doBattleArrayAnimate(invoke->owner, skill_target);
            //room->broadcastSkillInvoke(objectName(), invoke->owner);
            return true;
        }
        return false;
    }

    //virtual bool effect(TriggerEvent, Room *room, ServerPlayer *skill_target, QVariant &data, ServerPlayer *ask_who) const
    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        //room->sendCompulsoryTriggerLog(ask_who, objectName(), true);
        //CardUseStruct use = data.value<CardUseStruct>();
        foreach(ServerPlayer *skill_target, invoke->targets)
            room->loseHp(skill_target);
        /*
        int x = use.to.indexOf(skill_target);
        QVariantList jink_list = use.from->tag["Jink_" + use.card->toString()].toList();
        if (jink_list.at(x).toInt() == 1)
            jink_list[x] = 2;
        use.from->tag["Jink_" + use.card->toString()] = jink_list;
        */
        return false;
    }
};


/*
TuizhiHegemonyCard::TuizhiHegemonyCard()
{
    m_skillName = "tuizhi_hegemony";
}

bool TuizhiHegemonyCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    //return targets.isEmpty();
    return to_select->hasShownOneGeneral() && targets.isEmpty();
}

void TuizhiHegemonyCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    ServerPlayer *target = effect.to;
    QStringList select;
    if (target->hasShownGeneral())
        select << "head";
    if (target->getGeneral2() && target->hasShownGeneral2())
        select << "deputy";
    if (select.isEmpty())
        return;

    QString choice = room->askForChoice(target, objectName(), select.join("+"));
    bool ishead = (choice == "head");
    target->hideGeneral(ishead); //(ishead, true);

    //QString flag = (choice == "head") ? "h" : "d";
    //room->setPlayerDisableShow(target, flag, "huoshui");
}
*/
/*
class TuizhiHegemony : public ZeroCardViewAsSkill
{
public:
    TuizhiHegemony()
        : ZeroCardViewAsSkill("tuizhi_hegemony")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return true;//!player->hasUsed("BanyueHegemonyCard");
    }

    virtual const Card *viewAs() const
    {
        return new TuizhiHegemonyCard;
    }
};*/

class TuizhiHegemony : public TriggerSkill
{
public:
    TuizhiHegemony()
        : TriggerSkill("tuizhi_hegemony")
    {
        events << CardUsed << CardResponded;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *room, const QVariant &data) const
    {
        if (event == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->getSuit() == Card::Heart && use.card->getTypeId() != Card::TypeSkill) {
                if (use.from && use.from->hasSkill(this)) {
                    foreach(ServerPlayer *p, room->getOtherPlayers(use.from)) {
                        if (p->hasShownOneGeneral())
                            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
                    }
                }
            }
        }
        else if (event == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse && resp.m_card && resp.m_card->getSuit() == Card::Heart) {
                if (resp.m_from && resp.m_from->hasSkill(this)) {
                    foreach(ServerPlayer *p, room->getOtherPlayers(resp.m_from)) {
                        if (p->hasShownOneGeneral())
                            return QList<SkillInvokeDetail>()  << SkillInvokeDetail(this, resp.m_from, resp.m_from);
                    }
                }
            }
        }


        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getOtherPlayers(invoke->invoker)) {
            if (p->hasShownOneGeneral())
                targets << p;
        }

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@tuizhi", true, true);
        if (target)
            invoke->targets << target;
        return target != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *target = invoke->targets.first();
        QStringList select;
        if (target->hasShownGeneral())
            select << "head";
        if (target->getGeneral2() && target->hasShownGeneral2())
            select << "deputy";
        if (select.isEmpty())
            return false;

        QString choice = room->askForChoice(invoke->invoker, objectName(), select.join("+"));
        bool ishead = (choice == "head");
        target->hideGeneral(ishead);
        return false;
    }
};


class TongjieHegemony : public TriggerSkill
{
public:
    TongjieHegemony()
        : TriggerSkill("tongjie_hegemony")
    {
        events << GeneralShown << GeneralHidden << GeneralRemoved << EventPhaseStart << Death << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    void doTongjie(Room *room, ServerPlayer *reimu, bool set) const
    {
        if (set && !reimu->tag["tongjie"].toBool()) {
            foreach(ServerPlayer *p, room->getOtherPlayers(reimu))
                room->setPlayerDisableShow(p, "hd", "tongjie");

            reimu->tag["tongjie"] = true;
        }
        else if (!set && reimu->tag["tongjie"].toBool()) {
            foreach(ServerPlayer *p, room->getOtherPlayers(reimu))
                room->removePlayerDisableShow(p, "tongjie");

            reimu->tag["tongjie"] = false;
        }
    }


    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        ServerPlayer *player;
        if (triggerEvent != Death){
            if (triggerEvent == EventAcquireSkill || triggerEvent == EventLoseSkill) {
                SkillAcquireDetachStruct s = data.value<SkillAcquireDetachStruct>();
                player = s.player;
            }   
            else if (triggerEvent == GeneralRemoved || triggerEvent == GeneralHidden || triggerEvent == GeneralShown) {
                ShowGeneralStruct s = data.value<ShowGeneralStruct>();
                player = s.player;
            }
            else {
                player = data.value<ServerPlayer *>();
            }
            
            if (player == NULL || !player->isAlive())
                return;
        }
            
        ServerPlayer *c = room->getCurrent();      
        if (c == NULL || (triggerEvent != EventPhaseStart && c->getPhase() == Player::NotActive) || c != player)
            return;

        if ((triggerEvent == GeneralShown || triggerEvent == EventPhaseStart || triggerEvent == EventAcquireSkill) && !player->hasShownSkill(this))
            return;
        if ((triggerEvent == GeneralShown || triggerEvent == GeneralHidden)) {
            ShowGeneralStruct s = data.value<ShowGeneralStruct>();
            if (!s.player->ownSkill(this))
                return;
            if( s.player->inHeadSkills(this->objectName()) != s.isHead)
                return;
        }

        if (triggerEvent == GeneralRemoved) {
            ShowGeneralStruct s = data.value<ShowGeneralStruct>();

            bool removeReimu = false;
            QStringList generals = room->getTag(s.player->objectName()).toStringList();
            if (s.isHead) {
                if (generals.first() == "reimu_hegemony")
                    removeReimu = true;
            }
            else {
                if (generals.last() == "reimu_hegemony")
                    removeReimu = true;
            }
            if (!removeReimu)
                return;
        }
        if (triggerEvent == EventPhaseStart) {
            player = data.value<ServerPlayer *>();
            if(!(player->getPhase() == Player::RoundStart || player->getPhase() == Player::NotActive))
                return;
        }
        if (triggerEvent == Death) {
            DeathStruct d = data.value<DeathStruct>();
            player = d.who;
            if (!player->hasShownSkill(this))
                return;
        }

        if ((triggerEvent == EventAcquireSkill || triggerEvent == EventLoseSkill)) {
            SkillAcquireDetachStruct s = data.value<SkillAcquireDetachStruct>();
            if (s.skill->objectName() != objectName())
                return;
        }

        bool set = false;
        if (triggerEvent == GeneralShown || triggerEvent == EventAcquireSkill || (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart))
            set = true;

        if (player)
            doTongjie(room, player, set);

    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *room, const QVariant &data) const
    {
        if (event == GeneralShown) {
            ShowGeneralStruct s = data.value<ShowGeneralStruct>();
            ServerPlayer *target = s.player;
            if (target && target->isAlive()) {
                QList<SkillInvokeDetail> d;
                foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                    if (p != target)
                        d << SkillInvokeDetail(this, p, p, NULL, true);
                }
                return d;
            }
        }
        return QList<SkillInvokeDetail>();
    }



    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        invoke->invoker->drawCards(1);
        return false;
    }
};


//********  SPRING   **********

class LizhiHegemony : public TriggerSkill
{
public:
    LizhiHegemony()
        : TriggerSkill("lizhi_hegemony")
    {
        events << CardFinished << DamageDone << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.card && damage.card->isKindOf("Slash"))
                room->setCardFlag(damage.card, "lizhiDamage");
        }
        if (triggerEvent == EventPhaseChanging) {
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("lizhi_used"))
                    room->setPlayerFlag(p, "-lizhi_used");
            }
        }

    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *room, const QVariant &data) const
    {
        if (event == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->canDamage() || use.card->hasFlag("lizhiDamage") || !use.from || use.from->isDead()
                || !use.from->hasSkill(this) || use.from->hasFlag("lizhi_used"))
                return QList<SkillInvokeDetail>();

            QList<int> ids;
            if (use.card->isVirtualCard())
                ids = use.card->getSubcards();
            else
                ids << use.card->getEffectiveId();

            if (ids.isEmpty())
                return QList<SkillInvokeDetail>();
            foreach(int id, ids) {
                if (room->getCardPlace(id) != Player::DiscardPile) //place???
                    return QList<SkillInvokeDetail>();
            }

            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getAlivePlayers()) {
            if (invoke->invoker->isFriendWith(p, true))
                targets << p;
        }

        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@lizhi", true, true);
        if (target)
            invoke->targets << target;
        return target != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        room->setPlayerFlag(invoke->invoker, "lizhi_used");
        CardUseStruct use = data.value<CardUseStruct>();
        invoke->targets.first()->obtainCard(use.card);
        return false;
    }
};





class JingxiaHegemony : public MasochismSkill
{
public:
    JingxiaHegemony()
        : MasochismSkill("jingxia_hegemony")
    {
    }

    QList<SkillInvokeDetail> triggerable(const Room *room, const DamageStruct &damage) const
    {
        bool invoke = false;
        do {
            if (damage.to->isAlive() && damage.to->hasSkill(this)) {
                if (damage.from != NULL && damage.to->canDiscard(damage.from, "hes")) {
                    invoke = true;
                    break;
                }
                bool flag = false;
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (damage.to->canDiscard(p, "ej")) {
                        flag = true;
                        break;
                    }
                }
                if (flag) {
                    invoke = true;
                    break;
                }
            }
        } while (false);

        if (invoke) {
            QList<SkillInvokeDetail> d;
            for (int i = 0; i < damage.damage; ++i) {
                d << SkillInvokeDetail(this, damage.to, damage.to);
            }
            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QStringList select;
        if (damage.from != NULL && damage.to->canDiscard(damage.from, "hes"))
            select << "discard";

        QList<ServerPlayer *> fieldcard;
        foreach(ServerPlayer *p, room->getAllPlayers()) {
            if (damage.to->canDiscard(p, "ej")) {
                select << "discardfield";
                break;
            }
        }
        select.prepend("dismiss");

        ServerPlayer *player = damage.to;

        player->tag["jingxia"] = QVariant::fromValue(damage);
        QString choice = room->askForChoice(player, objectName(), select.join("+"), QVariant::fromValue(damage));
        player->tag.remove("jingxia");
        if (choice == "dismiss")
            return false;

        invoke->tag["jingxia"] = choice;
        return true;
    }

    void onDamaged(Room *room, QSharedPointer<SkillInvokeDetail> invoke, const DamageStruct &damage) const
    {
        QList<ServerPlayer *> fieldcard;
        foreach(ServerPlayer *p, room->getAllPlayers()) {
            if (damage.to->canDiscard(p, "ej"))
                fieldcard << p;
        }
        ServerPlayer *player = damage.to;

        QString choice = invoke->tag.value("jingxia").toString();

        room->touhouLogmessage("#InvokeSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        if (choice == "discard") {
            for (int i = 0; i < 2; i++) {
                if (player->isDead() || !player->canDiscard(damage.from, "hes"))
                    return;
                if (damage.from == player)
                    room->askForDiscard(player, "jingxia", 1, 1, false, true);
                else {
                    int card_id = room->askForCardChosen(player, damage.from, "hes", "jingxia", false, Card::MethodDiscard);
                    room->throwCard(card_id, damage.from, player);
                }
            }
        }
        else if (choice == "discardfield") {
            ServerPlayer *player1 = room->askForPlayerChosen(player, fieldcard, "jingxia", "@jingxia-discardfield");
            int card1 = room->askForCardChosen(player, player1, "ej", objectName(), false, Card::MethodDiscard);
            room->throwCard(card1, player1, player);
        }
    }
};


QingtingHegemonyCard::QingtingHegemonyCard()
{
    target_fixed = true;
    m_skillName = "qingting_hegemony";
}

void QingtingHegemonyCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    foreach(ServerPlayer *p, room->getOtherPlayers(source)) {
        if (p->isKongcheng())
            continue;
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), p->objectName());
    }
    foreach(ServerPlayer *p, room->getOtherPlayers(source)) {
        if (p->isKongcheng())
            continue;
        const Card *card;
        if (source->getMark("shengge") > 0 || p->getHandcardNum() == 1)
            card = new DummyCard(QList<int>() << room->askForCardChosen(source, p, "hs", "qingting"));
        else {
            p->tag["qingting_give"] = QVariant::fromValue(source);
            card = room->askForExchange(p, "qingting_give", 1, 1, false, "qingtingGive:" + source->objectName());
            p->tag.remove("qingting_give");
        }
        DELETE_OVER_SCOPE(const Card, card)

            source->obtainCard(card, false);
        room->setPlayerMark(p, "@qingting", 1);
    }

    //get delay
    if (source->isOnline())
        room->getThread()->delay(2000);

    foreach(ServerPlayer *p, room->getOtherPlayers(source)) {
        if (p->getMark("@qingting") == 0)
            continue;
        room->setPlayerMark(p, "@qingting", 0);
        if (source->isKongcheng())
            continue;

        source->tag["qingting_return"] = QVariant::fromValue(p);
        const Card *card = room->askForExchange(source, "qingting", 1, 1, false, "qingtingReturn:" + p->objectName());
        DELETE_OVER_SCOPE(const Card, card)
            source->tag.remove("qingting_return");
        p->obtainCard(card, false);
    }
}


class QingtingHegemony : public ZeroCardViewAsSkill
{
public:
    QingtingHegemony()
        : ZeroCardViewAsSkill("qingting_hegemony")
    {
    }
    static bool checkQingting(const Player *player)
    {
        foreach(const Player *p, player->getAliveSiblings()) {
            if (!p->isKongcheng())
                return true;
        }
        return false;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("QingtingHegemonyCard") && checkQingting(player);
    }

    virtual const Card *viewAs() const
    {
        return new QingtingHegemonyCard;
    }
};


class ShezhengHegemony : public AttackRangeSkill
{
public:
    ShezhengHegemony()
        : AttackRangeSkill("shezheng_hegemony")
    {
        relate_to_place = "deputy";
    }

    bool canPreshow() const
    {
        return true;
    }

    virtual int getExtra(const Player *player, bool) const
    {
        if (player->hasSkill(objectName()) && player->hasShownSkill(objectName()) && !player->getWeapon())
            return 1;
        return 0;
    }
};

ShowShezhengCard::ShowShezhengCard()
    : SkillCard()
{
    mute = true;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

const Card *ShowShezhengCard::validate(CardUseStruct &card_use) const
{
    bool head = card_use.from->inHeadSkills("shezheng_hegemony");
    card_use.from->showGeneral(head);
    return NULL;
}


class ShezhengAttach : public ViewAsSkill
{
public:
    ShezhengAttach()
        : ViewAsSkill("shezheng_attach")
    {
        attached_lord_skill = true;
    }

    virtual bool shouldBeVisible(const Player *Self) const
    {
        return Self;
    }


    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasShownSkill("shezheng_hegemony");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        return new ShowShezhengCard();
    }
};


class ShezhengHegemonyHandler : public TriggerSkill
{
public:
    ShezhengHegemonyHandler()
        : TriggerSkill("#shezheng_hegemony")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Death << Debut << EventSkillInvalidityChange;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == GameStart || triggerEvent == Debut || triggerEvent == EventAcquireSkill) {
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                if ((p->hasSkill("shezheng_hegemony", true) || p->ownSkill("shezheng_hegemony")) && !p->hasSkill("shezheng_attach"))
                    room->attachSkillToPlayer(p, "shezheng_attach");
            }
        }
        if (triggerEvent == Death || triggerEvent == EventLoseSkill) {
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                if ((!p->hasSkill("shezheng_hegemony", true) && !p->ownSkill("shezheng_hegemony")) && p->hasSkill("shezheng_attach"))
                    room->detachSkillFromPlayer(p, "shezheng_attach", true);
            }
        }
    }
};



class ShezhengViewHas : public ViewHasSkill

{

public:

    ShezhengViewHas() : ViewHasSkill("#shezheng_viewhas")

    {
        
    }

    virtual bool ViewHas(const Player *player, const QString &skill_name, const QString &flag, bool ignore_preshow) const
    {
        if (flag == "weapon" && skill_name == "DoubleSwordHegemony" && player->isAlive() && player->hasSkill("shezheng_hegemony") && !player->getWeapon())         
            return true;

        return false;

    }

};


class ChilingHegemony : public TriggerSkill
{
public:
    ChilingHegemony()
        : TriggerSkill("chiling_hegemony")
    {
        events << CardsMoveOneTime;
        relate_to_place = "head";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *miko = qobject_cast<ServerPlayer *>(move.from);
        if (miko != NULL && miko->isAlive() && miko->hasSkill(objectName()) //&& move.from_places.contains(Player::PlaceHand)
            && (move.to_place == Player::PlaceHand && move.to && move.to != miko))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, miko, miko);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *target = qobject_cast<ServerPlayer *>(move.to);

        room->askForUseCard(target, IntList2StringList(move.card_ids).join("#"), "@chiling:" + invoke->invoker->objectName(), -1, Card::MethodUse, false);
        return false;
    }
};



class FenleiHegemony : public TriggerSkill
{
public:
    FenleiHegemony()
        : TriggerSkill("fenlei_hegemony")
    {
        events << QuitDying;
    }



    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        ServerPlayer *who = data.value<DyingStruct>().who;
        if (who->isAlive() && who->hasSkill(this))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, who, who);
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *tokizo = invoke->invoker;
        ServerPlayer *target = room->askForPlayerChosen(tokizo, room->getOtherPlayers(tokizo), objectName(), "@fenlei", true, true);
        if (target)
            invoke->targets << target;
        return target != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>invoke, QVariant &) const
    {

        room->damage(DamageStruct(objectName(), NULL, invoke->targets.first(), 1, DamageStruct::Thunder));
        return false;
    }
};


//********  SUMMER   **********


class SkltKexueHegemony : public TriggerSkill
{
public:
    SkltKexueHegemony()
        : TriggerSkill("skltkexue_hegemony")
    {
        events  << EventAcquireSkill << EventLoseSkill << Death << Debut << Revive << GeneralShown << Dying; //<< GameStart
    }

    void record(TriggerEvent e, Room *room, QVariant &) const
    {
        if (e == Dying)
            return;

        static QString attachName = "skltkexue_attach"; // need rewrite vs skill
        QList<ServerPlayer *> sklts;
        foreach(ServerPlayer *p, room->getAllPlayers()) {
            if (p->hasSkill(this, true) && p->hasShownSkill(this))
                sklts << p;
        }

        if (sklts.length() > 1) {
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                if (!p->hasSkill(attachName, true))
                    room->attachSkillToPlayer(p, attachName);
            }
        }
        else if (sklts.length() == 1) {
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(this, true) && p->hasSkill(attachName, true))
                    room->detachSkillFromPlayer(p, attachName, true);
                else if (!p->hasSkill(this, true) && !p->hasSkill(attachName, true))
                    room->attachSkillToPlayer(p, attachName);
            }
        }
        else { // the case that sklts is empty
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasSkill(attachName, true))
                    room->detachSkillFromPlayer(p, attachName, true);
            }
        }
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == Dying) {
            DyingStruct dying = data.value<DyingStruct>();
            if (dying.who && dying.who->isAlive() && dying.who->hasSkill(this) && !dying.who->hasShownSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dying.who, dying.who);
            return QList<SkillInvokeDetail>();
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (invoke->invoker->askForSkillInvoke(this, data))
            invoke->invoker->showHiddenSkill(objectName());
        //invoke->invoker->showGeneral(invoke->invoker->inHeadSkills(objectName()))
        return false;
    }
};



class BolanHgemony : public TriggerSkill
{
public:
    BolanHgemony()
        : TriggerSkill("bolan_hegemony")
    {
        events << CardUsed << EventPhaseChanging << TargetConfirmed;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach(ServerPlayer *p, room->getAllPlayers())
                p->setFlags("-" + objectName());
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging)
            return QList<SkillInvokeDetail>();

        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (!use.card->isNDTrick())
            return d;

        QList<ServerPlayer *> owners =  room->findPlayersBySkillName(objectName());
        foreach(ServerPlayer *p, owners) {
            if (!p->hasFlag(objectName())) {
                if (triggerEvent == CardUsed &&  use.from && p->isFriendWith(use.from, true)) {//use.from->hasShownOneGeneral() && 
                    d << SkillInvokeDetail(this, p, p);
                    continue;
                }
                if (triggerEvent == TargetConfirmed) {
                    foreach(ServerPlayer *to, use.to) {
                        if (p->isFriendWith(to, true)) {
                            d << SkillInvokeDetail(this, p, p);
                            break;

                        }
                    }
                }
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<int> list = room->getNCards(2);
        ServerPlayer *player = invoke->invoker;
        player->setFlags(objectName());

        CardsMoveStruct move(list, NULL, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, invoke->invoker->objectName(), objectName(), QString()));
        room->moveCardsAtomic(move, true);

        QList<int> able;
        QList<int> disabled;
        foreach(int id, list) {
            Card *tmp_card = Sanguosha->getCard(id);
            if (tmp_card->isKindOf("TrickCard") || use.card->getSuit() == tmp_card->getSuit())
                able << id;
            else
                disabled << id;
        }

        if (!able.isEmpty()) {
            DummyCard dummy(able);
            room->obtainCard(player, &dummy);
        }
        if (!disabled.isEmpty()) {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, invoke->invoker->objectName(), objectName(), QString());
            DummyCard dummy(disabled);
            room->throwCard(&dummy, reason, NULL);
        }

        return false;
    }
};





class BeishuiHegemonyVS : public ViewAsSkill
{
public:
    BeishuiHegemonyVS()
        : ViewAsSkill("beishui_hegemony")
    {
        response_or_use = true;
    }

    static QStringList responsePatterns()
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();

        Card::HandlingMethod method = Card::MethodUse;
        QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
        const Skill *skill = Sanguosha->getSkill("beishui_hegemony");

        QStringList checkedPatterns;
        foreach(const Card *card, cards) {
            if ((card->isKindOf("BasicCard")) && !ServerInfo.Extensions.contains("!" + card->getPackage())) {
                QString name = card->objectName();
                if (!checkedPatterns.contains(name) && skill->matchAvaliablePattern(name, pattern) && !Self->isCardLimited(card, method))
                    checkedPatterns << name;
            }
        }

        return checkedPatterns;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark("beishui") > 0)
            return false;
        if (Slash::IsAvailable(player) || Analeptic::IsAvailable(player))
            return true;
        Card *card = Sanguosha->cloneCard("peach", Card::NoSuit, 0);
        DELETE_OVER_SCOPE(Card, card)
            Card *card1 = Sanguosha->cloneCard("super_peach", Card::NoSuit, 0);
        DELETE_OVER_SCOPE(Card, card1)
            return card->isAvailable(player) || card1->isAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
            return false;
        if (player->getMark("beishui") > 0)
            return false;

        //check main phase
        if (player->isCurrent()) {
            if (!player->isInMainPhase())
                return false;
        }
        else {
            foreach(const Player *p, player->getSiblings()) {
                if (p->isCurrent()) {
                    if (!p->isInMainPhase())
                        return false;
                    break;
                }
            }
        }

        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.contains("peach") && checkedPatterns.length() == 1 && player->getMark("Global_PreventPeach") > 0)
            return false;

        return !checkedPatterns.isEmpty();
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *) const
    {
        //int num = qMax(1, Self->getHp());
        int roles = 1;
        if (Self->getRole() != "careerist") {
            foreach(const Player *p, Self->getAliveSiblings()) {
                if (Self->isFriendWith(p))//p->getRole() == Self->getRole()
                    roles++;
            }
        }
        int num = qMax(roles, Self->getHp());
        return selected.length() < num;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        //int num = qMax(1, Self->getHp());
        int roles = 1;
        if (Self->getRole() != "careerist") {
            foreach(const Player *p, Self->getAliveSiblings()) {
                if (Self->isFriendWith(p)) //p->getRole() == Self->getRole()
                    roles++;
            }
        }

        int num = qMax(roles, Self->getHp());
        if (cards.length() != num)
            return NULL;

        QString name = Self->tag.value("beishui_hegemony", QString()).toString();
        QStringList checkedPatterns = responsePatterns();
        if (checkedPatterns.length() == 1)
            name = checkedPatterns.first();
        if (name != NULL) {
            Card *card = Sanguosha->cloneCard(name);
            card->setSkillName(objectName());
            card->addSubcards(cards);
            return card;
        }
        else
            return NULL;
    }
};


class BeishuiHegemony : public TriggerSkill
{
public:
    BeishuiHegemony()
        : TriggerSkill("beishui_hegemony")
    {
        events << EventPhaseChanging << PreCardUsed << CardResponded;
        view_as_skill = new BeishuiHegemonyVS;
    }

    virtual QDialog *getDialog() const
    {
        return BeishuiDialog::getInstance("beishui_hegemony", true, false);
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        if (e == EventPhaseChanging) {
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getMark("beishui") > 0)
                    room->setPlayerMark(p, "beishui", 0);
            }
        }
        //record for ai, since AI prefer use a specific card,  but not the SkillCard QijiCard.
        if (e == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() == objectName())
                room->setPlayerMark(use.from, "beishui", 1);
        }
        if (e == CardResponded) {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_from && response.m_isUse && !response.m_isProvision && response.m_card && response.m_card->getSkillName() == objectName())
                room->setPlayerMark(response.m_from, "beishui", 1);
        }
    }
};



class MoqiHgemony : public TriggerSkill
{
public:
    MoqiHgemony()
        : TriggerSkill("moqi_hegemony")
    {
        events << CardUsed << EventPhaseChanging;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &) const
    {
        if (triggerEvent == EventPhaseChanging) {
            foreach(ServerPlayer *p, room->getAllPlayers())
                p->setFlags("-" + objectName());
        }
    }

    static bool can_add(CardUseStruct use)
    {
        return use.card->isNDTrick()
            && !(use.card->isKindOf("IronChain") || use.card->isKindOf("LureTiger") || use.card->isKindOf("Nullification"));
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging)
            return QList<SkillInvokeDetail>();

        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (!can_add(use))
            return d;

        QList<ServerPlayer *> owners = room->findPlayersBySkillName(objectName());
        foreach(ServerPlayer *p, owners) {
            if (!p->hasFlag(objectName())) {
                if (use.from && p->isFriendWith(use.from, true)) {
                    d << SkillInvokeDetail(this, p, p);
                }
            }
        }
        return d;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        //add log?
        CardUseStruct use = data.value<CardUseStruct>();
        room->setCardFlag(use.card, "mopao");
        room->setPlayerFlag(invoke->invoker, objectName());
        return false;
    }
};







class XuyuHegemony : public TriggerSkill
{
public:
    XuyuHegemony()
        : TriggerSkill("xuyu_hegemony")
    {
        events << CardsMoveOneTime;
        frequency = Compulsory;
        relate_to_place = "head";
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            ServerPlayer *kaguya = qobject_cast<ServerPlayer *>(move.from);

            if (kaguya && kaguya->isAlive() && kaguya->hasSkill(this) && !kaguya->hasFlag("xuyu_invoked") 
                &&  move.from_places.contains(Player::PlaceHand) && kaguya->isKongcheng())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, kaguya, kaguya, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        room->setPlayerFlag(invoke->invoker, "xuyu_invoked");
        invoke->invoker->removeGeneral(!invoke->invoker->inHeadSkills(objectName()));
        
        // add this to Player::skills, not Player::acquired_skills.
        bool head = invoke->invoker->inHeadSkills(objectName());
        invoke->invoker->addSkill("yongheng", head);
        invoke->invoker->sendSkillsToOthers(head);

        const Skill *skill = Sanguosha->getSkill("yongheng");
        const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
        room->getThread()->addTriggerSkill(trigger_skill);

        QString flag = (head) ? "h" : "d";
        invoke->invoker->setSkillsPreshowed(flag, true);
        invoke->invoker->notifyPreshow();
        //room->setPlayerProperty(invoke->invoker, "general_showed", true);


        //QString skillname = invoke->invoker->inHeadSkills(objectName()) ? "yongheng" : "yongheng!";
        //room->handleAcquireDetachSkills(invoke->invoker, skillname);
        //room->acquireSkill(invoke->invoker, skillname);


        return false;
    }
};


class YaoshiHegemony : public TriggerSkill
{
public:
    YaoshiHegemony()
    : TriggerSkill("yaoshi_hegemony")
    {
    events << DamageInflicted;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.card  &&  damage.from->isAlive() && damage.from->hasSkill(this)) //&& damage.card->isKindOf("Slash")
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, false, damage.to);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->touhouLogmessage("#yaoshi_log", invoke->targets.first(), objectName(), QList<ServerPlayer *>(), QString::number(damage.damage));

        RecoverStruct recover;
        room->recover(invoke->targets.first(), recover);

        return true;
    }
};



class FengxiangHegemony : public OneCardViewAsSkill
{
public:
    FengxiangHegemony()
        : OneCardViewAsSkill("fengxiang_hegemony")
    {
        filter_pattern = ".|red|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return matchAvaliablePattern("fire_attack", pattern) && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard) {
            FireAttack *card = new FireAttack(Card::SuitToBeDecided, -1);
            card->addSubcard(originalCard);
            card->setSkillName(objectName());
            return card;
        }
        return NULL;
    }
};

class KaifengHegemony : public TriggerSkill
{
public:
    KaifengHegemony()
        : TriggerSkill("kaifeng_hegemony")
    {
        events << Damaged << DamageCaused;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Fire || !damage.from || damage.from == damage.to)
            return QList<SkillInvokeDetail>();
        if (triggerEvent == DamageCaused) {
            if (damage.from->hasSkill(this) && damage.from->getHp() < damage.to->getHp() && !damage.from->isDead())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, false, damage.to);
        }
        else if (triggerEvent == Damaged) {
            if (damage.to->hasSkill(this) && damage.to->getHp() < damage.from->getHp() && !damage.to->isDead())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, false, damage.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *mokou = invoke->invoker;
        ServerPlayer *target = invoke->targets.first();
        RecoverStruct recover;
        recover.recover = 1;
        room->recover(mokou, recover);
        return false;
    }
};



class XushiHegemony : public TriggerSkill
{
public:
    XushiHegemony()
        : TriggerSkill("xushi_hegemony")
    {
        events << TargetConfirming;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeSkill || use.to.length() < 2)
            return QList<SkillInvokeDetail>();

        QList<SkillInvokeDetail> d;
        foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName()))
            d << SkillInvokeDetail(this, p, p);
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QString prompt = "@xushi_hegemony_targetchosen:" + use.card->objectName();
        invoke->invoker->tag["xushi_hegemony_use"] = data;
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, use.to, objectName(), prompt, true, true);
        if (target)
            invoke->targets << target;
        return target != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();

        ServerPlayer *target = invoke->targets.first();
        use.to.removeAll(target);
        data = QVariant::fromValue(use);

        LogMessage log;
        log.type = "#XushiHegemonySkillAvoid";
        log.from = target;
        log.arg = objectName();
        log.arg2 = use.card->objectName();
        room->sendLog(log);
        return false;
    }
};


class XinyueHegemony : public TriggerSkill
{
public:
    XinyueHegemony()
        : TriggerSkill("xinyue_hegemony")
    {
        events << Damaged << EventPhaseChanging;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive() && damage.to->isAlive() && damage.to->hasSkill(this)) {// && damage.from->getHandcardNum() > damage.to->getHp()
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, false, damage.from);
            }
        }
        else if (e == EventPhaseChanging) {
            QList<SkillInvokeDetail> d;
            //ServerPlayer *player = data.value<ServerPlayer *>();
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {  //check current is dead
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("xinyue_transform")) {
                        d << SkillInvokeDetail(this, NULL, p, NULL, true);
                    }
                        
                }
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent e, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            QString prompt = "target:" + damage.from->objectName() + "::" + QString::number(invoke->invoker->getHp());
            return invoke->invoker->askForSkillInvoke(objectName(), data, prompt);
        }
        return true;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (e == Damaged) {
            if (invoke->invoker->hasShownGeneral() && invoke->invoker->getGeneralName() == "keine_hegemony")
                room->setPlayerFlag(invoke->invoker, "xinyue_transform");
            else if (invoke->invoker->getGeneral2() && invoke->invoker->hasShownGeneral2() && invoke->invoker->getGeneral2Name() == "keine_hegemony")
                room->setPlayerFlag(invoke->invoker, "xinyue_transform");
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
            int x = invoke->targets.first()->getHandcardNum() - invoke->invoker->getHp();
            if (x <= 0)
                return false;
            room->askForDiscard(invoke->targets.first(), objectName(), x, x, false, false);
        }
        else {
            room->setPlayerFlag(invoke->invoker, "-xinyue_transform");
            QStringList generals = room->getTag(invoke->invoker->objectName()).toStringList();
            QString old_general = "keine_hegemony";
            QString new_general = "keine_sp_hegemony";
            bool head = (generals.first() == old_general);
            room->transformGeneral(invoke->invoker, new_general, head);
        }
        return false;
    }
};


class WangyueHegemony : public TriggerSkill
{
public:
    WangyueHegemony()
        : TriggerSkill("wangyue_hegemony")
    {
        events << Damaged << EventPhaseChanging;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive() && damage.to->isAlive() && damage.to->hasSkill(this)) { 
                // && damage.from->getHandcardNum() > damage.to->getHandcardNum() && damage.to->getHandcardNum() < 5
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, false, damage.from);
            }
        }
        else if (e == EventPhaseChanging)
        {
            QList<SkillInvokeDetail> d;
            //ServerPlayer *player = data.value<ServerPlayer *>();
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {  //check current is dead
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("wangyue_transform"))
                        d << SkillInvokeDetail(this, NULL, p, NULL, true);
                }
            }
            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent e, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            int num = qMin(5, damage.from->getHandcardNum());
            QString prompt = "target:" + damage.from->objectName() + "::" + QString::number(num);
            return invoke->invoker->askForSkillInvoke(objectName(), data, prompt);
        }
        return true;
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        if (e == Damaged) {
            if (invoke->invoker->hasShownGeneral() && invoke->invoker->getGeneralName() == "keine_sp_hegemony")
                room->setPlayerFlag(invoke->invoker, "wangyue_transform");
            else if (invoke->invoker->getGeneral2() && invoke->invoker->hasShownGeneral2() && invoke->invoker->getGeneral2Name() == "keine_sp_hegemony")
                room->setPlayerFlag(invoke->invoker, "wangyue_transform");

            
            int num = qMin(5, invoke->targets.first()->getHandcardNum());
            if (num <= 0)
                return false;
            int x = num - invoke->invoker->getHandcardNum();
            invoke->invoker->drawCards(x);
        }
        else {
            room->setPlayerFlag(invoke->invoker, "-wangyue_transform");
            QStringList generals = room->getTag(invoke->invoker->objectName()).toStringList();
            QString old_general = "keine_sp_hegemony";
            QString new_general = "keine_hegemony";
            bool head = (generals.first() == old_general);
            room->transformGeneral(invoke->invoker, new_general, head);
        }
        return false;
    }
};





XingyunHegemonyCard::XingyunHegemonyCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
    m_skillName = "xingyun_hegemony";
}

void XingyunHegemonyCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    foreach(int id, subcards)
        room->showCard(source, id);
}

class XingyunHegemonyVS : public OneCardViewAsSkill
{
public:
    XingyunHegemonyVS()
        : OneCardViewAsSkill("xingyun_hegemony")
    {
        response_pattern = "@@xingyun_hegemony";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return  selected.isEmpty() && to_select->hasFlag("xingyun");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard) {
            XingyunHegemonyCard *card = new XingyunHegemonyCard;
            card->addSubcard(originalCard);
            return card;
        }
        else
            return NULL;
    }
};


class XingyungHegemony : public TriggerSkill
{
public:
    XingyungHegemony()
        : TriggerSkill("xingyun_hegemony")
    {
        events << CardsMoveOneTime;
        view_as_skill = new XingyunHegemonyVS;
    }

    bool canPreshow() const
    {
        return true;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        if (room->getTag("FirstRound").toBool())
            return QList<SkillInvokeDetail>();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *tewi = qobject_cast<ServerPlayer *>(move.to);
        if (tewi != NULL && tewi->hasSkill(this) && move.to_place == Player::PlaceHand) {
            foreach(int id, move.card_ids) {
                if (Sanguosha->getCard(id)->getSuit() == Card::Heart && room->getCardPlace(id) == Player::PlaceHand) {
                    ServerPlayer *owner = room->getCardOwner(id);
                    if (owner && owner == tewi)
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, tewi, tewi);
                }
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *tewi = invoke->invoker;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        foreach(int id, move.card_ids) {
            if (Sanguosha->getCard(id)->getSuit() == Card::Heart && room->getCardPlace(id) == Player::PlaceHand) {
                ServerPlayer *owner = room->getCardOwner(id);
                if (owner && owner == tewi)
                    room->setCardFlag(id, "xingyun");
            }
        }
        invoke->invoker->tag["xingyun_move"] = data;
        const Card *c = room->askForUseCard(tewi, "@@xingyun_hegemony", "@xingyun_hegemony");
        foreach(int id, move.card_ids)
            room->setCardFlag(id, "-xingyun");

        return c != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *player = invoke->invoker;        
        QString choice = "letdraw";
        //AI:askForChoice or askForPlayerChosen use the "xingyun" AI
        if (player->isWounded())
            choice = room->askForChoice(player, "xingyun", "letdraw+recover", data);
        if (choice == "letdraw") {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), "xingyun", "@xingyun-select");
            target->drawCards(1);
        }
        else if (choice == "recover") {
            RecoverStruct recover;
            recover.who = player;
            room->recover(player, recover);
        }
        
        return false;
    }
};



class YueshiHegemony : public TriggerSkill
{
public:
    YueshiHegemony()
        : TriggerSkill("yueshi_hegemony")
    {
        events << PostCardEffected;
        relate_to_place = "head";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.from && effect.to->hasSkill(this) && effect.to->isWounded() && effect.to->isAlive() && effect.card->isNDTrick()) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, effect.to, effect.to);
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        QString prompt = "invoke:" + effect.card->objectName();
        return invoke->invoker->askForSkillInvoke(objectName(), prompt);
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        JudgeStruct judge;
        judge.who = invoke->invoker;
        judge.pattern = ".|red";
        judge.good = true;
        judge.reason = objectName();
        room->judge(judge);

        if (judge.isGood() && !judge.ignore_judge) {
            RecoverStruct recover;
            recover.recover = 1;
            room->recover(invoke->invoker, recover);
        }
        return false;
    }
};



class PingyiHegemony : public TriggerSkill
{
public:
    PingyiHegemony()
        : TriggerSkill("pingyi_hegemony")
    {
        events << Damage << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {  
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *yori = (e == Damage) ? damage.from : damage.to;
        if (yori == NULL || yori->isDead() || !yori->hasSkill(this) || !yori->canDiscard(yori, "hes"))
            return QList<SkillInvokeDetail>();

        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, yori, yori);

    }


    static QStringList GetAvailableGenerals(Room *room, ServerPlayer *yori)
    {
        QList<const General *> all_generals = Sanguosha->findChildren<const General *>();
        //QStringList all = Sanguosha->getPackages  //Sanguosha->getLimitedGeneralNames(); //.toSet()
        QStringList all_shu;
        foreach(const General *general, all_generals) {
            if (general->getKingdom() == "shu")
                all_shu << general->objectName();
        }
        /*foreach(QString g_name, all) {
            const General *general = Sanguosha->getGeneral(g_name);
            if (general->getKingdom() == "shu")
                all_shu << g_name;
        }*/
        //all_shu << "keine_sp_hegemony";

        //all_shu.removeAll("yorihime_hegemony");
        QStringList names = room->getTag(yori->objectName()).toStringList();
        all_shu.removeAll(names.first());
        all_shu.removeAll(names.last());

        foreach(ServerPlayer *player, room->getAllPlayers()) {
            if (player->hasShownGeneral()) {
                QString name1 = player->getGeneralName();
                const General *general = Sanguosha->getGeneral(name1);
                if (general->getKingdom() == "shu")
                    all_shu.removeAll(name1);
            }
            if (player->hasShownGeneral2()) {
                QString name2 = player->getGeneral2Name();
                const General *general = Sanguosha->getGeneral(name2);
                if (general->getKingdom() == "shu")
                    all_shu.removeAll(name2);
            }
        }
        return all_shu;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        return room->askForCard(invoke->invoker, ".|.|.|hand,equipped", "@pingyi_hegemony", data, Card::MethodDiscard, NULL, false,
            objectName());
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        //bool other_general_place = !invoke->invoker->inHeadSkills(objectName());
        //invoke->invoker->showGeneral(other_general_place);

        QStringList choices = GetAvailableGenerals(room, invoke->invoker);
        if (choices.isEmpty())
            return false;

        //AI *ai = invoke->invoker->getAI();
        if (!invoke->invoker->isOnline()) {//  ai: Just make a random choice
            //QString general_name = room->askForChoice(invoke->invoker, objectName(), choices.join("+"));
            int idx = qrand() % choices.length();
            QString general_name = choices.at(idx);
            const General *general = Sanguosha->getGeneral(general_name);


            QStringList skill_names;

            foreach(const Skill *skill, general->getVisibleSkillList()) {
                if (skill->isAttachedLordSkill() || skill->getFrequency() == Skill::Limited || skill->relateToPlace(true) || skill->relateToPlace(false))
                    continue;

                skill_names << skill->objectName();
            }
            if (skill_names.isEmpty())
                return false;

            int skill_idx = qrand() % skill_names.length();
            QString skill_name = skill_names.at(skill_idx);
            const Skill *skill = Sanguosha->getSkill(skill_name);
            skillProcess(room, invoke->invoker, general_name, skill);
        }
        else {
            room->setPlayerFlag(invoke->invoker, "Pingyi_Choose");
            QString general_name = room->askForGeneral(invoke->invoker, choices);
            room->setPlayerFlag(invoke->invoker, "-Pingyi_Choose");
            const General *general = Sanguosha->getGeneral(general_name);

            QStringList skill_names;

            foreach(const Skill *skill, general->getVisibleSkillList()) {
                if (skill->isAttachedLordSkill() || skill->getFrequency() == Skill::Limited || skill->relateToPlace(true) || skill->relateToPlace(false))
                    continue;

                skill_names << skill->objectName();
            }
            if (skill_names.isEmpty())
                return false;

            QString skill_name = room->askForChoice(invoke->invoker, objectName(), skill_names.join("+"));
            const Skill *skill = Sanguosha->getSkill(skill_name); 
            skillProcess(room, invoke->invoker, general_name, skill);
        }        
        return false;
    }

    static void skillProcess(Room *room, ServerPlayer *yori, QString pingyi_general = QString(), const Skill *skill = NULL)
    {
        JsonArray arg;
        arg << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
        arg << yori->objectName();
        if (pingyi_general.isEmpty() || skill == NULL) {
            arg << QString() << QString() << QString() << QString();
        }
        else {
            if (yori->inHeadSkills("pingyi_hegemony")) {
                arg << pingyi_general;
                arg << skill->objectName();
            }
            arg << QString() << QString();
            if (!yori->inHeadSkills("pingyi_hegemony")) {
                arg << pingyi_general;
                arg << skill->objectName();
            }
        }
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);


        if (yori->tag.contains("pingyi_skill") && yori->tag.contains("pingyi_General")) {
            QString originalSkillName = yori->tag.value("pingyi_skill", QString()).toString();

            //ServerPlayer *originalOwner = yori->tag["pingyi_originalOwner"].value<ServerPlayer *>();

            // 1. yorihime lost the acquired skill
            if (!originalSkillName.isEmpty()) {
                if (!yori->inHeadSkills("pingyi_hegemony"))
                    originalSkillName = originalSkillName + "!";
                room->handleAcquireDetachSkills(yori, "-" + originalSkillName, false);//
            }
            yori->tag.remove("pingyi_skill");
            yori->tag.remove("pingyi_General");
            yori->tag.remove("Huashen_skill");
            yori->tag.remove("Huashen_target");
            yori->tag.remove("Huashen_place");
        }

        if (skill != NULL && !pingyi_general.isEmpty()) {
            yori->tag["pingyi_skill"] = skill->objectName();
            yori->tag["pingyi_General"] = pingyi_general;


            // 2. acquire the skill
            yori->tag["Huashen_skill"] = skill->objectName();//for marshal
            yori->tag["Huashen_target"] = pingyi_general;

            //QString skillname = yori->inHeadSkills("pingyi_hegemony") ? skill->objectName() : skill->objectName() + "!";
            //room->handleAcquireDetachSkills(yori, skillname, true); //need rewrite handleAcquireDetachSkills 


            bool head = yori->inHeadSkills("pingyi_hegemony");
            QString place = (head) ? "head" : "deputy";
            yori->tag["Huashen_place"] = place;

            bool game_start = false;
            // process main skill
            yori->addSkill(skill->objectName(), head);
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
            if (trigger_skill) {
                room->getThread()->addTriggerSkill(trigger_skill);
                if (trigger_skill->getTriggerEvents().contains(GameStart)) {
                    game_start = true;
                }
                    
            }
            SkillAcquireDetachStruct s;
            s.isAcquire = true;
            s.player = yori;
            s.skill = skill;
            QVariant data = QVariant::fromValue(s);
            room->getThread()->trigger(EventAcquireSkill, room, data);

            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_ACQUIRE_SKILL << yori->objectName() << skill->objectName() << head;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

            //process related skill
            foreach(const Skill *related_skill, Sanguosha->getRelatedSkills(skill->objectName())) {
                if (!related_skill->isVisible()) { //??
                                                   //acquireSkill(player, related_skill);
                    yori->addSkill(related_skill->objectName(), head);
                    const TriggerSkill *tr = qobject_cast<const TriggerSkill *>(related_skill);
                    if (tr) {
                        room->getThread()->addTriggerSkill(tr);
                        if (tr->getTriggerEvents().contains(GameStart))
                            game_start = true;
                    }
                    SkillAcquireDetachStruct s;
                    s.isAcquire = true;
                    s.player = yori;
                    s.skill = related_skill;
                    QVariant data = QVariant::fromValue(s);
                    room->getThread()->trigger(EventAcquireSkill, room, data);
                }

            }


            yori->sendSkillsToOthers(head);
            QString flag = (head) ? "h" : "d";
            yori->setSkillsPreshowed(flag, true);
            yori->notifyPreshow();


            

            //trigger game start
            if (game_start) {
                QVariant v = QVariant::fromValue(yori);
                room->getThread()->trigger(GameStart, room, v);
            }
        }
    }
};

class PingyiHegemonyHandler : public TriggerSkill
{
public:
    PingyiHegemonyHandler()
        : TriggerSkill("#pingyi_hegemony")
    {
        events << GeneralShown << GeneralHidden << GeneralRemoved;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {

        if (triggerEvent == GeneralShown) {
            ShowGeneralStruct s = data.value<ShowGeneralStruct>();
            ServerPlayer *target = s.player;

            if (target && target->isAlive()) {
                QString name = s.isHead ? s.player->getGeneralName() : s.player->getGeneral2Name();
                QList<SkillInvokeDetail> d;
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                    QString who = p->tag.value("pingyi_General", QString()).toString();
                    if (who == name)
                        d << SkillInvokeDetail(this, NULL, p, NULL, true);
                }
                return d;
            }
        
        }
        if (triggerEvent == GeneralHidden || triggerEvent == GeneralRemoved) {
            ShowGeneralStruct s = data.value<ShowGeneralStruct>();
            ServerPlayer *target = s.player;
            if (target && target->isAlive()) {
                //QStringList names = room->getTag(target->objectName()).toStringList();
                //QString name = s.isHead ? names.first() : names.last();
                bool invoke = true;
                if (target->hasShownGeneral() && target->getGeneralName() == "yorihime_hegemony")
                    invoke = false;
                if (target->getGeneral2() && target->hasShownGeneral2() && target->getGeneral2Name() == "yorihime_hegemony")
                    invoke = false;
                if (invoke) {
                    QString who = target->tag.value("pingyi_General", QString()).toString();
                    if (!who.isEmpty())
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, NULL, target, NULL, true);
                }
                
            }
        }



        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {

        PingyiHegemony::skillProcess(room, invoke->invoker);
        return false;
    }
};



class YinghuoHegemony : public TriggerSkill
{
public:
    YinghuoHegemony()
        : TriggerSkill("yinghuo_hegemony")
    {
        events << CardUsed << CardResponded;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && use.from->isAlive() && use.from->hasSkill(this) && use.card && use.card->getTypeId() == Card::TypeBasic){
                if (use.to.length() == 1 && use.to.first() == use.from)
                    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, use.from, use.from);
            }
        }
        else {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_from  && response.m_from->isAlive()  && response.m_from->hasSkill(this) &&  response.m_isUse 
                &&  response.m_card && response.m_card->getTypeId() == Card::TypeBasic)
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, response.m_from, response.m_from);
        }
        return QList<SkillInvokeDetail>();
    }


    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        invoke->invoker->drawCards(1);
        return false;
    }

};

class ChongqunHegemony : public TriggerSkill
{
public:
    ChongqunHegemony()
        : TriggerSkill("chongqun_hegemony")
    {
        events << CardsMoveOneTime;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
        if (player != NULL && player->isAlive() && player->hasSkill(this)
            && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            bool can = false;
            foreach(ServerPlayer *p, room->getOtherPlayers(player))
            {
                if (player->canDiscard(p, "hs")) {
                    can = true;
                    break;
                }

            }
            if (!can)
                return d;

            foreach(int id, move.card_ids) {
                if (Sanguosha->getCard(id)->getTypeId() == Card::TypeBasic) {
                    d << SkillInvokeDetail(this, player, player);
                }
                    
            }
        }
        return d;
    }

    bool cost(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    { 
        // how to notice player the remain times?
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(invoke->invoker))
        {
            if (invoke->invoker->canDiscard(p, "hs"))
                targets << p;
        }
        if (targets.isEmpty())
            return false;
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, targets, objectName(), "@chongqun_target", true, true);
        if (target) {
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        int id = room->askForCardChosen(invoke->invoker, invoke->targets.first(), "hs", objectName());
        room->throwCard(id, invoke->targets.first(), invoke->invoker);
        return false;
    }
};



//********  AUTUMN   **********


QiankunHegemony::QiankunHegemony(const QString &owner) : MaxCardsSkill("qiankun_" + owner)
{
}

int QiankunHegemony::getExtra(const Player *target) const
{
    if (target->hasSkill(objectName()) && target->hasShownSkill(objectName()))
        return 2;
    else
        return 0;
}

class ChuanchengHegemony : public TriggerSkill
{
public:
    ChuanchengHegemony()
        : TriggerSkill("chuancheng_hegemony")
    {
        events << Death;
    }

    bool canPreshow() const
    {
        return false;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who->hasSkill(objectName())) {
            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getOtherPlayers(death.who)) {
                if (death.who->isFriendWith(p))
                    targets << p;
            }
            if (!targets.isEmpty())
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, death.who, death.who, targets);
        }
            
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, invoke->targets, objectName(), "@chuancheng_hegemony", true, true);
        if (target) {
            invoke->targets.clear();
            invoke->targets << target;
            return true;
        }
        return false;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        ServerPlayer *target = invoke->targets.first();
        room->handleAcquireDetachSkills(target, "qiankun_suwako");
        room->handleAcquireDetachSkills(target, "chuancheng");
        if (invoke->invoker->getCards("hejs").length() > 0) {
            DummyCard *allcard = new DummyCard;
            allcard->deleteLater();
            allcard->addSubcards(invoke->invoker->getCards("hejs"));
            room->obtainCard(target, allcard, CardMoveReason(CardMoveReason::S_REASON_RECYCLE, target->objectName()), false);
        }
        return false;
    }
};






class DuxinHegemony : public TriggerSkill
{
public:
    DuxinHegemony()
        : TriggerSkill("duxin_hegemony")
    {
        events << TargetConfirmed;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("SkillCard") || use.from == NULL || use.to.length() != 1 || use.from == use.to.first()
            || use.from->hasFlag("Global_ProcessBroken"))
            return QList<SkillInvokeDetail>();

        ServerPlayer *satori = use.to.first();
        if (satori->hasSkill(objectName()) && use.from->getGeneral2() && !use.from->hasShownGeneral2()) {
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, satori, satori, NULL, true, use.from);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        QStringList list = room->getTag(invoke->targets.first()->objectName()).toStringList();
        //list.removeAt(choice == "showhead" ? 1 : 0);
        list.removeAt(0);//remove head
        foreach(const QString &name, list) {
            LogMessage log;
            log.type = "$KnownBothViewGeneral";
            log.from = invoke->invoker;
            log.to << invoke->targets.first();
            log.arg = name;
            log.arg2 = invoke->targets.first()->getRole();
            room->doNotify(invoke->invoker, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
        }
        JsonArray arg;
        arg << objectName();
        arg << JsonUtils::toJsonArray(list);
        room->doNotify(invoke->invoker, QSanProtocol::S_COMMAND_VIEW_GENERALS, arg);
        return false;
    }
};


class WunianHgemony : public TriggerSkill
{
public:
    WunianHgemony()
        : TriggerSkill("wunian_hegemony")
    {
        events << Predamage << TargetConfirming;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *, const QVariant &data) const
    {
        if (e == Predamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->hasSkill(objectName()))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, true);
        }
        else if (e == TargetConfirming) {
            QList<SkillInvokeDetail> d;
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() == Card::TypeTrick) {
                foreach(ServerPlayer *p, use.to) {
                    if (p->hasSkill(this) && p->isWounded() && use.from && use.from != p)
                        d << SkillInvokeDetail(this, p, p, NULL, true);
                }
            }
            return d;
        }

        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        //for AI
        if (e == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            room->setTag("wunian_hegemony_use", data);
        }
            
        return (invoke->invoker->hasShownSkill(this) || invoke->invoker->askForSkillInvoke(this, data));       
    }
    
    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == Predamage) {
            DamageStruct damage = data.value<DamageStruct>();

            ServerPlayer *target = room->askForPlayerChosen(invoke->invoker, room->getOtherPlayers(invoke->invoker), objectName(), "@wunian_transfer:" + damage.to->objectName(), false, true);
            damage.from = target;
            damage.transfer = true;
            //damage.by_user = false;

            //room->touhouLogmessage("#TriggerSkill", invoke->invoker, "wunian");
            //room->notifySkillInvoked(invoke->invoker, objectName());
            data = QVariant::fromValue(damage);
        }
        else if (e == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.to.removeAll(invoke->invoker);
            data = QVariant::fromValue(use);
            LogMessage log;
            log.type = "#SkillAvoid";
            log.from = invoke->invoker;
            log.arg = objectName();
            log.arg2 = use.card->objectName();
            room->notifySkillInvoked(invoke->invoker, objectName());
            room->sendLog(log);
        }
        return false;
    }
};



//********  WINTER   **********
/*
class ShihuiHegemonyVS : public ViewAsSkill
{
public:
    ShihuiHegemonyVS()
        : ViewAsSkill("shihui_hegemonyVS")
    {
        response_pattern = "@@shihui_hegemonyVS";
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *c) const
    {
        return selected.isEmpty() && c->getTypeId() == Card::TypeEquip;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        int maxnum = 1;
        if (cards.length() == maxnum) {
            ExNihilo *exnihilo = new ExNihilo(Card::SuitToBeDecided, -1);
            exnihilo->addSubcards(cards);
            exnihilo->setSkillName("_shihui");
            return exnihilo;
        }

        return NULL;
    }
};
*/

/*
class ShihuiHegemony : public TriggerSkill
{
public:
    ShihuiHegemony()
        : TriggerSkill("shihui_hegemony")
    {
        events << Damage <<Damaged << EventPhaseChanging;
    }

    void record(TriggerEvent e, Room *room, QVariant &data) const
    {
        //record times of using card
        if (e == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            //if (change.from == Player::Play) {
                foreach(ServerPlayer *p, room->getAllPlayers(true))
                    room->setPlayerFlag(p, "-shihui_used");
            //}
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.from || damage.from->isDead() || damage.card == NULL || !damage.card->isKindOf("Slash"))
                return QList<SkillInvokeDetail>();

            QList<SkillInvokeDetail> d;
            foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (!p->hasFlag("shihui_used") &&  p->isFriendWith(damage.from, true))
                    d << SkillInvokeDetail(this, p, p, NULL, false, damage.from);
            }
                
            return d;
        }
        if (e == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.to || damage.to->isDead() || damage.card == NULL || !damage.card->isKindOf("Slash"))
                return QList<SkillInvokeDetail>();

            QList<SkillInvokeDetail> d;
            foreach(ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (!p->hasFlag("shihui_used") && p->isFriendWith(damage.to, true))
                    d << SkillInvokeDetail(this, p, p, NULL, false, damage.to);
            }

            return d;
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        room->setPlayerFlag(invoke->invoker, "shihui_used");
        ServerPlayer *target = invoke->targets.first();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), target->objectName());
        //int maxnum = qMax(target->getEquips().length(), 1);
        room->askForUseCard(target, "@@shihui_hegemonyVS", "shihuiuse_hegemony");
        
            
        return false;
    }
};
*/

class ShihuiHegemony : public TriggerSkill
{
public:
    ShihuiHegemony()
        : TriggerSkill("shihui_hegemony")
    {
        events << Damage << Damaged;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.from || damage.from->isDead() || damage.card == NULL || !damage.card->isKindOf("Slash"))
                return QList<SkillInvokeDetail>();

            AwaitExhaustedHegemony *card = new AwaitExhaustedHegemony(Card::SuitToBeDecided, -1);
            card->setSkillName(objectName());
            card->deleteLater();
            if (damage.from->hasSkill(this) && !damage.from->isCardLimited(card, Card::HandlingMethod::MethodUse))
                return QList<SkillInvokeDetail> () << SkillInvokeDetail(this, damage.from, damage.from);
        }
        else if (e == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.to || damage.to->isDead() || damage.card == NULL || !damage.card->isKindOf("Slash"))
                return QList<SkillInvokeDetail>();

            AwaitExhaustedHegemony *card = new AwaitExhaustedHegemony(Card::SuitToBeDecided, -1);
            card->setSkillName(objectName());
            card->deleteLater();

            if (damage.to->hasSkill(this) && !damage.to->isCardLimited(card, Card::HandlingMethod::MethodUse))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to);
        }
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        AwaitExhaustedHegemony *card = new AwaitExhaustedHegemony(Card::SuitToBeDecided, -1);
        card->setSkillName(objectName());
        room->useCard(CardUseStruct(card, invoke->invoker, NULL), false);

        return false;
    }
};



class DunjiaHegemony : public TriggerSkill
{
public:
    DunjiaHegemony()
        : TriggerSkill("dunjia_hegemony")
    {
        events << Damage << Damaged;
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from == NULL || damage.to->isDead() || damage.from->isDead())
            return QList<SkillInvokeDetail>();
        if (damage.from == damage.to || damage.card == NULL || !damage.card->isKindOf("Slash"))
            return QList<SkillInvokeDetail>();
        int num1 = damage.from->getEquips().length();
        int num2 = damage.to->getEquips().length();
        if (num2 == 0 && num1 == 0)
            return QList<SkillInvokeDetail>();
        int diff = qAbs(num1 - num2);

        if (e == Damage &&  damage.from  && damage.from->hasSkill(this) && diff <= damage.from->getLostHp())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, false, damage.to);
        else if (e == Damaged &&  damage.to  && damage.to->hasSkill(this) && diff <= damage.to->getLostHp())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.to, damage.to, NULL, false, damage.from);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *first = invoke->invoker;
        ServerPlayer *second = invoke->targets.first();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, first->objectName(), second->objectName());

        QList<int> equips1, equips2;
        foreach(const Card *equip, first->getEquips())
            equips1.append(equip->getId());
        foreach(const Card *equip, second->getEquips())
            equips2.append(equip->getId());

        QList<CardsMoveStruct> exchangeMove;
        CardsMoveStruct move1(equips1, second, Player::PlaceEquip,
            CardMoveReason(CardMoveReason::S_REASON_SWAP, first->objectName(), second->objectName(), "dunjia_hegemony", QString()));
        CardsMoveStruct move2(equips2, first, Player::PlaceEquip,
            CardMoveReason(CardMoveReason::S_REASON_SWAP, second->objectName(), first->objectName(), "dunjia_hegemony", QString()));
        exchangeMove.push_back(move2);
        exchangeMove.push_back(move1);
        room->moveCardsAtomic(exchangeMove, false);

        
        return false;
    }
};



class HezouHegemonyVS : public OneCardViewAsSkill
{
public:
    HezouHegemonyVS()
        : OneCardViewAsSkill("hezou_hegemony")
    {
        filter_pattern = "TrickCard,EquipCard";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return matchAvaliablePattern("slash", pattern);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName(objectName());
            return slash;
        }
        return NULL;
    }
};

class HezouHegemony : public TriggerSkill
{
public:
    HezouHegemony()
        : TriggerSkill("hezou_hegemony")
    {
        events << CardUsed << CardResponded;
        view_as_skill = new HezouHegemonyVS;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        
        ServerPlayer *player = NULL;
        const Card *card = NULL;
        if (triggerEvent == CardUsed) {
            player = data.value<CardUseStruct>().from;
            card = data.value<CardUseStruct>().card;
        }
        else {
            CardResponseStruct response = data.value<CardResponseStruct>();
            player = response.m_from;
            //if (response.m_isUse)
            card = response.m_card;
        }
        if (player && card && card->getSkillName() == objectName()) {
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                QList<const Card*> cards = p->getCards("ej");
                foreach(const Card*c, cards) {
                    if (c->getSuit() == card->getSuit())
                        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player, NULL, true);
                }
            }
        }
            
        
        return QList<SkillInvokeDetail>();
    }


    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        invoke->invoker->drawCards(1);
        return false;
    }

};



class XianlingHegemony : public MaxCardsSkill
{
public:
    XianlingHegemony()
        : MaxCardsSkill("xianling_hegemony")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        int num = 0;
        if (target->hasSkill(objectName()) && target->hasShownSkill(objectName())) {
            ++num;
        }
        
        foreach(const Player *p, target->getAliveSiblings()) {
            if (target->isFriendWith(p) && p->hasSkill(objectName()) && p->hasShownSkill(objectName()))
                ++num;
        }
        return num;
    }
};


class JizouHegemony : public TriggerSkill
{
public:
    JizouHegemony()
        : TriggerSkill("jizou_hegemony")
    {
        events << Damage;
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from == NULL || damage.to->isDead() || damage.from->isDead())
            return QList<SkillInvokeDetail>();
        if (damage.from == damage.to || !damage.from->hasSkill(this))
            return QList<SkillInvokeDetail>();

        Dismantlement *card = new Dismantlement(Card::NoSuit, 0);
        card->setSkillName(objectName());
        card->deleteLater();

        if (damage.from->isCardLimited(card, Card::MethodUse) || damage.from->isProhibited(damage.to, card))
            return QList<SkillInvokeDetail>();
        if (card->targetFilter(QList<const Player *>(), damage.to, damage.from))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, false, damage.to);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        Dismantlement *card = new Dismantlement(Card::NoSuit, 0);
        card->setSkillName(objectName());
        card->deleteLater();

        room->useCard(CardUseStruct(card, invoke->invoker, invoke->targets.first()), false);
        return false;
    }
};

class GuanlingHegemony : public TargetModSkill
{
public:
    GuanlingHegemony()
        : TargetModSkill("guanling_hegemony")
    {
        pattern = "Slash";
    }

    virtual int getResidueNum(const Player *from, const Card *card) const
    {
        int num = 0;
        if (from->hasSkill(objectName())) // && from->hasShownSkill(objectName())
            ++num;
        
        foreach(const Player *p, from->getAliveSiblings()) {
            if (from->isFriendWith(p) && p->hasSkill(objectName()) && p->hasShownSkill(objectName()))
                ++num;
        }
        return num;
    }
};


class JianlingHegemony : public TriggerSkill
{
public:
    JianlingHegemony()
        : TriggerSkill("jianling_hegemony")
    {
        events << DrawNCards;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {
        QList<SkillInvokeDetail> d;
        DrawNCardsStruct qnum = data.value<DrawNCardsStruct>();
        if (qnum.player->hasSkill(objectName()))
            d << SkillInvokeDetail(this, qnum.player, qnum.player, NULL, qnum.player->hasShownSkill(this));


        foreach(ServerPlayer *p, room->getOtherPlayers(qnum.player)) {
            if (qnum.player->isFriendWith(p) && p->hasSkill(objectName()) && p->hasShownSkill(this))
                d << SkillInvokeDetail(this, p, qnum.player, NULL, true);
        }

        return d;   
    }

    bool effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DrawNCardsStruct qnum = data.value<DrawNCardsStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->owner->objectName(), qnum.player->objectName());
        qnum.n = qnum.n + 1;
        data = QVariant::fromValue(qnum);
        room->notifySkillInvoked(invoke->owner, objectName());
        room->touhouLogmessage("#TriggerSkill", invoke->owner, objectName());
        return false;
    }
};



class BaochunHegemony : public TriggerSkill
{
public:
    BaochunHegemony()
        : TriggerSkill("baochun_hegemony")
    {
        events << Damaged << HpRecover;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        if (e == Damaged) {
            QList<SkillInvokeDetail> d;
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->isAlive()) {
                foreach(ServerPlayer *p, room->getAlivePlayers()) {
                    if (damage.to == p) {
                        if (damage.to->hasSkill(this))
                            d << SkillInvokeDetail(this, damage.to, damage.to, NULL, true);
                    }
                    else {
                        if (p->hasSkill(this) && p->isFriendWith(damage.to, true))
                            d << SkillInvokeDetail(this, p, p, NULL, true);
                    }
                }
            }
            return d;
        }
        else if (e == HpRecover)
        {
            RecoverStruct r = data.value<RecoverStruct>();
            if (r.to->isAlive() && r.to->hasSkill(this))
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, r.to, r.to, NULL, true);
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        if (e == HpRecover)
            room->setPlayerFlag(invoke->invoker, "Global_baochunAIFailed");
        return (invoke->invoker->hasShownSkill(this) || invoke->invoker->askForSkillInvoke(this, data));
    }

    bool effect(TriggerEvent e, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        bool draw = true;
        if (e == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (invoke->invoker != damage.to)
                draw = false;
        }
        room->touhouLogmessage("#TriggerSkill", invoke->invoker, objectName());
        room->notifySkillInvoked(invoke->invoker, objectName());
        if (!draw) {
            if (invoke->invoker->canDiscard(invoke->invoker, "hes"))
                room->askForDiscard(invoke->invoker, objectName(), 1, 1, false, true);
        }
        else {
            QList<ServerPlayer *> targets;
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (invoke->invoker->isFriendWith(p, true))
                    targets << p;
            }
            foreach(ServerPlayer *p, targets) {
                if (p->isAlive())
                    p->drawCards(1);
            }
        
        }
        
        
        return false;
    }
};



ChunhenHegemonyCard::ChunhenHegemonyCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "chunhen_hegemony";
}

void ChunhenHegemonyCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    DummyCard dummy(subcards);
    room->obtainCard(targets.first(), &dummy);
}

class ChunhenHegemonyVS : public ViewAsSkill
{
public:
    ChunhenHegemonyVS() : ViewAsSkill("chunhen_hegemony")
    {
        expand_pile = "#chunhen_temp";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return Self->getPile("#chunhen_temp").contains(to_select->getId());
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@chunhen");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty()) return NULL;
        ChunhenHegemonyCard *card = new ChunhenHegemonyCard;
        card->addSubcards(cards);
        return card;
    }
};

class ChunhenHegemony : public TriggerSkill
{
public:
    ChunhenHegemony() : TriggerSkill("chunhen_hegemony")
    {
        events << CardsMoveOneTime;
        view_as_skill = new ChunhenHegemonyVS;
    }

    virtual bool canPreshow() const
    {
        return true;
    }

    static bool putToPile(Room *room, ServerPlayer *player, QList<int> ids)
    {
        CardsMoveStruct move;
        move.from_place = Player::DiscardPile;
        move.to = player;
        move.to_player_name = player->objectName();
        move.to_pile_name = "#chunhen_temp";
        move.card_ids = ids;
        move.to_place = Player::PlaceSpecial;
        move.open = true;

        QList<CardsMoveStruct> _moves = QList<CardsMoveStruct>() << move;
        QList<ServerPlayer *> _player = QList<ServerPlayer *>() << player;
        room->setPlayerFlag(player, "chunhen_InTempMoving");
        room->notifyMoveCards(true, _moves, true, _player);
        room->notifyMoveCards(false, _moves, true, _player);
        room->setPlayerFlag(player, "-chunhen_InTempMoving");

        QVariantList tag = IntList2VariantList(ids);
        player->tag["chunhen_tempcards"] = tag;
        return true;
    }

    static void cleanUp(Room *room, ServerPlayer *player)
    {
        QList<int> reds = VariantList2IntList(player->tag.value("chunhen_tempcards", QVariantList()).toList());
        player->tag.remove("chunhen_tempcards");
        if (reds.isEmpty())
            return;

        CardsMoveStruct move;
        move.from = player;
        move.from_player_name = player->objectName();
        move.from_place = Player::PlaceSpecial;
        move.from_pile_name = "#chunhen_temp";
        move.to_place = Player::DiscardPile;
        move.open = true;
        move.card_ids = reds;

        QList<CardsMoveStruct> _moves = QList<CardsMoveStruct>() << move;
        QList<ServerPlayer *> _player = QList<ServerPlayer *>() << player;
        room->setPlayerFlag(player, "chunhen_InTempMoving");
        room->notifyMoveCards(true, _moves, true, _player);
        room->notifyMoveCards(false, _moves, true, _player);
        room->setPlayerFlag(player, "-chunhen_InTempMoving");
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent event, const Room *room, const QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
        if (player == NULL  || !player->hasSkill(this))  // || player->tag["chunhen_to_judge"].toStringList().isEmpty()
            return QList<SkillInvokeDetail>();

        if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            if ((move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip)) && move.to_place == Player::DiscardPile) {
                QList<int> ids;
                foreach(int id, move.card_ids) {
                    if (Sanguosha->getCard(id)->isRed() && room->getCardPlace(id) == Player::DiscardPile)
                        ids << id;
                }
                if (ids.isEmpty())
                    return QList<SkillInvokeDetail>();
                
                return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, player, player);
            }
        }
        return QList<SkillInvokeDetail>();
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        while (true) {
            QList<int> ids;
            foreach(int id, move.card_ids) {
                if (Sanguosha->getCard(id)->isRed() && room->getCardPlace(id) == Player::DiscardPile)
                    ids << id;
            }
            if (ids.isEmpty())
                return false;

            ServerPlayer *player = invoke->invoker;
            if (!putToPile(room, player, ids))
                return false;

            QVariantList listc = IntList2VariantList(ids);
            invoke->invoker->tag["chunhen_cards"] = listc; //for ai
            const Card *usecard = room->askForUseCard(player, "@@chunhen_hegemony", "@chunhen_give", -1, Card::MethodNone);
            cleanUp(room, player);
            if (usecard == NULL)
                return false;
        }

        return false;
    }

    /*virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QStringList lirang_card = player->tag["lirang_to_judge"].toStringList();
        QList<int> cards = StringList2IntList(lirang_card.last().split("+"));
        lirang_card.removeLast();

        QList<CardsMoveStruct> moves;

        do {
            room->notifyMoveToPile(player, cards, objectName(), Player::DiscardPile, false, false);

            QStringList targets = player->tag["lirang_target"].toStringList();
            QStringList cards_get = player->tag["lirang_get"].toStringList();
            QList<int> get = StringList2IntList(cards_get.last().split("+"));
            ServerPlayer *target;
            foreach(ServerPlayer *p, room->getAlivePlayers()) {
                if (p->objectName() == targets.last())
                    target = p;
            }
            targets.removeLast();
            cards_get.removeLast();
            player->tag["lirang_target"] = targets;
            player->tag["lirang_get"] = cards_get;

            CardMoveReason reason(CardMoveReason::S_REASON_PREVIEWGIVE, player->objectName(), target->objectName(), objectName(), QString());
            CardsMoveStruct move(get, target, Player::PlaceHand, reason);
            moves.append(move);

            foreach(int id, get)
                cards.removeOne(id);

            if (!cards.isEmpty()) {
                room->notifyMoveToPile(player, cards, objectName(), Player::DiscardPile, true, true);
                player->tag["lirang_this_time"] = IntList2VariantList(cards);
            }
        }

        while (!cards.isEmpty() && player->isAlive()
            && room->askForUseCard(player, "@@lirang", "@lirang-distribute:::" + QString::number(cards.length()), -1, Card::MethodNone));

        if (!cards.isEmpty()) room->notifyMoveToPile(player, cards, objectName(), Player::DiscardPile, false, false);

        player->tag["lirang_to_judge"] = lirang_card;
        player->tag.remove("lirang_this_time");
        room->moveCardsAtomic(moves, true);
        room->broadcastSkillInvoke(objectName(), player);

        return false;
    }*/
};



class ZhancaoHegemony : public TriggerSkill
{
public:
    ZhancaoHegemony()
        : TriggerSkill("zhancao_hegemony")
    {
        events << TargetConfirmed;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *room, const QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        QList<SkillInvokeDetail> d;
        if (use.card->isKindOf("Slash")) {
            QList<ServerPlayer *> srcs = room->findPlayersBySkillName(objectName());
            foreach(ServerPlayer *p, srcs) {
                if (!p->canDiscard(p, "e"))
                    continue;
                foreach(ServerPlayer *to, use.to) {
                    if (to->isAlive() && (p->inMyAttackRange(to) || p == to))
                        d << SkillInvokeDetail(this, p, p, NULL, false, to);
                }
            }
        }
        return d;
    }

    bool cost(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        invoke->invoker->tag["zhancao_target"] = QVariant::fromValue(invoke->preferredTarget);
        QString prompt = "@zhancao_hegemony-discard:" + use.from->objectName() + ":" + invoke->preferredTarget->objectName();
        return room->askForCard(invoke->invoker, ".|.|.|equipped", prompt, data, objectName()) != NULL;
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        use.nullified_list << invoke->targets.first()->objectName();
        data = QVariant::fromValue(use);
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), invoke->targets.first()->objectName());
        room->touhouLogmessage("#zhancaoTarget", invoke->invoker, objectName(), QList<ServerPlayer *>() << invoke->targets.first());

        return false;
    }
};


MocaoHegemonyCard::MocaoHegemonyCard()
{
    m_skillName = "mocao_hegemony";
}

bool MocaoHegemonyCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;
    return !to_select->getEquips().isEmpty();
}

void MocaoHegemonyCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    int card_id = room->askForCardChosen(effect.from, effect.to, "e", "mocao");
    room->obtainCard(effect.from, card_id);
    effect.to->drawCards(qMax(1, effect.to->getLostHp()));
}

class MocaoHegemony : public ZeroCardViewAsSkill
{
public:
    MocaoHegemony()
        : ZeroCardViewAsSkill("mocao_hegemony")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MocaoHegemonyCard");
    }

    virtual const Card *viewAs() const
    {
        return new MocaoHegemonyCard;
    }
};


class HanboHegemony : public TriggerSkill
{
public:
    HanboHegemony()
        : TriggerSkill("hanbo_hegemony")
    {
        events << CardsMoveOneTime;
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
    {

        ServerPlayer *current = room->getCurrent();
        if (!current || current->isDead() || !current->hasSkill(this))
            return QList<SkillInvokeDetail>();

        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *target = qobject_cast<ServerPlayer *>(move.from);
        if (target &&  target != current &&  target->isAlive() && move.from_places.contains(Player::PlaceHand) && target->isKongcheng())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, current, current, NULL, false, target);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        ServerPlayer *target = invoke->targets.first();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), target->objectName());

        target->drawCards(2);
        target->turnOver();

        return false;
    }
};


DongzhiHegemonyCard::DongzhiHegemonyCard()
{
    m_skillName = "dongzhi_hegemony";
}

bool DongzhiHegemonyCard::targetFilter(const QList<const Player *> &selected, const Player *to_select, const Player *player) const
{
    return selected.isEmpty() && to_select->hasShownOneGeneral();

    /*if (!to_selected->hasShownOneGeneral()) //consider anjiang  has comfirmed??   to_selected->getRole() == NULL??  
        retrun false;
    if (selected.isEmpty())
        return true;
    QString role = selected.first()->getRole();

    return (role != "careerist"  &&  to_select->getRole() == role);*/
}


void DongzhiHegemonyCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    QList<ServerPlayer *> targets;
    ServerPlayer *target = card_use.to.first();
    targets << target;
    QList<ServerPlayer *> players = room->getOtherPlayers(target);
    foreach(ServerPlayer *player, players) {
        if (!target->isFriendWith(player))
            continue;
        targets << player;
    }

    CardUseStruct use = card_use;
    use.to = targets;
    room->doLightbox("$dongzhiAnimate", 4000);
    SkillCard::onUse(room, use);
}


void DongzhiHegemonyCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->removePlayerMark(source, "@dongzhi");
    foreach(ServerPlayer *p, targets) {
        int num = p->getHp();
        if (!room->askForDiscard(p, "dongzhi", num, num, true, true))
            room->setPlayerMark(p, "@dongzhi_damage", 1);
    }
}

class DongzhiHegemonyVS : public ZeroCardViewAsSkill
{
public:
    DongzhiHegemonyVS()
        : ZeroCardViewAsSkill("dongzhi_hegemony")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@dongzhi") > 0;
    }

    virtual const Card *viewAs() const
    {
        return new DongzhiHegemonyCard;
    }
};

class DongzhiHegemony : public TriggerSkill
{
public:
    DongzhiHegemony()
        : TriggerSkill("dongzhi_hegemony")
    {
        events << DamageInflicted << EventPhaseStart;
        limit_mark = "@dongzhi";
        view_as_skill = new DongzhiHegemonyVS;
        frequency = Limited;
    }

    void record(TriggerEvent triggerEvent, Room *room, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *player = data.value<ServerPlayer *>();
            if (player && player->getPhase() == Player::NotActive)
                foreach(ServerPlayer *p, room->getAllPlayers()) {
                if (p->getMark("@dongzhi_damage") > 0) {
                    room->setPlayerMark(p, "@dongzhi_damage", 0);
                }
            }
        }
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *, const QVariant &data) const
    {
        if (triggerEvent != DamageInflicted)
            return QList<SkillInvokeDetail>();

        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.to || damage.to->isDead() || damage.to->getMark("@dongzhi_damage") == 0)
            return QList<SkillInvokeDetail>();


        return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, NULL, damage.to, NULL, true);

    }


    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        //log
        DamageStruct damage = data.value<DamageStruct>();
        LogMessage log;
        log.type = "#DongzhiDamage";
        log.from = invoke->invoker;
        log.arg = QString::number(damage.damage);
        log.arg2 = QString::number(++damage.damage);
        room->sendLog(log);

        data = QVariant::fromValue(damage);
        return false;
    }
};




class DongjieHegemony : public TriggerSkill
{
public:
    DongjieHegemony()
        : TriggerSkill("dongjie_hegemony")
    {
        events << DamageCaused; //<< EventPhaseChanging
    }


    QList<SkillInvokeDetail> triggerable(TriggerEvent e, const Room *room, const QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        //if (damage.chain || damage.transfer || !damage.by_user)
        //	return QList<SkillInvokeDetail>();
        if (damage.from  && damage.from->hasSkill(this) && damage.card && damage.card->isKindOf("Slash")) //   !damage.from->hasFlag(objectName())
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, damage.from, damage.from, NULL, false, damage.to);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, invoke->invoker->objectName(), damage.to->objectName());

        //invoke->invoker->setFlags(objectName());


        if (!room->askForCard(damage.to, ".|.|.|hand", "@dongjie_discard:" + damage.from->objectName(), data, Card::MethodDiscard)) {
            damage.to->drawCards(1);
            damage.to->turnOver();
            return true;
        }
        return false;
    }
};

class BingpoHgemony : public TriggerSkill
{
public:
    BingpoHgemony()
        : TriggerSkill("bingpo_hegemony")
    {
        events << Dying;
        frequency = Compulsory;
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.who->hasSkill(this) && dying.who->getHp() < dying.who->dyingThreshold() 
            && (dying.damage == NULL || dying.damage->nature != DamageStruct::Fire))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dying.who, dying.who, NULL, true);
        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
    {
        room->touhouLogmessage("#bingpo_hegemony_log", invoke->invoker, objectName(), QList<ServerPlayer *>(), QString::number(1));
        room->notifySkillInvoked(invoke->invoker, objectName());
        RecoverStruct recover;
        room->recover(invoke->invoker, recover);
        return false;
    }
};



class JuxianHegemony : public TriggerSkill
{
public:
    JuxianHegemony()
        : TriggerSkill("juxian_hegemony")
    {
        events << Dying;
        frequency = Limited;
        limit_mark = "@juxian";
    }

    QList<SkillInvokeDetail> triggerable(TriggerEvent, const Room *, const QVariant &data) const
    {
        DyingStruct dying = data.value<DyingStruct>();

        if (dying.who->hasSkill(this) && dying.who->isAlive() && dying.who->getHp() < dying.who->dyingThreshold() && dying.who->getMark("@juxian") > 0)
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, dying.who, dying.who);

        return QList<SkillInvokeDetail>();
    }

    bool effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &) const
    {
        room->removePlayerMark(invoke->invoker, "@juxian");
        room->doLightbox("$juxianAnimate", 4000);

        QList<int> list = room->getNCards(3);
        CardsMoveStruct move(list, NULL, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, invoke->invoker->objectName(), objectName(), QString()));
        room->moveCardsAtomic(move, true);

        int s = 0; int h = 0; int c = 0; int d = 0;
        foreach(int id, list) {
            Card *card = Sanguosha->getCard(id);
            if (card->getSuit() == Card::Spade)
                s = 1;
            else if (card->getSuit() == Card::Heart)
                h = 1;
            else if (card->getSuit() == Card::Club)
                c = 1;
            else if (card->getSuit() == Card::Diamond)
                d = 1;
        }

        DummyCard dummy(list);
        invoke->invoker->obtainCard(&dummy);

        RecoverStruct recover;
        recover.recover = (s + h + c + d);
        room->recover(invoke->invoker, recover);

        return false;
    }
};


BanyueHegemonyCard::BanyueHegemonyCard()
{
    m_skillName = "banyue_hegemony";
}

bool BanyueHegemonyCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Card *card = Sanguosha->cloneCard("befriend_attacking");
    DELETE_OVER_SCOPE(Card, card)
    if (targets.isEmpty()) {
        return (to_select == Self || to_select->hasShownOneGeneral()) 
            && (!to_select->isCardLimited(card, Card::HandlingMethod::MethodUse));
    }
    else if (targets.length() == 1) {
        const Player *user = targets.first();
        return  to_select->hasShownOneGeneral() && !user->isFriendWith(to_select, (user == Self)) 
            && !user->isProhibited(to_select,card, QList<const Player *>());
    }
    return false;
}

bool BanyueHegemonyCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void BanyueHegemonyCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    QVariant data = QVariant::fromValue(use);
    RoomThread *thread = room->getThread();
    use.from->showHiddenSkill("banyue_hegemony");
    thread->trigger(PreCardUsed, room, data);
    use = data.value<CardUseStruct>();

    ServerPlayer *from = card_use.from;
    ServerPlayer *to1 = card_use.to.at(0);
    ServerPlayer *to2 = card_use.to.at(1);
    QList<ServerPlayer *> logto;
    logto << to1 << to2;
    room->touhouLogmessage("#ChoosePlayerWithSkill", from, "banyue_hegemony", logto, "");
    room->notifySkillInvoked(card_use.from, "banyue_hegemony");

    thread->trigger(CardUsed, room, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, data);
}


void BanyueHegemonyCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const // onEffect is better?
{
    
    room->loseHp(source);
    ServerPlayer *to1 = targets.first();
    ServerPlayer *to2 = targets.last();
    Card *card = Sanguosha->cloneCard("befriend_attacking");
    card->setSkillName("_banyue_hegemony");

    CardUseStruct use;
    use.from = to1;
    use.to << to2;
    use.card = card;
    room->useCard(use);
}

class BanyueHegemony : public ZeroCardViewAsSkill
{
public:
    BanyueHegemony()
        : ZeroCardViewAsSkill("banyue_hegemony")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("BanyueHegemonyCard");
    }

    virtual const Card *viewAs() const
    {
        return new BanyueHegemonyCard;
    }
};




HegemonyGeneralPackage::HegemonyGeneralPackage()
    : Package("hegemonyGeneral")
{
    



    General *reimu_hegemony = new General(this, "reimu_hegemony", "zhu", 4);
    reimu_hegemony->addSkill(new TuizhiHegemony);
    reimu_hegemony->addSkill(new TongjieHegemony);

    //reimu_hegemony->addSkill("qixiang");
    //reimu_hegemony->addSkill("fengmo");
    reimu_hegemony->addCompanion("marisa_hegemony");
    reimu_hegemony->addCompanion("yukari_hegemony");
    reimu_hegemony->addCompanion("aya_hegemony");

    General *marisa_hegemony = new General(this, "marisa_hegemony", "zhu", 4);
    marisa_hegemony->addSkill("mofa");
    marisa_hegemony->addCompanion("patchouli_hegemony");
    marisa_hegemony->addCompanion("alice_hegemony");
    marisa_hegemony->addCompanion("nitori_hegemony");
    
    
    

//Spring
    General *byakuren_hegemony = new General(this, "byakuren_hegemony", "wu", 4);
    byakuren_hegemony->addSkill("pudu");
    byakuren_hegemony->addSkill("jiushu");
    byakuren_hegemony->addCompanion("toramaru_hegemony");
    byakuren_hegemony->addCompanion("murasa_hegemony");
    byakuren_hegemony->addCompanion("ichirin_hegemony");

    General *nue_hegemony = new General(this, "nue_hegemony", "wu", 3);
    nue_hegemony->addSkill("weizhi");
    nue_hegemony->addSkill("weizhuang");
    nue_hegemony->addCompanion("mamizou_hegemony");

    General *toramaru_hegemony = new General(this, "toramaru_hegemony", "wu", 4);
    toramaru_hegemony->addSkill("jinghua");
    toramaru_hegemony->addSkill("weiguang");
    toramaru_hegemony->addCompanion("nazrin_hegemony");

    General *murasa_hegemony = new General(this, "murasa_hegemony", "wu", 4);
    murasa_hegemony->addSkill("shuinan");
    murasa_hegemony->addSkill("nihuo");

    General *ichirin_hegemony = new General(this, "ichirin_hegemony", "wu", 4);
    ichirin_hegemony->addSkill(new LizhiHegemony);
    ichirin_hegemony->addSkill("yunshang");

    General *nazrin_hegemony = new General(this, "nazrin_hegemony", "wu", 3);
    nazrin_hegemony->addSkill("xunbao");
    nazrin_hegemony->addSkill("lingbai");

    General *miko_hegemony = new General(this, "miko_hegemony", "wu", 4);
    miko_hegemony->addSkill(new QingtingHegemony);
    miko_hegemony->addSkill(new ChilingHegemony);
    miko_hegemony->addSkill(new ShezhengHegemony);
    miko_hegemony->addSkill(new ShezhengViewHas);
    miko_hegemony->addSkill(new ShezhengHegemonyHandler);
    miko_hegemony->addCompanion("futo_hegemony");
    miko_hegemony->addCompanion("toziko_hegemony");
    miko_hegemony->addCompanion("seiga_hegemony");
    miko_hegemony->setHeadMaxHpAdjustedValue(-1);
    related_skills.insertMulti("shezheng_hegemony", "#shezheng_hegemony");
    related_skills.insertMulti("shezheng_hegemony", "#shezheng_viewhas");

    General *mamizou_hegemony = new General(this, "mamizou_hegemony", "wu", 4);
    mamizou_hegemony->addSkill("xihua");
    mamizou_hegemony->addSkill("#xihua_clear");

    General *futo_hegemony = new General(this, "futo_hegemony", "wu", 3);
    futo_hegemony->addSkill("shijie");
    futo_hegemony->addSkill("fengshui");
    futo_hegemony->addCompanion("toziko_hegemony");

    General *toziko_hegemony = new General(this, "toziko_hegemony", "wu", 4);
    toziko_hegemony->addSkill("leishi");
    toziko_hegemony->addSkill(new FenleiHegemony);

    General *seiga_hegemony = new General(this, "seiga_hegemony", "wu", 3);
    seiga_hegemony->addSkill("xiefa");
    seiga_hegemony->addSkill("chuanbi");
    seiga_hegemony->addCompanion("yoshika_hegemony");

    General *yoshika_hegemony = new General(this, "yoshika_hegemony", "wu", 4);
    yoshika_hegemony->addSkill("duzhua");
    yoshika_hegemony->addSkill("#duzhuaTargetMod");
    yoshika_hegemony->addSkill("taotie");

    General *kyouko_hegemony = new General(this, "kyouko_hegemony", "wu", 3);
    kyouko_hegemony->addSkill("songjing");
    kyouko_hegemony->addSkill("gongzhen");



    General *kogasa_hegemony = new General(this, "kogasa_hegemony", "wu", 3);
    kogasa_hegemony->addSkill("yiwang");
    kogasa_hegemony->addSkill(new JingxiaHegemony);

    General *kokoro_hegemony = new General(this, "kokoro_hegemony", "wu", 4);
    kokoro_hegemony->addSkill("nengwu");
    kokoro_hegemony->addSkill("#nengwu2");
    kokoro_hegemony->addCompanion("miko_hegemony");


//Summer 

    General *remilia_hegemony = new General(this, "remilia_hegemony", "shu", 3);
    remilia_hegemony->addSkill(new SkltKexueHegemony);
    remilia_hegemony->addSkill("mingyun");
    remilia_hegemony->addCompanion("flandre_hegemony");
    remilia_hegemony->addCompanion("sakuya_hegemony");
    remilia_hegemony->addCompanion("patchouli_hegemony");

    General *flandre_hegemony = new General(this, "flandre_hegemony", "shu", 3);
    flandre_hegemony->addSkill("pohuai");
    flandre_hegemony->addSkill("yuxue");
    flandre_hegemony->addSkill("#yuxue-slash-ndl");
    flandre_hegemony->addSkill("shengyan");
    flandre_hegemony->addCompanion("meirin_hegemony");


    General *sakuya_hegemony = new General(this, "sakuya_hegemony", "shu", 4);
    sakuya_hegemony->addSkill("suoding");
    sakuya_hegemony->addSkill("huisu");
    sakuya_hegemony->addCompanion("meirin_hegemony");

    General *patchouli_hegemony = new General(this, "patchouli_hegemony", "shu", 3);
    patchouli_hegemony->addSkill(new BolanHgemony);
    patchouli_hegemony->addSkill("hezhou");
    patchouli_hegemony->addCompanion("koakuma_hegemony");

    General *meirin_hegemony = new General(this, "meirin_hegemony", "shu", 4);
    meirin_hegemony->addSkill("taiji");
    meirin_hegemony->addSkill(new BeishuiHegemony);

    General *koakuma_hegemony = new General(this, "koakuma_hegemony", "shu", 3);
    koakuma_hegemony->addSkill(new MoqiHgemony);
    koakuma_hegemony->addSkill("sishu");

    General *kaguya_hegemony = new General(this, "kaguya_hegemony", "shu", 4);
    kaguya_hegemony->addSkill(new XuyuHegemony);
    kaguya_hegemony->addSkill("shenbao");
    kaguya_hegemony->addSkill("#shenbao_distance");
    kaguya_hegemony->addSkill("#shenbao");
    kaguya_hegemony->addSkill("#shenbao_viewhas");
    kaguya_hegemony->addCompanion("eirin_hegemony");
    kaguya_hegemony->addCompanion("mokou_hegemony");

    General *eirin_hegemony = new General(this, "eirin_hegemony", "shu", 4);
    eirin_hegemony->addSkill("ruizhi");
    eirin_hegemony->addSkill(new YaoshiHegemony);
    //eirin_hegemony->addSkill("miyao");
    eirin_hegemony->addCompanion("reisen_hegemony");

    General *mokou_hegemony = new General(this, "mokou_hegemony", "shu", 4);
    mokou_hegemony->addSkill(new KaifengHegemony);
    mokou_hegemony->addSkill(new FengxiangHegemony);
    mokou_hegemony->addCompanion("keine_hegemony");
    mokou_hegemony->addCompanion("keine_sp_hegemony");

    General *reisen_hegemony = new General(this, "reisen_hegemony", "shu", 4);
    reisen_hegemony->addSkill("kuangzao");
    reisen_hegemony->addSkill("huanshi");
    reisen_hegemony->addCompanion("tewi_hegemony");


    General *keine_hegemony = new General(this, "keine_hegemony", "shu", 3);
    keine_hegemony->addSkill(new XushiHegemony);
    keine_hegemony->addSkill(new XinyueHegemony);

    General *keine_sp_hegemony = new General(this, "keine_sp_hegemony", "shu", 3, false, true);
    keine_sp_hegemony->addSkill("chuangshi");
    keine_sp_hegemony->addSkill(new WangyueHegemony);

    General *tewi_hegemony = new General(this, "tewi_hegemony", "shu", 3);
    tewi_hegemony->addSkill("buxian");
    tewi_hegemony->addSkill("#buxian");
    tewi_hegemony->addSkill(new XingyungHegemony);

    General *toyohime_hegemony = new General(this, "toyohime_hegemony", "shu", 4);
    toyohime_hegemony->addSkill("lianxi");
    toyohime_hegemony->addSkill(new YueshiHegemony);
    toyohime_hegemony->setHeadMaxHpAdjustedValue(-1);
    toyohime_hegemony->addCompanion("yorihime_hegemony");

    General *yorihime_hegemony = new General(this, "yorihime_hegemony", "shu", 4);
    yorihime_hegemony->addSkill(new PingyiHegemony);
    yorihime_hegemony->addSkill(new PingyiHegemonyHandler);
    related_skills.insertMulti("pingyi_hegemony", "#pingyi_hegemony");

    General *wriggle_hegemony = new General(this, "wriggle_hegemony", "shu", 3);
    wriggle_hegemony->addSkill(new YinghuoHegemony);
    wriggle_hegemony->addSkill(new ChongqunHegemony);


//Autumn
    General *kanako_hegemony = new General(this, "kanako_hegemony", "qun", 4);
    kanako_hegemony->addSkill("shende");
    kanako_hegemony->addSkill(new QiankunHegemony("kanako"));
    //kanako_hegemony->addSkill(new Niaoxiang);
    kanako_hegemony->addCompanion("suwako_hegemony");
    kanako_hegemony->addCompanion("sanae_hegemony");

    General *suwako_hegemony = new General(this, "suwako_hegemony", "qun", 3);
    suwako_hegemony->addSkill("bushu");
    suwako_hegemony->addSkill(new QiankunHegemony("suwako"));
    suwako_hegemony->addSkill(new ChuanchengHegemony);
    suwako_hegemony->addCompanion("sanae_hegemony");

    General *sanae_hegemony = new General(this, "sanae_hegemony", "qun", 3);
    sanae_hegemony->addSkill("dfgzmjiyi");
    sanae_hegemony->addSkill("qiji");

    General *aya_hegemony = new General(this, "aya_hegemony", "qun", 3);
    aya_hegemony->addSkill("fengshen");
    aya_hegemony->addSkill("fengsu");
    aya_hegemony->addSkill("#fengsu-effect");
    aya_hegemony->addCompanion("momizi_hegemony");

    General *nitori_hegemony = new General(this, "nitori_hegemony", "qun", 3);
    nitori_hegemony->addSkill("xinshang");
    nitori_hegemony->addSkill("#xinshang_effect");
    nitori_hegemony->addSkill("micai");

    General *hina_hegemony = new General(this, "hina_hegemony", "qun", 3);
    hina_hegemony->addSkill("jie");
    hina_hegemony->addSkill("liuxing");

    General *momizi_hegemony = new General(this, "momizi_hegemony", "qun", 4);
    //momizi_hegemony->addSkill("shouhu");
    //momizi_hegemony->addSkill("shaojie");
    momizi_hegemony->addSkill("buju");

    General *minoriko_hegemony = new General(this, "minoriko_hegemony", "qun", 4);
    minoriko_hegemony->addSkill("fengrang");
    minoriko_hegemony->addSkill("shouhuo");
    minoriko_hegemony->addCompanion("shizuha_hegemony");

    General *shizuha_hegemony = new General(this, "shizuha_hegemony", "qun", 4);
    shizuha_hegemony->addSkill("jiliao");
    shizuha_hegemony->addSkill("zhongyan");

    General *satori_hegemony = new General(this, "satori_hegemony", "qun", 3);
    satori_hegemony->addSkill("xiangqi");
    //satori_hegemony->addSkill("duxin");
    satori_hegemony->addSkill(new DuxinHegemony);
    satori_hegemony->addCompanion("koishi_hegemony");

    General *koishi_hegemony = new General(this, "koishi_hegemony", "qun", 3);
    koishi_hegemony->addSkill("maihuo");
    koishi_hegemony->addSkill(new WunianHgemony);


    General *utsuho_hegemony = new General(this, "utsuho_hegemony", "qun", 4);
    utsuho_hegemony->addSkill("yaoban");
    utsuho_hegemony->addSkill("here");
    utsuho_hegemony->addCompanion("rin_hegemony");

    General *rin_hegemony = new General(this, "rin_hegemony", "qun", 4);
    rin_hegemony->addSkill("yuanling");
    rin_hegemony->addSkill("songzang");

    General *yugi_hegemony = new General(this, "yugi_hegemony", "qun", 4);
    yugi_hegemony->addSkill("guaili");
    yugi_hegemony->addSkill("jiuhao");
    yugi_hegemony->addCompanion("parsee_hegemony");

    General *parsee_hegemony = new General(this, "parsee_hegemony", "qun", 3);
    parsee_hegemony->addSkill("jidu");
    parsee_hegemony->addSkill("gelong");

//Winter
    General *yuyuko_hegemony = new General(this, "yuyuko_hegemony", "wei", 4, false);
    yuyuko_hegemony->addSkill("sidie");
    yuyuko_hegemony->addSkill("huaxu");
    yuyuko_hegemony->addCompanion("yukari_hegemony");
    yuyuko_hegemony->addCompanion("youmu_hegemony");

    General *yukari_hegemony = new General(this, "yukari_hegemony", "wei", 4, false);
    yukari_hegemony->addSkill("shenyin");
    yukari_hegemony->addSkill("xijian");
    yukari_hegemony->addCompanion("ran_hegemony");

    General *ran_hegemony = new General(this, "ran_hegemony", "wei", 3, false);
    ran_hegemony->addSkill(new ShihuiHegemony);
    ran_hegemony->addSkill("huanzang");
    ran_hegemony->addSkill("#huanzang");
    ran_hegemony->addCompanion("chen_hegemony");


    General *youmu_hegemony = new General(this, "youmu_hegemony", "wei", 4, false);
    youmu_hegemony->addSkill("shuangren");
    youmu_hegemony->addSkill("zhanwang");

    General *lunasa_hegemony = new General(this, "lunasa_hegemony", "wei", 4, false);
    lunasa_hegemony->addSkill(new HezouHegemony);
    lunasa_hegemony->addSkill(new XianlingHegemony);
    lunasa_hegemony->addCompanion("merlin_hegemony");
    lunasa_hegemony->addCompanion("lyrica_hegemony");

    General *merlin_hegemony = new General(this, "merlin_hegemony", "wei", 3, false);
    merlin_hegemony->addSkill(new JizouHegemony);
    merlin_hegemony->addSkill(new GuanlingHegemony);
    merlin_hegemony->addCompanion("lyrica_hegemony");

    General *lyrica_hegemony = new General(this, "lyrica_hegemony", "wei", 3, false);
    lyrica_hegemony->addSkill("xiezou");
    lyrica_hegemony->addSkill(new JianlingHegemony);
        

    General *alice_hegemony = new General(this, "alice_hegemony", "wei", 4, false);
    alice_hegemony->addSkill("zhanzhen");
    alice_hegemony->addSkill("renou");
    alice_hegemony->addCompanion("shanghai_hegemony");

    General *chen_hegemony = new General(this, "chen_hegemony", "wei", 3, false);
    chen_hegemony->addSkill("qimen");
    chen_hegemony->addSkill(new DunjiaHegemony);
    chen_hegemony->addSkill("#qimen-dist");
    chen_hegemony->addSkill("#qimen-prohibit");

    General *letty_hegemony = new General(this, "letty_hegemony", "wei", 4);
    letty_hegemony->addSkill(new HanboHegemony);
    letty_hegemony->addSkill(new DongzhiHegemony);
    letty_hegemony->addCompanion("cirno_hegemony");

    General *lilywhite_hegemony = new General(this, "lilywhite_hegemony", "wei", 3);
    lilywhite_hegemony->addSkill(new BaochunHegemony);
    lilywhite_hegemony->addSkill(new ChunhenHegemony);

    General *shanghai_hegemony = new General(this, "shanghai_hegemony", "wei", 3);
    shanghai_hegemony->addSkill(new ZhancaoHegemony);
    shanghai_hegemony->addSkill(new MocaoHegemony);

    General *youki_hegemony = new General(this, "youki_hegemony", "wei", 4, true);
    youki_hegemony->addSkill("shoushu");
    youki_hegemony->addSkill("yujian");
    youki_hegemony->addCompanion("youmu_hegemony");

    General *cirno_hegemony = new General(this, "cirno_hegemony", "wei", 3);
    cirno_hegemony->addSkill(new DongjieHegemony);
    cirno_hegemony->addSkill(new BingpoHgemony);
    cirno_hegemony->addCompanion("daiyousei_hegemony");

    General *daiyousei_hegemony = new General(this, "daiyousei_hegemony", "wei", 3);
    daiyousei_hegemony->addSkill(new JuxianHegemony);
    daiyousei_hegemony->addSkill(new BanyueHegemony);


    //addMetaObject<TuizhiHegemonyCard>();
    addMetaObject<NiaoxiangSummon>();

    addMetaObject<QingtingHegemonyCard>();
    addMetaObject<ShowShezhengCard>();
    addMetaObject<XingyunHegemonyCard>();
    
    addMetaObject<ChunhenHegemonyCard>();
    addMetaObject<MocaoHegemonyCard>();
    addMetaObject<DongzhiHegemonyCard>();
    addMetaObject<BanyueHegemonyCard>();

    skills <<  new GameRule_AskForGeneralShowHead << new GameRule_AskForGeneralShowDeputy << new GameRule_AskForArraySummon  << new ShezhengAttach; //<< new ShihuiHegemonyVS
}

ADD_PACKAGE(HegemonyGeneral)
