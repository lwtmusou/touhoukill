#include "player.h"
#include "client.h"
#include "engine.h"
#include "exppattern.h"
#include "general.h"
#include "settings.h"
#include "util.h"

using namespace QSanguosha;

class PlayerPrivate
{
public:
    // Player is class shared from Server and Client.
    // for Server, all information is preserved.
    // but for Client, only part of them are exposed in order not to leak message from protocol.
    // We call data which should be exposed to Client as Public Data, and ones which shouldn't as Secret Data.

    // For Sanguosha, the secret data is only Handcard ids and Role.
    // Other things such as general, number of Handcard, Equip id, kingdom, gender, hp/maxhp, etc., are all public data.

    // Game essentials:
    RoomObject *room;
    QString screenName; // a better name may be UserName?
    QString state; // A limited set of "online", "offline", "trust", "robot", which indicates how the player's actual network and/or playing state, considering to refactor to enum

    // Public Data:
    QList<const General *> generals;
    QList<bool> generalShowed; // for Hegemony
    Gender gender;
    int hp;
    int linghp; // for skill 'banling', which devides the HP to 2 parts
    int renhp;
    int maxHp;
    int dyingFactor; // maybe renamed to minHP
    bool alive;
    QString kingdom; // Also used as Role for Hegemony mode. Careerist should also be dealt in this property
    Role role; // only for Role mode, not for Hegemony mode anymore. Secret data but will be public at some time
    bool roleShown;
    int seat;
    Phase phase;
    bool turnSkipping; // Turned over in Role mode and Put stacked in Hegemony mode. Original !face_up
    bool chained;
    bool removed;

    // use int here. Card can be obtained from RoomObject
    int weapon;
    int armor;
    int defensiveHorse;
    int offensiveHorse;
    int treasure;
    QList<int> judgingArea; // DO NOT USE IDSet SINCE THIS SHOULD KEEP ORIGINAL ORDER
    int handCardNum;

    QHash<const Player *, int> fixedDistance;
    QMap<HandlingMethod, QMap<QString, QStringList>> cardLimitation; // method, reason, pattern
    QStringList disableShow;

    // original mark is only visible if it starts with '@'. The refactor proposal is to make mark always visible
    QMap<QString, int> marks;
    QSet<QString> flags;
    QHash<QString, int> history;
    QMap<QString, IDSet> piles;
    QSet<QString> acquiredSkills; // Acquired skills isn't split into parts by game rule
    QList<QMap<QString, bool>> skills; // General card skill
    QStringList skillInvalid;
    IDSet shownHandcards;
    IDSet brokenEquips;

    QList<const Card *> handcards; // a.k.a. knownHandCards in client side

    PlayerPrivate(RoomObject *room)
        : room(room)
        , gender(Sexless)
        , hp(-1)
        , linghp(-1)
        , renhp(-1)
        , maxHp(-1)
        , dyingFactor(-1)
        , alive(true)
        , role(RoleLord)
        , roleShown(false)
        , seat(0)
        , phase(PhaseNone)
        , turnSkipping(false)
        , chained(false)
        , removed(false)
        , weapon(-1)
        , armor(-1)
        , defensiveHorse(-1)
        , offensiveHorse(-1)
        , treasure(-1)
        , handCardNum(0)

    {
    }

    // other data which belonged to original protected and private part of Player and got removed during refactor
#if 0
    // ------ Player ------
    // They are properties which is related to a specific skill.
    // unlike 'banling' which changed the core property and must be coupled into Player
    QStringList hidden_generals; //for anyun
    QString shown_hidden_general; // Fs: I remembered that Huashen isn't implemented using these variable. Tempoaray put it away until I found another solution
    int chaoren;

    QMap<QString, QStringList> pile_open; // Fs: only used to classify move in server side. Not that important since secret pile should remain its security.
    QStringList skills_originalOrder, skills2_originalOrder; //equals  skills.keys().  unlike QMap, QStringList will keep originalOrder // Fs: removed due to meanless
    bool owner; // Fs: for request & response
    int initialSeat; //for record
    QString next;

    // ------ ServerPlayer ------
    QSemaphore **semas;
    static const int S_NUM_SEMAPHORES;
    ClientSocket *socket;
    Room *room;
    Recorder *recorder;
    QList<QSanguosha::Phase> phases;
    int _m_phases_index;
    QList<PhaseStruct> _m_phases_state;
    QStringList selected; // 3v3 mode use only
    QDateTime test_time;
    QString m_clientResponseString;
    QVariant _m_clientResponse;
    bool ready;

    // ------ ClientPlayer ------
    QTextDocument *mark_doc; // originally this one is todo in ClientPlayer

#endif
};

Player::Player(RoomObject *parent)
    : QObject(parent)
    , d(new PlayerPrivate(parent))
{
}

Player::~Player()
{
    delete d;
}

void Player::setScreenName(const QString &screen_name)
{
    d->screenName = screen_name;
}

QString Player::screenName() const
{
    return d->screenName;
}

bool Player::hasShownRole() const
{
    return d->roleShown;
}

void Player::setShownRole(bool shown)
{
    d->roleShown = shown;
}

void Player::setHp(int hp)
{
    bool changed = false;
    if (d->hp != hp) {
        d->hp = hp;
        changed = true;
    }
    if (hasSkill(QStringLiteral("banling"))) {
        if (d->renhp != hp) {
            d->renhp = hp;
            changed = true;
        }
        if (d->linghp != hp) {
            d->linghp = hp;
            changed = true;
        }
    }
}

int Player::getHp() const
{
    if (hasSkill(QStringLiteral("huanmeng")))
        return 0;
    return d->hp;
}

int Player::dyingThreshold() const
{
    int value = 1 + d->dyingFactor;
    foreach (const Player *p, d->room->players()) {
        if (p == this)
            continue;

        if (p->isCurrent() && p->hasSkill(QStringLiteral("yousi")))
            value = qMax(0, p->getHp());
    }
    return value;
}

void Player::setRenHp(int renhp)
{
    if (d->renhp != renhp) {
        d->renhp = renhp;
        if (qMin(d->linghp, d->renhp) != d->hp)
            d->hp = qMin(d->linghp, d->renhp);
    }
}

void Player::setLingHp(int linghp)
{
    if (d->linghp != linghp) {
        d->linghp = linghp;
        if (qMin(d->linghp, d->renhp) != d->hp)
            d->hp = qMin(d->linghp, d->renhp);
    }
}

void Player::setDyingFactor(int dyingFactor)
{
    if (d->dyingFactor != dyingFactor)
        d->dyingFactor = dyingFactor;
}

int Player::getRenHp() const
{
    return d->renhp;
}

int Player::getLingHp() const
{
    return d->linghp;
}

int Player::getDyingFactor() const
{
    return d->dyingFactor;
}

const IDSet &Player::getShownHandcards() const
{
    return d->shownHandcards;
}

void Player::setShownHandcards(const IDSet &ids)
{
    d->shownHandcards = ids;
}

bool Player::isShownHandcard(int id) const
{
    if (d->shownHandcards.isEmpty() || id < 0)
        return false;
    return d->shownHandcards.contains(id);
}

const IDSet &Player::getBrokenEquips() const
{
    return d->brokenEquips;
}

void Player::setBrokenEquips(const IDSet &ids)
{
    d->brokenEquips = ids;
}

bool Player::isBrokenEquip(int id, bool consider_shenbao) const
{
    if (d->brokenEquips.isEmpty() || id < 0)
        return false;

    if (consider_shenbao)
        return d->brokenEquips.contains(id) && !hasSkill(QStringLiteral("shenbao"), false, false);
    return d->brokenEquips.contains(id);
}

