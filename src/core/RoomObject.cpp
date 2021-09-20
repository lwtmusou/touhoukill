#include "RoomObject.h"
#include "CardFace.h"
#include "card.h"
#include "engine.h"
#include "player.h"
#include "util.h"

using namespace QSanguosha;

namespace CardFactory {
QHash<QString, const CardFace *> faces;

void registerCardFace(const CardFace *cardFace)
{
    faces.insert(cardFace->name(), cardFace);
}

const CardFace *cardFace(const QString &name)
{
    return faces.value(name, nullptr);
}

void unregisterCardFace(const QString &name)
{
    auto face = faces.find(name);
    if (face != faces.end()) {
        const auto *handle = *face;
        faces.erase(face);
        delete handle;
    }
}
} // namespace CardFactory

class RoomObjectPrivate
{
public:
    QHash<int, Card *> cards;
    QString currentCardUsePattern;
    CardUseStruct::CardUseReason currentCardUseReason;
    QList<Card *> clonedCards;

    QList<int> discardPile;

    QList<Player *> players;
    Player *current;
    QStringList seatInfo;

    RoomObjectPrivate()
        : current(nullptr)
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
    foreach (Card *c, d->cards)
        delete c;

    foreach (Card *c, d->clonedCards)
        delete c;

    delete d;
}

QList<Player *> RoomObject::players(bool include_dead)
{
    QList<Player *> ps = d->players;
    if (!include_dead) {
        for (auto it = ps.begin(); it != ps.end();) {
            if ((*it)->isDead()) {
                auto x = it + 1;
                ps.erase(it);
                it = x;
            } else
                ++it;
        }
    }

    return ps;
}

QList<const Player *> RoomObject::players(bool include_dead) const
{
    QList<Player *> ps = d->players;
    if (!include_dead) {
        for (auto it = ps.begin(); it != ps.end();) {
            if ((*it)->isDead()) {
                auto x = it + 1;
                ps.erase(it);
                it = x;
            } else
                ++it;
        }
    }

    return NonConstList2ConstList(ps);
}

void RoomObject::registerPlayer(Player *player)
{
    d->players << player;
}

void RoomObject::unregisterPlayer(Player *player)
{
    d->players.removeAll(player);
}

void RoomObject::unregisterPlayer(const QString &objectName)
{
    for (auto it = d->players.begin(), lastIt = d->players.end(); it != d->players.end(); it == d->players.end() ? (it = d->players.begin()) : ++it) {
        if ((*it)->objectName() == objectName) {
            d->players.erase(it);
            it = lastIt;
        }

        lastIt = it;
    }
}

Player *RoomObject::findPlayer(const QString &objectName)
{
    foreach (Player *p, d->players)
        if (p->objectName() == objectName)
            return p;

    return nullptr;
}

const Player *RoomObject::findPlayer(const QString &objectName) const
{
    foreach (Player *p, d->players)
        if (p->objectName() == objectName)
            return p;

    return nullptr;
}

Player *RoomObject::current()
{
    return d->current;
}

const Player *RoomObject::current() const
{
    return d->current;
}

void RoomObject::setCurrent(Player *player)
{
    if (!(d->players.contains(player) || (player == nullptr)))
        return;

    d->current = player;

    if (player != nullptr) {
        QList<Player *> ps = d->players;
        while (ps.first() != player)
            ps << ps.takeFirst();

        d->players = ps;
    }
}

void RoomObject::arrangeSeat(const QStringList &_seatInfo)
{
    QList<Player *> ps = d->players;
    d->players.clear();

    QStringList seatInfo = _seatInfo;
    while (!seatInfo.isEmpty()) {
        QString first = seatInfo.takeFirst();
        for (auto it = ps.begin(); it != ps.end(); ++it) {
            if ((*it)->objectName() == first) {
                d->players << (*it);
                ps.erase(it);
                break;
            }
        }
    }

    for (auto it = d->players.begin(); it != d->players.end(); ++it) {
        auto nextIt = it + 1;
        if (nextIt == d->players.end())
            nextIt = d->players.begin();
        (*it)->setNext(*nextIt);
    }

    if (d->current != nullptr) {
        ps = d->players;
        while (ps.first() != d->current)
            ps << ps.takeFirst();

        d->players = ps;
    }
}

