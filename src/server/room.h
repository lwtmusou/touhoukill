#ifndef _ROOM_H
#define _ROOM_H

#include <QAtomicPointer>
#include <QMutex>
#include <QStack>
#include <QWaitCondition>

#include "RoomObject.h"
#include "protocol.h"
#include "roomthread.h"
#include "serverplayer.h"

class TriggerSkill;
class ProhibitSkill;
class TrickCard;
class GeneralSelector;

struct lua_State;
struct LogMessage;

#define QSGS_STATE_ROOM
#define QSGS_STATE_GAME
#define QSGS_LOGIC
#define QSGS_SOCKET

class Room : public RoomObject
{
    Q_OBJECT
    Q_ENUMS(GuanxingType)

public:
    explicit Room(QObject *parent, const QString &mode);
    ~Room() override;
    RoomThread *getThread() const;
    void output(const QString &message);
    void outputEventStack();
    void saveWinnerTable(const QString &winner, bool isSurrender = false);
    void countDescription();
    void resetAI(ServerPlayer *player);

    friend class RoomThread;
    friend class RoomThread3v3;
    friend class RoomThreadXMode;
    friend class RoomThread1v1;

    QSGS_LOGIC enum GuanxingType { GuanxingUpOnly = 1, GuanxingBothSides = 0, GuanxingDownOnly = -1 };

    QSGS_SOCKET typedef void (Room::*Callback)(ServerPlayer *, const QVariant &);
    QSGS_SOCKET typedef bool (Room::*ResponseVerifyFunction)(ServerPlayer *, const QVariant &, void *);

    QSGS_SOCKET ServerPlayer *addSocket(ClientSocket *socket);

    QSGS_SOCKET inline int getId() const
    {
        return _m_Id;
    }
    QSGS_STATE_ROOM bool isFull() const;
    QSGS_STATE_GAME bool isFinished() const;
    QSGS_STATE_ROOM bool canPause(ServerPlayer *p) const;
    QSGS_STATE_ROOM void tryPause();

    QSGS_STATE_ROOM int getLack() const;
    QSGS_STATE_GAME QString getMode() const;
    QSGS_STATE_GAME ServerPlayer *getCurrent() const;
    QSGS_STATE_GAME void setCurrent(ServerPlayer *current);
    QSGS_STATE_GAME int alivePlayerCount() const;
    QSGS_STATE_GAME QList<ServerPlayer *> getOtherPlayers(ServerPlayer *except, bool include_dead = false) const;
    QSGS_STATE_GAME QList<ServerPlayer *> getPlayers() const;
    QSGS_STATE_GAME QList<ServerPlayer *> getAllPlayers(bool include_dead = false) const;
    QSGS_STATE_GAME QList<ServerPlayer *> getAlivePlayers() const;
    QSGS_LOGIC void enterDying(ServerPlayer *player, DamageStruct *reason);
    QSGS_STATE_GAME ServerPlayer *getCurrentDyingPlayer() const;
    QSGS_LOGIC void killPlayer(ServerPlayer *victim, DamageStruct *reason = nullptr);
    QSGS_LOGIC void revivePlayer(ServerPlayer *player, bool initialize = true);
    QSGS_STATE_GAME QStringList aliveRoles(ServerPlayer *except = nullptr) const;
    QSGS_LOGIC void gameOver(const QString &winner, bool isSurrender = false);
    QSGS_LOGIC void slashEffect(const SlashEffectStruct &effect);
    QSGS_LOGIC void slashResult(const SlashEffectStruct &effect, const Card *jink);
    QSGS_LOGIC void attachSkillToPlayer(ServerPlayer *player, const QString &skill_name, bool is_other_attach = false);

