#include "room.h"
#include "ai.h"
#include "audio.h"
#include "engine.h"
#include "gamerule.h"
#include "generalselector.h"
#include "lua.hpp"
#include "server.h"
#include "settings.h"
#include "standard.h"
#include "structs.h"
#include "washout.h"

#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QHostAddress>
#include <QMessageBox>
#include <QMetaEnum>
#include <QStringList>
#include <QTextCodec>
#include <QTextStream>
#include <QTimer>
#include <QTimerEvent>
#include <ctime>

#ifdef QSAN_UI_LIBRARY_AVAILABLE
#pragma message WARN("UI elements detected in server side!!!")
#endif

using namespace QSanProtocol;

Room::Room(QObject *parent, const QString &mode)
    : QObject(parent)
    , _m_lastMovementId(0)
    , mode(mode)
    , current(nullptr)
    , pile1(Sanguosha->getRandomCards())
    , m_drawPile(&pile1)
    , m_discardPile(&pile2)
    , game_started(false)
    , game_started2(false)
    , game_finished(false)
    , game_paused(false)
    , L(nullptr)
    , fill_robot(false)
    , thread(nullptr)
    , _m_semRaceRequest(0)
    , _m_semRoomMutex(1)
    , _m_raceStarted(false)
    , provided(nullptr)
    , has_provided(false)
    , provider(nullptr)
    , _m_roomState(false)
    , m_fillAGWho(nullptr)
{
    static int s_global_room_id = 0;
    _m_Id = s_global_room_id++;
    player_count = Sanguosha->getPlayerCount(mode);

    initCallbacks();

    L = CreateLuaState();

    DoLuaScript(L, "lua/sanguosha.lua");
    if (isHegemonyGameMode(mode))
        DoLuaScript(L, "lua/ai/hegemony-smart-ai.lua");
    else
        DoLuaScript(L, QFile::exists("lua/ai/private-smart-ai.lua") ? "lua/ai/private-smart-ai.lua" : "lua/ai/smart-ai.lua");

    connect(this, SIGNAL(signalSetProperty(ServerPlayer *, const char *, QVariant)), this, SLOT(slotSetProperty(ServerPlayer *, const char *, QVariant)), Qt::QueuedConnection);

    m_generalSelector = new GeneralSelector(this);
}

Room::~Room()
{
    lua_close(L); // it cause a huge memory leak if we don't do this when quit
}

void Room::initCallbacks()
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
    m_callbacks[S_COMMAND_TOGGLE_READY] = &Room::toggleReadyCommand;
    m_callbacks[S_COMMAND_ADD_ROBOT] = &Room::addRobotCommand;
    m_callbacks[S_COMMAND_FILL_ROBOTS] = &Room::fillRobotsCommand;

    m_callbacks[S_COMMAND_SPEAK] = &Room::speakCommand;
    m_callbacks[S_COMMAND_TRUST] = &Room::trustCommand;
    m_callbacks[S_COMMAND_PAUSE] = &Room::pauseCommand;
    m_callbacks[S_COMMAND_SKIN_CHANGE] = &Room::skinChangeCommand;
    m_callbacks[S_COMMAND_HEARTBEAT] = &Room::heartbeatCommand;

    //Client request
    m_callbacks[S_COMMAND_NETWORK_DELAY_TEST] = &Room::networkDelayTestCommand;
    m_callbacks[S_COMMAND_PRESHOW] = &Room::processRequestPreshow;
}

ServerPlayer *Room::getCurrent() const
{
    return current;
}

void Room::setCurrent(ServerPlayer *current)
{
    this->current = current;
}

int Room::alivePlayerCount() const
{
    return m_alivePlayers.count();
}

bool Room::notifyUpdateCard(ServerPlayer *player, int cardId, const Card *newCard)
{
    JsonArray val;
    Q_ASSERT(newCard);
    QString className = newCard->getClassName();
    val << cardId << newCard->getSuit() << newCard->getNumber() << className << newCard->getSkillName() << newCard->objectName() << JsonUtils::toJsonArray(newCard->getFlags());
    doNotify(player, S_COMMAND_UPDATE_CARD, val);
    return true;
}

bool Room::broadcastUpdateCard(const QList<ServerPlayer *> &players, int cardId, const Card *newCard)
{
    foreach (ServerPlayer *player, players)
        notifyUpdateCard(player, cardId, newCard);
    return true;
}

bool Room::notifyResetCard(ServerPlayer *player, int cardId)
{
    doNotify(player, S_COMMAND_UPDATE_CARD, cardId);
    return true;
}

bool Room::broadcastResetCard(const QList<ServerPlayer *> &players, int cardId)
{
    foreach (ServerPlayer *player, players)
        notifyResetCard(player, cardId);
    return true;
}

QList<ServerPlayer *> Room::getPlayers() const
{
    return m_players;
}

QList<ServerPlayer *> Room::getAllPlayers(bool include_dead) const
{
    QList<ServerPlayer *> count_players = m_players;
    if (current == nullptr)
        return count_players;

    ServerPlayer *starter = current;
    if (current->getPhase() == Player::NotActive)
        starter = qobject_cast<ServerPlayer *>(current->getNextAlive(1, false));
    int index = count_players.indexOf(starter);
    if (index == -1)
        return count_players;

    QList<ServerPlayer *> all_players;
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

QList<ServerPlayer *> Room::getOtherPlayers(ServerPlayer *except, bool include_dead) const
{
    QList<ServerPlayer *> other_players = getAllPlayers(include_dead);
    if ((except != nullptr) && (except->isAlive() || include_dead))
        other_players.removeOne(except);
    return other_players;
}

QList<ServerPlayer *> Room::getAlivePlayers() const
{
    return m_alivePlayers;
}

void Room::output(const QString &message)
{
    emit room_message(message);
}

void Room::outputEventStack()
{
    QString msg = "End of Event Stack.";
    foreach (EventTriplet triplet, *thread->getEventStack())
        msg.prepend(triplet.toString());
    msg.prepend("Event Stack:\n");
    output(msg);
}

void Room::enterDying(ServerPlayer *player, DamageStruct *reason)
{
    if (player->dyingThreshold() > player->getMaxHp()) {
        killPlayer(player, reason);
        return;
    }

    setPlayerFlag(player, "Global_Dying");
    QStringList currentdying = getTag("CurrentDying").toStringList();
    currentdying << player->objectName();
    setTag("CurrentDying", QVariant::fromValue(currentdying));

    JsonArray arg;
    arg << QSanProtocol::S_GAME_EVENT_PLAYER_DYING << player->objectName();
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

    DyingStruct dying;
    dying.who = player;
    dying.damage = reason;
    QVariant dying_data = QVariant::fromValue(dying);

    bool enterdying = thread->trigger(EnterDying, this, dying_data);

    if (!(player->isDead() || player->getHp() >= player->dyingThreshold() || enterdying)) {
        thread->trigger(Dying, this, dying_data);
        if (player->isAlive()) {
            if (player->getHp() >= player->dyingThreshold()) {
                setPlayerFlag(player, "-Global_Dying");
            } else {
                LogMessage log;
                log.type = "#AskForPeaches";
                log.from = player;
                log.to = getAllPlayers();
                log.arg = QString::number(player->dyingThreshold() - player->getHp());
                sendLog(log);

                foreach (ServerPlayer *saver, getAllPlayers()) {
                    DyingStruct dying = dying_data.value<DyingStruct>();
                    dying.nowAskingForPeaches = saver;
                    dying_data = QVariant::fromValue(dying);
                    if (player->getHp() >= player->dyingThreshold() || player->isDead())
                        break;
                    QString cd = saver->property("currentdying").toString();
                    setPlayerProperty(saver, "currentdying", player->objectName());
                    saver->tag["songzang_dying"] = dying_data; //record for ai, like skill songzang

                    thread->trigger(AskForPeaches, this, dying_data);
                    setPlayerProperty(saver, "currentdying", cd);
                }
                DyingStruct dying = dying_data.value<DyingStruct>();
                dying.nowAskingForPeaches = nullptr;
                dying_data = QVariant::fromValue(dying);
                thread->trigger(AskForPeachesDone, this, dying_data);

                setPlayerFlag(player, "-Global_Dying");
            }
        }

    } else {
        setPlayerFlag(player, "-Global_Dying");
    }

    currentdying = getTag("CurrentDying").toStringList();
    currentdying.removeOne(player->objectName());
    setTag("CurrentDying", QVariant::fromValue(currentdying));

    if (player->isAlive()) {
        JsonArray arg;
        arg << QSanProtocol::S_GAME_EVENT_PLAYER_QUITDYING << player->objectName();
        doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
    }
    thread->trigger(QuitDying, this, dying_data);
}

ServerPlayer *Room::getCurrentDyingPlayer() const
{
    QStringList currentdying = getTag("CurrentDying").toStringList();
    if (currentdying.isEmpty())
        return nullptr;
    QString dyingobj = currentdying.last();
    ServerPlayer *who = nullptr;
    foreach (ServerPlayer *p, m_alivePlayers) {
        if (p->objectName() == dyingobj) {
            who = p;
            break;
        }
    }
    return who;
}

void Room::revivePlayer(ServerPlayer *player, bool initialize)
{
    player->setAlive(true);
    player->throwAllMarks(false);
    broadcastProperty(player, "alive");

    if (initialize) {
        setPlayerProperty(player, "maxhp", player->getGeneral()->getMaxHp());
        setPlayerProperty(player, "hp", player->getMaxHp());
        setEmotion(player, "revive");
        sendLog("#Revive", player);

        foreach (const Skill *skill, player->getVisibleSkillList()) {
            if (skill->getFrequency() == Skill::Limited && !skill->getLimitMark().isEmpty() && (!skill->isLordSkill() || player->hasLordSkill(skill->objectName())))
                setPlayerMark(player, skill->getLimitMark(), 1);
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
        setPlayerProperty(player, "role_shown", player->isLord() ? true : false);

        JsonArray args;
        args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

        if (player->getPhase() == Player::Play)
            setPlayerFlag(player, "Global_PlayPhaseTerminated");
        if (player->isCurrent())
            setPlayerFlag(player, "Global_TurnTerminated");
    }

    m_alivePlayers.clear();
    foreach (ServerPlayer *player, m_players) {
        if (player->isAlive())
            m_alivePlayers << player;
    }

    for (int i = 0; i < m_alivePlayers.length(); i++) {
        m_alivePlayers.at(i)->setSeat(i + 1);
        broadcastProperty(m_alivePlayers.at(i), "seat");
    }

    doBroadcastNotify(S_COMMAND_REVIVE_PLAYER, QVariant(player->objectName()));
    updateStateItem();

    QVariant v = QVariant::fromValue(player);
    thread->trigger(Revive, this, v);
}

void Room::updateStateItem()
{
    QList<ServerPlayer *> players = m_players;
    std::sort(players.begin(), players.end(), [](ServerPlayer *player1, ServerPlayer *player2) -> bool {
        int role1 = player1->getRoleEnum();
        int role2 = player2->getRoleEnum();

        if (role1 != role2)
            return role1 < role2;
        else
            return player1->isAlive();
    });
    QString roles;
    foreach (ServerPlayer *p, players) {
        QChar c = "ZCFN"[p->getRoleEnum()];
        if (p->isDead())
            c = c.toLower();

        roles.append(c);
    }

    doBroadcastNotify(S_COMMAND_UPDATE_STATE_ITEM, QVariant(roles));
}

void Room::killPlayer(ServerPlayer *victim, DamageStruct *reason)
{
    ServerPlayer *killer = reason != nullptr ? reason->from : nullptr;

    victim->setAlive(false);

    int index = m_alivePlayers.indexOf(victim);
    for (int i = index + 1; i < m_alivePlayers.length(); i++) {
        ServerPlayer *p = m_alivePlayers.at(i);
        p->setSeat(p->getSeat() - 1);
        broadcastProperty(p, "seat");
    }

    m_alivePlayers.removeOne(victim);

    DeathStruct death;
    death.who = victim;
    death.damage = reason;
    QVariant data = QVariant::fromValue(death);
    thread->trigger(BeforeGameOverJudge, this, data);

    updateStateItem();

    LogMessage log;
    log.type = killer != nullptr ? (killer == victim ? "#Suicide" : "#Murder") : "#Contingency";
    log.to << victim;
    log.arg = victim->getRole();
    log.from = killer;
    sendLog(log);

    broadcastProperty(victim, "alive");
    broadcastProperty(victim, "role");
    if (isNormalGameMode(mode))
        setPlayerProperty(victim, "role_shown", true);

    doBroadcastNotify(S_COMMAND_KILL_PLAYER, QVariant(victim->objectName()));

    thread->trigger(GameOverJudge, this, data);

    thread->trigger(Death, this, data);

    doNotify(victim, S_COMMAND_SET_DASHBOARD_SHADOW, QVariant(victim->objectName()));

    victim->detachAllSkills();
    thread->trigger(BuryVictim, this, data);
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
}

void Room::judge(JudgeStruct &judge_struct)
{
    Q_ASSERT(judge_struct.who != nullptr);

    JudgeStruct *judge_star = &judge_struct;
    QVariant data = QVariant::fromValue(judge_star);
    thread->trigger(StartJudge, this, data);
    thread->trigger(AskForRetrial, this, data);
    thread->trigger(FinishRetrial, this, data);
    thread->trigger(FinishJudge, this, data);
}

void Room::sendJudgeResult(const JudgeStruct *judge)
{
    JsonArray arg;
    arg << QSanProtocol::S_GAME_EVENT_JUDGE_RESULT << judge->card->getEffectiveId() << judge->isEffected() << judge->who->objectName() << judge->reason;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
}

QList<int> Room::getNCards(int n, bool update_pile_number, bool bottom)
{
    QList<int> card_ids;
    for (int i = 0; i < n; i++)
        card_ids << drawCard(bottom);

    if (update_pile_number)
        doBroadcastNotify(S_COMMAND_UPDATE_PILE, QVariant(m_drawPile->length()));

    return card_ids;
}

void Room::returnToDrawPile(const QList<int> &cards, bool bottom)
{
    QListIterator<int> i(cards);
    i.toBack();
    while (i.hasPrevious()) {
        int id = i.previous();
        setCardMapping(id, nullptr, Player::DrawPile);
        if (!bottom)
            m_drawPile->prepend(id);
        else
            m_drawPile->append(id);
    }
    doBroadcastNotify(S_COMMAND_UPDATE_PILE, QVariant(m_drawPile->length()));
}

QStringList Room::aliveRoles(ServerPlayer *except) const
{
    QStringList roles;
    foreach (ServerPlayer *player, m_alivePlayers) {
        if (player != except)
            roles << player->getRole();
    }

    return roles;
}

void Room::gameOver(const QString &winner, bool isSurrender)
{
    QStringList all_roles;
    foreach (ServerPlayer *player, m_players) {
        all_roles << player->getRole();
        if (player->getHandcardNum() > 0) {
            QStringList handcards;
            foreach (const Card *card, player->getHandcards())
                handcards << Sanguosha->getEngineCard(card->getId())->getLogName();
            QString handcard = handcards.join(", ").toUtf8().toBase64();
            setPlayerProperty(player, "last_handcards", handcard);
        }
    }

    game_finished = true;
    saveWinnerTable(winner, isSurrender);

    //defaultHeroSkin();
    emit game_over(winner);

    Config.AIDelay = Config.OriginAIDelay;

    if (!getTag("NextGameMode").toString().isNull()) {
        QString name = getTag("NextGameMode").toString();
        Config.GameMode = name;
        Config.setValue("GameMode", name);
        removeTag("NextGameMode");
    }
    if (!getTag("NextGameSecondGeneral").isNull()) {
        bool enable = getTag("NextGameSecondGeneral").toBool();
        Config.Enable2ndGeneral = enable;
        Config.setValue("Enable2ndGeneral", enable);
        removeTag("NextGameSecondGeneral");
    }

    JsonArray arg;
    arg << winner << JsonUtils::toJsonArray(all_roles);
    doBroadcastNotify(S_COMMAND_GAME_OVER, arg);
    throw GameFinished;
}

void Room::slashEffect(const SlashEffectStruct &effect)
{
    QVariant data = QVariant::fromValue(effect);
    if (thread->trigger(SlashEffected, this, data)) {
        if (!effect.to->hasFlag("Global_NonSkillNullify"))
            ; //setEmotion(effect.to, "skill_nullify");
        else
            effect.to->setFlags("-Global_NonSkillNullify");
        if (effect.slash != nullptr)
            effect.to->removeQinggangTag(effect.slash);
    }
}

void Room::slashResult(const SlashEffectStruct &effect, const Card *jink)
{
    SlashEffectStruct result_effect = effect;
    result_effect.jink = jink;
    QVariant data = QVariant::fromValue(result_effect);

    if (jink == nullptr && !effect.canceled) {
        if (effect.to->isAlive())
            thread->trigger(SlashHit, this, data);
    } else {
        if (effect.to->isAlive() && effect.jink != nullptr) {
            if (jink->getSkillName() != "eight_diagram")
                setEmotion(effect.to, "jink");
        }
        if (effect.slash != nullptr)
            effect.to->removeQinggangTag(effect.slash);
        thread->trigger(SlashMissed, this, data);
    }
}

void Room::attachSkillToPlayer(ServerPlayer *player, const QString &skill_name, bool is_other_attach)
{
    player->acquireSkill(skill_name);
    if (is_other_attach) {
        QStringList attach_skills = getTag("OtherAttachSkills").toStringList();
        if (!attach_skills.contains(skill_name)) {
            attach_skills << skill_name;
            setTag("OtherAttachSkills", QVariant::fromValue(attach_skills));
        }
    }
    doNotify(player, S_COMMAND_ATTACH_SKILL, QVariant(skill_name));
}

void Room::detachSkillFromPlayer(ServerPlayer *player, const QString &skill_name, bool is_equip, bool acquire_only, bool sendlog, bool head)
{
    if (!isHegemonyGameMode(mode) && !player->hasSkill(skill_name, true))
        return;
    if ((Sanguosha->getSkill(skill_name) != nullptr) && Sanguosha->getSkill(skill_name)->getFrequency() == Skill::Eternal)
        return;
    //if (player->getAcquiredSkills(head ? "head" : "deputy").contains(skill_name))
    if (player->getAcquiredSkills().contains(skill_name))
        player->detachSkill(skill_name, head);
    else if (!acquire_only)
        player->loseSkill(skill_name, head);
    else
        return;

    const Skill *skill = Sanguosha->getSkill(skill_name);
    if ((skill != nullptr) && skill->isVisible()) {
        JsonArray args;
        args << QSanProtocol::S_GAME_EVENT_DETACH_SKILL << player->objectName() << skill_name << head; //default head?
        doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

        if (!is_equip) {
            if (sendlog) {
                LogMessage log;
                log.type = "#LoseSkill";
                log.from = player;
                log.arg = skill_name;
                sendLog(log);
            }

            SkillAcquireDetachStruct s;
            s.player = player;
            s.skill = skill;
            s.isAcquire = false;
            QVariant data = QVariant::fromValue(s);
            thread->trigger(EventLoseSkill, this, data);
        }

        foreach (const Skill *skill, Sanguosha->getRelatedSkills(skill_name)) {
            if (skill->isVisible())
                detachSkillFromPlayer(player, skill->objectName(), is_equip, acquire_only, sendlog, head);
            //detachSkillFromPlayer(player, skill->objectName());
        }
    }
}

void Room::handleAcquireDetachSkills(ServerPlayer *player, const QStringList &skill_names, bool acquire_only)
{
    if (skill_names.isEmpty())
        return;
    QList<bool> isAcquire;
    QList<const Skill *> triggerList;

    foreach (const QString &skill_name, skill_names) {
        if (skill_name.startsWith("-")) {
            QString actual_skill = skill_name.mid(1);
            bool head = true;
            if (actual_skill.endsWith("!")) {
                actual_skill.chop(1);
                head = false;
            }

            if (isHegemonyGameMode(mode)) {
                if (!player->ownSkill(actual_skill) && !player->hasSkill(actual_skill, true))
                    continue;
            } else {
                if (!player->hasSkill(actual_skill, true))
                    continue;
            }

            if ((Sanguosha->getSkill(actual_skill) != nullptr) && Sanguosha->getSkill(actual_skill)->getFrequency() == Skill::Eternal)
                continue;

            if (player->getAcquiredSkills().contains(actual_skill))
                player->detachSkill(actual_skill, head);
            else if (!acquire_only)
                player->loseSkill(actual_skill, head);
            else
                continue;
            const Skill *skill = Sanguosha->getSkill(actual_skill);
            if ((skill != nullptr) && skill->isVisible()) {
                JsonArray args;
                args << QSanProtocol::S_GAME_EVENT_DETACH_SKILL << player->objectName() << actual_skill << head;
                doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

                LogMessage log;
                log.type = "#LoseSkill";
                log.from = player;
                log.arg = actual_skill;
                sendLog(log);

                triggerList << skill;
                isAcquire << false;

                foreach (const Skill *skill, Sanguosha->getRelatedSkills(actual_skill)) {
                    if (!skill->isVisible())
                        detachSkillFromPlayer(player, skill->objectName());
                }
            }
        } else {
            bool head = true;
            QString actual_skill = skill_name;
            if (actual_skill.endsWith("!")) {
                actual_skill.chop(1);
                head = false;
            }
            const Skill *skill = Sanguosha->getSkill(actual_skill);
            if (skill == nullptr)
                continue;
            if (player->getAcquiredSkills().contains(actual_skill))
                continue;
            player->acquireSkill(actual_skill, head);
            if (skill->inherits("TriggerSkill")) {
                const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
                thread->addTriggerSkill(trigger_skill);
            }
            if (skill->getFrequency() == Skill::Limited && !skill->getLimitMark().isEmpty())
                //addPlayerMark(player, skill->getLimitMark());
                setPlayerMark(player, skill->getLimitMark(), 1);
            if (skill->isVisible()) {
                JsonArray args;
                args << QSanProtocol::S_GAME_EVENT_ACQUIRE_SKILL << player->objectName() << actual_skill << head;
                doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

                foreach (const Skill *related_skill, Sanguosha->getRelatedSkills(actual_skill)) {
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
            SkillAcquireDetachStruct s;
            s.skill = triggerList.value(i);
            s.player = player;
            s.isAcquire = isAcquire.value(i);
            QVariant data = QVariant::fromValue(s);
            thread->trigger(isAcquire.at(i) ? EventAcquireSkill : EventLoseSkill, this, data);
        }
    }
}

void Room::handleAcquireDetachSkills(ServerPlayer *player, const QString &skill_names, bool acquire_only)
{
    handleAcquireDetachSkills(player, skill_names.split("|"), acquire_only);
}

bool Room::doRequest(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg, bool wait)
{
    time_t timeOut = ServerInfo.getCommandTimeout(command, S_SERVER_INSTANCE, Sanguosha->operationTimeRate(command, arg));
    return doRequest(player, command, arg, timeOut, wait);
}

bool Room::doRequest(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg, time_t timeOut, bool wait)
{
    Packet packet(S_SRC_ROOM | S_TYPE_REQUEST | S_DEST_CLIENT, command);
    packet.setMessageBody(arg);
    player->acquireLock(ServerPlayer::SEMA_MUTEX);
    player->m_isClientResponseReady = false;
    player->drainLock(ServerPlayer::SEMA_COMMAND_INTERACTIVE);
    player->setClientReply(QVariant());
    player->setClientReplyString(QString());
    player->m_isWaitingReply = true;
    player->m_expectedReplySerial = packet.globalSerial;
    if (m_requestResponsePair.contains(command))
        player->m_expectedReplyCommand = m_requestResponsePair[command];
    else
        player->m_expectedReplyCommand = command;

    player->invoke(&packet);
    player->releaseLock(ServerPlayer::SEMA_MUTEX);
    if (wait)
        return getResult(player, timeOut);
    else
        return true;
}

bool Room::doBroadcastRequest(QList<ServerPlayer *> &players, QSanProtocol::CommandType command)
{
    time_t timeOut = ServerInfo.getCommandTimeout(command, S_SERVER_INSTANCE);
    return doBroadcastRequest(players, command, timeOut);
}

bool Room::doBroadcastRequest(QList<ServerPlayer *> &players, QSanProtocol::CommandType command, time_t timeOut)
{
    foreach (ServerPlayer *player, players)
        doRequest(player, command, player->m_commandArgs, timeOut, false);

    QTime timer;
    time_t remainTime = timeOut;
    timer.start();
    foreach (ServerPlayer *player, players) {
        remainTime = timeOut - timer.elapsed();
        if (remainTime < 0)
            remainTime = 0;
        getResult(player, remainTime);
    }
    return true;
}

ServerPlayer *Room::doBroadcastRaceRequest(QList<ServerPlayer *> &players, QSanProtocol::CommandType command, time_t timeOut, ResponseVerifyFunction validateFunc, void *funcArg)
{
    _m_semRoomMutex.acquire();
    _m_raceStarted = true;
    _m_raceWinner.store(nullptr);
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
    foreach (ServerPlayer *player, players)
        doRequest(player, command, player->m_commandArgs, timeOut, false);

    ServerPlayer *winner = getRaceResult(players, command, timeOut, validateFunc, funcArg);
    return winner;
}

ServerPlayer *Room::getRaceResult(QList<ServerPlayer *> &players, QSanProtocol::CommandType, time_t timeOut, ResponseVerifyFunction validateFunc, void *funcArg)
{
    QTime timer;
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

        ServerPlayer *winner = _m_raceWinner.loadAcquire();
        if (winner == nullptr) {
            _m_raceWinner.storeRelease(winner);
            _m_semRoomMutex.release();
            continue;
        }

        // original line is:
        // if (validateFunc == NULL || (winner->m_isClientResponseReady && (this->*validateFunc)(winner, winner->getClientReply(), funcArg))) {
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
    foreach (ServerPlayer *player, players) {
        player->acquireLock(ServerPlayer::SEMA_MUTEX);
        player->m_expectedReplyCommand = S_COMMAND_UNKNOWN;
        player->m_isWaitingReply = false;
        player->m_expectedReplySerial = -1;
        player->releaseLock(ServerPlayer::SEMA_MUTEX);
    }
    _m_semRoomMutex.release();
    return _m_raceWinner.fetchAndStoreRelease(nullptr);
}

bool Room::doNotify(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg)
{
    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, command);
    packet.setMessageBody(arg);
    player->invoke(&packet);
    return true;
}

bool Room::doBroadcastNotify(const QList<ServerPlayer *> &players, QSanProtocol::CommandType command, const QVariant &arg)
{
    foreach (ServerPlayer *player, players)
        doNotify(player, command, arg);
    return true;
}

bool Room::doBroadcastNotify(QSanProtocol::CommandType command, const QVariant &arg)
{
    return doBroadcastNotify(m_players, command, arg);
}

// the following functions for Lua

bool Room::doNotify(ServerPlayer *player, int command, const char *arg)
{
    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, (QSanProtocol::CommandType)command);
    JsonDocument doc = JsonDocument::fromJson(arg);
    if (doc.isValid()) {
        packet.setMessageBody(doc.toVariant());
        player->invoke(&packet);
    } else {
        output(QString("Fail to parse the Json Value %1").arg(arg));
    }
    return true;
}

bool Room::doBroadcastNotify(const QList<ServerPlayer *> &players, int command, const char *arg)
{
    foreach (ServerPlayer *player, players)
        doNotify(player, command, arg);
    return true;
}

bool Room::doBroadcastNotify(int command, const char *arg)
{
    return doBroadcastNotify(m_players, command, arg);
}

void Room::broadcastInvoke(const char *method, const QString &arg, ServerPlayer *except)
{
    broadcast(QString("%1 %2").arg(method, arg), except);
}

void Room::broadcastInvoke(const QSanProtocol::AbstractPacket *packet, ServerPlayer *except)
{
    broadcast(packet->toString(), except);
}

bool Room::getResult(ServerPlayer *player, time_t timeOut)
{
    Q_ASSERT(player->m_isWaitingReply);
    bool validResult = false;
    player->acquireLock(ServerPlayer::SEMA_MUTEX);

    if (player->isOnline()) {
        player->releaseLock(ServerPlayer::SEMA_MUTEX);

        if (Config.OperationNoLimit)
            player->acquireLock(ServerPlayer::SEMA_COMMAND_INTERACTIVE);
        else
            player->tryAcquireLock(ServerPlayer::SEMA_COMMAND_INTERACTIVE, timeOut);

        // Note that we rely on processResponse to filter out all unrelevant packet.
        // By the time the lock is released, m_clientResponse must be the right message
        // assuming the client side is not tampered.

        // Also note that lock can be released when a player switch to trust or offline status.
        // It is ensured by trustCommand and reportDisconnection that the player reports these status
        // is the player waiting the lock. In these cases, the serial number and command type doesn't matter.
        player->acquireLock(ServerPlayer::SEMA_MUTEX);
        validResult = player->m_isClientResponseReady;
    }
    player->m_expectedReplyCommand = S_COMMAND_UNKNOWN;
    player->m_isWaitingReply = false;
    player->m_expectedReplySerial = -1;
    player->releaseLock(ServerPlayer::SEMA_MUTEX);
    return validResult;
}

bool Room::notifyMoveFocus(ServerPlayer *player)
{
    QList<ServerPlayer *> players;
    players.append(player);
    Countdown countdown;
    countdown.type = Countdown::S_COUNTDOWN_NO_LIMIT;
    notifyMoveFocus(players, S_COMMAND_MOVE_FOCUS, countdown);
    return notifyMoveFocus(players, S_COMMAND_MOVE_FOCUS, countdown);
}

bool Room::notifyMoveFocus(ServerPlayer *player, CommandType command)
{
    QList<ServerPlayer *> players;
    players.append(player);
    Countdown countdown;
    countdown.max = ServerInfo.getCommandTimeout(command, S_CLIENT_INSTANCE);
    countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
    return notifyMoveFocus(players, S_COMMAND_MOVE_FOCUS, countdown);
}

bool Room::notifyMoveFocus(const QList<ServerPlayer *> &players, CommandType command, Countdown countdown)
{
    JsonArray arg;
    JsonArray arg1;
    int n = players.length();
    for (int i = 0; i < n; ++i)
        arg1 << players.value(i)->objectName();
    arg << QVariant(arg1) << command << countdown.toVariant();
    return doBroadcastNotify(S_COMMAND_MOVE_FOCUS, arg);
}

bool Room::askForSkillInvoke(ServerPlayer *player, const QString &skill_name, const QVariant &data, const QString &prompt)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_INVOKE_SKILL);

    bool invoked = false;
    AI *ai = player->getAI();
    if (ai != nullptr) {
        invoked = ai->askForSkillInvoke(skill_name, data);
        const Skill *skill = Sanguosha->getSkill(skill_name);
        if (invoked && !((skill != nullptr) && skill->getFrequency() == Skill::Frequent))
            thread->delay();
    } else {
        JsonArray skillCommand;
        if (!prompt.isEmpty())
            skillCommand << skill_name << prompt;
        else if (data.type() == QVariant::String)
            skillCommand << skill_name << data.toString();
        else {
            ServerPlayer *player = data.value<ServerPlayer *>();
            QString data_str;
            if (player != nullptr)
                data_str = "playerdata:" + player->objectName();
            skillCommand << skill_name << data_str;
        }

        if (!doRequest(player, S_COMMAND_INVOKE_SKILL, skillCommand, true)) {
            invoked = false;
        } else {
            QVariant clientReply = player->getClientReply();
            if (clientReply.canConvert(QVariant::Bool))
                invoked = clientReply.toBool();
        }
    }

    if (invoked) {
        JsonArray msg;
        msg << skill_name << player->objectName();
        doBroadcastNotify(S_COMMAND_INVOKE_SKILL, msg);
        notifySkillInvoked(player, skill_name);
    }

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::SkillInvoke;
    s.args << skill_name << (invoked ? "yes" : "no");
    s.m_extraData = data;
    QVariant d = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, d);
    return invoked;
}

