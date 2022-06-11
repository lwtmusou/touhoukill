#ifndef qsgslegacy__ROOM_H
#define qsgslegacy__ROOM_H

#include "RoomObject.h"
#include "legacyprotocol.h"
#include "legacyroomthread.h"
#include "legacyserverplayer.h"
#include "legacystructs.h"
#include "protocol.h"

#include <QAtomicPointer>
#include <QMutex>
#include <QStack>
#include <QWaitCondition>

class ProhibitSkill;
class TrickCard;
struct ServerInfoStruct;

struct lua_State;
struct LogMessage;

#define QSGS_STATE_ROOM
#define QSGS_STATE_GAME
#define QSGS_LOGIC
#define QSGS_SOCKET

class LegacyRoom : public RoomObject
{
    Q_OBJECT

public:
    explicit LegacyRoom(QObject *parent, const ServerInfoStruct *si);
    ~LegacyRoom() override = default;
    RoomThread *getThread() const;
    void output(const QString &message);
    void saveWinnerTable(const QString &winner, bool isSurrender = false);
    void countDescription();

    friend class RoomThread;

    QSGS_STATE_ROOM int getLack() const;
    QSGS_STATE_GAME bool isFinished() const;
    QSGS_STATE_GAME QString getMode() const;
    QSGS_STATE_GAME LegacyServerPlayer *getCurrent() const;
    QSGS_STATE_GAME void setCurrent(LegacyServerPlayer *current);
    QSGS_STATE_GAME int alivePlayerCount() const;
    QSGS_STATE_GAME QList<LegacyServerPlayer *> getOtherPlayers(LegacyServerPlayer *except, bool include_dead = false) const;
    QSGS_STATE_GAME QList<LegacyServerPlayer *> getPlayers() const;
    QSGS_STATE_GAME QList<LegacyServerPlayer *> getAllPlayers(bool include_dead = false) const;
    QSGS_STATE_GAME QList<LegacyServerPlayer *> getAlivePlayers() const;
    QSGS_STATE_GAME LegacyServerPlayer *getCurrentDyingPlayer() const;
    QSGS_STATE_GAME QStringList aliveRoles(LegacyServerPlayer *except = nullptr) const;
    QSGS_STATE_GAME LegacyServerPlayer *getLord(const QString &kingdom = QStringLiteral("wei"), bool include_death = false) const;
    QSGS_STATE_GAME QList<LegacyServerPlayer *> getLieges(const QString &kingdom, LegacyServerPlayer *lord) const;
    QSGS_STATE_GAME inline QList<int> &getDrawPile()
    {
        return *m_drawPile;
    }
    QSGS_STATE_GAME inline const QList<int> &getDrawPile() const
    {
        return *m_drawPile;
    }
    QSGS_STATE_GAME LegacyServerPlayer *findPlayer(const QString &general_name, bool include_dead = false) const;
    QSGS_STATE_GAME QList<LegacyServerPlayer *> findPlayersBySkillName(const QString &skill_name, bool include_hidden = true) const;
    QSGS_STATE_GAME LegacyServerPlayer *findPlayerBySkillName(const QString &skill_name) const;
    QSGS_STATE_GAME LegacyServerPlayer *findPlayerByObjectName(const QString &name, bool include_dead = false) const;
    QSGS_STATE_GAME bool hasWelfare(const LegacyServerPlayer *player) const;
    QSGS_STATE_GAME LegacyServerPlayer *getFront(LegacyServerPlayer *a, LegacyServerPlayer *b) const;
    QSGS_STATE_GAME void setTag(const QString &key, const QVariant &value);
    QSGS_STATE_GAME QVariant getTag(const QString &key) const;
    QSGS_STATE_GAME void removeTag(const QString &key);
    QSGS_STATE_GAME QStringList getTagNames() const;
    QSGS_STATE_GAME QSanguosha::Place getCardPlace(int card_id) const;
    QSGS_STATE_GAME LegacyServerPlayer *getCardOwner(int card_id) const;
    QSGS_STATE_GAME void setCardMapping(int card_id, LegacyServerPlayer *owner, QSanguosha::Place place);
    // FIXME: Replace their return value to IDSet.
    QSGS_STATE_GAME QList<int> getCardIdsOnTable(const Card *) const;
    QSGS_STATE_GAME QList<int> getCardIdsOnTable(const IdSet &card_ids) const;

    QSGS_LOGIC enum GuanxingType { GuanxingUpOnly = 1, GuanxingBothSides = 0, GuanxingDownOnly = -1 };
    Q_ENUM(GuanxingType)

    QSGS_SOCKET typedef void (LegacyRoom::*Callback)(LegacyServerPlayer *, const QJsonValue &);
    QSGS_SOCKET typedef bool (LegacyRoom::*ResponseVerifyFunction)(LegacyServerPlayer *, const QJsonValue &, void *);

    QSGS_SOCKET LegacyServerPlayer *addSocket(LegacyClientSocket *socket);

    QSGS_SOCKET inline int getId() const
    {
        return _m_Id;
    }
    QSGS_STATE_ROOM bool isFull() const;
    QSGS_STATE_ROOM bool canPause(LegacyServerPlayer *p) const;
    QSGS_STATE_ROOM void tryPause();

