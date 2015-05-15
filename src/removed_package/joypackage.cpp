#include "joypackage.h"
#include "engine.h"
#include "maneuvering.h"

Shit::Shit(Suit suit, int number):BasicCard(suit, number){
    setObjectName("shit");

    target_fixed = true;
    will_throw = true;
    handling_method = Card::MethodDiscard;
}

QString Shit::getSubtype() const{
    return "disgusting_card";
}

void Shit::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *player = card_use.from;

    QList<int> used_cards;
    QList<CardsMoveStruct> moves;
    if (card_use.card->isVirtualCard())
        used_cards.append(card_use.card->getSubcards());
    else
        used_cards << card_use.card->getEffectiveId();

    QVariant data = QVariant::fromValue(card_use);
    RoomThread *thread = room->getThread();
    Q_ASSERT(thread != NULL);
    thread->trigger(PreCardUsed, room, player, data);

    CardMoveReason reason(CardMoveReason::S_REASON_THROW, player->objectName(), QString(), card_use.card->getSkillName(), QString());
    room->moveCardTo(this, player, NULL, Player::DiscardPile, reason, true);

    thread->trigger(CardUsed, room, player, data);
    thread->trigger(CardFinished, room, player, data);
}

class shitmove: public TriggerSkill{
public:
    shitmove(): TriggerSkill("shitmove"){
        global = true;
        events << CardsMoveOneTime;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive() && target->getPhase() != Player::NotActive;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == player && move.to_place == Player::DiscardPile){
            int n = move.card_ids.length();
            QList<const Card *> shits;
            for (int i = 0; i < n; i ++){
                if (Sanguosha->getCard(move.card_ids[i])->isKindOf("Shit") && move.from_places[i] == Player::PlaceHand){
                    shits << Sanguosha->getCard(move.card_ids[i]);
                }
            }

            LogMessage l;
            l.from = player;

            foreach (const Card *shit, shits){
                l.card_str = QString::number(shit->getEffectiveId());

                if (shit->getSuit() == Card::Spade){
                    l.type = "$ShitLostHp";
                    room->sendLog(l);
                    room->loseHp(player);
                    continue;
                }

                DamageStruct shitdamage(shit, player, player);
                switch (shit->getSuit()){
                    case Card::Heart:
                        shitdamage.nature = DamageStruct::Fire;
                        break;
                    case Card::Diamond:
                        shitdamage.nature = DamageStruct::Normal;
                        break;
                    case Card::Club:
                        shitdamage.nature = DamageStruct::Thunder;
                        break;
                }

                l.type = "$ShitDamage";
                room->sendLog(l);

                room->damage(shitdamage);
            }
        }
        return false;
    }
};

bool Shit::HasShit(const Card *card){
    if(card->isVirtualCard()){
        QList<int> card_ids = card->getSubcards();
        foreach(int card_id, card_ids){
            const Card *c = Sanguosha->getCard(card_id);
            if(c->objectName() == "shit")
                return true;
        }

        return false;
    }else
        return card->objectName() == "shit";
}

// -----------  Deluge -----------------

Deluge::Deluge(Card::Suit suit, int number)
    :Disaster(suit, number)
{
    setObjectName("deluge");

    judge.pattern = ".|.|1,13";
    judge.good = false;
    judge.reason = objectName();
}

void Deluge::takeEffect(ServerPlayer *target) const{
    QList<const Card *> cards = target->getCards("he");

    Room *room = target->getRoom();
    int n = qMin(cards.length(), target->aliveCount());
    if(n == 0)
        return;

    qShuffle(cards);
    cards = cards.mid(0, n);

    QList<int> card_ids;
    foreach(const Card *card, cards){
        card_ids << card->getEffectiveId();
        room->throwCard(card, NULL);
    }

    room->fillAG(card_ids);

    QList<ServerPlayer *> players = room->getOtherPlayers(target);
    players << target;
    players = players.mid(0, n);
    foreach(ServerPlayer *player, players){
        if(player->isAlive()){
            int card_id = room->askForAG(player, card_ids, false, "deluge");
            card_ids.removeOne(card_id);

            room->takeAG(player, card_id);
        }
    }

    foreach(int card_id, card_ids)
        room->takeAG(NULL, card_id);

    room->clearAG();
}

// -----------  Typhoon -----------------

