#include "structs.h"
#include "exppattern.h"
#include "json.h"
#include "protocol.h"
#include "room.h"
#include "skill.h"
#include <functional>

bool CardsMoveStruct::tryParse(const QVariant &arg)
{
    JsonArray args = arg.value<JsonArray>();
    if (args.size() != 8)
        return false;

    if ((!JsonUtils::isNumber(args[0]) && !args[0].canConvert<JsonArray>()) || !JsonUtils::isNumberArray(args, 1, 2) || !JsonUtils::isStringArray(args, 3, 6))
        return false;

    if (!JsonUtils::tryParse(args[0], card_ids))
        return false;

    from_place = (QSanguosha::Place)args[1].toInt();
    to_place = (QSanguosha::Place)args[2].toInt();
    from_player_name = args[3].toString();
    to_player_name = args[4].toString();
    from_pile_name = args[5].toString();
    to_pile_name = args[6].toString();
    reason.tryParse(args[7]);
    return true;
}

QVariant CardsMoveStruct::toVariant() const
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

bool CardMoveReason::tryParse(const QVariant &arg)
{
    JsonArray args = arg.value<JsonArray>();
    if (args.size() != 5 || !args[0].canConvert<int>() || !JsonUtils::isStringArray(args, 1, 4))
        return false;

    m_reason = args[0].toInt();
    m_playerId = args[1].toString();
    m_skillName = args[2].toString();
    m_eventName = args[3].toString();
    m_targetId = args[4].toString();

    return true;
}

QVariant CardMoveReason::toVariant() const
{
    JsonArray result;
    result << m_reason;
    result << m_playerId;
    result << m_skillName;
    result << m_eventName;
    result << m_targetId;
    return result;
}

DamageStruct::DamageStruct()
    : from(nullptr)
    , to(nullptr)
    , card(nullptr)
    , damage(1)
    , nature(Normal)
    , chain(false)
    , transfer(false)
    , by_user(true)
    , reason(QString())
    , trigger_chain(false)
    , trigger_info(QString())
{
}

DamageStruct::DamageStruct(const Card *card, ServerPlayer *from, ServerPlayer *to, int damage, DamageStruct::Nature nature)
    : chain(false)
    , transfer(false)
    , by_user(true)
    , reason(QString())
    , trigger_chain(false)
    , trigger_info(QString())
{
    this->card = card;
    this->from = from;
    this->to = to;
    this->damage = damage;
    this->nature = nature;
}

DamageStruct::DamageStruct(const QString &reason, ServerPlayer *from, ServerPlayer *to, int damage, DamageStruct::Nature nature)
    : card(nullptr)
    , chain(false)
    , transfer(false)
    , by_user(true)
    , trigger_chain(false)
    , trigger_info(QString())
{
    this->from = from;
    this->to = to;
    this->damage = damage;
    this->nature = nature;
    this->reason = reason;
}

QString DamageStruct::getReason() const
{
    if (reason != QString())
        return reason;
    else if (card != nullptr)
        return card->faceName();
    return QString();
}

CardEffectStruct::CardEffectStruct()
    : card(nullptr)
    , from(nullptr)
    , to(nullptr)
    , multiple(false)
    , nullified(false)
    , canceled(false)
    , effectValue(QList<int>() << 0 << 0)
{
}

SlashEffectStruct::SlashEffectStruct()
    : jink_num(1)
    , slash(nullptr)
    , jink(nullptr)
    , from(nullptr)
    , to(nullptr)
    , drank(0)
    , nature(DamageStruct::Normal)
    , multiple(false)
    , nullified(false)
    , canceled(false)
    , effectValue(QList<int>() << 0 << 0)
{
}

DyingStruct::DyingStruct()
    : who(nullptr)
    , damage(nullptr)
    , nowAskingForPeaches(nullptr)
{
}

DeathStruct::DeathStruct()
    : who(nullptr)
    , damage(nullptr)
    , viewAsKiller(nullptr)
    , useViewAsKiller(false)
{
}

RecoverStruct::RecoverStruct()
    : recover(1)
    , who(nullptr)
    , card(nullptr)
{
}

PindianStruct::PindianStruct()
    : from(nullptr)
    , to(nullptr)
    , askedPlayer(nullptr)
    , from_card(nullptr)
    , to_card(nullptr)
    , success(false)
{
}

bool PindianStruct::isSuccess() const
{
    return success;
}

