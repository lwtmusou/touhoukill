#include "power.h"
#include "skill.h"
#include "ai.h"
#include "jsonutils.h"
#include "util.h"
#include "engine.h"
#include "settings.h"
#include "clientplayer.h"
#include "standard.h"

class Xunxun: public PhaseChangeSkill{
public:
    Xunxun(): PhaseChangeSkill("xunxun"){
        //frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() == Player::Draw && target->askForSkillInvoke(objectName())){
            Room *room = target->getRoom();
            QList<int> getcards = room->getNCards(4);
            QList<int> origin_cards = getcards;
            QList<int> gained;

            int aidelay = Config.AIDelay;
            Config.AIDelay = 0;
            for (int i = 1; i <= 2; i++){
                room->fillAG(origin_cards, target, gained);
                int gain = room->askForAG(target, getcards, false, objectName());
                room->clearAG(target);
                getcards.removeOne(gain);
                gained << gain;
            }
            Config.AIDelay = aidelay;

            DummyCard gaindummy(gained);
            room->obtainCard(target, &gaindummy, false);

            room->askForGuanxing(target, getcards, Room::GuanxingDownOnly);

            return true;
        }
        return false;
    }
};

class Wangxi: public TriggerSkill{
public:
    Wangxi(): TriggerSkill("wangxi"){
        events << Damage << Damaged;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *target;
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == Damage)
            target = damage.to;
        else
            target = damage.from;
        if (target == NULL || target->isDead() || target->hasFlag("Global_DebutFlag"))
            return false;

        int x = damage.damage;
        for (int i = 0; i < x; i ++){
            if (player->isDead() || !room->askForSkillInvoke(player, objectName()))
                return false;

            QList<int> cards = room->getNCards(2, false, true);
            room->fillAG(cards, player);
            int id = room->askForAG(player, cards, false, objectName());
            room->obtainCard(player, id, false);
            room->clearAG(player);

            cards.removeOne(id);
            int to_give = cards.first();
            room->obtainCard(target, Sanguosha->getCard(to_give), CardMoveReason(CardMoveReason::S_REASON_PREVIEWGIVE, player->objectName()), false);

            if (target->isDead())
                return false;
        }
        return false;
    }
};

class Hengjiang: public TriggerSkill{ //temp version
public:
    Hengjiang(): TriggerSkill("hengjiang"){
        events << Damaged << CardsMoveOneTime << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        switch (triggerEvent){
            case (Damaged):{
                if (!TriggerSkill::triggerable(player))
                    return false;

                DamageStruct damage = data.value<DamageStruct>();

                ServerPlayer *current = room->getCurrent(); //当前回合角色躺枪
                if (current && current->isAlive() && current->getPhase() != Player::NotActive){
                    player->tag["hengjiang_damage"] = data;
                    if (player->askForSkillInvoke(objectName(), "maxcard:" + current->objectName()))
                        current->gainMark("@hengjiangmaxcard", damage.damage);
                    player->tag.remove("hengjiang_damage");
                }

                break;
            }
            case (CardsMoveOneTime):{
                if (!TriggerSkill::triggerable(player))
                    return false;

                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (move.from && move.from->getPhase() == Player::Discard && move.from_places.contains(Player::PlaceHand) &&
                        (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD){
                    ServerPlayer *from = (ServerPlayer *)move.from;
                    room->setPlayerMark(from, "hengjiang_discard", 1);
                }
                break;
            }
            case (EventPhaseStart):{
                if (player->getPhase() != Player::NotActive)
                    return false;
                player->loseAllMarks("@hengjiangmaxcard");
                ServerPlayer *lidian = room->findPlayerBySkillName(objectName());
                if (lidian == NULL || lidian->isDead())
                    return false;

                if (player->getMark("hengjiang_discard") == 0){
                    bool drawflag = false;
                    foreach(ServerPlayer *p, room->getOtherPlayers(lidian))
                        if (p->getHandcardNum() > lidian->getHandcardNum()){
                            drawflag = true;
                            break;
                        }
                    if (drawflag && lidian->askForSkillInvoke(objectName(), "drawcard"))
                        lidian->drawCards(1);
                }
                room->setPlayerMark(player, "hengjiang_discard", 0);
                
                break;
            }
        }
        return false;
    }
};
class HengjiangMaxCards: public MaxCardsSkill{
public:
    HengjiangMaxCards(): MaxCardsSkill("#hengjiang"){

    }

