#include "thndj.h"

#include "general.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "client.h"



class Rexue : public TriggerSkill
{
public:
    Rexue() : TriggerSkill("rexue")
    {
        events << EventPhaseChanging << EventPhaseStart << Death;
        frequency = Compulsory;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return;
            //room->setTag("rexue_count", true);
            ServerPlayer *source = room->findPlayerBySkillName("rexue");
            if (room->getCurrent() == source) {
                if (source->getMark("touhou-extra") > 0)
                    return;
                room->setPlayerFlag(source, "rexueDeath");
            }
        }
        
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                if (player->hasFlag("rexueDeath")) {
                    room->setPlayerFlag(player, "-rexueDeath");
                    return QStringList();
                }

                if (player->getMark("touhou-extra") > 0) {
                    player->removeMark("touhou-extra");
                    return QStringList();
                }
                if (room->canInsertExtraTurn())
                    player->tag["RexueInvoke"] = QVariant::fromValue(true);
            }
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::NotActive) {
            bool rexue = player->tag["RexueInvoke"].toBool();
            player->tag["RexueInvoke"] = QVariant::fromValue(false);
            if (rexue && player->getHp() == 1) {
                room->touhouLogmessage("#TriggerSkill", player, "rexue");
                room->notifySkillInvoked(player, "rexue");
                room->recover(player, RecoverStruct());

                if (room->canInsertExtraTurn()) {
                   return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->touhouLogmessage("#touhouExtraTurn", player, objectName());
        player->gainAnExtraTurn();
       
        return false;
    }
};


class Sidou : public TriggerSkill
{
public:
    Sidou() : TriggerSkill("sidou")
    {
        events << EventPhaseStart;
    }

    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (player->getPhase() == Player::Start) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (player->canDiscard(p, "ej", "sidou"))
                    return QStringList(objectName());
            }
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (player->canDiscard(p, "ej", "sidou"))
            targets << p;
        }

        
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@sidou_target", true, true);
        if (target) {
            QList<int> disable;
            foreach (const Card *equip, target->getEquips()) {
                if (equip->isKindOf("Weapon"))
                    disable << equip->getId();
            }
            int card_id = room->askForCardChosen(player, target, "je", objectName(), false, Card::MethodDiscard, disable);
            room->throwCard(card_id, target, player);
            player->drawCards(1);
            room->damage(DamageStruct(objectName(), player, player, 1, DamageStruct::Fire));
        }
            
        return false;
    }
};

class TymhWuyu : public TriggerSkill
{
public:
    TymhWuyu() : TriggerSkill("tymhwuyu$")
    {
        events << Death;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DeathStruct death = data.value<DeathStruct>();
        if (death.who == player || !player->hasLordSkill(objectName()) )
            return QStringList();
        if (death.damage != NULL) {
            if (death.damage->from == NULL || death.damage->from != player)
                return QStringList(objectName());
        } else {
            return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *source) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.damage != NULL) {
            death.damage->from = source;
        } else {
            DamageStruct *die = new DamageStruct();
            die->from = source;
            death.damage = die;
        }
        data = QVariant::fromValue(death);
        
        room->touhouLogmessage("#TriggerSkill", source, "tymhwuyu");
        room->notifySkillInvoked(player, objectName());
        return false;
    }
};



class Huanyue : public TriggerSkill
{
public:
    Huanyue() : TriggerSkill("huanyue")
    {
        events << DamageInflicted;
    }
   virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card == NULL || !damage.card->isNDTrick())
            return TriggerList();
        TriggerList skill_list;
        QList<ServerPlayer *> kaguyas = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *kaguya, kaguyas) {
            if (kaguya != damage.to && damage.to->canDiscard(kaguya, "h")) 
                skill_list.insert(kaguya, QStringList(objectName()));
        }
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *, QVariant &data, ServerPlayer *source) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt = "target:" + damage.to->objectName() + ":" + damage.card->objectName();
        source->tag["huanyue_damage"] = data;
        return room->askForSkillInvoke(source, objectName(), prompt);
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *, QVariant &data, ServerPlayer *source) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), damage.to->objectName());
        int card_id = room->askForCardChosen(damage.to, source, "h", objectName(), false, Card::MethodDiscard);
        room->throwCard(card_id, source, damage.to);
        if (Sanguosha->getCard(card_id)->isBlack()) {
            QList<ServerPlayer *>logto;
            logto << damage.to;
            room->touhouLogmessage("#huanyue_log", damage.from, QString::number(damage.damage), logto, QString::number(damage.damage + 1));
            damage.damage = damage.damage + 1;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

class Sizhai : public TriggerSkill
{
public:
    Sizhai() : TriggerSkill("sizhai")
    {
        events << EventPhaseStart << CardUsed << CardResponded;
        frequency = Frequent;
    }
    
    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardUsed || triggerEvent == CardResponded) {
            ServerPlayer *current = room->getCurrent();
            if (player != current)
                return;
            if (triggerEvent == CardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.card->isKindOf("BasicCard") || use.card->isKindOf("TrickCard"))
                    player->setFlags("sizhai");
            } else if (triggerEvent == CardResponded) {
                CardStar card_star = data.value<CardResponseStruct>().m_card;
                if (card_star->isKindOf("BasicCard") || card_star->isKindOf("TrickCard"))
                    player->setFlags("sizhai");
            }
        }
    }
    
    
    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &) const
    {  
        TriggerList skill_list;
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *current = room->getCurrent();
            if (current && current->getPhase() == Player::Finish) {
                if (!current->hasFlag("sizhai")) {
                    
                    QList<ServerPlayer *> kaguyas = room->findPlayersBySkillName(objectName());
                    foreach (ServerPlayer *kaguya, kaguyas) {
                        if (kaguya != current) 
                            skill_list.insert(kaguya, QStringList(objectName()));
                    }
                }
            }
        } 
        return skill_list;
    }
    
    
    virtual bool cost(TriggerEvent , Room *room, ServerPlayer *, QVariant &, ServerPlayer *s) const
    {
        ServerPlayer *current = room->getCurrent();
        return room->askForSkillInvoke(s, objectName(), "draw:" + current->objectName());
    }
    
    virtual bool effect(TriggerEvent , Room *, ServerPlayer *, QVariant &, ServerPlayer *s) const
    {
        s->drawCards(1);
        return false;
    }
};


