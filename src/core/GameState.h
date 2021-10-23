#ifndef _GAME_STATE_H_
#define _GAME_STATE_H_

#include <QObject>
#include <qstringliteral.h>

#include "CardFace.h"
#include "card.h"
#include "global.h"
#include "structs.h"

class Player;
class CardFace;
class General;
class ViewAsSkill;
class ProhibitSkill;
class TreatAsEquippingSkill;
class TargetModSkill;
class DistanceSkill;
class MaxCardsSkill;
class AttackRangeSkill;
class Trigger;

namespace RefactorProposal {

/**
 * @interface Game state keeps all the current information needed by the game.
 * 
 * It also provides a low-level API to manually modify Player's/Card's property. 
 * And these modification will not trigger game events. However, notification from server to clients will performed. 
 * 
 * Similar to the GameLogic, the server and the client should have different implementation of this class.
 * In the server, it needs to notify the clients when the game state is changed.
 * In the client, it needs to notify the UI when the game state is changed.
 * 
 */
class GameState
{
public:
    GameState();
    virtual ~GameState();

    // Game state records all the information required for one game, including
    // - Player update (if so all the setters of Player's properties should be hidden except this class.)
    // - Card management
    // - Region informaiton (Global region)
    // - Global management

    // ---------------- Player Management ---------------- 
    virtual void registerPlayer(Player *player) = 0;
    virtual void unregisterPlayer(Player *player) = 0;
    virtual QList<Player *> players(bool include_dead = true, bool include_removed = true) = 0;
    virtual QList<const Player *> players(bool include_dead = true, bool include_removed = true) const = 0;
    /**
     * @brief get all players in this room, by counter / counter clockwise order.
     * 
     * This function will get all players, and sort the result based on the viewer. 
     * For instance, with a specific viewer and counter-clockwise, the first element of the return result will be the player by his right hand. 
     * No order guarantee if viewer is nullptr.
     * 
     * @param viewer which player's view. nullptr means no specific viewer. 
     * @param is_counter_clockwise whether the order of the result is sorted counter-clockwise.
     * @return QList contains the sorted players.
     */ 
    virtual QList<Player *> allPlayers(Player *viewer = nullptr, bool is_counter_clockwise = true) = 0;
    /**
     * @brief get all alive players in this room, by counter / counter clockwise order.
     * 
     * @param viewer which player's view. nullptr means no specific viewer. 
     * @param is_counter_clockwise whether the order of the result is sorted counter-clockwise.
     * @return QList contains the sorted players.
     * 
     * @see GameState::allPlayers
     */ 
    virtual QList<Player *> alivePlayers(Player *viewer = nullptr, bool is_counter_clockwise = true, bool include_removed = true) = 0;
    virtual int alivePlayerCount(bool include_removed = true) const = 0;

    virtual Player *currentPlayer() = 0;
    virtual const Player *currentPlayer() const = 0;
    virtual void setCurrentPlayer(Player *player) = 0;

    virtual void arrangeSeat(const QStringList &seatInfo) = 0;

    // ---------------- Player update & manipulation ----------------
    virtual void setPlayerScreenName(Player *player, const QString &name) = 0;
    virtual void setPlayerShownHandcards(Player *player, const IDSet &ids) = 0;
    virtual void setPlayerBrokenEquips(Player *player, const IDSet &ids) = 0;

    virtual void setPlayerHp(Player *player, int hp) = 0;
    virtual void setPlayerRenHp(Player *player, int ren_hp) = 0;
    virtual void setPlayerLingHp(Player *player, int ling_hp) = 0;
    virtual void setPlayerDyingFactor(Player *player, int dying_factor) = 0;
    virtual void setPlayerMaxHp(Player *player, int max_hp) = 0;
    virtual void setPlayerAlive(Player *player, bool is_alive) = 0;
    virtual void setPlayerChained(Player *player, bool is_chained) = 0;
    virtual void setPlayerRemoved(Player *player, bool is_removed) = 0;

    virtual void setPlayerGender(Player *player, QSanguosha::Gender gender) = 0;
    virtual void setPlayerKingdom(Player *player, const QString &kingdom) = 0;
    virtual void setPlayerRole(Player *player, const QString &rule) = 0;
    virtual void setPlayerGeneral(Player *player, const General *general, int index = 0) = 0;
    virtual void setPlayerGeneralShowed(Player *player, bool is_first = true, bool showed = true) = 0;
    // TODO(@Fs): clarify the functionality of this function.
    virtual void setPlayerState(Player *player, const QString &state) = 0;
    
    virtual void setPlayerSeat(Player *player, int seat) = 0;

    virtual void setPlayerPhase(Player *player, QSanguosha::Phase phase) = 0;
    virtual void setPlayerPhase(Player *player, const QString &phase_string) = 0;

    /**
     * @brief Get the attack range of a specific player.
     * 
     * @param player the planer to evaluate
     * 
     * @return the attack range
     * 
     * @note this function will go over all related attack range skills.
     */ 
    virtual int getPlayerAttackRange(Player *player) = 0;