    virtual int getExtra(const Player *target) const{
        return -target->getMark("@hengjiangmaxcard");
    }
};

class Guixiu: public TriggerSkill{
public:
    Guixiu(): TriggerSkill("guixiu"){
        events << TurnStart;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        foreach(ServerPlayer *p, room->getOtherPlayers(player)){
            if (p->getHandcardNum() < player->getHandcardNum())
                return false;
        }
        if (player->askForSkillInvoke(objectName()))
            player->drawCards(1);

        return false;
    }
};

class Yongjue: public TriggerSkill{
public:
    Yongjue(): TriggerSkill("yongjue"){
        events << CardUsed << CardResponded << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent != Death){
            if (!player->hasFlag("yongjue")){
                const Card *card = NULL;
                if (triggerEvent == CardUsed){
                    CardUseStruct use = data.value<CardUseStruct>();
                    if (use.card != NULL)
                        card = use.card;
                }
                else {
                    CardResponseStruct resp = data.value<CardResponseStruct>();
                    if (resp.m_isUse)
                        card = resp.m_card;
                }

                if (card != NULL && !card->isKindOf("SkillCard")){
                    player->setFlags("yongjue");
                    if (card->isKindOf("Slash")){
                        ServerPlayer *mifuren = room->findPlayerBySkillName(objectName());
                        if (mifuren != NULL && mifuren->askForSkillInvoke(objectName(), QVariant::fromValue(player))){
                            player->obtainCard(card);

                        }
                    }
                }
            }
        }
        else {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player && player->hasSkill(objectName())){
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@yongjue-select", true, true);
                if (target != NULL){
                    room->handleAcquireDetachSkills(target, "yongjue");
                    target->drawCards(2);
                }
            }
        }
        return false;
    }
};

CunsiCard::CunsiCard(){

}

void CunsiCard::onEffect(const CardEffectStruct &effect) const{
    Player::Phase to_give = static_cast<Player::Phase>(Self->property("cunsi_phase").toInt());
    Room *room = effect.from->getRoom();
    effect.to->setPhase(to_give);
    room->broadcastProperty(effect.to, "phase");
    RoomThread *thread = room->getThread();
    try{
        if (!thread->trigger(EventPhaseStart, room, effect.to))
            thread->trigger(EventPhaseProceeding, room, effect.to);
        thread->trigger(EventPhaseEnd, room, effect.to);

        effect.to->setPhase(Player::NotActive);
        room->broadcastProperty(effect.to, "phase");
    }
    catch (TriggerEvent errorevent){
        if (errorevent == TurnBroken || errorevent == StageChange){
            effect.to->setPhase(Player::NotActive);
            room->broadcastProperty(effect.to, "phase");
        }
        throw errorevent;
    }
}

class CunsiVS: public OneCardViewAsSkill{
public:
    CunsiVS(): OneCardViewAsSkill("cunsi"){
        response_pattern = "@@cunsi";
    }

    static Card::Suit getCorrespondingSuit(Player::Phase phase){
        switch (phase){
            case Player::Start:
            case Player::Finish:
                return Card::Spade;
            case Player::Draw:
                return Card::Diamond;
            case Player::Play:
                return Card::Heart;
            case Player::Discard:
                return Card::Club;
            default:
                return Card::NoSuit;
        }
        return Card::NoSuit;
    }

    virtual bool viewFilter(const Card *to_select) const{
        if (Self->isJilei(to_select))
            return false;
        Card::Suit corresponding_suit = getCorrespondingSuit(static_cast<Player::Phase>(Self->property("cunsi_phase").toInt()));
        if (corresponding_suit != Card::NoSuit && corresponding_suit == to_select->getSuit())
            return true;
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        CunsiCard *c = new CunsiCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class Cunsi: public TriggerSkill{
public:
    Cunsi(): TriggerSkill("cunsi"){
        events << EventPhaseChanging;
    }

private:
    static QString getPhaseString(Player::Phase phase){
        switch (phase){
            case Player::Start:
                return "start";
            case Player::Draw:
                return "draw";
            case Player::Play:
                return "play";
            case Player::Discard:
                return "discard";
            case Player::Finish:
                return "finish";
            default:
                return "";
        }
        return "";
    }

public:
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Start || change.to == Player::Draw || change.to == Player::Play || change.to == Player::Discard || change.to == Player::Finish){
            room->setPlayerProperty(player, "cunsi_phase", static_cast<int>(change.to));
            if (room->askForUseCard(player, "@@cunsi", 
                    "@cunsi:::" + Card::Suit2String(CunsiVS::getCorrespondingSuit(change.to)) + ":" + getPhaseString(change.to)))
                player->skip(change.to, true);
        }
        return false;
    }
};

DuanxieCard::DuanxieCard(){

}

void DuanxieCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    room->setPlayerProperty(effect.to, "chained", true);
    room->setPlayerProperty(effect.from, "chained", true);
}

class Duanxie: public ZeroCardViewAsSkill{
public:
    Duanxie(): ZeroCardViewAsSkill("duanxie"){

    }

    virtual const Card *viewAs() const{
        return new DuanxieCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("DuanxieCard");
    }
};

class Fenming: public PhaseChangeSkill{
public:
    Fenming(): PhaseChangeSkill("fenming"){

    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() == Player::Finish && target->isChained() && target->askForSkillInvoke(objectName())){
            Room *room = target->getRoom();
            foreach(ServerPlayer *p, room->getAlivePlayers()){
                if (p->isChained() && target->canDiscard(p, "he")){
                    int to_discard = room->askForCardChosen(target, p, "he", objectName(), p == target, Card::MethodDiscard);
                    room->throwCard(to_discard, p, target);
                }
            }
        }
        return false;
    }
};

class Yingyang: public TriggerSkill{
public:
    Yingyang(): TriggerSkill("yingyang"){
        events << PindianVerifying;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        PindianStruct *pindian = data.value<PindianStruct *>();
        
        ServerPlayer *invoker = NULL;
        int *number_tochange;

        if (TriggerSkill::triggerable(pindian->from)){
            invoker = pindian->from;
            number_tochange = &pindian->from_number;
        }
        else if (TriggerSkill::triggerable(pindian->to)){
            invoker = pindian->to;
            number_tochange = &pindian->to_number;
        }

        if (invoker == NULL)
            return false;

        if (invoker->askForSkillInvoke(objectName(), data)){
            QString choice = room->askForChoice(invoker, objectName(), "add3+minus3", data);
            int &c = (*number_tochange);
            if (choice == "add3"){
                c += 3;
                if (c > 13)
                    c = 13;
            }
            else{
                c -= 3;
                if (c < 1)
                    c = 1;
            }
            data = QVariant::fromValue(pindian);
        }
        
        return false;
    }
};

PowerZhibaCard::PowerZhibaCard(){
    mute = true;
    m_skillName = "powerzhiba_pindian";
}

bool PowerZhibaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select->hasLordSkill("powerzhiba") && to_select != Self
           && !to_select->isKongcheng() && !to_select->hasFlag("powerZhibaInvoked");
}

void PowerZhibaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *sunce = targets.first();
    room->setPlayerFlag(sunce, "powerZhibaInvoked");
    room->notifySkillInvoked(sunce, "powerZhiba");
    if (sunce->getMark("hunzi") > 0 && room->askForChoice(sunce, "powerzhiba_pindian", "accept+reject") == "reject"){
        LogMessage log;
        log.type = "#ZhibaReject";
        log.from = sunce;
        log.to << source;
        log.arg = "powerzhiba_pindian";
        room->sendLog(log);

        room->broadcastSkillInvoke("zhiba", 3);
        return;
    }

    if (!sunce->isLord() && sunce->hasSkill("weidi"))
        room->broadcastSkillInvoke("weidi", 2);
    else
        room->broadcastSkillInvoke("zhiba", 1);

    source->pindian(sunce, "powerzhiba_pindian", NULL);

    QList<ServerPlayer *> sunces;
    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    foreach (ServerPlayer *p, players) {
        if (p->hasLordSkill("powerzhiba") && !p->hasFlag("powerZhibaInvoked"))
            sunces << p;
    }
    if (sunces.isEmpty())
        room->setPlayerFlag(source, "ForbidPowerZhiba");
}

class PowerZhibaPindian: public ZeroCardViewAsSkill{
public:
    PowerZhibaPindian(): ZeroCardViewAsSkill("powerzhiba_pindian"){
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getKingdom() == "wu" && !player->isKongcheng() && !player->hasFlag("ForbidPowerZhiba");
    }

    virtual const Card *viewAs() const{
        return new PowerZhibaCard;
    }
};

