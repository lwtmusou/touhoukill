#include "skill.h"
#include "client.h"
#include "engine.h"
#include "exppattern.h"
#include "player.h"
#include "settings.h"
#include "util.h"

// TODO: kill this
#include "room.h"

#include <random>

using namespace QSanguosha;

class SkillPrivate final
{
public:
    Skill::Categories categories;
    Skill::ShowType showType;
    bool preshow;
    bool frequent;
    QString limit_mark;
    QString related_mark;
    QString related_pile;
    QSet<const Trigger *> triggers;

    SkillPrivate(Skill::Categories categories, Skill::ShowType showType)
        : categories(categories)
        , showType(showType)
        , preshow(false)
        , frequent(false)
    {
        if ((categories & Skill::SkillArrayMask) != 0)
            setupForBattleArray();
    }

    ~SkillPrivate() = default;

private:
    Q_DISABLE_COPY_MOVE(SkillPrivate)

    // TODO: refactor propersal:
    // Battle Array Skill should not be a separate type
    // Currently BattleArraySkill runs SummonArray when turn starts.
    // This seems able to be done in a global trigger with priority 2 to trigger a formation summon in start phase.
    // Since a summon may also be done in the spare time during play phase, a common button for the summon is preferred.
    // The summon itself should be a function of GameLogic
    void setupForBattleArray()
    {
        // TODO: implementation
        Q_UNIMPLEMENTED();
    }
};

Skill::Skill(const QString &name, Categories skillCategories, ShowType showType)
    : d(new SkillPrivate(skillCategories, showType))
{
    setObjectName(name);
}

Skill::~Skill()
{
    delete d;
}

bool Skill::isLordSkill() const
{
    return (d->categories & SkillLord) != 0;
}

bool Skill::isAttachedSkill() const
{
    return (d->categories & SkillAttached) != 0;
}

QString Skill::getDescription() const
{
    bool normal_game = ServerInfo.DuringGame && isRoleGameMode(ServerInfo.GameMode);
    QString name = QStringLiteral("%1%2").arg(objectName(), normal_game ? QStringLiteral("_p") : QString());
    QString des_src = Sanguosha->translate(QStringLiteral(":") + name);
    if (normal_game && des_src.startsWith(QStringLiteral(":")))
        des_src = Sanguosha->translate(QStringLiteral(":") + objectName());
    if (des_src.startsWith(QStringLiteral(":")))
        return QString();
    QString desc = QStringLiteral("<font color=%1>%2</font>").arg(QStringLiteral("#FF0080"), des_src);
    return desc;
}

QString Skill::getNotice(int index) const
{
    if (index == -1)
        return Sanguosha->translate(QStringLiteral("~") + objectName());

    return Sanguosha->translate(QStringLiteral("~%1%2").arg(objectName(), index));
}

bool Skill::isHidden() const
{
    return (d->categories & SkillHidden) != 0;
}

bool Skill::isCompulsory() const
{
    return (d->categories & SkillCompulsory) != 0;
}

bool Skill::isEternal() const
{
    return (d->categories & SkillEternal) != 0;
}

bool Skill::isLimited() const
{
    return (d->categories & SkillLimited) != 0;
}

bool Skill::isFrequent() const
{
    return d->frequent;
}

void Skill::setFrequent(bool c)
{
    d->frequent = c;
}

void Skill::addTrigger(const Trigger *trigger)
{
    d->triggers << trigger;
}

const QSet<const Trigger *> &Skill::triggers() const
{
    return d->triggers;
}

int Skill::getAudioEffectIndex(const Player * /*unused*/, const Card * /*unused*/) const
{
    return -1;
}

Skill::ShowType Skill::getShowType() const
{
    return d->showType;
}

const QString &Skill::limitMark() const
{
    return d->limit_mark;
}

void Skill::setLimitMark(const QString &m)
{
    d->limit_mark = m;
}

const QString &Skill::relatedMark() const
{
    return d->related_mark;
}

void Skill::setRelatedMark(const QString &m)
{
    d->related_mark = m;
}

const QString &Skill::relatedPile() const
{
    return d->related_pile;
}

void Skill::setRelatedPile(const QString &m)
{
    d->related_pile = m;
}

bool Skill::canPreshow() const
{
    return d->preshow;
}

void Skill::setCanPreshow(bool c)
{
    d->preshow = c;
}

bool Skill::relateToPlace(bool head) const
{
    if (head)
        return (d->categories & SkillHead) != 0;
    else
        return (d->categories & SkillDeputy) != 0;
    return false;
}

Skill::ArrayType Skill::arrayType() const
{
    if ((d->categories & SkillArrayMask) != 0)
        return ((d->categories & SkillArrayFormation) != 0) ? ArrayFormation : ArraySiege;

    return ArrayNone;
}

