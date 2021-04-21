#include "RoomObject.h"
#include "WrappedCard.h"
#include "engine.h"

QHash<QString, const QMetaObject *> CardFactory::metaObjects;

void CardFactory::addCardMetaObject(const QString &key, const QMetaObject *staticMetaObject)
{
    metaObjects[key] = staticMetaObject;
}

void CardFactory::removeCardMetaObject(const QString &key)
{
    metaObjects.remove(key);
}

CardFactory::CardFactory()
{
}

Card *CardFactory::cloneCard(const Card *card) const
{
    Q_ASSERT(card->metaObject() != nullptr);
    QString name = card->metaObject()->className();
    Card *result = cloneCard(name, card->getSuit(), card->getNumber(), card->getFlags());
    if (result == nullptr)
        return nullptr;
    result->setId(card->getEffectiveId());
    result->setSkillName(card->getSkillName(false));
    result->setObjectName(card->objectName());
    return result;
}

Card *CardFactory::cloneCard(const QString &name, Card::Suit suit, int number, const QStringList &flags) const
{
    Card *card = nullptr;

    const QMetaObject *meta = metaObjects.value(name, NULL);
    if (meta != nullptr) {
        QObject *card_obj = meta->newInstance(Q_ARG(Card::Suit, suit), Q_ARG(int, number));
        // card_obj->setObjectName(className2objectName.value(name, name));
        card = qobject_cast<Card *>(card_obj);
    }

    if (card == nullptr)
        return nullptr;
    card->clearFlags();
    if (!flags.isEmpty()) {
        foreach (const QString &flag, flags)
            card->setFlags(flag);
    }
    return card;
}

SkillCard *CardFactory::cloneSkillCard(const QString &name) const
{
    const QMetaObject *meta = metaObjects.value(name, NULL);
    if (meta != nullptr) {
        QObject *card_obj = meta->newInstance();
        SkillCard *card = qobject_cast<SkillCard *>(card_obj);
        if (card == nullptr)
            delete card_obj;
        return card;
    }

    return nullptr;
}

RoomObject::RoomObject(QObject *parent)
    : QObject(parent)
{
}

RoomObject::~RoomObject()
{
    foreach (Card *card, m_cards)
        delete card;
    foreach (QPointer<Card> card, m_clonedCards)
        delete card;
}

Card *RoomObject::getCard(int cardId) const
{
    return getWrappedCard(cardId);
}

WrappedCard *RoomObject::getWrappedCard(int cardId) const
{
    if (!m_cards.contains(cardId))
        return nullptr;
    return m_cards[cardId];
}

void RoomObject::resetCard(int cardId)
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

Card *RoomObject::cloneCard(const Card *card)
{
    Card *c = cardFactory.cloneCard(card);
    if (c == nullptr)
        return nullptr;

    m_clonedCards << c;
    return c;
}

Card *RoomObject::cloneCard(const QString &name, Card::Suit suit, int number, const QStringList &flags)
{
    Card *c = cardFactory.cloneCard(name, suit, number, flags);
    if (c == nullptr)
        return nullptr;

    m_clonedCards << c;
    return c;
}

SkillCard *RoomObject::cloneSkillCard(const QString &name)
{
    SkillCard *c = cardFactory.cloneSkillCard(name);
    if (c == nullptr)
        return nullptr;

    m_clonedCards << c;
    return c;
}

void RoomObject::autoCleanupClonedCards()
{
    for (QList<QPointer<Card> >::iterator it = m_clonedCards.begin(); it != m_clonedCards.end(); ++it) {
        if ((*it).isNull())
            m_clonedCards.erase(it);
    }
}