class PowerZhiba: public TriggerSkill{
public:
    PowerZhiba(): TriggerSkill("powerzhiba$"){
        events << GameStart << EventAcquireSkill << EventLoseSkill << Pindian << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if ((triggerEvent == GameStart && player->isLord()) 
                || (triggerEvent == EventAcquireSkill && data.toString() == "powerzhiba")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.isEmpty()) return false;

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("powerzhiba_pindian"))
                    room->attachSkillToPlayer(p, "powerzhiba_pindian");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "powerzhiba") {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(objectName()))
                    lords << p;
            }
            if (lords.length() > 2) return false;

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("powerzhiba_pindian"))
                    room->detachSkillFromPlayer(p, "powerzhiba_pindian", true);
            }
        } else if (triggerEvent == Pindian) {
            PindianStar pindian = data.value<PindianStar>();
            if (pindian->reason != "powerzhiba_pindian" || !pindian->to->hasLordSkill(objectName()))
                return false;
            if (!pindian->isSuccess()) {
                if (!pindian->to->isLord() && pindian->to->hasSkill("weidi"))
                    room->broadcastSkillInvoke("weidi", 1);
                else
                    room->broadcastSkillInvoke("zhiba", 2);
                DummyCard dummy;
                dummy.addSubcard(pindian->from_card);
                dummy.addSubcard(pindian->to_card);
                if (room->askForChoice(pindian->to, objectName(), "obtain+give", data) == "obtain"){
                    pindian->to->obtainCard(&dummy);
                }
                else{
                    pindian->from->obtainCard(&dummy);
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                return false;
            if (player->hasFlag("ForbidPowerZhiba"))
                room->setPlayerFlag(player, "-ForbidPowerZhiba");
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("powerZhibaInvoked"))
                    room->setPlayerFlag(p, "-powerZhibaInvoked");
            }
        }
        return false;
    }
};

class Hengzheng: public PhaseChangeSkill{
public:
    Hengzheng(): PhaseChangeSkill("hengzheng"){
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() == Player::Draw && (target->getHp() == 1 || target->isKongcheng())){
            Room *room = target->getRoom();
            bool invoke = false;
            foreach(ServerPlayer *p, room->getOtherPlayers(target))
                if (!p->isAllNude()){
                    invoke = true;
                    break;
                }

            if (invoke && target->askForSkillInvoke(objectName())){
                foreach(ServerPlayer *p, room->getOtherPlayers(target))
                    if (!p->isAllNude()){
                        int id = room->askForCardChosen(target, p, "hej", objectName());
                        room->obtainCard(target, id);
                    }
                return true;
            }
        }
        return false;
    }
};

//技能暂时没有任何用途

class Baoling: public TriggerSkill{
public:
    Baoling(): TriggerSkill("baoling"){
        events << AskForPeaches;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.who == player){
            room->handleAcquireDetachSkills(player, "-baoling");
            room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 3);
            RecoverStruct recover;
            recover.recover = 3;
            recover.who = player;
            room->recover(player, recover);
            room->handleAcquireDetachSkills(player, "benghuai");
        }
        return false;
    }
};

class Chuanxin: public TriggerSkill{
public:
    Chuanxin(): TriggerSkill("chuanxin"){
        events << DamageCaused << EventPhaseStart;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == DamageCaused){
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != NULL && (damage.card->isKindOf("Duel") || damage.card->isKindOf("Slash")) 
                    && damage.by_user && !damage.chain && damage.transfer){
                QList<const Skill *> skills = damage.to->getVisibleSkillList();
                QList<const Skill *> fix_skills;
                foreach(const Skill *skill, skills){
                    if (skill->getLocation() == Skill::Right && !skill->isAttachedLordSkill())
                        fix_skills << skill;
                }
                if (!fix_skills.isEmpty() && player->askForSkillInvoke(objectName())){
                    QString choice = "loseskill";
                    if (damage.to->hasEquip())
                        choice = room->askForChoice(damage.to, objectName() + "_select", "discardequip+loseskill");
                    if (choice == "loseskill"){
                        QStringList skillnames;
                        foreach(const Skill *skill, skills)
                            skillnames << skill->objectName();
                        QStringList chuanxinskill = damage.to->tag["chuanxinskill"].toStringList();
                        foreach(QString s, chuanxinskill){
                            if (skillnames.contains(s))
                                skillnames.removeOne(s);
                        }

                        QString selectedskill = room->askForChoice(player, objectName() + "_loseskill", skillnames.join("+"), QVariant::fromValue(damage.to));
                        chuanxinskill << selectedskill;
                        damage.to->tag["chuanxinskill"] = chuanxinskill;

                        //log

                        room->addPlayerMark(damage.to, "Qingcheng" + selectedskill);

                        foreach (ServerPlayer *p, room->getAllPlayers())
                            room->filterCards(p, p->getCards("he"), true);

                        Json::Value args;
                        args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
                    }
                    else if (choice == "discardequip"){
                        DummyCard dummy;
                        dummy.addSubcards(damage.to->getEquips());
                        room->throwCard(&dummy, damage.to, player);
                        room->loseHp(damage.to);
                    }
                    return true;
                }
            }
            else if (triggerEvent == EventPhaseStart){
                if (player->getPhase() == Player::RoundStart){
                    foreach (ServerPlayer *p, room->getAlivePlayers()){
                        QStringList chuanxinskill = p->tag["chuanxinskill"].toStringList();
                        foreach(QString skill, chuanxinskill)
                            room->setPlayerMark(p, "Qingcheng" + skill, 0);
                        //log
                        p->tag.remove("chuanxinskill");
                    }

                    foreach (ServerPlayer *p, room->getAllPlayers())
                        room->filterCards(p, p->getCards("he"), true);

                    Json::Value args;
                    args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
                }
            }
        }
        return false;
    }
};

