#ifndef TOUHOUKILL_AGENT_H_
#define TOUHOUKILL_AGENT_H_

#include "qsgscore.h"
#include <QObject>

class QSGS_CORE_EXPORT Agent final : public QObject
{
    Q_OBJECT

public:
    Agent();
    ~Agent();

public:
    // low-level functions
    bool request(...);
    bool waitForReply(...);
    bool notify(...);

signals:
    void replied(int);
    void notified(int);
};

#endif
