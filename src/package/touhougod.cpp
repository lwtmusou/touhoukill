#include "touhougod.h"

#include "general.h"
#include "settings.h"
#include "skill.h"
#include "engine.h"
#include "standard.h"
#include "generaloverview.h" //for zun?
//#include "clientplayer.h"
#include "client.h"
//#include "ai.h"
#include "jsonutils.h"
#include "maneuvering.h" // for iceslash


class chuanghuan : public GameStartSkill {
public:
    chuanghuan() : GameStartSkill("chuanghuan") {
        frequency = Eternal;
    }

    static void AcquireGenerals(ServerPlayer *zun, int n) {
        Room *room = zun->getRoom();
        QVariantList huashens = zun->tag["Fantasy"].toList();
        QStringList list = GetAvailableGenerals(zun);
        qShuffle(list);
        if (list.isEmpty()) return;
        n = qMin(n, list.length());

        QStringList acquired = list.mid(0, n);
        //获得的化身加入thread的skillset？
        foreach(QString name, acquired) {
            huashens << name;
            const General *general = Sanguosha->getGeneral(name);
            if (general) {
                foreach(const TriggerSkill *skill, general->getTriggerSkills()) {
                    if (skill->isVisible())
                        room->getThread()->addTriggerSkill(skill);
                }
            }
        }
        zun->tag["Fantasy"] = huashens;
        //添加什么将 不需要hidden
        room->doAnimate(QSanProtocol::S_ANIMATE_HUASHEN, zun->objectName(), acquired.join(":"), room->getAllPlayers());


        LogMessage log;
        log.type = "#GetHuashen";
        log.from = zun;
        log.arg = QString::number(n);
        log.arg2 = QString::number(huashens.length());
        room->sendLog(log);

        LogMessage log2;
        log2.type = "#GetHuashenDetail";
        log2.from = zun;
        log2.arg = acquired.join("\\, \\");
        //room->doNotify(zun, QSanProtocol::S_COMMAND_LOG_SKILL, log2.toJsonValue());
        room->sendLog(log2);

        room->setPlayerMark(zun, "@huanxiangs", huashens.length());

        //以上为获得幻想牌
        //以下为预装技能
        QVariantList huashen_skills = zun->tag["FantasySkills"].toList();

        if (huashens.isEmpty()) return;

        QStringList huashen_generals;
        foreach(QVariant huashen, huashens)
            huashen_generals << huashen.toString();

        //没有考虑ai
        foreach(QString general_name, huashen_generals) {
            const General *general = Sanguosha->getGeneral(general_name);
            foreach(const Skill *skill, general->getVisibleSkillList()) {
                if (skill->isLordSkill()
                    || skill->getFrequency() == Skill::Limited
                    || skill->getFrequency() == Skill::Wake || skill->getFrequency() == Skill::Eternal)
                    continue;
                huashen_skills << skill->objectName();

            }
        }

        zun->tag["FantasySkills"] = huashen_skills;
    }

    static QStringList GetAvailableGenerals(ServerPlayer *zun) {
        QSet<QString> all = Sanguosha->getLimitedGeneralNames().toSet();
        Room *room = zun->getRoom();
        if (isNormalGameMode(room->getMode())
            || room->getMode().contains("_mini_")
            || room->getMode() == "custom_scenario")
            all.subtract(Config.value("Banlist/Roles", "").toStringList().toSet());
        else if (room->getMode() == "04_1v3")
            all.subtract(Config.value("Banlist/HulaoPass", "").toStringList().toSet());
        else if (room->getMode() == "06_XMode") {
            all.subtract(Config.value("Banlist/XMode", "").toStringList().toSet());
            foreach(ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["XModeBackup"].toStringList().toSet());
        }
        else if (room->getMode() == "02_1v1") {
            all.subtract(Config.value("Banlist/1v1", "").toStringList().toSet());
            foreach(ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["1v1Arrange"].toStringList().toSet());
        }

        QSet<QString> huashen_set, room_set;
        QVariantList huashens = zun->tag["Fantasy"].toList();
        foreach(QVariant huashen, huashens)
            huashen_set << huashen.toString();
        foreach(ServerPlayer *player, room->getAlivePlayers()) {
            QString name = player->getGeneralName();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->findConvertFrom(name);
                if (!fname.isEmpty()) name = fname;
            }
            room_set << name;

            if (!player->getGeneral2()) continue;

            name = player->getGeneral2Name();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->findConvertFrom(name);
                if (!fname.isEmpty()) name = fname;
            }
            room_set << name;
        }

        static QSet<QString> banned;
        if (banned.isEmpty()) {
            banned << "zun" << "guzhielai" << "dengshizai" << "yuanshu";
        }

        QStringList touhouGenerals = (all - banned - huashen_set - room_set).toList();
        foreach(QString name, touhouGenerals){
            const General *general = Sanguosha->getGeneral(name);
            if (general) {
                bool isTouhou = false;
                foreach(QString kingdom, Sanguosha->TouhouKingdoms) {
                    if (general->getKingdom() == kingdom){
                        isTouhou = true;
                        break;
                    }
                }
                if (!isTouhou)
                    touhouGenerals.removeOne(name);
            }
            else{
                touhouGenerals.removeOne(name);

            }
        }

        return touhouGenerals;
    }


    virtual void onGameStart(ServerPlayer *zun) const{
        zun->getRoom()->notifySkillInvoked(zun, "chuanghuan");
        AcquireGenerals(zun, 4);
        //SelectSkill(zun);
    }
};

class chuanghuanGet : public PhaseChangeSkill {
public:
    chuanghuanGet() : PhaseChangeSkill("#chuanghuan-get") {
    }

    virtual int getPriority(TriggerEvent) const{
        return 6;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target) &&
            (target->getPhase() == Player::Start);
    }

    virtual bool onPhaseChange(ServerPlayer *zun) const{
        zun->getRoom()->notifySkillInvoked(zun, "chuanghuan");
        chuanghuan::AcquireGenerals(zun, 2);
        return false;
    }
};

class jiexian : public TriggerSkill {
public:
    jiexian() : TriggerSkill("jiexian") {
        events << DamageInflicted << PreHpRecover;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (source == NULL || source->isNude())
            return false;
        if (triggerEvent == DamageInflicted){
            const Card *card = room->askForCard(source, "..H", "@jiexiandamage:" + player->objectName(), data, objectName());
            if (card != NULL) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), player->objectName());
            
                DamageStruct damage = data.value<DamageStruct>();
                room->touhouLogmessage("#jiexiandamage", player, objectName(), QList<ServerPlayer *>(), QString::number(damage.damage));
                if (player->isWounded()) {
                    RecoverStruct recover;
                    recover.who = source;
                    recover.reason = objectName();
                    room->recover(player, recover);
                }

                return true;
            }
        }
        else if (triggerEvent == PreHpRecover) {
            const Card *card = room->askForCard(source, "..S", "@jiexianrecover:" + player->objectName(), QVariant::fromValue(player), objectName());
            if (card != NULL){
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), player->objectName());
            
                room->touhouLogmessage("#jiexianrecover", player, objectName(), QList<ServerPlayer *>(), QString::number(data.value<RecoverStruct>().recover));
                room->damage(DamageStruct(objectName(), NULL, player));
                return true;
            }
        }
        return false;
    }
};

class zhouye : public TriggerSkill {
public:
    zhouye() : TriggerSkill("zhouye") {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseStart;

        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == GameStart || (triggerEvent == EventAcquireSkill && data.toString() == "zhouye")){
            if (player->getMark("zhouye_limit") == 0 && player->hasSkill("zhouye")){
                room->setPlayerMark(player, "zhouye_limit", 1);
                room->setPlayerCardLimitation(player, "use", "Slash", false);
            }
        }
        else if (triggerEvent == EventLoseSkill && data.toString() == "zhouye") {
            if (player->getMark("zhouye_limit") > 0 && !player->hasSkill("zhouye")){
                room->setPlayerMark(player, "zhouye_limit", 0);
                room->removePlayerCardLimitation(player, "use", "Slash$0");
            }
        }
        else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Start
            && player->hasSkill("zhouye")) {
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
            room->throwCard(cd_id, player);
        }
        return false;
    }
};

class zhouye_change : public TriggerSkill {
public:
    zhouye_change() : TriggerSkill("#zhouye_change") {
        events << PreMarkChange;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        MarkChangeStruct change = data.value<MarkChangeStruct>();
        if (change.name != "@ye")
            return false;
        int mark = player->getMark("@ye");

        if (mark == 0 && mark + change.num > 0 && player->getMark("zhouye_limit") > 0) {
            room->setPlayerMark(player, "zhouye_limit", 0);
            room->removePlayerCardLimitation(player, "use", "Slash$0");
        }
        else if (mark > 0 && mark + change.num == 0 && player->getMark("zhouye_limit") == 0) {
            room->setPlayerMark(player, "zhouye_limit", 1);
            room->setPlayerCardLimitation(player, "use", "Slash", false);
        }
        return false;
    }
};


