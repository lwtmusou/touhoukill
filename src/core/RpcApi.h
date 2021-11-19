#ifndef _REQUEST_API_
#define _REQUEST_API_

#include "global.h"
#include "structs.h"

#include <qglobal.h>

class Player;
class QIODevice;

namespace RefactorProposal {

enum class RequestStatus
{
    Normal = 0,
    Invalid = 1,
    TimeOut = 2,
    Offline = 3,
    Network = 4,
};

template<typename T> class RequestResult
{
public:
    // T should be default constructed, Or using (static_cast<QString>(0)) will construct a malformed QString.
    inline RequestResult()
        : m_result(T())
        , m_state(RequestStatus::Invalid)
    {
    }

    inline RequestResult(const T &t, RequestStatus state = RequestStatus::Normal)
        : m_result(t)
        , m_state(state)
    {
    }

    inline RequestResult(T &&t, RequestStatus state = RequestStatus::Normal)
        : m_result(t)
        , m_state(state)
    {
    }

    inline RequestStatus state() const
    {
        return m_state;
    }

    inline void setState(RequestStatus state)
    {
        m_state = state;
    }

    inline T value()
    {
        return m_result;
    }

    inline const T &value() const
    {
        return m_result;
    }

private:
    // Fs: I think this struct must be able to copy constructed.
    // Preferrably default copy construct?
    // Q_DISABLE_COPY(RequestResult)
    T m_result;
    RequestStatus m_state;
};

/**
 * @brief The result of askForGuanxing
 */
struct GuanxingResult
{
    QList<int> top_ids;
    QList<int> bottom_ids;
};

/**
 * @brief The result of askForPindianRace
 */
struct PindianCards
{
    Card *from;
    Card *to;
};

/**
 * @brief The result of askForDistrubuteCard
 */
struct CardDistributionResult
{
    QList<int> ids;
    Player *obtainer;
};

/**
 * @brief Defines the communication API from the server to the client.
 *
 * This class includes the following APIs:
 * - Request. (askForXXXX)
 * - Notification. (setPlayerXXXX/setCardXXXX)
 * - Animation.
 *
 * @see The protocol is defined in protocol.h
 */
class RpcApi
{
public:
    RpcApi();

    RequestResult<QString> askForChoice(Player *player, const QString &skill_name, const QStringList &choices, int time_limit = 0);
    RequestResult<QSanguosha::Suit> askForSuit(Player *player, const QString &reason, int time_limit = 0);
    RequestResult<QString> askForKingdom(Player *player, int time_limit = 0);
    RequestResult<QString> askForGeneral(Player *player, const QStringList &generals, int time_limit = 0);
    RequestResult<QString> askForOrder(Player *player, int time_limit = 0);
    RequestResult<QString> askForRole(Player *player, const QStringList &roles, const QString &scheme, int time_limit = 0);

    RequestResult<bool> askForSkillInvoke(Player *player, const QString &skill_name, const QString &prompt = QString(), const Player *target_player = nullptr, int time_limit = 0);

    RequestResult<int> askForCardChosen(Player *player, Player *target, const QString &flags, const QString &reason, QSanguosha::HandlingMethod method,
                                        const QList<int> &disabled_ids, int time_limit = 0);

    RequestResult<Card *> askForHandcardShow(Player *player, Player *requestor, int time_limit = 0);

    // TODO: askForCard (Wait for Fs.)
    RequestResult<Player *> askForNullification(const QList<Player *> &race_tagets, int time_limit = 0);
    RequestResult<CardUseStruct> askForUseCard(Player *player, const QString &pattern, const QString &prompt, int notice_index, QSanguosha::HandlingMethod method,
                                               const QString &skill_name, int time_limit = 0);
    RequestResult<Card *> askForSinglePeach(Player *user, Player *dying, int time_limit = 0);

    RequestResult<int> askForPickCardFromAG(Player *player, bool refusable, const QString &reason, int time_limit = 0);

    RequestResult<bool> askForDiscard(Player *player, const QString &reason, int discard_number, int min_num, bool optional, bool include_equip, const QString &prompt,
                                      int time_limit = 0);
    RequestResult<IDSet> askForExchange(Player *player, const QString &reason, int discard_number, int min_num, bool optional, bool include_equip, const QString &prompt,
                                        int time_limit = 0);

    RequestResult<Player *> askForPlayerChosen(Player *player, const QList<Player *> &targets, const QString &skill_name, const QString &prompt, bool optional, int time_limit = 0);

    RequestResult<Card *> askForPindianCard(Player *player, Player *from, Player *to, int time_limit = 0);
    RequestResult<PindianCards> askForPindianRace(Player *from, Player *to, const QString &reason, int time_limit = 0);

    RequestResult<TriggerDetailPtr> askForTriggerOrder(Player *player, const QList<TriggerDetailPtr> &sameTiming, bool cancelable, int time_limit = 0);

    RequestResult<GuanxingResult> askForGuanxing(Player *player, const QList<int> &ids, QSanguosha::GuanxingType guanxing_type, const QString &skill_name, int time_limit = 0);

    RequestResult<CardDistributionResult> askForDistributeCards(Player *player, const IDSet &card_ids, bool is_optional, int max_num, QList<Player *> available_players,
                                                                const QString &prompt, int time_limit = 0);

    // Notification
    void notifyCurrentCardUseReason(QSanguosha::CardUseReason reason);
    void notifyCurrentCardUsePattern(const QString &pattern);
    void notifyPlayerKilled(const Player *victim, const QList<Player *> &viewer);

    // - Card related methods
    void mapCardIdToNewCard(int card_id, Card *new_card, const QList<Player *> &viewers);
    void resetCardIdMapping(int card_id, const QList<Player *> &viewers);

    // TODO: updateDrawPileLength

    // - Animation and UI
    // -- Dashboard related methods
    void disablePlayerDashboard(const Player *player_losing_dashboard);
    void addPlayerSkillButtonToPrimaryGeneral(const Player *player, const QString &skill_name);
    // -- Amazing grace related methods (fillAG, takeAG, clearAG)
    void showAGFrame(const QList<int> &card_ids, const QList<Player *> &viewers, const QList<int> &disbaled_ids, const QList<int> &shownHandcard_ids = {});
    void updateAGOnPlayerTakeCard(Player *taker, int card_id, bool do_move_animation, const QList<Player *> &viewers);
    void hideAGFrame(const QList<Player *> &viewer);
    // -- Skill button

    // TODO: notifyMoveFocus

    // TODO: a structure map Player* to QIODevice.
    // QMap<Player *, QIODevice *> m_sockets;
    // QMap<QIODevice *, Player *> lookupPlayerBySocket;

private:
    QIODevice *findSocketByUser(const Player *player) const;
};

} // namespace RefactorProposal

#endif
