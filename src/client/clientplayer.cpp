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

int ClientPlayer::aliveCount(bool includeRemoved) const
{
    const Client *client = qobject_cast<Client *>(parent());
    if (client == nullptr)
        return -1;

    int n = client->alivePlayerCount();
    if (!includeRemoved) {
        if (isRemoved())
            n--;
        foreach (const Player *p, getAliveSiblings())
            if (p->isRemoved())
                n--;
    }
    return n;
}

int ClientPlayer::getHandcardNum() const
{
    return handcard_num;
}

void ClientPlayer::addCard(const Card *card, Place place)
{
    switch (place) {
    case PlaceHand: {
        if (card)
            known_cards << card;
        handcard_num++;
        break;
    }
    case PlaceEquip: {
        // WrappedCard *equip = getRoomObject()->getWrappedCard(card->getEffectiveId());
        setEquip(getRoomObject()->getCard(card->effectiveID()));
        break;
    }
    case PlaceDelayedTrick: {
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
    } else if (card->subcards().size() > 0) {
        if (!contain) {
            foreach (int card_id, card->subcards()) {
                if (!known_cards.contains(getRoomObject()->getCard(card_id)))
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

void ClientPlayer::removeCard(const Card *card, Place place)
{
    switch (place) {
    case PlaceHand: {
        handcard_num--;
        if (card)
            known_cards.removeOne(card);
        break;
    }
    case PlaceEquip: {
        // WrappedCard *equip = getRoomObject()->getWrappedCard(card->getEffectiveId());
        removeEquip(getRoomObject()->getCard(card->effectiveID()));
        break;
    }
    case PlaceDelayedTrick: {
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
        known_cards.append(getRoomObject()->getCard(cardId));
}

QTextDocument *ClientPlayer::getMarkDoc() const
{
    return mark_doc;
}

void ClientPlayer::changePile(const QString &name, bool add, QList<int> card_ids)
{
    if (name == QStringLiteral("shown_card") || name == QStringLiteral("huashencard"))
        emit pile_changed(name);
    else {
        if (add) {
            foreach (int id, card_ids)
                piles[name] << id;
        } else {
            foreach (int card_id, card_ids) {
                if (piles[name].isEmpty())
                    break;
                if (piles[name].contains(Card::S_UNKNOWN_CARD_ID) && !piles[name].contains(card_id))
                    piles[name].remove(Card::S_UNKNOWN_CARD_ID);
                else if (piles[name].contains(card_id))
                    piles[name].remove(card_id);
                else
                    piles[name].remove(*piles[name].cbegin());
            }
        }

        if (!name.startsWith(QStringLiteral("#")))
            emit pile_changed(name);
    }
}

QString ClientPlayer::getDeathPixmapPath() const
{
    QString basename;
    if (ServerInfo.GameMode == QStringLiteral("06_3v3") || ServerInfo.GameMode == QStringLiteral("06_XMode")) {
        if (getRole() == QStringLiteral("lord") || getRole() == QStringLiteral("renegade"))
            basename = QStringLiteral("marshal");
        else
            basename = QStringLiteral("guard");
    } else
        basename = getRole();

    if (basename.isEmpty())
        basename = QStringLiteral("unknown");

    return QStringLiteral("image/system/death/%1.png").arg(basename);
}

void ClientPlayer::setHandcardNum(int n)
{
    handcard_num = n;
}

QString ClientPlayer::getGameMode() const
{
    return ServerInfo.GameMode;
}

void ClientPlayer::setFlags(const QString &flag)
{
    Player::setFlags(flag);

    if (flag.endsWith(QStringLiteral("actioned")))
        emit action_taken();

    emit skill_state_changed(flag);
}

void ClientPlayer::setMark(const QString &mark, int value)
{
    if (marks[mark] == value)
        return;
    marks[mark] = value;

    if (mark == QStringLiteral("drank") || mark == QStringLiteral("magic_drank"))
        emit drank_changed();

    if (!mark.startsWith(QStringLiteral("@")))
        return;

        // @todo: consider move all the codes below to PlayerCardContainerUI.cpp
#if 0
    // set mark doc
    QString text = "";
    QMapIterator<QString, int> itor(marks);
    while (itor.hasNext()) {
        itor.next();

        if (itor.key().startsWith("@") && itor.value() > 0) {
            if (this == Self && (itor.key() == "@HalfLife" || itor.key() == "@CompanionEffect" || itor.key() == "@Pioneer"))
                continue;

            QString itorKey = itor.key();
            if (itorKey == "@dimai_displaying")
                itorKey.append(QString::number(itor.value()));

            QString mark_text = QString("<img src='image/mark/%1.png' />").arg(itorKey);
            if ((itor.key() != "@dimai_displaying") && (itor.value() != 1))
                mark_text.append(QString("<font size='4'>%1</font>").arg(itor.value()));
            if (this != Self)
                mark_text.append("<br>");
            text.append(mark_text);
        }
    }

    mark_doc->setHtml(text);
#endif
}

RoomObject *ClientPlayer::getRoomObject() const
{
    return getClient();
}

Client *ClientPlayer::getClient() const
{
    return qobject_cast<Client *>(parent());
}
