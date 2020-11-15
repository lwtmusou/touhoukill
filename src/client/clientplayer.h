#ifndef _CLIENT_PLAYER_H
#define _CLIENT_PLAYER_H

#include "clientstruct.h"
#include "player.h"

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
    void changePile(const QString &name, bool add, QList<int> card_ids);
    QString getDeathPixmapPath() const;
    void setHandcardNum(int n);
    QString getGameMode() const override;

    void setFlags(const QString &flag) override;
    int aliveCount(bool includeRemoved = true) const override;
    int getHandcardNum() const override;
    void removeCard(const Card *card, Place place) override;
    void addCard(const Card *card, Place place) override;
    virtual void addKnownHandCard(const Card *card);
    bool isLastHandCard(const Card *card, bool contain = false) const override;
    void setMark(const QString &mark, int value) override;

    RoomObject *getRoomObject() const override;

private:
    int handcard_num;
    QList<const Card *> known_cards;
    QTextDocument *mark_doc;

signals:
    void pile_changed(const QString &name);
    void drank_changed();
    void action_taken();
    void skill_state_changed(const QString &skill_name);
};

extern ClientPlayer *Self;

#endif