bool Room::askForSkillInvoke(ServerPlayer *player, const Skill *skill, const QVariant &data /* = QVariant() */, const QString &prompt)
{
    if (skill == nullptr)
        return false;

    return askForSkillInvoke(player, skill->objectName(), data, prompt);
}

QString Room::askForChoice(ServerPlayer *player, const QString &skill_name, const QString &choices, const QVariant &data)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_MULTIPLE_CHOICE);
    //QStringList validChoices = choices.split("+");
    QStringList validChoices;
    foreach (const QString &choice, choices.split("|"))
        validChoices.append(choice.split("+"));
    Q_ASSERT(!validChoices.isEmpty());
    QStringList titles = skill_name.split("%");
    QString skillname = titles.at(0);

    AI *ai = player->getAI();
    QString answer;
    if (validChoices.size() == 1)
        answer = validChoices.first();
    else {
        if (ai != nullptr) {
            answer = ai->askForChoice(skillname, choices, data);
            thread->delay();
        } else {
            bool success = doRequest(player, S_COMMAND_MULTIPLE_CHOICE, JsonArray() << skillname << choices, true);
            QVariant clientReply = player->getClientReply();
            if (!success || !clientReply.canConvert(QVariant::String)) {
                answer = "cancel";
            } else
                answer = clientReply.toString();
        }
    }

    if (!validChoices.contains(answer))
        answer = validChoices.at(qrand() % validChoices.length());
    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::SkillChoice;
    s.args << skillname << answer;
    s.m_extraData = data;
    QVariant d = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, d);
    return answer;
}

void Room::obtainCard(ServerPlayer *target, const Card *card, const CardMoveReason &reason, bool unhide)
{
    if (card == nullptr)
        return;
    moveCardTo(card, nullptr, target, Player::PlaceHand, reason, unhide);
}

void Room::obtainCard(ServerPlayer *target, const Card *card, bool unhide)
{
    if (card == nullptr)
        return;
    CardMoveReason reason(CardMoveReason::S_REASON_GOTBACK, target->objectName());
    obtainCard(target, card, reason, unhide);
}

void Room::obtainCard(ServerPlayer *target, int card_id, bool unhide)
{
    obtainCard(target, Sanguosha->getCard(card_id), unhide);
}

bool Room::isCanceled(const CardEffectStruct &effect)
{
    QVariant edata = QVariant::fromValue(effect);
    if (getThread()->trigger(Cancel, this, edata)) {
        CardEffectStruct effect1 = edata.value<CardEffectStruct>();
        return effect1.canceled;
    }

    if (!effect.card->isCancelable(effect))
        return false;

    //for HegNullification
    QStringList targets = getTag(effect.card->toString() + "HegNullificationTargets").toStringList();
    if (!targets.isEmpty()) {
        if (targets.contains(effect.to->objectName())) {
            LogMessage log;
            log.type = "#HegNullificationEffect";
            log.from = effect.from;
            log.to << effect.to;
            log.arg = effect.card->objectName();
            sendLog(log);

            setEmotion(effect.to, "skill_nullify");
            return true;
        }
    }

    setTag("HegNullificationValid", false);

    QVariant decisionData = QVariant::fromValue(effect.to);
    setTag("NullifyingTarget", decisionData);
    decisionData = QVariant::fromValue(effect.from);
    setTag("NullifyingSource", decisionData);
    decisionData = QVariant::fromValue(effect.card);
    setTag("NullifyingCard", decisionData);
    setTag("NullifyingTimes", 0);

    bool result = askForNullification(effect.card, effect.from, effect.to, true);
    if (getTag("HegNullificationValid").toBool() && effect.card->isNDTrick()) {
        foreach (ServerPlayer *p, m_players) {
            if (p->isAlive() && p->isFriendWith(effect.to))
                targets << p->objectName();
        }
        setTag(effect.card->toString() + "HegNullificationTargets", targets);
    }

    //deal xianshi
    foreach (ServerPlayer *p, getAlivePlayers()) {
        ServerPlayer *target = p->tag["xianshi_nullification_target"].value<ServerPlayer *>();
        p->tag.remove("xianshi_nullification_target");
        const Card *xianshi_nullification = p->tag.value("xianshi_nullification").value<const Card *>();
        p->tag.remove("xianshi_nullification");

        if ((target != nullptr) && (xianshi_nullification != nullptr)) {
            int xianshi_result = p->tag["xianshi_nullification_result"].toInt();
            p->tag.remove("xianshi_nullification_result");

            if ((xianshi_result == 0 && !result) || (xianshi_result == 1 && result)) {
                QString xianshi_name = p->property("xianshi_card").toString();
                if (xianshi_name != nullptr && p->isAlive() && target->isAlive()) {
                    Card *extraCard = Sanguosha->cloneCard(xianshi_name);
                    if (extraCard->isKindOf("Slash")) {
                        DamageStruct::Nature nature = DamageStruct::Normal;
                        if (extraCard->isKindOf("FireSlash"))
                            nature = DamageStruct::Fire;
                        else if (extraCard->isKindOf("ThunderSlash"))
                            nature = DamageStruct::Thunder;
                        int damageValue = 1;

                        if (extraCard->isKindOf("DebuffSlash")) {
                            SlashEffectStruct extraEffect;
                            extraEffect.from = p;
                            extraEffect.slash = extraCard;
                            extraEffect.to = target;

                            if (extraCard->isKindOf("IronSlash"))
                                IronSlash::debuffEffect(extraEffect);
                            else if (extraCard->isKindOf("LightSlash"))
                                LightSlash::debuffEffect(extraEffect);
                            else if (extraCard->isKindOf("PowerSlash"))
                                PowerSlash::debuffEffect(extraEffect);
                        }

                        if (!extraCard->isKindOf("LightSlash") && !extraCard->isKindOf("PowerSlash")) {
                            damageValue = damageValue + effect.effectValue.first();
                        }

                        DamageStruct d = DamageStruct(xianshi_nullification, p, target, damageValue, nature);
                        damage(d);

                    } else if (extraCard->isKindOf("Peach")) {
                        CardEffectStruct extraEffect;
                        extraCard->deleteLater();

                        extraEffect.card = xianshi_nullification;
                        extraEffect.from = p;
                        extraEffect.to = target;
                        extraEffect.multiple = effect.multiple;
                        extraCard->onEffect(extraEffect);
                    } else if (extraCard->isKindOf("Analeptic")) {
                        RecoverStruct re;
                        re.card = xianshi_nullification;
                        re.who = p;
                        recover(target, re);
                    } else if (extraCard->isKindOf("AmazingGrace")) {
                        doExtraAmazingGrace(p, target, 1);
                    } else { //Trick card
                        CardEffectStruct extraEffect;
                        extraCard->deleteLater();

                        extraEffect.card = xianshi_nullification;
                        extraEffect.from = p;
                        extraEffect.to = target;
                        extraEffect.multiple = effect.multiple;
                        extraCard->onEffect(extraEffect);
                    }
                }
            }
        }
    }

    return result;
}

bool Room::verifyNullificationResponse(ServerPlayer *player, const QVariant &response, void *)
{
    const Card *card = nullptr;
    if (player != nullptr && response.canConvert<JsonArray>()) {
        JsonArray responseArray = response.value<JsonArray>();
        if (JsonUtils::isString(responseArray[0]))
            card = Card::Parse(responseArray[0].toString());
    }
    return card != nullptr;
}

bool Room::askForNullification(const Card *trick, ServerPlayer *from, ServerPlayer *to, bool positive)
{
    _NullificationAiHelper aiHelper;
    aiHelper.m_from = from;
    aiHelper.m_to = to;
    aiHelper.m_trick = trick;
    return _askForNullification(trick, from, to, positive, aiHelper);
}

bool Room::_askForNullification(const Card *trick, ServerPlayer *from, ServerPlayer *to, bool positive, _NullificationAiHelper aiHelper)
{
    tryPause();

    _m_roomState.setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_RESPONSE_USE);
    QString trick_name = trick->objectName();
    QList<ServerPlayer *> validHumanPlayers;
    QList<ServerPlayer *> validAiPlayers;

    JsonArray arg;
    arg << trick_name;
    arg << (from != nullptr ? QVariant(from->objectName()) : QVariant());
    arg << (to != nullptr ? QVariant(to->objectName()) : QVariant());

    CardEffectStruct trickEffect;
    trickEffect.card = trick;
    trickEffect.from = from;
    trickEffect.to = to;
    QVariant data = QVariant::fromValue(trickEffect);

    setTag("NullifiationTarget", data);

    foreach (ServerPlayer *player, m_alivePlayers) {
        if (player->hasNullification()) {
            if (!thread->trigger(TrickCardCanceling, this, data)) {
                if (player->isOnline()) {
                    player->m_commandArgs = arg;
                    validHumanPlayers << player;
                } else
                    validAiPlayers << player;
            }
        }
    }

    ServerPlayer *repliedPlayer = nullptr;
    time_t timeOut = ServerInfo.getCommandTimeout(S_COMMAND_NULLIFICATION, S_SERVER_INSTANCE);
    if (!validHumanPlayers.isEmpty()) {
        if (trick->isKindOf("AOE") || trick->isKindOf("GlobalEffect")) {
            foreach (ServerPlayer *p, validHumanPlayers)
                doNotify(p, S_COMMAND_NULLIFICATION_ASKED, QVariant(trick->objectName()));
        }
        repliedPlayer = doBroadcastRaceRequest(validHumanPlayers, S_COMMAND_NULLIFICATION, timeOut, &Room::verifyNullificationResponse);
    }
    const Card *card = nullptr;
    if (repliedPlayer != nullptr) {
        JsonArray clientReply = repliedPlayer->getClientReply().value<JsonArray>();
        if (clientReply.size() > 0 && JsonUtils::isString(clientReply[0]))
            card = Card::Parse(clientReply[0].toString());
    }
    if (card == nullptr) {
        foreach (ServerPlayer *player, validAiPlayers) {
            AI *ai = player->getAI();
            if (ai == nullptr)
                continue;
            card = ai->askForNullification(aiHelper.m_trick, aiHelper.m_from, aiHelper.m_to, positive);
            if ((card != nullptr) && player->isCardLimited(card, Card::MethodUse))
                card = nullptr;
            if (card != nullptr) {
                thread->delay();
                repliedPlayer = player;
                break;
            }
        }
    }

    if (card == nullptr)
        return false;

    card = card->validateInResponse(repliedPlayer);
    if (card != nullptr && repliedPlayer->isCardLimited(card, Card::MethodUse))
        card = nullptr;
    if (card == nullptr)
        return _askForNullification(trick, from, to, positive, aiHelper);

    doAnimate(S_ANIMATE_NULLIFICATION, repliedPlayer->objectName(), to->objectName());
    useCard(CardUseStruct(card, repliedPlayer, QList<ServerPlayer *>()));
    //deal HegNullification
    bool isHegNullification = false;
    QString heg_nullification_selection;

    if (!repliedPlayer->hasFlag("nullifiationNul") && card->isKindOf("HegNullification") && !trick->isKindOf("Nullification") && trick->isNDTrick() && to->getRole() != "careerist"
        && to->hasShownOneGeneral()) {
        QVariantList qtargets = tag["targets" + trick->toString()].toList();
        QList<ServerPlayer *> targets;
        for (int i = 0; i < qtargets.size(); i++) {
            QVariant n = qtargets.at(i);
            targets.append(n.value<ServerPlayer *>());
        }
        QList<ServerPlayer *> targets_copy = targets;
        targets.removeOne(to);
        foreach (ServerPlayer *p, targets_copy) {
            if (targets_copy.indexOf(p) < targets_copy.indexOf(to))
                targets.removeOne(p);
        }
        if (!targets.isEmpty()) {
            foreach (ServerPlayer *p, targets) {
                if (p->getRole() != "careerist" && p->hasShownOneGeneral() && p->getRole() == to->getRole()) {
                    isHegNullification = true;
                    break;
                }
            }
        }
        if (isHegNullification) {
            //heg_nullification_selection = askForChoice(repliedPlayer, "heg_nullification%log:" + log, "single%to:" + to->objectName() +
            //    "+all%to:" + to->objectName() + "%log:" + Sanguosha->translate(to->getRole()), data);
            heg_nullification_selection = askForChoice(repliedPlayer, "heg_nullification", "single+all", data);
        }
        if (heg_nullification_selection.contains("all"))
            heg_nullification_selection = "all";
        else
            heg_nullification_selection = "single";
    }

    if (!isHegNullification) {
        LogMessage log;
        log.type = "#NullificationDetails";
        log.from = from;
        log.to << to;
        log.arg = trick_name;
        sendLog(log);
    } else {
        LogMessage log;
        log.type = "#HegNullificationDetails";
        log.from = from;
        log.to << to;
        log.arg = trick_name;
        sendLog(log);
        LogMessage log2;
        log2.type = "#HegNullificationSelection";
        log2.from = repliedPlayer;
        log2.arg = "hegnul_" + heg_nullification_selection;
        sendLog(log2);
    }
    setTag("NullificatonType", isHegNullification);
    thread->delay(500);

    //though weiya cancel the effect,but choicemade should be triggerable
    ChoiceMadeStruct s;
    s.player = repliedPlayer;
    s.type = ChoiceMadeStruct::Nullification;
    s.args << trick->getClassName() << to->objectName() << (positive ? "true" : "false");
    QVariant d = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, d);
    int pagoda = getTag("NullifyingTimes").toInt();
    setTag("NullifyingTimes", getTag("NullifyingTimes").toInt() + 1);

    bool result = true;
    CardEffectStruct effect;
    effect.card = card;
    effect.to = repliedPlayer;

    //for processing weiya
    if (repliedPlayer->hasFlag("nullifiationNul")) {
        result = false;
        setPlayerFlag(repliedPlayer, "-nullifiationNul");
    }
    //deal xianshi
    if (result && card->getSkillName() == "xianshi") {
        repliedPlayer->tag["xianshi_nullification_target"] = QVariant::fromValue(from);
        repliedPlayer->tag["xianshi_nullification"] = QVariant::fromValue(card);
        if (!positive)
            repliedPlayer->tag["xianshi_nullification_result"] = QVariant::fromValue(0);
        else
            repliedPlayer->tag["xianshi_nullification_result"] = QVariant::fromValue(1);
    }

    if (card->isCancelable(effect)) {
        if (result) {
            result = !_askForNullification(card, repliedPlayer, to, !positive, aiHelper);
        } else {
            result = _askForNullification(trick, from, to, positive, aiHelper);
        }
    }
    if (pagoda == 0 && result && EquipSkill::equipAvailable(repliedPlayer, EquipCard::TreasureLocation, "Pagoda")) {
        bool isLastTarget = true;
        foreach (const QString &flag, trick->getFlags()) {
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
            trick->setFlags("PagodaNullifiation");
    }

    removeTag("NullificatonType");

    if (isHegNullification && heg_nullification_selection == "all" && result) {
        setTag("HegNullificationValid", true);
    }

    return result;
}

int Room::askForCardChosen(ServerPlayer *player, ServerPlayer *who, const QString &flags, const QString &reason, bool handcard_visible, Card::HandlingMethod method,
                           const QList<int> &disabled_ids)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_CARD);

    if (player->hasSkill("duxin")) {
        handcard_visible = true;
        if (flags.contains("h"))
            notifySkillInvoked(player, "duxin");
        doAnimate(S_ANIMATE_INDICATE, player->objectName(), who->objectName());
    }

    if (player->hasSkill("duxin_hegemony") && flags.contains("h")) {
        if (player->hasShownSkill("duxin_hegemony") || player->askForSkillInvoke("duxin_hegemony", QVariant::fromValue(who))) {
            if (!player->hasShownSkill("duxin_hegemony"))
                player->showHiddenSkill("duxin_hegemony");
            handcard_visible = true;
            notifySkillInvoked(player, "duxin_hegemony");
            doAnimate(S_ANIMATE_INDICATE, player->objectName(), who->objectName());
        }
    }

    QList<int> shownHandcards = who->getShownHandcards();
    QList<int> unknownHandcards = who->handCards();
    foreach (int id, shownHandcards)
        unknownHandcards.removeOne(id);

    //re-check disable id
    QList<int> checked_disabled_ids;
    checked_disabled_ids << disabled_ids;
    bool EnableEmptyCard = false; //setting UI EmptyCard Enable
    foreach (const Card *c, who->getHandcards()) {
        if (!checked_disabled_ids.contains(c->getId())) {
            if (!flags.contains("h") && !who->isShownHandcard(c->getId()))
                checked_disabled_ids << c->getId();
            else if (!flags.contains("s") && who->isShownHandcard(c->getId()))
                checked_disabled_ids << c->getId();
        }
        if (!EnableEmptyCard && !who->isShownHandcard(c->getId()) && !checked_disabled_ids.contains(c->getId()))
            EnableEmptyCard = true;
    }

    //At first,collect selectable cards, and selectable knowncards
    QList<const Card *> cards = who->getCards(flags);
    QList<const Card *> knownCards = who->getCards(flags);
    foreach (const Card *card, cards) {
        if ((method == Card::MethodDiscard && !player->canDiscard(who, card->getEffectiveId(), reason)) || checked_disabled_ids.contains(card->getEffectiveId())) {
            cards.removeOne(card);
            knownCards.removeOne(card);
        } else if (unknownHandcards.contains(card->getEffectiveId()))
            knownCards.removeOne(card);
    }
    Q_ASSERT(!cards.isEmpty());

    if (handcard_visible && !who->isKongcheng()) {
        QList<int> handcards = who->handCards();
        JsonArray arg;
        arg << who->objectName();
        arg << JsonUtils::toJsonArray(handcards);
        doNotify(player, S_COMMAND_SET_KNOWN_CARDS, arg);
    }

    //secondly, if can not choose visible(known) cards, then randomly choose a card.
    int card_id = Card::S_UNKNOWN_CARD_ID;

    AI *ai = player->getAI();
    if (ai != nullptr) {
        thread->delay();
        card_id = ai->askForCardChosen(who, flags, reason, method);
    } else {
        JsonArray arg;
        arg << who->objectName();
        arg << flags;
        arg << reason;
        arg << handcard_visible;
        arg << (int)method;
        arg << JsonUtils::toJsonArray(checked_disabled_ids);
        arg << EnableEmptyCard;

        bool success = doRequest(player, S_COMMAND_CHOOSE_CARD, arg, true);
        //@todo: check if the card returned is valid
        const QVariant &clientReply = player->getClientReply();
        if (!success || !JsonUtils::isNumber(clientReply)) {
            // randomly choose a card
            card_id = cards.at(qrand() % cards.length())->getId();
        } else
            card_id = clientReply.toInt();

        if (card_id == Card::S_UNKNOWN_CARD_ID) {
            foreach (int id, checked_disabled_ids)
                unknownHandcards.removeOne(id);
            if (!unknownHandcards.isEmpty())
                card_id = unknownHandcards.at(qrand() % unknownHandcards.length());
        }
    }

    if (!cards.contains(Sanguosha->getCard(card_id)))
        card_id = cards.at(qrand() % cards.length())->getId();

    Q_ASSERT(card_id != Card::S_UNKNOWN_CARD_ID);

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::CardChosen;
    s.args << reason << QString::number(card_id) << player->objectName() << who->objectName();
    QVariant d = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, d);
    return card_id;
}

const Card *Room::askForCard(ServerPlayer *player, const QString &pattern, const QString &prompt, const QVariant &data, const QString &skill_name, int notice_index)
{
    return askForCard(player, pattern, prompt, data, Card::MethodDiscard, nullptr, false, skill_name, false, notice_index);
}

const Card *Room::askForCard(ServerPlayer *player, const QString &pattern, const QString &prompt, const QVariant &data, Card::HandlingMethod method, ServerPlayer *to,
                             bool isRetrial, const QString &skill_name, bool isProvision, int notice_index)
{
    Q_ASSERT(pattern != "slash" || method != Card::MethodUse); // use askForUseSlashTo instead
    tryPause();
    notifyMoveFocus(player, S_COMMAND_RESPONSE_CARD);

    _m_roomState.setCurrentCardUsePattern(pattern);

    const Card *card = nullptr;
    CardAskedStruct s;
    s.player = player;
    s.pattern = pattern;
    s.prompt = prompt;
    s.method = method;
    QVariant asked_data = QVariant::fromValue(s);
    if ((method == Card::MethodUse || method == Card::MethodResponse) && !isRetrial && !player->hasFlag("continuing"))
        thread->trigger(CardAsked, this, asked_data);

    //case 1. for the player died since a counter attack from juwang target
    //case 2. figure out dizhen and eghitDiagram and tianren
    //we also need clear the provider info
    if (!player->isAlive() && !isProvision) {
        provided = nullptr;
        has_provided = false;
        provider = nullptr;
        return nullptr;
    }
    CardUseStruct::CardUseReason reason = CardUseStruct::CARD_USE_REASON_UNKNOWN;
    if (method == Card::MethodResponse)
        reason = CardUseStruct::CARD_USE_REASON_RESPONSE;
    else if (method == Card::MethodUse)
        reason = CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    _m_roomState.setCurrentCardUseReason(reason);

    if (player->hasFlag("continuing"))
        setPlayerFlag(player, "-continuing");

    ServerPlayer *theProvider = nullptr;
    if (has_provided || !player->isAlive()) {
        card = provided;
        theProvider = provider;

        if (player->isCardLimited(card, method))
            card = nullptr;
        provided = nullptr;
        has_provided = false;
        provider = nullptr;
    } else {
        AI *ai = player->getAI();
        if (ai != nullptr) {
            card = ai->askForCard(pattern, prompt, data);
            if ((card != nullptr) && card->isKindOf("DummyCard") && card->subcardsLength() == 1)
                card = Sanguosha->getCard(card->getEffectiveId());
            if ((card != nullptr) && player->isCardLimited(card, method))
                card = nullptr;
            if (card != nullptr)
                thread->delay();
        } else {
            JsonArray arg;
            arg << pattern;
            arg << prompt;
            arg << int(method);
            arg << notice_index;
            arg << QString(skill_name);

            bool success = doRequest(player, S_COMMAND_RESPONSE_CARD, arg, true);
            JsonArray clientReply = player->getClientReply().value<JsonArray>();
            if (success && !clientReply.isEmpty())
                card = Card::Parse(clientReply[0].toString());
        }
    }

    if (card == nullptr) {
        ChoiceMadeStruct s;
        s.player = player;
        s.type = ChoiceMadeStruct::CardResponded;
        s.args << pattern << prompt << "_nil_";
        s.m_extraData = data;
        QVariant d = QVariant::fromValue(s);
        thread->trigger(ChoiceMade, this, d);
        return nullptr;
    }

    card = card->validateInResponse(player);
    if (card != nullptr && player->isCardLimited(card, method))
        card = nullptr;
    const Card *result = nullptr;
    //card log
    if (card != nullptr) {
        if (!card->isVirtualCard()) {
            WrappedCard *wrapped = Sanguosha->getWrappedCard(card->getEffectiveId());
            if (wrapped->isModified())
                broadcastUpdateCard(getPlayers(), card->getEffectiveId(), wrapped);
            else
                broadcastResetCard(getPlayers(), card->getEffectiveId());
        }

        if ((method == Card::MethodUse || method == Card::MethodResponse) && !isRetrial) {
            LogMessage log;
            log.card_str = card->toString();
            log.from = player;
            log.type = QString("#%1").arg(card->getClassName());
            if (method == Card::MethodResponse)
                log.type += "_Resp";
            sendLog(log);
            player->broadcastSkillInvoke(card);
        } else if (method == Card::MethodDiscard) {
            LogMessage log;
            log.type = skill_name.isEmpty() ? "$DiscardCard" : "$DiscardCardWithSkill";
            log.from = player;
            QList<int> to_discard;
            if (card->isVirtualCard())
                to_discard.append(card->getSubcards());
            else
                to_discard << card->getEffectiveId();
            log.card_str = IntList2StringList(to_discard).join("+");
            if (!skill_name.isEmpty())
                log.arg = skill_name;
            sendLog(log);
            if (!skill_name.isEmpty())
                notifySkillInvoked(player, skill_name);
        }
    }
    //trigger event
    if (card != nullptr) {
        bool isHandcard = true;
        bool isShowncard = false;
        QList<int> ids;
        if (!card->isVirtualCard())
            ids << card->getEffectiveId();
        else
            ids = card->getSubcards();
        if (!ids.isEmpty()) {
            foreach (int id, ids) {
                if (getCardOwner(id) != player || getCardPlace(id) != Player::PlaceHand) {
                    isHandcard = false;
                    break;
                }
            }
        } else {
            isHandcard = false;
        }

        if (!ids.isEmpty()) {
            foreach (int id, ids) {
                ServerPlayer *shownCardOwner = getCardOwner(id);
                if ((shownCardOwner != nullptr) && shownCardOwner->isShownHandcard(id)) {
                    isShowncard = true;
                    break;
                }
            }
        }

        ChoiceMadeStruct s;
        s.player = player;
        s.type = ChoiceMadeStruct::CardResponded;
        s.args << pattern << prompt << QString("_%1_").arg(card->toString());
        s.m_extraData = data;
        QVariant d = QVariant::fromValue(s);
        thread->trigger(ChoiceMade, this, d);
        //show hidden general
        player->showHiddenSkill(skill_name);
        player->showHiddenSkill(card->getSkillName());
        const Skill *equipSkill = Sanguosha->getSkill(skill_name);
        if ((equipSkill != nullptr) && equipSkill->inherits("WeaponSkill")) {
            const ViewHasSkill *v = Sanguosha->ViewHas(player, skill_name, "weapon", true);
            if (v != nullptr)
                player->showHiddenSkill(v->objectName());
        }

        //move1
        if (method == Card::MethodUse) {
            CardMoveReason reason(CardMoveReason::S_REASON_LETUSE, player->objectName(), QString(), card->getSkillName(), QString());

            reason.m_extraData = QVariant::fromValue(card);
            if (theProvider != nullptr)
                moveCardTo(card, theProvider, nullptr, Player::PlaceTable, reason, true);
            else if (!card->isVirtualCard() && (getCardOwner(card->getEffectiveId()) != nullptr) && getCardOwner(card->getEffectiveId()) != player) //only for Skill Xinhua
                moveCardTo(card, getCardOwner(card->getEffectiveId()), nullptr, Player::PlaceTable, reason, true);
            else
                moveCardTo(card, player, nullptr, Player::PlaceTable, reason, true);
        } else if (method == Card::MethodDiscard) {
            CardMoveReason reason(CardMoveReason::S_REASON_THROW, player->objectName());
            moveCardTo(card, player, nullptr, Player::DiscardPile, reason, pattern != "." && pattern != "..");
        } else if (method != Card::MethodNone && !isRetrial) {
            CardMoveReason reason(CardMoveReason::S_REASON_RESPONSE, player->objectName());
            reason.m_skillName = card->getSkillName();
            reason.m_extraData = QVariant::fromValue(card);
            if (theProvider != nullptr)
                moveCardTo(card, theProvider, nullptr, isProvision ? Player::PlaceTable : Player::DiscardPile, reason, method != Card::MethodPindian);
            else if (!card->isVirtualCard() && (getCardOwner(card->getEffectiveId()) != nullptr) && getCardOwner(card->getEffectiveId()) != player) //only for Skill Xinhua
                moveCardTo(card, getCardOwner(card->getEffectiveId()), nullptr, isProvision ? Player::PlaceTable : Player::DiscardPile, reason, method != Card::MethodPindian);
            else
                moveCardTo(card, player, nullptr, isProvision ? Player::PlaceTable : Player::DiscardPile, reason, method != Card::MethodPindian);
        }
        //move2
        if ((method == Card::MethodUse || method == Card::MethodResponse) && !isRetrial) {
            if (!card->getSkillName().isNull() && card->getSkillName(true) == card->getSkillName(false) && player->hasSkill(card->getSkillName()))
                notifySkillInvoked(player, card->getSkillName());
            CardResponseStruct resp(card, to, method == Card::MethodUse, isRetrial, isProvision, player);
            resp.m_isHandcard = isHandcard;
            resp.m_isShowncard = isShowncard;
            QVariant data = QVariant::fromValue(resp);
            thread->trigger(CardResponded, this, data);
            resp = data.value<CardResponseStruct>();
            if (method == Card::MethodUse) {
                if (getCardPlace(card->getEffectiveId()) == Player::PlaceTable) {
                    CardMoveReason reason(CardMoveReason::S_REASON_LETUSE, player->objectName(), QString(), card->getSkillName(), QString());
                    reason.m_extraData = QVariant::fromValue(card);
                    if (theProvider != nullptr)
                        moveCardTo(card, theProvider, nullptr, Player::DiscardPile, reason, true);
                    else
                        moveCardTo(card, player, nullptr, Player::DiscardPile, reason, true);
                }
                CardUseStruct card_use;
                card_use.card = card;
                card_use.from = player;
                if (to != nullptr)
                    card_use.to << to;
                QVariant data2 = QVariant::fromValue(card_use);
                thread->trigger(CardFinished, this, data2);
            }
            if (resp.m_isNullified)
                return nullptr;
        }
        result = card;
    } else {
        setPlayerFlag(player, "continuing");
        result = askForCard(player, pattern, prompt, data, method, to, isRetrial);
    }
    return result;
}