    QSGS_LOGIC void detachSkillFromPlayer(ServerPlayer *player, const QString &skill_name, bool is_equip = false, bool acquire_only = false, bool sendlog = true, bool head = true);
    QSGS_LOGIC void handleAcquireDetachSkills(ServerPlayer *player, const QStringList &skill_names, bool acquire_only = false);
    QSGS_LOGIC void handleAcquireDetachSkills(ServerPlayer *player, const QString &skill_names, bool acquire_only = false);
    QSGS_LOGIC void setPlayerFlag(ServerPlayer *player, const QString &flag);
    QSGS_LOGIC void setPlayerProperty(ServerPlayer *player, const char *property_name, const QVariant &value);
    QSGS_LOGIC void setPlayerMark(ServerPlayer *player, const QString &mark, int value);
    QSGS_LOGIC void addPlayerMark(ServerPlayer *player, const QString &mark, int add_num = 1);
    QSGS_LOGIC void removePlayerMark(ServerPlayer *player, const QString &mark, int remove_num = 1);
    QSGS_LOGIC void setPlayerCardLimitation(ServerPlayer *player, const QString &limit_list, const QString &pattern, const QString &reason, bool single_turn);
    QSGS_LOGIC void removePlayerCardLimitation(ServerPlayer *player, const QString &limit_list, const QString &pattern, const QString &reason, bool clearReason = false);
    QSGS_LOGIC void clearPlayerCardLimitation(ServerPlayer *player, bool single_turn);
    QSGS_LOGIC void setPlayerDisableShow(ServerPlayer *player, const QString &flags, const QString &reason);
    QSGS_LOGIC void removePlayerDisableShow(ServerPlayer *player, const QString &reason);
    QSGS_LOGIC void setCardFlag(const Card *card, const QString &flag, ServerPlayer *who = nullptr);
    QSGS_LOGIC void setCardFlag(int card_id, const QString &flag, ServerPlayer *who = nullptr);
    QSGS_LOGIC void clearCardFlag(const Card *card, ServerPlayer *who = nullptr);
    QSGS_LOGIC void clearCardFlag(int card_id, ServerPlayer *who = nullptr);
    QSGS_LOGIC bool useCard(const CardUseStruct &card_use, bool add_history = true);
    QSGS_LOGIC void damage(const DamageStruct &data);
    QSGS_LOGIC void sendDamageLog(const DamageStruct &data);
    QSGS_LOGIC void loseHp(ServerPlayer *victim, int lose = 1);
    QSGS_LOGIC void loseMaxHp(ServerPlayer *victim, int lose = 1);
    QSGS_LOGIC bool changeMaxHpForAwakenSkill(ServerPlayer *player, int magnitude = -1);
    QSGS_LOGIC void applyDamage(ServerPlayer *victim, const DamageStruct &damage);
    QSGS_LOGIC void recover(ServerPlayer *player, const RecoverStruct &recover, bool set_emotion = false);
    QSGS_LOGIC bool cardEffect(const Card *card, ServerPlayer *from, ServerPlayer *to, bool multiple = false);
    QSGS_LOGIC bool cardEffect(const CardEffectStruct &effect);
    QSGS_LOGIC bool isJinkEffected(SlashEffectStruct effect, const Card *jink);
    QSGS_LOGIC void judge(JudgeStruct &judge_struct);
    QSGS_LOGIC void sendJudgeResult(const JudgeStruct *judge);
    QSGS_LOGIC QList<int> getNCards(int n, bool update_pile_number = true, bool bottom = false);
    QSGS_LOGIC void returnToTopDrawPile(const QList<int> &cards);
    QSGS_STATE_GAME ServerPlayer *getLord(const QString &kingdom = "wei", bool include_death = false) const;
    QSGS_LOGIC void askForGuanxing(ServerPlayer *zhuge, const QList<int> &cards, GuanxingType guanxing_type = GuanxingBothSides, QString skillName = "");
    QSGS_LOGIC int doGongxin(ServerPlayer *shenlvmeng, ServerPlayer *target, QList<int> enabled_ids = QList<int>(), QString skill_name = "gongxin");
    QSGS_LOGIC int drawCard(bool bottom = false);
    QSGS_LOGIC void fillAG(const QList<int> &card_ids, ServerPlayer *who = nullptr, const QList<int> &disabled_ids = QList<int>(),
                           const QList<int> &shownHandcard_ids = QList<int>());
    QSGS_LOGIC void takeAG(ServerPlayer *player, int card_id, bool move_cards = true, QList<ServerPlayer *> to_notify = QList<ServerPlayer *>(),
                           Player::Place fromPlace = Player::DrawPile);
    QSGS_LOGIC void clearAG(ServerPlayer *player = nullptr);
    QSGS_LOGIC void provide(const Card *card, ServerPlayer *who = nullptr);
    QSGS_STATE_GAME QList<ServerPlayer *> getLieges(const QString &kingdom, ServerPlayer *lord) const;
    QSGS_LOGIC void sendLog(const LogMessage &log);
    QSGS_LOGIC void showCard(ServerPlayer *player, int card_id, ServerPlayer *only_viewer = nullptr);
    QSGS_LOGIC void showAllCards(ServerPlayer *player, ServerPlayer *to = nullptr);
    QSGS_LOGIC void retrial(const Card *card, ServerPlayer *player, JudgeStruct *judge, const QString &skill_name, bool exchange = false);

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
    QSGS_SOCKET bool doRequest(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg, time_t timeOut, bool wait);
    QSGS_SOCKET bool doRequest(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg, bool wait);

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
    QSGS_SOCKET bool doBroadcastRequest(QList<ServerPlayer *> &players, QSanProtocol::CommandType command, time_t timeOut);
    QSGS_SOCKET bool doBroadcastRequest(QList<ServerPlayer *> &players, QSanProtocol::CommandType command);

