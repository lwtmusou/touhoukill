#include "structs.h"
#include "RoomObject.h"
#include "card.h"
#include "exppattern.h"
#include "global.h"
#include "json.h"
#include "player.h"
#include "protocol.h"
#include "skill.h"
#include "trigger.h"
#include "util.h"

#include <functional>

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
    reason.tryParse(args[7]);
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
    arg << reason.toVariant();
    return arg;
}

bool LegacyCardsMoveStruct::isRelevant(const Player *player) const
{
    return player != nullptr && (from == player || (to == player && to_place != PlaceSpecial));
}

CardMoveReason::CardMoveReason(QSanguosha::MoveReasonCategory moveReason, const QString &playerId, const QString &skillName, const QString &eventName)
    : m_reason(moveReason)
    , m_playerId(playerId)
    , m_skillName(skillName)
    , m_eventName(eventName)
{
}

CardMoveReason::CardMoveReason(QSanguosha::MoveReasonCategory moveReason, const QString &playerId, const QString &targetId, const QString &skillName, const QString &eventName)
    : m_reason(moveReason)
    , m_playerId(playerId)
    , m_targetId(targetId)
    , m_skillName(skillName)
    , m_eventName(eventName)
{
}

bool CardMoveReason::tryParse(const QVariant &arg)
{
    JsonArray args = arg.value<JsonArray>();
    if (args.size() != 5 || !args[0].canConvert<int>() || !JsonUtils::isStringArray(args, 1, 4))
        return false;

    m_reason = static_cast<MoveReasonCategory>(args[0].toInt());
    m_playerId = args[1].toString();
    m_skillName = args[2].toString();
    m_eventName = args[3].toString();
    m_targetId = args[4].toString();

    return true;
}

QVariant CardMoveReason::toVariant() const
{
    JsonArray result;
    result << static_cast<int>(m_reason);
    result << m_playerId;
    result << m_skillName;
    result << m_eventName;
    result << m_targetId;
    return result;
}

DamageStruct::DamageStruct(const Card *card, Player *from, Player *to, int damage, DamageNature nature)
    : from(from)
    , to(to)
    , card(card)
    , damage(damage)
    , nature(nature)
    , chain(false)
    , transfer(false)
    , by_user(true)
    , trigger_chain(false)
{
}

DamageStruct::DamageStruct(const QString &reason, Player *from, Player *to, int damage, DamageNature nature)
    : from(from)
    , to(to)
    , card(nullptr)
    , damage(damage)
    , nature(nature)
    , chain(false)
    , transfer(false)
    , by_user(true)
    , reason(reason)
    , trigger_chain(false)
{
}

CardEffectStruct::CardEffectStruct(const Card *card, Player *from, Player *to)
    : card(card)
    , from(from)
    , to(to)
    , multiple(false)
    , nullified(false)
    , canceled(false)
    , effectValue({0, 0})
{
}

JudgeStruct::JudgeStruct(Player *who, const QString &pattern, const QString &reason)
    : who(who)
    , pattern(pattern)
    , reason(reason)
    , m_card(nullptr)
    , time_consuming(false)
    , negative(false)
    , play_animation(true)
    , ignore_judge(false)
    , retrial_by_response(nullptr)
    , relative_player(nullptr)
{
}

bool JudgeStruct::isPatternMatch() const
{
    return ExpPattern(pattern).match(who, m_card);
}

PhaseChangeStruct::PhaseChangeStruct(Player *player, Phase from, Phase to)
    : player(player)
    , from(from)
    , to(to)
{
}

CardUseStruct::CardUseStruct(const Card *card, Player *from, const QList<Player *> &to, bool isOwnerUse)
    : card(card)
    , from(from)
    , to(to)
    , toCard(nullptr)
    , m_isOwnerUse(isOwnerUse)
    , m_addHistory(true)
    , m_isHandcard(false)
    , m_isLastHandcard(false)
    , m_reason(CardUseReasonUnknown)
{
}

CardUseStruct::CardUseStruct(const Card *card, Player *from, Player *target, bool isOwnerUse)
    : card(card)
    , from(from)
    , toCard(nullptr)
    , m_isOwnerUse(isOwnerUse)
    , m_addHistory(true)
    , m_isHandcard(false)
    , m_isLastHandcard(false)
    , m_reason(CardUseReasonUnknown)
{
    if (target != nullptr)
        to << target;
}

