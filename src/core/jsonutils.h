#ifndef TOUHOUKILL_QJSONUTILS_H
#define TOUHOUKILL_QJSONUTILS_H

#include "qsgscore.h"

#include <QList>
#include <QStringList>

class QJsonArray;
class QJsonValue;

namespace QSgsJsonUtils {

bool isStringArray(const QJsonArray &var, int from = 0, int to = -1);
bool isStringArray(const QJsonValue &var, int from = 0, int to = -1);
bool isNumberArray(const QJsonArray &var, int from = 0, int to = -1);
bool isNumberArray(const QJsonValue &var, int from = 0, int to = -1);

QJsonArray toJsonArray(const QList<int> &intArray);
QJsonArray toJsonArray(const QStringList &stringArray);

QStringList toStringList(const QJsonValue &var, bool *ok = nullptr);
QList<int> toIntList(const QJsonValue &var, bool *ok = nullptr);

} // namespace QSgsJsonUtils

#endif
