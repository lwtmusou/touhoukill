#include "legacyroom.h"
#include "CardFace.h"
#include "audio.h"
#include "card.h"
#include "engine.h"
#include "general.h"
#include "jsonutils.h"
#include "legacygamerule.h"
#include "legacyjson.h"
#include "legacyserver.h"
#include "legacyutil.h"
#include "lua-wrapper.h"
#include "mode.h"
#include "settings.h"
#include "skill.h"
#include "structs.h"
#include "util.h"

#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <QMetaEnum>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QTimerEvent>
#include <ctime>

using namespace QSanProtocol;

LegacyRoom::LegacyRoom(QObject *parent, const ServerInfoStruct *si)
    : RoomObject(parent)
    , _m_lastMovementId(0)
    , current(nullptr)
    , m_drawPile(&pile1)
    , game_started(false)
    , game_started2(false)
    , game_finished(false)
    , game_paused(false)
    , fill_robot(false)
    , thread(nullptr)
    , _m_semRaceRequest(0)
    , _m_semRoomMutex(1)
    , _m_raceStarted(false)
    , provided(nullptr)
    , has_provided(false)
    , provider(nullptr)
    , m_fillAGWho(nullptr)
{
    *(serverInfo()) = *si;

    player_count = si->GameMode->playersCount();
    pile1 = si->GameMode->availableCards().values();
    qShuffle(pile1);

    initCallbacks();
}

void LegacyRoom::initCallbacks()
{
    // init request response pair
    m_requestResponsePair[S_COMMAND_PLAY_CARD] = S_COMMAND_RESPONSE_CARD;
    m_requestResponsePair[S_COMMAND_NULLIFICATION] = S_COMMAND_RESPONSE_CARD;
    m_requestResponsePair[S_COMMAND_SHOW_CARD] = S_COMMAND_RESPONSE_CARD;
    m_requestResponsePair[S_COMMAND_ASK_PEACH] = S_COMMAND_RESPONSE_CARD;
    m_requestResponsePair[S_COMMAND_PINDIAN] = S_COMMAND_RESPONSE_CARD;
    m_requestResponsePair[S_COMMAND_EXCHANGE_CARD] = S_COMMAND_DISCARD_CARD;
    m_requestResponsePair[S_COMMAND_CHOOSE_DIRECTION] = S_COMMAND_MULTIPLE_CHOICE;
    m_requestResponsePair[S_COMMAND_LUCK_CARD] = S_COMMAND_INVOKE_SKILL;

    // Client notifications
    m_callbacks[S_COMMAND_TOGGLE_READY] = &LegacyRoom::toggleReadyCommand;
    m_callbacks[S_COMMAND_ADD_ROBOT] = &LegacyRoom::addRobotCommand;
    m_callbacks[S_COMMAND_FILL_ROBOTS] = &LegacyRoom::fillRobotsCommand;

    m_callbacks[S_COMMAND_SPEAK] = &LegacyRoom::speakCommand;
    m_callbacks[S_COMMAND_TRUST] = &LegacyRoom::trustCommand;
    m_callbacks[S_COMMAND_PAUSE] = &LegacyRoom::pauseCommand;
    m_callbacks[S_COMMAND_SKIN_CHANGE] = &LegacyRoom::skinChangeCommand;
    m_callbacks[S_COMMAND_HEARTBEAT] = &LegacyRoom::heartbeatCommand;

    //Client request
    m_callbacks[S_COMMAND_NETWORK_DELAY_TEST] = &LegacyRoom::networkDelayTestCommand;
    m_callbacks[S_COMMAND_PRESHOW] = &LegacyRoom::processRequestPreshow;
}

bool LegacyRoom::notifyUpdateCard(LegacyServerPlayer *player, int cardId, const Card *newCard)
{
    // TODO： Duplicated faceName here.
    QJsonArray val;
    Q_ASSERT(newCard);
    QString className = newCard->faceName();
    val << cardId << newCard->suit() << newCard->numberString() << className << newCard->skillName() << newCard->faceName()
        << QSgsJsonUtils::toJsonArray(newCard->flags().values());
    doNotify(player, S_COMMAND_UPDATE_CARD, val);
    return true;
}

bool LegacyRoom::broadcastUpdateCard(const QList<LegacyServerPlayer *> &players, int cardId, const Card *newCard)
{
    foreach (LegacyServerPlayer *player, players)
        notifyUpdateCard(player, cardId, newCard);
    return true;
}

bool LegacyRoom::notifyResetCard(LegacyServerPlayer *player, int cardId)
{
    doNotify(player, S_COMMAND_UPDATE_CARD, cardId);
    return true;
}

bool LegacyRoom::broadcastResetCard(const QList<LegacyServerPlayer *> &players, int cardId)
{
    foreach (LegacyServerPlayer *player, players)
        notifyResetCard(player, cardId);
    return true;
}

QList<LegacyServerPlayer *> LegacyRoom::serverPlayers() const
{
    return m_players;
}

QList<LegacyServerPlayer *> LegacyRoom::getAllPlayers(bool include_dead) const
{
    QList<LegacyServerPlayer *> count_players = m_players;
    if (current == nullptr)
        return count_players;

    LegacyServerPlayer *starter = current;
    if (current->phase() == QSanguosha::PhaseNotActive)
        starter = qobject_cast<LegacyServerPlayer *>(current->getNextAlive(1, false));
    int index = count_players.indexOf(starter);
    if (index == -1)
        return count_players;

    QList<LegacyServerPlayer *> all_players;
    for (int i = index; i < count_players.length(); i++) {
        if (include_dead || count_players[i]->isAlive())
            all_players << count_players[i];
    }

    for (int i = 0; i < index; i++) {
        if (include_dead || count_players[i]->isAlive())
            all_players << count_players[i];
    }

    return all_players;
}

QList<LegacyServerPlayer *> LegacyRoom::getOtherPlayers(LegacyServerPlayer *except, bool include_dead) const
{
    QList<LegacyServerPlayer *> other_players = getAllPlayers(include_dead);
    if ((except != nullptr) && (except->isAlive() || include_dead))
        other_players.removeOne(except);
    return other_players;
}

void LegacyRoom::enterDying(LegacyServerPlayer *player, DamageStruct *reason)
{
    if (player->dyingFactor() > player->maxHp()) {
        killPlayer(player, reason);
        return;
    }

    setPlayerFlag(player, QStringLiteral("Global_Dying"));
    QStringList currentdying = getTag(QStringLiteral("CurrentDying")).toStringList();
    currentdying << player->objectName();
    setTag(QStringLiteral("CurrentDying"), QVariant::fromValue(currentdying));

    QJsonArray arg;
    arg << QSanProtocol::S_GAME_EVENT_PLAYER_DYING << player->objectName();
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

    DeathStruct dying(player, reason);
    QVariant dying_data = QVariant::fromValue(dying);

    bool enterdying = thread->trigger(QSanguosha::EnterDying, dying_data);

    if (!(player->isDead() || player->hp() >= player->dyingFactor() || enterdying)) {
        thread->trigger(QSanguosha::Dying, dying_data);
        if (player->isAlive()) {
            if (player->hp() >= player->dyingFactor()) {
                setPlayerFlag(player, QStringLiteral("-Global_Dying"));
            } else {
                LogStruct log;
                log.type = QStringLiteral("#AskForPeaches");
                log.from = player;
                // log.to = getAllPlayers();
                for (LegacyServerPlayer *p : getAllPlayers())
                    log.to << p;
                log.arg = QString::number(player->dyingFactor() - player->hp());
                sendLog(log);

                foreach (LegacyServerPlayer *saver, getAllPlayers()) {
                    DeathStruct dying = dying_data.value<DeathStruct>();
                    dying.nowAskingForPeaches = saver;
                    dying_data = QVariant::fromValue(dying);
                    if (player->hp() >= player->dyingFactor() || player->isDead())
                        break;
                    QString cd = saver->property("currentdying").toString();
                    setPlayerProperty(saver, "currentdying", player->objectName());
                    saver->tag[QStringLiteral("songzang_dying")] = dying_data; //record for ai, like skill songzang

                    thread->trigger(QSanguosha::AskForPeaches, dying_data);
                    setPlayerProperty(saver, "currentdying", cd);
                }
                DeathStruct dying = dying_data.value<DeathStruct>();
                dying.nowAskingForPeaches = nullptr;
                dying_data = QVariant::fromValue(dying);
                thread->trigger(QSanguosha::AskForPeachesDone, dying_data);

                setPlayerFlag(player, QStringLiteral("-Global_Dying"));
            }
        }

    } else {
        setPlayerFlag(player, QStringLiteral("-Global_Dying"));
    }

    currentdying = getTag(QStringLiteral("CurrentDying")).toStringList();
    currentdying.removeOne(player->objectName());
    setTag(QStringLiteral("CurrentDying"), QVariant::fromValue(currentdying));

    if (player->isAlive()) {
        QJsonArray arg;
        arg << QSanProtocol::S_GAME_EVENT_PLAYER_QUITDYING << player->objectName();
        doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
    }
    thread->trigger(QSanguosha::QuitDying, dying_data);
}

void LegacyRoom::revivePlayer(LegacyServerPlayer *player, bool initialize)
{
    player->setAlive(true);
    player->throwAllMarks(false);
    broadcastProperty(player, "alive");

    if (initialize) {
        setPlayerProperty(player, "maxhp", player->general()->maxHp());
        setPlayerProperty(player, "hp", player->maxHp());
        setEmotion(player, QStringLiteral("revive"));
        touhouLogmessage(QStringLiteral("#Revive"), player);

        foreach (const Skill *skill, player->skills(false)) {
            if (skill->isLimited() && !skill->limitMark().isEmpty() && (!skill->isLordSkill() || player->hasValidLordSkill(skill->name())))
                setPlayerMark(player, skill->limitMark(), 1);
        }

        player->drawCards(4);
        if (player->isChained()) {
            player->setChained(false);
            broadcastProperty(player, "chained");
        }
        if (!player->faceUp()) {
            player->setFaceUp(true);
            broadcastProperty(player, "faceup");
        }
        setPlayerProperty(player, "role_shown", player->isLord());

        QJsonArray args;
        args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

        if (player->phase() == QSanguosha::PhasePlay)
            setPlayerFlag(player, QStringLiteral("Global_PlayPhaseTerminated"));
        if (player->isCurrent())
            setPlayerFlag(player, QStringLiteral("Global_TurnTerminated"));
    }

    doBroadcastNotify(S_COMMAND_REVIVE_PLAYER, player->objectName());
    updateStateItem();

    QVariant v = QVariant::fromValue(player);
    thread->trigger(QSanguosha::Revive, v);
}

void LegacyRoom::updateStateItem()
{
    QList<LegacyServerPlayer *> players = m_players;
    std::sort(players.begin(), players.end(), [](LegacyServerPlayer *player1, LegacyServerPlayer *player2) -> bool {
        int role1 = player1->role();
        int role2 = player2->role();

        if (role1 != role2)
            return role1 < role2;
        else
            return player1->isAlive();
    });
    QString roles;
    foreach (LegacyServerPlayer *p, players) {
        QChar c = QLatin1Char("ZCFN"[p->role()]);
        if (p->isDead())
            c = c.toLower();

        roles.append(c);
    }

    doBroadcastNotify(S_COMMAND_UPDATE_STATE_ITEM, roles);
}

void LegacyRoom::killPlayer(LegacyServerPlayer *victim, DamageStruct *reason)
{
    LegacyServerPlayer *killer = reason != nullptr ? qobject_cast<LegacyServerPlayer *>(reason->from) : nullptr;

    victim->setAlive(false);

    DeathStruct death(victim, reason);
    QVariant data = QVariant::fromValue(death);
    thread->trigger(QSanguosha::BeforeGameOverJudge, data);

    updateStateItem();

    LogStruct log;
    log.type = killer != nullptr ? (killer == victim ? QStringLiteral("#Suicide") : QStringLiteral("#Murder")) : QStringLiteral("#Contingency");
    log.to << victim;
    log.arg = victim->roleString();
    log.from = killer;
    sendLog(log);

    broadcastProperty(victim, "alive");
    broadcastProperty(victim, "role");
    if (serverInfo()->GameMode->category() == QSanguosha::ModeRole)
        setPlayerProperty(victim, "role_shown", true);

    doBroadcastNotify(S_COMMAND_KILL_PLAYER, victim->objectName());

    thread->trigger(QSanguosha::GameOverJudge, data);

    thread->trigger(QSanguosha::Death, data);

    doNotify(victim, S_COMMAND_SET_DASHBOARD_SHADOW, victim->objectName());

    victim->detachAllSkills();
    thread->trigger(QSanguosha::BuryVictim, data);

    // TODO_Fs: Consider refactoring these logic
#if 0
    if (!victim->isAlive() && Config.EnableAI && !victim->hasSkill("huanhun")) {
        bool expose_roles = true;
        foreach (ServerPlayer *player, m_alivePlayers) {
            if (!player->isOffline()) {
                expose_roles = false;
                break;
            }
        }

        if (expose_roles) {
            foreach (ServerPlayer *player, m_alivePlayers) {
                broadcastProperty(player, "role");
                setPlayerProperty(victim, "role_shown", true);
            }

            static QStringList continue_list;
            if (continue_list.isEmpty())
                continue_list << "02_1v1"
                              << "04_1v3"
                              << "06_XMode";
            if (continue_list.contains(Config.GameMode))
                return;

            if (Config.AlterAIDelayAD)
                Config.AIDelay = Config.AIDelayAD;
            if (victim->isOnline() && Config.SurrenderAtDeath && mode != "02_1v1" && mode != "06_XMode" && askForSkillInvoke(victim, "surrender", "yes"))
                makeSurrender(victim);
        }
    }
#endif
}

void LegacyRoom::judge(JudgeStruct &judge_struct)
{
    Q_ASSERT(judge_struct.who != nullptr);

    JudgeStruct *judge_star = &judge_struct;
    QVariant data = QVariant::fromValue(judge_star);
    thread->trigger(QSanguosha::StartJudge, data);
    thread->trigger(QSanguosha::AskForRetrial, data);
    thread->trigger(QSanguosha::FinishRetrial, data);
    thread->trigger(QSanguosha::FinishJudge, data);
}

void LegacyRoom::sendJudgeResult(const JudgeStruct *judge)
{
    QJsonArray arg;
    arg << QSanProtocol::S_GAME_EVENT_JUDGE_RESULT << judge->card()->effectiveId() << judge->isEffected() << judge->who->objectName() << judge->reason;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
}

QList<int> LegacyRoom::getNCards(int n, bool update_pile_number, bool bottom)
{
    QList<int> card_ids;
    for (int i = 0; i < n; i++)
        card_ids << drawCard(bottom);

    if (update_pile_number)
        doBroadcastNotify(S_COMMAND_UPDATE_PILE, m_drawPile->length());

    return card_ids;
}

void LegacyRoom::returnToTopDrawPile(const QList<int> &cards)
{
    QListIterator<int> i(cards);
    i.toBack();
    while (i.hasPrevious()) {
        int id = i.previous();
        setCardMapping(id, nullptr, QSanguosha::PlaceDrawPile);
        m_drawPile->prepend(id);
    }
    doBroadcastNotify(S_COMMAND_UPDATE_PILE, m_drawPile->length());
}

QStringList LegacyRoom::aliveRoles(LegacyServerPlayer *except) const
{
    QStringList roles;
    foreach (LegacyServerPlayer *player, getAllPlayers()) {
        if (player != except)
            roles << player->roleString();
    }

    return roles;
}

void LegacyRoom::gameOver(const QString &winner, bool isSurrender)
{
    QStringList all_roles;
    foreach (LegacyServerPlayer *player, m_players) {
        all_roles << player->roleString();
        if (player->handCardNum() > 0) {
            QStringList handcards;
            foreach (const Card *card, player->handCards())
                handcards << Sanguosha->cardDescriptor(card->id()).logName();
            QString handcard = QString::fromUtf8(handcards.join(QStringLiteral(", ")).toUtf8().toBase64());
            setPlayerProperty(player, "last_handcards", handcard);
        }
    }

    game_finished = true;
    // saveWinnerTable(winner, isSurrender);

    emit game_over(winner);

    Config.AIDelay = Config.OriginAIDelay;

    if (!getTag(QStringLiteral("NextGameMode")).toString().isNull()) {
        QString name = getTag(QStringLiteral("NextGameMode")).toString();
        Config.GameMode = name;
        Config.setValue(QStringLiteral("GameMode"), name);
        removeTag(QStringLiteral("NextGameMode"));
    }
    if (!getTag(QStringLiteral("NextGameSecondGeneral")).isNull()) {
        bool enable = getTag(QStringLiteral("NextGameSecondGeneral")).toBool();
        Config.Enable2ndGeneral = enable;
        Config.setValue(QStringLiteral("Enable2ndGeneral"), enable);
        removeTag(QStringLiteral("NextGameSecondGeneral"));
    }

    QJsonArray arg;
    arg << winner << QSgsJsonUtils::toJsonArray(all_roles);
    doBroadcastNotify(S_COMMAND_GAME_OVER, arg);
    throw QSanguosha::GameFinished;
}

void LegacyRoom::slashEffect(const SlashEffectStruct &effect)
{
    QVariant data = QVariant::fromValue(effect);
    if (thread->trigger(QSanguosha::SlashEffected, data)) {
        if (!effect.to->hasFlag(QStringLiteral("Global_NonSkillNullify"))) {
        } else
            effect.to->setFlag(QStringLiteral("-Global_NonSkillNullify"));
        if (effect.slash != nullptr)
            QinggangSword::removeQinggangTag(effect.to, effect.slash);
    }
}

void LegacyRoom::slashResult(const SlashEffectStruct &effect, const Card *jink)
{
    SlashEffectStruct result_effect = effect;
    result_effect.jink = jink;
    QVariant data = QVariant::fromValue(result_effect);

    if (jink == nullptr && !effect.canceled) {
        if (effect.to->isAlive())
            thread->trigger(QSanguosha::SlashHit, data);
    } else {
        if (effect.to->isAlive() && jink != nullptr) {
            if (jink->skillName() != QStringLiteral("eight_diagram"))
                setEmotion(qobject_cast<LegacyServerPlayer *>(effect.to), QStringLiteral("jink"));
        }
        if (effect.slash != nullptr)
            QinggangSword::removeQinggangTag(effect.to, effect.slash);
        thread->trigger(QSanguosha::SlashMissed, data);
    }
}

void LegacyRoom::attachSkillToPlayer(LegacyServerPlayer *player, const QString &skill_name, bool is_other_attach)
{
    loadSkill(Sanguosha->skill(skill_name));
    player->acquireSkill(skill_name);
    if (is_other_attach) {
        QStringList attach_skills = getTag(QStringLiteral("OtherAttachSkills")).toStringList();
        if (!attach_skills.contains(skill_name)) {
            attach_skills << skill_name;
            setTag(QStringLiteral("OtherAttachSkills"), QVariant::fromValue(attach_skills));
        }
    }
    doNotify(player, S_COMMAND_ATTACH_SKILL, skill_name);
}

void LegacyRoom::detachSkillFromPlayer(LegacyServerPlayer *player, const QString &skill_name, bool is_equip, bool acquire_only, bool sendlog, bool head)
{
    if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony && !player->hasValidSkill(skill_name, true))
        return;
    if ((Sanguosha->skill(skill_name) != nullptr) && Sanguosha->skill(skill_name)->isEternal())
        return;
    if (player->acquiredSkills().contains(skill_name))
        player->detachSkill(skill_name);
    else if (!acquire_only)
        player->loseSkill(skill_name, head);
    else
        return;

    const Skill *skill = Sanguosha->skill(skill_name);
    if ((skill != nullptr) && skill->isVisible()) {
        QJsonArray args;
        args << QSanProtocol::S_GAME_EVENT_DETACH_SKILL << player->objectName() << skill_name << head; //default head?
        doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

        if (!is_equip) {
            if (sendlog) {
                LogStruct log;
                log.type = QStringLiteral("#LoseSkill");
                log.from = player;
                log.arg = skill_name;
                sendLog(log);
            }

            SkillAcquireLoseStruct s;
            s.player = player;
            s.skill = skill;
            s.isAcquire = false;
            QVariant data = QVariant::fromValue(s);
            thread->trigger(QSanguosha::EventLoseSkill, data);
        }

        foreach (const Skill *rskill, skill->affiliatedSkills()) {
            if (rskill->isVisible())
                detachSkillFromPlayer(player, rskill->name(), is_equip, acquire_only, sendlog, head);
        }
    }
}

void LegacyRoom::handleAcquireDetachSkills(LegacyServerPlayer *player, const QStringList &skill_names, bool acquire_only)
{
    if (skill_names.isEmpty())
        return;
    QList<bool> isAcquire;
    QList<const Skill *> triggerList;

    foreach (QString skill_name, skill_names) {
        if (skill_name.startsWith(QStringLiteral("-"))) {
            QString actual_skill = skill_name.mid(1);
            bool head = true;
            if (actual_skill.endsWith(QStringLiteral("!"))) {
                actual_skill.chop(1);
                head = false;
            }

            if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony) {
                if (!player->hasGeneralCardSkill(actual_skill) && !player->hasValidSkill(actual_skill, true))
                    continue;
            } else {
                if (!player->hasValidSkill(actual_skill, true))
                    continue;
            }

            if ((Sanguosha->skill(actual_skill) != nullptr) && Sanguosha->skill(actual_skill)->isEternal())
                continue;

            if (player->acquiredSkills().contains(actual_skill))
                player->detachSkill(actual_skill);
            else if (!acquire_only)
                player->loseSkill(actual_skill, head);
            else
                continue;
            const Skill *skill = Sanguosha->skill(actual_skill);
            if ((skill != nullptr) && skill->isVisible()) {
                QJsonArray args;
                args << QSanProtocol::S_GAME_EVENT_DETACH_SKILL << player->objectName() << actual_skill << head;
                doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

                LogStruct log;
                log.type = QStringLiteral("#LoseSkill");
                log.from = player;
                log.arg = actual_skill;
                sendLog(log);

                triggerList << skill;
                isAcquire << false;

                foreach (const Skill *rskill, skill->affiliatedSkills()) {
                    if (!rskill->isVisible())
                        detachSkillFromPlayer(player, rskill->name());
                }
            }
        } else {
            bool head = true;
            QString actual_skill = skill_name;
            if (actual_skill.endsWith(QStringLiteral("!"))) {
                actual_skill.chop(1);
                head = false;
            }
            const Skill *skill = Sanguosha->skill(actual_skill);
            if (skill == nullptr)
                continue;
            if (player->acquiredSkills().contains(actual_skill))
                continue;
            loadSkill(skill);
            player->acquireSkill(actual_skill);
            foreach (const Trigger *trigger, skill->triggers())
                thread->addTrigger(trigger);
            if (skill->isLimited() && !skill->limitMark().isEmpty())
                setPlayerMark(player, skill->limitMark(), 1);
            if (skill->isVisible()) {
                QJsonArray args;
                args << QSanProtocol::S_GAME_EVENT_ACQUIRE_SKILL << player->objectName() << actual_skill << head;
                doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

                foreach (const Skill *related_skill, skill->affiliatedSkills()) {
                    if (!related_skill->isVisible())
                        acquireSkill(player, related_skill);
                }

                triggerList << skill;
                isAcquire << true;
            }
        }
    }
    if (!triggerList.isEmpty()) {
        for (int i = 0; i < triggerList.length(); i++) {
            SkillAcquireLoseStruct s;
            s.skill = triggerList.value(i);
            s.player = player;
            s.isAcquire = isAcquire.value(i);
            QVariant data = QVariant::fromValue(s);
            thread->trigger(isAcquire.at(i) ? QSanguosha::EventAcquireSkill : QSanguosha::EventLoseSkill, data);
        }
    }
}

void LegacyRoom::handleAcquireDetachSkills(LegacyServerPlayer *player, const QString &skill_names, bool acquire_only)
{
    handleAcquireDetachSkills(player, skill_names.split(QStringLiteral("|")), acquire_only);
}

bool LegacyRoom::doRequest(LegacyServerPlayer *player, QSanProtocol::CommandType command, const QJsonValue &arg, bool wait)
{
    time_t timeOut = serverInfo()->getCommandTimeout(command, S_SERVER_INSTANCE);
    return doRequest(player, command, arg, timeOut, wait);
}

bool LegacyRoom::doRequest(LegacyServerPlayer *player, QSanProtocol::CommandType command, const QJsonValue &arg, time_t timeOut, bool wait)
{
    Packet packet(PacketDescriptionFlag(S_SRC_ROOM) | S_TYPE_REQUEST | S_DEST_CLIENT, command);
    packet.setMessageBody(arg);
    player->acquireLock(LegacyServerPlayer::SEMA_MUTEX);
    player->m_isClientResponseReady = false;
    player->drainLock(LegacyServerPlayer::SEMA_COMMAND_INTERACTIVE);
    player->setClientReply(QJsonValue());
    player->m_isWaitingReply = true;
    if (m_requestResponsePair.contains(command))
        player->m_expectedReplyCommand = m_requestResponsePair[command];
    else
        player->m_expectedReplyCommand = command;

    player->invoke(&packet);
    player->releaseLock(LegacyServerPlayer::SEMA_MUTEX);
    if (wait)
        return getResult(player, timeOut);
    else
        return true;
}

bool LegacyRoom::doBroadcastRequest(QList<LegacyServerPlayer *> &players, QSanProtocol::CommandType command)
{
    time_t timeOut = serverInfo()->getCommandTimeout(command, S_SERVER_INSTANCE);
    return doBroadcastRequest(players, command, timeOut);
}

bool LegacyRoom::doBroadcastRequest(QList<LegacyServerPlayer *> &players, QSanProtocol::CommandType command, time_t timeOut)
{
    foreach (LegacyServerPlayer *player, players)
        doRequest(player, command, player->m_commandArgs, timeOut, false);

    QElapsedTimer timer;
    time_t remainTime = timeOut;
    timer.start();
    foreach (LegacyServerPlayer *player, players) {
        remainTime = timeOut - timer.elapsed();
        if (remainTime < 0)
            remainTime = 0;
        getResult(player, remainTime);
    }
    return true;
}

LegacyServerPlayer *LegacyRoom::doBroadcastRaceRequest(QList<LegacyServerPlayer *> &players, QSanProtocol::CommandType command, time_t timeOut, ResponseVerifyFunction validateFunc,
                                                       void *funcArg)
{
    _m_semRoomMutex.acquire();
    _m_raceStarted = true;
    _m_raceWinner.storeRelaxed(nullptr);
    while (_m_semRaceRequest.tryAcquire(1)) {
    } //drain lock
    _m_semRoomMutex.release();
    Countdown countdown;
    countdown.max = timeOut;
    countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
    if (command == S_COMMAND_NULLIFICATION)
        notifyMoveFocus(getAllPlayers(), command, countdown);
    else
        notifyMoveFocus(players, command, countdown);
    foreach (LegacyServerPlayer *player, players)
        doRequest(player, command, player->m_commandArgs, timeOut, false);

    LegacyServerPlayer *winner = getRaceResult(players, command, timeOut, validateFunc, funcArg);
    return winner;
}

LegacyServerPlayer *LegacyRoom::getRaceResult(QList<LegacyServerPlayer *> &players, QSanProtocol::CommandType /*unused*/, time_t timeOut, ResponseVerifyFunction validateFunc,
                                              void *funcArg)
{
    QElapsedTimer timer;
    timer.start();
    bool validResult = false;
    for (int i = 0; i < players.size(); i++) {
        time_t timeRemain = timeOut - timer.elapsed();
        if (timeRemain < 0)
            timeRemain = 0;
        bool tryAcquireResult = true;
        if (Config.OperationNoLimit)
            _m_semRaceRequest.acquire();
        else
            tryAcquireResult = _m_semRaceRequest.tryAcquire(1, timeRemain);

        if (!tryAcquireResult)
            _m_semRoomMutex.tryAcquire(1);
        // So that processResponse cannot update raceWinner when we are reading it.

        LegacyServerPlayer *winner = _m_raceWinner.loadAcquire();
        if (winner == nullptr) {
            _m_raceWinner.storeRelease(winner);
            _m_semRoomMutex.release();
            continue;
        }

        // original line is:
        // if (validateFunc == nullptr || (winner->m_isClientResponseReady && (this->*validateFunc)(winner, winner->getClientReply(), funcArg))) {
        // but I prefer m_isClientResponseReady is judged first since disconnection makes m_isClientResponseReady = false
        // processResponse makes m_isClientResponseReady = true
        if (winner->m_isClientResponseReady && (validateFunc == nullptr || (this->*validateFunc)(winner, winner->getClientReply(), funcArg))) {
            validResult = true;
            break;
        } else {
            // Don't give this player any more chance for this race
            winner->m_isWaitingReply = false;
            _m_raceWinner.storeRelease(nullptr);
            _m_semRoomMutex.release();
        }
    }

    if (!validResult)
        _m_semRoomMutex.acquire();
    _m_raceStarted = false;
    foreach (LegacyServerPlayer *player, players) {
        player->acquireLock(LegacyServerPlayer::SEMA_MUTEX);
        player->m_expectedReplyCommand = S_COMMAND_UNKNOWN;
        player->m_isWaitingReply = false;
        player->releaseLock(LegacyServerPlayer::SEMA_MUTEX);
    }
    _m_semRoomMutex.release();
    return _m_raceWinner.fetchAndStoreRelease(nullptr);
}

bool LegacyRoom::doNotify(LegacyServerPlayer *player, QSanProtocol::CommandType command, const QJsonValue &arg)
{
    Packet packet(PacketDescriptionFlag(S_SRC_ROOM) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, command);
    packet.setMessageBody(arg);
    player->invoke(&packet);
    return true;
}

bool LegacyRoom::doBroadcastNotify(const QList<LegacyServerPlayer *> &players, QSanProtocol::CommandType command, const QJsonValue &arg)
{
    foreach (LegacyServerPlayer *player, players)
        doNotify(player, command, arg);
    return true;
}

bool LegacyRoom::doBroadcastNotify(QSanProtocol::CommandType command, const QJsonValue &arg)
{
    return doBroadcastNotify(m_players, command, arg);
}

// the following functions for Lua

bool LegacyRoom::doNotify(LegacyServerPlayer *player, int command, const char *arg)
{
    Packet packet(PacketDescriptionFlag(S_SRC_ROOM) | S_TYPE_NOTIFICATION | S_DEST_CLIENT, (QSanProtocol::CommandType)command);
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(arg, &error);
    if (error.error == QJsonParseError::NoError) {
        QJsonValue v;
        if (doc.isObject())
            v = doc.object();
        else
            v = doc.array();

        packet.setMessageBody(v);
        player->invoke(&packet);
    } else {
        // output(QStringLiteral("Fail to parse the Json Value %1").arg(QString::fromUtf8(arg)));
    }
    return true;
}

bool LegacyRoom::doBroadcastNotify(const QList<LegacyServerPlayer *> &players, int command, const char *arg)
{
    foreach (LegacyServerPlayer *player, players)
        doNotify(player, command, arg);
    return true;
}

bool LegacyRoom::doBroadcastNotify(int command, const char *arg)
{
    return doBroadcastNotify(m_players, command, arg);
}

void LegacyRoom::broadcastInvoke(const char *method, const QString &arg, LegacyServerPlayer *except)
{
    broadcast(QStringLiteral("%1 %2").arg(QString::fromUtf8(method), arg), except);
}

void LegacyRoom::broadcastInvoke(const QSanProtocol::Packet *packet, LegacyServerPlayer *except)
{
    broadcast(packet->toString(), except);
}