hongwuCard::hongwuCard() {
    mute = true;
    will_throw = true;
    target_fixed = true;
}
void hongwuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    source->gainMark("@ye", 1);
}

class hongwu : public ViewAsSkill {
public:
    hongwu() : ViewAsSkill("hongwu") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return  player->getMark("@ye") == 0;
    }


    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return to_select->isRed() && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() == 2) {
            hongwuCard *card = new hongwuCard;
            card->addSubcards(cards);

            return card;
        }
        else
            return NULL;

    }
};

shenqiangCard::shenqiangCard() {
    mute = true;
    will_throw = true;
}
void shenqiangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    room->damage(DamageStruct("shenqiang", source, target));
}

class shenqiang : public OneCardViewAsSkill {
public:
    shenqiang() :OneCardViewAsSkill("shenqiang") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@ye") > 0;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return (to_select->isKindOf("Weapon") || to_select->getSuit() == Card::Heart)
            && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        shenqiangCard *card = new shenqiangCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class yewang : public TriggerSkill {
public:
    yewang() : TriggerSkill("yewang") {
        events << DamageInflicted;
        frequency = Compulsory;
    }



    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (player->getMark("@ye") > 0) {
            damage.damage = damage.damage - 1;
            room->touhouLogmessage("#YewangTrigger", player, objectName(), QList<ServerPlayer *>(), QString::number(1));
            room->notifySkillInvoked(player, objectName());
            data = QVariant::fromValue(damage);
            if (damage.damage == 0)
                return true;
        }
        return false;
    }
};


class aoyi_handle : public TriggerSkill {
public:
    aoyi_handle() : TriggerSkill("#aoyi") {
        events << GameStart << CardUsed << EventAcquireSkill << EventLoseSkill;

        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == GameStart || (triggerEvent == EventAcquireSkill && data.toString() == "aoyi")){
            if (player->getMark("aoyi_limit") == 0 && player->hasSkill("aoyi")){
                room->setPlayerMark(player, "aoyi_limit", 1);
                room->setPlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick", false);
            }
        }
        if (triggerEvent == EventLoseSkill && data.toString() == "aoyi") {
            if (player->getMark("aoyi_limit") > 0 && !player->hasSkill("aoyi")){
                room->setPlayerMark(player, "aoyi_limit", 0);
                room->removePlayerCardLimitation(player, "use", "TrickCard+^DelayedTrick$0");
            }
        }
        else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("IceSlash"))
                return false;
            if (!player->hasSkill("aoyi") || player != use.from)
                return false;

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
            if (choice == "aoyi1"){
                room->touhouLogmessage("#InvokeSkill", player, "aoyi");
                room->notifySkillInvoked(player, objectName());
                player->drawCards(player->getLostHp());
            }
            else{

                ServerPlayer *s = room->askForPlayerChosen(player, all, "aoyi", "aoyi_chosenplayer", true, true);
                int to_throw = room->askForCardChosen(player, s, "ej", "aoyi", false, Card::MethodDiscard);
                room->throwCard(to_throw, s, player);
            }
        }
        return false;
    }
};

class aoyi : public FilterSkill {
public:
    aoyi() : FilterSkill("aoyi") {
    }

    virtual bool viewFilter(const Card *to_select) const{
        return  to_select->isNDTrick();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        IceSlash *slash = new IceSlash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class aoyi_mod : public TargetModSkill {
public:
    aoyi_mod() : TargetModSkill("#aoyi_mod") {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *card) const{
        if (from->hasSkill("aoyi") && card->isKindOf("IceSlash"))
            return 1000;
        else
            return 0;
    }

};

class shikong : public TargetModSkill {
public:
    shikong() : TargetModSkill("shikong") {
        pattern = "Slash";
    }

    static int shikong_modNum(const Player *player, const Card *slash) {

        int num = 0;
        int rangefix = 0;
        QList<int> ids = slash->getSubcards();
        if (player->getWeapon() != NULL && ids.contains(player->getWeapon()->getId())){
            if (player->getAttackRange() > player->getAttackRange(false))
                rangefix = rangefix + player->getAttackRange() - player->getAttackRange(false);
        }
        if (player->getOffensiveHorse() != NULL
            && ids.contains(player->getOffensiveHorse()->getId()))
            rangefix = rangefix + 1;

        foreach(const Player *p, player->getAliveSiblings()){
            if ((player->inMyAttackRange(p) && player->canSlash(p, slash, true, rangefix))
                || Slash::IsSpecificAssignee(p, player, slash))
                num = num + 1;
        }
        return qMax(1, num);
    }

    virtual int getExtraTargetNum(const Player *player, const Card *card) const{
        if (player->hasSkill(objectName()) && player->getPhase() == Player::Play && card->isKindOf("Slash"))
            return shikong_modNum(player, card) - 1;
        else
            return 0;
    }
};

class ronghui : public TriggerSkill {
public:
    ronghui() : TriggerSkill("ronghui") {
        events << DamageCaused;
        frequency = Compulsory;
    }



    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::Play)
            return false;
        DamageStruct damage = data.value<DamageStruct>();
        const Card *card = damage.card;
        if (damage.chain || damage.transfer || !damage.by_user)
            return false;
        if (damage.from == NULL || damage.from == damage.to)
            return false;

        if (card->isKindOf("Slash") && damage.to->getCards("e").length() > 0) {
            DummyCard *dummy = new DummyCard;
            dummy->deleteLater();
            foreach(const Card *c, damage.to->getCards("e")){
                if (player->canDiscard(damage.to, c->getEffectiveId())){
                    dummy->addSubcard(c);
                }
            }
            if (dummy->subcardsLength() > 0){
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
                room->throwCard(dummy, damage.to, player);
            }
        }
        return false;
    }
};


class jubian : public TriggerSkill {
public:
    jubian() : TriggerSkill("jubian") {
        events << Damage << CardFinished;
        frequency = Compulsory;
    }



    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::Play)
            return false;
        if (triggerEvent == Damage){
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card == NULL)
                return false;
            if (damage.card->hasFlag("jubian_card")) {
                if (!damage.card->hasFlag("jubian_used"))
                    room->setCardFlag(damage.card, "jubian_used");
            }
            else
                room->setCardFlag(damage.card, "jubian_card");
        }
        else if (triggerEvent == CardFinished) {
            const Card *card = data.value<CardUseStruct>().card;
            if (card->hasFlag("jubian_card") && card->hasFlag("jubian_used")){
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());

                RecoverStruct recov;
                recov.who = player;
                recov.recover = 1;
                room->recover(player, recov);
            }
        }
        return false;
    }
};

class hengxing : public TriggerSkill {
public:
    hengxing() : TriggerSkill("hengxing") {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Finish) {
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            room->loseHp(player);
            player->drawCards(3);
        }
        return false;
    }
};


class huanmeng : public TriggerSkill {
public:
    huanmeng() : TriggerSkill("huanmeng") {
        events << GameStart << PreHpLost << EventPhaseStart << EventPhaseChanging;
        frequency = Eternal;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == GameStart && player->isLord()){
            room->setPlayerProperty(player, "maxhp", 0);
            room->setPlayerProperty(player, "hp", 0);
        }
        else if (triggerEvent == PreHpLost){
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            return true;
        }
        else if (triggerEvent == EventPhaseStart 
            && player->getPhase() == Player::RoundStart) {
            if (player->getHandcardNum() == 0) {
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
                room->killPlayer(player);
            }
        }
        else if (triggerEvent == EventPhaseChanging){
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Draw) {
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
                player->skip(Player::Draw);
            }
            if (change.to == Player::Discard){
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
                player->skip(Player::Discard);
            }
        }
        return false;
    }
};
class cuixiang : public TriggerSkill {
public:
    cuixiang() : TriggerSkill("cuixiang") {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::Start)
            return false;

        room->touhouLogmessage("#TriggerSkill", player, objectName());
        room->notifySkillInvoked(player, objectName());
        QList<int> idlist;
        foreach(ServerPlayer *p, room->getOtherPlayers(player))
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
            
