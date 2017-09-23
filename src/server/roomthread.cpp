#include "roomthread.h"
#include "ai.h"
#include "engine.h"
#include "gamerule.h"
#include "room.h"
#include "scenario.h"
#include "scenerule.h"
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
    foreach (ServerPlayer *player, to)
        if (player != NULL)
            tos << player->objectName();

    return QString("%1:%2->%3:%4:%5:%6").arg(type).arg(from ? from->objectName() : "").arg(tos.join("+")).arg(card_str).arg(arg).arg(arg2);
}

QVariant LogMessage::toJsonValue() const
{
    QStringList tos;
    foreach (ServerPlayer *player, to)
        if (player != NULL)
            tos << player->objectName();

    QStringList log;
    log << type << (from ? from->objectName() : "") << tos.join("+") << card_str << arg << arg2;
    QVariant json_log = JsonUtils::toJsonArray(log);
    return json_log;
}

QString EventTriplet::toString() const
{
    return QString("event[%1], room[%2]\n").arg(_m_event).arg(_m_room->getId());
}

RoomThread::RoomThread(Room *room)
    : room(room)
    , nextExtraTurn(NULL)
    , extraTurnReturn(NULL)
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
    if (invoke_verify) {
        QVariant v = QVariant::fromValue(player);
        trigger(GameStart, room, v);
    }
}

void RoomThread::constructTriggerTable()
{
    foreach (ServerPlayer *player, room->getPlayers())
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
            QVariant v = QVariant::fromValue(player);
            trigger(ActionedReset, room, v);
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
            QVariant v = QVariant::fromValue(current);
            trigger(TurnStart, room, v);
            room->setPlayerFlag(current, "actioned");
            current = find3v3Next(first, second);
        }
    } catch (TriggerEvent triggerEvent) {
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
        QVariant v = QVariant::fromValue(player);
        trigger(TurnBroken, room, v);
        if (player->getPhase() != Player::NotActive) {
            QVariant data = QVariant::fromValue(player);
            game_rule->effect(EventPhaseEnd, room, QSharedPointer<SkillInvokeDetail>(), data);
            player->changePhase(player->getPhase(), Player::NotActive);
        }
        if (!player->hasFlag("actioned"))
            room->setPlayerFlag(player, "actioned");

        ServerPlayer *next = find3v3Next(first, second);
        run3v3(first, second, game_rule, next);
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken) {
            _handleTurnBroken3v3(first, second, game_rule);
        } else {
            throw triggerEvent;
        }
    }
}

ServerPlayer *RoomThread::findHulaoPassNext(ServerPlayer *, QList<ServerPlayer *>)
{
    ServerPlayer *current = room->getCurrent();
    return qobject_cast<ServerPlayer *>(current->getNextAlive(1, false));
}

