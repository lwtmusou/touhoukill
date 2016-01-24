#ifndef _UTIL_H
#define _UTIL_H

struct lua_State;
class QVariant;

#include <QList>
#include <QStringList>
#include <QVariant>
#include <QSharedPointer>

#include "compiler-specific.h"
#include <algorithm>

template<typename T>
void qShuffle(QList<T> &list)
{
    int i, n = list.length();
    for (i = 0; i < n; i++) {
        int r = qrand() % (n - i) + i;
        list.swap(i, r);
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

// cannot use do...while false here......
#define DELETE_OVER_SCOPE(type, var) \
    QScopedPointer<type> __ ## var ## _scoped(var); \
    Q_UNUSED(__ ## var ## _scoped);

#endif