    QSGS_LOGIC void setEmotion(LegacyServerPlayer *target, const QString &emotion);
    QSGS_LOGIC void enterDying(LegacyServerPlayer *player, DamageStruct *reason);
    QSGS_LOGIC void killPlayer(LegacyServerPlayer *victim, DamageStruct *reason = nullptr);
    QSGS_LOGIC void revivePlayer(LegacyServerPlayer *player, bool initialize = true);
    QSGS_LOGIC void gameOver(const QString &winner, bool isSurrender = false);
    QSGS_LOGIC void slashEffect(const SlashEffectStruct &effect);
    QSGS_LOGIC void slashResult(const SlashEffectStruct &effect, const Card *jink);
    QSGS_LOGIC void attachSkillToPlayer(LegacyServerPlayer *player, const QString &skill_name, bool is_other_attach = false);

    QSGS_LOGIC void detachSkillFromPlayer(LegacyServerPlayer *player, const QString &skill_name, bool is_equip = false, bool acquire_only = false, bool sendlog = true,
                                          bool head = true);
    QSGS_LOGIC void handleAcquireDetachSkills(LegacyServerPlayer *player, const QStringList &skill_names, bool acquire_only = false);
    QSGS_LOGIC void handleAcquireDetachSkills(LegacyServerPlayer *player, const QString &skill_names, bool acquire_only = false);
    QSGS_LOGIC void setPlayerFlag(LegacyServerPlayer *player, const QString &flag);
    QSGS_LOGIC void setPlayerProperty(LegacyServerPlayer *player, const char *property_name, const QVariant &value);
    QSGS_LOGIC void setPlayerMark(LegacyServerPlayer *player, const QString &mark, int value);
    QSGS_LOGIC void addPlayerMark(LegacyServerPlayer *player, const QString &mark, int add_num = 1);
    QSGS_LOGIC void removePlayerMark(LegacyServerPlayer *player, const QString &mark, int remove_num = 1);
    QSGS_LOGIC void setPlayerCardLimitation(LegacyServerPlayer *player, const QString &limit_list, const QString &pattern, const QString &reason, bool single_turn);
    QSGS_LOGIC void removePlayerCardLimitation(LegacyServerPlayer *player, const QString &limit_list, const QString &pattern, const QString &reason, bool clearReason = false);
    QSGS_LOGIC void clearPlayerCardLimitation(LegacyServerPlayer *player, bool single_turn);
    QSGS_LOGIC void setPlayerDisableShow(LegacyServerPlayer *player, const QString &flags, const QString &reason);
    QSGS_LOGIC void removePlayerDisableShow(LegacyServerPlayer *player, const QString &reason);
    QSGS_LOGIC void setCardFlag(const Card *card, const QString &flag, LegacyServerPlayer *who = nullptr);
    QSGS_LOGIC void setCardFlag(int card_id, const QString &flag, LegacyServerPlayer *who = nullptr);
    QSGS_LOGIC void clearCardFlag(const Card *card, LegacyServerPlayer *who = nullptr);
    QSGS_LOGIC void clearCardFlag(int card_id, LegacyServerPlayer *who = nullptr);
    QSGS_LOGIC bool useCard(const CardUseStruct &card_use, bool add_history = true);
    QSGS_LOGIC void damage(const DamageStruct &data);
    QSGS_LOGIC void sendDamageLog(const DamageStruct &data);
    QSGS_LOGIC void loseHp(LegacyServerPlayer *victim, int lose = 1);
    QSGS_LOGIC void loseMaxHp(LegacyServerPlayer *victim, int lose = 1);
    QSGS_LOGIC bool changeMaxHpForAwakenSkill(LegacyServerPlayer *player, int magnitude = -1);
    QSGS_LOGIC void applyDamage(LegacyServerPlayer *victim, const DamageStruct &damage);
    QSGS_LOGIC void recover(LegacyServerPlayer *player, const RecoverStruct &recover, bool set_emotion = false);
    QSGS_LOGIC bool cardEffect(const Card *card, LegacyServerPlayer *from, LegacyServerPlayer *to, bool multiple = false);
    QSGS_LOGIC bool cardEffect(const CardEffectStruct &effect);
    QSGS_LOGIC bool isJinkEffected(const SlashEffectStruct &effect, const Card *jink);
    QSGS_LOGIC void judge(JudgeStruct &judge_struct);
    QSGS_LOGIC void sendJudgeResult(const JudgeStruct *judge);
    QSGS_LOGIC QList<int> getNCards(int n, bool update_pile_number = true, bool bottom = false);
    QSGS_LOGIC void returnToTopDrawPile(const QList<int> &cards);
    QSGS_LOGIC void askForGuanxing(LegacyServerPlayer *zhuge, const QList<int> &cards, GuanxingType guanxing_type = GuanxingBothSides, const QString &skillName = QString());
    QSGS_LOGIC int doGongxin(LegacyServerPlayer *shenlvmeng, LegacyServerPlayer *target, const QList<int> &enabled_ids = QList<int>(),
                             const QString &skill_name = QStringLiteral("gongxin"), bool cancellable = true);
    QSGS_LOGIC int drawCard(bool bottom = false);
    QSGS_LOGIC void fillAG(const QList<int> &card_ids, LegacyServerPlayer *who = nullptr, const QList<int> &disabled_ids = QList<int>(),
                           const QList<int> &shownHandcard_ids = QList<int>());
    QSGS_LOGIC void takeAG(LegacyServerPlayer *player, int card_id, bool move_cards = true, QList<LegacyServerPlayer *> to_notify = QList<LegacyServerPlayer *>(),
                           QSanguosha::Place fromPlace = QSanguosha::PlaceDrawPile);
    QSGS_LOGIC void clearAG(LegacyServerPlayer *player = nullptr);
    QSGS_LOGIC void provide(const Card *card, LegacyServerPlayer *who = nullptr);
    QSGS_LOGIC void sendLog(const LogMessage &log);
    QSGS_LOGIC void showCard(LegacyServerPlayer *player, int card_id, LegacyServerPlayer *only_viewer = nullptr);
    QSGS_LOGIC void showAllCards(LegacyServerPlayer *player, LegacyServerPlayer *to = nullptr);
    QSGS_LOGIC void retrial(const Card *card, LegacyServerPlayer *player, JudgeStruct *judge, const QString &skill_name, bool exchange = false);

