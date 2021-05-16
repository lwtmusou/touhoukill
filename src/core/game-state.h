#ifndef _GAME_STATE_H_
#define _GAME_STATE_H_

#include <QObject>

#include "structs.h"
class Player;

namespace RefactorProposal {

/**
 * @class Game state keeps all the current information needed by the game.
 * 
 * It also provides a low-level API to manually modify Player's/Card's property. 
 * And these modification will not trigger game events. However, notification from server to clients will performed. 
 * 
 * Similar to the GameLogic, the server and the client should have different implementation of this class.
 * In the server, it needs to notify the clients when the game state is changed.
 * In the client, it needs to notify the UI when the game state is changed.
 * 
 */ 
class GameState : public QObject
{
    Q_OBJECT
public:
    GameState();
    virtual ~GameState();

    enum Place
    {
        PlaceHand,
        PlaceEquip,
        PlaceDelayedTrick,
        PlaceJudge,
        PlaceSpecial,
        DiscardPile,
        DrawPile,
        PlaceTable,
        PlaceUnknown,
        PlaceWuGu
    };

    enum Role
    {
        Lord,
        Loyalist,
        Rebel,
        Renegade
    };

    // Game state records all the information required for one game, including
    // - Player update (if so all the setters of Player's properties should be hidden except this class.)
    // - Card management
    // - Region informaiton (Global region)
    // - Global management

    // Player update & manipulation
    virtual void attachSkillToPlayer(Player *player, const QString &skill_name, bool is_other_attach = false);
    virtual void detachSkillFromPlayer(Player *player, const QString &skill_name, bool is_equip = false, bool acquire_only = false, bool sendlog = true, bool head = true);

    virtual void setPlayerFlag(Player *player, const QString &flag);

    virtual void setPlayerProperty(Player *player, const char *property_name, const QVariant &value);

    virtual void addPlayerMark(Player *player, const QString &mark, int add_num = 1);
    virtual void removePlayerMark(Player *player, const QString &mark, int remove_num = 1);
    virtual void setPlayerMark(Player *player, const QString &mark, int value);

    virtual void setFixedDistance(Player *from, const Player *to, int distance);

    virtual void swapSeat(Player *a, Player *b);

    virtual void setPlayerCardLimitation(Player *player, const QString &limit_list, const QString &pattern, const QString &reason, bool single_turn);
    virtual void removePlayerCardLimitation(Player *player, const QString &limit_list, const QString &pattern, const QString &reason, bool clearReason = false);
    virtual void clearPlayerCardLimitation(Player *player, bool single_turn);

    virtual void setPlayerDisableShow(Player *player, const QString &flags, const QString &reason);
    virtual void removePlayerDisableShow(Player *player, const QString &reason);

    virtual void changePlayerGeneral(Player *player, const QString &new_general);
    virtual void changePlayerGeneral2(Player *player, const QString &new_general);

    virtual void marshal(Player *player);

    virtual void addPlayerHistory(Player *player, const QString &key, int times = 1);

    virtual int aliveCount(bool includeRemoved = true) const;

    virtual const Player *getLord() const;
    virtual const Player *getCurrent() const;

    virtual void addPlayerEquip(Player *player, const Card *equip, Card::HandlingMethod method);
    virtual void removePlayerEquip(Player *player, const Card *equip, Card::HandlingMethod method);

    virtual void addPlayerDelayTrick(Player *player, const Card *dt, Card::HandlingMethod method);
    virtual void removePlayerDelayTrick(Player *player, const Card *dt, Card::HandlingMethod method);

    virtual void addPlayerCard(Player *player, const Card *card, Place place);
    virtual void removePlayerCard(Player *player, const Card *card, Place place);

    virtual void setPlayerChained(Player *player, bool chained);

    // FIXME(Xusine): What's the meaning of player_name here?
    virtual void setPlayerPipeOpen(Player *player, const QString &pile_name, const QString &player_name);

    virtual void addPlayerQinggangTag(Player *player, const Card *card);
    virtual void removePlayerQinggangTag(Player *player, const Card *card);

    virtual void setPlayerSkillPreshowed(Player *player, const QString &skill, bool preshowed = true);
    virtual void setPlayerSkillsPreshowed(Player *player, const QString &flag = "hd", bool preshowed = true);

    virtual void setPlayerGeneralShowed(Player *player, bool is_first = true, bool showed = true);

    virtual QStringList checkPlayerTargetModSkillShown(const CardUseStruct &use);

    // Card Management
    virtual Card *getCard(int cardId);
    virtual const Card *getCard(int cardId) const;
    // Update a card in the room.
    // @param cardId
    //        Id of card to be updated.
    // @param newCard
    //        Card to be updated in the room.
    // @return
    virtual void resetCard(int cardId);

    virtual Card *cloneSkillCard(const QString &name);
    virtual Card *cloneDummyCard();
    virtual Card *cloneDummyCard(const IDSet &idSet);
    virtual Card *cloneCard(const Card *card);
    virtual Card *cloneCard(const QString &name, Card::Suit suit = Card::SuitToBeDecided, Card::Number number = Card::NumberToBeDecided);
    virtual Card *cloneCard(const CardFace *cardFace = nullptr, Card::Suit suit = Card::SuitToBeDecided, Card::Number number = Card::NumberToBeDecided);
    virtual Card *cloneCard(const CardDescriptor &descriptor);

    void cardDeleting(const Card *card);

    virtual void setCardFlag(const Card *card, const QString &flag, Player *who = nullptr);
    virtual void setCardFlag(int card_id, const QString &flag, Player *who = nullptr);
    virtual void clearCardFlag(const Card *card, Player *who = nullptr);
    virtual void clearCardFlag(int card_id, Player *who = nullptr);

    // Region Information
    virtual QList<int> &getDiscardPile() = 0;
    virtual const QList<int> &getDiscardPile() const = 0;

    // Global Information
    // Reset all cards, generals' states of the room instance
    virtual void resetState();

    virtual void adjustSeats(); 

    virtual bool roleStatusCommand(Player *player); 
    virtual void updateRoleStatistic();

    QString getCurrentCardUsePattern() const;
    void setCurrentCardUsePattern(const QString &newPattern);
    CardUseStruct::CardUseReason getCurrentCardUseReason() const;
    void setCurrentCardUseReason(CardUseStruct::CardUseReason reason);

private:
    // TODO: Determine this part.
    // QList<Player *> players;
};

}

#endif