Typhoon::Typhoon(Card::Suit suit, int number)
    :Disaster(suit, number)
{
    setObjectName("typhoon");

    judge.pattern = ".|diamond|2~9";
    judge.good = false;
    judge.reason = objectName();
}

void Typhoon::takeEffect(ServerPlayer *target) const{
    Room *room = target->getRoom();
    Sanguosha->playSystemAudioEffect("typhoon");
    QList<ServerPlayer *> players = room->getOtherPlayers(target);
    foreach(ServerPlayer *player, players){
        if(target->distanceTo(player) == 1){
            int discard_num = qMin(6, player->getHandcardNum());
            if (discard_num != 0){
                room->askForDiscard(player, objectName(), discard_num, discard_num);
            }

            room->getThread()->delay();
        }
    }
}

// -----------  Earthquake -----------------

Earthquake::Earthquake(Card::Suit suit, int number)
    :Disaster(suit, number)
{
    setObjectName("earthquake");

    judge.pattern = ".|club|2~9";
    judge.good = false;
    judge.reason = objectName();
}

void Earthquake::takeEffect(ServerPlayer *target) const{
    Room *room = target->getRoom();
    Sanguosha->playSystemAudioEffect("earthquake");
    QList<ServerPlayer *> players = room->getAllPlayers();
    foreach(ServerPlayer *player, players){
        if(target->distanceTo(player) <= 1){
            if (!player->getEquips().isEmpty()){
                player->throwAllEquips();
            }

            room->getThread()->delay();
        }
    }
}

// -----------  Volcano -----------------

Volcano::Volcano(Card::Suit suit, int number)
    :Disaster(suit, number)
{
    setObjectName("volcano");

    judge.pattern = ".|heart|2~9";
    judge.good = false;
    judge.reason = objectName();
}

void Volcano::takeEffect(ServerPlayer *target) const{
    Room *room = target->getRoom();
    Sanguosha->playSystemAudioEffect("volcano");
    QList<ServerPlayer *> players = room->getAllPlayers();

    foreach(ServerPlayer *player, players){
        int point = 3 - target->distanceTo(player);
        if(point >= 1){
            DamageStruct damage;
            damage.card = this;
            damage.damage = point;
            damage.to = player;
            damage.nature = DamageStruct::Fire;
            room->damage(damage);
        }
    }
}

// -----------  MudSlide -----------------
MudSlide::MudSlide(Card::Suit suit, int number)
    :Disaster(suit, number)
{
    setObjectName("mudslide");

    judge.pattern = ".|black|1,13,4,7";
    judge.good = false;
    judge.reason = objectName();
}

void MudSlide::takeEffect(ServerPlayer *target) const{
    Room *room = target->getRoom();
    Sanguosha->playSystemAudioEffect("mudslide");
    QList<ServerPlayer *> players = room->getAllPlayers();
    int to_destroy = 4;
    foreach(ServerPlayer *player, players){


        QList<const Card *> equips = player->getEquips();
        if(equips.isEmpty()){
            DamageStruct damage;
            damage.card = this;
            damage.to = player;
            room->damage(damage);
        }else{
            int n = qMin(equips.length(), to_destroy);
            for(int i = 0; i < n; i++){
                CardMoveReason reason(CardMoveReason::S_REASON_DISCARD, QString(), QString(), "mudslide");
                room->throwCard(equips.at(i), reason, player);
            }

            to_destroy -= n;
            if(to_destroy == 0)
                break;
        }
    }
}

class GrabPeach: public TriggerSkill{
public:
    GrabPeach():TriggerSkill("grab_peach"){
        events << CardUsed;
        global = true;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if(use.card->isKindOf("Peach")){
            QList<ServerPlayer *> players = room->getOtherPlayers(player);

            foreach(ServerPlayer *p, players){
                if(p->getOffensiveHorse() != NULL && p->getOffensiveHorse()->isKindOf("Monkey") && p->getMark("Equips_Nullified_to_Yourself") == 0 &&
                   p->askForSkillInvoke("grab_peach", data))
                {
                    room->throwCard(p->getOffensiveHorse(), p, p);
                    p->obtainCard(use.card);

                    use.to.clear();
                    data = QVariant::fromValue(use);
                }
            }
        }

        return false;
    }
};