int Player::getMaxHp() const
{
    if (hasSkill(QStringLiteral("huanmeng")))
        return 0;

    return d->maxHp;
}

void Player::setMaxHp(int max_hp)
{
    if (d->maxHp == max_hp)
        return;
    d->maxHp = max_hp;
    if (d->hp > max_hp)
        d->hp = max_hp;
}

int Player::getLostHp() const
{
    return d->maxHp - qMax(getHp(), 0);
}

bool Player::isWounded() const
{
    if (d->hp < 0)
        return true;
    else
        return getHp() < d->maxHp;
}

Gender Player::getGender() const
{
    return d->gender;
}

void Player::setGender(Gender gender)
{
    d->gender = gender;
}

bool Player::isMale() const
{
    return d->gender == Male;
}

bool Player::isFemale() const
{
    return d->gender == Female;
}

bool Player::isNeuter() const
{
    return d->gender == Neuter;
}

int Player::getSeat() const
{
    return d->seat;
}

void Player::setSeat(int seat)
{
    d->seat = seat;
}

bool Player::isAdjacentTo(const Player *another) const
{
    int alive_length = d->room->players(false).count();
    return qAbs(d->seat - another->getSeat()) == 1 || (d->seat == 1 && another->getSeat() == alive_length) || (d->seat == alive_length && another->getSeat() == 1);
}

bool Player::isAlive() const
{
    return d->alive;
}

void Player::setAlive(bool alive)
{
    d->alive = alive;
}

QString Player::getFlags() const
{
    // QStringList constructor is needed for Qt 5 compatibility
    // Since Qt 6 QStringList is exactly equal to QList<QString>
    return QStringList(d->flags.values()).join(QStringLiteral("|"));
}

QStringList Player::getFlagList() const
{
    return d->flags.values();
}

void Player::setFlags(const QString &flag)
{
    if (flag == QStringLiteral(".")) {
        clearFlags();
        return;
    }
    static QLatin1Char unset_symbol('-');
    if (flag.startsWith(unset_symbol)) {
        QString copy = flag;
        copy.remove(unset_symbol);
        d->flags.remove(copy);
    } else {
        d->flags.insert(flag);
    }
}

bool Player::hasFlag(const QString &flag) const
{
    return d->flags.contains(flag);
}

void Player::clearFlags()
{
    d->flags.clear();
}

int Player::getAttackRange(bool include_weapon) const
{
    if (hasFlag(QStringLiteral("InfinityAttackRange")) || getMark(QStringLiteral("InfinityAttackRange")) > 0)
        return 1000;

    include_weapon = include_weapon && d->weapon != -1;

    int fixeddis = Sanguosha->correctAttackRange(this, include_weapon, true);
    if (fixeddis > 0)
        return fixeddis;

    int original_range = 1;
    int weapon_range = 0;

    if (include_weapon) {
        const Weapon *face = qobject_cast<const Weapon *>(d->room->getCard(d->weapon)->face());
        Q_ASSERT(face);
        if (!isBrokenEquip(d->weapon, true))
            weapon_range = face->range();
    }

    int real_range = qMax(original_range, weapon_range) + Sanguosha->correctAttackRange(this, include_weapon, false);

    if (real_range < 0)
        real_range = 0;

    return real_range;
}

bool Player::inMyAttackRange(const Player *other) const
{
    if (other->isDead())
        return false;
    if (distanceTo(other) == -1)
        return false;
    if (this == other)
        return false;
    return distanceTo(other) <= getAttackRange();
}

void Player::setFixedDistance(const Player *player, int distance)
{
    if (distance == -1)
        d->fixedDistance.remove(player);
    else
        d->fixedDistance.insert(player, distance);
}

int Player::originalRightDistanceTo(const Player *other) const
{
    int right = 0;
    const Player *next_p = this;
    while (next_p != other) {
        next_p = next_p->getNextAlive();
        right++;
    }
    return right;
}

int Player::distanceTo(const Player *other, int distance_fix) const
{
    if (this == other)
        return 0;

    if (isRemoved() || other->isRemoved())
        return -1;
    //point1: chuanwu is a fixed distance;
    int distance_limit = 0;
    if (hasSkill(QStringLiteral("chuanwu")))
        distance_limit = qMax(other->getHp(), 1);
    if (d->fixedDistance.contains(other)) {
        if (distance_limit > 0 && d->fixedDistance.value(other) > distance_limit)
            return distance_limit;
        else
            return d->fixedDistance.value(other);
    }

    int right = originalRightDistanceTo(other);
    int left = d->room->players(false, false).length() - right;
    int distance = qMin(left, right);

    distance += Sanguosha->correctDistance(this, other);
    distance += distance_fix;
    if (distance_limit > 0)
        distance = qMin(distance_limit, distance);
    // keep the distance >=1
    if (distance < 1)
        distance = 1;

    return distance;
}

Player *Player::getNext(bool ignoreRemoved)
{
    return d->room->findAdjecentPlayer(this, true, true, !ignoreRemoved);
}

Player *Player::getLast(bool ignoreRemoved)
{
    return d->room->findAdjecentPlayer(this, false, true, !ignoreRemoved);
}

Player *Player::getNextAlive(int n, bool ignoreRemoved)
{
    Player *p = getNext(ignoreRemoved);
    if (p->isAlive()) {
        n = n - 1;
        if (n == 0)
            return p;
    }

    return p->getNextAlive(n, ignoreRemoved);
}

Player *Player::getLastAlive(int n, bool ignoreRemoved)
{
    Player *p = getLast(ignoreRemoved);
    if (p->isAlive()) {
        n = n - 1;
        if (n == 0)
            return p;
    }

    return p->getLastAlive(n, ignoreRemoved);
}

const Player *Player::getNext(bool ignoreRemoved) const
{
    return d->room->findAdjecentPlayer(this, true, true, !ignoreRemoved);
}

const Player *Player::getLast(bool ignoreRemoved) const
{
    return d->room->findAdjecentPlayer(this, false, true, !ignoreRemoved);
}

const Player *Player::getNextAlive(int n, bool ignoreRemoved) const
{
    const Player *p = getNext(ignoreRemoved);
    if (p->isAlive()) {
        n = n - 1;
        if (n == 0)
            return p;
    }

    return p->getNextAlive(n, ignoreRemoved);
}

const Player *Player::getLastAlive(int n, bool ignoreRemoved) const
{
    const Player *p = getLast(ignoreRemoved);
    if (p->isAlive()) {
        n = n - 1;
        if (n == 0)
            return p;
    }

    return p->getLastAlive(n, ignoreRemoved);
}

void Player::setGeneral(const General *new_general, int pos)
{
    if (new_general == nullptr)
        return;

    if (d->generals.length() > pos)
        d->generals[pos] = new_general;
    else if (d->generals.length() == pos)
        d->generals << new_general;

    if (d->kingdom.isEmpty() && pos == 0)
        setKingdom(new_general->getKingdom());
}

const General *Player::getGeneral(int pos) const
{
    if (d->generals.length() > pos)
        return d->generals.at(pos);
    return nullptr;
}

QString Player::getGeneralName(int pos) const
{
    const General *general = getGeneral(pos);
    return (general == nullptr) ? QString() : general->name();
}

void Player::setGeneralName(const QString &name, int pos)
{
    setGeneral(Sanguosha->getGeneral(name), pos);
}

QString Player::getFootnoteName() const
{
    foreach (const General *general, d->generals) {
        if (general->name() != QStringLiteral("anjiang"))
            return general->name();
    }

    return Sanguosha->translate(QStringLiteral("SEAT(%1)").arg(QString::number(getSeat())));
}