    // Broadcast a request to a list of players and get the first valid client response. Call is blocking until the first
    // client response is received or server times out, whichever is earlier. Any client response is verified by the validation
    // function and argument passed in. When a response is verified to be invalid, the function will continue to wait for
    // the next client response.
    // @param validateFunc
    //        Validation function that verifies whether the reply is a valid one. The first parameter passed to the function
    //        is the response sender, the second parameter is the response content, the third parameter is funcArg passed in.
    // @return The player that first send a legal request to the server. NULL if no such request is received.
    QSGS_SOCKET ServerPlayer *doBroadcastRaceRequest(QList<ServerPlayer *> &players, QSanProtocol::CommandType command, time_t timeOut,
                                                     ResponseVerifyFunction validateFunc = nullptr, void *funcArg = nullptr);

    // Notify a player of a event by sending S_SERVER_NOTIFICATION packets. No reply should be expected from
    // the client for S_SERVER_NOTIFICATION as it's a one way notice. Any message from the client in reply to this call
    // will be rejected.
    QSGS_SOCKET bool doNotify(ServerPlayer *player, QSanProtocol::CommandType command, const QVariant &arg);

    // Broadcast a event to a list of players by sending S_SERVER_NOTIFICATION packets. No replies should be expected from
    // the clients for S_SERVER_NOTIFICATION as it's a one way notice. Any message from the client in reply to this call
    // will be rejected.
    QSGS_SOCKET bool doBroadcastNotify(QSanProtocol::CommandType command, const QVariant &arg);
    QSGS_SOCKET bool doBroadcastNotify(const QList<ServerPlayer *> &players, QSanProtocol::CommandType command, const QVariant &arg);

    QSGS_SOCKET bool doNotify(ServerPlayer *player, int command, const char *arg);
    QSGS_SOCKET bool doBroadcastNotify(int command, const char *arg);
    QSGS_SOCKET bool doBroadcastNotify(const QList<ServerPlayer *> &players, int command, const char *arg);

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
    QSGS_SOCKET bool getResult(ServerPlayer *player, time_t timeOut);
    QSGS_SOCKET ServerPlayer *getRaceResult(QList<ServerPlayer *> &players, QSanProtocol::CommandType command, time_t timeOut, ResponseVerifyFunction validateFunc = nullptr,
                                            void *funcArg = nullptr);

    // Verification functions
    bool verifyNullificationResponse(ServerPlayer *, const QVariant &, void *);

    // Notification functions
    QSGS_LOGIC bool notifyMoveFocus(ServerPlayer *player);
    QSGS_LOGIC bool notifyMoveFocus(ServerPlayer *player, QSanProtocol::CommandType command);
    QSGS_LOGIC bool notifyMoveFocus(const QList<ServerPlayer *> &players, QSanProtocol::CommandType command, QSanProtocol::Countdown countdown);

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
    bool notifyMoveCards(bool isLostPhase, QList<CardsMoveStruct> move, bool forceVisible, QList<ServerPlayer *> players = QList<ServerPlayer *>());
    bool notifyProperty(ServerPlayer *playerToNotify, const ServerPlayer *propertyOwner, const char *propertyName, const QString &value = QString());
    QSGS_LOGIC bool notifyUpdateCard(ServerPlayer *player, int cardId, const Card *newCard);
    QSGS_LOGIC bool broadcastUpdateCard(const QList<ServerPlayer *> &players, int cardId, const Card *newCard);
    QSGS_LOGIC bool notifyResetCard(ServerPlayer *player, int cardId);
    QSGS_LOGIC bool broadcastResetCard(const QList<ServerPlayer *> &players, int cardId);