class Fengshi: public TriggerSkill{
public:
    Fengshi(): TriggerSkill("fengshi"){
        events << TargetConfirmed;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card != NULL && use.card->isKindOf("Slash") && use.from != NULL){
            foreach(ServerPlayer *p, use.to){
                if (player->isAdjacentTo(p) && player->canDiscard(p, "e") && player->askForSkillInvoke(objectName(), QVariant::fromValue(p))){
                    int id = room->askForCardChosen(player, p, "e", objectName(), false, Card::MethodDiscard);
                    room->throwCard(id, p, player);
                }
            }
        }
        return false;
    }
};

WuxinCard::WuxinCard(){
    mute = true;
}

bool WuxinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    foreach(int id, Self->getPile("skysoldier")){
        if (Sanguosha->getCard(id)->isBlack()){
            Slash *theslash = new Slash(Card::SuitToBeDecided, -1);
            theslash->setSkillName("wuxin");
            theslash->addSubcard(id);
            theslash->deleteLater();
            bool can_slash = theslash->isAvailable(Self) && theslash->targetFilter(targets, to_select, Self) && !Sanguosha->isProhibited(Self, to_select, theslash);
            if (can_slash)
                return true;
        }
    }
    return false;
}

bool WuxinCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    foreach(int id, Self->getPile("skysoldier")){
        if (Sanguosha->getCard(id)->isBlack()){
            Slash *theslash = new Slash(Card::SuitToBeDecided, -1);
            theslash->setSkillName("wuxin");
            theslash->addSubcard(id);
            theslash->deleteLater();
            bool can_slash = theslash->isAvailable(Self) && theslash->targetsFeasible(targets, Self);
            if (can_slash)
                return true;
        }
    }
    return false;
}

QList<const Player *> WuxinCard::ServerPlayerList2PlayerList(QList<ServerPlayer *> thelist){
    QList<const Player *> targetlist;
    foreach (ServerPlayer *p, thelist){
        targetlist << (const Player *)p;
    }
    return targetlist;
}

const Card *WuxinCard::validate(CardUseStruct &cardUse) const{
    QList<int> skysoldier = cardUse.from->getPile("skysoldier");
    QList<int> black_skysoldier, disabled_skysoldier;
    foreach(int id, skysoldier){
        if (Sanguosha->getCard(id)->isBlack()){
            Slash *theslash = new Slash(Card::SuitToBeDecided, -1);
            theslash->setSkillName("wuxin");
            theslash->addSubcard(id);
            theslash->deleteLater();
            bool can_slash = theslash->isAvailable(cardUse.from) && 
                theslash->targetsFeasible(ServerPlayerList2PlayerList(cardUse.to), cardUse.from);
            if (can_slash)
                black_skysoldier << id;
            else
                disabled_skysoldier << id;
        }
        else
            disabled_skysoldier << id;
    }

    if (black_skysoldier.isEmpty())
        return NULL;

    Room *room = cardUse.from->getRoom();
    room->fillAG(skysoldier, cardUse.from, disabled_skysoldier);
    int slash_id = room->askForAG(cardUse.from, black_skysoldier, false, "wuxin");
    room->clearAG(cardUse.from);

    Slash *slash = new Slash(Card::SuitToBeDecided, -1);
    slash->addSubcard(slash_id);
    slash->setSkillName("wuxin");
    return slash;
}

class WuxinVS: public ZeroCardViewAsSkill{
public:
    WuxinVS(): ZeroCardViewAsSkill("wuxin"){

    }

