#include "skill.h"
#include "client.h"
#include "engine.h"
#include "player.h"
#include "room.h"
#include "settings.h"
#include "standard.h"

#include <QFile>

Skill::Skill(const QString &name, Frequency frequency, const QString &showType)
    : frequency(frequency)
    , attached_lord_skill(false)
    , show_type(showType)
    , relate_to_place(QString())
{
    static QChar lord_symbol('$');

    if (name.endsWith(lord_symbol)) {
        QString copy = name;
        copy.remove(lord_symbol);
        setObjectName(copy);
        lord_skill = true;
    } else {
        setObjectName(name);
        lord_skill = false;
    }
}

bool Skill::isLordSkill() const
{
    return lord_skill;
}

bool Skill::isAttachedLordSkill() const
{
    return attached_lord_skill;
}

bool Skill::shouldBeVisible(const Player *Self) const
{
    return Self != NULL;
}

QString Skill::getDescription(bool yellow, bool addHegemony) const
{
    bool normal_game = ServerInfo.DuringGame && isNormalGameMode(ServerInfo.GameMode);
    QString name = QString("%1%2").arg(objectName()).arg(normal_game ? "_p" : "");
    //bool addHegemony = isHegemony && !objectName().endsWith("_hegemony");
    QString des_src = Sanguosha->translate(":" + name, addHegemony);
    if (normal_game && des_src.startsWith(":"))
        des_src = Sanguosha->translate(":" + objectName());
    if (des_src.startsWith(":"))
        return QString();
    QString desc = QString("<font color=%1>%2</font>").arg(yellow ? "#FFFF33" : "#FF0080").arg(des_src);
    //if (isHegemonyGameMode(ServerInfo.GameMode) && !canPreshow())
    if (addHegemony && !canPreshow())
        desc.prepend(QString("<font color=gray>(%1)</font><br/>").arg(tr("this skill cannot preshow")));
    return desc;
}

QString Skill::getNotice(int index) const
{
    if (index == -1)
        return Sanguosha->translate("~" + objectName());

    return Sanguosha->translate(QString("~%1%2").arg(objectName()).arg(index));
}

bool Skill::isVisible() const
{
    return !objectName().startsWith("#");
}

int Skill::getEffectIndex(const ServerPlayer *, const Card *) const
{
    return -1;
}

void Skill::initMediaSource()
{
    sources.clear();
    for (int i = 1;; i++) {
        QString effect_file = QString("audio/skill/%1%2.ogg").arg(objectName()).arg(QString::number(i));
        if (QFile::exists(effect_file))
            sources << effect_file;
        else
            break;
    }

    if (sources.isEmpty()) {
        QString effect_file = QString("audio/skill/%1.ogg").arg(objectName());
        if (QFile::exists(effect_file))
            sources << effect_file;
    }
}

void Skill::playAudioEffect(int index) const
{
    if (!sources.isEmpty()) {
        if (index == -1)
            index = qrand() % sources.length();
        else
            index--;

        // check length
        QString filename;
        if (index >= 0 && index < sources.length())
            filename = sources.at(index);
        else if (index >= sources.length()) {
            while (index >= sources.length())
                index -= sources.length();
            filename = sources.at(index);
        } else
            filename = sources.first();

        Sanguosha->playAudioEffect(filename);
    }
}

Skill::Frequency Skill::getFrequency() const
{
    return frequency;
}

QString Skill::getShowType() const
{
    return show_type;
}

QString Skill::getLimitMark() const
{
    return limit_mark;
}

QString Skill::getRelatedMark() const
{
    return related_mark;
}

QString Skill::getRelatedPileName() const
{
    return related_pile;
}

QStringList Skill::getSources() const
{
    return sources;
}

QDialog *Skill::getDialog() const
{
    return NULL;
}