CardUseStruct::CardUseStruct(const Card *card, Player *from, const Card *toCard, bool isOwnerUse)
    : card(card)
    , from(from)
    , toCard(toCard)
    , m_isOwnerUse(isOwnerUse)
    , m_addHistory(true)
    , m_isHandcard(false)
    , m_isLastHandcard(false)
    , m_reason(CardUseReasonUnknown)
{
}

bool CardUseStruct::isValid(const QString &pattern) const
{
    Q_UNUSED(pattern)
    return card != nullptr;
}

bool CardUseStruct::tryParse(const QVariant &usage, RoomObject *room)
{
    JsonArray arr = usage.value<JsonArray>();
    if (arr.length() < 2 || !JsonUtils::isString(arr.first()) || (!arr.value(1).canConvert<JsonArray>() && !JsonUtils::isString(arr.value(1))))
        return false;

    card = Card::Parse(arr.first().toString(), room);
    if (arr.value(1).canConvert<JsonArray>()) {
        JsonArray targets = arr.value(1).value<JsonArray>();

        for (int i = 0; i < targets.size(); i++) {
            if (!JsonUtils::isString(targets.value(i)))
                return false;
            to << room->findChild<Player *>(targets.value(i).toString());
        }
    } else if (JsonUtils::isString(arr.value(1))) {
        // parse card
    }
    return true;
}

void CardUseStruct::parse(const QString &str, RoomObject *room)
{
    QStringList words = str.split(QStringLiteral("->"), Qt::KeepEmptyParts);
    Q_ASSERT(words.length() == 1 || words.length() == 2);

    QString card_str = words.at(0);
    QString target_str = QStringLiteral(".");

    if (words.length() == 2 && !words.at(1).isEmpty())
        target_str = words.at(1);

    card = Card::Parse(card_str, room);

    if (target_str != QStringLiteral(".")) {
        // todo: parse card
        QStringList target_names = target_str.split(QStringLiteral("+"));
        foreach (QString target_name, target_names)
            to << room->findChild<Player *>(target_name);
    }
}

QString CardUseStruct::toString() const
{
    if (card == nullptr)
        return QString();

    QStringList l;
    l << card->toString();

    if (toCard != nullptr) {
        l << toCard->toString();
    } else {
        if (to.isEmpty())
            l << QStringLiteral(".");
        else {
            QStringList tos;
            foreach (Player *p, to)
                tos << p->objectName();

            l << tos.join(QStringLiteral("+"));
        }
    }
    return l.join(QStringLiteral("->"));
}

class TriggerDetailSharedData : public QSharedData
{
public:
    RoomObject *room;
    const Trigger *trigger; // the trigger
    QString name; // the name of trigger, either "Rule-XXX" or skill name
    Player *owner; // skill owner. 2 structs with the same skill and skill owner are treated as of a same skill.
    Player *invoker; // skill invoker. When invoking skill, we sort firstly according to the priority, then the seat of invoker, at last weather it is a skill of an equip.
    QList<Player *> targets; // skill targets.
    bool isCompulsory; // judge the skill is compulsory or not. It is set in the skill's triggerable
    bool triggered; // judge whether the skill is triggere
    bool effectOnly;
    QVariantMap tag; // used to add a tag to the struct. useful for skills like Tieqi and Liegong to save a QVariantList for assisting to assign targets

    TriggerDetailSharedData()
        : room(nullptr)
        , trigger(nullptr)
        , owner(nullptr)
        , invoker(nullptr)
        , isCompulsory(false)
        , triggered(false)
        , effectOnly(false)
    {
    }
};

class TriggerDetailPrivate
{
public:
    QSharedDataPointer<TriggerDetailSharedData> d;

    TriggerDetailPrivate() = default;
    TriggerDetailPrivate(const TriggerDetailPrivate &) = default;
    TriggerDetailPrivate &operator=(const TriggerDetailPrivate &) = default;
};

bool TriggerDetail::operator<(const TriggerDetail &arg2) const // the operator < for sorting the invoke order.
{
    //  we sort firstly according to the priority, then the seat of invoker, at last whether it is a skill of an equip.
    if (!isValid() || !arg2.isValid())
        return false;

    if (trigger()->priority() > arg2.trigger()->priority())
        return true;
    else if (trigger()->priority() < arg2.trigger()->priority())
        return false;

    if (invoker() != nullptr && arg2.invoker() != nullptr && invoker() != arg2.invoker())
        return room()->comparePlayerByActionOrder(invoker(), arg2.invoker());

    return !trigger()->isEquipSkill() && arg2.trigger()->isEquipSkill();
}

