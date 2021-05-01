#include "RoomObject.h"
#include "engine.h"

QHash<QString, const CardFace *> CardFactory::faces;

// void CardFactory::addCardMetaObject(const QString &key, const QMetaObject *staticMetaObject)
// {
//     metaObjects[key] = staticMetaObject;
// }

// void CardFactory::removeCardMetaObject(const QString &key)
// {
//     metaObjects.remove(key);
// }

void CardFactory::registerCardFace(const CardFace *cardFace) {
    faces.insert(cardFace->name(), cardFace);
}

void CardFactory::unregisterCardFace(const QString &name) {
    auto face = faces.find(name);
    if(face != faces.end()){
        auto handle = *face;
        faces.erase(face);
        delete handle;
    }
}


CardFactory::CardFactory()
{
}

Card *CardFactory::cloneCard(const Card *card) const
{
    Q_ASSERT(card->face() != nullptr);
    auto name = card->faceName();
    Card *result = cloneCard(name, card->suit(), card->number(), card->flags());
    if (result == nullptr)
        return nullptr;
    result->setID(card->effectiveID());
    result->setSkillName(card->skillName(false));
    // result->setName(card->name());
    return result;
}

Card *CardFactory::cloneCard(const QString &name, Card::Suit suit, Card::Number number, const QSet<QString> &flags) const
{
    Card *card = nullptr;

    const CardFace *face = faces.value(name, nullptr);
    if (face != nullptr) {
        Card *card = new Card(nullptr, face, suit, number);
        // card_obj->setObjectName(className2objectName.value(name, name));
        // TODO: Bind the roomObject
    }

    if (card == nullptr)
        return nullptr;
    card->clearFlags();
    if (!flags.isEmpty()) {
        foreach (const QString &flag, flags)
            card->addFlag(flag);
    }
    return card;
}

Card *CardFactory::cloneSkillCard(const QString &name) const
{
    // TODO: Here we have to manually maintain all the skill card face.

    // const QMetaObject *meta = metaObjects.value(name, NULL);
    // if (meta != nullptr) {
    //     QObject *card_obj = meta->newInstance();
    //     SkillCard *card = qobject_cast<SkillCard *>(card_obj);
    //     if (card == nullptr)
    //         delete card_obj;
    //     return card;
    // }

    return cloneCard(name);
}

class RoomObjectPrivate
{
public:
    QHash<int, Card *> cards;
    QString currentCardUsePattern;
    CardUseStruct::CardUseReason currentCardUseReason;
    CardFactory cardFactory;
    QList<QPointer<Card> > clonedCardsPreRefactor;
    QList<const Card *> clonedCards;

    RoomObjectPrivate()
    {
    }
};

RoomObject::RoomObject(QObject *parent)
    : QObject(parent)
    , d(new RoomObjectPrivate)
{
}

RoomObject::~RoomObject()
{
    qDeleteAll(d->cards);
    qDeleteAll(d->clonedCardsPreRefactor);
    qDeleteAll(d->clonedCards);

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
    // It depends on how we achieve the filter skill.

    // Card *newCard = Card::Clone(Sanguosha->getEngineCard(cardId));
    // if (newCard == nullptr)
    //     return;
    // newCard->setFlags(d->cards[cardId]->getFlags());
    // d->cards[cardId]->copyEverythingFrom(newCard);
    // newCard->clearFlags();
    // d->cards[cardId]->setModified(false);
}

// Reset all cards, generals' states of the room instance
void RoomObject::resetState()
{
    foreach (Card *card, d->cards.values())
        delete card;
    d->cards.clear();

    int n = Sanguosha->getCardCount();
    for (int i = 0; i < n; i++) {
        const Card *card = Sanguosha->getEngineCard(i);
        d->cards[i] = cloneCard(card);
    }
}

Card *RoomObject::cloneCard(const Card *card)
{
    Card *c = d->cardFactory.cloneCard(card);
    if (c == nullptr)
        return nullptr;
    c->setRoomObject(this);
    d->clonedCardsPreRefactor << c;
    return c;
}

Card *RoomObject::cloneCard(const QString &name, Card::Suit suit, Card::Number number, const QSet<QString> &flags)
{
    Card *c = d->cardFactory.cloneCard(name, suit, number, flags);
    if (c == nullptr)
        return nullptr;
    c->setRoomObject(this);
    d->clonedCardsPreRefactor << c;
    return c;
}

// Card *RoomObject::cloneCard(const Card *card)
// {
//     return cloneCard(card->face(), card->suit(), card->number());
// }

Card *RoomObject::cloneCard(const CardFace *cardFace, Card::Suit suit, Card::Number number, const QSet<QString> &flags)
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

void RoomObject::cardDeleting(const Card *card)
{
    d->clonedCards.removeAll(card);
}

Card *RoomObject::cloneSkillCard(const QString &name)
{
    Card *c = d->cardFactory.cloneSkillCard(name);
    if (c == nullptr)
        return nullptr;

    d->clonedCardsPreRefactor << c;
    return c;
}

void RoomObject::autoCleanupClonedCards()
{
    for (QList<QPointer<Card> >::iterator it = d->clonedCardsPreRefactor.begin(); it != d->clonedCardsPreRefactor.end(); ++it) {
        if ((*it).isNull())
            d->clonedCardsPreRefactor.erase(it);
    }
}
