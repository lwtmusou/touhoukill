#include "legacyprotocol.h"
#include "jsonutils.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

bool QSanProtocol::Countdown::tryParse(const QJsonValue &val)
{
    if (!val.isArray())
        return false;
    QJsonArray var = val.toArray();

    if (var.size() == 2) {
        if (!QSgsJsonUtils::isNumberArray(var, 0, 1))
            return false;
        current = (time_t)var[0].toInt();
        max = (time_t)var[1].toInt();
        type = S_COUNTDOWN_USE_SPECIFIED;
        return true;

    } else if (var.size() == 1 && var[0].isDouble()) {
        CountdownType type = (CountdownType)var[0].toInt();
        if (type != S_COUNTDOWN_NO_LIMIT && type != S_COUNTDOWN_USE_DEFAULT)
            return false;
        else
            this->type = type;
        return true;
    } else
        return false;
}

QJsonValue QSanProtocol::Countdown::toVariant() const
{
    QJsonArray val;
    if (type == S_COUNTDOWN_NO_LIMIT || type == S_COUNTDOWN_USE_DEFAULT) {
        val << (int)type;
    } else {
        val << (int)current;
        val << (int)max;
    }
    return val;
}