Monkey::Monkey(Card::Suit suit, int number)
    :OffensiveHorse(suit, number)
{
    setObjectName("Monkey");
}


QString Monkey::getCommonEffectName() const{
    return "Monkey";
}

class GaleShellSkill: public ArmorSkill{
public:
    GaleShellSkill():ArmorSkill("GaleShell"){
        events << DamageInflicted;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.nature == DamageStruct::Fire){
            LogMessage log;
            log.type = "#GaleShellDamage";
            log.from = player;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

GaleShell::GaleShell(Suit suit, int number) :Armor(suit, number){
    setObjectName("GaleShell");

    target_fixed = false;
}

bool GaleShell::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && Self->distanceTo(to_select) <= 1;
}

DisasterPackage::DisasterPackage()
    :Package("Disaster")
{
    QList<Card *> cards;

    cards << new Deluge(Card::Spade, 1)
            << new Typhoon(Card::Spade, 4)
            << new Earthquake(Card::Club, 10)
            << new Volcano(Card::Heart, 13)
            << new MudSlide(Card::Heart, 7);

    foreach(Card *card, cards)
        card->setParent(this);

    type = CardPack;
}

JoyPackage::JoyPackage()
    :Package("Joy")
{
    QList<Card *> cards;

    cards << new IceSlash(Card::Spade, 4)
            << new Shit(Card::Club, 1)
            << new Shit(Card::Heart, 8)
            << new Shit(Card::Diamond, 13)
            << new Shit(Card::Spade, 10);

    foreach(Card *card, cards)
        card->setParent(this);

    type = CardPack;
    skills << new shitmove;
}

class YxSwordSkill: public WeaponSkill{
public:
    YxSwordSkill():WeaponSkill("YxSword"){
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.card && damage.card->isKindOf("Slash")){
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            QMutableListIterator<ServerPlayer *> itor(players);

            while(itor.hasNext()){
                itor.next();
                if(!player->inMyAttackRange(itor.value()))
                    itor.remove();
            }

            if(players.isEmpty())
                return false;

            QVariant _data = QVariant::fromValue(damage);
            room->setTag("YxSwordData", _data);
            ServerPlayer *target = room->askForPlayerChosen(player, players, objectName(), "@yxsword-select", true, true);
            room->removeTag("YxSwordData");
            if (target != NULL){
                damage.from = target;
                data = QVariant::fromValue(damage);
                room->moveCardTo(player->getWeapon(), player, target, Player::PlaceHand,
                    CardMoveReason(CardMoveReason::S_REASON_TRANSFER, player->objectName(), objectName(), QString()));
            }
        }
        return damage.to->isDead();
    }
};

YxSword::YxSword(Suit suit, int number)
    :Weapon(suit, number, 3)
{
    setObjectName("YxSword");
}

JoyEquipPackage::JoyEquipPackage()
    :Package("JoyEquip")
{
    (new Monkey(Card::Diamond, 5))->setParent(this);
    (new GaleShell(Card::Heart, 1))->setParent(this);
    (new YxSword(Card::Club, 9))->setParent(this);

    type = CardPack;
    skills << new GaleShellSkill << new YxSwordSkill << new GrabPeach;
}

class Xianiao: public TriggerSkill{
public:
    Xianiao(): TriggerSkill("xianiao"){
        events << Damage;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const{
        ServerPlayer *xiahoujie = room->findPlayerBySkillName(objectName());
        if (xiahoujie == NULL || xiahoujie->isDead() || !player->inMyAttackRange(xiahoujie))
            return false;

        room->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(xiahoujie, objectName());

        xiahoujie->throwAllHandCards();
        xiahoujie->drawCards(player->getHp());

        return false;
    }
};

class Tangqiang: public TriggerSkill{
public:
    Tangqiang(): TriggerSkill("tangqiang"){
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (player == death.who && death.damage && death.damage->from){
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            room->loseMaxHp(death.damage->from, 1);
            room->acquireSkill(death.damage->from, objectName());
        }
        return false;
    }
};

class Jieao: public PhaseChangeSkill{
public:
    Jieao(): PhaseChangeSkill("jieao"){
        frequency = Compulsory;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() == Player::Start){
            Room *room = target->getRoom();
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(target, objectName());

            target->drawCards(2);
        }

        return false;
    }
};

