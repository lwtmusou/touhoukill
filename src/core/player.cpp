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
    QList<bool> generalShown; // for Hegemony
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
    QMap<QString, int> disableShow;

    // original mark is only visible if it starts with '@'. The refactor proposal is to make mark always visible
    QMap<QString, int> marks;
    QSet<QString> flags;
    QHash<QString, int> history;
    QMap<QString, IDSet> piles;
    QSet<QString> acquiredSkills; // Acquired skills isn't split into parts by game rule
    QList<QMap<QString, bool>> skills; // General card skill
    QStringList invalidSkills;
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
    if (d->hp != hp) {
        d->hp = hp;
    }
    if (hasSkill(QStringLiteral("banling"))) {
        if (d->renhp != hp) {
            d->renhp = hp;
        }
        if (d->linghp != hp) {
            d->linghp = hp;
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
    if (!d->generals.isEmpty())
        return d->generals.first();

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

        // can't use "foreach (const QMap<QString, bool> &skillMap, d->skills)" here since the type contains comma
        foreach (const auto &skillMap, d->skills) {
            if (skillMap.contains(skill_name))
                return skillMap.value(skill_name);
        }

        return d->acquiredSkills.contains(skill_name);

        // return skills.value(skill_name, false) || skills2.value(skill_name, false) || acquired_skills.contains(skill_name) || acquired_skills2.contains(skill_name);
    }

    //Other modes
    //For skill "yibian" of reimu_god
    if (getMark(QStringLiteral("@disableShowRole")) > 0 && !hasShownRole()) {
        if (!skill->isEternal() && !skill->isAttachedSkill() && !hasEquipSkill(skill_name))
            return false;
    }

#if 0
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
#endif
    if (!include_lose && !hasEquipSkill(skill_name) && !skill->isEternal()) {
        if (isSkillInvalid(skill_name))
            return false;
    }

    foreach (const auto &skillMap, d->skills) {
        if (skillMap.contains(skill_name))
            return true;
    }

    return d->acquiredSkills.contains(skill_name);
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
    foreach (const General *g, d->generals) {
        if (g->hasSkill(skill_name))
            return true;
    }
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

    if (d->acquiredSkills.contains(skill_name))
        return true;

    QString mode = ServerInfo.GameMode;
    if (mode == QStringLiteral("06_3v3") || mode == QStringLiteral("06_XMode") || mode == QStringLiteral("02_1v1")
        || Config.value(QStringLiteral("WithoutLordskill"), false).toBool())
        return false;

    if (isLord()) {
        foreach (const auto skillMap, d->skills) {
            if (skillMap.contains(skill_name))
                return true;
        }
    }

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
    if (invalidity && !d->invalidSkills.contains(skill_name))
        d->invalidSkills << skill_name;
    else if (!invalidity && d->invalidSkills.contains(skill_name))
        d->invalidSkills.removeAll(skill_name);
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

    if (d->invalidSkills.contains(QStringLiteral("_ALL_SKILLS")))
        return true;

    return d->invalidSkills.contains(skill_name);
}

QStringList Player::invalidedSkills() const
{
    return d->invalidSkills;
}

void Player::acquireSkill(const QString &skill_name)
{
    d->acquiredSkills.insert(skill_name);
}

void Player::detachSkill(const QString &skill_name)
{
    d->acquiredSkills.remove(skill_name);
}

void Player::detachAllSkills()
{
    d->acquiredSkills.clear();
}

void Player::addSkill(const QString &skill_name, int place)
{
    if (place >= d->skills.length())
        return;

    const Skill *skill = Sanguosha->getSkill(skill_name);
    Q_ASSERT(skill);

    d->skills[place][skill_name] = !skill->canPreshow() || d->generalShown[place];
}

void Player::loseSkill(const QString &skill_name, int place)
{
    if (place >= d->skills.length())
        return;

    d->skills[place].remove(skill_name);
}
#if 0
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
#endif

