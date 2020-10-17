#ifndef _ROOM_STATE_H
#define _ROOM_STATE_H

#include "WrappedCard.h"
#include "player.h"
#include "structs.h"

// RoomState is a singleton that stores virtual generals, cards (versus factory loaded
// generals, cards in the Engine). Each room or roomscene should have one and only one
// associated RoomState.

class RoomObject
{
public:
    explicit inline RoomObject(bool isClient)
    {
        m_isClient = isClient;
    }
    ~RoomObject();
    inline bool isClient() const
    {
        return m_isClient;
    }
    Card *getCard(int cardId) const;
    inline void setCurrentPlayer(Player *player)
    {
        m_currentPlayer = player;
    }
    inline QString getCurrentCardUsePattern() const
    {
        return m_currentCardUsePattern;
    }
    inline void setCurrentCardUsePattern(const QString &newPattern)
    {
        m_currentCardUsePattern = newPattern;
    }
    inline Player *getCurrentPlayer() const
    {
        return m_currentPlayer;
    }
    inline CardUseStruct::CardUseReason getCurrentCardUseReason() const
    {
        return m_currentCardUseReason;
    }
    inline void setCurrentCardUseReason(CardUseStruct::CardUseReason reason)
    {
        m_currentCardUseReason = reason;
    }

    // Update a card in the room.
    // @param cardId
    //        Id of card to be updated.
    // @param newCard
    //        Card to be updated in the room.
    // @return
    void resetCard(int cardId);
    // Reset all cards, generals' states of the room instance
    void reset();

protected:
    QHash<int, WrappedCard *> m_cards;
    bool m_isClient;
    Player *m_currentPlayer;
    QString m_currentCardUsePattern;
    CardUseStruct::CardUseReason m_currentCardUseReason;
};

#endif