YuluCard::YuluCard() {
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void YuluCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const{
    source->addToPile("yulu", subcards);
}

class Yulu: public ViewAsSkill{
public:
    Yulu(): ViewAsSkill("yulu"){

    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (selected.length() >= 5)
            return false;

        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (cards.length() >= 2 && cards.length() <= 5){
            YuluCard *yulu = new YuluCard;
            yulu->addSubcards(cards);
            return yulu;
        }
        return NULL;
    }
};

NumaNRNMCard::NumaNRNMCard(){
    m_skillName = "numa_nrnm";
    mute = true;
}

bool NumaNRNMCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (targets.length() == 0)
        return true;
    else if (targets.length() == 1)
        return (to_select != targets[0] && !to_select->isKongcheng());

    return false;
}

bool NumaNRNMCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

void NumaNRNMCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *miheng = card_use.from;

    LogMessage log;
    log.from = miheng;
    log.to << card_use.to;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    QVariant data = QVariant::fromValue(card_use);
    RoomThread *thread = room->getThread();

    thread->trigger(PreCardUsed, room, miheng, data);
    thread->trigger(CardUsed, room, miheng, data);
    thread->trigger(CardFinished, room, miheng, data);
}

void NumaNRNMCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->showAllCards(targets[1], targets[0]);
}

class NumaNRNM: public ZeroCardViewAsSkill{
public:
    NumaNRNM(): ZeroCardViewAsSkill("numa"){
        response_pattern = "@@numa-card1!";
    }

    virtual const Card *viewAs() const{
        return new NumaNRNMCard;
    }
};

