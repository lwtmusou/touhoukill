#include "card.h"
#include "CardFace.h"
#include "RoomObject.h"
#include "engine.h"
#include "util.h"

#include <QFile>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

#include <optional>

const int Card::S_UNKNOWN_CARD_ID = -1;

using namespace QSanguosha;

class CardPrivate
{
public:
    // basic information
    const CardFace *face; // functional model
    Suit suit;
    Number number;
    int id; // real card has id.

    IDSet sub_cards; // for real card this should be empty.

    // Skill name related
    QString skill_name;
    QString show_skill_name;

    // Property of card itself
    bool can_recast;
    bool transferable;

    // property - use tristate
    std::optional<HandlingMethod> handling_method;
    std::optional<bool> can_damage;
    std::optional<bool> can_recover;
    std::optional<bool> has_effect_value;

    // flags
    QSet<QString> flags;

    QString user_string;

    // UI (Why server side?)
    // Fs: because the notification is from server! (Exactly, Room)
    bool mute;

    // The room
    RoomObject *room;

    CardPrivate(RoomObject *room, const CardFace *face, Suit suit, Number number, int id)
        : face(face)
        , suit(suit)
        , number(number)
        , id(id)
        , can_recast(false)
        , transferable(false)
        , mute(false)
        , room(room)
    {
    }
};

/**
 * @class Card
 * @brief The class which indicates a card no matter if it is virtual or not.
 *
 * This is the class which represents the game card in the Sanguosha game.
 * It only represents game card. It doesn't represent general card, role card nor HP card, etc.
 *
 * A game card is either a basic card, an equip card or a trick card.
 * In QSanguosha, we introcude a new type of card which is called skill card, to indicate a skill invokation.
 * What the card actually do is described in class @c CardFace .
 */

/**
 * @brief Constructor.
 * @param room the room the card belongs to.
 * @param face the card face it represents.
 * @param suit the suit of the card.
 * @param number the number of the card.
 * @param id (for real card) the id of the card.
 *
 * @note This constructor is private. One can only use @c RoomObject::cloneCard or @c RoomObject::getCard to get an instance of a card.
 */

Card::Card(RoomObject *room, const CardFace *face, Suit suit, Number number, int id)
    : d(new CardPrivate(room, face, suit, number, id))
{
}

/**
 * @brief Destructor.
 */

Card::~Card()
{
    delete d;
}

/**
 * @brief get the suit of the card.
 * @return the suit of the card.
 */

Suit Card::suit() const
{
    if (d->suit != SuitToBeDecided)
        return d->suit;

    if (isVirtualCard()) {
        // I don't want to check room != nullptr here because virtual cards must be created by RoomObject
        Q_ASSERT(room() != nullptr);
        if (d->sub_cards.empty())
            return NoSuit;

        if (d->sub_cards.size() == 1)
            return room()->getCard(*d->sub_cards.constBegin())->suit();
        Color color = Colorless;
        foreach (int id, d->sub_cards) {
            Color color2 = room()->getCard(id)->color();
            if (color == Colorless)
                color = color2;
            else if (color != color2)
                return NoSuit;
        }
        return (color == ColorRed) ? NoSuitRed : NoSuitBlack;
    }

    if (d->suit == SuitToBeDecided)
        return NoSuit; // NoSuit non-virtual card actually exists, but only with FilterSkill

    return d->suit;
}

/**
 * @brief set the suit of the card.
 * @param suit the suit of the card.
 */

void Card::setSuit(Suit suit)
{
    d->suit = suit;
}

/**
 * @brief get the suit of the card, and convert it to String.
 * @return the suit of the card, converted to String.
 */

QString Card::suitString() const
{
    return SuitToString(d->suit);
}

/**
 * @brief judges if the card is red.
 * @return if the card is red.
 */

bool Card::isRed() const
{
    return suit() == Heart || suit() == Diamond || suit() == NoSuitRed;
}

/**
 * @brief judges if the card is black.
 * @return if the card is black.
 */

bool Card::isBlack() const
{
    return suit() == Spade || suit() == Club || suit() == NoSuitBlack;
}