const Card *Room::askForUseCard(ServerPlayer *player, const QString &pattern, const QString &prompt, int notice_index, Card::HandlingMethod method, bool addHistory,
                                const QString &skill_name)
{
    Q_ASSERT(method != Card::MethodResponse);

    tryPause();
    notifyMoveFocus(player, S_COMMAND_RESPONSE_CARD);

    _m_roomState.setCurrentCardUsePattern(pattern);
    _m_roomState.setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_RESPONSE_USE);
    CardUseStruct card_use;

    bool isCardUsed = false;
    AI *ai = player->getAI();
    if (ai != nullptr) {
        QString answer = ai->askForUseCard(pattern, prompt, method);
        if (answer != ".") {
            isCardUsed = true;
            card_use.from = player;
            card_use.parse(answer, this);
            thread->delay();
        }
    } else {
        JsonArray ask_str;
        ask_str << pattern;
        ask_str << prompt;
        ask_str << int(method);
        ask_str << notice_index;
        ask_str << QString(skill_name);

        bool success = doRequest(player, S_COMMAND_RESPONSE_CARD, ask_str, true);
        if (success) {
            const QVariant &clientReply = player->getClientReply();
            isCardUsed = !clientReply.isNull();
            if (isCardUsed && card_use.tryParse(clientReply, this))
                card_use.from = player;
        }
    }
    card_use.m_reason = CardUseStruct::CARD_USE_REASON_RESPONSE_USE;

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::CardUsed;
    s.args << pattern << prompt;
    if (isCardUsed && card_use.isValid(pattern)) {
        s.args << card_use.toString();
        QVariant decisionData = QVariant::fromValue(s);
        thread->trigger(ChoiceMade, this, decisionData);
        //show hidden general
        player->showHiddenSkill(skill_name);
        player->showHiddenSkill(card_use.card->getSkillName());
        if (!useCard(card_use, addHistory))
            return askForUseCard(player, pattern, prompt, notice_index, method, addHistory);

        return card_use.card;
    } else {
        s.args << "nil";
        QVariant decisionData = QVariant::fromValue(s);
        thread->trigger(ChoiceMade, this, decisionData);
    }

    return nullptr;
}

const Card *Room::askForUseSlashTo(ServerPlayer *slasher, QList<ServerPlayer *> victims, const QString &prompt, bool distance_limit, bool disable_extra, bool addHistory)
{
    Q_ASSERT(!victims.isEmpty());

    // The realization of this function in the Slash::onUse and Slash::targetFilter.
    setPlayerFlag(slasher, "slashTargetFix");
    if (!distance_limit)
        setPlayerFlag(slasher, "slashNoDistanceLimit");
    if (disable_extra)
        setPlayerFlag(slasher, "slashDisableExtraTarget");
    if (victims.length() == 1)
        setPlayerFlag(slasher, "slashTargetFixToOne");
    foreach (ServerPlayer *victim, victims)
        setPlayerFlag(victim, "SlashAssignee");

    const Card *slash = askForUseCard(slasher, "slash", prompt, -1, Card::MethodUse, addHistory);
    if (slash == nullptr) {
        setPlayerFlag(slasher, "-slashTargetFix");
        setPlayerFlag(slasher, "-slashTargetFixToOne");
        foreach (ServerPlayer *victim, victims)
            setPlayerFlag(victim, "-SlashAssignee");
        if (slasher->hasFlag("slashNoDistanceLimit"))
            setPlayerFlag(slasher, "-slashNoDistanceLimit");
        if (slasher->hasFlag("slashDisableExtraTarget"))
            setPlayerFlag(slasher, "-slashDisableExtraTarget");
    }

    return slash;
}

const Card *Room::askForUseSlashTo(ServerPlayer *slasher, ServerPlayer *victim, const QString &prompt, bool distance_limit, bool disable_extra, bool addHistory)
{
    Q_ASSERT(victim != nullptr);
    QList<ServerPlayer *> victims;
    victims << victim;
    return askForUseSlashTo(slasher, victims, prompt, distance_limit, disable_extra, addHistory);
}

int Room::askForAG(ServerPlayer *player, const QList<int> &card_ids, bool refusable, const QString &reason)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_AMAZING_GRACE);
    Q_ASSERT(card_ids.length() > 0);

    int card_id = -1;
    if (card_ids.length() == 1 && !refusable)
        card_id = card_ids.first();
    else {
        AI *ai = player->getAI();
        if (ai != nullptr) {
            thread->delay();
            card_id = ai->askForAG(card_ids, refusable, reason);
        } else {
            JsonArray arg;
            arg << refusable << reason;

            bool success = doRequest(player, S_COMMAND_AMAZING_GRACE, arg, true);
            const QVariant &clientReply = player->getClientReply();
            if (success && JsonUtils::isNumber(clientReply))
                card_id = clientReply.toInt();
        }

        if (!card_ids.contains(card_id))
            card_id = refusable ? -1 : card_ids.first();
    }

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::AGChosen;
    s.args << reason << QString::number(card_id);
    QVariant decisionData = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, decisionData);

    return card_id;
}

void Room::doExtraAmazingGrace(ServerPlayer *from, ServerPlayer *target, int times)
{ //couple xianshi

    int count = getAllPlayers().length();
    QList<int> card_ids = getNCards(count);
    CardsMoveStruct move(card_ids, nullptr, Player::PlaceTable, CardMoveReason(CardMoveReason::S_REASON_TURNOVER, from->objectName(), "xianshi", QString()));
    moveCardsAtomic(move, true);
    fillAG(card_ids);

    for (int i = 0; i < times; i += 1) {
        int card_id = askForAG(target, card_ids, false, "xianshi");
        card_ids.removeOne(card_id);
        takeAG(target, card_id);
        if (card_ids.isEmpty())
            break;
    }
    clearAG();

    //throw other cards

    if (!card_ids.isEmpty()) {
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, from->objectName(), "xianshi", QString());
        DummyCard dummy(card_ids);
        throwCard(&dummy, reason, nullptr);
    }
}

const Card *Room::askForCardShow(ServerPlayer *player, ServerPlayer *requester, const QString &reason)
{
    Q_ASSERT(!player->isKongcheng());

    tryPause();
    notifyMoveFocus(player, S_COMMAND_SHOW_CARD);
    const Card *card = nullptr;

    AI *ai = player->getAI();
    if (ai != nullptr)
        card = ai->askForCardShow(requester, reason);
    else {
        if (player->getHandcardNum() == 1)
            card = player->getHandcards().constFirst();
        else {
            bool success = doRequest(player, S_COMMAND_SHOW_CARD, requester->getGeneralName(), true);
            JsonArray clientReply = player->getClientReply().value<JsonArray>();
            if (success && clientReply.size() > 0 && JsonUtils::isString(clientReply[0]))
                card = Card::Parse(clientReply[0].toString());
            if (card == nullptr)
                card = player->getRandomHandCard();
        }
    }

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::CardShow;
    s.args << reason << QString("_%1_").arg(card->toString());
    QVariant decisionData = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, decisionData);
    return card;
}

void Room::askForSinglePeach(ServerPlayer *player, ServerPlayer *dying, CardUseStruct &use)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_ASK_PEACH);
    _m_roomState.setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_RESPONSE_USE);

    use = CardUseStruct();
    use.from = player;
    int threshold = dying->dyingThreshold();

    AI *ai = player->getAI();
    if (ai != nullptr) {
        use.card = ai->askForSinglePeach(dying);
        if (use.card != nullptr)
            use.to << dying;
    } else {
        int peaches = threshold - dying->getHp();
        if (dying->hasSkill("banling")) {
            peaches = 0;
            if (dying->getRenHp() <= 0)
                peaches = peaches + threshold - dying->getRenHp();
            if (dying->getLingHp() <= 0)
                peaches = peaches + threshold - dying->getLingHp();
        }
        JsonArray arg;
        arg << dying->objectName();
        arg << peaches;
        bool success = doRequest(player, S_COMMAND_ASK_PEACH, arg, true);
        JsonArray clientReply = player->getClientReply().value<JsonArray>();
        if (!success || clientReply.isEmpty() || !JsonUtils::isString(clientReply[0]))
            return;
        if (!use.tryParse(clientReply, this)) {
            use.card = nullptr;
            return;
        }

        if (use.card != nullptr && use.to.isEmpty())
            use.to << dying;
    }

    if (use.card != nullptr && player->isCardLimited(use.card, use.card->getHandlingMethod()))
        use.card = nullptr;
    if (use.card != nullptr) {
        use.card = use.card->validateInResponse(player);
        Card::HandlingMethod method = Card::MethodUse;
        if (use.card != nullptr && use.card->getTypeId() == Card::TypeSkill) //keep TypeSkill after validateInResponse
            method = use.card->getHandlingMethod();

        if (use.card != nullptr && player->isCardLimited(use.card, method))
            use.card = nullptr;
    } else
        return;

    if (use.card != nullptr) {
        ChoiceMadeStruct s;
        s.player = player;
        s.type = ChoiceMadeStruct::Peach;
        s.args << dying->objectName() << QString::number(threshold - dying->getHp()) << use.card->toString();
        QVariant decisionData = QVariant::fromValue(s);
        thread->trigger(ChoiceMade, this, decisionData);
    } else
        askForSinglePeach(player, dying, use);
}

QSharedPointer<SkillInvokeDetail> Room::askForTriggerOrder(ServerPlayer *player, const QList<QSharedPointer<SkillInvokeDetail>> &sameTiming, bool cancelable, const QVariant &data)
{
    tryPause();

    Q_ASSERT(!sameTiming.isEmpty());

    QSharedPointer<SkillInvokeDetail> answer;

    if (sameTiming.length() == 1)
        answer = sameTiming.first();
    else {
        notifyMoveFocus(player, S_COMMAND_TRIGGER_ORDER);

        AI *ai = player->getAI();

        QString reply;

        if (ai != nullptr) {
            //AI system also need  the same reply with human.
            QStringList skills;
            QString currentSkillName = sameTiming.first()->skill->objectName();
            int preferIndex = 0;
            foreach (const QSharedPointer<SkillInvokeDetail> &ptr, sameTiming) {
                QString skill = ptr->skill->objectName();
                // owner or invoker could be NULL
                QString ownerName = (ptr->owner) != nullptr ? ptr->owner->objectName() : QString();
                QString invokerName = (ptr->invoker) != nullptr ? ptr->invoker->objectName() : QString();
                skill.append(":").append(ownerName).append(":").append(invokerName);
                if (ptr->preferredTarget != nullptr) {
                    skill.append(":").append(ptr->preferredTarget->objectName());
                    if (currentSkillName == ptr->skill->objectName()) {
                        preferIndex++;
                        skill.append(":").append(QString::number(preferIndex));
                    } else {
                        currentSkillName = ptr->skill->objectName();
                        preferIndex = 1;
                        skill.append(":").append(QString::number(preferIndex));
                    }
                }
                skills << skill;
            }
            if (cancelable)
                skills << "cancel";
#pragma message WARN("todo_Fs: use a new method to deal with askfortriggerorder")
            reply = ai->askForChoice("askForTriggerOrder", skills.join("+"), data);
        } else {
            JsonArray arr;
            foreach (const QSharedPointer<SkillInvokeDetail> &ptr, sameTiming)
                arr << ptr->toVariant();
            JsonArray arr2;
            arr2 << QVariant(arr) << cancelable;

            bool success = doRequest(player, S_COMMAND_TRIGGER_ORDER, arr2, true);
            const QVariant &clientReply = player->getClientReply();
            if (success && JsonUtils::isString(clientReply))
                reply = clientReply.toString();
        }

        if (reply != "cancel") {
            QStringList replyList = reply.split(":");
            foreach (const QSharedPointer<SkillInvokeDetail> &ptr, sameTiming) {
                if (ptr->skill->objectName() == replyList.first() && (ptr->owner == nullptr || ptr->owner->objectName() == replyList.value(1))
                    && ptr->invoker->objectName() == replyList.value(2)) {
                    if (ptr->preferredTarget != nullptr) {
                        if (replyList.length() < 4)
                            continue;
                        if (ptr->preferredTarget->objectName() != replyList.value(3))
                            continue;
                    }
                    answer = ptr;
                    break;
                }
            }
        }
        if (answer.isNull() && !cancelable)
            answer = sameTiming.value(qrand() % sameTiming.length());
    }

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::TriggerOrder;
    if (!answer.isNull())
        s.args = answer->toList();
    QVariant d = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, d);
    return answer;
}

void Room::addPlayerHistory(ServerPlayer *player, const QString &key, int times)
{
    if (player != nullptr) {
        if (key == ".")
            player->clearHistory();
        else
            player->addHistory(key, times);
    }

    JsonArray arg;
    arg << key;
    arg << times;

    if (player != nullptr)
        doNotify(player, S_COMMAND_ADD_HISTORY, arg);
    else
        doBroadcastNotify(S_COMMAND_ADD_HISTORY, arg);
}

void Room::setPlayerFlag(ServerPlayer *player, const QString &flag)
{
    if (flag.startsWith("-")) {
        QString set_flag = flag.mid(1);
        if (!player->hasFlag(set_flag))
            return;
    }
    player->setFlags(flag);
    broadcastProperty(player, "flags", flag);
}

void Room::setPlayerProperty(ServerPlayer *player, const char *property_name, const QVariant &value)
{
#if 0 // def QT_DEBUG
    // following code was OK when Room inherits QThread but is currently not
    if (currentThread() != player->thread()) {
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
            thread->trigger(HpChanged, this, v);
        }
    }
    if (strcmp(property_name, "maxhp") == 0) {
        if (game_started) {
            QVariant v = QVariant::fromValue(player);
            thread->trigger(MaxHpChanged, this, v);
        }
    }
    if (strcmp(property_name, "chained") == 0) {
        if (player->isAlive())
            setEmotion(player, "chain");
        if (game_started) {
            QVariant v = QVariant::fromValue(player);
            thread->trigger(ChainStateChanged, this, v);
        }
    }
    if (strcmp(property_name, "removed") == 0) {
        if (game_started) {
            QVariant pv = QVariant::fromValue(player);
            thread->trigger(RemoveStateChanged, this, pv);
        }
    }
    if (strcmp(property_name, "role_shown") == 0) {
        if (game_started) {
            QVariant pv = QVariant::fromValue(player);
            thread->trigger(RoleShownChanged, this, pv);
        }
        player->setMark("AI_RoleShown", value.toBool() ? 1 : 0);
        roleStatusCommand(player);
        if (value.toBool())
            player->setMark("AI_RolePredicted", 1);
    }
    if (strcmp(property_name, "kingdom") == 0) {
        if (game_started) {
            QVariant v = QVariant::fromValue(player);
            thread->trigger(KingdomChanged, this, v);
        }
    }
}

void Room::slotSetProperty(ServerPlayer *player, const char *property_name, const QVariant &value)
{
    player->setProperty(property_name, value);
    playerPropertySet = true;
}

void Room::setPlayerMark(ServerPlayer *player, const QString &mark, int value)
{
    player->setMark(mark, value);

    JsonArray arg;
    arg << player->objectName();
    arg << mark;
    arg << value;
    doBroadcastNotify(S_COMMAND_SET_MARK, arg);
}

void Room::addPlayerMark(ServerPlayer *player, const QString &mark, int add_num)
{
    int value = player->getMark(mark);
    value += add_num;
    setPlayerMark(player, mark, value);
}

void Room::removePlayerMark(ServerPlayer *player, const QString &mark, int remove_num)
{
    int value = player->getMark(mark);
    if (value == 0)
        return;
    value -= remove_num;
    value = qMax(0, value);
    setPlayerMark(player, mark, value);
}

void Room::setPlayerCardLimitation(ServerPlayer *player, const QString &limit_list, const QString &pattern, const QString &reason, bool single_turn)
{
    player->setCardLimitation(limit_list, pattern, reason, single_turn);

    JsonArray arg;
    arg << true;
    arg << limit_list;
    arg << pattern;
    arg << single_turn;
    arg << player->objectName();
    arg << reason;
    arg << false;
    doBroadcastNotify(S_COMMAND_CARD_LIMITATION, arg);
}

void Room::removePlayerCardLimitation(ServerPlayer *player, const QString &limit_list, const QString &pattern, const QString &reason, bool clearReason)
{
    player->removeCardLimitation(limit_list, pattern, reason, clearReason);

    JsonArray arg;
    arg << false;
    arg << limit_list;
    arg << pattern;
    arg << false;
    arg << player->objectName();
    arg << reason;
    arg << clearReason;
    doBroadcastNotify(S_COMMAND_CARD_LIMITATION, arg);
}

void Room::clearPlayerCardLimitation(ServerPlayer *player, bool single_turn)
{
    player->clearCardLimitation(single_turn);

    JsonArray arg;
    arg << true;
    arg << QVariant();
    arg << QVariant();
    arg << single_turn;
    arg << player->objectName();
    arg << QString();
    arg << true;
    doBroadcastNotify(S_COMMAND_CARD_LIMITATION, arg);
}

void Room::setPlayerDisableShow(ServerPlayer *player, const QString &flags, const QString &reason)
{
    player->setDisableShow(flags, reason);

    JsonArray arg;
    arg << player->objectName();
    arg << true;
    arg << flags;
    arg << reason;
    doBroadcastNotify(S_COMMAND_DISABLE_SHOW, arg);
}

void Room::removePlayerDisableShow(ServerPlayer *player, const QString &reason)
{
    player->removeDisableShow(reason);

    JsonArray arg;
    arg << player->objectName();
    arg << false;
    arg << QVariant();
    arg << reason;
    doBroadcastNotify(S_COMMAND_DISABLE_SHOW, arg);
}

void Room::setCardFlag(const Card *card, const QString &flag, ServerPlayer *who)
{
    if (flag.isEmpty())
        return;

    card->setFlags(flag);

    if (!card->isVirtualCard())
        setCardFlag(card->getEffectiveId(), flag, who);
}

void Room::setCardFlag(int card_id, const QString &flag, ServerPlayer *who)
{
    if (flag.isEmpty())
        return;

    Q_ASSERT(Sanguosha->getCard(card_id) != nullptr);
    Sanguosha->getCard(card_id)->setFlags(flag);

    JsonArray arg;
    arg << card_id;
    arg << flag;
    if (who != nullptr)
        doNotify(who, S_COMMAND_CARD_FLAG, arg);
    else
        doBroadcastNotify(S_COMMAND_CARD_FLAG, arg);
}

void Room::clearCardFlag(const Card *card, ServerPlayer *who)
{
    card->clearFlags();

    if (!card->isVirtualCard())
        clearCardFlag(card->getEffectiveId(), who);
}

void Room::clearCardFlag(int card_id, ServerPlayer *who)
{
    Q_ASSERT(Sanguosha->getCard(card_id) != nullptr);
    Sanguosha->getCard(card_id)->clearFlags();

    JsonArray arg;
    arg << card_id;
    arg << ".";
    if (who != nullptr)
        doNotify(who, S_COMMAND_CARD_FLAG, arg);
    else
        doBroadcastNotify(S_COMMAND_CARD_FLAG, arg);
}

ServerPlayer *Room::addSocket(ClientSocket *socket)
{
    ServerPlayer *player = new ServerPlayer(this);
    player->setSocket(socket);
    m_players << player;

    connect(player, SIGNAL(disconnected()), this, SLOT(reportDisconnection()));
    connect(player, SIGNAL(request_got(QString)), this, SLOT(processClientPacket(QString)));

    return player;
}

bool Room::isFull() const
{
    return m_players.length() == player_count;
}

bool Room::isFinished() const
{
    return game_finished;
}

bool Room::canPause(ServerPlayer *player) const
{
    if (!isFull())
        return false;
    if ((player == nullptr) || !player->isOwner())
        return false;
    foreach (ServerPlayer *p, m_players) {
        if (!p->isAlive() || p->isOwner())
            continue;
        if (p->getState() != "robot")
            return false;
    }
    return true;
}

void Room::tryPause()
{
    if (!canPause(getOwner()))
        return;
    QMutexLocker locker(&m_mutex);
    while (game_paused)
        m_waitCond.wait(locker.mutex());
}

int Room::getLack() const
{
    return player_count - m_players.length();
}

QString Room::getMode() const
{
    return mode;
}

void Room::broadcast(const QString &message, ServerPlayer *except)
{
    foreach (ServerPlayer *player, m_players) {
        if (player != except)
            player->unicast(message);
    }
}

void Room::swapPile()
{
    if (m_discardPile->isEmpty()) {
        // the standoff
        gameOver(".");
    }

    int times = tag.value("SwapPile", 0).toInt();
    tag.insert("SwapPile", ++times);

    int limit = Config.value("PileSwappingLimitation", 5).toInt() + 1;
    if (mode == "04_1v3")
        limit = qMin(limit, Config.BanPackages.contains("maneuvering") ? 3 : 2);
    if (limit > 0 && times == limit)
        gameOver(".");

    qSwap(m_drawPile, m_discardPile);

    doBroadcastNotify(S_COMMAND_RESET_PILE, QVariant());
    doBroadcastNotify(S_COMMAND_UPDATE_PILE, QVariant(m_drawPile->length()));

    qShuffle(*m_drawPile);
    foreach (int card_id, *m_drawPile)
        setCardMapping(card_id, nullptr, Player::DrawPile);

    QVariant qtimes;
    qtimes.setValue(times);
    thread->trigger(DrawPileSwaped, this, qtimes);
}

QList<int> Room::getDiscardPile()
{
    return *m_discardPile;
}

ServerPlayer *Room::findPlayer(const QString &general_name, bool include_dead) const
{
    const QList<ServerPlayer *> &list = include_dead ? m_players : m_alivePlayers;

    if (general_name.contains("+")) {
        QStringList names = general_name.split("+");
        foreach (ServerPlayer *player, list) {
            if (names.contains(player->getGeneralName()))
                return player;
        }
        return nullptr;
    }

    foreach (ServerPlayer *player, list) {
        if (player->getGeneralName() == general_name || player->objectName() == general_name)
            return player;
    }

    return nullptr;
}

QList<ServerPlayer *> Room::findPlayersBySkillName(const QString &skill_name, bool include_hidden) const
{
    QList<ServerPlayer *> list;
    foreach (ServerPlayer *player, getAllPlayers()) {
        if (player->hasSkill(skill_name, false, include_hidden))
            list << player;
    }
    return list;
}

ServerPlayer *Room::findPlayerBySkillName(const QString &skill_name) const
{
    foreach (ServerPlayer *player, getAllPlayers()) {
        if (player->hasSkill(skill_name))
            return player;
    }
    return nullptr;
}

ServerPlayer *Room::findPlayerByObjectName(const QString &name, bool) const
{
    foreach (ServerPlayer *player, getAllPlayers()) {
        if (player->objectName() == name)
            return player;
    }
    return nullptr;
}

void Room::installEquip(ServerPlayer *player, const QString &equip_name)
{
    if (player == nullptr)
        return;

    int card_id = getCardFromPile(equip_name);
    if (card_id == -1)
        return;

    moveCardTo(Sanguosha->getCard(card_id), player, Player::PlaceEquip, true);
}

void Room::resetAI(ServerPlayer *player)
{
    AI *smart_ai = player->getSmartAI();
    int index = -1;
    if (smart_ai != nullptr) {
        index = ais.indexOf(smart_ai);
        ais.removeOne(smart_ai);
        smart_ai->deleteLater();
    }
    AI *new_ai = cloneAI(player);
    player->setAI(new_ai);
    if (index == -1)
        ais.append(new_ai);
    else
        ais.insert(index, new_ai);
}

void Room::changeHero(ServerPlayer *player, const QString &new_general, bool full_state, bool invokeStart, bool isSecondaryHero, bool sendLog)
{
    JsonArray arg;
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
        player->setHp(player->getMaxHp());
    broadcastProperty(player, "hp");
    broadcastProperty(player, "maxhp");

    bool game_start = false;
    const General *gen = isSecondaryHero ? player->getGeneral2() : player->getGeneral();
    if (gen != nullptr) {
        foreach (const Skill *skill, gen->getSkillList()) {
            if (skill->inherits("TriggerSkill")) {
                const TriggerSkill *trigger = qobject_cast<const TriggerSkill *>(skill);
                thread->addTriggerSkill(trigger);
                if (invokeStart && trigger->getTriggerEvents().contains(GameStart))
                    game_start = true;
            }
            if (skill->getFrequency() == Skill::Limited && !skill->getLimitMark().isEmpty())
                setPlayerMark(player, skill->getLimitMark(), 1);
            SkillAcquireDetachStruct s;
            s.isAcquire = true;
            s.player = player;
            s.skill = skill;
            QVariant _skillobjectName = QVariant::fromValue(s);
            thread->trigger(EventAcquireSkill, this, _skillobjectName);
        }
    }
    if (invokeStart && game_start) {
        QVariant v = QVariant::fromValue(player);
        thread->trigger(GameStart, this, v);
    }

    resetAI(player);
}

lua_State *Room::getLuaState() const
{
    return L;
}

void Room::setFixedDistance(Player *from, const Player *to, int distance)
{
    from->setFixedDistance(to, distance);

    JsonArray arg;
    arg << from->objectName();
    arg << to->objectName();
    arg << distance;
    doBroadcastNotify(S_COMMAND_FIXED_DISTANCE, arg);
}

void Room::reverseFor3v3(const Card *card, ServerPlayer *player, QList<ServerPlayer *> &list)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_DIRECTION);

    bool isClockwise = false;
    if (player->isOnline()) {
        bool success = doRequest(player, S_COMMAND_CHOOSE_DIRECTION, QVariant(), true);
        QVariant clientReply = player->getClientReply();
        if (success && JsonUtils::isString(clientReply))
            isClockwise = (clientReply.toString() == "cw");
    } else {
        if (player->getAI() != nullptr) {
            QVariant data = QVariant::fromValue(card);
            isClockwise = (player->getAI()->askForChoice("3v3_direction", "cw+ccw", data) == "cw");
        } else
            isClockwise = true;
    }

    LogMessage log;
    log.type = "#TrickDirection";
    log.from = player;
    log.arg = isClockwise ? "cw" : "ccw";
    log.arg2 = card->objectName();
    sendLog(log);

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::ReverseFor3v3;
    s.args << card->toString() << (isClockwise ? "cw" : "ccw");
    QVariant d = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, d);

    if (isClockwise) {
        QList<ServerPlayer *> new_list;

        while (!list.isEmpty())
            new_list << list.takeLast();

        if (card->isKindOf("GlobalEffect")) {
            new_list.removeLast();
            new_list.prepend(player);
        }

        list = new_list;
    }
}

const ProhibitSkill *Room::isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others) const
{
    return Sanguosha->isProhibited(from, to, card, others);
}

int Room::drawCard(bool bottom)
{
    thread->trigger(FetchDrawPileCard, this);
    if (m_drawPile->isEmpty())
        swapPile();
    int id = -1;
    if (!bottom)
        id = m_drawPile->takeFirst();
    else
        id = m_drawPile->takeLast();

    return id;
}