bool LegacyRoom::getResult(LegacyServerPlayer *player, time_t timeOut)
{
    Q_ASSERT(player->m_isWaitingReply);
    bool validResult = false;
    player->acquireLock(LegacyServerPlayer::SEMA_MUTEX);
    if (player->getState() == QStringLiteral("online")) {
        player->releaseLock(LegacyServerPlayer::SEMA_MUTEX);

        if (Config.OperationNoLimit)
            player->acquireLock(LegacyServerPlayer::SEMA_COMMAND_INTERACTIVE);
        else
            player->tryAcquireLock(LegacyServerPlayer::SEMA_COMMAND_INTERACTIVE, timeOut);

        // Note that we rely on processResponse to filter out all unrelevant packet.
        // By the time the lock is released, m_clientResponse must be the right message
        // assuming the client side is not tampered.

        // Also note that lock can be released when a player switch to trust or offline status.
        // It is ensured by trustCommand and reportDisconnection that the player reports these status
        // is the player waiting the lock. In these cases, the serial number and command type doesn't matter.
        player->acquireLock(LegacyServerPlayer::SEMA_MUTEX);
        validResult = player->m_isClientResponseReady;
    }
    player->m_expectedReplyCommand = S_COMMAND_UNKNOWN;
    player->m_isWaitingReply = false;
    player->releaseLock(LegacyServerPlayer::SEMA_MUTEX);
    return validResult;
}

bool LegacyRoom::notifyMoveFocus(LegacyServerPlayer *player)
{
    QList<LegacyServerPlayer *> players;
    players.append(player);
    Countdown countdown;
    countdown.type = Countdown::S_COUNTDOWN_NO_LIMIT;
    notifyMoveFocus(players, S_COMMAND_MOVE_FOCUS, countdown);
    return notifyMoveFocus(players, S_COMMAND_MOVE_FOCUS, countdown);
}

bool LegacyRoom::notifyMoveFocus(LegacyServerPlayer *player, CommandType command)
{
    QList<LegacyServerPlayer *> players;
    players.append(player);
    Countdown countdown;
    countdown.max = serverInfo()->getCommandTimeout(command, S_CLIENT_INSTANCE);
    countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
    return notifyMoveFocus(players, S_COMMAND_MOVE_FOCUS, countdown);
}

bool LegacyRoom::notifyMoveFocus(const QList<LegacyServerPlayer *> &players, CommandType command, const Countdown &countdown)
{
    QJsonArray arg;
    QJsonArray arg1;
    int n = players.length();
    for (int i = 0; i < n; ++i)
        arg1 << players.value(i)->objectName();
    arg << arg1 << command << countdown.toVariant();
    return doBroadcastNotify(S_COMMAND_MOVE_FOCUS, arg);
}

bool LegacyRoom::askForSkillInvoke(LegacyServerPlayer *player, const QString &skill_name, const QVariant &data, const QString &prompt)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_INVOKE_SKILL);

    bool invoked = false;
    QJsonArray skillCommand;
    if (!prompt.isNull())
        skillCommand << skill_name << prompt;
    else if (data.canConvert<QString>())
        skillCommand << skill_name << data.toString();
    else {
        LegacyServerPlayer *player = data.value<LegacyServerPlayer *>();
        QString data_str;
        if (player != nullptr)
            data_str = QStringLiteral("playerdata:") + player->objectName();
        skillCommand << skill_name << data_str;
    }

    if (!doRequest(player, S_COMMAND_INVOKE_SKILL, skillCommand, true)) {
        invoked = false;
    } else {
        const QJsonValue &clientReply = player->getClientReply();
        if (clientReply.isBool())
            invoked = clientReply.toBool();
    }

    if (invoked) {
        QJsonArray msg;
        msg << skill_name << player->objectName();
        doBroadcastNotify(S_COMMAND_INVOKE_SKILL, msg);
        notifySkillInvoked(player, skill_name);
    }

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::SkillInvoke;
    s.args << skill_name << (invoked ? QStringLiteral("yes") : QStringLiteral("no"));
    s.m_extraData = data;
    QVariant d = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, d);
    return invoked;
}

bool LegacyRoom::askForSkillInvoke(LegacyServerPlayer *player, const Skill *skill, const QVariant &data /* = QVariant() */, const QString &prompt)
{
    if (skill == nullptr)
        return false;

    return askForSkillInvoke(player, skill->name(), data, prompt);
}

QString LegacyRoom::askForChoice(LegacyServerPlayer *player, const QString &skill_name, const QString &choices, const QVariant &data)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_MULTIPLE_CHOICE);
    //QStringList validChoices = choices.split("+");
    QStringList validChoices;
    foreach (const QString &choice, choices.split(QStringLiteral("|")))
        validChoices.append(choice.split(QStringLiteral("+")));
    Q_ASSERT(!validChoices.isEmpty());
    QStringList titles = skill_name.split(QStringLiteral("%"));
    const QString &skillname = titles.at(0);

    QString answer;
    if (validChoices.size() == 1)
        answer = validChoices.first();
    else {
        bool success = doRequest(player, S_COMMAND_MULTIPLE_CHOICE, QJsonArray() << skillname << choices, true);
        QVariant clientReply = player->getClientReply();
        if (!success || !clientReply.canConvert<QString>()) {
            answer = QStringLiteral("cancel");
        } else
            answer = clientReply.toString();
    }

    if (!validChoices.contains(answer))
        answer = validChoices.at(QRandomGenerator::global()->generate() % validChoices.length());
    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::SkillChoice;
    s.args << skillname << answer;
    s.m_extraData = data;
    QVariant d = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, d);
    return answer;
}

void LegacyRoom::obtainCard(LegacyServerPlayer *target, const Card *card, const CardMoveReason &reason, bool unhide)
{
    if (card == nullptr)
        return;
    moveCardTo(card, nullptr, target, QSanguosha::PlaceHand, reason, unhide);
}

void LegacyRoom::obtainCard(LegacyServerPlayer *target, const Card *card, bool unhide)
{
    if (card == nullptr)
        return;
    CardMoveReason reason(QSanguosha::MoveReasonGotBack, target->objectName());
    obtainCard(target, card, reason, unhide);
}

void LegacyRoom::obtainCard(LegacyServerPlayer *target, int card_id, bool unhide)
{
    obtainCard(target, card(card_id), unhide);
}

bool LegacyRoom::isCanceled(const CardEffectStruct &effect)
{
    QVariant edata = QVariant::fromValue(effect);
    if (getThread()->trigger(QSanguosha::Cancel, edata)) {
        CardEffectStruct effect1 = edata.value<CardEffectStruct>();
        return effect1.canceled;
    }

    if (!effect.card->face()->isCancelable(effect))
        return false;

    //for HegNullification
    QStringList targets = getTag(effect.card->toString() + QStringLiteral("HegNullificationTargets")).toStringList();
    if (!targets.isEmpty()) {
        if (targets.contains(effect.to->objectName())) {
            LogStruct log;
            log.type = QStringLiteral("#HegNullificationEffect");
            log.from = effect.from;
            log.to << effect.to;
            log.arg = effect.card->faceName();
            sendLog(log);

            setEmotion(qobject_cast<LegacyServerPlayer *>(effect.to), QStringLiteral("skill_nullify"));
            return true;
        }
    }

    setTag(QStringLiteral("HegNullificationValid"), false);

    QVariant decisionData = QVariant::fromValue(effect.to);
    setTag(QStringLiteral("NullifyingTarget"), decisionData);
    decisionData = QVariant::fromValue(effect.from);
    setTag(QStringLiteral("NullifyingSource"), decisionData);
    decisionData = QVariant::fromValue(effect.card);
    setTag(QStringLiteral("NullifyingCard"), decisionData);
    setTag(QStringLiteral("NullifyingTimes"), 0);

    bool result = askForNullification(effect.card, qobject_cast<LegacyServerPlayer *>(effect.from), qobject_cast<LegacyServerPlayer *>(effect.to), true);
    if (getTag(QStringLiteral("HegNullificationValid")).toBool() && effect.card->face()->isNdTrick()) {
        foreach (LegacyServerPlayer *p, m_players) {
            if (p->isAlive() && p->isFriendWith(effect.to))
                targets << p->objectName();
        }
        setTag(effect.card->toString() + QStringLiteral("HegNullificationTargets"), targets);
    }

    //deal xianshi
    foreach (LegacyServerPlayer *p, getAllPlayers()) {
        LegacyServerPlayer *target = p->tag[QStringLiteral("xianshi_nullification_target")].value<LegacyServerPlayer *>();
        p->tag.remove(QStringLiteral("xianshi_nullification_target"));
        const Card *xianshi_nullification = p->tag.value(QStringLiteral("xianshi_nullification")).value<const Card *>();
        p->tag.remove(QStringLiteral("xianshi_nullification"));

        if ((target != nullptr) && (xianshi_nullification != nullptr)) {
            int xianshi_result = p->tag[QStringLiteral("xianshi_nullification_result")].toInt();
            p->tag.remove(QStringLiteral("xianshi_nullification_result"));

            if ((xianshi_result == 0 && !result) || (xianshi_result == 1 && result)) {
                QString xianshi_name = p->property("xianshi_card").toString();
                if (!xianshi_name.isNull() && p->isAlive() && target->isAlive()) {
                    Card *extraCard = cloneCard(xianshi_name);
                    if (extraCard->face()->isKindOf(QStringLiteral("Slash"))) {
                        QSanguosha::DamageNature nature = QSanguosha::DamageNormal;
                        if (extraCard->face()->isKindOf(QStringLiteral("FireSlash")))
                            nature = QSanguosha::DamageFire;
                        else if (extraCard->face()->isKindOf(QStringLiteral("ThunderSlash")))
                            nature = QSanguosha::DamageThunder;
                        int damageValue = 1;

                        if (extraCard->face()->isKindOf(QStringLiteral("DebuffSlash"))) {
                            SlashEffectStruct extraEffect;
                            extraEffect.from = p;
                            extraEffect.slash = extraCard;
                            extraEffect.to = target;
#if 0
                            if (extraCard->face()->isKindOf("IronSlash"))
                                IronSlash::debuffEffect(extraEffect);
                            else if (extraCard->face()->isKindOf("LightSlash"))
                                LightSlash::debuffEffect(extraEffect);
                            else if (extraCard->face()->isKindOf("PowerSlash"))
                                PowerSlash::debuffEffect(extraEffect);
#endif
                        }

                        if (!extraCard->face()->isKindOf(QStringLiteral("LightSlash")) && !extraCard->face()->isKindOf(QStringLiteral("PowerSlash"))) {
                            damageValue = damageValue + effect.effectValue.first();
                        }

                        DamageStruct d = DamageStruct(xianshi_nullification, p, target, damageValue, nature);
                        damage(d);

                    } else if (extraCard->face()->isKindOf(QStringLiteral("Peach"))) {
                        CardEffectStruct extraEffect;

                        extraEffect.card = xianshi_nullification;
                        extraEffect.from = p;
                        extraEffect.to = target;
                        extraEffect.multiple = effect.multiple;
                        extraCard->face()->onEffect(extraEffect);
                    } else if (extraCard->face()->isKindOf(QStringLiteral("Analeptic"))) {
                        RecoverStruct re;
                        re.card = xianshi_nullification;
                        re.from = p;
                        recover(target, re);
                    } else if (extraCard->face()->isKindOf(QStringLiteral("AmazingGrace"))) {
                        doExtraAmazingGrace(p, target, 1);
                    } else { //Trick card
                        CardEffectStruct extraEffect;

                        extraEffect.card = xianshi_nullification;
                        extraEffect.from = p;
                        extraEffect.to = target;
                        extraEffect.multiple = effect.multiple;
                        extraCard->face()->onEffect(extraEffect);
                    }
                    delete extraCard;
                }
            }
        }
    }

    return result;
}

bool LegacyRoom::verifyNullificationResponse(LegacyServerPlayer *player, const QJsonValue &response, void * /*unused*/)
{
    const Card *card = nullptr;
    if (player != nullptr && response.isArray()) {
        QJsonArray responseArray = response.toArray();
        if (QSgsJsonUtils::isString(responseArray[0]))
            card = Card::Parse(responseArray[0].toString(), this);
    }
    return card != nullptr;
}

bool LegacyRoom::askForNullification(const Card *trick, LegacyServerPlayer *from, LegacyServerPlayer *to, bool positive)
{
    _NullificationAiHelper aiHelper;
    aiHelper.m_from = from;
    aiHelper.m_to = to;
    aiHelper.m_trick = trick;
    return _askForNullification(trick, from, to, positive, aiHelper);
}

bool LegacyRoom::_askForNullification(const Card *trick, LegacyServerPlayer *from, LegacyServerPlayer *to, bool positive, const _NullificationAiHelper &aiHelper)
{
    tryPause();

    setCurrentCardUseReason(QSanguosha::CardUseReasonResponseUse);
    QString trick_name = trick->faceName();
    QList<LegacyServerPlayer *> validHumanPlayers;

    QJsonArray arg;
    arg << trick_name;
    arg << (from != nullptr ? QJsonValue(from->objectName()) : QJsonValue());
    arg << (to != nullptr ? QJsonValue(to->objectName()) : QJsonValue());

    CardEffectStruct trickEffect;
    trickEffect.card = trick;
    trickEffect.from = from;
    trickEffect.to = to;
    QVariant data = QVariant::fromValue(trickEffect);

    setTag(QStringLiteral("NullifiationTarget"), data);

    foreach (LegacyServerPlayer *player, getAllPlayers()) {
        if (player->hasNullification()) {
            if (!thread->trigger(QSanguosha::TrickCardCanceling, data)) {
                player->m_commandArgs = arg;
                validHumanPlayers << player;
            }
        }
    }

    LegacyServerPlayer *repliedPlayer = nullptr;
    time_t timeOut = serverInfo()->getCommandTimeout(S_COMMAND_NULLIFICATION, S_SERVER_INSTANCE);
    if (!validHumanPlayers.isEmpty()) {
        if (trick->face()->isKindOf(QStringLiteral("AOE")) || trick->face()->isKindOf(QStringLiteral("GlobalEffect"))) {
            foreach (LegacyServerPlayer *p, validHumanPlayers)
                doNotify(p, S_COMMAND_NULLIFICATION_ASKED, QJsonValue(trick->faceName()));
        }
        repliedPlayer = doBroadcastRaceRequest(validHumanPlayers, S_COMMAND_NULLIFICATION, timeOut, &LegacyRoom::verifyNullificationResponse);
    }
    const Card *card = nullptr;
    if (repliedPlayer != nullptr) {
        QJsonArray clientReply = repliedPlayer->getClientReply().toArray();
        if (!clientReply.empty() && QSgsJsonUtils::isString(clientReply[0]))
            card = Card::Parse(clientReply[0].toString(), this);
    }

    if (card == nullptr)
        return false;

    card = card->face()->validateInResponse(repliedPlayer, card);
    if (card != nullptr && repliedPlayer->isCardLimited(card, QSanguosha::MethodUse))
        card = nullptr;
    if (card == nullptr)
        return _askForNullification(trick, from, to, positive, aiHelper);

    doAnimate(S_ANIMATE_NULLIFICATION, repliedPlayer->objectName(), to->objectName());
    useCard(CardUseStruct(card, repliedPlayer, QList<Player *>()));
    //deal HegNullification
    bool isHegNullification = false;
    QString heg_nullification_selection;

    if (!repliedPlayer->hasFlag(QStringLiteral("nullifiationNul")) && card->face()->isKindOf(QStringLiteral("HegNullification"))
        && !trick->face()->isKindOf(QStringLiteral("Nullification")) && trick->face()->isNdTrick() && to->roleString() != QStringLiteral("careerist")
        && to->haveShownOneGeneral()) {
        QVariantList qtargets = tag[QStringLiteral("targets") + trick->toString()].toList();
        QList<LegacyServerPlayer *> targets;
        for (int i = 0; i < qtargets.size(); i++) {
            const QVariant &n = qtargets.at(i);
            targets.append(n.value<LegacyServerPlayer *>());
        }
        QList<LegacyServerPlayer *> targets_copy = targets;
        targets.removeOne(to);
        foreach (LegacyServerPlayer *p, targets_copy) {
            if (targets_copy.indexOf(p) < targets_copy.indexOf(to))
                targets.removeOne(p);
        }
        if (!targets.isEmpty()) {
            foreach (LegacyServerPlayer *p, targets) {
                if (p->roleString() != QStringLiteral("careerist") && p->haveShownOneGeneral() && p->roleString() == to->roleString()) {
                    isHegNullification = true;
                    break;
                }
            }
        }
        if (isHegNullification)
            heg_nullification_selection = askForChoice(repliedPlayer, QStringLiteral("heg_nullification"), QStringLiteral("single+all"), data);

        if (heg_nullification_selection.contains(QStringLiteral("all")))
            heg_nullification_selection = QStringLiteral("all");
        else
            heg_nullification_selection = QStringLiteral("single");
    }

    if (!isHegNullification) {
        LogStruct log;
        log.type = QStringLiteral("#NullificationDetails");
        log.from = from;
        log.to << to;
        log.arg = trick_name;
        sendLog(log);
    } else {
        LogStruct log;
        log.type = QStringLiteral("#HegNullificationDetails");
        log.from = from;
        log.to << to;
        log.arg = trick_name;
        sendLog(log);
        LogStruct log2;
        log2.type = QStringLiteral("#HegNullificationSelection");
        log2.from = repliedPlayer;
        log2.arg = QStringLiteral("hegnul_") + heg_nullification_selection;
        sendLog(log2);
    }
    setTag(QStringLiteral("NullificatonType"), isHegNullification);
    thread->delay(500);

    //though weiya cancel the effect,but choicemade should be triggerable
    ChoiceMadeStruct s;
    s.player = repliedPlayer;
    s.type = ChoiceMadeStruct::Nullification;
    s.args << trick->faceName() << to->objectName() << (positive ? QStringLiteral("true") : QStringLiteral("false"));
    QVariant d = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, d);
    int pagoda = getTag(QStringLiteral("NullifyingTimes")).toInt();
    setTag(QStringLiteral("NullifyingTimes"), getTag(QStringLiteral("NullifyingTimes")).toInt() + 1);

    bool result = true;
    CardEffectStruct effect;
    effect.card = card;
    effect.to = repliedPlayer;

    //for processing weiya
    if (repliedPlayer->hasFlag(QStringLiteral("nullifiationNul"))) {
        result = false;
        setPlayerFlag(repliedPlayer, QStringLiteral("-nullifiationNul"));
    }
    //deal xianshi
    if (result && card->skillName() == QStringLiteral("xianshi")) {
        repliedPlayer->tag[QStringLiteral("xianshi_nullification_target")] = QVariant::fromValue(from);
        repliedPlayer->tag[QStringLiteral("xianshi_nullification")] = QVariant::fromValue(card);
        if (!positive)
            repliedPlayer->tag[QStringLiteral("xianshi_nullification_result")] = QVariant::fromValue(0);
        else
            repliedPlayer->tag[QStringLiteral("xianshi_nullification_result")] = QVariant::fromValue(1);
    }

    if (card->face()->isCancelable(effect)) {
        if (result) {
            result = !_askForNullification(card, repliedPlayer, to, !positive, aiHelper);
        } else {
            result = _askForNullification(trick, from, to, positive, aiHelper);
        }
    }
    // TODO: deal with Pagoda when refactoring Nullification
    Q_UNUSED(pagoda);
#if 0
    if (pagoda == 0 && result && EquipSkill::equipAvailable(repliedPlayer, QSanguosha::TreasureLocation, "Pagoda")) {
        bool isLastTarget = true;
        foreach (QString flag, trick->flags()) {
            if (flag.startsWith("LastTrickTarget_")) {
                QStringList f = flag.split("_");
                ServerPlayer *last = findPlayerByObjectName(f.at(1));
                if (last != nullptr) {
                    if (last != to)
                        isLastTarget = false;
                    break;
                }
            }
        }
        if (!isLastTarget && askForSkillInvoke(repliedPlayer, "Pagoda", data))
            trick->addFlag("PagodaNullifiation");
    }
#endif
    removeTag(QStringLiteral("NullificatonType"));

    if (isHegNullification && heg_nullification_selection == QStringLiteral("all") && result) {
        setTag(QStringLiteral("HegNullificationValid"), true);
    }

    return result;
}

int LegacyRoom::askForCardChosen(LegacyServerPlayer *player, LegacyServerPlayer *who, const QString &flags, const QString &reason, bool handcard_visible,
                                 QSanguosha::HandlingMethod method, const QList<int> &disabled_ids)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_CARD);

    //lord skill: youtong
    if (player != who && player->kingdom() == QStringLiteral("dld")) {
        QList<LegacyServerPlayer *> targets;
        foreach (LegacyServerPlayer *p, getOtherPlayers(player)) {
            if (p->hasValidLordSkill(QStringLiteral("youtong"))) {
                targets << p;
            }
        }
        if (!targets.isEmpty()) {
            LegacyServerPlayer *lord = askForPlayerChosen(player, targets, QStringLiteral("youtong"), QStringLiteral("@youtong:") + who->objectName(), true);
            if (lord != nullptr) {
                LogStruct log;
                log.type = QStringLiteral("#InvokeOthersSkill");
                log.from = player;
                log.to << lord;
                log.arg = QStringLiteral("youtong");
                sendLog(log);

                notifySkillInvoked(lord, QStringLiteral("youtong"));
                doAnimate(S_ANIMATE_INDICATE, player->objectName(), lord->objectName());
                player = lord;
            }
        }
    }

    if (player->hasValidSkill(QStringLiteral("duxin"))) {
        handcard_visible = true;
        if (flags.contains(QStringLiteral("h")))
            notifySkillInvoked(player, QStringLiteral("duxin"));
        doAnimate(S_ANIMATE_INDICATE, player->objectName(), who->objectName());
    }

    if (player->hasValidSkill(QStringLiteral("duxin_hegemony")) && flags.contains(QStringLiteral("h"))) {
        if (player->haveShownSkill(QStringLiteral("duxin_hegemony")) || player->askForSkillInvoke(QStringLiteral("duxin_hegemony"), QVariant::fromValue(who))) {
            if (!player->haveShownSkill(QStringLiteral("duxin_hegemony")))
                player->showHiddenSkill(QStringLiteral("duxin_hegemony"));
            handcard_visible = true;
            notifySkillInvoked(player, QStringLiteral("duxin_hegemony"));
            doAnimate(S_ANIMATE_INDICATE, player->objectName(), who->objectName());
        }
    }

    const IdSet &shownHandcards = who->shownHandcards();
    QList<int> unknownHandcards = who->handCardIds().values();
    foreach (int id, shownHandcards)
        unknownHandcards.removeOne(id);

    //re-check disable id
    QList<int> checked_disabled_ids;
    checked_disabled_ids << disabled_ids;
    bool EnableEmptyCard = false; //setting UI EmptyCard Enable
    foreach (const Card *c, who->handCards()) {
        if (!checked_disabled_ids.contains(c->id())) {
            if (!flags.contains(QStringLiteral("h")) && !who->isShownHandcard(c->id()))
                checked_disabled_ids << c->id();
            else if (!flags.contains(QStringLiteral("s")) && who->isShownHandcard(c->id()))
                checked_disabled_ids << c->id();
        }
        if (!EnableEmptyCard && !who->isShownHandcard(c->id()) && !checked_disabled_ids.contains(c->id()))
            EnableEmptyCard = true;
    }

    //At first,collect selectable cards, and selectable knowncards
    QList<const Card *> cards = who->getCards(flags);
    QList<const Card *> knownCards = who->getCards(flags);
    foreach (const Card *card, cards) {
        if ((method == QSanguosha::MethodDiscard && !player->canDiscard(who, card->effectiveId(), reason)) || checked_disabled_ids.contains(card->effectiveId())) {
            cards.removeOne(card);
            knownCards.removeOne(card);
        } else if (unknownHandcards.contains(card->effectiveId()))
            knownCards.removeOne(card);
    }
    Q_ASSERT(!cards.isEmpty());

    if (handcard_visible && !who->isKongcheng()) {
        QList<int> handcards = who->handCardIds().values();
        QJsonArray arg;
        arg << who->objectName();
        arg << QSgsJsonUtils::toJsonArray(handcards);
        doNotify(player, S_COMMAND_SET_KNOWN_CARDS, arg);
    }

    //secondly, if can not choose visible(known) cards, then randomly choose a card.
    int card_id = Card::S_UNKNOWN_CARD_ID;

    QJsonArray arg;
    arg << who->objectName();
    arg << flags;
    arg << reason;
    arg << handcard_visible;
    arg << (int)method;
    arg << QSgsJsonUtils::toJsonArray(checked_disabled_ids);
    arg << EnableEmptyCard;

    bool success = doRequest(player, S_COMMAND_CHOOSE_CARD, arg, true);
    //@todo: check if the card returned is valid
    const QJsonValue &clientReply = player->getClientReply();
    if (!success || !QSgsJsonUtils::isNumber(clientReply)) {
        // randomly choose a card
        card_id = cards.at(QRandomGenerator::global()->generate() % cards.length())->id();
    } else
        card_id = clientReply.toInt();

    if (card_id == Card::S_UNKNOWN_CARD_ID) {
        foreach (int id, checked_disabled_ids)
            unknownHandcards.removeOne(id);
        if (!unknownHandcards.isEmpty())
            card_id = unknownHandcards.at(QRandomGenerator::global()->generate() % unknownHandcards.length());
    }

    if (!cards.contains(card(card_id)))
        card_id = cards.at(QRandomGenerator::global()->generate() % cards.length())->id();

    Q_ASSERT(card_id != Card::S_UNKNOWN_CARD_ID);

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::CardChosen;
    s.args << reason << QString::number(card_id) << player->objectName() << who->objectName();
    QVariant d = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, d);
    return card_id;
}

const Card *LegacyRoom::askForCard(LegacyServerPlayer *player, const QString &pattern, const QString &prompt, const QVariant &data, const QString &skill_name, int notice_index)
{
    return askForCard(player, pattern, prompt, data, QSanguosha::MethodDiscard, nullptr, false, skill_name, false, notice_index);
}

const Card *LegacyRoom::askForCard(LegacyServerPlayer *player, const QString &pattern, const QString &prompt, const QVariant &data, QSanguosha::HandlingMethod method,
                                   LegacyServerPlayer *to, bool isRetrial, const QString &skill_name, bool isProvision, int notice_index)
{
    Q_ASSERT(pattern != QStringLiteral("slash") || method != QSanguosha::MethodUse); // use askForUseSlashTo instead

    // FIXME: Q_ASSERT(method != QSanguosha::MethodUse); // Use ask for use card instead
    tryPause();
    notifyMoveFocus(player, S_COMMAND_RESPONSE_CARD);

    setCurrentCardUsePattern(pattern);

    const Card *card_ = nullptr;
    CardAskedStruct s;
    s.player = player;
    s.pattern = pattern;
    s.prompt = prompt;
    s.method = method;
    QVariant asked_data = QVariant::fromValue(s);
    if ((method == QSanguosha::MethodUse || method == QSanguosha::MethodResponse) && !isRetrial && !player->hasFlag(QStringLiteral("continuing")))
        thread->trigger(QSanguosha::CardAsked, asked_data);

    //case 1. for the player died since a counter attack from juwang target
    //case 2. figure out dizhen and eghitDiagram and tianren
    //we also need clear the provider info
    if (!player->isAlive() && !isProvision) {
        provided = nullptr;
        has_provided = false;
        provider = nullptr;
        return nullptr;
    }
    QSanguosha::CardUseReason reason = QSanguosha::CardUseReasonUnknown;
    if (method == QSanguosha::MethodResponse)
        reason = QSanguosha::CardUseReasonResponse;
    else if (method == QSanguosha::MethodUse)
        reason = QSanguosha::CardUseReasonResponseUse;
    setCurrentCardUseReason(reason);

    if (player->hasFlag(QStringLiteral("continuing")))
        setPlayerFlag(player, QStringLiteral("-continuing"));

    LegacyServerPlayer *theProvider = nullptr;
    if (has_provided || !player->isAlive()) {
        card_ = provided;
        theProvider = provider;

        if (player->isCardLimited(card_, method))
            card_ = nullptr;
        provided = nullptr;
        has_provided = false;
        provider = nullptr;

    } else {
        QJsonArray arg;
        arg << pattern;
        arg << prompt;
        arg << int(method);
        arg << notice_index;
        arg << QString(skill_name);

        bool success = doRequest(player, S_COMMAND_RESPONSE_CARD, arg, true);
        QJsonArray clientReply = player->getClientReply().toArray();
        if (success && !clientReply.isEmpty())
            card_ = Card::Parse(clientReply[0].toString(), this);
    }

    if (card_ == nullptr) {
        ChoiceMadeStruct s;
        s.player = player;
        s.type = ChoiceMadeStruct::CardResponded;
        s.args << pattern << prompt << QStringLiteral("_nil_");
        s.m_extraData = data;
        QVariant d = QVariant::fromValue(s);
        thread->trigger(QSanguosha::ChoiceMade, d);
        return nullptr;
    }

    card_ = card_->face()->validateInResponse(player, card_);
    if (card_ != nullptr && player->isCardLimited(card_, method))
        card_ = nullptr;
    const Card *result = nullptr;
    //card log
    if (card_ != nullptr) {
        if (!card_->isVirtualCard()) {
            Card *wrapped = card(card_->effectiveId());
            if (wrapped->isModified())
                broadcastUpdateCard(serverPlayers(), card_->effectiveId(), wrapped);
            else
                broadcastResetCard(serverPlayers(), card_->effectiveId());
        }

        if ((method == QSanguosha::MethodUse || method == QSanguosha::MethodResponse) && !isRetrial) {
            LogStruct log;
            log.card_str = card_->toString();
            log.from = player;
            log.type = QStringLiteral("#%1").arg(card_->faceName());
            if (method == QSanguosha::MethodResponse)
                log.type += QStringLiteral("_Resp");
            sendLog(log);
            player->broadcastSkillInvoke(card_);
        } else if (method == QSanguosha::MethodDiscard) {
            LogStruct log;
            log.type = skill_name.isEmpty() ? QStringLiteral("$DiscardCard") : QStringLiteral("$DiscardCardWithSkill");
            log.from = player;
            QList<int> to_discard;
            if (card_->isVirtualCard())
                to_discard.append(card_->subcards().values());
            else
                to_discard << card_->effectiveId();
            log.card_str = IntList2StringList(to_discard).join(QStringLiteral("+"));
            if (!skill_name.isEmpty())
                log.arg = skill_name;
            sendLog(log);
            if (!skill_name.isEmpty())
                notifySkillInvoked(player, skill_name);
        }
    }
    //trigger event
    if (card_ != nullptr) {
        bool isHandcard = true;
        bool isShowncard = false;
        IdSet ids;
        if (!card_->isVirtualCard())
            ids << card_->effectiveId();
        else
            ids = card_->subcards();
        if (!ids.isEmpty()) {
            foreach (int id, ids) {
                if (cardOwner(id) != player || cardPlace(id) != QSanguosha::PlaceHand) {
                    isHandcard = false;
                    break;
                }
            }
        } else {
            isHandcard = false;
        }

        if (!ids.isEmpty()) {
            foreach (int id, ids) {
                Player *shownCardOwner = cardOwner(id);
                if ((shownCardOwner != nullptr) && shownCardOwner->isShownHandcard(id)) {
                    isShowncard = true;
                    break;
                }
            }
        }

        ChoiceMadeStruct s;
        s.player = player;
        s.type = ChoiceMadeStruct::CardResponded;
        s.args << pattern << prompt << QStringLiteral("_%1_").arg(card_->toString());
        s.m_extraData = data;
        QVariant d = QVariant::fromValue(s);
        thread->trigger(QSanguosha::ChoiceMade, d);
        //show hidden general
        QString showskill = card_->skillName();
        player->showHiddenSkill(showskill);
        const Skill *equipSkill = Sanguosha->skill(skill_name);
        if ((equipSkill != nullptr) && equipSkill->isEquipSkill()) {
            const TreatAsEquippingSkill *v = treatAsEquipping(player, skill_name, QSanguosha::WeaponLocation);
            if (v != nullptr)
                player->showHiddenSkill(v->name());
        }

        //move1
        if (method == QSanguosha::MethodUse) {
            CardMoveReason reason(QSanguosha::MoveReasonLetUse, player->objectName(), QString(), card_->skillName(), QString());

            reason.m_extraData = QVariant::fromValue(card_);
            if (theProvider != nullptr)
                moveCardTo(card_, theProvider, nullptr, QSanguosha::PlaceTable, reason, true);
            else if (!card_->isVirtualCard() && (cardOwner(card_->effectiveId()) != nullptr) && cardOwner(card_->effectiveId()) != player) //only for Skill Xinhua
                moveCardTo(card_, cardOwner(card_->effectiveId()), nullptr, QSanguosha::PlaceTable, reason, true);
            else
                moveCardTo(card_, player, nullptr, QSanguosha::PlaceTable, reason, true);
        } else if (method == QSanguosha::MethodDiscard) {
            CardMoveReason reason(QSanguosha::MoveReasonThrow, player->objectName());
            moveCardTo(card_, player, nullptr, QSanguosha::PlaceDiscardPile, reason, pattern != QStringLiteral(".") && pattern != QStringLiteral(".."));
        } else if (method != QSanguosha::MethodNone && !isRetrial) {
            CardMoveReason reason(QSanguosha::MoveReasonResponse, player->objectName());
            reason.m_skillName = card_->skillName();
            reason.m_extraData = QVariant::fromValue(card_);
            if (theProvider != nullptr)
                moveCardTo(card_, theProvider, nullptr, isProvision ? QSanguosha::PlaceTable : QSanguosha::PlaceDiscardPile, reason, method != QSanguosha::MethodPindian);
            else if (!card_->isVirtualCard() && (cardOwner(card_->effectiveId()) != nullptr) && cardOwner(card_->effectiveId()) != player) //only for Skill Xinhua
                moveCardTo(card_, cardOwner(card_->effectiveId()), nullptr, isProvision ? QSanguosha::PlaceTable : QSanguosha::PlaceDiscardPile, reason,
                           method != QSanguosha::MethodPindian);
            else
                moveCardTo(card_, player, nullptr, isProvision ? QSanguosha::PlaceTable : QSanguosha::PlaceDiscardPile, reason, method != QSanguosha::MethodPindian);
        }
        //move2
        if ((method == QSanguosha::MethodUse || method == QSanguosha::MethodResponse) && !isRetrial) {
            if (!card_->skillName().isNull() && card_->skillName(true) == card_->skillName(false) && player->hasValidSkill(card_->skillName()))
                notifySkillInvoked(player, card_->skillName());
            CardResponseStruct resp(card_, to, isRetrial, isProvision);
            resp.m_isHandcard = isHandcard;
            resp.m_isShowncard = isShowncard;
            QVariant data = QVariant::fromValue(resp);
            thread->trigger(QSanguosha::CardResponded, data);
            resp = data.value<CardResponseStruct>();
            if (method == QSanguosha::MethodUse) {
                if (cardPlace(card_->effectiveId()) == QSanguosha::PlaceTable) {
                    CardMoveReason reason(QSanguosha::MoveReasonLetUse, player->objectName(), QString(), card_->skillName(), QString());
                    reason.m_extraData = QVariant::fromValue(card_);
                    if (theProvider != nullptr)
                        moveCardTo(card_, theProvider, nullptr, QSanguosha::PlaceDiscardPile, reason, true);
                    else
                        moveCardTo(card_, player, nullptr, QSanguosha::PlaceDiscardPile, reason, true);
                }
                CardUseStruct card_use;
                card_use.card = card_;
                card_use.from = player;
                if (to != nullptr)
                    card_use.to << to;
                QVariant data2 = QVariant::fromValue(card_use);
                thread->trigger(QSanguosha::CardFinished, data2);
            }
            if (resp.m_isNullified)
                return nullptr;
        }
        result = card_;
    } else {
        setPlayerFlag(player, QStringLiteral("continuing"));
        result = askForCard(player, pattern, prompt, data, method, to, isRetrial);
    }
    return result;
}