/**
 * @brief get the color of the card.
 * @return the color of the card.
 */

Color Card::color() const
{
    if (isRed())
        return ColorRed;
    if (isBlack())
        return ColorBlack;
    return Colorless;
}

Number Card::number() const
{
    if (d->number != NumberToBeDecided)
        return d->number;

    if (isVirtualCard()) {
        // I don't want to check room != nullptr here because virtual cards must be created by RoomObject
        Q_ASSERT(room() != nullptr);
        if (d->sub_cards.empty())
            return NumberNA;
        if (d->sub_cards.size() == 1)
            return room()->getCard(*d->sub_cards.constBegin())->number();

        return NumberNA;
    }

    Q_ASSERT(d->number != NumberToBeDecided); // I wonder NoNumber non-virtual card actually exists even with FilterSkill

    return d->number;
}

void Card::setNumber(Number number)
{
    // FIXME: Check the boundary for number.
    // The parameter is given a Number type, but how?
    // overload it?
    d->number = number;
}

QString Card::numberString() const
{
    if (isVirtualCard()) {
        if (d->sub_cards.empty() || d->sub_cards.size() >= 2)
            return NumberToString(NumberNA);
    }

    return NumberToString(number());
}

int Card::id() const
{
    return d->id;
}

void Card::setID(int id)
{
    d->id = id;
}

int Card::effectiveID() const
{
    if (isVirtualCard()) {
        if (d->sub_cards.isEmpty())
            return -1;

        return *d->sub_cards.constBegin();
    }

    return d->id;
}

QString Card::faceName() const
{
    if (d->face != nullptr)
        return d->face->name();

    return QStringLiteral("DummyCard");
}

QString Card::fullName(bool include_suit) const
{
    QString name = this->faceName();
    if (include_suit) {
        QString suit_name = Sanguosha->translate(suitString());
        return QStringLiteral("%1%2 %3").arg(suit_name, numberString(), name);
    }

    return QStringLiteral("%1 %2").arg(numberString(), name);
}

QString Card::logName() const
{
    QString suit_char;
    QString number_string;

    switch (suit()) {
    case Spade:
    case Heart:
    case Club:
    case Diamond: {
        suit_char = QStringLiteral("<img src='image/system/log/%1.png' height = 12/>").arg(suitString());
        break;
    }
    case NoSuitRed: {
        // FIXME: use tr here will raise error since card does not inherits QObject.
        // Fs: so use QObject::tr
        suit_char = QObject::tr("NoSuitRed");
        break;
    }
    case NoSuitBlack: {
        suit_char = QObject::tr("NoSuitBlack");
        break;
    }
    case NoSuit: {
        suit_char = QObject::tr("NoSuit");
        break;
    }
    default:
        break;
    }

    // FIXME: Should we compare the Number with int directly?
    if (number() > NumberA && number() <= NumberK)
        number_string = numberString();

    return QStringLiteral("%1[%2%3]").arg(faceName(), suit_char, number_string);
}

bool Card::isModified() const
{
    Q_ASSERT(!isVirtualCard());

    if (isVirtualCard())
        return false;

    const CardDescriptor &c = Sanguosha->getEngineCard(id());
    return (c.face() != face()) || (c.suit != suit()) || (c.number != number()) || (!skillName(false).isEmpty());
}

QString Card::skillName(bool removePrefix) const
{
    QString r = d->skill_name;
    if (removePrefix && r.startsWith(QLatin1Char('_')))
        r = r.mid(1);

    return r;
}

void Card::setSkillName(const QString &skill_name)
{
    d->skill_name = skill_name;
}

const QString &Card::showSkillName() const
{
    return d->show_skill_name;
}

void Card::setShowSkillName(const QString &show_skill_name)
{
    d->show_skill_name = show_skill_name;
}

HandlingMethod Card::handleMethod() const
{
    return d->handling_method.value_or((d->face != nullptr) ? d->face->defaultHandlingMethod() : MethodNone);
}

