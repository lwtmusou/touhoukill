#ifndef _UTIL_H
#define _UTIL_H

class QVariant;

#include "compiler-specific.h"

#include <QList>
#include <QRandomGenerator>
#include <QSharedPointer>
#include <QStringList>
#include <QVariant>

#include <algorithm>
#include <type_traits>

template <typename T> void qShuffle(QList<T> &list)
{
    int n = list.length();
    for (int i = 0; i < n; i++) {
        int r = QRandomGenerator::global()->generate() % (n - i) + i;
        list.swapItemsAt(i, r);
    }
}

QStringList IntList2StringList(const QList<int> &intlist);
QList<int> StringList2IntList(const QStringList &stringlist);
QVariantList IntList2VariantList(const QList<int> &intlist);
QList<int> VariantList2IntList(const QVariantList &variantlist);

// QList::toSet is got deleted by Qt since Qt 5.14
// Can this be inlined?
template <typename T> inline QSet<T> List2Set(const QList<T> &list)
{
    return QSet<T>(list.begin(), list.end());
}

bool isNormalGameMode(const QString &mode);
bool isHegemonyGameMode(const QString &mode);

// cannot use do...while false here......
#define DELETE_OVER_SCOPE(type, var)            \
    QScopedPointer<type> __##var##_scoped(var); \
    Q_UNUSED(__##var##_scoped);

namespace RefactorProposal {

template <typename T1, typename T2> QT_DEPRECATED_X("FIXME: THIS SHOULD BE REMOVED AFTER REFACTORING") inline T1 fixme_cast(T2 t2)
{
    static_assert(!std::is_same<T1, T2>::value, "T1 and T2 should not be the same type.");
    return (T1)t2;
}

}

#endif