    // Ask a player to send a server request and returns the client response. Call is blocking until client
    // replies or server times out, whichever is earlier.
    // @param player
    //        The server player to carry out the command.
    // @param command
    //        Command to be executed on client side.
    // @param arg
    //        Command args.
    // @param timeOut
    //        Maximum milliseconds that server should wait for client response before returning.
    // @param wait
    //        If true, return immediately after sending the request without waiting for client reply.
    // @return True if the a valid response is returned from client.
    // Usage note: when you need a round trip request-response vector with a SINGLE client, use this command
    // with wait = true and read the reponse from player->getClientReply(). If you want to initiate a poll
    // where more than one clients can respond simultaneously, you have to do it in two steps:
    // 1. Use this command with wait = false once for each client involved in the poll (or you can call this
    //    command only once in all with broadcast = true if the poll is to everypody).
    // 2. Call getResult(player, timeout) on each player to retrieve the result. Read manual for getResults
    //    before you use.
    QSGS_SOCKET bool doRequest(LegacyServerPlayer *player, QSanProtocol::CommandType command, const QJsonValue &arg, time_t timeOut, bool wait);
    QSGS_SOCKET bool doRequest(LegacyServerPlayer *player, QSanProtocol::CommandType command, const QJsonValue &arg, bool wait);

    // Broadcast a request to a list of players and get the client responses. Call is blocking until all client
    // replies or server times out, whichever is earlier. Check each player's m_isClientResponseReady to see if a valid
    // result has been received. The client response can be accessed by calling each player's getClientReply() function.
    // @param players
    //        The list of server players to carry out the command.
    // @param command
    //        Command to be executed on client side. Command arguments should be stored in players->m_commandArgs.
    // @param timeOut
    //        Maximum total milliseconds that server will wait for all clients to respond before returning. Any client
    //        response after the timeOut will be rejected.
    // @return True if the a valid response is returned from client.
    QSGS_SOCKET bool doBroadcastRequest(QList<LegacyServerPlayer *> &players, QSanProtocol::CommandType command, time_t timeOut);
    QSGS_SOCKET bool doBroadcastRequest(QList<LegacyServerPlayer *> &players, QSanProtocol::CommandType command);

    // Broadcast a request to a list of players and get the first valid client response. Call is blocking until the first
    // client response is received or server times out, whichever is earlier. Any client response is verified by the validation
    // function and argument passed in. When a response is verified to be invalid, the function will continue to wait for
    // the next client response.
    // @param validateFunc
    //        Validation function that verifies whether the reply is a valid one. The first parameter passed to the function
    //        is the response sender, the second parameter is the response content, the third parameter is funcArg passed in.
    // @return The player that first send a legal request to the server. NULL if no such request is received.
    QSGS_SOCKET LegacyServerPlayer *doBroadcastRaceRequest(QList<LegacyServerPlayer *> &players, QSanProtocol::CommandType command, time_t timeOut,
                                                           ResponseVerifyFunction validateFunc = nullptr, void *funcArg = nullptr);

    // Notify a player of a event by sending S_SERVER_NOTIFICATION packets. No reply should be expected from
    // the client for S_SERVER_NOTIFICATION as it's a one way notice. Any message from the client in reply to this call
    // will be rejected.
    QSGS_SOCKET bool doNotify(LegacyServerPlayer *player, QSanProtocol::CommandType command, const QJsonValue &arg);

    // Broadcast a event to a list of players by sending S_SERVER_NOTIFICATION packets. No replies should be expected from
    // the clients for S_SERVER_NOTIFICATION as it's a one way notice. Any message from the client in reply to this call
    // will be rejected.
    QSGS_SOCKET bool doBroadcastNotify(QSanProtocol::CommandType command, const QJsonValue &arg);
    QSGS_SOCKET bool doBroadcastNotify(const QList<LegacyServerPlayer *> &players, QSanProtocol::CommandType command, const QJsonValue &arg);

    QSGS_SOCKET bool doNotify(LegacyServerPlayer *player, int command, const char *arg);
    QSGS_SOCKET bool doBroadcastNotify(int command, const char *arg);
    QSGS_SOCKET bool doBroadcastNotify(const QList<LegacyServerPlayer *> &players, int command, const char *arg);

    // Ask a server player to wait for the client response. Call is blocking until client replies or server times out,
    // whichever is earlier.
    // @param player
    //        The server player to retrieve the client response.
    // @param timeOut
    //        Maximum milliseconds that server should wait for client response before returning.
    // @return True if the a valid response is returned from client.