JudgeStruct::JudgeStruct()
    : who(nullptr)
    , card(nullptr)
    , pattern(QStringLiteral("."))
    , good(true)
    , time_consuming(false)
    , negative(false)
    , play_animation(true)
    , retrial_by_response(nullptr)
    , relative_player(nullptr)
    , ignore_judge(false)
    , _m_result(TRIAL_RESULT_UNKNOWN)
{
}

bool JudgeStruct::isEffected() const
{
    return negative ? isBad() : isGood();
}

void JudgeStruct::updateResult()
{
    bool effected = (good == ExpPattern(pattern).match(who, card));
    if (effected)
        _m_result = TRIAL_RESULT_GOOD;
    else
        _m_result = TRIAL_RESULT_BAD;
}

bool JudgeStruct::isGood() const
{
    Q_ASSERT(_m_result != TRIAL_RESULT_UNKNOWN);
    return _m_result == TRIAL_RESULT_GOOD;
}

bool JudgeStruct::isBad() const
{
    return !isGood();
}

bool JudgeStruct::isGood(const Card *card) const
{
    Q_ASSERT(card);
    return (good == ExpPattern(pattern).match(who, card));
}

PhaseChangeStruct::PhaseChangeStruct()
    : from(QSanguosha::PhaseNotActive)
    , to(QSanguosha::PhaseNotActive)
    , player(nullptr)
{
}

CardUseStruct::CardUseStruct()
    : card(nullptr)
    , from(nullptr)
    , m_isOwnerUse(true)
    , m_addHistory(true)
{
}

CardUseStruct::CardUseStruct(const Card *card, ServerPlayer *from, const QList<ServerPlayer *> &to, bool isOwnerUse)
{
    this->card = card;
    this->from = from;
    this->to = to;
    this->m_isOwnerUse = isOwnerUse;
    this->m_addHistory = true;
    this->m_isHandcard = false;
    this->m_isLastHandcard = false;
}

CardUseStruct::CardUseStruct(const Card *card, ServerPlayer *from, ServerPlayer *target, bool isOwnerUse)
{
    this->card = card;
    this->from = from;
    if (target != nullptr)
        to << target;
    this->m_isOwnerUse = isOwnerUse;
    this->m_addHistory = true;
    this->m_isHandcard = false;
    this->m_isLastHandcard = false;
}

bool CardUseStruct::isValid(const QString &pattern) const
{
    Q_UNUSED(pattern)
    return card != nullptr;
}

bool CardUseStruct::tryParse(const QVariant &usage, Room *room)
{
    JsonArray arr = usage.value<JsonArray>();
    if (arr.length() < 2 || !JsonUtils::isString(arr.first()) || !arr.value(1).canConvert<JsonArray>())
        return false;

    card = Card::Parse(arr.first().toString(), room);
    JsonArray targets = arr.value(1).value<JsonArray>();

    for (int i = 0; i < targets.size(); i++) {
        if (!JsonUtils::isString(targets.value(i)))
            return false;
        to << room->findChild<ServerPlayer *>(targets.value(i).toString());
    }
    return true;
}

void CardUseStruct::parse(const QString &str, Room *room)
{
    QStringList words = str.split(QStringLiteral("->"), Qt::KeepEmptyParts);
    Q_ASSERT(words.length() == 1 || words.length() == 2);

    QString card_str = words.at(0);
    QString target_str = QStringLiteral(".");

    if (words.length() == 2 && !words.at(1).isEmpty())
        target_str = words.at(1);

    card = Card::Parse(card_str, room);

    if (target_str != QStringLiteral(".")) {
        QStringList target_names = target_str.split(QStringLiteral("+"));
        foreach (QString target_name, target_names)
            to << room->findChild<ServerPlayer *>(target_name);
    }
}

QString CardUseStruct::toString() const
{
    if (card == nullptr)
        return QString();

    QStringList l;
    l << card->toString();

    if (to.isEmpty())
        l << QStringLiteral(".");
    else {
        QStringList tos;
        foreach (ServerPlayer *p, to)
            tos << p->objectName();

        l << tos.join(QStringLiteral("+"));
    }
    return l.join(QStringLiteral("->"));
}

MarkChangeStruct::MarkChangeStruct()
    : num(1)
    , player(nullptr)
{
}

