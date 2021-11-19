#include "RpcApi.h"
#include "global.h"
#include "structs.h"

namespace RefactorProposal {

/**
  * @brief Send skill invocation request to a player's socket.
  *
  * @param player the socket of the requested player.
  * @param skill_name the name of the skill.
  * @param prompt the prompt information. If empty, use the default prompt.
  * @param target_player the name of the target player. If empty, do not display name.
  * @param time_limit the limitation of time. the unit is second and 0 means infinite.
  *
  * @return whether the skill is invoked.
  */
RequestResult<bool> RpcApi::askForSkillInvoke(Player *player, const QString &skill_name, const QString &prompt, const Player *target_player, int time_limit)
{
    return RequestResult<bool> {};
}

/**
  * @brief Request a player to choose option.
  *
  * @param player the socket of the requested player.
  * @param skill_name the reason (skill name) to make choice.
  * @param choices all available choices for the player to choose.
  * @param time_limit the limitation of time. the unit is second and 0 means infinite.
  *
  * @return the choice
  */
RequestResult<QString> RpcApi::askForChoice(Player *player, const QString &skill_name, const QStringList &choices, int time_limit)
{
    return RequestResult<QString> {};
}

/**
  * @brief Do race request among players for nullification.
  *
  * @param race_tagets the racers for nullification.
  * @param time_limit the limitation of time. the unit is second and 0 means infinite.
  *
  * @return the user of the nullification.
  */
RequestResult<Player *> RpcApi::askForNullification(const QList<Player *> &race_tagets, int time_limit)
{
    return RequestResult<Player *>(nullptr);
}

/**
  * @brief Request a player to choose a card from another player's region.
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
RequestResult<int> RpcApi::askForCardChosen(Player *player, Player *target, const QString &flags, const QString &reason, QSanguosha::HandlingMethod method,
                                            const QList<int> &disabled_ids, int time_limit)
{
    return RequestResult<int>(-1);
}

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
RequestResult<CardUseStruct> RpcApi::askForUseCard(Player *player, const QString &pattern, const QString &prompt, int notice_index, QSanguosha::HandlingMethod method,
                                                   const QString &skill_name, int time_limit)
{
    return RequestResult<CardUseStruct>();
}

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
RequestResult<int> RpcApi::askForPickCardFromAG(Player *player, bool refusable, const QString &reason, int time_limit)
{
    return RequestResult<int>(-1);
}

/**
 * @brief ask the player to show a handcard.
 *
 * @param player the player to show one of his handcard.
 * @param requestor the player who request this show.
 * @param time_limit the limitation of time.
 *
 * @return the card being showed.
 */
RequestResult<Card *> RpcApi::askForHandcardShow(Player *player, Player *requestor, int time_limit)
{
    return RequestResult<Card *>(nullptr);
}

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
RequestResult<Card *> RpcApi::askForSinglePeach(Player *user, Player *dying, int time_limit)
{
    return RequestResult<Card *>(nullptr);
}

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
RequestResult<TriggerDetailPtr> RpcApi::askForTriggerOrder(Player *player, const QList<TriggerDetailPtr> &sameTiming, bool cancelable, int time_limit)
{
    return RequestResult<TriggerDetailPtr>();
}

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
RequestResult<QSanguosha::Suit> RpcApi::askForSuit(Player *player, const QString &reason, int time_limit)
{
    return RequestResult<QSanguosha::Suit>();
}

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
RequestResult<QString> RpcApi::askForKingdom(Player *player, int time_limit)
{
    return RequestResult<QString>();
}

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
RequestResult<bool> RpcApi::askForDiscard(Player *player, const QString &reason, int discard_number, int min_num, bool optional, bool include_equip, const QString &prompt,
                                          int time_limit)
{
    return RequestResult<bool>();
}

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
RequestResult<IDSet> RpcApi::askForExchange(Player *player, const QString &reason, int discard_number, int min_num, bool optional, bool include_equip, const QString &prompt,
                                            int time_limit)
{
    return RequestResult<IDSet>();
}

/**
 * @brief ask a player to classify cards into two groups and change their order. (For guanxing)
 *
 * @param player the player to classify cards.
 * @param ids the card to arrange.
 * @param guanxing_type the operation mode. (See QSanguosha::GuanxingType for detail)
 * @param skill_name the limitation of time.
 * @param time_limit the limitation of time.
 *
 * @return A struct. struct.top_ids means the ids put on top, struct.bottom_ids means the ids put on bottom.
 */
RequestResult<GuanxingResult> RpcApi::askForGuanxing(Player *player, const QList<int> &ids, QSanguosha::GuanxingType guanxing_type, const QString &skill_name, int time_limit)
{
    return RequestResult<GuanxingResult>();
}

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
RequestResult<Card *> RpcApi::askForPindianCard(Player *player, Player *from, Player *to, int time_limit)
{
    return RequestResult<Card *>();
}

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
RequestResult<PindianCards> RpcApi::askForPindianRace(Player *from, Player *to, const QString &reason, int time_limit)
{
    return RequestResult<PindianCards>();
}

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
RequestResult<Player *> RpcApi::askForPlayerChosen(Player *player, const QList<Player *> &targets, const QString &skill_name, const QString &prompt, bool optional, int time_limit)
{
    return RequestResult<Player *>();
}

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
RequestResult<QString> RpcApi::askForGeneral(Player *player, const QStringList &generals, int time_limit)
{
    return RequestResult<QString>();
}

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
 * @return the single assignment: result.ids are the card ids, and result.obtainer is the player who should obtain these cards.
 */
RequestResult<CardDistributionResult> RpcApi::askForDistributeCards(Player *player, const IDSet &card_ids, bool is_optional, int max_num, QList<Player *> available_players,
                                                                    const QString &prompt, int time_limit)
{
    return RequestResult<CardDistributionResult>();
}

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
RequestResult<QString> RpcApi::askForOrder(Player *player, int time_limit)
{
    return RequestResult<QString> {};
}

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
RequestResult<QString> RpcApi::askForRole(Player *player, const QStringList &roles, const QString &scheme, int time_limit)
{
    return RequestResult<QString>();
}

/**
 * @brief broadcast current card use pattern to all agents.
 *
 * @param pattern the pattern to broadcast.
 */
void RpcApi::notifyCurrentCardUsePattern(const QString &pattern)
{
}

/**
 * @brief broadcast current card use reason to all agents.
 *
 * @param reason the reason of using card.
 */
void RpcApi::notifyCurrentCardUseReason(QSanguosha::CardUseReason reason)
{
}

/**
 * @brief Let viewers know that a player is killed.
 *
 * @note this API should update UI and agent's game state simultaneously.
 *
 * @param victim the player being killed
 * @param viewer the viewer of this event.
 */
void RpcApi::notifyPlayerKilled(const Player *victim, const QList<Player *> &viewer)
{
}

/**
 * @brief Let viewers know that a card_id is mapped to another card.
 *
 * @param card_id the updated id
 * @param new_card the information of the card belonging to the new id.
 * @param viewers the notified agents.
 *
 * @note This function is usually used by filterSkill
 */
void RpcApi::mapCardIdToNewCard(int card_id, Card *new_card, const QList<Player *> &viewers)
{
    // Room::notifyUpdateCard
}

/**
 * @brief reset the mapping of a card_id to its default card.
 *
 * @param card_id the id of card that will be reset.
 * @param viewer the notified agents.
 */
void RpcApi::resetCardIdMapping(int card_id, const QList<Player *> &viewers)
{
    // Room::notifyResetCard
}

/**
 * @brief disable a player's dashboard and make it gray.
 *
 * @note this function is only called when a player is killed.
 *
 * @param player_losing_dashboard the player who loses its dashboard.
 */
void RpcApi::disablePlayerDashboard(const Player *player_losing_dashboard)
{
    // S_COMMAND_SET_DASHBOARD_SHADOW
}

/**
 * @brief add a skill button to a player's primary general card.
 *
 * @note Not sure if the meaning is correct. (Need to check the protocol)
 *
 * @param player the player who will get the skill button.
 * @param skill_name the name of the skill
 *
 * @see Room::attachSkillToPlayer
 */
void RpcApi::addPlayerSkillButtonToPrimaryGeneral(const Player *player, const QString &skill_name)
{
}

/**
 * @brief display the frame for amazing grace to all agents.
 *
 * @param card_ids the card to fill the frame.
 * @param viewers the viewer of this frame.
 * @param disabled_ids the list of cards that cannot be selected.
 * @param shownHandcards_ids ??????
 */
void RpcApi::showAGFrame(const QList<int> &card_ids, const QList<Player *> &viewers, const QList<int> &disbaled_ids, const QList<int> &shownHandcard_ids)
{
}

/**
 * @brief update agents' AG frame when a player choose one card from AG frame.
 *
 * @param taker the player who takes the card.
 * @param card_id the id of the card taken.
 * @param do_move_animation whether to play move animation. If true, an animation where the card is moved to taker's hand will be displayed.
 * @param viewer the agents who can see this update.
 */
void RpcApi::updateAGOnPlayerTakeCard(Player *taker, int card_id, bool do_move_animation, const QList<Player *> &viewers)
{
}

/**
 * @brief hide agents' AG frame.
 *
 * @param viewer the agents whose AG frame will be hidden.
 */
void RpcApi::hideAGFrame(const QList<Player *> &viewer)
{
}

} // namespace RefactorProposal