void Card::setHandleMethod(HandlingMethod method)
{
    d->handling_method = method;
}

bool Card::canDamage() const
{
    return d->can_damage.value_or((d->face != nullptr) ? d->face->canDamage() : false);
}

void Card::setCanDamage(bool can_damage)
{
    d->can_damage = can_damage;
}

bool Card::canRecover() const
{
    return d->can_recover.value_or((d->face != nullptr) ? d->face->canRecover() : false);
}

void Card::setCanRecover(bool can_recover)
{
    d->can_recover = can_recover;
}

bool Card::canRecast() const
{
    return d->can_recast;
}

void Card::setCanRecast(bool can_recast)
{
    d->can_recast = can_recast;
}

bool Card::transferable() const
{
    return d->transferable;
}

void Card::setTransferable(bool can)
{
    d->transferable = can;
}

bool Card::hasEffectValue() const
{
    return d->has_effect_value.value_or((d->face != nullptr) ? d->face->hasEffectValue() : false);
}

void Card::setHasEffectValue(bool has_effect_value)
{
    d->has_effect_value = has_effect_value;
}

const CardFace *Card::face() const
{
    return d->face;
}

void Card::setFace(const CardFace *face)
{
    d->face = face;
}

const QSet<QString> &Card::flags() const
{
    return d->flags;
}

void Card::addFlag(const QString &flag) const /* mutable */
{
    d->flags.insert(flag);
}

void Card::addFlags(const QSet<QString> &flags) const /* mutable */
{
    foreach (const QString &flag, flags)
        d->flags.insert(flag);
}

void Card::removeFlag(const QString &flag) const /* mutable */
{
    d->flags.remove(flag);
}

void Card::removeFlag(const QSet<QString> &flags) const /* mutable */
{
    foreach (const QString &flag, flags)
        d->flags.remove(flag);
}

void Card::clearFlags() const /* mutable */
{
    d->flags.clear();
}

bool Card::hasFlag(const QString &flag) const
{
    return d->flags.contains(flag);
}

bool Card::isVirtualCard() const
{
    return id() < 0;
}

const IDSet &Card::subcards() const
{
    return d->sub_cards;
}

void Card::addSubcard(int card_id)
{
    Q_ASSERT(card_id >= 0);
    d->sub_cards.insert(card_id);
}

void Card::addSubcard(const Card *card)
{
    d->sub_cards.insert(card->effectiveID());
}

void Card::addSubcards(const IDSet &subcards)
{
    foreach (int id, subcards)
        d->sub_cards.insert(id);
}

void Card::clearSubcards()
{
    d->sub_cards.clear();
}

QString Card::subcardString() const
{
    if (subcards().isEmpty())
        return QStringLiteral(".");

    QStringList str;
    foreach (int subcard, subcards())
        str << QString::number(subcard);

    return str.join(QStringLiteral("+"));
}

bool Card::mute() const
{
    return d->mute;
}

void Card::setMute(bool mute)
{
    d->mute = mute;
}

const QString &Card::userString() const
{
    return d->user_string;
}

void Card::setUserString(const QString &str)
{
    d->user_string = str;
}

RoomObject *Card::room()
{
    return d->room;
}

const RoomObject *Card::room() const
{
    return d->room;
}

void Card::setRoomObject(RoomObject *room)
{
    d->room = room;
}

QString Card::toString(bool hidden) const
{
    if (d->face == nullptr)
        return QStringLiteral("$") + subcardString();

    if (d->face->isKindOf(QStringLiteral("SkillCard"))) {
        QString str;
        if (!hidden)
            str = QStringLiteral("@%1[%2:%3]=%4").arg(d->face->name(), suitString(), numberString(), subcardString());
        else
            str = QStringLiteral("@%1[no_suit:-]=.").arg(d->face->name());

        if (!d->user_string.isEmpty())
            str = str + (QStringLiteral(":%1").arg(d->user_string));

        return str;
    }

    if (isVirtualCard())
        return QStringLiteral("%1:%2[%3:%4]=%5").arg(d->face->name(), skillName(), suitString(), numberString(), subcardString());

    return QString::number(d->id);
}