        foreach(ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->canDiscard(p, "h")) {
                const Card *cards = room->askForExchange(p, objectName(), 1, false, "cuixiang-exchange:" + player->objectName() + ":" + objectName());
                int id = cards->getSubcards().first();
                room->throwCard(id, p, p);
                //we need id to check cardplace,
                //since skill "jinian",  the last handcard will be return.
                if (room->getCardPlace(id) == Player::DiscardPile)
                    idlist << id;
            }
            else{
                QList<int> cards = room->getNCards(1);
                room->throwCard(cards.first(), NULL, p);
                idlist << cards.first();
            }
        }

        int x = qMin(idlist.length(), 2);
        if (x == 0)
            return false;
        room->fillAG(idlist, NULL);
        for (int i = 0; i < x; i++) {

            int card_id = room->askForAG(player, idlist, false, "cuixiang");
            //just for displaying chosen card in ag container
            room->takeAG(player, card_id, false);
            room->obtainCard(player, card_id, true);
            idlist.removeOne(card_id);
        }
        room->clearAG();
        return false;
    }
};
class xuying : public TriggerSkill {
public:
    xuying() : TriggerSkill("xuying") {
        events << SlashHit << SlashMissed;
        frequency = Eternal;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (triggerEvent == SlashHit && effect.to->hasSkill(objectName())) {
            if (effect.to->getHandcardNum() > 0){
                int x = qMax(effect.to->getHandcardNum() / 2, 1);
                room->touhouLogmessage("#TriggerSkill", effect.to, objectName());
                room->notifySkillInvoked(effect.to, objectName());
                room->askForDiscard(effect.to, "xuying", x, x, false, false, "xuying_discard:" + QString::number(x));
            }
        }
        if (triggerEvent == SlashMissed && effect.to->hasSkill(objectName())) {
            room->touhouLogmessage("#TriggerSkill", effect.to, objectName());
            room->notifySkillInvoked(effect.to, objectName());
            effect.to->drawCards(1);
        }
        return false;
    }
};


class kuangyan : public TriggerSkill {
public:
    kuangyan() : TriggerSkill("kuangyan") {
        events << Dying;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *who = room->getCurrentDyingPlayer();
        if (who != player)
            return false;
        ServerPlayer *current = room->getCurrent();
        if (current == NULL || !current->isAlive() || player == current)
            return false;
        if (player->askForSkillInvoke(objectName(), "recover:" + current->objectName())){
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), current->objectName());
            
            player->gainMark("@kinki");
            RecoverStruct recover;
            recover.recover = 1 - player->getHp();
            room->recover(player, recover);

            room->damage(DamageStruct(objectName(), player, current));
        }
        return false;
    }
};

huimieCard::huimieCard() {
    mute = true;
}
bool huimieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return(targets.isEmpty() && to_select != Self);
}
void huimieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    source->gainMark("@kinki");
    if (!target->isChained()){
        target->setChained(!target->isChained());
        Sanguosha->playSystemAudioEffect("chained");
        room->broadcastProperty(target, "chained");
        room->setEmotion(target, "chain");
    }
    room->damage(DamageStruct("huimie", source, target, 1, DamageStruct::Fire));

}


class huimie : public ZeroCardViewAsSkill {
public:
    huimie() : ZeroCardViewAsSkill("huimie") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("huimieCard");
    }

    virtual const Card *viewAs() const{
        return new huimieCard;
    }
};

class jinguo : public TriggerSkill {
public:
    jinguo() : TriggerSkill("jinguo") {
        events << EventPhaseEnd;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Play){
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

            if (!judge.isGood()){
                int x = player->getMark("@kinki");
                if (x == 0)
                    return false;
                int y = x / 2;
                if (x > player->getCards("he").length())
                    room->loseHp(player, y);
                else{
                    if (!room->askForDiscard(player, objectName(), x, x, true, true, "@jinguo:" + QString::number(x) + ":" + QString::number(y)))
                        room->loseHp(player, y);
                }
            }
        }
        return false;
    }
};


class shicao : public TriggerSkill {
public:
    shicao() : TriggerSkill("shicao") {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Start) {
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            if (player->getMark("@clock") > 0)
                player->loseAllMarks("@clock");
            if (player->getMark("touhou-extra") == 0)
                player->gainMark("@clock", 1);
        }
        return false;
    }
};

class shiting : public TriggerSkill {
public:
    shiting() : TriggerSkill("shiting") {
        events << TurnStart;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        //since limit skill,this character can play one more extraturn in a turnstart event,
        //do not use trigger turn start
        ServerPlayer *skillowner = room->findPlayerBySkillName(objectName());
        while (true){
            ServerPlayer *current = room->getCurrent();
            if (current && current==player &&current->isAlive()
                && skillowner &&  skillowner->isAlive()
                && room->canInsertExtraTurn()  && skillowner->getMark("@clock") > 0){
                QString prompt = "extraturn:" + current->objectName();
                if (room->askForSkillInvoke(skillowner, objectName(), prompt)) {
                    skillowner->loseAllMarks("@clock");
                    room->touhouLogmessage("#touhouExtraTurn", skillowner, NULL);

                    skillowner->gainAnExtraTurn();
                }
                else
                    break;
            }
            else
                break;
        }
        //room->getThread()->trigger(TurnStart, room, player, data);
        return false;
    }
};

class huanzai : public TriggerSkill {
public:
    huanzai() : TriggerSkill("huanzai") {
        events << EventPhaseStart;
        frequency = Limited;
        limit_mark = "@huanzai";
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Finish) {
            if (player->getMark("@clock") == 0 && player->getMark("@huanzai") > 0){
                if (room->askForSkillInvoke(player, objectName())){
                    room->doLightbox("$huanzaiAnimate", 4000);
                    player->gainMark("@clock", 1);
                    room->removePlayerMark(player, "@huanzai");
                }
            }
        }
        return false;
    }
};
class shanghun : public TriggerSkill {
public:
    shanghun() : TriggerSkill("shanghun") {
        events << Damaged;
        frequency = Limited;
        limit_mark = "@shanghun";
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getMark("@clock") == 0 && player->getMark("@shanghun") > 0){
            if (room->askForSkillInvoke(player, objectName())){
                room->doLightbox("$shanghunAnimate", 4000);
                player->gainMark("@clock", 1);
                room->removePlayerMark(player, "@shanghun");
            }
        }
        return false;
    }
};


