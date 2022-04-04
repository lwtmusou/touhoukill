#ifndef QSANGUOSHA_ROOMOBJECT_H
#define QSANGUOSHA_ROOMOBJECT_H

#include "qsgscore.h"
#include "structs.h"

#include <QList>
#include <QObject>
#include <QPointer>

class CardFace;
struct CardDescriptor;
class ProhibitSkill;
class TreatAsEquippingSkill;
class DistanceSkill;
class ViewAsSkill;
class Card;

class RoomObjectPrivate;

class QSGS_CORE_EXPORT RoomObject : public QObject
{
    Q_OBJECT

public:
    explicit RoomObject(QObject *parent = nullptr);
    ~RoomObject() override;

    // -------------------- Player Related --------------------
    // Sorted by action order if action order is decided
    QList<Player *> players(bool include_dead = true, bool include_removed = true);
    QList<const Player *> players(bool include_dead = true, bool include_removed = true) const;
    // Register a player. DO NOT CALL THIS FUNCTION AFTER A GAME HAS BEEN STARTED!!!!
    void registerPlayer(Player *player);
    void unregisterPlayer(Player *player);
    void unregisterPlayer(const QString &objectName);

    Player *findPlayer(const QString &objectName);
    const Player *findPlayer(const QString &objectName) const;

    Player *current();
    const Player *current() const;
    void setCurrent(Player *player);

    Player *findAdjecentPlayer(Player *player, bool next = true, bool include_dead = true, bool include_removed = true);
    const Player *findAdjecentPlayer(const Player *player, bool next = true, bool include_dead = true, bool include_removed = true) const;

    void arrangeSeat(const QStringList &seatInfo);

    void sortPlayersByActionOrder(QList<Player *> &players) const;
    void sortPlayersByActionOrder(QList<const Player *> &players) const;
    bool comparePlayerByActionOrder(const Player *a, const Player *b) const;

    // --------------------- Card Related ---------------------
    Card *getCard(int cardId);
    const Card *getCard(int cardId) const;
    QSet<Card *> getCards();
    QSet<const Card *> getCards() const;

    QString currentCardUsePattern() const;
    void setCurrentCardUsePattern(const QString &newPattern);
    QSanguosha::CardUseReason currentCardUseReason() const;
    void setCurrentCardUseReason(QSanguosha::CardUseReason reason);

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
    Card *cloneDummyCard(const IdSet &idSet);
    Card *cloneCard(const Card *card);
    Card *cloneCard(const QString &name, QSanguosha::Suit suit = QSanguosha::SuitToBeDecided, QSanguosha::Number number = QSanguosha::NumberToBeDecided);
    Card *cloneCard(const CardFace *cardFace = nullptr, QSanguosha::Suit suit = QSanguosha::SuitToBeDecided, QSanguosha::Number number = QSanguosha::NumberToBeDecided);
    Card *cloneCard(const CardDescriptor &descriptor);

    void cardDeleting(const Card *card);

    // --------------------- Skill Related ---------------------
    const ProhibitSkill *isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;
    const TreatAsEquippingSkill *treatAsEquipping(const Player *player, const QString &equipName, QSanguosha::EquipLocation location) const;
    int correctDistance(const Player *from, const Player *to) const;
    int correctMaxCards(const Player *target, bool fixed = false, const QString &except = QString()) const;
    int correctCardTarget(QSanguosha::TargetModType type, const Player *from, const Card *card) const;
    int correctAttackRange(const Player *target, bool include_weapon = true, bool fixed = false) const;

    QSet<const DistanceSkill *> getDistanceSkills() const;
    const ViewAsSkill *getViewAsSkill(const QString &skill_name) const;

    void loadSkill(const Skill *skill);

private:
    RoomObjectPrivate *const d;
};

// TODO_Fs: find a suitable way for this
namespace QinggangSword {
QSGS_CORE_EXPORT void addQinggangTag(Player *p, const Card *card);
QSGS_CORE_EXPORT void removeQinggangTag(Player *p, const Card *card);
} // namespace QinggangSword

#endif
