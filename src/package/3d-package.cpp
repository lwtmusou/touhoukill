#include "3d-package.h"
#include "skill.h"
#include "clientplayer.h"
#include "engine.h"
#include "wind.h"
#include "standard.h"

class SanD1Chishen: public TriggerSkill{
public:
    SanD1Chishen(): TriggerSkill("sand1chishen"){
        events << AfterDrawNCards;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (!player->isKongcheng() && player->askForSkillInvoke(objectName())){
            player->throwAllHandCards();
        }
        return false;
    }
};

SanD1XinveCard::SanD1XinveCard(){
    mute = true;
    will_throw = false;
}

bool SanD1XinveCard::targetFixed() const{
    const Card *c = Self->tag["sand1xinve"].value<const Card *>();
    Card *_touse = Sanguosha->cloneCard(c->objectName(), Card::NoSuit, 0);
    if (_touse == NULL)
        return false;
    _touse->deleteLater();
    return _touse->targetFixed();
}

bool SanD1XinveCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    const Card *c = Self->tag["sand1xinve"].value<const Card *>();
    Card *_touse = Sanguosha->cloneCard(c->objectName(), Card::NoSuit, 0);
    if (_touse == NULL)
        return false;
    _touse->deleteLater();
    return _touse->targetFilter(targets, to_select, Self);
}

bool SanD1XinveCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    const Card *c = Self->tag["sand1xinve"].value<const Card *>();
    Card *_touse = Sanguosha->cloneCard(c->objectName(), Card::NoSuit, 0);
    if (_touse == NULL)
        return false;
    _touse->deleteLater();
    return _touse->targetsFeasible(targets, Self);
}

const Card *SanD1XinveCard::validate(CardUseStruct &cardUse) const{
    Card *_touse = Sanguosha->cloneCard(user_string, Card::NoSuit, 0);
    _touse->setSkillName("sand1xinve");
    bool available = true;
    foreach (ServerPlayer *to, cardUse.to){
        if (cardUse.from->isProhibited(to, _touse)){
            available = false;
            break;
        }
    }
    available = available && _touse->isAvailable(cardUse.from);
    _touse->deleteLater();
    if (!available) return NULL;
    return _touse;
}

class SanD1Xinve: public ZeroCardViewAsSkill{
public:
    SanD1Xinve(): ZeroCardViewAsSkill("sand1xinve"){

    }

    virtual QDialog *getDialog() const{
        return GuhuoDialog::getInstance("sand1xinve");
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->isKongcheng() && !player->hasUsed("SanD1XinveCard");
    }

    virtual const Card *viewAs() const{
        const Card *c = Self->tag["sand1xinve"].value<const Card *>();
        if (c != NULL){
            SanD1XinveCard *xinve = new SanD1XinveCard;
            xinve->setUserString(c->objectName());
            return xinve;
        }
        
        return NULL;
    }
};

class SanD1Mingzhan: public TriggerSkill{
public:
    SanD1Mingzhan(): TriggerSkill("sand1mingzhan"){
        events << TargetConfirmed << DamageDone << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return player != NULL && player->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed && TriggerSkill::triggerable(player)){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from == player && use.card != NULL && use.card->isKindOf("Slash") && player->askForSkillInvoke(objectName(), data)){
                QString choice = room->askForChoice(player, objectName(), "damage+nodamage", data);
                player->setFlags("sand1mingzhan_invoke" + use.card->toString());
                player->tag["sand1mingzhan_predict" + use.card->toString()] = (choice == "damage");
                player->tag["sand1mingzhan_real" + use.card->toString()] = false;
            }
        }
        else if (triggerEvent == DamageDone){
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != NULL && damage.card->isKindOf("Slash") && damage.from->hasFlag("sand1mingzhan_invoke" + damage.card->toString())){
                damage.from->tag["sand1mingzhan_real" + damage.card->toString()] = true;
            }
        }
        else if (triggerEvent == CardFinished && TriggerSkill::triggerable(player)){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from == player && use.card != NULL && use.card->isKindOf("Slash") && player->hasFlag("sand1mingzhan_invoke" + use.card->toString())){
                player->setFlags("-sand1mingzhan_invoke" + use.card->toString());
                if (player->tag["sand1mingzhan_predict" + use.card->toString()].toBool() == player->tag["sand1mingzhan_real" + use.card->toString()].toBool()){
                    if (player->askForSkillInvoke(objectName(), "drawcard"))
                        player->drawCards(1);
                }

                player->tag.remove("sand1mingzhan_predict" + use.card->toString());
                player->tag.remove("sand1mingzhan_real" + use.card->toString());
            }
        }

        return false;
    }
};

