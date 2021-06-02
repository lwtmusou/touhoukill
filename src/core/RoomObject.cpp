#include "RoomObject.h"
#include "engine.h"
namespace CardFactory {
QHash<QString, const CardFace *> faces;

void registerCardFace(const CardFace *cardFace)
{
    faces.insert(cardFace->name(), cardFace);
}

const CardFace *cardFace(const QString &name)
{
    return faces.value(name, nullptr);
}

void unregisterCardFace(const QString &name)
{
    auto face = faces.find(name);
    if (face != faces.end()) {
        const auto *handle = *face;
        faces.erase(face);
        delete handle;
    }
}
}

class RoomObjectPrivate
{
public:
    QHash<int, Card *> cards;
    QString currentCardUsePattern;
    CardUseStruct::CardUseReason currentCardUseReason;
    QList<Card *> clonedCards;

    RoomObjectPrivate() = default;
};

RoomObject::RoomObject(QObject *parent)
    : QObject(parent)
    , d(new RoomObjectPrivate)
{
}

RoomObject::~RoomObject()
{
    foreach (Card *c, d->cards)
        delete c;

    foreach (Card *c, d->clonedCards)
        delete c;

    delete d;
}

Card *RoomObject::getCard(int cardId)
{
    if (!d->cards.contains(cardId))
        return nullptr;
    return d->cards[cardId];
}

const Card *RoomObject::getCard(int cardId) const
{
    if (!d->cards.contains(cardId))
        return nullptr;
    return d->cards[cardId];
}

QString RoomObject::getCurrentCardUsePattern() const
{
    return d->currentCardUsePattern;
}

void RoomObject::setCurrentCardUsePattern(const QString &newPattern)
{
    d->currentCardUsePattern = newPattern;
}

CardUseStruct::CardUseReason RoomObject::getCurrentCardUseReason() const
{
    return d->currentCardUseReason;
}

void RoomObject::setCurrentCardUseReason(CardUseStruct::CardUseReason reason)
{
    d->currentCardUseReason = reason;
}

void RoomObject::resetCard(int cardId)
{
    // TODO_Fs: error check and sanity check
    delete d->cards[cardId];
    d->cards[cardId] = cloneCard(Sanguosha->getEngineCard(cardId));

    // It does not depend on how we achieve the filter skill.
}

// Reset all cards, generals' states of the room instance
void RoomObject::resetState()
{
    foreach (Card *card, d->cards.values())
        delete card;
    d->cards.clear();

    int n = Sanguosha->getCardCount();
    for (int i = 0; i < n; i++)
        d->cards[i] = cloneCard(Sanguosha->getEngineCard(i));
}

Card *RoomObject::cloneCard(const Card *card)
{
    return cloneCard(card->face(), card->suit(), card->number());
}

Card *RoomObject::cloneCard(const QString &name, Card::Suit suit, Card::Number number)
{
    const CardFace *face = nullptr;

    if (!name.isEmpty()) {
        face = CardFactory::cardFace(name);
        if (face == nullptr && name != QStringLiteral("DummyCard"))
            return nullptr;
    }
    return cloneCard(face, suit, number);
}

Card *RoomObject::cloneCard(const CardFace *cardFace, Card::Suit suit, Card::Number number)
{
    // Design change: dummy cards does not have CardFace
#if 0
    if (cardFace == nullptr) {
        qDebug() << "RoomObject::cloneCard - cardFace is nullptr";
        return nullptr;
    }
#endif

    Card *c = new Card(this, cardFace, suit, number);
    d->clonedCards << c;
    return c;
}

Card *RoomObject::cloneCard(const CardDescriptor &descriptor)
{
    return cloneCard(descriptor.face(), descriptor.suit, descriptor.number);
}

void RoomObject::cardDeleting(const Card *card)
{
    // const_cast is necessary for this function since card may be const
    // but for Room, this Card is variable
    if (card != nullptr)
        d->clonedCards.removeAll(const_cast<Card *>(card));
    delete card;
}

Card *RoomObject::cloneSkillCard(const QString &name)
{
    return cloneCard(name);
}

Card *RoomObject::cloneDummyCard()
{
    return cloneCard();
}

Card *RoomObject::cloneDummyCard(const IDSet &idSet)
{
    Card *c = cloneCard();
    if (c == nullptr)
        return nullptr;

    c->addSubcards(idSet);

    return c;
}