bool Skill::matchAvaliablePattern(QString avaliablePattern, QString askedPattern) const
{
    //avaliablePattern specifying to a real card
    Card *card = Sanguosha->cloneCard(avaliablePattern);
    //for askForPeach
    if (askedPattern == "peach+analeptic")
        askedPattern = "peach,analeptic";
    //ignore spliting "#"
    QStringList factors = askedPattern.split('|');
    bool checkpoint = false;
    QStringList card_types = factors.at(0).split(',');

    foreach (QString or_name, card_types) {
        checkpoint = false;
        foreach (QString name, or_name.split('+')) {
            if (name == ".") {
                checkpoint = true;
            } else {
                bool isInt = false;
                bool positive = true;
                if (name.startsWith('^')) {
                    positive = false;
                    name = name.mid(1);
                }

                //sometimes, the first character need to Upper
                QString kindOfName = name.left(1).toUpper() + name.right(name.length() - 1);
                if (name.contains(card->objectName()) || card->isKindOf(kindOfName.toLocal8Bit().data()) || ("%" + card->objectName() == name)
                    || (card->getEffectiveId() == name.toInt(&isInt) && isInt))
                    checkpoint = positive;
                else
                    checkpoint = !positive;
            }
            if (!checkpoint)
                break;
        }
        if (checkpoint)
            break;
    }

    delete card;
    return checkpoint;
}

bool Skill::canPreshow() const
{
    if (inherits("TriggerSkill")) {
        const TriggerSkill *triskill = qobject_cast<const TriggerSkill *>(this);
        return triskill->getViewAsSkill() == NULL;
    }

    return false;
}

bool Skill::relateToPlace(bool head) const
{
    if (head)
        return relate_to_place == "head";
    else
        return relate_to_place == "deputy";
    return false;
}

ViewAsSkill::ViewAsSkill(const QString &name)
    : Skill(name, Skill::NotFrequent, "viewas")
    , response_pattern(QString())
    , response_or_use(false)
    , expand_pile(QString())
{
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
        return isEnabledAtResponse(invoker, pattern);
    default:
        return false;
    }
}

bool ViewAsSkill::isEnabledAtPlay(const Player *) const
{
    return response_pattern.isEmpty();
}

bool ViewAsSkill::isEnabledAtResponse(const Player *, const QString &pattern) const
{
    if (!response_pattern.isEmpty())
        return pattern == response_pattern;
    return false;
}

QStringList ViewAsSkill::getDialogCardOptions() const
{
    return QStringList();
}

bool ViewAsSkill::isEnabledAtNullification(const ServerPlayer *) const
{
    return false;
}

const ViewAsSkill *ViewAsSkill::parseViewAsSkill(const Skill *skill)
{
    if (skill == NULL)
        return NULL;
    if (skill->inherits("ViewAsSkill")) {
        const ViewAsSkill *view_as_skill = qobject_cast<const ViewAsSkill *>(skill);
        return view_as_skill;
    }
    if (skill->inherits("TriggerSkill")) {
        const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
        Q_ASSERT(trigger_skill != NULL);
        const ViewAsSkill *view_as_skill = trigger_skill->getViewAsSkill();
        if (view_as_skill != NULL)
            return view_as_skill;
    }
    if (skill->inherits("DistanceSkill")) {
        const DistanceSkill *trigger_skill = qobject_cast<const DistanceSkill *>(skill);
        Q_ASSERT(trigger_skill != NULL);
        const ViewAsSkill *view_as_skill = trigger_skill->getViewAsSkill();
        if (view_as_skill != NULL)
            return view_as_skill;
    }
    if (skill->inherits("AttackRangeSkill")) {
        const AttackRangeSkill *trigger_skill = qobject_cast<const AttackRangeSkill *>(skill);
        Q_ASSERT(trigger_skill != NULL);
        const ViewAsSkill *view_as_skill = trigger_skill->getViewAsSkill();
        if (view_as_skill != NULL)
            return view_as_skill;
    }
    if (skill->inherits("MaxCardsSkill")) {
        const MaxCardsSkill *trigger_skill = qobject_cast<const MaxCardsSkill *>(skill);
        Q_ASSERT(trigger_skill != NULL);
        const ViewAsSkill *view_as_skill = trigger_skill->getViewAsSkill();
        if (view_as_skill != NULL)
            return view_as_skill;
    }
    return NULL;
}

QString ViewAsSkill::getExpandPile() const
{
    return expand_pile;
}