class ViewAsSkillPrivate
{
public:
    QString response_pattern;
    HandlingMethod method;
    QString expand_pile;

    ViewAsSkillPrivate()
        : method(MethodNone)
    {
    }
};

ViewAsSkill::ViewAsSkill(const QString &name)
    : Skill(name, SkillNoFlag, ShowViewAs)
    , d(new ViewAsSkillPrivate)
{
}

ViewAsSkill::~ViewAsSkill()
{
    delete d;
}

bool ViewAsSkill::isAvailable(const Player *invoker, CardUseStruct::CardUseReason reason, const QString &pattern) const
{
    if (!invoker->hasSkill(objectName()) && !invoker->hasLordSkill(objectName()) && !invoker->hasFlag(objectName())) // For Shuangxiong
        return false;
    switch (reason) {
    case CardUseStruct::CARD_USE_REASON_PLAY:
        return isEnabledAtPlay(invoker);
    case CardUseStruct::CARD_USE_REASON_RESPONSE:
    case CardUseStruct::CARD_USE_REASON_RESPONSE_USE:
        return isEnabledAtResponse(invoker, reason, pattern);
    default:
        return false;
    }
}

bool ViewAsSkill::isEnabledAtPlay(const Player * /*unused*/) const
{
    return d->response_pattern.isEmpty();
}

bool ViewAsSkill::isEnabledAtResponse(const Player * /*unused*/, CardUseStruct::CardUseReason /*reason*/, const QString &pattern) const
{
    if (!d->response_pattern.isEmpty())
        return pattern == d->response_pattern;
    return false;
}

#if 0
// Example for Guhuo
class Guhuo : public OneCardViewAsSkill
{
public:
    Guhuo()
        : OneCardViewAsSkill(QStringLiteral("guhuo"))
    {
    }

    ~Guhuo() override = default;

    bool viewFilter(const Card *to_select, const Player *Self, const QStringList &) const override
    {
        return !Self->hasEquip(to_select);
    }

    const Card *viewAs(const Card *originalCard, const Player *Self, const QStringList &CurrentViewAsSkillChain) const override
    {
        QString selectedName = CurrentViewAsSkillChain.last();
        if (selectedName == QStringLiteral("NormalSlash"))
            selectedName = QStringLiteral("Slash");

        Card *card = Self->roomObject()->cloneCard(QStringLiteral("GuhuoCard"));
        card->addSubcard(originalCard);
        card->setUserString(selectedName);
        return card;
    }

    Q_REQUIRED_RESULT const ViewAsSkillSelection *selections(const Player * /*Self*/) const override
    {
        static ViewAsSkillSelection root {QStringLiteral("Guhuo")};

        if (root.next.isEmpty()) {
            // generate list of selections
            ViewAsSkillSelection *basicCard = new ViewAsSkillSelection {QStringLiteral("BasicCard")};
            ViewAsSkillSelection *trickCard = new ViewAsSkillSelection {QStringLiteral("TrickCard")};
            ViewAsSkillSelection *ndTrick = new ViewAsSkillSelection {QStringLiteral("NonDelayedTrick")};
            ViewAsSkillSelection *dTrick = new ViewAsSkillSelection {QStringLiteral("DelayedTrick")};
            root.next << basicCard << trickCard;
            trickCard->next << ndTrick << dTrick;

            QSet<QString> added;

            int count = Sanguosha->getCardCount();
            for (int i = 0; i < count; ++i) {
                const CardDescriptor &descriptor = Sanguosha->getEngineCard(i);
                const CardFace *face = descriptor.face();
                if (!added.contains(face->name())) {
                    ViewAsSkillSelection *selection = new ViewAsSkillSelection {face->name()};
                    if (face->type() == QSanguosha::TypeBasic) {
                        // deal with Slash
                        if (face->name() == QStringLiteral("Slash"))
                            selection->name = QStringLiteral("NormalSlash");
                        basicCard->next << selection;
                    } else if (face->type() == QSanguosha::TypeTrick) {
                        if (face->isNDTrick())
                            ndTrick->next << selection;
                        else
                            dTrick->next << selection;
                    } else
                        delete selection; // in case it is EquipCard or SkillCard (but how can SkillCard be EngineCard?)

                    added << face->name();
                }
            }
        }
        return &root;
    }

    bool isSelectionEnabled(const QStringList &name, const Player *Self) const override
    {
        // e.g. name is something like {"Guhuo", "TrickCard", "NonDelayedTrick", "Dismantlement"}
        // e.g. name is something like {"Guhuo", "BasicCard", "FireSlash"}
        QString selectedName = name.last();
        if (selectedName == QStringLiteral("NormalSlash"))
            selectedName = QStringLiteral("Slash");

        Card *viewAsCard = Self->roomObject()->cloneCard(selectedName, QSanguosha::NoSuit, QSanguosha::NumberNA);
        bool available = viewAsCard->face()->isAvailable(Self, viewAsCard);
        Self->roomObject()->cardDeleting(viewAsCard);
        return available;
    }
};
#endif

