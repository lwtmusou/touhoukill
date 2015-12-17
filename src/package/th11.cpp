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

    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *, QVariant &data) const
    {   
        DamageStruct damage = data.value<DamageStruct>();
        TriggerList skill_list;
        QList<ServerPlayer *> satoris = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *satori, satoris) {
            if (damage.from && damage.from != satori && damage.card  && !damage.from->isKongcheng()
            && damage.to != damage.from && damage.to->isAlive()
            && (satori->inMyAttackRange(damage.to) || damage.to == satori) )
                skill_list.insert(satori, QStringList(objectName()));
        }
        return skill_list;
    }

    virtual bool cost(TriggerEvent , Room *, ServerPlayer *, QVariant &data, ServerPlayer *source) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt = "show:" + damage.from->objectName() + ":" + damage.to->objectName() + ":" + damage.card->objectName();
        source->tag["xiangqi_from"] = QVariant::fromValue(damage.from);
        source->tag["xiangqi_to"] = QVariant::fromValue(damage.to);
        source->tag["xiangqi_card"] = QVariant::fromValue(damage.card);
        return source->askForSkillInvoke("xiangqi", prompt); 
    }
    
    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *, QVariant &data, ServerPlayer *source) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), damage.from->objectName());

        int id = room->askForCardChosen(source, damage.from, "h", objectName());
        room->showCard(damage.from, id);
        Card *showcard = Sanguosha->getCard(id);
        bool same = false;
        if (showcard->getTypeId() == damage.card->getTypeId())
            same = true;

        if (same && damage.to != source) {
            room->throwCard(id, damage.from, source);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), damage.to->objectName());

            room->damage(DamageStruct("xiangqi", source, damage.to));
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

    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {   
        if (!TriggerSkill::triggerable(player)) return QStringList();
         CardUseStruct use = data.value<CardUseStruct>();
        if (!player->hasLordSkill(objectName()))
            return QStringList();
        if (use.card == NULL || use.from == NULL || !use.card->isKindOf("Slash"))
            return QStringList();
        foreach (ServerPlayer *p, room->getLieges("dld", player)) {
            if (p != use.from && use.from->canSlash(p, use.card, false) && !use.to.contains(p))
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        return player->askForSkillInvoke(objectName(), data);
    }
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
    
        CardUseStruct use = data.value<CardUseStruct>();
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getLieges("dld", player)) {
            if (p == use.from)
                continue;
            if (use.from->canSlash(p, use.card, false) && !use.to.contains(p))
                targets << p;
        }
        
        foreach (ServerPlayer *p, targets) {
            room->setTag("huzhu_target", QVariant::fromValue(player));
            QString prompt = "slashtarget:" + use.from->objectName() + ":" + player->objectName() + ":" + use.card->objectName();
            if (p->askForSkillInvoke("huzhu_change", prompt)) {
                room->removeTag("huzhu_target");
                use.to << p;
                use.to.removeOne(player);
                data = QVariant::fromValue(use);

                QList<ServerPlayer *> logto;
                logto << player;
                room->touhouLogmessage("$CancelTarget", use.from, use.card->objectName(), logto);
                logto << p;
                logto.removeOne(player);
                room->touhouLogmessage("#huzhu_change", use.from, use.card->objectName(), logto);

                room->getThread()->trigger(TargetConfirming, room, p, data);
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

    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        damage.from = NULL;
        damage.by_user = false;

        room->touhouLogmessage("#TriggerSkill", player, "wunian");
        room->notifySkillInvoked(player, objectName());
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

    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *, QVariant &data) const
    {   
        DamageStruct damage = data.value<DamageStruct>();
        TriggerList skill_list;
        QList<ServerPlayer *> utsuhos = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *utsuho, utsuhos) {
            if (!utsuho->isKongcheng() && damage.nature == DamageStruct::Fire)
                skill_list.insert(utsuho, QStringList(objectName()));
        }
        return skill_list;
    }

    
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ldlk) const
    {
        QStringList    yaobanTargets;
        foreach(ServerPlayer *p, room->getOtherPlayers(player))
            yaobanTargets << p->objectName();

        DamageStruct damage = data.value<DamageStruct>();    
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

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();

        
        if (use.card->isKindOf("Slash") && !use.card->isKindOf("FireSlash"))
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();

        room->touhouLogmessage("#TriggerSkill", player, "here");
        room->notifySkillInvoked(player, objectName());
        if (use.from )
            room->touhouLogmessage("#HereFilter", use.from, "here");
        FireSlash *new_slash = new FireSlash(use.card->getSuit(), use.card->getNumber());
        if (use.card->getSubcards().length() > 0)
            new_slash->addSubcards(use.card->getSubcards());
        else {//use.from is ai...
            int id = use.card->getEffectiveId();
            if (id > -1)
                new_slash->addSubcard(id);
        }

        new_slash->setSkillName(use.card->getSkillName());

        QStringList flags = use.card->getFlags();
        foreach(QString flag, flags)
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

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from->isAlive()
            && damage.from != player) {
            FireSlash *slash = new FireSlash(Card::NoSuit, 0);
            if (player->isCardLimited(slash, Card::MethodUse) || !player->canSlash(damage.from, slash, false))
                return QStringList();
            return QStringList(objectName());
        }
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt = "target:" + damage.from->objectName();
        player->tag["yuanling"] = QVariant::fromValue(damage.from);
        return room->askForSkillInvoke(player, objectName(), prompt);
    }
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        FireSlash *slash = new FireSlash(Card::NoSuit, 0); 
        slash->setSkillName("_" + objectName());
        room->useCard(CardUseStruct(slash, player, damage.from), false);
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

    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        ServerPlayer *victim = room->getCurrentDyingPlayer();
        if (victim == NULL || player == victim)
            return QStringList();
            
        return QStringList(objectName());
    }
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        ServerPlayer *victim = room->getCurrentDyingPlayer();
        //just for ai
        player->tag["songzang_dying"] = data;

        const Card *c = room->askForCard(player, ".|spade", "@songzang:" + victim->objectName(), data, objectName());
        if (c) 
            return true;
        return false;
    }
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *victim = room->getCurrentDyingPlayer();        
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), victim->objectName());

        DamageStruct damage;
        damage.from = player;
        room->killPlayer(victim, &damage);
        room->setPlayerFlag(victim, "-Global_Dying");
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

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (player->hasFlag("hitAfterMissed"))
            return QStringList();
        if (!effect.to->isAlive())
            return QStringList();
        if (player->canDiscard(player, "h"))
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        player->tag["guaili_target"] = QVariant::fromValue(effect.to);
        const Card *card = room->askForCard(player, ".|red|.|hand", "@guaili:" + effect.to->objectName(), data, objectName());
        if (card)
            return true;
        return false;
    }
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        player->drawCards(1);
        room->setPlayerFlag(player, "hitAfterMissed");
        room->slashResult(effect, NULL);
        return false;
    }
};

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
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("jiuhao");
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
    
     virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        //if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if ( player->getPhase() == Player::Play
            &&(use.card->isKindOf("Peach") || use.card->isKindOf("Analeptic"))) {
                room->setPlayerFlag(player, "jiuhao");
            }
            if (use.card->getSkillName() == "jiuhao")
                room->setPlayerFlag(player, "jiuhaoused");
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play) {
                room->setPlayerFlag(player, "-jiuhao");
                room->setPlayerFlag(player, "-jiuhaoused");
            }
        }
    }
    
    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *, QVariant &, ServerPlayer* &) const
    {
        //only for record
        return QStringList();
    }
};

