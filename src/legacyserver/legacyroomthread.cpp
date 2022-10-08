#include "legacyroomthread.h"
#include "engine.h"
#include "general.h"
#include "jsonutils.h"
#include "legacygamerule.h"
#include "legacyroom.h"
#include "legacyutil.h"
#include "mode.h"
#include "package.h"
#include "protocol.h"
#include "serverinfostruct.h"
#include "settings.h"
#include "skill.h"
#include "util.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QTime>
#include <functional>

using namespace QSgsJsonUtils;
using namespace QSanProtocol;

RoomThread::RoomThread(LegacyRoom *room)
    : room(room)
    , game_rule(nullptr)
    , nextExtraTurn(nullptr)
    , extraTurnReturn(nullptr)
{
}

void RoomThread::addPlayerSkills(LegacyServerPlayer *player, bool /*unused*/)
{
    foreach (const Skill *skill, player->skills(true, true)) {
        foreach (const Trigger *trigger, skill->triggers())
            addTrigger(trigger);
    }
}

void RoomThread::constructTriggerTable()
{
    foreach (LegacyServerPlayer *player, room->serverPlayers())
        addPlayerSkills(player, true);
}

LegacyServerPlayer *RoomThread::find3v3Next(QList<LegacyServerPlayer *> &first, QList<LegacyServerPlayer *> &second)
{
    bool all_actioned = true;
    foreach (LegacyServerPlayer *player, room->getAllPlayers()) {
        if (!player->hasFlag(QStringLiteral("actioned"))) {
            all_actioned = false;
            break;
        }
    }

    if (all_actioned) {
        foreach (LegacyServerPlayer *player, room->getAllPlayers()) {
            room->setPlayerFlag(player, QStringLiteral("-actioned"));
            QVariant v = QVariant::fromValue(player);
            trigger(QSanguosha::ActionedReset, v);
        }

        qSwap(first, second);
        QList<LegacyServerPlayer *> first_alive;
        foreach (LegacyServerPlayer *p, first) {
            if (p->isAlive())
                first_alive << p;
        }
        return room->askForPlayerChosen(first.first(), first_alive, QStringLiteral("3v3-action"), QStringLiteral("@3v3-action"));
    }

    LegacyServerPlayer *current = RefactorProposal::fixme_cast<LegacyServerPlayer *>(room->currentRound());
    if (current != first.first()) {
        LegacyServerPlayer *another = nullptr;
        if (current == first.last())
            another = first.at(1);
        else
            another = first.last();
        if (!another->hasFlag(QStringLiteral("actioned")) && another->isAlive())
            return another;
    }

    QList<LegacyServerPlayer *> targets;
    do {
        targets.clear();
        qSwap(first, second);
        foreach (LegacyServerPlayer *player, first) {
            if (!player->hasFlag(QStringLiteral("actioned")) && player->isAlive())
                targets << player;
        }
    } while (targets.isEmpty());

    return room->askForPlayerChosen(first.first(), targets, QStringLiteral("3v3-action"), QStringLiteral("@3v3-action"));
}