QString Player::getPhaseString() const
{
    switch (d->phase) {
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
        d->weapon = equip->id();
        break;
    case ArmorLocation:
        d->armor = equip->id();
        break;
    case DefensiveHorseLocation:
        d->defensiveHorse = equip->id();
        break;
    case OffensiveHorseLocation:
        d->offensiveHorse = equip->id();
        break;
    case TreasureLocation:
        d->treasure = equip->id();
        break;
    }
}

void Player::removeEquip(const Card *equip)
{
    const EquipCard *face = qobject_cast<const EquipCard *>(equip->face());
    Q_ASSERT(face != nullptr);
    switch (face->location()) {
    case WeaponLocation:
        d->weapon = -1;
        break;
    case ArmorLocation:
        d->armor = -1;
        break;
    case DefensiveHorseLocation:
        d->defensiveHorse = -1;
        break;
    case OffensiveHorseLocation:
        d->offensiveHorse = -1;
        break;
    case TreasureLocation:
        d->treasure = -1;
        break;
    }
}

bool Player::hasEquip(const Card *card) const
{
    Q_ASSERT(card != nullptr);
    QList<int> ids;
    if (card->isVirtualCard())
        ids << card->effectiveID();
    else
        ids << card->id();

    if (ids.isEmpty())
        return false;
    foreach (int id, ids) {
        if (id != d->weapon && id != d->armor && id != d->defensiveHorse && id != d->offensiveHorse && id != d->treasure)
            return false;
    }
    return true;
}

bool Player::hasEquip() const
{
    return d->weapon != -1 || d->armor != -1 || d->defensiveHorse != -1 || d->offensiveHorse != -1 || d->treasure != -1;
}

const Card *Player::getWeapon() const
{
    if (d->weapon != -1)
        return d->room->getCard(d->weapon);
    return nullptr;
}

const Card *Player::getArmor() const
{
    if (d->armor != -1)
        return d->room->getCard(d->armor);
    return nullptr;
}

const Card *Player::getDefensiveHorse() const
{
    if (d->defensiveHorse != -1)
        return d->room->getCard(d->defensiveHorse);
    return nullptr;
}

const Card *Player::getOffensiveHorse() const
{
    if (d->offensiveHorse != -1)
        return d->room->getCard(d->offensiveHorse);
    return nullptr;
}

const Card *Player::getTreasure() const
{
    if (d->treasure != -1)
        return d->room->getCard(d->treasure);
    return nullptr;
}

QList<const Card *> Player::getEquips() const
{
    QList<const Card *> equips {getWeapon(), getArmor(), getDefensiveHorse(), getOffensiveHorse(), getTreasure()};
    equips.removeAll(nullptr);
    return equips;
}