class JiuhaoTargetMod : public TargetModSkill
{
public:
    JiuhaoTargetMod() : TargetModSkill("#jiuhaoTargetMod")
    {
        frequency = NotFrequent;
        pattern = "Slash";
    }

    virtual int getResidueNum(const Player *from, const Card *card) const
    {
        if (from->hasSkill("jiuhao") && card->getSkillName() == "jiuhao")
            return 1;
        else
            return 0;
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

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Duel"))
            return QStringList(objectName());
        return QStringList();
    }
    
    virtual bool cost(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
       return player->askForSkillInvoke(objectName(), QVariant::fromValue(damage));
    }
    
    virtual void onDamaged(ServerPlayer *player, const DamageStruct &) const
    {
        player->drawCards(1);    
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
        events << TargetConfirming;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.to.contains(player)) {
            if (use.from  && use.from->isAlive()) 
                return QStringList(objectName());
        }
        return QStringList();
    }
    
    
    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
     
        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.from->objectName());

        SupplyShortage *supply = new SupplyShortage(Card::NoSuit, 0);
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
        events << CardsMoveOneTime << BeforeCardsMove << EventPhaseChanging;
        view_as_skill = new ChuanranVS;
    }

    
    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *yamame, QVariant &data) const
    {
        //need check null current?
        ServerPlayer *current = room->getCurrent();
        if (!yamame->hasSkill(objectName()) || !current || current->getPhase() != Player::Judge)
            return ;
        if (triggerEvent == BeforeCardsMove) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!move.from  || move.from != current)
                return;
            if ((move.from_places.contains(Player::PlaceDelayedTrick) || move.origin_from_places.contains(Player::PlaceDelayedTrick))
                ) {
                QVariantList ids1 = yamame->tag["chuanran"].toList();
                foreach (int id, move.card_ids) {
                    if (room->getCardPlace(id) == Player::PlaceDelayedTrick && !ids1.contains(id)) {
                        ids1 << id;
                    }
                }
                yamame->tag["chuanran"] = ids1;
            }
        } 
    }
    
    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        ServerPlayer *current = room->getCurrent();
        if (!player->hasSkill(objectName()) || !current || current->getPhase() != Player::Judge)
            return QStringList();
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Judge) {
                player->tag.remove("chuanran");
            }
        }else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QVariantList chuanran_ids = player->tag["chuanran"].toList();
            foreach (QVariant card_data, chuanran_ids) {
                int id = card_data.toInt();
                if (room->getCardPlace(id) == Player::DiscardPile && move.card_ids.contains(id)) {
                    return QStringList(objectName());
                }
            }
        }
        return QStringList();
    }

    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *source, QVariant &data, ServerPlayer *) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (triggerEvent == CardsMoveOneTime) {
            QVariantList chuanran_ids = source->tag["chuanran"].toList();
            QList<int> all;
            foreach (QVariant card_data, chuanran_ids) {
                int id = card_data.toInt();
                if (room->getCardPlace(id) == Player::DiscardPile && move.card_ids.contains(id)) {
                    all << id;
                    chuanran_ids.removeOne(card_data);
                }
            }
            source->tag["chuanran"] = chuanran_ids;
            foreach (int id, all) {
                
                QList<ServerPlayer *>others;
                QStringList chuanranTargets;
                QString trickname;
                if (Sanguosha->getCard(id)->isKindOf("DelayedTrick"))
                    trickname = Sanguosha->getCard(id)->objectName();
                else
                    trickname = "supply_shortage";
                foreach (ServerPlayer *p, room->getOtherPlayers(room->getCurrent())) {

                    if (!p->containsTrick(trickname)) {
                        others << p;
                        chuanranTargets << p->objectName();
                    }
                }
                if (!chuanranTargets.isEmpty()) {
                    room->setPlayerProperty(source, "chuanran", chuanranTargets.join("+"));
                    source->tag["chuanran_cardname"] == QVariant::fromValue(trickname);
                    source->tag["chuanran_id"] = QVariant::fromValue(id);
                    room->askForUseCard(source, "@@chuanran", "@chuanran:" + trickname);
                    room->setPlayerProperty(source, "chuanran", QVariant());
                    
                }

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

    virtual QStringList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (!p->isKongcheng() && p->getCards("j").length() > 0)
                return QStringList(objectName());
        }
        return QStringList();
        
    }
    
    virtual void onDamaged(ServerPlayer *player, const DamageStruct &) const
    {
        Room *room = player->getRoom();
        QList<ServerPlayer *> listt;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (!p->isKongcheng() && p->getCards("j").length() > 0)
                listt << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(player, listt, objectName(), "@rebing", true, true);

        if (target) {
            int id = room->askForCardChosen(player, target, "h", objectName());
            room->obtainCard(player, id, false);
        }
    }
};