    virtual const Card *viewAs() const{
        return new WuxinCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash" && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    }
};

class Wuxin: public PhaseChangeSkill{
public:
    Wuxin(): PhaseChangeSkill("wuxin"){
        view_as_skill = new WuxinVS;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() == Player::Draw){
            Room *room = target->getRoom();

            int qunplayers = 0;
            foreach(ServerPlayer *p, room->getAlivePlayers())
                if (p->getKingdom() == "qun")
                    qunplayers ++;

            if (qunplayers <= 1)
                return false;

            if (qunplayers > 0 && target->askForSkillInvoke(objectName())){
                QList<int> guanxing_cards = room->getNCards(qunplayers);
                room->askForGuanxing(target, guanxing_cards, Room::GuanxingUpOnly);
            }

            if (target->getPile("skysoldier").length() == 0){
                Room *room = target->getRoom();

                int qunplayers = 0;
                foreach(ServerPlayer *p, room->getAlivePlayers())
                    if (p->getKingdom() == "qun")
                        qunplayers ++;

                if (qunplayers == 0)
                    return false;

                QList<int> skill2cards = room->getNCards(qunplayers);
                CardMoveReason reason(CardMoveReason::S_REASON_TURNOVER, target->objectName(), objectName(), QString());
                CardsMoveStruct move(skill2cards, NULL, Player::PlaceTable, reason);
                room->moveCardsAtomic(move, true);
                room->getThread()->delay();
                room->getThread()->delay();

                target->addToPile("skysoldier", skill2cards, true);

            }

        }
        return false;
    }
};

class WuxinPreventLoseHp: public TriggerSkill{
public:
    WuxinPreventLoseHp(): TriggerSkill("#wuxin-prevent"){
        events << PreDamageDone << PreHpLost;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QList<int> skysoldier = player->getPile("skysoldier");
        QList<int> red_skysoldier, disabled_skysoldier;
        foreach(int id, skysoldier){
            if (Sanguosha->getCard(id)->isRed())
                red_skysoldier << id;
            else
                disabled_skysoldier << id;
        }
        //todo:data
        if (!red_skysoldier.isEmpty() && player->askForSkillInvoke("wuxin", data)){
            room->fillAG(skysoldier, player, disabled_skysoldier);
            int id = room->askForAG(player, red_skysoldier, false, "wuxin");
            room->clearAG(player);

            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, player->objectName(), "wuxin", QString());
            room->moveCardTo(Sanguosha->getCard(id), NULL, Player::DiscardPile, reason, true);
            if (triggerEvent == PreDamageDone){
                DamageStruct damage = data.value<DamageStruct>();
                damage.damage = 0;
                data = QVariant::fromValue(damage);
            }
            else
                data = 0;
        }
        return false;
    }
};

class WuxinSlashResponse: public TriggerSkill{
public:
    WuxinSlashResponse(): TriggerSkill("#wuxin-slashresponse"){
        events << CardAsked;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QStringList ask = data.toStringList();
        if (ask.first() == "slash"){
            QList<int> skysoldier = player->getPile("skysoldier");
            QList<int> black_skysoldier, disabled_skysoldier;
            foreach(int id, skysoldier){
                if (Sanguosha->getCard(id)->isBlack())
                    black_skysoldier << id;
                else
                    disabled_skysoldier << id;
            }

            if (black_skysoldier.isEmpty())
                return false;
            //data
            if (player->askForSkillInvoke("wuxin", data)){
                room->fillAG(skysoldier, player, disabled_skysoldier);
                int slash_id = room->askForAG(player, black_skysoldier, false, "wuxin");
                room->clearAG(player);

                Slash *slash = new Slash(Card::SuitToBeDecided, -1);
                slash->addSubcard(slash_id);
                slash->setSkillName("wuxin");
                room->provide(slash);
                return true;
            }
        }
        return false;
    }
};

WendaoCard::WendaoCard(){
    target_fixed = true;
}

void WendaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    const Card *tpys = NULL;
    foreach(ServerPlayer *p, room->getAlivePlayers()){
        foreach(const Card *card, p->getEquips()){
            if (Sanguosha->getEngineCard(card->getEffectiveId())->isKindOf("PeaceSpell")){
                tpys = Sanguosha->getCard(card->getEffectiveId());
                break;
            }
        }
        if (tpys != NULL)
            break;
        foreach(const Card *card, p->getJudgingArea()){
            if (Sanguosha->getEngineCard(card->getEffectiveId())->isKindOf("PeaceSpell")){
                tpys = Sanguosha->getCard(card->getEffectiveId());
                break;
            }
        }
        if (tpys != NULL)
            break;
    }
    if (tpys == NULL)
        foreach(int id, room->getDiscardPile()){
            if (Sanguosha->getEngineCard(id)->isKindOf("PeaceSpell")){
                tpys = Sanguosha->getCard(id);
                break;
            }
        }
        
    if (tpys == NULL)
        return;

    source->obtainCard(tpys, true);
}

