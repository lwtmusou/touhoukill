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

    if (JsonUtils::isNumber(args[0])) {
        int size = args[0].toInt();
        for (int i = 0; i < size; i++)
            card_ids.append(Card::S_UNKNOWN_CARD_ID);
    } else if (!JsonUtils::tryParse(args[0], card_ids)) {
        return false;
    }

    from_place = (Player::Place)args[1].toInt();
    to_place = (Player::Place)args[2].toInt();
    from_player_name = args[3].toString();
    to_player_name = args[4].toString();
    from_pile_name = args[5].toString();
    to_pile_name = args[6].toString();
    reason.tryParse(args[7]);
    return true;
}

QVariant CardsMoveStruct::toVariant() const
{
    JsonArray arg;
    if (open) {
        arg << JsonUtils::toJsonArray(card_ids);
    } else {
        arg << card_ids.size();
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
    : from(NULL)
    , to(NULL)
    , card(NULL)
    , damage(1)
    , nature(Normal)
    , chain(false)
    , transfer(false)
    , by_user(true)
    , reason(QString())
    , trigger_chain(false)
{
}

DamageStruct::DamageStruct(const Card *card, ServerPlayer *from, ServerPlayer *to, int damage, DamageStruct::Nature nature)
    : chain(false)
    , transfer(false)
    , by_user(true)
    , reason(QString())
    , trigger_chain(false)
{
    this->card = card;
    this->from = from;
    this->to = to;
    this->damage = damage;
    this->nature = nature;
}

DamageStruct::DamageStruct(const QString &reason, ServerPlayer *from, ServerPlayer *to, int damage, DamageStruct::Nature nature)
    : card(NULL)
    , chain(false)
    , transfer(false)
    , by_user(true)
    , trigger_chain(false)
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
    else if (card)
        return card->objectName();
    return QString();
}

CardEffectStruct::CardEffectStruct()
    : card(NULL)
    , from(NULL)
    , to(NULL)
    , multiple(false)
    , nullified(false)
    , canceled(false)
{
}

SlashEffectStruct::SlashEffectStruct()
    : jink_num(1)
    , slash(NULL)
    , jink(NULL)
    , from(NULL)
    , to(NULL)
    , drank(0)
    , nature(DamageStruct::Normal)
    , multiple(false)
    , nullified(false)
    , canceled(false)
{
}

DyingStruct::DyingStruct()
    : who(NULL)
    , damage(NULL)
    , nowAskingForPeaches(NULL)
{
}

DeathStruct::DeathStruct()
    : who(NULL)
    , damage(NULL)
    , viewAsKiller(NULL)
    , useViewAsKiller(false)
{
}

RecoverStruct::RecoverStruct()
    : recover(1)
    , who(NULL)
    , card(NULL)
{
}

PindianStruct::PindianStruct()
    : from(NULL)
    , to(NULL)
    , askedPlayer(NULL)
    , from_card(NULL)
    , to_card(NULL)
    , success(false)
{
}

bool PindianStruct::isSuccess() const
{
    return success;
}

JudgeStruct::JudgeStruct()
    : who(NULL)
    , card(NULL)
    , pattern(".")
    , good(true)
    , time_consuming(false)
    , negative(false)
    , play_animation(true)
    , retrial_by_response(NULL)
    , relative_player(NULL)
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
    : from(Player::NotActive)
    , to(Player::NotActive)
    , player(NULL)
{
}

CardUseStruct::CardUseStruct()
    : card(NULL)
    , from(NULL)
    , m_isOwnerUse(true)
    , m_addHistory(true)
    , nullified_list(QStringList())
{
}

CardUseStruct::CardUseStruct(const Card *card, ServerPlayer *from, QList<ServerPlayer *> to, bool isOwnerUse)
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
    if (target != NULL)
        to << target;
    this->m_isOwnerUse = isOwnerUse;
    this->m_addHistory = true;
    this->m_isHandcard = false;
    this->m_isLastHandcard = false;
}