Card::Card(CardPrivate *d)
    : d(new CardPrivate(*d))
{
}

QString Card::SuitToString(Suit suit)
{
    switch (suit) {
    case Spade:
        return QStringLiteral("spade");
    case Heart:
        return QStringLiteral("heart");
    case Club:
        return QStringLiteral("club");
    case Diamond:
        return QStringLiteral("diamond");
    case NoSuitBlack:
        return QStringLiteral("no_suit_black");
    case NoSuitRed:
        return QStringLiteral("no_suit_red");
    default:
        return QStringLiteral("no_suit");
    }
}

QString Card::NumberToString(Number number)
{
    static const QHash<Number, QString> r {
        {NumberToBeDecided, QStringLiteral("TBD")},
        {NumberNA, QStringLiteral("-")},
        {NumberA, QStringLiteral("A")},
        {Number2, QStringLiteral("2")},
        {Number3, QStringLiteral("3")},
        {Number4, QStringLiteral("4")},
        {Number5, QStringLiteral("5")},
        {Number6, QStringLiteral("6")},
        {Number7, QStringLiteral("7")},
        {Number8, QStringLiteral("8")},
        {Number9, QStringLiteral("9")},
        {Number10, QStringLiteral("10")},
        {NumberJ, QStringLiteral("J")},
        {NumberQ, QStringLiteral("Q")},
        {NumberK, QStringLiteral("K")},
    };

    return r.value(number);
}

Card *Card::Parse(const QString &str, RoomObject *room)
{
    // This should match Card::toString since this is the reverse function of Card::toString

    static QMap<QString, Suit> suit_map;
    if (suit_map.isEmpty()) {
        suit_map.insert(QStringLiteral("spade"), Spade);
        suit_map.insert(QStringLiteral("club"), Club);
        suit_map.insert(QStringLiteral("heart"), Heart);
        suit_map.insert(QStringLiteral("diamond"), Diamond);
        suit_map.insert(QStringLiteral("no_suit_red"), NoSuitRed);
        suit_map.insert(QStringLiteral("no_suit_black"), NoSuitBlack);
        suit_map.insert(QStringLiteral("no_suit"), NoSuit);
        suit_map.insert(QStringLiteral("to_be_decided"), SuitToBeDecided);
    }

    // for skill cards
    if (str.startsWith(QLatin1Char('@'))) {
        QRegularExpression pattern(QStringLiteral("^@(\\w+)=([^:]+)(:.+)?$"));
        QRegularExpression ex_pattern(QStringLiteral("^@(\\w*)\\[(\\w+):(.+)\\]=([^:]+)(:.+)?$"));

        QStringList texts;
        QString card_name;
        QString card_suit;
        QString card_number;
        QStringList subcard_ids;
        QString subcard_str;
        QString user_string;
        Suit suit = NoSuit;
        Number number = NumberNA;

        QRegularExpressionMatch match;
        if ((match = pattern.match(str)).hasMatch()) {
            texts = match.capturedTexts();
            card_name = texts.at(1);
            subcard_str = texts.at(2);
            user_string = texts.at(3);
        } else if ((match = ex_pattern.match(str)).hasMatch()) {
            texts = match.capturedTexts();
            card_name = texts.at(1);
            card_suit = texts.at(2);
            card_number = texts.at(3);
            subcard_str = texts.at(4);
            user_string = texts.at(5);
        } else
            return nullptr;

        if (subcard_str != QStringLiteral("."))
            subcard_ids = subcard_str.split(QStringLiteral("+"));

        const CardFace *skillCardFace = Sanguosha->cardFace(card_name);
        if (skillCardFace == nullptr)
            return nullptr;

        if (!card_suit.isEmpty())
            suit = suit_map.value(card_suit, NoSuit);

        if (!card_number.isEmpty()) {
            if (card_number == QStringLiteral("A"))
                number = NumberA;
            else if (card_number == QStringLiteral("J"))
                number = NumberJ;
            else if (card_number == QStringLiteral("Q"))
                number = NumberQ;
            else if (card_number == QStringLiteral("K"))
                number = NumberK;
            else
                number = static_cast<Number>(card_number.toInt());
        }

        Card *card = room->cloneCard(skillCardFace, suit, number);
        card->addSubcards(List2Set(StringList2IntList(subcard_ids)));
        if (!user_string.isEmpty())
            card->setUserString(user_string.mid(1));

        return card;
    }

    // for dummy cards
    if (str.startsWith(QLatin1Char('$'))) {
        QString copy = str;
        copy.remove(QLatin1Char('$'));
        QStringList card_strs = copy.split(QStringLiteral("+"));

        return room->cloneDummyCard(List2Set(StringList2IntList(card_strs)));
    }

    // for regular virtual cards
    if (str.contains(QLatin1Char('='))) {
        QRegularExpression pattern(QStringLiteral("^(\\w+):(\\w*)\\[(\\w+):(.+)\\]=(.+)$"));
        QRegularExpressionMatch match = pattern.match(str);
        if (!match.hasMatch())
            return nullptr;

        QStringList texts = match.capturedTexts();
        QString card_name = texts.at(1);
        QString m_skillName = texts.at(2);
        QString suit_string = texts.at(3);
        QString number_string = texts.at(4);
        QString subcard_str = texts.at(5);
        QStringList subcard_ids;
        if (subcard_str != QStringLiteral("."))
            subcard_ids = subcard_str.split(QStringLiteral("+"));

        Suit suit = suit_map.value(suit_string, SuitToBeDecided);

        Number number = NumberNA;
        if (number_string == QStringLiteral("A"))
            number = NumberA;
        else if (number_string == QStringLiteral("J"))
            number = NumberJ;
        else if (number_string == QStringLiteral("Q"))
            number = NumberQ;
        else if (number_string == QStringLiteral("K"))
            number = NumberK;
        else
            number = static_cast<Number>(number_string.toInt());

        Card *card = room->cloneCard(card_name, suit, number);
        if (card == nullptr)
            return nullptr;

        card->addSubcards(List2Set(StringList2IntList(subcard_ids)));
        card->setSkillName(m_skillName);

        return card;
    }

    // for actual cards: str is Card ID
    {
        bool ok = false;
        int id = str.toInt(&ok);
        if (ok && id >= 0)
            return room->getCard(id);
    }
    return nullptr;
}

