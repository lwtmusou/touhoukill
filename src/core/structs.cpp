#include "structs.h"
#include "RoomObject.h"
#include "card.h"
#include "exppattern.h"
#include "global.h"
#include "player.h"
#include "protocol.h"
#include "skill.h"
#include "trigger.h"
#include "util.h"

#include <functional>

using namespace QSanguosha;

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

class TriggerDetailSharedData : public QSharedData
{
public:
    RoomObject *room;
    const Trigger *trigger; // the trigger
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

bool TriggerDetail::operator==(const TriggerDetail &arg2) const
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

TriggerDetail::TriggerDetail(RoomObject *room, const Trigger *trigger /*= NULL*/, Player *owner /*= NULL*/, Player *invoker /*= NULL*/,
                             const QList<Player *> &targets /*= QList<Player *>()*/, bool isCompulsory /*= false*/, bool effectOnly /*=false*/)
    : d(new TriggerDetailPrivate)
{
    d->d->room = room;
    d->d->trigger = trigger;
    d->d->owner = owner;
    d->d->invoker = invoker;
    d->d->targets = targets;
    d->d->isCompulsory = isCompulsory;
    d->d->triggered = false;
    d->d->effectOnly = effectOnly;
}

TriggerDetail::TriggerDetail(RoomObject *room, const Trigger *trigger, Player *owner, Player *invoker, Player *target, bool isCompulsory /*= false*/, bool effectOnly /*=false*/)
    : d(new TriggerDetailPrivate)
{
    d->d->room = room;
    d->d->trigger = trigger;
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

QString TriggerDetail::name() const
{
    if (d->d->trigger != nullptr)
        return d->d->trigger->name();

    return QString();
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
