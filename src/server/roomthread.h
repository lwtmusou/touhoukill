#ifndef _ROOM_THREAD_H
#define _ROOM_THREAD_H

#include <QSemaphore>
#include <QThread>
#include <QVariant>

#include "structs.h"

class GameRule;

struct LogMessage
{
    LogMessage();
    QString toString() const;
    QVariant toJsonValue() const;

    QString type;
    ServerPlayer *from;
    QList<ServerPlayer *> to;
    QString card_str;
    QString arg;
    QString arg2;
};

class EventTriplet
{
public:
    inline EventTriplet(TriggerEvent triggerEvent, Room *room)
        : _m_event(triggerEvent)
        , _m_room(room)
    {
    }
    QString toString() const;

private:
    TriggerEvent _m_event;
    Room *_m_room;
};

class RoomThread : public QThread
{
    Q_OBJECT

public:
    explicit RoomThread(Room *room);
    void constructTriggerTable();
    bool trigger(TriggerEvent triggerEvent, Room *room);

    void getSkillAndSort(TriggerEvent triggerEvent, Room *room, QList<QSharedPointer<SkillInvokeDetail> > &detailsList, const QList<QSharedPointer<SkillInvokeDetail> > &triggered,
                         const QVariant &data);
    bool trigger(TriggerEvent triggerEvent, Room *room,
                 QVariant &data); // player is deleted. a lot of things is able to put in data. make a struct for every triggerevent isn't absolutely unreasonable.

    void addPlayerSkills(ServerPlayer *player, bool invoke_game_start = false);

    void addTriggerSkill(const TriggerSkill *skill);
    void delay(long msecs = -1);
    ServerPlayer *find3v3Next(QList<ServerPlayer *> &first, QList<ServerPlayer *> &second);
    void run3v3(QList<ServerPlayer *> &first, QList<ServerPlayer *> &second, GameRule *game_rule, ServerPlayer *current);
    void actionHulaoPass(ServerPlayer *uuz, QList<ServerPlayer *> league, GameRule *game_rule);
    ServerPlayer *findHulaoPassNext(ServerPlayer *uuz, QList<ServerPlayer *> league);
    void actionNormal(GameRule *game_rule);

    const QList<EventTriplet> *getEventStack() const;
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
        return nextExtraTurn != NULL;
    }

    inline Room *getRoom() const
    {
        return room;
    }

protected:
    virtual void run();

private:
    void _handleTurnBroken3v3(QList<ServerPlayer *> &first, QList<ServerPlayer *> &second, GameRule *game_rule);
    void _handleTurnBrokenHulaoPass(ServerPlayer *uuz, QList<ServerPlayer *> league, GameRule *game_rule);
    void _handleTurnBrokenNormal(GameRule *game_rule);

    Room *room;
    QString order;

    QList<const TriggerSkill *> skill_table[NumOfEvents];
    QSet<QString> skillSet;

    QList<EventTriplet> event_stack;
    GameRule *game_rule;

    ServerPlayer *nextExtraTurn;
    ServerPlayer *extraTurnReturn;
};

#endif