void RoomThread::run3v3(QList<LegacyServerPlayer *> &first, QList<LegacyServerPlayer *> &second, LegacyGameRule *game_rule, LegacyServerPlayer *current)
{
    try {
        forever {
            room->setCurrentRound(current);
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

void RoomThread::_handleTurnBroken3v3(QList<LegacyServerPlayer *> &first, QList<LegacyServerPlayer *> &second, LegacyGameRule *game_rule)
{
    try {
        LegacyServerPlayer *player = RefactorProposal::fixme_cast<LegacyServerPlayer *>(room->currentRound());
        QVariant v = QVariant::fromValue(player);
        trigger(QSanguosha::TurnBroken, v);
        if (player->phase() != QSanguosha::PhaseNotActive) {
            QVariant data = QVariant::fromValue(player);
            game_rule->trigger(QSanguosha::EventPhaseEnd, room, TriggerDetail(room), data);
            player->changePhase(player->phase(), QSanguosha::PhaseNotActive);
        }
        if (!player->hasFlag(QStringLiteral("actioned")))
            room->setPlayerFlag(player, QStringLiteral("actioned"));

        LegacyServerPlayer *next = find3v3Next(first, second);
        run3v3(first, second, game_rule, next);
    } catch (QSanguosha::TriggerEvent triggerEvent) {
        if (triggerEvent == QSanguosha::TurnBroken) {
            _handleTurnBroken3v3(first, second, game_rule);
        } else {
            throw triggerEvent;
        }
    }
}

LegacyServerPlayer *RoomThread::findHulaoPassNext(LegacyServerPlayer * /*unused*/, const QList<LegacyServerPlayer *> & /*unused*/)
{
    LegacyServerPlayer *current = RefactorProposal::fixme_cast<LegacyServerPlayer *>(room->currentRound());
    return qobject_cast<LegacyServerPlayer *>(current->findNextAlive(1, false));
}

void RoomThread::actionHulaoPass(LegacyServerPlayer *uuz, QList<LegacyServerPlayer *> league, LegacyGameRule *game_rule)
{
    try {
        forever {
            LegacyServerPlayer *current = RefactorProposal::fixme_cast<LegacyServerPlayer *>(room->currentRound());
            QVariant v = QVariant::fromValue(current);
            trigger(QSanguosha::TurnStart, v);

            LegacyServerPlayer *next = findHulaoPassNext(uuz, league);

            if (current == uuz) {
                foreach (LegacyServerPlayer *player, league) {
                    if (player->isDead()) {
                        QVariant v = QVariant::fromValue(player);
                        trigger(QSanguosha::TurnStart, v);
                    }
                }
            }
            room->setCurrentRound(next);
        }
    } catch (QSanguosha::TriggerEvent triggerEvent) {
        if (triggerEvent == QSanguosha::TurnBroken)
            _handleTurnBrokenHulaoPass(uuz, league, game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::_handleTurnBrokenHulaoPass(LegacyServerPlayer *uuz, const QList<LegacyServerPlayer *> &league, LegacyGameRule *game_rule)
{
    try {
        LegacyServerPlayer *player = RefactorProposal::fixme_cast<LegacyServerPlayer *>(room->currentRound());
        QVariant v = QVariant::fromValue(player);
        trigger(QSanguosha::TurnBroken, v);
        LegacyServerPlayer *next = findHulaoPassNext(uuz, league);
        if (player->phase() != QSanguosha::PhaseNotActive) {
            QVariant data = QVariant::fromValue(player);
            game_rule->trigger(QSanguosha::EventPhaseEnd, room, TriggerDetail(room), data);
            player->changePhase(player->phase(), QSanguosha::PhaseNotActive);
        }

        room->setCurrentRound(next);
        actionHulaoPass(uuz, league, game_rule);
    } catch (QSanguosha::TriggerEvent triggerEvent) {
        if (triggerEvent == QSanguosha::TurnBroken)
            _handleTurnBrokenHulaoPass(uuz, league, game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::actionNormal(LegacyGameRule *game_rule)
{
    try {
        forever {
            LegacyServerPlayer *current = RefactorProposal::fixme_cast<LegacyServerPlayer *>(room->currentRound());
            QVariant data = QVariant::fromValue(current);
            trigger(QSanguosha::TurnStart, data);
            if (room->isFinished())
                break;

            while (nextExtraTurn != nullptr) {
                extraTurnReturn = current;
                room->setCurrentRound(nextExtraTurn);
                LegacyServerPlayer *nextExtraTurnCopy = nextExtraTurn;
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

            room->setCurrentRound(qobject_cast<LegacyServerPlayer *>(current->findNextAlive(1, false)));
        }
    } catch (QSanguosha::TriggerEvent triggerEvent) {
        if (triggerEvent == QSanguosha::TurnBroken)
            _handleTurnBrokenNormal(game_rule);
        else
            throw triggerEvent;
    }
}

void RoomThread::_handleTurnBrokenNormal(LegacyGameRule *game_rule)
{
    try {
        LegacyServerPlayer *player = RefactorProposal::fixme_cast<LegacyServerPlayer *>(room->currentRound());
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
            room->setCurrentRound(nextExtraTurn);
            LegacyServerPlayer *nextExtraTurnCopy = nextExtraTurn;
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

        LegacyServerPlayer *next = qobject_cast<LegacyServerPlayer *>(player->findNextAlive(1, false));
        room->setCurrentRound(next);
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
    foreach (LegacyServerPlayer *player, room->serverPlayers()) {
        //Ensure that the game starts with all player's mutex locked
        player->drainAllLocks();
        player->releaseLock(LegacyServerPlayer::SEMA_MUTEX);
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
        room->doBroadcastNotify(S_COMMAND_START_IN_X_SECONDS, QJsonValue(0));

    if (room->serverInfo()->GameMode->name() == QStringLiteral("04_1v3")) {
        LegacyServerPlayer *lord = room->serverPlayers().first();
        room->setPlayerProperty(lord, "general", QStringLiteral("yuyuko_1v3"));

        QList<const General *> generals = QList<const General *>();
        foreach (QString pack_name, Sanguosha->configuration(QStringLiteral("hulao_packages")).toStringList()) {
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

        foreach (LegacyServerPlayer *player, room->serverPlayers()) {
            if (player == lord)
                continue;

            qShuffle(names);
            QStringList choices = names.mid(0, 3);
            QString name = room->askForGeneral(player, choices);

            room->setPlayerProperty(player, "general", name);
            names.removeOne(name);
        }

        room->startGame();
    } else if (room->serverInfo()->GameMode->category() == QSanguosha::ModeHegemony) {
        room->chooseHegemonyGenerals();
        room->startGame();
    } else {
        room->chooseGenerals();
        room->startGame();
    }

    game_rule = new LegacyGameRule;

    addTrigger(game_rule);
    //    foreach (const TriggerSkill *triggerSkill, Sanguosha->getGlobalTriggerSkills())
    //        addTriggerSkill(triggerSkill);

    // start game
    try {
        QString order;
        QList<LegacyServerPlayer *> warm;
        QList<LegacyServerPlayer *> cool;
        QList<LegacyServerPlayer *> first;
        QList<LegacyServerPlayer *> second;
        if (room->serverInfo()->GameMode->name() == QStringLiteral("06_3v3")) {
            foreach (LegacyServerPlayer *player, room->serverPlayers()) {
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
        if (room->serverInfo()->GameMode->name() == QStringLiteral("06_3v3")) {
            run3v3(first, second, game_rule, first.first());
        } else if (room->serverInfo()->GameMode->name() == QStringLiteral("04_1v3")) {
            LegacyServerPlayer *uuz = room->getLord();
            QList<LegacyServerPlayer *> league = room->serverPlayers();
            league.removeOne(uuz);

            room->setCurrentRound(league.first());
            actionHulaoPass(uuz, league, game_rule);
        } else {
            if (room->serverInfo()->GameMode->name() == QStringLiteral("02_1v1")) {
                LegacyServerPlayer *first = room->serverPlayers().first();
                if (first->roleString() != QStringLiteral("renegade"))
                    first = room->serverPlayers().at(1);
                LegacyServerPlayer *second = room->getOtherPlayers(first).first();
                QVariant v1 = QVariant::fromValue(first);
                trigger(QSanguosha::Debut, v1);
                QVariant v2 = QVariant::fromValue(second);
                trigger(QSanguosha::Debut, v2);
                room->setCurrentRound(first);
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
            QSharedPointer<TriggerDetail> detailSelected
                = room->askForTriggerOrder(qobject_cast<LegacyServerPlayer *>(sameTiming.first()->invoker()), sameTiming, !has_compulsory, data);

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