QString Player::getState() const
{
    return d->state;
}

void Player::setState(const QString &state)
{
    if (d->state != state)
        d->state = state;
}

void Player::setRole(const QString &role)
{
    static QMap<QString, Role> role_map;
    if (role_map.isEmpty()) {
        role_map.insert(QStringLiteral("lord"), RoleLord);
        role_map.insert(QStringLiteral("loyalist"), RoleLoyalist);
        role_map.insert(QStringLiteral("rebel"), RoleRebel);
        role_map.insert(QStringLiteral("renegade"), RoleRenegade);
    }

    setRole(role_map.value(role));
}

void Player::setRole(Role role)
{
    d->role = role;
}

QString Player::getRoleString() const
{
    static QHash<Role, QString> role_map;
    if (role_map.isEmpty()) {
        role_map.insert(RoleLord, QStringLiteral("lord"));
        role_map.insert(RoleLoyalist, QStringLiteral("loyalist"));
        role_map.insert(RoleRebel, QStringLiteral("rebel"));
        role_map.insert(RoleRenegade, QStringLiteral("renegade"));
    }

    return role_map.value(d->role);
}

Role Player::getRole() const
{
    return d->role;
}

const General *Player::getAvatarGeneral() const
{
    if (general != nullptr)
        return general;

    QString general_name = property("avatar").toString();
    if (general_name.isEmpty())
        return nullptr;
    return Sanguosha->getGeneral(general_name);
}

bool Player::isCurrent() const
{
    return d->phase != PhaseNotActive;
}

bool Player::hasSkill(const QString &skill_name, bool include_lose, bool include_hidden) const
{
    return hasSkill(Sanguosha->getSkill(skill_name), include_lose, include_hidden);
}

bool Player::hasSkill(const Skill *skill, bool include_lose, bool include_hidden) const
{
    if (skill == nullptr)
        return false;

    QString skill_name = skill->objectName();

    //@todo: need check
    if (isHegemonyGameMode(ServerInfo.GameMode)) {
        if (!include_lose && !hasEquipSkill(skill_name) && !getAcquiredSkills().contains(skill_name) && ownSkill(skill_name)
            && !canShowGeneral(inHeadSkills(skill_name) ? QStringLiteral("h") : QStringLiteral("d")))
            return false;
        if (!include_lose && !hasEquipSkill(skill_name) && !skill->isEternal()) {
            if (isSkillInvalid(skill_name))
                return false;
        }
        return skills.value(skill_name, false) || skills2.value(skill_name, false) || acquired_skills.contains(skill_name) || acquired_skills2.contains(skill_name);
    }

    //Other modes
    //For skill "yibian" of reimu_god
    if (getMark(QStringLiteral("@disableShowRole")) > 0 && !hasShownRole()) {
        if (!skill->isEternal() && !skill->isAttachedSkill() && !hasEquipSkill(skill_name))
            return false;
    }

    //prevent infinite recursion
    if (include_hidden && !isSkillInvalid(QStringLiteral("anyun"))
        && (skills.contains(QStringLiteral("anyun")) || skills2.contains(QStringLiteral("anyun")) || acquired_skills.contains(QStringLiteral("anyun"))
            || acquired_skills2.contains(QStringLiteral("anyun")))
        && !skill->isLordSkill() && !skill->isAttachedSkill() && !skill->isLimited() && !skill->isEternal()
        && (skill->getShowType() != Skill::ShowStatic || hasFlag(QStringLiteral("has_anyu_state")))) {
        QString shown = shown_hidden_general;
        if (shown.isNull()) {
            foreach (QString hidden, hidden_generals) {
                const General *g = Sanguosha->getGeneral(hidden);
                if (g->hasSkill(skill_name))
                    return true;
            }
        }
    }

    if (!include_lose && !hasEquipSkill(skill_name) && !skill->isEternal()) {
        if (isSkillInvalid(skill_name))
            return false;
    }

    return skills.contains(skill_name) || skills2.value(skill_name) || acquired_skills.contains(skill_name) || acquired_skills2.contains(skill_name);
}

bool Player::hasSkills(const QString &skill_name, bool include_lose) const
{
    foreach (QString skill, skill_name.split(QStringLiteral("|"))) {
        bool checkpoint = true;
        foreach (QString sk, skill.split(QStringLiteral("+"))) {
            if (!hasSkill(sk, include_lose)) {
                checkpoint = false;
                break;
            }
        }
        if (checkpoint)
            return true;
    }
    return false;
}

bool Player::hasInnateSkill(const QString &skill_name) const
{
    if ((general != nullptr) && general->hasSkill(skill_name))
        return true;

    if ((general2 != nullptr) && general2->hasSkill(skill_name))
        return true;

    return false;
}

bool Player::hasInnateSkill(const Skill *skill) const
{
    if (skill == nullptr)
        return false;

    return hasInnateSkill(skill->objectName());
}

bool Player::hasLordSkill(const QString &skill_name, bool include_lose) const
{
    if (!hasSkill(skill_name, include_lose))
        return false;

    if (acquired_skills.contains(skill_name) || acquired_skills2.contains(skill_name))
        return true;

    QString mode = ServerInfo.GameMode;
    if (mode == QStringLiteral("06_3v3") || mode == QStringLiteral("06_XMode") || mode == QStringLiteral("02_1v1")
        || Config.value(QStringLiteral("WithoutLordskill"), false).toBool())
        return false;

    if (isLord())
        return skills.contains(skill_name);

    return false;
}

bool Player::hasLordSkill(const Skill *skill, bool include_lose /* = false */) const
{
    if (skill == nullptr)
        return false;

    return hasLordSkill(skill->objectName(), include_lose);
}

void Player::setSkillInvalidity(const Skill *skill, bool invalidity)
{
    if (skill == nullptr)
        setSkillInvalidity(QStringLiteral("_ALL_SKILLS"), invalidity);
    else
        setSkillInvalidity(skill->objectName(), invalidity);
}

void Player::setSkillInvalidity(const QString &skill_name, bool invalidity)
{
    if (invalidity && !skill_invalid.contains(skill_name))
        skill_invalid << skill_name;
    else if (!invalidity && skill_invalid.contains(skill_name))
        skill_invalid.removeAll(skill_name);
}

bool Player::isSkillInvalid(const Skill *skill) const
{
    if (skill == nullptr)
        return isSkillInvalid(QStringLiteral("_ALL_SKILLS"));

    if (skill->isEternal() || skill->isAttachedSkill())
        return false;

    return isSkillInvalid(skill->objectName());
}

bool Player::isSkillInvalid(const QString &skill_name) const
{
    if (skill_name != QStringLiteral("_ALL_SKILLS")) {
        const Skill *skill = Sanguosha->getSkill(skill_name);
        if ((skill != nullptr) && (skill->isEternal() || skill->isAttachedSkill()))
            return false;
    }

    if (skill_invalid.contains(QStringLiteral("_ALL_SKILLS")))
        return true;

    return skill_invalid.contains(skill_name);
}

void Player::acquireSkill(const QString &skill_name, bool head)
{
    QSet<QString> &skills = head ? acquired_skills : acquired_skills2;
    skills.insert(skill_name);
}

void Player::detachSkill(const QString &skill_name, bool head)
{
    if (head)
        acquired_skills.remove(skill_name);
    else
        acquired_skills2.remove(skill_name);
}

void Player::detachAllSkills()
{
    acquired_skills.clear();
    acquired_skills2.clear();
}

void Player::addSkill(const QString &skill_name, bool head_skill)
{
    const Skill *skill = Sanguosha->getSkill(skill_name);
    Q_ASSERT(skill);

    if (head_skill)
        skills[skill_name] = !skill->canPreshow() || general_showed;
    else
        skills2[skill_name] = !skill->canPreshow() || general2_showed;
}

