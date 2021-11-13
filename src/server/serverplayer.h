#ifndef _SERVER_PLAYER_H
#define _SERVER_PLAYER_H

class Room;
class Recorder;
class DummyCard;

class CardMoveReason;
struct PhaseStruct;
struct CardUseStruct;

#include "player.h"
#include "protocol.h"
#include "socket.h"

#include <QDateTime>
#include <QSemaphore>

#define QSGS_STATE_ROOM
#define QSGS_STATE_GAME
#define QSGS_LOGIC
#define QSGS_SOCKET

class ServerPlayer : public Player
{
    Q_OBJECT
    Q_PROPERTY(QString ip READ getIp)

public:
    explicit ServerPlayer(Room *room);

    QSGS_SOCKET void setSocket(ClientSocket *socket);

    QSGS_SOCKET void invoke(const QSanProtocol::Packet *packet);
    QSGS_SOCKET void invoke(const char *method, const QString &arg = QStringLiteral("."));
    QSGS_SOCKET QString reportHeader() const;
    QSGS_SOCKET void unicast(const QString &message);

    QSGS_LOGIC void drawCard(const Card *card);
    QSGS_STATE_ROOM Room *getRoom() const;
    QSGS_LOGIC void broadcastSkillInvoke(const Card *card) const;
    QSGS_LOGIC void broadcastSkillInvoke(const QString &card_name) const;
    QSGS_STATE_GAME int getRandomHandCardId() const;
    QSGS_STATE_GAME const Card *getRandomHandCard() const;
    QSGS_LOGIC void obtainCard(const Card *card, bool unhide = true);
    QSGS_LOGIC void throwAllEquips();
    QSGS_LOGIC void throwAllHandCards();
    QSGS_LOGIC void throwAllHandCardsAndEquips();
    QSGS_LOGIC void throwAllCards();
    QSGS_LOGIC void bury();
    QSGS_LOGIC void throwAllMarks(bool visible_only = true);
    QSGS_LOGIC void clearOnePrivatePile(const QString &pile_name);
    QSGS_LOGIC void clearPrivatePiles();
    QSGS_LOGIC void drawCards(int n, const QString &reason = QString());
    QSGS_LOGIC bool askForSkillInvoke(const QString &skill_name, const QVariant &data = QVariant(), const QString &prompt = QString());
    QSGS_LOGIC bool askForSkillInvoke(const Skill *skill, const QVariant &data = QVariant(), const QString &prompt = QString());
    QSGS_LOGIC QList<int> forceToDiscard(int discard_num, bool include_equip, bool is_discard = true);
    QSGS_STATE_GAME QList<const Card *> getCards(const QString &flags) const;
    QSGS_STATE_GAME Card *wholeHandCards() const; // FIXME: Memory Leakage!!!
    QSGS_STATE_GAME bool hasNullification() const;
    // FIXME: Trying to move this function to the game-logic.h
    QSGS_LOGIC bool pindian(ServerPlayer *target, const QString &reason, const Card *card1 = nullptr);
    QSGS_LOGIC void turnOver();
    QSGS_LOGIC void play(QList<QSanguosha::Phase> set_phases = QList<QSanguosha::Phase>());
    QSGS_LOGIC bool changePhase(QSanguosha::Phase from, QSanguosha::Phase to);

    QSGS_STATE_GAME QList<QSanguosha::Phase> &getPhases();
    QSGS_STATE_GAME int getPhasesIndex() const;
    QSGS_LOGIC void skip(QSanguosha::Phase phase, bool isCost = false, bool sendLog = true);
    QSGS_LOGIC void insertPhases(QList<QSanguosha::Phase> new_phases, int index = -1);
    QSGS_LOGIC void exchangePhases(QSanguosha::Phase phase, QSanguosha::Phase phase1);
    QSGS_STATE_GAME bool isSkipped(QSanguosha::Phase phase);

    QSGS_LOGIC void gainMark(const QString &mark, int n = 1);
    QSGS_LOGIC void loseMark(const QString &mark, int n = 1);
    QSGS_LOGIC void loseAllMarks(const QString &mark_name);

#if 0
    QSGS_LOGIC void addSkill(const QString &skill_name, bool head_skill = true) override;
    QSGS_LOGIC void loseSkill(const QString &skill_name, bool head_skill = true) override;
#endif

    // utilities?
    void startRecord();
    void saveRecord(const QString &filename);

    // 3v3 methods
    QSGS_LOGIC void addToSelected(const QString &general);
    QSGS_STATE_GAME QStringList getSelected() const;
    QString findReasonable(const QStringList &generals, bool no_unreasonable = false); // ???
    QSGS_LOGIC void clearSelected();

    QSGS_STATE_GAME int getGeneralMaxHp() const;

    QSGS_SOCKET QString getIp() const;
    QSGS_SOCKET quint32 ipv4Address() const;
    QSGS_SOCKET void introduceTo(ServerPlayer *player);
    QSGS_LOGIC void marshal(ServerPlayer *player) const;