const Card *Player::getEquip(int index) const
{
    const Card *equip = nullptr;
    switch (index) {
    case 0:
        equip = getWeapon();
        break;
    case 1:
        equip = getArmor();
        break;
    case 2:
        equip = getDefensiveHorse();
        break;
    case 3:
        equip = getOffensiveHorse();
        break;
    case 4:
        equip = getTreasure();
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

    if ((d->weapon == -1) || isBrokenEquip(d->weapon, true))
        return false;

    if (d->room->getCard(d->weapon)->faceName() == weapon_name || d->room->getCard(d->weapon)->face()->isKindOf(weapon_name.toStdString().c_str()))
        return true;

    // TODO_Fs: Consider view-as equip later
    const CardDescriptor &real_weapon = Sanguosha->getEngineCard(d->weapon);
    return real_weapon.face()->name() == weapon_name || real_weapon.face()->isKindOf(weapon_name.toStdString().c_str());
}

bool Player::hasArmor(const QString &armor_name, bool /*unused*/) const
{
    if (!tag[QStringLiteral("Qinggang")].toStringList().isEmpty() || getMark(QStringLiteral("Armor_Nullified")) > 0 || getMark(QStringLiteral("Equips_Nullified_to_Yourself")) > 0)
        return false;

    if (Sanguosha->treatAsEquipping(this, armor_name, ArmorLocation) != nullptr)
        return true;

    if ((d->armor == -1) || isBrokenEquip(d->armor, true))
        return false;

    if (d->room->getCard(d->armor)->faceName() == armor_name || d->room->getCard(d->armor)->face()->isKindOf(armor_name.toStdString().c_str()))
        return true;

    // TODO_Fs: Consider view-as equip later
    const CardDescriptor &real_weapon = Sanguosha->getEngineCard(d->armor);
    return real_weapon.face()->name() == armor_name || real_weapon.face()->isKindOf(armor_name.toStdString().c_str());
}

bool Player::hasTreasure(const QString &treasure_name, bool /*unused*/) const
{
    if (getMark(QStringLiteral("Equips_Nullified_to_Yourself")) > 0)
        return false;

    if (Sanguosha->treatAsEquipping(this, treasure_name, TreasureLocation) != nullptr)
        return true;

    if ((d->treasure == -1) || isBrokenEquip(d->treasure, true))
        return false;

    if (d->room->getCard(d->treasure)->faceName() == treasure_name || d->room->getCard(d->treasure)->face()->isKindOf(treasure_name.toStdString().c_str()))
        return true;

    // TODO_Fs: Consider view-as equip later
    const CardDescriptor &real_weapon = Sanguosha->getEngineCard(d->treasure);
    return real_weapon.face()->name() == treasure_name || real_weapon.face()->isKindOf(treasure_name.toStdString().c_str());
}

QList<const Card *> Player::getJudgingArea() const
{
    QList<const Card *> cards;

    foreach (int card_id, d->judgingArea)
        cards.append(roomObject()->getCard(card_id));
    return cards;
}

QList<int> Player::getJudgingAreaID() const
{
    return d->judgingArea;
}

Phase Player::getPhase() const
{
    return d->phase;
}

void Player::setPhase(Phase phase)
{
    d->phase = phase;
}

bool Player::isInMainPhase() const
{
    return d->phase == PhaseStart || d->phase == PhaseJudge || d->phase == PhaseDraw || d->phase == PhasePlay || d->phase == PhaseDiscard || d->phase == PhaseFinish;
}

bool Player::turnSkipping() const
{
    return d->turnSkipping;
}

void Player::setTurnSkipping(bool turnSkipping)
{
    d->turnSkipping = turnSkipping;
}

int Player::getMaxCards(const QString &except) const
{
    int origin = Sanguosha->correctMaxCards(this, true, except);
    if (origin == 0)
        origin = qMax(getHp(), 0);
    int rule = 0;
    // int total = 0;
    int extra = 0;
#if 0
    // todo: refactor MaxHpScheme for Multi General
    if (Config.MaxHpScheme == 3 && (general2 != nullptr)) {
        total = general->getMaxHp() + general2->getMaxHp();
        if (total % 2 != 0 && getMark(QStringLiteral("AwakenLostMaxHp")) == 0)
            rule = 1;
    }
#endif
    extra += Sanguosha->correctMaxCards(this, false, except);

    return qMax(origin + rule + extra, 0);
}

QString Player::getKingdom() const
{
    if (d->kingdom.isEmpty() && (!d->generals.isEmpty()))
        return d->generals.first()->getKingdom();
    else
        return d->kingdom;
}

void Player::setKingdom(const QString &kingdom)
{
    d->kingdom = kingdom;
#if 0
    // todo: refactor hegemony
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
#endif
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
    return isNude() && d->judgingArea.isEmpty();
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
        if (isCardLimited(roomObject()->getCard(card_id), MethodDiscard))
            return false;
    }
    return true;
}

void Player::addDelayedTrick(const Card *trick)
{
    d->judgingArea << trick->id();
}

void Player::removeDelayedTrick(const Card *trick)
{
    int index = d->judgingArea.indexOf(trick->id());
    if (index >= 0)
        d->judgingArea.removeAt(index);
}

