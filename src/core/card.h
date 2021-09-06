#ifndef _CARD_H
#define _CARD_H

#include "global.h"

#include <QIcon>
#include <QMap>
#include <QObject>
#include <QSet>

class RoomObject;
class Player;
struct CardEffectStruct;
struct CardUseStruct;

class CardPrivate;
class CardFace;

class Card final
{
    Q_GADGET

    friend class RoomObject;

public:
    static const int S_UNKNOWN_CARD_ID;

private:
    // constructor to create real card
    explicit Card(RoomObject *room, const CardFace *face, QSanguosha::Suit suit = QSanguosha::SuitToBeDecided, QSanguosha::Number number = QSanguosha::NumberToBeDecided,
                  int id = -1);
    ~Card();

public:
    // Suit method
    QSanguosha::Suit suit() const;
    void setSuit(QSanguosha::Suit suit);
    QString suitString() const;
    bool isRed() const;
    bool isBlack() const;
    QSanguosha::Color color() const;

    // Number method
    QSanguosha::Number number() const;
    void setNumber(QSanguosha::Number number);
    QString numberString() const;

    // id
    int id() const;
    void setID(int id);
    int effectiveID() const;

    // name
    QString faceName() const;
    QString fullName(bool include_suit = false) const;
    QString logName() const;

    // ??
    bool isModified() const;

    // skill name
    QString skillName(bool removePrefix = true) const;
    void setSkillName(const QString &skill_name);
    const QString &showSkillName() const;
    void setShowSkillName(const QString &show_skill_name);

    // handling method
    QSanguosha::HandlingMethod handleMethod() const;
    void setHandleMethod(QSanguosha::HandlingMethod method);

    // property (override the CardFace)
    bool canDamage() const;
    void setCanDamage(bool can);
    bool canRecover() const;
    void setCanRecover(bool can);
    bool canRecast() const;
    void setCanRecast(bool can);
    bool hasEffectValue() const;
    void setHasEffectValue(bool has);
    bool throwWhenUsing() const;
    void setThrowWhenUsing(bool thrown);

    // Face (functional model)
    const CardFace *face() const;
    // For compulsory view as skill. (Doesn't this kind of skill should return a new card and make that card as subcard?)
    void setFace(const CardFace *face);

    // Flags
    const QSet<QString> &flags() const;
    void addFlag(const QString &flag) const /* mutable */;
    void addFlags(const QSet<QString> &flags) const /* mutable */;
    void removeFlag(const QString &flag) const /* mutable */;
    void removeFlag(const QSet<QString> &flag) const /* mutable */;
    void clearFlags() const /* mutable */;
    bool hasFlag(const QString &flag) const;

    // Virtual Card
    bool isVirtualCard() const;

    // Subcard
    const IDSet &subcards() const;
    void addSubcard(int card_id);
    void addSubcard(const Card *card);
    void addSubcards(const IDSet &subcards);
    void clearSubcards();
    QString subcardString() const; // Used for converting card to string

    // Status
    // Same method as Player::hasEquip
    // bool isEquipped(const Player *self) const;

    // UI property
    bool mute() const;
    void setMute(bool mute);

    const QString &userString() const;
    void setUserString(const QString &str);

    // room Object
    RoomObject *room();
    const RoomObject *room() const;
    void setRoomObject(RoomObject *room);

    // toString
    // What's the meaning of this hidden card?
    // Fs: SkillCard with subcard which does not always need to show to others
    QString toString(bool hidden = false) const;

    // helpers
    // static Card *Clone(const Card *other);
    static QString SuitToString(QSanguosha::Suit suit);
    static QString NumberToString(QSanguosha::Number number);
    static Card *Parse(const QString &str, RoomObject *room);

private:
    explicit Card(CardPrivate *p);
    Q_DISABLE_COPY_MOVE(Card) // no copy is allowed.
    CardPrivate *const d;
};

Q_DECLARE_METATYPE(const Card *)

/**
 * @struct For creating new cards.
 */
struct CardDescriptor
{
    // const CardFace *face;
    // or following? or both?
    QString faceName;
    QSanguosha::Suit suit;
    QSanguosha::Number number;
    QString package;

    // share some interfaces of Card?
    QString fullName(bool include_suit = false) const;
    QString logName() const;
    bool isBlack() const;
    bool isRed() const;
    const CardFace *face() const;
};

#endif