bool CardUseStruct::isValid(const QString &pattern) const
{
    Q_UNUSED(pattern)
    return card != NULL;
    /*if (card == NULL) return false;
    if (!card->getSkillName().isEmpty()) {
    bool validSkill = false;
    QString skillName = card->getSkillName();
    QSet<const Skill *> skills = from->getVisibleSkills();
    for (int i = 0; i < 4; i++) {
    const EquipCard *equip = from->getEquip(i);
    if (equip == NULL) continue;
    const Skill *skill = Sanguosha->getSkill(equip);
    if (skill)
    skills.insert(skill);
    }
    foreach (const Skill *skill, skills) {
    if (skill->objectName() != skillName) continue;
    const ViewAsSkill *vsSkill = ViewAsSkill::parseViewAsSkill(skill);
    if (vsSkill) {
    if (!vsSkill->isAvailable(from, m_reason, pattern))
    return false;
    else {
    validSkill = true;
    break;
    }
    } else if (skill->getFrequency() == Skill::Wake) {
    bool valid = (from->getMark(skill->objectName()) > 0);
    if (!valid)
    return false;
    else
    validSkill = true;
    } else
    return false;
    }
    if (!validSkill) return false;
    }
    if (card->targetFixed())
    return true;
    else {
    QList<const Player *> targets;
    foreach (const ServerPlayer *player, to)
    targets.push_back(player);
    return card->targetsFeasible(targets, from);
    }*/
}

bool CardUseStruct::tryParse(const QVariant &usage, Room *room)
{
    JsonArray arr = usage.value<JsonArray>();
    if (arr.length() < 2 || !JsonUtils::isString(arr.first()) || !arr.value(1).canConvert<JsonArray>())
        return false;

    card = Card::Parse(arr.first().toString());
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
    QStringList words = str.split("->", QString::KeepEmptyParts);
    Q_ASSERT(words.length() == 1 || words.length() == 2);

    QString card_str = words.at(0);
    QString target_str = ".";

    if (words.length() == 2 && !words.at(1).isEmpty())
        target_str = words.at(1);

    card = Card::Parse(card_str);

    if (target_str != ".") {
        QStringList target_names = target_str.split("+");
        foreach (QString target_name, target_names)
            to << room->findChild<ServerPlayer *>(target_name);
    }
}

QString CardUseStruct::toString() const
{
    if (card == NULL)
        return QString();

    QStringList l;
    l << card->toString();

    if (to.isEmpty())
        l << ".";
    else {
        QStringList tos;
        foreach (ServerPlayer *p, to)
            tos << p->objectName();

        l << tos.join("+");
    }
    return l.join("->");
}

MarkChangeStruct::MarkChangeStruct()
    : num(1)
    , player(NULL)
{
}

bool SkillInvokeDetail::operator<(const SkillInvokeDetail &arg2) const // the operator < for sorting the invoke order.
{
    //  we sort firstly according to the priority, then the seat of invoker, at last whether it is a skill of an equip.
    if (!isValid() || !arg2.isValid())
        return false;

    if (skill->getPriority() > arg2.skill->getPriority())
        return true;
    else if (skill->getPriority() < arg2.skill->getPriority())
        return false;

    std::function<Room *(ServerPlayer *)> getRoom = [this](ServerPlayer *p) -> Room * {
        if (p != NULL)
            return p->getRoom();
        else {
            // let's treat it as a gamerule, the gamerule is created inside roomthread
            RoomThread *thread = qobject_cast<RoomThread *>(skill->thread());
            if (thread == NULL)
                return NULL;

            return thread->getRoom();
        }

        return NULL;
    };

    if (invoker != arg2.invoker) {
        Room *room = getRoom(owner);
        if (room == NULL)
            return false;

        return room->getFront(invoker, arg2.invoker) == invoker;
    }

    return !skill->inherits("EquipSkill") && arg2.skill->inherits("EquipSkill");
}

bool SkillInvokeDetail::sameSkill(const SkillInvokeDetail &arg2) const
{
    // it only judge the skill name, the skill invoker and the skill owner. It don't judge the skill target because it is chosen by the skill invoker
    return skill == arg2.skill && owner == arg2.owner && invoker == arg2.invoker;
}

bool SkillInvokeDetail::sameTimingWith(const SkillInvokeDetail &arg2) const
{
    // used to judge 2 skills has the same timing. only 2 structs with the same priority and the same invoker and the same "whether or not it is a skill of equip"
    if (!isValid() || !arg2.isValid())
        return false;

    return skill->getPriority() == arg2.skill->getPriority() && invoker == arg2.invoker && skill->inherits("EquipSkill") == arg2.skill->inherits("EquipSkill");
}