    QSGS_LOGIC bool broadcastProperty(ServerPlayer *player, const char *property_name, const QString &value = QString());
    QSGS_LOGIC void notifySkillInvoked(ServerPlayer *player, const QString &skill_name);
    QSGS_LOGIC void broadcastSkillInvoke(const QString &skillName);
    QSGS_LOGIC void broadcastSkillInvoke(const QString &skillName, const QString &category);
    QSGS_LOGIC void broadcastSkillInvoke(const QString &skillName, int type);
    QSGS_LOGIC void broadcastSkillInvoke(const QString &skillName, bool isMale, int type);
    QSGS_LOGIC void doLightbox(const QString &lightboxName, int duration = 2000);
    QSGS_LOGIC void doAnimate(QSanProtocol::AnimateType type, const QString &arg1 = QString(), const QString &arg2 = QString(),
                              QList<ServerPlayer *> players = QList<ServerPlayer *>());

    QSGS_LOGIC inline void doAnimate(int type, const QString &arg1 = QString(), const QString &arg2 = QString(), const QList<ServerPlayer *> &players = QList<ServerPlayer *>())
    {
        doAnimate((QSanProtocol::AnimateType)type, arg1, arg2, players);
    }
    QSGS_LOGIC void doBattleArrayAnimate(ServerPlayer *player, ServerPlayer *target = nullptr);

    QSGS_LOGIC void preparePlayers();
    QSGS_LOGIC void changePlayerGeneral(ServerPlayer *player, const QString &new_general);
    QSGS_LOGIC void changePlayerGeneral2(ServerPlayer *player, const QString &new_general);
    QSGS_LOGIC void filterCards(ServerPlayer *player, QList<const Card *> cards, bool refilter);

    QSGS_LOGIC void acquireSkill(ServerPlayer *player, const Skill *skill, bool open = true, bool head = true);
    QSGS_LOGIC void acquireSkill(ServerPlayer *player, const QString &skill_name, bool open = true, bool head = true);

    QSGS_LOGIC void setPlayerSkillInvalidity(ServerPlayer *player, const Skill *skill, bool invalidity, bool trigger_event = true);
    QSGS_LOGIC void setPlayerSkillInvalidity(ServerPlayer *player, const QString &skill_name, bool invalidity, bool trigger_event = true);

    QSGS_LOGIC void adjustSeats();
    QSGS_LOGIC void swapPile();
    QSGS_STATE_GAME QList<int> &getDiscardPile() override;
    QSGS_STATE_GAME const QList<int> &getDiscardPile() const override;
    QSGS_STATE_GAME inline QList<int> &getDrawPile()
    {
        return *m_drawPile;
    }
    QSGS_STATE_GAME inline const QList<int> &getDrawPile() const
    {
        return *m_drawPile;
    }
    QSGS_STATE_GAME ServerPlayer *findPlayer(const QString &general_name, bool include_dead = false) const;
    QSGS_STATE_GAME QList<ServerPlayer *> findPlayersBySkillName(const QString &skill_name, bool include_hidden = true) const;
    QSGS_STATE_GAME ServerPlayer *findPlayerBySkillName(const QString &skill_name) const;
    QSGS_STATE_GAME ServerPlayer *findPlayerByObjectName(const QString &name, bool include_dead = false) const;
    QSGS_LOGIC void changeHero(ServerPlayer *player, const QString &new_general, bool full_state, bool invoke_start = true, bool isSecondaryHero = false, bool sendLog = true);
    QSGS_LOGIC void swapSeat(ServerPlayer *a, ServerPlayer *b);
    lua_State *getLuaState() const;
    QSGS_LOGIC void setFixedDistance(Player *from, const Player *to, int distance);
    QSGS_LOGIC void reverseFor3v3(const Card *card, ServerPlayer *player, QList<ServerPlayer *> &list);
    QSGS_STATE_GAME bool hasWelfare(const ServerPlayer *player) const;
    QSGS_STATE_GAME ServerPlayer *getFront(ServerPlayer *a, ServerPlayer *b) const;
    QSGS_SOCKET void signup(ServerPlayer *player, const QString &screen_name, const QString &avatar, bool is_robot);
    QSGS_STATE_ROOM ServerPlayer *getOwner() const;
    QSGS_LOGIC void updateStateItem();