const Card *LegacyRoom::askForUseCard(LegacyServerPlayer *player, const QString &pattern, const QString &prompt, int notice_index, QSanguosha::HandlingMethod method,
                                      bool addHistory, const QString &skill_name)
{
    Q_ASSERT(method != QSanguosha::MethodResponse);

    tryPause();
    notifyMoveFocus(player, S_COMMAND_RESPONSE_CARD);

    setCurrentCardUsePattern(pattern);
    setCurrentCardUseReason(QSanguosha::CardUseReasonResponseUse);
    CardUseStruct card_use;

    bool isCardUsed = false;
    QJsonArray ask_str;
    ask_str << pattern;
    ask_str << prompt;
    ask_str << int(method);
    ask_str << notice_index;
    ask_str << QString(skill_name);

    bool success = doRequest(player, S_COMMAND_RESPONSE_CARD, ask_str, true);
    if (success) {
        const QJsonValue &clientReply = player->getClientReply();
        isCardUsed = !clientReply.isNull();
        if (isCardUsed && ExtendCardUseStruct::tryParse(card_use, clientReply, this))
            card_use.from = player;
    }

    card_use.m_reason = QSanguosha::CardUseReasonResponseUse;

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::CardUsed;
    s.args << pattern << prompt;
    if (isCardUsed && ExtendCardUseStruct::isValid(card_use, pattern)) {
        s.args << ExtendCardUseStruct::toString(card_use);
        QVariant decisionData = QVariant::fromValue(s);
        thread->trigger(QSanguosha::ChoiceMade, decisionData);
        //show hidden general
        player->showHiddenSkill(skill_name);
        player->showHiddenSkill(card_use.card->skillName());
        if (!useCard(card_use, addHistory))
            return askForUseCard(player, pattern, prompt, notice_index, method, addHistory);

        return card_use.card;
    } else {
        s.args << QStringLiteral("nil");
        QVariant decisionData = QVariant::fromValue(s);
        thread->trigger(QSanguosha::ChoiceMade, decisionData);
    }

    return nullptr;
}

const Card *LegacyRoom::askForUseSlashTo(LegacyServerPlayer *slasher, QList<LegacyServerPlayer *> victims, const QString &prompt, bool distance_limit, bool disable_extra,
                                         bool addHistory)
{
    Q_ASSERT(!victims.isEmpty());

    // The realization of this function in the Slash::onUse and Slash::targetFilter.
    setPlayerFlag(slasher, QStringLiteral("slashTargetFix"));
    if (!distance_limit)
        setPlayerFlag(slasher, QStringLiteral("slashNoDistanceLimit"));
    if (disable_extra)
        setPlayerFlag(slasher, QStringLiteral("slashDisableExtraTarget"));
    if (victims.length() == 1)
        setPlayerFlag(slasher, QStringLiteral("slashTargetFixToOne"));
    foreach (LegacyServerPlayer *victim, victims)
        setPlayerFlag(victim, QStringLiteral("SlashAssignee"));

    const Card *slash = askForUseCard(slasher, QStringLiteral("slash"), prompt, -1, QSanguosha::MethodUse, addHistory);
    if (slash == nullptr) {
        setPlayerFlag(slasher, QStringLiteral("-slashTargetFix"));
        setPlayerFlag(slasher, QStringLiteral("-slashTargetFixToOne"));
        foreach (LegacyServerPlayer *victim, victims)
            setPlayerFlag(victim, QStringLiteral("-SlashAssignee"));
        if (slasher->hasFlag(QStringLiteral("slashNoDistanceLimit")))
            setPlayerFlag(slasher, QStringLiteral("-slashNoDistanceLimit"));
        if (slasher->hasFlag(QStringLiteral("slashDisableExtraTarget")))
            setPlayerFlag(slasher, QStringLiteral("-slashDisableExtraTarget"));
    }

    return slash;
}

const Card *LegacyRoom::askForUseSlashTo(LegacyServerPlayer *slasher, LegacyServerPlayer *victim, const QString &prompt, bool distance_limit, bool disable_extra, bool addHistory)
{
    Q_ASSERT(victim != nullptr);
    QList<LegacyServerPlayer *> victims;
    victims << victim;
    return askForUseSlashTo(slasher, victims, prompt, distance_limit, disable_extra, addHistory);
}

int LegacyRoom::askForAG(LegacyServerPlayer *player, const QList<int> &card_ids, bool refusable, const QString &reason)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_AMAZING_GRACE);
    Q_ASSERT(card_ids.length() > 0);

    int card_id = -1;
    if (card_ids.length() == 1 && !refusable)
        card_id = card_ids.first();
    else {
        QJsonArray arg;
        arg << refusable << reason;

        bool success = doRequest(player, S_COMMAND_AMAZING_GRACE, arg, true);
        const QJsonValue &clientReply = player->getClientReply();
        if (success && QSgsJsonUtils::isNumber(clientReply))
            card_id = clientReply.toInt();

        if (!card_ids.contains(card_id))
            card_id = refusable ? -1 : card_ids.first();
    }

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::AGChosen;
    s.args << reason << QString::number(card_id);
    QVariant decisionData = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, decisionData);

    return card_id;
}

void LegacyRoom::doExtraAmazingGrace(LegacyServerPlayer *from, LegacyServerPlayer *target, int times)
{
    //couple xianshi
    target->gainMark(QStringLiteral("@MMP"));
    int count = getAllPlayers().length();
    QList<int> card_ids = getNCards(count);
    LegacyCardsMoveStruct move(card_ids, nullptr, QSanguosha::PlaceTable, CardMoveReason(QSanguosha::MoveReasonTurnover, from->objectName(), QStringLiteral("xianshi"), QString()));
    moveCardsAtomic(move, true);
    fillAG(card_ids);

    for (int i = 0; i < times; i += 1) {
        int card_id = askForAG(target, card_ids, false, QStringLiteral("xianshi"));
        card_ids.removeOne(card_id);
        takeAG(target, card_id);
        if (card_ids.isEmpty())
            break;
    }
    clearAG();

    //throw other cards

    if (!card_ids.isEmpty()) {
        CardMoveReason reason(QSanguosha::MoveReasonNaturalEnter, from->objectName(), QStringLiteral("xianshi"), QString());
        Card *dummy = cloneCard(QStringLiteral("DummyCard"));
        foreach (int id, card_ids)
            dummy->addSubcard(id);
        throwCard(dummy, reason, nullptr);
        delete (dummy);
    }
}

const Card *LegacyRoom::askForCardShow(LegacyServerPlayer *player, LegacyServerPlayer *requestor, const QString &reason)
{
    Q_ASSERT(!player->isKongcheng());

    tryPause();
    notifyMoveFocus(player, S_COMMAND_SHOW_CARD);
    const Card *card = nullptr;

    if (player->handCardNum() == 1)
        card = player->handCards().first();
    else {
        bool success = doRequest(player, S_COMMAND_SHOW_CARD, requestor->generalName(), true);
        QJsonArray clientReply = player->getClientReply().toArray();
        if (success && !clientReply.empty() && QSgsJsonUtils::isString(clientReply[0]))
            card = Card::Parse(clientReply[0].toString(), this);
        if (card == nullptr)
            card = player->getRandomHandCard();
    }

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::CardShow;
    s.args << reason << QStringLiteral("_%1_").arg(card->toString());
    QVariant decisionData = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, decisionData);
    return card;
}

void LegacyRoom::askForSinglePeach(LegacyServerPlayer *player, LegacyServerPlayer *dying, CardUseStruct &use)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_ASK_PEACH);
    setCurrentCardUseReason(QSanguosha::CardUseReasonResponseUse);

    use = CardUseStruct();
    use.from = player;
    int threshold = dying->dyingFactor();

    int peaches = threshold - dying->hp();
    if (dying->hasValidSkill(QStringLiteral("banling"))) {
        peaches = 0;
        if (dying->renHp() <= 0)
            peaches = peaches + threshold - dying->renHp();
        if (dying->lingHp() <= 0)
            peaches = peaches + threshold - dying->lingHp();
    }
    QJsonArray arg;
    arg << dying->objectName();
    arg << peaches;
    bool success = doRequest(player, S_COMMAND_ASK_PEACH, arg, true);
    QJsonArray clientReply = player->getClientReply().toArray();
    if (!success || clientReply.isEmpty() || !QSgsJsonUtils::isString(clientReply[0]))
        return;
    if (!ExtendCardUseStruct::tryParse(use, clientReply, this)) {
        use.card = nullptr;
        return;
    }

    if (use.card != nullptr && use.to.isEmpty())
        use.to << dying;

    if (use.card != nullptr && player->isCardLimited(use.card, use.card->handleMethod()))
        use.card = nullptr;
    if (use.card != nullptr) {
        use.card = use.card->face()->validateInResponse(player, use.card);
        QSanguosha::HandlingMethod method = QSanguosha::MethodUse;
        if (use.card != nullptr && use.card->face()->type() == QSanguosha::TypeSkill) //keep TypeSkill after validateInResponse
            method = use.card->handleMethod();

        if (use.card != nullptr && player->isCardLimited(use.card, method))
            use.card = nullptr;
    } else
        return;

    if (use.card != nullptr) {
        ChoiceMadeStruct s;
        s.player = player;
        s.type = ChoiceMadeStruct::Peach;
        s.args << dying->objectName() << QString::number(threshold - dying->hp()) << use.card->toString();
        QVariant decisionData = QVariant::fromValue(s);
        thread->trigger(QSanguosha::ChoiceMade, decisionData);
    } else
        askForSinglePeach(player, dying, use);
}

QSharedPointer<TriggerDetail> LegacyRoom::askForTriggerOrder(LegacyServerPlayer *player, const QList<QSharedPointer<TriggerDetail>> &sameTiming, bool cancelable,
                                                             const QVariant & /*unused*/)
{
    tryPause();

    Q_ASSERT(!sameTiming.isEmpty());

    QSharedPointer<TriggerDetail> answer;

    if (sameTiming.length() == 1)
        answer = sameTiming.first();
    else {
        notifyMoveFocus(player, S_COMMAND_TRIGGER_ORDER);

        QString reply;

        QJsonArray arr;
        foreach (const QSharedPointer<TriggerDetail> &ptr, sameTiming)
            arr << ExtendTriggerDetail::toVariant(*ptr);
        QJsonArray arr2;
        arr2 << QJsonValue(arr) << cancelable;

        bool success = doRequest(player, S_COMMAND_TRIGGER_ORDER, arr2, true);
        const QJsonValue &clientReply = player->getClientReply();
        if (success && QSgsJsonUtils::isString(clientReply))
            reply = clientReply.toString();

        if (reply != QStringLiteral("cancel")) {
            QStringList replyList = reply.split(QStringLiteral(":"));
            foreach (const QSharedPointer<TriggerDetail> &ptr, sameTiming) {
                if (ptr->name() == replyList.first() && (ptr->owner() == nullptr || ptr->owner()->objectName() == replyList.value(1))
                    && ptr->invoker()->objectName() == replyList.value(2)) {
                    answer = ptr;
                    break;
                }
            }
        }
        if (answer.isNull() && !cancelable)
            answer = sameTiming.value(QRandomGenerator::global()->generate() % sameTiming.length());
    }

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::TriggerOrder;
    if (!answer.isNull())
        s.args = ExtendTriggerDetail::toList(*answer);
    QVariant d = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, d);
    return answer;
}

void LegacyRoom::addPlayerHistory(LegacyServerPlayer *player, const QString &key, int times)
{
    if (player != nullptr) {
        if (key == QStringLiteral("."))
            player->clearHistory();
        else
            player->addHistory(key, times);
    }

    QJsonArray arg;
    arg << key;
    arg << times;

    if (player != nullptr)
        doNotify(player, S_COMMAND_ADD_HISTORY, arg);
    else
        doBroadcastNotify(S_COMMAND_ADD_HISTORY, arg);
}

void LegacyRoom::setPlayerFlag(LegacyServerPlayer *player, const QString &flag)
{
    if (flag.startsWith(QStringLiteral("-"))) {
        QString set_flag = flag.mid(1);
        if (!player->hasFlag(set_flag))
            return;
    }
    player->setFlag(flag);
    broadcastProperty(player, "flags", flag);
}

void LegacyRoom::setPlayerProperty(LegacyServerPlayer *player, const char *property_name, const QVariant &value)
{
#if 0 // def QT_DEBUG
    if (getThread() != player->thread()) {
        playerPropertySet = false;
        emit signalSetProperty(player, property_name, value);
        while (!playerPropertySet) {
        }
    } else {
        player->setProperty(property_name, value);
    }
#else
    player->setProperty(property_name, value);
#endif

    broadcastProperty(player, property_name);

    if (strcmp(property_name, "hp") == 0) {
        if (game_started) {
            QVariant v = QVariant::fromValue(player);
            thread->trigger(QSanguosha::HpChanged, v);
        }
    }
    if (strcmp(property_name, "maxhp") == 0) {
        if (game_started) {
            QVariant v = QVariant::fromValue(player);
            thread->trigger(QSanguosha::MaxHpChanged, v);
        }
    }
    if (strcmp(property_name, "chained") == 0) {
        if (player->isAlive())
            setEmotion(player, QStringLiteral("chain"));
        if (game_started) {
            QVariant v = QVariant::fromValue(player);
            thread->trigger(QSanguosha::ChainStateChanged, v);
        }
    }
    if (strcmp(property_name, "removed") == 0) {
        if (game_started) {
            QVariant pv = QVariant::fromValue(player);
            thread->trigger(QSanguosha::RemoveStateChanged, pv);
        }
    }
    if (strcmp(property_name, "role_shown") == 0) {
        if (game_started) {
            QVariant pv = QVariant::fromValue(player);
            thread->trigger(QSanguosha::RoleShownChanged, pv);
        }
        player->setMark(QStringLiteral("AI_RoleShown"), value.toBool() ? 1 : 0);
        roleStatusCommand(player);
        if (value.toBool())
            player->setMark(QStringLiteral("AI_RolePredicted"), 1);
    }
    if (strcmp(property_name, "kingdom") == 0) {
        if (game_started) {
            QVariant v = QVariant::fromValue(player);
            thread->trigger(QSanguosha::KingdomChanged, v);
        }
    }
}

void LegacyRoom::setPlayerMark(LegacyServerPlayer *player, const QString &mark, int value)
{
    player->setMark(mark, value);

    QJsonArray arg;
    arg << player->objectName();
    arg << mark;
    arg << value;
    doBroadcastNotify(S_COMMAND_SET_MARK, arg);
}

void LegacyRoom::addPlayerMark(LegacyServerPlayer *player, const QString &mark, int add_num)
{
    int value = player->mark(mark);
    value += add_num;
    setPlayerMark(player, mark, value);
}

void LegacyRoom::removePlayerMark(LegacyServerPlayer *player, const QString &mark, int remove_num)
{
    int value = player->mark(mark);
    if (value == 0)
        return;
    value -= remove_num;
    value = qMax(0, value);
    setPlayerMark(player, mark, value);
}

void LegacyRoom::setPlayerCardLimitation(LegacyServerPlayer *player, const QString &limit_list, const QString &pattern, const QString &reason, bool single_turn)
{
    player->setCardLimitation(limit_list, pattern, reason, single_turn);

    QJsonArray arg;
    arg << true;
    arg << limit_list;
    arg << pattern;
    arg << single_turn;
    arg << player->objectName();
    arg << reason;
    arg << false;
    doBroadcastNotify(S_COMMAND_CARD_LIMITATION, arg);
}

void LegacyRoom::removePlayerCardLimitation(LegacyServerPlayer *player, const QString &limit_list, const QString &pattern, const QString &reason, bool clearReason)
{
    player->removeCardLimitation(limit_list, pattern, reason, clearReason);

    QJsonArray arg;
    arg << false;
    arg << limit_list;
    arg << pattern;
    arg << false;
    arg << player->objectName();
    arg << reason;
    arg << clearReason;
    doBroadcastNotify(S_COMMAND_CARD_LIMITATION, arg);
}

void LegacyRoom::clearPlayerCardLimitation(LegacyServerPlayer *player, bool single_turn)
{
    player->clearCardLimitation(single_turn);

    QJsonArray arg;
    arg << true;
    arg << QJsonValue();
    arg << QJsonValue();
    arg << single_turn;
    arg << player->objectName();
    arg << QString();
    arg << true;
    doBroadcastNotify(S_COMMAND_CARD_LIMITATION, arg);
}

void LegacyRoom::setPlayerDisableShow(LegacyServerPlayer *player, const QString &flags, const QString &reason)
{
    player->setDisableShow(flags, reason);

    QJsonArray arg;
    arg << player->objectName();
    arg << true;
    arg << flags;
    arg << reason;
    doBroadcastNotify(S_COMMAND_DISABLE_SHOW, arg);
}

void LegacyRoom::removePlayerDisableShow(LegacyServerPlayer *player, const QString &reason)
{
    player->removeDisableShow(reason);

    QJsonArray arg;
    arg << player->objectName();
    arg << false;
    arg << QJsonValue();
    arg << reason;
    doBroadcastNotify(S_COMMAND_DISABLE_SHOW, arg);
}

void LegacyRoom::setCardFlag(const Card *card, const QString &flag, LegacyServerPlayer *who)
{
    if (flag.isEmpty())
        return;

    card->addFlag(flag);

    if (!card->isVirtualCard())
        setCardFlag(card->effectiveId(), flag, who);
}

void LegacyRoom::setCardFlag(int card_id, const QString &flag, LegacyServerPlayer *who)
{
    if (flag.isEmpty())
        return;

    Q_ASSERT(card(card_id) != nullptr);
    card(card_id)->addFlag(flag);

    QJsonArray arg;
    arg << card_id;
    arg << flag;
    if (who != nullptr)
        doNotify(who, S_COMMAND_CARD_FLAG, arg);
    else
        doBroadcastNotify(S_COMMAND_CARD_FLAG, arg);
}

void LegacyRoom::clearCardFlag(const Card *card, LegacyServerPlayer *who)
{
    card->clearFlags();

    if (!card->isVirtualCard())
        clearCardFlag(card->effectiveId(), who);
}

void LegacyRoom::clearCardFlag(int card_id, LegacyServerPlayer *who)
{
    Q_ASSERT(card(card_id) != nullptr);
    card(card_id)->clearFlags();

    QJsonArray arg;
    arg << card_id;
    arg << QStringLiteral(".");
    if (who != nullptr)
        doNotify(who, S_COMMAND_CARD_FLAG, arg);
    else
        doBroadcastNotify(S_COMMAND_CARD_FLAG, arg);
}

LegacyServerPlayer *LegacyRoom::addSocket(LegacyClientSocket *socket)
{
    LegacyServerPlayer *player = new LegacyServerPlayer(this);
    player->setSocket(socket);
    m_players << player;

    connect(player, &LegacyServerPlayer::disconnected, this, &LegacyRoom::reportDisconnection);
    connect(player, &LegacyServerPlayer::request_got, this, &LegacyRoom::processClientPacket);

    return player;
}

bool LegacyRoom::isFull() const
{
    return m_players.length() == player_count;
}

bool LegacyRoom::isFinished() const
{
    return game_finished;
}

bool LegacyRoom::canPause(LegacyServerPlayer *player) const
{
    if (!isFull())
        return false;
    if ((player == nullptr) /*|| !player->isOwner()*/)
        return false;
    foreach (LegacyServerPlayer *p, m_players) {
        if (!p->isAlive() /*|| p->isOwner()*/)
            continue;
        if (p->getState() != QStringLiteral("robot"))
            return false;
    }
    return true;
}

void LegacyRoom::tryPause()
{
#if 0
    if (!canPause(getOwner()))
        return;
#endif
    QMutexLocker locker(&m_mutex);
    while (game_paused)
        m_waitCond.wait(locker.mutex());
}

int LegacyRoom::getLack() const
{
    return player_count - m_players.length();
}

void LegacyRoom::broadcast(const QString &message, LegacyServerPlayer *except)
{
    foreach (LegacyServerPlayer *player, m_players) {
        if (player != except)
            player->unicast(message);
    }
}

void LegacyRoom::swapPile()
{
    if (discardPile().isEmpty()) {
        // the standoff
        gameOver(QStringLiteral("."));
    }

    int times = tag.value(QStringLiteral("SwapPile"), 0).toInt();
    tag.insert(QStringLiteral("SwapPile"), ++times);

    int limit = Config.value(QStringLiteral("PileSwappingLimitation"), 5).toInt() + 1;
    if (serverInfo()->GameMode->name() == QStringLiteral("04_1v3"))
        limit = qMin(limit, Config.BanPackages.contains(QStringLiteral("maneuvering")) ? 3 : 2);
    if (limit > 0 && times == limit)
        gameOver(QStringLiteral("."));

    qSwap(pile1, discardPile());

    doBroadcastNotify(S_COMMAND_RESET_PILE, QJsonValue());
    doBroadcastNotify(S_COMMAND_UPDATE_PILE, QJsonValue(m_drawPile->length()));

    qShuffle(*m_drawPile);
    foreach (int card_id, *m_drawPile)
        setCardMapping(card_id, nullptr, QSanguosha::PlaceDrawPile);

    QVariant qtimes;
    qtimes.setValue(times);
    thread->trigger(QSanguosha::DrawPileSwaped, qtimes);
}

LegacyServerPlayer *LegacyRoom::findPlayer(const QString &general_name, bool include_dead) const
{
    const QList<LegacyServerPlayer *> &list = getAllPlayers(include_dead);

    if (general_name.contains(QStringLiteral("+"))) {
        QStringList names = general_name.split(QStringLiteral("+"));
        foreach (LegacyServerPlayer *player, list) {
            if (names.contains(player->generalName()))
                return player;
        }
        return nullptr;
    }

    foreach (LegacyServerPlayer *player, list) {
        if (player->generalName() == general_name || player->objectName() == general_name)
            return player;
    }

    return nullptr;
}

QList<LegacyServerPlayer *> LegacyRoom::findPlayersBySkillName(const QString &skill_name, bool include_hidden) const
{
    QList<LegacyServerPlayer *> list;
    foreach (LegacyServerPlayer *player, getAllPlayers()) {
        if (player->hasValidSkill(skill_name, false, include_hidden))
            list << player;
    }
    return list;
}

LegacyServerPlayer *LegacyRoom::findPlayerBySkillName(const QString &skill_name) const
{
    foreach (LegacyServerPlayer *player, getAllPlayers()) {
        if (player->hasValidSkill(skill_name))
            return player;
    }
    return nullptr;
}

void LegacyRoom::changeHero(LegacyServerPlayer *player, const QString &new_general, bool full_state, bool /*unused*/, bool isSecondaryHero, bool sendLog)
{
    QJsonArray arg;
    arg << (int)S_GAME_EVENT_CHANGE_HERO;
    arg << player->objectName();
    arg << new_general;
    arg << isSecondaryHero;
    arg << sendLog;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

    if (isSecondaryHero)
        changePlayerGeneral2(player, new_general);
    else
        changePlayerGeneral(player, new_general);
    player->setMaxHp(player->getGeneralMaxHp());

    if (full_state)
        player->setHp(player->maxHp());
    broadcastProperty(player, "hp");
    broadcastProperty(player, "maxhp");

    const General *gen = isSecondaryHero ? player->getGeneral2() : player->general();
    if (gen != nullptr) {
        foreach (const Skill *skill, gen->skills()) {
            loadSkill(skill);
            foreach (const Trigger *trigger, skill->triggers())
                thread->addTrigger(trigger);
            // I'd like to remove following logic....
            //                if (invokeStart && trigger->triggerEvents().contains(QSanguosha::GameStart))
            //                    game_start = true;

            if (skill->isLimited() && !skill->limitMark().isEmpty())
                setPlayerMark(player, skill->limitMark(), 1);
            SkillAcquireLoseStruct s;
            s.isAcquire = true;
            s.player = player;
            s.skill = skill;
            QVariant _skillobjectName = QVariant::fromValue(s);
            thread->trigger(QSanguosha::EventAcquireSkill, _skillobjectName);
        }
    }
}

void LegacyRoom::setFixedDistance(Player *from, const Player *to, int distance)
{
    from->setFixedDistance(to, distance);

    QJsonArray arg;
    arg << from->objectName();
    arg << to->objectName();
    arg << distance;
    doBroadcastNotify(S_COMMAND_FIXED_DISTANCE, arg);
}

void LegacyRoom::reverseFor3v3(const Card *card, LegacyServerPlayer *player, QList<LegacyServerPlayer *> &list)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_DIRECTION);

    bool isClockwise = false;
    if (player->getState() == QStringLiteral("online")) {
        bool success = doRequest(player, S_COMMAND_CHOOSE_DIRECTION, QJsonValue(), true);
        QJsonValue clientReply = player->getClientReply();
        if (success && QSgsJsonUtils::isString(clientReply))
            isClockwise = (clientReply.toString() == QStringLiteral("cw"));
    } else
        isClockwise = true;

    LogStruct log;
    log.type = QStringLiteral("#TrickDirection");
    log.from = player;
    log.arg = isClockwise ? QStringLiteral("cw") : QStringLiteral("ccw");
    log.arg2 = card->faceName();
    sendLog(log);

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::ReverseFor3v3;
    s.args << card->toString() << (isClockwise ? QStringLiteral("cw") : QStringLiteral("ccw"));
    QVariant d = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, d);

    if (isClockwise) {
        QList<LegacyServerPlayer *> new_list;

        while (!list.isEmpty())
            new_list << list.takeLast();

        if (card->face()->isKindOf(QStringLiteral("GlobalEffect"))) {
            new_list.removeLast();
            new_list.prepend(player);
        }

        list = new_list;
    }
}

int LegacyRoom::drawCard(bool bottom)
{
    QVariant v;
    thread->trigger(QSanguosha::FetchDrawPileCard, v);
    if (m_drawPile->isEmpty())
        swapPile();
    int id = -1;
    if (!bottom)
        id = m_drawPile->takeFirst();
    else
        id = m_drawPile->takeLast();

    return id;
}

