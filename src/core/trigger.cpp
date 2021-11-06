#include "trigger.h"
#include "CardFace.h"
#include "RoomObject.h"
#include "card.h"
#include "player.h"
#include "util.h"

// TODO: KILL THIS
#include "serverplayer.h"

using namespace QSanguosha;

class TriggerPrivate
{
public:
    TriggerEvents e;
    bool global;

    TriggerPrivate()
        : global(false)
    {
    }
};

Trigger::Trigger()
    : d(new TriggerPrivate)
{
}

Trigger::~Trigger()
{
    delete d;
}

TriggerEvents Trigger::triggerEvents() const
{
    if (d->e.contains(NumOfEvents))
        return TriggerEvents() << NumOfEvents;

    return d->e;
}

bool Trigger::canTrigger(TriggerEvent e) const
{
    Q_ASSERT(e != NumOfEvents);

    return d->e.contains(NumOfEvents) || d->e.contains(e);
}

void Trigger::addTriggerEvent(TriggerEvent e)
{
    d->e.insert(e);
}

void Trigger::addTriggerEvents(const TriggerEvents &e)
{
    d->e.unite(e);
}

bool Trigger::isGlobal() const
{
    return d->global;
}

void Trigger::setGlobal(bool global)
{
    d->global = global;
}

void Trigger::record(TriggerEvent /*unused*/, RoomObject * /*unused*/, QVariant & /*unused*/) const
{
    // Intentionally empty
}

bool Trigger::trigger(TriggerEvent /*unused*/, RoomObject * /*unused*/, const TriggerDetail & /*unused*/, QVariant & /*unused*/) const
{
    return false;
}

Rule::Rule()
    : Trigger()
{
}

int Rule::priority() const
{
    // for rule
    return 0;
}

QList<TriggerDetail> Rule::triggerable(TriggerEvent /*event*/, RoomObject *room, const QVariant & /*data*/) const
{
    TriggerDetail d(room, this);
    return QList<TriggerDetail>() << d;
}

class SkillTriggerPrivate
{
public:
    QString name;
};

SkillTrigger::SkillTrigger(const QString &name)
    : d(new SkillTriggerPrivate)
{
    d->name = name;
}

SkillTrigger::~SkillTrigger()
{
    delete d;
}

const QString &SkillTrigger::skillName() const
{
    return d->name;
}

int SkillTrigger::priority() const
{
    // for regular skill
    return 2;
}

bool SkillTrigger::trigger(TriggerEvent event, RoomObject *room, const TriggerDetail &_detail, QVariant &data) const
{
    TriggerDetail detail = _detail;
    if (!detail.effectOnly()) {
        if (!cost(event, room, detail, data))
            return false;

        if (detail.owner()->hasValidSkill(d->name) && !detail.owner()->haveShownSkill(d->name))
            RefactorProposal::fixme_cast<ServerPlayer *>(detail.owner())->showHiddenSkill(d->name);
    }

    return effect(event, room, detail, data);
}

bool SkillTrigger::cost(TriggerEvent /*unused*/, RoomObject * /*unused*/, TriggerDetail &detail, QVariant & /*unused*/) const
{
    if ((detail.owner() == nullptr) || (detail.owner() != detail.invoker()) || (detail.invoker() == nullptr))
        return true;

    // detail.owner == detail.invoker
    bool isCompulsory = detail.isCompulsory() && (detail.invoker()->hasValidSkill(d->name) && !detail.invoker()->haveShownSkill(d->name));
    bool invoke = true;
    if (!isCompulsory)
        invoke = RefactorProposal::fixme_cast<ServerPlayer *>(detail.invoker())->askForSkillInvoke(d->name);

    return invoke;
}

EquipSkillTrigger::EquipSkillTrigger(const QString &name)
    : SkillTrigger(name)
{
}

bool EquipSkillTrigger::equipAvailable(const Player *p, EquipLocation location, const QString &equipName, const Player *to)
{
    if (p == nullptr)
        return false;

    if (p->mark(QStringLiteral("Equips_Nullified_to_Yourself")) > 0)
        return false;

    // for StarSP Pangtong? It needs investigation for real 'Armor ignored by someone' effect
    // But 'Armor ignored by someone' is too complicated while its effect has just few differences compared to 'Armor invalid'
    // So we just use 'Armor invalid' everywhere
    // I prefer removing 'to' from this function and use regular QinggangSword method or a simular one for StarSP Pangtong
    if (to != nullptr && to->mark(QStringLiteral("Equips_of_Others_Nullified_to_You")) > 0)
        return false;

    switch (location) {
    case WeaponLocation:
        if (!p->hasValidWeapon(equipName))
            return false;
        break;
    case ArmorLocation:
        if (!p->hasValidArmor(equipName))
            return false;
        break;
    case TreasureLocation:
        if (!p->hasValidTreasure(equipName))
            return false;
        break;
    default:
        break; // shenmegui?
    }

    return true;
}

bool EquipSkillTrigger::equipAvailable(const Player *p, const Card *equip, const Player *to /*= NULL*/)
{
    if (equip == nullptr)
        return false;

    const EquipCard *face = dynamic_cast<const EquipCard *>(equip->face());
    Q_ASSERT(face != nullptr);

    return equipAvailable(p, face->location(), equip->faceName(), to);
}

int EquipSkillTrigger::priority() const
{
    // for EquipSkill
    return 2;
}

GlobalRecord::GlobalRecord()
    : Trigger()
{
}

int GlobalRecord::priority() const
{
    return 10;
}

QList<TriggerDetail> GlobalRecord::triggerable(TriggerEvent /*event*/, RoomObject * /*room*/, const QVariant & /*data*/) const
{
    return QList<TriggerDetail>();
}

class FakeMoveRecordPrivate
{
public:
    QString skillName;
};

FakeMoveRecord::FakeMoveRecord(const QString &skillName)
    : GlobalRecord()
    , d(new FakeMoveRecordPrivate)
{
    addTriggerEvents({BeforeCardsMove, CardsMoveOneTime});
    d->skillName = skillName;
}

FakeMoveRecord::~FakeMoveRecord()
{
    delete d;
}

QList<TriggerDetail> FakeMoveRecord::triggerable(TriggerEvent /*event*/, RoomObject *room, const QVariant & /*data*/) const
{
    Player *owner = nullptr;
    foreach (Player *p, room->players(false)) {
        if (p->hasValidSkill(d->skillName)) {
            owner = p;
            break;
        }
    }

    QString flag = QString(QStringLiteral("%1_InTempMoving")).arg(d->skillName);
    foreach (Player *p, room->players(false)) {
        if (p->hasFlag(flag))
            return {TriggerDetail(room, this, d->skillName, owner, p, nullptr)};
    }

    return {};
}

bool FakeMoveRecord::trigger(TriggerEvent /*event*/, RoomObject * /*room*/, const TriggerDetail & /*detail*/, QVariant & /*data*/) const
{
    return true;
}