class banling : public TriggerSkill {
public:
    banling() : TriggerSkill("banling") {
        events << GameStart << PreHpLost << DamageInflicted << PreHpRecover;
        frequency = Eternal;
    }
    virtual int getPriority(TriggerEvent) const{ //important?
        return -1;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == GameStart) {
            room->setPlayerMark(player, "lingtili", player->getMaxHp());
            room->setPlayerMark(player, "rentili", player->getMaxHp());
            room->setPlayerProperty(player, "hp", player->getMaxHp());
        }
        else if (triggerEvent == PreHpLost) {
            int x = player->getMark("lingtili");
            int y = player->getMark("rentili");
            int minus_x = player->getMark("minus_lingtili");
            int minus_y = player->getMark("minus_rentili");
            player->tag["banling_minus"] = data;
            for (int i = 0; i < data.toInt(); i++) {

                QStringList choices;

                choices << "lingtili";
                choices << "rentili";
                QString choice = room->askForChoice(player, "banling_minus", choices.join("+"));
                if (choice == "lingtili") {
                    if (x > 0)
                        x = x - 1;
                    else
                        minus_x = minus_x + 1;
                    room->touhouLogmessage("#lingtilidamage", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                }
                else if (choice == "rentili") {
                    if (y > 0)
                        y = y - 1;
                    else
                        minus_y = minus_y + 1;
                    room->touhouLogmessage("#rentilidamage", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                }
            }
            room->setPlayerMark(player, "lingtili", x);
            room->setPlayerMark(player, "rentili", y);
            room->setPlayerMark(player, "minus_lingtili", minus_x);
            room->setPlayerMark(player, "minus_rentili", minus_y);
            room->notifySkillInvoked(player, objectName());
            room->setPlayerProperty(player, "hp", qMin(x - minus_x, y - minus_y));


            LogMessage log;
            log.type = "#LoseHp";
            log.from = player;
            log.arg = QString::number(data.toInt());
            room->sendLog(log);
            log.arg = QString::number(player->getHp());
            log.arg2 = QString::number(player->getMaxHp());
            log.type = "#GetHp";
            room->sendLog(log);

            room->getThread()->trigger(PostHpReduced, room, player, data);
            return true;
        }
        else if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            int x = player->getMark("lingtili");
            int y = player->getMark("rentili");
            int minus_x = player->getMark("minus_lingtili");
            int minus_y = player->getMark("minus_rentili");
            player->tag["banling_minus"] = QVariant::fromValue(damage.damage);
            for (int i = 0; i < damage.damage; i++) {

                QStringList choices;

                choices << "lingtili";
                choices << "rentili";
                QString choice = room->askForChoice(player, "banling_minus", choices.join("+"));
                if (choice == "lingtili"){
                    if (x > 0)
                        x = x - 1;
                    else
                        minus_x = minus_x + 1;
                    room->touhouLogmessage("#lingtilidamage", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                }
                if (choice == "rentili") {
                    if (y > 0)
                        y = y - 1;
                    else
                        minus_y = minus_y + 1;
                    room->touhouLogmessage("#rentilidamage", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                }
            }
            room->setPlayerMark(player, "lingtili", x);
            room->setPlayerMark(player, "rentili", y);
            room->setPlayerMark(player, "minus_lingtili", minus_x);
            room->setPlayerMark(player, "minus_rentili", minus_y);

            QVariant qdata = QVariant::fromValue(damage);
            room->getThread()->trigger(PreDamageDone, room, damage.to, qdata);

            room->getThread()->trigger(DamageDone, room, damage.to, qdata);


            room->notifySkillInvoked(player, objectName());
            int z = qMin(x - minus_x, y - minus_y);
            room->setPlayerProperty(damage.to, "hp", z);

            room->getThread()->trigger(Damage, room, damage.from, qdata);

            room->getThread()->trigger(Damaged, room, damage.to, qdata);

            room->getThread()->trigger(DamageComplete, room, damage.to, qdata);

            return true;
        }
        else if (triggerEvent == PreHpRecover) {
            RecoverStruct recov = data.value<RecoverStruct>();
            for (int i = 0; i < recov.recover; i++){
                int x = player->getMark("lingtili");
                int y = player->getMark("rentili");
                int minus_x = player->getMark("minus_lingtili");
                int minus_y = player->getMark("minus_rentili");
                QString choice = "rentili";

                if (x < player->getMaxHp() && y < player->getMaxHp())
                    choice = room->askForChoice(player, "banling_plus", "lingtili+rentili");
                else{
                    if (x == player->getMaxHp())
                        choice = "rentili";
                    else
                        choice = "lingtili";
                }
                if (choice == "rentili") {
                    if (minus_y == 0)
                        room->setPlayerMark(player, "rentili", y + 1);
                    else{
                        minus_y = minus_y - 1;
                        room->setPlayerMark(player, "minus_rentili", minus_y);
                    }
                    room->touhouLogmessage("#rentilirecover", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                    continue;
                }
                if (choice == "lingtili") {
                    if (minus_x == 0)
                        room->setPlayerMark(player, "lingtili", x + 1);
                    else{
                        minus_x = minus_x - 1;
                        room->setPlayerMark(player, "minus_lingtili", minus_x);
                    }
                    room->touhouLogmessage("#lingtilirecover", player, "banling", QList<ServerPlayer *>(), QString::number(1));
                    continue;
                }
            }
            room->notifySkillInvoked(player, objectName());
        }
        return false;
    }
};

class rengui : public PhaseChangeSkill {
public:
    rengui() :PhaseChangeSkill("rengui") {
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if (!player->hasSkill("banling"))
            return false;
        Room *room = player->getRoom();
        if (player->getPhase() == Player::Start && player->hasSkill("banling")) {//左慈 zun 依姬时会有问题
            if (player->getLingHp() < player->getMaxHp()){ //使用专门的gethp函数 直接取mark值也行
                int  x = player->getMaxHp() - player->getLingHp();
                x = qMin(x, 2);
                ServerPlayer *s = room->askForPlayerChosen(player, room->getAlivePlayers(), "renguidraw", "@rengui-draw:" + QString::number(x), true, true);
                if (s != NULL)
                    room->notifySkillInvoked(player, objectName());
                s->drawCards(x);
            }
            if (player->getRenHp() < player->getMaxHp()){
                int y = player->getMaxHp() - player->getRenHp();
                y = qMin(y, 2);
                QList<ServerPlayer *> all;
                foreach(ServerPlayer *p, room->getAlivePlayers()){
                    if (player->canDiscard(p, "he"))
                        all << p;
                }
                if (!all.isEmpty()) {
                    ServerPlayer *s = room->askForPlayerChosen(player, all, "renguidiscard", "@rengui-discard:" + QString::number(y), true, true);
                    if (s != NULL) {
                        room->notifySkillInvoked(player, objectName());
                        DummyCard *dummy = new DummyCard;
                        dummy->deleteLater();
                        QList<int> card_ids;
                        QList<Player::Place> original_places;
                        s->setFlags("rengui_InTempMoving");
                        for (int i = 0; i < y; i++){
                            if (!player->canDiscard(s, "he"))
                                break;
                            int id = room->askForCardChosen(player, s, "he", objectName(), false, Card::MethodDiscard);
                            card_ids << id;
                            original_places << room->getCardPlace(id);
                            dummy->addSubcard(id);
                            s->addToPile("#rengui", id, false);
                        }
                        for (int i = 0; i < dummy->subcardsLength(); i++) {
                            Card *c = Sanguosha->getCard(card_ids.at(i));
                            room->moveCardTo(c, s, original_places.at(i), false);
                        }
                        s->setFlags("-rengui_InTempMoving");
                        if (dummy->subcardsLength() > 0)
                            room->throwCard(dummy, s, player);

                    }
                }
            }
        }
        return false;
    }
};

class ningshi : public TriggerSkill {
public:
    ningshi() : TriggerSkill("ningshi") {
        events << TargetConfirmed;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (player->hasSkill(objectName()) && player->getPhase() == Player::Play
            && use.from == player && use.to.length() == 1 && !use.to.contains(player)) {
            if (use.card->isKindOf("Slash") || use.card->isKindOf("TrickCard")){
                ServerPlayer *target = use.to.first();
                room->touhouLogmessage("#TriggerSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
                room->loseHp(target);

            }
        }
        return false;
    }
};

class gaoao : public TriggerSkill {
public:
    gaoao() : TriggerSkill("gaoao") {
        events << BeforeCardsMove;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() != Player::NotActive)
            return false;
        if (room->getTag("FirstRound").toBool())
            return false;
        //need special process when processing takeAG and askforrende
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to != NULL && move.to == player){
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->notifySkillInvoked(player, objectName());
            //Card *dummy = Sanguosha->cloneCard("Slash");
            //foreach (int id, move.card_ids)                             
            //    dummy->addSubcard(Sanguosha->getCard(id)); 
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), "");
            //room->throwCard(dummy,reason,NULL);

            move.to = NULL;
            move.origin_to = NULL;
            move.origin_to_place = Player::DiscardPile;
            move.reason = reason;
            move.to_place = Player::DiscardPile;


            data = QVariant::fromValue(move);
            foreach(ServerPlayer *p, room->getAllPlayers())
                room->getThread()->trigger(BeforeCardsMove, room, p, data);
            //return true;    
        }
        return false;
    }
};



shenshouCard::shenshouCard() {
    mute = true;
    will_throw = false;
}
void shenshouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
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
    if (source->hasSkill("shenshou") && source->getHandcardNum() == 1){
        x = 1;
        y = 1;
        z = 1;
    }
    QStringList choices;

    if (x == 1){
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

    while (!choices.isEmpty()){
        source->tag["shenshou_x"] = QVariant::fromValue(x);
        source->tag["shenshou_y"] = QVariant::fromValue(y);
        source->tag["shenshou_z"] = QVariant::fromValue(z);
        source->tag["shenshou_target"] = QVariant::fromValue(target);
        QString choice = room->askForChoice(source, "shenshou", choices.join("+"));
        choices.removeOne(choice);
        if (choice == "cancel")
            break;
        if (choice == "shenshou_slash"){
            x = 0;
            QList<ServerPlayer *>listt;
            foreach(ServerPlayer *p, room->getOtherPlayers(target)) {
                if (target->inMyAttackRange(p) && target->canSlash(p, NULL, false))
                    listt << p;
            }
            if (listt.length() > 0){
                ServerPlayer *slashtarget = room->askForPlayerChosen(source, listt, "shenshou", "@shenshou-slash:" + target->objectName());
                if (slashtarget != NULL) {
                    Slash *slash = new Slash(Card::NoSuit, 0);
                    slash->setSkillName("shenshou");
                    room->useCard(CardUseStruct(slash, target, slashtarget));
                }
            }
        }
        if (choice == "shenshou_obtain"){
            y = 0;
            QList<ServerPlayer *>listt;
            foreach(ServerPlayer *p, room->getOtherPlayers(target)) {
                if (target->inMyAttackRange(p) && !p->isKongcheng())
                    listt << p;
            }
            if (listt.length() > 0){
                ServerPlayer *cardtarget = room->askForPlayerChosen(source, listt, "shenshou", "@shenshou-obtain:" + target->objectName());
                if (cardtarget != NULL) {
                    int card1 = room->askForCardChosen(target, cardtarget, "h", "shenshou");
                    room->obtainCard(target, card1, false);
                }
            }
        }
        if (choice == "shenshou_draw"){
            z = 0;
            source->drawCards(1);
        }
        source->tag.remove("shenshou_x");
        source->tag.remove("shenshou_y");
        source->tag.remove("shenshou_z");

        source->tag.remove("shenshou_target");
    }
}


