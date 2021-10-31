#ifndef _REQUEST_API_
#define _REQUEST_API_

#include <optional>

namespace RefactorProposal {

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
class RequestAPI {
public:
  RequestAPI();
  /**
   * Send skill invocation request to a player's socket.
   *
   * @param socket_id the socket of the requested player.
   * @param skill_name the name of the skill.
   * @param prompt the prompt information. If empty, use the default prompt.
   * @param target_player the name of the target player. If empty, do not display name.
   * @param time_limit the limitation of time. the unit is second and 0 means infinite.
   *
   * @return true/false if a successful reply is get from client. Othewise, no valid value.
   */
  std::optional<bool> askForSkillInvoke(int socket_id, const QString &skill_name, const QString &prompt = QString(), const QString &target_player = QString(), int time_limit = 0);

  /**
   * Request a player to choose option.
   *
   * @param socket_id the socket of the requested player.
   * @param skill_name the reason (skill name) to make choice.
   * @param choices all available choices for the player to choose.
   * @param time_limit the limitation of time. the unit is second and 0 means infinite.
   *
   * @return optional string if a successful reply is get from the client. Otherwise, no valid value.
   */
  std::optional<QString> askForChoice(int socket_id, const QString &skill_name, const QStringList &choices, int time_limit = 0);

};


};

#endif