ZeroCardViewAsSkill::ZeroCardViewAsSkill(const QString &name)
    : ViewAsSkill(name)
{
}

const Card *ZeroCardViewAsSkill::viewAs(const QList<const Card *> &cards) const
{
    if (cards.isEmpty())
        return viewAs();
    else
        return NULL;
}

bool ZeroCardViewAsSkill::viewFilter(const QList<const Card *> &, const Card *) const
{
    return false;
}

OneCardViewAsSkill::OneCardViewAsSkill(const QString &name)
    : ViewAsSkill(name)
    , filter_pattern(QString())
{
}

bool OneCardViewAsSkill::viewFilter(const QList<const Card *> &selected, const Card *to_select) const
{
    return selected.isEmpty() && !to_select->hasFlag("using") && viewFilter(to_select);
}

bool OneCardViewAsSkill::viewFilter(const Card *to_select) const
{
    if (!inherits("FilterSkill") && !filter_pattern.isEmpty()) {
        QString pat = filter_pattern;
        if (pat.endsWith("!")) {
            if (Self->isJilei(to_select))
                return false;
            pat.chop(1);
        } else if (response_or_use && pat.contains("hand")) {
            pat.replace("hand", "hand,wooden_ox");
            //pat.replace("hand", handlist.join(","));
        }
        ExpPattern pattern(pat);
        return pattern.match(Self, to_select);
    }
    return false;
}

const Card *OneCardViewAsSkill::viewAs(const QList<const Card *> &cards) const
{
    if (cards.length() != 1)
        return NULL;
    else
        return viewAs(cards.first());
}

FilterSkill::FilterSkill(const QString &name)
    : OneCardViewAsSkill(name)
{
    frequency = Compulsory;
    show_type = "static";
}

TriggerSkill::TriggerSkill(const QString &name)
    : Skill(name)
    , view_as_skill(NULL)
    , global(false)
{
}

const ViewAsSkill *TriggerSkill::getViewAsSkill() const
{
    return view_as_skill;
}

QList<TriggerEvent> TriggerSkill::getTriggerEvents() const
{
    return events;
}

int TriggerSkill::getPriority() const
{
    return 2;
}

void TriggerSkill::record(TriggerEvent, Room *, QVariant &) const
{
}

QList<SkillInvokeDetail> TriggerSkill::triggerable(TriggerEvent, const Room *, const QVariant &) const
{
    return QList<SkillInvokeDetail>();
}

bool TriggerSkill::cost(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
{
    if (invoke->isCompulsory) { //for hegemony
        if (invoke->owner == NULL || invoke->owner != invoke->invoker)
            return true;
        if (invoke->invoker != NULL) {
            if (!invoke->invoker->hasSkill(this))
                return true;
            if (invoke->invoker->hasShownSkill(this) || invoke->invoker->askForSkillInvoke(this, data))
                //invoke->invoker->showHiddenSkill(objectName());
                return true;
            else
                return false;
        }
        return true;
    } else {
        if (invoke->invoker != NULL) {
            //for ai
            invoke->invoker->tag[this->objectName()] = data;
            QVariant notify_data = data;
            if (invoke->preferredTarget != NULL)
                notify_data = QVariant::fromValue(invoke->preferredTarget);
            return invoke->invoker->askForSkillInvoke(this, notify_data);
        }
    }

    return false;
}

bool TriggerSkill::effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &) const
{
    return false;
}

MasochismSkill::MasochismSkill(const QString &name)
    : TriggerSkill(name)
{
    events << Damaged;
}

QList<SkillInvokeDetail> MasochismSkill::triggerable(TriggerEvent, const Room *room, const QVariant &data) const
{
    DamageStruct damage = data.value<DamageStruct>();
    return triggerable(room, damage);
}

QList<SkillInvokeDetail> MasochismSkill::triggerable(const Room *, const DamageStruct &) const
{
    return QList<SkillInvokeDetail>();
}

