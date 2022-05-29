#include "legacystructs.h"
#include "RoomObject.h"
#include "card.h"
#include "json.h"
#include "player.h"
#include "structs.h"
#include "trigger.h"

using namespace QSanguosha;

LegacyCardsMoveStruct::LegacyCardsMoveStruct()
{
    from_place = PlaceUnknown;
    to_place = PlaceUnknown;
    from = nullptr;
    to = nullptr;
    is_last_handcard = false;
}

LegacyCardsMoveStruct::LegacyCardsMoveStruct(const QList<int> &ids, Player *from, Player *to, Place from_place, Place to_place, const CardMoveReason &reason)
{
    this->card_ids = ids;
    this->from_place = from_place;
    this->to_place = to_place;
    this->from = from;
    this->to = to;
    this->reason = reason;
    is_last_handcard = false;
    if (from != nullptr)
        from_player_name = from->objectName();
    if (to != nullptr)
        to_player_name = to->objectName();
}

LegacyCardsMoveStruct::LegacyCardsMoveStruct(const QList<int> &ids, Player *to, Place to_place, const CardMoveReason &reason)
{
    this->card_ids = ids;
    this->from_place = PlaceUnknown;
    this->to_place = to_place;
    this->from = nullptr;
    this->to = to;
    this->reason = reason;
    is_last_handcard = false;
    if (to != nullptr)
        to_player_name = to->objectName();
}

LegacyCardsMoveStruct::LegacyCardsMoveStruct(int id, Player *from, Player *to, Place from_place, Place to_place, const CardMoveReason &reason)
{
    this->card_ids << id;
    this->from_place = from_place;
    this->to_place = to_place;
    this->from = from;
    this->to = to;
    this->reason = reason;
    is_last_handcard = false;
    if (from != nullptr)
        from_player_name = from->objectName();
    if (to != nullptr)
        to_player_name = to->objectName();
}

LegacyCardsMoveStruct::LegacyCardsMoveStruct(int id, Player *to, Place to_place, const CardMoveReason &reason)
{
    this->card_ids << id;
    this->from_place = PlaceUnknown;
    this->to_place = to_place;
    this->from = nullptr;
    this->to = to;
    this->reason = reason;
    is_last_handcard = false;
    if (to != nullptr)
        to_player_name = to->objectName();
}

bool LegacyCardsMoveStruct::operator==(const LegacyCardsMoveStruct &other) const
{
    return from == other.from && from_place == other.from_place && from_pile_name == other.from_pile_name && from_player_name == other.from_player_name;
}

bool LegacyCardsMoveStruct::operator<(const LegacyCardsMoveStruct &other) const
{
    return from < other.from || from_place < other.from_place || from_pile_name < other.from_pile_name || from_player_name < other.from_player_name;
}

bool LegacyCardsMoveStruct::tryParse(const QVariant &arg)
{
    JsonArray args = arg.value<JsonArray>();
    if (args.size() != 8)
        return false;

    if ((!JsonUtils::isNumber(args[0]) && !args[0].canConvert<JsonArray>()) || !JsonUtils::isNumberArray(args, 1, 2) || !JsonUtils::isStringArray(args, 3, 6))
        return false;

    if (!JsonUtils::tryParse(args[0], card_ids))
        return false;

    from_place = (Place)args[1].toInt();
    to_place = (Place)args[2].toInt();
    from_player_name = args[3].toString();
    to_player_name = args[4].toString();
    from_pile_name = args[5].toString();
    to_pile_name = args[6].toString();
    ExtendCardMoveReason::tryParse(reason, args[7]);
    return true;
}

QVariant LegacyCardsMoveStruct::toVariant() const
{
    //notify Client
    JsonArray arg;
    if (open) {
        arg << JsonUtils::toJsonArray(card_ids);
    } else {
        QList<int> notify_ids;
        //keep original order?
        foreach (int id, card_ids) {
            if (shown_ids.contains(id))
                notify_ids << id;
            else
                notify_ids.append(Card::S_UNKNOWN_CARD_ID);
        }
        arg << JsonUtils::toJsonArray(notify_ids);
    }

    arg << (int)from_place;
    arg << (int)to_place;
    arg << from_player_name;
    arg << to_player_name;
    arg << from_pile_name;
    arg << to_pile_name;
    arg << ExtendCardMoveReason::toVariant(reason);
    return arg;
}

