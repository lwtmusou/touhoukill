#ifndef _REQUEST_API_
#define _REQUEST_API_

#include "global.h"

#include <qglobal.h>

class Player;

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
 * @class Defines the communication API from the server to the client.
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
    /**
      * Send skill invocation request to a player's socket.
      *
      * @param player the socket of the requested player.
      * @param skill_name the name of the skill.
      * @param prompt the prompt information. If empty, use the default prompt.
      * @param target_player the name of the target player. If empty, do not display name.
      * @param time_limit the limitation of time. the unit is second and 0 means infinite.
      *
      * @return whether the skill is invoked.
      */
    RequestResult<bool> askForSkillInvoke(Player *player, const QString &skill_name, const QString &prompt = QString(), const Player *target_player = nullptr, int time_limit = 0);

    /**
      * Request a player to choose option.
      *
      * @param player the socket of the requested player.
      * @param skill_name the reason (skill name) to make choice.
      * @param choices all available choices for the player to choose.
      * @param time_limit the limitation of time. the unit is second and 0 means infinite.
      *
      * @return the choice
      */
    RequestResult<QString> askForChoice(Player *player, const QString &skill_name, const QStringList &choices, int time_limit = 0);

    /**
      * Do race request among players for nullification.
      *
      * @param race_tagets the racers for nullification.
      * @param time_limit the limitation of time. the unit is second and 0 means infinite.
      *
      * @return the user of the nullification.
      */
    RequestResult<Player *> askForNullification(const QList<Player *> &race_tagets, int time_limit = 0);

    /**
      * Request a player to choose a card from another player's region.
      *
      * @param player the player asked to choose card
      * @param target the owner of the card
      * @param flags which regions to choose. If the string contains:
      *  - h: handcards
      *    - s: shown handcards
      *  - e: equipments
      *  - j: judge region
      * @param reason the region of choosing card. (Used by UI in the dialog)
      * @param handcard_visible whether the handcard is visible in this choice. If true, a handcard notification will be done to the chooser before doing request.
      * @param method the handling method of chosen card. It could trigger some skills.
      * @param disabled_ids Lists of cards that cannot be chosen.
      * @param the limitation of time.
      *
      * @return the chosen id.
      */
    RequestResult<int> askForCardChosen(Player *player, Player *target, const QString &flags, const QString &reason, bool handcard_visible = false,
                                        QSanguosha::HandlingMethod method = QSanguosha::MethodNone, const QList<int> &disabled_ids = {}, int time_limit = 0);

    // TODO: askForCard
    // TODO: askForUseCard
    // TODO: askForUseSlashTo
    // TODO: askForAG
    // TODO: askForCardShow
    // TODO: askForSinglePeach
    // TODO: askForTriggerOrder
    // TODO: askForLuckCard
    // TODO: askForSuit
    // TODO: askForKingdom
    // TODO: askForDiscard
    // TODO: askForExchange
    // TODO: askForGuanxing
    // TODO: askForPindian
    // TODO: askForPindianRace
    // TODO: askForPlayerChosen
    // TODO: askForGeneral
    // TODO: askForRende (askForAssignCard?)
    // TODO: askForYiji (askForAssignCard?)
    // TODO: askForOrder (3v3)
    // TODO: askForRole

    // TODO: a structure map Player* to QIODevice.
    // QMap<Player *, QIODevice *> m_sockets;
    // QMap<QIODevice *, Player *> lookupPlayerBySocket;
};

} // namespace RefactorProposal

#endif