    // Usage note: this function is only supposed to be either internally used by doRequest (wait = true) or externally
    // used in pair with doRequest (wait = false). Any other usage could result in unexpected synchronization errors.
    // When getResult returns true, it's guaranteed that the expected client response has been stored and can be accessed by
    // calling player->getClientReply(). If getResult returns false, the value stored in player->getClientReply() could be
    // corrupted or in response to an unrelevant server request. Therefore, if the return value is false, do not poke into
    // player->getClientReply(), use the default value directly. If the return value is true, the reply value should still be
    // examined as a malicious client can have tampered with the content of the package for cheating purposes.
    QSGS_SOCKET bool getResult(LegacyServerPlayer *player, time_t timeOut);
    QSGS_SOCKET LegacyServerPlayer *getRaceResult(QList<LegacyServerPlayer *> &players, QSanProtocol::CommandType command, time_t timeOut,
                                                  ResponseVerifyFunction validateFunc = nullptr, void *funcArg = nullptr);

    // Verification functions
    bool verifyNullificationResponse(LegacyServerPlayer *, const QJsonValue &, void *);

    // Notification functions
    QSGS_LOGIC bool notifyMoveFocus(LegacyServerPlayer *player);
    QSGS_LOGIC bool notifyMoveFocus(LegacyServerPlayer *player, QSanProtocol::CommandType command);
    QSGS_LOGIC bool notifyMoveFocus(const QList<LegacyServerPlayer *> &players, QSanProtocol::CommandType command, const QSanProtocol::Countdown &countdown);

    // Notify client side to move cards from one place to another place. A movement should always be completed by
    // calling notifyMoveCards in pairs, one with isLostPhase equaling true followed by one with isLostPhase
    // equaling false. The two phase design is needed because the target player doesn't necessarily gets the
    // cards that the source player lost. Any trigger during the movement can cause either the target player to
    // be dead or some of the cards to be moved to another place before the target player actually gets it.
    // @param isLostPhase
    //        Specify whether this is a S_COMMAND_LOSE_CARD notification.
    // @param move
    //        Specify all movements need to be broadcasted.
    // @param forceVisible
    //        If true, all players will be able to see the face of card regardless of whether the movement is
    //        relevant or not.
    bool notifyMoveCards(bool isLostPhase, QList<LegacyCardsMoveStruct> move, bool forceVisible, QList<LegacyServerPlayer *> players = QList<LegacyServerPlayer *>());
    bool notifyProperty(LegacyServerPlayer *playerToNotify, const LegacyServerPlayer *propertyOwner, const char *propertyName, const QString &value = QString());
    QSGS_LOGIC bool notifyUpdateCard(LegacyServerPlayer *player, int cardId, const Card *newCard);
    QSGS_LOGIC bool broadcastUpdateCard(const QList<LegacyServerPlayer *> &players, int cardId, const Card *newCard);
    QSGS_LOGIC bool notifyResetCard(LegacyServerPlayer *player, int cardId);
    QSGS_LOGIC bool broadcastResetCard(const QList<LegacyServerPlayer *> &players, int cardId);

    QSGS_LOGIC bool broadcastProperty(LegacyServerPlayer *player, const char *property_name, const QString &value = QString());
    QSGS_LOGIC void notifySkillInvoked(LegacyServerPlayer *player, const QString &skill_name);
    QSGS_LOGIC void broadcastSkillInvoke(const QString &skillName);
    QSGS_LOGIC void broadcastSkillInvoke(const QString &skillName, const QString &category);
    QSGS_LOGIC void broadcastSkillInvoke(const QString &skillName, int type);
    QSGS_LOGIC void broadcastSkillInvoke(const QString &skillName, bool isMale, int type);
    QSGS_LOGIC void doLightbox(const QString &lightboxName, int duration = 2000);
    QSGS_LOGIC void doAnimate(QSanProtocol::AnimateType type, const QString &arg1 = QString(), const QString &arg2 = QString(),
                              QList<LegacyServerPlayer *> players = QList<LegacyServerPlayer *>());

    QSGS_LOGIC inline void doAnimate(int type, const QString &arg1 = QString(), const QString &arg2 = QString(),
                                     const QList<LegacyServerPlayer *> &players = QList<LegacyServerPlayer *>())
    {
        doAnimate((QSanProtocol::AnimateType)type, arg1, arg2, players);
    }
    QSGS_LOGIC void doBattleArrayAnimate(LegacyServerPlayer *player, LegacyServerPlayer *target = nullptr);

    QSGS_LOGIC void preparePlayers();
    QSGS_LOGIC void changePlayerGeneral(LegacyServerPlayer *player, const QString &new_general);
    QSGS_LOGIC void changePlayerGeneral2(LegacyServerPlayer *player, const QString &new_general);
    QSGS_LOGIC void filterCards(LegacyServerPlayer *player, QList<const Card *> cards, bool refilter);

    QSGS_LOGIC void acquireSkill(LegacyServerPlayer *player, const Skill *skill, bool open = true, bool head = true);
    QSGS_LOGIC void acquireSkill(LegacyServerPlayer *player, const QString &skill_name, bool open = true, bool head = true);