class Diaoping : public TriggerSkill
{
public:
    Diaoping() : TriggerSkill("diaoping")
    {
        events << TargetSpecified;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {   
        TriggerList skill_list;
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("Slash"))
                return skill_list;
            if (!use.from || use.from->isDead())
                return skill_list;
            QList<ServerPlayer *> kisumes = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *kisume, kisumes) {
                foreach (ServerPlayer *p, use.to) {
                    if (kisume->inMyAttackRange(p) || kisume == p){
                        if (kisume->getHandcardNum() > 0 && use.from->getHandcardNum() > 0 && kisume != use.from)
                            skill_list.insert(kisume, QStringList(objectName()));
                    }
                }
            }
        }
        return skill_list;
    }
    
    virtual bool cost(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *skillowner) const
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (skillowner->getHandcardNum() > 0 && use.from->getHandcardNum() > 0) {
                bool good_result = false;
                QString prompt = "slashtarget:" + use.from->objectName() + ":" + use.card->objectName();
                skillowner->tag["diaoping_slash"] = data;
                while (use.from->isAlive() && good_result == false && skillowner->getHandcardNum() > 0 && use.from->getHandcardNum() > 0) {

                    if (!room->askForSkillInvoke(skillowner, "diaoping", prompt))
                        return false;
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, skillowner->objectName(), use.from->objectName());

                    if (skillowner->pindian(use.from, "diaoping", NULL)) {
                        use.from->turnOver();
                        good_result = true;
                        use.nullified_list << "_ALL_TARGETS";
                        data = QVariant::fromValue(use);
                        break;
                    }
                }
            }
        } 
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



class Cuiji : public DrawCardsSkill
{
public:
    Cuiji() : DrawCardsSkill("cuiji")
    {

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

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        if (do_cuiji(player)) {
            n = n - 1;
            if (do_cuiji(player))
                n = n - 1;
        }
        return n;
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
    yugi->addSkill(new JiuhaoTargetMod);;
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
    addMetaObject<ChuanranCard>();
}

ADD_PACKAGE(TH11)