bool Player::containsTrick(const QString &trick_name) const
{
    foreach (int trick_id, d->judgingArea) {
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
    return d->chained;
}

bool Player::isDebuffStatus() const
{
    return d->chained || (!d->shownHandcards.isEmpty()) || (!d->brokenEquips.isEmpty());
}

void Player::setChained(bool chained)
{
    if (d->chained != chained)
        d->chained = chained;
}

bool Player::isRemoved() const
{
    return d->removed;
}

void Player::setRemoved(bool removed)
{
    if (d->removed != removed)
        d->removed = removed;
}

void Player::addMark(const QString &mark, int add_num)
{
    int value = d->marks.value(mark, 0);
    value += add_num;
    setMark(mark, value);
}

void Player::removeMark(const QString &mark, int remove_num)
{
    int value = d->marks.value(mark, 0);
    value -= remove_num;
    value = qMax(0, value);
    setMark(mark, value);
}

void Player::setMark(const QString &mark, int value)
{
    if (d->marks[mark] != value)
        d->marks[mark] = value;
}

int Player::getMark(const QString &mark) const
{
    return d->marks.value(mark, 0);
}

QMap<QString, int> Player::getMarkMap() const
{
    return d->marks;
}

bool Player::canSlash(const Player *other, const Card *slash, bool distance_limit, int rangefix, const QList<const Player *> &others) const
{
    if (other == this || !other->isAlive())
        return false;

    const Card *new_shash = roomObject()->cloneCard(QStringLiteral("Slash"));
#define THIS_SLASH (slash == nullptr ? new_shash : slash)
    if (isProhibited(other, THIS_SLASH, others)) {
        roomObject()->cardDeleting(new_shash);
        return false;
    }

    if (distance_limit) {
        bool res = distanceTo(other, rangefix) <= getAttackRange() + Sanguosha->correctCardTarget(ModDistance, this, THIS_SLASH);
        roomObject()->cardDeleting(new_shash);
        return res;
    } else {
        roomObject()->cardDeleting(new_shash);
        return true;
    }
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
        if (d->weapon != -1)
            count++;
        if (d->armor != -1)
            count++;
        if (d->defensiveHorse != -1)
            count++;
        if (d->offensiveHorse != -1)
            count++;
        if (d->treasure != -1)
            count++;
    }
    if (include_judging)
        count += d->judgingArea.length();
    return count;
}

IDSet Player::getPile(const QString &pile_name) const
{
    if (pile_name == QStringLiteral("shown_card"))
        return getShownHandcards();
    return d->piles[pile_name];
}

QStringList Player::getPileNames() const
{
    QStringList names;
    foreach (QString pile_name, d->piles.keys())
        names.append(pile_name);
    return names;
}

QString Player::getPileName(int card_id) const
{
    foreach (QString pile_name, d->piles.keys()) {
        const IDSet pile = d->piles[pile_name];
        if (pile.contains(card_id))
            return pile_name;
    }

    return QString();
}

bool Player::pileOpen(const QString &, const QString &) const
{
    return false;
}

void Player::setPileOpen(const QString &, const QString &)
{
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
    d->history[name] += times;
}

int Player::getSlashCount() const
{
    return d->history.value(QStringLiteral("Slash"), 0) + d->history.value(QStringLiteral("ThunderSlash"), 0) + d->history.value(QStringLiteral("FireSlash"), 0)
        + d->history.value(QStringLiteral("PowerSlash"), 0) + d->history.value(QStringLiteral("IronSlash"), 0) + d->history.value(QStringLiteral("LightSlash"), 0);
}

int Player::getAnalepticCount() const
{
    return d->history.value(QStringLiteral("Analeptic"), 0) + d->history.value(QStringLiteral("MagicAnaleptic"), 0);
}

QHash<QString, int> Player::getHistories() const
{
    return d->history;
}

void Player::clearHistory()
{
    d->history.clear();
}

bool Player::hasUsed(const QString &card_class) const
{
    return d->history.value(card_class, 0) > 0;
}

int Player::usedTimes(const QString &card_class) const
{
    return d->history.value(card_class, 0);
}