void LegacyRoom::prepareForStart()
{
    if (serverInfo()->GameMode->name() == QStringLiteral("06_3v3") || serverInfo()->GameMode->name() == QStringLiteral("06_XMode")
        || serverInfo()->GameMode->name() == QStringLiteral("02_1v1")) {
        return;
    } else if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony) {
        if (!serverInfo()->isMultiGeneralEnabled())
            assignRoles();
        if (Config.RandomSeat)
            qShuffle(m_players);
    } else if (Config.EnableCheat && Config.value(QStringLiteral("FreeAssign"), false).toBool()) {
        // TODO: reconsider this
#if 0
        LegacyServerPlayer *owner = getOwner();
        notifyMoveFocus(owner, S_COMMAND_CHOOSE_ROLE);
        if ((owner != nullptr) && owner->getState() == QStringLiteral("online")) {
            bool success = doRequest(owner, S_COMMAND_CHOOSE_ROLE, QJsonValue(), true);
            QJsonValue clientReply = owner->getClientReply();
            if (!success || !clientReply.isArray() || clientReply.toArray().size() != 2) {
                if (Config.RandomSeat)
                    qShuffle(m_players);
                assignRoles();
            } else if (Config.FreeAssignSelf) {
                QJsonArray replyArray = clientReply.toArray();
                QString name = replyArray.at(0).toArray().at(0).toString();
                QString role = replyArray.at(1).toArray().at(0).toString();
                LegacyServerPlayer *player_self = findChild<LegacyServerPlayer *>(name);
                setPlayerProperty(player_self, "role", role);

                QList<LegacyServerPlayer *> all_players = m_players;
                all_players.removeOne(player_self);
                int n = all_players.count();
                QStringList roles = Sanguosha->getRoleList(serverInfo()->GameMode->name());
                roles.removeOne(role);
                qShuffle(roles);

                for (int i = 0; i < n; i++) {
                    LegacyServerPlayer *player = all_players[i];
                    const QString &role = roles.at(i);

                    player->setRole(role);
                    if (role == QStringLiteral("lord")) {
                        broadcastProperty(player, "role", QStringLiteral("lord"));
                        setPlayerProperty(player, "role_shown", true);
                    } else {
                        if (serverInfo()->GameMode->name() == QStringLiteral("04_1v3")) {
                            broadcastProperty(player, "role", role);
                            setPlayerProperty(player, "role_shown", true);
                        } else
                            notifyProperty(player, player, "role");
                    }
                }
            } else {
                QJsonArray replyArray = clientReply.toArray();
                for (int i = 0; i < replyArray.at(0).toArray().size(); i++) {
                    QString name = replyArray.at(0).toArray().at(i).toString();
                    QString role = replyArray.at(1).toArray().at(i).toString();

                    LegacyServerPlayer *player = findChild<LegacyServerPlayer *>(name);
                    setPlayerProperty(player, "role", role);

                    m_players.swapItemsAt(i, m_players.indexOf(player));
                }
            }
        } else if (serverInfo()->GameMode->name() == QStringLiteral("04_1v3")) {
            if (Config.RandomSeat)
                qShuffle(m_players);
            LegacyServerPlayer *lord = m_players.at(QRandomGenerator::global()->generate() % 4);
            for (int i = 0; i < 4; i++) {
                LegacyServerPlayer *player = m_players.at(i);
                if (player == lord)
                    player->setRole(QStringLiteral("lord"));
                else
                    player->setRole(QStringLiteral("rebel"));
                broadcastProperty(player, "role");
                setPlayerProperty(player, "role_shown", true);
            }
        } else
#endif
        {
            if (Config.RandomSeat)
                qShuffle(m_players);
            assignRoles();
        }
    } else {
        if (Config.RandomSeat)
            qShuffle(m_players);
        assignRoles();
    }

    adjustSeats();
}

void LegacyRoom::reportDisconnection()
{
    LegacyServerPlayer *player = qobject_cast<LegacyServerPlayer *>(sender());
    if (player == nullptr)
        return;

    // send disconnection message to server log
    emit room_message(player->reportHeader() + tr("disconnected"));

    // the 4 kinds of circumstances
    // 1. Just connected, with no object name : just remove it from player list
    // 2. Connected, with an object name : remove it, tell other clients and decrease signup_count
    // 3. Game is not started, but role is assigned, give it the default general(general2) and others same with fourth case
    // 4. Game is started, do not remove it just set its state as offline
    // all above should set its socket to NULL

    player->setSocket(nullptr);

    if (player->objectName().isEmpty()) {
        // first case
        player->setParent(nullptr);
        m_players.removeOne(player);
    } else if (player->roleString().isEmpty()) {
        // second case
        if (m_players.length() < player_count) {
            player->setParent(nullptr);
            m_players.removeOne(player);

            if (player->getState() != QStringLiteral("robot")) {
                QString screen_name = player->screenName();
                QString leaveStr = tr("<font color=#000000>Player <b>%1</b> left the game</font>").arg(screen_name);
                speakCommand(player, QString::fromLatin1(leaveStr.toUtf8().toBase64()));
            }

            doBroadcastNotify(S_COMMAND_REMOVE_PLAYER, player->objectName());
        }
    } else {
        // fourth case
        if (player->m_isWaitingReply) {
            if (_m_raceStarted) {
                // copied from processResponse about race request
                _m_raceWinner.storeRelaxed(player);
                _m_semRaceRequest.release();
            } else
                player->releaseLock(LegacyServerPlayer::SEMA_COMMAND_INTERACTIVE);
        }
        setPlayerProperty(player, "state", QStringLiteral("offline"));

        bool someone_is_online = false;
        foreach (LegacyServerPlayer *player, m_players) {
            if (player->getState() == QStringLiteral("online") || player->getState() == QStringLiteral("trust")) {
                someone_is_online = true;
                break;
            }
        }

        if (!someone_is_online) {
            game_finished = true;
            emit game_over(QString());
            return;
        }
    }

    //    if (player->isOwner()) {
    //        player->setOwner(false);
    //        broadcastProperty(player, "owner");
    //        foreach (ServerPlayer *p, m_players) {
    //            if (p->getState() == QStringLiteral("online")) {
    //                p->setOwner(true);
    //                broadcastProperty(p, "owner");
    //                break;
    //            }
    //        }
    //    }
}

void LegacyRoom::trustCommand(LegacyServerPlayer *player, const QJsonValue & /*unused*/)
{
    player->acquireLock(LegacyServerPlayer::SEMA_MUTEX);
    if (player->getState() == QStringLiteral("online")) {
        player->setState(QStringLiteral("trust"));
        if (player->m_isWaitingReply) {
            player->releaseLock(LegacyServerPlayer::SEMA_MUTEX);
            if (_m_raceStarted) {
                // copied from processResponse about race request
                _m_raceWinner.storeRelaxed(player);
                _m_semRaceRequest.release();
            } else
                player->releaseLock(LegacyServerPlayer::SEMA_COMMAND_INTERACTIVE);
        }
    } else
        player->setState(QStringLiteral("online"));

    player->releaseLock(LegacyServerPlayer::SEMA_MUTEX);
    broadcastProperty(player, "state");
}

void LegacyRoom::pauseCommand(LegacyServerPlayer *player, const QJsonValue &arg)
{
    if (!canPause(player))
        return;
    bool pause = (arg.toString() == QStringLiteral("true"));
    QMutexLocker locker(&m_mutex);
    if (game_paused != pause) {
        QJsonArray arg;
        arg << (int)S_GAME_EVENT_PAUSE;
        arg << pause;
        doNotify(player, S_COMMAND_LOG_EVENT, arg);

        game_paused = pause;
        if (!game_paused)
            m_waitCond.wakeAll();
    }
}

void LegacyRoom::cheat(LegacyServerPlayer *player, const QJsonValue &args)
{
    player->m_cheatArgs = QJsonValue();
    if (!Config.EnableCheat)
        return;
    if (!args.isArray() || !args.toArray().at(0).isDouble())
        return;

    player->m_cheatArgs = args;
    makeCheat(player);
}

bool LegacyRoom::makeSurrender(LegacyServerPlayer *initiator)
{
    bool loyalGiveup = true;
    int loyalAlive = 0;
    bool renegadeGiveup = true;
    int renegadeAlive = 0;
    bool rebelGiveup = true;
    int rebelAlive = 0;

    // broadcast polling request
    QList<LegacyServerPlayer *> playersAlive;
    foreach (LegacyServerPlayer *player, m_players) {
        QString playerRole = player->roleString();
        if (serverInfo()->GameMode->category() == QSanguosha::ModeRole) {
            if ((playerRole == QStringLiteral("loyalist") || playerRole == QStringLiteral("lord")) && player->isAlive())
                loyalAlive++;
            else if (playerRole == QStringLiteral("rebel") && player->isAlive())
                rebelAlive++;
            else if (playerRole == QStringLiteral("renegade") && player->isAlive())
                renegadeAlive++;
        }

        if (player != initiator && player->isAlive() && player->getState() == QStringLiteral("online")) {
            player->m_commandArgs = (initiator->general()->name());
            playersAlive << player;
        }
    }
    doBroadcastRequest(playersAlive, S_COMMAND_SURRENDER);

    // collect polls
    int hegemony_give_up = 1;
    foreach (LegacyServerPlayer *player, playersAlive) {
        bool result = false;
        if (!player->m_isClientResponseReady || !player->getClientReply().isBool())
            result = player->getState() != QStringLiteral("online");
        else
            result = player->getClientReply().toBool();

        if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony) {
            if (result)
                hegemony_give_up++;
        } else {
            QString playerRole = player->roleString();
            if (playerRole == QStringLiteral("loyalist") || playerRole == QStringLiteral("lord")) {
                loyalGiveup &= result;
                if (player->isAlive())
                    loyalAlive++;
            } else if (playerRole == QStringLiteral("rebel")) {
                rebelGiveup &= result;
                if (player->isAlive())
                    rebelAlive++;
            } else if (playerRole == QStringLiteral("renegade")) {
                renegadeGiveup &= result;
                if (player->isAlive())
                    renegadeAlive++;
            }
        }
    }

    // vote counting
    if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony) {
        if (hegemony_give_up > (playersAlive.length() + 1) / 2) {
            foreach (LegacyServerPlayer *p, getAllPlayers()) {
                if (!p->haveShownGeneral())
                    p->showGeneral(true, false, false);
                if ((p->getGeneral2() != nullptr) && !p->hasShownGeneral2())
                    p->showGeneral(false, false, false);
            }
            gameOver(QStringLiteral("."), true);
        }

    } else {
        if (loyalGiveup && renegadeGiveup && !rebelGiveup)
            gameOver(QStringLiteral("rebel"), true);
        else if (loyalGiveup && !renegadeGiveup && rebelGiveup)
            gameOver(QStringLiteral("renegade"), true);
        else if (!loyalGiveup && renegadeGiveup && rebelGiveup)
            gameOver(QStringLiteral("lord+loyalist"), true);
        else if (loyalGiveup && renegadeGiveup && rebelGiveup) {
            // if everyone give up, then ensure that the initiator doesn't win.
            QString playerRole = initiator->roleString();
            if (playerRole == QStringLiteral("lord") || playerRole == QStringLiteral("loyalist"))
                gameOver(renegadeAlive >= rebelAlive ? QStringLiteral("renegade") : QStringLiteral("rebel"), true);
            else if (playerRole == QStringLiteral("renegade"))
                gameOver(loyalAlive >= rebelAlive ? QStringLiteral("loyalist+lord") : QStringLiteral("rebel"), true);
            else if (playerRole == QStringLiteral("rebel"))
                gameOver(renegadeAlive >= loyalAlive ? QStringLiteral("renegade") : QStringLiteral("loyalist+lord"), true);
        }
    }

    initiator->setFlag(QStringLiteral("Global_ForbidSurrender"));
    doNotify(initiator, S_COMMAND_ENABLE_SURRENDER, QJsonValue(false));
    return true;
}

void LegacyRoom::processRequestPreshow(LegacyServerPlayer *player, const QJsonValue &arg)
{
    if (player == nullptr)
        return;

    QJsonArray args = arg.toArray();
    if (args.size() != 2 || !QSgsJsonUtils::isString(args[0]) || !QSgsJsonUtils::isBool(args[1]))
        return;
    player->acquireLock(LegacyServerPlayer::SEMA_MUTEX);

    const QString skill_name = args[0].toString();
    const bool isPreshowed = args[1].toBool();
    player->setSkillPreshowed(skill_name, isPreshowed);

    player->releaseLock(LegacyServerPlayer::SEMA_MUTEX);
}

void LegacyRoom::processClientPacket(const QString &request)
{
    Packet packet;
    if (packet.parse(request.toLatin1().constData())) {
        LegacyServerPlayer *player = qobject_cast<LegacyServerPlayer *>(sender());
#ifdef LOGNETWORK
        emit Sanguosha->logNetworkMessage("recv " + player->objectName() + ":" + request);
#endif // LOGNETWORK
        if (game_finished) {
            if ((player != nullptr) && player->getState() == QStringLiteral("online"))
                doNotify(player, S_COMMAND_WARN, QStringLiteral("GAME_OVER"));
            return;
        }
        if (packet.type() == S_TYPE_REPLY) {
            if (player == nullptr)
                return;
            player->setClientReply(request);
            processResponse(player, &packet);
        } else if (packet.type() == S_TYPE_REQUEST || packet.type() == S_TYPE_NOTIFICATION) {
            Callback callback = m_callbacks[packet.commandType()];
            if (callback == nullptr)
                return;
            (this->*callback)(player, packet.messageBody());
        }
    }
}

void LegacyRoom::addRobotCommand(LegacyServerPlayer *player, const QJsonValue & /*unused*/)
{
    if ((player != nullptr) /*&& !player->isOwner()*/)
        return;
    if (isFull())
        return;

    if (Config.LimitRobot && !fill_robot) {
        speakCommand(nullptr, QString::fromUtf8(tr("This server is limited to add robot. YOU CAN ONLY ADD ROBOT USING \"Fill Robots\".").toUtf8().toBase64()));
        return;
    }

    QStringList names = Sanguosha->configuration(QStringLiteral("robot_names")).toStringList();
    qShuffle(names);

    int n = 0;
    foreach (LegacyServerPlayer *player, m_players) {
        if (player->getState() == QStringLiteral("robot")) {
            QString screenname = player->screenName();
            if (names.contains(screenname))
                names.removeAll(screenname);
            else
                n++;
        }
    }

    LegacyServerPlayer *robot = new LegacyServerPlayer(this);
    robot->setState(QStringLiteral("robot"));

    m_players << robot;

    const QString robot_name = names.isEmpty() ? tr("Computer %1").arg(QLatin1Char('A' + n)) : names.first();
    const QString robot_avatar = Sanguosha->generalNames().values().value(std::random_device()() % Sanguosha->generalNames().count());
    signup(robot, robot_name, robot_avatar, true);

    QString greeting = QString::fromUtf8(tr("Hello, I'm a robot").toUtf8().toBase64());
    speakCommand(robot, greeting);

    broadcastProperty(robot, "state");
}

void LegacyRoom::fillRobotsCommand(LegacyServerPlayer *player, const QJsonValue & /*unused*/)
{
    if (Config.LimitRobot && (m_players.length() <= player_count / 2 || player_count <= 4)) {
        speakCommand(
            nullptr,
            QString::fromUtf8(tr("This server is limited to add robot. Please ensure that the number of players is more than 4 and there is more than a half human players.")
                                  .toUtf8()
                                  .toBase64()));
        return;
    }
    fill_robot = true;
    int left = player_count - m_players.length();
    for (int i = 0; i < left; i++)
        addRobotCommand(player, QJsonValue());
}

void LegacyRoom::toggleReadyCommand(LegacyServerPlayer *, const QJsonValue & /*unused*/)
{
    if (!game_started2 && isFull()) {
        game_started2 = true;
        thread = new RoomThread(this);
        thread->start();
    }
}

void LegacyRoom::signup(LegacyServerPlayer *player, const QString &screen_name, const QString &avatar, bool is_robot)
{
    player->setObjectName(generatePlayerName());
    player->setProperty("avatar", avatar);
    player->setScreenName(screen_name);

    if (!is_robot) {
        // TODO: since MG_SELF is removed, this notify is no longer usable.
        // There should be another method to send this object name and remove the coupling in notifyProperty (such as send the player name through setup string)
        notifyProperty(player, player, "objectName");
#if 0
        LegacyServerPlayer *owner = getOwner();
        if (owner == nullptr) {
            player->setOwner(true);
            notifyProperty(player, player, "owner");
        }
#endif
    }

    // introduce the new joined player to existing players except himself
    player->introduceTo(nullptr);
    if (!is_robot) {
        QString plus = QString(QStringLiteral("<font color=red>(%1)</font>")).arg(QString::number(player->ipv4Address() / 256, 16).toUpper());
        QString greetingStr = tr("<font color=#EEB422>Player <b>%1</b> joined the game</font>").arg(screen_name + plus);
        speakCommand(player, QString::fromLatin1(greetingStr.toUtf8().toBase64()));
        player->startNetworkDelayTest();

        // introduce all existing player to the new joined
        foreach (LegacyServerPlayer *p, m_players) {
            if (p != player)
                p->introduceTo(player);
        }
    } else
        toggleReadyCommand(player, QString());
}

void LegacyRoom::assignGeneralsForPlayers(const QList<LegacyServerPlayer *> &to_assign)
{
    QSet<const General *> existed;
    foreach (LegacyServerPlayer *player, m_players)
        existed.unite(List2Set(player->generals()));

    QSet<const General *> generals = serverInfo()->GameMode->availableGenerals();

    int max_choice = Config.value(QStringLiteral("MaxChoice"), 6).toInt();
    int total = generals.count();
    int max_available = (total - existed.size()) / to_assign.length();
    int choice_count = qMin(max_choice, max_available);

    QSet<const General *> choices = generals;
    choices.subtract(existed);

    // QStringList choices = Sanguosha->getRandomGenerals(total - existed.count(), existed).values();
    // QStringList latest = Sanguosha->latestGenerals().values();

    QSet<QString> latestNames = Sanguosha->latestGeneralNames();
    QSet<const General *> latest;
    foreach (const QString &name, latestNames)
        latest << Sanguosha->general(name);

    latest.subtract(existed);

    bool assign_latest_general = Config.value(QStringLiteral("AssignLatestGeneral"), true).toBool() && serverInfo()->GameMode->category() == QSanguosha::ModeRole;
    foreach (LegacyServerPlayer *player, to_assign) {
        player->clearSelected();
        int i = 0;
        if (assign_latest_general && !latest.empty()) {
            const General *choice = player->findReasonable(latest, true);
            if (choice != nullptr) {
                player->addToSelected(choice->name());
                latest.remove(choice);
                i++;
                if (choices.contains(choice))
                    choices.remove(choice);
            }
        }

        for (; i < choice_count; i++) {
            const General *choice = player->findReasonable(choices, true);
            if (choice == nullptr)
                break;
            player->addToSelected(choice->name());
            choices.remove(choice);
            if (latest.contains(choice))
                latest.remove(choice);
        }
    }
}

#if 0

QSet<QString> Engine::getRandomLords() const
{


    QStringList banlist_ban;
    QStringList lords;
    QStringList splords_package; //lords  in sp package will be not count as a lord.
    splords_package << QStringLiteral("thndj");

    foreach (QString alord, availableLords()) {
        if (banlist_ban.contains(alord))
            continue;
        const General *theGeneral = general(alord);
        if (splords_package.contains(theGeneral->getPackage()))
            continue;
        lords << alord;
    }

    // todo: make this variable in serverinfo
    int lord_num = 6; // Config.value(QStringLiteral("LordMaxChoice"), 6).toInt();
    if (lord_num != -1 && lord_num < lords.length()) {
        int to_remove = lords.length() - lord_num;
        for (int i = 0; i < to_remove; i++) {
            lords.removeAt(QRandomGenerator::global()->generate() % lords.length());
        }
    }

    QStringList nonlord_list;
    foreach (QString nonlord, d->generals.keys()) {
        if (isGeneralHidden(nonlord))
            continue;
        const General *general = d->generals.value(nonlord);
        if (d->lord_list.contains(nonlord)) {
            if (!splords_package.contains(general->getPackage()))
                continue;
        }
        if (getBanPackages().contains(general->getPackage()))
            continue;
        if (banlist_ban.contains(general->name()))
            continue;

        nonlord_list << nonlord;
    }

    qShuffle(nonlord_list);

    int addcount = 0;
    int extra = 6; // Config.value(QStringLiteral("NonLordMaxChoice"), 6).toInt();

    int godmax = 1; // Config.value(QStringLiteral("GodLimit"), 1).toInt();
    int godCount = 0;

    if (lord_num == 0 && extra == 0)
        extra = 1;
    bool assign_latest_general = false; // Config.value(QStringLiteral("AssignLatestGeneral"), true).toBool();
    QStringList latest = latestGenerals(QSet<QString>(lords.begin(), lords.end()));
    if (assign_latest_general && !latest.isEmpty()) {
        lords << latest.first();
        if (nonlord_list.contains(latest.first()))
            nonlord_list.removeOne(latest.first());
        extra--;
    }
    for (int i = 0; addcount < extra; i++) {
        if (general(nonlord_list.at(i))->kingdom() != QStringLiteral("touhougod")) {
            lords << nonlord_list.at(i);
            addcount++;
        } else if (godmax > 0 && godCount < godmax) {
            lords << nonlord_list.at(i);
            godCount++;
            addcount++;
        }

        if (i == nonlord_list.length() - 1)
            break;
    }

    return lords;
}

QSet<QString> Engine::getRandomGenerals(int count, const QSet<QString> &ban_set) const
{
    // TODO: reimplement this function in separated class Mode
    QStringList all_generals = getLimitedGeneralNames();
    QSet<QString> general_set = QSet<QString>(all_generals.begin(), all_generals.end());

    Q_ASSERT(all_generals.count() >= count);

    QStringList subtractList;
    bool needsubtract = true;
#if 0
    if (isRoleGameMode(serverInfo()->GameMode))
        subtractList = (Config.value(QStringLiteral("Banlist/Roles"), QStringList()).toStringList());
    else if (serverInfo()->GameMode == QStringLiteral("04_1v3"))
        subtractList = (Config.value(QStringLiteral("Banlist/HulaoPass"), QStringList()).toStringList());
    else if (serverInfo()->GameMode == QStringLiteral("06_XMode"))
        subtractList = (Config.value(QStringLiteral("Banlist/XMode"), QStringList()).toStringList());
    else if (isHegemonyGameMode(serverInfo()->GameMode))
        subtractList = (Config.value(QStringLiteral("Banlist/Hegemony"), QStringList()).toStringList());
    else
#endif
    needsubtract = false;

    if (needsubtract)
        general_set.subtract(QSet<QString>(subtractList.begin(), subtractList.end()));

    all_generals = general_set.subtract(ban_set).values();

    // shuffle them
    qShuffle(all_generals);

    int addcount = 0;
    QSet<QString> general_list;
    int godmax = 1; // Config.value(QStringLiteral("GodLimit"), 1).toInt();
    int godCount = 0;
    for (int i = 0; addcount < count; i++) {
        if (general(all_generals.at(i))->kingdom() != QStringLiteral("touhougod")) {
            general_list << all_generals.at(i);
            addcount++;
        } else if (godmax > 0 && godCount < godmax) {
            general_list << all_generals.at(i);
            godCount++;
            addcount++;
        }
        if (i == all_generals.count() - 1)
            break;
    }

    return general_list;
}

#endif

void LegacyRoom::chooseGenerals()
{
    QStringList ban_list = Config.value(QStringLiteral("Banlist/Roles")).toStringList();
    // for lord.
    int lord_num = Config.value(QStringLiteral("LordMaxChoice"), 6).toInt();
    int nonlord_num = Config.value(QStringLiteral("NonLordMaxChoice"), 6).toInt();
    if (lord_num == 0 && nonlord_num == 0)
        nonlord_num = 1;
    int nonlord_prob = (lord_num == -1) ? 5 : 55 - qMin(lord_num, 10);
    QSet<QString> lord_list;
    LegacyServerPlayer *the_lord = getLord();

    // Function used Engine::getRandomGenerals
    if (Config.EnableSame) {
        int choiceNum = Config.value(QStringLiteral("MaxChoice"), 6).toInt();
        QList<const General *> generals = serverInfo()->GameMode->availableGenerals().values();
        qShuffle(generals, choiceNum);
        for (int i = 0; i < choiceNum; ++i)
            lord_list << generals.value(i)->name();
    } else if (the_lord->getState() == QStringLiteral("robot")) {
        int ramdom_value = QRandomGenerator::global()->generate() % 100;
        if (((ramdom_value < nonlord_prob || lord_num == 0) && nonlord_num > 0) || Sanguosha->availableLordNames().count() == 0) {
            QList<const General *> generals = serverInfo()->GameMode->availableGenerals().values();
            qShuffle(generals, 1);
            lord_list << generals.first()->name();
        } else {
            lord_list = Sanguosha->availableLordNames();
        }
    } else {
        // lord_list = Sanguosha->getRandomLords();
        int lordChoiceNum = Config.value(QStringLiteral("LordMaxChoice"), 6).toInt();
        QStringList values = Sanguosha->availableLordNames().values();
        qShuffle(values, lordChoiceNum);
        lord_list = List2Set(values.mid(0, lordChoiceNum));
    }
    QString general = askForGeneral(the_lord, lord_list.values());
    the_lord->setGeneral(Sanguosha->general(general));
    broadcastProperty(the_lord, "general", general);

    if (Config.EnableSame) {
        foreach (LegacyServerPlayer *p, m_players) {
            if (!p->isLord())
                p->setGeneral(Sanguosha->general(general));
        }

        Config.Enable2ndGeneral = false;
        return;
    }
    //for others without lord
    QList<LegacyServerPlayer *> to_assign = m_players;
    to_assign.removeOne(getLord());

    assignGeneralsForPlayers(to_assign);
    foreach (LegacyServerPlayer *player, to_assign)
        _setupChooseGeneralRequestArgs(player);

    doBroadcastRequest(to_assign, S_COMMAND_CHOOSE_GENERAL);
    foreach (LegacyServerPlayer *player, to_assign) {
        if (player->general() != nullptr)
            continue;
        QString generalName = player->getClientReply().toString();
        if (!player->m_isClientResponseReady || !_setPlayerGeneral(player, generalName, true))
            _setPlayerGeneral(player, _chooseDefaultGeneral(player), true);
    }

    if (Config.Enable2ndGeneral) {
        QList<LegacyServerPlayer *> to_assign = m_players;
        assignGeneralsForPlayers(to_assign);
        foreach (LegacyServerPlayer *player, to_assign)
            _setupChooseGeneralRequestArgs(player);

        doBroadcastRequest(to_assign, S_COMMAND_CHOOSE_GENERAL);
        foreach (LegacyServerPlayer *player, to_assign) {
            if (player->getGeneral2() != nullptr)
                continue;
            QString generalName = player->getClientReply().toString();
            if (!player->m_isClientResponseReady || !_setPlayerGeneral(player, generalName, false))
                _setPlayerGeneral(player, _chooseDefaultGeneral(player), false);
        }
    }

    Config.setValue(QStringLiteral("Banlist/Roles"), ban_list);
}

void LegacyRoom::chooseHegemonyGenerals()
{
    QStringList ban_list = Config.value(QStringLiteral("Banlist/Roles")).toStringList();
    QList<LegacyServerPlayer *> to_assign = m_players;

    assignGeneralsForPlayers(to_assign);
    foreach (LegacyServerPlayer *player, to_assign)
        _setupChooseGeneralRequestArgs(player);

    doBroadcastRequest(to_assign, S_COMMAND_CHOOSE_GENERAL);

    if (!serverInfo()->isMultiGeneralEnabled()) {
        foreach (LegacyServerPlayer *player, to_assign) {
            if (player->general() != nullptr)
                continue;
            QString generalName = player->getClientReply().toString();
            if (!player->m_isClientResponseReady || !_setPlayerGeneral(player, generalName, true))
                _setPlayerGeneral(player, _chooseDefaultGeneral(player), true);
        }
    } else {
        foreach (LegacyServerPlayer *player, to_assign) {
            if (player->general() != nullptr)
                continue;
            const QJsonValue &generalName = player->getClientReply();
            if (!player->m_isClientResponseReady || !QSgsJsonUtils::isString(generalName)) {
                QStringList default_generals = _chooseDefaultGenerals(player);
                _setPlayerGeneral(player, default_generals.first(), true);
                _setPlayerGeneral(player, default_generals.last(), false);
            } else {
                QStringList generals = generalName.toString().split(QStringLiteral("+"));
                if (generals.length() != 2 || !_setPlayerGeneral(player, generals.first(), true) || !_setPlayerGeneral(player, generals.last(), false)) {
                    QStringList default_generals = _chooseDefaultGenerals(player);
                    _setPlayerGeneral(player, default_generals.first(), true);
                    _setPlayerGeneral(player, default_generals.last(), false);
                }
            }
        }
    }

    //@todo
    foreach (LegacyServerPlayer *player, m_players) {
        QStringList names;
        if (player->general() != nullptr) {
            QString name = player->generalName();
            QString role = Sanguosha->general(name)->kingdom();
            if (role == QStringLiteral("zhu"))
                role = QStringLiteral("careerist");
            names.append(name);
            player->setRole(role);
            player->setGeneral(Sanguosha->general(QStringLiteral("anjiang")));
            foreach (LegacyServerPlayer *p, getOtherPlayers(player))
                notifyProperty(p, player, "general");
            notifyProperty(player, player, "general", name);
            notifyProperty(player, player, "role", role);
        }
        if (player->getGeneral2() != nullptr) {
            QString name = player->getGeneral2Name();
            names.append(name);
            player->setGeneral(Sanguosha->general(QStringLiteral("anjiang")), 1);
            foreach (LegacyServerPlayer *p, getOtherPlayers(player))
                notifyProperty(p, player, "general2");
            notifyProperty(player, player, "general2", name);

            QString role = Sanguosha->general(name)->kingdom();
            if (role == QStringLiteral("zhu")) {
                role = QStringLiteral("careerist");
                player->setRole(role);
                notifyProperty(player, player, "role", role);
            }
        }

        this->setTag(player->objectName(), QVariant::fromValue(names));
    }

    Config.setValue(QStringLiteral("Banlist/Roles"), ban_list);
}

void LegacyRoom::assignRoles()
{
    int n = m_players.count();

    QStringList roles = Sanguosha->getRoleList(serverInfo()->GameMode->name());
    qShuffle(roles);

    for (int i = 0; i < n; i++) {
        LegacyServerPlayer *player = m_players[i];
        const QString &role = roles.at(i);

        player->setRole(role);
        if (role == QStringLiteral("lord")) {
            broadcastProperty(player, "role", QStringLiteral("lord"));
            setPlayerProperty(player, "role_shown", true);
        } else
            notifyProperty(player, player, "role");
    }
}

void LegacyRoom::swapSeat(LegacyServerPlayer *a, LegacyServerPlayer *b)
{
    int seat1 = m_players.indexOf(a);
    int seat2 = m_players.indexOf(b);

    m_players.swapItemsAt(seat1, seat2);

    QStringList player_circle;
    foreach (LegacyServerPlayer *player, m_players)
        player_circle << player->objectName();
    doBroadcastNotify(S_COMMAND_ARRANGE_SEATS, QSgsJsonUtils::toJsonArray(player_circle));

    for (int i = 0; i < m_players.length(); i++) {
        LegacyServerPlayer *player = m_players.at(i);
        player->setSeat(i);

        broadcastProperty(player, "seat");
        broadcastProperty(player, "next");
    }
}

void LegacyRoom::setPlayerSkillInvalidity(LegacyServerPlayer *player, const Skill *skill, bool invalidity, bool trigger_event)
{
    QString skill_name = QStringLiteral("_ALL_SKILLS");
    if (skill != nullptr)
        skill_name = skill->name();

    setPlayerSkillInvalidity(player, skill_name, invalidity, trigger_event);
}

void LegacyRoom::setPlayerSkillInvalidity(LegacyServerPlayer *player, const QString &skill_name, bool invalidity, bool trigger_event)
{
    player->setSkillInvalidity(skill_name, invalidity);

    QJsonArray arr;
    arr << player->objectName() << skill_name << invalidity;
    doBroadcastNotify(QSanProtocol::S_COMMAND_SET_SKILL_INVALIDITY, arr);

    foreach (LegacyServerPlayer *p, getAllPlayers())
        filterCards(p, p->getCards(QStringLiteral("hes")), true);

    QJsonArray args;
    args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

    if (trigger_event) {
        QList<SkillInvalidStruct> invalid_list;
        //how to deal skill_name == "_ALL_SKILLS"??
        SkillInvalidStruct invalid;
        invalid.invalid = invalidity;
        invalid.player = player;
        invalid.skill = Sanguosha->skill(skill_name);
        invalid_list << invalid;

        QVariant v = QVariant::fromValue(invalid_list);
        thread->trigger(QSanguosha::EventSkillInvalidityChange, v);
    }
}