void Player::loseSkill(const QString &skill_name, bool head)
{
    if (head)
        skills.remove(skill_name);
    else
        skills2.remove(skill_name);
}

QString Player::getPhaseString() const
{
    switch (phase) {
    case PhaseRoundStart:
        return QStringLiteral("round_start");
    case PhaseStart:
        return QStringLiteral("start");
    case PhaseJudge:
        return QStringLiteral("judge");
    case PhaseDraw:
        return QStringLiteral("draw");
    case PhasePlay:
        return QStringLiteral("play");
    case PhaseDiscard:
        return QStringLiteral("discard");
    case PhaseFinish:
        return QStringLiteral("finish");
    case PhaseNotActive:
    default:
        return QStringLiteral("not_active");
    }
}

void Player::setPhaseString(const QString &phase_str)
{
    static QMap<QString, Phase> phase_map;
    if (phase_map.isEmpty()) {
        phase_map.insert(QStringLiteral("round_start"), PhaseRoundStart);
        phase_map.insert(QStringLiteral("start"), PhaseStart);
        phase_map.insert(QStringLiteral("judge"), PhaseJudge);
        phase_map.insert(QStringLiteral("draw"), PhaseDraw);
        phase_map.insert(QStringLiteral("play"), PhasePlay);
        phase_map.insert(QStringLiteral("discard"), PhaseDiscard);
        phase_map.insert(QStringLiteral("finish"), PhaseFinish);
        phase_map.insert(QStringLiteral("not_active"), PhaseNotActive);
    }

    setPhase(phase_map.value(phase_str, PhaseNotActive));
}

void Player::setEquip(const Card *equip)
{
    const EquipCard *face = qobject_cast<const EquipCard *>(equip->face());
    Q_ASSERT(face != nullptr);
    switch (face->location()) {
    case WeaponLocation:
        weapon = equip;
        break;
    case ArmorLocation:
        armor = equip;
        break;
    case DefensiveHorseLocation:
        defensive_horse = equip;
        break;
    case OffensiveHorseLocation:
        offensive_horse = equip;
        break;
    case TreasureLocation:
        treasure = equip;
        break;
    }
}

void Player::removeEquip(const Card *equip)
{
    const EquipCard *face = qobject_cast<const EquipCard *>(equip->face());
    Q_ASSERT(face != nullptr);
    switch (face->location()) {
    case WeaponLocation:
        weapon = nullptr;
        break;
    case ArmorLocation:
        armor = nullptr;
        break;
    case DefensiveHorseLocation:
        defensive_horse = nullptr;
        break;
    case OffensiveHorseLocation:
        offensive_horse = nullptr;
        break;
    case TreasureLocation:
        treasure = nullptr;
        break;
    }
}

bool Player::hasEquip(const Card *card) const
{
    Q_ASSERT(card != nullptr);
    int weapon_id = -1;
    int armor_id = -1;
    int def_id = -1;
    int off_id = -1;
    int tr_id = -1;
    if (weapon != nullptr)
        weapon_id = weapon->effectiveID();
    if (armor != nullptr)
        armor_id = armor->effectiveID();
    if (defensive_horse != nullptr)
        def_id = defensive_horse->effectiveID();
    if (offensive_horse != nullptr)
        off_id = offensive_horse->effectiveID();
    if (treasure != nullptr)
        tr_id = treasure->effectiveID();
    QList<int> ids;
    if (card->isVirtualCard())
        ids << card->effectiveID();
    else
        ids << card->id();

    if (ids.isEmpty())
        return false;
    foreach (int id, ids) {
        if (id != weapon_id && id != armor_id && id != def_id && id != off_id && id != tr_id)
            return false;
    }
    return true;
}

bool Player::hasEquip() const
{
    return weapon != nullptr || armor != nullptr || defensive_horse != nullptr || offensive_horse != nullptr || treasure != nullptr;
}

const Card *Player::getWeapon() const
{
    return weapon;
}

const Card *Player::getArmor() const
{
    return armor;
}

const Card *Player::getDefensiveHorse() const
{
    return defensive_horse;
}

const Card *Player::getOffensiveHorse() const
{
    return offensive_horse;
}

const Card *Player::getTreasure() const
{
    return treasure;
}

QList<const Card *> Player::getEquips() const
{
    QList<const Card *> equips;
    if (weapon != nullptr)
        equips << weapon;
    if (armor != nullptr)
        equips << armor;
    if (defensive_horse != nullptr)
        equips << defensive_horse;
    if (offensive_horse != nullptr)
        equips << offensive_horse;
    if (treasure != nullptr)
        equips << treasure;

    return equips;
}

const Card *Player::getEquip(int index) const
{
    const Card *equip = nullptr;
    switch (index) {
    case 0:
        equip = weapon;
        break;
    case 1:
        equip = armor;
        break;
    case 2:
        equip = defensive_horse;
        break;
    case 3:
        equip = offensive_horse;
        break;
    case 4:
        equip = treasure;
        break;
    default:
        // TODO: Raise warning here.
        return nullptr;
    }
    return equip;
}

bool Player::hasWeapon(const QString &weapon_name, bool /*unused*/, bool ignore_preshow) const
{
    if (getMark(QStringLiteral("Equips_Nullified_to_Yourself")) > 0)
        return false;

    if (Sanguosha->treatAsEquipping(this, weapon_name, WeaponLocation) != nullptr)
        return true;

    if ((weapon == nullptr) || isBrokenEquip(weapon->effectiveID(), true))
        return false;

    if (weapon->faceName() == weapon_name || weapon->face()->isKindOf(weapon_name.toStdString().c_str()))
        return true;

    // TODO_Fs: Consider view-as weapon later
    const CardDescriptor &real_weapon = Sanguosha->getEngineCard(weapon->effectiveID());
    return real_weapon.face()->name() == weapon_name || real_weapon.face()->isKindOf(weapon_name.toStdString().c_str());
}

bool Player::hasArmor(const QString &armor_name, bool /*unused*/) const
{
    if (!tag[QStringLiteral("Qinggang")].toStringList().isEmpty() || getMark(QStringLiteral("Armor_Nullified")) > 0 || getMark(QStringLiteral("Equips_Nullified_to_Yourself")) > 0)
        return false;

    if (Sanguosha->treatAsEquipping(this, armor_name, ArmorLocation) != nullptr)
        return true;

    if ((armor == nullptr) || isBrokenEquip(armor->effectiveID(), true))
        return false;

    if (armor->faceName() == armor_name || armor->face()->isKindOf(armor_name.toStdString().c_str()))
        return true;

    const CardDescriptor &real_weapon = Sanguosha->getEngineCard(armor->effectiveID());
    return real_weapon.face()->name() == armor_name || real_weapon.face()->isKindOf(armor_name.toStdString().c_str());
}

bool Player::hasTreasure(const QString &treasure_name, bool /*unused*/) const
{
    if (getMark(QStringLiteral("Equips_Nullified_to_Yourself")) > 0)
        return false;

    if (Sanguosha->treatAsEquipping(this, treasure_name, TreasureLocation) != nullptr)
        return true;

    if ((treasure == nullptr) || isBrokenEquip(treasure->effectiveID(), true))
        return false;

    if (treasure->faceName() == treasure_name || treasure->face()->isKindOf(treasure_name.toStdString().c_str()))
        return true;

    const CardDescriptor &real_weapon = Sanguosha->getEngineCard(treasure->effectiveID());
    return real_weapon.face()->name() == treasure_name || real_weapon.face()->isKindOf(treasure_name.toStdString().c_str());
}