HunpoCard::HunpoCard()
{
    will_throw = true;
    target_fixed = true;
    mute = true;
}
void HunpoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->setPlayerProperty(source, "maxhp", source->getMaxHp() + 1);
    room->touhouLogmessage("#GainMaxHp", source, QString::number(1));
    room->touhouLogmessage("#GetHp", source, QString::number(source->getHp()), QList<ServerPlayer *>(), QString::number(source->getMaxHp()));
}
class Hunpo : public OneCardViewAsSkill
{
public:
    Hunpo() : OneCardViewAsSkill("hunpo")
    {
        filter_pattern = ".|.|.|.!";
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return !Self->isJilei(to_select);
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isNude() && player->getMaxHp() < 4;
    }



    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard != NULL) {
            HunpoCard *card = new HunpoCard;
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }
};


class Fanji : public TriggerSkill
{
public:
    Fanji() : TriggerSkill("fanji")
    {
        events << Damaged;
    }
    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from == NULL || damage.from->isDead()
            || damage.to->isDead() || damage.from == damage.to)
            return TriggerList();
        TriggerList skill_list;
        QList<ServerPlayer *> youmus = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *youmu, youmus) {
            if (youmu == damage.from )
                continue;
            if (youmu == damage.to)
                skill_list.insert(youmu, QStringList(objectName()));
            else if ( youmu->inMyAttackRange(damage.to) && youmu->getMaxHp() > 1)
                skill_list.insert(youmu, QStringList(objectName()));
        }
        return skill_list;
    }
    
    
     virtual bool cost(TriggerEvent , Room *room, ServerPlayer *, QVariant &data, ServerPlayer *source) const
    {    
        DamageStruct damage = data.value<DamageStruct>();
        source->tag["fanji_damage"] = data;
        QString prompt = "target:" + damage.from->objectName() + ":" + damage.to->objectName();
        return room->askForSkillInvoke(source, objectName(), prompt);
    }
     
     virtual bool effect(TriggerEvent , Room *room, ServerPlayer *, QVariant &data, ServerPlayer *source) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), damage.from->objectName());
        if (damage.to == source) {
            room->damage(DamageStruct("fanji", source, damage.from));

        }
        else {
            room->loseMaxHp(source, 1);
            room->damage(DamageStruct("fanji", source, damage.from));
        }
        return false;
    }
};



class Liangzi : public TriggerSkill
{
public:
    Liangzi() : TriggerSkill("liangzi")
    {
        events << CardUsed << CardResponded;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("BasicCard"))
                return QStringList(objectName());
        } else if (triggerEvent == CardResponded) {
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isKindOf("BasicCard"))
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        LogMessage log;
        log.from = player;
        log.arg = "liangzi";
        log.type = "#TriggerSkill";
        room->sendLog(log);
        room->notifySkillInvoked(player, "liangzi");

        player->setChained(!player->isChained());
        Sanguosha->playSystemAudioEffect("chained");
        room->broadcastProperty(player, "chained");
        room->setEmotion(player, "chain");
        return false;
    }
};

class Kexue : public TargetModSkill
{
public:
    Kexue() : TargetModSkill("kexue")
    {
        pattern = "Slash";
    }
    virtual int getDistanceLimit(const Player *from, const Card *) const
    {
        if (from->getPhase() == Player::Play  && from->hasSkill(objectName()) && from->isChained())
            return 1000;
        else
            return 0;
    }


    virtual int getExtraTargetNum(const Player *player, const Card *) const
    {
        if (player->getPhase() == Player::Play  && player->hasSkill(objectName()) && player->isChained())
            return 1000;
        else
            return 0;
    }
};
class KexueEffect : public TriggerSkill
{
public:
    KexueEffect() : TriggerSkill("#kexue-effect")
    {
        events << PreCardUsed;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.to.length() > 1 && player->isChained())
                room->notifySkillInvoked(player, "kexue");
        }
        return QStringList();
    }
};


THNDJPackage::THNDJPackage()
    : Package("thndj")
{
    General *mokou_ndj = new General(this, "mokou_ndj$", "zhu", 4, false);
    mokou_ndj->addSkill(new Rexue);
    mokou_ndj->addSkill(new Sidou);
    mokou_ndj->addSkill(new TymhWuyu);
    related_skills.insertMulti("rexue", "#rexue_count");

    General *kaguya_ndj = new General(this, "kaguya_ndj", "zhu", 3, false);
    kaguya_ndj->addSkill(new Huanyue);
    kaguya_ndj->addSkill(new Sizhai);


    General *youmu_ndj = new General(this, "youmu_ndj", "yym", 3, false);
    youmu_ndj->addSkill(new Hunpo);
    youmu_ndj->addSkill(new Fanji);

    //General *merry_ndj = new General(this, "merry_ndj", "wai", 1, false);

    General *renko_ndj = new General(this, "renko_ndj", "wai", 4, false);
    renko_ndj->addSkill(new Liangzi);
    renko_ndj->addSkill(new Kexue);
    renko_ndj->addSkill(new KexueEffect);
    related_skills.insertMulti("kexue", "#kexue-effect");

    addMetaObject<HunpoCard>();
}

ADD_PACKAGE(THNDJ)