    QSGS_SOCKET void reconnect(ServerPlayer *player, ClientSocket *socket);
    QSGS_LOGIC void marshal(ServerPlayer *player);

    QSGS_STATE_GAME void sortByActionOrder(QList<ServerPlayer *> &players);
    void defaultHeroSkin();

    // TODO: use sendLog() instead
    QSGS_LOGIC void touhouLogmessage(const QString &logtype, ServerPlayer *logfrom, const QString &logarg = QString(), const QList<ServerPlayer *> &logto = QList<ServerPlayer *>(),
                                     const QString &logarg2 = QString());

    QSGS_STATE_GAME const ProhibitSkill *isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;

    QSGS_STATE_GAME void setTag(const QString &key, const QVariant &value);
    QSGS_STATE_GAME QVariant getTag(const QString &key) const;
    QSGS_STATE_GAME void removeTag(const QString &key);
    QSGS_STATE_GAME QStringList getTagNames() const;

    QSGS_LOGIC void setEmotion(ServerPlayer *target, const QString &emotion);

    QSGS_STATE_GAME Player::Place getCardPlace(int card_id) const;
    QSGS_STATE_GAME ServerPlayer *getCardOwner(int card_id) const;
    QSGS_STATE_GAME void setCardMapping(int card_id, ServerPlayer *owner, Player::Place place);
    QSGS_STATE_GAME QList<int> getCardIdsOnTable(const Card *) const;
    QSGS_STATE_GAME QList<int> getCardIdsOnTable(const QList<int> &card_ids) const;

    QSGS_LOGIC void drawCards(ServerPlayer *player, int n, const QString &reason = QString());
    QSGS_LOGIC void drawCards(QList<ServerPlayer *> players, int n, const QString &reason = QString());
    QSGS_LOGIC void drawCards(QList<ServerPlayer *> players, QList<int> n_list, const QString &reason = QString());
    QSGS_LOGIC void obtainCard(ServerPlayer *target, const Card *card, bool unhide = true);
    QSGS_LOGIC void obtainCard(ServerPlayer *target, int card_id, bool unhide = true);
    QSGS_LOGIC void obtainCard(ServerPlayer *target, const Card *card, const CardMoveReason &reason, bool unhide = true);

    QSGS_LOGIC void throwCard(int card_id, ServerPlayer *who, ServerPlayer *thrower = nullptr, bool notifyLog = true);
    QSGS_LOGIC void throwCard(const Card *card, ServerPlayer *who, ServerPlayer *thrower = nullptr, bool notifyLog = true);
    QSGS_LOGIC void throwCard(const Card *card, const CardMoveReason &reason, ServerPlayer *who, ServerPlayer *thrower = nullptr, bool notifyLog = true);

    QSGS_LOGIC void moveCardTo(const Card *card, ServerPlayer *dstPlayer, Player::Place dstPlace, bool forceMoveVisible = false);
    QSGS_LOGIC void moveCardTo(const Card *card, ServerPlayer *dstPlayer, Player::Place dstPlace, const CardMoveReason &reason, bool forceMoveVisible = false);
    QSGS_LOGIC void moveCardTo(const Card *card, ServerPlayer *srcPlayer, ServerPlayer *dstPlayer, Player::Place dstPlace, const CardMoveReason &reason,
                               bool forceMoveVisible = false);
    QSGS_LOGIC void moveCardTo(const Card *card, ServerPlayer *srcPlayer, ServerPlayer *dstPlayer, Player::Place dstPlace, const QString &pileName, const CardMoveReason &reason,
                               bool forceMoveVisible = false);
    QSGS_LOGIC void moveCardsAtomic(QList<CardsMoveStruct> cards_move, bool forceMoveVisible);
    QSGS_LOGIC void moveCardsAtomic(CardsMoveStruct cards_move, bool forceMoveVisible);
    QSGS_LOGIC void moveCardsToEndOfDrawpile(QList<int> card_ids, bool forceVisible = false);
    QSGS_LOGIC QList<CardsMoveStruct> _breakDownCardMoves(QList<CardsMoveStruct> &cards_moves);