bool TriggerDetail::sameTrigger(const TriggerDetail &arg2) const
{
    // it only judge the skill name, the skill invoker and the skill owner. It don't judge the skill target because it is chosen by the skill invoker
    return trigger() == arg2.trigger() && owner() == arg2.owner() && invoker() == arg2.invoker();
}

bool TriggerDetail::sameTimingWith(const TriggerDetail &arg2) const
{
    // used to judge 2 skills has the same timing. only 2 structs with the same priority and the same invoker and the same "whether or not it is a skill of equip"
    if (!isValid() || !arg2.isValid())
        return false;

    return trigger()->priority() == arg2.trigger()->priority() && invoker() == arg2.invoker() && trigger()->isEquipSkill() == arg2.trigger()->isEquipSkill();
}

TriggerDetail::TriggerDetail(RoomObject *room, const Trigger *trigger /*= NULL*/, const QString &name, Player *owner /*= NULL*/, Player *invoker /*= NULL*/,
                             const QList<Player *> &targets /*= QList<Player *>()*/, bool isCompulsory /*= false*/, bool effectOnly /*=false*/)
    : d(new TriggerDetailPrivate)
{
    d->d->room = room;
    d->d->trigger = trigger;
    d->d->name = name;
    d->d->owner = owner;
    d->d->invoker = invoker;
    d->d->targets = targets;
    d->d->isCompulsory = isCompulsory;
    d->d->triggered = false;
    d->d->effectOnly = effectOnly;
}

TriggerDetail::TriggerDetail(RoomObject *room, const Trigger *trigger, const QString &name, Player *owner, Player *invoker, Player *target, bool isCompulsory /*= false*/,
                             bool effectOnly /*=false*/)
    : d(new TriggerDetailPrivate)
{
    d->d->room = room;
    d->d->trigger = trigger;
    d->d->name = name;
    d->d->owner = owner;
    d->d->invoker = invoker;
    if (target != nullptr)
        d->d->targets << target;
    d->d->isCompulsory = isCompulsory;
    d->d->triggered = false;
    d->d->effectOnly = effectOnly;
}

TriggerDetail::TriggerDetail(const TriggerDetail &other)
    : d(new TriggerDetailPrivate(*other.d))
{
}

TriggerDetail &TriggerDetail::operator=(const TriggerDetail &other)
{
    if (&other == this)
        return *this;

    delete d;
    d = new TriggerDetailPrivate(*other.d);
    return *this;
}

TriggerDetail::~TriggerDetail()
{
    delete d;
}

RoomObject *TriggerDetail::room() const
{
    return d->d->room;
}

const Trigger *TriggerDetail::trigger() const
{
    return d->d->trigger;
}

const QString &TriggerDetail::name() const
{
    return d->d->name;
}

Player *TriggerDetail::owner() const
{
    return d->d->owner;
}

Player *TriggerDetail::invoker() const
{
    return d->d->invoker;
}

QList<Player *> TriggerDetail::targets() const
{
    return d->d->targets;
}

bool TriggerDetail::isCompulsory() const
{
    return d->d->isCompulsory;
}

bool TriggerDetail::triggered() const
{
    return d->d->triggered;
}

bool TriggerDetail::effectOnly() const
{
    return d->d->effectOnly;
}

const QVariantMap &TriggerDetail::tag() const
{
    return d->d->tag;
}

void TriggerDetail::addTarget(Player *target)
{
    d->d->targets << target;
}

void TriggerDetail::setTriggered(bool t)
{
    d->d->triggered = t;
}

QVariantMap &TriggerDetail::tag()
{
    return d->d->tag;
}

bool TriggerDetail::isValid() const // validity check
{
    return room() != nullptr && trigger() != nullptr;
}

QVariant TriggerDetail::toVariant() const
{
    if (!isValid())
        return QVariant();

    JsonObject ob;
    if (trigger() != nullptr)
        ob[QStringLiteral("skill")] = d->d->name;
    if (owner() != nullptr)
        ob[QStringLiteral("owner")] = owner()->objectName();
    if (invoker() != nullptr)
        ob[QStringLiteral("invoker")] = invoker()->objectName();

    return ob;
}