void RoomThread::actionHulaoPass(ServerPlayer *uuz, QList<ServerPlayer *> league, GameRule *game_rule)
{
    try {
        forever {
            ServerPlayer *current = room->getCurrent();
            QVariant v = QVariant::fromValue(current);
            trigger(TurnStart, room, v);

            ServerPlayer *next = findHulaoPassNext(uuz, league);

            if (current == uuz) {
                foreach (ServerPlayer *player, league) {
                    if (player->isDead()) {
                        QVariant v = QVariant::fromValue(player);
                        trigger(TurnStart, room, v);
                    }
                }
            }
            room->setCurrent(next);
        }
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            _handleTurnBrokenHulaoPass(uuz, league, game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::_handleTurnBrokenHulaoPass(ServerPlayer *uuz, QList<ServerPlayer *> league, GameRule *game_rule)
{
    try {
        ServerPlayer *player = room->getCurrent();
        QVariant v = QVariant::fromValue(player);
        trigger(TurnBroken, room, v);
        ServerPlayer *next = findHulaoPassNext(uuz, league);
        if (player->getPhase() != Player::NotActive) {
            QVariant data = QVariant::fromValue(player);
            game_rule->effect(EventPhaseEnd, room, QSharedPointer<SkillInvokeDetail>(), data);
            player->changePhase(player->getPhase(), Player::NotActive);
        }

        room->setCurrent(next);
        actionHulaoPass(uuz, league, game_rule);
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            _handleTurnBrokenHulaoPass(uuz, league, game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::actionNormal(GameRule *game_rule)
{
    try {
        forever {
            ServerPlayer *current = room->getCurrent();
            QVariant data = QVariant::fromValue(current);
            trigger(TurnStart, room, data);
            if (room->isFinished())
                break;

            while (nextExtraTurn != NULL) {
                extraTurnReturn = current;
                room->setCurrent(nextExtraTurn);
                ServerPlayer *nextExtraTurnCopy = nextExtraTurn;
                QVariant data = QVariant::fromValue(nextExtraTurn);
                nextExtraTurn = NULL;
                room->setTag("touhou-extra", true);
                nextExtraTurnCopy->tag["touhou-extra"] = true;
                trigger(TurnStart, room, data);

                if (room->isFinished())
                    return; // break;
                nextExtraTurnCopy->tag["touhou-extra"] = false;
                nextExtraTurnCopy->tag.remove("ExtraTurnInfo");
                room->setTag("touhou-extra", false);

                current = extraTurnReturn;
                extraTurnReturn = NULL;
            }

            room->setCurrent(qobject_cast<ServerPlayer *>(current->getNextAlive(1, false)));
        }
    } catch (TriggerEvent triggerEvent) {
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
        QVariant data = QVariant::fromValue(player);
        trigger(TurnBroken, room, data);

        if (player->getPhase() != Player::NotActive) {
            game_rule->effect(EventPhaseEnd, room, QSharedPointer<SkillInvokeDetail>(), data);
            player->changePhase(player->getPhase(), Player::NotActive);
        }

        if (room->getTag("touhou-extra").toBool()) {
            room->setTag("touhou-extra", false);
            if (extraTurnReturn != NULL) {
                player = extraTurnReturn;
                extraTurnReturn = NULL;
            }
        }

        while (nextExtraTurn != NULL) {
            extraTurnReturn = player;
            room->setCurrent(nextExtraTurn);
            ServerPlayer *nextExtraTurnCopy = nextExtraTurn;
            QVariant data = QVariant::fromValue(nextExtraTurn);
            nextExtraTurn = NULL;
            room->setTag("touhou-extra", true);
            nextExtraTurnCopy->tag["touhou-extra"] = true;
            trigger(TurnStart, room, data);

            if (room->isFinished())
                return; //break;
            nextExtraTurnCopy->tag["touhou-extra"] = false;
            room->setTag("touhou-extra", false);

            player = extraTurnReturn;
            extraTurnReturn = NULL;
        }

        ServerPlayer *next = qobject_cast<ServerPlayer *>(player->getNextAlive(1, false));
        room->setCurrent(next);
        actionNormal(game_rule);
    } catch (TriggerEvent triggerEvent) {
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
    else
        game_rule = new GameRule(this);

    addTriggerSkill(game_rule);
    foreach (const TriggerSkill *triggerSkill, Sanguosha->getGlobalTriggerSkills())
        addTriggerSkill(triggerSkill);

    if (room->getScenario() != NULL) {
        const ScenarioRule *rule = room->getScenario()->getRule();
        if (rule)
            addTriggerSkill(rule);
    }

    // start game
    try {
        QString order;
        QList<ServerPlayer *> warm, cool;
        QList<ServerPlayer *> first, second;
        if (room->getMode() == "06_3v3") {
            foreach (ServerPlayer *player, room->m_players) {
                switch (player->getRoleEnum()) {
                case Player::Lord:
                    warm.prepend(player);
                    break;
                case Player::Loyalist:
                    warm.append(player);
                    break;
                case Player::Renegade:
                    cool.prepend(player);
                    break;
                case Player::Rebel:
                    cool.append(player);
                    break;
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
            ServerPlayer *uuz = room->getLord();
            QList<ServerPlayer *> league = room->getPlayers();
            league.removeOne(uuz);

            room->setCurrent(league.first());
            actionHulaoPass(uuz, league, game_rule);
        } else {
            if (room->getMode() == "02_1v1") {
                ServerPlayer *first = room->getPlayers().first();
                if (first->getRole() != "renegade")
                    first = room->getPlayers().at(1);
                ServerPlayer *second = room->getOtherPlayers(first).first();
                //ServerPlayer *second = first->getNext();
                QVariant v1 = QVariant::fromValue(first);
                trigger(Debut, room, v1);
                QVariant v2 = QVariant::fromValue(second);
                trigger(Debut, room, v2);
                room->setCurrent(first);
            }

            actionNormal(game_rule);
        }
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == GameFinished) {
            game_rule->deleteLater();
            Sanguosha->unregisterRoom();
            return;
        } else
            Q_ASSERT(false);
    }
}

const QList<EventTriplet> *RoomThread::getEventStack() const
{
    return &event_stack;
}

bool RoomThread::trigger(TriggerEvent triggerEvent, Room *room)
{
    QVariant data;
    return trigger(triggerEvent, room, data);
}

void RoomThread::getSkillAndSort(TriggerEvent triggerEvent, Room *room, QList<QSharedPointer<SkillInvokeDetail> > &detailsList,
                                 const QList<QSharedPointer<SkillInvokeDetail> > &triggered, const QVariant &data)
{
    // used to get all the skills which can be triggered now, and sort them.
    // everytime this function is called, it will get all the skiils and judge the triggerable one by one
    QList<const TriggerSkill *> skillList = skill_table[triggerEvent];
    QList<QSharedPointer<SkillInvokeDetail> > details; // We create a new list everytime this function is called
    foreach (const TriggerSkill *skill, skillList) {
        // judge every skill
        QList<SkillInvokeDetail> triggerable = skill->triggerable(triggerEvent, room, data);

        QMutableListIterator<SkillInvokeDetail> it_triggerable(triggerable);
        while (it_triggerable.hasNext()) {
            const SkillInvokeDetail &t = it_triggerable.next();
            if (!t.isValid())
                it_triggerable.remove(); // remove the invalid item from the list
        }

        if (triggerable.isEmpty()) // i.e. there is no valid item returned from the skill's triggerable
            continue;

        QList<QSharedPointer<SkillInvokeDetail> > r; // we create a list for every skill
        foreach (const SkillInvokeDetail &t, triggerable) {
            // we copy construct a new SkillInvokeDetail in the heap area (because the return value from triggerable is in the stack). use a shared pointer to point to it, and add it to the new list.
            // because the shared pointer will destroy the item it point to when all the instances of the pointer is destroyed, so there is no memory leak.
            QSharedPointer<SkillInvokeDetail> ptr(new SkillInvokeDetail(t));
            r << ptr;
        }
        if (r.length() == 1 && r.first()->preferredTarget == NULL) {
            // if the skill has only one instance of the invokedetail, we copy the tag to the old instance(overwrite the old ones), and use the old instance, delete the new one
            foreach (const QSharedPointer<SkillInvokeDetail> &detail, (detailsList + triggered).toSet()) {
                if (detail->sameSkill(*r.first())) {
                    foreach (const QString &key, r.first()->tag.keys())
                        detail->tag[key] = r.first()->tag.value(key);
                    r.clear();
                    r << detail;
                    break;
                }
            }
        } else {
            bool isPreferredTargetSkill = false;
            QList<QSharedPointer<SkillInvokeDetail> > s;
            // judge whether this skill in this event is a preferred-target skill, make a invoke list as s
            foreach (const QSharedPointer<SkillInvokeDetail> &detail, (detailsList + triggered).toSet()) {
                if (detail->skill == r.first()->skill) {
                    s << detail;
                    if (detail->preferredTarget != NULL)
                        isPreferredTargetSkill = true;
                }
            }
            if (!isPreferredTargetSkill) {
                std::stable_sort(s.begin(), s.end(),
                                 [](const QSharedPointer<SkillInvokeDetail> &a1, const QSharedPointer<SkillInvokeDetail> &a2) { return a1->triggered && !a2->triggered; });
                // because these are of one single skill, so we can pick the invoke list using a trick like this
                s.append(r);
                r = s.mid(0, r.length());
            } else {
                // do a stable sort to r and s since we should judge the trigger order
                static std::function<bool(const QSharedPointer<SkillInvokeDetail> &, const QSharedPointer<SkillInvokeDetail> &)> preferredTargetLess
                    = [](const QSharedPointer<SkillInvokeDetail> &a1, const QSharedPointer<SkillInvokeDetail> &a2) { return a1->preferredTargetLess(*a2); };

                std::stable_sort(r.begin(), r.end(), preferredTargetLess);
                std::stable_sort(s.begin(), s.end(), preferredTargetLess);

                {
                    // we should mark the ones who passed the trigger order as triggered. Judging both r and s here
                    // ASSUMING the values returned by TriggerSkill::triggerable are all with triggered == false
                    QSharedPointer<SkillInvokeDetail> over_trigger_prefferedTarget;

                    QListIterator<QSharedPointer<SkillInvokeDetail> > s_it(s);
                    s_it.toBack();
                    while (s_it.hasPrevious()) {
                        const QSharedPointer<SkillInvokeDetail> p = s_it.previous();
                        if (over_trigger_prefferedTarget.isNull() || !over_trigger_prefferedTarget->isValid()) {
                            if (p->triggered) {
                                over_trigger_prefferedTarget = p;
                                break;
                            }
                        }
                    }

                    // find an over-triggered target, need to set all over-triggered items to triggered
                    if (!over_trigger_prefferedTarget.isNull() && over_trigger_prefferedTarget->isValid()) {
                        QListIterator<QSharedPointer<SkillInvokeDetail> > r_it(r);
                        r_it.toBack();
                        while (r_it.hasPrevious()) {
                            const QSharedPointer<SkillInvokeDetail> p = r_it.previous();
                            if (preferredTargetLess(p, over_trigger_prefferedTarget))
                                p->triggered = true;
                        }
                        QListIterator<QSharedPointer<SkillInvokeDetail> > s_it(s);
                        s_it.toBack();
                        while (s_it.hasPrevious()) {
                            const QSharedPointer<SkillInvokeDetail> p = s_it.previous();
                            if (preferredTargetLess(p, over_trigger_prefferedTarget))
                                p->triggered = true;
                        }
                    }
                }

                {
                    // add new valid items to s and remove the invalid values from s, using r
                    QListIterator<QSharedPointer<SkillInvokeDetail> > r_it(r);
                    QMutableListIterator<QSharedPointer<SkillInvokeDetail> > s_it(s);
                    while (r_it.hasNext() && s_it.hasNext()) {
                        QSharedPointer<SkillInvokeDetail> r_now = r_it.next();
                        QSharedPointer<SkillInvokeDetail> s_now = s_it.next();

                        if (r_now->preferredTarget == s_now->preferredTarget)
                            continue;

                        // e.g. let r =  a b c d e f   h
                        //      let s =  a b   d e f g h i j
                        // it pos:      *

                        // the position of Qt's Java style iterator is between 2 items,
                        // we can use next() to get the next item and use previous() to get the previous item, and move the iterator according to the direction.

                        if (ServerPlayer::CompareByActionOrder(r_now->preferredTarget, s_now->preferredTarget)) {
                            // 1.the case that ServerPlayer::compareByActionOrder(r_now, s_now) == true, i.e. seat(r_now) < seat(s_now)
                            // because the r is triggerable list, s is the invoke list, so we should move s_it to the front of the just passed item add the r_now into s

                            // the list is now like this: r = a b c d e f   h
                            //                            s = a b   d e f g h i j
                            // it pos:                             r s

                            s_it.previous();
                            s_it.insert(r_now);

                            // so the list becomes like:  r = a b c d e f   h
                            //                            s = a b c d e f g h i j
                            // it pos:                             *
                        } else {
                            // 2. the case that ServerPlayer::compareByActionOrder(r_now, s_now) == false, i.e. seat(r_now) > seat(s_now)
                            // because the r is triggerable list, s is the invoke list, so we should remove the s_now and move r_it to the position just before the deleted item

                            // the list is now like this: r = a b c d e f   h
                            //                            s = a b c d e f g h i j
                            // it pos:                                     s r

                            s_it.remove();
                            r_it.previous();
                            // so the list becomes like:  r = a b c d e f   h
                            //                            s = a b c d e f   h i j
                            // it pos:                                   r s
                        }
                    }

                    // the whole loop will be over when one of r_it or s_it has no next item, but there are situations that another one has more items. Let's deal with this situation.
                    // let's take some other examples.

                    // e.g. let r = a b c d e
                    //      let s = a b c
                    // it pos:           *

                    // now s_it has no next item, but r_it has some next items.
                    // since r is the trigger list, we should add the more items to s.

                    while (r_it.hasNext())
                        s_it.insert(r_it.next());

                    // another example.

                    // e.g. let r = a b c
                    //          s = a b c d e
                    // it pos:           *

                    // now s_it has more items.
                    // since r is the triggerable list, we should remove the more items from s.
                    while (s_it.hasNext()) {
                        s_it.next();
                        s_it.remove();
                    }
                }

                // let the r become the invoke list.
                r = s;
            }
        }

        details << r;
    }

    // do a stable sort to details use the operator < of SkillInvokeDetail in which judge the priority, the seat of invoker, and whether it is a skill of an equip.
    std::stable_sort(details.begin(), details.end(), [](const QSharedPointer<SkillInvokeDetail> &a1, const QSharedPointer<SkillInvokeDetail> &a2) { return *a1 < *a2; });

    // mark the skills which missed the trigger timing as it has triggered
    QSharedPointer<SkillInvokeDetail> over_trigger = (triggered.isEmpty() ? QSharedPointer<SkillInvokeDetail>() : triggered.last());

    QListIterator<QSharedPointer<SkillInvokeDetail> > it(details);
    it.toBack();
    while (it.hasPrevious()) {
        // search the last skill which triggered times isn't 0 from back to front. if found, save it to over_trigger.
        // if over_trigger is valid, then mark the skills which missed the trigger timing as it has triggered.
        const QSharedPointer<SkillInvokeDetail> &detail = it.previous();
        if (over_trigger.isNull() || !over_trigger->isValid()) {
            if (detail->triggered)
                over_trigger = detail;
        } else if (*detail < *over_trigger)
            detail->triggered = true;
    }

    detailsList = details;
}

// player is deleted. a lot of things is able to put in data. make a struct for every triggerevent isn't absolutely unreasonable.
bool RoomThread::trigger(TriggerEvent triggerEvent, Room *room, QVariant &data)
{
    EventTriplet triplet(triggerEvent, room);
    event_stack.push_back(triplet);

    // find all the skills, do the record first. it do the things only for record. it should not and must not interfere the procedure of other skills.
    QList<const TriggerSkill *> skillList = skill_table[triggerEvent];
    foreach (const TriggerSkill *skill, skillList)
        skill->record(triggerEvent, room, data);

    QList<QSharedPointer<SkillInvokeDetail> > details;
    QList<QSharedPointer<SkillInvokeDetail> > triggered;
    bool interrupt = false;
    try {
        forever {
            getSkillAndSort(triggerEvent, room, details, triggered, data);

            QList<QSharedPointer<SkillInvokeDetail> > sameTiming;
            // search for the first skills which can trigger
            foreach (const QSharedPointer<SkillInvokeDetail> &ptr, details) {
                if (ptr->triggered)
                    continue;
                if (sameTiming.isEmpty())
                    sameTiming << ptr;
                else if (ptr->sameTimingWith(*sameTiming.first())) {
                    if (!ptr->isCompulsory)
                        sameTiming << ptr;
                    else {
                        // For Compulsory Skill at the same timing, just add the first one into sameTiming. etc. like QinggangSword
                        bool sameTimingCompulsory = false;
                        foreach (const QSharedPointer<SkillInvokeDetail> &detail, sameTiming) {
                            if (detail->skill == ptr->skill) {
                                sameTimingCompulsory = true;
                                break;
                            }
                        }
                        if (!sameTimingCompulsory)
                            sameTiming << ptr;
                    }
                }
            }

            // if not found, it means that all the skills is triggered done, we can exit the loop now.
            if (sameTiming.isEmpty())
                break;

            QSharedPointer<SkillInvokeDetail> invoke = sameTiming.first();
            // treat the invoker is NULL, if the triggered skill is some kind of gamerule
            // if the priority is bigger than 5 or smaller than -5, that means it could be some kind of record skill,
            //    notify-client skill or fakemove skill, then no need to select the trigger order at this time
            if (sameTiming.length() >= 2 && invoke->invoker != NULL && (invoke->skill->getPriority() >= -5 && invoke->skill->getPriority() <= 5)) {
                // select the triggerorder of same timing
                // if there is a compulsory skill or compulsory effect, it shouldn't be able to cancel
                bool has_compulsory = false;
                foreach (const QSharedPointer<SkillInvokeDetail> &detail, sameTiming) {
                    if (detail->isCompulsory && !detail->invoker->isHiddenSkill(detail->skill->objectName())) { // judge the compulsory effect/skill in the detail struct
                        has_compulsory = true;
                        break;
                    }
                }
                // since the invoker of the sametiming list is the same, we can use sameTiming.first()->invoker to judge the invoker of this time
                QSharedPointer<SkillInvokeDetail> detailSelected = room->askForTriggerOrder(sameTiming.first()->invoker, sameTiming, !has_compulsory, data);
                if (detailSelected.isNull() || !detailSelected->isValid()) {
                    // if cancel is pushed when it is cancelable, we set all the sametiming as triggered, and add all the skills to triggeredList, continue the next loop
                    foreach (const QSharedPointer<SkillInvokeDetail> &ptr, sameTiming) {
                        ptr->triggered = true;
                        triggered << ptr;
                    }
                    continue;
                } else
                    invoke = detailSelected;
            }

            // if not cancelled, then we add the selected skill to triggeredList, and add the triggered times of the skill. then we process with the skill's cost and effect.
            invoke->triggered = true;
            triggered << invoke;

            // if cost returned false, we don't process with the skill's left trigger times(use the trick of set it as triggered)
            // if effect returned true, exit the whole loop.
            bool do_cost = true;
            if (invoke->isCompulsory && invoke->showhidden && invoke->invoker && invoke->invoker->isHiddenSkill(invoke->skill->objectName())) {
                if (invoke->invoker->canShowHiddenSkill()) {
                    do_cost = invoke->invoker->askForSkillInvoke("invoke_hidden_compulsory", QVariant::fromValue("compulsory:" + invoke->skill->objectName()));
                    if (do_cost)
                        invoke->invoker->showHiddenSkill(invoke->skill->objectName());
                }
            }
            if (do_cost && invoke->skill->cost(triggerEvent, room, invoke, data)) {
                //show hidden skill firstly
                if (!invoke->isCompulsory && invoke->invoker)
                    invoke->invoker->showHiddenSkill(invoke->skill->objectName());
                // if we don't insert the target in the cost and there is a preferred target, we set the preferred target as the only target of the skill
                if (invoke->preferredTarget != NULL && invoke->targets.isEmpty())
                    invoke->targets << invoke->preferredTarget;
                // the show general of hegemony mode can be inserted here
                if (invoke->skill->effect(triggerEvent, room, invoke, data)) {
                    interrupt = true;
                    break;
                }
            }
        }

        foreach (AI *ai, room->ais)
            ai->filterEvent(triggerEvent, data);

        event_stack.pop_back();

    } catch (TriggerEvent triggerEvent) {
        foreach (AI *ai, room->ais)
            ai->filterEvent(triggerEvent, data);

        event_stack.pop_back();
        throw;
    }
    return interrupt;
}

void RoomThread::addTriggerSkill(const TriggerSkill *skill)
{
    if (skill == NULL || skillSet.contains(skill->objectName()))
        return;

    skillSet << skill->objectName();

    QList<TriggerEvent> events = skill->getTriggerEvents();
    if (events.length() == 1 && events.first() == NumOfEvents) {
        for (int i = NonTrigger + 1; i < NumOfEvents; ++i)
            skill_table[static_cast<TriggerEvent>(i)] << skill;
    } else {
        foreach (TriggerEvent triggerEvent, events) {
            QList<const TriggerSkill *> &table = skill_table[triggerEvent];
            table << skill;
        }
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
    if (secs == -1)
        secs = Config.AIDelay;
    Q_ASSERT(secs >= 0);
    if (room->property("to_test").toString().isEmpty() && Config.AIDelay > 0)
        msleep(secs);
}