    // interactive methods
    QSGS_LOGIC void activate(ServerPlayer *player, CardUseStruct &card_use);
    QSGS_LOGIC void askForLuckCard();
    QSGS_LOGIC Card::Suit askForSuit(ServerPlayer *player, const QString &reason);
    QSGS_LOGIC QString askForKingdom(ServerPlayer *player);
    QSGS_LOGIC bool askForSkillInvoke(ServerPlayer *player, const QString &skill_name, const QVariant &data = QVariant(), const QString &prompt = QString());
    QSGS_LOGIC bool askForSkillInvoke(ServerPlayer *player, const Skill *skill, const QVariant &data = QVariant(), const QString &prompt = QString());
    QSGS_LOGIC QString askForChoice(ServerPlayer *player, const QString &skill_name, const QString &choices, const QVariant &data = QVariant());
    QSGS_LOGIC bool askForDiscard(ServerPlayer *target, const QString &reason, int discard_num, int min_num, bool optional = false, bool include_equip = false,
                                  const QString &prompt = QString());
    QSGS_LOGIC void doJileiShow(ServerPlayer *player, QList<int> jilei_ids);
    QSGS_LOGIC const Card *askForExchange(ServerPlayer *player, const QString &reason, int discard_num, int min_num, bool include_equip = false, const QString &prompt = QString(),
                                          bool optional = false);
    QSGS_LOGIC bool askForNullification(const Card *trick, ServerPlayer *from, ServerPlayer *to, bool positive);
    QSGS_LOGIC bool isCanceled(const CardEffectStruct &effect);
    QSGS_LOGIC int askForCardChosen(ServerPlayer *player, ServerPlayer *who, const QString &flags, const QString &reason, bool handcard_visible = false,
                                    Card::HandlingMethod method = Card::MethodNone, const QList<int> &disabled_ids = QList<int>());
    QSGS_LOGIC const Card *askForCard(ServerPlayer *player, const QString &pattern, const QString &prompt, const QVariant &data, const QString &skill_name, int notice_index = -1);
    QSGS_LOGIC const Card *askForCard(ServerPlayer *player, const QString &pattern, const QString &prompt, const QVariant &data = QVariant(),
                                      Card::HandlingMethod method = Card::MethodDiscard, ServerPlayer *to = nullptr, bool isRetrial = false, const QString &skill_name = QString(),
                                      bool isProvision = false, int notice_index = -1);
    QSGS_LOGIC const Card *askForUseCard(ServerPlayer *player, const QString &pattern, const QString &prompt, int notice_index = -1, Card::HandlingMethod method = Card::MethodUse,
                                         bool addHistory = true, const QString &skill_name = QString());
    QSGS_LOGIC const Card *askForUseSlashTo(ServerPlayer *slasher, ServerPlayer *victim, const QString &prompt, bool distance_limit = true, bool disable_extra = false,
                                            bool addHistory = false);
    QSGS_LOGIC const Card *askForUseSlashTo(ServerPlayer *slasher, QList<ServerPlayer *> victims, const QString &prompt, bool distance_limit = true, bool disable_extra = false,
                                            bool addHistory = false);
    QSGS_LOGIC int askForAG(ServerPlayer *player, const QList<int> &card_ids, bool refusable, const QString &reason);
    QSGS_LOGIC void doExtraAmazingGrace(ServerPlayer *from, ServerPlayer *target, int times);
    QSGS_LOGIC const Card *askForCardShow(ServerPlayer *player, ServerPlayer *requestor, const QString &reason);
    QSGS_LOGIC int askForRende(ServerPlayer *liubei, QList<int> &cards, const QString &skill_name = QString(), bool visible = false, bool optional = true, int max_num = -1,
                               QList<ServerPlayer *> players = QList<ServerPlayer *>(), CardMoveReason reason = CardMoveReason(), const QString &prompt = QString(),
                               bool notify_skill = false);
    QSGS_LOGIC bool askForYiji(ServerPlayer *guojia, QList<int> &cards, const QString &skill_name = QString(), bool is_preview = false, bool visible = false, bool optional = true,
                               int max_num = -1, QList<ServerPlayer *> players = QList<ServerPlayer *>(), CardMoveReason reason = CardMoveReason(),
                               const QString &prompt = QString(), bool notify_skill = false);
    QSGS_LOGIC const Card *askForPindian(ServerPlayer *player, ServerPlayer *from, ServerPlayer *to, const QString &reason, PindianStruct *pindian);
    QSGS_LOGIC QList<const Card *> askForPindianRace(ServerPlayer *from, ServerPlayer *to, const QString &reason);
    QSGS_LOGIC ServerPlayer *askForPlayerChosen(ServerPlayer *player, const QList<ServerPlayer *> &targets, const QString &reason, const QString &prompt = QString(),
                                                bool optional = false, bool notify_skill = false);
    QSGS_LOGIC QString askForGeneral(ServerPlayer *player, const QStringList &generals, QString default_choice = QString());
    QSGS_LOGIC QString askForGeneral(ServerPlayer *player, const QString &generals, QString default_choice = QString());
    QSGS_LOGIC const Card *askForSinglePeach(ServerPlayer *player, ServerPlayer *dying);
    QSGS_LOGIC QSharedPointer<SkillInvokeDetail> askForTriggerOrder(ServerPlayer *player, const QList<QSharedPointer<SkillInvokeDetail> > &sameTiming, bool cancelable,
                                                                    const QVariant &data);
    QSGS_LOGIC void addPlayerHistory(ServerPlayer *player, const QString &key, int times = 1);
    QSGS_LOGIC void transformGeneral(ServerPlayer *player, QString general_name, int head);

