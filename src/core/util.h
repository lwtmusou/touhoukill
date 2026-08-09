#ifndef THKILL_UTIL_H
#define THKILL_UTIL_H

struct lua_State;
class QVariant;

#include <QList>
#include <QSharedPointer>
#include <QStringList>
#include <QVariant>

#include "compiler-specific.h"

#include <algorithm>
#include <random>

unsigned int qsgsRand();

// TODO: rename this function
template<typename T> [[nodiscard]] auto List2Set(const T &t)
{
    return QSet<typename T::value_type>(std::cbegin(t), std::cend(t));
}

// TODO: rename this function
template<typename T> void qShuffle(T &t)
{
    static thread_local std::minstd_rand engine {std::random_device()()};
    std::shuffle(std::begin(t), std::end(t), engine);
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