    QSGS_LOGIC void setPlayerSkillInvalidity(LegacyServerPlayer *player, const Skill *skill, bool invalidity, bool trigger_event = true);
    QSGS_LOGIC void setPlayerSkillInvalidity(LegacyServerPlayer *player, const QString &skill_name, bool invalidity, bool trigger_event = true);

    QSGS_LOGIC void adjustSeats();
    QSGS_LOGIC void swapPile();
    QSGS_LOGIC void changeHero(LegacyServerPlayer *player, const QString &new_general, bool full_state, bool invoke_start = true, bool isSecondaryHero = false,
                               bool sendLog = true);
    QSGS_LOGIC void swapSeat(LegacyServerPlayer *a, LegacyServerPlayer *b);
    lua_State *getLuaState() const;
    QSGS_LOGIC void setFixedDistance(Player *from, const Player *to, int distance);
    QSGS_LOGIC void reverseFor3v3(const Card *card, LegacyServerPlayer *player, QList<LegacyServerPlayer *> &list);
    QSGS_SOCKET void signup(LegacyServerPlayer *player, const QString &screen_name, const QString &avatar, bool is_robot);
    QSGS_STATE_ROOM LegacyServerPlayer *getOwner() const;
    QSGS_LOGIC void updateStateItem();

    QSGS_SOCKET void reconnect(LegacyServerPlayer *player, LegacyClientSocket *socket);
    QSGS_LOGIC void marshal(LegacyServerPlayer *player);

    // TODO: use sendLog() instead
    QSGS_LOGIC void touhouLogmessage(const QString &logtype, LegacyServerPlayer *logfrom, const QString &logarg = QString(),
                                     const QList<LegacyServerPlayer *> &logto = QList<LegacyServerPlayer *>(), const QString &logarg2 = QString());

    QSGS_LOGIC void drawCards(LegacyServerPlayer *player, int n, const QString &reason = QString());
    QSGS_LOGIC void drawCards(const QList<LegacyServerPlayer *> &players, int n, const QString &reason = QString());
    QSGS_LOGIC void drawCards(QList<LegacyServerPlayer *> players, const QList<int> &n_list, const QString &reason = QString());
    QSGS_LOGIC void obtainCard(LegacyServerPlayer *target, const Card *card, bool unhide = true);
    QSGS_LOGIC void obtainCard(LegacyServerPlayer *target, int card_id, bool unhide = true);
    QSGS_LOGIC void obtainCard(LegacyServerPlayer *target, const Card *card, const CardMoveReason &reason, bool unhide = true);

    QSGS_LOGIC void throwCard(int card_id, LegacyServerPlayer *who, LegacyServerPlayer *thrower = nullptr, bool notifyLog = true);
    QSGS_LOGIC void throwCard(const Card *card, LegacyServerPlayer *who, LegacyServerPlayer *thrower = nullptr, bool notifyLog = true);
    QSGS_LOGIC void throwCard(const Card *card, const CardMoveReason &reason, LegacyServerPlayer *who, LegacyServerPlayer *thrower = nullptr, bool notifyLog = true);

    QSGS_LOGIC void moveCardTo(const Card *card, LegacyServerPlayer *dstPlayer, QSanguosha::Place dstPlace, bool forceMoveVisible = false);
    QSGS_LOGIC void moveCardTo(const Card *card, LegacyServerPlayer *dstPlayer, QSanguosha::Place dstPlace, const CardMoveReason &reason, bool forceMoveVisible = false);
    QSGS_LOGIC void moveCardTo(const Card *card, LegacyServerPlayer *srcPlayer, LegacyServerPlayer *dstPlayer, QSanguosha::Place dstPlace, const CardMoveReason &reason,
                               bool forceMoveVisible = false);
    QSGS_LOGIC void moveCardTo(const Card *card, LegacyServerPlayer *srcPlayer, LegacyServerPlayer *dstPlayer, QSanguosha::Place dstPlace, const QString &pileName,
                               const CardMoveReason &reason, bool forceMoveVisible = false);
    QSGS_LOGIC void moveCardsAtomic(QList<LegacyCardsMoveStruct> cards_move, bool forceMoveVisible);
    QSGS_LOGIC void moveCardsAtomic(const LegacyCardsMoveStruct &cards_move, bool forceMoveVisible);
    QSGS_LOGIC void moveCardsToEndOfDrawpile(const QList<int> &card_ids, bool forceVisible = false);
    QSGS_LOGIC QList<LegacyCardsMoveStruct> _breakDownCardMoves(QList<LegacyCardsMoveStruct> &cards_moves);

