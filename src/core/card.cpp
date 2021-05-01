#include "card.h"
#include "client.h"
#include "engine.h"
#include "room.h"
#include "settings.h"
#include "standard.h"
#include "structs.h"

#include "CardFace.h"

#include <QFile>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>

class CardPrivate
{
public:
    // basic information
    const CardFace *face; // functional model
    Card::Suit suit;
    Card::Number number;
    int id; // real card has id.

    QSet<int> sub_cards; // for real card this should be empty.

    // Skill name related
    QString skill_name;
    QString show_skill_name;

    // handling method
    Card::HandlingMethod handling_method;

    // property - Fs: use tristate?
    bool can_damage;
    bool can_recover;
    bool can_recast;
    bool has_effect_value;
    bool throw_when_using;

    // flags
    QSet<QString> flags;

    QString user_string;

    // UI (Why server side?)
    // Fs: because the notification is from server! (Exactly, Room)
    bool mute;

    // The room
    RoomObject *room;

    CardPrivate(RoomObject *room, const CardFace *face, Card::Suit suit, Card::Number number, int id)
        : face(face)
        , suit(suit)
        , number(number)
        , id(id)
        , handling_method(Card::MethodNone)
        , can_damage(false)
        , can_recover(false)
        , can_recast(false)
        , has_effect_value(false)
        , throw_when_using(false)
        , mute(false)
        , room(room)
    {
        if (face != nullptr) {
            can_damage = face->canDamage();
            can_recover = face->canRecover();
            has_effect_value = face->hasEffectValue();
            throw_when_using = face->throwWhenUsing();
            handling_method = face->defaultHandlingMethod();
        }
    }
};

Card::Card(RoomObject *room, const CardFace *face, Suit suit, Number number, int id)
    : d(new CardPrivate(room, face, suit, number, id))
{
}

Card::~Card()
{
    // Since room keeps the pointer of the Card object, we should notify Room about deletion
    if (d->room != nullptr)
        d->room->cardDeleting(this);

    delete d;
}

Card::Suit Card::suit() const
{
    if (d->suit != SuitToBeDecided)
        return d->suit;

    if (isVirtualCard()) {
        // I don't want to check room != nullptr here because virtual cards must be created by RoomObject
        Q_ASSERT(room() != nullptr);
        if (d->sub_cards.size() == 0)
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
        return (color == Red) ? NoSuitRed : NoSuitBlack;
    }

    if (d->suit == SuitToBeDecided)
        return NoSuit; // NoSuit non-virtual card actually exists, but only with FilterSkill

    return d->suit;
}

void Card::setSuit(Suit suit)
{
    d->suit = suit;
}

QString Card::suitString() const
{
    return SuitToString(d->suit);
}

bool Card::isRed() const
{
    return suit() == Heart || suit() == Diamond || suit() == NoSuitRed;
}

bool Card::isBlack() const
{
    return suit() == Spade || suit() == Club || suit() == NoSuitBlack;
}

Card::Color Card::color() const
{
    if (isRed())
        return Red;
    if (isBlack())
        return Black;
    return Colorless;
}

Card::Number Card::number() const
{
    if (d->number != NumberToBeDecided)
        return d->number;

    if (isVirtualCard()) {
        // I don't want to check room != nullptr here because virtual cards must be created by RoomObject
        Q_ASSERT(room() != nullptr);
        if (d->sub_cards.size() == 0)
            return NumberNA;
        if (d->sub_cards.size() == 1)
            return room()->getCard(*d->sub_cards.constBegin())->number();

        return NumberNA;
    }
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
    Card::Number number = this->number();
    if (isVirtualCard()) {
        if (d->sub_cards.size() == 0 || d->sub_cards.size() >= 2)
            number = Number::NumberNA;
    }

    if (number == X)
        return "10";

    static const char *number_string = "-A23456789-JQK";
    return QString(number_string[static_cast<int>(number)]);
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

    return "DummyCard";
}

QString Card::fullName(bool include_suit) const
{
    QString name = this->faceName();
    if (include_suit) {
        QString suit_name = Sanguosha->translate(suitString());
        return QString("%1%2 %3").arg(suit_name).arg(numberString()).arg(name);
    }

    return QString("%1 %2").arg(numberString()).arg(name);
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
        suit_char = QString("<img src='image/system/log/%1.png' height = 12/>").arg(suitString());
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

    return QString("%1[%2%3]").arg(faceName()).arg(suit_char).arg(number_string);
}