class TriggerDetailSharedData : public QSharedData
{
public:
    const ::Room *room;
    const Trigger *trigger; // the trigger
    ::ServerPlayer *owner; // skill owner. 2 structs with the same skill and skill owner are treated as of a same skill.
    ::ServerPlayer *invoker; // skill invoker. When invoking skill, we sort firstly according to the priority, then the seat of invoker, at last weather it is a skill of an equip.
    QList< ::ServerPlayer *> targets; // skill targets.
    bool isCompulsory; // judge the skill is compulsory or not. It is set in the skill's triggerable
    bool triggered; // judge whether the skill is triggere
    bool showhidden;
    bool effectOnly;
    QVariantMap tag; // used to add a tag to the struct. useful for skills like Tieqi and Liegong to save a QVariantList for assisting to assign targets

    TriggerDetailSharedData()
        : room(nullptr)
        , trigger(nullptr)
        , owner(nullptr)
        , invoker(nullptr)
        , isCompulsory(false)
        , triggered(false)
        , showhidden(false)
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
        return room()->getFront(invoker(), arg2.invoker()) == invoker();

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

TriggerDetail::TriggerDetail(const Room *room, const Trigger *trigger /*= NULL*/, ServerPlayer *owner /*= NULL*/, ServerPlayer *invoker /*= NULL*/,
                             const QList<ServerPlayer *> &targets /*= QList<ServerPlayer *>()*/, bool isCompulsory /*= false*/, bool showHidden)
    : d(new TriggerDetailPrivate)
{
    d->d->room = (room);
    d->d->trigger = (trigger);
    d->d->owner = (owner);
    d->d->invoker = (invoker);
    d->d->targets = (targets);
    d->d->isCompulsory = (isCompulsory);
    d->d->triggered = (false);
    d->d->showhidden = (showHidden);
}

TriggerDetail::TriggerDetail(const Room *room, const Trigger *trigger, ServerPlayer *owner, ServerPlayer *invoker, ServerPlayer *target, bool isCompulsory /*= false*/,
                             bool showHidden)
    : d(new TriggerDetailPrivate)
{
    d->d->room = (room);
    d->d->trigger = (trigger);
    d->d->owner = (owner);
    d->d->invoker = (invoker);
    d->d->isCompulsory = (isCompulsory);
    d->d->triggered = (false);
    d->d->showhidden = (showHidden);

    if (target != nullptr)
        d->d->targets << target;
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

const Room *TriggerDetail::room() const
{
    return d->d->room;
}

const Trigger *TriggerDetail::trigger() const
{
    return d->d->trigger;
}

ServerPlayer *TriggerDetail::owner() const
{
    return d->d->owner;
}

ServerPlayer *TriggerDetail::invoker() const
{
    return d->d->invoker;
}

QList<ServerPlayer *> TriggerDetail::targets() const
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

bool TriggerDetail::showhidden() const
{
    return d->d->showhidden;
}

bool TriggerDetail::effectOnly() const
{
    return d->d->effectOnly;
}

const QVariantMap &TriggerDetail::tag() const
{
    return d->d->tag;
}

void TriggerDetail::addTarget(ServerPlayer *target)
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
        ob[QStringLiteral("skill")] = trigger()->name();
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
            l << trigger()->name();
        else
            l << QString();
        insert(owner());
        insert(invoker());
    }

    return l;
}

SkillAcquireDetachStruct::SkillAcquireDetachStruct()
    : skill(nullptr)
    , player(nullptr)
    , isAcquire(false)
{
}

ChoiceMadeStruct::ChoiceMadeStruct()
    : player(nullptr)
{
}

CardAskedStruct::CardAskedStruct()
    : player(nullptr)
{
}

HpLostStruct::HpLostStruct()
    : player(nullptr)
    , num(0)
{
}

JinkEffectStruct::JinkEffectStruct()
    : jink(nullptr)
{
}

PhaseSkippingStruct::PhaseSkippingStruct()
    : phase(QSanguosha::PhaseNotActive)
    , player(nullptr)
    , isCost(false)
{
}

DrawNCardsStruct::DrawNCardsStruct()
    : player(nullptr)
    , n(0)
    , isInitial(false)
{
}

SkillInvalidStruct::SkillInvalidStruct()
    : player(nullptr)
    , skill(nullptr)
    , invalid(false)
{
}

ExtraTurnStruct::ExtraTurnStruct()
    : player(nullptr)
    , reason(QString())
    , extraTarget(nullptr)
{
}

BrokenEquipChangedStruct::BrokenEquipChangedStruct()
    : player(nullptr)
    , broken(false)
    , moveFromEquip(false)
{
}

ShownCardChangedStruct::ShownCardChangedStruct()
    : player(nullptr)
    , shown(false)
    , moveFromHand(false)
{
}

ShowGeneralStruct::ShowGeneralStruct()
    : player(nullptr)
    , isHead(true)
    , isShow(true)
{
}