    // interactive methods
    QSGS_LOGIC void activate(LegacyServerPlayer *player, CardUseStruct &card_use);
    QSGS_LOGIC void askForLuckCard();
    QSGS_LOGIC QSanguosha::Suit askForSuit(LegacyServerPlayer *player, const QString &reason);
    QSGS_LOGIC QString askForKingdom(LegacyServerPlayer *player);
    QSGS_LOGIC bool askForSkillInvoke(LegacyServerPlayer *player, const QString &skill_name, const QVariant &data = QVariant(), const QString &prompt = QString());
    QSGS_LOGIC bool askForSkillInvoke(LegacyServerPlayer *player, const Skill *skill, const QVariant &data = QVariant(), const QString &prompt = QString());
    QSGS_LOGIC QString askForChoice(LegacyServerPlayer *player, const QString &skill_name, const QString &choices, const QVariant &data = QVariant());
    // TODO: Add pattern to askForDiscard (QSanguosha::MethodDiscard)
    QSGS_LOGIC bool askForDiscard(LegacyServerPlayer *target, const QString &reason, int discard_num, int min_num, bool optional = false, bool include_equip = false,
                                  const QString &prompt = QString());
    QSGS_LOGIC void doJileiShow(LegacyServerPlayer *player, QList<int> jilei_ids);
    // TODO: Add pattern to askForExchange (QSanguosha::MethodNone)
    QSGS_LOGIC const Card *askForExchange(LegacyServerPlayer *player, const QString &reason, int discard_num, int min_num, bool include_equip = false,
                                          const QString &prompt = QString(), bool optional = false);
    QSGS_LOGIC bool askForNullification(const Card *trick, LegacyServerPlayer *from, LegacyServerPlayer *to, bool positive);
    QSGS_LOGIC bool isCanceled(const CardEffectStruct &effect);
    QSGS_LOGIC int askForCardChosen(LegacyServerPlayer *player, LegacyServerPlayer *who, const QString &flags, const QString &reason, bool handcard_visible = false,
                                    QSanguosha::HandlingMethod method = QSanguosha::MethodNone, const QList<int> &disabled_ids = QList<int>());
    // TODO: Break askForCard to askForResponse, and askForJink.
    QSGS_LOGIC const Card *askForCard(LegacyServerPlayer *player, const QString &pattern, const QString &prompt, const QVariant &data, const QString &skill_name,
                                      int notice_index = -1);
    QSGS_LOGIC const Card *askForCard(LegacyServerPlayer *player, const QString &pattern, const QString &prompt, const QVariant &data = QVariant(),
                                      QSanguosha::HandlingMethod method = QSanguosha::MethodDiscard, LegacyServerPlayer *to = nullptr, bool isRetrial = false,
                                      const QString &skill_name = QString(), bool isProvision = false, int notice_index = -1);
    // askForUseCard / askForUseSlashTo (QSanguosha::MethodUse)
    // TODO: askForUseCard supports the recast here. (Add additional parameter to this function, or change method -> QList<QSanguosha::HandlingMethod>)
    QSGS_LOGIC const Card *askForUseCard(LegacyServerPlayer *player, const QString &pattern, const QString &prompt, int notice_index = -1,
                                         QSanguosha::HandlingMethod method = QSanguosha::MethodUse, bool addHistory = true, const QString &skill_name = QString());
    QSGS_LOGIC const Card *askForUseSlashTo(LegacyServerPlayer *slasher, LegacyServerPlayer *victim, const QString &prompt, bool distance_limit = true, bool disable_extra = false,
                                            bool addHistory = false);
    QSGS_LOGIC const Card *askForUseSlashTo(LegacyServerPlayer *slasher, QList<LegacyServerPlayer *> victims, const QString &prompt, bool distance_limit = true,
                                            bool disable_extra = false, bool addHistory = false);
    QSGS_LOGIC int askForAG(LegacyServerPlayer *player, const QList<int> &card_ids, bool refusable, const QString &reason);
    QSGS_LOGIC void doExtraAmazingGrace(LegacyServerPlayer *from, LegacyServerPlayer *target, int times);
    QSGS_LOGIC const Card *askForCardShow(LegacyServerPlayer *player, LegacyServerPlayer *requestor, const QString &reason);
    QSGS_LOGIC int askForRende(LegacyServerPlayer *liubei, QList<int> &cards, const QString &skill_name = QString(), bool visible = false, bool optional = true, int max_num = -1,
                               QList<LegacyServerPlayer *> players = QList<LegacyServerPlayer *>(), CardMoveReason reason = CardMoveReason(), const QString &prompt = QString(),
                               bool notify_skill = false);
    QSGS_LOGIC bool askForYiji(LegacyServerPlayer *guojia, QList<int> &cards, const QString &skill_name = QString(), bool is_preview = false, bool visible = false,
                               bool optional = true, int max_num = -1, QList<LegacyServerPlayer *> players = QList<LegacyServerPlayer *>(),
                               CardMoveReason reason = CardMoveReason(), const QString &prompt = QString(), bool notify_skill = false);
    QSGS_LOGIC const Card *askForPindian(LegacyServerPlayer *player, LegacyServerPlayer *from, LegacyServerPlayer *to, const QString &reason, PindianStruct *pindian);
    QSGS_LOGIC QList<const Card *> askForPindianRace(LegacyServerPlayer *from, LegacyServerPlayer *to, const QString &reason);
    QSGS_LOGIC LegacyServerPlayer *askForPlayerChosen(LegacyServerPlayer *player, const QList<LegacyServerPlayer *> &targets, const QString &reason,
                                                      const QString &prompt = QString(), bool optional = false, bool notify_skill = false);
    QSGS_LOGIC QString askForGeneral(LegacyServerPlayer *player, const QStringList &generals, QString default_choice = QString());
    QSGS_LOGIC QString askForGeneral(LegacyServerPlayer *player, const QString &generals, const QString &default_choice = QString());
    QSGS_LOGIC void askForSinglePeach(LegacyServerPlayer *player, LegacyServerPlayer *dying, CardUseStruct &use);
    QSGS_LOGIC QSharedPointer<TriggerDetail> askForTriggerOrder(LegacyServerPlayer *player, const QList<QSharedPointer<TriggerDetail>> &sameTiming, bool cancelable,
                                                                const QVariant &data);
    QSGS_LOGIC void addPlayerHistory(LegacyServerPlayer *player, const QString &key, int times = 1);
    QSGS_LOGIC void transformGeneral(LegacyServerPlayer *player, const QString &general_name, int head);

