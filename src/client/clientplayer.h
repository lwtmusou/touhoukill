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

public:
    explicit ClientPlayer(Client *client);
    QTextDocument *getMarkDoc() const;
    QString getDeathPixmapPath() const;

    Client *getClient() const;

private:
    QTextDocument *mark_doc;

#if 0
signals:
    void drank_changed();
    void action_taken();
    void skill_state_changed(const QString &skill_name);
#endif
};

#endif