bool MasochismSkill::effect(TriggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
{
    DamageStruct damage = data.value<DamageStruct>();
    onDamaged(room, invoke, damage);

    return false;
}

PhaseChangeSkill::PhaseChangeSkill(const QString &name)
    : TriggerSkill(name)
{
    events << EventPhaseStart;
}

bool PhaseChangeSkill::effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
{
    ServerPlayer *player = data.value<ServerPlayer *>();
    return onPhaseChange(player);
}

DrawCardsSkill::DrawCardsSkill(const QString &name, bool is_initial)
    : TriggerSkill(name)
    , is_initial(is_initial)
{
    if (is_initial)
        events << DrawInitialCards;
    else
        events << DrawNCards;
}

bool DrawCardsSkill::effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
{
    DrawNCardsStruct s = data.value<DrawNCardsStruct>();
    s.n = getDrawNum(s);
    data = QVariant::fromValue(s);
    return false;
}

GameStartSkill::GameStartSkill(const QString &name)
    : TriggerSkill(name)
{
    events << GameStart;
}

bool GameStartSkill::effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &) const
{
    onGameStart();
    return false;
}

int MaxCardsSkill::getExtra(const Player *) const
{
    return 0;
}

int MaxCardsSkill::getFixed(const Player *) const
{
    return -1;
}

ProhibitSkill::ProhibitSkill(const QString &name)
    : Skill(name, Skill::Compulsory)
{
}

DistanceSkill::DistanceSkill(const QString &name)
    : Skill(name, Skill::Compulsory, "static")
{
    view_as_skill = new ShowDistanceSkill(objectName());
}

const ViewAsSkill *DistanceSkill::getViewAsSkill() const
{
    return view_as_skill;
}

ShowDistanceSkill::ShowDistanceSkill(const QString &name)
    : ZeroCardViewAsSkill(name)
{
}

const Card *ShowDistanceSkill::viewAs() const
{
    SkillCard *card = Sanguosha->cloneSkillCard("ShowFengsu");
    card->setUserString(objectName());
    return card;
}

bool ShowDistanceSkill::isEnabledAtPlay(const Player *player) const
{
    if (!isHegemonyGameMode(ServerInfo.GameMode))
        return false;
    //const DistanceSkill *skill = qobject_cast<const DistanceSkill *>(Sanguosha->getSkill(objectName()));
    const Skill *skill = Sanguosha->getSkill(objectName());
    if (skill) {
        if (!player->hasShownSkill(skill->objectName()))
            return true;
    }
    return false;
}

MaxCardsSkill::MaxCardsSkill(const QString &name)
    : Skill(name, Skill::Compulsory, "static")
{
    view_as_skill = new ShowDistanceSkill(objectName());
}

const ViewAsSkill *MaxCardsSkill::getViewAsSkill() const
{
    return view_as_skill;
}

TargetModSkill::TargetModSkill(const QString &name)
    : Skill(name, Skill::Compulsory)
{
    pattern = "Slash";
}

QString TargetModSkill::getPattern() const
{
    return pattern;
}

int TargetModSkill::getResidueNum(const Player *, const Card *) const
{
    return 0;
}

int TargetModSkill::getDistanceLimit(const Player *, const Card *) const
{
    return 0;
}

int TargetModSkill::getExtraTargetNum(const Player *, const Card *) const
{
    return 0;
}

AttackRangeSkill::AttackRangeSkill(const QString &name)
    : Skill(name, Skill::Compulsory, "static")
{
    view_as_skill = new ShowDistanceSkill(objectName()); //alternative method: add ShowDistanceSkill to specific AttackRangeSkills.
}

const ViewAsSkill *AttackRangeSkill::getViewAsSkill() const
{
    return view_as_skill;
}

int AttackRangeSkill::getExtra(const Player *, bool) const
{
    return 0;
}

int AttackRangeSkill::getFixed(const Player *, bool) const
{
    return -1;
}

SlashNoDistanceLimitSkill::SlashNoDistanceLimitSkill(const QString &skill_name)
    : TargetModSkill(QString("#%1-slash-ndl").arg(skill_name))
    , name(skill_name)
{
}

int SlashNoDistanceLimitSkill::getDistanceLimit(const Player *from, const Card *card) const
{
    if (from->hasSkill(name) && card->getSkillName() == name)
        return 1000;
    else
        return 0;
}