void Room::prepareForStart()
{
    if (mode == "06_3v3" || mode == "06_XMode" || mode == "02_1v1") {
        return;
    } else if (isHegemonyGameMode(mode)) {
        if (!ServerInfo.Enable2ndGeneral)
            assignRoles();
        if (Config.RandomSeat)
            qShuffle(m_players);
    } else if (Config.EnableCheat && Config.value("FreeAssign", false).toBool()) {
        ServerPlayer *owner = getOwner();
        notifyMoveFocus(owner, S_COMMAND_CHOOSE_ROLE);
        if ((owner != nullptr) && owner->isOnline()) {
            bool success = doRequest(owner, S_COMMAND_CHOOSE_ROLE, QVariant(), true);
            QVariant clientReply = owner->getClientReply();
            if (!success || !clientReply.canConvert<JsonArray>() || clientReply.value<JsonArray>().size() != 2) {
                if (Config.RandomSeat)
                    qShuffle(m_players);
                assignRoles();
            } else if (Config.FreeAssignSelf) {
                JsonArray replyArray = clientReply.value<JsonArray>();
                QString name = replyArray.value(0).value<JsonArray>().value(0).toString();
                QString role = replyArray.value(1).value<JsonArray>().value(0).toString();
                ServerPlayer *player_self = findChild<ServerPlayer *>(name);
                setPlayerProperty(player_self, "role", role);
                if (mode == "03_1v2" || mode == "04_2v2") {
                    broadcastProperty(player_self, "role", role);
                }

                QList<ServerPlayer *> all_players = m_players;
                all_players.removeOne(player_self);
                int n = all_players.count();
                QStringList roles = Sanguosha->getRoleList(mode);
                roles.removeOne(role);
                qShuffle(roles);

                for (int i = 0; i < n; i++) {
                    ServerPlayer *player = all_players[i];
                    QString role = roles.at(i);

                    player->setRole(role);
                    if (mode == "03_1v2" || mode == "04_2v2") {
                        broadcastProperty(player, "role", role);
                    } else if (role == "lord") {
                        broadcastProperty(player, "role", "lord");
                        setPlayerProperty(player, "role_shown", true);
                    } else {
                        if (mode == "04_1v3") {
                            broadcastProperty(player, "role", role);
                            setPlayerProperty(player, "role_shown", true);
                        } else
                            notifyProperty(player, player, "role");
                    }
                }
            } else {
                JsonArray replyArray = clientReply.value<JsonArray>();
                for (int i = 0; i < replyArray.value(0).value<JsonArray>().size(); i++) {
                    QString name = replyArray.value(0).value<JsonArray>().value(i).toString();
                    QString role = replyArray.value(1).value<JsonArray>().value(i).toString();

                    ServerPlayer *player = findChild<ServerPlayer *>(name);
                    setPlayerProperty(player, "role", role);

                    m_players.swap(i, m_players.indexOf(player));
                }
            }
        } else if (mode == "04_1v3") {
            if (Config.RandomSeat)
                qShuffle(m_players);
            ServerPlayer *lord = m_players.at(qrand() % 4);
            for (int i = 0; i < 4; i++) {
                ServerPlayer *player = m_players.at(i);
                if (player == lord)
                    player->setRole("lord");
                else
                    player->setRole("rebel");
                broadcastProperty(player, "role");
                setPlayerProperty(player, "role_shown", true);
            }
        } else {
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

void Room::reportDisconnection()
{
    ServerPlayer *player = qobject_cast<ServerPlayer *>(sender());
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
    } else if (player->getRole().isEmpty()) {
        // second case
        if (m_players.length() < player_count) {
            player->setParent(nullptr);
            m_players.removeOne(player);

            if (player->getState() != "robot") {
                QString screen_name = player->screenName();
                QString leaveStr = tr("<font color=#000000>Player <b>%1</b> left the game</font>").arg(screen_name);
                speakCommand(player, leaveStr.toUtf8().toBase64());
            }

            doBroadcastNotify(S_COMMAND_REMOVE_PLAYER, player->objectName());
        }
    } else {
        // fourth case
        if (player->m_isWaitingReply) {
            if (_m_raceStarted) {
                // copied from processResponse about race request
                _m_raceWinner.store(player);
                _m_semRaceRequest.release();
            } else
                player->releaseLock(ServerPlayer::SEMA_COMMAND_INTERACTIVE);
        }
        setPlayerProperty(player, "state", "offline");

        bool someone_is_online = false;
        foreach (ServerPlayer *player, m_players) {
            if (player->getState() == "online" || player->getState() == "trust") {
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

    if (player->isOwner()) {
        player->setOwner(false);
        broadcastProperty(player, "owner");
        foreach (ServerPlayer *p, m_players) {
            if (p->getState() == "online") {
                p->setOwner(true);
                broadcastProperty(p, "owner");
                break;
            }
        }
    }
}

void Room::trustCommand(ServerPlayer *player, const QVariant &)
{
    player->acquireLock(ServerPlayer::SEMA_MUTEX);
    if (player->isOnline()) {
        player->setState("trust");
        if (player->m_isWaitingReply) {
            player->releaseLock(ServerPlayer::SEMA_MUTEX);
            if (_m_raceStarted) {
                // copied from processResponse about race request
                _m_raceWinner.store(player);
                _m_semRaceRequest.release();
            } else
                player->releaseLock(ServerPlayer::SEMA_COMMAND_INTERACTIVE);
        }
    } else
        player->setState("online");

    player->releaseLock(ServerPlayer::SEMA_MUTEX);
    broadcastProperty(player, "state");
}

void Room::pauseCommand(ServerPlayer *player, const QVariant &arg)
{
    if (!canPause(player))
        return;
    bool pause = (arg == "true");
    QMutexLocker locker(&m_mutex);
    if (game_paused != pause) {
        JsonArray arg;
        arg << (int)S_GAME_EVENT_PAUSE;
        arg << pause;
        doNotify(player, S_COMMAND_LOG_EVENT, arg);

        game_paused = pause;
        if (!game_paused)
            m_waitCond.wakeAll();
    }
}

void Room::cheat(ServerPlayer *player, const QVariant &args)
{
    player->m_cheatArgs = QVariant();
    if (!Config.EnableCheat)
        return;
    if (!args.canConvert<JsonArray>() || !args.value<JsonArray>().value(0).canConvert(QVariant::Int))
        return;
    //@todo: synchronize this
    player->m_cheatArgs = args;
    makeCheat(player);
}

bool Room::makeSurrender(ServerPlayer *initiator)
{
    bool loyalGiveup = true;
    int loyalAlive = 0;
    bool renegadeGiveup = true;
    int renegadeAlive = 0;
    bool rebelGiveup = true;
    int rebelAlive = 0;

    // broadcast polling request
    QList<ServerPlayer *> playersAlive;
    foreach (ServerPlayer *player, m_players) {
        QString playerRole = player->getRole();
        if (!isHegemonyGameMode(mode)) {
            if ((playerRole == "loyalist" || playerRole == "lord") && player->isAlive())
                loyalAlive++;
            else if (playerRole == "rebel" && player->isAlive())
                rebelAlive++;
            else if (playerRole == "renegade" && player->isAlive())
                renegadeAlive++;
        }

        if (player != initiator && player->isAlive() && player->isOnline()) {
            player->m_commandArgs = (initiator->getGeneral()->objectName());
            playersAlive << player;
        }
    }
    doBroadcastRequest(playersAlive, S_COMMAND_SURRENDER);

    // collect polls
    int hegemony_give_up = 1;
    foreach (ServerPlayer *player, playersAlive) {
        bool result = false;
        if (!player->m_isClientResponseReady || !player->getClientReply().canConvert(QVariant::Bool))
            result = !player->isOnline();
        else
            result = player->getClientReply().toBool();

        if (isHegemonyGameMode(mode)) {
            if (result)
                hegemony_give_up++;
        } else {
            QString playerRole = player->getRole();
            if (playerRole == "loyalist" || playerRole == "lord") {
                loyalGiveup &= result;
                if (player->isAlive())
                    loyalAlive++;
            } else if (playerRole == "rebel") {
                rebelGiveup &= result;
                if (player->isAlive())
                    rebelAlive++;
            } else if (playerRole == "renegade") {
                renegadeGiveup &= result;
                if (player->isAlive())
                    renegadeAlive++;
            }
        }
    }

    // vote counting
    if (isHegemonyGameMode(mode)) {
        if (hegemony_give_up > (playersAlive.length() + 1) / 2) {
            foreach (ServerPlayer *p, getAlivePlayers()) {
                if (!p->hasShownGeneral())
                    p->showGeneral(true, false, false);
                if ((p->getGeneral2() != nullptr) && !p->hasShownGeneral2())
                    p->showGeneral(false, false, false);
            }
            gameOver(".", true);
        }

    } else {
        if (loyalGiveup && renegadeGiveup && !rebelGiveup)
            gameOver("rebel", true);
        else if (loyalGiveup && !renegadeGiveup && rebelGiveup)
            gameOver("renegade", true);
        else if (!loyalGiveup && renegadeGiveup && rebelGiveup)
            gameOver("lord+loyalist", true);
        else if (loyalGiveup && renegadeGiveup && rebelGiveup) {
            // if everyone give up, then ensure that the initiator doesn't win.
            QString playerRole = initiator->getRole();
            if (playerRole == "lord" || playerRole == "loyalist")
                gameOver(renegadeAlive >= rebelAlive ? "renegade" : "rebel", true);
            else if (playerRole == "renegade")
                gameOver(loyalAlive >= rebelAlive ? "loyalist+lord" : "rebel", true);
            else if (playerRole == "rebel")
                gameOver(renegadeAlive >= loyalAlive ? "renegade" : "loyalist+lord", true);
        }
    }

    initiator->setFlags("Global_ForbidSurrender");
    doNotify(initiator, S_COMMAND_ENABLE_SURRENDER, QVariant(false));
    return true;
}

void Room::processRequestPreshow(ServerPlayer *player, const QVariant &arg)
{
    if (player == nullptr)
        return;

    JsonArray args = arg.value<JsonArray>();
    //if (args.size() != 3 || !JsonUtils::isString(args[0]) || !JsonUtils::isBool(args[1]) || !JsonUtils::isBool(args[2])) return;
    if (args.size() != 2 || !JsonUtils::isString(args[0]) || !JsonUtils::isBool(args[1]))
        return;
    player->acquireLock(ServerPlayer::SEMA_MUTEX);

    const QString skill_name = args[0].toString();
    const bool isPreshowed = args[1].toBool();
    //const bool head = args[2].toBool();
    player->setSkillPreshowed(skill_name, isPreshowed); // , head

    player->releaseLock(ServerPlayer::SEMA_MUTEX);
}

void Room::processClientPacket(const QString &request)
{
    Packet packet;
    if (packet.parse(request.toLatin1().constData())) {
        ServerPlayer *player = qobject_cast<ServerPlayer *>(sender());
#ifdef LOGNETWORK
        emit Sanguosha->logNetworkMessage("recv " + player->objectName() + ":" + request);
#endif // LOGNETWORK
        if (game_finished) {
            if ((player != nullptr) && player->isOnline())
                doNotify(player, S_COMMAND_WARN, QString("GAME_OVER"));
            return;
        }
        if (packet.getPacketType() == S_TYPE_REPLY) {
            if (player == nullptr)
                return;
            player->setClientReplyString(request);
            processResponse(player, &packet);
        } else if (packet.getPacketType() == S_TYPE_REQUEST || packet.getPacketType() == S_TYPE_NOTIFICATION) {
            Callback callback = m_callbacks[packet.getCommandType()];
            if (callback == nullptr)
                return;
            (this->*callback)(player, packet.getMessageBody());
        }
    }
}

void Room::addRobotCommand(ServerPlayer *player, const QVariant &)
{
    if ((player != nullptr) && !player->isOwner())
        return;
    if (isFull())
        return;

    if (Config.LimitRobot && !fill_robot) {
        speakCommand(nullptr, QString(tr("This server is limited to add robot. YOU CAN ONLY ADD ROBOT USING \"Fill Robots\".").toUtf8().toBase64()));
        return;
    }

    QStringList names = GetConfigFromLuaState(Sanguosha->getLuaState(), "robot_names").toStringList();
    qShuffle(names);

    int n = 0;
    foreach (ServerPlayer *player, m_players) {
        if (player->getState() == "robot") {
            QString screenname = player->screenName();
            if (names.contains(screenname))
                names.removeAll(screenname);
            else
                n++;
        }
    }

    ServerPlayer *robot = new ServerPlayer(this);
    robot->setState("robot");

    m_players << robot;

    const QString robot_name = names.isEmpty() ? tr("Computer %1").arg(QChar('A' + n)) : names.first();
    const QString robot_avatar = Sanguosha->getRandomGeneralName();
    signup(robot, robot_name, robot_avatar, true);

    QString greeting = tr("Hello, I'm a robot").toUtf8().toBase64();
    speakCommand(robot, greeting);

    broadcastProperty(robot, "state");
}

void Room::fillRobotsCommand(ServerPlayer *player, const QVariant &)
{
    if (Config.LimitRobot && (m_players.length() <= player_count / 2 || player_count <= 4)) {
        speakCommand(nullptr,
                     QString(tr("This server is limited to add robot. Please ensure that the number of players is more than 4 and there is more than a half human players.")
                                 .toUtf8()
                                 .toBase64()));
        return;
    }
    fill_robot = true;
    int left = player_count - m_players.length();
    for (int i = 0; i < left; i++)
        addRobotCommand(player, QVariant());
}

ServerPlayer *Room::getOwner() const
{
    foreach (ServerPlayer *player, m_players) {
        if (player->isOwner())
            return player;
    }

    return nullptr;
}

void Room::toggleReadyCommand(ServerPlayer *, const QVariant &)
{
    if (!game_started2 && isFull()) {
        game_started2 = true;
        thread = new RoomThread(this);
        thread->start();
    }
}

void Room::signup(ServerPlayer *player, const QString &screen_name, const QString &avatar, bool is_robot)
{
    player->setObjectName(generatePlayerName());
    player->setProperty("avatar", avatar);
    player->setScreenName(screen_name);

    if (!is_robot) {
        notifyProperty(player, player, "objectName");

        ServerPlayer *owner = getOwner();
        if (owner == nullptr) {
            player->setOwner(true);
            notifyProperty(player, player, "owner");
        }
    }

    // introduce the new joined player to existing players except himself
    player->introduceTo(nullptr);
    if (!is_robot) {
        QString plus = QString(QStringLiteral("<font color=red>(%1)</font>")).arg(QString::number(player->ipv4Address() / 256, 16).toUpper());
        QString greetingStr = tr("<font color=#EEB422>Player <b>%1</b> joined the game</font>").arg(screen_name + plus);
        speakCommand(player, greetingStr.toUtf8().toBase64());
        player->startNetworkDelayTest();

        // introduce all existing player to the new joined
        foreach (ServerPlayer *p, m_players) {
            if (p != player)
                p->introduceTo(player);
        }
    } else
        toggleReadyCommand(player, QString());
}

void Room::assignGeneralsForPlayers(const QList<ServerPlayer *> &to_assign)
{
    QSet<QString> existed;
    foreach (ServerPlayer *player, m_players) {
        if (player->getGeneral() != nullptr)
            existed << player->getGeneralName();
        if (player->getGeneral2() != nullptr)
            existed << player->getGeneral2Name();
    }

    const int total = Sanguosha->getGeneralCount();
    const int max_available = (total - existed.size()) / to_assign.length();

    QStringList choices = Sanguosha->getRandomGenerals(total - existed.size(), existed);
    QStringList latest = Sanguosha->getLatestGenerals(existed);
    bool assign_latest_general = Config.value("AssignLatestGeneral", true).toBool() && !isHegemonyGameMode(mode);
    foreach (ServerPlayer *player, to_assign) {
        int max_choice = Config.value("MaxChoice", 6).toInt();
        if (mode == "03_1v2") {
            if (player->isLord())
                max_choice = Config.value("LandlordMaxChoice", 8).toInt();
            else
                max_choice = Config.value("PeasantMaxChoice", 5).toInt();
        }
        int choice_count = qMin(max_choice, max_available);

        player->clearSelected();
        int i = 0;
        if (assign_latest_general && !latest.isEmpty()) {
            QString choice = player->findReasonable(latest, true);
            if (!choice.isEmpty()) {
                player->addToSelected(choice);
                latest.removeOne(choice);
                i++;
                if (choices.contains(choice))
                    choices.removeOne(choice);
            }
        }

        for (; i < choice_count; i++) {
            QString choice = player->findReasonable(choices, true);
            if (choice.isEmpty())
                break;
            player->addToSelected(choice);
            choices.removeOne(choice);
            if (latest.contains(choice))
                latest.removeOne(choice);
        }
    }
}

void Room::chooseGenerals()
{
    QStringList ban_list = Config.value("Banlist/Roles").toStringList();
    //Sanguosha->banRandomGods(); //why this function add the rest gods into banlist....
    // for lord.
    int lord_num = Config.value("LordMaxChoice", 6).toInt();
    int nonlord_num = Config.value("NonLordMaxChoice", 6).toInt();
    if (lord_num == 0 && nonlord_num == 0)
        nonlord_num = 1;
    int nonlord_prob = (lord_num == -1) ? 5 : 55 - qMin(lord_num, 10);
    QStringList lord_list;
    ServerPlayer *the_lord = getLord();
    if (Config.EnableSame)
        lord_list = Sanguosha->getRandomGenerals(Config.value("MaxChoice", 6).toInt());
    else if (the_lord->getState() == "robot")
        if (((qrand() % 100 < nonlord_prob || lord_num == 0) && nonlord_num > 0) || Sanguosha->getLords().length() == 0)
            lord_list = Sanguosha->getRandomGenerals(1);
        else
            lord_list = Sanguosha->getLords();
    else
        lord_list = Sanguosha->getRandomLords();
    QString general = askForGeneral(the_lord, lord_list);
    the_lord->setGeneralName(general);
    broadcastProperty(the_lord, "general", general);

    if (Config.EnableSame) {
        foreach (ServerPlayer *p, m_players) {
            if (!p->isLord())
                p->setGeneralName(general);
        }

        Config.Enable2ndGeneral = false;
        return;
    }
    //for others without lord
    QList<ServerPlayer *> to_assign = m_players;
    to_assign.removeOne(getLord());

    assignGeneralsForPlayers(to_assign);
    foreach (ServerPlayer *player, to_assign)
        _setupChooseGeneralRequestArgs(player);

    doBroadcastRequest(to_assign, S_COMMAND_CHOOSE_GENERAL);
    foreach (ServerPlayer *player, to_assign) {
        if (player->getGeneral() != nullptr)
            continue;
        QString generalName = player->getClientReply().toString();
        if (!player->m_isClientResponseReady || !_setPlayerGeneral(player, generalName, true))
            _setPlayerGeneral(player, _chooseDefaultGeneral(player), true);
    }

    if (Config.Enable2ndGeneral) {
        QList<ServerPlayer *> to_assign = m_players;
        assignGeneralsForPlayers(to_assign);
        foreach (ServerPlayer *player, to_assign)
            _setupChooseGeneralRequestArgs(player);

        doBroadcastRequest(to_assign, S_COMMAND_CHOOSE_GENERAL);
        foreach (ServerPlayer *player, to_assign) {
            if (player->getGeneral2() != nullptr)
                continue;
            QString generalName = player->getClientReply().toString();
            if (!player->m_isClientResponseReady || !_setPlayerGeneral(player, generalName, false))
                _setPlayerGeneral(player, _chooseDefaultGeneral(player), false);
        }
    }

    Config.setValue("Banlist/Roles", ban_list);
}

void Room::choose1v2Generals()
{
    QStringList ban_list = Config.value("Banlist/03_1v2").toStringList();
    //Sanguosha->banRandomGods(); //why this function add the rest gods into banlist....

    QList<ServerPlayer *> to_assign = m_players;

    assignGeneralsForPlayers(to_assign);
    foreach (ServerPlayer *player, to_assign)
        _setupChooseGeneralRequestArgs(player);

    doBroadcastRequest(to_assign, S_COMMAND_CHOOSE_GENERAL);
    foreach (ServerPlayer *player, to_assign) {
        if (player->getGeneral() != nullptr)
            continue;
        QString generalName = player->getClientReply().toString();
        if (!player->m_isClientResponseReady || !_setPlayerGeneral(player, generalName, true))
            _setPlayerGeneral(player, _chooseDefaultGeneral(player), true);
    }

    if (Config.Enable2ndGeneral) {
        QList<ServerPlayer *> to_assign = m_players;
        assignGeneralsForPlayers(to_assign);
        foreach (ServerPlayer *player, to_assign)
            _setupChooseGeneralRequestArgs(player);

        doBroadcastRequest(to_assign, S_COMMAND_CHOOSE_GENERAL);
        foreach (ServerPlayer *player, to_assign) {
            if (player->getGeneral2() != nullptr)
                continue;
            QString generalName = player->getClientReply().toString();
            if (!player->m_isClientResponseReady || !_setPlayerGeneral(player, generalName, false))
                _setPlayerGeneral(player, _chooseDefaultGeneral(player), false);
        }
    }

    Config.setValue("Banlist/03_1v2", ban_list);
}

void Room::choose2v2Generals()
{
    QStringList ban_list = Config.value("Banlist/04_2v2").toStringList();
    //Sanguosha->banRandomGods(); //why this function add the rest gods into banlist....

    QList<ServerPlayer *> to_assign = m_players;

    assignGeneralsForPlayers(to_assign);
    foreach (ServerPlayer *player, to_assign)
        _setupChooseGeneralRequestArgs(player);

    doBroadcastRequest(to_assign, S_COMMAND_CHOOSE_GENERAL);
    foreach (ServerPlayer *player, to_assign) {
        if (player->getGeneral() != nullptr)
            continue;
        QString generalName = player->getClientReply().toString();
        if (!player->m_isClientResponseReady || !_setPlayerGeneral(player, generalName, true))
            _setPlayerGeneral(player, _chooseDefaultGeneral(player), true);
    }

    if (Config.Enable2ndGeneral) {
        QList<ServerPlayer *> to_assign = m_players;
        assignGeneralsForPlayers(to_assign);
        foreach (ServerPlayer *player, to_assign)
            _setupChooseGeneralRequestArgs(player);

        doBroadcastRequest(to_assign, S_COMMAND_CHOOSE_GENERAL);
        foreach (ServerPlayer *player, to_assign) {
            if (player->getGeneral2() != nullptr)
                continue;
            QString generalName = player->getClientReply().toString();
            if (!player->m_isClientResponseReady || !_setPlayerGeneral(player, generalName, false))
                _setPlayerGeneral(player, _chooseDefaultGeneral(player), false);
        }
    }

    Config.setValue("Banlist/04_2v2", ban_list);
}

void Room::chooseHegemonyGenerals()
{
    QStringList ban_list = Config.value("Banlist/Hegemony").toStringList();
    QList<ServerPlayer *> to_assign = m_players;

    assignGeneralsForPlayers(to_assign);
    foreach (ServerPlayer *player, to_assign)
        _setupChooseGeneralRequestArgs(player);

    doBroadcastRequest(to_assign, S_COMMAND_CHOOSE_GENERAL);

    if (!ServerInfo.Enable2ndGeneral) {
        foreach (ServerPlayer *player, to_assign) {
            if (player->getGeneral() != nullptr)
                continue;
            QString generalName = player->getClientReply().toString();
            if (!player->m_isClientResponseReady || !_setPlayerGeneral(player, generalName, true))
                _setPlayerGeneral(player, _chooseDefaultGeneral(player), true);
        }
    } else {
        foreach (ServerPlayer *player, to_assign) {
            if (player->getGeneral() != nullptr)
                continue;
            const QVariant &generalName = player->getClientReply();
            if (!player->m_isClientResponseReady || !JsonUtils::isString(generalName)) {
                QStringList default_generals = _chooseDefaultGenerals(player);
                _setPlayerGeneral(player, default_generals.first(), true);
                _setPlayerGeneral(player, default_generals.last(), false);
            } else {
                QStringList generals = generalName.toString().split("+");
                if (generals.length() != 2 || !_setPlayerGeneral(player, generals.first(), true) || !_setPlayerGeneral(player, generals.last(), false)) {
                    QStringList default_generals = _chooseDefaultGenerals(player);
                    _setPlayerGeneral(player, default_generals.first(), true);
                    _setPlayerGeneral(player, default_generals.last(), false);
                }
            }
        }
    }

    /*if (Config.Enable2ndGeneral) {
        //QList<ServerPlayer *> to_assign = m_players;
        assignGeneralsForPlayers(to_assign);
        foreach(ServerPlayer *player, to_assign)
            _setupChooseGeneralRequestArgs(player);

        doBroadcastRequest(to_assign, S_COMMAND_CHOOSE_GENERAL);
        foreach(ServerPlayer *player, to_assign) {
            if (player->getGeneral2() != NULL)
                continue;
            QString generalName = player->getClientReply().toString();
            if (!player->m_isClientResponseReady || !_setPlayerGeneral(player, generalName, false))
                _setPlayerGeneral(player, _chooseDefaultGeneral(player), false);
        }
    }*/

    //@todo
    //int i = 1;
    foreach (ServerPlayer *player, m_players) {
        QStringList names;
        if (player->getGeneral() != nullptr) {
            QString name = player->getGeneralName();
            //QStringList roles;
            //roles << "wei" << "shu" << "wu" << "qun";
            //int role_idx = qrand() % roles.length();
            QString role = Sanguosha->getGeneral(name)->getKingdom();
            if (role == "zhu")
                role = "careerist";
            names.append(name);
            //player->setActualGeneral1Name(name);
            player->setRole(role);
            player->setGeneralName("anjiang");
            //notifyProperty(player, player, "actual_general1");
            foreach (ServerPlayer *p, getOtherPlayers(player))
                notifyProperty(p, player, "general");
            notifyProperty(player, player, "general", name);
            notifyProperty(player, player, "role", role);
        }
        if (player->getGeneral2() != nullptr) {
            QString name = player->getGeneral2Name();
            names.append(name);
            //player->setActualGeneral2Name(name);
            player->setGeneral2Name("anjiang");
            //notifyProperty(player, player, "actual_general2");
            foreach (ServerPlayer *p, getOtherPlayers(player))
                notifyProperty(p, player, "general2");
            notifyProperty(player, player, "general2", name);

            QString role = Sanguosha->getGeneral(name)->getKingdom();
            if (role == "zhu") {
                role = "careerist";
                player->setRole(role);
                notifyProperty(player, player, "role", role);
            }
        }

        this->setTag(player->objectName(), QVariant::fromValue(names));
    }

    Config.setValue("Banlist/Hegemony", ban_list);
}

void Room::assignRoles()
{
    int n = m_players.count();

    QStringList roles = Sanguosha->getRoleList(mode);
    if (mode != "04_2v2")
        qShuffle(roles);

    for (int i = 0; i < n; i++) {
        ServerPlayer *player = m_players[i];
        QString role = roles.at(i);

        player->setRole(role);
        if (role == "lord") {
            broadcastProperty(player, "role", "lord");
            setPlayerProperty(player, "role_shown", true);
        } else if (mode == "03_1v2" || mode == "04_2v2") {
            broadcastProperty(player, "role", role);
            //room->setPlayerProperty(player, "role_shown", true); //important! to notify client
        } else
            notifyProperty(player, player, "role");
    }
}

void Room::swapSeat(ServerPlayer *a, ServerPlayer *b)
{
    int seat1 = m_players.indexOf(a);
    int seat2 = m_players.indexOf(b);

    m_players.swap(seat1, seat2);

    QStringList player_circle;
    foreach (ServerPlayer *player, m_players)
        player_circle << player->objectName();
    doBroadcastNotify(S_COMMAND_ARRANGE_SEATS, JsonUtils::toJsonArray(player_circle));

    m_alivePlayers.clear();
    for (int i = 0; i < m_players.length(); i++) {
        ServerPlayer *player = m_players.at(i);
        if (player->isAlive()) {
            m_alivePlayers << player;
            player->setSeat(m_alivePlayers.length());
        } else {
            player->setSeat(0);
        }

        broadcastProperty(player, "seat");

        player->setNext(m_players.at((i + 1) % m_players.length()));
        broadcastProperty(player, "next");
    }
}

void Room::setPlayerSkillInvalidity(ServerPlayer *player, const Skill *skill, bool invalidity, bool trigger_event)
{
    QString skill_name = "_ALL_SKILLS";
    if (skill != nullptr)
        skill_name = skill->objectName();

    setPlayerSkillInvalidity(player, skill_name, invalidity, trigger_event);
}

void Room::setPlayerSkillInvalidity(ServerPlayer *player, const QString &skill_name, bool invalidity, bool trigger_event)
{
    player->setSkillInvalidity(skill_name, invalidity);

    JsonArray arr;
    arr << player->objectName() << skill_name << invalidity;
    doBroadcastNotify(QSanProtocol::S_COMMAND_SET_SKILL_INVALIDITY, arr);

    foreach (ServerPlayer *p, getAllPlayers())
        filterCards(p, p->getCards("hes"), true);

    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

    if (trigger_event) {
        QList<SkillInvalidStruct> invalid_list;
        //how to deal skill_name == "_ALL_SKILLS"??
        SkillInvalidStruct invalid;
        invalid.invalid = invalidity;
        invalid.player = player;
        invalid.skill = Sanguosha->getSkill(skill_name);
        invalid_list << invalid;

        QVariant v = QVariant::fromValue(invalid_list);
        thread->trigger(EventSkillInvalidityChange, this, v);
    }
}

void Room::adjustSeats()
{
    QList<ServerPlayer *> players;
    int i = 0;
    if (mode == "04_2v2" && Config.EnableCheat && Config.value("FreeAssign", false).toBool() && Config.FreeAssignSelf) { //shffule players then fix seats
        qShuffle(m_players);
        for (i = 0; i < m_players.length(); i++) {
            if (m_players.at(i)->getRoleEnum() == Player::Loyalist) {
                players << m_players.at(i);
                break;
            }
        }
        for (int j = 0; j < m_players.length(); j++) {
            if (m_players.at(j)->getRoleEnum() != Player::Loyalist)
                players << m_players.at(j);
        }
        for (int k = i + 1; k < m_players.length(); k++) {
            if (m_players.at(k)->getRoleEnum() == Player::Loyalist) {
                players << m_players.at(k);
                break;
            }
        }
        m_players = players;
    } else {
        for (i = 0; i < m_players.length(); i++) {
            if (m_players.at(i)->getRoleEnum() == Player::Lord)
                break;
        }
        for (int j = i; j < m_players.length(); j++)
            players << m_players.at(j);
        for (int j = 0; j < i; j++)
            players << m_players.at(j);

        m_players = players;
    }
    for (int i = 0; i < m_players.length(); i++)
        m_players.at(i)->setSeat(i + 1);

    // tell the players about the seat, and the first is always the lord
    QStringList player_circle;
    foreach (ServerPlayer *player, m_players)
        player_circle << player->objectName();

    doBroadcastNotify(S_COMMAND_ARRANGE_SEATS, JsonUtils::toJsonArray(player_circle));

    //record initial seat
    foreach (ServerPlayer *player, m_players) {
        if (player->getInitialSeat() == 0) {
            player->setInitialSeat(player->getSeat());
            broadcastProperty(player, "inital_seat");
        }
    }
}

int Room::getCardFromPile(const QString &card_pattern)
{
    if (m_drawPile->isEmpty())
        swapPile();

    if (card_pattern.startsWith("@")) {
        if (card_pattern == "@duanliang") {
            foreach (int card_id, *m_drawPile) {
                const Card *card = Sanguosha->getCard(card_id);
                if (card->isBlack() && (card->isKindOf("BasicCard") || card->isKindOf("EquipCard")))
                    return card_id;
            }
        }
    } else {
        QString card_name = card_pattern;
        foreach (int card_id, *m_drawPile) {
            const Card *card = Sanguosha->getCard(card_id);
            if (card->objectName() == card_name)
                return card_id;
        }
    }

    return -1;
}

QString Room::_chooseDefaultGeneral(ServerPlayer *player) const
{
    Q_ASSERT(!player->getSelected().isEmpty());
    QString choice = m_generalSelector->selectFirst(player, player->getSelected());
    return choice;
}

QStringList Room::_chooseDefaultGenerals(ServerPlayer *player) const
{
    Q_ASSERT(!player->getSelected().isEmpty());
    QStringList generals;
    QStringList candidates = player->getSelected();

    QStringList general_pairs;
    //temp AI
    foreach (const QString &a, candidates) {
        foreach (const QString &b, candidates) {
            if (a == b)
                continue;
            const General *general1 = Sanguosha->getGeneral(a);
            const General *general2 = Sanguosha->getGeneral(b);
            if ((general1->getKingdom() == general2->getKingdom()) || (general1->getKingdom() == "zhu") || (general2->getKingdom() == "zhu")) {
                general_pairs << (a + "+" + b);
                //generals << a << b;
                //break;
            }
        }
        //if (!generals.isEmpty())
        //    break;
    }
    int index = qrand() % general_pairs.length();
    generals = general_pairs[index].split("+");
    return generals;

    if (isHegemonyGameMode(mode)) {
        QString choice = m_generalSelector->selectPair(player, candidates);
        generals = choice.split("+");

    } else {
        QString choice1 = m_generalSelector->selectFirst(player, candidates);
        candidates.removeAll(choice1);
        QString choice2 = m_generalSelector->selectSecond(player, candidates);
        generals << choice1 << choice2;
    }

    //m_generalSelector->selectGenerals(player, player->getSelected());

    Q_ASSERT(!generals.isEmpty());
    return generals;
}

bool Room::_setPlayerGeneral(ServerPlayer *player, const QString &generalName, bool isFirst)
{
    const General *general = Sanguosha->getGeneral(generalName);
    if (general == nullptr)
        return false;
    else if (!Config.FreeChoose && !player->getSelected().contains(generalName))
        return false;

    if (isFirst) {
        player->setGeneralName(general->objectName());
        notifyProperty(player, player, "general");
    } else {
        player->setGeneral2Name(general->objectName());
        notifyProperty(player, player, "general2");
    }
    return true;
}

void Room::speakCommand(ServerPlayer *player, const QVariant &arg)
{
#define _NO_BROADCAST_SPEAKING                     \
    do {                                           \
        broadcast = false;                         \
        JsonArray nbbody;                          \
        nbbody << player->objectName();            \
        nbbody << arg;                             \
        doNotify(player, S_COMMAND_SPEAK, nbbody); \
    } while (false)
    bool broadcast = true;
    if ((player != nullptr) && Config.EnableCheat) {
        QString sentence = QString::fromUtf8(QByteArray::fromBase64(arg.toString().toLatin1()));
        if (sentence == ".BroadcastRoles") {
            _NO_BROADCAST_SPEAKING;
            foreach (ServerPlayer *p, m_alivePlayers) {
                broadcastProperty(p, "role", p->getRole());
                setPlayerProperty(p, "role_shown", true);
            }
        } else if (sentence.startsWith(".BroadcastRoles=")) {
            _NO_BROADCAST_SPEAKING;
            QString name = sentence.mid(12);
            foreach (ServerPlayer *p, m_alivePlayers) {
                if (p->objectName() == name || p->getGeneralName() == name) {
                    broadcastProperty(p, "role", p->getRole());
                    setPlayerProperty(p, "role_shown", true);
                    break;
                }
            }
        } else if (sentence == ".ShowHandCards") {
            _NO_BROADCAST_SPEAKING;
            QString split("----------");
            split = split.toUtf8().toBase64();
            JsonArray body;
            body << player->objectName() << split;
            doNotify(player, S_COMMAND_SPEAK, body);
            foreach (ServerPlayer *p, m_alivePlayers) {
                if (!p->isKongcheng()) {
                    QStringList handcards;
                    foreach (const Card *card, p->getHandcards())
                        handcards << QString("<b>%1</b>").arg(Sanguosha->getEngineCard(card->getId())->getLogName());
                    QString hand = handcards.join(", ");
                    hand = hand.toUtf8().toBase64();
                    JsonArray body;
                    body << p->objectName() << hand;
                    doNotify(player, S_COMMAND_SPEAK, body);
                }
            }
            doNotify(player, S_COMMAND_SPEAK, body);
        } else if (sentence.startsWith(".ShowHandCards=")) {
            _NO_BROADCAST_SPEAKING;
            QString name = sentence.mid(15);
            foreach (ServerPlayer *p, m_alivePlayers) {
                if (p->objectName() == name || p->getGeneralName() == name) {
                    if (!p->isKongcheng()) {
                        QStringList handcards;
                        foreach (const Card *card, p->getHandcards())
                            handcards << QString("<b>%1</b>").arg(Sanguosha->getEngineCard(card->getId())->getLogName());
                        QString hand = handcards.join(", ");
                        hand = hand.toUtf8().toBase64();
                        JsonArray body;
                        body << p->objectName() << hand;
                        doNotify(player, S_COMMAND_SPEAK, body);
                    }
                    break;
                }
            }
        } else if (sentence.startsWith(".ShowPrivatePile=")) {
            _NO_BROADCAST_SPEAKING;
            QStringList arg = sentence.mid(17).split(":");
            if (arg.length() == 2) {
                QString name = arg.first();
                QString pile_name = arg.last();
                foreach (ServerPlayer *p, m_alivePlayers) {
                    if (p->objectName() == name || p->getGeneralName() == name) {
                        if (!p->getPile(pile_name).isEmpty()) {
                            QStringList pile_cards;
                            foreach (int id, p->getPile(pile_name))
                                pile_cards << QString("<b>%1</b>").arg(Sanguosha->getEngineCard(id)->getLogName());
                            QString pile = pile_cards.join(", ");
                            pile = pile.toUtf8().toBase64();
                            JsonArray body;
                            body << p->objectName() << pile;
                            doNotify(player, S_COMMAND_SPEAK, body);
                        }
                        break;
                    }
                }
            }
        } else if (sentence.startsWith(".SetAIDelay=")) {
            _NO_BROADCAST_SPEAKING;
            bool ok = false;
            int delay = sentence.midRef(12).toInt(&ok);
            if (ok) {
                Config.AIDelay = Config.OriginAIDelay = delay;
                Config.setValue("OriginAIDelay", delay);
            }
        } else if (sentence.startsWith(".SetGameMode=")) {
            _NO_BROADCAST_SPEAKING;
            QString name = sentence.mid(13);
            setTag("NextGameMode", name);
        } else if (sentence.startsWith(".SecondGeneral=")) {
            _NO_BROADCAST_SPEAKING;
            QString prop = sentence.mid(15);
            setTag("NextGameSecondGeneral", !prop.isEmpty() && prop != "0" && prop != "false");
        } else if (sentence == ".Pause") {
            _NO_BROADCAST_SPEAKING;
            pauseCommand(player, "true");
        } else if (sentence == ".Resume") {
            _NO_BROADCAST_SPEAKING;
            pauseCommand(player, "false");
        }
    }
    if (broadcast) {
        JsonArray body;
        if (player != nullptr)
            body << player->objectName();
        else
            body << ".";
        body << arg;
        doBroadcastNotify(S_COMMAND_SPEAK, body);
    }

#undef _NO_BROADCAST_SPEAKING
}

void Room::processResponse(ServerPlayer *player, const Packet *packet)
{
    player->acquireLock(ServerPlayer::SEMA_MUTEX);
    bool success = false;
    if (player == nullptr)
        emit room_message(tr("Unable to parse player"));
    else if (!player->m_isWaitingReply || player->m_isClientResponseReady)
        emit room_message(tr("Server is not waiting for reply from %1").arg(player->objectName()));
    else if (packet->getCommandType() != player->m_expectedReplyCommand)
        emit room_message(tr("Reply command should be %1 instead of %2").arg(player->m_expectedReplyCommand).arg(packet->getCommandType()));
    else if (packet->localSerial != player->m_expectedReplySerial)
        emit room_message(tr("Reply serial should be %1 instead of %2").arg(player->m_expectedReplySerial).arg(packet->localSerial));
    else
        success = true;

    if (!success) {
        player->releaseLock(ServerPlayer::SEMA_MUTEX);
        return;
    } else {
        _m_semRoomMutex.acquire();
        if (_m_raceStarted) {
            player->setClientReply(packet->getMessageBody());
            player->m_isClientResponseReady = true;
            // Warning: the statement below must be the last one before releasing the lock!!!
            // Any statement after this statement will totally compromise the synchronization
            // because getRaceResult will then be able to acquire the lock, reading a non-null
            // raceWinner and proceed with partial data. The current implementation is based on
            // the assumption that the following line is ATOMIC!!!
            _m_raceWinner.store(player);
            // the _m_semRoomMutex.release() signal is in getRaceResult();
            _m_semRaceRequest.release();
        } else {
            _m_semRoomMutex.release();
            player->setClientReply(packet->getMessageBody());
            player->m_isClientResponseReady = true;
            player->releaseLock(ServerPlayer::SEMA_COMMAND_INTERACTIVE);
        }

        player->releaseLock(ServerPlayer::SEMA_MUTEX);
    }
}

bool Room::useCard(const CardUseStruct &use, bool add_history)
{
    CardUseStruct card_use = use;
    card_use.m_addHistory = false;
    card_use.m_isHandcard = true;
    card_use.m_isLastHandcard = true;
    const Card *card = card_use.card;

    QList<int> ids;
    if (!card->isVirtualCard())
        ids << card->getEffectiveId();
    else
        ids = card->getSubcards();
    if (!ids.isEmpty()) {
        foreach (int id, ids) {
            if (getCardOwner(id) != use.from || getCardPlace(id) != Player::PlaceHand) {
                card_use.m_isHandcard = false;
                break;
            }
        }
    } else {
        card_use.m_isHandcard = false;
    }

    if (!ids.isEmpty()) {
        foreach (const Card *c, use.from->getHandcards()) {
            if (!ids.contains(c->getEffectiveId())) {
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
            setCardFlag(card_use.card, "showncards");
    }
    if (card_use.from->isCardLimited(card, card->getHandlingMethod()) && (!card->canRecast() || card_use.from->isCardLimited(card, Card::MethodRecast)))
        return true;

    QString key;
    if (card->inherits("LuaSkillCard"))
        key = "#" + card->objectName();
    else
        key = card->getClassName();
    int slash_count = card_use.from->getSlashCount();
    bool showTMskill = false;
    foreach (const Skill *skill, card_use.from->getSkillList(false, true)) {
        if (skill->inherits("TargetModSkill")) {
            const TargetModSkill *tm = qobject_cast<const TargetModSkill *>(skill);
            if (tm->getResidueNum(card_use.from, card) > 500)
                showTMskill = true; //&& card_use.from->isHiddenSkill()
        }
    }
    bool slash_not_record = key.contains("Slash") && slash_count > 0 && (card_use.from->hasWeapon("Crossbow") || showTMskill);

    card = card_use.card->validate(card_use);
    if (card == nullptr)
        return false;

    if (card_use.from->getPhase() == Player::Play && add_history && (card_use.m_reason == CardUseStruct::CARD_USE_REASON_PLAY || card->hasFlag("Add_History"))) {
        if (!slash_not_record) {
            card_use.m_addHistory = true;
            addPlayerHistory(card_use.from, key);
        }
        addPlayerHistory(nullptr, "pushPile");
    }

    try {
        if (card_use.card->getRealCard() == card) {
            if (use.from != nullptr) {
                QStringList tarmod_detect = use.from->checkTargetModSkillShow(card_use);
                if (!tarmod_detect.isEmpty()) {
                    QString to_show = askForChoice(card_use.from, "tarmod_show", tarmod_detect.join("+"), QVariant::fromValue(card_use));
                    card_use.from->showHiddenSkill(to_show);
                }
            }

            if (card->isKindOf("DelayedTrick") && card->isVirtualCard() && card->subcardsLength() == 1) {
                Card *trick = Sanguosha->cloneCard(card);
                Q_ASSERT(trick != nullptr);
                WrappedCard *wrapped = Sanguosha->getWrappedCard(card->getSubcards().constFirst());
                wrapped->takeOver(trick);
                broadcastUpdateCard(getPlayers(), wrapped->getId(), wrapped);
                card_use.card = wrapped;
                wrapped->onUse(this, card_use);
                return true;
            }
            if (card_use.card->isKindOf("Slash") && add_history && slash_count > 0)
                card_use.from->setFlags("Global_MoreSlashInOneTurn");
            if (!card_use.card->isVirtualCard()) {
                WrappedCard *wrapped = Sanguosha->getWrappedCard(card_use.card->getEffectiveId());
                if (wrapped->isModified())
                    broadcastUpdateCard(getPlayers(), card_use.card->getEffectiveId(), wrapped);
                else
                    broadcastResetCard(getPlayers(), card_use.card->getEffectiveId());
            }
            card_use.card->onUse(this, card_use);
        } else if (card != nullptr) {
            CardUseStruct new_use = card_use;
            new_use.card = card;
            new_use.m_effectValue = card_use.m_effectValue;
            useCard(new_use);
        }
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken) {
            if (getCardPlace(card_use.card->getEffectiveId()) == Player::PlaceTable) {
                CardMoveReason reason(CardMoveReason::S_REASON_UNKNOWN, card_use.from->objectName(), QString(), card_use.card->getSkillName(), QString());
                if (card_use.to.size() == 1)
                    reason.m_targetId = card_use.to.first()->objectName();
                moveCardTo(card_use.card, card_use.from, nullptr, Player::DiscardPile, reason, true);
            }
            QVariant data = QVariant::fromValue(card_use);
            card_use.from->setFlags("Global_ProcessBroken");
            thread->trigger(CardFinished, this, data);
            card_use.from->setFlags("-Global_ProcessBroken");

            foreach (ServerPlayer *p, m_alivePlayers) {
                p->tag.remove("Qinggang");

                foreach (const QString &flag, p->getFlagList()) {
                    if (flag == "Global_GongxinOperator")
                        p->setFlags("-" + flag);
                    else if (flag.endsWith("_InTempMoving"))
                        setPlayerFlag(p, "-" + flag);
                }
            }

            foreach (int id, Sanguosha->getRandomCards()) {
                if (getCardPlace(id) == Player::PlaceTable || getCardPlace(id) == Player::PlaceJudge)
                    moveCardTo(Sanguosha->getCard(id), nullptr, Player::DiscardPile, true);
                if (Sanguosha->getCard(id)->hasFlag("using"))
                    setCardFlag(id, "-using");
            }
        }
        throw triggerEvent;
    }
    return true;
}

void Room::loseHp(ServerPlayer *victim, int lose)
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
    if (thread->trigger(PreHpLost, this, data))
        return;

    l = data.value<HpLostStruct>();
    if (l.num <= 0)
        return;

    LogMessage log;
    log.type = "#LoseHp";
    log.from = victim;
    log.arg = QString::number(l.num);
    sendLog(log);

    setPlayerProperty(victim, "hp", victim->getHp() - l.num);

    JsonArray arg;
    arg << victim->objectName();
    arg << -l.num;
    arg << -1;
    doBroadcastNotify(S_COMMAND_CHANGE_HP, arg);

    thread->trigger(PostHpReduced, this, data);
    thread->trigger(PostHpLost, this, data);
}

void Room::loseMaxHp(ServerPlayer *victim, int lose)
{
    Q_ASSERT(lose > 0);
    if (lose <= 0)
        return;

    int hp_1 = victim->getHp();
    victim->setMaxHp(qMax(victim->getMaxHp() - lose, 0));
    int hp_2 = victim->getHp();

    broadcastProperty(victim, "maxhp");
    broadcastProperty(victim, "hp");

    LogMessage log;
    log.type = "#LoseMaxHp";
    log.from = victim;
    log.arg = QString::number(lose);
    sendLog(log);

    JsonArray arg;
    arg << victim->objectName();
    arg << -lose;
    doBroadcastNotify(S_COMMAND_CHANGE_MAXHP, arg);

    LogMessage log2;
    log2.type = "#GetHp";
    log2.from = victim;
    log2.arg = QString::number(victim->getHp());
    log2.arg2 = QString::number(victim->getMaxHp());
    sendLog(log2);

    if (victim->getMaxHp() < victim->dyingThreshold())
        killPlayer(victim);
    else {
        QVariant v = QVariant::fromValue(victim);
        thread->trigger(MaxHpChanged, this, v);
        if (hp_1 > hp_2) {
            HpLostStruct l;
            l.num = hp_1 - hp_2;
            l.player = victim;
            QVariant data = QVariant::fromValue(l);
            thread->trigger(PostHpReduced, this, data);
        }
    }
}

bool Room::changeMaxHpForAwakenSkill(ServerPlayer *player, int magnitude)
{
    player->gainMark("@waked");
    int n = player->getMark("@waked");
    if (magnitude < 0) {
        if (Config.Enable2ndGeneral && (player->getGeneral() != nullptr) && (player->getGeneral2() != nullptr) && Config.MaxHpScheme > 0 && Config.PreventAwakenBelow3
            && player->getMaxHp() <= 3) {
            setPlayerMark(player, "AwakenLostMaxHp", 1);
        } else {
            loseMaxHp(player, -magnitude);
        }
    } else {
        setPlayerProperty(player, "maxhp", player->getMaxHp() + magnitude);

        LogMessage log;
        log.type = "#GainMaxHp";
        log.from = player;
        log.arg = QString::number(magnitude);
        sendLog(log);

        LogMessage log2;
        log2.type = "#GetHp";
        log2.from = player;
        log2.arg = QString::number(player->getHp());
        log2.arg2 = QString::number(player->getMaxHp());
        sendLog(log2);
    }
    return (player->getMark("@waked") >= n);
}

void Room::applyDamage(ServerPlayer *victim, const DamageStruct &damage)
{
    int new_hp = victim->getHp() - damage.damage;

    if (!victim->hasSkill("banling"))
        setPlayerProperty(victim, "hp", new_hp);
    QString change_str = QString("%1:%2").arg(victim->objectName()).arg(-damage.damage);
    switch (damage.nature) {
    case DamageStruct::Fire:
        change_str.append("F");
        break;
    case DamageStruct::Thunder:
        change_str.append("T");
        break;
    default:
        break;
    }

    JsonArray arg;
    arg << victim->objectName() << -damage.damage << damage.nature;
    doBroadcastNotify(QSanProtocol::S_COMMAND_CHANGE_HP, arg);
}

void Room::recover(ServerPlayer *player, const RecoverStruct &recover, bool set_emotion)
{
    if (player->getLostHp() == 0 || player->isDead())
        return;
    RecoverStruct recover_struct = recover;
    recover_struct.to = player;

    QVariant data = QVariant::fromValue(recover_struct);
    if (thread->trigger(PreHpRecover, this, data))
        return;

    recover_struct = data.value<RecoverStruct>();
    int recover_num = recover_struct.recover;

    if (!player->hasSkill("banling")) {
        int new_hp = qMin(player->getHp() + recover_num, player->getMaxHp());
        setPlayerProperty(player, "hp", new_hp);
    }

    JsonArray arg;
    arg << player->objectName();
    arg << recover_num;
    arg << 0;
    doBroadcastNotify(S_COMMAND_CHANGE_HP, arg);

    if (set_emotion)
        setEmotion(player, "recover");

    thread->trigger(HpRecover, this, data);
}

bool Room::cardEffect(const Card *card, ServerPlayer *from, ServerPlayer *to, bool multiple)
{
    CardEffectStruct effect;
    effect.card = card;
    effect.from = from;
    effect.to = to;
    effect.multiple = multiple;
    return cardEffect(effect);
}

bool Room::cardEffect(const CardEffectStruct &effect)
{
    QVariant data = QVariant::fromValue(effect);
    bool cancel = false;

    if (effect.to->isAlive() || effect.card->isKindOf("Slash")) { // Be care!!!
        // No skills should be triggered here!
        thread->trigger(CardEffect, this, data);
        // Make sure that effectiveness of Slash isn't judged here!
        if (!thread->trigger(CardEffected, this, data)) {
            cancel = true;
        } else {
            if (!effect.to->hasFlag("Global_NonSkillNullify"))
                setEmotion(effect.to, "skill_nullify");
            else
                effect.to->setFlags("-Global_NonSkillNullify");
        }
    }
    thread->trigger(PostCardEffected, this, data);
    return cancel;
}

bool Room::isJinkEffected(SlashEffectStruct effect, const Card *jink)
{
    if (jink == nullptr)
        return false;
    Q_ASSERT(jink->isKindOf("Jink"));
    JinkEffectStruct j;
    j.jink = jink;
    j.slashEffect = effect;
    QVariant jink_data = QVariant::fromValue(j);
    return !thread->trigger(JinkEffect, this, jink_data);
}

void Room::damage(const DamageStruct &data)
{
    DamageStruct damage_data = data;
    if (damage_data.to == nullptr || damage_data.to->isDead())
        return;
    if (damage_data.to->hasSkill("huanmeng")) {
        LogMessage log2;
        log2.type = "#TriggerSkill";
        log2.from = damage_data.to;
        log2.arg = "huanmeng";
        sendLog(log2);
        notifySkillInvoked(damage_data.to, "huanmeng");
        if ((damage_data.card != nullptr) && damage_data.card->isKindOf("Slash"))
            damage_data.to->removeQinggangTag(damage_data.card);

        return;
    }
    if (damage_data.to->isRemoved())
        return;

    QVariant qdata = QVariant::fromValue(damage_data);

    if (!damage_data.chain && !damage_data.transfer) {
        thread->trigger(ConfirmDamage, this, qdata);
        damage_data = qdata.value<DamageStruct>();
    }

    // Predamage
    if (thread->trigger(Predamage, this, qdata)) {
        if ((damage_data.card != nullptr) && damage_data.card->isKindOf("Slash"))
            damage_data.to->removeQinggangTag(damage_data.card);
        return;
    }

    try {
        bool enter_stack = false;
        do {
            bool prevent = thread->trigger(DamageForseen, this, qdata);
            if (prevent) {
                if ((damage_data.card != nullptr) && damage_data.card->isKindOf("Slash"))
                    damage_data.to->removeQinggangTag(damage_data.card);

                break;
            }
            if (damage_data.from != nullptr) {
                if (thread->trigger(DamageCaused, this, qdata)) {
                    if ((damage_data.card != nullptr) && damage_data.card->isKindOf("Slash"))
                        damage_data.to->removeQinggangTag(damage_data.card);

                    break;
                }
            }

            damage_data = qdata.value<DamageStruct>();

            bool broken = thread->trigger(DamageInflicted, this, qdata);
            if (broken) {
                if ((damage_data.card != nullptr) && damage_data.card->isKindOf("Slash"))
                    damage_data.to->removeQinggangTag(damage_data.card);

                break;
            }
            enter_stack = true;
            m_damageStack.push_back(damage_data);
            setTag("CurrentDamageStruct", qdata);

            thread->trigger(PreDamageDone, this, qdata);

            if ((damage_data.card != nullptr) && damage_data.card->isKindOf("Slash"))
                damage_data.to->removeQinggangTag(damage_data.card);
            thread->trigger(DamageDone, this, qdata);

            if ((damage_data.from != nullptr) && !damage_data.from->hasFlag("Global_DebutFlag"))
                thread->trigger(Damage, this, qdata);

            if (!damage_data.to->hasFlag("Global_DebutFlag"))
                thread->trigger(Damaged, this, qdata);
        } while (false);

        if (!enter_stack)
            setTag("SkipGameRule", true);
        damage_data = qdata.value<DamageStruct>();
        thread->trigger(DamageComplete, this, qdata);

        if (enter_stack) {
            m_damageStack.pop();
            if (m_damageStack.isEmpty())
                removeTag("CurrentDamageStruct");
            else
                setTag("CurrentDamageStruct", QVariant::fromValue(m_damageStack.first()));
        }
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken) {
            removeTag("is_chained");
            removeTag("CurrentDamageStruct");
            m_damageStack.clear();
        }
        throw triggerEvent;
    }
}

void Room::sendDamageLog(const DamageStruct &data)
{
    LogMessage log;

    if (data.from != nullptr) {
        log.type = "#Damage";
        log.from = data.from;
    } else {
        log.type = "#DamageNoSource";
    }

    log.to << data.to;
    log.arg = QString::number(data.damage);

    switch (data.nature) {
    case DamageStruct::Normal:
        log.arg2 = "normal_nature";
        break;
    case DamageStruct::Fire:
        log.arg2 = "fire_nature";
        break;
    case DamageStruct::Thunder:
        log.arg2 = "thunder_nature";
        break;
    case DamageStruct::Ice:
        log.arg2 = "ice_nature";
        break;
    }

    sendLog(log);
}

bool Room::hasWelfare(const ServerPlayer *player) const
{
    if (mode == "06_3v3")
        return player->isLord() || player->getRole() == "renegade";
    else if (mode == "06_XMode" || isHegemonyGameMode(mode))
        return false;
    else if (mode == "03_1v2")
        return player->isLord();
    else
        return player->isLord() && player_count > 4;
}

ServerPlayer *Room::getFront(ServerPlayer *a, ServerPlayer *b) const
{
    QList<ServerPlayer *> players = getAllPlayers(true);
    int index_a = players.indexOf(a), index_b = players.indexOf(b);
    if (index_a < index_b)
        return a;
    else
        return b;
}

void Room::reconnect(ServerPlayer *player, ClientSocket *socket)
{
    player->setSocket(socket);
    player->setState("online");

    marshal(player);

    broadcastProperty(player, "state");
}

void Room::marshal(ServerPlayer *player)
{
    notifyProperty(player, player, "objectName");
    notifyProperty(player, player, "role");
    notifyProperty(player, player, "flags", "marshalling");

    foreach (ServerPlayer *p, m_players) {
        if (p != player)
            p->introduceTo(player);
    }

    QStringList player_circle;
    foreach (ServerPlayer *player, m_players)
        player_circle << player->objectName();

    doNotify(player, S_COMMAND_ARRANGE_SEATS, JsonUtils::toJsonArray(player_circle));
    doNotify(player, S_COMMAND_START_IN_X_SECONDS, QVariant(0));

    foreach (ServerPlayer *p, m_players) {
        if (isHegemonyGameMode(mode) && p == player && !p->hasShownGeneral()) {
            QString general1_name = tag[player->objectName()].toStringList().at(0);
            notifyProperty(player, p, "general", general1_name);
        } else
            notifyProperty(player, p, "general");

        if (p->getGeneral2() != nullptr) {
            if (isHegemonyGameMode(mode) && p == player && !p->hasShownGeneral2()) {
                QString general2_name = tag[player->objectName()].toStringList().at(1);
                notifyProperty(player, p, "general2", general2_name);
            } else {
                notifyProperty(player, p, "general2");
            }
        }
    }

    if (isHegemonyGameMode(mode)) {
        foreach (const Skill *skill, player->getVisibleSkillList()) {
            JsonArray args1;
            args1 << (int)S_GAME_EVENT_ADD_SKILL;
            args1 << player->objectName();
            args1 << skill->objectName();
            args1 << player->inHeadSkills(skill->objectName());

            doNotify(player, S_COMMAND_LOG_EVENT, args1);
        }

        player->notifyPreshow();
    }

    if (game_started) {
        ServerPlayer *lord = getLord();
        JsonArray lord_info;

        lord_info << (lord != nullptr ? lord->getGeneralName() : QVariant());
        doNotify(player, S_COMMAND_GAME_START, lord_info);

        QList<int> drawPile = Sanguosha->getRandomCards();
        doNotify(player, S_COMMAND_AVAILABLE_CARDS, JsonUtils::toJsonArray(drawPile));
    }

    foreach (ServerPlayer *p, m_players)
        p->marshal(player);

    notifyProperty(player, player, "flags", "-marshalling");
    if (game_started) {
        doNotify(player, S_COMMAND_UPDATE_PILE, QVariant(m_drawPile->length()));

        //disconnect wugu
        if (!m_fillAGarg.isNull() && (m_fillAGWho == nullptr || m_fillAGWho == player)) {
            doNotify(player, S_COMMAND_FILL_AMAZING_GRACE, m_fillAGarg);
            foreach (const QVariant &takeAGarg, m_takeAGargs.value<JsonArray>())
                doNotify(player, S_COMMAND_TAKE_AMAZING_GRACE, takeAGarg);
        }

        QVariant discard = JsonUtils::toJsonArray(*m_discardPile);
        doNotify(player, S_COMMAND_SYNCHRONIZE_DISCARD_PILE, discard);
    }
}

void Room::startGame()
{
    m_alivePlayers = m_players;
    for (int i = 0; i < player_count - 1; i++)
        m_players.at(i)->setNext(m_players.at(i + 1));
    m_players.last()->setNext(m_players.first());

    //step1 : player set  MaxHP and CompanionEffect
    foreach (ServerPlayer *player, m_players) {
        Q_ASSERT(player->getGeneral());
        if (isHegemonyGameMode(mode)) {
            QStringList generals = getTag(player->objectName()).toStringList();
            const General *general1 = Sanguosha->getGeneral(generals.first());
            Q_ASSERT(general1);
            int max_hp = general1->getMaxHp();
            //if (Config.Enable2ndGeneral) {
            const General *general2 = Sanguosha->getGeneral(generals.last());
            max_hp = (general1->getMaxHpHead() + general2->getMaxHpDeputy());
            if (general1->isCompanionWith(generals.last()))
                player->setMark("CompanionEffect", 1);

            player->setMark("HalfMaxHpLeft", max_hp % 2);
            max_hp = max_hp / 2;
            //}
            player->setMaxHp(max_hp);
        } else
            player->setMaxHp(player->getGeneralMaxHp());

        player->setHp(player->getMaxHp());
        // setup AI
        AI *ai = cloneAI(player);
        ais << ai;
        player->setAI(ai);
    }

    foreach (ServerPlayer *player, m_players) {
        if (mode == "06_3v3" || mode == "02_1v1" || mode == "06_XMode" || mode == "03_1v2"
            || (!isHegemonyGameMode(mode) && !player->isLord())) // hegemony has already notified "general"
            broadcastProperty(player, "general");

        if (mode == "02_1v1")
            doBroadcastNotify(getOtherPlayers(player, true), S_COMMAND_REVEAL_GENERAL, JsonArray() << player->objectName() << player->getGeneralName());

        if (Config.Enable2ndGeneral && mode != "02_1v1" && mode != "06_3v3" && mode != "06_XMode" && mode != "04_1v3" && !isHegemonyGameMode(mode))
            broadcastProperty(player, "general2");

        broadcastProperty(player, "hp");
        broadcastProperty(player, "maxhp");

        if (mode == "06_3v3" || mode == "06_XMode") {
            broadcastProperty(player, "role");
            setPlayerProperty(player, "role_shown", true);
        }

        if (!isHegemonyGameMode(mode)) {
            setPlayerProperty(player, "general_showed", true);
            setPlayerProperty(player, "general2_showed", true);
            if (player->isLord())
                setPlayerProperty(player, "role_shown", true);
        }
    }

    preparePlayers();

    QList<int> drawPile = *m_drawPile;
    qShuffle(drawPile);
    doBroadcastNotify(S_COMMAND_AVAILABLE_CARDS, JsonUtils::toJsonArray(drawPile));

    ServerPlayer *lord = getLord();
    JsonArray lord_info;
    lord_info << (lord != nullptr ? lord->getGeneralName() : QVariant());
    doBroadcastNotify(S_COMMAND_GAME_START, lord_info);

    game_started = true;

    Server *server = qobject_cast<Server *>(parent());
    foreach (ServerPlayer *player, m_players) {
        if (player->getState() == "online")
            server->signupPlayer(player);
    }

    current = m_players.first();

    // initialize the place_map and owner_map;

    foreach (int card_id, *m_drawPile)
        setCardMapping(card_id, nullptr, Player::DrawPile);
    doBroadcastNotify(S_COMMAND_UPDATE_PILE, QVariant(m_drawPile->length()));

    if (mode != "02_1v1" && mode != "06_3v3" && mode != "06_XMode")
        _m_roomState.reset();
}

bool Room::notifyProperty(ServerPlayer *playerToNotify, const ServerPlayer *propertyOwner, const char *propertyName, const QString &value)
{
    if (propertyOwner == nullptr)
        return false;
    QString real_value = value;
    if (real_value.isNull())
        real_value = propertyOwner->property(propertyName).toString();
    JsonArray arg;
    if (propertyOwner == playerToNotify)
        arg << QSanProtocol::S_PLAYER_SELF_REFERENCE_ID;
    else
        arg << propertyOwner->objectName();
    arg << propertyName;
    arg << real_value;
    return doNotify(playerToNotify, S_COMMAND_SET_PROPERTY, arg);
}

bool Room::broadcastProperty(ServerPlayer *player, const char *property_name, const QString &value)
{
    if (player == nullptr)
        return false;
    QString real_value = value;
    if (real_value.isNull())
        real_value = player->property(property_name).toString();

    JsonArray arg;
    arg << player->objectName() << property_name << real_value;
    return doBroadcastNotify(S_COMMAND_SET_PROPERTY, arg);
}

void Room::drawCards(ServerPlayer *player, int n, const QString &reason)
{
    QList<ServerPlayer *> players;
    players.append(player);
    drawCards(players, n, reason);
}

void Room::drawCards(QList<ServerPlayer *> players, int n, const QString &reason)
{
    QList<int> n_list;
    n_list.append(n);
    drawCards(players, n_list, reason);
}

void Room::drawCards(QList<ServerPlayer *> players, QList<int> n_list, const QString &reason)
{
    QList<CardsMoveStruct> moves;
    int index = -1, len = n_list.length();
    Q_ASSERT(len >= 1);
    foreach (ServerPlayer *player, players) {
        index++;
        if (!player->isAlive() && reason != "reform")
            continue;
        int n = n_list.at(qMin(index, len - 1));
        if (n <= 0)
            continue;

        QList<int> card_ids;
        card_ids = getNCards(n, false);

        CardsMoveStruct move;
        move.card_ids = card_ids;
        move.from = nullptr;
        move.to = player;
        move.to_place = Player::PlaceHand;
        move.reason = CardMoveReason(CardMoveReason::S_REASON_DRAW, player->objectName());
        move.reason.m_extraData = reason;

        moves.append(move);
    }
    moveCardsAtomic(moves, false);
}

void Room::throwCard(const Card *card, ServerPlayer *who, ServerPlayer *thrower, bool notifyLog)
{
    CardMoveReason reason;
    if (thrower == nullptr) { // && thrower != who
        reason.m_reason = CardMoveReason::S_REASON_THROW;
        reason.m_playerId = who != nullptr ? who->objectName() : QString();
    } else {
        reason.m_reason = CardMoveReason::S_REASON_DISMANTLE;
        reason.m_targetId = who != nullptr ? who->objectName() : QString();
        reason.m_playerId = thrower->objectName();
    }
    reason.m_skillName = card->getSkillName();
    throwCard(card, reason, who, thrower, notifyLog);
}

void Room::throwCard(const Card *card, const CardMoveReason &reason, ServerPlayer *who, ServerPlayer *thrower, bool notifyLog)
{
    if (card == nullptr)
        return;

    QList<int> to_discard;
    if (card->isVirtualCard())
        to_discard.append(card->getSubcards());
    else
        to_discard << card->getEffectiveId();

    if (notifyLog) {
        LogMessage log;
        if (who != nullptr) {
            if (thrower == nullptr) {
                log.type = "$DiscardCard";
                log.from = who;
            } else {
                log.type = "$DiscardCardByOther";
                log.from = thrower;
                log.to << who;
            }
        } else {
            log.type = "$EnterDiscardPile";
        }

        log.card_str = IntList2StringList(to_discard).join("+");
        sendLog(log);
    }

    QList<CardsMoveStruct> moves;
    if (who != nullptr) {
        CardsMoveStruct move(to_discard, who, nullptr, Player::PlaceUnknown, Player::DiscardPile, reason);
        moves.append(move);
        moveCardsAtomic(moves, true);
    } else {
        CardsMoveStruct move(to_discard, nullptr, Player::DiscardPile, reason);
        moves.append(move);
        moveCardsAtomic(moves, true);
    }
}

void Room::throwCard(int card_id, ServerPlayer *who, ServerPlayer *thrower, bool notifyLog)
{
    throwCard(Sanguosha->getCard(card_id), who, thrower, notifyLog);
}

RoomThread *Room::getThread() const
{
    return thread;
}

void Room::moveCardTo(const Card *card, ServerPlayer *dstPlayer, Player::Place dstPlace, bool forceMoveVisible)
{
    moveCardTo(card, dstPlayer, dstPlace, CardMoveReason(CardMoveReason::S_REASON_UNKNOWN, QString()), forceMoveVisible);
}

void Room::moveCardTo(const Card *card, ServerPlayer *dstPlayer, Player::Place dstPlace, const CardMoveReason &reason, bool forceMoveVisible)
{
    moveCardTo(card, nullptr, dstPlayer, dstPlace, QString(), reason, forceMoveVisible);
}

void Room::moveCardTo(const Card *card, ServerPlayer *srcPlayer, ServerPlayer *dstPlayer, Player::Place dstPlace, const CardMoveReason &reason, bool forceMoveVisible)
{
    moveCardTo(card, srcPlayer, dstPlayer, dstPlace, QString(), reason, forceMoveVisible);
}

void Room::moveCardTo(const Card *card, ServerPlayer *srcPlayer, ServerPlayer *dstPlayer, Player::Place dstPlace, const QString &pileName, const CardMoveReason &reason,
                      bool forceMoveVisible)
{
    CardsMoveStruct move;
    if (card->isVirtualCard()) {
        move.card_ids = card->getSubcards();
        if (move.card_ids.size() == 0)
            return;
    } else
        move.card_ids.append(card->getId());
    move.to = dstPlayer;
    move.to_place = dstPlace;
    move.to_pile_name = pileName;
    move.from = srcPlayer;
    move.reason = reason;
    QList<CardsMoveStruct> moves;
    moves.append(move);
    moveCardsAtomic(moves, forceMoveVisible);
}

void Room::_fillMoveInfo(CardsMoveStruct &moves, int card_index) const
{
    int card_id = moves.card_ids[card_index];
    if (moves.from == nullptr)
        moves.from = getCardOwner(card_id);
    moves.from_place = getCardPlace(card_id);
    if (moves.from != nullptr) { // Hand/Equip/Judge
        if (moves.from_place == Player::PlaceSpecial || moves.from_place == Player::PlaceTable)
            moves.from_pile_name = moves.from->getPileName(card_id);
        if (moves.from_player_name.isEmpty())
            moves.from_player_name = moves.from->objectName();
    }
    if (moves.to != nullptr) {
        if (moves.to_player_name.isEmpty())
            moves.to_player_name = moves.to->objectName();
        int card_id = moves.card_ids[card_index];
        if (moves.to_place == Player::PlaceSpecial || moves.to_place == Player::PlaceTable)
            moves.to_pile_name = moves.to->getPileName(card_id);
    }
}

QList<CardsMoveOneTimeStruct> Room::_mergeMoves(QList<CardsMoveStruct> cards_moves)
{
    QMap<_MoveMergeClassifier, QList<CardsMoveStruct>> moveMap;

    foreach (const CardsMoveStruct &cards_move, cards_moves) {
        _MoveMergeClassifier classifier(cards_move);
        moveMap[classifier].append(cards_move);
    }

    QList<CardsMoveOneTimeStruct> result;
    foreach (const _MoveMergeClassifier &cls, moveMap.keys()) {
        CardsMoveOneTimeStruct moveOneTime;
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
        foreach (const CardsMoveStruct &move, moveMap[cls]) {
            moveOneTime.card_ids.append(move.card_ids);
            moveOneTime.broken_ids.append(move.broken_ids);
            moveOneTime.shown_ids.append(move.shown_ids);
            for (int i = 0; i < move.card_ids.size(); i++) {
                moveOneTime.from_places.append(move.from_place);
                moveOneTime.origin_from_places.append(move.from_place); //move.origin_from_place??
                moveOneTime.from_pile_names.append(move.from_pile_name);
                moveOneTime.origin_from_pile_names.append(move.from_pile_name); //move.origin_from_pile_name??
                moveOneTime.open.append(move.open);
            }
            if (move.is_last_handcard)
                moveOneTime.is_last_handcard = true;
        }
        result.append(moveOneTime);
    }

    if (result.size() > 1) {
        std::sort(result.begin(), result.end(), [](CardsMoveOneTimeStruct move1, CardsMoveOneTimeStruct move2) -> bool {
            ServerPlayer *a = (ServerPlayer *)move1.from;
            if (a == nullptr)
                a = (ServerPlayer *)move1.to;
            ServerPlayer *b = (ServerPlayer *)move2.from;
            if (b == nullptr)
                b = (ServerPlayer *)move2.to;

            if (a == nullptr || b == nullptr)
                return a != nullptr;

            Room *room = a->getRoom();
            return room->getFront(a, b) == a;
        });
    }

    return result;
}

QList<CardsMoveStruct> Room::_separateMoves(QList<CardsMoveOneTimeStruct> moveOneTimes)
{
    QList<_MoveSeparateClassifier> classifiers;
    QList<QList<int>> ids;
    QList<int> broken_ids;
    QList<int> shown_ids;
    foreach (const CardsMoveOneTimeStruct &moveOneTime, moveOneTimes) {
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

    QList<CardsMoveStruct> card_moves;
    int i = 0;
    QHash<ServerPlayer *, QList<int>> from_handcards;
    foreach (const _MoveSeparateClassifier &cls, classifiers) {
        CardsMoveStruct card_move;
        ServerPlayer *from = (ServerPlayer *)cls.m_from;
        card_move.from = cls.m_from;
        if ((from != nullptr) && !from_handcards.contains(from))
            from_handcards[from] = from->handCards();
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

        if ((from != nullptr) && from_handcards.contains(from)) {
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
        std::sort(card_moves.begin(), card_moves.end(), [](CardsMoveStruct move1, CardsMoveStruct move2) -> bool {
            ServerPlayer *a = (ServerPlayer *)move1.from;
            if (a == nullptr)
                a = (ServerPlayer *)move1.to;
            ServerPlayer *b = (ServerPlayer *)move2.from;
            if (b == nullptr)
                b = (ServerPlayer *)move2.to;

            if (a == nullptr || b == nullptr)
                return a != nullptr;

            Room *room = a->getRoom();
            return room->getFront(a, b) == a;
        });
    }
    return card_moves;
}

void Room::moveCardsAtomic(CardsMoveStruct cards_move, bool forceMoveVisible)
{
    QList<CardsMoveStruct> cards_moves;
    cards_moves.append(cards_move);
    moveCardsAtomic(cards_moves, forceMoveVisible);
}

void Room::moveCardsAtomic(QList<CardsMoveStruct> cards_moves, bool forceMoveVisible)
{
    cards_moves = _breakDownCardMoves(cards_moves);

    QList<CardsMoveOneTimeStruct> moveOneTimes = _mergeMoves(cards_moves);
    int i = 0;
    foreach (CardsMoveOneTimeStruct moveOneTime, moveOneTimes) {
        if (moveOneTime.card_ids.size() == 0) {
            ++i;
            continue;
        }

        QVariant data = QVariant::fromValue(moveOneTime);
        thread->trigger(BeforeCardsMove, this, data);
        moveOneTime = data.value<CardsMoveOneTimeStruct>();
        moveOneTimes[i] = moveOneTime;
        i++;
    }
    cards_moves = _separateMoves(moveOneTimes);
    notifyMoveCards(true, cards_moves, forceMoveVisible);
    // First, process remove card
    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            int card_id = cards_move.card_ids[j];
            const Card *card = Sanguosha->getCard(card_id);

            if (cards_move.from != nullptr) // Hand/Equip/Judge
                cards_move.from->removeCard(card, cards_move.from_place);

            switch (cards_move.from_place) {
            case Player::DiscardPile:
                m_discardPile->removeOne(card_id);
                break;
            case Player::DrawPile:
                m_drawPile->removeOne(card_id);
                break;
            case Player::PlaceSpecial:
                table_cards.removeOne(card_id);
                break;
            default:
                break;
            }
        }
        if (cards_move.from_place == Player::DrawPile)
            doBroadcastNotify(S_COMMAND_UPDATE_PILE, QVariant(m_drawPile->length()));
    }

    foreach (const CardsMoveStruct &move, cards_moves)
        updateCardsOnLose(move);

    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            setCardMapping(cards_move.card_ids[j], (ServerPlayer *)cards_move.to, cards_move.to_place);
        }
    }
    foreach (const CardsMoveStruct &move, cards_moves)
        updateCardsOnGet(move);
    notifyMoveCards(false, cards_moves, forceMoveVisible);

    // Now, process add cards
    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            int card_id = cards_move.card_ids[j];
            const Card *card = Sanguosha->getCard(card_id);
            if (forceMoveVisible && cards_move.to_place == Player::PlaceHand)
                card->setFlags("visible");
            else
                card->setFlags("-visible");
            if (cards_move.to != nullptr) // Hand/Equip/Judge
                cards_move.to->addCard(card, cards_move.to_place);

            switch (cards_move.to_place) {
            case Player::DiscardPile:
                m_discardPile->prepend(card_id);
                break;
            case Player::DrawPile:
                m_drawPile->prepend(card_id);
                break;
            case Player::PlaceSpecial:
                table_cards.append(card_id);
                break;
            default:
                break;
            }
        }
    }

    //trigger event
    moveOneTimes = _mergeMoves(cards_moves);
    foreach (const CardsMoveOneTimeStruct &moveOneTime, moveOneTimes) {
        if (moveOneTime.card_ids.size() == 0)
            continue;
        QVariant data = QVariant::fromValue(moveOneTime);
        thread->trigger(CardsMoveOneTime, this, data);
    }
}

