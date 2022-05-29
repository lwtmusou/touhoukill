#include "jsonutils.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

bool QSgsJsonUtils::isStringArray(const QJsonArray &var, int from, int to)
{
    if (to < 0)
        to = var.count();

    for (int i = from; i < to; ++i) {
        if (!var.at(i).isString())
            return false;
    }

    return true;
}

bool QSgsJsonUtils::isStringArray(const QJsonValue &var, int from, int to)
{
    if (var.isArray())
        return isStringArray(var.toArray(), from, to);
    return false;
}

bool QSgsJsonUtils::isNumberArray(const QJsonArray &var, int from, int to)
{
    if (to < 0)
        to = var.count();

    for (int i = from; i < to; ++i) {
        if (!var.at(i).isDouble())
            return false;
    }

    return true;
}

bool QSgsJsonUtils::isNumberArray(const QJsonValue &var, int from, int to)
{
    if (var.isArray())
        return isNumberArray(var.toArray(), from, to);
    return false;
}

QJsonArray QSgsJsonUtils::toJsonArray(const QList<int> &intArray)
{
    QJsonArray arr;
    foreach (int i, intArray)
        arr.append(i);
    return arr;
}

QJsonArray QSgsJsonUtils::toJsonArray(const QStringList &stringArray)
{
    QJsonArray arr;
    foreach (const QString &i, stringArray)
        arr.append(i);
    return arr;
}

QStringList QSgsJsonUtils::toStringList(const QJsonValue &var, bool *ok)
{
    if (ok == nullptr) {
        static bool _ok;
        ok = &_ok;
    }
    *ok = false;
    QStringList ret;
    bool flag = true;
    if (var.isArray()) {
        QJsonArray arr = var.toArray();
        for (const QJsonValue &value : arr) {
            if (value.isString()) {
                ret << value.toString();
            } else {
                flag = false;
                break;
            }
        }
    }

    if (!flag)
        return QStringList();

    *ok = true;
    return ret;
}

QList<int> QSgsJsonUtils::toIntList(const QJsonValue &var, bool *ok)
{
    if (ok == nullptr) {
        static bool _ok;
        ok = &_ok;
    }
    *ok = false;
    QList<int> ret;
    bool flag = true;
    if (var.isArray()) {
        QJsonArray arr = var.toArray();
        for (const QJsonValue &value : arr) {
            if (value.isDouble()) {
                ret << value.toInt();
            } else {
                flag = false;
                break;
            }
        }
    }

    if (!flag)
        return QList<int>();

    *ok = true;
    return ret;
}

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
