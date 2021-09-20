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

} // namespace CardFactory

class Card;
class CardFace;

class RoomObjectPrivate;

class RoomObject : public QObject
{
    Q_OBJECT

public:
    explicit RoomObject(QObject *parent = nullptr);
    ~RoomObject() override;

    // -------------------- Player Related --------------------
    // Sorted by action order if any
    QList<Player *> players(bool include_dead = true);
    QList<const Player *> players(bool include_dead = true) const;
    // Register a player. DO NOT CALL THIS FUNCTION AFTER A GAME HAS BEEN STARTED!!!!
    void registerPlayer(Player *player);
    void unregisterPlayer(Player *player);
    void unregisterPlayer(const QString &objectName);

    Player *findPlayer(const QString &objectName);
    const Player *findPlayer(const QString &objectName) const;

    Player *current();
    const Player *current() const;
    void setCurrent(Player *player);

    void arrangeSeat(const QStringList &seatInfo);

    void sortPlayersByActionOrder(QList<Player *> &players) const;
    void sortPlayersByActionOrder(QList<const Player *> &players) const;
    bool comparePlayerByActionOrder(const Player *a, const Player *b) const;

    // --------------------- Card Related ---------------------
    Card *getCard(int cardId);
    const Card *getCard(int cardId) const;

    QString currentCardUsePattern() const;
    void setCurrentCardUsePattern(const QString &newPattern);
    CardUseStruct::CardUseReason currentCardUseReason() const;
    void setCurrentCardUseReason(CardUseStruct::CardUseReason reason);

    // Update a card in the room.
    // @param cardId
    //        Id of card to be updated.
    // @param newCard
    //        Card to be updated in the room.
    // @return
    void resetCard(int cardId);
    void resetAllCards();

    QList<int> &discardPile();
    const QList<int> &discardPile() const;

    Card *cloneSkillCard(const QString &name);
    Card *cloneDummyCard();
    Card *cloneDummyCard(const IDSet &idSet);
    Card *cloneCard(const Card *card);
    Card *cloneCard(const QString &name, QSanguosha::Suit suit = QSanguosha::SuitToBeDecided, QSanguosha::Number number = QSanguosha::NumberToBeDecided);
    Card *cloneCard(const CardFace *cardFace = nullptr, QSanguosha::Suit suit = QSanguosha::SuitToBeDecided, QSanguosha::Number number = QSanguosha::NumberToBeDecided);
    Card *cloneCard(const CardDescriptor &descriptor);

    void cardDeleting(const Card *card);

private:
    RoomObjectPrivate *const d;
};

#endif