class shenshou : public OneCardViewAsSkill {
public:
    shenshou() :OneCardViewAsSkill("shenshou") {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("shenshouCard");
    }


    virtual const Card *viewAs(const Card *originalCard) const{
        shenshouCard *card = new shenshouCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class quanjie : public PhaseChangeSkill {
public:
    quanjie() :PhaseChangeSkill("quanjie") {
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }
    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        if (player->getPhase() == Player::Play){
            ServerPlayer *ymsnd = room->findPlayerBySkillName(objectName());
            if (ymsnd == NULL || player == ymsnd)
                return false;
            room->setPlayerMark(player, objectName(), 0);
            if (ymsnd->askForSkillInvoke(objectName(), QVariant::fromValue(player))){
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, ymsnd->objectName(), player->objectName());
            
                const Card *card = room->askForCard(player, "%slash,%thunder_slash,%fire_slash", "@quanjie-discard");
                if (card == NULL){
                    player->drawCards(1);
                    room->setPlayerMark(player, objectName(), 1);
                    room->setPlayerCardLimitation(player, "use", "Slash", true);
                }

            }
        }
        return false;
    }
};

class duanzuicount : public TriggerSkill {
public:
    duanzuicount() : TriggerSkill("#duanzui-count") {
        events << Death << TurnStart;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who == player)
                room->setTag("duanzui", true);
        }
        else if (triggerEvent == TurnStart)//(player->getPhase() == Player::NotActive)
            room->setTag("duanzui", false);

        return false;
    }
};

class duanzui : public TriggerSkill {
public:
    duanzui() : TriggerSkill("duanzui") {
        events << EventPhaseStart;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer * current = room->getCurrent();
        if (current->getPhase() == Player::NotActive) {
            ServerPlayer *ymsnd = room->findPlayerBySkillName(objectName());
            if (ymsnd == NULL || !room->getTag("duanzui").toBool())
                return false;

            room->setTag("duanzui", false);
            if (current == ymsnd)
                return false;
            if (room->canInsertExtraTurn()){
                if (ymsnd->askForSkillInvoke(objectName())){
                    if (!ymsnd->faceUp())
                        ymsnd->turnOver();

                    ymsnd->gainMark("@duanzui-extra");
                    QList<ServerPlayer *> logto;
                    logto << current->getNext();
                    room->touhouLogmessage("#touhouExtraTurn", ymsnd, NULL, logto);
                    ymsnd->gainAnExtraTurn();
                    ymsnd->loseMark("@duanzui-extra");
                }
            }
            else if (!ymsnd->faceUp() && ymsnd->askForSkillInvoke(objectName()))
                ymsnd->turnOver();
        }
        return false;
    }
};
class duanzuishenpan : public TriggerSkill {
public:
    duanzuishenpan() : TriggerSkill("#duanzui-shenpan") {
        events << PreMarkChange;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return (target->getMark("@duanzui-extra") > 0 || target->hasSkill("duanzui")) && target->getPhase() == Player::NotActive;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        MarkChangeStruct change = data.value<MarkChangeStruct>();
        if (change.name == "@duanzui-extra") {
            if (change.num > 0 && player->getMark("@duanzui-extra") == 0 && !player->hasSkill("shenpan"))
                room->handleAcquireDetachSkills(player, "shenpan");
            else if (change.num < 0 && player->getMark("@duanzui-extra") + change.num <= 0 && player->hasSkill("shenpan"))
                room->handleAcquireDetachSkills(player, "-shenpan");
        }

        return false;
    }
};




class hualong : public TriggerSkill {
public:
    hualong() : TriggerSkill("hualong") {
        events << Dying;
        frequency = Wake;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{


        ServerPlayer *who = room->getCurrentDyingPlayer();
        if (who != player)
            return false;

        if (player->getMark("hualong") == 0){
            room->doLightbox("$hualongAnimate", 4000);
            room->touhouLogmessage("#YizhiWake", player, objectName());
            room->notifySkillInvoked(player, objectName());
            if (room->changeMaxHpForAwakenSkill(player, 1)){
                RecoverStruct recover;
                recover.recover = player->getMaxHp() - player->getHp();;
                room->recover(player, recover);
                room->setPlayerMark(player, "hualong", 1);
            }
        }
        return false;
    }
};

class luanwu : public ViewAsSkill {
public:
    luanwu() : ViewAsSkill("luanwu") {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return (pattern == "slash" || pattern == "jink");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (Self->getMark("hualong") > 0)
            return selected.isEmpty();
        else if (selected.isEmpty())
            return !to_select->isEquipped();
        if (selected.length() == 1)
            return !to_select->isEquipped() && selected.first()->sameColorWith(to_select);
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        Sanguosha->getCurrentCardUseReason();
        if (cards.length() == 0)
            return NULL;
        else if (cards.length() == 1 && Self->getMark("hualong") > 0){
            const Card *card = cards.first();
            if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
                Slash *slash = new Slash(card->getSuit(), card->getNumber());
                slash->addSubcards(cards);
                slash->setSkillName(objectName());
                return slash;
            }
            else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE || Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
                QString pattern = Sanguosha->getCurrentCardUsePattern();
                if (pattern == "jink"){
                    Jink *jink = new Jink(card->getSuit(), card->getNumber());
                    jink->addSubcards(cards);
                    jink->setSkillName(objectName());
                    return jink;
                }
                else if (pattern == "slash"){
                    Slash *slash = new Slash(card->getSuit(), card->getNumber());
                    slash->addSubcards(cards);
                    slash->setSkillName(objectName());
                    return slash;
                }
            }
        }
        else if (cards.length() == 2){
            if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY){
                Slash *slash = new Slash(Card::SuitToBeDecided, 0);
                slash->addSubcards(cards);
                slash->setSkillName(objectName());
                return slash;
            }
            else if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE || Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE){
                QString pattern = Sanguosha->getCurrentCardUsePattern();
                if (pattern == "jink"){
                    Jink *jink = new Jink(Card::SuitToBeDecided, 0);
                    jink->addSubcards(cards);
                    jink->setSkillName(objectName());
                    return jink;
                }
                else if (pattern == "slash") {
                    Slash *slash = new Slash(Card::SuitToBeDecided, 0);
                    slash->addSubcards(cards);
                    slash->setSkillName(objectName());
                    return slash;
                }
            }
        }
        return NULL;
    }
};

class longwei : public TriggerSkill {
public:
    longwei() : TriggerSkill("longwei") {
        events << TargetConfirming;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirming){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from != NULL && player != use.from  &&  use.to.contains(player)
                && (use.card->isKindOf("Slash") || use.card->isKindOf("TrickCard"))){

                if (player->getMark("hualong") > 0) {
                    if (player->canDiscard(use.from, "he") && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(use.from))){
                        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.from->objectName());
            
                        int id = room->askForCardChosen(player, use.from, "he", objectName(), false, Card::MethodDiscard);
                        room->throwCard(id, use.from, player);
                        if (player->canDiscard(use.from, "he")){
                            int id1 = room->askForCardChosen(player, use.from, "he", objectName(), false, Card::MethodDiscard);
                            room->throwCard(id1, use.from, player);
                        }
                    }
                }
                else{
                    if (use.from->canDiscard(use.from, "he") && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(use.from))){
                        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.from->objectName());
            
                        room->askForDiscard(use.from, objectName(), 1, 1, false, true, "@longwei-askfordiscard:" + player->objectName() + ":" + QString::number(1));
                        
                    }
                }

            }
        }
        return false;
    }
};


class qiannian_draw : public DrawCardsSkill {
public:
    qiannian_draw() : DrawCardsSkill("#qiannian_draw") {
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        return  n + player->getMark("@qiannian");
    }
};

class qiannian_max : public MaxCardsSkill {
public:
    qiannian_max() : MaxCardsSkill("#qiannian_max") {
    }

    virtual int getExtra(const Player *target) const{
        int n = target->getMark("@qiannian");
        if (n > 0 && target->hasSkill("qiannian"))
            return 2 * n;
        else
            return 0;
    }
};
class qiannian : public TriggerSkill {
public:
    qiannian() : TriggerSkill("qiannian") {
        events << GameStart << PreMarkChange;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        LogMessage log;
        log.type = "#TriggerSkill";
        log.from = player;
        log.arg = objectName();

        if (triggerEvent == GameStart) {
            player->gainMark("@qiannian", 1);
        }
        if (triggerEvent == PreMarkChange) {
            //gainMark when Room::swapPile()
            MarkChangeStruct change = data.value<MarkChangeStruct>();
            if (change.name == "@qiannian" && change.num > 0){
                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
            }
        }
        return false;
    }
};


