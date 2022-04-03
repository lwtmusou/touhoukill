/********************************************************************
    Copyright (c) 2013-2015 - Mogara

    This file is part of QSanguosha-Hegemony.

    This game is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3.0
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    See the LICENSE file for more details.

    Mogara
    *********************************************************************/

#ifndef JSON_H
#define JSON_H

#include "qsgscore.h"

#include <QVariantList>
#include <QVariantMap>

//Directly apply two containers of Qt here. Reimplement the 2 classes if necessary.
typedef QVariantList JsonArray;
typedef QVariantMap JsonObject;

class QSGS_CORE_EXPORT JsonDocument
{
public:
    JsonDocument();
    explicit JsonDocument(const QVariant &var);

    explicit JsonDocument(const JsonArray &array);
    explicit JsonDocument(const JsonObject &object);

    QByteArray toJson(bool isIndented = false) const;
    static JsonDocument fromJson(const QByteArray &json, bool allowComment = false);
    static JsonDocument fromFilePath(const QString &path, bool allowComment = true);

    inline bool isArray() const
    {
        return value.canConvert<JsonArray>();
    }
    inline bool isObject() const
    {
        return value.canConvert<JsonObject>();
    }
    inline bool isValid() const
    {
        return valid;
    }

    inline JsonArray array() const
    {
        return value.value<JsonArray>();
    }
    inline JsonObject object() const
    {
        return value.value<JsonObject>();
    }
    inline const QVariant &toVariant() const
    {
        return value;
    }
    inline QString errorString() const
    {
        return error;
    }

protected:
    QVariant value;
    bool valid;
    QString error;
};

namespace JsonUtils {

inline bool isNumber(const QVariant &var)
{
    return var.userType() == QMetaType::Double || var.userType() == QMetaType::Int || var.userType() == QMetaType::UInt || var.userType() == QMetaType::Long
        || var.userType() == QMetaType::LongLong || var.userType() == QMetaType::ULong || var.userType() == QMetaType::ULongLong || var.userType() == QMetaType::UShort
        || var.userType() == QMetaType::Short;
}

inline bool isString(const QVariant &var)
{
    return var.userType() == QMetaType::QString;
}

inline bool isBool(const QVariant &var)
{
    return var.userType() == QMetaType::Bool;
}

QSGS_CORE_EXPORT bool isStringArray(const QVariant &var, unsigned from, unsigned to);
QSGS_CORE_EXPORT bool isNumberArray(const QVariant &var, unsigned from, unsigned to);

QSGS_CORE_EXPORT QVariant toJsonArray(const QList<int> &intArray);
QSGS_CORE_EXPORT QVariant toJsonArray(const QStringList &stringArray);

QSGS_CORE_EXPORT bool tryParse(const QVariant &, int &);
QSGS_CORE_EXPORT bool tryParse(const QVariant &, double &);
QSGS_CORE_EXPORT bool tryParse(const QVariant &, bool &);

QSGS_CORE_EXPORT bool tryParse(const QVariant &var, QStringList &list);
QSGS_CORE_EXPORT bool tryParse(const QVariant &var, QList<int> &list);
QSGS_CORE_EXPORT bool tryParse(const QVariant &var, QVariantMap &map);
QSGS_CORE_EXPORT bool tryParse(const QVariant &arg, QRect &result);
QSGS_CORE_EXPORT bool tryParse(const QVariant &arg, QSize &result);
QSGS_CORE_EXPORT bool tryParse(const QVariant &arg, QPoint &result);
QSGS_CORE_EXPORT bool tryParse(const QVariant &arg, QColor &result);
QSGS_CORE_EXPORT bool tryParse(const QVariant &arg, Qt::Alignment &align);
} // namespace JsonUtils

#endif // JSON_H
