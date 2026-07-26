#include "RoomState.h"
#include "WrappedCard.h"
#include "engine.h"

RoomState::~RoomState()
{
    foreach (QPointer<WrappedCard> card, m_cards)
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
    Card *newCard = Card::Clone(Sanguosha->getEngineCard(cardId)); // Caution: memory leak here
    if (newCard == nullptr)
        return;
    newCard->setFlags(m_cards[cardId]->getFlags());
    m_cards[cardId]->copyEverythingFrom(newCard);
    newCard->clearFlags();
    m_cards[cardId]->setModified(false);
}

// Reset all cards, generals' states of the room instance
void RoomState::reset(QObject *parent)
{
    foreach (QPointer<WrappedCard> card, m_cards)
        delete card;
    m_cards.clear();

    int n = Sanguosha->getCardCount();
    for (int i = 0; i < n; i++) {
        const Card *card = Sanguosha->getEngineCard(i);
        WrappedCard *wc = new WrappedCard(Card::Clone(card));
        if (parent != nullptr)
            wc->setParent(parent);
        m_cards[i] = wc;
    }
}
