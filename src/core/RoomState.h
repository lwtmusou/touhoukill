#ifndef THKILL_ROOM_STATE_H
#define THKILL_ROOM_STATE_H

#include "WrappedCard.h"
#include "player.h"
#include "structs.h"

#include <QPointer>

// RoomState is a singleton that stores virtual generals, cards (versus factory loaded
// generals, cards in the Engine). Each room or roomscene should have one and only one
// associated RoomState.

// TODO: remove this class after refactor out of WrappedCard.
// CurrentCardUsePattern / CurrentCardUseReason should be in Room.

class RoomState
{
public:
    explicit inline RoomState() = default;
    ~RoomState();
    [[nodiscard]] Card *getCard(int cardId) const;
    [[nodiscard]] inline QString getCurrentCardUsePattern() const
    {
        return m_currentCardUsePattern;
    }
    inline void setCurrentCardUsePattern(const QString &newPattern)
    {
        m_currentCardUsePattern = newPattern;
    }
    [[nodiscard]] inline CardUseStruct::CardUseReason getCurrentCardUseReason() const
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
    // Reset all cards, generals' states of the room instance.
    // parent: the owning QObject (Client on client side, Room on server side).
    // Newly created WrappedCards get setParent(parent) to prevent double-free
    // when QML holds references to cards returned via Client::getCard.
    void reset(QObject *parent = nullptr);

protected:
    QHash<int, QPointer<WrappedCard>> m_cards;
    QString m_currentCardUsePattern;
    CardUseStruct::CardUseReason m_currentCardUseReason;
};

#endif