    QSGS_SOCKET void toggleReadyCommand(LegacyServerPlayer *player, const QJsonValue &);
    QSGS_SOCKET void speakCommand(LegacyServerPlayer *player, const QJsonValue &arg);
    QSGS_SOCKET void trustCommand(LegacyServerPlayer *player, const QJsonValue &arg);
    QSGS_SOCKET void pauseCommand(LegacyServerPlayer *player, const QJsonValue &arg);
    QSGS_SOCKET void processResponse(LegacyServerPlayer *player, const QSanProtocol::Packet *arg);
    QSGS_SOCKET void addRobotCommand(LegacyServerPlayer *player, const QJsonValue &arg);
    QSGS_SOCKET void fillRobotsCommand(LegacyServerPlayer *player, const QJsonValue &arg);
    QSGS_SOCKET void broadcastInvoke(const QSanProtocol::Packet *packet, LegacyServerPlayer *except = nullptr);
    QSGS_SOCKET void broadcastInvoke(const char *method, const QString &arg = QStringLiteral("."), LegacyServerPlayer *except = nullptr);
    QSGS_SOCKET void networkDelayTestCommand(LegacyServerPlayer *player, const QJsonValue &);
    QSGS_LOGIC bool roleStatusCommand(LegacyServerPlayer *player);

    QSGS_LOGIC void updateCardsOnLose(const LegacyCardsMoveStruct &move);
    QSGS_LOGIC void updateCardsOnGet(const LegacyCardsMoveStruct &move);

    QSGS_LOGIC void cheat(LegacyServerPlayer *player, const QJsonValue &args);
    QSGS_LOGIC bool makeSurrender(LegacyServerPlayer *player);

protected:
    int _m_Id;

private:
    struct _MoveSourceClassifier
    {
        explicit inline _MoveSourceClassifier(const LegacyCardsMoveStruct &move)
        {
            m_from = move.from;
            m_from_place = move.from_place;
            m_from_pile_name = move.from_pile_name;
            m_from_player_name = move.from_player_name;
        }
        inline void copyTo(LegacyCardsMoveStruct &move) const
        {
            move.from = m_from;
            move.from_place = m_from_place;
            move.from_pile_name = m_from_pile_name;
            move.from_player_name = m_from_player_name;
        }
        inline bool operator==(const _MoveSourceClassifier &other) const
        {
            return m_from == other.m_from && m_from_place == other.m_from_place && m_from_pile_name == other.m_from_pile_name && m_from_player_name == other.m_from_player_name;
        }
        inline bool operator<(const _MoveSourceClassifier &other) const
        {
            return m_from < other.m_from || m_from_place < other.m_from_place || m_from_pile_name < other.m_from_pile_name || m_from_player_name < other.m_from_player_name;
        }
        Player *m_from;
        QSanguosha::Place m_from_place;
        QString m_from_pile_name;
        QString m_from_player_name;
    };

    struct _MoveMergeClassifier
    {
        explicit inline _MoveMergeClassifier(const LegacyCardsMoveStruct &move)
        {
            m_from = move.from;
            m_to = move.to;
            m_to_place = move.to_place;
            m_to_pile_name = move.to_pile_name;

            m_origin_from = move.origin_from;
            m_origin_to = move.origin_to;
            m_origin_to_place = move.origin_to_place;
            m_origin_to_pile_name = move.origin_to_pile_name;
        }
        inline bool operator==(const _MoveMergeClassifier &other) const
        {
            return m_from == other.m_from && m_to == other.m_to && m_to_place == other.m_to_place && m_to_pile_name == other.m_to_pile_name;
        }
        inline bool operator<(const _MoveMergeClassifier &other) const
        {
            return m_from < other.m_from || m_to < other.m_to || m_to_place < other.m_to_place || m_to_pile_name < other.m_to_pile_name;
        }
        Player *m_from;
        Player *m_to;
        QSanguosha::Place m_to_place;
        QString m_to_pile_name;

        Player *m_origin_from;
        Player *m_origin_to;
        QSanguosha::Place m_origin_to_place;
        QString m_origin_to_pile_name;
    };

    struct _MoveSeparateClassifier
    {
        inline _MoveSeparateClassifier(const LegacyCardsMoveOneTimeStruct &moveOneTime, int index)
        {
            m_from = moveOneTime.from;
            m_to = moveOneTime.to;
            m_from_place = moveOneTime.from_places[index];
            m_to_place = moveOneTime.to_place;
            m_from_pile_name = moveOneTime.from_pile_names[index];
            m_to_pile_name = moveOneTime.to_pile_name;
            m_open = moveOneTime.open[index];
            m_reason = moveOneTime.reason;
            m_is_last_handcard = moveOneTime.is_last_handcard;
        }

