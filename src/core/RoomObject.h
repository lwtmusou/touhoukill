#ifndef QSANGUOSHA_ROOMOBJECT_H
#define QSANGUOSHA_ROOMOBJECT_H

#include "structs.h"

#include <QList>
#include <QObject>
#include <QPointer>

class CardFace;
struct CardDescriptor;

namespace CardFactory {
// static methods for Engine. Used to add metaobjects
// this staticMetaObject is used to call "newInstance" function to create a new card

void registerCardFace(const CardFace *face);
const CardFace *cardFace(const QString &name);
void unregisterCardFace(const QString &name);

}

class Card;
class CardFace;

class RoomObjectPrivate;

class RoomObject : public QObject
{
    Q_OBJECT

public:
    explicit RoomObject(QObject *parent = nullptr);
    ~RoomObject() override;

    Card *getCard(int cardId);
    const Card *getCard(int cardId) const;

    // WrappedCard *getWrappedCard(int cardId) const;

    QString getCurrentCardUsePattern() const;
    void setCurrentCardUsePattern(const QString &newPattern);
    CardUseStruct::CardUseReason getCurrentCardUseReason() const;
    void setCurrentCardUseReason(CardUseStruct::CardUseReason reason);

    // Update a card in the room.
    // @param cardId
    //        Id of card to be updated.
    // @param newCard
    //        Card to be updated in the room.
    // @return
    void resetCard(int cardId);
    // Reset all cards, generals' states of the room instance
    void resetState();

    virtual QList<int> &getDiscardPile() = 0;
    virtual const QList<int> &getDiscardPile() const = 0;

    Card *cloneSkillCard(const QString &name);
    Card *cloneDummyCard();
    Card *cloneDummyCard(const IDSet &idSet);
    Card *cloneCard(const Card *card);
    Card *cloneCard(const QString &name, QSanguosha::Suit suit = QSanguosha::SuitToBeDecided, QSanguosha::Number number = QSanguosha::NumberToBeDecided);
    Card *cloneCard(const CardFace *cardFace = nullptr, QSanguosha::Suit suit = QSanguosha::SuitToBeDecided, QSanguosha::Number number = QSanguosha::NumberToBeDecided);
    Card *cloneCard(const CardDescriptor &descriptor);

    void cardDeleting(const Card *card);

private:
    RoomObjectPrivate *d;
};

#endif
