#ifndef _SERVER_H
#define _SERVER_H

class Room;
class QGroupBox;
class QLabel;
class QRadioButton;

#include "clientstruct.h"
#include "detector.h"
#include "socket.h"

class ServerPlayer;

class Server : public QObject
{
    Q_OBJECT

public:
    explicit Server(QObject *parent);

    void broadcast(const QString &msg);
    bool listen();
    void daemonize();
    Room *createNewRoom();
    void signupPlayer(ServerPlayer *player);

private:
    ServerSocket *server;
    Room *current;
    QSet<Room *> rooms;
    QHash<QString, ServerPlayer *> players;
    QSet<QString> addresses;
    QMultiHash<QString, QString> name2objname;

private:
    void getLack(ClientSocket *socket);
    void getWinnersTableFile(ClientSocket *socket, const QString &tableName);

private slots:
    void processNewConnection(ClientSocket *socket);
    void processRequest(const char *request);
    void cleanupSimc();
    void gameOver();

signals:
    void server_message(const QString &);
};

#endif