void LegacyRoom::adjustSeats()
{
    QList<LegacyServerPlayer *> players;
    int i = 0;
    for (i = 0; i < m_players.length(); i++) {
        if (m_players.at(i)->role() == QSanguosha::RoleLord)
            break;
    }
    for (int j = i; j < m_players.length(); j++)
        players << m_players.at(j);
    for (int j = 0; j < i; j++)
        players << m_players.at(j);

    m_players = players;

    for (int i = 0; i < m_players.length(); i++)
        m_players.at(i)->setSeat(i + 1);

    // tell the players about the seat, and the first is always the lord
    QStringList player_circle;
    foreach (LegacyServerPlayer *player, m_players)
        player_circle << player->objectName();

    doBroadcastNotify(S_COMMAND_ARRANGE_SEATS, QSgsJsonUtils::toJsonArray(player_circle));
}

QString LegacyRoom::_chooseDefaultGeneral(LegacyServerPlayer *player) const
{
    Q_ASSERT(!player->getSelected().isEmpty());
    QString choice = player->getSelected().first();
    return choice;
}

QStringList LegacyRoom::_chooseDefaultGenerals(LegacyServerPlayer *player) const
{
    Q_ASSERT(!player->getSelected().isEmpty());
    QStringList generals;
    QStringList candidates = player->getSelected();

    QStringList general_pairs;
    //temp AI
    foreach (QString a, candidates) {
        foreach (QString b, candidates) {
            if (a == b)
                continue;
            const General *general1 = Sanguosha->general(a);
            const General *general2 = Sanguosha->general(b);
            if ((general1->kingdom() == general2->kingdom()) || (general1->kingdom() == QStringLiteral("zhu")) || (general2->kingdom() == QStringLiteral("zhu")))
                general_pairs << (a + QStringLiteral("+") + b);
        }
    }
    int index = QRandomGenerator::global()->generate() % general_pairs.length();
    generals = general_pairs[index].split(QStringLiteral("+"));
    return generals;
}

bool LegacyRoom::_setPlayerGeneral(LegacyServerPlayer *player, const QString &generalName, bool isFirst)
{
    const General *general = Sanguosha->general(generalName);
    if (general == nullptr)
        return false;
    else if (!Config.FreeChoose && !player->getSelected().contains(generalName))
        return false;

    if (isFirst) {
        player->setGeneral(general);
        notifyProperty(player, player, "general");
    } else {
        player->setGeneral(general, 1);
        notifyProperty(player, player, "general2");
    }
    return true;
}

void LegacyRoom::speakCommand(LegacyServerPlayer *player, const QJsonValue &arg)
{
    bool broadcast = true;
    auto noBroadcastSpeaking = [&broadcast, player, arg, this]() {
        broadcast = false;
        QJsonArray nbbody;
        nbbody << player->objectName();
        nbbody << arg;
        doNotify(player, S_COMMAND_SPEAK, nbbody);
    };

    if ((player != nullptr) && Config.EnableCheat) {
        QString sentence = QString::fromUtf8(QByteArray::fromBase64(arg.toString().toLatin1()));
        if (sentence == QStringLiteral(".BroadcastRoles")) {
            noBroadcastSpeaking();
            foreach (LegacyServerPlayer *p, getAllPlayers()) {
                broadcastProperty(p, "role", p->roleString());
                setPlayerProperty(p, "role_shown", true);
            }
        } else if (sentence.startsWith(QStringLiteral(".BroadcastRoles="))) {
            noBroadcastSpeaking();
            QString name = sentence.mid(12);
            foreach (LegacyServerPlayer *p, getAllPlayers()) {
                if (p->objectName() == name || p->generalName() == name) {
                    broadcastProperty(p, "role", p->roleString());
                    setPlayerProperty(p, "role_shown", true);
                    break;
                }
            }
        } else if (sentence == QStringLiteral(".ShowHandCards")) {
            noBroadcastSpeaking();
            QString split(QStringLiteral("----------"));
            split = QString::fromUtf8(split.toUtf8().toBase64());
            QJsonArray body;
            body << player->objectName() << split;
            doNotify(player, S_COMMAND_SPEAK, body);
            foreach (LegacyServerPlayer *p, getAllPlayers()) {
                if (!p->isKongcheng()) {
                    QStringList handcards;
                    foreach (const Card *card, p->handCards())
                        handcards << QStringLiteral("<b>%1</b>").arg(Sanguosha->cardDescriptor(card->id()).logName());
                    QString hand = handcards.join(QStringLiteral(", "));
                    hand = QString::fromUtf8(hand.toUtf8().toBase64());
                    QJsonArray body;
                    body << p->objectName() << hand;
                    doNotify(player, S_COMMAND_SPEAK, body);
                }
            }
            doNotify(player, S_COMMAND_SPEAK, body);
        } else if (sentence.startsWith(QStringLiteral(".ShowHandCards="))) {
            noBroadcastSpeaking();
            QString name = sentence.mid(15);
            foreach (LegacyServerPlayer *p, getAllPlayers()) {
                if (p->objectName() == name || p->generalName() == name) {
                    if (!p->isKongcheng()) {
                        QStringList handcards;
                        foreach (const Card *card, p->handCards())
                            handcards << QStringLiteral("<b>%1</b>").arg(Sanguosha->cardDescriptor(card->id()).logName());
                        QString hand = handcards.join(QStringLiteral(", "));
                        hand = QString::fromUtf8(hand.toUtf8().toBase64());
                        QJsonArray body;
                        body << p->objectName() << hand;
                        doNotify(player, S_COMMAND_SPEAK, body);
                    }
                    break;
                }
            }
        } else if (sentence.startsWith(QStringLiteral(".ShowPrivatePile="))) {
            noBroadcastSpeaking();
            QStringList arg = sentence.mid(17).split(QStringLiteral(":"));
            if (arg.length() == 2) {
                QString name = arg.first();
                QString pile_name = arg.last();
                foreach (LegacyServerPlayer *p, getAllPlayers()) {
                    if (p->objectName() == name || p->generalName() == name) {
                        if (!p->pile(pile_name).isEmpty()) {
                            QStringList pile_cards;
                            foreach (int id, p->pile(pile_name))
                                pile_cards << QStringLiteral("<b>%1</b>").arg(Sanguosha->cardDescriptor(id).logName());
                            QString pile = pile_cards.join(QStringLiteral(", "));
                            pile = QString::fromUtf8(pile.toUtf8().toBase64());
                            QJsonArray body;
                            body << p->objectName() << pile;
                            doNotify(player, S_COMMAND_SPEAK, body);
                        }
                        break;
                    }
                }
            }
        } else if (sentence.startsWith(QStringLiteral(".SetAIDelay="))) {
            noBroadcastSpeaking();
            bool ok = false;
            int delay = sentence.mid(12).toInt(&ok);
            if (ok) {
                Config.AIDelay = Config.OriginAIDelay = delay;
                Config.setValue(QStringLiteral("OriginAIDelay"), delay);
            }
        } else if (sentence.startsWith(QStringLiteral(".SetGameMode="))) {
            noBroadcastSpeaking();
            QString name = sentence.mid(13);
            setTag(QStringLiteral("NextGameMode"), name);
        } else if (sentence.startsWith(QStringLiteral(".SecondGeneral="))) {
            noBroadcastSpeaking();
            QString prop = sentence.mid(15);
            setTag(QStringLiteral("NextGameSecondGeneral"), !prop.isEmpty() && prop != QStringLiteral("0") && prop != QStringLiteral("false"));
        } else if (sentence == QStringLiteral(".Pause")) {
            noBroadcastSpeaking();
            pauseCommand(player, QStringLiteral("true"));
        } else if (sentence == QStringLiteral(".Resume")) {
            noBroadcastSpeaking();
            pauseCommand(player, QStringLiteral("false"));
        }
    }
    if (broadcast) {
        QJsonArray body;
        if (player != nullptr)
            body << player->objectName();
        else
            body << QStringLiteral(".");
        body << arg;
        doBroadcastNotify(S_COMMAND_SPEAK, body);
    }
}

void LegacyRoom::processResponse(LegacyServerPlayer *player, const Packet *packet)
{
    player->acquireLock(LegacyServerPlayer::SEMA_MUTEX);
    bool success = false;
    if (player == nullptr)
        emit room_message(tr("Unable to parse player"));
    else if (!player->m_isWaitingReply || player->m_isClientResponseReady)
        emit room_message(tr("Server is not waiting for reply from %1").arg(player->objectName()));
    else if (packet->commandType() != player->m_expectedReplyCommand)
        emit room_message(tr("Reply command should be %1 instead of %2").arg(player->m_expectedReplyCommand, packet->commandType()));
    else
        success = true;

    if (!success) {
        player->releaseLock(LegacyServerPlayer::SEMA_MUTEX);
        return;
    } else {
        _m_semRoomMutex.acquire();
        if (_m_raceStarted) {
            player->setClientReply(packet->messageBody());
            player->m_isClientResponseReady = true;
            // Warning: the statement below must be the last one before releasing the lock!!!
            // Any statement after this statement will totally compromise the synchronization
            // because getRaceResult will then be able to acquire the lock, reading a non-null
            // raceWinner and proceed with partial data. The current implementation is based on
            // the assumption that the following line is ATOMIC!!!
            _m_raceWinner.storeRelaxed(player);
            // the _m_semRoomMutex.release() signal is in getRaceResult();
            _m_semRaceRequest.release();
        } else {
            _m_semRoomMutex.release();
            player->setClientReply(packet->messageBody());
            player->m_isClientResponseReady = true;
            player->releaseLock(LegacyServerPlayer::SEMA_COMMAND_INTERACTIVE);
        }

        player->releaseLock(LegacyServerPlayer::SEMA_MUTEX);
    }
}

bool LegacyRoom::useCard(const CardUseStruct &use, bool add_history)
{
    CardUseStruct card_use = use;
    card_use.m_addHistory = false;
    card_use.m_isHandcard = true;
    card_use.m_isLastHandcard = true;
    const Card *card_ = card_use.card;

    IdSet ids;
    if (!card_->isVirtualCard())
        ids << card_->effectiveId();
    else
        ids = card_->subcards();
    if (!ids.isEmpty()) {
        foreach (int id, ids) {
            if (cardOwner(id) != use.from || cardPlace(id) != QSanguosha::PlaceHand) {
                card_use.m_isHandcard = false;
                break;
            }
        }
    } else {
        card_use.m_isHandcard = false;
    }

    if (!ids.isEmpty()) {
        foreach (const Card *c, use.from->handCards()) {
            if (!ids.contains(c->effectiveId())) {
                card_use.m_isLastHandcard = false;
                break;
            }
        }
    } else
        card_use.m_isLastHandcard = false;

    if (!ids.isEmpty()) {
        foreach (int id, ids) {
            if (use.from->isShownHandcard(id))
                card_use.m_showncards << id;
        }
        if (!card_use.m_showncards.isEmpty())
            setCardFlag(card_use.card, QStringLiteral("showncards"));
    }
    if (card_use.from->isCardLimited(card_, card_->handleMethod()) && (!card_->canRecast() || card_use.from->isCardLimited(card_, QSanguosha::MethodRecast)))
        return true;

    QString key = card_->faceName();
    int slash_count = card_use.from->slashCount();
    bool showTMskill = false;
    foreach (const Skill *skill, card_use.from->skills(false)) {
        const TargetModSkill *tm = dynamic_cast<const TargetModSkill *>(skill);
        if (tm != nullptr && tm->getResidueNum(card_use.from, card_) > 500)
            showTMskill = true;
    }
    bool slash_not_record = key.contains(QStringLiteral("Slash")) && slash_count > 0 && (card_use.from->hasValidWeapon(QStringLiteral("Crossbow")) || showTMskill);

    card_ = card_use.card->face()->validate(card_use);
    if (card_ == nullptr)
        return false;

    if (card_use.from->phase() == QSanguosha::PhasePlay && add_history && (card_use.m_reason == QSanguosha::CardUseReasonPlay || card_->hasFlag(QStringLiteral("Add_History")))) {
        if (!slash_not_record) {
            card_use.m_addHistory = true;
            addPlayerHistory(qobject_cast<LegacyServerPlayer *>(card_use.from), key);
        }
        addPlayerHistory(nullptr, QStringLiteral("pushPile"));
    }

    try {
        if (card(card_use.card->effectiveId()) == card_) {
            if (use.from != nullptr) {
                QStringList tarmod_detect = qobject_cast<LegacyServerPlayer *>(use.from)->checkTargetModSkillShow(card_use);
                if (!tarmod_detect.isEmpty()) {
                    QString to_show = askForChoice(qobject_cast<LegacyServerPlayer *>(card_use.from), QStringLiteral("tarmod_show"), tarmod_detect.join(QStringLiteral("+")),
                                                   QVariant::fromValue(card_use));
                    qobject_cast<LegacyServerPlayer *>(card_use.from)->showHiddenSkill(to_show);
                }
            }

            if (card_->face()->isKindOf(QStringLiteral("DelayedTrick")) && card_->isVirtualCard() && card_->subcards().size() == 1) {
                Card *wrapped = card(card_use.card->effectiveId());
                broadcastUpdateCard(serverPlayers(), wrapped->id(), wrapped);
                card_use.card = wrapped;
                wrapped->face()->onUse(this, card_use);
                return true;
            }
            if (card_use.card->face()->isKindOf(QStringLiteral("Slash")) && add_history && slash_count > 0)
                card_use.from->setFlag(QStringLiteral("Global_MoreSlashInOneTurn"));
            if (!card_use.card->isVirtualCard()) {
                Card *wrapped = card(card_use.card->effectiveId());

                if (wrapped->isModified())
                    broadcastUpdateCard(serverPlayers(), card_use.card->effectiveId(), wrapped);
                else
                    broadcastResetCard(serverPlayers(), card_use.card->effectiveId());
            }
            card_use.card->face()->onUse(this, card_use);
        } else if (card_ != nullptr) {
            CardUseStruct new_use = card_use;
            new_use.card = card_;
            useCard(new_use);
        }
    } catch (QSanguosha::TriggerEvent triggerEvent) {
        if (triggerEvent == QSanguosha::TurnBroken) {
            if (cardPlace(card_use.card->effectiveId()) == QSanguosha::PlaceTable) {
                CardMoveReason reason(QSanguosha::MoveReasonUnknown, card_use.from->objectName(), QString(), card_use.card->skillName(), QString());
                if (card_use.to.size() == 1)
                    reason.m_targetId = card_use.to.first()->objectName();
                moveCardTo(card_use.card, qobject_cast<LegacyServerPlayer *>(card_use.from), nullptr, QSanguosha::PlaceDiscardPile, reason, true);
            }
            QVariant data = QVariant::fromValue(card_use);
            card_use.from->setFlag(QStringLiteral("Global_ProcessBroken"));
            thread->trigger(QSanguosha::CardFinished, data);
            card_use.from->setFlag(QStringLiteral("-Global_ProcessBroken"));

            foreach (LegacyServerPlayer *p, getAllPlayers()) {
                p->tag.remove(QStringLiteral("Qinggang"));

                foreach (QString flag, p->flagList()) {
                    if (flag == QStringLiteral("Global_GongxinOperator"))
                        p->setFlag(QStringLiteral("-") + flag);
                    else if (flag.endsWith(QStringLiteral("_InTempMoving")))
                        setPlayerFlag(p, QStringLiteral("-") + flag);
                }
            }

            foreach (const Card *c, cards()) {
                if (cardPlace(c->id()) == QSanguosha::PlaceTable || cardPlace(c->id()) == QSanguosha::PlaceJudge)
                    moveCardTo(c, nullptr, QSanguosha::PlaceDiscardPile, true);
                if (c->hasFlag(QStringLiteral("using")))
                    setCardFlag(c->id(), QStringLiteral("-using"));
            }
        }
        throw triggerEvent;
    }
    return true;
}

void LegacyRoom::loseHp(LegacyServerPlayer *victim, int lose)
{
    Q_ASSERT(lose > 0);
    if (lose <= 0)
        return;

    if (victim->isDead() || victim->isRemoved())
        return;
    HpLostStruct l;
    l.player = victim;
    l.num = lose;
    QVariant data = QVariant::fromValue(l);
    if (thread->trigger(QSanguosha::PreHpLost, data))
        return;

    l = data.value<HpLostStruct>();
    if (l.num <= 0)
        return;

    LogStruct log;
    log.type = QStringLiteral("#LoseHp");
    log.from = victim;
    log.arg = QString::number(l.num);
    sendLog(log);

    setPlayerProperty(victim, "hp", victim->hp() - l.num);

    QJsonArray arg;
    arg << victim->objectName();
    arg << -l.num;
    arg << -1;
    doBroadcastNotify(S_COMMAND_CHANGE_HP, arg);

    thread->trigger(QSanguosha::PostHpReduced, data);
    thread->trigger(QSanguosha::PostHpLost, data);
}

void LegacyRoom::loseMaxHp(LegacyServerPlayer *victim, int lose)
{
    Q_ASSERT(lose > 0);
    if (lose <= 0)
        return;

    int hp_1 = victim->hp();
    victim->setMaxHp(qMax(victim->maxHp() - lose, 0));
    int hp_2 = victim->hp();

    broadcastProperty(victim, "maxhp");
    broadcastProperty(victim, "hp");

    LogStruct log;
    log.type = QStringLiteral("#LoseMaxHp");
    log.from = victim;
    log.arg = QString::number(lose);
    sendLog(log);

    QJsonArray arg;
    arg << victim->objectName();
    arg << -lose;
    doBroadcastNotify(S_COMMAND_CHANGE_MAXHP, arg);

    LogStruct log2;
    log2.type = QStringLiteral("#GetHp");
    log2.from = victim;
    log2.arg = QString::number(victim->hp());
    log2.arg2 = QString::number(victim->maxHp());
    sendLog(log2);

    if (victim->maxHp() < victim->dyingFactor())
        killPlayer(victim);
    else {
        QVariant v = QVariant::fromValue(victim);
        thread->trigger(QSanguosha::MaxHpChanged, v);
        if (hp_1 > hp_2) {
            HpLostStruct l;
            l.num = hp_1 - hp_2;
            l.player = victim;
            QVariant data = QVariant::fromValue(l);
            thread->trigger(QSanguosha::PostHpReduced, data);
        }
    }
}

bool LegacyRoom::changeMaxHpForAwakenSkill(LegacyServerPlayer *player, int magnitude)
{
    player->gainMark(QStringLiteral("@waked"));
    int n = player->mark(QStringLiteral("@waked"));
    if (magnitude < 0) {
        if (Config.Enable2ndGeneral && (player->general() != nullptr) && (player->getGeneral2() != nullptr) && Config.MaxHpScheme > 0 && Config.PreventAwakenBelow3
            && player->maxHp() <= 3) {
            setPlayerMark(player, QStringLiteral("AwakenLostMaxHp"), 1);
        } else {
            loseMaxHp(player, -magnitude);
        }
    } else {
        setPlayerProperty(player, "maxhp", player->maxHp() + magnitude);

        LogStruct log;
        log.type = QStringLiteral("#GainMaxHp");
        log.from = player;
        log.arg = QString::number(magnitude);
        sendLog(log);

        LogStruct log2;
        log2.type = QStringLiteral("#GetHp");
        log2.from = player;
        log2.arg = QString::number(player->hp());
        log2.arg2 = QString::number(player->maxHp());
        sendLog(log2);
    }
    return (player->mark(QStringLiteral("@waked")) >= n);
}

void LegacyRoom::applyDamage(LegacyServerPlayer *victim, const DamageStruct &damage)
{
    int new_hp = victim->hp() - damage.num;

    if (!victim->hasValidSkill(QStringLiteral("banling")))
        setPlayerProperty(victim, "hp", new_hp);
    QString change_str = QStringLiteral("%1:%2").arg(victim->objectName(), -damage.num);
    switch (damage.nature) {
    case QSanguosha::DamageFire:
        change_str.append(QStringLiteral("F"));
        break;
    case QSanguosha::DamageThunder:
        change_str.append(QStringLiteral("T"));
        break;
    default:
        break;
    }

    QJsonArray arg;
    arg << victim->objectName() << -damage.num << damage.nature;
    doBroadcastNotify(QSanProtocol::S_COMMAND_CHANGE_HP, arg);
}

void LegacyRoom::recover(LegacyServerPlayer *player, const RecoverStruct &recover, bool set_emotion)
{
    if (player->lostHp() == 0 || player->isDead())
        return;
    RecoverStruct recover_struct = recover;
    recover_struct.nature = QSanguosha::DamageRecover;
    recover_struct.to = player;

    QVariant data = QVariant::fromValue(recover_struct);
    if (thread->trigger(QSanguosha::PreHpRecover, data))
        return;

    recover_struct = data.value<RecoverStruct>();
    int recover_num = recover_struct.num;

    if (!player->hasValidSkill(QStringLiteral("banling"))) {
        int new_hp = qMin(player->hp() + recover_num, player->maxHp());
        setPlayerProperty(player, "hp", new_hp);
    }

    QJsonArray arg;
    arg << player->objectName();
    arg << recover_num;
    arg << 0;
    doBroadcastNotify(S_COMMAND_CHANGE_HP, arg);

    if (set_emotion)
        setEmotion(player, QStringLiteral("recover"));

    thread->trigger(QSanguosha::HpRecover, data);
}

bool LegacyRoom::cardEffect(const Card *card, LegacyServerPlayer *from, LegacyServerPlayer *to, bool multiple)
{
    CardEffectStruct effect;
    effect.card = card;
    effect.from = from;
    effect.to = to;
    effect.multiple = multiple;
    return cardEffect(effect);
}

bool LegacyRoom::cardEffect(const CardEffectStruct &effect)
{
    QVariant data = QVariant::fromValue(effect);
    bool cancel = false;

    if (effect.to->isAlive() || effect.card->face()->isKindOf(QStringLiteral("Slash"))) { // Be care!!!
        // No skills should be triggered here!
        thread->trigger(QSanguosha::CardEffect, data);
        // Make sure that effectiveness of Slash isn't judged here!
        if (!thread->trigger(QSanguosha::CardEffected, data)) {
            cancel = true;
        } else {
            if (!effect.to->hasFlag(QStringLiteral("Global_NonSkillNullify")))
                setEmotion(qobject_cast<LegacyServerPlayer *>(effect.to), QStringLiteral("skill_nullify"));
            else
                effect.to->setFlag(QStringLiteral("-Global_NonSkillNullify"));
        }
    }
    thread->trigger(QSanguosha::PostCardEffected, data);
    return cancel;
}

bool LegacyRoom::isJinkEffected(const SlashEffectStruct &effect, const Card *jink)
{
    if (jink == nullptr)
        return false;
    Q_ASSERT(jink->face()->isKindOf(QStringLiteral("Jink")));
    JinkEffectStruct j;
    j.jink = jink;
    j.slashEffect = effect;
    QVariant jink_data = QVariant::fromValue(j);
    return !thread->trigger(QSanguosha::JinkEffect, jink_data);
}

void LegacyRoom::damage(const DamageStruct &data)
{
    DamageStruct damage_data = data;
    if (damage_data.to == nullptr || damage_data.to->isDead())
        return;
    if (damage_data.to->hasValidSkill(QStringLiteral("huanmeng"))) {
        LogStruct log2;
        log2.type = QStringLiteral("#TriggerSkill");
        log2.from = damage_data.to;
        log2.arg = QStringLiteral("huanmeng");
        sendLog(log2);
        notifySkillInvoked(qobject_cast<LegacyServerPlayer *>(damage_data.to), QStringLiteral("huanmeng"));
        if ((damage_data.card != nullptr) && damage_data.card->face()->isKindOf(QStringLiteral("Slash")))
            QinggangSword::removeQinggangTag(damage_data.to, damage_data.card);

        return;
    }
    if (damage_data.to->isRemoved())
        return;

    QVariant qdata = QVariant::fromValue(damage_data);

    if (!damage_data.chain && !damage_data.transfer) {
        thread->trigger(QSanguosha::ConfirmDamage, qdata);
        damage_data = qdata.value<DamageStruct>();
    }

    if (thread->trigger(QSanguosha::Predamage, qdata)) {
        if ((damage_data.card != nullptr) && damage_data.card->face()->isKindOf(QStringLiteral("Slash")))
            QinggangSword::removeQinggangTag(damage_data.to, damage_data.card);
        return;
    }

    try {
        bool enter_stack = false;
        do {
            bool prevent = thread->trigger(QSanguosha::DamageForseen, qdata);
            if (prevent) {
                if ((damage_data.card != nullptr) && damage_data.card->face()->isKindOf(QStringLiteral("Slash")))
                    QinggangSword::removeQinggangTag(damage_data.to, damage_data.card);

                break;
            }
            if (damage_data.from != nullptr) {
                if (thread->trigger(QSanguosha::DamageCaused, qdata)) {
                    if ((damage_data.card != nullptr) && damage_data.card->face()->isKindOf(QStringLiteral("Slash")))
                        QinggangSword::removeQinggangTag(damage_data.to, damage_data.card);

                    break;
                }
            }

            damage_data = qdata.value<DamageStruct>();

            bool broken = thread->trigger(QSanguosha::DamageInflicted, qdata);
            if (broken) {
                if ((damage_data.card != nullptr) && damage_data.card->face()->isKindOf(QStringLiteral("Slash")))
                    QinggangSword::removeQinggangTag(damage_data.to, damage_data.card);

                break;
            }
            enter_stack = true;
            m_damageStack.push_back(damage_data);
            setTag(QStringLiteral("CurrentDamageStruct"), qdata);

            thread->trigger(QSanguosha::PreDamageDone, qdata);

            if ((damage_data.card != nullptr) && damage_data.card->face()->isKindOf(QStringLiteral("Slash")))
                QinggangSword::removeQinggangTag(damage_data.to, damage_data.card);
            thread->trigger(QSanguosha::DamageDone, qdata);

            if ((damage_data.from != nullptr) && !damage_data.from->hasFlag(QStringLiteral("Global_DebutFlag")))
                thread->trigger(QSanguosha::Damage, qdata);

            if (!damage_data.to->hasFlag(QStringLiteral("Global_DebutFlag")))
                thread->trigger(QSanguosha::Damaged, qdata);
        } while (false);

        if (!enter_stack)
            setTag(QStringLiteral("SkipGameRule"), true);
        damage_data = qdata.value<DamageStruct>();
        thread->trigger(QSanguosha::DamageComplete, qdata);

        if (enter_stack) {
            m_damageStack.pop();
            if (m_damageStack.isEmpty())
                removeTag(QStringLiteral("CurrentDamageStruct"));
            else
                setTag(QStringLiteral("CurrentDamageStruct"), QVariant::fromValue(m_damageStack.first()));
        }
    } catch (QSanguosha::TriggerEvent triggerEvent) {
        if (triggerEvent == QSanguosha::TurnBroken) {
            removeTag(QStringLiteral("is_chained"));
            removeTag(QStringLiteral("CurrentDamageStruct"));
            m_damageStack.clear();
        }
        throw triggerEvent;
    }
}

void LegacyRoom::sendDamageLog(const DamageStruct &data)
{
    LogStruct log;

    if (data.from != nullptr) {
        log.type = QStringLiteral("#Damage");
        log.from = data.from;
    } else {
        log.type = QStringLiteral("#DamageNoSource");
    }

    log.to << data.to;
    log.arg = QString::number(data.num);

    switch (data.nature) {
    case QSanguosha::DamageNormal:
        log.arg2 = QStringLiteral("normal_nature");
        break;
    case QSanguosha::DamageFire:
        log.arg2 = QStringLiteral("fire_nature");
        break;
    case QSanguosha::DamageThunder:
        log.arg2 = QStringLiteral("thunder_nature");
        break;
    }

    sendLog(log);
}

LegacyServerPlayer *LegacyRoom::getFront(LegacyServerPlayer *a, LegacyServerPlayer *b) const
{
    QList<LegacyServerPlayer *> players = getAllPlayers(true);
    int index_a = players.indexOf(a);
    int index_b = players.indexOf(b);
    if (index_a < index_b)
        return a;
    else
        return b;
}

void LegacyRoom::reconnect(LegacyServerPlayer *player, LegacyClientSocket *socket)
{
    player->setSocket(socket);
    player->setState(QStringLiteral("online"));

    marshal(player);

    broadcastProperty(player, "state");
}

void LegacyRoom::marshal(LegacyServerPlayer *player)
{
    notifyProperty(player, player, "objectName");
    notifyProperty(player, player, "role");
    notifyProperty(player, player, "flags", QStringLiteral("marshalling"));

    foreach (LegacyServerPlayer *p, m_players) {
        if (p != player)
            p->introduceTo(player);
    }

    QStringList player_circle;
    foreach (LegacyServerPlayer *player, m_players)
        player_circle << player->objectName();

    doNotify(player, S_COMMAND_ARRANGE_SEATS, QSgsJsonUtils::toJsonArray(player_circle));
    doNotify(player, S_COMMAND_START_IN_X_SECONDS, QJsonValue(0));

    foreach (LegacyServerPlayer *p, m_players) {
        if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony && p == player && !p->haveShownGeneral()) {
            QString general1_name = tag[player->objectName()].toStringList().at(0);
            notifyProperty(player, p, "general", general1_name);
        } else
            notifyProperty(player, p, "general");

        if (p->getGeneral2() != nullptr) {
            if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony && p == player && !p->hasShownGeneral2()) {
                QString general2_name = tag[player->objectName()].toStringList().at(1);
                notifyProperty(player, p, "general2", general2_name);
            } else {
                notifyProperty(player, p, "general2");
            }
        }
    }

    if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony) {
        foreach (const Skill *skill, player->skills(false)) {
            QJsonArray args1;
            args1 << (int)S_GAME_EVENT_ADD_SKILL;
            args1 << player->objectName();
            args1 << skill->name();
            args1 << player->inHeadSkills(skill->name());

            doNotify(player, S_COMMAND_LOG_EVENT, args1);
        }
    }

    if (game_started) {
        LegacyServerPlayer *lord = getLord();
        QJsonArray lord_info;

        lord_info << (lord != nullptr ? lord->generalName() : QJsonValue());
        doNotify(player, S_COMMAND_GAME_START, lord_info);

        QList<int> drawPile = serverInfo()->GameMode->availableCards().values();
        doNotify(player, S_COMMAND_AVAILABLE_CARDS, QSgsJsonUtils::toJsonArray(drawPile));
    }

    foreach (LegacyServerPlayer *p, m_players)
        p->marshal(player);

    notifyProperty(player, player, "flags", QStringLiteral("-marshalling"));
    if (game_started) {
        doNotify(player, S_COMMAND_UPDATE_PILE, QJsonValue(m_drawPile->length()));

        //disconnect wugu
        if (!m_fillAGarg.isNull() && (m_fillAGWho == nullptr || m_fillAGWho == player)) {
            doNotify(player, S_COMMAND_FILL_AMAZING_GRACE, m_fillAGarg);
            for (const QJsonValue &takeAGarg : m_takeAGargs.toArray())
                doNotify(player, S_COMMAND_TAKE_AMAZING_GRACE, takeAGarg);
        }

        QJsonValue discard = QSgsJsonUtils::toJsonArray(discardPile());
        doNotify(player, S_COMMAND_SYNCHRONIZE_DISCARD_PILE, discard);
    }
}