class SanD1Xielu: public TriggerSkill{
public:
    SanD1Xielu(): TriggerSkill("sand1xielu"){
        events << TargetConfirmed << SlashEffected << BeforeCardsMove;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed && TriggerSkill::triggerable(player)){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("Slash")){
                foreach (ServerPlayer *to, use.to){
                    if (player->inMyAttackRange(to) && player->askForSkillInvoke(objectName(), QVariant::fromValue(to))){
                        bool can_discard_horse = false;
                        foreach (const Card *c, player->getEquips()){
                            if (c->isKindOf("Horse") && player->canDiscard(player, c->getEffectiveId())){
                                can_discard_horse = true;
                                break;
                            }
                        }
                        QString choice = "losehp";
                        if (can_discard_horse)
                            choice = room->askForChoice(player, objectName(), "losehp+discardhorse", QVariant::fromValue(to));

                        if (choice == "discardhorse")
                            room->askForCard(player, ".Horse!", "@sand1xielu-discard");
                        else {
                            room->loseHp(player, 1);
                            player->setFlags("sand1xielulosehp");
                            player->tag["sand1xielu_losehp"] = use.card->toString();
                            
                        }
                        room->setPlayerFlag(to, "sand1xieluinvoke");
                        room->setCardFlag(use.card, "sand1xieluinvoke");
                    }
                }
            }
        }
        else if (triggerEvent == SlashEffected){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash != NULL && effect.slash->hasFlag("sand1xieluinvoke") && effect.to->hasFlag("sand1xieluinvoke")){
                room->setPlayerFlag(effect.to, "-sand1xieluinvoke");
                //danlaoavoid log
                return true;
            }
        }
        else if (triggerEvent == BeforeCardsMove && TriggerSkill::triggerable(player) && player->hasFlag("sand1xielulosehp")){
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.reason.m_reason == CardMoveReason::S_REASON_USE && move.from_places.contains(Player::PlaceTable) && move.to_place == Player::DiscardPile){
                const Card *original_card = Card::Parse(player->tag["sand1xielu_losehp"].toString());
                bool real_slash = false;
                if (original_card->isVirtualCard()){
                    if (original_card->getSubcards() == move.card_ids)
                        real_slash = true;
                }
                else if (original_card->getId() == move.card_ids[0])
                    real_slash = true;

                if (real_slash){
                    player->setFlags("-sand1xielulosehp");
                    player->tag.remove("sand1xielu_losehp");

                    move.card_ids.clear();
                    move.from_places.clear();
                    data = QVariant::fromValue(move);

                    room->obtainCard(player, original_card);
                }
            }
        }
        return false;
    }
};

SanD1BianzhenCard::SanD1BianzhenCard(){

}

bool SanD1BianzhenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty();
}

Player::Phase SanD1BianzhenCard::getPhaseFromString(const QString &s){
    if (s == "start")
        return Player::Start;
    else if (s == "judge")
        return Player::Judge;
    else if (s == "draw")
        return Player::Draw;
    else if (s == "discard")
        return Player::Discard;
    else if (s == "finish")
        return Player::Finish;
    else
        return Player::PhaseNone;

    return Player::PhaseNone;
}

void SanD1BianzhenCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    if (subcards.isEmpty())
        room->loseHp(effect.from);

    QString choice = room->askForChoice(effect.from, "sand1bianzhen", "start+judge+draw+discard+finish");

    Player::Phase phase = getPhaseFromString(choice);

    effect.from->setFlags("sand1bianzhen_used");
    effect.from->tag["sand1bianzhen_target"] = QVariant::fromValue(effect.to);
    effect.to->tag["sand1bianzhen_phase"] = static_cast<int>(phase);
}

class SanD1BianzhenVS: public ViewAsSkill{
public:
    SanD1BianzhenVS(): ViewAsSkill("sand1bianzhen"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("SanD1BianzhenCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return selected.length() < 2;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.isEmpty() || cards.length() == 2){
            SanD1BianzhenCard *c = new SanD1BianzhenCard;
            c->addSubcards(cards);
            return c;
        }
        return NULL;
    }
};