// I don't think that there will be multiple players who share same seat, so use quicker std::sort instead of stable_sort
void RoomObject::sortPlayersByActionOrder(QList<Player *> &players) const
{
    std::sort(players.begin(), players.end(), [this](Player *a, Player *b) -> bool {
        return comparePlayerByActionOrder(a, b);
    });
}

void RoomObject::sortPlayersByActionOrder(QList<const Player *> &players) const
{
    std::sort(players.begin(), players.end(), [this](const Player *a, const Player *b) -> bool {
        return comparePlayerByActionOrder(a, b);
    });
}

bool RoomObject::comparePlayerByActionOrder(const Player *a, const Player *b) const
{
    QList<const Player *> ps = NonConstList2ConstList(d->players);
    return ps.indexOf(a) < ps.indexOf(b);
}

Card *RoomObject::getCard(int cardId)
{
    if (!d->cards.contains(cardId))
        return nullptr;
    return d->cards[cardId];
}

const Card *RoomObject::getCard(int cardId) const
{
    if (!d->cards.contains(cardId))
        return nullptr;
    return d->cards[cardId];
}

QString RoomObject::currentCardUsePattern() const
{
    return d->currentCardUsePattern;
}

void RoomObject::setCurrentCardUsePattern(const QString &newPattern)
{
    d->currentCardUsePattern = newPattern;
}

CardUseStruct::CardUseReason RoomObject::currentCardUseReason() const
{
    return d->currentCardUseReason;
}

void RoomObject::setCurrentCardUseReason(CardUseStruct::CardUseReason reason)
{
    d->currentCardUseReason = reason;
}

void RoomObject::resetCard(int cardId)
{
    // TODO_Fs: error check and sanity check
    delete d->cards[cardId];
    d->cards[cardId] = cloneCard(Sanguosha->getEngineCard(cardId));

    // It does not depend on how we achieve the filter skill.
}

// Reset all cards, generals' states of the room instance
void RoomObject::resetAllCards()
{
    foreach (Card *card, d->cards.values())
        delete card;
    d->cards.clear();

    int n = Sanguosha->getCardCount();
    for (int i = 0; i < n; i++)
        d->cards[i] = cloneCard(Sanguosha->getEngineCard(i));
}

QList<int> &RoomObject::discardPile()
{
    return d->discardPile;
}

const QList<int> &RoomObject::discardPile() const
{
    return d->discardPile;
}

Card *RoomObject::cloneCard(const Card *card)
{
    return cloneCard(card->face(), card->suit(), card->number());
}

Card *RoomObject::cloneCard(const QString &name, Suit suit, Number number)
{
    const CardFace *face = nullptr;

    if (!name.isEmpty()) {
        face = CardFactory::cardFace(name);
        if (face == nullptr && name != QStringLiteral("DummyCard"))
            return nullptr;
    }
    return cloneCard(face, suit, number);
}

Card *RoomObject::cloneCard(const CardFace *cardFace, Suit suit, Number number)
{
    // Design change: dummy cards does not have CardFace
#if 0
    if (cardFace == nullptr) {
        qDebug() << "RoomObject::cloneCard - cardFace is nullptr";
        return nullptr;
    }
#endif

    Card *c = new Card(this, cardFace, suit, number);
    d->clonedCards << c;
    return c;
}

Card *RoomObject::cloneCard(const CardDescriptor &descriptor)
{
    return cloneCard(descriptor.face(), descriptor.suit, descriptor.number);
}

void RoomObject::cardDeleting(const Card *card)
{
    // const_cast is necessary for this function since card may be const
    // but for non-const Room, this Card instance is definitely mutable
    if (card != nullptr)
        d->clonedCards.removeAll(const_cast<Card *>(card));
    delete card;
}

Card *RoomObject::cloneSkillCard(const QString &name)
{
    return cloneCard(name);
}

Card *RoomObject::cloneDummyCard()
{
    return cloneCard();
}

Card *RoomObject::cloneDummyCard(const IDSet &idSet)
{
    Card *c = cloneCard();
    if (c == nullptr)
        return nullptr;

    c->addSubcards(idSet);

    return c;
}