ViewAsSkillSelection::~ViewAsSkillSelection()
{
    qDeleteAll(next);
}

const ViewAsSkillSelection *ViewAsSkill::selections(const Player * /*unused*/) const
{
    return nullptr;
}

bool ViewAsSkill::isSelectionEnabled(const QStringList & /*unused*/, const Player * /*unused*/) const
{
    return true;
}

HandlingMethod ViewAsSkill::handlingMethod() const
{
    return d->method;
}

void ViewAsSkill::setHandlingMethod(HandlingMethod method)
{
    d->method = method;
}

QString ViewAsSkill::expandPile(const Player * /*Self*/) const
{
    return d->expand_pile;
}

void ViewAsSkill::setExpandPile(const QString &expand)
{
    d->expand_pile = expand;
}

const QString &ViewAsSkill::responsePattern() const
{
    return d->response_pattern;
}

void ViewAsSkill::setResponsePattern(const QString &pattern)
{
    d->response_pattern = pattern;
}

ZeroCardViewAsSkill::ZeroCardViewAsSkill(const QString &name)
    : ViewAsSkill(name)
{
}

const Card *ZeroCardViewAsSkill::viewAs(const QList<const Card *> &cards, const Player *Self, const QStringList &CurrentViewAsSkillChain) const
{
    if (cards.isEmpty())
        return viewAs(Self, CurrentViewAsSkillChain);
    else
        return nullptr;
}

bool ZeroCardViewAsSkill::viewFilter(const QList<const Card *> & /*selected*/, const Card * /*to_select*/, const Player * /*Self*/,
                                     const QStringList & /*CurrentViewAsSkillChain*/) const
{
    return false;
}

class OneCardViewAsSkillPrivate
{
public:
    QString filter_pattern;

    virtual ~OneCardViewAsSkillPrivate() = default;
};

OneCardViewAsSkill::OneCardViewAsSkill(const QString &name)
    : ViewAsSkill(name)
    , d(new OneCardViewAsSkillPrivate)
{
}

OneCardViewAsSkill::~OneCardViewAsSkill()
{
    delete d;
}

bool OneCardViewAsSkill::viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self, const QStringList &CurrentViewAsSkillChain) const
{
    return selected.isEmpty() && !to_select->hasFlag(QStringLiteral("using")) && viewFilter(to_select, Self, CurrentViewAsSkillChain);
}

bool OneCardViewAsSkill::viewFilter(const Card *to_select, const Player *Self, const QStringList & /*CurrentViewAsSkillChain*/) const
{
    if (!d->filter_pattern.isEmpty())
        return ExpPattern(d->filter_pattern).match(Self, to_select);

    return false;
}

void OneCardViewAsSkill::setFilterPattern(const QString &p)
{
    d->filter_pattern = p;
}

const Card *OneCardViewAsSkill::viewAs(const QList<const Card *> &cards, const Player *Self, const QStringList &CurrentViewAsSkillChain) const
{
    if (cards.length() != 1)
        return nullptr;
    else
        return viewAs(cards.first(), Self, CurrentViewAsSkillChain);
}

class FilterSkillPrivate : public OneCardViewAsSkillPrivate
{
public:
    ~FilterSkillPrivate() override = default;
};

FilterSkill::FilterSkill(const QString &name)
    : Skill(name, SkillCompulsory, ShowStatic)
    , d(new FilterSkillPrivate)
{
}

FilterSkill::~FilterSkill()
{
    delete d;
}

bool FilterSkill::viewFilter(const Card *to_select, const Player *Self) const
{
    if (!d->filter_pattern.isEmpty())
        return ExpPattern(d->filter_pattern).match(Self, to_select);

    return false;
}

void FilterSkill::setFilterPattern(const QString &p)
{
    d->filter_pattern = p;
}

int MaxCardsSkill::getExtra(const Player * /*unused*/) const
{
    return 0;
}

int MaxCardsSkill::getFixed(const Player * /*unused*/) const
{
    return -1;
}

ProhibitSkill::ProhibitSkill(const QString &name)
    : Skill(name)
{
}

DistanceSkill::DistanceSkill(const QString &name)
    : Skill(name, SkillCompulsory, ShowStatic)
{
}

MaxCardsSkill::MaxCardsSkill(const QString &name)
    : Skill(name, SkillCompulsory, ShowStatic)
{
}

