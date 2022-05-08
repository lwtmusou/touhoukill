#include "roomthread.h"
#include "engine.h"
#include "gamerule.h"
#include "general.h"
#include "json.h"
#include "package.h"
#include "protocol.h"
#include "room.h"
#include "settings.h"
#include "skill.h"
#include "util.h"

#include <QTime>
#include <functional>

using namespace JsonUtils;
using namespace QSanProtocol;

LogMessage::LogMessage()
    : from(nullptr)
{
}

QString LogMessage::toString() const
{
    QStringList tos;
    foreach (Player *player, to)
        if (player != nullptr)
            tos << player->objectName();

    return QStringLiteral("%1:%2->%3:%4:%5:%6").arg(type, from != nullptr ? from->objectName() : QString(), tos.join(QStringLiteral("+")), card_str, arg, arg2);
}

QVariant LogMessage::toJsonValue() const
{
    QStringList tos;
    foreach (Player *player, to)
        if (player != nullptr)
            tos << player->objectName();

    QStringList log;
    log << type << (from != nullptr ? from->objectName() : QString()) << tos.join(QStringLiteral("+")) << card_str << arg << arg2;
    QVariant json_log = JsonUtils::toJsonArray(log);
    return json_log;
}

RoomThread::RoomThread(Room *room)
    : room(room)
    , game_rule(nullptr)
    , nextExtraTurn(nullptr)
    , extraTurnReturn(nullptr)
{
}

void RoomThread::addPlayerSkills(ServerPlayer *player, bool /*unused*/)
{
    foreach (const Skill *skill, player->skills(true, true)) {
        foreach (const Trigger *trigger, skill->triggers())
            addTrigger(trigger);
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
        if (!player->hasFlag(QStringLiteral("actioned"))) {
            all_actioned = false;
            break;
        }
    }

    if (all_actioned) {
        foreach (ServerPlayer *player, room->m_alivePlayers) {
            room->setPlayerFlag(player, QStringLiteral("-actioned"));
            QVariant v = QVariant::fromValue(player);
            trigger(QSanguosha::ActionedReset, v);
        }

        qSwap(first, second);
        QList<ServerPlayer *> first_alive;
        foreach (ServerPlayer *p, first) {
            if (p->isAlive())
                first_alive << p;
        }
        return room->askForPlayerChosen(first.first(), first_alive, QStringLiteral("3v3-action"), QStringLiteral("@3v3-action"));
    }

    ServerPlayer *current = room->getCurrent();
    if (current != first.first()) {
        ServerPlayer *another = nullptr;
        if (current == first.last())
            another = first.at(1);
        else
            another = first.last();
        if (!another->hasFlag(QStringLiteral("actioned")) && another->isAlive())
            return another;
    }

    QList<ServerPlayer *> targets;
    do {
        targets.clear();
        qSwap(first, second);
        foreach (ServerPlayer *player, first) {
            if (!player->hasFlag(QStringLiteral("actioned")) && player->isAlive())
                targets << player;
        }
    } while (targets.isEmpty());

    return room->askForPlayerChosen(first.first(), targets, QStringLiteral("3v3-action"), QStringLiteral("@3v3-action"));
}

void RoomThread::run3v3(QList<ServerPlayer *> &first, QList<ServerPlayer *> &second, GameRule *game_rule, ServerPlayer *current)
{
    try {
        forever {
            room->setCurrent(current);
            QVariant v = QVariant::fromValue(current);
            trigger(QSanguosha::TurnStart, v);
            room->setPlayerFlag(current, QStringLiteral("actioned"));
            current = find3v3Next(first, second);
        }
    } catch (QSanguosha::TriggerEvent triggerEvent) {
        if (triggerEvent == QSanguosha::TurnBroken)
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
        trigger(QSanguosha::TurnBroken, v);
        if (player->phase() != QSanguosha::PhaseNotActive) {
            QVariant data = QVariant::fromValue(player);
            game_rule->trigger(QSanguosha::EventPhaseEnd, room, TriggerDetail(room), data);
            player->changePhase(player->phase(), QSanguosha::PhaseNotActive);
        }
        if (!player->hasFlag(QStringLiteral("actioned")))
            room->setPlayerFlag(player, QStringLiteral("actioned"));

        ServerPlayer *next = find3v3Next(first, second);
        run3v3(first, second, game_rule, next);
    } catch (QSanguosha::TriggerEvent triggerEvent) {
        if (triggerEvent == QSanguosha::TurnBroken) {
            _handleTurnBroken3v3(first, second, game_rule);
        } else {
            throw triggerEvent;
        }
    }
}