class Wendao: public OneCardViewAsSkill{
public:
    Wendao(): OneCardViewAsSkill("wendao"){
        filter_pattern = ".|red!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("WendaoCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        WendaoCard *c = new WendaoCard;
        c->addSubcard(originalCard);
        return c;
    }
};

HongfaCard::HongfaCard(){
    mute = true;
    m_skillName = "hongfa_slash";
}

bool HongfaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (targets.length() == 0)
        return to_select->hasLordSkill("hongfa") && !to_select->getPile("skysoldier").isEmpty() && to_select != Self;
    else if (targets.length() >= 1){
        if (!targets[0]->hasLordSkill("hongfa") || targets[0]->getPile("skysoldier").isEmpty())
            return false;

        QList<int> skysoldiers = targets[0]->getPile("skysoldier");

        QList<const Player *> fixed_targets = targets;
        fixed_targets.removeOne(targets[0]);

        foreach (int id, skysoldiers){
            if (Sanguosha->getCard(id)->isBlack()){
                Slash *theslash = new Slash(Card::SuitToBeDecided, -1);
                theslash->setSkillName("hongfa");
                theslash->addSubcard(id);
                theslash->deleteLater();
                bool can_slash = theslash->isAvailable(Self) && theslash->targetFilter(fixed_targets, to_select, Self) && !Sanguosha->isProhibited(Self, to_select, theslash);
                if (can_slash)
                    return true;
            }
        }
        return false;
    }
    return false;
}

bool HongfaCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (targets.length() >= 2 && targets[0]->hasLordSkill("hongfa") && !targets[0]->getPile("skysoldier").isEmpty()){

        QList<int> skysoldiers = targets[0]->getPile("skysoldier");

        QList<const Player *> fixed_targets = targets;
        fixed_targets.removeOne(targets[0]);

        foreach (int id, skysoldiers){
            if (Sanguosha->getCard(id)->isBlack()){
                Slash *theslash = new Slash(Card::SuitToBeDecided, -1);
                theslash->setSkillName("hongfa");
                theslash->addSubcard(id);
                theslash->deleteLater();
                bool can_slash = theslash->isAvailable(Self) && theslash->targetsFeasible(fixed_targets, Self);
                if (can_slash)
                    return true;
            }
        }
        return false;
    }
    return false;
}

const Card *HongfaCard::validate(CardUseStruct &cardUse) const{
    if (!cardUse.to.first()->hasLordSkill("hongfa") || cardUse.to.first()->getPile("skysoldier").isEmpty())
        return NULL;
    Room *room = cardUse.from->getRoom();
    ServerPlayer *slasher = cardUse.to.first();
    cardUse.to.removeOne(slasher);
    QList<int> skysoldier = slasher->getPile("skysoldier");
    QList<int> black_skysoldier, disabled_skysoldier;
    foreach(int id, skysoldier){
        if (Sanguosha->getCard(id)->isBlack()){
            Slash *theslash = new Slash(Card::SuitToBeDecided, -1);
            theslash->setSkillName("hongfa");
            theslash->addSubcard(id);
            theslash->deleteLater();
            bool can_slash = theslash->isAvailable(cardUse.from) && 
                theslash->targetsFeasible(WuxinCard::ServerPlayerList2PlayerList(cardUse.to), cardUse.from);
            if (can_slash)
                black_skysoldier << id;
            else
                disabled_skysoldier << id;
        }
        else
            disabled_skysoldier << id;
    }

    if (black_skysoldier.isEmpty())
        return NULL;

    room->fillAG(skysoldier, cardUse.from, disabled_skysoldier);
    int slash_id = room->askForAG(cardUse.from, black_skysoldier, false, "hongfa");
    room->clearAG(cardUse.from);

    room->showCard(slasher, slash_id);
    bool can_use = room->askForSkillInvoke(slasher, "hongfa_slash", "letslash:" + cardUse.from->objectName());
    if (can_use){
        Slash *slash = new Slash(Card::SuitToBeDecided, -1);
        slash->addSubcard(slash_id);
        slash->setSkillName("hongfa");
        return slash;
    }
    return NULL;
}

class HongfaSlashVS: public ZeroCardViewAsSkill{
public:
    HongfaSlashVS(): ZeroCardViewAsSkill("hongfa_slash"){
        attached_lord_skill = true;
    }

