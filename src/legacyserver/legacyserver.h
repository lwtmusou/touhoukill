#ifndef _SERVER_H
#define _SERVER_H

class LegacyRoom;

#include "detector.h"
#include "serverinfostruct.h"
#include "socket.h"

class LegacyServerPlayer;

class LegacyServer : public QObject
{
    Q_OBJECT

public:
    explicit LegacyServer(QObject *parent);

    void broadcast(const QString &msg);
    bool listen();
    void daemonize();
    LegacyRoom *createNewRoom();
    void signupPlayer(LegacyServerPlayer *player);

private:
    ServerSocket *server;
    LegacyRoom *current;
    QSet<LegacyRoom *> rooms;
    QHash<QString, LegacyServerPlayer *> players;
    QSet<QString> addresses;
    QMultiHash<QString, QString> name2objname;

    QStringList getNeededPackages() const;

private slots:
    void processNewConnection(ClientSocket *socket);
    void processRequest(const char *request);
    void cleanupSimc();
    void gameOver();

signals:
    void server_message(const QString &);
};

#endif