class qinlue : public TriggerSkill {
public:
    qinlue() : TriggerSkill("qinlue") {
        events << EventPhaseChanging;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (source == NULL) return false;
        ServerPlayer *current = room->getCurrent();
        if (current->getMark("touhou-extra") > 0)  return false;
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Play && current != source && !current->isSkipped(Player::Play)){
            QString  prompt = "@qinlue-discard:" + current->objectName();
            const Card *card = room->askForCard(source, "Slash,EquipCard", prompt, QVariant::fromValue(current), Card::MethodDiscard, current, false, "qinlue");
            if (card != NULL){
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), current->objectName());
            
                QString  prompt = "@qinlue-discard1:" + source->objectName();
                const Card *card1 = room->askForCard(current, "Jink", prompt, QVariant::fromValue(source), Card::MethodDiscard);
                if (!card1){
                    current->skip(Player::Play);

                    source->tag["qinlue_current"] = QVariant::fromValue(current);
                    room->setPlayerMark(source, "touhou-extra", 1);
                    source->setFlags("qinlue");
                    source->setPhase(Player::Play);
                    current->setPhase(Player::NotActive);
                    room->setCurrent(source);
                    room->broadcastProperty(source, "phase");
                    room->broadcastProperty(current, "phase");
                    RoomThread *thread = room->getThread();
                    if (!thread->trigger(EventPhaseStart, room, source))
                        thread->trigger(EventPhaseProceeding, room, source);

                    thread->trigger(EventPhaseEnd, room, source);
                    room->setCurrent(current);

                    source->changePhase(Player::Play, Player::NotActive);

                    room->setPlayerMark(source, "touhou-extra", 0);
                    current->setPhase(Player::PhaseNone);

                    source->setFlags("-qinlue");
                    room->broadcastProperty(source, "phase");
                }
            }
        }

        return false;
    }
};

class qinlue_effect : public TriggerSkill {
public:
    qinlue_effect() : TriggerSkill("#qinlue_effect") {
        events << EventPhaseChanging << EventPhaseStart;
    }
    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL && target->hasFlag("qinlue"));
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart) {
            if (player->hasFlag("qinlue") && player->getPhase() == Player::Play){

                ServerPlayer *target = player->tag["qinlue_current"].value<ServerPlayer *>();
                if (target != NULL && target->isAlive()) {
                    QList<int> ids;
                    foreach(const Card *c, player->getHandcards())
                        ids << (c->getId());
                    player->addToPile("qinlue", ids, false);
                    DummyCard *dummy = new DummyCard;
                    dummy->deleteLater();
                    dummy->addSubcards(target->getHandcards());
                    room->obtainCard(player, dummy, false);
                }
            }
        }
        else if (triggerEvent == EventPhaseChanging) {
            if (player->hasFlag("qinlue")){
                ServerPlayer *target = player->tag["qinlue_current"].value<ServerPlayer *>();
                if (target != NULL && target->isAlive()) {
                    DummyCard *dummy = new DummyCard;
                    dummy->deleteLater();
                    dummy->addSubcards(player->getHandcards());
                    room->obtainCard(target, dummy, false);
                }
                if (!player->getPile("qinlue").isEmpty()){
                    DummyCard *dummy1 = new DummyCard;
                    dummy1->deleteLater();
                    foreach(int c, player->getPile("qinlue"))
                        dummy1->addSubcard(c);
                    room->obtainCard(player, dummy1, false);
                }
                if (target != NULL)  //for siyu broken
                    room->setCurrent(target);

            }
        }

        return false;
    }
};



chaorenpreventrecast::chaorenpreventrecast() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodUse;
}
bool chaorenpreventrecast::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    Card *card = Sanguosha->getCard(Self->property("chaoren").toInt());
    if (Sanguosha->isProhibited(Self, to_select, card))
        return false;
    return card->targetFilter(targets, to_select, Self);

}
bool chaorenpreventrecast::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (targets.isEmpty())
        return false;
    Card *card = Sanguosha->getCard(Self->property("chaoren").toInt());
    foreach(const Player *p, targets){
        if (Sanguosha->isProhibited(Self, p, card))
            return false;
    }
    return card->targetsFeasible(targets, Self);

}
const Card *chaorenpreventrecast::validate(CardUseStruct &card_use) const{
    Card *card = Sanguosha->getCard(card_use.from->property("chaoren").toInt());
    card->setSkillName("chaoren");
    return card;
}


class chaorenvs : public ZeroCardViewAsSkill {
public:
    chaorenvs() : ZeroCardViewAsSkill("chaoren") {
    }
    static bool cardCanRecast(Card *card){
        return card->canRecast() && !card->isKindOf("Weapon");
    }
    virtual bool isEnabledAtPlay(const Player *player) const{
        Card *card = Sanguosha->getCard(player->property("chaoren").toInt());
        if (card != NULL && card->isAvailable(player)){
            if (cardCanRecast(card) && player->isCardLimited(card, Card::MethodUse))
                return false;
            return true;
        }
        return false;
    }
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        Card *card = Sanguosha->getCard(player->property("chaoren").toInt());
        if (card == NULL)
            return false;
        QStringList realpattern = pattern.split("+");
        if (player->hasFlag("Global_PreventPeach"))
            realpattern.removeOne("peach");
        Card::HandlingMethod handlingmethod = (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) ? Card::MethodUse : Card::MethodResponse;
        if (realpattern.contains("slash")){
            realpattern << "fire_slash";
            realpattern << "thunder_slash";
        }
        return realpattern.contains(card->objectName()) && !player->isCardLimited(card, handlingmethod);
    }
    virtual bool isEnabledAtNullification(const ServerPlayer *player) const{
        Card *card = Sanguosha->getCard(player->property("chaoren").toInt());
        if (card == NULL)
            return false;
        return card->isKindOf("Nullification") && !player->isCardLimited(card, Card::MethodUse);

    }


    virtual const Card *viewAs() const{
        Card *card = Sanguosha->getCard(Self->property("chaoren").toInt());
        if (card != NULL) {
            if (cardCanRecast(card))
                return new chaorenpreventrecast;
            else{
                card->setSkillName("chaoren");
                return card;
            }
        }
        return NULL;
    }
};

class chaoren : public TriggerSkill {
public:
    chaoren() : TriggerSkill("chaoren") {
        events << PreCardUsed << CardResponded << CardUsed << CardFinished << CardsMoveOneTime << TargetConfirming;
        view_as_skill = new chaorenvs;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isVirtualCard() && use.card->subcardsLength() != 1)
                return false;
            if (player == use.from && player->hasSkill("chaoren") &&
                use.card->getId() == player->property("chaoren").toInt()){
                //if (player==use.from && use.card->getSkillName()==objectName()){
                room->touhouLogmessage("#InvokeSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
            }
            return false;
        }
        if (triggerEvent == CardResponded){
            CardStar card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isVirtualCard() && card_star->subcardsLength() != 1)
                return false;
            if (player == data.value<CardResponseStruct>().m_who && player->hasSkill("chaoren")
                && card_star->getId() == player->property("chaoren").toInt()) {
                //if (player== data.value<CardResponseStruct>().m_who && card_star->getSkillName()==objectName()) {
                room->touhouLogmessage("#InvokeSkill", player, objectName());
                room->notifySkillInvoked(player, objectName());
            }
            return false;
        }

        ServerPlayer *sbl = room->findPlayerBySkillName(objectName());
        if (sbl == NULL || sbl->isDead())
            return false;
        QList<int> drawpile = room->getDrawPile();
        int firstcard = -1;
        if (!drawpile.isEmpty())
            firstcard = drawpile.first();
        //deal the amazinggrace 
        //update firstcard。。。
        if (triggerEvent == TargetConfirming){
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->isKindOf("AmazingGrace"))
                return false;
        }

        room->setPlayerProperty(sbl, "chaoren", firstcard);

        if (room->getTag("FirstRound").toBool())
            return false;
        if (triggerEvent == CardUsed || triggerEvent == CardFinished){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("AmazingGrace"))
                sbl->setFlags((triggerEvent == CardUsed) ? "agusing" : "-agusing");
        }
        bool invoke = (triggerEvent == CardUsed || triggerEvent == TargetConfirming);
        if (triggerEvent == CardsMoveOneTime && player != NULL && player->isAlive() && player->hasSkill(objectName())) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from_places.contains(Player::DrawPile) || (move.to_place == Player::DrawPile))
                invoke = true;
        }
        if (invoke && firstcard != -1) {
            QList<int> watchlist;
            watchlist << firstcard;
            LogMessage l;
            l.type = "$chaorendrawpile";
            l.card_str = IntList2StringList(watchlist).join("+");

            room->doNotify(sbl, QSanProtocol::S_COMMAND_LOG_SKILL, l.toJsonValue());


            if (!sbl->hasFlag("agusing")) {
                Json::Value gongxinArgs(Json::arrayValue);

                gongxinArgs[0] = QSanProtocol::Utils::toJsonString(QString());
                gongxinArgs[1] = false;
                gongxinArgs[2] = QSanProtocol::Utils::toJsonArray(watchlist);

                room->doNotify(sbl, QSanProtocol::S_COMMAND_SHOW_ALL_CARDS, gongxinArgs);
            }
        }
        return false;
    }
};