QList<const Card *> Player::getJudgingArea() const
{
    QList<const Card *> cards;
    foreach (int card_id, judging_area)
        cards.append(roomObject()->getCard(card_id));
    return cards;
}

QList<int> Player::getJudgingAreaID() const
{
    //for marshal
    return judging_area;
}

Phase Player::getPhase() const
{
    return phase;
}

void Player::setPhase(Phase phase)
{
    this->phase = phase;
}

bool Player::isInMainPhase() const
{
    return phase == PhaseStart || phase == PhaseJudge || phase == PhaseDraw || phase == PhasePlay || phase == PhaseDiscard || phase == PhaseFinish;
}

bool Player::faceUp() const
{
    return face_up;
}

void Player::setFaceUp(bool face_up)
{
    if (this->face_up != face_up)
        this->face_up = face_up;
}

int Player::getMaxCards(const QString &except) const
{
    int origin = Sanguosha->correctMaxCards(this, true, except);
    if (origin == 0)
        origin = qMax(getHp(), 0);
    int rule = 0;
    int total = 0;
    int extra = 0;
    if (Config.MaxHpScheme == 3 && (general2 != nullptr)) {
        total = general->getMaxHp() + general2->getMaxHp();
        if (total % 2 != 0 && getMark(QStringLiteral("AwakenLostMaxHp")) == 0)
            rule = 1;
    }
    extra += Sanguosha->correctMaxCards(this, false, except);

    return qMax(origin + rule + extra, 0);
}

QString Player::getKingdom() const
{
    if (kingdom.isEmpty() && (general != nullptr))
        return general->getKingdom();
    else
        return kingdom;
}

void Player::setKingdom(const QString &kingdom)
{
    if (this->kingdom != kingdom) {
        this->kingdom = kingdom;
        if (isHegemonyGameMode(ServerInfo.GameMode) && role == QStringLiteral("careerist"))
            return;
        if (isHegemonyGameMode(ServerInfo.GameMode)) {
            QStringList kingdoms = Sanguosha->getHegemonyKingdoms();
            if (!kingdoms.contains(kingdom))
                return;
        }
    }
}

bool Player::isKongcheng() const
{
    return getHandcardNum() == 0;
}

bool Player::isNude() const
{
    return isKongcheng() && !hasEquip();
}

bool Player::isAllNude() const
{
    return isNude() && judging_area.isEmpty();
}

bool Player::canDiscard(const Player *to, const QString &flags, const QString &reason) const
{
    static QLatin1Char equip_flag('e');
    static QLatin1Char judging_flag('j');

    if (flags.contains(QStringLiteral("s")) && flags.contains(QStringLiteral("h"))) {
        if (!to->isKongcheng())
            return true;
    } else if (flags.contains(QStringLiteral("s"))) {
        if (!to->getShownHandcards().isEmpty())
            return true;
    } else if (flags.contains(QStringLiteral("h"))) {
        if ((to->getHandcardNum() - to->getShownHandcards().size()) > 0)
            return true;
    }
    if (flags.contains(judging_flag) && !to->getJudgingArea().isEmpty())
        return true;
    if (flags.contains(equip_flag)) {
        QSet<QString> Equips;
        if (to->getWeapon() != nullptr) {
            Equips << QStringLiteral("weapon");
        }
        if (to->getArmor() != nullptr) {
            Equips << QStringLiteral("armor");
        }
        if (to->getDefensiveHorse() != nullptr) {
            Equips << QStringLiteral("dh");
        }
        if (to->getOffensiveHorse() != nullptr) {
            Equips << QStringLiteral("oh");
        }
        if (to->getTreasure() != nullptr) {
            Equips << QStringLiteral("treasure");
        }

        if (reason == QStringLiteral("sidou")) {
            if (Equips.contains(QStringLiteral("weapon")))
                Equips.remove(QStringLiteral("weapon"));
        }
        if (!Equips.isEmpty())
            return true;
    }
    return false;
}

bool Player::canDiscard(const Player *to, int card_id, const QString &reason) const
{
    if (reason == QStringLiteral("sidou")) {
        if ((to->getWeapon() != nullptr) && card_id == to->getWeapon()->effectiveID())
            return false;
    }

    if (this == to) {
        if (isJilei(roomObject()->getCard(card_id)))
            return false;
    }
    return true;
}

void Player::addDelayedTrick(const Card *trick)
{
    judging_area << trick->id();
}

void Player::removeDelayedTrick(const Card *trick)
{
    int index = judging_area.indexOf(trick->id());
    if (index >= 0)
        judging_area.removeAt(index);
}

bool Player::containsTrick(const QString &trick_name) const
{
    foreach (int trick_id, judging_area) {
        const Card *trick = roomObject()->getCard(trick_id);
        // TODO: Wait! I don't know how to distinguish between card->name() and card->faceName()()
        // Fs: Just use a unified name! Don't you feel it's difficult to distinguish 2 names now?
        if (trick->faceName() == trick_name)
            return true;
    }
    return false;
}

bool Player::isChained() const
{
    return chained;
}

bool Player::isDebuffStatus() const
{
    return chained || (!shown_handcards.isEmpty()) || (!broken_equips.isEmpty());
}

void Player::setChained(bool chained)
{
    if (this->chained != chained)
        this->chained = chained;
}

bool Player::isRemoved() const
{
    return removed;
}

void Player::setRemoved(bool removed)
{
    if (this->removed != removed)
        this->removed = removed;
}

void Player::addMark(const QString &mark, int add_num)
{
    int value = marks.value(mark, 0);
    value += add_num;
    setMark(mark, value);
}

void Player::removeMark(const QString &mark, int remove_num)
{
    int value = marks.value(mark, 0);
    value -= remove_num;
    value = qMax(0, value);
    setMark(mark, value);
}

void Player::setMark(const QString &mark, int value)
{
    if (marks[mark] != value)
        marks[mark] = value;
}

int Player::getMark(const QString &mark) const
{
    return marks.value(mark, 0);
}

QMap<QString, int> Player::getMarkMap() const
{
    return marks;
}

bool Player::canSlash(const Player *other, const Card *slash, bool distance_limit, int rangefix, const QList<const Player *> &others) const
{
    if (other == this || !other->isAlive())
        return false;

    // Slash *newslash = new Slash(NoSuit, 0);
    const Card *new_shash = roomObject()->cloneCard(QStringLiteral("Slash"));
    // newslash->deleteLater();
#define THIS_SLASH (slash == nullptr ? new_shash : slash)
    if (isProhibited(other, THIS_SLASH, others)) {
        roomObject()->cardDeleting(new_shash);
        return false;
    }

    if (distance_limit) {
        bool res = distanceTo(other, rangefix) <= getAttackRange() + Sanguosha->correctCardTarget(ModDistance, this, THIS_SLASH);
        roomObject()->cardDeleting(new_shash);
        return res;
    } else
        return true;
#undef THIS_SLASH
}

bool Player::canSlash(const Player *other, bool distance_limit, int rangefix, const QList<const Player *> &others) const
{
    return canSlash(other, nullptr, distance_limit, rangefix, others);
}

int Player::getCardCount(bool include_equip, bool include_judging) const
{
    int count = getHandcardNum();
    if (include_equip) {
        if (weapon != nullptr)
            count++;
        if (armor != nullptr)
            count++;
        if (defensive_horse != nullptr)
            count++;
        if (offensive_horse != nullptr)
            count++;
        if (treasure != nullptr)
            count++;
    }
    if (include_judging)
        count += judging_area.length();
    return count;
}

