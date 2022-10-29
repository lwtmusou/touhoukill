#include "legacysemaphore.h"

#if QT_CONFIG(thread)
// use QSemaphore
#else

QSgsNullSemaphore::QSgsNullSemaphore(int n)
{
}

QSgsNullSemaphore::~QSgsNullSemaphore()
{
}

void QSgsNullSemaphore::acquire(int n)
{
}

bool QSgsNullSemaphore::tryAcquire(int n)
{
    return false;
}

bool QSgsNullSemaphore::tryAcquire(int n, int timeout)
{
    return false;
}

void QSgsNullSemaphore::release(int n)
{
}

int QSgsNullSemaphore::available() const
{
    return 0;
}

#endif