class Numa: public PhaseChangeSkill{
public:
    Numa(): PhaseChangeSkill("numa"){
        view_as_skill = new NumaNRNM;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Finish && !target->getPile("yulu").isEmpty();
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        static QMap<Card::Suit, QChar> suitmap;
        if (suitmap.isEmpty()){
            suitmap[Card::Heart] = 'w';
            suitmap[Card::Spade] = 'r';
            suitmap[Card::Diamond] = 'n';
            suitmap[Card::Club] = 'm';
        }

        static QStringList knownyulu;
        if (knownyulu.isEmpty()){
            knownyulu << "wm" << "nm" << "mm" << "rn" << "wr" << "wrm" << "wrn" << "nrw" << "rnm" << "rwm" << "www" << "rrr" << "nnn" << "mmm"
                      << "nmnm" << "wrnm" << "mmrw" << "nrnm" << "nrwm" << "nrwmm" << "wrnmm" << "nmrwm" << "rrnmm" << "rrrmm" << "wwwww";
        }

        QString to_speak;

        QList<int> yulu = player->getPile("yulu");
        foreach (int id, yulu){
            to_speak = to_speak + suitmap[Sanguosha->getCard(id)->getSuit()];
        }

        if (!knownyulu.contains(to_speak))
            to_speak = "unknown" + QString::number(qMin(yulu.length(), 6));



        if (player->askForSkillInvoke(objectName(), "speak:::yulu_" + to_speak)){
            Room *room = player->getRoom();
            LogMessage log;
            log.type = "#numaspeak";
            if (!to_speak.startsWith("unknown"))
                log.arg = "yulu_" + to_speak;
            else
                log.arg = "yulu_unknown";
            log.from = player;
            room->sendLog(log);

            DummyCard dummy(yulu);
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
            room->moveCardTo(&dummy, NULL, Player::DiscardPile, reason, true);

            //lightbox

            if (to_speak == "wm"){
                RecoverStruct recover;
                recover.who = player;
                room->recover(player, recover);
            }
            else if (to_speak == "nm"){
                QList<ServerPlayer *> pls;
                foreach (ServerPlayer *p, room->getAlivePlayers()){
                    if (p->canDiscard(p, "h"))
                        pls << p;
                }

                if (pls.isEmpty())
                    return false;

                ServerPlayer *victim = room->askForPlayerChosen(player, pls, objectName() + "_nm", "@numa_nm", false, true);
                room->askForDiscard(victim, objectName() + "_nm", 2, 2, false, false);
            }
            else if (to_speak == "mm"){
                QList<ServerPlayer *> pls;
                foreach (ServerPlayer *p, room->getAlivePlayers()){
                    if (player->canDiscard(p, "j"))
                        pls << p;
                }

                if (pls.isEmpty())
                    return false;

                ServerPlayer *victim = room->askForPlayerChosen(player, pls, objectName() + "_mm", "@numa_mm", false, true);
                DummyCard dummy2(victim->getJudgingAreaID());
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), victim->objectName(), objectName(), QString());
                room->moveCardTo(&dummy2, NULL, Player::DiscardPile, reason, true);
            }
            else if (to_speak == "rn"){
                QList<ServerPlayer *> pls;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)){
                    if (!p->isKongcheng())
                        pls << p;
                }

                if (pls.isEmpty())
                    return false;

                ServerPlayer *victim = room->askForPlayerChosen(player, pls, objectName() + "_rn", "@numa_rn", false, true);
                const Card *card = room->askForExchange(victim, objectName() + "_rn", 1);
                player->obtainCard(card, false);
                RecoverStruct recover;
                recover.who = player;
                room->recover(victim, recover);
            }
            else if (to_speak == "wr"){
                JudgeStruct judge;
                judge.who = player;
                judge.pattern = "Peach,GodSalvation";
                judge.good = true;
                judge.reason = objectName() + "_wr";

                room->judge(judge);

                if (judge.isGood()){
                    room->handleAcquireDetachSkills(player, "fanchun");
                }
            }
            else if (to_speak == "wrm" || to_speak == "wrn"){
                QList<ServerPlayer *> pls;
                bool selectMale = (to_speak == "wrn");
                bool selectFemale = (to_speak == "wrm");
                foreach (ServerPlayer *p, room->getOtherPlayers(player)){
                    if (p->isMale() == selectMale && p->isFemale() == selectFemale)
                        pls << p;
                }

                if (pls.isEmpty())
                    return false;

                ServerPlayer *target = room->askForPlayerChosen(player, pls, objectName() + "_" + to_speak, "@numa_" + to_speak, false, true);
                RecoverStruct recover;
                recover.who = player;
                room->recover(player, recover);
                room->recover(target, recover);
            }
            else if (to_speak == "nrw"){
                QList<ServerPlayer *> pls;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)){
                    if (p->canSlash(player))
                        pls << p;
                }

                if (pls.isEmpty())
                    return false;

                ServerPlayer *victim = room->askForPlayerChosen(player, pls, objectName() + "_nrw", "@numa_nrw", false, true);
                bool slashed = room->askForUseSlashTo(victim, player, "@numa_nrw_slash");
                if (!slashed){
                    DummyCard dummy3;
                    dummy3.addSubcards(victim->getCards("he"));
                    room->moveCardTo(&dummy3, player, Player::PlaceHand, false);
                }
            }
            else if (to_speak == "rnm" || to_speak == "wrnm"){
                if (to_speak == "rnm")
                    room->setPlayerMark(player, "drank", 0);
                else
                    room->setPlayerMark(player, "drank", 1);

                int ri = (to_speak == "rnm") ? yulu[0] : yulu[1];
                int riPoint = Sanguosha->getCard(ri)->getNumber();

                Slash *slash;
                if (riPoint < 5)
                    slash = new ThunderSlash(Card::NoSuit, 0);
                else if (riPoint > 9)
                    slash = new FireSlash(Card::NoSuit, 0);
                else
                    slash = new Slash(Card::NoSuit, 0);

                QList<ServerPlayer *> pls;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)){
                    if (player->canSlash(p, false))
                        pls << p;
                }

                if (pls.isEmpty()){
                    delete slash;
                    return false;
                }

                slash->setSkillName("_numa_" + to_speak);
                ServerPlayer *victim = room->askForPlayerChosen(player, pls, objectName() + "_" + to_speak, "@numa_" + to_speak + ":" + slash->objectName(), false, true);
                CardUseStruct use(slash, player, victim, true);
                room->useCard(use);
            }
            else if (to_speak == "rwm"){
                ServerPlayer *victim = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName() + "_rwm", "@numa_rwm", false, true);
                room->damage(DamageStruct(objectName() + "_rwm", victim, player));
                RecoverStruct recover;
                recover.who = player;
                room->recover(victim, recover);
            }
            else if (to_speak == "www"){
                player->turnOver();
                player->drawCards(3);
            }
            else if (to_speak == "rrr"){
                ServerPlayer *victim = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName() + "_rrr", "@numa_rrr", false, true);
                victim->turnOver();
                victim->drawCards(player->getLostHp());
            }
            else if (to_speak == "nnn"){
                ServerPlayer *victim = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName() + "_nnn", "@numa_nnn", false, true);
                victim->obtainCard(&dummy);
            }
            else if (to_speak == "mmm"){
                QList<ServerPlayer *> pls;
                foreach (ServerPlayer *p, room->getAlivePlayers()){
                    if (!p->getEquips().isEmpty())
                        pls << p;
                }

                if (pls.isEmpty())
                    return false;

                ServerPlayer *victim = room->askForPlayerChosen(player, pls, objectName() + "_mmm", "@numa_mmm", false, true);
                DummyCard dummy4;
                dummy4.addSubcards(victim->getEquips());
                room->throwCard(&dummy4, victim, player);
            }
            else if (to_speak == "nmnm"){
                ServerPlayer *victim = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName() + "_nmnm", "@numa_nmnm", false, true);
                victim->gainAnExtraTurn();
            }
            else if (to_speak == "mmrw"){
                RecoverStruct recover;
                recover.recover = player->getLostHp();
                recover.who = player;
                room->recover(player, recover);
            }
            else if (to_speak == "nrnm"){
                room->askForUseCard(player, "@@numa-card1!", "@numa_nrnm", 1, Card::MethodNone);
            }
            else if (to_speak == "nrwm"){
                DamageStruct damage;
                damage.from = player;
                room->killPlayer(player, &damage);
            }
            else if (to_speak == "nrwmm"){
                ServerPlayer *victim = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName() + "_nrwmm", "@numa_nrwmm", false, true);
                room->setPlayerFlag(victim, "numa_nrwmm_InTempMoving");
                QList<int> discard_ids;
                QList<Player::Place> discard_places;
                for (int i = 1; i <= 4; i++)
                    if (player->canDiscard(victim, "he")){
                        int id = room->askForCardChosen(player, victim, "he", objectName() + "_nrwmm", player == victim, Card::MethodDiscard);
                        discard_ids << id;
                        discard_places << room->getCardPlace(id);
                        victim->addToPile("#numa_nrwmm", id);
                    }
                    else
                        break;

                for (int i = 0; i < discard_ids.length(); i++)
                    room->moveCardTo(Sanguosha->getCard(discard_ids[i]), victim, discard_places[i], false);
                room->setPlayerFlag(victim, "-numa_nrwmm_InTempMoving");
                DummyCard dummy5(discard_ids);
                room->throwCard(&dummy5, victim, player);

                room->damage(DamageStruct(objectName() + "_nrwmm", player, player, 2));
            }
            else if (to_speak == "wrnmm"){
                if (player->getMark("@numawrnmm") == 0){
                    player->gainMark("@numawrnmm");
                    ServerPlayer *victim = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName() + "_wrnmm", "@numa_wrnmm", false, true);

                    DamageStruct damage(objectName() + "_wrnmm", player, victim);

                    damage.nature = DamageStruct::Thunder;
                    if (victim->isAlive())
                        room->damage(damage);
                    damage.nature = DamageStruct::Fire;
                    if (victim->isAlive())
                        room->damage(damage);
                    damage.nature = DamageStruct::Normal;
                    if (victim->isAlive())
                        room->damage(damage);

                    room->loseHp(player, 2);
                }
            }
            else if (to_speak == "nmrwm"){
                if (player->getMark("@numarrnmm") == 0){
                    player->gainMark("@numanmrwm");
                    room->loseHp(player);

                    foreach (ServerPlayer *p, room->getOtherPlayers(player)){
                        if (!p->isKongcheng()){
                            int id = room->askForCardChosen(player, p, "h", objectName() + "_nmrwm");
                            player->obtainCard(Sanguosha->getCard(id), false);
                        }
                    }

                    player->turnOver();
                }
            }
            else if (to_speak == "rrnmm"){
                if (player->getMark("@numarrnmm") == 0){
                    player->gainMark("@numarrnmm");
                    QMap<ServerPlayer *, int> light;
                    foreach (ServerPlayer *p, room->getAlivePlayers()){
                        QList<const Card *> judgingarea = p->getJudgingArea();
                        foreach (const Card *c, judgingarea){
                            if (c->isKindOf("Lightning")){
                                light[p] = c->getEffectiveId();
                                break;
                            }
                        }
                    }

                    if (light.isEmpty())
                        return false;

                    ServerPlayer *victim = room->askForPlayerChosen(player, light.keys(), objectName() + "_rrnmm", "@numa_rrnmm", false, true);

                    room->throwCard(light[victim], NULL);

                    room->damage(DamageStruct(Sanguosha->getCard(light[victim]), NULL, victim, 3, DamageStruct::Thunder));
                }
            }
            else if (to_speak == "rrrmm"){
                if (player->getMark("@numarrrmm") == 0){
                    player->gainMark("@numarrrmm");
                    QList<ServerPlayer *> pls;
                    foreach (ServerPlayer *p, room->getOtherPlayers(player)){
                        if (p->getMaxHp() > player->getMaxHp())
                            pls << p;
                    }

                    if (pls.isEmpty())
                        return false;

                    ServerPlayer *victim = room->askForPlayerChosen(player, pls, objectName() + "_rrrmm", "@numa_rrrmm", false, true);

                    room->setPlayerProperty(victim, "maxhp", victim->getMaxHp() + 1);

                    QString to_gain = room->askForChoice(player, objectName() + "_rrrmm", "benghuai+wumou", QVariant::fromValue(victim));
                    room->handleAcquireDetachSkills(victim, to_gain);
                }
            }
            else if (to_speak == "wwwww"){
                room->changeHero(player, "dengai", true, true, false);
                player->addToPile("field", &dummy, true);
            }
            else {
                if (dummy.subcardsLength() == 4 && player->getMark("@numa4wd") == 0){
                    player->gainMark("@numa4wd");

                    if (room->changeMaxHpForAwakenSkill(player, 2)){
                        QList<const Skill *> skillslist = player->getVisibleSkillList();
                        QStringList detachlist;
                        foreach (const Skill *skill, skillslist){
                            if (skill->getLocation() == Skill::Right && !skill->isAttachedLordSkill())
                                detachlist.append("-" + skill->objectName());
                        }
                        room->handleAcquireDetachSkills(player, detachlist);
                        if (player->isAlive())
                            player->gainMark("@duanchang");
                    }
                }
                else if (dummy.subcardsLength() == 5 && player->getMark("@numa5wd") == 0){
                    player->gainMark("@numa5wd");
                    if (room->changeMaxHpForAwakenSkill(player))
                        room->handleAcquireDetachSkills(player, "longhun");
                }
                else if (dummy.subcardsLength() >= 6 && player->getMark("@numa6wd") == 0){
                    player->gainMark("@numa6wd");
                    if (room->changeMaxHpForAwakenSkill(player, -2))
                        room->handleAcquireDetachSkills(player, "wuyan|buqu");
                }
            }
        }
        return false;
    }
};