IDSet Player::getPile(const QString &pile_name) const
{
    if (pile_name == QStringLiteral("shown_card"))
        return getShownHandcards();
    return piles[pile_name];
}

QStringList Player::getPileNames() const
{
    QStringList names;
    foreach (QString pile_name, piles.keys())
        names.append(pile_name);
    return names;
}

QString Player::getPileName(int card_id) const
{
    foreach (QString pile_name, piles.keys()) {
        const IDSet pile = piles[pile_name];
        if (pile.contains(card_id))
            return pile_name;
    }

    return QString();
}

bool Player::pileOpen(const QString &pile_name, const QString &player) const
{
    return pile_open[pile_name].contains(player);
}

void Player::setPileOpen(const QString &pile_name, const QString &player)
{
    if (pile_open[pile_name].contains(player))
        return;
    pile_open[pile_name].append(player);
}

IDSet Player::getHandPile() const
{
    IDSet result;
    foreach (const QString &pile, getPileNames()) {
        if (pile.startsWith(QStringLiteral("&")) || (pile == QStringLiteral("wooden_ox") && hasTreasure(QStringLiteral("wooden_ox")))) {
            foreach (int id, getPile(pile))
                result << id;
        }
    }
    return result;
}

QStringList Player::getHandPileList(bool view_as_skill) const
{
    QStringList handlist;
    if (view_as_skill)
        handlist.append(QStringLiteral("hand"));
    foreach (const QString &pile, this->getPileNames()) {
        if (pile.startsWith(QStringLiteral("&")) || pile.startsWith(QStringLiteral("^")))
            handlist.append(pile);
        else if (pile == QStringLiteral("wooden_ox") && hasTreasure(QStringLiteral("wooden_ox")))
            handlist.append(pile);
    }
    return handlist;
}

void Player::addHistory(const QString &name, int times)
{
    history[name] += times;
}

int Player::getSlashCount() const
{
    return history.value(QStringLiteral("Slash"), 0) + history.value(QStringLiteral("ThunderSlash"), 0) + history.value(QStringLiteral("FireSlash"), 0)
        + history.value(QStringLiteral("PowerSlash"), 0) + history.value(QStringLiteral("IronSlash"), 0) + history.value(QStringLiteral("LightSlash"), 0);
}

int Player::getAnalepticCount() const
{
    return history.value(QStringLiteral("Analeptic"), 0) + history.value(QStringLiteral("MagicAnaleptic"), 0);
}

void Player::clearHistory()
{
    history.clear();
}

bool Player::hasUsed(const QString &card_class) const
{
    return history.value(card_class, 0) > 0;
}

int Player::usedTimes(const QString &card_class) const
{
    return history.value(card_class, 0);
}

bool Player::hasEquipSkill(const QString &skill_name) const
{
    if (skill_name == QStringLiteral("shenbao")) // prevent infinite recursion for skill "shenbao"
        return false;

    if (hasSkill(QStringLiteral("shenbao"))) {
        foreach (const Player *p, getAliveSiblings()) {
            if (p->hasEquipSkill(skill_name))
                return true;
        }
    }

    if (weapon != nullptr) {
        const Weapon *weaponc = qobject_cast<const Weapon *>(weapon->face());
        if ((Sanguosha->getSkill(weaponc) != nullptr) && Sanguosha->getSkill(weaponc)->objectName() == skill_name)
            return true;
    }
    if (armor != nullptr) {
        const Armor *armorc = qobject_cast<const Armor *>(armor->face());
        if ((Sanguosha->getSkill(armorc) != nullptr) && Sanguosha->getSkill(armorc)->objectName() == skill_name)
            return true;
    }
    if (treasure != nullptr) {
        const Treasure *treasurec = qobject_cast<const Treasure *>(treasure->face());
        if ((Sanguosha->getSkill(treasurec) != nullptr) && Sanguosha->getSkill(treasurec)->objectName() == skill_name)
            return true;
    }
    return false;
}

QSet<const TriggerSkill *> Player::getTriggerSkills() const
{
    QSet<const TriggerSkill *> skillList;

    foreach (QString skill_name, skills.keys() + skills2.keys() + acquired_skills.values() + acquired_skills2.values()) {
        const TriggerSkill *skill = Sanguosha->getTriggerSkill(skill_name);
        if ((skill != nullptr) && !hasEquipSkill(skill->objectName()))
            skillList << skill;
    }

    return skillList;
}

QSet<const Skill *> Player::getSkills(bool include_equip, bool visible_only) const
{
    QList<const Skill *> list = getSkillList(include_equip, visible_only);
    return QSet<const Skill *>(list.begin(), list.end());
}

QList<const Skill *> Player::getSkillList(bool include_equip, bool visible_only) const
{
    QList<const Skill *> skillList;

    foreach (QString skill_name, skills.keys() + skills2.keys() + acquired_skills.values() + acquired_skills2.values()) {
        //foreach (QString skill_name, skills_originalOrder + skills2_originalOrder + acquired_skills.toList() + acquired_skills2.toList()) {
        const Skill *skill = Sanguosha->getSkill(skill_name);
        if ((skill != nullptr) && (include_equip || !hasEquipSkill(skill->objectName())) && (!visible_only || skill->isVisible()))
            skillList << skill;
    }

    return skillList;
}

QSet<const Skill *> Player::getVisibleSkills(bool include_equip) const
{
    QList<const Skill *> list = getVisibleSkillList(include_equip);
    return QSet<const Skill *>(list.begin(), list.end());
}

QList<const Skill *> Player::getVisibleSkillList(bool include_equip) const
{
    return getSkillList(include_equip, true);
}

QSet<QString> Player::getAcquiredSkills() const
{
    return acquired_skills + acquired_skills2;
}

QString Player::getSkillDescription(bool yellow, const QString &flag) const
{
    QString description = QString();
    QString color = yellow ? QStringLiteral("#FFFF33") : QStringLiteral("#FF0080");
    QList<const Skill *> skillList = getVisibleSkillList();
    if (flag == QStringLiteral("head"))
        skillList = getHeadSkillList(true, true);
    else if (flag == QStringLiteral("deputy"))
        skillList = getDeputySkillList(true, true);

    foreach (const Skill *skill, skillList) {
        if (skill->isAttachedSkill())
            continue;
        if (!isHegemonyGameMode(ServerInfo.GameMode) && !hasSkill(skill->objectName()))
            continue;

        //remove lord skill Description
        if (skill->isLordSkill() && !hasLordSkill(skill->objectName()))
            continue;

        QString skill_name = Sanguosha->translate(skill->objectName());
        QString desc = skill->getDescription();
        desc.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
        description.append(QStringLiteral("<font color=%1><b>%2</b>:</font> %3 <br/> <br/>").arg(color).arg(skill_name).arg(desc));
    }

    if (description.isEmpty())
        description = tr("<font color=%1>No skills</font>").arg(color);
    return description;
}

bool Player::isProhibited(const Player *to, const Card *card, const QList<const Player *> &others) const
{
    return Sanguosha->isProhibited(this, to, card, others) != nullptr;
}

bool Player::canSlashWithoutCrossbow(const Card *slash) const
{
    const Card *newslash = roomObject()->cloneCard(QStringLiteral("Slash"));
#define THIS_SLASH (slash == NULL ? newslash : slash)
    int slash_count = getSlashCount();
    int valid_slash_count = 1;
    valid_slash_count += Sanguosha->correctCardTarget(ModResidue, this, THIS_SLASH);
    roomObject()->cardDeleting(newslash);
    return slash_count < valid_slash_count;
#undef THIS_SLASH
}

