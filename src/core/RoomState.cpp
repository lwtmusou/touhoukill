#include "RoomState.h"
#include "WrappedCard.h"
#include "engine.h"

RoomState::~RoomState()
{
    foreach (Card *card, m_cards.values())
        delete card;
    m_cards.clear();
}

Card *RoomState::getCard(int cardId) const
{
    if (!m_cards.contains(cardId))
        return nullptr;
    return m_cards[cardId];
}

void RoomState::resetCard(int cardId)
{
    Card *newCard = Card::Clone(Sanguosha->getEngineCard(cardId));
    if (newCard == nullptr)
        return;
    newCard->setFlags(m_cards[cardId]->getFlags());
    m_cards[cardId]->copyEverythingFrom(newCard);
    newCard->clearFlags();
    m_cards[cardId]->setModified(false);
}

// Reset all cards, generals' states of the room instance
void RoomState::reset()
{
    foreach (WrappedCard *card, m_cards.values())
        delete card;
    m_cards.clear();

    int n = Sanguosha->getCardCount();
    for (int i = 0; i < n; i++) {
        const Card *card = Sanguosha->getEngineCard(i);
        m_cards[i] = new WrappedCard(Card::Clone(card));
    }
}
