#ifndef _CARD_H
#define _CARD_H

#include <QIcon>
#include <QMap>
#include <QObject>
#include <QSet>
#include <qobjectdefs.h>

class Room;
class RoomObject;
class Player;
class ServerPlayer;
class Client;
class ClientPlayer;
class CardItem;

struct CardEffectStruct;
struct CardUseStruct;

class CardPrivate;
class CardFace;

// To keep compatibility, the new card class is defined here.
// After its interface is carefully examined, we will push it to replace existing class.

class Card final
{
    Q_GADGET
public:
    enum Suit
    {
        Spade,
        Club,
        Heart,
        Diamond,
        NoSuitBlack,
        NoSuitRed,
        NoSuit,
        SuitToBeDecided = -1
    };

    Q_ENUM(Suit)

    enum Color
    {
        Red,
        Black,
        Colorless
    };

    Q_ENUM(Color)

    enum HandlingMethod
    {
        MethodNone,
        MethodUse,
        MethodResponse,
        MethodDiscard,
        MethodRecast,
        MethodPindian
    };

    Q_ENUM(HandlingMethod)

    enum Number
    {
        // Regular numbers
        NumberA = 1,
        Number2,
        Number3,
        Number4,
        Number5,
        Number6,
        Number7,
        Number8,
        Number9,
        Number10,
        NumberJ,
        NumberQ,
        NumberK,

        // Alternative notation
        Number1 = NumberA,
        NumberX = Number10,
        Number11 = NumberJ,
        Number12 = NumberQ,
        Number13 = NumberK,

        // Extremely simplified notation
        A = NumberA,
        X = Number10,
        J = NumberJ,
        Q = NumberQ,
        K = NumberK,

        // Special numbers
        NumberNA = 0,
        NumberToBeDecided = -1

        // TODO: Add -2
    };
    Q_ENUM(Number)

    // constructor to create real card
    explicit Card(RoomObject *room, const CardFace *face, Suit suit = SuitToBeDecided, Number number = Number::NumberToBeDecided, int id = -1);
    ~Card();

    // Suit method
    Card::Suit suit() const;
    void setSuit(Suit suit);
    QString suitString() const;
    bool isRed() const;
    bool isBlack() const;
    Color color() const;

    // Number method
    Card::Number number() const;
    void setNumber(Card::Number number);
    QString numberString() const;

    // id
    int id() const;
    int effectiveID() const;

    // name
    QString name() const;
    QString fullName(bool include_suit = false) const;
    QString logName() const;

    // skill name
    // TODO_Fs: add mechanics to underscore-prefixed effect identifier
    QString skillName(bool removePrefix = true) const;
    void setSkillName(const QString &skill_name);
    const QString &showSkillName() const;
    void setShowSkillName(const QString &show_skill_name);

    // handling method
    Card::HandlingMethod handleMethod() const;
    void setHandleMethod(Card::HandlingMethod method);

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
    const QSet<int> &subcards() const;
    void addSubcard(int card_id);
    void addSubcard(const Card *card);
    void addSubcards(const QSet<int> &subcards);
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

    // toString
    // What's the meaning of this hidden card?
    // Fs: SkillCard with subcard which does not always need to show to others
    QString toString(bool hidden = false) const;

    // helpers
    // static Card *Clone(const Card *other);
    static QString SuitToString(Suit suit);
    static Card *Parse(const QString &str, RoomObject *room);

private:
    explicit Card(CardPrivate *p);
    Q_DISABLE_COPY_MOVE(Card) // no copy is allowed.
    CardPrivate *d;
};

#endif