class SanD1Bianzhen: public TriggerSkill{
public:
    SanD1Bianzhen(): TriggerSkill("sand1bianzhen"){
        events << EventPhaseStart;
        view_as_skill = new SanD1BianzhenVS;
    }

    virtual int getPriority(TriggerEvent triggerEvent) const{
        return 1;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->hasFlag("sand1bianzhen_used") && player->getPhase() == Player::NotActive){
            ServerPlayer *target = player->tag["sand1bianzhen_target"].value<ServerPlayer *>();
            player->tag.remove("sand1bianzhen_target");
            if (target == NULL)
                return false;

            Player::Phase phase = static_cast<Player::Phase>(target->tag["sand1bianzhen_phase"].toInt());
            target->tag.remove("sand1bianzhen_phase");

            target->setPhase(phase);
            room->broadcastProperty(target, "phase");
            RoomThread *thread = room->getThread();
            try{
                if (!thread->trigger(EventPhaseStart, room, target))
                    thread->trigger(EventPhaseProceeding, room, target);
                thread->trigger(EventPhaseEnd, room, target);

                target->setPhase(Player::NotActive);
                room->broadcastProperty(target, "phase");
            }
            catch (TriggerEvent errorevent){
                if (errorevent == TurnBroken || errorevent == StageChange){
                    target->setPhase(Player::NotActive);
                    room->broadcastProperty(target, "phase");
                }
                throw errorevent;
            }
        }
        return false;
    }
};

class SanD1Congwen: public PhaseChangeSkill{
public:
    SanD1Congwen(): PhaseChangeSkill("sand1congwen"){
        frequency = Skill::Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getMark(objectName()) == 0
            && target->isKongcheng();
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        Room *room = target->getRoom();
        room->notifySkillInvoked(target, objectName());
        room->broadcastSkillInvoke(objectName());
        room->doLightbox("$sand1congwen");

        QString choice = "draw";
        if (target->isWounded())
            choice = room->askForChoice(target, objectName(), "draw+recover");

        if (choice == "draw")
            target->drawCards(2);
        else {
            RecoverStruct recover;
            recover.who = target;
            room->recover(target, recover);
        }

        if (room->changeMaxHpForAwakenSkill(target)){
            room->addPlayerMark(target, objectName());
            room->acquireSkill(target, "wuyan");
        }
        return false;
    }
};

class SanD1Xianxi: public TargetModSkill{
public:
    SanD1Xianxi(): TargetModSkill("sand1xianxi"){
        pattern = "slash";
    }

    virtual int getExtraTargetNum(const Player *from, const Card *card) const{
        if (from->hasSkill(objectName()) && from->getPhase() == Player::Play)
            return 99;

        return 0;
    }
};

class SanD1XianxiEffect1: public TriggerSkill{
public:
    SanD1XianxiEffect1(): TriggerSkill("#sand1xianxi1"){
        events << CardUsed << CardResponded << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent != CardResponded && TriggerSkill::triggerable(player)){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from->hasSkill("sand1xianxi") && use.to.length() > 1){
                if (triggerEvent == CardUsed){
                    QVariantMap xianxi_list;
                    for (int i = 0; i <= use.to.length(); i ++)
                        xianxi_list[use.to[i]->objectName()] = false;
                    use.from->tag["Xianxi_" + use.card->toString()] = xianxi_list;
                    use.from->setFlags("xianxi_using");
                }
                else {
                    use.from->tag.remove("xianxi_" + use.card->toString());
                    use.from->setFlags("-xianxi_using");
                }
            }
        }
        else if (triggerEvent == CardResponded){
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card->isKindOf("Jink") && resp.m_isUse == true && TriggerSkill::triggerable(resp.m_who)
                    && resp.m_who->hasFlag("xianxi_using") && player->hasFlag("xianxi_target")){
                QVariantMap xianxi_list = resp.m_who->tag["Xianxi_" + player->tag["xianxi_slash"].value<const Card *>()->toString()].toMap();
                xianxi_list[player->objectName()] = true;

                bool used_jink = false, need_discard = false;
                foreach (QVariant b, xianxi_list){
                    if (b.toBool()){
                        if (!used_jink)
                            used_jink = true;
                        else {
                            need_discard = true;
                            break;
                        }
                    }
                }

                if (need_discard){
                    if (!room->askForDiscard(resp.m_who, objectName(), 2, 2, true, true, "@SanD1Xianxi-discard"))
                        room->loseHp(resp.m_who, 1);
                }

            }
        }
        return false;
    }
};

