#include "RoomObject.h"
#include "WrappedCard.h"
#include "engine.h"

QHash<QString, const QMetaObject *> PreRefactor::CardFactory::metaObjects;

void PreRefactor::CardFactory::addCardMetaObject(const QString &key, const QMetaObject *staticMetaObject)
{
    metaObjects[key] = staticMetaObject;
}

void PreRefactor::CardFactory::removeCardMetaObject(const QString &key)
{
    metaObjects.remove(key);
}

PreRefactor::CardFactory::CardFactory()
{
}

Card *PreRefactor::CardFactory::cloneCard(const Card *card) const
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

Card *PreRefactor::CardFactory::cloneCard(const QString &name, Card::Suit suit, int number, const QStringList &flags) const
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

SkillCard *PreRefactor::CardFactory::cloneSkillCard(const QString &name) const
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

class RoomObjectPrivate
{
public:
    QHash<int, WrappedCard *> cards;
    QString currentCardUsePattern;
    CardUseStruct::CardUseReason currentCardUseReason;
    PreRefactor::CardFactory cardFactory;
    QList<QPointer<Card> > clonedCardsPreRefactor;
    QList<const RefactorProposal::Card *> clonedCards;

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
    return getWrappedCard(cardId);
}

const Card *RoomObject::getCard(int cardId) const
{
    return getWrappedCard(cardId);
}

WrappedCard *RoomObject::getWrappedCard(int cardId) const
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
    Card *newCard = Card::Clone(Sanguosha->getEngineCard(cardId));
    if (newCard == nullptr)
        return;
    newCard->setFlags(d->cards[cardId]->getFlags());
    d->cards[cardId]->copyEverythingFrom(newCard);
    newCard->clearFlags();
    d->cards[cardId]->setModified(false);
}

// Reset all cards, generals' states of the room instance
void RoomObject::resetState()
{
    foreach (WrappedCard *card, d->cards.values())
        delete card;
    d->cards.clear();

    int n = Sanguosha->getCardCount();
    for (int i = 0; i < n; i++) {
        const Card *card = Sanguosha->getEngineCard(i);
        d->cards[i] = new WrappedCard(Card::Clone(card));
    }
}

Card *RoomObject::cloneCard(const Card *card)
{
    Card *c = d->cardFactory.cloneCard(card);
    if (c == nullptr)
        return nullptr;

    d->clonedCardsPreRefactor << c;
    return c;
}

Card *RoomObject::cloneCard(const QString &name, Card::Suit suit, int number, const QStringList &flags)
{
    Card *c = d->cardFactory.cloneCard(name, suit, number, flags);
    if (c == nullptr)
        return nullptr;

    d->clonedCardsPreRefactor << c;
    return c;
}

RefactorProposal::Card *RoomObject::cloneCard(const RefactorProposal::Card *card)
{
    return cloneCard(card->face(), card->suit(), card->number());
}

RefactorProposal::Card *RoomObject::cloneCard(const QString &name, RefactorProposal::Card::Suit suit, RefactorProposal::Card::Number number)
{
    const RefactorProposal::CardFace *face = nullptr;
    if (name != "DummyCard")
        face = Sanguosha->getCardFace(name);

    return cloneCard(face, suit, number);
}

RefactorProposal::Card *RoomObject::cloneCard(const RefactorProposal::CardFace *cardFace, RefactorProposal::Card::Suit suit, RefactorProposal::Card::Number number)
{
    // Design change: dummy cards does not have CardFace
#if 0
    if (cardFace == nullptr) {
        qDebug() << "RoomObject::cloneCard - cardFace is nullptr";
        return nullptr;
    }
#endif

    RefactorProposal::Card *c = new RefactorProposal::Card(this, cardFace, suit, number);
    d->clonedCards << c;
    return c;
}

void RoomObject::cardDeleting(const RefactorProposal::Card *card)
{
    d->clonedCards.removeAll(card);
}

SkillCard *RoomObject::cloneSkillCard(const QString &name)
{
    SkillCard *c = d->cardFactory.cloneSkillCard(name);
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