QString CardDescriptor::fullName(bool include_suit) const
{
    QString name = face()->name();
    if (include_suit) {
        QString suit_name = Sanguosha->translate(Card::SuitToString(suit));
        return QStringLiteral("%1%2 %3").arg(suit_name, Card::NumberToString(number), name);
    }

    return QStringLiteral("%1 %2").arg(Card::NumberToString(number), name);
}

QString CardDescriptor::logName() const
{
    QString suit_char;
    QString number_string;

    switch (suit) {
    case Spade:
    case Heart:
    case Club:
    case Diamond: {
        suit_char = QStringLiteral("<img src='image/system/log/%1.png' height = 12/>").arg(Card::SuitToString(suit));
        break;
    }
    case NoSuitRed: {
        suit_char = QObject::tr("NoSuitRed");
        break;
    }
    case NoSuitBlack: {
        suit_char = QObject::tr("NoSuitBlack");
        break;
    }
    case NoSuit: {
        suit_char = QObject::tr("NoSuit");
        break;
    }
    default:
        break;
    }

    // FIXME: Should we compare the Number with int directly?
    if (number > NumberA && number <= NumberK)
        number_string = Card::NumberToString(number);

    return QStringLiteral("%1[%2%3]").arg(face()->name(), suit_char, number_string);
}

bool CardDescriptor::isBlack() const
{
    return suit == Spade || suit == Club || suit == NoSuitBlack;
}

bool CardDescriptor::isRed() const
{
    return suit == Heart || suit == Diamond || suit == NoSuitRed;
}

const CardFace *CardDescriptor::face() const
{
    return Sanguosha->cardFace(faceName);
}