    QSGS_LOGIC void addToPile(const QString &pile_name, const Card *card, bool open = true, const QList<ServerPlayer *> &open_players = QList<ServerPlayer *>());
    QSGS_LOGIC void addToPile(const QString &pile_name, int card_id, bool open = true, const QList<ServerPlayer *> &open_players = QList<ServerPlayer *>());
    QSGS_LOGIC void addToPile(const QString &pile_name, const IDSet &card_ids, bool open = true, const QList<ServerPlayer *> &open_players = QList<ServerPlayer *>());
    QSGS_LOGIC void addToPile(const QString &pile_name, const IDSet &card_ids, bool open, const CardMoveReason &reason,
                              QList<ServerPlayer *> open_players = QList<ServerPlayer *>());
    QSGS_LOGIC void gainAnExtraTurn();

    QSGS_LOGIC void showHiddenSkill(const QString &skill_name);
    QSGS_LOGIC QStringList checkTargetModSkillShow(const CardUseStruct &use);

    QSGS_LOGIC void startNetworkDelayTest();
    QSGS_LOGIC qint64 endNetworkDelayTest();

    //Synchronization helpers
    enum SemaphoreType
    {
        SEMA_MUTEX, // used to protect mutex access to member variables
        SEMA_COMMAND_INTERACTIVE // used to wait for response from client
    };
    QSGS_SOCKET inline QSemaphore *getSemaphore(SemaphoreType type)
    {
        return semas[type];
    }
    QSGS_SOCKET inline void acquireLock(SemaphoreType type)
    {
        semas[type]->acquire();
    }
    QSGS_SOCKET inline bool tryAcquireLock(SemaphoreType type, int timeout = 0)
    {
        return semas[type]->tryAcquire(1, timeout);
    }
    QSGS_SOCKET inline void releaseLock(SemaphoreType type)
    {
        semas[type]->release();
    }
    QSGS_SOCKET inline void drainLock(SemaphoreType type)
    {
        while (semas[type]->tryAcquire()) {
        }
    }
    QSGS_SOCKET inline void drainAllLocks()
    {
        for (int i = 0; i < S_NUM_SEMAPHORES; i++) {
            drainLock((SemaphoreType)i);
        }
    }
    QSGS_SOCKET inline QString getClientReplyString()
    {
        return m_clientResponseString;
    }
    QSGS_SOCKET inline void setClientReplyString(const QString &val)
    {
        m_clientResponseString = val;
    }
    QSGS_SOCKET inline const QVariant &getClientReply()
    {
        return _m_clientResponse;
    }
    QSGS_SOCKET inline void setClientReply(const QVariant &val)
    {
        _m_clientResponse = val;
    }
    unsigned int m_expectedReplySerial; // Suggest the acceptable serial number of an expected response.
    bool m_isClientResponseReady; //Suggest whether a valid player's reponse has been received.
    bool m_isWaitingReply; // Suggest if the server player is waiting for client's response.
    QVariant m_cheatArgs; // Store the cheat code received from client.
    QSanProtocol::CommandType m_expectedReplyCommand; // Store the command to be sent to the client.
    QVariant m_commandArgs; // Store the command args to be sent to the client.

    // static function
    static bool CompareByActionOrder(ServerPlayer *a, ServerPlayer *b);

    QSGS_LOGIC void showGeneral(bool head_general = true, bool trigger_event = true, bool sendLog = true, bool ignore_rule = true);
    QSGS_LOGIC void hideGeneral(bool head_general = true);
    QSGS_LOGIC void removeGeneral(bool head_general = true);
    QSGS_LOGIC void sendSkillsToOthers(bool head_skill = true);
    QSGS_STATE_GAME int getPlayerNumWithSameKingdom(const QString &reason, const QString &_to_calculate = QString()) const;
    QSGS_LOGIC bool askForGeneralShow(bool one = true, bool refusable = false);

    QSGS_STATE_GAME bool inSiegeRelation(const ServerPlayer *skill_owner, const ServerPlayer *victim) const;
    QSGS_STATE_GAME bool inFormationRalation(ServerPlayer *teammate) const;
    QSGS_LOGIC void summonFriends(const QString &type);

    bool isReady() const;
    void setReady(bool ready);

protected:
    //Synchronization helpers
    QSemaphore **semas;
    static const int S_NUM_SEMAPHORES;

private:
    ClientSocket *socket;
    QList<const Card *> m_handcards;
    Room *room;
    Recorder *recorder;
    QList<QSanguosha::Phase> phases;
    int _m_phases_index;
    QList<PhaseStruct> _m_phases_state;
    QStringList selected; // 3v3 mode use only
    QDateTime test_time;
    QString m_clientResponseString;
    QVariant _m_clientResponse;

    bool ready;

private slots:
    void getMessage(const char *message);
    void sendMessage(const QString &message);

signals:
    void disconnected();
    void request_got(const QString &request);
    void message_ready(const QString &msg);
};

#undef QSGS_STATE_ROOM
#undef QSGS_STATE_GAME
#undef QSGS_LOGIC
#undef QSGS_SOCKET

#endif
