#ifndef _ROOM_STATE_H
#define _ROOM_STATE_H

// #include "WrappedCard.h"
#include "player.h"
#include "structs.h"
#include "card.h"

#include <QList>
#include <QObject>
#include <QPointer>

class SkillCard;

class CardFactory
{
public:
    // static methods for Engine. Used to add metaobjects
    // this staticMetaObject is used to call "newInstance" function to create a new card
    static void addCardMetaObject(const QString &key, const QMetaObject *staticMetaObject);
    static void removeCardMetaObject(const QString &key);

public:
    CardFactory();

    Card *cloneCard(const Card *card) const;
    Card *cloneCard(const QString &name, Card::Suit suit = Card::SuitToBeDecided, int number = -1, const QStringList &flags = QStringList()) const;
    SkillCard *cloneSkillCard(const QString &name) const;

private:
    static QHash<QString, const QMetaObject *> metaObjects;
};

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

    SkillCard *cloneSkillCard(const QString &name);
    void autoCleanupClonedCards();

    Card *cloneCard(const Card *card);
#if 0
    Card *cloneCard(const QString &name, Card::Suit suit = Card::SuitToBeDecided,
                                      Card::Number number = Card::NumberToBeDecided);
#else
    // Fs: after Refactor done, replace the function to the one with default value
    Card *cloneCard(const QString &name, Card::Suit suit, Card::Number number);
#endif
    Card *cloneCard(const CardFace *cardFace, Card::Suit suit = Card::SuitToBeDecided,
                                      Card::Number number = Card::NumberToBeDecided);
    void cardDeleting(const Card *card);

private:
    RoomObjectPrivate *d;
};

#endif
