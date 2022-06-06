#ifndef qsgslegacy__SERVER_H
#define qsgslegacy__SERVER_H

class LegacyRoom;

#include "legacydetector.h"
#include "legacysocket.h"
#include "serverinfostruct.h"

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
    LegacyServerSocket *server;
    LegacyRoom *current;
    QSet<LegacyRoom *> rooms;
    QHash<QString, LegacyServerPlayer *> players;
    QSet<QString> addresses;
    QMultiHash<QString, QString> name2objname;

    QStringList getNeededPackages() const;

    ServerInfoStruct ServerInfo;

private slots:
    void processNewConnection(LegacyClientSocket *socket);
    void processRequest(const char *request);
    void cleanupSimc();
    void gameOver();

signals:
    void server_message(const QString &);
};

#endif