void LegacyRoom::startGame()
{
    //step1 : player set  MaxHP and CompanionEffect
    foreach (LegacyServerPlayer *player, m_players) {
        Q_ASSERT(player->general());
        if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony) {
            QStringList generals = getTag(player->objectName()).toStringList();
            const General *general1 = Sanguosha->general(generals.first());
            Q_ASSERT(general1);
            const General *general2 = Sanguosha->general(generals.last());
            int max_hp = (general1->maxHpHead() + general2->maxHpDeputy());
            if (general1->isCompanionWith(generals.last()))
                player->setMark(QStringLiteral("CompanionEffect"), 1);

            player->setMark(QStringLiteral("HalfMaxHpLeft"), max_hp % 2);
            max_hp = max_hp / 2;
            player->setMaxHp(max_hp);
        } else
            player->setMaxHp(player->getGeneralMaxHp());

        player->setHp(player->maxHp());
    }

    foreach (LegacyServerPlayer *player, m_players) {
        if (serverInfo()->GameMode->name() == QStringLiteral("06_3v3") || serverInfo()->GameMode->name() == QStringLiteral("02_1v1")
            || serverInfo()->GameMode->name() == QStringLiteral("06_XMode")
            || (serverInfo()->GameMode->category() == QSanguosha::ModeRole && !player->isLord())) // hegemony has already notified "general"
            broadcastProperty(player, "general");

        if (serverInfo()->GameMode->name() == QStringLiteral("02_1v1"))
            doBroadcastNotify(getOtherPlayers(player, true), S_COMMAND_REVEAL_GENERAL, QJsonArray() << player->objectName() << player->generalName());

        if (Config.Enable2ndGeneral && serverInfo()->GameMode->name() != QStringLiteral("02_1v1") && serverInfo()->GameMode->name() != QStringLiteral("06_3v3")
            && serverInfo()->GameMode->name() != QStringLiteral("06_XMode") && serverInfo()->GameMode->name() != QStringLiteral("04_1v3")
            && serverInfo()->GameMode->category() == QSanguosha::ModeHegemony)
            broadcastProperty(player, "general2");

        broadcastProperty(player, "hp");
        broadcastProperty(player, "maxhp");

        if (serverInfo()->GameMode->name() == QStringLiteral("06_3v3") || serverInfo()->GameMode->name() == QStringLiteral("06_XMode")) {
            broadcastProperty(player, "role");
            setPlayerProperty(player, "role_shown", true);
        }

        if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony) {
            setPlayerProperty(player, "general_showed", true);
            setPlayerProperty(player, "general2_showed", true);
            if (player->isLord())
                setPlayerProperty(player, "role_shown", true);
        }
    }

    preparePlayers();

    QList<int> drawPile = *m_drawPile;
    qShuffle(drawPile);
    doBroadcastNotify(S_COMMAND_AVAILABLE_CARDS, QSgsJsonUtils::toJsonArray(drawPile));

    LegacyServerPlayer *lord = getLord();
    QJsonArray lord_info;
    lord_info << (lord != nullptr ? lord->generalName() : QJsonValue());
    doBroadcastNotify(S_COMMAND_GAME_START, lord_info);

    game_started = true;

    LegacyServer *server = qobject_cast<LegacyServer *>(parent());
    foreach (LegacyServerPlayer *player, m_players) {
        if (player->getState() == QStringLiteral("online"))
            server->signupPlayer(player);
    }

    current = m_players.first();

    // initialize the place_map and owner_map;

    foreach (int card_id, *m_drawPile)
        setCardMapping(card_id, nullptr, QSanguosha::PlaceDrawPile);
    doBroadcastNotify(S_COMMAND_UPDATE_PILE, QJsonValue(m_drawPile->length()));

    if (serverInfo()->GameMode->name() != QStringLiteral("02_1v1") && serverInfo()->GameMode->name() != QStringLiteral("06_3v3")
        && serverInfo()->GameMode->name() != QStringLiteral("06_XMode"))
        resetAllCards();
}

bool LegacyRoom::notifyProperty(LegacyServerPlayer *playerToNotify, const LegacyServerPlayer *propertyOwner, const char *propertyName, const QString &value)
{
    if (propertyOwner == nullptr)
        return false;
    QString real_value = value;
    if (real_value.isNull())
        real_value = propertyOwner->property(propertyName).toString();
    QJsonArray arg;
    arg << propertyOwner->objectName();
    arg << QString::fromUtf8(propertyName);
    arg << real_value;
    return doNotify(playerToNotify, S_COMMAND_SET_PROPERTY, arg);
}

bool LegacyRoom::broadcastProperty(LegacyServerPlayer *player, const char *property_name, const QString &value)
{
    if (player == nullptr)
        return false;
    QString real_value = value;
    if (real_value.isNull())
        real_value = player->property(property_name).toString();

    QJsonArray arg;
    arg << player->objectName() << QString::fromUtf8(property_name) << real_value;
    return doBroadcastNotify(S_COMMAND_SET_PROPERTY, arg);
}

void LegacyRoom::drawCards(LegacyServerPlayer *player, int n, const QString &reason)
{
    QList<LegacyServerPlayer *> players;
    players.append(player);
    drawCards(players, n, reason);
}

void LegacyRoom::drawCards(const QList<LegacyServerPlayer *> &players, int n, const QString &reason)
{
    QList<int> n_list;
    n_list.append(n);
    drawCards(players, n_list, reason);
}

void LegacyRoom::drawCards(QList<LegacyServerPlayer *> players, const QList<int> &n_list, const QString &reason)
{
    QList<LegacyCardsMoveStruct> moves;
    int index = -1;
    int len = n_list.length();
    Q_ASSERT(len >= 1);
    foreach (LegacyServerPlayer *player, players) {
        index++;
        if (!player->isAlive() && reason != QStringLiteral("reform"))
            continue;
        int n = n_list.at(qMin(index, len - 1));
        if (n <= 0)
            continue;

        QList<int> card_ids;
        card_ids = getNCards(n, false);

        LegacyCardsMoveStruct move;
        move.card_ids = card_ids;
        move.from = nullptr;
        move.to = player;
        move.to_place = QSanguosha::PlaceHand;
        move.reason = CardMoveReason(QSanguosha::MoveReasonDraw, player->objectName());
        move.reason.m_extraData = reason;

        moves.append(move);
    }
    moveCardsAtomic(moves, false);
}

void LegacyRoom::throwCard(const Card *card, LegacyServerPlayer *who, LegacyServerPlayer *thrower, bool notifyLog)
{
    CardMoveReason reason;
    if (thrower == nullptr) {
        reason.m_reason = QSanguosha::MoveReasonThrow;
        reason.m_playerId = who != nullptr ? who->objectName() : QString();
    } else {
        reason.m_reason = QSanguosha::MoveReasonDismantle;
        reason.m_targetId = who != nullptr ? who->objectName() : QString();
        reason.m_playerId = thrower->objectName();
    }
    reason.m_skillName = card->skillName();
    throwCard(card, reason, who, thrower, notifyLog);
}

void LegacyRoom::throwCard(const Card *card, const CardMoveReason &reason, LegacyServerPlayer *who, LegacyServerPlayer *thrower, bool notifyLog)
{
    if (card == nullptr)
        return;

    QList<int> to_discard;
    if (card->isVirtualCard())
        foreach (int id, card->subcards())
            to_discard.append(id);
    else
        to_discard << card->effectiveId();

    if (notifyLog) {
        LogStruct log;
        if (who != nullptr) {
            if (thrower == nullptr) {
                log.type = QStringLiteral("$DiscardCard");
                log.from = who;
            } else {
                log.type = QStringLiteral("$DiscardCardByOther");
                log.from = thrower;
                log.to << who;
            }
        } else {
            log.type = QStringLiteral("$EnterDiscardPile");
        }

        log.card_str = IntList2StringList(to_discard).join(QStringLiteral("+"));
        sendLog(log);
    }

    QList<LegacyCardsMoveStruct> moves;
    if (who != nullptr) {
        LegacyCardsMoveStruct move(to_discard, who, nullptr, QSanguosha::PlaceUnknown, QSanguosha::PlaceDiscardPile, reason);
        moves.append(move);
        moveCardsAtomic(moves, true);
    } else {
        LegacyCardsMoveStruct move(to_discard, nullptr, QSanguosha::PlaceDiscardPile, reason);
        moves.append(move);
        moveCardsAtomic(moves, true);
    }
}

void LegacyRoom::throwCard(int card_id, LegacyServerPlayer *who, LegacyServerPlayer *thrower, bool notifyLog)
{
    throwCard(card(card_id), who, thrower, notifyLog);
}

RoomThread *LegacyRoom::getThread() const
{
    return thread;
}

void LegacyRoom::moveCardTo(const Card *card, Player *dstPlayer, QSanguosha::Place dstPlace, bool forceMoveVisible)
{
    moveCardTo(card, dstPlayer, dstPlace, CardMoveReason(QSanguosha::MoveReasonUnknown, QString()), forceMoveVisible);
}

void LegacyRoom::moveCardTo(const Card *card, Player *dstPlayer, QSanguosha::Place dstPlace, const CardMoveReason &reason, bool forceMoveVisible)
{
    moveCardTo(card, nullptr, dstPlayer, dstPlace, QString(), reason, forceMoveVisible);
}

void LegacyRoom::moveCardTo(const Card *card, Player *srcPlayer, Player *dstPlayer, QSanguosha::Place dstPlace, const CardMoveReason &reason, bool forceMoveVisible)
{
    moveCardTo(card, srcPlayer, dstPlayer, dstPlace, QString(), reason, forceMoveVisible);
}

void LegacyRoom::moveCardTo(const Card *card, Player *srcPlayer, Player *dstPlayer, QSanguosha::Place dstPlace, const QString &pileName, const CardMoveReason &reason,
                            bool forceMoveVisible)
{
    LegacyCardsMoveStruct move;
    if (card->isVirtualCard()) {
        move.card_ids = card->subcards().values();
        if (move.card_ids.empty())
            return;
    } else
        move.card_ids.append(card->id());
    move.to = dstPlayer;
    move.to_place = dstPlace;
    move.to_pile_name = pileName;
    move.from = srcPlayer;
    move.reason = reason;
    QList<LegacyCardsMoveStruct> moves;
    moves.append(move);
    moveCardsAtomic(moves, forceMoveVisible);
}

void LegacyRoom::_fillMoveInfo(LegacyCardsMoveStruct &moves, int card_index)
{
    int card_id = moves.card_ids[card_index];
    if (moves.from == nullptr)
        moves.from = cardOwner(card_id);
    moves.from_place = cardPlace(card_id);
    if (moves.from != nullptr) { // Hand/Equip/Judge
        if (moves.from_place == QSanguosha::PlaceSpecial || moves.from_place == QSanguosha::PlaceTable)
            moves.from_pile_name = moves.from->pileName(card_id);
        if (moves.from_player_name.isEmpty())
            moves.from_player_name = moves.from->objectName();
    }
    if (moves.to != nullptr) {
        if (moves.to_player_name.isEmpty())
            moves.to_player_name = moves.to->objectName();
        int card_id = moves.card_ids[card_index];
        if (moves.to_place == QSanguosha::PlaceSpecial || moves.to_place == QSanguosha::PlaceTable)
            moves.to_pile_name = moves.to->pileName(card_id);
    }
}

QList<LegacyCardsMoveOneTimeStruct> LegacyRoom::_mergeMoves(QList<LegacyCardsMoveStruct> cards_moves)
{
    QMap<_MoveMergeClassifier, QList<LegacyCardsMoveStruct>> moveMap;

    foreach (LegacyCardsMoveStruct cards_move, cards_moves) {
        _MoveMergeClassifier classifier(cards_move);
        moveMap[classifier].append(cards_move);
    }

    QList<LegacyCardsMoveOneTimeStruct> result;
    foreach (_MoveMergeClassifier cls, moveMap.keys()) {
        LegacyCardsMoveOneTimeStruct moveOneTime;
        moveOneTime.from = cls.m_from;
        moveOneTime.reason = moveMap[cls].first().reason;
        moveOneTime.to = cls.m_to;
        moveOneTime.to_place = cls.m_to_place;
        moveOneTime.to_pile_name = cls.m_to_pile_name;
        moveOneTime.is_last_handcard = false;
        moveOneTime.origin_from = cls.m_origin_from;
        moveOneTime.origin_to = cls.m_origin_to;
        moveOneTime.origin_to_place = cls.m_origin_to_place;
        moveOneTime.origin_to_pile_name = cls.m_origin_to_pile_name;
        foreach (LegacyCardsMoveStruct move, moveMap[cls]) {
            moveOneTime.card_ids.append(move.card_ids);
            moveOneTime.broken_ids.append(move.broken_ids);
            moveOneTime.shown_ids.append(move.shown_ids);
            for (int i = 0; i < move.card_ids.size(); i++) {
                moveOneTime.from_places.append(move.from_place);
                moveOneTime.origin_from_places.append(move.from_place);
                moveOneTime.from_pile_names.append(move.from_pile_name);
                moveOneTime.origin_from_pile_names.append(move.from_pile_name);
                moveOneTime.open.append(move.open);
            }
            if (move.is_last_handcard)
                moveOneTime.is_last_handcard = true;
        }
        result.append(moveOneTime);
    }

    if (result.size() > 1) {
        std::sort(result.begin(), result.end(), [this](const LegacyCardsMoveOneTimeStruct &move1, const LegacyCardsMoveOneTimeStruct &move2) -> bool {
            LegacyServerPlayer *a = (LegacyServerPlayer *)move1.from;
            if (a == nullptr)
                a = (LegacyServerPlayer *)move1.to;
            LegacyServerPlayer *b = (LegacyServerPlayer *)move2.from;
            if (b == nullptr)
                b = (LegacyServerPlayer *)move2.to;

            if (a == nullptr || b == nullptr)
                return a != nullptr;

            return getFront(a, b) == a;
        });
    }

    return result;
}

QList<LegacyCardsMoveStruct> LegacyRoom::_separateMoves(QList<LegacyCardsMoveOneTimeStruct> moveOneTimes)
{
    QList<_MoveSeparateClassifier> classifiers;
    QList<QList<int>> ids;
    QList<int> broken_ids;
    QList<int> shown_ids;
    foreach (LegacyCardsMoveOneTimeStruct moveOneTime, moveOneTimes) {
        for (int i = 0; i < moveOneTime.card_ids.size(); i++) {
            _MoveSeparateClassifier classifier(moveOneTime, i);
            if (classifiers.contains(classifier)) {
                int pos = classifiers.indexOf(classifier);
                ids[pos].append(moveOneTime.card_ids[i]);
            } else {
                classifiers << classifier;
                QList<int> new_ids;
                new_ids << moveOneTime.card_ids[i];
                ids << new_ids;
            }
        }
        broken_ids << moveOneTime.broken_ids;
        shown_ids << moveOneTime.shown_ids;
    }

    QList<LegacyCardsMoveStruct> card_moves;
    int i = 0;
    QMap<LegacyServerPlayer *, QList<int>> from_handcards;
    foreach (_MoveSeparateClassifier cls, classifiers) {
        LegacyCardsMoveStruct card_move;
        LegacyServerPlayer *from = (LegacyServerPlayer *)cls.m_from;
        card_move.from = cls.m_from;
        if ((from != nullptr) && !from_handcards.keys().contains(from))
            from_handcards[from] = from->handCardIds().values();
        card_move.to = cls.m_to;
        if (card_move.from != nullptr)
            card_move.from_player_name = card_move.from->objectName();
        if (card_move.to != nullptr)
            card_move.to_player_name = card_move.to->objectName();
        card_move.from_place = cls.m_from_place;
        card_move.to_place = cls.m_to_place;
        card_move.from_pile_name = cls.m_from_pile_name;
        card_move.to_pile_name = cls.m_to_pile_name;
        card_move.open = cls.m_open;
        card_move.card_ids = ids.at(i);
        foreach (int id, ids.at(i)) {
            if (broken_ids.contains(id))
                card_move.broken_ids << id;
            if (shown_ids.contains(id))
                card_move.shown_ids << id;
        }
        card_move.reason = cls.m_reason;

        card_move.origin_from = cls.m_from;
        card_move.origin_to = cls.m_to;
        card_move.origin_from_place = cls.m_from_place;
        card_move.origin_to_place = cls.m_to_place;
        card_move.origin_from_pile_name = cls.m_from_pile_name;
        card_move.origin_to_pile_name = cls.m_to_pile_name;

        if ((from != nullptr) && from_handcards.keys().contains(from)) {
            QList<int> &move_ids = from_handcards[from];
            if (!move_ids.isEmpty()) {
                foreach (int id, card_move.card_ids)
                    move_ids.removeOne(id);
                card_move.is_last_handcard = move_ids.isEmpty();
            }
        }

        card_moves.append(card_move);
        i++;
    }
    if (card_moves.size() > 1) {
        std::sort(card_moves.begin(), card_moves.end(), [this](const LegacyCardsMoveStruct &move1, const LegacyCardsMoveStruct &move2) -> bool {
            LegacyServerPlayer *a = (LegacyServerPlayer *)move1.from;
            if (a == nullptr)
                a = (LegacyServerPlayer *)move1.to;
            LegacyServerPlayer *b = (LegacyServerPlayer *)move2.from;
            if (b == nullptr)
                b = (LegacyServerPlayer *)move2.to;

            if (a == nullptr || b == nullptr)
                return a != nullptr;

            return getFront(a, b) == a;
        });
    }
    return card_moves;
}

void LegacyRoom::moveCardsAtomic(const LegacyCardsMoveStruct &cards_move, bool forceMoveVisible)
{
    QList<LegacyCardsMoveStruct> cards_moves;
    cards_moves.append(cards_move);
    moveCardsAtomic(cards_moves, forceMoveVisible);
}

void LegacyRoom::moveCardsAtomic(QList<LegacyCardsMoveStruct> cards_moves, bool forceMoveVisible)
{
    cards_moves = _breakDownCardMoves(cards_moves);

    QList<LegacyCardsMoveOneTimeStruct> moveOneTimes = _mergeMoves(cards_moves);
    int i = 0;
    foreach (LegacyCardsMoveOneTimeStruct moveOneTime, moveOneTimes) {
        if (moveOneTime.card_ids.empty()) {
            ++i;
            continue;
        }

        QVariant data = QVariant::fromValue(moveOneTime);
        thread->trigger(QSanguosha::BeforeCardsMove, data);
        moveOneTime = data.value<LegacyCardsMoveOneTimeStruct>();
        moveOneTimes[i] = moveOneTime;
        i++;
    }
    cards_moves = _separateMoves(moveOneTimes);

    // TODO: check this logic
    foreach (LegacyCardsMoveStruct move, cards_moves) {
        if (move.from != nullptr) {
            for (int i = 0; i < move.card_ids.length(); ++i)
                move.from->removeCard(card(move.card_ids.at(i)), move.from_place, move.from_pile_name);
        }
    }
    notifyMoveCards(true, cards_moves, forceMoveVisible);

    // First, process remove card
    for (int i = 0; i < cards_moves.size(); i++) {
        LegacyCardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            int card_id = cards_move.card_ids[j];
            const Card *card_ = card(card_id);

            if (cards_move.from != nullptr) // Hand/Equip/Judge
                cards_move.from->removeCard(card_, cards_move.from_place);

            switch (cards_move.from_place) {
            case QSanguosha::PlaceDiscardPile:
                discardPile().removeOne(card_id);
                break;
            case QSanguosha::PlaceDrawPile:
                m_drawPile->removeOne(card_id);
                break;
            case QSanguosha::PlaceSpecial:
                table_cards.removeOne(card_id);
                break;
            default:
                break;
            }
        }
        if (cards_move.from_place == QSanguosha::PlaceDrawPile)
            doBroadcastNotify(S_COMMAND_UPDATE_PILE, QJsonValue(m_drawPile->length()));
    }

    foreach (LegacyCardsMoveStruct move, cards_moves)
        updateCardsOnLose(move);

    for (int i = 0; i < cards_moves.size(); i++) {
        LegacyCardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            setCardMapping(cards_move.card_ids[j], (LegacyServerPlayer *)cards_move.to, cards_move.to_place);
        }
    }
    foreach (LegacyCardsMoveStruct move, cards_moves)
        updateCardsOnGet(move);

    // TODO: check this logic
    foreach (LegacyCardsMoveStruct move, cards_moves) {
        if (move.to != nullptr) {
            for (int i = 0; i < move.card_ids.length(); ++i)
                move.to->addCard(card(move.card_ids.at(i)), move.to_place, move.to_pile_name);
        }
    }
    notifyMoveCards(false, cards_moves, forceMoveVisible);

    // Now, process add cards
    for (int i = 0; i < cards_moves.size(); i++) {
        LegacyCardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            int card_id = cards_move.card_ids[j];
            const Card *card_ = card(card_id);
            if (forceMoveVisible && cards_move.to_place == QSanguosha::PlaceHand)
                card_->addFlag(QStringLiteral("visible"));
            else
                card_->addFlag(QStringLiteral("-visible"));
            if (cards_move.to != nullptr) // Hand/Equip/Judge
                cards_move.to->addCard(card_, cards_move.to_place);

            switch (cards_move.to_place) {
            case QSanguosha::PlaceDiscardPile:
                discardPile().prepend(card_id);
                break;
            case QSanguosha::PlaceDrawPile:
                m_drawPile->prepend(card_id);
                break;
            case QSanguosha::PlaceSpecial:
                table_cards.append(card_id);
                break;
            default:
                break;
            }
        }
    }

    //trigger event
    moveOneTimes = _mergeMoves(cards_moves);
    foreach (LegacyCardsMoveOneTimeStruct moveOneTime, moveOneTimes) {
        if (moveOneTime.card_ids.empty())
            continue;
        QVariant data = QVariant::fromValue(moveOneTime);
        thread->trigger(QSanguosha::CardsMoveOneTime, data);
    }
}

void LegacyRoom::moveCardsToEndOfDrawpile(const QList<int> &card_ids, bool forceVisible)
{
    QList<LegacyCardsMoveStruct> moves;
    LegacyCardsMoveStruct move(card_ids, nullptr, QSanguosha::PlaceDrawPile, CardMoveReason(QSanguosha::MoveReasonUnknown, QString()));
    moves << move;

    QList<LegacyCardsMoveStruct> cards_moves = _breakDownCardMoves(moves);

    QList<LegacyCardsMoveOneTimeStruct> moveOneTimes = _mergeMoves(cards_moves);
    int i = 0;
    foreach (LegacyCardsMoveOneTimeStruct moveOneTime, moveOneTimes) {
        if (moveOneTime.card_ids.empty()) {
            ++i;
            continue;
        }
        QVariant data = QVariant::fromValue(moveOneTime);
        thread->trigger(QSanguosha::BeforeCardsMove, data);
        moveOneTime = data.value<LegacyCardsMoveOneTimeStruct>();
        moveOneTimes[i] = moveOneTime;
        i++;
    }
    cards_moves = _separateMoves(moveOneTimes);

    notifyMoveCards(true, cards_moves, forceVisible);
    // First, process remove card
    for (int i = 0; i < cards_moves.size(); i++) {
        LegacyCardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            int card_id = cards_move.card_ids[j];
            const Card *card_ = card(card_id);

            if (cards_move.from != nullptr) // Hand/Equip/Judge
                cards_move.from->removeCard(card_, cards_move.from_place);

            switch (cards_move.from_place) {
            case QSanguosha::PlaceDiscardPile:
                discardPile().removeOne(card_id);
                break;
            case QSanguosha::PlaceDrawPile:
                m_drawPile->removeOne(card_id);
                break;
            case QSanguosha::PlaceSpecial:
                table_cards.removeOne(card_id);
                break;
            default:
                break;
            }
        }
        if (cards_move.from_place == QSanguosha::PlaceDrawPile)
            doBroadcastNotify(S_COMMAND_UPDATE_PILE, QJsonValue(m_drawPile->length()));
    }

    foreach (LegacyCardsMoveStruct move, cards_moves)
        updateCardsOnLose(move);

    for (int i = 0; i < cards_moves.size(); i++) {
        LegacyCardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            setCardMapping(cards_move.card_ids[j], (LegacyServerPlayer *)cards_move.to, cards_move.to_place);
        }
    }
    foreach (LegacyCardsMoveStruct move, cards_moves)
        updateCardsOnGet(move);
    notifyMoveCards(false, cards_moves, forceVisible);
    // Now, process add cards
    for (int i = 0; i < cards_moves.size(); i++) {
        LegacyCardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            int card_id = cards_move.card_ids[j];
            const Card *card_ = card(card_id);
            card_->addFlag(QStringLiteral("-visible"));
            if (cards_move.to != nullptr) // Hand/Equip/Judge
                cards_move.to->addCard(card_, cards_move.to_place);

            m_drawPile->append(card_id);
            doBroadcastNotify(S_COMMAND_UPDATE_PILE, QJsonValue(m_drawPile->length()));
        }
    }

    //trigger event
    moveOneTimes = _mergeMoves(cards_moves);
    foreach (LegacyCardsMoveOneTimeStruct moveOneTime, moveOneTimes) {
        if (moveOneTime.card_ids.empty())
            continue;
        QVariant data = QVariant::fromValue(moveOneTime);
        thread->trigger(QSanguosha::CardsMoveOneTime, data);
    }
}

QList<LegacyCardsMoveStruct> LegacyRoom::_breakDownCardMoves(QList<LegacyCardsMoveStruct> &cards_moves)
{
    QList<LegacyCardsMoveStruct> all_sub_moves;
    for (int i = 0; i < cards_moves.size(); i++) {
        LegacyCardsMoveStruct &move = cards_moves[i];
        if (move.card_ids.empty())
            continue;
        QMap<_MoveSourceClassifier, QList<int>> moveMap;
        // reassemble move sources
        for (int j = 0; j < move.card_ids.size(); j++) {
            _fillMoveInfo(move, j);
            _MoveSourceClassifier classifier(move);
            moveMap[classifier].append(move.card_ids[j]);
        }
        foreach (_MoveSourceClassifier cls, moveMap.keys()) {
            LegacyCardsMoveStruct sub_move = move;
            cls.copyTo(sub_move);
            if ((sub_move.from == sub_move.to && sub_move.from_place == sub_move.to_place) || sub_move.card_ids.empty())
                continue;
            sub_move.card_ids = moveMap[cls];
            all_sub_moves.append(sub_move);
        }
    }
    return all_sub_moves;
}

void LegacyRoom::updateCardsOnLose(const LegacyCardsMoveStruct &move)
{
    for (int i = 0; i < move.card_ids.size(); i++) {
        Card *card_ = card(move.card_ids[i]);
        if (card_->isModified()) {
            if (move.to_place == QSanguosha::PlaceDiscardPile) {
                card_ = nullptr;
                // resetCard deletes card
                resetCard(move.card_ids[i]);
                broadcastResetCard(serverPlayers(), move.card_ids[i]);
            }
        }
    }
}

void LegacyRoom::updateCardsOnGet(const LegacyCardsMoveStruct &move)
{
    if (move.card_ids.isEmpty())
        return;
    LegacyServerPlayer *player = qobject_cast<LegacyServerPlayer *>(move.from);
    if (player != nullptr && move.to_place == QSanguosha::PlaceDelayedTrick) {
        for (int i = 0; i < move.card_ids.size(); i++) {
#if 0
            WrappedCard *card = qobject_cast<WrappedCard *>(getCard(move.card_ids[i]));
            const Card *engine_card = Sanguosha->getEngineCard(move.card_ids[i]);
            if (card->getSuit() != engine_card->getSuit() || card->getNumber() != engine_card->getNumber()) {
                Card *trick = player->getRoomObject()->cloneCard(card->getRealCard());
                trick->setSuit(engine_card->getSuit());
                trick->setNumber(engine_card->getNumber());
                card->takeOver(trick);
                broadcastUpdateCard(getPlayers(), move.card_ids[i], card);
            }
#endif
        }

        return;
    }

    player = (LegacyServerPlayer *)move.to;
    if (player != nullptr
        && (move.to_place == QSanguosha::PlaceHand || move.to_place == QSanguosha::PlaceEquip || move.to_place == QSanguosha::PlaceJudge
            || move.to_place == QSanguosha::PlaceSpecial)) {
        QList<const Card *> cards;
        foreach (int cardId, move.card_ids)
            cards.append(card(cardId));
        filterCards(player, cards, true);
    }
}

bool LegacyRoom::notifyMoveCards(bool isLostPhase, QList<LegacyCardsMoveStruct> cards_moves, bool forceVisible, QList<LegacyServerPlayer *> players)
{
    if (players.isEmpty())
        players = m_players;

    // Notify clients
    int moveId = 0;
    if (isLostPhase)
        moveId = _m_lastMovementId++;
    else
        moveId = --_m_lastMovementId;
    Q_ASSERT(_m_lastMovementId >= 0);
    foreach (LegacyServerPlayer *player, players) {
        if (player->getState() != QStringLiteral("online"))
            continue;
        QJsonArray arg;
        arg << moveId;
        for (int i = 0; i < cards_moves.size(); i++) {
            LegacyServerPlayer *to = nullptr;
            foreach (LegacyServerPlayer *player, m_players) {
                if (player->objectName() == cards_moves[i].to_player_name) {
                    to = player;
                    break;
                }
            }
            cards_moves[i].open = forceVisible
                || cards_moves[i].isRelevant(player)
                // forceVisible will override cards to be visible
                || cards_moves[i].to_place == QSanguosha::PlaceEquip || cards_moves[i].from_place == QSanguosha::PlaceEquip
                || cards_moves[i].to_place == QSanguosha::PlaceDelayedTrick
                || cards_moves[i].from_place == QSanguosha::PlaceDelayedTrick
                // only cards moved to hand/special can be invisible
                || cards_moves[i].from_place == QSanguosha::PlaceDiscardPile
                || cards_moves[i].to_place == QSanguosha::PlaceDiscardPile
                // any card from/to discard pile should be visible
                || cards_moves[i].from_place == QSanguosha::PlaceTable
                // any card from/to place table should be visible,except pindian
                || (cards_moves[i].to_place == QSanguosha::PlaceSpecial && (to != nullptr) && to->pileOpen(cards_moves[i].to_pile_name, player->objectName()))
                // pile open to specific players
                || player->hasFlag(QStringLiteral("Global_GongxinOperator"));
            // the player put someone's cards to the drawpile

            arg << cards_moves[i].toVariant();
        }
        doNotify(player, isLostPhase ? S_COMMAND_LOSE_CARD_LEGACY : S_COMMAND_GET_CARD_LEGACY, arg);
    }
    return true;
}