class Fanchun: public MasochismSkill{
public:
    Fanchun(): MasochismSkill("fanchun"){
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        Room *room = target->getRoom();
        const Card *card = damage.card;
        if (card != NULL && room->getCardPlace(card->getEffectiveId()) == Player::PlaceTable){
            if (target->askForSkillInvoke(objectName(), QVariant::fromValue(card))){
                room->broadcastSkillInvoke(objectName());
                target->addToPile("yulu", card);
            }
        }
    }
};

DCPackage::DCPackage(): Package("DC"){
    General *xiahoujie = new General(this, "xiahoujie", "wei", 3);
    xiahoujie->addSkill(new Xianiao);
    xiahoujie->addSkill(new Tangqiang);

    General *miheng = new General(this, "miheng", "god", 3);
    miheng->addSkill(new Jieao);
    miheng->addSkill(new Yulu);
    miheng->addSkill(new Numa);
    miheng->addSkill(new FakeMoveSkill("numa_nrwmm"));
    related_skills.insertMulti("numa", "#numa_nrwmm-fake-move");
    miheng->addRelateSkill("fanchun");
    skills << new Fanchun;

    addMetaObject<YuluCard>();
    addMetaObject<NumaNRNMCard>();

}

ADD_PACKAGE(Joy)
ADD_PACKAGE(Disaster)
ADD_PACKAGE(JoyEquip)
ADD_PACKAGE(DC)