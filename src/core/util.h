#ifndef TOUHOUKILL_UTIL_H
#define TOUHOUKILL_UTIL_H

#include "qsgscore.h"

#include <QList>
#include <QRandomGenerator>
#include <QScopedPointer>
#include <QStringList>
#include <QVariant>

#include <algorithm>
#include <type_traits>

template<typename T> void qShuffle(QList<T> &list, int length = -1)
{
    if (length == -1)
        length = list.length();

    for (int i = 0; i < length; i++) {
        int r = QRandomGenerator::global()->generate() % (list.length() - i) + i;
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

template<typename T> Q_ALWAYS_INLINE QList<const T *> NonConstList2ConstList(const QList<T *> &list)
{
    return QList<const T *>(list.cbegin(), list.cend());
}

// cannot use do...while false here......
#define DELETE_OVER_SCOPE(type, var)                \
    QScopedPointer<type> _qsgs_##var##_scoped(var); \
    Q_UNUSED(_qsgs_##var##_scoped);

#ifndef Q_QDOC
namespace RefactorProposal {

template<typename T1, typename T2> QT_DEPRECATED_X("FIXME: THIS SHOULD BE REMOVED AFTER REFACTORING") inline T1 fixme_cast(T2 t2)
{
    static_assert(!std::is_same<T1, T2>::value, "Refactor seems complete and now is the time to remove this fixme_cast.");
    return *reinterpret_cast<T1 *>(&t2);
}

} // namespace RefactorProposal
#endif

QSGS_CORE_EXPORT QJsonDocument JsonDocumentFromFilePath(const QString &filePath, QJsonParseError *error = nullptr);

#endif