class SanD1XianxiEffect2: public TriggerSkill{
public:
    SanD1XianxiEffect2(): TriggerSkill("#sand1xianxi2"){
        events << SlashProceed;
    }

    virtual int getPriority(TriggerEvent triggerEvent) const{
        return 1;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->hasFlag("xianxi_using")){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            effect.to->setFlags("xianxi_target");
            effect.to->tag["xianxi_slash"] = QVariant::fromValue(effect.slash);
        }
        return false;
    }
};

class SanD1XianxiEffect3: public TriggerSkill{
public:
    SanD1XianxiEffect3(): TriggerSkill("#sand1xianxi3"){
        events << SlashProceed;
    }

    virtual int getPriority(TriggerEvent triggerEvent) const{
        return -2;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->hasFlag("xianxi_using")){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            effect.to->setFlags("xianxi_target");
            effect.to->tag.remove("xianxi_slash");
        }
        return false;
    }
};

SanD1KuangxiCard::SanD1KuangxiCard(){

}

bool SanD1KuangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && Self->distanceTo(to_select) == 1 && to_select != Self && !to_select->isKongcheng();
}

void SanD1KuangxiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    int card_id = room->askForCardChosen(effect.from, effect.to, "h", "sand1kuangxi");
    room->showCard(effect.to, card_id);

    const Card *card = Sanguosha->getCard(card_id);
    if (card->isKindOf("Slash") || card->isKindOf("Jink")){
        effect.from->obtainCard(card);

        Slash *slash = new Slash(Card::SuitToBeDecided, -1);
        slash->addSubcard(card);
        slash->setSkillName("_sand1kuangxi");
        room->useCard(CardUseStruct(slash, effect.from, effect.to));


        room->askForUseCard(effect.from, "@@sand1kuangxi", "@sand1kuangxi", -1, Card::MethodDiscard);
    }
}

class SanD1KuangxiVS: public OneCardViewAsSkill{
public:
    SanD1KuangxiVS(): OneCardViewAsSkill("sand1kuangxi"){
        filter_pattern = ".!";
        response_pattern = "@@sand1kuangxi";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        SanD1KuangxiCard *c = new SanD1KuangxiCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class SanD1Kuangxi: public PhaseChangeSkill{
public:
    SanD1Kuangxi(): PhaseChangeSkill("sand1kuangxi"){

    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() == Player::Start){
            if (target->getRoom()->askForUseCard(target, "@@sand1kuangxi", "@sand1kuangxi", -1, Card::MethodDiscard)){
                target->skip(Player::Judge);
                target->skip(Player::Draw);
            }
        }
        return false;
    }
};

SanD1ShenzhiCard::SanD1ShenzhiCard(){
    will_throw = false;
    handling_method = Card::MethodNone;
}

void SanD1ShenzhiCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    int card_id = subcards.first();
    room->showCard(effect.from, card_id, effect.to);

    QString choice = room->askForChoice(effect.to, objectName(), "obtaindrawlosehp+discardletdraw", card_id);
    if (choice == "obtaindrawlosehp"){
        effect.to->obtainCard(Sanguosha->getCard(card_id), false);
        effect.to->drawCards(1);
        room->loseHp(effect.to);
    }
    else {
        room->throwCard(card_id, effect.from, effect.to);
        if (!effect.to->isNude()){
            int to_gain = room->askForCardChosen(effect.from, effect.to, "he", objectName());
            effect.from->obtainCard(Sanguosha->getCard(to_gain), false);
        }
    }
}