void Room::moveCardsToEndOfDrawpile(QList<int> card_ids, bool forceVisible)
{
    QList<CardsMoveStruct> moves;
    CardsMoveStruct move(card_ids, nullptr, Player::DrawPile, CardMoveReason(CardMoveReason::S_REASON_UNKNOWN, QString()));
    moves << move;

    QList<CardsMoveStruct> cards_moves = _breakDownCardMoves(moves);

    QList<CardsMoveOneTimeStruct> moveOneTimes = _mergeMoves(cards_moves);
    int i = 0;
    foreach (CardsMoveOneTimeStruct moveOneTime, moveOneTimes) {
        if (moveOneTime.card_ids.size() == 0) {
            ++i;
            continue;
        }
        QVariant data = QVariant::fromValue(moveOneTime);
        thread->trigger(BeforeCardsMove, this, data);
        moveOneTime = data.value<CardsMoveOneTimeStruct>();
        moveOneTimes[i] = moveOneTime;
        i++;
    }
    cards_moves = _separateMoves(moveOneTimes);

    notifyMoveCards(true, cards_moves, forceVisible);
    // First, process remove card
    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            int card_id = cards_move.card_ids[j];
            const Card *card = Sanguosha->getCard(card_id);

            if (cards_move.from != nullptr) // Hand/Equip/Judge
                cards_move.from->removeCard(card, cards_move.from_place);

            switch (cards_move.from_place) {
            case Player::DiscardPile:
                m_discardPile->removeOne(card_id);
                break;
            case Player::DrawPile:
                m_drawPile->removeOne(card_id);
                break;
            case Player::PlaceSpecial:
                table_cards.removeOne(card_id);
                break;
            default:
                break;
            }
        }
        if (cards_move.from_place == Player::DrawPile)
            doBroadcastNotify(S_COMMAND_UPDATE_PILE, QVariant(m_drawPile->length()));
    }

    foreach (const CardsMoveStruct &move, cards_moves)
        updateCardsOnLose(move);

    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            setCardMapping(cards_move.card_ids[j], (ServerPlayer *)cards_move.to, cards_move.to_place);
        }
    }
    foreach (const CardsMoveStruct &move, cards_moves)
        updateCardsOnGet(move);
    notifyMoveCards(false, cards_moves, forceVisible);
    // Now, process add cards
    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &cards_move = cards_moves[i];
        for (int j = 0; j < cards_move.card_ids.size(); j++) {
            int card_id = cards_move.card_ids[j];
            const Card *card = Sanguosha->getCard(card_id);
            card->setFlags("-visible");
            if (cards_move.to != nullptr) // Hand/Equip/Judge
                cards_move.to->addCard(card, cards_move.to_place);

            m_drawPile->append(card_id);
            doBroadcastNotify(S_COMMAND_UPDATE_PILE, QVariant(m_drawPile->length()));
        }
    }

    //trigger event
    moveOneTimes = _mergeMoves(cards_moves);
    foreach (CardsMoveOneTimeStruct moveOneTime, moveOneTimes) {
        if (moveOneTime.card_ids.size() == 0)
            continue;
        QVariant data = QVariant::fromValue(moveOneTime);
        thread->trigger(CardsMoveOneTime, this, data);
    }
}