FakeMoveSkill::FakeMoveSkill(const QString &name)
    : TriggerSkill(QString("#%1-fake-move").arg(name))
    , name(name)
{
    events << BeforeCardsMove << CardsMoveOneTime;
    frequency = Compulsory;
    global = true;
}

int FakeMoveSkill::getPriority() const
{
    return 10;
}

bool FakeMoveSkill::effect(TriggerEvent, Room *, QSharedPointer<SkillInvokeDetail>, QVariant &) const
{
    return true;
}

QList<SkillInvokeDetail> FakeMoveSkill::triggerable(TriggerEvent, const Room *room, const QVariant &) const
{
    ServerPlayer *owner = NULL;
    foreach (ServerPlayer *p, room->getAllPlayers()) {
        if (p->hasSkill(this)) {
            owner = p;
            break;
        }
    }

    QString flag = QString("%1_InTempMoving").arg(name);
    foreach (ServerPlayer *p, room->getAllPlayers()) {
        if (p->hasFlag(flag))
            return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, owner, p, NULL, true);
    }

    return QList<SkillInvokeDetail>();
}

EquipSkill::EquipSkill(const QString &name)
    : TriggerSkill(name)
{
}

bool EquipSkill::equipAvailable(const Player *p, EquipCard::Location location, const QString &equipName, const Player *to /*= NULL*/)
{
    if (p == NULL)
        return false;

    if (p->getMark("Equips_Nullified_to_Yourself") > 0)
        return false;

    if (to != NULL && to->getMark("Equips_of_Others_Nullified_to_You") > 0)
        return false;

    switch (location) {
    case EquipCard::WeaponLocation:
        if (!p->hasWeapon(equipName))
            return false;
        break;
    case EquipCard::ArmorLocation:
        if (!p->hasArmorEffect(equipName))
            return false;
        break;
    case EquipCard::TreasureLocation:
        if (!p->hasTreasure(equipName))
            return false;
        break;
    default:
        break; // shenmegui?
    }

    return true;
}

bool EquipSkill::equipAvailable(const Player *p, const EquipCard *card, const Player *to /*= NULL*/)
{
    if (card == NULL)
        return false;

    return equipAvailable(p, card->location(), card->objectName(), to);
}

WeaponSkill::WeaponSkill(const QString &name)
    : EquipSkill(name)
{
}

ArmorSkill::ArmorSkill(const QString &name)
    : EquipSkill(name)
{
}

TreasureSkill::TreasureSkill(const QString &name)
    : EquipSkill(name)
{
}

ViewHasSkill::ViewHasSkill(const QString &name)
    : Skill(name, Skill::Compulsory)
    , global(false)
{
}

BattleArraySkill::BattleArraySkill(const QString &name, const QString type) //
    : TriggerSkill(name)
    , array_type(type)
{
    if (!inherits("LuaBattleArraySkill")) //extremely dirty hack!!!
        view_as_skill = new ArraySummonSkill(objectName());
}

//bool BattleArraySkill::triggerable(const ServerPlayer *player) const
//QList<SkillInvokeDetail> triggerable(TriggerEvent triggerEvent, const Room *room, const QVariant &data) const
//{
//return  TriggerSkill::triggerable(        //TriggerSkill::triggerable(player) && player->aliveCount() >= 4;
//if (room->getAlivePlayers().length() >= 4 && TriggerSkill::triggerable(triggerEvent, )

//    return QList<SkillInvokeDetail>();
//}

void BattleArraySkill::summonFriends(ServerPlayer *player) const
{
    player->summonFriends(array_type);
}

ArraySummonSkill::ArraySummonSkill(const QString &name)
    : ZeroCardViewAsSkill(name)
{
}

const Card *ArraySummonSkill::viewAs() const
{
    QString name = objectName();
    name[0] = name[0].toUpper();
    name += "Summon";
    Card *card = Sanguosha->cloneSkillCard(name);
    card->setShowSkill(objectName());
    return card;
}

//using namespace HegemonyMode;
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
            //return true;
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