    QSGS_SOCKET void toggleReadyCommand(ServerPlayer *player, const QVariant &);
    QSGS_SOCKET void speakCommand(ServerPlayer *player, const QVariant &arg);
    QSGS_SOCKET void trustCommand(ServerPlayer *player, const QVariant &arg);
    QSGS_SOCKET void pauseCommand(ServerPlayer *player, const QVariant &arg);
    QSGS_SOCKET void processResponse(ServerPlayer *player, const QSanProtocol::Packet *arg);
    QSGS_SOCKET void addRobotCommand(ServerPlayer *player, const QVariant &arg);
    QSGS_SOCKET void fillRobotsCommand(ServerPlayer *player, const QVariant &arg);
    QSGS_SOCKET void broadcastInvoke(const QSanProtocol::AbstractPacket *packet, ServerPlayer *except = nullptr);
    QSGS_SOCKET void broadcastInvoke(const char *method, const QString &arg = ".", ServerPlayer *except = nullptr);
    QSGS_SOCKET void networkDelayTestCommand(ServerPlayer *player, const QVariant &);
    QSGS_LOGIC bool roleStatusCommand(ServerPlayer *player);

    QSGS_LOGIC void updateCardsOnLose(const CardsMoveStruct &move);
    QSGS_LOGIC void updateCardsOnGet(const CardsMoveStruct &move);

    /* AI Related */ GeneralSelector *generalSelector() const
    {
        return m_generalSelector;
    }

    QSGS_LOGIC void cheat(ServerPlayer *player, const QVariant &args);
    QSGS_LOGIC bool makeSurrender(ServerPlayer *player);

protected:
    int _m_Id;

private:
    struct _MoveSourceClassifier
    {
        explicit inline _MoveSourceClassifier(const CardsMoveStruct &move)
        {
            m_from = move.from;
            m_from_place = move.from_place;
            m_from_pile_name = move.from_pile_name;
            m_from_player_name = move.from_player_name;
        }
        inline void copyTo(CardsMoveStruct &move) const
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
        Player::Place m_from_place;
        QString m_from_pile_name;
        QString m_from_player_name;
    };

    struct _MoveMergeClassifier
    {
        explicit inline _MoveMergeClassifier(const CardsMoveStruct &move)
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
        Player::Place m_to_place;
        QString m_to_pile_name;

        Player *m_origin_from;
        Player *m_origin_to;
        Player::Place m_origin_to_place;
        QString m_origin_to_pile_name;
    };

