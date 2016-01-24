#include "roomthread.h"
#include "room.h"
#include "engine.h"
#include "gamerule.h"
#include "scenerule.h"
#include "scenario.h"
#include "ai.h"
#include "settings.h"
#include "standard.h"

#include <QTime>
#include <functional>

#ifdef QSAN_UI_LIBRARY_AVAILABLE
#pragma message WARN("UI elements detected in server side!!!")
#endif

using namespace JsonUtils;

LogMessage::LogMessage()
    : from(NULL)
{
}

QString LogMessage::toString() const
{
    QStringList tos;
    foreach(ServerPlayer *player, to)
        if (player != NULL) tos << player->objectName();

    return QString("%1:%2->%3:%4:%5:%6")
        .arg(type)
        .arg(from ? from->objectName() : "")
        .arg(tos.join("+"))
        .arg(card_str).arg(arg).arg(arg2);
}

QVariant LogMessage::toJsonValue() const
{
    QStringList tos;
    foreach(ServerPlayer *player, to)
        if (player != NULL) tos << player->objectName();

    QStringList log;
    log << type << (from ? from->objectName() : "") << tos.join("+") << card_str << arg << arg2;
    QVariant json_log = JsonUtils::toJsonArray(log);
    return json_log;
}

QString EventTriplet::toString() const
{
    return QString("event[%1], room[%2]\n")
        .arg(_m_event)
        .arg(_m_room->getId());
}

RoomThread::RoomThread(Room *room)
    : room(room)
{
}

void RoomThread::addPlayerSkills(ServerPlayer *player, bool invoke_game_start)
{
    bool invoke_verify = false;

    foreach (const TriggerSkill *skill, player->getTriggerSkills()) {
        addTriggerSkill(skill);

        if (invoke_game_start && skill->getTriggerEvents().contains(GameStart))
            invoke_verify = true;
    }

    //We should make someone trigger a whole GameStart event instead of trigger a skill only.
    if (invoke_verify)
        trigger(GameStart, room, QVariant::fromValue(player));
}

void RoomThread::constructTriggerTable()
{
    foreach(ServerPlayer *player, room->getPlayers())
        addPlayerSkills(player, true);
}

ServerPlayer *RoomThread::find3v3Next(QList<ServerPlayer *> &first, QList<ServerPlayer *> &second)
{
    bool all_actioned = true;
    foreach (ServerPlayer *player, room->m_alivePlayers) {
        if (!player->hasFlag("actioned")) {
            all_actioned = false;
            break;
        }
    }

    if (all_actioned) {
        foreach (ServerPlayer *player, room->m_alivePlayers) {
            room->setPlayerFlag(player, "-actioned");
            trigger(ActionedReset, room, QVariant::fromValue(player));
        }

        qSwap(first, second);
        QList<ServerPlayer *> first_alive;
        foreach (ServerPlayer *p, first) {
            if (p->isAlive())
                first_alive << p;
        }
        return room->askForPlayerChosen(first.first(), first_alive, "3v3-action", "@3v3-action");
    }

    ServerPlayer *current = room->getCurrent();
    if (current != first.first()) {
        ServerPlayer *another = NULL;
        if (current == first.last())
            another = first.at(1);
        else
            another = first.last();
        if (!another->hasFlag("actioned") && another->isAlive())
            return another;
    }

    QList<ServerPlayer *> targets;
    do {
        targets.clear();
        qSwap(first, second);
        foreach (ServerPlayer *player, first) {
            if (!player->hasFlag("actioned") && player->isAlive())
                targets << player;
        }
    } while (targets.isEmpty());

    return room->askForPlayerChosen(first.first(), targets, "3v3-action", "@3v3-action");
}

