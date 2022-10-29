#ifndef qsgslegacy__SEMAPHORE_H
#define qsgslegacy__SEMAPHORE_H

#include <QtGlobal>

#if QT_CONFIG(thread)
#include <QSemaphore>
#else

class QSgsNullSemaphore
{
public:
    explicit QSgsNullSemaphore(int n = 0);

    ~QSgsNullSemaphore();

    void acquire(int n = 1);
    bool tryAcquire(int n = 1);
    bool tryAcquire(int n, int timeout);

    void release(int n = 1);
    int available() const;

private:
    Q_DISABLE_COPY(QSgsNullSemaphore);
};

using QSemaphore = QSgsNullSemaphore;

#endif
#endif
