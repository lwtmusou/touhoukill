#include "structs.h"
#include "RoomObject.h"
#include "card.h"
#include "exppattern.h"
#include "global.h"
#include "jsonutils.h"
#include "player.h"
#include "protocol.h"
#include "skill.h"
#include "trigger.h"
#include "util.h"

#include <QJsonArray>
#include <QJsonObject>

#include <functional>

using namespace QSanguosha;

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
    , toCardEffect(nullptr)
    , multiple(false)
    , nullified(false)
    , canceled(false)
    , effectValue {0, 0}
{
}

CardEffectStruct::CardEffectStruct(const Card *card, Player *from, CardEffectStruct *toCardEffect)
    : card(card)
    , from(from)
    , to(nullptr)
    , toCardEffect(toCardEffect)
    , multiple(false)
    , nullified(false)
    , canceled(false)
    , effectValue {0, 0}
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
    , toCardEffect(nullptr)
    , m_isOwnerUse(isOwnerUse)
    , m_addHistory(true)
    , m_isHandcard(false)
    , m_isLastHandcard(false)
    , m_reason(CardUseReasonUnknown)
    , effectValue {0, 0}
{
}

CardUseStruct::CardUseStruct(const Card *card, Player *from, Player *target, bool isOwnerUse)
    : card(card)
    , from(from)
    , toCardEffect(nullptr)
    , m_isOwnerUse(isOwnerUse)
    , m_addHistory(true)
    , m_isHandcard(false)
    , m_isLastHandcard(false)
    , m_reason(CardUseReasonUnknown)
    , effectValue {0, 0}
{
    if (target != nullptr)
        to << target;
}

CardUseStruct::CardUseStruct(const Card *card, Player *from, CardEffectStruct *toCardEffect, bool isOwnerUse)
    : card(card)
    , from(from)
    , toCardEffect(toCardEffect)
    , m_isOwnerUse(isOwnerUse)
    , m_addHistory(true)
    , m_isHandcard(false)
    , m_isLastHandcard(false)
    , m_reason(CardUseReasonUnknown)
    , effectValue {0, 0}
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
    bool triggered; // judge whether the skill is triggered
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

SingleCardMoveStruct::SingleCardMoveStruct(int id)
    : card_id(id)
    , brokenEquipBeforeMove(false)
    , shownHandcardBeforeMove(false)
    , visibleToAll(false)
    , from(nullptr)
    , fromPlace(QSanguosha::PlaceUnknown)
    , isFromLastHandcard(false)
    , to(nullptr)
    , toPlace(QSanguosha::PlaceDiscardPile)
{
}
SingleCardMoveStruct::SingleCardMoveStruct(int id, Player *to, QSanguosha::Place toPlace)
    : card_id(id)
    , brokenEquipBeforeMove(false)
    , shownHandcardBeforeMove(false)
    , visibleToAll(false)
    , from(nullptr)
    , fromPlace(QSanguosha::PlaceUnknown)
    , isFromLastHandcard(false)
    , to(to)
    , toPlace(toPlace)
{
}

SingleCardMoveStruct::SingleCardMoveStruct(int id, Player *from, Player *to, QSanguosha::Place fromPlace, QSanguosha::Place toPlace)
    : card_id(id)
    , brokenEquipBeforeMove(false)
    , shownHandcardBeforeMove(false)
    , visibleToAll(false)
    , from(from)
    , fromPlace(fromPlace)
    , isFromLastHandcard(false)
    , to(to)
    , toPlace(toPlace)
{
}

QJsonValue SingleCardMoveStruct::serializeLegacy(bool visible, QSanguosha::MoveReasonCategory reason, Player *causedBy, Player *aimFor, const QString &via) const
{
    QJsonArray arr;
    QJsonArray cardArr;
    cardArr.append((visible || visibleToAll) ? card_id : -1);
    arr.append(cardArr);
    arr.append((from == nullptr) ? QString() : from->objectName());
    arr.append(static_cast<int>(fromPlace));
    arr.append(fromPile);
    arr.append((to == nullptr) ? QString() : to->objectName());
    arr.append(static_cast<int>(toPlace));
    arr.append(toPile);
    QJsonArray reasonArr;
    reasonArr.append(static_cast<int>(reason));
    reasonArr.append((causedBy == nullptr) ? QString() : causedBy->objectName());
    reasonArr.append(via);
    reasonArr.append(via);
    reasonArr.append((aimFor == nullptr) ? QString() : aimFor->objectName());
    arr << reasonArr;
    return arr;
}

