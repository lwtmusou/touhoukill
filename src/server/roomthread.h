#ifndef _ROOM_THREAD_H
#define _ROOM_THREAD_H

#include <QSemaphore>
#include <QThread>
#include <QVariant>

#include "structs.h"

class GameRule;
class Room;
class ServerPlayer;

struct LogMessage
{
    LogMessage();
    QString toString() const;
    QVariant toJsonValue() const;

    QString type;
    Player *from;
    QList<Player *> to;
    QString card_str;
    QString arg;
    QString arg2;
};

class RoomThread : public QThread
{
    Q_OBJECT

public:
    explicit RoomThread(Room *room);
    void constructTriggerTable();

private:
    void getTriggerAndSort(QSanguosha::TriggerEvent e, QList<QSharedPointer<TriggerDetail>> &detailsList, const QList<QSharedPointer<TriggerDetail>> &triggered,
                           const QVariant &data);

public:
    bool trigger(QSanguosha::TriggerEvent e, QVariant &data);

    void addPlayerSkills(ServerPlayer *player, bool invoke_game_start = false);

    void addTriggerSkill(const Trigger *skill);
    void delay(long msecs = -1);
    ServerPlayer *find3v3Next(QList<ServerPlayer *> &first, QList<ServerPlayer *> &second);
    void run3v3(QList<ServerPlayer *> &first, QList<ServerPlayer *> &second, GameRule *game_rule, ServerPlayer *current);
    void actionHulaoPass(ServerPlayer *uuz, QList<ServerPlayer *> league, GameRule *game_rule);
    ServerPlayer *findHulaoPassNext(ServerPlayer *uuz, const QList<ServerPlayer *> &league);
    void actionNormal(GameRule *game_rule);

    inline GameRule *gameRule() const
    {
        return game_rule;
    }

    void setNextExtraTurn(ServerPlayer *p)
    {
        nextExtraTurn = p;
    }

    inline bool hasExtraTurn() const
    {
        return nextExtraTurn != nullptr;
    }

    inline ServerPlayer *getNextExtraTurn() const
    {
        return nextExtraTurn;
    }

    inline ServerPlayer *getExtraTurnReturn() const
    {
        return extraTurnReturn;
    }

    inline Room *getRoom() const
    {
        return room;
    }

protected:
    void run() override;

private:
    void _handleTurnBroken3v3(QList<ServerPlayer *> &first, QList<ServerPlayer *> &second, GameRule *game_rule);
    void _handleTurnBrokenHulaoPass(ServerPlayer *uuz, const QList<ServerPlayer *> &league, GameRule *game_rule);
    void _handleTurnBrokenNormal(GameRule *game_rule);

    Room *room;

    QList<const Trigger *> skill_table[QSanguosha::NumOfEvents];
    QSet<QString> skillSet;

    GameRule *game_rule;

    ServerPlayer *nextExtraTurn;
    ServerPlayer *extraTurnReturn;
};

#endif
