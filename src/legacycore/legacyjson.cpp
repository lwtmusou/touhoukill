#include "legacyjson.h"

#include <QJsonArray>
#include <QJsonValue>

bool QSgsJsonUtils::tryParse(const QJsonValue &var, QStringList &list)
{
    if (!var.isArray())
        return false;

    QJsonArray array = var.toArray();

    for (const QJsonValue &var : array) {
        if (!var.isString())
            return false;
    }

    list.clear();
    for (const QJsonValue &var : array)
        list << var.toString();

    return true;
}

bool QSgsJsonUtils::tryParse(const QJsonValue &var, QList<int> &list)
{
    if (!var.isArray())
        return false;

    QJsonArray array = var.toArray();

    for (const QJsonValue &var : array) {
        if (!var.isDouble())
            return false;
    }

    list.clear();
    for (const QJsonValue &var : array)
        list << var.toInt();

    return true;
}

bool QSgsJsonUtils::tryParse(const QJsonValue &var, QVariantMap &map)
{
    if (!var.isObject())
        return false;

    map = var.toObject().toVariantMap();
    return true;
}