void LegacyRoom::notifySkillInvoked(LegacyServerPlayer *player, const QString &skill_name)
{
    QJsonArray args;
    args << QSanProtocol::S_GAME_EVENT_SKILL_INVOKED;
    args << player->objectName();
    args << skill_name;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

void LegacyRoom::broadcastSkillInvoke(const QString &skill_name, const QString &category)
{
    QJsonArray args;
    args << QSanProtocol::S_GAME_EVENT_PLAY_EFFECT;
    args << skill_name;
    args << category;
    args << -1;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

void LegacyRoom::broadcastSkillInvoke(const QString &skill_name)
{
    QJsonArray args;
    args << QSanProtocol::S_GAME_EVENT_PLAY_EFFECT;
    args << skill_name;
    args << true;
    args << -1;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

void LegacyRoom::broadcastSkillInvoke(const QString &skill_name, int type)
{
    QJsonArray args;
    args << QSanProtocol::S_GAME_EVENT_PLAY_EFFECT;
    args << skill_name;
    args << true;
    args << type;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

void LegacyRoom::broadcastSkillInvoke(const QString &skill_name, bool isMale, int type)
{
    QJsonArray args;
    args << QSanProtocol::S_GAME_EVENT_PLAY_EFFECT;
    args << skill_name;
    args << isMale;
    args << type;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

void LegacyRoom::doLightbox(const QString &lightboxName, int duration)
{
    if (Config.AIDelay == 0)
        return;
    doAnimate(S_ANIMATE_LIGHTBOX, lightboxName, QString::number(duration));
    thread->delay(duration / 1.2);
}

void LegacyRoom::doAnimate(QSanProtocol::AnimateType type, const QString &arg1, const QString &arg2, QList<LegacyServerPlayer *> players)
{
    if (players.isEmpty())
        players = m_players;
    QJsonArray arg;
    arg << (int)type;
    arg << arg1;
    arg << arg2;
    doBroadcastNotify(players, S_COMMAND_ANIMATE, arg);
}

void LegacyRoom::doBattleArrayAnimate(LegacyServerPlayer *player, LegacyServerPlayer *target)
{
    if (getAllPlayers().length() < 4)
        return;
    if (target == nullptr) {
        QStringList names;
        foreach (const Player *p, player->formationPlayers())
            names << p->objectName();
        if (names.length() > 1)
            doAnimate(QSanProtocol::S_ANIMATE_BATTLEARRAY, player->objectName(), names.join(QStringLiteral("+")));
    } else {
        foreach (LegacyServerPlayer *p, getOtherPlayers(player))
            if (p->inSiegeRelation(player, target))
                doAnimate(QSanProtocol::S_ANIMATE_BATTLEARRAY, player->objectName(), QStringLiteral("%1+%2").arg(p->objectName(), player->objectName()));
    }
}

void LegacyRoom::preparePlayers()
{
    if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony) {
        foreach (LegacyServerPlayer *player, m_players) {
            QString general1_name = tag[player->objectName()].toStringList().at(0);
            QSet<const Skill *> skills = Sanguosha->general(general1_name)->skills(true, true);
            foreach (const Skill *skill, skills)
                player->addSkill(skill->name());
            QString general2_name = tag[player->objectName()].toStringList().at(1);
            QSet<const Skill *> skills2 = Sanguosha->general(general2_name)->skills(true, false);
            foreach (const Skill *skill, skills2)
                player->addSkill(skill->name(), false);

            QJsonArray args;
            args << (int)QSanProtocol::S_GAME_EVENT_PREPARE_SKILL;
            doNotify(player, QSanProtocol::S_COMMAND_LOG_EVENT, args);
        }

    } else {
        foreach (LegacyServerPlayer *player, m_players) {
            QSet<const Skill *> skills = player->general()->skills();
            foreach (const Skill *skill, skills)
                player->addSkill(skill->name());
            if (player->getGeneral2() != nullptr) {
                skills = player->getGeneral2()->skills();
                foreach (const Skill *skill, skills)
                    player->addSkill(skill->name(), false);
            }
            player->setGender(player->general()->gender());
        }

        QJsonArray args;
        args << (int)QSanProtocol::S_GAME_EVENT_PREPARE_SKILL;
        doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
    }
}

void LegacyRoom::changePlayerGeneral(LegacyServerPlayer *player, const QString &new_general)
{
    QString originalName = player->tag.value(QStringLiteral("init_general"), QString()).toString();
    if (!originalName.isEmpty())
        player->tag[QStringLiteral("init_general")] = player->generalName();

    if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony && player->general() != nullptr) {
        foreach (const Skill *skill, player->general()->skills(true, true))
            player->loseSkill(skill->name());
    }
    player->setProperty("general", new_general);
    QList<LegacyServerPlayer *> players = m_players;
    if (new_general == QStringLiteral("anjiang"))
        players.removeOne(player);
    foreach (LegacyServerPlayer *p, players)
        notifyProperty(p, player, "general");
    Q_ASSERT(player->general() != nullptr);
    if (new_general != QStringLiteral("anjiang"))
        player->setGender(player->general()->gender());
    if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony) {
        foreach (const Skill *skill, player->general()->skills(true, true)) {
            if (skill->isLordSkill() && !player->isLord()) {
                continue;
            }
            player->addSkill(skill->name());
        }
    }

    filterCards(player, player->getCards(QStringLiteral("hes")), true);
}

void LegacyRoom::changePlayerGeneral2(LegacyServerPlayer *player, const QString &new_general)
{
    QString originalName2 = player->tag.value(QStringLiteral("init_general2"), QString()).toString();
    if (!originalName2.isEmpty())
        player->tag[QStringLiteral("init_general2")] = player->getGeneral2Name();

    if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony && player->getGeneral2() != nullptr) {
        foreach (const Skill *skill, player->getGeneral2()->skills(true, false))
            player->loseSkill(skill->name());
    }
    player->setProperty("general2", new_general);
    QList<LegacyServerPlayer *> players = m_players;
    if (new_general == QStringLiteral("anjiang"))
        players.removeOne(player);
    foreach (LegacyServerPlayer *p, players)
        notifyProperty(p, player, "general2");
    Q_ASSERT(player->getGeneral2() != nullptr);
    if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony && (player->getGeneral2() != nullptr)) {
        foreach (const Skill *skill, player->getGeneral2()->skills(true, false)) {
            if (skill->isLordSkill() && !player->isLord())
                continue;
            player->addSkill(skill->name(), false);
        }
    }
    filterCards(player, player->getCards(QStringLiteral("hes")), true);
}

void LegacyRoom::filterCards(LegacyServerPlayer *player, QList<const Card *> cards, bool refilter)
{
    if (refilter) {
        for (int i = 0; i < cards.size(); i++) {
#if 0
            WrappedCard *card = qobject_cast<WrappedCard *>(getCard(cards[i]->getId()));
            if (card->isModified()) {
                int cardId = card->getId();
                resetCard(cardId);
                if (getCardPlace(cardId) != QSanguosha::PlaceHand || player->isShownHandcard(cardId))
                    broadcastResetCard(m_players, cardId);
                else
                    notifyResetCard(player, cardId);
            }
#endif
        }
    }

    bool *cardChanged = new bool[cards.size()];
    for (int i = 0; i < cards.size(); i++)
        cardChanged[i] = false;

    QSet<const Skill *> skills = player->skills(false);
    QList<const FilterSkill *> filterSkills;

    foreach (const Skill *skill, skills) {
        if (player->hasValidSkill(skill->name())) {
            const FilterSkill *filter = dynamic_cast<const FilterSkill *>(skill);
            if (filter != nullptr)
                filterSkills.append(filter);
        }
    }
    if (filterSkills.empty()) {
        delete[] cardChanged;
        return;
    }

    for (int i = 0; i < cards.size(); i++) {
        const Card *card = cards[i];
        for (int fTime = 0; fTime < filterSkills.size(); fTime++) {
            bool converged = true;
            foreach (const FilterSkill *skill, filterSkills) {
                Q_ASSERT(skill);
                if (skill->viewFilter(cards[i], player)) {
                    cards[i] = skill->viewAs(card, player);
                    Q_ASSERT(cards[i] != NULL);
                    converged = false;
                    cardChanged[i] = true;
                }
            }
            if (converged)
                break;
        }
    }

    for (int i = 0; i < cards.size(); i++) {
        int cardId = cards[i]->id();
        QSanguosha::Place place = cardPlace(cardId);
        if (!cardChanged[i])
            continue;
        if (place == QSanguosha::PlaceHand && !player->isShownHandcard(cardId))
            notifyUpdateCard(player, cardId, cards[i]);
        else {
            broadcastUpdateCard(serverPlayers(), cardId, cards[i]);
            if (place == QSanguosha::PlaceJudge) {
                LogStruct log;
                log.type = QStringLiteral("#FilterJudge");
                log.arg = cards[i]->skillName();
                log.from = player;

                sendLog(log);
                notifySkillInvoked(player, cards[i]->skillName());
                broadcastSkillInvoke(cards[i]->skillName());
            }
        }
    }

    delete[] cardChanged;
}

void LegacyRoom::acquireSkill(LegacyServerPlayer *player, const Skill *skill, bool open, bool head)
{
    QString skill_name = skill->name();
    if (player->acquiredSkills().contains(skill_name))
        return;
    loadSkill(skill);
    player->acquireSkill(skill_name);

    foreach (const Trigger *trigger, skill->triggers())
        thread->addTrigger(trigger);

    if (skill->isLimited() && !skill->limitMark().isEmpty())
        setPlayerMark(player, skill->limitMark(), 1);

    if (skill->isVisible()) {
        if (open) {
            QJsonArray args;
            args << QSanProtocol::S_GAME_EVENT_ACQUIRE_SKILL;
            args << player->objectName();
            args << skill_name;
            args << head;
            doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        }

        foreach (const Skill *related_skill, skill->affiliatedSkills()) {
            if (!related_skill->isVisible())
                acquireSkill(player, related_skill);
        }

        SkillAcquireLoseStruct s;
        s.isAcquire = true;
        s.player = player;
        s.skill = skill;
        QVariant data = QVariant::fromValue(s);
        thread->trigger(QSanguosha::EventAcquireSkill, data);
    }
}

void LegacyRoom::acquireSkill(LegacyServerPlayer *player, const QString &skill_name, bool open, bool head)
{
    const Skill *skill = Sanguosha->skill(skill_name);
    if (skill != nullptr)
        acquireSkill(player, skill, open, head);
}

void LegacyRoom::setTag(const QString &key, const QVariant &value)
{
    tag.insert(key, value);
}

QVariant LegacyRoom::getTag(const QString &key) const
{
    return tag.value(key);
}

void LegacyRoom::removeTag(const QString &key)
{
    tag.remove(key);
}

QStringList LegacyRoom::getTagNames() const
{
    return tag.keys();
}

void LegacyRoom::setEmotion(LegacyServerPlayer *target, const QString &emotion)
{
    QJsonArray arg;
    arg << target->objectName();
    arg << (emotion.isEmpty() ? QStringLiteral(".") : emotion);
    doBroadcastNotify(S_COMMAND_SET_EMOTION, arg);
}

void LegacyRoom::activate(LegacyServerPlayer *player, CardUseStruct &card_use)
{
    tryPause();

    if (player->hasFlag(QStringLiteral("Global_PlayPhaseTerminated")) || player->hasFlag(QStringLiteral("Global_TurnTerminated"))) {
        setPlayerFlag(player, QStringLiteral("-Global_PlayPhaseTerminated"));
        //for "dangjia" intention ai
        //need record  PlayPhaseTerminated
        setPlayerFlag(player, QStringLiteral("PlayPhaseTerminated"));
        card_use.card = nullptr;
        return;
    }

    notifyMoveFocus(player, S_COMMAND_PLAY_CARD);

    setCurrentCardUsePattern(QString());
    setCurrentCardUseReason(QSanguosha::CardUseReasonPlay);

    if (player->phase() != QSanguosha::PhasePlay) {
        return;
    } else {
        bool success = doRequest(player, S_COMMAND_PLAY_CARD, player->objectName(), true);
        const QJsonValue &clientReply = player->getClientReply();

        if (!success || clientReply.isNull())
            return;

        card_use.from = player;
        if (!ExtendCardUseStruct::tryParse(card_use, clientReply, this)) {
            QJsonArray use = clientReply.toArray();
            emit room_message(tr("Card cannot be parsed:\n %1").arg(use[0].toString()));
            return;
        }
    }
    card_use.m_reason = QSanguosha::CardUseReasonPlay;
    if (!ExtendCardUseStruct::isValid(card_use, QString()))
        return;

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::Activate;
    s.args << ExtendCardUseStruct::toString(card_use);
    QVariant data = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, data);
}

void LegacyRoom::askForLuckCard()
{
    tryPause();

    QList<LegacyServerPlayer *> players;
    foreach (LegacyServerPlayer *player, m_players) {
        if (player->getState() == QStringLiteral("online")) {
            player->m_commandArgs = QJsonValue();
            players << player;
        }
    }

    int n = 0;
    while (n < Config.LuckCardLimitation) {
        if (players.isEmpty())
            return;

        n++;

        Countdown countdown;
        countdown.max = serverInfo()->getCommandTimeout(S_COMMAND_LUCK_CARD, S_CLIENT_INSTANCE);
        countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
        notifyMoveFocus(players, S_COMMAND_LUCK_CARD, countdown);

        doBroadcastRequest(players, S_COMMAND_LUCK_CARD);

        QList<LegacyServerPlayer *> used;
        foreach (LegacyServerPlayer *player, players) {
            const QJsonValue &clientReply = player->getClientReply();
            if (!player->m_isClientResponseReady || !QSgsJsonUtils::isBool(clientReply) || !clientReply.toBool())
                continue;
            used << player;
        }
        if (used.isEmpty())
            return;

        LogStruct log;
        log.type = QStringLiteral("#UseLuckCard");
        foreach (LegacyServerPlayer *player, used) {
            log.from = player;
            sendLog(log);
        }

        QList<int> draw_list;
        foreach (LegacyServerPlayer *player, used) {
            draw_list << player->handCardNum();

            CardMoveReason reason(QSanguosha::MoveReasonPut, player->objectName(), QStringLiteral("luck_card"), QString());
            QList<LegacyCardsMoveStruct> moves;
            LegacyCardsMoveStruct move;
            move.from = player;
            move.from_place = QSanguosha::PlaceHand;
            move.to = nullptr;
            move.to_place = QSanguosha::PlaceDrawPile;
            move.card_ids = player->handCardIds().values();
            move.reason = reason;
            moves.append(move);
            moves = _breakDownCardMoves(moves);

            QList<LegacyServerPlayer *> tmp_list;
            tmp_list.append(player);

            notifyMoveCards(true, moves, false, tmp_list);
            for (int j = 0; j < move.card_ids.size(); j++) {
                int card_id = move.card_ids[j];
                const Card *card_ = card(card_id);
                player->removeCard(card_, QSanguosha::PlaceHand);
            }

            updateCardsOnLose(move);
            for (int j = 0; j < move.card_ids.size(); j++)
                setCardMapping(move.card_ids[j], nullptr, QSanguosha::PlaceDrawPile);
            updateCardsOnGet(move);

            notifyMoveCards(false, moves, false, tmp_list);
            for (int j = 0; j < move.card_ids.size(); j++) {
                int card_id = move.card_ids[j];
                m_drawPile->prepend(card_id);
            }
        }
        qShuffle(*m_drawPile);
        int index = -1;
        foreach (LegacyServerPlayer *player, used) {
            index++;
            QList<LegacyCardsMoveStruct> moves;
            LegacyCardsMoveStruct move;
            move.from = nullptr;
            move.from_place = QSanguosha::PlaceDrawPile;
            move.to = player;
            move.to_place = QSanguosha::PlaceHand;
            move.card_ids = getNCards(draw_list.at(index), false);
            moves.append(move);
            moves = _breakDownCardMoves(moves);

            notifyMoveCards(true, moves, false);
            for (int j = 0; j < move.card_ids.size(); j++) {
                int card_id = move.card_ids[j];
                m_drawPile->removeOne(card_id);
            }

            updateCardsOnLose(move);
            for (int j = 0; j < move.card_ids.size(); j++)
                setCardMapping(move.card_ids[j], player, QSanguosha::PlaceHand);
            updateCardsOnGet(move);

            notifyMoveCards(false, moves, false);
            for (int j = 0; j < move.card_ids.size(); j++) {
                int card_id = move.card_ids[j];
                const Card *card_ = card(card_id);
                player->addCard(card_, QSanguosha::PlaceHand);
            }
        }
        doBroadcastNotify(S_COMMAND_UPDATE_PILE, QJsonValue(m_drawPile->length()));
    }
}

QSanguosha::Suit LegacyRoom::askForSuit(LegacyServerPlayer *player, const QString &reason)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_SUIT);

    static const QList<QSanguosha::Suit> all_suits = {QSanguosha::Spade, QSanguosha::Club, QSanguosha::Heart, QSanguosha::Diamond};

    QSanguosha::Suit suit = all_suits[QRandomGenerator::global()->generate() % 4];

    bool success = doRequest(player, S_COMMAND_CHOOSE_SUIT, QJsonArray() << reason << player->objectName(), true);

    if (success) {
        QVariant clientReply = player->getClientReply();
        QString suitStr = clientReply.toString();
        if (suitStr == QStringLiteral("spade"))
            suit = QSanguosha::Spade;
        else if (suitStr == QStringLiteral("club"))
            suit = QSanguosha::Club;
        else if (suitStr == QStringLiteral("heart"))
            suit = QSanguosha::Heart;
        else if (suitStr == QStringLiteral("diamond"))
            suit = QSanguosha::Diamond;
    }

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::Suit;
    s.args << reason << Card::SuitToString(suit);
    QVariant d = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, d);

    return suit;
}

QString LegacyRoom::askForKingdom(LegacyServerPlayer *player)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_KINGDOM);

    QString kingdomChoice;

    QJsonArray arg;
    arg << player->general()->kingdom();

    bool success = doRequest(player, S_COMMAND_CHOOSE_KINGDOM, arg, true);
    const QJsonValue &clientReply = player->getClientReply();
    if (success && QSgsJsonUtils::isString(clientReply)) {
        QString kingdom = clientReply.toString();
        if (Sanguosha->kingdoms().contains(kingdom))
            kingdomChoice = kingdom;
    }

    //set default
    if (kingdomChoice.isNull())
        kingdomChoice = QStringLiteral("wai");

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::Kingdom;
    s.args << kingdomChoice;
    QVariant decisionData = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, decisionData);

    return kingdomChoice;
}

bool LegacyRoom::askForDiscard(LegacyServerPlayer *player, const QString &reason, int discard_num, int min_num, bool optional, bool include_equip, const QString &prompt)
{
    if (!player->isAlive())
        return false;
    tryPause();
    notifyMoveFocus(player, S_COMMAND_DISCARD_CARD);

    if (!optional) {
        Card *dummy = cloneCard(QStringLiteral("DummyCard"));
        QList<int> jilei_list;
        QList<const Card *> handcards = player->handCards();
        foreach (const Card *card, handcards) {
            if (!player->isJilei(card))
                dummy->addSubcard(card);
            else
                jilei_list << card->id();
        }
        if (include_equip) {
            QList<const Card *> equips = player->equipCards();
            foreach (const Card *card, equips) {
                if (!player->isJilei(card))
                    dummy->addSubcard(card);
            }
        }

        int card_num = dummy->subcards().size();
        if (card_num <= min_num) {
            if (card_num > 0) {
                CardMoveReason movereason;
                movereason.m_playerId = player->objectName();
                movereason.m_skillName = dummy->skillName();
                if (reason == QStringLiteral("gamerule"))
                    movereason.m_reason = QSanguosha::MoveReasonRuleDiscard;
                else
                    movereason.m_reason = QSanguosha::MoveReasonThrow;

                throwCard(dummy, movereason, player);

                ChoiceMadeStruct s;
                s.player = player;
                s.type = ChoiceMadeStruct::CardDiscard;
                s.args << dummy->toString();
                QVariant data = QVariant::fromValue(s);
                thread->trigger(QSanguosha::ChoiceMade, data);
            }

            if (card_num < min_num && !jilei_list.isEmpty()) {
                doJileiShow(player, jilei_list);
                delete (dummy);
                return false;
            }
            delete (dummy);
            return true;
        }
    }

    QList<int> to_discard;
    QJsonArray ask_str;
    ask_str << discard_num;
    ask_str << min_num;
    ask_str << optional;
    ask_str << include_equip;
    ask_str << prompt;
    ask_str << reason;
    bool success = doRequest(player, S_COMMAND_DISCARD_CARD, ask_str, true);
    //@todo: also check if the player does have that card!!!
    QJsonArray clientReply = player->getClientReply().toArray();
    if (!success || ((int)clientReply.size() > discard_num || (int)clientReply.size() < min_num) || !QSgsJsonUtils::tryParse(clientReply, to_discard)) {
        if (optional)
            return false;
        // time is up, and the server choose the cards to discard
        to_discard = player->forceToDiscard(discard_num, include_equip);
    }

    if (to_discard.isEmpty())
        return false;

    Card *dummy_card = cloneCard(QStringLiteral("DummyCard"));
    if (reason == QStringLiteral("gamerule")) {
        CardMoveReason reason(QSanguosha::MoveReasonRuleDiscard, player->objectName(), QString(), dummy_card->skillName(), QString());
        throwCard(dummy_card, reason, player);
    } else {
        CardMoveReason reason(QSanguosha::MoveReasonThrow, player->objectName(), QString(), dummy_card->skillName(), QString());
        throwCard(dummy_card, reason, player);
    }

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::CardDiscard;
    s.args << dummy_card->toString();
    QVariant data = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, data);

    delete (dummy_card);

    return true;
}

void LegacyRoom::doJileiShow(LegacyServerPlayer *player, QList<int> jilei_ids)
{
    QJsonArray gongxinArgs;
    gongxinArgs << player->objectName();
    gongxinArgs << false;
    gongxinArgs << QSgsJsonUtils::toJsonArray(jilei_ids);

    foreach (int cardId, jilei_ids) {
        Card *card_ = card(cardId);
        if (card_->isModified())
            broadcastUpdateCard(getOtherPlayers(player), cardId, card_);
        else
            broadcastResetCard(getOtherPlayers(player), cardId);
    }

    LogStruct log;
    log.type = QStringLiteral("$JileiShowAllCards");
    log.from = player;

    foreach (int card_id, jilei_ids)
        card(card_id)->addFlag(QStringLiteral("visible"));
    log.card_str = IntList2StringList(jilei_ids).join(QStringLiteral("+"));
    sendLog(log);

    doBroadcastNotify(S_COMMAND_SHOW_ALL_CARDS, gongxinArgs);
}

const Card *LegacyRoom::askForExchange(LegacyServerPlayer *player, const QString &reason, int discard_num, int min_num, bool include_equip, const QString &prompt, bool optional)
{
    if (!player->isAlive())
        return nullptr;
    tryPause();
    notifyMoveFocus(player, S_COMMAND_EXCHANGE_CARD);

    QList<int> to_exchange;
    QJsonArray exchange_str;
    exchange_str << discard_num;
    exchange_str << min_num;
    exchange_str << include_equip;
    exchange_str << prompt;
    exchange_str << optional;
    exchange_str << reason;

    bool success = doRequest(player, S_COMMAND_EXCHANGE_CARD, exchange_str, true);
    //@todo: also check if the player does have that card!!!
    QJsonArray clientReply = player->getClientReply().toArray();
    if (!success || clientReply.size() > discard_num || clientReply.size() < min_num || !QSgsJsonUtils::tryParse(clientReply, to_exchange)) {
        if (optional)
            return nullptr;
        to_exchange = player->forceToDiscard(discard_num, include_equip, false);
    }

    if (to_exchange.isEmpty())
        return nullptr;

    Card *card = cloneCard();
    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::CardExchange;
    s.args << reason << card->toString();
    QVariant data = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, data);
    return card;
}

QList<int> LegacyRoom::getCardIdsOnTable(const Card *virtual_card) const
{
    if (virtual_card == nullptr)
        return QList<int>();
    if (!virtual_card->isVirtualCard()) {
        IdSet ids;
        ids << virtual_card->effectiveId();
        return getCardIdsOnTable(ids);
    } else {
        return getCardIdsOnTable(virtual_card->subcards());
    }
    return QList<int>();
}

QList<int> LegacyRoom::getCardIdsOnTable(const IdSet &card_ids) const
{
    QList<int> r;
    foreach (int id, card_ids) {
        if (cardPlace(id) == QSanguosha::PlaceTable)
            r << id;
    }
    return r;
}

LegacyServerPlayer *LegacyRoom::getLord(const QString & /*unused*/, bool /*unused*/) const
{
    if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony)
        return nullptr;
    LegacyServerPlayer *the_lord = m_players.first();
    if (the_lord->roleString() == QStringLiteral("lord"))
        return the_lord;

    foreach (LegacyServerPlayer *player, m_players) {
        if (player->roleString() == QStringLiteral("lord"))
            return player;
    }

    return nullptr;
}

void LegacyRoom::askForGuanxing(LegacyServerPlayer *zhuge, const QList<int> &cards, QSanguosha::GuanxingType guanxing_type, const QString &skillName)
{
    QList<int> top_cards;
    QList<int> bottom_cards;
    tryPause();
    notifyMoveFocus(zhuge, S_COMMAND_SKILL_GUANXING);

    if (guanxing_type == QSanguosha::GuanxingUpOnly && cards.length() == 1) {
        top_cards = cards;
    } else if (guanxing_type == QSanguosha::GuanxingDownOnly && cards.length() == 1) {
        bottom_cards = cards;
    } else {
        QJsonArray guanxingArgs;
        guanxingArgs << QSgsJsonUtils::toJsonArray(cards);
        guanxingArgs << (guanxing_type != QSanguosha::GuanxingBothSides);
        guanxingArgs << skillName;
        bool success = doRequest(zhuge, S_COMMAND_SKILL_GUANXING, guanxingArgs, true);
        if (!success) {
            foreach (int card_id, cards) {
                if (guanxing_type == QSanguosha::GuanxingDownOnly)
                    m_drawPile->append(card_id);
                else
                    m_drawPile->prepend(card_id);
            }
        }
        QJsonArray clientReply = zhuge->getClientReply().toArray();
        if (clientReply.size() == 2) {
            QSgsJsonUtils::tryParse(clientReply[0], top_cards);
            QSgsJsonUtils::tryParse(clientReply[1], bottom_cards);
            if (guanxing_type == QSanguosha::GuanxingDownOnly) {
                bottom_cards = top_cards;
                top_cards.clear();
            }
        }
    }

    bool length_equal = top_cards.length() + bottom_cards.length() == cards.length();
    auto allcards = top_cards + bottom_cards;
    bool result_equal = IdSet(allcards.begin(), allcards.end()) == IdSet(cards.begin(), cards.end());

    if (!length_equal || !result_equal) {
        if (guanxing_type == QSanguosha::GuanxingDownOnly) {
            bottom_cards = cards;
            top_cards.clear();
        } else {
            top_cards = cards;
            bottom_cards.clear();
        }
    }

    if (guanxing_type == QSanguosha::GuanxingBothSides) {
        LogStruct log;
        log.type = QStringLiteral("#GuanxingResult");
        if (skillName == QStringLiteral("fengshui"))
            log.type = QStringLiteral("#fengshuiResult");
        else if (skillName == QStringLiteral("bolan"))
            log.type = QStringLiteral("#bolanResult");
        log.from = zhuge;
        log.arg = QString::number(top_cards.length());
        log.arg2 = QString::number(bottom_cards.length());
        sendLog(log);
    }

    if (!top_cards.isEmpty()) {
        LogStruct log;
        log.type = QStringLiteral("$GuanxingTop");
        log.from = zhuge;
        log.card_str = IntList2StringList(top_cards).join(QStringLiteral("+"));
        doNotify(zhuge, QSanProtocol::S_COMMAND_LOG_SKILL, log.serialize());
    }
    if (!bottom_cards.isEmpty()) {
        LogStruct log;
        log.type = QStringLiteral("$GuanxingBottom");
        log.from = zhuge;
        log.card_str = IntList2StringList(bottom_cards).join(QStringLiteral("+"));
        doNotify(zhuge, QSanProtocol::S_COMMAND_LOG_SKILL, log.serialize());
    }

    QListIterator<int> i(top_cards);
    i.toBack();
    while (i.hasPrevious())
        m_drawPile->prepend(i.previous());

    i = bottom_cards;
    while (i.hasNext())
        m_drawPile->append(i.next());

    doBroadcastNotify(S_COMMAND_UPDATE_PILE, QJsonValue(m_drawPile->length()));
    QVariant v = QVariant::fromValue(zhuge);
    thread->trigger(QSanguosha::AfterGuanXing, v);
}

int LegacyRoom::doGongxin(LegacyServerPlayer *shenlvmeng, LegacyServerPlayer *target, const QList<int> &enabled_ids, const QString &skill_name, bool cancellable)
{
    Q_ASSERT(!target->isKongcheng());
    tryPause();
    notifyMoveFocus(shenlvmeng, S_COMMAND_SKILL_GONGXIN);

    LogStruct log;
    log.type = QStringLiteral("$ViewAllCards");
    log.from = shenlvmeng;
    log.to << target;
    log.card_str = IntList2StringList(target->handCardIds().values()).join(QStringLiteral("+"));
    doNotify(shenlvmeng, QSanProtocol::S_COMMAND_LOG_SKILL, log.serialize());

    ChoiceMadeStruct s;
    s.player = shenlvmeng;
    s.type = ChoiceMadeStruct::ViewCards;
    s.args << shenlvmeng->objectName() << target->objectName();
    QVariant decisionData = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, decisionData);

    shenlvmeng->tag[skill_name] = QVariant::fromValue((LegacyServerPlayer *)target);
    int card_id = 0;
    foreach (int cardId, target->handCardIds()) {
        Card *card_ = card(cardId);
        if (card_->isModified())
            notifyUpdateCard(shenlvmeng, cardId, card_);
        else

            notifyResetCard(shenlvmeng, cardId);
    }

    QJsonArray gongxinArgs;
    gongxinArgs << target->objectName();
    gongxinArgs << true;
    gongxinArgs << QSgsJsonUtils::toJsonArray(target->handCardIds().values());
    gongxinArgs << QSgsJsonUtils::toJsonArray(enabled_ids);
    gongxinArgs << skill_name;
    gongxinArgs << cancellable;
    bool success = doRequest(shenlvmeng, S_COMMAND_SKILL_GONGXIN, gongxinArgs, true);
    const QJsonValue &clientReply = shenlvmeng->getClientReply();
    if (!success || !QSgsJsonUtils::isNumber(clientReply) || !target->handCardIds().contains(clientReply.toInt())) {
        if (cancellable)
            shenlvmeng->tag.remove(skill_name);
        return -1;
    }
    card_id = clientReply.toInt();

    if (card_id == -1 && !cancellable && !enabled_ids.isEmpty())
        card_id = enabled_ids.first();

    return card_id; // Do remember to remove the tag later!
}

const Card *LegacyRoom::askForPindian(LegacyServerPlayer *player, LegacyServerPlayer *from, LegacyServerPlayer *to, const QString &reason, PindianStruct *pindian)
{
    if (!from->isAlive() || !to->isAlive() || player->isKongcheng())
        return nullptr;
    Q_ASSERT(!player->isKongcheng());

    QVariant pdata = QVariant::fromValue(pindian);
    thread->trigger(QSanguosha::PindianAsked, pdata);
    pindian = pdata.value<PindianStruct *>();

    if (player == from && pindian->from_card != nullptr)
        return pindian->from_card;
    if (player == to && pindian->to_card != nullptr)
        return pindian->to_card;

    tryPause();
    notifyMoveFocus(player, S_COMMAND_PINDIAN);

    if (player->handCardNum() == 1)
        return player->handCards().first();

    bool success = doRequest(player, S_COMMAND_PINDIAN, QJsonArray() << from->objectName() << to->objectName(), true);

    QJsonArray clientReply = player->getClientReply().toArray();
    if (!success || clientReply.isEmpty() || !QSgsJsonUtils::isString(clientReply[0])) {
        int card_id = player->getRandomHandCardId();
        return card(card_id);
    } else {
        const Card *card_ = Card::Parse(clientReply[0].toString(), this);
        if (card_->isVirtualCard()) {
            const Card *real_card = card(card_->effectiveId());
            delete (card_);
            return real_card;
        } else
            return card_;
    }
}