TargetModSkill::TargetModSkill(const QString &name)
    : Skill(name, SkillCompulsory)
{
    pattern = QStringLiteral("Slash");
}

QString TargetModSkill::getPattern() const
{
    return pattern;
}

int TargetModSkill::getResidueNum(const Player * /*unused*/, const Card * /*unused*/) const
{
    return 0;
}

int TargetModSkill::getDistanceLimit(const Player * /*unused*/, const Card * /*unused*/) const
{
    return 0;
}

int TargetModSkill::getExtraTargetNum(const Player * /*unused*/, const Card * /*unused*/) const
{
    return 0;
}

AttackRangeSkill::AttackRangeSkill(const QString &name)
    : Skill(name, SkillCompulsory, ShowStatic)
{
}

int AttackRangeSkill::getExtra(const Player * /*unused*/, bool /*unused*/) const
{
    return 0;
}

int AttackRangeSkill::getFixed(const Player * /*unused*/, bool /*unused*/) const
{
    return -1;
}

SlashNoDistanceLimitSkill::SlashNoDistanceLimitSkill(const QString &skill_name)
    : TargetModSkill(QStringLiteral("#%1-slash-ndl").arg(skill_name))
    , name(skill_name)
{
}

int SlashNoDistanceLimitSkill::getDistanceLimit(const Player *from, const Card *card) const
{
    if (from->hasSkill(name) && card->skillName() == name)
        return 1000;
    else
        return 0;
}

TreatAsEquippingSkill::TreatAsEquippingSkill(const QString &name)
    : Skill(name, SkillCompulsory)
{
}

#if 0
// In case the code are used in future when Skill::setupForBattleArray implementation


class BattleArraySkill : public ::TriggerSkill
{
    // Q_OBJECT

public:
    BattleArraySkill(const QString &name, const QString arrayType);

    virtual void summonFriends(ServerPlayer *player) const;

    inline QString getArrayType() const
    {
        return array_type;
    }

private:
    QString array_type;
};

class ArraySummonSkill : public ZeroCardViewAsSkill
{
    // Q_OBJECT

public:
    explicit ArraySummonSkill(const QString &name);

    const Card *viewAs(const Player *Self) const override;
    bool isEnabledAtPlay(const Player *player) const override;
};


BattleArraySkill::BattleArraySkill(const QString &name, const QString type) //
    : TriggerSkill(name)
    , array_type(type)
{
    if (!inherits("LuaBattleArraySkill")) //extremely dirty hack!!!
        view_as_skill = new ArraySummonSkill(objectName());
}

void BattleArraySkill::summonFriends(ServerPlayer *player) const
{
    player->summonFriends(array_type);
}

ArraySummonSkill::ArraySummonSkill(const QString &name)
    : ZeroCardViewAsSkill(name)
{
}

const Card *ArraySummonSkill::viewAs(const Player *Self) const
{
    QString name = objectName();
    name[0] = name[0].toUpper();
    name += "Summon";
    Card *card = Self->getRoomObject()->cloneSkillCard(name);
    card->setShowSkillName(objectName());
    return card;
}

bool ArraySummonSkill::isEnabledAtPlay(const Player *player) const
{
    if (player->getAliveSiblings().length() < 3)
        return false;
    if (player->hasFlag("Global_SummonFailed"))
        return false;
    if (!player->canShowGeneral(player->inHeadSkills(objectName()) ? "h" : "d"))
        return false;
    const BattleArraySkill *skill = qobject_cast<const BattleArraySkill *>(Sanguosha->getTriggerSkill(objectName()));
    if (skill) {
        QString type = skill->getArrayType();

        if (type == "Siege") {
            if (player->willBeFriendWith(player->getNextAlive()) && player->willBeFriendWith(player->getLastAlive()))
                return false;
            if (!player->willBeFriendWith(player->getNextAlive())) {
                if (!player->getNextAlive(2)->hasShownOneGeneral() && player->getNextAlive()->hasShownOneGeneral())
                    return true;
            }
            if (!player->willBeFriendWith(player->getLastAlive()))
                return !player->getLastAlive(2)->hasShownOneGeneral() && player->getLastAlive()->hasShownOneGeneral();

        } else if (type == "Formation") {
            int n = player->aliveCount(false);
            int asked = n;
            for (int i = 1; i < n; ++i) {
                Player *target = player->getNextAlive(i);
                if (player->isFriendWith(target))
                    continue;
                else if (!target->hasShownOneGeneral())
                    return true;
                else {
                    asked = i;
                    break;
                }
            }
            n -= asked;
            for (int i = 1; i < n; ++i) {
                Player *target = player->getLastAlive(i);
                if (player->isFriendWith(target))
                    continue;
                else
                    return !target->hasShownOneGeneral();
            }
        }
    }
    return false;
}
#endif
