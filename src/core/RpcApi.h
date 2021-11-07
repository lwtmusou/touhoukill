#ifndef _REQUEST_API_
#define _REQUEST_API_

#include "global.h"
#include "structs.h"

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
      * @param player the player asked to choose card.
      * @param target the owner of the card.
      * @param flags which regions to choose. If the string contains:
      *  - h: handcards
      *    - s: shown handcards (a notification of handcard broadcasting will be done before choosing.)
      *  - e: equipments
      *  - j: judge region
      * @param reason the region of choosing card. (Used by UI in the dialog)
      * @param method the handling method of chosen card. It could trigger some skills.
      * @param disabled_ids Lists of cards that cannot be chosen.
      * @param time_limit the limitation of time.
      *
      * @return the chosen id.
      */
    RequestResult<int> askForCardChosen(Player *player, Player *target, const QString &flags, const QString &reason, QSanguosha::HandlingMethod method,
                                        const QList<int> &disabled_ids, int time_limit = 0);

    // TODO: askForCard (Wait for Fs.)

    /**
     * @brief ask a player to use a card. (RESPONSE_USE)
     *
     * @param player the player to use card.
     * @param pattern the pattern of the available card.
     * @param prompt the prompt information.
     * @param notice_index (?)
     * @param method the handling method of this card.
     * @param skill_name the skill name.
     * @param time_limit the limitation of time.
     *
     * @return card use result.
     */
    RequestResult<CardUseStruct> askForUseCard(Player *player, const QString &pattern, const QString &prompt, int notice_index, QSanguosha::HandlingMethod method,
                                               const QString &skill_name, int time_limit = 0);
    /**
     * @brief ask a player to choose a card from the given AG area.
     *
     * @param player the player
     * @param refusable whether the player can choose to click cancel button
     * @param reason the skill_name/card_name of this request.
     * @param time_limit the limitation of time.
     *
     * @return the id of chosen card. If no choice is made, -1 will be return.
     *
     * @see RpcApi::fillAG RpcApi::clearAG
     */
    RequestResult<int> askForPickCardFromAG(Player *player, bool refusable, const QString &reason, int time_limit = 0);
    /**
     * @brief ask the player to show a handcard.
     *
     * @param player the player to show one of his handcard.
     * @param requestor the player who request this show.
     * @param time_limit the limitation of time.
     *
     * @return the card being showed.
     */
    RequestResult<Card *> askForHandcardShow(Player *player, Player *requestor, int time_limit = 0);
    /**
     * @brief ask a player to use peach to the dying player.
     *
     * @param user the player being asked to use peach.
     * @param dying the dying player.
     * @param limit_time the limitation of time.
     *
     * @return the peach. nullptr means no peach is used.
     *
     * @note should we merge this function with askForUseCard?
     */
    RequestResult<Card *> askForSinglePeach(Player *user, Player *dying, int time_limit = 0);
    /**
     * @brief ask a player to choose a trigger from all triggers that share the same trigger time.
     *
     * @param player the player
     * @param sameTiming triggers sharing the same invocation time.
     * @param cancelable whether the player can click cancel button.
     * @param limit_time the limitation of time.
     *
     * @return the selected trigger. If no trigger is selected, a nullptr will be return.
     */
    RequestResult<TriggerDetailPtr> askForTriggerOrder(Player *player, const QList<TriggerDetailPtr> &sameTiming, bool cancelable, int time_limit = 0);
    // TODO: askForSuit
    /**
     * @brief ask a player to choose a suit.
     *
     * @param player the player to choose suit.
     * @param reason the reason of this request.
     * @param time_limit the limitation of time.
     *
     * @return the selected suit.
     *
     * TODO: Add a cancelable parameter?
     * TODO: Should we merge this function with askForChoice?
     */
    RequestResult<QSanguosha::Suit> askForSuit(Player *player, const QString &reason, int time_limit = 0);
    // TODO: askForKingdom
    /**
     * @brief ask a player to choose a kingdom
     *
     * @param player the player to choose the kingdom
     * @param time_limit the limitation of time.
     *
     * @return the selected kingdom.
     *
     * TODO: Should we merge this function with askForChoice?
     */
    RequestResult<QString> askForKingdom(Player *player, int time_limit = 0);

    // TODO: askForDiscard
    /**
     * @brief ask a player to discard some cards.
     *
     * @param player the player to discard the handcard.
     * @param reason the reason of discarding cards.
     * @param discard_number the number of required cards.
     * @param min_num the minimum of requested cards.
     * @param optional whether the player can refuse to discard cards.
     * @param include_equip whether equip card is also included.
     * @param prompt the prompt information.
     *
     * @return whether the player choose to discard.
     *
     * TODO: Is it possible to merge this API with askForExchange?
     */
    RequestResult<bool> askForDiscard(Player *player, const QString &reason, int discard_number, int min_num, bool optional, bool include_equip, const QString &prompt,
                                      int time_limit = 0);
    // TODO: askForExchange
    /**
     * @brief ask a player to exchange some cards.
     *
     * @param player the player to discard the handcard.
     * @param reason the reason of discarding cards.
     * @param discard_number the number of required cards.
     * @param min_num the minimum of requested cards.
     * @param optional whether the player can refuse to discard cards.
     * @param include_equip whether equip card is also included.
     * @param prompt the prompt information.
     * @param time_limit the limitation of time.
     *
     * @return the card ids selected by the player.
     *
     * TODO: Is it possible to merge this API with askForExchange?
     */
    RequestResult<IDSet> askForExchange(Player *player, const QString &reason, int discard_number, int min_num, bool optional, bool include_equip, const QString &prompt,
                                        int time_limit = 0);
    // TODO: askForGuanxing
    /**
     * @brief ask a player to classify cards into two groups and change their order. (For guanxing)
     *
     * @param player the player to classify cards.
     * @param ids the card to arrange.
     * @param guanxing_type the operation mode. (See QSanguosha::GuanxingType for detail)
     * @param skill_name the limitation of time.
     * @param time_limit the limitation of time.
     *
     * @return A Pair. Pair.first means the ids put on top, pair.second means the ids put on bottom.
     */
    RequestResult<QPair<QList<int>, QList<int>>> askForGuanxing(Player *player, const QList<int> &ids, QSanguosha::GuanxingType guanxing_type, const QString &skill_name,
                                                                int time_limit = 0);
    // TODO: askForPindian
    /**
     * @brief Ask a player to provide its pindian card.
     *
     * @param player the player to procide cards.
     * @param from the requester of the pindian operation.
     * @param to the requestee of the pindian opeartion.
     * @param time_limit the limitation of time.
     *
     * @return the card for Pindian.
     *
     * TODO: Use askForCard to replace this function.
     */
    RequestResult<Card *> askForPindianCard(Player *player, Player *from, Player *to, int time_limit = 0);
    /**
     * @brief Ask two player to provide their pindian card at the same time.
     *
     * @param from the requester player.
     * @param to the requestee player.
     * @param reason the reason of pindian.
     * @param time_limit the limitation of time.
     *
     * @return two cards. pair.first is from "from", pair.to is from "to".
     */
    RequestResult<QPair<Card *, Card *>> askForPindianRace(Player *from, Player *to, const QString &reason, int time_limit = 0);

    // TODO: askForPlayerChosen
    /**
     * @brief ask a player to choose between players.
     *
     * @param player the asked player.
     * @param targets the potential choices.
     * @param skill_name the skill name of this choice.
     * @param prompt the prompt.
     * @param optional whether the player can cancel this operation.
     * @param time_limit the limitation of time.
     *
     * @return the chosen player. nullptr means no choice is made.
     */
    RequestResult<Player *> askForPlayerChosen(Player *player, const QList<Player *> &targets, const QString &skill_name, const QString &prompt, bool optional, int time_limit = 0);
    // TODO: askForGeneral
    /**
     * @brief ask a player to choose a general.
     *
     * @param player the player.
     * @param generals potential general options.
     * @param time_limit the limitation of time.
     *
     * @return the chosen result.
     *
     * TODO: Can we combine this function with askForChoice?
     */
    RequestResult<QString> askForGeneral(Player *player, const QStringList &generals, int time_limit = 0);
    /**
     * @brief ask a player to distribute cards to players.
     * @note the original name is askForYiji/askForRende
     *
     * @param player the player to distrubute the cards.
     * @param card_ids the cards to assign.
     * @param is_optional whether the player can cancel this operation.
     * @param max_num the maximum possible cards that can be distributed.
     * @param available_players all possible players to get cards.
     * @param prompt the prompt information
     * @param time_limit the limitation of time.
     *
     * @return the single assignment: pair.first indicates the cards, and pair.second indicates the player obtaining the card.
     */
    RequestResult<QPair<QList<int>, Player *>> askForDistributeCards(Player *player, const IDSet &card_ids, bool is_optional, int max_num, QList<Player *> available_players,
                                                                     const QString &prompt, int time_limit = 0);
    // TODO: askForOrder (3v3)
    /**
     * @brief ask the player to choose action order. (For 3v3.)
     *
     * @param player the player to make the choice.
     * @param time_limit the limitation of time.
     *
     * @return the option made by the player.
     *
     * TODO: Merge this request with askForChoice.
     */
    RequestResult<QString> askForOrder(Player *player, int time_limit = 0);
    // TODO: askForRole
    /**
     * @brief ask the player to choose a role.
     *
     * @param player the player.
     * @param roles the potential roles.
     * @param scheme ?
     * @param time_limit the limitation of time.
     *
     * @return the choice made by the player.
     *
     * TODO: Merge this request with askForChoice.
     */
    RequestResult<QString> askForRole(Player *player, const QStringList &roles, const QString &scheme, int time_limit = 0);

    // Notification
    // TODO: setCurrentCardUseReason
    // TODO: setCurrentCardUsePattern
    // TODO: fillAG
    // TODO: clearAG
    // TODO: updateDrawPileLength

    // Animation
    // TODO: notifyMoveFocus

    // TODO: a structure map Player* to QIODevice.
    // QMap<Player *, QIODevice *> m_sockets;
    // QMap<QIODevice *, Player *> lookupPlayerBySocket;
};

} // namespace RefactorProposal

#endif
