#include "RoomObject.h"
#include "WrappedCard.h"
#include "engine.h"

RoomObject::RoomObject(QObject *parent)
    : QObject(parent)
{
}

RoomObject::~RoomObject()
{
    foreach (Card *card, m_cards.values())
        delete card;
    m_cards.clear();
}

Card *RoomObject::getCard(int cardId) const
{
    if (!m_cards.contains(cardId))
        return NULL;
    return m_cards[cardId];
}

void RoomObject::resetCard(int cardId)
{
    Card *newCard = Card::Clone(Sanguosha->getEngineCard(cardId));
    if (newCard == NULL)
        return;
    newCard->setFlags(m_cards[cardId]->getFlags());
    m_cards[cardId]->copyEverythingFrom(newCard);
    newCard->clearFlags();
    m_cards[cardId]->setModified(false);
}

// Reset all cards, generals' states of the room instance
void RoomObject::resetState()
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