void Player::setCardLimitation(const QString &limit_list, const QString &pattern, const QString &reason, bool single_turn)
{
    QStringList limit_type = limit_list.split(QStringLiteral(","));
    QString _pattern = pattern;
    if (!pattern.endsWith(QStringLiteral("$1")) && !pattern.endsWith(QStringLiteral("$0"))) {
        QString symb = single_turn ? QStringLiteral("$1") : QStringLiteral("$0");
        _pattern = _pattern + symb;
    }
    foreach (QString limit, limit_type) {
        HandlingMethod method = Sanguosha->getCardHandlingMethod(limit);
        card_limitation[method][reason] << _pattern;
    }
}

void Player::removeCardLimitation(const QString &limit_list, const QString &pattern, const QString &reason, bool clearReason)
{
    QStringList limit_type = limit_list.split(QStringLiteral(","));
    QString _pattern = pattern;
    if (!_pattern.endsWith(QStringLiteral("$1")) && !_pattern.endsWith(QStringLiteral("$0")))
        _pattern = _pattern + QStringLiteral("$0");
    foreach (QString limit, limit_type) {
        HandlingMethod method = Sanguosha->getCardHandlingMethod(limit);
        card_limitation[method][reason].removeOne(_pattern);
        if (card_limitation[method][reason].isEmpty() || _pattern.endsWith(QStringLiteral("$1")) || clearReason)
            card_limitation[method].remove(reason);
    }
}

void Player::clearCardLimitation(bool single_turn)
{
    QList<HandlingMethod> limit_type;
    limit_type << MethodUse << MethodResponse << MethodDiscard << MethodRecast << MethodPindian;
    foreach (HandlingMethod method, limit_type) {
        QMap<QString, QStringList> map = card_limitation[method];
        QMap<QString, QStringList>::iterator it;
        for (it = map.begin(); it != map.end(); ++it) {
            QString pattern = it.value().at(0);
            if (!single_turn || pattern.endsWith(QStringLiteral("$1"))) {
                card_limitation[method].remove(it.key());
            }
        }
    }
}

bool Player::isCardLimited(const Card *card, HandlingMethod method, bool isHandcard) const
{
    if (method == MethodNone)
        return false;
    if (card->face()->type() == TypeSkill && method == card->handleMethod()) {
        foreach (int card_id, card->subcards()) {
            const Card *c = roomObject()->getCard(card_id);
            QMap<QString, QStringList> map = card_limitation[method];
            QMap<QString, QStringList>::iterator it;
            for (it = map.begin(); it != map.end(); ++it) {
                QString pattern = it.value().at(0);
                QString _pattern = pattern.split(QStringLiteral("$")).first();
                if (isHandcard)
                    _pattern.replace(QStringLiteral("hand"), QStringLiteral("."));
                ExpPattern p(_pattern);
                if (p.match(this, c))
                    return true;
            }
        }
    } else {
        QMap<QString, QStringList> map = card_limitation[method];
        QMap<QString, QStringList>::iterator it;
        for (it = map.begin(); it != map.end(); ++it) {
            QString pattern = it.value().at(0);
            QString _pattern = pattern.split(QStringLiteral("$")).first();
            if (isHandcard)
                _pattern.replace(QStringLiteral("hand"), QStringLiteral("."));
            ExpPattern p(_pattern);
            if (p.match(this, card))
                return true;
        }
    }

    return false;
}

bool Player::isCardLimited(const QString &limit_list, const QString &reason) const
{
    QStringList limit_type = limit_list.split(QStringLiteral(","));
    foreach (QString limit, limit_type) {
        HandlingMethod method = Sanguosha->getCardHandlingMethod(limit);
        if (card_limitation[method].contains(reason))
            return true;
    }
    return false;
}

void Player::addQinggangTag(const Card *card)
{
    QStringList qinggang = tag[QStringLiteral("Qinggang")].toStringList();
    qinggang.append(card->toString());
    tag[QStringLiteral("Qinggang")] = QVariant::fromValue(qinggang);
}

void Player::removeQinggangTag(const Card *card)
{
    QStringList qinggang = tag[QStringLiteral("Qinggang")].toStringList();
    if (!qinggang.isEmpty()) {
        qinggang.removeOne(card->toString());
        tag[QStringLiteral("Qinggang")] = qinggang;
    }
}

bool Player::hasShownSkill(const Skill *skill) const
{
    if (skill == nullptr)
        return false;

    if (acquired_skills.contains(skill->objectName()) || acquired_skills2.contains(skill->objectName())) // deputy
        return true;

    if (skill->inherits("ArmorSkill") || skill->inherits("WeaponSkill") || skill->inherits("TreasureSkill"))
        return true;

    if (skill->inherits("TriggerSkill")) {
        const TriggerSkill *tr_skill = qobject_cast<const TriggerSkill *>(skill);
        if ((tr_skill != nullptr) && tr_skill->isGlobal() && !(skills.contains(tr_skill->objectName()) || skills2.contains(tr_skill->objectName())))
            return true;
    }

    if (!skill->isVisible()) {
        const Skill *main_skill = Sanguosha->getMainSkill(skill->objectName());
        if (main_skill != nullptr)
            return hasShownSkill(main_skill);
        else
            return false;
    }

    if (general_showed && skills.contains(skill->objectName()))
        return true;
    else if (general2_showed && skills2.contains(skill->objectName()))
        return true;

    return false;
}

bool Player::hasShownSkill(const QString &skill_name) const
{
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (skill == nullptr)
        return false;

    return hasShownSkill(skill);
}

bool Player::hasShownSkills(const QString &skill_name) const
{
    foreach (const QString &skill, skill_name.split(QStringLiteral("|"))) {
        bool checkpoint = true;
        foreach (const QString &sk, skill.split(QStringLiteral("+"))) {
            if (!hasShownSkill(sk)) {
                checkpoint = false;
                break;
            }
        }
        if (checkpoint)
            return true;
    }
    return false;
}

void Player::setSkillPreshowed(const QString &skill, bool preshowed)
{
    if (skills.contains(skill))
        skills[skill] = preshowed;

    if (skills2.contains(skill))
        skills2[skill] = preshowed;
}

void Player::setSkillsPreshowed(const QString &flag, bool preshowed)
{
    if (flag.contains(QStringLiteral("h"))) {
        foreach (const QString &skill, skills.keys()) {
            if (!Sanguosha->getSkill(skill)->canPreshow())
                continue;
            skills[skill] = preshowed;
        }
    }

    if (flag.contains(QStringLiteral("d"))) {
        foreach (const QString &skill, skills2.keys()) {
            if (!Sanguosha->getSkill(skill)->canPreshow())
                continue;
            skills2[skill] = preshowed;
        }
    }
}

bool Player::hasPreshowedSkill(const QString &name) const
{
    return skills.value(name, false) || skills2.value(name, false);
}

bool Player::hasPreshowedSkill(const Skill *skill) const
{
    return hasPreshowedSkill(skill->objectName());
}

bool Player::isHidden(bool head_general) const
{
    if (head_general ? general_showed : general2_showed)
        return false;
    const QList<const Skill *> skills = head_general ? getHeadSkillList() : getDeputySkillList();
    int count = 0;
    foreach (const Skill *skill, skills) {
        if (skill->canPreshow() && hasPreshowedSkill(skill->objectName()))
            return false;
        else if (!skill->canPreshow())
            ++count;
    }
    return count != skills.length();
}

bool Player::hasShownGeneral() const
{
    return general_showed;
}

bool Player::hasShownGeneral2() const
{
    return general2_showed;
}

bool Player::hasShownOneGeneral() const
{
    return general_showed || ((general2 != nullptr) && general2_showed);
}

bool Player::hasShownAllGenerals() const
{
    return general_showed && ((general2 == nullptr) || general2_showed);
}

