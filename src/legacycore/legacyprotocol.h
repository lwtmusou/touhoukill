#ifndef qsgslegacy__PROTOCOL_H
#define qsgslegacy__PROTOCOL_H

#include <QVariant>

namespace QSanProtocol {

class Countdown
{
public:
    enum CountdownType
    {
        S_COUNTDOWN_NO_LIMIT,
        S_COUNTDOWN_USE_SPECIFIED,
        S_COUNTDOWN_USE_DEFAULT
    } type;

    time_t current;
    time_t max;
    inline explicit Countdown(CountdownType type = S_COUNTDOWN_NO_LIMIT, time_t current = 0, time_t max = 0)
        : type(type)
        , current(current)
        , max(max)
    {
    }
    bool tryParse(const QJsonValue &val);
    QJsonValue toVariant() const;
};

} // namespace QSanProtocol

#endif
