#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "skill.h"

#include <QTextDocument>
#include <QTextOption>

ClientPlayer::ClientPlayer(Client *client)
    : Player(client)
    , handcard_num(0)
{
    mark_doc = new QTextDocument(this);
}

int ClientPlayer::getHandcardNum() const
{
    return handcard_num;
}

void ClientPlayer::addCard(const Card *card, QSanguosha::Place place)
{
    switch (place) {
    case QSanguosha::PlaceHand: {
        if (card != nullptr)
            known_cards << card;
        handcard_num++;
        break;
    }
    case QSanguosha::PlaceEquip: {
        // WrappedCard *equip = getRoomObject()->getWrappedCard(card->getEffectiveId());
        setEquip(roomObject()->getCard(card->effectiveID()));
        break;
    }
    case QSanguosha::PlaceDelayedTrick: {
        addDelayedTrick(card);
        break;
    }
    default:
        break;
    }
}

void ClientPlayer::addKnownHandCard(const Card *card)
{
    if (!known_cards.contains(card))
        known_cards << card;
}

bool ClientPlayer::isLastHandCard(const Card *card, bool contain) const
{
    if (!card->isVirtualCard()) {
        if (known_cards.length() != 1)
            return false;
        return known_cards.first()->id() == card->effectiveID();
    } else if (!card->subcards().empty()) {
        if (!contain) {
            foreach (int card_id, card->subcards()) {
                if (!known_cards.contains(roomObject()->getCard(card_id)))
                    return false;
            }
            return known_cards.length() == card->subcards().size();
        } else {
            foreach (const Card *ncard, known_cards) {
                if (!card->subcards().contains(ncard->effectiveID()))
                    return false;
            }
            return true;
        }
    }
    return false;
}

void ClientPlayer::removeCard(const Card *card, QSanguosha::Place place)
{
    switch (place) {
    case QSanguosha::PlaceHand: {
        handcard_num--;
        if (card != nullptr)
            known_cards.removeOne(card);
        break;
    }
    case QSanguosha::PlaceEquip: {
        // WrappedCard *equip = getRoomObject()->getWrappedCard(card->getEffectiveId());
        removeEquip(roomObject()->getCard(card->effectiveID()));
        break;
    }
    case QSanguosha::PlaceDelayedTrick: {
        removeDelayedTrick(card);
        break;
    }
    default:
        break;
    }
}

QList<const Card *> ClientPlayer::getHandcards() const
{
    return known_cards;
}

void ClientPlayer::setCards(const QList<int> &card_ids)
{
    known_cards.clear();
    foreach (int cardId, card_ids)
        known_cards.append(roomObject()->getCard(cardId));
}

QTextDocument *ClientPlayer::getMarkDoc() const
{
    return mark_doc;
}

QString ClientPlayer::getDeathPixmapPath() const
{
    QString basename;
    if (ServerInfo.GameMode == QStringLiteral("06_3v3") || ServerInfo.GameMode == QStringLiteral("06_XMode")) {
        if (getRoleString() == QStringLiteral("lord") || getRoleString() == QStringLiteral("renegade"))
            basename = QStringLiteral("marshal");
        else
            basename = QStringLiteral("guard");
    } else
        basename = getRoleString();

    if (basename.isEmpty())
        basename = QStringLiteral("unknown");

    return QStringLiteral("image/system/death/%1.png").arg(basename);
}

void ClientPlayer::setHandcardNum(int n)
{
    handcard_num = n;
}

Client *ClientPlayer::getClient() const
{
    return qobject_cast<Client *>(parent());
}