QList<CardsMoveStruct> Room::_breakDownCardMoves(QList<CardsMoveStruct> &cards_moves)
{
    QList<CardsMoveStruct> all_sub_moves;
    for (int i = 0; i < cards_moves.size(); i++) {
        CardsMoveStruct &move = cards_moves[i];
        if (move.card_ids.size() == 0)
            continue;
        QMap<_MoveSourceClassifier, QList<int>> moveMap;
        // reassemble move sources
        for (int j = 0; j < move.card_ids.size(); j++) {
            _fillMoveInfo(move, j);
            _MoveSourceClassifier classifier(move);
            moveMap[classifier].append(move.card_ids[j]);
        }
        foreach (const _MoveSourceClassifier &cls, moveMap.keys()) {
            CardsMoveStruct sub_move = move;
            cls.copyTo(sub_move);
            if ((sub_move.from == sub_move.to && sub_move.from_place == sub_move.to_place) || sub_move.card_ids.size() == 0)
                continue;
            sub_move.card_ids = moveMap[cls];
            all_sub_moves.append(sub_move);
        }
    }
    return all_sub_moves;
}

void Room::updateCardsOnLose(const CardsMoveStruct &move)
{
    for (int i = 0; i < move.card_ids.size(); i++) {
        WrappedCard *card = qobject_cast<WrappedCard *>(getCard(move.card_ids[i]));
        if (card->isModified()) {
            if (move.to_place == Player::DiscardPile) {
                resetCard(move.card_ids[i]);
                broadcastResetCard(getPlayers(), move.card_ids[i]);
            }
        }
    }
}

void Room::updateCardsOnGet(const CardsMoveStruct &move)
{
    if (move.card_ids.isEmpty())
        return;
    ServerPlayer *player = (ServerPlayer *)move.from;
    if (player != nullptr && move.to_place == Player::PlaceDelayedTrick) {
        for (int i = 0; i < move.card_ids.size(); i++) {
            WrappedCard *card = qobject_cast<WrappedCard *>(getCard(move.card_ids[i]));
            const Card *engine_card = Sanguosha->getEngineCard(move.card_ids[i]);
            if (card->getSuit() != engine_card->getSuit() || card->getNumber() != engine_card->getNumber()) {
                Card *trick = Sanguosha->cloneCard(card->getRealCard());
                trick->setSuit(engine_card->getSuit());
                trick->setNumber(engine_card->getNumber());
                card->takeOver(trick);
                broadcastUpdateCard(getPlayers(), move.card_ids[i], card);
            }
        }
        return;
    }

    player = (ServerPlayer *)move.to;
    if (player != nullptr
        && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip || move.to_place == Player::PlaceJudge || move.to_place == Player::PlaceSpecial)) {
        QList<const Card *> cards;
        foreach (int cardId, move.card_ids)
            cards.append(getCard(cardId));
        filterCards(player, cards, true);
    }
}

bool Room::notifyMoveCards(bool isLostPhase, QList<CardsMoveStruct> cards_moves, bool forceVisible, QList<ServerPlayer *> players)
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
    foreach (ServerPlayer *player, players) {
        if (player->isOffline())
            continue;
        JsonArray arg;
        arg << moveId;
        for (int i = 0; i < cards_moves.size(); i++) {
            ServerPlayer *to = nullptr;
            foreach (ServerPlayer *player, m_players) {
                if (player->objectName() == cards_moves[i].to_player_name) {
                    to = player;
                    break;
                }
            }
            cards_moves[i].open = forceVisible
                || cards_moves[i].isRelevant(player)
                // forceVisible will override cards to be visible
                || cards_moves[i].to_place == Player::PlaceEquip || cards_moves[i].from_place == Player::PlaceEquip || cards_moves[i].to_place == Player::PlaceDelayedTrick
                || cards_moves[i].from_place == Player::PlaceDelayedTrick
                // only cards moved to hand/special can be invisible
                || cards_moves[i].from_place == Player::DiscardPile
                || cards_moves[i].to_place == Player::DiscardPile
                // any card from/to discard pile should be visible
                || cards_moves[i].from_place == Player::PlaceTable
                // any card from/to place table should be visible,except pindian
                || (cards_moves[i].to_place == Player::PlaceSpecial && (to != nullptr) && to->pileOpen(cards_moves[i].to_pile_name, player->objectName()))
                // pile open to specific players
                || player->hasFlag("Global_GongxinOperator");
            // the player put someone's cards to the drawpile

            arg << cards_moves[i].toVariant();
        }
        doNotify(player, isLostPhase ? S_COMMAND_LOSE_CARD : S_COMMAND_GET_CARD, arg);
    }
    return true;
}

