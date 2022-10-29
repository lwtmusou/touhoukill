#ifndef qsgslegacy__ROOM_THREAD_H
#define qsgslegacy__ROOM_THREAD_H

#include <QThread>
#include <QVariant>

#include "structs.h"

class LegacyGameRule;
class LegacyRoom;
class LegacyServerPlayer;

class RoomThread : public QThread
{
    Q_OBJECT

public:
    explicit RoomThread(LegacyRoom *room);
    void constructTriggerTable();

private:
    void getTriggerAndSort(QSanguosha::TriggerEvent e, QList<QSharedPointer<TriggerDetail>> &detailsList, const QList<QSharedPointer<TriggerDetail>> &triggered,
                           const QVariant &data);

public:
    bool trigger(QSanguosha::TriggerEvent e, QVariant &data);

    void addPlayerSkills(LegacyServerPlayer *player, bool invoke_game_start = false);

    void addTrigger(const Trigger *skill);
    void delay(long msecs = -1);
    LegacyServerPlayer *find3v3Next(QList<LegacyServerPlayer *> &first, QList<LegacyServerPlayer *> &second);
    void run3v3(QList<LegacyServerPlayer *> &first, QList<LegacyServerPlayer *> &second, LegacyGameRule *game_rule, LegacyServerPlayer *current);
    void actionHulaoPass(LegacyServerPlayer *uuz, QList<LegacyServerPlayer *> league, LegacyGameRule *game_rule);
    LegacyServerPlayer *findHulaoPassNext(LegacyServerPlayer *uuz, const QList<LegacyServerPlayer *> &league);
    void actionNormal(LegacyGameRule *game_rule);

    inline LegacyGameRule *gameRule() const
    {
        return game_rule;
    }

    void setNextExtraTurn(LegacyServerPlayer *p)
    {
        nextExtraTurn = p;
    }

    inline bool hasExtraTurn() const
    {
        return nextExtraTurn != nullptr;
    }

    inline LegacyServerPlayer *getNextExtraTurn() const
    {
        return nextExtraTurn;
    }

    inline LegacyServerPlayer *getExtraTurnReturn() const
    {
        return extraTurnReturn;
    }

    inline LegacyRoom *getRoom() const
    {
        return room;
    }

protected:
    void run() override;

private:
    void _handleTurnBroken3v3(QList<LegacyServerPlayer *> &first, QList<LegacyServerPlayer *> &second, LegacyGameRule *game_rule);
    void _handleTurnBrokenHulaoPass(LegacyServerPlayer *uuz, const QList<LegacyServerPlayer *> &league, LegacyGameRule *game_rule);
    void _handleTurnBrokenNormal(LegacyGameRule *game_rule);

    LegacyRoom *room;

    QList<const Trigger *> m_triggerList[QSanguosha::NumOfEvents];

    LegacyGameRule *game_rule;

    LegacyServerPlayer *nextExtraTurn;
    LegacyServerPlayer *extraTurnReturn;
};

#endif
