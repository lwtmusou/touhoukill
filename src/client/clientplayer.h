#ifndef _CLIENT_PLAYER_H
#define _CLIENT_PLAYER_H

#include "player.h"
#include "serverinfostruct.h"

class Client;
class RoomObject;
class QTextDocument;

class ClientPlayer : public Player
{
    Q_OBJECT
    Q_PROPERTY(int handcard READ getHandcardNum WRITE setHandcardNum)

public:
    explicit ClientPlayer(Client *client);
    QList<const Card *> getHandcards() const override;
    void setCards(const QList<int> &card_ids);
    QTextDocument *getMarkDoc() const;
    QString getDeathPixmapPath() const;
    void setHandcardNum(int n);

    int getHandcardNum() const override;
    void removeCard(const Card *card, QSanguosha::Place place) override;
    void addCard(const Card *card, QSanguosha::Place place) override;
    void addKnownHandCard(const Card *card);
    bool isLastHandCard(const Card *card, bool contain = false) const override;

    Client *getClient() const;

private:
    int handcard_num;
    QList<const Card *> known_cards;
    QTextDocument *mark_doc;

#if 0
signals:
    void drank_changed();
    void action_taken();
    void skill_state_changed(const QString &skill_name);
#endif
};

#endif
