#ifndef _UTIL_H
#define _UTIL_H

#include "qsgscore.h"

#include <QList>
#include <QRandomGenerator>
#include <QSharedPointer>
#include <QStringList>
#include <QVariant>

#include <algorithm>
#include <type_traits>

template<typename T> void qShuffle(QList<T> &list)
{
    int n = list.length();
    for (int i = 0; i < n; i++) {
        int r = QRandomGenerator::global()->generate() % (n - i) + i;
        list.swapItemsAt(i, r);
    }
}

QSGS_CORE_EXPORT QStringList IntList2StringList(const QList<int> &intlist);
QSGS_CORE_EXPORT QList<int> StringList2IntList(const QStringList &stringlist);
QSGS_CORE_EXPORT QVariantList IntList2VariantList(const QList<int> &intlist);
QSGS_CORE_EXPORT QList<int> VariantList2IntList(const QVariantList &variantlist);

// QList::toSet is got deleted by Qt since Qt 5.14
// Can this be inlined?
template<typename T> Q_ALWAYS_INLINE QSet<T> List2Set(const QList<T> &list)
{
    return QSet<T>(list.begin(), list.end());
}

template<typename T> inline QList<const T *> NonConstList2ConstList(const QList<T *> &list)
{
    QList<const T *> cl;
    foreach (T *i, list)
        cl << i;
    return cl;
}

QSGS_CORE_EXPORT bool isRoleGameMode(const QString &mode);
QSGS_CORE_EXPORT bool isHegemonyGameMode(const QString &mode);

// cannot use do...while false here......
#define DELETE_OVER_SCOPE(type, var)            \
    QScopedPointer<type> __##var##_scoped(var); \
    Q_UNUSED(__##var##_scoped);

#ifndef Q_QDOC
namespace RefactorProposal {

template<typename T1, typename T2> QT_DEPRECATED_X("FIXME: THIS SHOULD BE REMOVED AFTER REFACTORING") inline T1 fixme_cast(T2 t2)
{
    static_assert(!std::is_same<T1, T2>::value, "Refactor seems complete and now is the time to remove this fixme_cast.");
    return *reinterpret_cast<T1 *>(&t2);
}

} // namespace RefactorProposal
#endif

#endif