class SanD1Shenzhi: public OneCardViewAsSkill{
public:
    SanD1Shenzhi(): OneCardViewAsSkill("sand1shenzhi"){
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("SanD1ShenzhiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        SanD1ShenzhiCard *c = new SanD1ShenzhiCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class SanD1Zhaolie: public TriggerSkill{
public:
    SanD1Zhaolie(): TriggerSkill("sand1zhaolie"){
        events << TargetConfirmed << CardAsked << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("Slash") && use.to.contains(player) && TriggerSkill::triggerable(player)){
                const Card *slash = use.card;
                if (player->askForSkillInvoke(objectName(), QVariant::fromValue(slash))){
                    QString pattern;
                    if (slash->isBlack())
                        pattern = ".|red";
                    else if (slash->isRed())
                        pattern = ".|black";
                    else
                        pattern = ".";

                    JudgeStruct judge;
                    judge.who = player;
                    judge.pattern = pattern;
                    judge.good = true;
                    
                    room->judge(judge);

                    if (judge.isGood()){
                        player->addMark("SanD1Zhaolie_" + slash->toString());
                    }
                }
            }
        }
        else if (triggerEvent == CardFinished){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("Slash")){
                foreach(ServerPlayer *p, room->getAlivePlayers()){
                    if (p->getMark("SanD1Zhaolie_" + use.card->toString()) > 0)
                        p->setMark("SanD1Zhaolie_" + use.card->toString(), 0);
                }
            }
        }
        else if (triggerEvent == CardAsked && TriggerSkill::triggerable(player)){
            QStringList ask = data.toStringList();
            if (ask.first() == "jink" && player->hasFlag("SanD1Zhaolie_Jink")){
                player->setFlags("-SanD1Zhaolie_Jink");
                Jink *jink = new Jink(Card::NoSuit, 0);
                jink->setSkillName("_sand1zhaolie");
                room->provide(jink);
                return true;
            }
        }
        return false;
    }
};

class SanD1ZhaolieEffect1: public TriggerSkill{
public:
    SanD1ZhaolieEffect1(): TriggerSkill("#sand1zhaolie1"){
        events << SlashProceed;
    }

    virtual int getPriority(TriggerEvent triggerEvent) const{
        return 1;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (player->getMark("SanD1Zhaolie_" + effect.slash->toString()) > 0){
            player->setFlags("SanD1Zhaolie_Jink");
        }
        return false;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }
};

class SanD1ZhaolieEffect2: public TriggerSkill{
public:
    SanD1ZhaolieEffect2(): TriggerSkill("#sand1zhaolie2"){
        events << SlashProceed;
    }

    virtual int getPriority(TriggerEvent triggerEvent) const{
        return -2;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (player->getMark("SanD1Zhaolie_" + effect.slash->toString()) > 0){
            player->setFlags("-SanD1Zhaolie_Jink");
        }
        return false;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }
};

SanD1DoudanCard::SanD1DoudanCard(){
    target_fixed = true;
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void SanD1DoudanCard::onUse(Room *room, const CardUseStruct &card_use) const{
    QVariantList l;
    foreach (int id, subcards){
        l << id;
    }

    room->setPlayerProperty(card_use.from, "doudan_sorted", l);
}

class SanD1DoudanVS: public ViewAsSkill{
public:
    SanD1DoudanVS(): ViewAsSkill("sand1doudan"){
        response_pattern = "@@sand1doudan!";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        QVariantList l = Self->property("doudan_table").toList();
        if (l.contains(to_select->getEffectiveId()))
            return true;

        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        QVariantList l = Self->property("doudan_table").toList();
        if (l.length() == cards.length()){
            SanD1DoudanCard *c = new SanD1DoudanCard;
            c->addSubcards(cards);
            return c;
        }
        return NULL;
    }
};

class SanD1Doudan: public PhaseChangeSkill{
public:
    SanD1Doudan(): PhaseChangeSkill("sand1doudan"){
        view_as_skill = new SanD1DoudanVS;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() == Player::Draw && target->askForSkillInvoke(objectName())){
            Room *room = target->getRoom();
            QList<int> card_ids = room->getNCards(3);
            
            CardMoveReason reason(CardMoveReason::S_REASON_TURNOVER, target->objectName(), objectName(), QString());
            CardsMoveStruct move(card_ids, NULL, Player::PlaceTable, reason);
            room->moveCardsAtomic(move, true);
            room->getThread()->delay();
            room->getThread()->delay();

            //for ai
            QVariantList l;
            foreach (int id, card_ids)
                l << id;

            QString choice = room->askForChoice(target, objectName(), "gainone+puttotop", l);

            if (choice == "gainone"){
                room->fillAG(card_ids, target);
                int id = room->askForAG(target, card_ids, false, objectName());
                room->clearAG(target);
                room->obtainCard(target, id, true);

                card_ids.removeOne(id);
                CardMoveReason reason2(CardMoveReason::S_REASON_PUT, target->objectName(), objectName(), QString());
                CardsMoveStruct move2(card_ids, NULL, Player::DiscardPile, reason);
                room->moveCardsAtomic(move2, true);
            }
            else {
                QList<ServerPlayer *> _target;
                _target << target;
                QList<CardsMoveStruct> _clientmovelist;
                _clientmovelist << CardsMoveStruct(card_ids, NULL, target, Player::PlaceTable, Player::PlaceHand, 
                        CardMoveReason(CardMoveReason::S_REASON_PREVIEW, target->objectName()));

                room->notifyMoveCards(true, _clientmovelist, true, _target);
                room->notifyMoveCards(false, _clientmovelist, true, _target);
                
                room->setPlayerProperty(target, "doudan_table", l);
                room->askForUseCard(target, "@@sand1doudan!", "@sand1doudan-put", -1, Card::MethodNone);
                room->setPlayerProperty(target, "doudan_table", QVariant());

                QVariantList sorted_l = target->property("doudan_sorted").toList();
                room->setPlayerProperty(target, "doudan_sorted", QVariant());

                _clientmovelist.clear();
                _clientmovelist << CardsMoveStruct(card_ids, target, NULL, Player::PlaceHand, Player::PlaceTable,
                        CardMoveReason(CardMoveReason::S_REASON_PREVIEW, target->objectName()));

                room->notifyMoveCards(true, _clientmovelist, true, _target);
                room->notifyMoveCards(false, _clientmovelist, true, _target);

                DummyCard dummy;
                foreach (QVariant id, sorted_l){
                    dummy.addSubcard(id.toInt());
                }

                room->moveCardTo(&dummy, NULL, Player::DrawPile, 
                        CardMoveReason(CardMoveReason::S_REASON_PUT, target->objectName(), objectName(), QString()), true);
            }

            QList<ServerPlayer *> can_slash;
            foreach (ServerPlayer *p, room->getOtherPlayers(target)){
                if (target->canSlash(p))
                    can_slash << p;
            }

            if (!can_slash.isEmpty()){
                ServerPlayer *victim = room->askForPlayerChosen(target, can_slash, objectName(), "@sand1doudan-slash", false, true);
                Slash *slash = new Slash(Card::NoSuit, 0);
                slash->setSkillName("_sand1doudan");
                room->useCard(CardUseStruct(slash, target, victim), false);
            }

            target->skip(Player::Play);
        }
        return false;
    }
};



