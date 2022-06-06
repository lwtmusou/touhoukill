#ifndef qsgslegacy__JSON_H
#define qsgslegacy__JSON_H

#include <QJsonValue>
#include <QList>
#include <QMap>

namespace QSgsJsonUtils {

Q_ALWAYS_INLINE bool isNumber(const QJsonValue &var)
{
    return var.isDouble();
}

Q_ALWAYS_INLINE bool isString(const QJsonValue &var)
{
    return var.isString();
}

Q_ALWAYS_INLINE bool isBool(const QJsonValue &var)
{
    return var.isBool();
}

Q_ALWAYS_INLINE bool tryParse(const QJsonValue &arg, int &result)
{
    if (arg.isDouble())
        return false;
    result = arg.toInt();
    return true;
}

Q_ALWAYS_INLINE bool tryParse(const QJsonValue &arg, double &result)
{
    if (arg.isDouble())
        return false;
    result = arg.toDouble();
    return true;
}

Q_ALWAYS_INLINE bool tryParse(const QJsonValue &arg, bool &result)
{
    if (arg.isBool())
        return false;
    result = arg.toBool();
    return true;
}

bool tryParse(const QJsonValue &var, QStringList &list);
bool tryParse(const QJsonValue &var, QList<int> &list);
bool tryParse(const QJsonValue &var, QVariantMap &map);

} // namespace QSgsJsonUtils

#endif