QStringList TriggerDetail::toList() const
{
    QStringList l;
    if (!isValid())
        l << QString() << QString() << QString();
    else {
        std::function<void(const QObject *)> insert = [&l](const QObject *item) {
            if (item != nullptr)
                l << item->objectName();
            else
                l << QString();
        };

        if (trigger() != nullptr)
            l << d->d->name;
        else
            l << QString();
        insert(owner());
        insert(invoker());
    }

    return l;
}

SingleCardMoveStruct::SingleCardMoveStruct(int id, Player *to, QSanguosha::Place toPlace)
    : card_id(id)
    , broken(false)
    , shown(false)
    , open(false)
    , from(nullptr)
    , fromPlace(QSanguosha::PlaceUnknown)
    , to(to)
    , toPlace(toPlace)
{
}

SingleCardMoveStruct::SingleCardMoveStruct(int id, Player *from, Player *to, QSanguosha::Place fromPlace, QSanguosha::Place toPlace)
    : card_id(id)
    , broken(false)
    , shown(false)
    , open(false)
    , from(from)
    , fromPlace(fromPlace)
    , to(to)
    , toPlace(toPlace)
{
}

DeathStruct::DeathStruct(Player *who, DamageStruct *damage)
    : who(who)
    , damage(damage)
    , nowAskingForPeaches(nullptr)
    , viewAsKiller(nullptr)
    , useViewAsKiller(false)
{
}

RecoverStruct::RecoverStruct(const Card *card, Player *from, Player *to, int recover)
    : recover(recover)
    , from(from)
    , to(to)
    , card(card)
{
}

RecoverStruct::RecoverStruct(const QString &reason, Player *from, Player *to, int recover)
    : recover(recover)
    , from(from)
    , to(to)
    , card(nullptr)
    , reason(reason)
{
}

PindianStruct::PindianStruct(Player *from, Player *to)
    : from(from)
    , to(to)
    , from_card(nullptr)
    , to_card(nullptr)
    , from_number(QSanguosha::NumberNA)
    , to_number(QSanguosha::NumberNA)
    , success(false)
{
}

PhaseSkippingStruct::PhaseSkippingStruct(Player *player, QSanguosha::Phase phase, bool isCost)
    : player(player)
    , phase(phase)
    , isCost(false)
{
}

CardResponseStruct::CardResponseStruct(const Card *card, Player *from, bool isRetrial, bool isProvision, Player *to)
    : m_card(card)
    , from(from)
    , m_isRetrial(isRetrial)
    , m_isProvision(isProvision)
    , to(to)
    , m_isHandcard(false)
    , m_isNullified(false)
    , m_isShowncard(false)
{
}

MarkChangeStruct::MarkChangeStruct(Player *player, const QString &name, int num)
    : player(player)
    , name(name)
    , num(num)
{
}

SkillAcquireDetachStruct::SkillAcquireDetachStruct(Player *player, const Skill *skill, bool isAcquire)
    : player(player)
    , skill(skill)
    , isAcquire(isAcquire)
{
}

CardAskedStruct::CardAskedStruct(Player *player, const QString &pattern, const QString &prompt, QSanguosha::HandlingMethod method)
    : player(player)
    , pattern(pattern)
    , prompt(prompt)
    , method(method)
{
}

HpLostStruct::HpLostStruct(Player *player, int num)
    : player(player)
    , num(num)
{
}

DrawNCardsStruct::DrawNCardsStruct(Player *player, int n, bool isInitial)
    : player(player)
    , n(n)
    , isInitial(isInitial)
{
}

SkillInvalidStruct::SkillInvalidStruct(Player *player, const Skill *skill, bool invalid)
    : player(player)
    , skill(skill)
    , invalid(invalid)
{
}

BrokenEquipChangedStruct::BrokenEquipChangedStruct(Player *player, QList<int> ids, bool broken, bool moveFromEquip)
    : player(player)
    , ids(ids)
    , broken(broken)
    , moveFromEquip(moveFromEquip)
{
}

ShownCardChangedStruct::ShownCardChangedStruct(Player *player, QList<int> ids, bool shown, bool moveFromHand)
    : player(player)
    , ids(ids)
    , shown(shown)
    , moveFromHand(moveFromHand)
{
}

ShowGeneralStruct::ShowGeneralStruct(Player *player, int pos, bool isShow)
    : player(player)
    , pos(pos)
    , isShow(isShow)
{
}

ExtraTurnStruct::ExtraTurnStruct(Player *player)
    : player(player)
{
}