SanD1Package::SanD1Package(): Package("sand1"){

    General *miheng = new General(this, "sand1_miheng", "qun", 3);
    miheng->addSkill(new SanD1Chishen);
    miheng->addSkill(new SanD1Xinve);

    General *mushun = new General(this, "sand1_mushun", "qun", 4);
    mushun->addSkill(new SanD1Mingzhan);

    General *caoang = new General(this, "sand1_caoang", "wei", 4);
    caoang->addSkill(new SanD1Xielu);

    General *xushu = new General(this, "sand1_xushu", "shu", 4);
    xushu->addSkill(new SanD1Bianzhen);
    xushu->addSkill(new SanD1Congwen);

    General *handang = new General(this, "sand1_handang", "wu", 4);
    handang->addSkill(new SanD1Xianxi);
    handang->addSkill(new SanD1XianxiEffect1);
    handang->addSkill(new SanD1XianxiEffect2);
    handang->addSkill(new SanD1XianxiEffect3);
    related_skills.insertMulti("sand1xianxi", "#sand1xianxi1");
    related_skills.insertMulti("sand1xianxi", "#sand1xianxi2");
    related_skills.insertMulti("sand1xianxi", "#sand1xianxi3");

    General *weiyan = new General(this, "sand1_weiyan", "shu", 4);
    weiyan->addSkill(new SanD1Kuangxi);

    General *ganqian = new General(this, "sand1_ganqian", "shu", 3);
    ganqian->addSkill(new SanD1Shenzhi);
    ganqian->addSkill(new SanD1Zhaolie);
    ganqian->addSkill(new SanD1ZhaolieEffect1);
    ganqian->addSkill(new SanD1ZhaolieEffect2);
    related_skills.insertMulti("sand1zhaolie", "#sand1zhaolie1");
    related_skills.insertMulti("sand1zhaolie", "#sand1zhaolie2");

    General *jiangwei = new General(this, "sand1_jiangwei", "shu", 4);
    jiangwei->addSkill(new SanD1Doudan);

    addMetaObject<SanD1XinveCard>();
    addMetaObject<SanD1BianzhenCard>();
    addMetaObject<SanD1KuangxiCard>();
    addMetaObject<SanD1ShenzhiCard>();
    addMetaObject<SanD1DoudanCard>();
}

ADD_PACKAGE(SanD1)