    /**
     * @brief Get the distance from source to target
     * 
     * @param source the starting player for calculating distance.
     * @param target the destination player for calculating distance.
     * 
     * @return the distance
     * 
     * @note this function will go over all distance skills. 
     */ 
    virtual int getPlayerDistance(Player *source, Player *target) = 0;

    virtual void setPlayerFlag(Player *player, const QString &flag) = 0;

    /**
     * @brief bind a skill to a player's specific general card. 
     * 
     * By calling this function, the player get this skill as if the skill is on general card.
     * When the specific general is disabled or removed, the player lose the skill.
     * 
     * @param player the player
     * @param skill the name of the skill
     * @param general_index the index of the general card. default 0.
     */ 
    virtual void addGeneralCardSkillToPlayer(Player *player, const QString &skill, int general_index = 0) = 0;
    /**
     * @brief remove a skill from the player's specific general card.
     * 
     * @param player the player
     * @param skill the name of the skill
     * @param general_index the index of the general card. default 0.
     * 
     * @return true if the skill is detected and successfully removed. 
     * 
     * @see GameState::addGeneralCardSkillToPlayer
     */ 
    virtual bool removeGeneralCardSkillFromPlayer(Player *player, const QString &skill, int general_index = 0) = 0;
    /**
     * @brief Add a skill to the player. 
     * 
     * Different from the skill on general card, the acquired skill is attached to player, thus even if the general card is removed, the skill is still valid.
     * 
     * @param player the player
     * @param skill the skill name.
     * 
     * @note Acquired skill can be only removed by GameState::removeAcquiredSkillFromPlayer.
     * @note Lord's sibling-skill can be attached to player with this function.
     */ 
    virtual void addAcquiredSkillToPlayer(Player *player, const QString &skill) = 0;
    /**
     * @brief Add a skill to the player. 
     * 
     * Different from the skill on general card, the acquired skill is attached to player, thus even if the general card is removed, the skill is still valid.
     * 
     * @param player the player
     * @param skill the skill name.
     * 
     * @return true if a acquired skill is successfully removed.
     * 
     * @note Acquired skill can be only removed by this function.
     * @see GameState::addAcquiredSkillToPlayer
     */ 
    virtual void removeAcquiredSkillFromPlayer(Player *player, const QString &skill_name) = 0;

    virtual void setPlayerSkillInvalidity(Player *player, const QString &skill_name, bool is_valid) = 0;
    virtual void setPlayerSkillInvalidity(Player *player, const Skill *skill, bool is_valid) = 0;

    virtual void setPlayerSkillPreshowed(Player *player, const QString &skill, bool preshowed = true) = 0;
    virtual void setPlayerSkillsPreshowed(Player *player, const QString &flag = QStringLiteral("hd"), bool preshowed = true) = 0;

    virtual void setPlayerDisableShow(Player *player, const QString &flags, const QString &reason);
    virtual void removePlayerDisableShow(Player *player, const QString &reason);

    virtual void addPlayerEquip(Player *player, const Card *equip, QSanguosha::HandlingMethod method);
    virtual void removePlayerEquip(Player *player, const Card *equip, QSanguosha::HandlingMethod method);

    virtual void addPlayerDelayTrick(Player *player, const Card *dt, QSanguosha::HandlingMethod method);
    virtual void removePlayerDelayTrick(Player *player, const Card *dt, QSanguosha::HandlingMethod method);

    virtual bool canDiscardPlayerCard(const Player *from, const Player *to, const QString &flag, const QString &reason = QString()) = 0;
    virtual bool canDiscardPlayerCard(const Player *from, const Player *to, int card_id, const QString &reason = QString()) = 0;

    /**
     * @brief Whether it's feasible for a player to use slash to another player.
     * 
     * @param user the user of the slash
     * @param target the target to be checked of the slash.
     * @param slash the slash. A colorless virtual slash will be used if nullptr is provided.
     * @param consider_distance_limit whether to consider distance limint. default true.
     * @param attack_range_fix fix factor to user's attack range.
     * @param others TODO(@Fs) I don't know the meaning.
     * 
     * @return true if the user can.
     */ 
    virtual bool canPlayerUseSlashTo(const Player *user, const Player *target, const Card *slash = nullptr, bool consider_distance_limit = true, int attack_range_fix = 0, const QList<const Player *> &others = {}) = 0;

    /**
     * @brief Whether it's feasible for a player to use a card to another player.
     * 
     * @param user the user of the card
     * @param target the target of the card
     * @param card the card to be judged
     * @param others TODO(@Fs) I don't know the meaning.
     * 
     * @return true if the user can.
     * 
     * @note When the card is a slash, it's recommended to use GameState::canPlayerUseSlashTo.
     * @note This function will go over all target mode skills. 
     */ 
    virtual bool isCardUsageProhibited(const Player *user, const Player *target, const Card *card, const QList<Player *> others = {});