class biaoxiang : public TriggerSkill {
public:
    biaoxiang() : TriggerSkill("biaoxiang") {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getMark("biaoxiang") == 0 && player->faceUp()
            && player->getPhase() == Player::Start){
            if (player->getHandcardNum() < 2){
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
            }
        }
        return false;
    }
};

ziwoCard::ziwoCard() {
    mute = true;
    will_throw = true;
    target_fixed = true;
}
void ziwoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    RecoverStruct recov;
    recov.recover = 1;
    recov.who = source;
    room->recover(source, recov);
}

class ziwo : public ViewAsSkill {
public:
    ziwo() : ViewAsSkill("ziwo") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return  player->isWounded();
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        return !to_select->isEquipped() && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() == 2) {
            ziwoCard *card = new ziwoCard;
            card->addSubcards(cards);

            return card;
        }
        else
            return NULL;

    }
};

class shifang : public TriggerSkill {
public:
    shifang() : TriggerSkill("shifang") {
        events << CardsMoveOneTime << BeforeCardsMove;
        frequency = Wake;
    }
    static void koishi_removeskill(Room *room, ServerPlayer *player) {
        foreach(const Skill *skill, player->getVisibleSkillList()) {
            if (skill->getFrequency() == Skill::Wake || skill->isAttachedLordSkill()
                || skill->getFrequency() == Skill::Eternal)
                continue;
            else{
                QString skill_name = skill->objectName();

                room->handleAcquireDetachSkills(player, "-" + skill_name);

            }
        }
    }
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getMark("shifang") > 0)
            return false;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (triggerEvent == BeforeCardsMove){
            if (move.from != NULL && move.from == player && player->getCards("e").length() == 1){
                foreach(int id, move.card_ids){
                    if (room->getCardPlace(id) == Player::PlaceEquip){
                        room->setCardFlag(Sanguosha->getCard(id), "shifang");
                        room->setPlayerFlag(player, "shifangInvoked");
                        break;
                    }
                }
            }
        }
        else if (triggerEvent == CardsMoveOneTime){
            if (move.from != NULL && move.from == player){ //&& move.from_place == Player::Equip
                if (player->hasFlag("shifangInvoked") && player->getMark("shifang") == 0) {
                    bool t = false;
                    foreach(int id, move.card_ids){
                        if (Sanguosha->getCard(id)->hasFlag("shifang")){
                            t = true;
                            break;
                        }
                    }
                    if (!t)
                        return false;
                    room->doLightbox("$shifangAnimate", 4000);
                    room->touhouLogmessage("#ShifangWake", player, objectName());
                    koishi_removeskill(room, player);

                    room->notifySkillInvoked(player, objectName());
                    room->addPlayerMark(player, objectName());
                    int x = player->getMaxHp();
                    if (room->changeMaxHpForAwakenSkill(player, 4 - x)){
                        room->handleAcquireDetachSkills(player, "benwo");
                        room->setPlayerFlag(player, "-shifangInvoked");
                    }
                    return false;
                }
            }
        }
        return false;
    }
};

class benwo : public TriggerSkill {
public:
    benwo() : TriggerSkill("benwo") {
        events << DamageInflicted;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (player->isWounded()){
            if (damage.from != NULL && damage.from->isAlive()){
                player->tag["benwo_target"] = QVariant::fromValue(damage.from);

                int x = player->getMaxHp() - player->getHp();
                QString prompt = "invoke:" + damage.from->objectName() + ":" + QString::number(x);
                if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(damage.from))) {
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());
            
                    player->drawCards(x);
                    room->askForDiscard(damage.from, objectName(), x, x, false, true);
                }
            }
        }
        return false;
    }
};

class yizhi : public TriggerSkill {
public:
    yizhi() : TriggerSkill("yizhi") {
        events << Dying;
        frequency = Wake;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getMark("yizhi") > 0)
            return false;
        ServerPlayer *victim = room->getCurrentDyingPlayer();
        if (victim != player)
            return false;
        room->doLightbox("$yizhiAnimate", 4000);
        room->touhouLogmessage("#YizhiWake", player, objectName());
        room->notifySkillInvoked(player, objectName());
        int x = 1 - player->getHp();
        RecoverStruct recov;
        recov.recover = x;
        recov.who = player;
        room->recover(player, recov);



        x = player->getMaxHp();
        if (room->changeMaxHpForAwakenSkill(player, 3 - x)) {
            shifang::koishi_removeskill(room, player);
            room->addPlayerMark(player, objectName());
            room->handleAcquireDetachSkills(player, "chaowo");
        }

        return false;
    }
};


chaowoCard::chaowoCard() {
    mute = true;
    will_throw = true;
}
bool chaowoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return(targets.isEmpty() && to_select->getMaxHp() >= Self->getMaxHp());
}
void chaowoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{

    targets.first()->drawCards(2);
    if (targets.first()->getMaxHp() == 3)
        source->drawCards(2);
}

class chaowovs : public OneCardViewAsSkill {
public:
    chaowovs() : OneCardViewAsSkill("chaowo") {
        filter_pattern = ".|.|.|hand!";
        response_pattern = "@@chaowo";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        if (originalCard != NULL){
            chaowoCard *card = new chaowoCard;
            card->addSubcard(originalCard);
            return card;
        }
        else
            return NULL;
    }
};

class chaowo : public TriggerSkill {
public:
    chaowo() : TriggerSkill("chaowo") {
        events << EventPhaseStart;
        view_as_skill = new chaowovs;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (player->getPhase() == Player::Finish){
            room->askForUseCard(player, "@@chaowo", "@chaowo");
        }
        return false;
    }
};


class shenhua : public TriggerSkill {
public:
    shenhua() : TriggerSkill("shenhua") {
        events << GameStart << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == GameStart) {
            room->notifySkillInvoked(player, objectName());
			room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 2);
            room->touhouLogmessage("#TriggerSkill", player, objectName());
            room->touhouLogmessage("#GainMaxHp", player, QString::number(2));
            room->touhouLogmessage("#GetHp", player, QString::number(player->getHp()), QList<ServerPlayer *>(), QString::number(player->getMaxHp()));

        }
		else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish){
			room->notifySkillInvoked(player, objectName());
			room->touhouLogmessage("#TriggerSkill", player, objectName());
			room->loseMaxHp(player, 1);
		}
		/*if (player->getPhase() == Player::Start){
			foreach (ServerPlayer *p, room->getOtherPlayers(player)){
				if (p->getHp() > player->getHp() && player->isWounded()){
					room->notifySkillInvoked(player, objectName());
					RecoverStruct recover;
					recover.recover = 1;
					room->recover(player, recover);
					break;
				}
			}
		
		}*/
        return false;
    }
};

class zuosui : public TriggerSkill {
public:
    zuosui() : TriggerSkill("zuosui") {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        
        DamageStruct damage = data.value<DamageStruct>();
		//if (damage.chain || damage.transfer || !damage.by_user)
        //    return false;
        if (!damage.from  || !damage.to || damage.from == damage.to)
            return false;
		if (damage.card ){ //&& damage.card->isKindOf("Slash")
			if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(damage.to))){
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());
                
				room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);
				room->touhouLogmessage("#GainMaxHp", player, QString::number(1));
				room->touhouLogmessage("#GetHp", player, QString::number(player->getHp()), QList<ServerPlayer *>(), QString::number(player->getMaxHp()));

			    
			   
				QString choice = room->askForChoice(damage.to, objectName(), "1+2+3+4");
				int x;
				if (choice =="1")
					x=1;
				else if (choice =="2")
					x=2;
				else if (choice =="3")
					x=3;
				else
					x=4;
				room->touhouLogmessage("#zuosuichoice", damage.to, objectName(), QList<ServerPlayer *>(), QString::number(x));
				choice = room->askForChoice(player, objectName(), "losehp+discard");
				if (choice == "losehp"){
					damage.to->drawCards(x);

					room->loseHp(damage.to, x);
				}
				else{
					int discardNum = damage.to->getCards("he").length()> x ? damage.to->getCards("he").length() - x: 0;
					if (discardNum >0)
						room->askForDiscard(damage.to, objectName(), discardNum, discardNum, false, true);
				}
				return true;
			}
		}

        return false;
    }
};