bool LegacyCardsMoveStruct::isRelevant(const Player *player) const
{
    return player != nullptr && (from == player || (to == player && to_place != PlaceSpecial));
}

bool ExtendCardUseStruct::isValid(const CardUseStruct &use, const QString &pattern)
{
    Q_UNUSED(pattern)
    return use.card != nullptr;
}

bool ExtendCardUseStruct::tryParse(CardUseStruct &use, const QVariant &usage, RoomObject *room)
{
    JsonArray arr = usage.value<JsonArray>();
    if (arr.length() < 2 || !JsonUtils::isString(arr.first()) || (!arr.value(1).canConvert<JsonArray>() && !JsonUtils::isString(arr.value(1))))
        return false;

    use.card = Card::Parse(arr.first().toString(), room);
    if (arr.value(1).canConvert<JsonArray>()) {
        JsonArray targets = arr.value(1).value<JsonArray>();

        for (int i = 0; i < targets.size(); i++) {
            if (!JsonUtils::isString(targets.value(i)))
                return false;
            use.to << room->findChild<Player *>(targets.value(i).toString());
        }
    } else if (JsonUtils::isString(arr.value(1))) {
        // todo: parse card
    }
    return true;
}

QString ExtendCardUseStruct::toString(const CardUseStruct &use)
{
    if (use.card == nullptr)
        return QString();

    QStringList l;
    l << use.card->toString();

    if (use.toCard != nullptr) {
        l << use.toCard->toString();
    } else {
        if (use.to.isEmpty())
            l << QStringLiteral(".");
        else {
            QStringList tos;
            foreach (Player *p, use.to)
                tos << p->objectName();

            l << tos.join(QStringLiteral("+"));
        }
    }
    return l.join(QStringLiteral("->"));
}

bool ExtendCardMoveReason::tryParse(CardMoveReason &reason, const QVariant &arg)
{
    JsonArray args = arg.value<JsonArray>();
    if (args.size() != 5 || !args[0].canConvert<int>() || !JsonUtils::isStringArray(args, 1, 4))
        return false;

    reason.m_reason = static_cast<MoveReasonCategory>(args[0].toInt());
    reason.m_playerId = args[1].toString();
    reason.m_skillName = args[2].toString();
    reason.m_eventName = args[3].toString();
    reason.m_targetId = args[4].toString();

    return true;
}

QVariant ExtendCardMoveReason::toVariant(const CardMoveReason &reason)
{
    JsonArray result;
    result << static_cast<int>(reason.m_reason);
    result << reason.m_playerId;
    result << reason.m_skillName;
    result << reason.m_eventName;
    result << reason.m_targetId;
    return result;
}

QVariant ExtendTriggerDetail::toVariant(const TriggerDetail &detail)
{
    if (!detail.isValid())
        return QVariant();

    JsonObject ob;
    if (detail.trigger() != nullptr)
        ob[QStringLiteral("skill")] = detail.trigger()->name();
    if (detail.owner() != nullptr)
        ob[QStringLiteral("owner")] = detail.owner()->objectName();
    if (detail.invoker() != nullptr)
        ob[QStringLiteral("invoker")] = detail.invoker()->objectName();
    if (detail.targets().length() == 1) {
        Player *preferredTarget = detail.targets().first();
        ob[QStringLiteral("preferredtarget")] = preferredTarget->objectName();

        Player *current = detail.room()->current();
        if (current == nullptr)
            current = preferredTarget;

        // send the seat info to the client so that we can compare the trigger order of tieqi-like skill in the client side
        int seat = preferredTarget->seat() - current->seat();
        if (seat < 0)
            seat += detail.room()->players().length();

        ob[QStringLiteral("preferredtargetseat")] = seat;
    }

    return ob;
}

QStringList ExtendTriggerDetail::toList(const TriggerDetail &detail)
{
    QStringList l;
    if (!detail.isValid())
        l << QString() << QString() << QString();
    else {
        std::function<void(const QObject *)> insert = [&l](const QObject *item) {
            if (item != nullptr)
                l << item->objectName();
            else
                l << QString();
        };

        if (detail.trigger() != nullptr)
            l << detail.trigger()->name();
        else
            l << QString();
        insert(detail.owner());
        insert(detail.invoker());
    }

    return l;
}