    virtual void addPlayerHistory(Player *player, const QString &key, int times = 1) = 0;
    virtual void setPlayerHistory(Player *player, const QString &key, int times) = 0;

    virtual void addPlayerMark(Player *player, const QString &mark, int add_num = 1);
    virtual void removePlayerMark(Player *player, const QString &mark, int remove_num = 1);
    virtual void setPlayerMark(Player *player, const QString &mark, int value);

    virtual void setFixedDistance(Player *from, const Player *to, int distance);

    virtual void setPlayerCardLimitation(Player *player, const QString &limit_list, const QString &pattern, const QString &reason, bool single_turn);
    virtual void removePlayerCardLimitation(Player *player, const QString &limit_list, const QString &pattern, const QString &reason, bool clearReason = false);
    virtual void clearPlayerCardLimitation(Player *player, bool single_turn);

    virtual void addPlayerCard(Player *player, const Card *card, QSanguosha::Place place);
    virtual void removePlayerCard(Player *player, const Card *card, QSanguosha::Place place);

    // TODO(Fs): What's the meaning of player_name here?
    virtual void setPlayerPipeOpen(Player *player, const QString &pile_name, const QString &player_name);

    // ---------------- Skill Management ----------------
    virtual void registerTriggerSkill(Trigger *skill) = 0;
    virtual void registerDistanceSkill(DistanceSkill *skill) = 0;
    virtual void registerProhibitSkill(ProhibitSkill *skill) = 0;
    virtual void registerTreatAsEquipmentSkill(TreatAsEquippingSkill *skill) = 0;
    virtual void registerTargetModeSkill(TargetModSkill *skill) = 0;
    virtual void registerMaxCardSkill(MaxCardsSkill *skill) = 0;
    virtual void registerAttackRangeSkill(AttackRangeSkill *skill) = 0;

    // FIXME: Duplicate function with GameState::isCardUsageProhibited
    virtual const ProhibitSkill *isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others = {}) const = 0;
    virtual const TreatAsEquippingSkill *treatAsEquipping(const Player *player, const QString &equipName, QSanguosha::EquipLocation location) const = 0;
    // FIXME: Duplicate function with GameState::getPlayerDistance
    virtual int correctDistance(const Player *from, const Player *to) const = 0;
    virtual int correctMaxCards(const Player *target, bool fixed = false, const QString &except = QString()) const = 0;
    virtual int correctCardTarget(const QSanguosha::TargetModType type, const Player *from, const Card *card) const = 0;
    // FIXME: Duplicated function with GameState::getPlayerAttackRange
    virtual int correctAttackRange(const Player *target, bool include_weapon = true, bool fixed = false) const = 0;

    QSet<const DistanceSkill *> getDistanceSkills() const;
    const ViewAsSkill *getViewAsSkill(const QString &skill_name) const;

    virtual QStringList checkPlayerTargetModSkillShown(const CardUseStruct &use);

    // ---------------- Card Management ----------------
    virtual Card *getCard(int cardId);
    virtual const Card *getCard(int cardId) const;
    /**
     * @brief Update a card in the room.
     * 
     * @param cardId Id of card to be updated.
     */ 
    virtual void resetCard(int cardId);

    virtual Card *cloneSkillCard(const QString &name);
    virtual Card *cloneDummyCard();
    virtual Card *cloneDummyCard(const IDSet &idSet);
    virtual Card *cloneCard(const Card *card);
    virtual Card *cloneCard(const QString &name, QSanguosha::Suit suit = QSanguosha::SuitToBeDecided, QSanguosha::Number number = QSanguosha::NumberToBeDecided);
    virtual Card *cloneCard(const CardFace *cardFace = nullptr, QSanguosha::Suit suit = QSanguosha::SuitToBeDecided, QSanguosha::Number number = QSanguosha::NumberToBeDecided);
    virtual Card *cloneCard(const CardDescriptor &descriptor);

    void cardDeleting(const Card *card);

    virtual void setCardFlag(const Card *card, const QString &flag, Player *who = nullptr);
    virtual void setCardFlag(int card_id, const QString &flag, Player *who = nullptr);
    virtual void clearCardFlag(const Card *card, Player *who = nullptr);
    virtual void clearCardFlag(int card_id, Player *who = nullptr);

    // ---------------- Region Information ---------------- 
    virtual QList<int> &getDiscardPile() = 0;
    virtual const QList<int> &getDiscardPile() const = 0;

    // ---------------- Global Information ----------------
    virtual bool roleStatusCommand(Player *player);
    virtual void updateRoleStatistic();

    QString getCurrentCardUsePattern() const;
    void setCurrentCardUsePattern(const QString &newPattern);
    CardUseStruct::CardUseReason getCurrentCardUseReason() const;
    void setCurrentCardUseReason(CardUseStruct::CardUseReason reason);
};

} // namespace RefactorProposal

#endif