    virtual const Card *viewAs() const{
        return new HongfaCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash" && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    }
};

class HongfaSlash: public TriggerSkill{
public:
    HongfaSlash(): TriggerSkill("hongfa_slash"){
        view_as_skill = new HongfaSlashVS;
        events << CardAsked;
        attached_lord_skill = true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QStringList ask = data.toStringList();
        if (ask.first() == "slash"){

            QList<ServerPlayer *> skysoldiers;
            foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                QList<int> skysoldier = p->getPile("skysoldier");
                QList<int> black_skysoldier, disabled_skysoldier;
                foreach(int id, skysoldier){
                    if (Sanguosha->getCard(id)->isBlack())
                        black_skysoldier << id;
                    else
                        disabled_skysoldier << id;
                }

                if (!black_skysoldier.isEmpty())
                    skysoldiers << p;
            }
            //data
            if (!skysoldiers.isEmpty()){
                ServerPlayer *zhangjiao = room->askForPlayerChosen(player, skysoldiers, objectName(), "@hongfa-response", true);
                if (zhangjiao != NULL){
                    QList<int> skysoldier = zhangjiao->getPile("skysoldier");
                    QList<int> black_skysoldier, disabled_skysoldier;
                    foreach(int id, skysoldier){
                        if (Sanguosha->getCard(id)->isBlack())
                            black_skysoldier << id;
                        else
                            disabled_skysoldier << id;
                    }

                    room->fillAG(skysoldier, player, disabled_skysoldier);
                    int slash_id = room->askForAG(player, black_skysoldier, false, "hongfa");
                    room->clearAG(player);

                    room->showCard(zhangjiao, slash_id);
                    bool can_use = room->askForSkillInvoke(zhangjiao, "hongfa_slash", "letslash:" + player->objectName());
                    if (can_use){
                        Slash *slash = new Slash(Card::SuitToBeDecided, -1);
                        slash->addSubcard(slash_id);
                        slash->setSkillName("hongfa");
                        room->provide(slash);
                        return true;
                    }
                }
            }
        }
        return false;
    }
};

class Hongfa: public TriggerSkill{
public:
    Hongfa(): TriggerSkill("hongfa$"){
        events << GameStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasLordSkill(objectName()) && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        foreach(ServerPlayer *p, room->getAllPlayers()){
            room->attachSkillToPlayer(p, "hongfa_slash");
        }
        return false;
    }
};

PowerPackage::PowerPackage(): Package("Power"){

    General *lidian = new General(this, "lidian", "wei", 3);
    lidian->addSkill(new Xunxun);
    lidian->addSkill(new Wangxi);

    General *zangba = new General(this, "zangba", "wei");
    zangba->addSkill(new Hengjiang);
    zangba->addSkill(new HengjiangMaxCards);
    related_skills.insertMulti("hengjiang", "#hengjiang");

    General *mifuren = new General(this, "mifuren", "shu", 3);
    mifuren->addSkill(new Guixiu);
    mifuren->addSkill(new Yongjue);
    mifuren->addSkill(new Cunsi);
    related_skills.insertMulti("guixiu", "#guixiu-initial");

    General *chenwudongxi = new General(this, "chenwudongxi", "wu", 4);
    chenwudongxi->addSkill(new Duanxie);
    chenwudongxi->addSkill(new Fenming);

    General *sunce = new General(this, "heg_sunce$", "wu", 4);
    sunce->addSkill("jiang");
    sunce->addSkill("hunzi");
    sunce->addSkill(new Yingyang);
    sunce->addSkill(new PowerZhiba);
    skills << new PowerZhibaPindian;

    General *dongzhuo = new General(this, "heg_dongzhuo$", "qun", 4);
    dongzhuo->addSkill(new Hengzheng);
    dongzhuo->addSkill("roulin");
    dongzhuo->addSkill("baonue");

    General *zhangren = new General(this, "zhangren", "qun", 4);
    zhangren->addSkill(new Chuanxin);
    zhangren->addSkill(new Fengshi);

    General *zhangjiao = new General(this, "heg_zhangjiao$", "qun", 3);
    zhangjiao->addSkill(new Wuxin);
    zhangjiao->addSkill(new WuxinPreventLoseHp);
    zhangjiao->addSkill(new WuxinSlashResponse);
    related_skills.insertMulti("wuxin", "#wuxin-prevent");
    related_skills.insertMulti("wuxin", "#wuxin-slashresponse");
    zhangjiao->addSkill(new Wendao);
    zhangjiao->addSkill(new Hongfa);
    skills << new HongfaSlash;

    addMetaObject<CunsiCard>();
    addMetaObject<DuanxieCard>();
    addMetaObject<PowerZhibaCard>();
    addMetaObject<WuxinCard>();
    addMetaObject<WendaoCard>();
    addMetaObject<HongfaCard>();

    skills << new Baoling; //for future use
}

ADD_PACKAGE(Power)