void RoomThread::run3v3(QList<ServerPlayer *> &first, QList<ServerPlayer *> &second, GameRule *game_rule, ServerPlayer *current)
{
    try {
        forever {
            room->setCurrent(current);
            trigger(TurnStart, room, QVariant::fromValue(current));
            room->setPlayerFlag(current, "actioned");
            current = find3v3Next(first, second);
        }
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            _handleTurnBroken3v3(first, second, game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::_handleTurnBroken3v3(QList<ServerPlayer *> &first, QList<ServerPlayer *> &second, GameRule *game_rule)
{
    try {
        ServerPlayer *player = room->getCurrent();
        trigger(TurnBroken, room, QVariant::fromValue(player));
        if (player->getPhase() != Player::NotActive) {
            QVariant _;
            game_rule->trigger(EventPhaseEnd, room, player, _);
            player->changePhase(player->getPhase(), Player::NotActive);
        }
        if (!player->hasFlag("actioned"))
            room->setPlayerFlag(player, "actioned");

        ServerPlayer *next = find3v3Next(first, second);
        run3v3(first, second, game_rule, next);
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken) {
            _handleTurnBroken3v3(first, second, game_rule);
        } else {
            throw triggerEvent;
        }
    }
}

ServerPlayer *RoomThread::findHulaoPassNext(ServerPlayer *shenlvbu, QList<ServerPlayer *> league, int stage)
{
    ServerPlayer *current = room->getCurrent();
    if (stage == 1) {
        if (current == shenlvbu) {
            foreach (ServerPlayer *p, league) {
                if (p->isAlive() && !p->hasFlag("actioned"))
                    return p;
            }
            foreach (ServerPlayer *p, league) {
                if (p->isAlive())
                    return p;
            }
            Q_ASSERT(false);
            return league.first();
        } else {
            return shenlvbu;
        }
    } else {
        Q_ASSERT(stage == 2);
        return current->getNextAlive();
    }
}

void RoomThread::actionHulaoPass(ServerPlayer *shenlvbu, QList<ServerPlayer *> league, GameRule *game_rule, int stage)
{
    try {
        if (stage == 1) {
            forever{
                ServerPlayer *current = room->getCurrent();
                trigger(TurnStart, room, QVariant::fromValue(current));

                ServerPlayer *next = findHulaoPassNext(shenlvbu, league, 1);
                if (current != shenlvbu) {
                    if (current->isAlive() && !current->hasFlag("actioned"))
                        room->setPlayerFlag(current, "actioned");
                } else {
                    bool all_actioned = true;
                    foreach (ServerPlayer *player, league) {
                        if (player->isAlive() && !player->hasFlag("actioned")) {
                            all_actioned = false;
                            break;
                        }
                    }
                    if (all_actioned) {
                        foreach (ServerPlayer *player, league) {
                            if (player->hasFlag("actioned"))
                                room->setPlayerFlag(player, "-actioned");
                        }
                        foreach (ServerPlayer *player, league) {
                            if (player->isDead())
                                trigger(TurnStart, room, QVariant::fromValue(player));
                        }
                    }
                }

                room->setCurrent(next);
            }
        } else {
            Q_ASSERT(stage == 2);
            forever{
                ServerPlayer *current = room->getCurrent();
                trigger(TurnStart, room);

                ServerPlayer *next = findHulaoPassNext(shenlvbu, league, 2);

                if (current == shenlvbu) {
                    foreach (ServerPlayer *player, league) {
                        if (player->isDead())
                            trigger(TurnStart, room, QVariant::fromValue(player));
                    }
                }
                room->setCurrent(next);
            }
        }
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == StageChange) {
            stage = 2;
            trigger(triggerEvent, room, QVariant::fromValue(shenlvbu));
            foreach (ServerPlayer *player, room->getPlayers()) {
                if (player != shenlvbu) {
                    if (player->hasFlag("actioned"))
                        room->setPlayerFlag(player, "-actioned");

                    if (player->getPhase() != Player::NotActive) {
                        game_rule->trigger(EventPhaseEnd, room, player);
                        player->changePhase(player->getPhase(), Player::NotActive);
                    }
                }
            }

            room->setCurrent(shenlvbu);
            actionHulaoPass(shenlvbu, league, game_rule, 2);
        } else if (triggerEvent == TurnBroken) {
            _handleTurnBrokenHulaoPass(shenlvbu, league, game_rule, stage);
        } else {
            throw triggerEvent;
        }
    }
}

void RoomThread::_handleTurnBrokenHulaoPass(ServerPlayer *shenlvbu, QList<ServerPlayer *> league, GameRule *game_rule, int stage)
{
    try {
        ServerPlayer *player = room->getCurrent();
        trigger(TurnBroken, room, QVariant::fromValue(player));
        ServerPlayer *next = findHulaoPassNext(shenlvbu, league, stage);
        if (player->getPhase() != Player::NotActive) {
            QVariant _;
            game_rule->trigger(EventPhaseEnd, room, player, _);
            player->changePhase(player->getPhase(), Player::NotActive);
            if (player != shenlvbu && stage == 1)
                room->setPlayerFlag(player, "actioned");
        }

        room->setCurrent(next);
        actionHulaoPass(shenlvbu, league, game_rule, stage);
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            _handleTurnBrokenHulaoPass(shenlvbu, league, game_rule, stage);
        else
            throw triggerEvent;
    }
}

void RoomThread::actionNormal(GameRule *game_rule)
{
    try {
        forever{
            trigger(TurnStart, room, QVariant::fromValue(room->getCurrent()));
            if (room->isFinished()) break;
            room->setCurrent(room->getCurrent()->getNextAlive());
        }
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            _handleTurnBrokenNormal(game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::_handleTurnBrokenNormal(GameRule *game_rule)
{
    try {
        ServerPlayer *player = room->getCurrent();
        trigger(TurnBroken, room, QVariant::fromValue(player));
        ServerPlayer *next = player->getNextAlive();
        if (player->getPhase() != Player::NotActive) {
            QVariant _;
            game_rule->trigger(EventPhaseEnd, room, player, _);
            player->changePhase(player->getPhase(), Player::NotActive);
        }

        room->setCurrent(next);
        actionNormal(game_rule);
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            _handleTurnBrokenNormal(game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::run()
{
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
    Sanguosha->registerRoom(room);
    // GameRule *game_rule;
    if (room->getMode() == "04_1v3")
        game_rule = new HulaoPassMode(this);
    else if (Config.EnableScene)    //changjing
        game_rule = new SceneRule(this);    //changjing
    else
        game_rule = new GameRule(this);

    addTriggerSkill(game_rule);
    foreach(const TriggerSkill *triggerSkill, Sanguosha->getGlobalTriggerSkills())
        addTriggerSkill(triggerSkill);
    if (Config.EnableBasara) addTriggerSkill(new BasaraMode(this));

    if (room->getScenario() != NULL) {
        const ScenarioRule *rule = room->getScenario()->getRule();
        if (rule) addTriggerSkill(rule);
    }

    // start game
    try {
        QString order;
        QList<ServerPlayer *> warm, cool;
        QList<ServerPlayer *> first, second;
        if (room->getMode() == "06_3v3") {
            foreach (ServerPlayer *player, room->m_players) {
                switch (player->getRoleEnum()) {
                    case Player::Lord: warm.prepend(player); break;
                    case Player::Loyalist: warm.append(player); break;
                    case Player::Renegade: cool.prepend(player); break;
                    case Player::Rebel: cool.append(player); break;
                }
            }
            order = room->askForOrder(cool.first(), "cool");
            if (order == "warm") {
                first = warm;
                second = cool;
            } else {
                first = cool;
                second = warm;
            }
        }
        constructTriggerTable();
        trigger(GameStart, room);
        if (room->getMode() == "06_3v3") {
            run3v3(first, second, game_rule, first.first());
        } else if (room->getMode() == "04_1v3") {
            ServerPlayer *shenlvbu = room->getLord();
            QList<ServerPlayer *> league = room->getPlayers();
            league.removeOne(shenlvbu);

            room->setCurrent(league.first());
            actionHulaoPass(shenlvbu, league, game_rule, 1);
        } else {
            if (room->getMode() == "02_1v1") {
                ServerPlayer *first = room->getPlayers().first();
                if (first->getRole() != "renegade")
                    first = room->getPlayers().at(1);
                ServerPlayer *second = first->getNext();
                trigger(Debut, room, QVariant::fromValue(first));
                trigger(Debut, room, QVariant::fromValue(second));
                room->setCurrent(first);
            }

            actionNormal(game_rule);
        }
    }
    catch (TriggerEvent triggerEvent) {
        if (triggerEvent == GameFinished) {
            game_rule->deleteLater();
            Sanguosha->unregisterRoom();
            return;
        } else
            Q_ASSERT(false);
    }
}

//Eternal

const QList<EventTriplet> *RoomThread::getEventStack() const
{
    return &event_stack;
}

bool RoomThread::trigger(TriggerEvent triggerEvent, Room *room)
{
    QVariant data;
    return trigger(triggerEvent, room, data);
}

bool RoomThread::trigger(TriggerEvent triggerEvent, Room *room, QVariant &data) // player is deleted. a lot of things is able to put in data. make a struct for every triggerevent isn't absolutely unreasonable.
{
    EventTriplet triplet(triggerEvent, room);
    event_stack.push_back(triplet);

    QList<const TriggerSkill *> skillList = skill_table[triggerEvent]; // find all the skills, do the record first. it do the things only for record. it should not and must not interfere the procedure of other skills.
    foreach(const TriggerSkill *skill, skillList)
        skill->record(triggerEvent, room, data);

    QList<QSharedPointer<SkillInvokeDetail> > details;
    QSet<QSharedPointer<SkillInvokeDetail> > triggered;
    bool interrupt = false;
    try {
        forever {
            getSkillAndSort(triggerEvent, room, details, triggered, data);

            QList<QSharedPointer<SkillInvokeDetail> > sameTiming;
            // search for the first skills which can trigger
            foreach (const QSharedPointer<SkillInvokeDetail> &ptr, details) {
                if (ptr->triggeredTimes >= ptr->times)
                    continue;
                if (sameTiming.isEmpty())
                    sameTiming << ptr;
                else if (ptr->sameTimingWith(*sameTiming.first()))
                    sameTiming << ptr;
            }

            // if not found, it means that all the skills is triggered done, we can exit the loop now.
            if (sameTiming.isEmpty())
                break;

            QSharedPointer<SkillInvokeDetail> invoke = sameTiming.first();

            // treat the invoker is NULL, if the triggered skill is some kind of gamerule
            if (sameTiming.length() >= 2 && invoke->invoker != NULL && invoke->skill->getPriority() < 5) { // if the priority is bigger than 5, that means it could be some kind of notify-client skill or fakemove skill, then no need to select the trigger order at this time
                // select the triggerorder of same timing
                // if there is a compulsory skill or compulsory effect, it shouldn't be able to cancel
                bool has_compulsory = false;
                foreach (const QSharedPointer<SkillInvokeDetail> &detail, sameTiming) {
                    if (detail->skill->getFrequency() == Skill::Compulsory) { // according to the difference of the different Sha game, there could be wake skill or not-compulsory skill
                        has_compulsory = true;
                        break;
                    }
                }
                // since the invoker of the sametiming list is the same, we can use sameTiming.first()->invoker to judge the invoker of this time
                QSharedPointer<SkillInvokeDetail> detailSelected = room->askForTriggerOrder(sameTiming.first()->invoker, sameTiming, !has_compulsory, data);
                if (detailSelected.isNull() || !detailSelected->isValid()) {
                    // if cancel is pushed when it is cancelable, we set the triggered time of all the sametiming to 65535, and add all the skills to triggeredList, continue the next loop
                    foreach (const QSharedPointer<SkillInvokeDetail> &ptr, sameTiming) {
                        ptr->triggeredTimes = 65535;
                        triggered.insert(ptr);
                    }
                    continue;
                } else
                    invoke = detailSelected;
            }

            // if not cancelled, then we add the selected skill to triggeredList, and add the triggered times of the skill. then we process with the skill's cost and effect.

            ++invoke->triggeredTimes;
            triggered.insert(invoke);

            // if cost returned false, we don't process with the skill's left trigger times(use the trick of set the triggered times to 65535) .
            // if effect returned true, exit the whole loop.

            if (invoke->skill->cost(triggerEvent, invoke, data)) {
                // the show general of hegemony mode can be inserted here
                if (invoke->skill->effect(triggerEvent, invoke, data)) {
                    interrupt = true;
                    break;
                }
            } else
                invoke->triggeredTimes = 65535;
        }

        foreach(AI *ai, room->ais)
            ai->filterEvent(triggerEvent, data);

        event_stack.pop_back();

    }
    catch (TriggerEvent triggerEvent) {
        foreach(AI *ai, room->ais)
            ai->filterEvent(triggerEvent, data);

        event_stack.pop_back();
        throw;
    }
    return interrupt;
}

void RoomThread::getSkillAndSort(TriggerEvent triggerEvent, Room *room, QList<QSharedPointer<SkillInvokeDetail> > &detailsList, const QSet<QSharedPointer<SkillInvokeDetail> > &triggered, const QVariant &data)
{
    // used to get all the skills which can be triggered now, and sort them.
    // everytime this function is called, it will get all the skiils and judge the triggerable one by one
    QList<const TriggerSkill *> skillList = skill_table[triggerEvent];
    QList<QSharedPointer<SkillInvokeDetail> > details; // We create a new list everytime this function is called
    foreach (const TriggerSkill *skill, skillList) {
        // judge every skill
        QList<SkillInvokeDetail> triggerable = skill->triggerable(triggerEvent, room, data);
        foreach (const SkillInvokeDetail &t, triggerable) {
            if (!t.isValid()) // remove the invalid item from the list
                triggerable.removeOne(t);
        }
        if (triggerable.isEmpty()) // i.e. there is no valid item returned from the skill's triggerable
            continue;

        QList<QSharedPointer<SkillInvokeDetail> > r; // we create a list for every skill
        foreach (const QSharedPointer<SkillInvokeDetail> &ptr, (detailsList + triggered.toList()).toSet()) {
            // If there is the same skill in detailsList or in triggeredList(i.e. the skill, the invoker, the owner is all the same), we use the old item and add it to the new list in order to preserve the old messages from tag, etc.
            // but there could be some status to be updated, so we can't call removeOne directly after the judgement of contains. we should update the new status to the old item. so we should pick up the new contents.
            foreach (const SkillInvokeDetail &t, triggerable) {
                if (t == *ptr) { // this is the overrided operator =. as the operator = judge the skill, the skill owner, the skill invoker (i.e, it is judging the same skill), the other variables may be different
                    // temperily we decided only to update the times and tag. we overwrite the new times to the old ones. 
                    // as for tag, if we only copy the (key, value) pair to the old ones, it is possible that some useful messages be overwritten. so we add a new procedure of copying the old data.
                    ptr->times = t.times;
                    foreach (const QString &key, t.tag.keys()) {
                        std::function<void(const QString &)> copyKeyToOld = [&ptr, copyKeyToOld](const QString &key) {
                            QString old_key = key + "_old";
                            if (ptr->tag.contains(old_key)) {
                                copyKeyToOld(old_key);
                                ptr->tag[old_key] = ptr->tag.value(key);
                            }
                        };
                        if (ptr->tag.contains(key))
                            copyKeyToOld(key);
                        ptr->tag[key] = t.tag.value(key);
                    }

                    triggerable.removeOne(t);
                    r << ptr;
                    break;
                }
            }
        }
        foreach (const SkillInvokeDetail &t, triggerable) {
            // if there is no related skill in detailsList and triggeredList, we copy construct a new SkillInvokeDetail in the heap area (because the return value from triggerable is in the stack). use a shared pointer to point to it, and add it to the new list.
            // because the shared pointer will destroy the item it point to when all the instances of the pointer is destroyed, so there is no memory leak. this method also guaranteed all the details has only one instances in the listÀý
            // it is needed that there is only one instance of every certain skill. so we add a procedure of remove duplicate here
            bool duplicateFlag = false;
            foreach (const QSharedPointer<SkillInvokeDetail> &d, (details + r).toSet()) {
                if (*d == t) {
                    duplicateFlag = true; // the skill judge is duplicated (Bazhen, Linglong, since a certain player can have only one EightDiagram)
                    break;
                }
            }
            if (duplicateFlag)
                continue;

            QSharedPointer<SkillInvokeDetail> ptr(new SkillInvokeDetail(t));
            r << ptr;
        }
        details << r;
    }

    // do a stable sort to details use the operator < of SkillInvokeDetail in which judge the priority, the seat of invoker, and whether it is a skill of an equip.
    std::stable_sort(details.begin(), details.end(), [](const QSharedPointer<SkillInvokeDetail> &a1, const QSharedPointer<SkillInvokeDetail> &a2) { return *a1 < *a2; });

    // mark the skills which missed the trigger timing as it has triggered all the times
    QSharedPointer<SkillInvokeDetail> over_trigger;
    QListIterator<QSharedPointer<SkillInvokeDetail> > it(details);
    it.toBack();
    while (it.hasPrevious()) { // search the last skill which triggered times isn't 0 from back to front. if found, save it to over_trigger. if over_trigger is valid, then mark the skills which missed the trigger timing as it has triggered 65535 times (we assume that a skill can't trigger tiat much times)
        const QSharedPointer<SkillInvokeDetail> &detail = it.previous();
        if (over_trigger.isNull() || !over_trigger->isValid()) {
            if (detail->triggeredTimes > 0)
                over_trigger = detail;
        } else if (*detail < *over_trigger)
            detail->triggeredTimes = 65535;
    }

    detailsList = details;
}

void RoomThread::addTriggerSkill(const TriggerSkill *skill)
{
    if (skill == NULL || skillSet.contains(skill->objectName()))
        return;

    skillSet << skill->objectName();

    QList<TriggerEvent> events = skill->getTriggerEvents();
    foreach (TriggerEvent triggerEvent, events) {
        QList<const TriggerSkill *> &table = skill_table[triggerEvent];
        table << skill;
    }

    if (skill->isVisible()) {
        foreach (const Skill *skill, Sanguosha->getRelatedSkills(skill->objectName())) {
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
            if (trigger_skill)
                addTriggerSkill(trigger_skill);
        }
    }
}

void RoomThread::delay(long secs)
{
    if (secs == -1) secs = Config.AIDelay;
    Q_ASSERT(secs >= 0);
    if (room->property("to_test").toString().isEmpty() && Config.AIDelay > 0)
        msleep(secs);
}

