#ifndef _UTIL_H
#define _UTIL_H

struct lua_State;
class QVariant;

#include <QList>
#include <QSharedPointer>
#include <QStringList>
#include <QVariant>

#include "compiler-specific.h"

#include <algorithm>
#include <random>

int qsgsRand();

template<typename T> QSet<T> List2Set(const QList<T> &t)
{
#if QT_VERSION_MAJOR >= 6
    return QSet<T>(t.constBegin(), t.constEnd());
#else
    return t.toSet();
#endif
}

template<typename T> void qShuffle(QList<T> &list)
{
    int n = list.length();
    for (int i = 0; i < n; i++) {
        int r = qsgsRand() % (n - i) + i;
        list.swapItemsAt(i, r);
    }
}

// lua interpreter related
lua_State *CreateLuaState();
void DoLuaScript(lua_State *L, const char *script);

QVariant GetValueFromLuaState(lua_State *L, const char *table_name, const char *key);

QStringList IntList2StringList(const QList<int> &intlist);
QList<int> StringList2IntList(const QStringList &stringlist);
QVariantList IntList2VariantList(const QList<int> &intlist);
QList<int> VariantList2IntList(const QVariantList &variantlist);

bool isNormalGameMode(const QString &mode);
bool isHegemonyGameMode(const QString &mode);

// cannot use do...while false here......
#define DELETE_OVER_SCOPE(type, var)                \
    QScopedPointer<type> _qsgs_##var##_scoped(var); \
    Q_UNUSED(_qsgs_##var##_scoped);

#endif