ServerPlayer *RoomThread::findHulaoPassNext(ServerPlayer * /*unused*/, const QList<ServerPlayer *> & /*unused*/)
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
            trigger(QSanguosha::TurnStart, v);

            ServerPlayer *next = findHulaoPassNext(uuz, league);

            if (current == uuz) {
                foreach (ServerPlayer *player, league) {
                    if (player->isDead()) {
                        QVariant v = QVariant::fromValue(player);
                        trigger(QSanguosha::TurnStart, v);
                    }
                }
            }
            room->setCurrent(next);
        }
    } catch (QSanguosha::TriggerEvent triggerEvent) {
        if (triggerEvent == QSanguosha::TurnBroken)
            _handleTurnBrokenHulaoPass(uuz, league, game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::_handleTurnBrokenHulaoPass(ServerPlayer *uuz, const QList<ServerPlayer *> &league, GameRule *game_rule)
{
    try {
        ServerPlayer *player = room->getCurrent();
        QVariant v = QVariant::fromValue(player);
        trigger(QSanguosha::TurnBroken, v);
        ServerPlayer *next = findHulaoPassNext(uuz, league);
        if (player->phase() != QSanguosha::PhaseNotActive) {
            QVariant data = QVariant::fromValue(player);
            game_rule->trigger(QSanguosha::EventPhaseEnd, room, TriggerDetail(room), data);
            player->changePhase(player->phase(), QSanguosha::PhaseNotActive);
        }

        room->setCurrent(next);
        actionHulaoPass(uuz, league, game_rule);
    } catch (QSanguosha::TriggerEvent triggerEvent) {
        if (triggerEvent == QSanguosha::TurnBroken)
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
            trigger(QSanguosha::TurnStart, data);
            if (room->isFinished())
                break;

            while (nextExtraTurn != nullptr) {
                extraTurnReturn = current;
                room->setCurrent(nextExtraTurn);
                ServerPlayer *nextExtraTurnCopy = nextExtraTurn;
                QVariant data = QVariant::fromValue(nextExtraTurn);
                nextExtraTurn = nullptr;
                room->setTag(QStringLiteral("touhou-extra"), true);
                nextExtraTurnCopy->tag[QStringLiteral("touhou-extra")] = true;
                trigger(QSanguosha::TurnStart, data);

                if (room->isFinished())
                    return;
                nextExtraTurnCopy->tag[QStringLiteral("touhou-extra")] = false;
                nextExtraTurnCopy->tag.remove(QStringLiteral("ExtraTurnInfo"));
                room->setTag(QStringLiteral("touhou-extra"), false);

                current = extraTurnReturn;
                extraTurnReturn = nullptr;
            }

            room->setCurrent(qobject_cast<ServerPlayer *>(current->getNextAlive(1, false)));
        }
    } catch (QSanguosha::TriggerEvent triggerEvent) {
        if (triggerEvent == QSanguosha::TurnBroken)
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
        trigger(QSanguosha::TurnBroken, data);

        if (player->phase() != QSanguosha::PhaseNotActive) {
            game_rule->trigger(QSanguosha::EventPhaseEnd, room, TriggerDetail(room), data);
            player->changePhase(player->phase(), QSanguosha::PhaseNotActive);
        }

        if (room->getTag(QStringLiteral("touhou-extra")).toBool()) {
            room->setTag(QStringLiteral("touhou-extra"), false);
            if (extraTurnReturn != nullptr) {
                player = extraTurnReturn;
                extraTurnReturn = nullptr;
            }
        }

        while (nextExtraTurn != nullptr) {
            extraTurnReturn = player;
            room->setCurrent(nextExtraTurn);
            ServerPlayer *nextExtraTurnCopy = nextExtraTurn;
            QVariant data = QVariant::fromValue(nextExtraTurn);
            nextExtraTurn = nullptr;
            room->setTag(QStringLiteral("touhou-extra"), true);
            nextExtraTurnCopy->tag[QStringLiteral("touhou-extra")] = true;
            trigger(QSanguosha::TurnStart, data);

            if (room->isFinished())
                return;
            nextExtraTurnCopy->tag[QStringLiteral("touhou-extra")] = false;
            room->setTag(QStringLiteral("touhou-extra"), false);

            player = extraTurnReturn;
            extraTurnReturn = nullptr;
        }

        ServerPlayer *next = qobject_cast<ServerPlayer *>(player->getNextAlive(1, false));
        room->setCurrent(next);
        actionNormal(game_rule);
    } catch (QSanguosha::TriggerEvent triggerEvent) {
        if (triggerEvent == QSanguosha::TurnBroken)
            _handleTurnBrokenNormal(game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::run()
{
    // initialize random seed for later use
    Config.AIDelay = Config.OriginAIDelay;
    foreach (ServerPlayer *player, room->getPlayers()) {
        //Ensure that the game starts with all player's mutex locked
        player->drainAllLocks();
        player->releaseLock(ServerPlayer::SEMA_MUTEX);
    }

    room->prepareForStart();

    bool using_countdown = true;
    if (!room->property("to_test").toString().isEmpty())
        using_countdown = false;

#ifndef QT_NO_DEBUG
    using_countdown = false;
#endif

    if (using_countdown) {
        for (int i = Config.CountDownSeconds; i >= 0; i--) {
            room->doBroadcastNotify(S_COMMAND_START_IN_X_SECONDS, i);
            sleep(1);
        }
    } else
        room->doBroadcastNotify(S_COMMAND_START_IN_X_SECONDS, QVariant(0));

    if (room->getMode() == QStringLiteral("04_1v3")) {
        ServerPlayer *lord = room->getPlayers().first();
        room->setPlayerProperty(lord, "general", QStringLiteral("yuyuko_1v3"));

        QList<const General *> generals = QList<const General *>();
        foreach (QString pack_name, Sanguosha->config(QStringLiteral("hulao_packages")).toStringList()) {
            const Package *pack = Sanguosha->findPackage(pack_name);
            if (pack != nullptr) {
                foreach (auto gn, pack->generals())
                    generals << gn;
            }
        }

        QStringList names;
        foreach (const General *general, generals) {
            if (general->isTotallyHidden())
                continue;
            names << general->name();
        }

        foreach (const QString &name, Config.value(QStringLiteral("Banlist/HulaoPass")).toStringList())
            if (names.contains(name))
                names.removeOne(name);

        foreach (ServerPlayer *player, room->getPlayers()) {
            if (player == lord)
                continue;

            qShuffle(names);
            QStringList choices = names.mid(0, 3);
            QString name = room->askForGeneral(player, choices);

            room->setPlayerProperty(player, "general", name);
            names.removeOne(name);
        }

        room->startGame();
    } else if (isHegemonyGameMode(room->getMode())) {
        room->chooseHegemonyGenerals();
        room->startGame();
    } else {
        room->chooseGenerals();
        room->startGame();
    }

    game_rule = new GameRule;

    addTrigger(game_rule);
    //    foreach (const TriggerSkill *triggerSkill, Sanguosha->getGlobalTriggerSkills())
    //        addTriggerSkill(triggerSkill);

    // start game
    try {
        QString order;
        QList<ServerPlayer *> warm;
        QList<ServerPlayer *> cool;
        QList<ServerPlayer *> first;
        QList<ServerPlayer *> second;
        if (room->getMode() == QStringLiteral("06_3v3")) {
            foreach (ServerPlayer *player, room->m_players) {
                switch (player->role()) {
                case QSanguosha::RoleLord:
                    warm.prepend(player);
                    break;
                case QSanguosha::RoleLoyalist:
                    warm.append(player);
                    break;
                case QSanguosha::RoleRenegade:
                    cool.prepend(player);
                    break;
                case QSanguosha::RoleRebel:
                    cool.append(player);
                    break;
                }
            }
            order = room->askForOrder(cool.first(), QStringLiteral("cool"));
            if (order == QStringLiteral("warm")) {
                first = warm;
                second = cool;
            } else {
                first = cool;
                second = warm;
            }
        }
        constructTriggerTable();
        QVariant v;
        trigger(QSanguosha::GameStart, v);
        if (room->getMode() == QStringLiteral("06_3v3")) {
            run3v3(first, second, game_rule, first.first());
        } else if (room->getMode() == QStringLiteral("04_1v3")) {
            ServerPlayer *uuz = room->getLord();
            QList<ServerPlayer *> league = room->getPlayers();
            league.removeOne(uuz);

            room->setCurrent(league.first());
            actionHulaoPass(uuz, league, game_rule);
        } else {
            if (room->getMode() == QStringLiteral("02_1v1")) {
                ServerPlayer *first = room->getPlayers().first();
                if (first->getRoleString() != QStringLiteral("renegade"))
                    first = room->getPlayers().at(1);
                ServerPlayer *second = room->getOtherPlayers(first).first();
                QVariant v1 = QVariant::fromValue(first);
                trigger(QSanguosha::Debut, v1);
                QVariant v2 = QVariant::fromValue(second);
                trigger(QSanguosha::Debut, v2);
                room->setCurrent(first);
            }

            actionNormal(game_rule);
        }
    } catch (QSanguosha::TriggerEvent triggerEvent) {
        if (triggerEvent == QSanguosha::GameFinished) {
            delete game_rule;
            return;
        } else
            Q_ASSERT(false);
    }
}

void RoomThread::getTriggerAndSort(QSanguosha::TriggerEvent e, QList<QSharedPointer<TriggerDetail>> &detailsList, const QList<QSharedPointer<TriggerDetail>> &triggered,
                                   const QVariant &data)
{
    // used to get all the skills which can be triggered now, and sort them.
    // everytime this function is called, it will get all the skiils and judge the triggerable one by one
    QList<const Trigger *> triggerList = m_triggerList[e];
    QList<QSharedPointer<TriggerDetail>> details; // We create a new list everytime this function is called
    foreach (const Trigger *skill, triggerList) {
        // judge every skill
        QList<TriggerDetail> triggerable = skill->triggerable(e, room, data);

        // assuming triggerable[X].trigger() equals with each other
        QMutableListIterator<TriggerDetail> it_triggerable(triggerable);
        while (it_triggerable.hasNext()) {
            const TriggerDetail &t = it_triggerable.next();
            if (!t.isValid())
                it_triggerable.remove(); // remove the invalid item from the list
        }

        if (triggerable.isEmpty()) // i.e. there is no valid item returned from the skill's triggerable
            continue;

        QHash<const Trigger *, QList<QSharedPointer<TriggerDetail>>> totalR; // we create a list for every skill

        foreach (const TriggerDetail &t, triggerable) {
            // we copy construct a new SkillInvokeDetail in the heap area (because the return value from triggerable is in the stack). use a shared pointer to point to it, and add it to the new list.
            // because the shared pointer will destroy the item it point to when all the instances of the pointer is destroyed, so there is no memory leak.
            QSharedPointer<TriggerDetail> ptr(new TriggerDetail(t));
            totalR[t.trigger()].append(ptr);
        }

        foreach (const QList<QSharedPointer<TriggerDetail>> &_r, totalR) {
            QList<QSharedPointer<TriggerDetail>> r = _r;
            if (r.length() == 1) {
                // if the skill has only one instance of the invokedetail, we copy the tag to the old instance(overwrite the old ones), and use the old instance, delete the new one
                auto triggeredPlusDetails = detailsList + triggered;
                foreach (const QSharedPointer<TriggerDetail> &detail, QSet<QSharedPointer<TriggerDetail>>(triggeredPlusDetails.begin(), triggeredPlusDetails.end())) {
                    if ((*detail) == (*r.first())) {
                        foreach (const QString &key, r.first()->tag().keys())
                            detail->tag()[key] = r.first()->tag().value(key);
                        r.clear();
                        r << detail;
                        break;
                    }
                }
            } else {
                QList<QSharedPointer<TriggerDetail>> s;
                // make a invoke list as s
                auto triggeredPlusDetails = detailsList + triggered;
                foreach (const QSharedPointer<TriggerDetail> &detail, QSet<QSharedPointer<TriggerDetail>>(triggeredPlusDetails.begin(), triggeredPlusDetails.end())) {
                    if (detail->trigger() == r.first()->trigger()) {
                        s << detail;
                    }
                }
                std::stable_sort(s.begin(), s.end(), [](const QSharedPointer<TriggerDetail> &a1, const QSharedPointer<TriggerDetail> &a2) {
                    return a1->triggered() && !a2->triggered();
                });
                // because these are of one single skill, so we can pick the invoke list using a trick like this
                s.append(r);
                r = s.mid(0, r.length());

                // TODO: copy tag?
            }

            details << r;
        }
    }

    // do a stable sort to details use the operator < of SkillInvokeDetail in which judge the priority, the seat of invoker, and whether it is a skill of an equip.
    std::stable_sort(details.begin(), details.end(), [](const QSharedPointer<TriggerDetail> &a1, const QSharedPointer<TriggerDetail> &a2) {
        return *a1 < *a2;
    });

    // mark the skills which missed the trigger timing as it has triggered
    QSharedPointer<TriggerDetail> over_trigger = (triggered.isEmpty() ? QSharedPointer<TriggerDetail>() : triggered.last());

    QListIterator<QSharedPointer<TriggerDetail>> it(details);
    it.toBack();
    while (it.hasPrevious()) {
        // search the last skill which triggered times isn't 0 from back to front. if found, save it to over_trigger.
        // if over_trigger is valid, then mark the skills which missed the trigger timing as it has triggered.
        const QSharedPointer<TriggerDetail> &detail = it.previous();
        if (over_trigger.isNull() || !over_trigger->isValid()) {
            if (detail->triggered())
                over_trigger = detail;
        } else if (*detail < *over_trigger)
            detail->setTriggered(true);
    }

    detailsList = details;
}

bool RoomThread::trigger(QSanguosha::TriggerEvent e, QVariant &data)
{
    // find all the skills, do the record first. it do the things only for record. it should not and must not interfere the procedure of other skills.
    QList<const Trigger *> triggerList = m_triggerList[e];
    foreach (const Trigger *trigger, triggerList)
        trigger->record(e, room, data);

    QList<QSharedPointer<TriggerDetail>> details;
    QList<QSharedPointer<TriggerDetail>> triggered;
    bool interrupt = false;
    forever {
        getTriggerAndSort(e, details, triggered, data);

        QList<QSharedPointer<TriggerDetail>> sameTiming;
        // search for the first skills which can trigger
        foreach (QSharedPointer<TriggerDetail> ptr, details) {
            if (ptr->triggered())
                continue;
            bool sameTimingCompulsory = false;
            if (sameTiming.isEmpty())
                sameTiming << ptr;
            else if (ptr->sameTimingWith(*sameTiming.first())) {
                if (!ptr->isCompulsory() && !ptr->effectOnly())
                    sameTiming << ptr;
                else {
                    // For Compulsory Skill / Effect Only at the same timing, just add the first one into sameTiming. etc. like QinggangSword
                    if (!sameTimingCompulsory) {
                        foreach (QSharedPointer<TriggerDetail> detail, sameTiming) {
                            if (detail->trigger() == ptr->trigger()) {
                                sameTimingCompulsory = true;
                                break;
                            }
                        }
                        if (!sameTimingCompulsory)
                            sameTiming << ptr;
                    }
                }
            }
        }

        // if not found, it means that all the triggers is triggered, we can exit the loop now.
        if (sameTiming.isEmpty())
            break;

        QSharedPointer<TriggerDetail> invoke = sameTiming.first();
        // Treat the trigger is Rule if the invoker is nullptr
        // if the priority is bigger than 5 or smaller than -5, that means it could be some kind of record skill,
        //    notify-client skill or fakemove skill, then no need to select the trigger order at this time
        if (sameTiming.length() >= 2 && invoke->invoker() != nullptr && (invoke->trigger()->priority() >= -5 && invoke->trigger()->priority() <= 5)) {
            // select the triggerorder of same timing
            // if there is a compulsory skill or compulsory effect, it shouldn't be able to cancel
            bool has_compulsory = false;
            foreach (QSharedPointer<TriggerDetail> detail, sameTiming) {
                if (detail->effectOnly()) {
                    has_compulsory = true;
                    break;
                }

                if (detail->isCompulsory()) {
                    if (detail->owner() == nullptr) {
                        has_compulsory = true;
                        break;
                    } else if (detail->owner()->haveShownSkill(detail->name())) {
                        has_compulsory = true;
                        break;
                    }
                }
            }

            // since the invoker of the sametiming list is the same, we can use sameTiming.first()->invoker to judge the invoker of this time
            QSharedPointer<TriggerDetail> detailSelected = room->askForTriggerOrder(qobject_cast<ServerPlayer *>(sameTiming.first()->invoker()), sameTiming, !has_compulsory, data);

            if (detailSelected.isNull() || !detailSelected->isValid()) {
                // if cancel is pushed when it is cancelable, we set all the sametiming as triggered, and add all the skills to triggeredList, continue the next loop
                foreach (QSharedPointer<TriggerDetail> ptr, sameTiming) {
                    ptr->setTriggered(true);
                    triggered << ptr;
                }
                continue;
            } else
                invoke = detailSelected;
        }

        // if not cancelled, then we add the selected skill to triggeredList, and add the triggered times of the skill. then we process with the skill's cost and effect.
        invoke->setTriggered(true);
        triggered << invoke;

        if (invoke->trigger()->trigger(e, room, *invoke, data)) {
            interrupt = true;
            break;
        }
    }
    return interrupt;
}

void RoomThread::addTrigger(const Trigger *skill)
{
    if (skill == nullptr)
        return;

    QSanguosha::TriggerEvents events = skill->triggerEvents();
    if (events.contains(QSanguosha::NumOfEvents)) {
        for (int i = QSanguosha::NonTrigger + 1; i < QSanguosha::NumOfEvents; ++i)
            m_triggerList[static_cast<QSanguosha::TriggerEvent>(i)] << skill;
    } else {
        foreach (QSanguosha::TriggerEvent triggerEvent, events) {
            QList<const Trigger *> &table = m_triggerList[triggerEvent];
            table << skill;
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