QList<const Card *> LegacyRoom::askForPindianRace(LegacyServerPlayer *from, LegacyServerPlayer *to, const QString &reason)
{
    if (!from->isAlive() || !to->isAlive())
        return QList<const Card *>() << nullptr << nullptr;
    Q_ASSERT(!from->isKongcheng() && !to->isKongcheng());
    tryPause();
    Countdown countdown;
    countdown.max = serverInfo()->getCommandTimeout(S_COMMAND_PINDIAN, S_CLIENT_INSTANCE);
    countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
    notifyMoveFocus(QList<LegacyServerPlayer *>() << from << to, S_COMMAND_PINDIAN, countdown);

    const Card *from_card = nullptr;
    const Card *to_card = nullptr;

    if (from->handCardNum() == 1)
        from_card = from->handCards().first();
    if (to->handCardNum() == 1)
        to_card = to->handCards().first();

    if ((from_card != nullptr) && (to_card != nullptr)) {
        thread->delay();
        return QList<const Card *>() << from_card << to_card;
    }

    QList<LegacyServerPlayer *> players;
    if (from_card == nullptr) {
        QJsonArray arr;
        arr << from->objectName() << to->objectName();
        from->m_commandArgs = arr;
        players << from;
    }
    if (to_card == nullptr) {
        QJsonArray arr;
        arr << from->objectName() << to->objectName();
        to->m_commandArgs = arr;
        players << to;
    }

    doBroadcastRequest(players, S_COMMAND_PINDIAN);

    foreach (LegacyServerPlayer *player, players) {
        const Card *c = nullptr;
        QJsonArray clientReply = player->getClientReply().toArray();
        if (!player->m_isClientResponseReady || clientReply.isEmpty() || !QSgsJsonUtils::isString(clientReply[0])) {
            int card_id = player->getRandomHandCardId();
            c = card(card_id);
        } else {
            const Card *card_ = Card::Parse(clientReply[0].toString(), this);
            if (card_ == nullptr) {
                int card_id = player->getRandomHandCardId();
                c = card(card_id);
            } else if (card_->isVirtualCard()) {
                const Card *real_card = card(card_->effectiveId());
                delete (card_);
                c = real_card;
            } else
                c = card_;
        }
        if (player == from)
            from_card = c;
        else
            to_card = c;
    }
    return QList<const Card *>() << from_card << to_card;
}

LegacyServerPlayer *LegacyRoom::askForPlayerChosen(LegacyServerPlayer *player, const QList<LegacyServerPlayer *> &targets, const QString &skillName, const QString &prompt,
                                                   bool optional, bool notify_skill)
{
    if (targets.isEmpty()) {
        Q_ASSERT(optional);
        return nullptr;
    } else if (targets.length() == 1 && !optional) {
        ChoiceMadeStruct s;
        s.player = player;
        s.type = ChoiceMadeStruct::PlayerChosen;
        s.args << skillName << targets.first()->objectName();
        QVariant data = QVariant::fromValue(s);
        thread->trigger(QSanguosha::ChoiceMade, data);
        return targets.first();
    }

    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_PLAYER);
    LegacyServerPlayer *choice = nullptr;
    QJsonArray req;
    QJsonArray req_targets;
    foreach (LegacyServerPlayer *target, targets)
        req_targets << target->objectName();
    req << QJsonValue(req_targets);
    req << skillName;
    req << prompt;
    req << optional;
    bool success = doRequest(player, S_COMMAND_CHOOSE_PLAYER, req, true);

    const QJsonValue &clientReply = player->getClientReply();
    if (success && QSgsJsonUtils::isString(clientReply))
        choice = findChild<LegacyServerPlayer *>(clientReply.toString());

    if ((choice != nullptr) && !targets.contains(choice))
        choice = nullptr;
    if (choice == nullptr && !optional)
        choice = targets.at(QRandomGenerator::global()->generate() % targets.length());
    if (choice != nullptr) {
        if (notify_skill) {
            notifySkillInvoked(player, skillName);
            ChoiceMadeStruct s;
            s.player = player;
            s.type = ChoiceMadeStruct::SkillInvoke;
            s.args << skillName << QStringLiteral("yes");
            QVariant decisionData = QVariant::fromValue(s);
            thread->trigger(QSanguosha::ChoiceMade, decisionData);

            doAnimate(S_ANIMATE_INDICATE, player->objectName(), choice->objectName());
            LogStruct log;
            log.type = QStringLiteral("#ChoosePlayerWithSkill");
            log.from = player;
            log.to << choice;
            log.arg = skillName;
            sendLog(log);
        }

        ChoiceMadeStruct s;
        s.player = player;
        s.type = ChoiceMadeStruct::PlayerChosen;
        s.args << skillName << choice->objectName();
        QVariant data = QVariant::fromValue(s);
        thread->trigger(QSanguosha::ChoiceMade, data);
    } else { //for ai
        ChoiceMadeStruct s;
        s.player = player;
        s.type = ChoiceMadeStruct::PlayerChosen;
        s.args << skillName << QString();
        QVariant data = QVariant::fromValue(s);
        thread->trigger(QSanguosha::ChoiceMade, data);
    }

    return choice;
}

void LegacyRoom::_setupChooseGeneralRequestArgs(LegacyServerPlayer *player)
{
    QJsonArray options;
    if (serverInfo()->GameMode->category() == QSanguosha::ModeHegemony) {
        options << QSgsJsonUtils::toJsonArray(player->getSelected());
        options << false;
        options << false;
    } else {
        options = QSgsJsonUtils::toJsonArray(player->getSelected());
        if (getLord() != nullptr)
            options.append(QStringLiteral("%1(lord)").arg(getLord()->generalName()));
    }

    player->m_commandArgs = options;
}

QString LegacyRoom::askForGeneral(LegacyServerPlayer *player, const QStringList &generals, QString default_choice)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_GENERAL);

    if (generals.length() == 1)
        return generals.first();

    if (default_choice.isEmpty())
        default_choice = generals.at(QRandomGenerator::global()->generate() % generals.length());

    if (player->getState() == QStringLiteral("online")) {
        QJsonArray options = QSgsJsonUtils::toJsonArray(generals);
        bool success = doRequest(player, S_COMMAND_CHOOSE_GENERAL, options, true);

        QJsonValue clientResponse = player->getClientReply();
        bool free = Config.FreeChoose;
        if (!success || !QSgsJsonUtils::isString(clientResponse) || (!free && !generals.contains(clientResponse.toString())))
            return default_choice;
        else
            return clientResponse.toString();
    }
    return default_choice;
}

QString LegacyRoom::askForGeneral(LegacyServerPlayer *player, const QString &generals, const QString &default_choice)
{
    return askForGeneral(player, generals.split(QStringLiteral("+")), default_choice); // For Lua only!!!
}

bool LegacyRoom::makeCheat(LegacyServerPlayer *player)
{
    QJsonArray arg = player->m_cheatArgs.toArray();
    player->m_cheatArgs = QJsonValue();
    if (arg.isEmpty() || !QSgsJsonUtils::isNumber(arg[0]))
        return false;

    CheatCode code = (CheatCode)arg[0].toInt();
    if (code == S_CHEAT_KILL_PLAYER) {
        QJsonArray arg1 = arg[1].toArray();
        if (!QSgsJsonUtils::isStringArray(arg1, 0, 1))
            return false;
        makeKilling(arg1[0].toString(), arg1[1].toString());

    } else if (code == S_CHEAT_MAKE_DAMAGE) {
        QJsonArray arg1 = arg[1].toArray();
        if (arg1.size() != 4 || !QSgsJsonUtils::isStringArray(arg1, 0, 1) || !QSgsJsonUtils::isNumber(arg1[2]) || !QSgsJsonUtils::isNumber(arg1[3]))
            return false;
        makeDamage(arg1[0].toString(), arg1[1].toString(), (QSanProtocol::CheatCategory)arg1[2].toInt(), arg1[3].toInt());

    } else if (code == S_CHEAT_REVIVE_PLAYER) {
        if (!QSgsJsonUtils::isString(arg[1]))
            return false;
        makeReviving(arg[1].toString());

    } else if (code == S_CHEAT_RUN_SCRIPT) {
        if (!QSgsJsonUtils::isString(arg[1]))
            return false;
        QByteArray data = QByteArray::fromBase64(arg[1].toString().toLatin1());
        data = qUncompress(data);
        doScript(QString::fromUtf8(data));

    } else if (code == S_CHEAT_GET_ONE_CARD) {
        if (!QSgsJsonUtils::isNumber(arg[1]))
            return false;
        int card_id = arg[1].toInt();

        LogStruct log;
        log.type = QStringLiteral("$CheatCard");
        log.from = player;
        log.card_str = QString::number(card_id);
        sendLog(log);

        obtainCard(player, card_id);
    } else if (code == S_CHEAT_CHANGE_GENERAL) {
        if (!QSgsJsonUtils::isString(arg[1]) || !QSgsJsonUtils::isBool(arg[2]))
            return false;
        QString generalName = arg[1].toString();
        bool isSecondaryHero = arg[2].toBool();
        changeHero(player, generalName, false, true, isSecondaryHero);
    }

    return true;
}

void LegacyRoom::makeDamage(const QString &source, const QString &target, QSanProtocol::CheatCategory nature, int point)
{
    LegacyServerPlayer *sourcePlayer = findChild<LegacyServerPlayer *>(source);
    LegacyServerPlayer *targetPlayer = findChild<LegacyServerPlayer *>(target);
    if (targetPlayer == nullptr)
        return;
    // damage
    if (nature == S_CHEAT_HP_LOSE) {
        loseHp(targetPlayer, point);
        return;
    } else if (nature == S_CHEAT_MAX_HP_LOSE) {
        loseMaxHp(targetPlayer, point);
        return;
    } else if (nature == S_CHEAT_HP_RECOVER) {
        RecoverStruct r;
        r.from = sourcePlayer;
        r.num = point;
        recover(targetPlayer, r);
        return;
    } else if (nature == S_CHEAT_MAX_HP_RESET) {
        setPlayerProperty(targetPlayer, "maxhp", point);
        return;
    }

    static QMap<QSanProtocol::CheatCategory, QSanguosha::DamageNature> nature_map;
    if (nature_map.isEmpty()) {
        nature_map[S_CHEAT_NORMAL_DAMAGE] = QSanguosha::DamageNormal;
        nature_map[S_CHEAT_THUNDER_DAMAGE] = QSanguosha::DamageThunder;
        nature_map[S_CHEAT_FIRE_DAMAGE] = QSanguosha::DamageFire;
    }

    if (targetPlayer == nullptr)
        return;
    damage(DamageStruct(QStringLiteral("cheat"), sourcePlayer, targetPlayer, point, nature_map[nature]));
}

void LegacyRoom::makeKilling(const QString &killerName, const QString &victimName)
{
    LegacyServerPlayer *killer = findChild<LegacyServerPlayer *>(killerName);
    LegacyServerPlayer *victim = findChild<LegacyServerPlayer *>(victimName);

    if (victim == nullptr)
        return;

    if (killer == nullptr) {
        killPlayer(victim);
        return;
    }

    DamageStruct damage(QStringLiteral("cheat"), killer, victim);
    killPlayer(victim, &damage);
}

void LegacyRoom::makeReviving(const QString &name)
{
    LegacyServerPlayer *player = findChild<LegacyServerPlayer *>(name);
    Q_ASSERT(player);
    revivePlayer(player);
    setPlayerProperty(player, "maxhp", player->getGeneralMaxHp());
    setPlayerProperty(player, "hp", player->maxHp());
}

void LegacyRoom::doScript(const QString &script)
{
}

void LegacyRoom::fillAG(const QList<int> &card_ids, LegacyServerPlayer *who, const QList<int> &disabled_ids, const QList<int> &shownHandcard_ids)
{
    QJsonArray arg;
    arg << QSgsJsonUtils::toJsonArray(card_ids);
    arg << QSgsJsonUtils::toJsonArray(disabled_ids);
    arg << QSgsJsonUtils::toJsonArray(shownHandcard_ids);

    m_fillAGarg = arg;
    m_fillAGWho = who;

    if (who != nullptr)
        doNotify(who, S_COMMAND_FILL_AMAZING_GRACE, arg);
    else
        doBroadcastNotify(S_COMMAND_FILL_AMAZING_GRACE, arg);
}

void LegacyRoom::takeAG(LegacyServerPlayer *player, int card_id, bool move_cards, QList<LegacyServerPlayer *> to_notify, QSanguosha::Place fromPlace)
{
    if (to_notify.isEmpty())
        to_notify = getAllPlayers(true); //notice dead players

    QJsonArray arg;
    arg << (player != nullptr ? QJsonValue(player->objectName()) : QJsonValue());
    arg << card_id;
    arg << move_cards;

    if (player != nullptr) {
        LegacyCardsMoveOneTimeStruct moveOneTime;
        if (move_cards) {
            LegacyCardsMoveOneTimeStruct move;
            move.from = nullptr;
            move.from_places << fromPlace;
            move.to = player;
            move.to_place = QSanguosha::PlaceHand;
            move.card_ids << card_id;
            QVariant data = QVariant::fromValue(move);
            thread->trigger(QSanguosha::BeforeCardsMove, data);

            move = data.value<LegacyCardsMoveOneTimeStruct>();
            moveOneTime = move;

            if (move.card_ids.length() > 0) {
                if (move.to != nullptr && move.to == player) {
                    player->addCard(card(card_id), QSanguosha::PlaceHand);
                    setCardMapping(card_id, player, QSanguosha::PlaceHand);
                    card(card_id)->addFlag(QStringLiteral("visible"));
                    QList<const Card *> cards;
                    cards << card(card_id);
                    filterCards(player, cards, false);
                } else
                    arg[2] = false;
            } else {
                arg[2] = false;
            }
        }

        doBroadcastNotify(to_notify, S_COMMAND_TAKE_AMAZING_GRACE, arg);

        if (move_cards && moveOneTime.card_ids.length() > 0) {
            QVariant data = QVariant::fromValue(moveOneTime);
            thread->trigger(QSanguosha::CardsMoveOneTime, data);
        }

    } else {
        doBroadcastNotify(to_notify, S_COMMAND_TAKE_AMAZING_GRACE, arg);
        if (!move_cards)
            return;
        LogStruct log;
        log.type = QStringLiteral("$EnterDiscardPile");
        log.card_str = QString::number(card_id);
        sendLog(log);

        discardPile().prepend(card_id);
        setCardMapping(card_id, nullptr, QSanguosha::PlaceDiscardPile);
    }
    QJsonArray takeagargs = m_takeAGargs.toArray();
    takeagargs << arg;
    m_takeAGargs = takeagargs;
}

void LegacyRoom::clearAG(LegacyServerPlayer *player)
{
    m_fillAGarg = QJsonValue();
    m_fillAGWho = nullptr;
    m_takeAGargs = QJsonValue();
    if (player != nullptr)
        doNotify(player, S_COMMAND_CLEAR_AMAZING_GRACE, QJsonValue());
    else
        doBroadcastNotify(S_COMMAND_CLEAR_AMAZING_GRACE, QJsonValue());
}

void LegacyRoom::provide(const Card *card, LegacyServerPlayer *who)
{
    Q_ASSERT(provided == nullptr);
    Q_ASSERT(!has_provided);
    Q_ASSERT(provider == nullptr);

    provided = card;
    has_provided = true;
    provider = who;
}

QList<LegacyServerPlayer *> LegacyRoom::getLieges(const QString &kingdom, LegacyServerPlayer *lord) const
{
    QList<LegacyServerPlayer *> lieges;
    foreach (LegacyServerPlayer *player, getAllPlayers()) {
        if (player != lord && player->kingdom() == kingdom)
            lieges << player;
    }

    return lieges;
}

void LegacyRoom::sendLog(const LogStruct &log)
{
    if (log.type.isEmpty())
        return;

    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_SKILL, log.serialize());
}

void LegacyRoom::showCard(LegacyServerPlayer *player, int card_id, LegacyServerPlayer *only_viewer)
{
    if (cardOwner(card_id) != player)
        return;

    tryPause();
    notifyMoveFocus(player);
    QJsonArray show_arg;
    show_arg << player->objectName();
    show_arg << card_id;

    Card *card_ = card(card_id);
    bool modified = card_->isModified();
    if (only_viewer != nullptr) {
        QList<LegacyServerPlayer *> players;
        players << only_viewer << player;
        if (modified)
            notifyUpdateCard(only_viewer, card_id, card_);
        else
            notifyResetCard(only_viewer, card_id);
        doBroadcastNotify(players, S_COMMAND_SHOW_CARD, show_arg);
    } else {
        if (card_id > 0)
            card(card_id)->addFlag(QStringLiteral("visible"));
        if (modified)
            broadcastUpdateCard(getOtherPlayers(player), card_id, card_);
        else
            broadcastResetCard(getOtherPlayers(player), card_id);
        doBroadcastNotify(S_COMMAND_SHOW_CARD, show_arg);
    }
}

void LegacyRoom::showAllCards(LegacyServerPlayer *player, LegacyServerPlayer *to)
{
    if (player->isKongcheng())
        return;
    tryPause();

    QJsonArray gongxinArgs;
    gongxinArgs << player->objectName();
    gongxinArgs << false;
    gongxinArgs << QSgsJsonUtils::toJsonArray(player->handCardIds().values());

    bool isUnicast = (to != nullptr);

    foreach (int cardId, player->handCardIds()) {
        Card *card_ = card(cardId);
        if (card_->isModified()) {
            if (isUnicast)
                notifyUpdateCard(to, cardId, card_);
            else
                broadcastUpdateCard(getOtherPlayers(player), cardId, card_);
        } else {
            if (isUnicast)
                notifyResetCard(to, cardId);
            else
                broadcastResetCard(getOtherPlayers(player), cardId);
        }
    }

    if (isUnicast) {
        LogStruct log;
        log.type = QStringLiteral("$ViewAllCards");
        log.from = to;
        log.to << player;
        log.card_str = IntList2StringList(player->handCardIds().values()).join(QStringLiteral("+"));
        doNotify(to, QSanProtocol::S_COMMAND_LOG_SKILL, log.serialize());

        ChoiceMadeStruct s;
        s.player = to;
        s.type = ChoiceMadeStruct::ViewCards;
        s.args << to->objectName() << player->objectName();
        QVariant decisionData = QVariant::fromValue(s);
        thread->trigger(QSanguosha::ChoiceMade, decisionData);

        doNotify(to, S_COMMAND_SHOW_ALL_CARDS, gongxinArgs);
    } else {
        LogStruct log;
        log.type = QStringLiteral("$ShowAllCards");
        log.from = player;
        foreach (int card_id, player->handCardIds())
            card(card_id)->addFlag(QStringLiteral("visible"));
        log.card_str = IntList2StringList(player->handCardIds().values()).join(QStringLiteral("+"));
        sendLog(log);

        doBroadcastNotify(S_COMMAND_SHOW_ALL_CARDS, gongxinArgs);
    }
}

void LegacyRoom::retrial(const Card *card_, LegacyServerPlayer *player, JudgeStruct *judge, const QString &skill_name, bool exchange)
{
    if (card_ == nullptr)
        return;

    bool triggerResponded = cardOwner(card_->effectiveId()) == player;
    bool isHandcard = (triggerResponded && cardPlace(card_->effectiveId()) == QSanguosha::PlaceHand);

    const Card *oldJudge = judge->card();
    Player *rebyre = judge->retrial_by_response; //old judge provider
    judge->retrial_by_response = player;

    LegacyCardsMoveStruct move1(QList<int>(), judge->who, QSanguosha::PlaceJudge, CardMoveReason(QSanguosha::MoveReasonRetrial, player->objectName(), skill_name, QString()));

    move1.card_ids.append(card_->effectiveId());

    QSanguosha::MoveReasonCategory reasonType = exchange ? QSanguosha::MoveReasonOverride : QSanguosha::MoveReasonJudgeDone;

    CardMoveReason reason(reasonType, player->objectName(), exchange ? skill_name : QString(), QString());
    if (rebyre != nullptr)
        reason.m_extraData = QVariant::fromValue(rebyre);

    LegacyCardsMoveStruct move2(QList<int>(), judge->who, exchange ? player : nullptr, QSanguosha::PlaceUnknown, exchange ? QSanguosha::PlaceHand : QSanguosha::PlaceDiscardPile,
                                reason);
    move2.card_ids.append(oldJudge->effectiveId());

    LogStruct log;
    log.type = QStringLiteral("$ChangedJudge");
    log.arg = skill_name;
    log.from = player;
    log.to << judge->who;
    log.card_str = QString::number(card_->effectiveId());
    sendLog(log);

    QList<LegacyCardsMoveStruct> moves;
    moves.append(move1);
    moves.append(move2);
    moveCardsAtomic(moves, true);

    judge->setCard(card(card_->effectiveId()));

    if (triggerResponded) {
        CardResponseStruct resp(card_, judge->who, true, false);
        QVariant data = QVariant::fromValue(resp);
        resp.m_isHandcard = isHandcard;
        thread->trigger(QSanguosha::CardResponded, data);
    }
}

int LegacyRoom::askForRende(LegacyServerPlayer *liubei, QList<int> &cards, const QString &skill_name, bool /*unused*/, bool optional, int max_num,
                            QList<LegacyServerPlayer *> players, CardMoveReason reason, const QString &prompt, bool notify_skill)
{
    if (max_num == -1)
        max_num = cards.length();
    if (players.isEmpty())
        players = getOtherPlayers(liubei);
    if (cards.isEmpty() || max_num == 0)
        return 0;
    if (reason.m_reason == QSanguosha::MoveReasonUnknown) {
        reason.m_playerId = liubei->objectName();
        reason.m_reason = QSanguosha::MoveReasonGive;
    }
    tryPause();
    notifyMoveFocus(liubei, S_COMMAND_SKILL_YIJI);

    QMap<int, LegacyServerPlayer *> give_map;
    QList<int> remain_cards = cards;
    int num = max_num;

    while (!remain_cards.isEmpty() && num > 0) {
        QList<int> ids;
        LegacyServerPlayer *target = nullptr;
        QJsonArray arg;
        arg << QSgsJsonUtils::toJsonArray(remain_cards);
        arg << optional;
        arg << num;
        QJsonArray player_names;
        foreach (LegacyServerPlayer *player, players)
            player_names << player->objectName();
        arg << QJsonValue(player_names);
        if (!prompt.isEmpty())
            arg << prompt;
        else
            arg << QString();
        arg << QString(skill_name);

        bool success = doRequest(liubei, S_COMMAND_SKILL_YIJI, arg, true);

        //Validate client response
        QJsonArray clientReply = liubei->getClientReply().toArray();
        if (!success || clientReply.size() != 2) {
            if (!give_map.isEmpty())
                break;
            else
                return 0;
        }

        if (!QSgsJsonUtils::tryParse(clientReply[0], ids) || !QSgsJsonUtils::isString(clientReply[1]))
            return 0;

        bool foreach_flag = true;
        foreach (int id, ids)
            if (!remain_cards.contains(id)) {
                foreach_flag = false;
                break;
            }

        if (!foreach_flag)
            break;

        LegacyServerPlayer *who = findChild<LegacyServerPlayer *>(clientReply[1].toString());
        if (who == nullptr)
            break;
        else
            target = who;

        Q_ASSERT(target != nullptr);

        foreach (int id, ids) {
            remain_cards.removeOne(id);
            give_map.insert(id, target);
        }

        num -= ids.length();
    }

    while (!optional && num > 0) {
        int id = remain_cards[QRandomGenerator::global()->generate() % remain_cards.length()];
        remain_cards.removeOne(id);
        num--;
        give_map.insert(id, players[QRandomGenerator::global()->generate() % players.length()]);
    }

    if (give_map.isEmpty())
        return 0;

    cards = remain_cards;

    QStringList namelist;
    foreach (LegacyServerPlayer *p, give_map)
        namelist << p->objectName();

    ChoiceMadeStruct s;
    s.player = liubei;
    s.type = ChoiceMadeStruct::Rende;
    s.args << skill_name << liubei->objectName() << namelist.join(QStringLiteral("+")) << IntList2StringList(give_map.keys()).join(QStringLiteral("+"));
    QVariant decisionData = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, decisionData);

    if (notify_skill) {
        LogStruct log;
        log.type = QStringLiteral("#InvokeSkill");
        log.from = liubei;
        log.arg = skill_name;
        sendLog(log);

        const Skill *skill = Sanguosha->skill(skill_name);
        if (skill != nullptr)
            broadcastSkillInvoke(skill_name);
        notifySkillInvoked(liubei, skill_name);
    }

    QList<LegacyCardsMoveStruct> movelist;

    foreach (int card_id, give_map.keys()) {
        LegacyCardsMoveStruct move(card_id, give_map.value(card_id), QSanguosha::PlaceHand, reason);
        movelist << move;
    }

    liubei->setFlag(QStringLiteral("Global_GongxinOperator"));
    moveCardsAtomic(movelist, false);
    liubei->setFlag(QStringLiteral("-Global_GongxinOperator"));

    return give_map.keys().length();
}

bool LegacyRoom::askForYiji(LegacyServerPlayer *guojia, QList<int> &cards, const QString &skill_name, bool is_preview, bool visible, bool optional, int max_num,
                            QList<LegacyServerPlayer *> players, CardMoveReason reason, const QString &prompt, bool notify_skill)
{
    if (max_num == -1)
        max_num = cards.length();
    if (players.isEmpty())
        players = getOtherPlayers(guojia);
    if (cards.isEmpty() || max_num == 0)
        return false;
    if (reason.m_reason == QSanguosha::MoveReasonUnknown) {
        reason.m_playerId = guojia->objectName();
        // when we use ? : here, compiling error occurs under debug mode...
        if (is_preview)
            reason.m_reason = QSanguosha::MoveReasonPreviewGive;
        else
            reason.m_reason = QSanguosha::MoveReasonGive;
    }
    tryPause();
    notifyMoveFocus(guojia, S_COMMAND_SKILL_YIJI);

    LegacyServerPlayer *target = nullptr;

    QList<int> ids;
    QJsonArray arg;
    arg << QSgsJsonUtils::toJsonArray(cards);
    arg << optional;
    arg << max_num;
    QJsonArray player_names;
    foreach (LegacyServerPlayer *player, players)
        player_names << player->objectName();
    arg << QJsonValue(player_names);
    if (!prompt.isEmpty())
        arg << prompt;
    else
        arg << QString();
    arg << QString(skill_name);

    bool success = doRequest(guojia, S_COMMAND_SKILL_YIJI, arg, true);

    //Validate client response
    QJsonArray clientReply = guojia->getClientReply().toArray();
    if (!success || clientReply.size() != 2)
        return false;

    if (!QSgsJsonUtils::tryParse(clientReply[0], ids) || !QSgsJsonUtils::isString(clientReply[1]))
        return false;

    foreach (int id, ids)
        if (!cards.contains(id))
            return false;

    LegacyServerPlayer *who = findChild<LegacyServerPlayer *>(clientReply[1].toString());
    if (who == nullptr)
        return false;
    else
        target = who;

    Q_ASSERT(target != nullptr);

    ChoiceMadeStruct s;
    s.player = guojia;
    s.type = ChoiceMadeStruct::Yiji;
    s.args << skill_name << guojia->objectName() << target->objectName() << IntList2StringList(ids).join(QStringLiteral("+"));
    QVariant decisionData = QVariant::fromValue(s);
    thread->trigger(QSanguosha::ChoiceMade, decisionData);

    if (notify_skill) {
        LogStruct log;
        log.type = QStringLiteral("#InvokeSkill");
        log.from = guojia;
        log.arg = skill_name;
        sendLog(log);

        const Skill *skill = Sanguosha->skill(skill_name);
        if (skill != nullptr)
            broadcastSkillInvoke(skill_name);
        notifySkillInvoked(guojia, skill_name);
    }

    guojia->setFlag(QStringLiteral("Global_GongxinOperator"));
    foreach (int card_id, ids) {
        cards.removeOne(card_id);
        moveCardTo(card(card_id), target, QSanguosha::PlaceHand, reason, visible);
    }
    guojia->setFlag(QStringLiteral("-Global_GongxinOperator"));

    return true;
}

QString LegacyRoom::generatePlayerName()
{
    static unsigned int id = 0;
    id++;
    return QStringLiteral("sgs%1").arg(id);
}

QString LegacyRoom::askForOrder(LegacyServerPlayer *player, const QString &default_choice)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_ORDER);

    bool success = doRequest(player, S_COMMAND_CHOOSE_ORDER, (int)S_REASON_CHOOSE_ORDER_TURN, true);

    const QJsonValue &clientReply = player->getClientReply();
    if (success && QSgsJsonUtils::isNumber(clientReply))
        return ((Game3v3Camp)clientReply.toInt() == S_CAMP_WARM) ? QStringLiteral("warm") : QStringLiteral("cool");
    return default_choice;
}

QString LegacyRoom::askForRole(LegacyServerPlayer *player, const QStringList &roles, const QString &scheme)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_ROLE_3V3);

    QStringList squeezed = roles;
    squeezed.removeDuplicates();
    QJsonArray arg;
    arg << scheme << QSgsJsonUtils::toJsonArray(squeezed);
    bool success = doRequest(player, S_COMMAND_CHOOSE_ROLE_3V3, arg, true);
    const QJsonValue &clientReply = player->getClientReply();
    QString result = QStringLiteral("abstain");
    if (success && QSgsJsonUtils::isString(clientReply))
        result = clientReply.toString();
    return result;
}

void LegacyRoom::networkDelayTestCommand(LegacyServerPlayer *player, const QJsonValue & /*unused*/)
{
    qint64 delay = player->endNetworkDelayTest();
    QString reportStr = tr("<font color=#EEB422>The network delay of player <b>%1</b> is %2 milliseconds.</font>").arg(player->screenName(), QString::number(delay));
    speakCommand(player, QString::fromLatin1(reportStr.toUtf8().toBase64()));
}

void LegacyRoom::touhouLogmessage(const QString &logtype, LegacyServerPlayer *logfrom, const QString &logarg, const QList<LegacyServerPlayer *> &logto, const QString &logarg2)
{
    LogStruct alog;

    alog.type = logtype;
    alog.from = logfrom;
    for (LegacyServerPlayer *p : logto)
        alog.to << p;
    alog.arg = logarg;
    alog.arg2 = logarg2;

    sendLog(alog);
}

void LegacyRoom::skinChangeCommand(LegacyServerPlayer *player, const QJsonValue &packet)
{
    QJsonArray arg = packet.toArray();
    QString generalName = arg[0].toString();

    QJsonArray val;
    val << (int)QSanProtocol::S_GAME_EVENT_SKIN_CHANGED;
    val << player->objectName();
    val << generalName;
    val << arg[1].toInt();
    val << (player->generalName() == generalName);

    setTag(generalName + QStringLiteral("_skin_id"), arg[1].toInt());
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, val);
}

void LegacyRoom::heartbeatCommand(LegacyServerPlayer *player, const QJsonValue & /*unused*/)
{
    doNotify(player, QSanProtocol::S_COMMAND_HEARTBEAT, QJsonValue());
}

bool LegacyRoom::roleStatusCommand(LegacyServerPlayer *player)
{
    QJsonArray val;

    val << (int)QSanProtocol::S_GAME_ROLE_STATUS_CHANGED;
    val << player->objectName();
    val << player->hasShownRole();

    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, val);
    return true;
}

void LegacyRoom::transformGeneral(LegacyServerPlayer *player, const QString &general_name, int head)
{
#if 0
    if (!player->canTransform(head != 0))
        return; //check sujiang
#endif
    QStringList names;
    QStringList generals = getTag(player->objectName()).toStringList();
    names << generals.first() << generals.last();

    player->removeGeneral(head != 0);

    foreach (const Skill *skill, Sanguosha->general(general_name)->skills(true, head)) {
        foreach (const Trigger *trigger, skill->triggers())
            thread->addTrigger(trigger);
        player->addSkill(skill->name(), head != 0);
    }

    if (head != 0) {
        changePlayerGeneral(player, QStringLiteral("anjiang"));
        notifyProperty(player, player, "general", general_name);
        names[0] = general_name;
        setPlayerProperty(player, "general_showed", false);
    } else {
        changePlayerGeneral2(player, QStringLiteral("anjiang"));
        notifyProperty(player, player, "general2", general_name);

        names[1] = general_name;
        setPlayerProperty(player, "general2_showed", false);
    }

    setTag(player->objectName(), names);

    foreach (const Skill *skill, Sanguosha->general(general_name)->skills(true, head)) {
        if (skill->isLimited() && !skill->limitMark().isEmpty()) {
            player->setMark(skill->limitMark(), 1);
            QJsonArray arg;
            arg << player->objectName();
            arg << skill->limitMark();
            arg << 1;
            doNotify(player, S_COMMAND_SET_MARK, arg);
        }
    }

    player->showGeneral(head != 0);
}