        inline bool operator==(const _MoveSeparateClassifier &other) const
        {
            return m_from == other.m_from && m_to == other.m_to && m_from_place == other.m_from_place && m_to_place == other.m_to_place
                && m_from_pile_name == other.m_from_pile_name && m_to_pile_name == other.m_to_pile_name && m_open == other.m_open && m_reason == other.m_reason
                && m_is_last_handcard == other.m_is_last_handcard;
        }
        inline bool operator<(const _MoveSeparateClassifier &other) const
        {
            return m_from < other.m_from && m_to < other.m_to && m_from_place < other.m_from_place && m_to_place < other.m_to_place && m_from_pile_name < other.m_from_pile_name
                && m_to_pile_name < other.m_to_pile_name && m_open < other.m_open && m_is_last_handcard < other.m_is_last_handcard; // NOLINT
        }
        Player *m_from;
        Player *m_to;
        QSanguosha::Place m_from_place, m_to_place;
        QString m_from_pile_name, m_to_pile_name;
        bool m_open;
        CardMoveReason m_reason;
        bool m_is_last_handcard;
    };

    int _m_lastMovementId;
    void _fillMoveInfo(LegacyCardsMoveStruct &moves, int card_index) const;
    QList<LegacyCardsMoveOneTimeStruct> _mergeMoves(QList<LegacyCardsMoveStruct> cards_moves);
    QList<LegacyCardsMoveStruct> _separateMoves(QList<LegacyCardsMoveOneTimeStruct> moveOneTimes);
    QString _chooseDefaultGeneral(LegacyServerPlayer *player) const;
    QStringList _chooseDefaultGenerals(LegacyServerPlayer *player) const;
    bool _setPlayerGeneral(LegacyServerPlayer *player, const QString &generalName, bool isFirst);
    QList<LegacyServerPlayer *> m_players;
    QList<LegacyServerPlayer *> m_alivePlayers;
    int player_count;
    LegacyServerPlayer *current;
    QList<int> pile1;
    QList<int> table_cards;
    QList<int> *m_drawPile;
    QStack<DamageStruct> m_damageStack;
    bool game_started;
    bool game_started2;
    bool game_finished;
    bool game_paused;
    lua_State *L;
    bool fill_robot;

    RoomThread *thread;
    QSemaphore _m_semRaceRequest; // When race starts, server waits on his semaphore for the first replier
    QSemaphore _m_semRoomMutex; // Provide per-room  (rather than per-player) level protection of any shared variables

    QHash<QSanProtocol::CommandType, Callback> m_callbacks; // Stores the callbacks for client request. Do not use this
    // this map for anything else but S_CLIENT_REQUEST!!!!!
    QHash<QSanProtocol::CommandType, QSanProtocol::CommandType> m_requestResponsePair;
    // Stores the expected client response for each server request, any unmatched client response will be discarded.

    //helper variables for race request function
    bool _m_raceStarted;
    QAtomicPointer<LegacyServerPlayer> _m_raceWinner;

    QMap<int, QSanguosha::Place> place_map;
    QMap<int, LegacyServerPlayer *> owner_map;

    const Card *provided;
    bool has_provided;
    LegacyServerPlayer *provider;

    QVariantMap tag;

    QJsonValue m_fillAGarg;
    LegacyServerPlayer *m_fillAGWho;
    QJsonValue m_takeAGargs;

    QWaitCondition m_waitCond;
    mutable QMutex m_mutex;

    volatile bool playerPropertySet;

    static QString generatePlayerName();
    void prepareForStart();
    void assignGeneralsForPlayers(const QList<LegacyServerPlayer *> &to_assign);
    void chooseGenerals();
    void chooseHegemonyGenerals();
    void broadcast(const QString &message, LegacyServerPlayer *except = nullptr);
    void initCallbacks();
    QString askForOrder(LegacyServerPlayer *player, const QString &default_choice);
    QString askForRole(LegacyServerPlayer *player, const QStringList &roles, const QString &scheme);

    //process client requests

    bool makeCheat(LegacyServerPlayer *player);
    void makeDamage(const QString &source, const QString &target, QSanProtocol::CheatCategory nature, int point);
    void makeKilling(const QString &killer, const QString &victim);
    void makeReviving(const QString &name);
    void doScript(const QString &script);

    QSGS_SOCKET void skinChangeCommand(LegacyServerPlayer *player, const QJsonValue &packet);
    QSGS_SOCKET void heartbeatCommand(LegacyServerPlayer *player, const QJsonValue &packet);
    QSGS_SOCKET void processRequestPreshow(LegacyServerPlayer *player, const QJsonValue &arg); //hegemony

    //helper functions and structs
    struct _NullificationAiHelper
    {
        const Card *m_trick;
        LegacyServerPlayer *m_from;
        LegacyServerPlayer *m_to;
    };
    bool _askForNullification(const Card *trick, LegacyServerPlayer *from, LegacyServerPlayer *to, bool positive, const _NullificationAiHelper &helper);
    void _setupChooseGeneralRequestArgs(LegacyServerPlayer *player);

private slots:
    void reportDisconnection();
    void processClientPacket(const QString &packet);
    void assignRoles();
    void startGame();
    void slotSetProperty(LegacyServerPlayer *player, const char *property_name, const QVariant &value);

signals:
    void room_message(const QString &msg);
    void game_over(const QString &winner);
    void signalSetProperty(LegacyServerPlayer *player, const char *property_name, const QVariant &value);
};

#undef QSGS_STATE_ROOM
#undef QSGS_STATE_GAME
#undef QSGS_LOGIC
#undef QSGS_SOCKET

#endif