    struct _MoveSeparateClassifier
    {
        inline _MoveSeparateClassifier(const CardsMoveOneTimeStruct &moveOneTime, int index)
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
                && m_to_pile_name < other.m_to_pile_name && m_open < other.m_open && m_is_last_handcard < other.m_is_last_handcard;
        }
        Player *m_from;
        Player *m_to;
        Player::Place m_from_place, m_to_place;
        QString m_from_pile_name, m_to_pile_name;
        bool m_open;
        CardMoveReason m_reason;
        bool m_is_last_handcard;
    };

    int _m_lastMovementId;
    void _fillMoveInfo(CardsMoveStruct &moves, int card_index) const;
    QList<CardsMoveOneTimeStruct> _mergeMoves(QList<CardsMoveStruct> cards_moves);
    QList<CardsMoveStruct> _separateMoves(QList<CardsMoveOneTimeStruct> moveOneTimes);
    QString _chooseDefaultGeneral(ServerPlayer *player) const;
    QStringList _chooseDefaultGenerals(ServerPlayer *player) const;
    bool _setPlayerGeneral(ServerPlayer *player, const QString &generalName, bool isFirst);
    QString mode;
    QList<ServerPlayer *> m_players;
    QList<ServerPlayer *> m_alivePlayers;
    int player_count;
    ServerPlayer *current;
    QList<int> pile1, pile2;
    QList<int> table_cards;
    QList<int> *m_drawPile;
    QList<int> *m_discardPile;
    QStack<DamageStruct> m_damageStack;
    bool game_started;
    bool game_started2;
    bool game_finished;
    bool game_paused;
    lua_State *L;
    QList<AI *> ais;
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
    QAtomicPointer<ServerPlayer> _m_raceWinner;

    QMap<int, Player::Place> place_map;
    QMap<int, ServerPlayer *> owner_map;

    const Card *provided;
    bool has_provided;
    ServerPlayer *provider;

    QVariantMap tag;

    QVariant m_fillAGarg;
    ServerPlayer *m_fillAGWho;
    QVariant m_takeAGargs;

    QWaitCondition m_waitCond;
    mutable QMutex m_mutex;

    volatile bool playerPropertySet;

    GeneralSelector *m_generalSelector;

    static QString generatePlayerName();
    void prepareForStart();
    void assignGeneralsForPlayers(const QList<ServerPlayer *> &to_assign);
    void chooseGenerals();
    void chooseHegemonyGenerals();
    AI *cloneAI(ServerPlayer *player);
    void broadcast(const QString &message, ServerPlayer *except = nullptr);
    void initCallbacks();
    QString askForOrder(ServerPlayer *player, const QString &default_choice);
    QString askForRole(ServerPlayer *player, const QStringList &roles, const QString &scheme);

    //process client requests

    bool makeCheat(ServerPlayer *player);
    void makeDamage(const QString &source, const QString &target, QSanProtocol::CheatCategory nature, int point);
    void makeKilling(const QString &killer, const QString &victim);
    void makeReviving(const QString &name);
    void doScript(const QString &script);

    QSGS_SOCKET void skinChangeCommand(ServerPlayer *player, const QVariant &packet);
    QSGS_SOCKET void heartbeatCommand(ServerPlayer *player, const QVariant &packet);
    QSGS_SOCKET void processRequestPreshow(ServerPlayer *player, const QVariant &arg); //hegemony

    //helper functions and structs
    struct _NullificationAiHelper
    {
        const Card *m_trick;
        ServerPlayer *m_from;
        ServerPlayer *m_to;
    };
    bool _askForNullification(const Card *trick, ServerPlayer *from, ServerPlayer *to, bool positive, _NullificationAiHelper helper);
    void _setupChooseGeneralRequestArgs(ServerPlayer *player);

private slots:
    void reportDisconnection();
    void processClientPacket(const QString &packet);
    void assignRoles();
    void startGame();
    void slotSetProperty(ServerPlayer *player, const char *property_name, const QVariant &value);

signals:
    void room_message(const QString &msg);
    void game_over(const QString &winner);
    void signalSetProperty(ServerPlayer *player, const char *property_name, const QVariant &value);
};

#endif