QString Card::skillName(bool removePrefix) const
{
    QString r = d->skill_name;
    if (removePrefix && r.startsWith('_'))
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

Card::HandlingMethod Card::handleMethod() const
{
    return d->handling_method;
}

void Card::setHandleMethod(Card::HandlingMethod method)
{
    d->handling_method = method;
}

bool Card::canDamage() const
{
    return d->can_damage;
}

void Card::setCanDamage(bool can_damage)
{
    d->can_damage = can_damage;
}

bool Card::canRecover() const
{
    return d->can_recover;
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

bool Card::hasEffectValue() const
{
    return d->has_effect_value;
}

void Card::setHasEffectValue(bool has_effect_value)
{
    d->has_effect_value = has_effect_value;
}

bool Card::throwWhenUsing() const
{
    return d->throw_when_using;
}

void Card::setThrowWhenUsing(bool throw_when_using)
{
    d->throw_when_using = throw_when_using;
}

const CardFace *Card::face() const
{
    return d->face;
}

void Card::setFace(const CardFace *face)
{
    d->face = face;
    if (face != nullptr) {
        // Some properties should get updated as well.
        d->can_damage = face->canDamage();
        d->can_recover = face->canRecover();
        d->has_effect_value = face->hasEffectValue();
        d->throw_when_using = face->throwWhenUsing();
        d->handling_method = face->defaultHandlingMethod();
    }
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

const QSet<int> &Card::subcards() const
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

void Card::addSubcards(const QSet<int> &subcards)
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
        return ".";

    QStringList str;
    foreach (int subcard, subcards())
        str << QString::number(subcard);

    return str.join("+");
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
        return "$" + subcardString();

    if (d->face->isKindOf("SkillCard")) {
        QString str;
        if (!hidden)
            str = QString("@%1[%2:%3]=%4").arg(d->face->name()).arg(suitString()).arg(numberString()).arg(subcardString());
        else
            str = QString("@%1[no_suit:-]=.").arg(d->face->name());

        if (!d->user_string.isEmpty())
            str = str + (QString(":%1").arg(d->user_string));

        return str;
    }

    if (isVirtualCard())
        return QString("%1:%2[%3:%4]=%5").arg(d->face->name()).arg(skillName()).arg(suitString()).arg(numberString()).arg(subcardString());

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
        return "spade";
    case Heart:
        return "heart";
    case Club:
        return "club";
    case Diamond:
        return "diamond";
    case NoSuitBlack:
        return "no_suit_black";
    case NoSuitRed:
        return "no_suit_red";
    default:
        return "no_suit";
    }
}

Card *Card::Parse(const QString &str, RoomObject *room)
{
    // This should match Card::toString since this is the reverse function of Card::toString

    static QMap<QString, Suit> suit_map;
    if (suit_map.isEmpty()) {
        suit_map.insert("spade", Spade);
        suit_map.insert("club", Club);
        suit_map.insert("heart", Heart);
        suit_map.insert("diamond", Diamond);
        suit_map.insert("no_suit_red", NoSuitRed);
        suit_map.insert("no_suit_black", NoSuitBlack);
        suit_map.insert("no_suit", NoSuit);
        suit_map.insert("to_be_decided", SuitToBeDecided);
    }

    // for skill cards
    if (str.startsWith(QChar('@'))) {
        QRegularExpression pattern("^@(\\w+)=([^:]+)(:.+)?$");
        QRegularExpression ex_pattern("^@(\\w*)\\[(\\w+):(.+)\\]=([^:]+)(:.+)?$");

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

        if (subcard_str != ".")
            subcard_ids = subcard_str.split("+");

        const CardFace *skillCardFace = Sanguosha->getCardFace(card_name);
        if (skillCardFace == nullptr)
            return nullptr;

        if (!card_suit.isEmpty())
            suit = suit_map.value(card_suit, NoSuit);

        if (!card_number.isEmpty()) {
            if (card_number == "A")
                number = A;
            else if (card_number == "J")
                number = J;
            else if (card_number == "Q")
                number = Q;
            else if (card_number == "K")
                number = K;
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
    if (str.startsWith(QChar('$'))) {
        QString copy = str;
        copy.remove(QChar('$'));
        QStringList card_strs = copy.split("+");

        Card *card = room->cloneCard(nullptr, SuitToBeDecided, NumberToBeDecided);
        card->addSubcards(List2Set(StringList2IntList(card_strs)));

        return card;
    }

    // for regular virtual cards
    if (str.contains(QChar('='))) {
        QRegularExpression pattern("^(\\w+):(\\w*)\\[(\\w+):(.+)\\]=(.+)$");
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
        if (subcard_str != ".")
            subcard_ids = subcard_str.split("+");

        Suit suit = suit_map.value(suit_string, Card::SuitToBeDecided);

        Number number = NumberNA;
        if (number_string == "A")
            number = A;
        else if (number_string == "J")
            number = J;
        else if (number_string == "Q")
            number = Q;
        else if (number_string == "K")
            number = K;
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
