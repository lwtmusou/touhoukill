#ifndef TOUHOUKILL_QJSONUTILS_H
#define TOUHOUKILL_QJSONUTILS_H

#include "qsgscore.h"

#include <QList>
#include <QStringList>

class QJsonArray;
class QJsonValue;

namespace QSgsJsonUtils {

QSGS_CORE_EXPORT bool isStringArray(const QJsonArray &var, int from = 0, int to = -1);
QSGS_CORE_EXPORT bool isStringArray(const QJsonValue &var, int from = 0, int to = -1);
QSGS_CORE_EXPORT bool isNumberArray(const QJsonArray &var, int from = 0, int to = -1);
QSGS_CORE_EXPORT bool isNumberArray(const QJsonValue &var, int from = 0, int to = -1);

QSGS_CORE_EXPORT QJsonArray toJsonArray(const QList<int> &intArray);
QSGS_CORE_EXPORT QJsonArray toJsonArray(const QStringList &stringArray);

QSGS_CORE_EXPORT QStringList toStringList(const QJsonValue &var, bool *ok = nullptr);
QSGS_CORE_EXPORT QList<int> toIntList(const QJsonValue &var, bool *ok = nullptr);

// deprecated

Q_DECL_DEPRECATED Q_ALWAYS_INLINE bool isNumber(const QJsonValue &var)
{
    return var.isDouble();
}

Q_DECL_DEPRECATED Q_ALWAYS_INLINE bool isString(const QJsonValue &var)
{
    return var.isString();
}

Q_DECL_DEPRECATED Q_ALWAYS_INLINE bool isBool(const QJsonValue &var)
{
    return var.isBool();
}

Q_DECL_DEPRECATED Q_ALWAYS_INLINE bool tryParse(const QJsonValue &arg, int &result)
{
    if (arg.isDouble())
        return false;
    result = arg.toInt();
    return true;
}

Q_DECL_DEPRECATED Q_ALWAYS_INLINE bool tryParse(const QJsonValue &arg, double &result)
{
    if (arg.isDouble())
        return false;
    result = arg.toDouble();
    return true;
}

Q_DECL_DEPRECATED Q_ALWAYS_INLINE bool tryParse(const QJsonValue &arg, bool &result)
{
    if (arg.isBool())
        return false;
    result = arg.toBool();
    return true;
}

Q_DECL_DEPRECATED QSGS_CORE_EXPORT bool tryParse(const QJsonValue &var, QStringList &list);
Q_DECL_DEPRECATED QSGS_CORE_EXPORT bool tryParse(const QJsonValue &var, QList<int> &list);
Q_DECL_DEPRECATED QSGS_CORE_EXPORT bool tryParse(const QJsonValue &var, QVariantMap &map);

} // namespace QSgsJsonUtils

#endif
