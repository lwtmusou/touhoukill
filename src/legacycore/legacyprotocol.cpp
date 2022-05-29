#include "legacyprotocol.h"

#include "json.h"

bool QSanProtocol::Countdown::tryParse(const QVariant &var)
{
    if (!var.canConvert<QVariantList>())
        return false;

    QVariantList val = var.value<QVariantList>();

    //compatible with old JSON representation of Countdown
    if (JsonUtils::isString(val[0])) {
        if (val[0].toString() == QStringLiteral("MG_COUNTDOWN"))
            val.removeFirst();
        else
            return false;
    }

    if (val.size() == 2) {
        if (!JsonUtils::isNumberArray(val, 0, 1))
            return false;
        current = (time_t)val[0].toInt();
        max = (time_t)val[1].toInt();
        type = S_COUNTDOWN_USE_SPECIFIED;
        return true;

    } else if (val.size() == 1 && val[0].canConvert<int>()) {
        CountdownType type = (CountdownType)val[0].toInt();
        if (type != S_COUNTDOWN_NO_LIMIT && type != S_COUNTDOWN_USE_DEFAULT)
            return false;
        else
            this->type = type;
        return true;

    } else
        return false;
}

QVariant QSanProtocol::Countdown::toVariant() const
{
    JsonArray val;
    if (type == S_COUNTDOWN_NO_LIMIT || type == S_COUNTDOWN_USE_DEFAULT) {
        val << (int)type;
    } else {
        val << (int)current;
        val << (int)max;
    }
    return val;
}