void Room::notifySkillInvoked(ServerPlayer *player, const QString &skill_name)
{
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_SKILL_INVOKED;
    args << player->objectName();
    args << skill_name;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

void Room::broadcastSkillInvoke(const QString &skill_name, const QString &category)
{
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_PLAY_EFFECT;
    args << skill_name;
    args << category;
    args << -1;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

void Room::broadcastSkillInvoke(const QString &skill_name)
{
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_PLAY_EFFECT;
    args << skill_name;
    args << true;
    args << -1;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

void Room::broadcastSkillInvoke(const QString &skill_name, int type)
{
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_PLAY_EFFECT;
    args << skill_name;
    args << true;
    args << type;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

void Room::broadcastSkillInvoke(const QString &skill_name, bool isMale, int type)
{
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_PLAY_EFFECT;
    args << skill_name;
    args << isMale;
    args << type;
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

void Room::doLightbox(const QString &lightboxName, int duration)
{
    if (Config.AIDelay == 0)
        return;
    doAnimate(S_ANIMATE_LIGHTBOX, lightboxName, QString::number(duration));
    thread->delay(duration / 1.2);
}

void Room::doAnimate(QSanProtocol::AnimateType type, const QString &arg1, const QString &arg2, QList<ServerPlayer *> players)
{
    if (players.isEmpty())
        players = m_players;
    JsonArray arg;
    arg << (int)type;
    arg << arg1;
    arg << arg2;
    doBroadcastNotify(players, S_COMMAND_ANIMATE, arg);
}

void Room::doBattleArrayAnimate(ServerPlayer *player, ServerPlayer *target)
{
    if (getAlivePlayers().length() < 4)
        return;
    if (target == nullptr) {
        QStringList names;
        foreach (const Player *p, player->getFormation())
            names << p->objectName();
        if (names.length() > 1)
            doAnimate(QSanProtocol::S_ANIMATE_BATTLEARRAY, player->objectName(), names.join("+"));
    } else {
        foreach (ServerPlayer *p, getOtherPlayers(player))
            if (p->inSiegeRelation(player, target))
                doAnimate(QSanProtocol::S_ANIMATE_BATTLEARRAY, player->objectName(), QString("%1+%2").arg(p->objectName(), player->objectName()));
    }
}

void Room::preparePlayers()
{
    if (isHegemonyGameMode(mode)) {
        foreach (ServerPlayer *player, m_players) {
            QString general1_name = tag[player->objectName()].toStringList().at(0);
            QList<const Skill *> skills = Sanguosha->getGeneral(general1_name)->getSkillList(true, true);
            foreach (const Skill *skill, skills)
                player->addSkill(skill->objectName());

            QString general2_name = tag[player->objectName()].toStringList().at(1);
            QList<const Skill *> skills2 = Sanguosha->getGeneral(general2_name)->getSkillList(true, false);
            foreach (const Skill *skill, skills2)
                player->addSkill(skill->objectName(), false);

            JsonArray args;
            args << (int)QSanProtocol::S_GAME_EVENT_PREPARE_SKILL; //(int)QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            doNotify(player, QSanProtocol::S_COMMAND_LOG_EVENT, args);

            notifyProperty(player, player, "flags", "AutoPreshowAvailable");
            player->notifyPreshow();
            notifyProperty(player, player, "flags", "-AutoPreshowAvailable");
        }

    } else {
        foreach (ServerPlayer *player, m_players) {
            QList<const Skill *> skills = player->getGeneral()->getSkillList();
            foreach (const Skill *skill, skills)
                player->addSkill(skill->objectName());
            if (player->getGeneral2() != nullptr) {
                skills = player->getGeneral2()->getSkillList();
                foreach (const Skill *skill, skills)
                    player->addSkill(skill->objectName(), false);
            }
            player->setGender(player->getGeneral()->getGender());
        }

        JsonArray args;
        args << (int)QSanProtocol::S_GAME_EVENT_PREPARE_SKILL;
        doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
    }
}

void Room::changePlayerGeneral(ServerPlayer *player, const QString &new_general)
{
    QString originalName = player->tag.value("init_general", QString()).toString();
    if (originalName != nullptr)
        player->tag["init_general"] = player->getGeneralName();

    if (!isHegemonyGameMode(mode) && player->getGeneral() != nullptr) {
        foreach (const Skill *skill, player->getGeneral()->getSkillList(true, true))
            player->loseSkill(skill->objectName());
    }
    player->setProperty("general", new_general);
    QList<ServerPlayer *> players = m_players;
    if (new_general == "anjiang")
        players.removeOne(player);
    foreach (ServerPlayer *p, players)
        notifyProperty(p, player, "general");
    //setPlayerProperty(player, "general", new_general); //
    Q_ASSERT(player->getGeneral() != nullptr);
    if (new_general != "anjiang")
        player->setGender(player->getGeneral()->getGender());
    if (!isHegemonyGameMode(mode)) {
        foreach (const Skill *skill, player->getGeneral()->getSkillList(true, true)) {
            if (skill->isLordSkill() && !player->isLord()) {
                continue;
            }
            player->addSkill(skill->objectName());
        }
    }

    filterCards(player, player->getCards("hes"), true);
}

void Room::changePlayerGeneral2(ServerPlayer *player, const QString &new_general)
{
    QString originalName2 = player->tag.value("init_general2", QString()).toString();
    if (originalName2 != nullptr)
        player->tag["init_general2"] = player->getGeneral2Name();

    if (!isHegemonyGameMode(mode) && player->getGeneral2() != nullptr) {
        foreach (const Skill *skill, player->getGeneral2()->getSkillList(true, false))
            player->loseSkill(skill->objectName());
    }
    player->setProperty("general2", new_general);
    QList<ServerPlayer *> players = m_players;
    if (new_general == "anjiang")
        players.removeOne(player);
    foreach (ServerPlayer *p, players)
        notifyProperty(p, player, "general2");
    //setPlayerProperty(player, "general2", new_general);
    Q_ASSERT(player->getGeneral2() != nullptr);
    if (!isHegemonyGameMode(mode) && (player->getGeneral2() != nullptr)) {
        foreach (const Skill *skill, player->getGeneral2()->getSkillList(true, false)) {
            if (skill->isLordSkill() && !player->isLord())
                continue;
            player->addSkill(skill->objectName(), false);
        }
    }
    filterCards(player, player->getCards("hes"), true);
}

void Room::filterCards(ServerPlayer *player, QList<const Card *> cards, bool refilter)
{
    if (refilter) {
        for (int i = 0; i < cards.size(); i++) {
            WrappedCard *card = qobject_cast<WrappedCard *>(getCard(cards[i]->getId()));
            if (card->isModified()) {
                int cardId = card->getId();
                resetCard(cardId);
                if (getCardPlace(cardId) != Player::PlaceHand || player->isShownHandcard(cardId))
                    broadcastResetCard(m_players, cardId);
                else
                    notifyResetCard(player, cardId);
            }
        }
    }

    bool *cardChanged = new bool[cards.size()];
    for (int i = 0; i < cards.size(); i++)
        cardChanged[i] = false;

    QSet<const Skill *> skills = player->getSkills(false, false);
    QList<const FilterSkill *> filterSkills;

    foreach (const Skill *skill, skills) {
        if (player->hasSkill(skill->objectName()) && skill->inherits("FilterSkill")) {
            const FilterSkill *filter = qobject_cast<const FilterSkill *>(skill);
            Q_ASSERT(filter);
            filterSkills.append(filter);
        }
    }
    if (filterSkills.size() == 0) {
        delete[] cardChanged;
        return;
    }

    for (int i = 0; i < cards.size(); i++) {
        const Card *card = cards[i];
        for (int fTime = 0; fTime < filterSkills.size(); fTime++) {
            bool converged = true;
            foreach (const FilterSkill *skill, filterSkills) {
                Q_ASSERT(skill);
                if (skill->viewFilter(cards[i])) {
                    cards[i] = skill->viewAs(card);
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
        int cardId = cards[i]->getId();
        Player::Place place = getCardPlace(cardId);
        if (!cardChanged[i])
            continue;
        if (place == Player::PlaceHand && !player->isShownHandcard(cardId))
            notifyUpdateCard(player, cardId, cards[i]);
        else {
            broadcastUpdateCard(getPlayers(), cardId, cards[i]);
            if (place == Player::PlaceJudge) {
                LogMessage log;
                log.type = "#FilterJudge";
                log.arg = cards[i]->getSkillName();
                log.from = player;

                sendLog(log);
                notifySkillInvoked(player, cards[i]->getSkillName());
                broadcastSkillInvoke(cards[i]->getSkillName());
            }
        }
    }

    delete[] cardChanged;
}

void Room::acquireSkill(ServerPlayer *player, const Skill *skill, bool open, bool head)
{
    QString skill_name = skill->objectName();
    if (player->getAcquiredSkills().contains(skill_name))
        return;
    player->acquireSkill(skill_name);

    if (skill->inherits("TriggerSkill")) {
        const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
        thread->addTriggerSkill(trigger_skill);
    }
    if (skill->getFrequency() == Skill::Limited && !skill->getLimitMark().isEmpty())
        setPlayerMark(player, skill->getLimitMark(), 1);

    if (skill->isVisible()) {
        if (open) {
            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_ACQUIRE_SKILL;
            args << player->objectName();
            args << skill_name;
            args << head;
            doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        }

        foreach (const Skill *related_skill, Sanguosha->getRelatedSkills(skill_name)) {
            if (!related_skill->isVisible())
                acquireSkill(player, related_skill);
        }

        SkillAcquireDetachStruct s;
        s.isAcquire = true;
        s.player = player;
        s.skill = skill;
        QVariant data = QVariant::fromValue(s);
        thread->trigger(EventAcquireSkill, this, data);
    }
}

void Room::acquireSkill(ServerPlayer *player, const QString &skill_name, bool open, bool head)
{
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (skill != nullptr)
        acquireSkill(player, skill, open, head);
}

void Room::setTag(const QString &key, const QVariant &value)
{
    tag.insert(key, value);
}

QVariant Room::getTag(const QString &key) const
{
    return tag.value(key);
}

void Room::removeTag(const QString &key)
{
    tag.remove(key);
}

QStringList Room::getTagNames() const
{
    return tag.keys();
}

void Room::setEmotion(ServerPlayer *target, const QString &emotion)
{
    JsonArray arg;
    arg << target->objectName();
    arg << (emotion.isEmpty() ? QString(".") : emotion);
    doBroadcastNotify(S_COMMAND_SET_EMOTION, arg);
}

void Room::activate(ServerPlayer *player, CardUseStruct &card_use)
{
    tryPause();

    if (player->hasFlag("Global_PlayPhaseTerminated") || player->hasFlag("Global_TurnTerminated")) {
        setPlayerFlag(player, "-Global_PlayPhaseTerminated");
        //for "dangjia" intention ai
        //need record  PlayPhaseTerminated
        setPlayerFlag(player, "PlayPhaseTerminated");
        card_use.card = nullptr;
        return;
    }

    notifyMoveFocus(player, S_COMMAND_PLAY_CARD);

    _m_roomState.setCurrentCardUsePattern(QString());
    _m_roomState.setCurrentCardUseReason(CardUseStruct::CARD_USE_REASON_PLAY);

    AI *ai = player->getAI();
    if (ai != nullptr) {
        QElapsedTimer timer;
        timer.start();

        card_use.from = player;
        ai->activate(card_use);
        if ((card_use.card != nullptr) && !card_use.card->isVirtualCard())
            card_use.card = card_use.card->getRealCard();

        qint64 diff = Config.AIDelay - timer.elapsed();
        if (diff > 0)
            thread->delay(diff);
    } else if (player->getPhase() != Player::Play) {
        return;
    } else {
        bool success = doRequest(player, S_COMMAND_PLAY_CARD, player->objectName(), true);
        const QVariant &clientReply = player->getClientReply();

        if (!success || clientReply.isNull())
            return;

        card_use.from = player;
        if (!card_use.tryParse(clientReply, this)) {
            JsonArray use = clientReply.value<JsonArray>();
            emit room_message(tr("Card cannot be parsed:\n %1").arg(use[0].toString()));
            return;
        }
    }
    card_use.m_reason = CardUseStruct::CARD_USE_REASON_PLAY;
    if (!card_use.isValid(QString()))
        return;

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::Activate;
    s.args << card_use.toString();
    QVariant data = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, data);
}

void Room::askForLuckCard()
{
    tryPause();

    QList<ServerPlayer *> players;
    foreach (ServerPlayer *player, m_players) {
        if (player->getAI() == nullptr) {
            player->m_commandArgs = QVariant();
            players << player;
        }
    }

    int n = 0;
    while (n < Config.LuckCardLimitation) {
        if (players.isEmpty())
            return;

        n++;

        Countdown countdown;
        countdown.max = ServerInfo.getCommandTimeout(S_COMMAND_LUCK_CARD, S_CLIENT_INSTANCE);
        countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
        notifyMoveFocus(players, S_COMMAND_LUCK_CARD, countdown);

        doBroadcastRequest(players, S_COMMAND_LUCK_CARD);

        QList<ServerPlayer *> used;
        foreach (ServerPlayer *player, players) {
            const QVariant &clientReply = player->getClientReply();
            if (!player->m_isClientResponseReady || !JsonUtils::isBool(clientReply) || !clientReply.toBool())
                continue;
            used << player;
        }
        if (used.isEmpty())
            return;

        LogMessage log;
        log.type = "#UseLuckCard";
        foreach (ServerPlayer *player, used) {
            log.from = player;
            sendLog(log);
        }

        QList<int> draw_list;
        foreach (ServerPlayer *player, used) {
            draw_list << player->getHandcardNum();

            CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), "luck_card", QString());
            QList<CardsMoveStruct> moves;
            CardsMoveStruct move;
            move.from = player;
            move.from_place = Player::PlaceHand;
            move.to = nullptr;
            move.to_place = Player::DrawPile;
            move.card_ids = player->handCards();
            move.reason = reason;
            moves.append(move);
            moves = _breakDownCardMoves(moves);

            QList<ServerPlayer *> tmp_list;
            tmp_list.append(player);

            notifyMoveCards(true, moves, false, tmp_list);
            for (int j = 0; j < move.card_ids.size(); j++) {
                int card_id = move.card_ids[j];
                const Card *card = Sanguosha->getCard(card_id);
                player->removeCard(card, Player::PlaceHand);
            }

            updateCardsOnLose(move);
            for (int j = 0; j < move.card_ids.size(); j++)
                setCardMapping(move.card_ids[j], nullptr, Player::DrawPile);
            updateCardsOnGet(move);

            notifyMoveCards(false, moves, false, tmp_list);
            for (int j = 0; j < move.card_ids.size(); j++) {
                int card_id = move.card_ids[j];
                m_drawPile->prepend(card_id);
            }
        }
        qShuffle(*m_drawPile);
        int index = -1;
        foreach (ServerPlayer *player, used) {
            index++;
            QList<CardsMoveStruct> moves;
            CardsMoveStruct move;
            move.from = nullptr;
            move.from_place = Player::DrawPile;
            move.to = player;
            move.to_place = Player::PlaceHand;
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
                setCardMapping(move.card_ids[j], player, Player::PlaceHand);
            updateCardsOnGet(move);

            notifyMoveCards(false, moves, false);
            for (int j = 0; j < move.card_ids.size(); j++) {
                int card_id = move.card_ids[j];
                const Card *card = Sanguosha->getCard(card_id);
                player->addCard(card, Player::PlaceHand);
            }
        }
        doBroadcastNotify(S_COMMAND_UPDATE_PILE, QVariant(m_drawPile->length()));
    }
}

Card::Suit Room::askForSuit(ServerPlayer *player, const QString &reason)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_SUIT);

    Card::Suit suit = Card::AllSuits[qrand() % 4];

    AI *ai = player->getAI();
    if (ai != nullptr)
        suit = ai->askForSuit(reason);
    else {
        bool success = doRequest(player, S_COMMAND_CHOOSE_SUIT, JsonArray() << reason << player->objectName(), true);

        if (success) {
            QVariant clientReply = player->getClientReply();
            QString suitStr = clientReply.toString();
            if (suitStr == "spade")
                suit = Card::Spade;
            else if (suitStr == "club")
                suit = Card::Club;
            else if (suitStr == "heart")
                suit = Card::Heart;
            else if (suitStr == "diamond")
                suit = Card::Diamond;
        }
    }

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::Suit;
    s.args << reason << Card::Suit2String(suit);
    QVariant d = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, d);

    return suit;
}

QString Room::askForKingdom(ServerPlayer *player)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_KINGDOM);

    QString kingdomChoice = "";

    AI *ai = player->getAI();
    if (ai != nullptr)
        kingdomChoice = ai->askForKingdom();
    else {
        JsonArray arg;
        arg << player->getGeneral()->getKingdom();

        bool success = doRequest(player, S_COMMAND_CHOOSE_KINGDOM, arg, true);
        QVariant clientReply = player->getClientReply();
        if (success && JsonUtils::isString(clientReply)) {
            QString kingdom = clientReply.toString();
            if (Sanguosha->getKingdoms().contains(kingdom))
                kingdomChoice = kingdom;
        }

        //set default
        if (kingdomChoice == "")
            kingdomChoice = "wai";
    }

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::Kingdom;
    s.args << kingdomChoice;
    QVariant decisionData = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, decisionData);

    return kingdomChoice;
}

bool Room::askForDiscard(ServerPlayer *player, const QString &reason, int discard_num, int min_num, bool optional, bool include_equip, const QString &prompt)
{
    if (!player->isAlive())
        return false;
    tryPause();
    notifyMoveFocus(player, S_COMMAND_DISCARD_CARD);

    if (!optional) {
        DummyCard *dummy = new DummyCard;
        dummy->deleteLater();
        QList<int> jilei_list;
        QList<const Card *> handcards = player->getHandcards();
        foreach (const Card *card, handcards) {
            if (!player->isJilei(card))
                dummy->addSubcard(card);
            else
                jilei_list << card->getId();
        }
        if (include_equip) {
            QList<const Card *> equips = player->getEquips();
            foreach (const Card *card, equips) {
                if (!player->isJilei(card))
                    dummy->addSubcard(card);
            }
        }

        int card_num = dummy->subcardsLength();
        if (card_num <= min_num) {
            if (card_num > 0) {
                CardMoveReason movereason;
                movereason.m_playerId = player->objectName();
                movereason.m_skillName = dummy->getSkillName();
                if (reason == "gamerule")
                    movereason.m_reason = CardMoveReason::S_REASON_RULEDISCARD;
                else
                    movereason.m_reason = CardMoveReason::S_REASON_THROW;

                throwCard(dummy, movereason, player);

                ChoiceMadeStruct s;
                s.player = player;
                s.type = ChoiceMadeStruct::CardDiscard;
                s.args << dummy->toString();
                QVariant data = QVariant::fromValue(s);
                thread->trigger(ChoiceMade, this, data);
            }

            if (card_num < min_num && !jilei_list.isEmpty()) {
                doJileiShow(player, jilei_list);
                return false;
            }
            return true;
        }
    }

    AI *ai = player->getAI();
    QList<int> to_discard;
    if (ai != nullptr) {
        to_discard = ai->askForDiscard(reason, discard_num, min_num, optional, include_equip);
    } else {
        JsonArray ask_str;
        ask_str << discard_num;
        ask_str << min_num;
        ask_str << optional;
        ask_str << include_equip;
        ask_str << prompt;
        ask_str << reason;
        bool success = doRequest(player, S_COMMAND_DISCARD_CARD, ask_str, true);
        //@todo: also check if the player does have that card!!!
        JsonArray clientReply = player->getClientReply().value<JsonArray>();
        if (!success || ((int)clientReply.size() > discard_num || (int)clientReply.size() < min_num) || !JsonUtils::tryParse(clientReply, to_discard)) {
            if (optional)
                return false;
            // time is up, and the server choose the cards to discard
            to_discard = player->forceToDiscard(discard_num, include_equip);
        }
    }

    if (to_discard.isEmpty())
        return false;

    DummyCard *dummy_card = new DummyCard(to_discard);
    if (reason == "gamerule") {
        CardMoveReason reason(CardMoveReason::S_REASON_RULEDISCARD, player->objectName(), QString(), dummy_card->getSkillName(), QString());
        throwCard(dummy_card, reason, player);
    } else {
        CardMoveReason reason(CardMoveReason::S_REASON_THROW, player->objectName(), QString(), dummy_card->getSkillName(), QString());
        throwCard(dummy_card, reason, player);
    }

    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::CardDiscard;
    s.args << dummy_card->toString();
    QVariant data = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, data);

    dummy_card->deleteLater();

    return true;
}

void Room::doJileiShow(ServerPlayer *player, QList<int> jilei_ids)
{
    JsonArray gongxinArgs;
    gongxinArgs << player->objectName();
    gongxinArgs << false;
    gongxinArgs << JsonUtils::toJsonArray(jilei_ids);

    foreach (int cardId, jilei_ids) {
        WrappedCard *card = Sanguosha->getWrappedCard(cardId);
        if (card->isModified())
            broadcastUpdateCard(getOtherPlayers(player), cardId, card);
        else
            broadcastResetCard(getOtherPlayers(player), cardId);
    }

    LogMessage log;
    log.type = "$JileiShowAllCards";
    log.from = player;

    foreach (int card_id, jilei_ids)
        Sanguosha->getCard(card_id)->setFlags("visible");
    log.card_str = IntList2StringList(jilei_ids).join("+");
    sendLog(log);

    doBroadcastNotify(S_COMMAND_SHOW_ALL_CARDS, gongxinArgs);
}

const Card *Room::askForExchange(ServerPlayer *player, const QString &reason, int discard_num, int min_num, bool include_equip, const QString &prompt, bool optional)
{
    if (!player->isAlive())
        return nullptr;
    tryPause();
    notifyMoveFocus(player, S_COMMAND_EXCHANGE_CARD);

    AI *ai = player->getAI();
    QList<int> to_exchange;
    if (ai != nullptr) {
        // share the same callback interface
        player->setFlags("Global_AIDiscardExchanging");
        try {
            to_exchange = ai->askForDiscard(reason, discard_num, min_num, optional, include_equip);
            player->setFlags("-Global_AIDiscardExchanging");
        } catch (TriggerEvent triggerEvent) {
            if (triggerEvent == TurnBroken)
                player->setFlags("-Global_AIDiscardExchanging");
            throw triggerEvent;
        }
    } else {
        JsonArray exchange_str;
        exchange_str << discard_num;
        exchange_str << min_num;
        exchange_str << include_equip;
        exchange_str << prompt;
        exchange_str << optional;
        exchange_str << reason;

        bool success = doRequest(player, S_COMMAND_EXCHANGE_CARD, exchange_str, true);
        //@todo: also check if the player does have that card!!!
        JsonArray clientReply = player->getClientReply().value<JsonArray>();
        if (!success || clientReply.size() > discard_num || clientReply.size() < min_num || !JsonUtils::tryParse(clientReply, to_exchange)) {
            if (optional)
                return nullptr;
            to_exchange = player->forceToDiscard(discard_num, include_equip, false);
        }
    }

    if (to_exchange.isEmpty())
        return nullptr;

    DummyCard *card = new DummyCard(to_exchange);
    ChoiceMadeStruct s;
    s.player = player;
    s.type = ChoiceMadeStruct::CardExchange;
    s.args << reason << card->toString();
    QVariant data = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, data);
    return card;
}

void Room::setCardMapping(int card_id, ServerPlayer *owner, Player::Place place)
{
    owner_map.insert(card_id, owner);
    place_map.insert(card_id, place);
}

ServerPlayer *Room::getCardOwner(int card_id) const
{
    return owner_map.value(card_id);
}

Player::Place Room::getCardPlace(int card_id) const
{
    if (card_id < 0)
        return Player::PlaceUnknown;
    return place_map.value(card_id);
}

QList<int> Room::getCardIdsOnTable(const Card *virtual_card) const
{
    if (virtual_card == nullptr)
        return QList<int>();
    if (!virtual_card->isVirtualCard()) {
        QList<int> ids;
        ids << virtual_card->getEffectiveId();
        return getCardIdsOnTable(ids);
    } else {
        return getCardIdsOnTable(virtual_card->getSubcards());
    }
    return QList<int>();
}

QList<int> Room::getCardIdsOnTable(const QList<int> &card_ids) const
{
    QList<int> r;
    foreach (int id, card_ids) {
        if (getCardPlace(id) == Player::PlaceTable)
            r << id;
    }
    return r;
}

ServerPlayer *Room::getLord(const QString &, bool) const
{
    if (isHegemonyGameMode(mode))
        return nullptr;
    ServerPlayer *the_lord = m_players.first();
    if (the_lord->getRole() == "lord")
        return the_lord;

    foreach (ServerPlayer *player, m_players) {
        if (player->getRole() == "lord")
            return player;
    }

    return nullptr;
}

void Room::askForGuanxing(ServerPlayer *zhuge, const QList<int> &cards, GuanxingType guanxing_type, QString skillName)
{
    // ATTENTION: DO REMOVE THE CARD FROM getDrawPile (But keep their place in DrawPile, via getNCards or something) BEFORE USING THIS FUNCTION!
    // Else duplicated card ids will appear in the bottom of draw pile, which will cause card stuck when accessed, and (maybe other) subsequent Client / Server misbehave

    QList<int> top_cards, bottom_cards;
    tryPause();
    notifyMoveFocus(zhuge, S_COMMAND_SKILL_GUANXING);

    AI *ai = zhuge->getAI();
    if (guanxing_type == GuanxingUpOnly && cards.length() == 1) {
        top_cards = cards;
    } else if (guanxing_type == GuanxingDownOnly && cards.length() == 1) {
        bottom_cards = cards;
    } else if (ai != nullptr) {
        ai->askForGuanxing(cards, top_cards, bottom_cards, (int)guanxing_type);

        // I'm too lazy to deal with this AI so....
        if (skillName == "fengshui") {
            top_cards << bottom_cards;
            bottom_cards = {top_cards.takeLast()};
        }
    } else {
        JsonArray guanxingArgs;
        guanxingArgs << JsonUtils::toJsonArray(cards);
        guanxingArgs << (guanxing_type != GuanxingBothSides);
        guanxingArgs << skillName;
        bool success = doRequest(zhuge, S_COMMAND_SKILL_GUANXING, guanxingArgs, true);
        if (!success) {
            foreach (int card_id, cards) {
                if (guanxing_type == GuanxingDownOnly)
                    m_drawPile->append(card_id);
                else
                    m_drawPile->prepend(card_id);
            }
        }
        JsonArray clientReply = zhuge->getClientReply().value<JsonArray>();
        if (clientReply.size() == 2) {
            success &= JsonUtils::tryParse(clientReply[0], top_cards);
            success &= JsonUtils::tryParse(clientReply[1], bottom_cards);
            if (guanxing_type == GuanxingDownOnly) {
                bottom_cards = top_cards;
                top_cards.clear();
            }
        }
    }

    bool length_equal = top_cards.length() + bottom_cards.length() == cards.length();
    bool result_equal = top_cards.toSet() + bottom_cards.toSet() == cards.toSet();
    if (!length_equal || !result_equal) {
        if (skillName == "fengshui") {
            top_cards = {cards.first()};
            bottom_cards = {cards.last()};
        } else if (guanxing_type == GuanxingDownOnly) {
            bottom_cards = cards;
            top_cards.clear();
        } else {
            top_cards = cards;
            bottom_cards.clear();
        }
    }

    if (guanxing_type == GuanxingBothSides) {
        LogMessage log;
        log.type = "#GuanxingResult";
        if (skillName == "fengshui")
            log.type = "#fengshuiResult";
        else if (skillName == "bolan")
            log.type = "#bolanResult";
        log.from = zhuge;
        log.arg = QString::number(top_cards.length());
        log.arg2 = QString::number(bottom_cards.length());
        sendLog(log);
    }

    if (!top_cards.isEmpty()) {
        LogMessage log;
        log.type = "$GuanxingTop";
        log.from = zhuge;
        log.card_str = IntList2StringList(top_cards).join("+");
        doNotify(zhuge, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
    }
    if (!bottom_cards.isEmpty()) {
        LogMessage log;
        log.type = "$GuanxingBottom";
        log.from = zhuge;
        log.card_str = IntList2StringList(bottom_cards).join("+");
        doNotify(zhuge, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
    }

    QListIterator<int> i(top_cards);
    i.toBack();
    while (i.hasPrevious())
        m_drawPile->prepend(i.previous());

    i = bottom_cards;
    while (i.hasNext())
        m_drawPile->append(i.next());

    doBroadcastNotify(S_COMMAND_UPDATE_PILE, QVariant(m_drawPile->length()));
    QVariant v = QVariant::fromValue(zhuge);
    thread->trigger(AfterGuanXing, this, v);
}

int Room::doGongxin(ServerPlayer *shenlvmeng, ServerPlayer *target, QList<int> enabled_ids, QString skill_name, bool cancellable)
{
    Q_ASSERT(!target->isKongcheng());
    tryPause();
    notifyMoveFocus(shenlvmeng, S_COMMAND_SKILL_GONGXIN);

    LogMessage log;
    log.type = "$ViewAllCards";
    log.from = shenlvmeng;
    log.to << target;
    log.card_str = IntList2StringList(target->handCards()).join("+");
    doNotify(shenlvmeng, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());

    ChoiceMadeStruct s;
    s.player = shenlvmeng;
    s.type = ChoiceMadeStruct::ViewCards;
    s.args << shenlvmeng->objectName() << target->objectName();
    QVariant decisionData = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, decisionData);

    shenlvmeng->tag[skill_name] = QVariant::fromValue((ServerPlayer *)target);
    int card_id = 0;
    AI *ai = shenlvmeng->getAI();
    if (ai != nullptr) {
        if (enabled_ids.isEmpty()) {
            if (cancellable)
                shenlvmeng->tag.remove(skill_name);
            card_id = -1;
        } else {
            card_id = ai->askForAG(enabled_ids, true, objectName());
            if (card_id == -1 && cancellable)
                shenlvmeng->tag.remove(skill_name);
        }
    } else {
        foreach (int cardId, target->handCards()) {
            WrappedCard *card = Sanguosha->getWrappedCard(cardId);
            if (card->isModified())
                notifyUpdateCard(shenlvmeng, cardId, card);
            else
                notifyResetCard(shenlvmeng, cardId);
        }

        JsonArray gongxinArgs;
        gongxinArgs << target->objectName();
        gongxinArgs << true;
        gongxinArgs << JsonUtils::toJsonArray(target->handCards());
        gongxinArgs << JsonUtils::toJsonArray(enabled_ids);
        gongxinArgs << skill_name;
        gongxinArgs << cancellable;
        bool success = doRequest(shenlvmeng, S_COMMAND_SKILL_GONGXIN, gongxinArgs, true);
        const QVariant &clientReply = shenlvmeng->getClientReply();
        if (!success || !JsonUtils::isNumber(clientReply) || !target->handCards().contains(clientReply.toInt())) {
            if (cancellable)
                shenlvmeng->tag.remove(skill_name);
            card_id = -1;
        } else
            card_id = clientReply.toInt();
    }

    if (card_id == -1 && !cancellable && !enabled_ids.isEmpty())
        card_id = enabled_ids.first();

    return card_id; // Do remember to remove the tag later!
}

const Card *Room::askForPindian(ServerPlayer *player, ServerPlayer *from, ServerPlayer *to, const QString &reason, PindianStruct *pindian)
{
    if (!from->isAlive() || !to->isAlive() || player->isKongcheng())
        return nullptr;
    Q_ASSERT(!player->isKongcheng());

    pindian->askedPlayer = player;
    QVariant pdata = QVariant::fromValue(pindian);
    getThread()->trigger(PindianAsked, this, pdata);
    pindian = pdata.value<PindianStruct *>();

    if (player == from && pindian->from_card != nullptr)
        return pindian->from_card;
    if (player == to && pindian->to_card != nullptr)
        return pindian->to_card;

    tryPause();
    notifyMoveFocus(player, S_COMMAND_PINDIAN);

    if (player->getHandcardNum() == 1)
        return player->getHandcards().constFirst();

    AI *ai = player->getAI();
    if (ai != nullptr) {
        thread->delay();
        return ai->askForPindian(from, reason);
    }

    bool success = doRequest(player, S_COMMAND_PINDIAN, JsonArray() << from->objectName() << to->objectName(), true);

    JsonArray clientReply = player->getClientReply().value<JsonArray>();
    if (!success || clientReply.isEmpty() || !JsonUtils::isString(clientReply[0])) {
        int card_id = player->getRandomHandCardId();
        return Sanguosha->getCard(card_id);
    } else {
        const Card *card = Card::Parse(clientReply[0].toString());
        if (card->isVirtualCard()) {
            const Card *real_card = Sanguosha->getCard(card->getEffectiveId());
            delete card;
            return real_card;
        } else
            return card;
    }
}

QList<const Card *> Room::askForPindianRace(ServerPlayer *from, ServerPlayer *to, const QString &reason)
{
    if (!from->isAlive() || !to->isAlive())
        return QList<const Card *>() << NULL << NULL;
    Q_ASSERT(!from->isKongcheng() && !to->isKongcheng());
    tryPause();
    Countdown countdown;
    countdown.max = ServerInfo.getCommandTimeout(S_COMMAND_PINDIAN, S_CLIENT_INSTANCE);
    countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
    notifyMoveFocus(QList<ServerPlayer *>() << from << to, S_COMMAND_PINDIAN, countdown);

    const Card *from_card = nullptr, *to_card = nullptr;

    if (from->getHandcardNum() == 1)
        from_card = from->getHandcards().constFirst();
    if (to->getHandcardNum() == 1)
        to_card = to->getHandcards().constFirst();

    AI *ai = nullptr;
    if (from_card == nullptr) {
        ai = from->getAI();
        if (ai != nullptr)
            from_card = ai->askForPindian(from, reason);
    }
    if (to_card == nullptr) {
        ai = to->getAI();
        if (ai != nullptr)
            to_card = ai->askForPindian(from, reason);
    }
    if ((from_card != nullptr) && (to_card != nullptr)) {
        thread->delay();
        return QList<const Card *>() << from_card << to_card;
    }

    QList<ServerPlayer *> players;
    if (from_card == nullptr) {
        JsonArray arr;
        arr << from->objectName() << to->objectName();
        from->m_commandArgs = arr;
        players << from;
    }
    if (to_card == nullptr) {
        JsonArray arr;
        arr << from->objectName() << to->objectName();
        to->m_commandArgs = arr;
        players << to;
    }

    doBroadcastRequest(players, S_COMMAND_PINDIAN);

    foreach (ServerPlayer *player, players) {
        const Card *c = nullptr;
        JsonArray clientReply = player->getClientReply().value<JsonArray>();
        if (!player->m_isClientResponseReady || clientReply.isEmpty() || !JsonUtils::isString(clientReply[0])) {
            int card_id = player->getRandomHandCardId();
            c = Sanguosha->getCard(card_id);
        } else {
            const Card *card = Card::Parse(clientReply[0].toString());
            if (card == nullptr) {
                int card_id = player->getRandomHandCardId();
                c = Sanguosha->getCard(card_id);
            } else if (card->isVirtualCard()) {
                const Card *real_card = Sanguosha->getCard(card->getEffectiveId());
                delete card;
                c = real_card;
            } else
                c = card;
        }
        if (player == from)
            from_card = c;
        else
            to_card = c;
    }
    return QList<const Card *>() << from_card << to_card;
}

ServerPlayer *Room::askForPlayerChosen(ServerPlayer *player, const QList<ServerPlayer *> &targets, const QString &skillName, const QString &prompt, bool optional,
                                       bool notify_skill)
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
        thread->trigger(ChoiceMade, this, data);
        return targets.first();
    }

    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_PLAYER);
    AI *ai = player->getAI();
    ServerPlayer *choice = nullptr;
    if (ai != nullptr) {
        choice = ai->askForPlayerChosen(targets, skillName, optional);
        if ((choice != nullptr) && notify_skill)
            thread->delay();
    } else {
        JsonArray req;
        JsonArray req_targets;
        foreach (ServerPlayer *target, targets)
            req_targets << target->objectName();
        req << QVariant(req_targets);
        req << skillName;
        req << prompt;
        req << optional;
        bool success = doRequest(player, S_COMMAND_CHOOSE_PLAYER, req, true);

        const QVariant &clientReply = player->getClientReply();
        if (success && JsonUtils::isString(clientReply))
            choice = findChild<ServerPlayer *>(clientReply.toString());
    }
    if ((choice != nullptr) && !targets.contains(choice))
        choice = nullptr;
    if (choice == nullptr && !optional)
        choice = targets.at(qrand() % targets.length());
    if (choice != nullptr) {
        if (notify_skill) {
            notifySkillInvoked(player, skillName);
            ChoiceMadeStruct s;
            s.player = player;
            s.type = ChoiceMadeStruct::SkillInvoke;
            s.args << skillName << "yes";
            QVariant decisionData = QVariant::fromValue(s);
            thread->trigger(ChoiceMade, this, decisionData);

            doAnimate(S_ANIMATE_INDICATE, player->objectName(), choice->objectName());
            LogMessage log;
            log.type = "#ChoosePlayerWithSkill";
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
        thread->trigger(ChoiceMade, this, data);
    } else { //for ai
        ChoiceMadeStruct s;
        s.player = player;
        s.type = ChoiceMadeStruct::PlayerChosen;
        s.args << skillName << "";
        QVariant data = QVariant::fromValue(s);
        thread->trigger(ChoiceMade, this, data);
    }

    return choice;
}

void Room::_setupChooseGeneralRequestArgs(ServerPlayer *player)
{
    JsonArray options;
    if (isHegemonyGameMode(mode)) {
        options << JsonUtils::toJsonArray(player->getSelected());
        options << false; //!Config.Enable2ndGeneral; //false;
        options << false;
    } else {
        options = JsonUtils::toJsonArray(player->getSelected()).value<JsonArray>();
        if (getLord() != nullptr && mode != "03_1v2")
            options.append(QString("%1(lord)").arg(getLord()->getGeneralName()));
    }

    player->m_commandArgs = options;
}

QString Room::askForGeneral(ServerPlayer *player, const QStringList &generals, QString default_choice)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_GENERAL);

    if (generals.length() == 1)
        return generals.first();

    if (default_choice.isEmpty())
        default_choice = generals.at(qrand() % generals.length());

    if (player->isOnline()) {
        JsonArray options = JsonUtils::toJsonArray(generals).value<JsonArray>();
        bool success = doRequest(player, S_COMMAND_CHOOSE_GENERAL, options, true);

        QVariant clientResponse = player->getClientReply();
        bool free = Config.FreeChoose;
        if (!success || !JsonUtils::isString(clientResponse) || (!free && !generals.contains(clientResponse.toString())))
            return default_choice;
        else
            return clientResponse.toString();
    }
    return default_choice;
}

