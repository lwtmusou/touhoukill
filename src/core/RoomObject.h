#ifndef _ROOM_STATE_H
#define _ROOM_STATE_H

#include "WrappedCard.h"
#include "player.h"
#include "structs.h"

#include <QList>
#include <QObject>
#include <QPointer>

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

class RoomObject : public QObject
{
    Q_OBJECT

public:
    explicit RoomObject(QObject *parent = nullptr);
    ~RoomObject() override;
    Card *getCard(int cardId) const;
    WrappedCard *getWrappedCard(int cardId) const;

    inline QString getCurrentCardUsePattern() const
    {
        return m_currentCardUsePattern;
    }
    inline void setCurrentCardUsePattern(const QString &newPattern)
    {
        m_currentCardUsePattern = newPattern;
    }
    inline CardUseStruct::CardUseReason getCurrentCardUseReason() const
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
    // Reset all cards, generals' states of the room instance
    void resetState();

    virtual QList<int> &getDiscardPile() = 0;
    virtual const QList<int> &getDiscardPile() const = 0;

    Card *cloneCard(const Card *card);
    Card *cloneCard(const QString &name, Card::Suit suit = Card::SuitToBeDecided, int number = -1, const QStringList &flags = QStringList());
    SkillCard *cloneSkillCard(const QString &name);

    void autoCleanupClonedCards();

protected:
    QHash<int, WrappedCard *> m_cards;
    QString m_currentCardUsePattern;
    CardUseStruct::CardUseReason m_currentCardUseReason;
    CardFactory cardFactory;
    QList<QPointer<Card> > m_clonedCards;
};

#endif