bool Player::hasEquipSkill(const QString &skill_name) const
{
    if (skill_name == QStringLiteral("shenbao")) // prevent infinite recursion for skill "shenbao"
        return false;

    if (hasSkill(QStringLiteral("shenbao"))) {
        foreach (const Player *p, d->room->players(false, true)) {
            if (p == this)
                continue;
            if (p->hasEquipSkill(skill_name))
                return true;
        }
    }

    if (d->weapon != -1) {
        const Weapon *weaponc = qobject_cast<const Weapon *>(d->room->getCard(d->weapon)->face());
        if ((Sanguosha->getSkill(weaponc) != nullptr) && Sanguosha->getSkill(weaponc)->objectName() == skill_name)
            return true;
    }
    if (d->armor != -1) {
        const Armor *armorc = qobject_cast<const Armor *>(d->room->getCard(d->armor)->face());
        if ((Sanguosha->getSkill(armorc) != nullptr) && Sanguosha->getSkill(armorc)->objectName() == skill_name)
            return true;
    }
    if (d->treasure != -1) {
        const Treasure *treasurec = qobject_cast<const Treasure *>(d->room->getCard(d->treasure)->face());
        if ((Sanguosha->getSkill(treasurec) != nullptr) && Sanguosha->getSkill(treasurec)->objectName() == skill_name)
            return true;
    }
    return false;
}

QSet<const TriggerSkill *> Player::getTriggerSkills() const
{
    QSet<const TriggerSkill *> skillList;
    QSet<QString> skills;
    foreach (const auto &x, d->skills)
        skills.unite(List2Set(x.keys()));

    skills.unite(d->acquiredSkills);

    foreach (const QString &skill_name, skills) {
        const TriggerSkill *skill = Sanguosha->getTriggerSkill(skill_name);
        if ((skill != nullptr) && !hasEquipSkill(skill->objectName()))
            skillList << skill;
    }

    return skillList;
}

QSet<const Skill *> Player::getSkills(bool include_equip, bool visible_only, bool include_acquired, const QList<int> &positions) const
{
    QList<const Skill *> list = getSkillList(include_equip, visible_only, include_acquired, positions);
    return QSet<const Skill *>(list.begin(), list.end());
}

QList<const Skill *> Player::getSkillList(bool include_equip, bool visible_only, bool include_acquired, const QList<int> &positions) const
{
    QList<const Skill *> skillList;
    QSet<QString> skills;

    for (int i = 0; i < d->skills.length(); ++i) {
        if (positions.isEmpty() || positions.contains(i))
            skills.unite(List2Set(d->skills.value(i).keys()));
    }

    foreach (const auto &x, d->skills)
        skills.unite(List2Set(x.keys()));

    if (include_acquired)
        skills.unite(d->acquiredSkills);

    foreach (const QString &skill_name, skills) {
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
    return d->acquiredSkills;
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
        d->cardLimitation[method][reason] << _pattern;
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
        d->cardLimitation[method][reason].removeOne(_pattern);
        if (d->cardLimitation[method][reason].isEmpty() || _pattern.endsWith(QStringLiteral("$1")) || clearReason)
            d->cardLimitation[method].remove(reason);
    }
}

