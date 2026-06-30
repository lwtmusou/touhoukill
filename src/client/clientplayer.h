#ifndef THKILL_CLIENT_PLAYER_H
#define THKILL_CLIENT_PLAYER_H

#include "clientstruct.h"
#include "player.h"

#include <QPointer>

class Client;
class QTextDocument;

class ClientPlayer : public Player
{
    Q_OBJECT
    Q_PROPERTY(int handcard READ getHandcardNum WRITE setHandcardNum NOTIFY handcardChanged)

public:
    explicit ClientPlayer(Client *client);
    [[nodiscard]] QList<const Card *> getHandcards() const override;
    void setCards(const QList<int> &card_ids);
    void changePile(const QString &name, bool add, const QList<int> &card_ids);
    [[nodiscard]] QString getDeathPixmapPath() const;
    void setPile(const QString &name, const QList<int> &card_ids);
    void setHandcardNum(int n);
    [[nodiscard]] QString getGameMode() const override;

    void setFlags(const QString &flag) override;
    [[nodiscard]] int aliveCount(bool includeRemoved = true) const override;
    [[nodiscard]] int getHandcardNum() const override;
    void removeCard(const Card *card, Place place) override;
    void addCard(const Card *card, Place place) override;
    virtual void addKnownHandCard(const Card *card);
    virtual void removeKnownHandCard(int card_id);
    QList<int> getKnownHandCardIds() const;
    bool isLastHandCard(const Card *card, bool contain = false) const override;
    void setMark(const QString &mark, int value) override;

private:
    int handcard_num;
    QList<const Card *> known_cards;

signals:
    void pile_changed(const QString &name);
    void drank_changed();
    void action_taken();
    void duozhi_changed();
    void mark_changed();

    void handcardChanged();
};

extern QPointer<ClientPlayer> Self;

#endif
