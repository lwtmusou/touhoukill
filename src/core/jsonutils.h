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

} // namespace QSgsJsonUtils

#endif