void Player::clearCardLimitation(bool single_turn)
{
    QList<HandlingMethod> limit_type;
    limit_type << MethodUse << MethodResponse << MethodDiscard << MethodRecast << MethodPindian;
    foreach (HandlingMethod method, limit_type) {
        QMap<QString, QStringList> map = d->cardLimitation[method];
        QMap<QString, QStringList>::iterator it;
        for (it = map.begin(); it != map.end(); ++it) {
            QString pattern = it.value().at(0);
            if (!single_turn || pattern.endsWith(QStringLiteral("$1"))) {
                d->cardLimitation[method].remove(it.key());
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
            QMap<QString, QStringList> map = d->cardLimitation[method];
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
        QMap<QString, QStringList> map = d->cardLimitation[method];
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
        if (d->cardLimitation[method].contains(reason))
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

    if (d->acquiredSkills.contains(skill->objectName())) // deputy
        return true;

    if (skill->inherits("ArmorSkill") || skill->inherits("WeaponSkill") || skill->inherits("TreasureSkill"))
        return true;

    if (skill->inherits("TriggerSkill")) {
        const TriggerSkill *tr_skill = qobject_cast<const TriggerSkill *>(skill);
        if ((tr_skill != nullptr) && tr_skill->isGlobal()) {
            bool flag = false;
            foreach (auto x, d->skills) {
                if (x.contains(tr_skill->objectName())) {
                    flag = true;
                    break;
                }
            }
            if (!flag)
                return true;
        }
    }

    if (!skill->isVisible()) {
        const Skill *main_skill = Sanguosha->getMainSkill(skill->objectName());
        if (main_skill != nullptr)
            return hasShownSkill(main_skill);
        else
            return false;
    }

    for (int i = 0; i < d->skills.length(); ++i) {
        if (d->generalShown.value(i, false) && d->skills.value(i, {}).contains(skill->objectName()))
            return true;
    }

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
    for (auto it = d->skills.begin(); it != d->skills.end(); ++it) {
        if (it->contains(skill))
            (*it)[skill] = preshowed;
    }
}

void Player::setSkillsPreshowed(const QList<int> &positions, bool preshowed)
{
    foreach (int pos, positions) {
        auto &skills = d->skills[pos];
        foreach (const QString &skill, skills.keys()) {
            if (!Sanguosha->getSkill(skill)->canPreshow())
                continue;
            skills[skill] = preshowed;
        }
    }
}

bool Player::hasPreshowedSkill(const QString &name) const
{
    foreach (const auto &x, d->skills) {
        if (x.value(name, false))
            return true;
    }

    return false;
}

bool Player::hasPreshowedSkill(const Skill *skill) const
{
    return hasPreshowedSkill(skill->objectName());
}

bool Player::isHidden(int pos) const
{
    const auto &skills = d->skills[pos];
    int count = 0;
    foreach (const QString &skillName, skills.keys()) {
        const Skill *skill = Sanguosha->getSkill(skillName);
        if (skill->canPreshow() && hasPreshowedSkill(skill->objectName()))
            return false;
        else if (!skill->canPreshow())
            ++count;
    }
    return count != skills.keys().length();
}

bool Player::hasShownGeneral(int pos) const
{
    return d->generalShown.value(pos);
}

void Player::setShownGeneral(int pos, bool show)
{
    d->generalShown[pos] = show;
}

bool Player::hasShownOneGeneral() const
{
    return d->generalShown.count(true) == 1;
}

bool Player::hasShownAllGenerals() const
{
    return d->generalShown.count(false) == 0;
}

bool Player::ownSkill(const QString &skill_name) const
{
    foreach (const auto &x, d->skills) {
        if (x.contains(skill_name))
            return true;
    }

    return false;
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

    // TODO: refactor
    if (getKingdom() == QStringLiteral("careerist") || player->getKingdom() == QStringLiteral("careerist"))
        return false;

    return d->role == player->d->role;
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
        foreach (const Player *p, d->room->players()) {
            if (p == this)
                continue;
            if (p->getRoleString() == role) {
                if (p->hasShownGeneral() && p->getKingdom() != QStringLiteral("careerist"))
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

RoomObject *Player::roomObject() const
{
    return d->room;
}

int Player::findPositionOfGeneralOwningSkill(const QString &skill_name) const
{
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (skill == nullptr)
        return false;

    if (!skill->isVisible()) { //really confused about invisible skills! by weidouncle
        const Skill *main_skill = Sanguosha->getMainSkill(skill_name);
        if (main_skill != nullptr)
            return findPositionOfGeneralOwningSkill(main_skill->objectName());
    }

    for (int i = 0; i < d->skills.length(); ++i) {
        if (d->skills.value(i).contains(skill_name))
            return i;
    }

    return -1;
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