void Player::setGeneralShowed(bool showed)
{
    this->general_showed = showed;
}

void Player::setGeneral2Showed(bool showed)
{
    this->general2_showed = showed;
}

bool Player::ownSkill(const QString &skill_name) const
{
    return skills.contains(skill_name) || skills2.contains(skill_name);
}

bool Player::ownSkill(const Skill *skill) const
{
    return ownSkill(skill->objectName());
}

bool Player::isFriendWith(const Player *player, bool considerAnjiang) const
{
    Q_ASSERT(player);
    if (player == nullptr || !isHegemonyGameMode(ServerInfo.GameMode))
        return false;

    if (considerAnjiang) {
        if (!player->hasShownOneGeneral() && this != player)
            return false;
    } else {
        if (!hasShownOneGeneral() || !player->hasShownOneGeneral())
            return false;
    }

    if (this == player)
        return true;

    if (role == QStringLiteral("careerist") || player->role == QStringLiteral("careerist"))
        return false;

    return role == player->role;
}

bool Player::willBeFriendWith(const Player *player) const
{
    if (this == player)
        return true;
    if (isFriendWith(player))
        return true;
    if (player == nullptr)
        return false;
    if (!player->hasShownOneGeneral())
        return false;

    if (!hasShownGeneral()) {
        QString role = getRoleString();
        int i = 1;
        foreach (const Player *p, getSiblings()) {
            if (p->getRoleString() == role) {
                if (p->hasShownGeneral() && p->getRoleString() != QStringLiteral("careerist"))
                    ++i;
            }
        }
        if (i > (parent()->findChildren<const Player *>().length() / 2))
            return false;
        else if (role == player->getRoleString())
            return true;
    }
    return false;
}

const Player *Player::getLord(bool include_death) const
{
    return nullptr;

    QList<const Player *> sib = include_death ? getSiblings() : getAliveSiblings();
    sib << this;
    foreach (const Player *p, sib) {
        if ((p->getGeneral() != nullptr) && p->getGeneral()->isLord() && p->getKingdom() == kingdom)
            return p;
    }

    return nullptr;
}

RoomObject *Player::roomObject() const
{
    return d->room;
}

QList<const Skill *> Player::getHeadSkillList(bool visible_only, bool include_acquired, bool include_equip) const
{
    QList<const Skill *> skillList;
    QStringList skillslist;
    if (include_acquired)
        skillslist = skills.keys() + acquired_skills.values();
    else
        skillslist = skills.keys();
    foreach (const QString &skill_name, skillslist) {
        const Skill *skill = Sanguosha->getSkill(skill_name);
        if (skill != nullptr) {
            if ((include_equip || !hasEquipSkill(skill->objectName())) && (!visible_only || skill->isVisible()))
                skillList << skill;
            if (skill->isVisible() && !visible_only) {
                QList<const Skill *> related_skill = Sanguosha->getRelatedSkills(skill->objectName());
                foreach (const Skill *s, related_skill) {
                    if (!skillList.contains(s) && !s->isVisible())
                        skillList << s;
                }
            }
        }
    }
    return skillList;
}

QList<const Skill *> Player::getDeputySkillList(bool visible_only, bool include_acquired, bool include_equip) const
{
    QList<const Skill *> skillList;
    QStringList skillslist;
    if (include_acquired)
        skillslist = skills2.keys() + acquired_skills2.values();
    else
        skillslist = skills2.keys();
    foreach (const QString &skill_name, skillslist) {
        const Skill *skill = Sanguosha->getSkill(skill_name);
        if (skill != nullptr) {
            if ((include_equip || !hasEquipSkill(skill->objectName())) && (!visible_only || skill->isVisible()))
                skillList << skill;
            if (skill->isVisible() && !visible_only) {
                QList<const Skill *> related_skill = Sanguosha->getRelatedSkills(skill->objectName());
                foreach (const Skill *s, related_skill) {
                    if (!skillList.contains(s) && !s->isVisible())
                        skillList << s;
                }
            }
        }
    }
    return skillList;
}

bool Player::inHeadSkills(const QString &skill_name) const
{
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (skill == nullptr)
        return false;

    if (!skill->isVisible()) { //really confused about invisible skills! by weidouncle
        const Skill *main_skill = Sanguosha->getMainSkill(skill_name);
        if (main_skill != nullptr)
            return inHeadSkills(main_skill->objectName());
    }
    return skills.contains(skill_name) || acquired_skills.contains(skill_name);
}

bool Player::inDeputySkills(const QString &skill_name) const
{
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (skill == nullptr)
        return false;

    if (!skill->isVisible()) {
        const Skill *main_skill = Sanguosha->getMainSkill(skill_name);
        if (main_skill != nullptr)
            return inDeputySkills(main_skill->objectName());
    }
    return skills2.contains(skill_name) || acquired_skills2.contains(skill_name);
}

void Player::setDisableShow(const QString &flags, const QString &reason)
{
    if (flags.contains(QLatin1Char('h'))) {
        if (disableShow(true).contains(reason))
            return;
    }
    if (flags.contains(QLatin1Char('d'))) {
        if (disableShow(false).contains(reason))
            return;
    }

    QString dis_str = flags + QLatin1Char(',') + reason;
    disable_show << dis_str;
}

void Player::removeDisableShow(const QString &reason)
{
    QStringList remove_list;
    foreach (const QString &dis_str, disable_show) {
        QString dis_reason = dis_str.split(QLatin1Char(',')).at(1);
        if (dis_reason == reason)
            remove_list << dis_str;
    }

    if (remove_list.isEmpty())
        return;

    foreach (const QString &to_remove, remove_list)
        disable_show.removeOne(to_remove);
}

QStringList Player::disableShow(bool head) const
{
    QLatin1Char head_flag('h');
    if (!head)
        head_flag = QLatin1Char('d');

    QStringList r;
    foreach (const QString &dis_str, disable_show) {
        QStringList dis_list = dis_str.split(QLatin1Char(','));
        if (dis_list.at(0).contains(head_flag))
            r << dis_list.at(1);
    }

    return r;
}

bool Player::canShowGeneral(const QString &flags) const
{
    bool head = true;
    bool deputy = true;
    foreach (const QString &dis_str, disable_show) {
        QStringList dis_list = dis_str.split(QLatin1Char(','));
        if (dis_list.at(0).contains(QStringLiteral("h")))
            head = false;
        if (dis_list.at(0).contains(QStringLiteral("d")))
            deputy = false;
    }
    if (flags.isEmpty())
        return head || deputy || hasShownOneGeneral();
    if (flags == QStringLiteral("h"))
        return head || hasShownGeneral();
    if (flags == QStringLiteral("d"))
        return deputy || hasShownGeneral2();
    if (flags == QStringLiteral("hd"))
        return (deputy || hasShownGeneral2()) && (head || hasShownGeneral());
    return false;
}

QList<const Player *> Player::getFormation() const
{
    QList<const Player *> teammates;
    teammates << this;
    int n = aliveCount(false);
    int num = n;
    for (int i = 1; i < n; ++i) {
        Player *target = getNextAlive(i);
        if (isFriendWith(target))
            teammates << target;
        else {
            num = i;
            break;
        }
    }

    n -= num;
    for (int i = 1; i < n; ++i) {
        Player *target = getLastAlive(i);
        if (isFriendWith(target))
            teammates << target;
        else
            break;
    }

    return teammates;
}

bool Player::canTransform(bool head) const
{
    if (head)
        return !getGeneralName().contains(QStringLiteral("sujiang"));
    else
        return (getGeneral2() != nullptr) && !getGeneral2Name().contains(QStringLiteral("sujiang"));
}