QString Room::askForGeneral(ServerPlayer *player, const QString &generals, QString default_choice)
{
    return askForGeneral(player, generals.split("+"), default_choice); // For Lua only!!!
}

bool Room::makeCheat(ServerPlayer *player)
{
    JsonArray arg = player->m_cheatArgs.value<JsonArray>();
    player->m_cheatArgs = QVariant();
    if (arg.isEmpty() || !JsonUtils::isNumber(arg[0]))
        return false;

    CheatCode code = (CheatCode)arg[0].toInt();
    if (code == S_CHEAT_KILL_PLAYER) {
        JsonArray arg1 = arg[1].value<JsonArray>();
        if (!JsonUtils::isStringArray(arg1, 0, 1))
            return false;
        makeKilling(arg1[0].toString(), arg1[1].toString());

    } else if (code == S_CHEAT_MAKE_DAMAGE) {
        JsonArray arg1 = arg[1].value<JsonArray>();
        if (arg1.size() != 4 || !JsonUtils::isStringArray(arg1, 0, 1) || !JsonUtils::isNumber(arg1[2]) || !JsonUtils::isNumber(arg1[3]))
            return false;
        makeDamage(arg1[0].toString(), arg1[1].toString(), (QSanProtocol::CheatCategory)arg1[2].toInt(), arg1[3].toInt());

    } else if (code == S_CHEAT_REVIVE_PLAYER) {
        if (!JsonUtils::isString(arg[1]))
            return false;
        makeReviving(arg[1].toString());

    } else if (code == S_CHEAT_RUN_SCRIPT) {
        if (!JsonUtils::isString(arg[1]))
            return false;
        QByteArray data = QByteArray::fromBase64(arg[1].toString().toLatin1());
        data = qUncompress(data);
        doScript(data);

    } else if (code == S_CHEAT_GET_ONE_CARD) {
        if (!JsonUtils::isNumber(arg[1]))
            return false;
        int card_id = arg[1].toInt();

        LogMessage log;
        log.type = "$CheatCard";
        log.from = player;
        log.card_str = QString::number(card_id);
        sendLog(log);

        obtainCard(player, card_id);
    } else if (code == S_CHEAT_CHANGE_GENERAL) {
        if (!JsonUtils::isString(arg[1]) || !JsonUtils::isBool(arg[2]))
            return false;
        QString generalName = arg[1].toString();
        bool isSecondaryHero = arg[2].toBool();
        changeHero(player, generalName, false, true, isSecondaryHero);
    }

    return true;
}

void Room::makeDamage(const QString &source, const QString &target, QSanProtocol::CheatCategory nature, int point)
{
    ServerPlayer *sourcePlayer = findChild<ServerPlayer *>(source);
    ServerPlayer *targetPlayer = findChild<ServerPlayer *>(target);
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
        r.who = sourcePlayer;
        r.recover = point;
        recover(targetPlayer, r);
        return;
    } else if (nature == S_CHEAT_MAX_HP_RESET) {
        setPlayerProperty(targetPlayer, "maxhp", point);
        return;
    }

    static QMap<QSanProtocol::CheatCategory, DamageStruct::Nature> nature_map;
    if (nature_map.isEmpty()) {
        nature_map[S_CHEAT_NORMAL_DAMAGE] = DamageStruct::Normal;
        nature_map[S_CHEAT_THUNDER_DAMAGE] = DamageStruct::Thunder;
        nature_map[S_CHEAT_FIRE_DAMAGE] = DamageStruct::Fire;
    }

    if (targetPlayer == nullptr)
        return;
    damage(DamageStruct("cheat", sourcePlayer, targetPlayer, point, nature_map[nature]));
}

void Room::makeKilling(const QString &killerName, const QString &victimName)
{
    ServerPlayer *killer = findChild<ServerPlayer *>(killerName);
    ServerPlayer *victim = findChild<ServerPlayer *>(victimName);

    if (victim == nullptr)
        return;
    if (killer == nullptr)
        return killPlayer(victim);

    DamageStruct damage("cheat", killer, victim);
    killPlayer(victim, &damage);
}

void Room::makeReviving(const QString &name)
{
    ServerPlayer *player = findChild<ServerPlayer *>(name);
    Q_ASSERT(player);
    revivePlayer(player);
    setPlayerProperty(player, "maxhp", player->getGeneralMaxHp());
    setPlayerProperty(player, "hp", player->getMaxHp());
}

void Room::fillAG(const QList<int> &card_ids, ServerPlayer *who, const QList<int> &disabled_ids, const QList<int> &shownHandcard_ids)
{
    JsonArray arg;
    arg << JsonUtils::toJsonArray(card_ids);
    arg << JsonUtils::toJsonArray(disabled_ids);
    arg << JsonUtils::toJsonArray(shownHandcard_ids);

    m_fillAGarg = arg;
    m_fillAGWho = who;

    if (who != nullptr)
        doNotify(who, S_COMMAND_FILL_AMAZING_GRACE, arg);
    else
        doBroadcastNotify(S_COMMAND_FILL_AMAZING_GRACE, arg);
}

void Room::takeAG(ServerPlayer *player, int card_id, bool move_cards, QList<ServerPlayer *> to_notify, Player::Place fromPlace)
{
    if (to_notify.isEmpty())
        to_notify = getAllPlayers(true); //notice dead players

    JsonArray arg;
    arg << (player != nullptr ? QVariant(player->objectName()) : QVariant());
    arg << card_id;
    arg << move_cards;

    if (player != nullptr) {
        CardsMoveOneTimeStruct moveOneTime;
        if (move_cards) {
            CardsMoveOneTimeStruct move;
            move.from = nullptr;
            move.from_places << fromPlace;
            move.to = player;
            move.to_place = Player::PlaceHand;
            move.card_ids << card_id;
            QVariant data = QVariant::fromValue(move);
            thread->trigger(BeforeCardsMove, this, data);

            move = data.value<CardsMoveOneTimeStruct>();
            moveOneTime = move;

            if (move.card_ids.length() > 0) {
                if (move.to != nullptr && move.to == player) {
                    player->addCard(Sanguosha->getCard(card_id), Player::PlaceHand);
                    setCardMapping(card_id, player, Player::PlaceHand);
                    Sanguosha->getCard(card_id)->setFlags("visible");
                    QList<const Card *> cards;
                    cards << Sanguosha->getCard(card_id);
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
            thread->trigger(CardsMoveOneTime, this, data);
        }

    } else {
        doBroadcastNotify(to_notify, S_COMMAND_TAKE_AMAZING_GRACE, arg);
        if (!move_cards)
            return;
        LogMessage log;
        log.type = "$EnterDiscardPile";
        log.card_str = QString::number(card_id);
        sendLog(log);

        m_discardPile->prepend(card_id);
        setCardMapping(card_id, nullptr, Player::DiscardPile);
    }
    JsonArray takeagargs = m_takeAGargs.value<JsonArray>();
    takeagargs << arg;
    m_takeAGargs = takeagargs;
}

void Room::clearAG(ServerPlayer *player)
{
    m_fillAGarg = QVariant();
    m_fillAGWho = nullptr;
    m_takeAGargs = QVariant();
    if (player != nullptr)
        doNotify(player, S_COMMAND_CLEAR_AMAZING_GRACE, QVariant());
    else
        doBroadcastNotify(S_COMMAND_CLEAR_AMAZING_GRACE, QVariant());
}

void Room::provide(const Card *card, ServerPlayer *who)
{
    Q_ASSERT(provided == nullptr);
    Q_ASSERT(!has_provided);
    Q_ASSERT(provider == nullptr);

    provided = card;
    has_provided = true;
    provider = who;
}

QList<ServerPlayer *> Room::getLieges(const QString &kingdom, ServerPlayer *lord) const
{
    QList<ServerPlayer *> lieges;
    foreach (ServerPlayer *player, getAllPlayers()) {
        if (player != lord && player->getKingdom() == kingdom)
            lieges << player;
    }

    return lieges;
}

void Room::sendLog(const LogMessage &log)
{
    if (log.type.isEmpty())
        return;

    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
}

void Room::showCard(ServerPlayer *player, int card_id, ServerPlayer *only_viewer)
{
    if (getCardOwner(card_id) != player)
        return;

    tryPause();
    notifyMoveFocus(player);
    JsonArray show_arg;
    show_arg << player->objectName();
    show_arg << card_id;

    WrappedCard *card = Sanguosha->getWrappedCard(card_id);
    bool modified = card->isModified();
    if (only_viewer != nullptr) {
        QList<ServerPlayer *> players;
        players << only_viewer << player;
        if (modified)
            notifyUpdateCard(only_viewer, card_id, card);
        else
            notifyResetCard(only_viewer, card_id);
        doBroadcastNotify(players, S_COMMAND_SHOW_CARD, show_arg);
    } else {
        if (card_id > 0)
            Sanguosha->getCard(card_id)->setFlags("visible");
        if (modified)
            broadcastUpdateCard(getOtherPlayers(player), card_id, card);
        else
            broadcastResetCard(getOtherPlayers(player), card_id);
        doBroadcastNotify(S_COMMAND_SHOW_CARD, show_arg);
    }
}

void Room::showAllCards(ServerPlayer *player, ServerPlayer *to)
{
    if (player->isKongcheng())
        return;
    tryPause();

    JsonArray gongxinArgs;
    gongxinArgs << player->objectName();
    gongxinArgs << false;
    gongxinArgs << JsonUtils::toJsonArray(player->handCards());

    bool isUnicast = (to != nullptr);

    foreach (int cardId, player->handCards()) {
        WrappedCard *card = Sanguosha->getWrappedCard(cardId);
        if (card->isModified()) {
            if (isUnicast)
                notifyUpdateCard(to, cardId, card);
            else
                broadcastUpdateCard(getOtherPlayers(player), cardId, card);
        } else {
            if (isUnicast)
                notifyResetCard(to, cardId);
            else
                broadcastResetCard(getOtherPlayers(player), cardId);
        }
    }

    if (isUnicast) {
        LogMessage log;
        log.type = "$ViewAllCards";
        log.from = to;
        log.to << player;
        log.card_str = IntList2StringList(player->handCards()).join("+");
        doNotify(to, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());

        ChoiceMadeStruct s;
        s.player = to;
        s.type = ChoiceMadeStruct::ViewCards;
        s.args << to->objectName() << player->objectName();
        QVariant decisionData = QVariant::fromValue(s);
        thread->trigger(ChoiceMade, this, decisionData);

        doNotify(to, S_COMMAND_SHOW_ALL_CARDS, gongxinArgs);
    } else {
        LogMessage log;
        log.type = "$ShowAllCards";
        log.from = player;
        foreach (int card_id, player->handCards())
            Sanguosha->getCard(card_id)->setFlags("visible");
        log.card_str = IntList2StringList(player->handCards()).join("+");
        sendLog(log);

        doBroadcastNotify(S_COMMAND_SHOW_ALL_CARDS, gongxinArgs);
    }
}

void Room::retrial(const Card *card, ServerPlayer *player, JudgeStruct *judge, const QString &skill_name, bool exchange)
{
    if (card == nullptr)
        return;

    bool triggerResponded = getCardOwner(card->getEffectiveId()) == player;
    bool isHandcard = (triggerResponded && getCardPlace(card->getEffectiveId()) == Player::PlaceHand);

    const Card *oldJudge = judge->card;
    judge->card = Sanguosha->getCard(card->getEffectiveId());
    ServerPlayer *rebyre = judge->retrial_by_response; //old judge provider
    judge->retrial_by_response = player;

    CardsMoveStruct move1(QList<int>(), judge->who, Player::PlaceJudge, CardMoveReason(CardMoveReason::S_REASON_RETRIAL, player->objectName(), skill_name, QString()));

    move1.card_ids.append(card->getEffectiveId());

    int reasonType = exchange ? CardMoveReason::S_REASON_OVERRIDE : CardMoveReason::S_REASON_JUDGEDONE;

    CardMoveReason reason(reasonType, player->objectName(), exchange ? skill_name : QString(), QString());
    if (rebyre != nullptr)
        reason.m_extraData = QVariant::fromValue(rebyre);

    CardsMoveStruct move2(QList<int>(), judge->who, exchange ? player : nullptr, Player::PlaceUnknown, exchange ? Player::PlaceHand : Player::DiscardPile, reason);
    move2.card_ids.append(oldJudge->getEffectiveId());

    LogMessage log;
    log.type = "$ChangedJudge";
    log.arg = skill_name;
    log.from = player;
    log.to << judge->who;
    log.card_str = QString::number(card->getEffectiveId());
    sendLog(log);

    QList<CardsMoveStruct> moves;
    moves.append(move1);
    moves.append(move2);
    moveCardsAtomic(moves, true);
    judge->updateResult();

    if (triggerResponded) {
        CardResponseStruct resp(card, judge->who, false, true, false, player);
        QVariant data = QVariant::fromValue(resp);
        resp.m_isHandcard = isHandcard;
        thread->trigger(CardResponded, this, data);
    }
}

int Room::askForRende(ServerPlayer *liubei, QList<int> &cards, const QString &skill_name, bool, bool optional, int max_num, QList<ServerPlayer *> players, CardMoveReason reason,
                      const QString &prompt, bool notify_skill)
{
    if (max_num == -1)
        max_num = cards.length();
    if (players.isEmpty())
        players = getOtherPlayers(liubei);
    if (cards.isEmpty() || max_num == 0)
        return 0;
    if (reason.m_reason == CardMoveReason::S_REASON_UNKNOWN) {
        reason.m_playerId = liubei->objectName();
        reason.m_reason = CardMoveReason::S_REASON_GIVE;
    }
    tryPause();
    notifyMoveFocus(liubei, S_COMMAND_SKILL_YIJI);

    QMap<int, ServerPlayer *> give_map;
    QList<int> remain_cards = cards;
    int num = max_num;

    while (!remain_cards.isEmpty() && num > 0) {
        QList<int> ids;
        ServerPlayer *target = nullptr;
        AI *ai = liubei->getAI();
        if (ai != nullptr) {
            setPlayerFlag(liubei, "Global_AIAskForRende");
            int card_id = 0;
            ServerPlayer *who = ai->askForYiji(remain_cards, skill_name, card_id);
            setPlayerFlag(liubei, "-Global_AIAskForRende");
            if (who == nullptr)
                break;
            else {
                target = who;
                ids << card_id;
            }
        } else {
            JsonArray arg;
            arg << JsonUtils::toJsonArray(remain_cards);
            arg << optional;
            arg << num; //max_num;
            JsonArray player_names;
            foreach (ServerPlayer *player, players)
                player_names << player->objectName();
            arg << QVariant(player_names);
            if (!prompt.isEmpty())
                arg << prompt;
            else
                arg << QString();
            arg << QString(skill_name);

            bool success = doRequest(liubei, S_COMMAND_SKILL_YIJI, arg, true);

            //Validate client response
            JsonArray clientReply = liubei->getClientReply().value<JsonArray>();
            if (!success || clientReply.size() != 2) {
                if (!give_map.isEmpty())
                    break;
                else
                    return 0;
            }

            if (!JsonUtils::tryParse(clientReply[0], ids) || !JsonUtils::isString(clientReply[1]))
                return 0;

            bool foreach_flag = true;
            foreach (int id, ids)
                if (!remain_cards.contains(id)) {
                    foreach_flag = false;
                    break;
                }

            if (!foreach_flag)
                break;

            ServerPlayer *who = findChild<ServerPlayer *>(clientReply[1].toString());
            if (who == nullptr)
                break;
            else
                target = who;
        }

        Q_ASSERT(target != nullptr);

        foreach (int id, ids) {
            remain_cards.removeOne(id);
            give_map.insert(id, target);
        }

        num -= ids.length();
    }

    while (!optional && num > 0) {
        int id = remain_cards[qrand() % remain_cards.length()];
        remain_cards.removeOne(id);
        num--;
        give_map.insert(id, players[qrand() % players.length()]);
    }

    if (give_map.isEmpty())
        return 0;

    cards = remain_cards;

    QStringList namelist;
    foreach (ServerPlayer *p, give_map)
        namelist << p->objectName();

    ChoiceMadeStruct s;
    s.player = liubei;
    s.type = ChoiceMadeStruct::Rende;
    s.args << skill_name << liubei->objectName() << namelist.join("+") << IntList2StringList(give_map.keys()).join("+");
    QVariant decisionData = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, decisionData);

    if (notify_skill) {
        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = liubei;
        log.arg = skill_name;
        sendLog(log);

        const Skill *skill = Sanguosha->getSkill(skill_name);
        if (skill != nullptr)
            broadcastSkillInvoke(skill_name);
        notifySkillInvoked(liubei, skill_name);
    }

    QList<CardsMoveStruct> movelist;

    foreach (int card_id, give_map.keys()) {
        CardsMoveStruct move(card_id, give_map.value(card_id), Player::PlaceHand, reason);
        movelist << move;
    }

    liubei->setFlags("Global_GongxinOperator");
    moveCardsAtomic(movelist, false);
    liubei->setFlags("-Global_GongxinOperator");

    return give_map.keys().length();
}

bool Room::askForYiji(ServerPlayer *guojia, QList<int> &cards, const QString &skill_name, bool is_preview, bool visible, bool optional, int max_num, QList<ServerPlayer *> players,
                      CardMoveReason reason, const QString &prompt, bool notify_skill)
{
    if (max_num == -1)
        max_num = cards.length();
    if (players.isEmpty())
        players = getOtherPlayers(guojia);
    if (cards.isEmpty() || max_num == 0)
        return false;
    if (reason.m_reason == CardMoveReason::S_REASON_UNKNOWN) {
        reason.m_playerId = guojia->objectName();
        // when we use ? : here, compiling error occurs under debug mode...
        if (is_preview)
            reason.m_reason = CardMoveReason::S_REASON_PREVIEWGIVE;
        else
            reason.m_reason = CardMoveReason::S_REASON_GIVE;
    }
    tryPause();
    notifyMoveFocus(guojia, S_COMMAND_SKILL_YIJI);

    ServerPlayer *target = nullptr;

    QList<int> ids;
    AI *ai = guojia->getAI();
    if (ai != nullptr) {
        int card_id = 0;
        ServerPlayer *who = ai->askForYiji(cards, skill_name, card_id);
        if (who == nullptr)
            return false;
        else {
            target = who;
            ids << card_id;
        }
    } else {
        JsonArray arg;
        arg << JsonUtils::toJsonArray(cards);
        arg << optional;
        arg << max_num;
        JsonArray player_names;
        foreach (ServerPlayer *player, players)
            player_names << player->objectName();
        arg << QVariant(player_names);
        if (!prompt.isEmpty())
            arg << prompt;
        else
            arg << QString();
        arg << QString(skill_name);

        bool success = doRequest(guojia, S_COMMAND_SKILL_YIJI, arg, true);

        //Validate client response
        JsonArray clientReply = guojia->getClientReply().value<JsonArray>();
        if (!success || clientReply.size() != 2)
            return false;

        if (!JsonUtils::tryParse(clientReply[0], ids) || !JsonUtils::isString(clientReply[1]))
            return false;

        foreach (int id, ids)
            if (!cards.contains(id))
                return false;

        ServerPlayer *who = findChild<ServerPlayer *>(clientReply[1].toString());
        if (who == nullptr)
            return false;
        else
            target = who;
    }
    Q_ASSERT(target != nullptr);

    ChoiceMadeStruct s;
    s.player = guojia;
    s.type = ChoiceMadeStruct::Yiji;
    s.args << skill_name << guojia->objectName() << target->objectName() << IntList2StringList(ids).join("+");
    QVariant decisionData = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, this, decisionData);

    if (notify_skill) {
        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = guojia;
        log.arg = skill_name;
        sendLog(log);

        const Skill *skill = Sanguosha->getSkill(skill_name);
        if (skill != nullptr)
            broadcastSkillInvoke(skill_name);
        notifySkillInvoked(guojia, skill_name);
    }

    guojia->setFlags("Global_GongxinOperator");
    foreach (int card_id, ids) {
        cards.removeOne(card_id);
        moveCardTo(Sanguosha->getCard(card_id), target, Player::PlaceHand, reason, visible);
    }
    guojia->setFlags("-Global_GongxinOperator");

    return true;
}

QString Room::generatePlayerName()
{
    static unsigned int id = 0;
    id++;
    return QString("sgs%1").arg(id);
}

QString Room::askForOrder(ServerPlayer *player, const QString &default_choice)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_ORDER);

    if (player->getAI() != nullptr)
        return default_choice;

    bool success = doRequest(player, S_COMMAND_CHOOSE_ORDER, (int)S_REASON_CHOOSE_ORDER_TURN, true);

    QVariant clientReply = player->getClientReply();
    if (success && JsonUtils::isNumber(clientReply))
        return ((Game3v3Camp)clientReply.toInt() == S_CAMP_WARM) ? "warm" : "cool";
    return default_choice;
}

QString Room::askForRole(ServerPlayer *player, const QStringList &roles, const QString &scheme)
{
    tryPause();
    notifyMoveFocus(player, S_COMMAND_CHOOSE_ROLE_3V3);

    QStringList squeezed = roles.toSet().toList();
    JsonArray arg;
    arg << scheme << JsonUtils::toJsonArray(squeezed);
    bool success = doRequest(player, S_COMMAND_CHOOSE_ROLE_3V3, arg, true);
    QVariant clientReply = player->getClientReply();
    QString result = "abstain";
    if (success && JsonUtils::isString(clientReply))
        result = clientReply.toString();
    return result;
}

void Room::networkDelayTestCommand(ServerPlayer *player, const QVariant &)
{
    qint64 delay = player->endNetworkDelayTest();
    QString reportStr = tr("<font color=#EEB422>The network delay of player <b>%1</b> is %2 milliseconds.</font>").arg(player->screenName(), QString::number(delay));
    speakCommand(player, reportStr.toUtf8().toBase64());
}

void Room::sortByActionOrder(QList<ServerPlayer *> &players)
{
    if (players.length() > 1)
        std::sort(players.begin(), players.end(), ServerPlayer::CompareByActionOrder);
}

void Room::defaultHeroSkin()
{
    if (Config.DefaultHeroSkin) {
        QStringList all = Sanguosha->getLimitedGeneralNames();
        Config.beginGroup("HeroSkin");
        foreach (const QString &general_name, all)
            Config.remove(general_name);

        Config.endGroup();
    }
}

void Room::sendLog(const QString &logtype, ServerPlayer *logfrom, const QString &logarg, const QList<ServerPlayer *> &logto, const QString &logarg2)
{
    LogMessage alog;

    alog.type = logtype;
    alog.from = logfrom;
    alog.to = logto;
    alog.arg = logarg;
    alog.arg2 = logarg2;

    sendLog(alog);
}

void Room::skinChangeCommand(ServerPlayer *player, const QVariant &packet)
{
    JsonArray arg = packet.value<JsonArray>();
    QString generalName = arg[0].toString();

    JsonArray val;
    val << (int)QSanProtocol::S_GAME_EVENT_SKIN_CHANGED;
    val << player->objectName();
    val << generalName;
    val << arg[1].toInt();
    val << (player->getGeneralName() == generalName);

    setTag(generalName + "_skin_id", arg[1].toInt());
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, val);
}

void Room::heartbeatCommand(ServerPlayer *player, const QVariant &)
{
    doNotify(player, QSanProtocol::S_COMMAND_HEARTBEAT, QVariant());
}

bool Room::roleStatusCommand(ServerPlayer *player)
{
    JsonArray val;

    val << (int)QSanProtocol::S_GAME_ROLE_STATUS_CHANGED;
    val << player->objectName();
    val << player->hasShownRole();

    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, val);
    return true;
}

void Room::saveWinnerTable(const QString &winner, bool isSurrender)
{
    //check gameMode
    QString mode = Config.GameMode;
    if (mode.endsWith("1v1") || mode.endsWith("1v3") || mode == "06_XMode")
        return;

    //check human players and check Surrender
    int count = 0;
    foreach (ServerPlayer *p, getAllPlayers(true)) {
        if (p->getState() != "robot")
            count++;
    }
    if (getAllPlayers(true).length() < 6 || count < getAllPlayers(true).length() / 2)
        return;

    if (isSurrender) {
        ServerPlayer *lord = getLord();
        bool lordALive = false;
        if ((lord == nullptr) || lord->isAlive())
            lordALive = true;
        int round = getTag("Global_RoundCount").toInt();
        int death = getAllPlayers(true).length() - getAllPlayers().length();
        if (round < 2 && lordALive && death < (getAllPlayers(true).length() / 4))
            return;
    }

    QString location = "etc/winner/";
    if (!QDir(location).exists())
        QDir().mkdir(location);
    QDateTime time = QDateTime::currentDateTime();
    if (isHegemonyGameMode(mode))
        location.append("Heg");
    location.append(time.toString("yyyyMM"));
    location.append(".txt");
    QFile file(location);

    if (!file.open(QIODevice::ReadWrite))
        return;

    QString line;
    QTextStream stream(&file);
    stream.seek(file.size());

    line.append(QString("Date: %1/%2 %3:%4:%5").arg(time.toString("MM"), time.toString("dd"), time.toString("hh"), time.toString("mm"), time.toString("ss")));
    line.append("\n");
    QStringList winners = winner.split("+");
    foreach (ServerPlayer *p, getAllPlayers(true)) {
        QString gname;
        QString originalName = p->tag.value("init_general", QString()).toString();
        if (originalName != nullptr && (Sanguosha->getGeneral(originalName) != nullptr))
            gname = Sanguosha->getGeneral(originalName)->objectName();
        else
            gname = p->getGeneralName();
        if (isHegemonyGameMode(mode)) {
            QString originalName2 = p->tag.value("init_general2", QString()).toString();
            if (originalName2 != nullptr && (Sanguosha->getGeneral(originalName2) != nullptr))
                gname = gname + "|" + Sanguosha->getGeneral(originalName2)->objectName();
            else
                gname = gname + "|" + p->getGeneral2Name();
        }
        line.append(gname);
        line.append(" ");
        line.append(p->getRole());
        line.append(" ");
        line.append(QString::number(p->getInitialSeat()));
        line.append(" ");
        if (p->getState() != "robot")
            line.append("human");
        else
            line.append("robot");
        line.append(" ");
        if (winner == ".")
            line.append("draw");
        else if (winners.contains(p->getRole()) || winner == p->objectName())
            line.append("win");
        else
            line.append("lose");
        line.append("\n");
    }

    stream << line;
    file.close();
}

void Room::countDescription()
{
    QString location = "etc/count.txt";
    QFile file(location);
    if (!file.open(QIODevice::ReadWrite))
        return;

    QTextStream stream(&file);
    QList<QString> all = Sanguosha->getLimitedGeneralNames();
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    QMultiMap<int, QString> map;
    foreach (const QString &name, all) {
        const General *gen = Sanguosha->getGeneral(name);
        QString line = "";
        foreach (const Skill *skill, gen->getVisibleSkillList()) {
            QString skill_name = Sanguosha->translate(skill->objectName());

            QString desc = Sanguosha->translate(":" + skill->objectName());
            QRegExp rx("<[^>]*>");
            desc.remove(rx);
            QString skill_line = QString("%1:%2").arg(skill_name, desc);
            line = line + skill_line;
        }
        map.insert(line.length(), name);
    }
    QMultiMap<int, QString>::iterator it;
    int num = 0;
    for (it = map.begin(); it != map.end(); ++it) {
        QString countString = "";
        countString.append(QString("%1: %2 ").arg(Sanguosha->translate(it.value()), QString::number(it.key())));
        countString.append("\n");
        stream << countString;
        num += it.key();
    }
    QString count = QString::number(num / map.size());
    stream << count;
    file.close();
}

void Room::transformGeneral(ServerPlayer *player, QString general_name, int head)
{
    if (!player->canTransform(head != 0))
        return; //check sujiang

    QStringList names;
    //names << player->getActualGeneral1Name() << player->getActualGeneral2Name();
    QStringList generals = getTag(player->objectName()).toStringList();
    names << generals.first() << generals.last();

    //handleUsedGeneral("-" + player->getActualGeneral2Name());
    //handleUsedGeneral(general_name);

    player->removeGeneral(head != 0);
    QList<const TriggerSkill *> game_start;

    foreach (const Skill *skill, Sanguosha->getGeneral(general_name)->getVisibleSkillList(true, head)) {
        if (skill->inherits("TriggerSkill")) {
            const TriggerSkill *tr = qobject_cast<const TriggerSkill *>(skill);
            if (tr != nullptr) {
                getThread()->addTriggerSkill(tr);
                if (tr->getTriggerEvents().contains(GameStart)) // && !tr->triggerable(GameStart, this, player, void_data).isEmpty()
                    game_start << tr;
            }
        }
        player->addSkill(skill->objectName(), head != 0);
        //invoke->invoker->sendSkillsToOthers(head);//check shown
    }

    if (head != 0) {
        changePlayerGeneral(player, "anjiang");
        notifyProperty(player, player, "general", general_name);
        names[0] = general_name;
        setPlayerProperty(player, "general_showed", false);
    } else {
        changePlayerGeneral2(player, "anjiang");
        notifyProperty(player, player, "general2", general_name);

        names[1] = general_name;
        setPlayerProperty(player, "general2_showed", false);
    }

    setTag(player->objectName(), names);

    foreach (const Skill *skill, Sanguosha->getGeneral(general_name)->getSkillList(true, head)) {
        if (skill->getFrequency() == Skill::Limited && !skill->getLimitMark().isEmpty()) {
            player->setMark(skill->getLimitMark(), 1);
            JsonArray arg;
            arg << player->objectName();
            arg << skill->getLimitMark();
            arg << 1;
            doNotify(player, S_COMMAND_SET_MARK, arg);
        }
    }

    if (!game_start.isEmpty()) {
        QVariant v = QVariant::fromValue(player);
        thread->trigger(GameStart, this, v);
    }
    /*foreach(const TriggerSkill *skill, game_start) {
    if (skill->cost(GameStart, this, player, void_data, player))
    skill->effect(GameStart, this, player, void_data, player);
    }*/

    //do not consider CompanionEffect
    //if (Sanguosha->getGeneral(names[0])->isCompanionWith(general_name))
    //	setPlayerMark(player, "CompanionEffect", 1);
    //if (need_show)
    player->showGeneral(head != 0);
}