class worao : public TriggerSkill {
public:
    worao() : TriggerSkill("worao") {
        events << TargetConfirming;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirming){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from  && player != use.from  &&  use.to.contains(player)
				//&& use.to.length() ==1 
                && (use.card->isKindOf("Slash") || use.card->isNDTrick())){
				QString prompt = "invoke:" + use.from->objectName() + ":" + use.card->objectName();
                if (room->askForSkillInvoke(player, objectName(), prompt)){
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.from->objectName());
			
					room->setPlayerProperty(player, "maxhp", player->getMaxHp() + 1);
					room->touhouLogmessage("#GainMaxHp", player, QString::number(1));
					room->touhouLogmessage("#GetHp", player, QString::number(player->getHp()), QList<ServerPlayer *>(), QString::number(player->getMaxHp()));
					player->drawCards(2);
					if (!player->isKongcheng()){
						const Card *card = room->askForExchange(player, objectName(), 1, false, "woraoGive:" + use.from->objectName());
						use.from->obtainCard(card, false);
					}
				}
            }
        }
        return false;
    }
};

class yindu : public TriggerSkill {
public:
    yindu() : TriggerSkill("yindu") {
        events << Death;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *source = room->findPlayerBySkillName(objectName());
		DeathStruct death = data.value<DeathStruct>();
        
		if (source && death.who && death.who == player && death.who != source){
			QString prompt = "invoke:" + death.who->objectName();
			if (room->askForSkillInvoke(source, objectName(), prompt)){
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), death.who->objectName());
                //QList<ServerPlayer *> logto;
                //logto << source;
                //room->touhouLogmessage("#InvokeOthersSkill", death.who, objectName(), logto);
                //room->notifySkillInvoked(source, objectName());
				
				source->drawCards(3);
				room->setPlayerFlag(death.who,"skipRewardAndPunish");
			} 
		}
        return false;
    }
};


class huanming : public TriggerSkill {
public:
    huanming() : TriggerSkill("huanming") {
        events << DamageCaused;
        frequency = Limited;
        limit_mark = "@huanming";
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.to || player == damage.to 
        || player->getMark("@huanming") == 0
        || damage.to->getHp()<=0)
            return false;

        if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(damage.to))) {
            room->removePlayerMark(player, "@huanming");
            room->doLightbox("$huanmingAnimate", 4000);
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());
               
			int source_newHp = qMin(damage.to->getHp(), player->getMaxHp());
            int victim_newHp = qMin(player->getHp(), damage.to->getMaxHp());
            room->setPlayerProperty(player, "hp", source_newHp);
            if (damage.to->hasSkill("banling")){
                room->setPlayerMark(damage.to, "lingtili", victim_newHp);
                room->setPlayerMark(damage.to, "rentili", victim_newHp);
                //room->setPlayerMark(player, "minus_lingtili", minus_x);
                //room->setPlayerMark(player, "minus_rentili", minus_y); 
                room->setPlayerProperty(damage.to, "hp", victim_newHp);
            }
            else
                room->setPlayerProperty(damage.to, "hp", victim_newHp);
				
            room->touhouLogmessage("#GetHp", player, QString::number(player->getHp()), QList<ServerPlayer *>(), QString::number(player->getMaxHp()));
			room->touhouLogmessage("#GetHp", damage.to, QString::number(damage.to->getHp()), QList<ServerPlayer *>(), QString::number(damage.to->getMaxHp()));		
            return true;
        }
        return false;
    }
};


touhougodPackage::touhougodPackage()
    : Package("touhougod")
{
    General *shen001 = new General(this, "shen001", "touhougod", 4, false);
    shen001->addSkill(new jiexian);

    General *shen002 = new General(this, "shen002", "touhougod", 3, false);
    shen002->addSkill(new zhouye);
    shen002->addSkill(new zhouye_change);
    shen002->addSkill(new hongwu);
    shen002->addSkill(new shenqiang);
    shen002->addSkill(new yewang);
    related_skills.insertMulti("zhouye", "#zhouye_change");

    General *shen003 = new General(this, "shen003", "touhougod", 4, false);
    shen003->addSkill(new aoyi);
    shen003->addSkill(new aoyi_handle);
    shen003->addSkill(new aoyi_mod);
    related_skills.insertMulti("aoyi", "#aoyi_handle");
    related_skills.insertMulti("aoyi", "#aoyi_mod");

    General *shen004 = new General(this, "shen004", "touhougod", 4, false);
    shen004->addSkill(new shikong);
    shen004->addSkill(new ronghui);
    shen004->addSkill(new jubian);
    shen004->addSkill(new hengxing);

    General *shen005 = new General(this, "shen005", "touhougod", 0, false);
    shen005->addSkill(new huanmeng);
    shen005->addSkill(new cuixiang);
    shen005->addSkill(new xuying);


    General *shen006 = new General(this, "shen006", "touhougod", 3, false);
    shen006->addSkill(new kuangyan);
    shen006->addSkill(new huimie);
    shen006->addSkill(new jinguo);

    General *shen007 = new General(this, "shen007", "touhougod", 3, false);
    shen007->addSkill(new shicao);
    shen007->addSkill(new shiting);
    shen007->addSkill(new huanzai);
    shen007->addSkill(new shanghun);


    General *shen008 = new General(this, "shen008", "touhougod", 3, false);
    shen008->addSkill(new banling);
    shen008->addSkill(new rengui);
    shen008->addSkill(new FakeMoveSkill("rengui"));
    related_skills.insertMulti("rengui", "#rengui-fake-move");

    General *shen009 = new General(this, "shen009", "touhougod", 4, false);
    shen009->addSkill(new ningshi);
    shen009->addSkill(new gaoao);

    General *shen010 = new General(this, "shen010", "touhougod", 4, false);
    shen010->addSkill(new shenshou);

    General *shen011 = new General(this, "shen011", "touhougod", 4, false);

    General *shen012 = new General(this, "shen012", "touhougod", 4, false);
    shen012->addSkill(new quanjie);
    shen012->addSkill(new duanzuicount);
    shen012->addSkill(new duanzui);
    shen012->addSkill(new duanzuishenpan);
    related_skills.insertMulti("duanzui", "#duanzui-count");
    related_skills.insertMulti("duanzui", "#duanzui-shenpan");


    General *shen013 = new General(this, "shen013", "touhougod", 3, false);
    shen013->addSkill(new hualong);
    shen013->addSkill(new luanwu);
    shen013->addSkill(new longwei);


    General *shen014 = new General(this, "shen014", "touhougod", 4, false);
    shen014->addSkill(new qiannian);
    shen014->addSkill(new qiannian_max);
    shen014->addSkill(new qiannian_draw);
    related_skills.insertMulti("qiannian", "#qiannian_max");
    related_skills.insertMulti("qiannian", "#qiannian_draw");

    General *shen015 = new General(this, "shen015", "touhougod", 4, false);
    shen015->addSkill(new qinlue);
    shen015->addSkill(new qinlue_effect);
    related_skills.insertMulti("qinlue", "#qinlue_effect");

    General *shen016 = new General(this, "shen016", "touhougod", 4, false);
    shen016->addSkill(new chaoren);

    General *shen017 = new General(this, "shen017", "touhougod", 3, false);
    shen017->addSkill(new biaoxiang);
    shen017->addSkill(new shifang);
    shen017->addSkill(new yizhi);
    shen017->addRelateSkill("ziwo");
    shen017->addRelateSkill("benwo");
    shen017->addRelateSkill("chaowo");

	General *shen018 = new General(this, "shen018", "touhougod", 3, false);
	shen018->addSkill(new shenhua);
	shen018->addSkill(new zuosui);
	shen018->addSkill(new worao);
	
	General *shen020 = new General(this, "shen020", "touhougod", 4, false);
	shen020->addSkill(new Skill("wunan", Skill::Compulsory));

    General *shen021 = new General(this, "shen021", "touhougod", 4, false);
    shen021->addSkill(new yindu);
    shen021->addSkill(new huanming);
    
    General *shen000 = new General(this, "shen000", "touhougod", 4, true);
    shen000->addSkill(new chuanghuan);
    shen000->addSkill(new chuanghuanGet);
    related_skills.insertMulti("chuanghuan", "#chuanghuan-get");

    addMetaObject<hongwuCard>();
    addMetaObject<shenqiangCard>();
    addMetaObject<huimieCard>();
    addMetaObject<shenshouCard>();
    addMetaObject<chaorenpreventrecast>();
    addMetaObject<ziwoCard>();
    addMetaObject<chaowoCard>();


    skills << new ziwo << new benwo << new chaowo;
}

ADD_PACKAGE(touhougod)