SkillInvokeDetail::SkillInvokeDetail(const TriggerSkill *skill /*= NULL*/, ServerPlayer *owner /*= NULL*/, ServerPlayer *invoker /*= NULL*/,
                                     QList<ServerPlayer *> targets /*= QList<ServerPlayer *>()*/, bool isCompulsory /*= false*/, ServerPlayer *preferredTarget /*= NULL*/,
                                     bool showHidden)
    : skill(skill)
    , owner(owner)
    , invoker(invoker)
    , targets(targets)
    , isCompulsory(isCompulsory)
    , triggered(false)
    , preferredTarget(preferredTarget)
    , showhidden(showHidden)
{
}

SkillInvokeDetail::SkillInvokeDetail(const TriggerSkill *skill, ServerPlayer *owner, ServerPlayer *invoker, ServerPlayer *target, bool isCompulsory /*= false*/,
                                     ServerPlayer *preferredTarget /*= NULL*/, bool showHidden)
    : skill(skill)
    , owner(owner)
    , invoker(invoker)
    , isCompulsory(isCompulsory)
    , triggered(false)
    , preferredTarget(preferredTarget)
    , showhidden(showHidden)
{
    if (target != NULL)
        targets << target;
}

bool SkillInvokeDetail::isValid() const // validity check
{
    return skill != NULL;
}

bool SkillInvokeDetail::preferredTargetLess(const SkillInvokeDetail &arg2) const
{
    if (skill == arg2.skill && owner == arg2.owner && invoker == arg2.invoker) {
        // we compare preferred target to ensure the target selected is in the order of seat only in the case that 2 skills are the same
        if (preferredTarget != NULL && arg2.preferredTarget != NULL)
            return ServerPlayer::CompareByActionOrder(preferredTarget, arg2.preferredTarget);
    }

    return false;
}

QVariant SkillInvokeDetail::toVariant() const
{
    if (!isValid())
        return QVariant();

    JsonObject ob;
    if (skill)
        ob["skill"] = skill->objectName();
    if (owner)
        ob["owner"] = owner->objectName();
    if (invoker)
        ob["invoker"] = invoker->objectName();
    if (preferredTarget) {
        ob["preferredtarget"] = preferredTarget->objectName();
        Room *room = preferredTarget->getRoom();
        ServerPlayer *current = room->getCurrent();
        if (current == NULL)
            current = room->getLord();
        if (current == NULL)
            current = preferredTarget;

        // send the seat info to the client so that we can compare the trigger order of tieqi-like skill in the client side
        int seat = preferredTarget->getSeat() - current->getSeat();
        if (seat < 0)
            seat += room->getPlayers().length();

        ob["preferredtargetseat"] = seat;
    }
    return ob;
}

QStringList SkillInvokeDetail::toList() const
{
    QStringList l;
    if (!isValid())
        l << QString() << QString() << QString() << QString();
    else {
        std::function<void(const QObject *)> insert = [&l](const QObject *item) {
            if (item)
                l << item->objectName();
            else
                l << QString();
        };

        insert(skill);
        insert(owner);
        insert(invoker);
        insert(preferredTarget);
    }

    return l;
}

SkillAcquireDetachStruct::SkillAcquireDetachStruct()
    : skill(NULL)
    , player(NULL)
    , isAcquire(false)
{
}

ChoiceMadeStruct::ChoiceMadeStruct()
    : player(NULL)
{
}

CardAskedStruct::CardAskedStruct()
    : player(NULL)
{
}

HpLostStruct::HpLostStruct()
    : player(NULL)
    , num(0)
{
}

JinkEffectStruct::JinkEffectStruct()
    : jink(NULL)
{
}

PhaseSkippingStruct::PhaseSkippingStruct()
    : phase(Player::NotActive)
    , player(NULL)
    , isCost(false)
{
}

DrawNCardsStruct::DrawNCardsStruct()
    : player(NULL)
    , n(0)
    , isInitial(false)
{
}

SkillInvalidStruct::SkillInvalidStruct()
    : player(NULL)
    , skill(NULL)
    , invalid(false)
{
}

ExtraTurnStruct::ExtraTurnStruct()
    : player(NULL)
    , set_phases(QList<Player::Phase>())
    , reason(QString())
    , extraTarget(NULL)
{
}

BrokenEquipChangedStruct::BrokenEquipChangedStruct()
    : player(NULL)
    , ids(QList<int>())
    , broken(false)
    , moveFromEquip(false)
{
}

ShownCardChangedStruct::ShownCardChangedStruct()
    : player(NULL)
    , ids(QList<int>())
    , shown(false)
    , moveFromHand(false)
{
}