QJsonValue SingleCardMoveStruct::serialize(bool visible) const
{
    QJsonArray arr;
    arr.append((visible || visibleToAll) ? card_id : -1);
    arr.append((from == nullptr) ? QString() : from->objectName());
    arr.append(static_cast<int>(fromPlace));
    arr.append(fromPile);
    arr.append((to == nullptr) ? QString() : to->objectName());
    arr.append(static_cast<int>(toPlace));
    arr.append(toPile);
    return arr;
}

QJsonValue _qsgs_CardsMoveStructSerializeLegacyImpl(const CardsMoveStruct &moves, QList<bool> visibles)
{
    QJsonArray arr;
    arr.append(0);

    for (int i = 0; i < moves.length(); ++i) {
        const SingleCardMoveStruct &move = moves.value(i);
        bool visible = visibles.value(i, false);
        arr.append(move.serializeLegacy(visible, moves.reason, moves.causedBy, moves.aimFor, moves.via));
    }

    return arr;
}

QJsonValue _qsgs_CardsMoveStructSerializeImpl(const CardsMoveStruct &moves, QList<bool> visibles)
{
    QJsonObject ob;

    QJsonArray arr;

    for (int i = 0; i < moves.length(); ++i) {
        const SingleCardMoveStruct &move = moves.value(i);
        bool visible = visibles.value(i, false);
        arr.append(move.serialize(visible));
    }

    ob[QStringLiteral("moves")] = arr;
    ob[QStringLiteral("reason")] = static_cast<int>(moves.reason);
    ob[QStringLiteral("causedBy")] = ((moves.causedBy == nullptr) ? QString() : moves.causedBy->objectName());
    ob[QStringLiteral("aimFor")] = ((moves.aimFor == nullptr) ? QString() : moves.aimFor->objectName());
    ob[QStringLiteral("via")] = moves.via;

    return ob;
}

LogStruct::LogStruct(const QString &type, Player *from, const QList<Player *> &to, const QString &arg, const QString &arg2, const Card *card)
    : type(type)
    , from(from)
    , to(to)
    , arg(arg)
    , arg2(arg2)
{
    if (card != nullptr)
        card_str = card->toString();
}

LogStruct::LogStruct(const QString &type, Player *from, Player *_to, const QString &arg, const QString &arg2, const Card *card)
    : type(type)
    , from(from)
    , arg(arg)
    , arg2(arg2)
{
    if (_to != nullptr)
        to << _to;
    if (card != nullptr)
        card_str = card->toString();
}

QJsonValue LogStruct::serialize() const
{
    QStringList tos;
    foreach (Player *player, to) {
        if (player != nullptr)
            tos << player->objectName();
    }
    QJsonArray log;
    log << type << (from != nullptr ? from->objectName() : QString()) << tos.join(QStringLiteral("+")) << card_str << arg << arg2;
    return log;
}

bool LogStruct::parse(const QJsonValue &value, RoomObject *room)
{
    if (!QSgsJsonUtils::isStringArray(value, 0, 6))
        return false;

    bool ok = false;
    QStringList sl = QSgsJsonUtils::toStringList(value, &ok);
    if (!ok)
        return false;

    type = sl.takeFirst();
    if (type.isEmpty())
        return false;

    QString fromName = sl.takeFirst();
    if (fromName.isEmpty()) {
        from = nullptr;
    } else {
        from = room->findPlayerByObjectName(fromName);
        if (from == nullptr)
            return false;
    }

    to.clear();
    QString toNames = sl.takeFirst();
    if (toNames.isEmpty()) {
        // do nothing
    } else {
        QStringList toNameSplitted = toNames.split(QStringLiteral("+"), Qt::SkipEmptyParts);
        foreach (const QString &toName, toNameSplitted) {
            Player *p = room->findPlayerByObjectName(toName);
            if (p == nullptr)
                return false;
            to << p;
        }
    }

    card_str = sl.takeFirst();
    arg = sl.takeFirst();
    arg2 = sl.takeFirst();
    return true;
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

CardResponseStruct::CardResponseStruct(const Card *card, Player *from, bool isRetrial, bool isProvision)
    : m_card(card)
    , from(from)
    , m_isRetrial(isRetrial)
    , m_isProvision(isProvision)
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
